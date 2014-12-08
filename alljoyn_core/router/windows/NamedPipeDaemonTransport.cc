/**
 * @file
 * NamedPipeDaemonTransport (Daemon Transport for named pipe) implementation for Windows
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <Netfw.h>

#define QCC_MODULE "DAEMON_TRANSPORT"

using namespace std;
using namespace qcc;

namespace ajn {

const char* NamedPipeDaemonTransport::NamedPipeTransportName = "npipe";

class _NamedPipeDaemonEndpoint;

typedef ManagedObj<_NamedPipeDaemonEndpoint> NamedPipeDaemonEndpoint;

/*
 * An endpoint class to handle the details of authenticating a connection.
 */
class _NamedPipeDaemonEndpoint : public _RemoteEndpoint {

  public:

    _NamedPipeDaemonEndpoint(BusAttachment& bus, HANDLE serverHandle) :
        _RemoteEndpoint(bus, true, NamedPipeDaemonTransport::NamedPipeTransportName, &pipeStream, NamedPipeDaemonTransport::NamedPipeTransportName, false),
        pipeStream(serverHandle)
    {
    }

    ~_NamedPipeDaemonEndpoint()
    {
    }

    NamedPipeStream pipeStream;
};

static const uint32_t NUL_BYTE_TIMEOUT = 5000;  // in msec

NamedPipeDaemonTransport::NamedPipeDaemonTransport(BusAttachment& bus)
    : DaemonTransport(bus), bus(bus)
{
    /*
     * We know we are daemon code, so we'd better be running with a daemon router.
     */
    assert(bus.GetInternal().GetRouter().IsDaemon());
}

void* NamedPipeDaemonTransport::Run(void* arg)
{
    BOOL isConnected = FALSE;
    QStatus status = ER_OK;
    static const uint32_t BUFSIZE = 128 * 1024;

    /*
     * Security Descriptor
     */
    SECURITY_ATTRIBUTES securityAttributes;
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;
    ULONG securityDescriptorSize;
    ZeroMemory(&securityAttributes, sizeof(securityAttributes));

    /*
     * Allows Read/Write access to all AppContainers (AC) and Built in Users (BU).
     */
    static const WCHAR securityDescriptorString[] = L"D:(A;;GA;;;BU)(A;;GA;;;AC)";

    /*
     * Using the wide character version because it is available on Windows versions with smaller footprints.
     */
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            securityDescriptorString,
            SDDL_REVISION_1,
            &securityDescriptor,
            &securityDescriptorSize)) {
        status = ER_OS_ERROR;
        QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): Conversion to Security Descriptor failed error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
        return (void*) status;
    }

    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.bInheritHandle = FALSE;
    securityAttributes.lpSecurityDescriptor = securityDescriptor;

    while (!IsStopping()) {

        HANDLE serverHandle = INVALID_HANDLE_VALUE;

        /*
         * Creating a new instance of the AllJoyn router node named pipe.
         */
        serverHandle = AllJoynCreateBus(BUFSIZE, BUFSIZE, &securityAttributes);

        if (serverHandle == INVALID_HANDLE_VALUE) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): AllJoynCreateBus failed error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
            break;
        }

        QCC_DbgHLPrintf(("NamedPipeDaemonTransport::Run(): Waiting for Client Connection serverHandle=%p", serverHandle));

        /*
         * AllJoynAcceptBusConnection accepts a named pipe connection asynchronously, then sets the pipe mode to PIPE_NOWAIT
         */
        DWORD acceptResult = AllJoynAcceptBusConnection(serverHandle, Thread::GetStopEvent().GetHandle());

        if (acceptResult != ERROR_SUCCESS) {
            BOOL success = AllJoynCloseBusHandle(serverHandle);
            assert(success);
            serverHandle = INVALID_HANDLE_VALUE;

            if (acceptResult == ERROR_OPERATION_ABORTED) {
                status = ER_STOPPING_THREAD;
                break;
            } else {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): AllJoynAcceptBusConnection failed (0x%08X)", acceptResult));
                continue;
            }
        }

        QCC_DbgTrace(("NamedPipeDaemonTransport::Run(): Connected to client (serverHandle = %p)", serverHandle));

        qcc::String authName;
        qcc::String redirection;
        NamedPipeDaemonEndpoint conn(bus, serverHandle);

        /*
         * Initialize the features for this endpoint.
         */
        conn->GetFeatures().isBusToBus = false;
        conn->GetFeatures().allowRemote = false;
        conn->GetFeatures().handlePassing = false;

        endpointListLock.Lock(MUTEX_CONTEXT);
        endpointList.push_back(RemoteEndpoint::cast(conn));
        endpointListLock.Unlock(MUTEX_CONTEXT);

        uint8_t byte;
        size_t nbytes;
        /*
         * Read the initial NUL byte.
         */
        status = conn->pipeStream.PullBytes(&byte, 1, nbytes, NUL_BYTE_TIMEOUT);

        if ((status != ER_OK) || (nbytes != 1) || (byte != 0)) {
            status = (status == ER_OK) ? ER_FAIL : status;
        } else {

            /*
             * We need to determine if the connecting client is a Desktop or Universal Windows app to correctly enforce the Windows app
             * isolation policies. Named pipe impersonation is used to determine who the caller is and the groupId is set to the
             * correct group. The groupId can be used by the PolicyDB to enforce the app isolation rules.
             */
            if (!ImpersonateNamedPipeClient(serverHandle)) {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): ImpersonateNamedPipeClient failed error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
            }

            HANDLE hClientToken = NULL;
            if ((status == ER_OK) && !OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hClientToken)) {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): OpenThreadToken failed error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
            }

            DWORD isAppContainer = 0;
            DWORD length = sizeof(DWORD);
            if ((status == ER_OK) && !GetTokenInformation(hClientToken, TokenIsAppContainer, &isAppContainer, length, &length)) {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): GetTokenInformation - TokenIsAppContainer failed error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
            }

            SECURITY_IMPERSONATION_LEVEL securityLevel;
            length = sizeof(securityLevel);
            if ((status == ER_OK) && !GetTokenInformation(hClientToken, TokenImpersonationLevel, &securityLevel, length, &length)) {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): GetTokenInformation - TokenImpersonationLevel failed error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
            }

            if (securityLevel == SecurityIdentification) {
                /* We've been provided an identification-level impersonation token
                 * so we can't actually verify if this application is an app container.
                 * Fail out as a result.
                 */
                status = ER_BUS_NOT_ALLOWED;
                QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): Impersonation token was an identification-level token and can't be trusted"));
            }

            PSID appContainerSid = NULL;
            PSID_AND_ATTRIBUTES sidAndAttributes = NULL;
            DWORD numAppContainers = 0;
            BOOL isWhitelisted = FALSE;
            length = SECURITY_MAX_SID_SIZE + sizeof(TOKEN_APPCONTAINER_INFORMATION);
            BYTE buffer[SECURITY_MAX_SID_SIZE + sizeof(TOKEN_APPCONTAINER_INFORMATION)];
            if ((status == ER_OK) && isAppContainer == TRUE) {
                if (!GetTokenInformation(hClientToken, TokenAppContainerSid, buffer, length, &length)) {
                    status = ER_OS_ERROR;
                    QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): GetTokenInformation - TokenAppContainerSid failed OS error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
                }
            }

            // Done impersonating at this point, revert to self
            if (!RevertToSelf()) {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): RevertToSelf failed error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
            }

            if (hClientToken != NULL && !CloseHandle(hClientToken)) {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): CloseHandle failed error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
            }

            /*
             * If a universal Windows app is in the loopback exemption list, then we will treat it as a desktop application. This
             * will allow the Universal Windows app to bypass the application isolation rules. This is allowed because an app on the
             * loopback exemption list could start its own bundled router, so it already has permissions to talk to the system.
             */
            if ((status == ER_OK) && isAppContainer == TRUE) {
                appContainerSid = ((PTOKEN_APPCONTAINER_INFORMATION)buffer)->TokenAppContainer;

                uint32_t err = NetworkIsolationGetAppContainerConfig(&numAppContainers, &sidAndAttributes);
                if (err != ERROR_SUCCESS) {
                    status = ER_FAIL;
                    QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): NetworkIsolationGetAppContainerConfig failed (0x%08X)", err));
                }

                if (status == ER_OK) {
                    for (uint32_t i = 0; i < numAppContainers; i++) {
                        if (EqualSid(appContainerSid, sidAndAttributes[i].Sid)) {
                            LPTSTR sidString = NULL;
                            if (ConvertSidToStringSid(appContainerSid, &sidString)) {
                                QCC_DbgPrintf(("NamedPipeDaemonTransport::Run(): Connecting app with SID %s has a loopback exemption, will be treated as a Desktop application", sidString));
                            }

                            if (sidString != NULL) {
                                if (LocalFree(sidString) != NULL) {
                                    // Non-critical error, don't fail the connection but do log the error code
                                    QCC_LogError(status, ("NamedPipeDaemonTransport::Run(): LocalFree of sidString failed error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
                                }
                            }

                            isAppContainer = FALSE;
                            break;
                        }
                    }
                }
            }

            if (status == ER_OK) {
                if (isWhitelisted == TRUE) {
                    QCC_DbgPrintf(("NamedPipeDaemonTransport::Run(): Connecting application is a %s", WHITELISTED_APPLICATION));
                    conn->SetGroupId(GetUsersGid(WHITELISTED_APPLICATION));
                } else if (isAppContainer == TRUE) {
                    QCC_DbgPrintf(("NamedPipeDaemonTransport::Run(): Connecting application is a %s", UNIVERSAL_WINDOWS_APPLICATION));
                    conn->SetGroupId(GetUsersGid(UNIVERSAL_WINDOWS_APPLICATION));
                } else {
                    QCC_DbgPrintf(("NamedPipeDaemonTransport::Run(): Connecting application is a %s", DESKTOP_APPLICATION));
                    conn->SetGroupId(GetUsersGid(DESKTOP_APPLICATION));
                }
            }

            if (status == ER_OK) {
                /* Since Windows NamedPipeDaemonTransport enforces access control
                 * using the security descriptors, no need to implement
                 * UntrustedClientStart and UntrustedClientExit.
                 */
                conn->SetListener(this);
                status = conn->Establish("EXTERNAL", authName, redirection);
            }
        }
        if (status == ER_OK) {
            /*
             * Since named pipe clients/daemons do not go through version negotiations
             * and older clients/daemons won't connect over named pipe at all,
             * we are giving this end point the latest AllJoyn protocol version here.
             */
            conn->GetFeatures().protocolVersion = ALLJOYN_PROTOCOL_VERSION;
            status = conn->Start();
        }
        if (status != ER_OK) {
            QCC_LogError(status, ("Error starting NamedPipeDaemonEndpoint"));
            endpointListLock.Lock(MUTEX_CONTEXT);
            list<RemoteEndpoint>::iterator ei = find(endpointList.begin(), endpointList.end(), RemoteEndpoint::cast(conn));
            if (ei != endpointList.end()) {
                endpointList.erase(ei);
            }
            endpointListLock.Unlock(MUTEX_CONTEXT);
            conn->Invalidate();
            QCC_LogError(status, ("Error accepting new connection. Ignoring..."));
        }

    }

    if (securityDescriptor != NULL) {
        LocalFree(securityDescriptor);
    }

    QCC_DbgPrintf(("NamedPipeDaemonTransport::Run is exiting. status=%s", QCC_StatusText(status)));
    return (void*) status;
}

QStatus NamedPipeDaemonTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, std::map<qcc::String, qcc::String>& argMap) const
{
    outSpec = inSpec;
    return ER_OK;
}

/*
 * Function used to get NamedPipe server on a new thread.
 */
QStatus NamedPipeDaemonTransport::StartListen(const char* listenSpec)
{
    if (IsTransportStopping()) {
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }
    if (IsRunning()) {
        return ER_BUS_ALREADY_LISTENING;
    }

    return Thread::Start(NULL);
}

QStatus NamedPipeDaemonTransport::StopListen(const char* listenSpec)
{
    return Thread::Stop();
}

} // namespace ajn

#endif // #if (_WIN32_WINNT > 0x0603)
