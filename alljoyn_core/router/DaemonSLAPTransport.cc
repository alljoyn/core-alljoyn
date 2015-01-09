/**
 * @file
 * Implementation of DaemonSLAPTransport.
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
#include <qcc/UARTStream.h>
#include <qcc/SLAPStream.h>

#include "DaemonRouter.h"
#include "BusInternal.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "DaemonSLAPTransport.h"
#include "BusController.h"
#include "ConfigDB.h"

#define QCC_MODULE "DAEMON_SLAP"

using namespace std;
using namespace qcc;

namespace ajn {

const char* DaemonSLAPTransport::TransportName = "slap";

/*
 * An endpoint class to handle the details of authenticating a connection over the SLAP transport.
 */
class _DaemonSLAPEndpoint : public _RemoteEndpoint {

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

    _DaemonSLAPEndpoint(DaemonSLAPTransport* transport, BusAttachment& bus, bool incoming, const qcc::String connectSpec, UARTFd fd, uint32_t packetSize, uint32_t baudrate) :
        _RemoteEndpoint(bus, incoming, connectSpec, &m_stream, DaemonSLAPTransport::TransportName),
        m_transport(transport), m_authThread(this), m_fd(fd), m_authState(AUTH_INITIALIZED), m_epState(EP_INITIALIZED),
        m_timer("SLAPEp", true, 1, false, 10),
        m_rawStream(fd),
        m_stream(&m_rawStream, m_timer, packetSize, 4, baudrate),
        m_uartController(&m_rawStream, bus.GetInternal().GetIODispatch(), &m_stream)
    {
    }

    EndpointState GetEpState(void) { return m_epState; }
    ~_DaemonSLAPEndpoint() { }

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
    }

    void SetEpDone(void)
    {
        assert(m_epState == EP_FAILED || m_epState == EP_STOPPING);
        m_epState = EP_DONE;
    }
    AuthState GetAuthState(void) { return m_authState; }

    UARTFd GetFd() { return m_fd; }

    QStatus Stop();
    QStatus Join();

    virtual void ThreadExit(qcc::Thread* thread);

    QStatus SetIdleTimeouts(uint32_t& reqIdleTimeout, uint32_t& reqProbeTimeout)
    {
        uint32_t maxIdleProbes = m_transport->m_numHbeatProbes;

        /* If reqProbeTimeout == 0, Make no change to Probe timeout. */
        if (reqProbeTimeout == 0) {
            reqProbeTimeout = _RemoteEndpoint::GetProbeTimeout();
        } else if (reqProbeTimeout > m_transport->m_maxHbeatProbeTimeout) {
            /* Max allowed Probe timeout is m_maxHbeatProbeTimeout */
            reqProbeTimeout = m_transport->m_maxHbeatProbeTimeout;
        }

        /* If reqIdleTimeout == 0, Make no change to Idle timeout. */
        if (reqIdleTimeout == 0) {
            reqIdleTimeout = _RemoteEndpoint::GetIdleTimeout();
        }

        /* Requested link timeout must be >= m_minHbeatIdleTimeout */
        if (reqIdleTimeout < m_transport->m_minHbeatIdleTimeout) {
            reqIdleTimeout = m_transport->m_minHbeatIdleTimeout;
        }

        /* Requested link timeout must be <= m_maxHbeatIdleTimeout */
        if (reqIdleTimeout > m_transport->m_maxHbeatIdleTimeout) {
            reqIdleTimeout = m_transport->m_maxHbeatIdleTimeout;
        }

        return _RemoteEndpoint::SetIdleTimeouts(reqIdleTimeout, reqProbeTimeout, maxIdleProbes);
    }

  private:
    class AuthThread : public qcc::Thread {
      public:
        AuthThread(_DaemonSLAPEndpoint* ep) : Thread("auth"), m_endpoint(ep)  { }
      private:
        virtual qcc::ThreadReturn STDCALL Run(void* arg);

        _DaemonSLAPEndpoint* m_endpoint;
    };
    DaemonSLAPTransport* m_transport;        /**< The server holding the connection */
    AuthThread m_authThread;          /**< Thread used to do blocking calls during startup */

    UARTFd m_fd;                      /**< The file descriptor for UART */
    volatile AuthState m_authState;   /**< The state of the endpoint authentication process */
    volatile EndpointState m_epState; /**< The state of the endpoint authentication process */
    Timer m_timer;                    /**< Multipurpose timer for sending/resend/acks */
    UARTStream m_rawStream;           /**< The raw UART stream */
    SLAPStream m_stream;              /**< The SLAP stream used for Alljoyn communication */
    UARTController m_uartController;  /**< Controller responsible for reading from UART */
};

void _DaemonSLAPEndpoint::ThreadExit(qcc::Thread* thread)
{
    if (thread == &m_authThread) {
        if (m_authState == AUTH_INITIALIZED) {
            m_authState = AUTH_FAILED;
        }
        m_transport->Alert();
    }
    _RemoteEndpoint::ThreadExit(thread);
}

QStatus _DaemonSLAPEndpoint::Authenticate(void)
{
    QCC_DbgTrace(("DaemonSLAPEndpoint::Authenticate()"));
    m_timer.Start();

    m_uartController.Start();

    QStatus status = m_stream.ScheduleLinkControlPacket();
    /*
     * Start the authentication thread.
     */
    if (status == ER_OK) {
        status = m_authThread.Start(this, this);
    }
    if (status != ER_OK) {
        QCC_DbgPrintf(("DaemonSLAPEndpoint::Authenticate() Failed to authenticate endpoint"));
        m_authState = AUTH_FAILED;
        /* Alert the Run() thread to refresh the list of com ports to listen on. */
        m_transport->Alert();
    }
    return status;
}

void _DaemonSLAPEndpoint::AuthStop(void)
{
    QCC_DbgTrace(("DaemonSLAPEndpoint::AuthStop()"));

    /* Stop the controller only if Authentication failed */
    if (m_authState != AUTH_SUCCEEDED) {
        m_timer.Stop();
        m_uartController.Stop();
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

void _DaemonSLAPEndpoint::AuthJoin(void)
{
    QCC_DbgTrace(("DaemonSLAPEndpoint::AuthJoin()"));
    /* Join the controller only if Authentication failed */
    if (m_authState != AUTH_SUCCEEDED) {
        m_timer.Join();
        m_uartController.Join();
    }

    /*
     * Join the auth thread to stop executing.  All threads must be joined in
     * order to communicate their return status.  The auth thread is no exception.
     * This is done in a lazy fashion from the main server accept loop, where we
     * cleanup every time through the loop.
     */
    m_authThread.Join();
}
QStatus _DaemonSLAPEndpoint::Stop(void)
{
    QCC_DbgTrace(("DaemonSLAPEndpoint::Stop()"));
    m_timer.Stop();
    m_uartController.Stop();
    QStatus status = _RemoteEndpoint::Stop();

    return status;
}

QStatus _DaemonSLAPEndpoint::Join(void)
{
    QCC_DbgTrace(("DaemonSLAPEndpoint::Join()"));
    m_timer.Join();
    m_uartController.Join();
    QStatus status = _RemoteEndpoint::Join();
    return status;
}

void* _DaemonSLAPEndpoint::AuthThread::Run(void* arg)
{
    QCC_DbgPrintf(("DaemonSLAPEndpoint::AuthThread::Run()"));

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
    QCC_DbgPrintf(("DaemonSLAPEndpoint::AuthThread::Run() calling pullbytes"));
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
    /* Since the DaemonSLAPTransport allows untrusted clients, it must implement UntrustedClientStart and
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
    DaemonSLAPEndpoint dEp = DaemonSLAPEndpoint::wrap(m_endpoint);
    m_endpoint->m_transport->Authenticated(dEp);

    QCC_DbgPrintf(("DaemonSLAPEndpoint::AuthThread::Run(): Returning"));

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

DaemonSLAPTransport::DaemonSLAPTransport(BusAttachment& bus)
    : m_bus(bus), stopping(false)
{
    /*
     * We know we are daemon code, so we'd better be running with a daemon
     * router.  This is assumed elsewhere.
     */
    assert(bus.GetInternal().GetRouter().IsDaemon());
}


DaemonSLAPTransport::~DaemonSLAPTransport()
{
    Stop();
    Join();
}

QStatus DaemonSLAPTransport::Start()
{
    stopping = false;
    ConfigDB* config = ConfigDB::GetConfigDB();
    m_minHbeatIdleTimeout = config->GetLimit("slap_min_idle_timeout", MIN_HEARTBEAT_IDLE_TIMEOUT_DEFAULT);
    m_maxHbeatIdleTimeout = config->GetLimit("slap_max_idle_timeout", MAX_HEARTBEAT_IDLE_TIMEOUT_DEFAULT);
    m_defaultHbeatIdleTimeout = config->GetLimit("slap_default_idle_timeout", DEFAULT_HEARTBEAT_IDLE_TIMEOUT_DEFAULT);

    m_numHbeatProbes = HEARTBEAT_NUM_PROBES;
    m_maxHbeatProbeTimeout = config->GetLimit("slap_max_probe_timeout", MAX_HEARTBEAT_PROBE_TIMEOUT_DEFAULT);
    m_defaultHbeatProbeTimeout = config->GetLimit("slap_default_probe_timeout", DEFAULT_HEARTBEAT_PROBE_TIMEOUT_DEFAULT);

    QCC_DbgPrintf(("DaemonSLAPTransport: Using m_minHbeatIdleTimeout=%u, m_maxHbeatIdleTimeout=%u, m_numHbeatProbes=%u, m_defaultHbeatProbeTimeout=%u m_maxHbeatProbeTimeout=%u", m_minHbeatIdleTimeout, m_maxHbeatIdleTimeout, m_numHbeatProbes, m_defaultHbeatProbeTimeout, m_maxHbeatProbeTimeout));

    return Thread::Start();
}

QStatus DaemonSLAPTransport::Stop()
{
    stopping = true;
    /*
     * Tell the DaemonSLAPTransport::Run thread to shut down.
     */
    QStatus status = Thread::Stop();
    if (status != ER_OK) {
        QCC_LogError(status, ("DaemonSLAPTransport::Stop(): Failed to Stop() main thread"));
    }

    m_lock.Lock(MUTEX_CONTEXT);
    /*
     * Ask any authenticating endpoints to shut down and exit their threads.
     */
    for (set<DaemonSLAPEndpoint>::iterator i = m_authList.begin(); i != m_authList.end(); ++i) {
        DaemonSLAPEndpoint ep = *i;
        ep->AuthStop();
    }

    /*
     * Ask any running endpoints to shut down and exit their threads.
     */
    for (set<DaemonSLAPEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        DaemonSLAPEndpoint ep = *i;
        ep->Stop();
    }

    m_lock.Unlock(MUTEX_CONTEXT);

    return ER_OK;

}

QStatus DaemonSLAPTransport::Join()
{
    /*
     * Wait for the DaemonSLAPTransport::Run thread to exit.
     */
    QStatus status = Thread::Join();
    if (status != ER_OK) {
        QCC_LogError(status, ("DaemonSLAPTransport::Join(): Failed to Join() main thread"));
        return status;
    }

    m_lock.Lock(MUTEX_CONTEXT);
    /*
     * Any authenticating endpoints have been asked to shut down and exit their
     * authentication threads in a previously required Stop().  We need to
     * Join() all of these auth threads here.
     */
    set<DaemonSLAPEndpoint>::iterator it = m_authList.begin();
    while (it != m_authList.end()) {
        DaemonSLAPEndpoint ep = *it;
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
        DaemonSLAPEndpoint ep = *it;
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

QStatus DaemonSLAPTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, std::map<qcc::String, qcc::String>& argMap) const
{
    QStatus status = ParseArguments(DaemonSLAPTransport::TransportName, inSpec, argMap);
    qcc::String type = Trim(argMap["type"]);
    qcc::String dev = Trim(argMap["dev"]);
    qcc::String baud = Trim(argMap["baud"]);
    /*
     * databits, parity, and stopbits are optional.  They default to 8, none, and 1.
     */
    qcc::String databits = Trim(argMap["databits"]);
    if (databits.empty()) {
        argMap["databits"] = databits = "8";
    }
    qcc::String parity = Trim(argMap["parity"]);
    if (parity.empty()) {
        argMap["parity"] = parity = "none";
    }
    qcc::String stopbits = Trim(argMap["stopbits"]);
    if (stopbits.empty()) {
        argMap["stopbits"] = stopbits = "1";
    }

    if (status == ER_OK) {
        /*
         * Include only the type and dev in the outSpec.  The outSpec
         * is intended to be unique per device (i.e. you can't have two
         * connections to the same device with different parameters).
         */
        outSpec = "slap:";
        if (!type.empty()) {
            outSpec.append("type=");
            outSpec.append(type);
        } else {
            status = ER_BUS_BAD_TRANSPORT_ARGS;
        }
        if (!dev.empty()) {
            outSpec.append(",dev=");
            outSpec.append(dev);
        } else {
            status = ER_BUS_BAD_TRANSPORT_ARGS;
        }
    }

    return status;
}

QStatus DaemonSLAPTransport::StartListen(const char* listenSpec)
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
    if (serverArgs["type"] == "uart") {
        m_listenList.push_back(ListenEntry(normSpec, serverArgs));
    }
    m_lock.Unlock(MUTEX_CONTEXT);
    Thread::Alert();

    return ER_OK;

}

QStatus DaemonSLAPTransport::StopListen(const char* listenSpec)
{
    return ER_OK;
}

void DaemonSLAPTransport::EndpointExit(RemoteEndpoint& ep)
{
    /*
     * This is a callback driven from the remote endpoint thread exit function.
     * Our DaemonEndpoint inherits from class RemoteEndpoint and so when
     * either of the threads (transmit or receive) of one of our endpoints exits
     * for some reason, we get called back here.
     */
    QCC_DbgPrintf(("DaemonSLAPTransport::EndpointExit()"));
    DaemonSLAPEndpoint dEp = DaemonSLAPEndpoint::cast(ep);
    /* Remove the dead endpoint from the live endpoint list */
    m_lock.Lock(MUTEX_CONTEXT);
    QCC_DbgPrintf(("DaemonSLAPTransport::EndpointExit()setting stopping"));
    dEp->SetEpStopping();
    Thread::Alert();
    m_lock.Unlock(MUTEX_CONTEXT);
    ep->Invalidate();
}

void* DaemonSLAPTransport::Run(void* arg)
{

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
        QCC_DbgPrintf(("DaemonSLAPTransport::Run()"));

        UARTFd uartFd = -1;
        set<DaemonSLAPEndpoint>::iterator i = m_authList.begin();
        while (i != m_authList.end()) {
            uartFd = -1;
            DaemonSLAPEndpoint ep = *i;
            _DaemonSLAPEndpoint::AuthState authState = ep->GetAuthState();

            if (authState == _DaemonSLAPEndpoint::AUTH_FAILED) {
                /*
                 * The endpoint has failed authentication and the auth thread is
                 * gone or is going away.  Since it has failed there is no way this
                 * endpoint is going to be started so we can get rid of it as soon
                 * as we Join() the (failed) authentication thread.
                 */
                QCC_DbgPrintf(("DaemonSLAPTransport::Run(): Scavenging failed authenticator"));
                m_authList.erase(i);
                uartFd = ep->GetFd();
                m_lock.Unlock(MUTEX_CONTEXT);
                ep->AuthStop();
                ep->AuthJoin();
                m_lock.Lock(MUTEX_CONTEXT);
                i = m_authList.upper_bound(ep);
                for (list<ListenEntry>::iterator it = m_listenList.begin(); it != m_listenList.end(); it++) {

                    if (it->listenFd == uartFd) {
                        QCC_DbgPrintf(("DaemonSLAPTransport::Run(): Reenabling %s in the listenEvents", it->args["port"].c_str()));
                        it->listenFd = -1;
                        it->endpointStarted = false;
                        Thread::Alert();
                    }
                }
            } else {
                i++;
            }
        }
        i = m_endpointList.begin();
        while (i != m_endpointList.end()) {
            uartFd = -1;
            DaemonSLAPEndpoint ep = *i;

            _DaemonSLAPEndpoint::EndpointState endpointState = ep->GetEpState();
            /*
             * There are two possibilities for the disposition of the RX and
             * TX threads.  First, they were never successfully started.  In
             * this case, the epState will be EP_FAILED.  If we find this, we
             * can just remove the useless endpoint from the list and delete
             * it.  Since the threads were never started, they must not be
             * joined.
             */
            if (endpointState == _DaemonSLAPEndpoint::EP_FAILED) {
                m_endpointList.erase(i);
                uartFd = ep->GetFd();
                i = m_endpointList.upper_bound(ep);
            } else if (endpointState == _DaemonSLAPEndpoint::EP_STOPPING) {
                /*
                 * The second possibility for the disposition of the RX and
                 * TX threads is that they were successfully started but
                 * have been stopped for some reason, either because of a
                 * Disconnect() or a network error.  In this case, the
                 * epState will be EP_STOPPING, which was set in the
                 * EndpointExit function.  If we find this, we need to Join
                 * the endpoint threads, remove the endpoint from the
                 * endpoint list and delete it.  Note that we are calling
                 * the endpoint Join() to join the TX and RX threads and not
                 * the endpoint AuthJoin() to join the auth thread.
                 */
                m_endpointList.erase(i);
                uartFd = ep->GetFd();
                m_lock.Unlock(MUTEX_CONTEXT);
                ep->Stop();
                ep->Join();
                m_lock.Lock(MUTEX_CONTEXT);
                i = m_endpointList.upper_bound(ep);
            } else {
                i++;
            }
            if (uartFd != -1) {
                for (list<ListenEntry>::iterator it = m_listenList.begin(); it != m_listenList.end(); it++) {

                    if (it->listenFd == uartFd) {
                        QCC_DbgPrintf(("DaemonSLAPTransport::Run(): Reenabling back %s in the listenEvents", it->args["port"].c_str()));
                        it->listenFd = -1;
                        it->endpointStarted = false;
                        break;
                    }
                }
            }
        }
        vector<Event*> checkEvents, signaledEvents;
        checkEvents.clear();
        checkEvents.push_back(&stopEvent);

        for (list<ListenEntry>::iterator it = m_listenList.begin(); it != m_listenList.end(); ++it) {

            if (it->listenFd == -1) {
                /* open the port and set listen fd */
                UARTFd listenFd;
                QStatus uartStatus = UART(it->args["dev"], StringToU32(it->args["baud"]), StringToU32(it->args["databits"]),
                                          it->args["parity"], StringToU32(it->args["stopbits"]), listenFd);

                if (uartStatus == ER_OK && listenFd != -1) {
                    it->listenFd = listenFd;
                    checkEvents.push_back(new Event(it->listenFd, Event::IO_READ));
                    QCC_DbgPrintf(("DaemonSLAPTransport::Run(): Adding checkevent for %s to list of events", it->args["dev"].c_str()));
                } else {
                    QCC_LogError(uartStatus, ("DaemonSLAPTransport::Run(): Failed to open for %s", it->args["dev"].c_str()));
                    m_listenList.erase(it++);
                }
            } else if (!it->endpointStarted) {
                checkEvents.push_back(new Event(it->listenFd, Event::IO_READ));
                QCC_DbgPrintf(("DaemonSLAPTransport::Run(): Adding checkevent for %s to list of events", it->args["dev"].c_str()));
            }
        }

        m_lock.Unlock(MUTEX_CONTEXT);
        status = Event::Wait(checkEvents, signaledEvents);
        if (ER_OK != status) {
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
            } else {
                Event* e = *i;
                QCC_DbgPrintf(("DaemonSLAPTransport::Run(): Accepting connection "));
                for (list<ListenEntry>::iterator i = m_listenList.begin(); i != m_listenList.end(); i++) {
                    if (i->listenFd == e->GetFD()) {
                        i->endpointStarted = true;
                        static const bool truthiness = true;
                        DaemonSLAPTransport* ptr = this;
                        uint32_t packetSize = SLAP_DEFAULT_PACKET_SIZE;
                        uint32_t baudrate = StringToU32(i->args["baud"]);
                        QCC_DbgPrintf(("DaemonSLAPTransport::Run(): Creating endpoint for %s",  i->args["dev"].c_str()));
                        DaemonSLAPEndpoint conn(ptr, m_bus, truthiness, "slap", i->listenFd, packetSize, baudrate);
                        QCC_DbgPrintf(("DaemonSLAPTransport::Run(): Authenticating endpoint for %s",  i->args["dev"].c_str()));

                        status = conn->Authenticate();
                        if (status == ER_OK) {
                            m_authList.insert(conn);
                        }
                        break;
                    }
                }
                delete e;
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
    QCC_DbgPrintf(("DaemonSLAPTransport::Run() is exiting. status = %s", QCC_StatusText(status)));
    return (void*) status;
}
void DaemonSLAPTransport::Authenticated(DaemonSLAPEndpoint& conn)
{
    QCC_DbgPrintf(("DaemonSLAPTransport::Authenticated()"));
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

    set<DaemonSLAPEndpoint>::iterator i = find(m_authList.begin(), m_authList.end(), conn);
    assert(i != m_authList.end() && "DaemonSLAPTransport::Authenticated(): Conn not on m_authList");

    /*
     * Note here that we have not yet marked the authState as AUTH_SUCCEEDED so
     * this is a point in time where the authState can be AUTH_AUTHENTICATING
     * and the endpoint can be on the endpointList and not the authList.
     */
    m_authList.erase(i);
    m_endpointList.insert(conn);

    m_lock.Unlock(MUTEX_CONTEXT);

    conn->SetListener(this);

    conn->SetEpStarting();

    QStatus status = conn->Start(m_defaultHbeatIdleTimeout, m_defaultHbeatProbeTimeout, m_numHbeatProbes, m_maxHbeatProbeTimeout);
    if (status != ER_OK) {
        QCC_LogError(status, ("DaemonSLAPTransport::Authenticated(): Failed to start DaemonSLAPEndpoint"));
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
QStatus DaemonSLAPTransport::UntrustedClientStart() {
    /** Since UART implies physical security, always allow clients with ANONYMOUS authentication to connect. */
    return ER_OK;
}

}
