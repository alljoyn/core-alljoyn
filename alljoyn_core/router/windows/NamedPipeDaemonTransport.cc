/**
 * @file
 * NamedPipeDaemonTransport (Daemon Transport for named pipe) implementation for Windows
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

/*
 * Compile this file only for Windows version 10 or greater.
 */
 #if (_WIN32_WINNT > 0x0603)

#include <qcc/platform.h>
#include <qcc/IPAddress.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Windows/NamedPipeStream.h>
#include <aclapi.h>
#include <sddl.h>
#include <MSAJTransport.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Session.h>
#include <alljoyn/AllJoynStd.h>

#include "BusInternal.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "NamedPipeDaemonTransport.h"
#include "unicode.h"
#include "ConfigDB.h"

#include <Netfw.h>

#define QCC_MODULE "DAEMON_TRANSPORT"

using namespace std;
using namespace qcc;

namespace ajn {

const char* NamedPipeDaemonTransport::NamedPipeTransportName = "npipe";

/*
 * An endpoint class to handle the details of authenticating a connection.
 */
class _NamedPipeDaemonEndpoint : public _RemoteEndpoint {
    friend class NamedPipeDaemonTransport;

  public:
    enum AuthState {
        AUTH_ILLEGAL = 0,
        AUTH_INITIALIZED = 1,    /**< Initialized but authentication has been scheduled yet */
        AUTH_AUTHENTICATING = 2, /**< Authentication is in progress */
        AUTH_FAILED = 3,         /**< Authentication and the authentication thread is exiting immidiately */
        AUTH_SUCCEEDED = 4,      /**< The auth process (Establish) has succeeded and the connection is ready to be started */
        AUTH_STOPPING = 5,       /**< Authentication is being stopped */
        AUTH_STOPPED = 6,        /**< Authentication has been stopped */
    };

    /**
     * Constructor.
     */
    _NamedPipeDaemonEndpoint(_In_ BusAttachment& bus, _In_ HANDLE pipeHandle, _In_ NamedPipeDaemonTransport* transport);

    /**
     * Destructor.
     */
    virtual ~_NamedPipeDaemonEndpoint();

    /**
     * Ask the endpoint to stop executing.
     *
     * @return
     *      - ER_OK if successful.
     *      - An error status otherwise.
     */
    virtual QStatus Stop();

    /**
     * Join the endpoint.
     * Block the caller until the endpoint is stopped.
     *
     * @return ER_OK if successful.
     */
    virtual QStatus Join();

  private:

    /**
     * Queue a thread pool work item that performs endpoint authentication.
     */
    QStatus AuthStart();

    /**
     * Attempt to cancel the authentication work item. When this method returns, the work item
     * either has never been processed, or its processing finished.
     */
    void AuthStop();

    /**
     * Thread pool worker that performs endpoint authentication.
     */
    static void AuthenticationWorker(_Inout_ PTP_CALLBACK_INSTANCE instance, _Inout_opt_ PVOID context, _Inout_ PTP_WORK work);

    /**
     * Atomically compare m_authState with oldState and, if they were equal, assign value newState to m_authState.
     *
     * @param oldState Previous state.
     * @param newState Next state.
     *
     * @return
     *      - true if successful.
     *      - false if current state was not oldState.
     */
    bool TryToChangeAuthState(_In_ AuthState oldState, _In_ AuthState newState);

    bool AuthSucceeded() { return (m_authState == AUTH_SUCCEEDED); }
    bool AuthFailed() { return (m_authState == AUTH_FAILED); }
    bool AuthStopped() { return (m_authState == AUTH_STOPPED); }

    /**
     * Address of the one and only named pipe transport.
     */
    NamedPipeDaemonTransport* m_transport;

    /**
     * Stream associated with this endpoint.
     */
    NamedPipeStream m_pipeStream;

    /**
     * Stream's associated pipe handle, using during authentication.
     */
    HANDLE m_pipeHandle;

    /**
     * Authentication state - one of the enum AuthState values.
     */
    uint32_t m_authState;

    /**
     * Thread pool work item used for authentication.
     */
    PTP_WORK m_threadPoolWork;
};

_NamedPipeDaemonEndpoint::_NamedPipeDaemonEndpoint(_In_ BusAttachment& bus, _In_ HANDLE pipeHandle, _In_ NamedPipeDaemonTransport* transport) :
    _RemoteEndpoint(bus, true, NamedPipeDaemonTransport::NamedPipeTransportName, &m_pipeStream, NamedPipeDaemonTransport::NamedPipeTransportName, false),
    m_transport(transport),
    m_pipeStream(pipeHandle),
    m_pipeHandle(pipeHandle),
    m_authState(AUTH_INITIALIZED),
    m_threadPoolWork(nullptr)
{
    QCC_DbgTrace(("_NamedPipeDaemonEndpoint()"));
    GetFeatures().isBusToBus = false;
    GetFeatures().allowRemote = false;
    GetFeatures().handlePassing = false;
}

_NamedPipeDaemonEndpoint::~_NamedPipeDaemonEndpoint()
{
    QCC_DbgTrace(("~_NamedPipeDaemonEndpoint: m_threadPoolWork = 0x%p", m_threadPoolWork));
    assert(m_threadPoolWork == nullptr);
}

QStatus _NamedPipeDaemonEndpoint::Stop()
{
    QCC_DbgTrace(("Stop"));
    AuthStop();
    return _RemoteEndpoint::Stop();
}

/*
 * Join() must be called with the endpointListLock held, to synchronize the access to m_threadPoolWork.
 */
QStatus _NamedPipeDaemonEndpoint::Join()
{
    QCC_DbgTrace(("_NamedPipeDaemonEndpoint::Join: work = 0x%p", m_threadPoolWork));
    if (m_threadPoolWork != nullptr) {
        /*
         * Wait for the worker if it's running already, cancel the work item it it's not running yet.
         */
        WaitForThreadpoolWorkCallbacks(m_threadPoolWork, true);
        CloseThreadpoolWork(m_threadPoolWork);
        m_threadPoolWork = nullptr;

        /*
         * Release the reference previously added by AuthStart().
         */
        NamedPipeDaemonEndpoint::wrap(this).DecRef();
    }

    return ER_OK;
}

QStatus _NamedPipeDaemonEndpoint::AuthStart()
{
    QCC_DbgTrace(("AuthStart"));
    QStatus status = ER_OK;
    assert(m_threadPoolWork == nullptr);

    if (!TryToChangeAuthState(AUTH_INITIALIZED, AUTH_AUTHENTICATING)) {
        assert(m_authState == AUTH_STOPPED);
        QCC_DbgHLPrintf(("AuthStart: already stopped"));
    } else {
        /*
         * Add a reference that will be released by Join(), after the worker finished execution.
         */
        NamedPipeDaemonEndpoint::wrap(this).IncRef();

        /*
         * Start authentication on a thread pool thread.
         */
        m_threadPoolWork = CreateThreadpoolWork(AuthenticationWorker, this, nullptr);

        if (m_threadPoolWork == nullptr) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("CreateThreadpoolWork failed with OS error %u", ::GetLastError()));
            NamedPipeDaemonEndpoint::wrap(this).DecRef();
        }

        if (status == ER_OK) {
            SubmitThreadpoolWork(m_threadPoolWork);
        } else {
            TryToChangeAuthState(AUTH_AUTHENTICATING, AUTH_FAILED);
        }
    }

    return status;
}

void _NamedPipeDaemonEndpoint::AuthStop()
{
    QCC_DbgTrace(("AuthStop: m_authState = %u, m_threadPoolWork = 0x%p", m_authState, m_threadPoolWork));
    if (TryToChangeAuthState(AUTH_INITIALIZED, AUTH_STOPPED)) {
        /*
         * Authentication hasn't started yet, so it's stopped already.
         */
        QCC_DbgHLPrintf(("AuthStop: SetEvent"));
        m_transport->m_authFinishedEvent.SetEvent();
    } else {
        /*
         * Ask the auth worker to stop as soon as possible.
         */
        TryToChangeAuthState(AUTH_AUTHENTICATING, AUTH_STOPPING);
    }
}

void _NamedPipeDaemonEndpoint::AuthenticationWorker(_Inout_ PTP_CALLBACK_INSTANCE instance, _Inout_opt_ PVOID context, _Inout_ PTP_WORK work)
{
    _NamedPipeDaemonEndpoint* conn = reinterpret_cast<_NamedPipeDaemonEndpoint*>(context);
    QCC_DbgHLPrintf(("Worker: reading NUL byte"));
    uint8_t byte;
    size_t nbytes;
    QStatus status = conn->m_pipeStream.PullBytes(&byte, 1, nbytes, conn->m_transport->GetAuthTimeout());

    if ((status != ER_OK) || (nbytes != 1) || (byte != 0)) {
        status = (status == ER_OK) ? ER_FAIL : status;
        QCC_LogError(status, ("Worker: failed to read NUL byte"));
    }

    /*
     * Check if AuthStop() changed the state to AUTH_STOPPING while executing PullBytes() above.
     */
    if ((status == ER_OK) && (conn->m_authState != AUTH_AUTHENTICATING)) {
        assert(conn->m_authState == AUTH_STOPPING);
        status = ER_STOPPING_THREAD;
    }

    if (status == ER_OK) {
        /*
         * We need to determine if the connecting client is a Desktop or Universal Windows app to correctly enforce the Windows app
         * isolation policies. Named pipe impersonation is used to determine who the caller is and the groupId is set to the
         * correct group. The groupId can be used by the PolicyDB to enforce the app isolation rules.
         */
        if (!ImpersonateNamedPipeClient(conn->m_pipeHandle)) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Worker: ImpersonateNamedPipeClient failed, error %u", ::GetLastError()));
        }

        HANDLE hClientToken = nullptr;
        if ((status == ER_OK) && !OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hClientToken)) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Worker: OpenThreadToken failed, error %u", ::GetLastError()));
        }

        /*
         * Done impersonating at this point, revert to self. Always stop impersonating, even if there was a failure.
         */
        if (!RevertToSelf()) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Worker: RevertToSelf failed, error %u", ::GetLastError()));

            /*
             * This thread pool thread might execute unrelated work later on, and that would be too dangerous if reverting impersonation failed.
             */
            abort();
        }

        DWORD isAppContainer = 0;
        DWORD length = sizeof(isAppContainer);
        if ((status == ER_OK) && !GetTokenInformation(hClientToken, TokenIsAppContainer, &isAppContainer, length, &length)) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Worker: GetTokenInformation - TokenIsAppContainer failed, error %u", ::GetLastError()));
        }

        SECURITY_IMPERSONATION_LEVEL securityLevel;
        length = sizeof(securityLevel);
        if ((status == ER_OK) && !GetTokenInformation(hClientToken, TokenImpersonationLevel, &securityLevel, length, &length)) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Worker: GetTokenInformation - TokenImpersonationLevel failed, error %u", ::GetLastError()));
        }

        if (securityLevel == SecurityIdentification) {
            /*
             * We've been provided an identification-level impersonation token, so we can't actually verify if this application
             * is an app container. Fail out as a result.
             */
            status = ER_BUS_NOT_ALLOWED;
            QCC_LogError(status, ("Worker: Impersonation token was an identification-level token and can't be trusted"));
        }

        PSID appContainerSid = nullptr;
        PSID_AND_ATTRIBUTES sidAndAttributes = nullptr;
        DWORD numAppContainers = 0;
        bool isWhitelisted = false;
        length = SECURITY_MAX_SID_SIZE + sizeof(TOKEN_APPCONTAINER_INFORMATION);
        BYTE buffer[SECURITY_MAX_SID_SIZE + sizeof(TOKEN_APPCONTAINER_INFORMATION)];
        if ((status == ER_OK) && isAppContainer == TRUE) {
            if (!GetTokenInformation(hClientToken, TokenAppContainerSid, buffer, length, &length)) {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("Worker: GetTokenInformation - TokenAppContainerSid failed, error %u", ::GetLastError()));
            }
        }

        /*
         * If a universal Windows app is in the loopback exemption list, then we will treat it as a desktop application. This
         * will allow the Universal Windows app to bypass the application isolation rules. This is allowed because an app on the
         * loopback exemption list could start its own bundled router, so it already has permissions to talk to the system. Skip
         * this step if the application is already whitelisted because of the isolation bypass capability.
         */
        PTSTR sidString = NULL;
        if ((status == ER_OK) && isAppContainer && !isWhitelisted) {
            appContainerSid = ((PTOKEN_APPCONTAINER_INFORMATION)buffer)->TokenAppContainer;

            if (!ConvertSidToStringSid(appContainerSid, &sidString)) {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("Worker: ConvertSidToStringSid failed, error %u", ::GetLastError()));
            }

            if (status == ER_OK) {
                uint32_t err = NetworkIsolationGetAppContainerConfig(&numAppContainers, &sidAndAttributes);
                if (err != ERROR_SUCCESS) {
                    status = ER_FAIL;
                    QCC_LogError(status, ("Worker: NetworkIsolationGetAppContainerConfig failed, error %u", err));
                }
            }

            if (status == ER_OK) {
                for (uint32_t i = 0; i < numAppContainers; i++) {
                    if (EqualSid(appContainerSid, sidAndAttributes[i].Sid)) {
                        QCC_DbgPrintf(("Worker: Connecting app with SID %s has a loopback exemption, will be treated as a Desktop application", sidString));
                        isAppContainer = FALSE;
                        break;
                    }
                }
            }

            /*
             * Universal Windows apps will have a unique user ID constructed for each application. This ID will be
             * based on the app container SID and the user SID to ensure that multiple users running the same
             * application will receive separate IDs. This unique user ID is required for policy rules which
             * ensure app isolation doesn't prevent an application with multiple busattachments from talking
             * to itself.
             */
            if (status == ER_OK) {
                TOKEN_USER* userToken = nullptr;
                LPTSTR userSidString = nullptr;
                String appIdStringUtf8;
                std::wstring appIdString(sidString);

                if (!GetTokenInformation(hClientToken, TokenUser, nullptr, 0, &length) && (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
                    userToken = (TOKEN_USER*)new BYTE[length];
                    if (userToken == nullptr) {
                        status = ER_OS_ERROR;
                        QCC_LogError(status, ("Worker: TOKEN_USER allocation failed"));
                    }

                    if ((status == ER_OK) && !GetTokenInformation(hClientToken, TokenUser, userToken, length, &length)) {
                        status = ER_OS_ERROR;
                        QCC_LogError(status, ("Worker: GetTokenInformation - TokenUser failed, error %u", ::GetLastError()));
                    }

                    if ((status == ER_OK) && !ConvertSidToStringSid(userToken->User.Sid, &userSidString)) {
                        status = ER_OS_ERROR;
                        QCC_LogError(status, ("Worker: ConvertSidToStringSid - User SID failed, error %u", ::GetLastError()));
                    }

                    if (status == ER_OK) {
                        appIdString += userSidString;

                        status = ConvertUTF(appIdString, appIdStringUtf8, false);
                        if (status != ER_OK) {
                            QCC_LogError(status, ("Worker: ConvertUTF failed"));
                        }
                    }

                    if (status == ER_OK) {
                        conn->SetUserId(GetUsersUid(appIdStringUtf8.c_str()));
                    }
                } else {
                    status = ER_OS_ERROR;
                    QCC_LogError(status, ("Worker: GetTokenInformation - TokenUser buffer size failed, error %u", ::GetLastError()));
                }

                delete[] userToken;
                LocalFree(userSidString);
            }
            LocalFree(sidString);
        }

        if ((hClientToken != nullptr) && !CloseHandle(hClientToken)) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Worker: CloseHandle failed, error %u", ::GetLastError()));
        }

        if (status == ER_OK) {
            if (isWhitelisted) {
                QCC_DbgPrintf(("Worker: Connecting application is a %s", WHITELISTED_APPLICATION));
                conn->SetGroupId(GetUsersGid(WHITELISTED_APPLICATION));
            } else if (isAppContainer == TRUE) {
                QCC_DbgPrintf(("Worker: Connecting application is a %s", UNIVERSAL_WINDOWS_APPLICATION));
                conn->SetGroupId(GetUsersGid(UNIVERSAL_WINDOWS_APPLICATION));
            } else {
                QCC_DbgPrintf(("Worker: Connecting application is a %s", DESKTOP_APPLICATION));
                conn->SetGroupId(GetUsersGid(DESKTOP_APPLICATION));
            }
        }

        if (status == ER_OK) {
            conn->SetListener(conn->m_transport);

            /*
             * Since Windows NamedPipeDaemonTransport enforces access control
             * using the security descriptors, no need to implement
             * UntrustedClientStart and UntrustedClientExit.
             */
            QCC_DbgHLPrintf(("Worker: calling Establish"));
            qcc::String authName;
            qcc::String redirection;
            status = conn->Establish("EXTERNAL", authName, redirection, nullptr, conn->m_transport->GetAuthTimeout());

            if (status != ER_OK) {
                QCC_LogError(status, ("Worker: failed to establish connection"));
            }
        }
    }

    if (status == ER_OK) {
        if (conn->TryToChangeAuthState(AUTH_AUTHENTICATING, AUTH_SUCCEEDED)) {
            QCC_DbgHLPrintf(("Worker: auth suceeded"));
            /*
             * Since named pipe clients/daemons do not go through version negotiations
             * and older clients/daemons won't connect over named pipe at all,
             * we are giving this end point the latest AllJoyn protocol version here.
             */
            conn->GetFeatures().protocolVersion = ALLJOYN_PROTOCOL_VERSION;
            status = conn->Start();
        } else if (!conn->TryToChangeAuthState(AUTH_STOPPING, AUTH_STOPPED)) {
            assert(false);
        }
    } else {
        if (conn->TryToChangeAuthState(AUTH_AUTHENTICATING, AUTH_FAILED)) {
            QCC_DbgHLPrintf(("Worker: auth failed"));
        } else if (!conn->TryToChangeAuthState(AUTH_STOPPING, AUTH_STOPPED)) {
            assert(false);
        }
    }

    QCC_DbgHLPrintf(("Worker: SetEvent"));
    conn->m_transport->m_authFinishedEvent.SetEvent();
}

bool _NamedPipeDaemonEndpoint::TryToChangeAuthState(_In_ AuthState oldState, _In_ AuthState newState)
{
    bool success = (InterlockedCompareExchange((LONG volatile*)&m_authState, newState, oldState) == oldState);
    QCC_DbgPrintf(("TryToChangeAuthState: from %u to %u - %s", (uint32_t)oldState, (uint32_t)newState, success ? "succeeded" : "failed"));
    return success;
}


NamedPipeDaemonTransport::NamedPipeDaemonTransport(_In_ BusAttachment& bus)
    : DaemonTransport(bus), m_authTimeout(ALLJOYN_AUTH_TIMEOUT_DEFAULT), m_acceptThread(*this)
{
    QCC_DbgTrace(("NamedPipeDaemonTransport()"));

    /*
     * We know we are daemon code, so we'd better be running with a daemon router.
     */
    assert(bus.GetInternal().GetRouter().IsDaemon());
}

NamedPipeDaemonTransport::~NamedPipeDaemonTransport()
{
    QCC_DbgTrace(("~NamedPipeDaemonTransport"));
}

void* NamedPipeDaemonTransport::Run(_In_ void* arg)
{
    QCC_DbgTrace(("NamedPipeDaemonTransport::Run"));
    QStatus status = ER_OK;

    while (!IsTransportStopping()) {
        status = Event::Wait(m_authFinishedEvent);

        if (status == ER_OK) {
            m_authFinishedEvent.ResetEvent();
            ManageAuthenticatingEndpoints();
        } else {
            assert(IsTransportStopping());
            break;
        }
    }

    QCC_DbgHLPrintf(("Run: exiting, status=%s", QCC_StatusText(status)));
    return (void*) status;
}

QStatus NamedPipeDaemonTransport::NormalizeTransportSpec(_In_z_ const char* inSpec, _Out_ qcc::String& outSpec, _Inout_ std::map<qcc::String, qcc::String>& argMap) const
{
    outSpec = inSpec;
    return ER_OK;
}

/*
 * Function used to get NamedPipe server on a new thread.
 */
QStatus NamedPipeDaemonTransport::StartListen(_In_z_ const char* listenSpec)
{
    QCC_DbgTrace(("StartListen"));
    QStatus status = ER_OK;

    if (IsTransportStopping()) {
        status = ER_BUS_TRANSPORT_NOT_STARTED;
    }

    if (status == ER_OK && IsRunning()) {
        status = ER_BUS_ALREADY_LISTENING;
    }

    if (status == ER_OK) {
        /*
         * Enforce a timeout for each pipe I/O during authentication phase.
         */
        ConfigDB* config = ConfigDB::GetConfigDB();
        m_authTimeout = config->GetLimit("auth_timeout", ALLJOYN_AUTH_TIMEOUT_DEFAULT);

        /*
         * Start the transport thread, responsible for managing endpoints.
         */
        status = Thread::Start(NULL);
    }

    if (status == ER_OK) {
        /*
         * Start the thread responsible for accepting pipe connections.
         */
        status = m_acceptThread.Start(NULL);
    }

    return status;
}

QStatus NamedPipeDaemonTransport::StopListen(_In_z_ const char* listenSpec)
{
    QCC_DbgTrace(("StopListen"));
    return Stop();
}

QStatus NamedPipeDaemonTransport::Stop()
{
    QCC_DbgTrace(("NamedPipeDaemonTransport::Stop"));
    m_acceptThread.Stop();

    /*
     * Stop the authenticatingEndpointList.
     */
    endpointListLock.Lock(MUTEX_CONTEXT);
    for (list<NamedPipeDaemonEndpoint>::iterator i = authenticatingEndpointList.begin(); i != authenticatingEndpointList.end(); ++i) {
        (*i)->Stop();
    }
    endpointListLock.Unlock(MUTEX_CONTEXT);

    /*
     * Base class Stop() will stop the endpointList.
     */
    return DaemonTransport::Stop();
}

QStatus NamedPipeDaemonTransport::Join()
{
    QCC_DbgTrace(("NamedPipeDaemonTransport::Join"));
    QStatus status = ER_OK;

    status = m_acceptThread.Join();

    if (status == ER_OK) {
        /*
         * Endpoints are joined before moving from authenticatingEndpointList to endpointList, so join just the remaining
         * authenticating endpoints here.
         */
        endpointListLock.Lock(MUTEX_CONTEXT);
        for (list<NamedPipeDaemonEndpoint>::iterator i = authenticatingEndpointList.begin(); i != authenticatingEndpointList.end(); ++i) {
            (*i)->Join();
            (*i)->Invalidate();
        }
        authenticatingEndpointList.clear();
        endpointListLock.Unlock(MUTEX_CONTEXT);

        status = DaemonTransport::Join();
    }

    return status;
}

void NamedPipeDaemonTransport::AcceptedConnection(_In_ HANDLE pipeHandle)
{
    QCC_DbgTrace(("AcceptedConnection"));
    QStatus status = ER_OK;
    NamedPipeDaemonEndpoint endpoint(bus, pipeHandle, this);

    endpointListLock.Lock(MUTEX_CONTEXT);

    /*
     * Perform authentication on a separate thread. AuthStart() has to run with
     * the endpointListLock held, because Join() relies on the same lock to
     * properly wait for the auth worker to finish execution.
     */
    status = endpoint->AuthStart();

    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to start authentication"));
    } else {
        authenticatingEndpointList.push_back(endpoint);
    }

    endpointListLock.Unlock(MUTEX_CONTEXT);
}

void NamedPipeDaemonTransport::ManageAuthenticatingEndpoints()
{
    QCC_DbgTrace(("ManageAuthenticatingEndpoints"));

    /*
     * Both authenticatingEndpointList and endpointList are protected by the same lock.
     */
    endpointListLock.Lock(MUTEX_CONTEXT);
    list<NamedPipeDaemonEndpoint>::iterator authIterator = authenticatingEndpointList.begin();

    while (authIterator != authenticatingEndpointList.end()) {
        if ((*authIterator)->AuthSucceeded()) {
            QCC_DbgHLPrintf(("ManageAuthenticatingEndpoints: moving endpoint to active list"));
            list<NamedPipeDaemonEndpoint>::iterator movedIterator = authIterator;
            authIterator++;
            (*movedIterator)->Join();
            NamedPipeDaemonEndpoint movedEndpoint = *movedIterator;
            authenticatingEndpointList.erase(movedIterator);
            endpointList.push_back(RemoteEndpoint::cast(movedEndpoint));
        } else if ((*authIterator)->AuthFailed() || (*authIterator)->AuthStopped()) {
            QCC_DbgHLPrintf(("ManageAuthenticatingEndpoints: removing failed or stopped endpoint"));
            list<NamedPipeDaemonEndpoint>::iterator erasedIterator = authIterator;
            authIterator++;
            (*erasedIterator)->Join();
            (*erasedIterator)->Invalidate();
            authenticatingEndpointList.erase(erasedIterator);
        } else {
            authIterator++;
        }
    }

    endpointListLock.Unlock(MUTEX_CONTEXT);
}

void* NamedPipeAcceptThread::Run(_In_ void* arg)
{
    QStatus status = ER_OK;

    /*
     * Accept client connections and connect those clients to the bus, in a loop.
     */
    while (!IsStopping()) {
        /*
         * Creating a new instance of the named pipe.
         */
        static const uint32_t BUFSIZE = 128 * 1024;
        HANDLE pipeHandle = AllJoynCreateBus(BUFSIZE, BUFSIZE, nullptr);

        if (pipeHandle == INVALID_HANDLE_VALUE) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("NamedPipeAcceptThread: AllJoynCreateBus failed, error %u", ::GetLastError()));
            break;
        }

        /*
         * AllJoynAcceptBusConnection accepts a named pipe connection, then sets the pipe mode to PIPE_NOWAIT.
         * AllJoynAcceptBusConnection returns ERROR_OPERATION_ABORTED when the stopEvent gets signaled - i.e.
         * when this transport should stop its execution.
         */
        QCC_DbgHLPrintf(("NamedPipeAcceptThread: Waiting for connection on pipeHandle = 0x%p", pipeHandle));
        DWORD acceptResult = AllJoynAcceptBusConnection(pipeHandle, stopEvent.GetHandle());

        if (acceptResult == ERROR_SUCCESS) {
            QCC_DbgHLPrintf(("NamedPipeAcceptThread: Accepted client connection on pipeHandle 0x%p", pipeHandle));
        } else {
            if (acceptResult == ERROR_OPERATION_ABORTED) {
                assert(IsStopping());
                QCC_DbgHLPrintf(("NamedPipeAcceptThread: transport is stopping"));
                status = ER_STOPPING_THREAD;
            } else {
                QCC_LogError(status, ("NamedPipeAcceptThread: AllJoynAcceptBusConnection failed, error %u", acceptResult));
                status = ER_OS_ERROR;
            }

            BOOL success = AllJoynCloseBusHandle(pipeHandle);
            assert(success);
        }

        if (status == ER_OK) {
            /*
             * Transfer ownership of the connected pipe handle to the transport object.
             */
            m_transport.AcceptedConnection(pipeHandle);
        }
    }

    QCC_DbgHLPrintf(("NamedPipeAcceptThread: exiting, status=%s", QCC_StatusText(status)));
    return (void*) status;
}

} // namespace ajn

#endif // #if (_WIN32_WINNT > 0x0603)
