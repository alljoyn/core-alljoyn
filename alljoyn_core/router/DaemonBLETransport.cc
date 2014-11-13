/**
 * @file
 * Implementation of DaemonBLETransport.
 */

/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#include <errno.h>

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>
#include <qcc/BLEStream.h>
#include <qcc/SLAPStream.h>
#include <qcc/Thread.h>

#include "DaemonRouter.h"
#include "BusInternal.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "DaemonBLETransport.h"
#include "BusController.h"
#include "BTController.h"

#if defined QCC_OS_GROUP_POSIX
#if defined(QCC_OS_DARWIN)
#warning Darwin support for bluetooth to be implemented
#else
#include "bt_bluez/BLEAccessor.h"
#endif
#elif defined QCC_OS_GROUP_WINDOWS
#include "bt_windows/BTAccessor.h"
#endif

#define QCC_MODULE "DAEMON_BLE"

using namespace std;
using namespace qcc;
using namespace ajn;

#if !defined QCC_OS_GROUP_WINDOWS
using namespace ajn::bluez;
#endif


namespace ajn {

const char* DaemonBLETransport::TransportName = "ble";

#define BUS_NAME_TTL numeric_limits<uint8_t>::max()

/*
 * An endpoint class to handle the details of authenticating a connection over the SLAP transport.
 */
class _DaemonBLEEndpoint :
    public _RemoteEndpoint {

    friend class DaemonBLETransport;
  public:
    enum EndpointState {
        EP_ILLEGAL = 0,
        EP_INITIALIZED,      /**< This endpoint structure has been allocated but not used */
        EP_FAILED,           /**< Starting the RX and TX threads has failed and this endpoint is not usable */
        EP_STARTING,          /**< The RX and TX threads are being started */
        EP_STARTED,          /**< The RX and TX threads have been started (they work as a unit) */
        EP_STOPPING,         /**< The RX and TX threads are stopping (have run ThreadExit) but have not been joined */
        EP_DONE              /**< The RX and TX threads have been shut down and joined */
    };

    enum AuthState {
        AUTH_ILLEGAL = 0,
        AUTH_INITIALIZED,    /**< This endpoint structure has been allocated but no auth thread has been run */
        AUTH_AUTHENTICATING, /**< We have spun up an authentication thread and it has begun running our user function */
        AUTH_FAILED,         /**< The authentication has failed and the authentication thread is exiting immidiately */
        AUTH_SUCCEEDED,      /**< The auth process (Establish) has succeeded and the connection is ready to be started */
        AUTH_DONE,           /**< The auth thread has been successfully shut down and joined */
    };

    _DaemonBLEEndpoint(DaemonBLETransport* transport, BusAttachment& bus, BLEStreamAccessor* accessor, bool incoming, const qcc::String connectSpec, const char*remDev, uint32_t packetSize) :
        _RemoteEndpoint(bus, incoming, connectSpec, &m_stream, DaemonBLETransport::TransportName),
        m_transport(transport), m_authThread(this), m_remObj(remDev), m_authState(AUTH_INITIALIZED), m_epState(EP_INITIALIZED),
        m_timer("SLAPEp", true, 1, false, 10),
        m_rawStream(accessor, m_remObj),
        m_stream(&m_bleController, m_timer, packetSize, 2, 15000),
        m_bleController(&m_rawStream, &m_stream)
    {
        m_stream.AckImmediate(true);
    }

    EndpointState GetEpState(void) { return m_epState; }
    ~_DaemonBLEEndpoint() { QCC_LogError(ER_OK, ("_DaemonBLEEndpoint::Destructor state:%d", m_epState)); }

    QStatus Authenticate(void);
    void AuthStop(void);
    void AuthJoin(void);

    void SetEpFailed(void)
    {
        m_epState = EP_FAILED;
    }

    void SetEpStarting(void)
    {
        m_epState = EP_STARTING;
    }

    void SetEpStarted(void)
    {
        m_epState = EP_STARTED;
    }

    void SetEpStopping(void)
    {
        assert(m_epState == EP_STARTING || m_epState == EP_STARTED || m_epState == EP_STOPPING);
        m_epState = EP_STOPPING;
        QCC_LogError(ER_OK, ("_DaemonBLEEndpoint::SetEpStopping"));
    }

    void SetEpDone(void)
    {
        assert(m_epState == EP_FAILED || m_epState == EP_STOPPING);
        m_epState = EP_DONE;
    }
    AuthState GetAuthState(void) { return m_authState; }

    qcc::String GetRemObj() { return m_remObj; }

    QStatus Stop();
    QStatus Join();

    virtual void ThreadExit(qcc::Thread* thread);

  private:
    class AuthThread : public qcc::Thread {
      public:
        AuthThread(_DaemonBLEEndpoint* ep) : Thread("auth"), m_endpoint(ep)  { }
      private:
        virtual qcc::ThreadReturn STDCALL Run(void* arg);

        _DaemonBLEEndpoint* m_endpoint;
    };
    DaemonBLETransport* m_transport;        /**< The server holding the connection */
    AuthThread m_authThread;          /**< Thread used to do blocking calls during startup */

    qcc::String m_remObj;             /**< The Remote Object for BLE */
    volatile AuthState m_authState;   /**< The state of the endpoint authentication process */
    volatile EndpointState m_epState; /**< The state of the endpoint authentication process */
    Timer m_timer;                    /**< Multipurpose timer for sending/resend/acks */
    BLEStream m_rawStream;           /**< The raw BLE stream */
    SLAPStream m_stream;              /**< The SLAP stream used for Alljoyn communication */
    BLEController m_bleController;  /**< Controller responsible for reading from BLE */
};

void _DaemonBLEEndpoint::ThreadExit(qcc::Thread* thread)
{
    QCC_LogError(ER_OK, ("_DaemonBLEEndpoint::ThreadExit"));
    if (thread == &m_authThread) {
        if (m_authState == AUTH_INITIALIZED) {
            m_authState = AUTH_FAILED;
        }
        m_transport->Alert();
    }
    _RemoteEndpoint::ThreadExit(thread);
}

QStatus _DaemonBLEEndpoint::Authenticate(void)
{
    QCC_DbgTrace(("DaemonBLEEndpoint::Authenticate()"));
    m_timer.Start();

    QStatus status = m_stream.ScheduleLinkControlPacket();
    /*
     * Start the authentication thread.
     */
    if (status == ER_OK) {
        status = m_authThread.Start(this, this);
    }
    if (status != ER_OK) {
        QCC_DbgPrintf(("DaemonBLEEndpoint::Authenticate() Failed to authenticate endpoint"));
        m_authState = AUTH_FAILED;
        /* Alert the Run() thread to refresh the list of com ports to listen on. */
        m_transport->Alert();
    }
    return status;
}

void _DaemonBLEEndpoint::AuthStop(void)
{
    QCC_DbgTrace(("DaemonBLEEndpoint::AuthStop()"));

    /* Stop the controller only if Authentication failed */
    if (m_authState != AUTH_SUCCEEDED) {
        m_timer.Stop();
    }
    /*
     * Ask the auth thread to stop executing.  The only ways out of the thread
     * run function will set the state to either AUTH_SUCCEEDED or AUTH_FAILED.
     * There is a very small chance that we will send a stop to the thread after
     * it has successfully authenticated, but we expect that this will result in
     * an AUTH_FAILED state for the vast majority of cases.  In this case, we
     * notice that the thread failed the next time through the main server run
     * loop, join the thread via AuthJoin below and delete the endpoint.  Note
     * that this is a lazy cleanup of the endpoint.
     */
    m_authThread.Stop();
}

void _DaemonBLEEndpoint::AuthJoin(void)
{
    QCC_DbgTrace(("DaemonBLEEndpoint::AuthJoin()"));
    /* Join the controller only if Authentication failed */
    if (m_authState != AUTH_SUCCEEDED) {
        m_timer.Join();
    }

    /*
     * Join the auth thread to stop executing.  All threads must be joined in
     * order to communicate their return status.  The auth thread is no exception.
     * This is done in a lazy fashion from the main server accept loop, where we
     * cleanup every time through the loop.
     */
    m_authThread.Join();
}
QStatus _DaemonBLEEndpoint::Stop(void)
{
    QCC_DbgTrace(("DaemonBLEEndpoint::Stop()"));
    m_timer.Stop();
    QStatus status = _RemoteEndpoint::Stop();

    return status;
}

QStatus _DaemonBLEEndpoint::Join(void)
{
    QCC_DbgTrace(("DaemonBLEEndpoint::Join()"));
    m_timer.Join();
    QStatus status = _RemoteEndpoint::Join();
    return status;
}

void* _DaemonBLEEndpoint::AuthThread::Run(void* arg)
{
    QCC_DbgPrintf(("DaemonBLEEndpoint::AuthThread::Run()"));

    m_endpoint->m_authState = AUTH_AUTHENTICATING;

    /*
     * We're running an authentication process here and we are cooperating with
     * the main server thread.  This thread is running in an object that is
     * allocated on the heap, and the server is managing these objects so we
     * need to coordinate getting all of this cleaned up.
     *
     * Since this is a serial point-to-point connection, we do not bother about
     * denial of service attacks.
     */
    uint8_t byte;
    size_t nbytes;
    QCC_DbgPrintf(("DaemonBLEEndpoint::AuthThread::Run() calling pullbytes"));
    /*
     * Eat the first byte of the stream.  This is required to be zero by the
     * DBus protocol.  It is used in the Unix socket implementation to carry
     * out-of-band capabilities, but is discarded here.  We do this here since
     * it involves a read that can block.
     */
    QStatus status = m_endpoint->m_stream.PullBytes(&byte, 1, nbytes);
    if ((status != ER_OK) || (nbytes != 1) || (byte != 0)) {
        QCC_LogError(status, ("Failed to read first byte from stream %d %d", nbytes, byte));

        /*
         * Management of the resources used by the authentication thread is done
         * in one place, by the server Accept loop.  The authentication thread
         * writes its state into the connection and the server Accept loop reads
         * this state.  As soon as we set this state to AUTH_FAILED, we are
         * telling the Accept loop that we are done with the conn data
         * structure.  That thread is then free to do anything it wants with the
         * connection, including deleting it, so we are not allowed to touch
         * conn after setting this state.
         *
         * In addition to releasing responsibility for the conn data structure,
         * when we set the state to AUTH_SUCCEEDED we are telling the server
         * accept loop that we are exiting now and so it can Join() on us (the
         * authentication thread) without being worried about blocking since the
         * next thing we do is exit.
         */
        m_endpoint->m_authState = AUTH_FAILED;
        /* Alert the Run() thread to refresh the list of com ports to listen on. */
        m_endpoint->m_transport->Alert();
        return (void*)ER_FAIL;
    }
    /* Initialized the features for this endpoint */
    m_endpoint->GetFeatures().isBusToBus = false;
    m_endpoint->GetFeatures().handlePassing = false;

    /* Run the actual connection authentication code. */
    qcc::String authName;
    qcc::String redirection;
    DaemonRouter& router = reinterpret_cast<DaemonRouter&>(m_endpoint->m_transport->m_bus.GetInternal().GetRouter());
    AuthListener* authListener = router.GetBusController()->GetAuthListener();
    /* Since the DaemonBLETransport allows untrusted clients, it must implement UntrustedClientStart and
     * UntrustedClientExit.
     * As a part of Establish, the endpoint can call the Transport's UntrustedClientStart method if
     * it is an untrusted client, so the transport MUST call m_endpoint->SetListener before calling Establish
     * Note: This is only required on the accepting end i.e. for incoming endpoints.
     */
    m_endpoint->SetListener(m_endpoint->m_transport);
    if (authListener) {
        status = m_endpoint->Establish("ALLJOYN_PIN_KEYX ANONYMOUS", authName, redirection, authListener);
    } else {
        status = m_endpoint->Establish("ANONYMOUS", authName, redirection, authListener);
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to establish SLAP endpoint"));

        /*
         * Management of the resources used by the authentication thread is done
         * in one place, by the server Accept loop.  The authentication thread
         * writes its state into the connection and the server Accept loop reads
         * this state.  As soon as we set this state to AUTH_FAILED, we are
         * telling the Accept loop that we are done with the conn data
         * structure.  That thread is then free to do anything it wants with the
         * connection, including deleting it, so we are not allowed to touch
         * conn after setting this state.
         *
         * In addition to releasing responsibility for the conn data structure,
         * when we set the state to AUTH_SUCCEEDED we are telling the server
         * accept loop that we are exiting now and so it can Join() on us (the
         * authentication thread) without being worried about blocking since the
         * next thing we do is exit.
         */
        m_endpoint->m_authState = AUTH_FAILED;
        /* Alert the Run() thread to refresh the list of com ports to listen on. */
        m_endpoint->m_transport->Alert();
        return (void*)status;
    }

    /*
     * Tell the transport that the authentication has succeeded and that it can
     * now bring the connection up.
     */
    DaemonBLEEndpoint dEp = DaemonBLEEndpoint::wrap(m_endpoint);
    m_endpoint->m_transport->Authenticated(dEp);

    QCC_DbgPrintf(("DaemonBLEEndpoint::AuthThread::Run(): Returning"));

    /*
     * We are now done with the authentication process.  We have succeeded doing
     * the authentication and we may or may not have succeeded in starting the
     * endpoint TX and RX threads depending on what happened down in
     * Authenticated().  What concerns us here is that we are done with this
     * thread (the authentication thread) and we are about to exit.  Before
     * exiting, we must tell server accept loop that we are done with this data
     * structure.  As soon as we set this state to AUTH_SUCCEEDED that thread is
     * then free to do anything it wants with the connection, including deleting
     * it, so we are not allowed to touch conn after setting this state.
     *
     * In addition to releasing responsibility for the conn data structure, when
     * we set the state to AUTH_SUCCEEDED we are telling the server accept loop
     * that we are exiting now and so it can Join() the authentication thread
     * without being worried about blocking since the next thing we do is exit.
     */
    m_endpoint->m_authState = AUTH_SUCCEEDED;
    return (void*)status;
}

DaemonBLETransport::DaemonBLETransport(BusAttachment& bus)
    : m_bus(bus), stopping(false)
{
    /*
     * We know we are daemon code, so we'd better be running with a daemon
     * router.  This is assumed elsewhere.
     */
    btController = new BTController(bus, *this);
    QStatus status = btController->Init();
    if (status == ER_OK) {
        bleAccessor = new BLEAccessor(this, bus.GetGlobalGUIDString());
        btmActive = true;
    }
}


DaemonBLETransport::~DaemonBLETransport()
{
    Stop();
    Join();
    delete btController;
    btController = NULL;
    if (btmActive) {
        delete bleAccessor;
    }
}

QStatus DaemonBLETransport::Start()
{
    QStatus status;

    QCC_DbgTrace(("BLETransport::Start()"));
    stopping = false;
    status = bleAccessor->Start();
    if (status == ER_OK) {
        return Thread::Start();
    }

    return status;
}

QStatus DaemonBLETransport::Stop()
{
    stopping = true;

    if (IsStopping()) {
        bleAccessor->Stop();
    }

    /*
     * Tell the DaemonBLETransport::Run thread to shut down.
     */
    QStatus status = Thread::Stop();
    if (status != ER_OK) {
        QCC_LogError(status, ("DaemonBLETransport::Stop(): Failed to Stop() main thread"));
    }

    m_lock.Lock(MUTEX_CONTEXT);
    /*
     * Ask any authenticating endpoints to shut down and exit their threads.
     */
    for (set<DaemonBLEEndpoint>::iterator i = m_authList.begin(); i != m_authList.end(); ++i) {
        DaemonBLEEndpoint ep = *i;
        ep->AuthStop();
    }

    /*
     * Ask any running endpoints to shut down and exit their threads.
     */
    for (set<DaemonBLEEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        DaemonBLEEndpoint ep = *i;
        ep->Stop();
    }

    m_lock.Unlock(MUTEX_CONTEXT);

    return ER_OK;

}

QStatus DaemonBLETransport::Join()
{
    /*
     * Wait for the DaemonBLETransport::Run thread to exit.
     */
    QStatus status = Thread::Join();
    if (status != ER_OK) {
        QCC_LogError(status, ("DaemonBLETransport::Join(): Failed to Join() main thread"));
        return status;
    }

    m_lock.Lock(MUTEX_CONTEXT);
    /*
     * Any authenticating endpoints have been asked to shut down and exit their
     * authentication threads in a previously required Stop().  We need to
     * Join() all of these auth threads here.
     */
    set<DaemonBLEEndpoint>::iterator it = m_authList.begin();
    while (it != m_authList.end()) {
        DaemonBLEEndpoint ep = *it;
        m_authList.erase(it);
        m_lock.Unlock(MUTEX_CONTEXT);
        ep->AuthJoin();
        m_lock.Lock(MUTEX_CONTEXT);
        it = m_authList.upper_bound(ep);
    }


    /*
     * Any running endpoints have been asked it their threads in a previously
     * required Stop().  We need to Join() all of thesse threads here.  This
     * Join() will wait on the endpoint rx and tx threads to exit as opposed to
     * the joining of the auth thread we did above.
     */
    it = m_endpointList.begin();
    while (it != m_endpointList.end()) {
        DaemonBLEEndpoint ep = *it;
        m_endpointList.erase(it);
        m_lock.Unlock(MUTEX_CONTEXT);
        ep->Join();
        m_lock.Lock(MUTEX_CONTEXT);
        it = m_endpointList.upper_bound(ep);
    }

    stopping = false;
    m_lock.Unlock(MUTEX_CONTEXT);

    return ER_OK;

}

QStatus DaemonBLETransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, std::map<qcc::String, qcc::String>& argMap) const
{
    QStatus status = ParseArguments(DaemonBLETransport::TransportName, inSpec, argMap);

    if (status == ER_OK) {
        map<qcc::String, qcc::String>::iterator it;
        /*
         * Include only the type and dev in the outSpec.  The outSpec
         * is intended to be unique per device (i.e. you can't have two
         * connections to the same device with different parameters).
         */
        outSpec = "ble:";
        it = argMap.find("addr");
        if (it == argMap.end()) {
            QCC_LogError(status, ("'addr=' must be specified for 'bluetooth:'"));
        } else {
            outSpec.append("addr=");
            outSpec += it->second;
        }
    }

    return status;
}

QStatus DaemonBLETransport::StartListen(const char* listenSpec)
{
    if (stopping == true) {
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    /* Normalize the listen spec. */
    QStatus status;
    qcc::String normSpec;
    map<qcc::String, qcc::String> serverArgs;
    status = NormalizeTransportSpec(listenSpec, normSpec, serverArgs);
    if (status != ER_OK) {
        QCC_LogError(status, ("DaemonTransport::StartListen(): Invalid SLAP listen spec \"%s\"", listenSpec));
        return status;
    }
    m_lock.Lock(MUTEX_CONTEXT);

    for (list<ListenEntry>::iterator i = m_listenList.begin(); i != m_listenList.end(); i++) {
        if (i->normSpec == normSpec) {
            m_lock.Unlock(MUTEX_CONTEXT);
            return ER_BUS_ALREADY_LISTENING;
        }
    }
    /* Ignore incorrect listen spec i.e. other than uart */
    m_listenList.push_back(ListenEntry(normSpec, serverArgs));
    m_lock.Unlock(MUTEX_CONTEXT);
    Thread::Alert();

    return ER_OK;

}

QStatus DaemonBLETransport::StopListen(const char* listenSpec)
{
    return ER_OK;
}

void DaemonBLETransport::BLEDeviceAvailable(bool avail)
{
    if (btController) {
        btController->BLEDeviceAvailable(avail);
    }
}

void DaemonBLETransport::DisconnectAll()
{
}

void DaemonBLETransport::EndpointExit(DaemonBLEEndpoint& dEp)
{
    /*
     * This is a callback driven from the remote endpoint thread exit function.
     * Our DaemonEndpoint inherits from class RemoteEndpoint and so when
     * either of the threads (transmit or receive) of one of our endpoints exits
     * for some reason, we get called back here.
     */
    QCC_DbgPrintf(("DaemonBLETransport::EndpointExit()"));
    /* Remove the dead endpoint from the live endpoint list */
    m_lock.Lock(MUTEX_CONTEXT);
    QCC_DbgPrintf(("DaemonBLETransport::EndpointExit()setting stopping"));
    QCC_LogError(ER_OK, ("DaemonBLETransport::EndpointExit"));
    dEp->SetEpStopping();
    m_EPset.erase(dEp);
    Thread::Alert();
    m_lock.Unlock(MUTEX_CONTEXT);
}

void DaemonBLETransport::EndpointExit(RemoteEndpoint& ep)
{
    DaemonBLEEndpoint dEp = DaemonBLEEndpoint::cast(ep);
    EndpointExit(dEp);
    ep->Invalidate();
}

BLEController* DaemonBLETransport::NewDeviceFound(const char* remoteDevice)
{
    DaemonBLETransport* ptr = this;
    uint32_t packetSize = SLAP_DEFAULT_PACKET_SIZE;
    static const bool truthiness = true;
    QStatus status;
    DaemonBLEEndpoint conn(ptr, m_bus, bleAccessor, truthiness, "slap", remoteDevice, packetSize);

    status = conn->Authenticate();
    if (status == ER_OK) {
        m_EPset.insert(conn);
        return &conn->m_bleController;
    }

    return NULL;

}

bool DaemonBLETransport::IsConnValid(qcc::BLEController* bleController)
{
    m_lock.Lock(MUTEX_CONTEXT);
    set<DaemonBLEEndpoint>::iterator i = m_EPset.begin();
    i = m_EPset.begin();
    while (i != m_EPset.end()) {
        DaemonBLEEndpoint ep = *i;
        if (&ep->m_bleController == bleController) {
            m_lock.Unlock(MUTEX_CONTEXT);
            return true;
        }
        i++;
    }
    QCC_LogError(ER_OK, ("BLEController *NOT* Found"));
    m_lock.Unlock(MUTEX_CONTEXT);
    return false;
}

void* DaemonBLETransport::Run(void* arg)
{

    if (!btmActive) {
        return (void*)ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    QStatus status = ER_OK;
    m_lock.Lock(MUTEX_CONTEXT);

    while (!IsStopping()) {
        /*
         * Each time through the loop we create a set of events to wait on.
         * We need to wait on the stop event and all of the SocketFds of the
         * addresses and ports we are listening on.  If the list changes, the
         * code that does the change Alert()s this thread and we wake up and
         * re-evaluate the list of SocketFds.
         * Set reload to true to indicate that the set of events has been reloaded.
         */
        QCC_DbgPrintf(("DaemonBLETransport::Run()"));

        vector<Event*> checkEvents, signaledEvents;
        checkEvents.clear();
        checkEvents.push_back(&stopEvent);

        m_lock.Unlock(MUTEX_CONTEXT);
        status = Event::Wait(checkEvents, signaledEvents);
        if (ER_OK != status) {
            QCC_LogError(status, ("Event::Wait failed"));
            break;
        }
        m_lock.Lock(MUTEX_CONTEXT);
        for (vector<qcc::Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {
            if (*i == &stopEvent) {
                /* This thread has been alerted or is being stopped. Will check the IsStopping()
                 * flag when the while condition is encountered
                 */
                stopEvent.ResetEvent();
                continue;
            }
        }
        for (vector<qcc::Event*>::iterator i = checkEvents.begin(); i != checkEvents.end(); ++i) {
            Event* evt = *i;
            if (evt != &stopEvent) {
                delete evt;
            }
        }
    }
    m_lock.Unlock(MUTEX_CONTEXT);
    QCC_DbgPrintf(("DaemonBLETransport::Run() is exiting. status = %s", QCC_StatusText(status)));
    return (void*) status;
}
void DaemonBLETransport::Authenticated(DaemonBLEEndpoint& conn)
{
    QCC_DbgPrintf(("DaemonBLETransport::Authenticated()"));
    /*
     * If the transport is stopping, dont start the Tx and RxThreads.
     */
    if (stopping == true) {
        return;
    }
    /*
     * If Authenticated() is being called, it is as a result of the
     * authentication thread telling us that it has succeeded.  What we need to
     * do here is to try and Start() the endpoint which will spin up its TX and
     * RX threads and register the endpoint with the daemon router.  As soon as
     * we call Start(), we are transferring responsibility for error reporting
     * through endpoint ThreadExit() function.  This will percolate out our
     * EndpointExit function.  It will expect to find <conn> on the endpoint
     * list so we move it from the authList to the endpointList before calling
     * Start.
     */
    m_lock.Lock(MUTEX_CONTEXT);

    set<DaemonBLEEndpoint>::iterator i = find(m_authList.begin(), m_authList.end(), conn);

    if (i !=  m_authList.end()) {
        assert(i != m_authList.end() && "DaemonBLETransport::Authenticated(): Conn not on m_authList");

        /*
         * Note here that we have not yet marked the authState as AUTH_SUCCEEDED so
         * this is a point in time where the authState can be AUTH_AUTHENTICATING
         * and the endpoint can be on the endpointList and not the authList.
         */
        m_authList.erase(i);
    }
    m_endpointList.insert(conn);

    m_lock.Unlock(MUTEX_CONTEXT);

    conn->SetListener(this);

    conn->SetEpStarting();

    QStatus status = conn->Start();
    if (status != ER_OK) {
        QCC_LogError(status, ("DaemonBLETransport::Authenticated(): Failed to start DaemonBLEEndpoint"));
        /*
         * We were unable to start up the endpoint for some reason.  As soon as
         * we set this state to EP_FAILED, we are telling the server accept loop
         * that we tried to start the connection but it failed.  This connection
         * is now useless and is a candidate for cleanup.  This will be
         * prevented until authState changes from AUTH_AUTHENTICATING to
         * AUTH_SUCCEEDED.  This may be a little confusing, but the
         * authentication process has really succeeded but the endpoint start
         * has failed.  The combination of status in this case will be
         * AUTH_SUCCEEDED and EP_FAILED.  Once this state is detected by the
         * server accept loop it is then free to do anything it wants with the
         * connection, including deleting it.
         */
        conn->SetEpFailed();
    } else {
        /*
         * We were able to successfully start up the endpoint.  As soon as we
         * set this state to EP_STARTED, we are telling the server accept loop
         * that there are TX and RX threads wandering around in this endpoint.
         */
        conn->SetEpStarted();
    }
}
QStatus DaemonBLETransport::UntrustedClientStart() {
    /** Since UART implies physical security, always allow clients with ANONYMOUS authentication to connect. */
    return ER_OK;
}

QStatus DaemonBLETransport::Disconnect(const String& busName)
{
    QCC_DbgTrace(("DaemonBLETransport::Disconnect(busName = %s)", busName.c_str()));
    QStatus status(ER_BUS_BAD_TRANSPORT_ARGS);

    m_lock.Lock(MUTEX_CONTEXT);
    for (set<DaemonBLEEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        DaemonBLEEndpoint r = *i;
        if (r->GetUniqueName() == busName) {
            status = r->Stop();
        }
    }
    m_lock.Unlock(MUTEX_CONTEXT);
    return status;
}


RemoteEndpoint DaemonBLETransport::LookupEndpoint(const qcc::String& busName)
{
    RemoteEndpoint ep;
    m_lock.Lock(MUTEX_CONTEXT);
    for (set<DaemonBLEEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        DaemonBLEEndpoint r = *i;
        if (r->GetRemoteName() == busName) {
            ep = RemoteEndpoint::cast(r);
        }
    }
    m_lock.Unlock(MUTEX_CONTEXT);
    return ep;
}


void DaemonBLETransport::ReturnEndpoint(RemoteEndpoint& r) {
    DaemonBLEEndpoint ep = DaemonBLEEndpoint::cast(r);
    if (m_endpointList.find(ep) != m_endpointList.end()) {
        m_lock.Unlock(MUTEX_CONTEXT);
    }
}


QStatus DaemonBLETransport::StartListen()
{
    if (!btmActive) {
        return ER_BUS_TRANSPORT_NOT_AVAILABLE;
    }

    QStatus status;
    status = bleAccessor->StartConnectable();
    if (status == ER_OK) {
        QCC_DbgHLPrintf(("Listening"));
        Thread::Start();
    }
    return status;
}


void DaemonBLETransport::StopListen()
{
    Thread::Stop();
    Thread::Join();
    bleAccessor->StopConnectable();
    QCC_DbgHLPrintf(("Stopped listening"));
}

}


