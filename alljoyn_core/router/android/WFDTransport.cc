/**
 * @file
 * WFDTransport is a specialization of class Transport for daemons talking over
 * Wi-Fi Direct links and doing Wi_Fi Direct pre-association service discovery.
 */

/******************************************************************************
 * Copyright (c) 2012, 2014, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>
#include <qcc/IPAddress.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/IfConfig.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/TransportMask.h>
#include <alljoyn/Session.h>

#include "BusInternal.h"
#include "ConfigDB.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "ns/IpNameService.h"
#include "WFDTransport.h"

#define QCC_MODULE "WFD"

using namespace std;
using namespace qcc;

const uint32_t WFD_LINK_TIMEOUT_PROBE_ATTEMPTS       = 1;
const uint32_t WFD_LINK_TIMEOUT_PROBE_RESPONSE_DELAY = 10;
const uint32_t WFD_LINK_TIMEOUT_MIN_LINK_TIMEOUT     = 40;

namespace ajn {
class _WFDEndpoint;

/**
 * Name of transport used in transport specs.
 */
const char* WFDTransport::TransportName = "wfd";

/*
 * An endpoint class to handle the details of authenticating a connection in a
 * way that avoids denial of service attacks.
 */
class _WFDEndpoint : public _RemoteEndpoint {
  public:
    /**
     * There are three threads that can be running around in this data
     * structure.  An auth thread is run before the endpoint is started in order
     * to handle the security stuff that must be taken care of before messages
     * can start passing.  This enum reflects the states of the authentication
     * process and the state can be found in m_authState.  Once authentication
     * is complete, the auth thread must go away, but it must also be joined,
     * which is indicated by the AUTH_DONE state.  The other threads are the
     * endpoint RX and TX threads, which are dealt with by the EndpointState.
     */
    enum AuthState {
        AUTH_ILLEGAL = 0,
        AUTH_INITIALIZED,    /**< This endpoint structure has been allocated but no auth thread has been run */
        AUTH_AUTHENTICATING, /**< We have spun up an authentication thread and it has begun running our user function */
        AUTH_FAILED,         /**< The authentication has failed and the authentication thread is exiting immidiately */
        AUTH_SUCCEEDED,      /**< The auth process (Establish) has succeeded and the connection is ready to be started */
        AUTH_DONE,           /**< The auth thread has been successfully shut down and joined */
    };

    /**
     * There are three threads that can be running around in this data
     * structure.  Two threads, and RX thread and a TX thread are used to pump
     * messages through an endpoint.  These threads cannot be run until the
     * authentication process has completed.  This enum reflects the states of
     * the endpoint RX and TX threads and can be found in m_epState.  The auth
     * thread is dealt with by the AuthState enum above.  These threads must be
     * joined when they exit, which is indicated by the EP_DONE state.
     */
    enum EndpointState {
        EP_ILLEGAL = 0,
        EP_INITIALIZED,      /**< This endpoint structure has been allocated but not used */
        EP_FAILED,           /**< Starting the RX and TX threads has failed and this endpoint is not usable */
        EP_STARTED,          /**< The RX and TX threads have been started (they work as a unit) */
        EP_STOPPING,         /**< The RX and TX threads are stopping (have run ThreadExit) but have not been joined */
        EP_DONE              /**< The RX and TX threads have been shut down and joined */
    };

    /**
     * Connections can either be created as a result of a Connect() or an Accept().
     * If a connection happens as a result of a connect it is the active side of
     * a connection.  If a connection happens because of an Accpet() it is the
     * passive side of a connection.  This is important because of reference
     * counting of bus-to-bus endpoints.
     */
    enum SideState {
        SIDE_ILLEGAL = 0,
        SIDE_INITIALIZED,    /**< This endpoint structure has been allocated but don't know if active or passive yet */
        SIDE_ACTIVE,         /**< This endpoint is the active side of a connection */
        SIDE_PASSIVE         /**< This endpoint is the passive side of a connection */
    };

    _WFDEndpoint(WFDTransport* transport,
                 BusAttachment& bus,
                 bool incoming,
                 const qcc::String connectSpec,
                 qcc::SocketFd sock,
                 const qcc::IPAddress& ipAddr,
                 uint16_t port,
                 qcc::String guid)
        : _RemoteEndpoint(bus, incoming, connectSpec, &m_stream, "wfd"),
        m_transport(transport),
        m_sideState(SIDE_INITIALIZED),
        m_authState(AUTH_INITIALIZED),
        m_epState(EP_INITIALIZED),
        m_tStart(qcc::Timespec(0)),
        m_authThread(this),
        m_stream(sock),
        m_ipAddr(ipAddr),
        m_port(port),
        m_guid(guid),
        m_wasSuddenDisconnect(!incoming) { }

    virtual ~_WFDEndpoint() { }

    void SetStartTime(qcc::Timespec tStart) { m_tStart = tStart; }
    qcc::Timespec GetStartTime(void) { return m_tStart; }
    QStatus Authenticate(void);
    void AuthStop(void);
    void AuthJoin(void);
    const qcc::IPAddress& GetIPAddress() { return m_ipAddr; }
    uint16_t GetPort() { return m_port; }
    qcc::String GetGuid() { return m_guid; }

    SideState GetSideState(void) { return m_sideState; }

    void SetActive(void)
    {
        m_sideState = SIDE_ACTIVE;
    }

    void SetPassive(void)
    {
        m_sideState = SIDE_PASSIVE;
    }


    AuthState GetAuthState(void) { return m_authState; }

    void SetAuthDone(void)
    {
        m_authState = AUTH_DONE;
    }

    void SetAuthenticating(void)
    {
        m_authState = AUTH_AUTHENTICATING;
    }

    EndpointState GetEpState(void) { return m_epState; }

    void SetEpFailed(void)
    {
        m_epState = EP_FAILED;
    }

    void SetEpStarted(void)
    {
        m_epState = EP_STARTED;
    }

    void SetEpStopping(void)
    {
        assert(m_epState == EP_STARTED);
        m_epState = EP_STOPPING;
    }

    void SetEpDone(void)
    {
        assert(m_epState == EP_FAILED || m_epState == EP_STOPPING);
        m_epState = EP_DONE;
    }

    bool IsSuddenDisconnect() { return m_wasSuddenDisconnect; }
    void SetSuddenDisconnect(bool val) { m_wasSuddenDisconnect = val; }

    QStatus SetLinkTimeout(uint32_t& linkTimeout)
    {
        QStatus status = ER_OK;
        if (linkTimeout > 0) {
            uint32_t to = max(linkTimeout, WFD_LINK_TIMEOUT_MIN_LINK_TIMEOUT);
            to -= WFD_LINK_TIMEOUT_PROBE_RESPONSE_DELAY * WFD_LINK_TIMEOUT_PROBE_ATTEMPTS;
            status = _RemoteEndpoint::SetLinkTimeout(to, WFD_LINK_TIMEOUT_PROBE_RESPONSE_DELAY, WFD_LINK_TIMEOUT_PROBE_ATTEMPTS);
            if ((status == ER_OK) && (to > 0)) {
                linkTimeout = to + WFD_LINK_TIMEOUT_PROBE_RESPONSE_DELAY * WFD_LINK_TIMEOUT_PROBE_ATTEMPTS;
            }

        } else {
            _RemoteEndpoint::SetLinkTimeout(0, 0, 0);
        }
        return status;
    }

    /*
     * Return true if the auth thread is STARTED, RUNNING or STOPPING.  A true
     * response means the authentication thread is in a state that indicates
     * a possibility it might touch the endpoint data structure.  This means
     * don't delete the endpoint if this method returns true.  This method
     * indicates nothing about endpoint rx and tx thread state.
     */
    bool IsAuthThreadRunning(void)
    {
        return m_authThread.IsRunning();
    }

  private:
    class AuthThread : public qcc::Thread {
      public:
        AuthThread(_WFDEndpoint* conn) : Thread("auth"), ep(conn) { }
      private:
        virtual qcc::ThreadReturn STDCALL Run(void* arg);
        _WFDEndpoint* ep;

    };

    WFDTransport* m_transport;  /**< The server holding the connection */
    volatile SideState m_sideState;   /**< Is this an active or passive connection */
    volatile AuthState m_authState;   /**< The state of the endpoint authentication process */
    volatile EndpointState m_epState; /**< The state of the endpoint authentication process */
    qcc::Timespec m_tStart;           /**< Timestamp indicating when the authentication process started */
    AuthThread m_authThread;          /**< Thread used to do blocking calls during startup */
    qcc::SocketStream m_stream;       /**< Stream used by authentication code */
    qcc::IPAddress m_ipAddr;          /**< Remote IP address. */
    uint16_t m_port;                  /**< Remote port. */
    qcc::String m_guid;               /**< The GUID of the remote daemon corresponding to this endpoint */
    bool m_wasSuddenDisconnect;       /**< If true, assumption is that any disconnect is unexpected due to lower level error */
};

QStatus _WFDEndpoint::Authenticate(void)
{
    QCC_DbgTrace(("WFDEndpoint::Authenticate()"));
    /*
     * Start the authentication thread.
     */
    QStatus status = m_authThread.Start(this);
    if (status != ER_OK) {
        m_authState = AUTH_FAILED;
    }
    return status;
}

void _WFDEndpoint::AuthStop(void)
{
    QCC_DbgTrace(("WFDEndpoint::AuthStop()"));

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

void _WFDEndpoint::AuthJoin(void)
{
    QCC_DbgTrace(("WFDEndpoint::AuthJoin()"));

    /*
     * Join the auth thread to stop executing.  All threads must be joined in
     * order to communicate their return status.  The auth thread is no exception.
     * This is done in a lazy fashion from the main server accept loop, where we
     * cleanup every time through the loop.
     */
    m_authThread.Join();
}

void* _WFDEndpoint::AuthThread::Run(void* arg)
{
    QCC_DbgTrace(("WFDEndpoint::AuthThread::Run()"));



    ep->m_authState = AUTH_AUTHENTICATING;

    /*
     * We're running an authentication process here and we are cooperating with
     * the main server thread.  This thread is running in an object that is
     * allocated on the heap, and the server is managing these objects so we
     * need to coordinate getting all of this cleaned up.
     *
     * There is a state variable that only we write.  The server thread only
     * reads this variable, so there are no data sharing issues.  If there is an
     * authentication failure, this thread sets that state variable to
     * AUTH_FAILED and then exits.  The server holds a list of currently
     * authenticating connections and will look for AUTH_FAILED connections when
     * it runs its Accept loop.  If it finds one, it will AuthJoin() this
     * thread.  Since we set AUTH_FAILED immediately before exiting, there will
     * be no problem having the server block waiting for the Join() to complete.
     * We fail authentication here and let the server clean up after us, lazily.
     *
     * If we succeed in the authentication process, we set the state variable
     * to AUTH_SUCEEDED and then call back into the server telling it that we are
     * up and running.  It needs to take us off of the list of authenticating
     * connections and put us on the list of running connections.  This thread
     * will quickly go away and will be replaced by the RX and TX threads of
     * the running RemoteEndpoint.
     *
     * If we are running an authentication process, we are probably ultimately
     * blocked on a socket.  We expect that if the server is asked to shut
     * down, it will run through its list of authenticating connections and
     * AuthStop() each one.  That will cause a thread Stop() which should unblock
     * all of the reads and return an error which will eventually pop out here
     * with an authentication failure.
     *
     * Finally, if the server decides we've spent too much time here and we are
     * actually a denial of service attack, it can close us down by doing an
     * AuthStop() on the authenticating endpoint.  This will do a thread Stop()
     * on the auth thread of the endpoint which will pop out of here as an
     * authentication failure as well.  The only ways out of this method must be
     * with state = AUTH_FAILED or state = AUTH_SUCCEEDED.
     */
    uint8_t byte;
    size_t nbytes;

    /*
     * Eat the first byte of the stream.  This is required to be zero by the
     * DBus protocol.  It is used in the Unix socket implementation to carry
     * out-of-band capabilities, but is discarded here.  We do this here since
     * it involves a read that can block.
     */
    QStatus status = ep->m_stream.PullBytes(&byte, 1, nbytes);
    if ((status != ER_OK) || (nbytes != 1) || (byte != 0)) {
        ep->m_stream.Close();
        QCC_LogError(status, ("Failed to read first byte from stream"));

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
        ep->m_authState = AUTH_FAILED;
        return (void*)ER_FAIL;
    }

    /* Initialize the features for this endpoint */
    ep->GetFeatures().isBusToBus = false;
    ep->GetFeatures().isBusToBus = false;
    ep->GetFeatures().handlePassing = false;

    qcc::String authName;
    qcc::String redirection;

    /* Run the actual connection authentication code. */
    QCC_DbgTrace(("WFDEndpoint::AuthThread::Run(): Establish()"));
    status = ep->Establish("ANONYMOUS", authName, redirection);
    if (status != ER_OK) {
        ep->m_stream.Close();
        QCC_LogError(status, ("Failed to establish WFD endpoint"));

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
        ep->m_authState = AUTH_FAILED;
        return (void*)status;
    }

    /*
     * Tell the transport that the authentication has succeeded and that it can
     * now bring the connection up.
     */
    QCC_DbgTrace(("WFDEndpoint::AuthThread::Run(): Authenticated()"));
    WFDEndpoint wfdEp = WFDEndpoint::wrap(ep);
    ep->m_transport->Authenticated(wfdEp);

    QCC_DbgTrace(("WFDEndpoint::AuthThread::Run(): Returning"));

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
    ep->m_authState = AUTH_SUCCEEDED;
    return (void*)status;
}

WFDTransport::WFDTransport(BusAttachment& bus)
    : Thread("WFDTransport"), m_bus(bus), m_stopping(false), m_listener(0),
    m_isAdvertising(false), m_isDiscovering(false), m_isListening(false), m_isNsEnabled(false),
    m_listenPort(0), m_p2pNsAcquired(false), m_p2pCmAcquired(false), m_ipNsAcquired(false)
{
    QCC_DbgTrace(("WFDTransport::WFDTransport()"));
    /*
     * We know we are daemon code, so we'd better be running with a daemon
     * router.  This is assumed elsewhere.
     */
    assert(m_bus.GetInternal().GetRouter().IsDaemon());
}

WFDTransport::~WFDTransport()
{
    QCC_DbgTrace(("WFDTransport::~WFDTransport()"));
    Stop();
    Join();
}

void WFDTransport::Authenticated(WFDEndpoint& conn)
{
    QCC_DbgTrace(("WFDTransport::Authenticated()"));

    /*
     * If the transport is stopping, dont start the Tx and RxThreads.
     */
    if (m_stopping == true) {
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
    m_endpointListLock.Lock(MUTEX_CONTEXT);

    set<WFDEndpoint>::iterator i = find(m_authList.begin(), m_authList.end(), conn);
    assert(i != m_authList.end() && "WFDTransport::Authenticated(): Conn not on m_authList");

    /*
     * Note here that we have not yet marked the authState as AUTH_SUCCEEDED so
     * this is a point in time where the authState can be AUTH_AUTHENTICATING
     * and the endpoint can be on the endpointList and not the authList.
     */
    m_authList.erase(i);
    m_endpointList.insert(conn);

    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    conn->SetListener(this);
    QStatus status = conn->Start();
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::Authenticated(): Failed to start WFD endpoint"));
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

QStatus WFDTransport::Start()
{
    QCC_DbgTrace(("WFDTransport::Start()"));

    /*
     * We rely on the status of the server accept thead as the primary
     * gatekeeper.
     *
     * A true response from IsRunning tells us that the server accept thread is
     * STARTED, RUNNING or STOPPING.
     *
     * When a thread is created it is in state INITIAL.  When an actual tread is
     * spun up as a result of Start(), it becomes STARTED.  Just before the
     * user's Run method is called, the thread becomes RUNNING.  If the Run
     * method exits, the thread becomes STOPPING.  When the thread is Join()ed
     * it becomes DEAD.
     *
     * IsRunning means that someone has called Thread::Start() and the process
     * has progressed enough that the thread has begun to execute.  If we get
     * multiple Start() calls calls on multiple threads, this test may fail to
     * detect multiple starts in a failsafe way and we may end up with multiple
     * server accept threads running.  We assume that since Start() requests
     * come in from our containing transport list it will not allow concurrent
     * start requests.
     */
    if (IsRunning()) {
        QCC_LogError(ER_BUS_BUS_ALREADY_STARTED, ("WFDTransport::Start(): Already started"));
        return ER_BUS_BUS_ALREADY_STARTED;
    }

    m_stopping = false;

    /*
     * Get the guid from the bus attachment which will act as the globally unique
     * ID of the daemon.
     */
    qcc::String guidStr = m_bus.GetInternal().GetGlobalGUID().ToString();

    /*
     * We're a WFD transport in the AllJoyn sense, so we are going to have to
     * use the P2P name service and P2P connection manager to get our Wi-Fi
     * requests done for us.  This means we are going to have to Acquire() and
     * Release() the corresponding singletons.
     *
     * Start() will legally be called exactly once, but Stop() and Join() may be
     * called multiple times.  Since we are essentially reference counting the
     * name service and connection manager singletons with calls to Acquire and
     * Release, we need to make sure that we Release exactly as many times as we
     * Acquire.  We just use a flag to mark whether or not we have done each
     * operation exactly one time.
     */
    m_p2pNsAcquired = false;
    m_p2pCmAcquired = false;
    m_ipNsAcquired = false;

    /*
     * Start the server accept loop through the thread base class.  This will
     * close or open the IsRunning() gate we use to control access to our
     * public API.
     */
    return Thread::Start();
}

QStatus WFDTransport::Stop(void)
{
    QCC_DbgTrace(("WFDTransport::Stop()"));

    /*
     * It is legal to call Stop() more than once, so it must be possible to
     * call Stop() on a stopped transport.
     */
    m_stopping = true;

    /*
     * Tell the P2P name service to stop calling us back if it's there (we may
     * get called more than once in the chain of destruction) so the pointer
     * to the name service is not required to be valid -- i.e., it may be NULL
     * from a previous call.
     */
    if (m_p2pNsAcquired) {
        P2PNameService::Instance().SetCallback(TRANSPORT_WFD, NULL);
    }

    /*
     * Tell the P2P connection manager to stop calling us back as well over its
     * state-changed callback.
     */
    if (m_p2pCmAcquired) {
        P2PConMan::Instance().SetStateCallback(NULL);
        P2PConMan::Instance().SetNameCallback(NULL);
    }

    /*
     * Tell the server accept loop thread to shut down through the thead
     * base class.
     */
    QStatus status = Thread::Stop();
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::Stop(): Failed to Stop() server thread"));
        return status;
    }

    m_endpointListLock.Lock(MUTEX_CONTEXT);

    /*
     * Ask any authenticating ACTIVE endpoints to shut down and return to the
     * caller.  By its presence on the m_activeEndpointsThreadList, we know that
     * an external (from the point of this module) thread is authenticating and
     * is probably blocked waiting for the other side to respond.  We can't call
     * Stop() to stop that thread from running, we have to Alert() it to make it
     * pop out of its blocking calls.
     */
    for (set<Thread*>::iterator i = m_activeEndpointsThreadList.begin(); i != m_activeEndpointsThreadList.end(); ++i) {
        (*i)->Alert();
    }

    /*
     * Ask any authenticating endpoints to shut down and exit their threads.  By its
     * presence on the m_authList, we know that the endpoint is authenticating and
     * the authentication thread has responsibility for dealing with the endpoint
     * data structure.  We call Stop() to stop that thread from running.  The
     * endpoint Rx and Tx threads will not be running yet.
     */
    for (set<WFDEndpoint>::iterator i = m_authList.begin(); i != m_authList.end(); ++i) {
        WFDEndpoint ep = *i;
        ep->AuthStop();
    }

    /*
     * Ask any running endpoints to shut down and exit their threads.  By its
     * presence on the m_endpointList, we know that authentication is compete and
     * the Rx and Tx threads have responsibility for dealing with the endpoint
     * data structure.  We call Stop() to stop those threads from running.  Since
     * the connnection is on the m_endpointList, we know that the authentication
     * thread has handed off responsibility.
     */
    for (set<WFDEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        WFDEndpoint ep = *i;
        ep->Stop();
    }

    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    return ER_OK;
}

QStatus WFDTransport::Join(void)
{
    QCC_DbgTrace(("WFDTransport::Join()"));

    /*
     * It is legal to call Join() more than once, so it must be possible to
     * call Join() on a joined transport and also on a joined name service.
     */
    QStatus status = Thread::Join();
    if (status != ER_OK) {
        return status;
    }

    /*
     * We expect that all of our calls to Start(), Stop() and Join() are
     * orchestrated through the transport list and will ultimately come from
     * only one thread.  The place that we are setting these flags is in the
     * main accept loop thread, but we just joined that thread immediately
     * above, so it cannot be running now.  So we're not concerned about
     * multithreading and we just look at our acquired flags and set them
     * without "protection."
     */
    if (m_p2pNsAcquired) {
        P2PNameService::Instance().Release();
        m_p2pNsAcquired = false;
    }
    if (m_p2pCmAcquired) {
        P2PConMan::Instance().Release();
        m_p2pCmAcquired = false;
    }
    if (m_ipNsAcquired) {
        IpNameService::Instance().Release();
        m_ipNsAcquired = false;
    }

    /*
     * A required call to Stop() that needs to happen before this Join will ask
     * all of the endpoints to stop; and will also cause any authenticating
     * endpoints to stop.  We still need to wait here until all of the threads
     * running in those endpoints actually stop running.
     *
     * Since Stop() is a request to stop, and this is what has ultimately been
     * done to both authentication threads and Rx and Tx threads, it is possible
     * that a thread is actually running after the call to Stop().  If that
     * thead happens to be an authenticating endpoint, it is possible that an
     * authentication actually completes after Stop() is called.  This will move
     * a connection from the m_authList to the m_endpointList, so we need to
     * make sure we wait for all of the connections on the m_authList to go away
     * before we look for the connections on the m_endpointlist.
     */
    m_endpointListLock.Lock(MUTEX_CONTEXT);

    /*
     * Any authenticating endpoints have been asked to shut down and exit their
     * authentication threads in a previously required Stop().  We need to
     * Join() all of these auth threads here.
     */
    set<WFDEndpoint>::iterator it = m_authList.begin();
    while (it != m_authList.end()) {
        WFDEndpoint ep = *it;
        m_authList.erase(it);
        m_endpointListLock.Unlock(MUTEX_CONTEXT);
        ep->AuthJoin();
        m_endpointListLock.Lock(MUTEX_CONTEXT);
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
        WFDEndpoint ep = *it;
        m_endpointList.erase(it);
        m_endpointListLock.Unlock(MUTEX_CONTEXT);
        ep->Join();
        m_endpointListLock.Lock(MUTEX_CONTEXT);
        it = m_endpointList.upper_bound(ep);
    }

    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    m_stopping = false;
    return ER_OK;
}

QStatus WFDTransport::GetListenAddresses(const SessionOpts& opts, std::vector<qcc::String>& busAddrs) const
{
    QCC_DbgTrace(("WFDTransport::GetListenAddresses()"));

    /*
     * We are given a session options structure that defines the kind of
     * transports that are being sought.  WFD provides reliable traffic as
     * understood by the session options, so we only return someting if
     * the traffic type is TRAFFIC_MESSAGES or TRAFFIC_RAW_RELIABLE.  It's
     * not an error if we don't match, we just don't have anything to offer.
     */
    if (opts.traffic != SessionOpts::TRAFFIC_MESSAGES && opts.traffic != SessionOpts::TRAFFIC_RAW_RELIABLE) {
        QCC_DbgPrintf(("WFDTransport::GetListenAddresses(): traffic mismatch"));
        return ER_OK;
    }

    /*
     * The other session option that we need to filter on is the transport
     * bitfield.  There is a single bit in a TransportMask that corresponds
     * to a transport in the AllJoyn sense.  We are TRANSPORT_WFD.
     */
    if (!(opts.transports & TRANSPORT_WFD)) {
        QCC_DbgPrintf(("WFDTransport::GetListenAddresses(): transport mismatch"));
        return ER_OK;
    }

    /*
     * The abstract goal of a GetListenAddresses() call is to generate a list of
     * interfaces that could possibly be used by a remote daemon to connect to
     * this instance of our WFD transport.  The interfaces are returned in the
     * form of bus addresses and are shipped back to the remote side to be used
     * in a WFDTransport::Connect() there.  Since a connect spec for a WFD
     * transport is just a guid=xxx, the only meaningful thing we could possibly
     * return is our daemon's guid.
     */
    qcc::String busAddr = qcc::String(GetTransportName()) + qcc::String(":guid=") + m_bus.GetInternal().GetGlobalGUID().ToString();
    busAddrs.push_back(busAddr);
    return ER_OK;
}

void WFDTransport::EndpointExit(RemoteEndpoint& ep)
{
    QCC_DbgTrace(("WFDTransport::EndpointExit()"));

    /*
     * This is a callback driven from the remote endpoint thread exit function.
     * Our WFDEndpoint inherits from class RemoteEndpoint and so when
     * either of the threads (transmit or receive) of one of our endpoints exits
     * for some reason, we get called back here.  We only get called if either
     * the tx or rx thread exits, which implies that they have been run.  It
     * turns out that in the case of an endpoint receiving a connection, it
     * means that authentication has succeeded.  In the case of an endpoint
     * doing the connect, the EndpointExit may have resulted from an
     * authentication error since authentication is done in the context of the
     * Connect()ing thread and may be reported through EndpointExit.
     */
    WFDEndpoint tep = WFDEndpoint::cast(ep);

    /*
     * The endpoint can exit if it was asked to by us in response to a
     * Disconnect() from higher level code, or if it got an error from the
     * underlying transport.  We need to notify upper level code if the
     * disconnect is due to an event from the transport.
     */
    if (m_listener && tep->IsSuddenDisconnect()) {
        m_listener->BusConnectionLost(tep->GetConnectSpec());
    }

    /*
     * If this is an active connection, what has happened is that the reference
     * count on the underlying RemoteEndpoint has been decremented to zero and
     * the Stop() function of the endpoint has been called.  This means that
     * we are done with the endpoint and it should be cleaned up.  Marking
     * the connection as active prevented the passive side cleanup, so we need
     * to deal with cleanup now.
     */
    tep->SetPassive();

    /*
     * Mark the endpoint as no longer running.  Since we are called from
     * the RemoteEndpoint ThreadExit routine, we know it has stopped both
     * the RX and TX threads and we can Join them in a timely manner.
     */
    tep->SetEpStopping();

    /*
     * Wake up the server accept loop so that it deals with our passing immediately.
     */
    Alert();
}

void WFDTransport::ManageEndpoints(Timespec tTimeout)
{
    QCC_DbgTrace(("WFDTransport::ManageEndpoints()"));

    m_endpointListLock.Lock(MUTEX_CONTEXT);

    /*
     * This is the one place where we deal with the management (deletion) of
     * endpoints.  This is the place where we have to decide what to do when
     * the last of the endpoints we are managing is destroyed.
     *
     * In the case of a client application, when a Connect() is performed, we
     * arrange with the P2P Helper Service to bring up a Wi-Fi Direct STA
     * connection to the service device.  You might think that when the last
     * endpoint is freed, you would see a corresponding Disconnect() but you
     * would be mistaken.  Disconnect() is defined for transports, but it turns
     * out that it is never called.  When the daemon is done with a link to an
     * external entity, it simply tears down the endpoint.  Therefore, we need
     * to detect when to tear down the underlying Wi-Fi Direct connection here.
     *
     * In Connect() we need to be careful to only force the actual link
     * establishment for the first connection attempt to a remote device since
     * we can have more than one layer four-based (TCP) endpoint running a TCP
     * connection over a layer two-based (MAC) Wi-Fi Direct link.  Here we need
     * to be careful to only tear down the actual link when the last endpoint
     * goes away.
     *
     * So, we need to make sure that the link is kept up 1) before any endpoints
     * are actually created; 2) while endpoints exist; and  then take down the
     * link when the last of the endpoints have exited and been cleaned up.
     *
     * Another way of saying this is that we only send a ReleaseLink to the
     * P2P Helper Service if there are no endpoints left and we've cleaned up
     * at least one here in ManageEndpoints.
     *
     * If we are representing a service application, we enter a ready state when
     * we advertise the service and when a remote application/daemon connects,
     * we enter the connected state on reception of an OnLinkEstablished().
     *
     * We can actually be running both as a client and a service if we are
     * hosting a pure peer-to-peer application.  In this case, if all of the
     * endpoints are torn down, we have to be careful to re-enter the ready
     * state appropriate to a service and not the idle state appropriate to a
     * client.
     */
    bool endpointCleaned = false;

    /*
     * Run through the list of connections on the authList and cleanup
     * any that are no longer running or are taking too long to authenticate
     * (we assume a denial of service attack in this case).
     */
    set<WFDEndpoint>::iterator i = m_authList.begin();
    while (i != m_authList.end()) {
        WFDEndpoint ep = *i;
        _WFDEndpoint::AuthState authState = ep->GetAuthState();

        if (authState == _WFDEndpoint::AUTH_FAILED) {
            /*
             * The endpoint has failed authentication and the auth thread is
             * gone or is going away.  Since it has failed there is no way this
             * endpoint is going to be started so we can get rid of it as soon
             * as we Join() the (failed) authentication thread.
             */
            QCC_DbgPrintf(("WFDTransport::ManageEndpoints(): Scavenging failed authenticator"));
            m_authList.erase(i);
            endpointCleaned = true;
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            ep->AuthJoin();
            m_endpointListLock.Lock(MUTEX_CONTEXT);
            i = m_authList.upper_bound(ep);
            continue;
        }

        Timespec tNow;
        GetTimeNow(&tNow);

        if (ep->GetStartTime() + tTimeout < tNow) {
            /*
             * This endpoint is taking too long to authenticate.  Stop the
             * authentication process.  The auth thread is still running, so we
             * can't just delete the connection, we need to let it stop in its
             * own time.  What that thread will do is to set AUTH_FAILED and
             * exit.  We will then clean it up the next time through this loop.
             * In the hope that the thread can exit and we can catch its exit
             * here and now, we take our thread off the OS ready list (Sleep)
             * and let the other thread run before looping back.
             */
            QCC_DbgPrintf(("WFDTransport::ManageEndpoints(): Scavenging slow authenticator"));
            ep->AuthStop();
            qcc::Sleep(1);
        }
        ++i;
    }

    /*
     * We've handled the authList, so now run through the list of connections on
     * the endpointList and cleanup any that are no longer running or Join()
     * authentication threads that have successfully completed.
     */
    i = m_endpointList.begin();
    while (i != m_endpointList.end()) {
        WFDEndpoint ep = *i;

        /*
         * We are only managing passive connections here, or active connections
         * that are done and are explicitly ready to be cleaned up.
         */
        _WFDEndpoint::SideState sideState = ep->GetSideState();
        if (sideState == _WFDEndpoint::SIDE_ACTIVE) {
            ++i;
            continue;
        }

        _WFDEndpoint::AuthState authState = ep->GetAuthState();
        _WFDEndpoint::EndpointState endpointState = ep->GetEpState();

        if (authState == _WFDEndpoint::AUTH_SUCCEEDED) {
            /*
             * The endpoint has succeeded authentication and the auth thread is
             * gone or is going away.  Take this opportunity to join the auth
             * thread.  Since the auth thread promised not to touch the state
             * after setting AUTH_SUCCEEEDED, we can safely change the state
             * here since we now own the conn.  We do this through a method call
             * to enable this single special case where we are allowed to set
             * the state.
             */
            QCC_DbgPrintf(("WFDTransport::ManageEndpoints(): Scavenging failed authenticator"));
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            ep->AuthJoin();
            ep->SetAuthDone();
            m_endpointListLock.Lock(MUTEX_CONTEXT);
            i = m_endpointList.upper_bound(ep);
            continue;
        }

        /*
         * There are two possibilities for the disposition of the RX and
         * TX threads.  First, they were never successfully started.  In
         * this case, the epState will be EP_FAILED.  If we find this, we
         * can just remove the useless endpoint from the list and delete
         * it.  Since the threads were never started, they must not be
         * joined.
         */
        if (endpointState == _WFDEndpoint::EP_FAILED) {
            m_endpointList.erase(i);
            endpointCleaned = true;
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            m_endpointListLock.Lock(MUTEX_CONTEXT);
            i = m_endpointList.upper_bound(ep);
            continue;
        }

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
        if (endpointState == _WFDEndpoint::EP_STOPPING) {
            m_endpointList.erase(i);
            endpointCleaned = true;
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            ep->Join();
            m_endpointListLock.Lock(MUTEX_CONTEXT);
            i = m_endpointList.upper_bound(ep);
            continue;
        }
        ++i;
    }

    /*
     * As mentioned in the lengthy comment above, if we've cleaned up an
     * endpoint and there are no more left (in the list of currently active
     * endpoints and the list of currently authenticating endpoitns), then we
     * need to release any resources we may have reserved as a result of the
     * now unneeded (possibly already released) Wi-Fi Direct link.
     *
     * If we think were only a client (using the link in STA mode), we just go
     * idle by calling DestroyTemporaryNetwork().
     *
     * However, if we think we are a service (if we are advertising) we need to
     * free the Wi-Fi Group resource by calling DestroyTemporaryNetwork() but we
     * also need to make sure to enter the ready state by calling
     * CreateTemporaryNetwork() in order to be ready to accept new connections
     * from possible clients in the future.
     *
     * Note that the m_isAdvertising test below is not a failsafe test for
     * advertisement, since AdvertiseName() and CancelAdvertiseName() calls may
     * be percolating through the main thread, but if we get it wrong here, when
     * those percolating calls are actually executed, they will get it right.
     *
     * To further complicate things, we will also get an OnLinkLost() signal
     * down in the P2PConMan when the last wireless link of our Wi-Fi interface
     * is dropped.  This happens if the single STA connection drops or if the
     * last STA disconnects from the interface if in GO mode.  The important
     * thing to realize is that at this level we are dealing with endpoint
     * (TCP/IP -- layers three and four) connections being lost not Wi-Fi
     * (layers one and two) connections being lost, so if the link remains up
     * we need to cause it to be torn down.  If the link dropping has caused
     * the endpoint exits, these events can happen in unfortunate sequences.
     *
     * The bottom line is that our last endpoint has exited so we need to
     * release our resources and get back into the appropriate state.  This may
     * be the ready state if we are advertising a service or the idle state if
     * we are not.
     *
     * This situation is full of possible race conditions since the low level
     * (layer two) link lost messages are being routed out to the Android
     * Application Framework and back over an AllJoyn service, but the high
     * level (layer four) connection lost messages are routed up from the kernel
     * through TCP directly here.  This means that the ordering of the events
     * EndpointExit() and OnLinkLost() is not deterministic at all.
     */
    if (endpointCleaned && m_endpointList.empty() && m_authList.empty()) {
        QCC_DbgPrintf(("WFDTransport::ManageEndpoints(): DestroyTemporaryNetwork()"));
        QStatus status = P2PConMan::Instance().DestroyTemporaryNetwork();
        if (status != ER_OK) {
            QCC_LogError(status, ("WFDTransport::ManageEndpoints(): Unable to destroy temporary network"));
        }

        if (m_isAdvertising) {
            qcc::String localDevice("");
            QStatus status = P2PConMan::Instance().CreateTemporaryNetwork(localDevice, P2PConMan::DEVICE_SHOULD_BE_GO);
            if (status != ER_OK) {
                QCC_LogError(status, ("WFDTransport::ManageEndpoints(): Unable to recreate temporary network (SHOULD_BE_GO)"));
            }
        }
    }

    m_endpointListLock.Unlock(MUTEX_CONTEXT);
}

void* WFDTransport::Run(void* arg)
{
    QCC_DbgTrace(("WFDTransport::Run()"));

    /*
     * We need to find the defaults for our connection limits.  These limits
     * can be specified in the configuration database with corresponding limits
     * used for DBus.  If any of those are present, we use them, otherwise we
     * provide some hopefully reasonable defaults.
     */
    ConfigDB* config = ConfigDB::GetConfigDB();

    /*
     * tTimeout is the maximum amount of time we allow incoming connections to
     * mess about while they should be authenticating.  If they take longer
     * than this time, we feel free to disconnect them as deniers of service.
     */
    Timespec tTimeout = config->GetLimit("auth_timeout", ALLJOYN_AUTH_TIMEOUT_DEFAULT);

    /*
     * maxAuth is the maximum number of incoming connections that can be in
     * the process of authenticating.  If starting to authenticate a new
     * connection would mean exceeding this number, we drop the new connection.
     */
    uint32_t maxAuth = config->GetLimit("max_incomplete_connections", ALLJOYN_MAX_INCOMPLETE_CONNECTIONS_WFD_DEFAULT);

    /*
     * maxConn is the maximum number of active connections possible over the
     * WFD transport.  If starting to process a new connection would mean
     * exceeding this number, we drop the new connection.
     */
    uint32_t maxConn = config->GetLimit("max_completed_connections", ALLJOYN_MAX_COMPLETED_CONNECTIONS_WFD_DEFAULT);

    QStatus status = ER_OK;

    while (!IsStopping()) {
        /*
         * Each time through the loop we create a set of events to wait on.
         * We need to wait on the stop event and all of the SocketFds of the
         * addresses and ports we are listening on.  If the list changes, the
         * code that does the change Alert()s this thread and we wake up and
         * re-evaluate the list of SocketFds.
         */
        m_listenFdsLock.Lock(MUTEX_CONTEXT);
        vector<Event*> checkEvents, signaledEvents;
        checkEvents.push_back(&stopEvent);
        for (list<pair<qcc::String, SocketFd> >::const_iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
            checkEvents.push_back(new Event(i->second, Event::IO_READ));
        }
        m_listenFdsLock.Unlock(MUTEX_CONTEXT);

        /*
         * We have our list of events, so now wait for something to happen
         * on that list (or get alerted).
         */
        signaledEvents.clear();

        status = Event::Wait(checkEvents, signaledEvents);
        if (ER_OK != status) {
            QCC_LogError(status, ("Event::Wait failed"));
            break;
        }

        /*
         * We're back from our Wait() so one of three things has happened.  Our
         * thread has been asked to Stop(), our thread has been Alert()ed, or
         * one of the socketFds we are listening on for connecte events has
         * becomed signalled.
         *
         * If we have been asked to Stop(), or our thread has been Alert()ed,
         * the stopEvent will be on the list of signalled events.  The
         * difference can be found by a call to IsStopping() which is found
         * above.  An alert means that a request to start or stop listening
         * on a given address and port has been queued up for us.
         */
        for (vector<Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {
            /*
             * In order to rationalize management of resources, we manage the
             * various lists in one place on one thread.  This thread is a
             * convenient victim, so we do it here.
             */
            ManageEndpoints(tTimeout);

            /*
             * Reset an existing Alert() or Stop().  If it's an alert, we
             * will deal with looking for the incoming listen requests at
             * the bottom of the server loop.  If it's a stop we will
             * exit the next time through the top of the server loop.
             */
            if (*i == &stopEvent) {
                stopEvent.ResetEvent();
                continue;
            }

            /*
             * Since the current event is not the stop event, it must reflect at
             * least one of the SocketFds we are waiting on for incoming
             * connections.  Go ahead and Accept() the new connection on the
             * current SocketFd.
             */
            IPAddress remoteAddr;
            uint16_t remotePort;
            SocketFd newSock;

            while (true) {
                status = Accept((*i)->GetFD(), remoteAddr, remotePort, newSock);
                if (status != ER_OK) {
                    break;
                }

                QCC_DbgPrintf(("WFDTransport::Run(): Accepting connection newSock=%d", newSock));

                QCC_DbgPrintf(("WFDTransport::Run(): maxAuth == %d", maxAuth));
                QCC_DbgPrintf(("WFDTransport::Run(): maxConn == %d", maxConn));
                QCC_DbgPrintf(("WFDTransport::Run(): mAuthList.size() == %d", m_authList.size()));
                QCC_DbgPrintf(("WFDTransport::Run(): mEndpointList.size() == %d", m_endpointList.size()));
                assert(m_authList.size() + m_endpointList.size() <= maxConn);

                /*
                 * Do we have a slot available for a new connection?  If so, use
                 * it.
                 */
                m_endpointListLock.Lock(MUTEX_CONTEXT);
                if ((m_authList.size() < maxAuth) && (m_authList.size() + m_endpointList.size() < maxConn)) {
                    static const bool truthiness = true;
                    WFDEndpoint conn(this, m_bus, truthiness, "", newSock, remoteAddr, remotePort,
                                     m_bus.GetInternal().GetGlobalGUID().ToString());
                    conn->SetPassive();
                    Timespec tNow;
                    GetTimeNow(&tNow);
                    conn->SetStartTime(tNow);
                    /*
                     * By putting the connection on the m_authList, we are
                     * transferring responsibility for the connection to the
                     * Authentication thread.  Therefore, we must check that the
                     * thread actually started running to ensure the handoff
                     * worked.  If it didn't we need to deal with the connection
                     * here.  Since there are no threads running we can just
                     * pitch the connection.
                     */
                    std::pair<std::set<WFDEndpoint>::iterator, bool> ins = m_authList.insert(conn);
                    status = conn->Authenticate();
                    if (status != ER_OK) {
                        m_authList.erase(ins.first);
                        conn->Invalidate();
                    }

                    m_endpointListLock.Unlock(MUTEX_CONTEXT);
                } else {
                    m_endpointListLock.Unlock(MUTEX_CONTEXT);
                    qcc::Shutdown(newSock);
                    qcc::Close(newSock);
                    status = ER_AUTH_FAIL;
                    QCC_LogError(status, ("WFDTransport::Run(): No slot for new connection"));
                }
            }

            /*
             * Accept returns ER_WOULDBLOCK when all of the incoming connections have been handled
             */
            if (ER_WOULDBLOCK == status) {
                status = ER_OK;
            }

            if (status != ER_OK) {
                QCC_LogError(status, ("WFDTransport::Run(): Error accepting new connection. Ignoring..."));
            }
        }

        /*
         * We're going to loop back and create a new list of checkEvents that
         * reflect the current state, so we need to delete the checkEvents we
         * created on this iteration.
         */
        for (vector<Event*>::iterator i = checkEvents.begin(); i != checkEvents.end(); ++i) {
            if (*i != &stopEvent) {
                delete *i;
            }
        }

        /*
         * If we're not stopping, we always check for queued requests to start
         * and stop listening on address and port combinations (listen specs).
         * We need to change the state of the sockets in one place (here) to
         * ensure that we don't ever end up with Events that contain references
         * to closed Sockets; and this is the one place were we can be assured
         * we don't have those Events live.
         *
         * When we loop back to the top of the server accept loop, we will
         * re-evaluate the list of listenFds and create new Events based on the
         * current state of the list (after we remove or add anything here).
         *
         * We also take this opportunity to run the state machine that deals
         * with whether or not to enable WFD listeners and the name service
         * UDP listeners.
         */
        RunListenMachine();
    }

    /*
     * If we're stopping, it is our responsibility to clean up the list of FDs
     * we are listening to.  Since we've gotten a Stop() and are exiting the
     * server loop, and FDs are added in the server loop, this is the place to
     * get rid of them.  We don't have to take the list lock since a Stop()
     * request to the WFDTransport is required to lock out any new
     * requests that may possibly touch the listen FDs list.
     */
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    for (list<pair<qcc::String, SocketFd> >::iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
        qcc::Shutdown(i->second);
        qcc::Close(i->second);
    }
    m_listenFds.clear();
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("WFDTransport::Run is exiting status=%s", QCC_StatusText(status)));
    return (void*) status;
}

/*
 * The purpose of this code is really to ensure that we don't have any listeners
 * active on Android systems if we have no ongoing advertisements.  This is to
 * satisfy a requirement driven from the Android Compatibility Test Suite (CTS)
 * which fails systems that have processes listening for WFD connections when
 * the test is run.
 *
 * Listeners and advertisements are interrelated.  In order to Advertise a
 * service, the name service must have an endpoint to include in its
 * advertisements; and there must be at least one listener running and ready to
 * receive connections before telling the name service to advertise.
 *
 * Discovery requests do not require listeners be present per se before being
 * forwarded to the name service.  A discovery request will ulitmately lead to a
 * bus-to-bus connection once a remote daemon has been discovered; but the local
 * side will always start the connection.  Sessions throw a bit of a monkey
 * wrench in the works, though.  Since a JoinSession request is sent to the
 * (already connected) remote daemon and it decides what to do, we don't want to
 * arbitrarily constrain the remote daemon by disallowing it to try and connect
 * back to the local daemon.  For this reason, we do require listeners to be
 * present before discovery starts.
 *
 * So the goal is to not have active listeners in the system unless there are
 * outstanding advertisements or discovery requests, but we cannot have
 * outstanding advertisements or discovery requests until there are active
 * listeners.  Some care is obviously required here to accomplish this
 * seemingly inconsistent behavior.
 *
 * We call the state of no outstanding advertisements and not outstanding
 * discovery requests "Name Service Quiescent".  In this case, the name service
 * must be disabled so that it doesn't interact with the network and cause a CTS
 * failure.  As soon as a either a discovery request or an advertisement request
 * is started, we need to enable the name service to recieve and send network
 * packets, which will cause the daemon process to begin listening on the name
 * service well-known UDP port.
 *
 * Before an advertisement or a discovery request can acutally be sent over the
 * wire, we must start a listener which will receive connection requests, and
 * we must provide the name service with endpoint information that it can include
 * in its advertisement.  So, from the name service and network perspective,
 * listens must preceed advertisements.
 *
 * In order to accomplish the CTS requirements, however, advertisements must
 * preceed listens.  It turns out that this is how the high-level system wants
 * to work.  Essentually, the system calls StartListen at the beginning of time
 * (when the daemon is first brought up) and it calls StopListen at the end of
 * time (when the daemon is going down).  Advertisements and discovery requests
 * come and go in between as clients and services come up and go down.
 *
 * To deal with this time-inversion, we save a list of all listen requests, a
 * list of all advertisement requests and a list of all discovery requests.  At
 * the beginning of time we get one or more StartListen calls and save the
 * listen specs, but do not actually do the socket operations to start the
 * corresponding socket-level listens.  When the first advertisement or
 * discovery request comes in from the higher-level code, we first start all of
 * the saved listens and then enable the name service and ask it to start
 * advertising or discovering as appropriate.  Further advertisements and
 * discovery requests are also saved, but the calls to the name service are
 * passed through when it is not quiescent.
 *
 * We keep track of the disable advertisement and discovery calls as well.  Each
 * time an advertisement or discover operation is disabled, we remove the
 * corresponding entry in the associated list.  As soon as all advertisements
 * and discovery operations are disabled, we disable the name service and remove
 * our WFD listeners, and therefore remove all listeners from the system.  Since
 * we have a saved a list of listeners, they can be restarted if another
 * advertisement or discovery request comes in.
 *
 * We need to do all of this in one place (here) to make it easy to keep the
 * state of the transport (us) and the name service consistent.  We are
 * basically a state machine handling the following transitions:
 *
 *   START_LISTEN_INSTANCE: An instance of a StartListen() has happened so we
 *     need to add the associated listen spec to our list of listeners and be
 *     ready for a subsequent advertisement.  We expect these to happen at the
 *     beginning of time; but there is nothing preventing a StartListen after we
 *     start advertising.  In this case we need to execute the start listen.
 *
 *   STOP_LISTEN_INSTANCE: An instance of a StopListen() has happened so we need
 *     to remove the listen spec from our list of listeners.  We expect these to
 *     happen at the end of time; but there is nothing preventing a StopListen
 *     at any other time.  In this case we need to execute the stop listen and
 *     remove the specified listener immediately
 *
 *   ENABLE_ADVERTISEMENT_INSTANCE: An instance of an EnableAdvertisement() has
 *     happened.  If there are no other ongoing advertisements, we need to
 *     enable the stored listeners, pass the endpoint information down to the
 *     name servcie, enable the name service communication with the outside
 *     world if it is disabled and finally pass the advertisement down to the
 *     name service.  If there are other ongoing advertisements we just pass
 *     down the new advertisement.  It is an AllJoyn system programming error to
 *     start advertising before starting at least one listen.
 *
 *   DISABLE_ADVERTISEMENT_INSTANCE: An instance of a DisableAdvertisement()
 *     call has happened.  We always want to pass the corresponding Cancel down
 *     to the name service.  If we decide that this is the last of our ongoing
 *     advertisements, we need to continue and disable the name service from
 *     talking to the outside world.  For completeness, we remove endpoint
 *     information from the name service.  Finally, we shut down our WFD
 *     transport listeners.
 *
 *   ENABLE_DISCOVERY_INSTANCE: An instance of an EnableDiscovery() has
 *     happened.  This is a fundamentally different request than an enable
 *     advertisement.  We don't need any listeners to be present in order to do
 *     discovery, but the name service must be enabled so it can send and
 *     receive WHO-HAS packets.  If the name service communications are
 *     disabled, we need to enable them.  In any case we pass the request down
 *     to the name service.
 *
 *   DISABLE_DISCOVERY_INSTANCE: An instance of a DisableDiscovery() call has
 *     happened.  There is no corresponding disable call in the name service,
 *     but we do have to decide if we want to disable the name service to keep
 *     it from listening.  We do so if this is the last discovery instance and
 *     there are no other advertisements.
 *
 * There are four member variables that reflect the state of the transport
 * and name service with respect to this code:
 *
 *   m_isListening:  The list of listeners is reflected by currently listening
 *     sockets.  We have network infrastructure in place to receive inbound
 *     connection requests.
 *
 *   m_isNsEnabled:  The name service is up and running and listening on its
 *     sockets for incoming requests.
 *
 *   m_isAdvertising: The list of advertisements is reflected by current
 *     advertisements in the name service.  if we are m_isAdvertising then
 *     m_isNsEnabled must be true.
 *
 *   m_isDiscovering: The list of discovery requests has been sent to the name
 *     service.  if we are m_isDiscovering then m_isNsEnabled must be true.
 */
void WFDTransport::RunListenMachine(void)
{
    QCC_DbgTrace(("WFDTransport::RunListenMachine()"));

    while (m_listenRequests.empty() == false) {
        QCC_DbgPrintf(("WFDTransport::RunListenMachine(): Do request."));

        /*
         * Pull a request to do a listen request off of the queue of requests.
         * These requests relate to starting and stopping discovery and
         * advertisements; and also whether or not to listen for inbound
         * connections.
         */
        m_listenRequestsLock.Lock(MUTEX_CONTEXT);
        ListenRequest listenRequest = m_listenRequests.front();
        m_listenRequests.pop();
        m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

        /*
         * Do some consistency checks to make sure we're not confused about what
         * is going on.
         *
         * First, if we are not listening, then we had better not think we're
         * advertising or discovering.  If we are not listening, then the name
         * service must not be enabled and sending or responding to external
         * daemons.
         */
        if (m_isListening == false) {
            assert(m_isAdvertising == false);
            assert(m_isDiscovering == false);
            assert(m_isNsEnabled == false);
        }

        /*
         * If we think the name service is enabled, it had better think it is
         * enabled.  It must be enabled either because we are advertising or we
         * are discovering.  If we are advertising or discovering, then there
         * must be listeners waiting for connections as a result of those
         * advertisements or discovery requests.  If there are listeners, then
         * there must be a non-zero listenPort.
         */
        if (m_isNsEnabled) {
            assert(m_isAdvertising || m_isDiscovering);
            assert(m_isListening);
            assert(m_listenPort);
        }

        /*
         * If we think we are advertising, we'd better have an entry in the
         * advertisements list to make us advertise, and there must be listeners
         * waiting for inbound connections as a result of those advertisements.
         * If we are advertising the name service had better be enabled.
         */
        if (m_isAdvertising) {
            assert(!m_advertising.empty());
            assert(m_isListening);
            assert(m_listenPort);
            assert(m_isNsEnabled);
        }

        /*
         * If we are discovering, we'd better have an entry in the discovering
         * list to make us discover, and there must be listeners waiting for
         * inbound connections as a result of session operations driven by those
         * discoveries.  If we are discovering the name service had better be
         * enabled.
         */
        if (m_isDiscovering) {
            assert(!m_discovering.empty());
            assert(m_isListening);
            assert(m_listenPort);
            assert(m_isNsEnabled);
        }

        /*
         * We're a WFD transport in the AllJoyn sense, so we also have to use
         * Wi-Fi pre-association service discovery.  This means we are going to
         * have to use the P2P (layer two) name service, and if we find a
         * service, we are going to have to use the P2P connection manager.
         * Since we have an advertisement/discovery call that drove us here we
         * know that the DBus interface they require must be ready.  This is a
         * convenient time to Acquire those singletons, since they must just
         * be ready before either a discovery or advertisement operation is
         * actually attempted.  Since we drive that process from immediately
         * below, we're good.
         */
        switch (listenRequest.m_requestOp) {
        case ENABLE_ADVERTISEMENT_INSTANCE:
        case DISABLE_ADVERTISEMENT_INSTANCE:
        case ENABLE_DISCOVERY_INSTANCE:
        case DISABLE_DISCOVERY_INSTANCE:
            if (m_p2pNsAcquired == false) {
                P2PNameService::Instance().Acquire(&m_bus, m_bus.GetInternal().GetGlobalGUID().ToString());
                P2PNameService::Instance().SetCallback(TRANSPORT_WFD, new CallbackImpl<WFDTransport, void, const qcc::String&, qcc::String&, uint8_t> (this, &WFDTransport::P2PNameServiceCallback));
                m_p2pNsAcquired = true;
            }
            if (m_p2pCmAcquired == false) {
                P2PConMan::Instance().Acquire(&m_bus, m_bus.GetInternal().GetGlobalGUID().ToString());
                P2PConMan::Instance().SetStateCallback(new CallbackImpl<WFDTransport, void, P2PConMan::LinkState, const qcc::String&> (this, &WFDTransport::P2PConManStateCallback));
                P2PConMan::Instance().SetNameCallback(new CallbackImpl<WFDTransport, void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint8_t>(this, &WFDTransport::P2PConManNameCallback));


                m_p2pCmAcquired = true;
            }
            if (m_ipNsAcquired == false) {
                IpNameService::Instance().Acquire(m_bus.GetInternal().GetGlobalGUID().ToString());
                m_ipNsAcquired = true;
            }
            break;

        default:
            break;
        }

        /*
         * Now that are sure we have a consistent view of the world, let's do
         * what needs to be done.
         */
        switch (listenRequest.m_requestOp) {
        case START_LISTEN_INSTANCE:
            StartListenInstance(listenRequest);
            break;

        case STOP_LISTEN_INSTANCE:
            StopListenInstance(listenRequest);
            break;

        case ENABLE_ADVERTISEMENT_INSTANCE:
            EnableAdvertisementInstance(listenRequest);
            break;

        case DISABLE_ADVERTISEMENT_INSTANCE:
            DisableAdvertisementInstance(listenRequest);
            break;

        case ENABLE_DISCOVERY_INSTANCE:
            EnableDiscoveryInstance(listenRequest);
            break;

        case DISABLE_DISCOVERY_INSTANCE:
            DisableDiscoveryInstance(listenRequest);
            break;
        }
    }
}

void WFDTransport::StartListenInstance(ListenRequest& listenRequest)
{
    QCC_DbgTrace(("WFDTransport::StartListenInstance()"));

    /*
     * We have a new StartListen request, so save the listen spec so we
     * can restart the listen if we stop advertising.
     */
    NewListenOp(START_LISTEN, listenRequest.m_requestParam);

    /*
     * If we're running on Windows, we always start listening immediately
     * since Windows uses WFD as the client to daemon communication link.
     *
     * On other operating systems (i.e. Posix) we use unix domain sockets and so
     * we can delay listening to passify the Android Compatibility Test Suite.
     * We do this unless we have any outstanding advertisements or discovery
     * operations in which case we start up the listens immediately.
     */
    if (m_isAdvertising || m_isDiscovering) {
        DoStartListen(listenRequest.m_requestParam);
    }
}

void WFDTransport::StopListenInstance(ListenRequest& listenRequest)
{
    QCC_DbgTrace(("WFDTransport::StopListenInstance()"));

    /*
     * We have a new StopListen request, so we need to remove this
     * particular listen spec from our lists so it will not be
     * restarted.
     */
    bool empty = NewListenOp(STOP_LISTEN, listenRequest.m_requestParam);

    /*
     * If we have just removed the last listener, we have a problem if
     * we have active advertisements.  This is because we will be
     * advertising soon to be non-existent endpoints.  The question is,
     * what do we want to do about it.  We could just ignore it since
     * since clients receiving advertisements may just try to connect to
     * a non-existent endpoint and fail.  It does seem better to log an
     * error and then cancel any outstanding advertisements since they
     * are soon to be meaningless.
     */
    if (empty && m_isAdvertising) {
        QCC_LogError(ER_FAIL, ("WFDTransport::StopListenInstance(): No listeners with outstanding advertisements."));
        for (list<qcc::String>::iterator i = m_advertising.begin(); i != m_advertising.end(); ++i) {
            if (m_p2pNsAcquired) {
                P2PNameService::Instance().CancelAdvertiseName(TRANSPORT_WFD, *i);
            }
            if (m_ipNsAcquired) {
                IpNameService::Instance().CancelAdvertiseName(TRANSPORT_WFD, *i, TRANSPORT_WFD);
            }
        }
    }

    /*
     * Execute the code that will actually tear down the specified
     * listening endpoint.  Note that we always stop listening
     * immediately since that is Good (TM) from a power and CTS point of
     * view.  We only delay starting to listen.
     */
    DoStopListen(listenRequest.m_requestParam);
}

void WFDTransport::EnableAdvertisementInstance(ListenRequest& listenRequest)
{
    QCC_DbgTrace(("WFDTransport::EnableAdvertisementInstance()"));

    /*
     * We have a new advertisement request to deal with.  The first
     * order of business is to save the well-known name away for
     * use later.
     */
    bool isFirst;
    NewAdvertiseOp(ENABLE_ADVERTISEMENT, listenRequest.m_requestParam, isFirst);

    /*
     * If it turned out that is the first advertisement on our list, we need to
     * prepare before actually doing the advertisement.
     */
    if (isFirst) {

        /*
         * If we don't have any listeners up and running, we need to get them
         * up.  If this is a Windows box, the listeners will start running
         * immediately and will never go down, so they may already be running.
         */
        if (!m_isListening) {
            for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
                DoStartListen(*i);
                assert(m_listenPort);
                m_isListening = true;
            }
        }

        /*
         * We can only enable the requested advertisement if there is something
         * listening inbound connections on.  Therefore, we should only enable
         * the name service if there is a listener.  This catches the case where
         * there was no StartListen() done before the first advertisement.
         */
        if (m_isListening) {
            if (!m_isNsEnabled) {
                /*
                 * We have to enable the P2P name service to get pre-association
                 * service discovery working, and we have to enable the IP name
                 * service to allow clients to discover our address and port
                 * information.
                 */
                P2PNameService::Instance().Enable(TRANSPORT_WFD);
                std::map<qcc::String, uint16_t> listenPortMap;
                listenPortMap["*"] = m_listenPort;
                IpNameService::Instance().Enable(TRANSPORT_WFD, listenPortMap, 0, std::map<qcc::String, uint16_t>(), 0, true, false, false, false);
                m_isNsEnabled = true;
            }
        } else {
            QCC_LogError(ER_FAIL, ("WFDTransport::EnableAdvertisementInstance(): Advertise with no WFD listeners"));
            return;
        }
    }

    /*
     * We think we're ready to send the advertisement.  Are we really?
     */
    assert(m_isListening);
    assert(m_listenPort);
    assert(m_isNsEnabled);

    /*
     * We're going to need the P2P name service and connection manager to make
     * this happen, and we're going to need the IP name service to respond when
     * the other side looks for an IP address and port, so they'd better be
     * started and ready to go.
     */
    assert(P2PNameService::Instance().Started() && "WFDTransport::EnableAdvertisementInstance(): P2PNameService not started");
    assert(P2PConMan::Instance().Started() && "WFDTransport::EnableAdvertisementInstance(): P2PNameService not started");
    assert(IpNameService::Instance().Started() && "WFDTransport::EnableAdvertisementInstance(): IpNameService not started");

    /*
     * If we're going to advertise a name, we must tell the underlying P2P
     * system that we want to be a group owner (GO).  The model we use is
     * that services become GO and clients become station nodes (STA).
     *
     * There is no management of the underlying device in Android, and that
     * is where we are going to be running.  Basically, the last caller in
     * gets to write over any previous callers.
     *
     * This means that if we have an existing client (STA) connection to another
     * device, and the user decides to advertise a service, we will summarily
     * kill the STA connection and prepare the device for incoming connections
     * to the service as a GO.
     *
     * This means that if we are advertising/hosting a service and a client
     * decides to connect to another device, we will summarily kill the GO
     * connection and try to connect to the STA.
     *
     * To try and keep the user experience simple and understandable, we only
     * allow one service to advertise over WFD at a time and we only allow one
     * client to connect over WFD at a time.
     *
     * So, every time we advertise, we just take out anything else that may
     * be there.
     */
    qcc::String localDevice("");
    QStatus status = P2PConMan::Instance().CreateTemporaryNetwork(localDevice, P2PConMan::DEVICE_SHOULD_BE_GO);
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::EnableAdvertisementInstance(): Unable to create a GO side network"));
        return;
    }

    /*
     * We need to advertise the name over the IP name service because that is
     * how the other side is going to determine addressing information for the
     * ultimately desired TCP/UDP connection.
     *
     * When we start advertising here, there will be no temporary network
     * actually created and therefore there is no network to send advertisements
     * out over.  We can't do anything with respect to opening an interface in
     * the name service since we won't know the interface name until the link is
     * actually established.  We are just enabling the advertisements here.
     *
     * When a client eventually connects to the group, the connection manager
     * will get an OnLinkEstablished signal from the P2P Helper service.  This
     * signal provides the interface name, and the signal is plumbed back to us
     * via the callback from the P2PConMan.  We do the call to open the name
     * service interface in our callback handler.  When this happens, the
     * responses to FindAdvertiseName (who-has) requests will be answered and
     * our advertisements will begin percolating out to the other (client) side.
     */
    status = IpNameService::Instance().AdvertiseName(TRANSPORT_WFD, listenRequest.m_requestParam, false, TRANSPORT_WFD);
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::EnableAdvertisementInstance(): Failed to advertise \"%s\"", listenRequest.m_requestParam.c_str()));
        return;
    }

    /*
     * We need to advertise the name over the P2P name service because that is
     * the reason for being of this transport -- Wi-Fi Direct pre-association
     * service discovery.
     */
    status = P2PNameService::Instance().AdvertiseName(TRANSPORT_WFD, listenRequest.m_requestParam);
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::EnableAdvertisementInstance(): Failed to advertise \"%s\"", listenRequest.m_requestParam.c_str()));
        return;
    }

    QCC_DbgPrintf(("WFDTransport::EnableAdvertisementInstance(): Done"));
    m_isAdvertising = true;
}

void WFDTransport::DisableAdvertisementInstance(ListenRequest& listenRequest)
{
    QCC_DbgTrace(("WFDTransport::DisableAdvertisementInstance()"));

    /*
     * We have a new disable advertisement request to deal with.  The first
     * order of business is to remove the well-known name from our saved list.
     */
    bool isFirst;
    bool isEmpty = NewAdvertiseOp(DISABLE_ADVERTISEMENT, listenRequest.m_requestParam, isFirst);

    QStatus status = IpNameService::Instance().CancelAdvertiseName(TRANSPORT_WFD, listenRequest.m_requestParam, TRANSPORT_WFD);
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::DisableAdvertisementInstance(): Failed to IP Cancel \"%s\"", listenRequest.m_requestParam.c_str()));
    }

    status = P2PNameService::Instance().CancelAdvertiseName(TRANSPORT_WFD, listenRequest.m_requestParam);
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::DisableAdvertisementInstance(): Failed to P2P Cancel \"%s\"", listenRequest.m_requestParam.c_str()));
    }

    /*
     * If it turns out that this was the last advertisement on our list, we need
     * to think about disabling our listeners and turning off the name service.
     * We only to this if there are no discovery instances in progress.
     */
    if (isEmpty && !m_isDiscovering) {

        /*
         * Since the cancel advertised name has been sent, we can disable the
         * P2P name service.  Telling the IP name service we don't have any
         * enabled ports tells it to disable.
         */
        P2PNameService::Instance().Disable(TRANSPORT_WFD);
        std::map<qcc::String, uint16_t> listenPortMap;
        listenPortMap["*"] = m_listenPort;
        IpNameService::Instance().Enable(TRANSPORT_WFD, listenPortMap, 0, std::map<qcc::String, uint16_t>(), 0, false, false, false, false);

        m_isNsEnabled = false;

        /*
         * If we had the name service running, we must have had listeners
         * waiting for connections due to the name service.  We need to stop
         * them all now.
         */
        for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
            DoStopListen(*i);
        }

        m_isListening = false;
        m_listenPort = 0;

        QStatus status = P2PConMan::Instance().DestroyTemporaryNetwork();
        if (status != ER_OK) {
            QCC_LogError(status, ("WFDTransport::DisableAdvertisementInstance(): Unable to destroy GO side network"));
        }
    }

    if (isEmpty) {
        m_isAdvertising = false;
    }
}

void WFDTransport::EnableDiscoveryInstance(ListenRequest& listenRequest)
{
    QCC_DbgTrace(("WFDTransport::EnableDiscoveryInstance()"));

    /*
     * We have a new discovery request to deal with.  The first
     * order of business is to save the well-known name away for
     * use later.
     */
    bool isFirst;
    NewDiscoveryOp(ENABLE_DISCOVERY, listenRequest.m_requestParam, isFirst);

    /*
     * If it turned out that is the first discovery request on our list, we need
     * to prepare before actually doing the discovery.
     */
    if (isFirst) {

        /*
         * If we don't have any listeners up and running, we need to get them
         * up.  If this is a Windows box, the listeners will start running
         * immediately and will never go down, so they may already be running.
         */
        if (!m_isListening) {
            for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
                DoStartListen(*i);
                assert(m_listenPort);
                m_isListening = true;
            }
        }

        /*
         * We can only enable the requested advertisement if there is something
         * listening inbound connections on.  Therefore, we should only enable
         * the name service if there is a listener.  This catches the case where
         * there was no StartListen() done before the first discover.
         */
        if (m_isListening) {
            if (!m_isNsEnabled) {
                P2PNameService::Instance().Enable(TRANSPORT_WFD);
                m_isNsEnabled = true;
            }
        } else {
            QCC_LogError(ER_FAIL, ("WFDTransport::EnableDiscoveryInstance(): Discover with no WFD listeners"));
            return;
        }
    }

    /*
     * We think we're ready to send the FindAdvertisedName.  Are we really?
     */
    assert(m_isListening);
    assert(m_listenPort);
    assert(m_isNsEnabled);
    assert(P2PNameService::Instance().Started() && "WFDTransport::EnableDiscoveryInstance(): P2PNameService not started");

    qcc::String starred = listenRequest.m_requestParam;
//  starred.append('*');

    QStatus status = P2PNameService::Instance().FindAdvertisedName(TRANSPORT_WFD, starred);
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::EnableDiscoveryInstance(): Failed to begin discovery on \"%s\"", starred.c_str()));
    }

    m_isDiscovering = true;
}

void WFDTransport::DisableDiscoveryInstance(ListenRequest& listenRequest)
{
    QCC_DbgTrace(("WFDTransport::DisableDiscoveryInstance()"));

    /*
     * We have a new disable discovery request to deal with.  The first
     * order of business is to remove the well-known name from our saved list.
     */
    bool isFirst;
    bool isEmpty = NewDiscoveryOp(DISABLE_DISCOVERY, listenRequest.m_requestParam, isFirst);

    qcc::String starred = listenRequest.m_requestParam;
//  starred.append('*');

    QStatus status = P2PNameService::Instance().CancelFindAdvertisedName(TRANSPORT_WFD, starred);
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::DisableDiscoveryInstance(): Failed to end discovery on \"%s\"", starred.c_str()));
    }

    /*
     * If it turns out that this was the last discovery operation on our list,
     * we need to think about disabling our listeners and turning off the name
     * service.  We only to this if there are no advertisements in progress.
     */
    if (isEmpty && !m_isAdvertising) {

        /*
         * We disable the P2P name service explicitly.  Telling the IP name
         * service that we have no enabled ports tells it to disable.
         */
        P2PNameService::Instance().Disable(TRANSPORT_WFD);
        std::map<qcc::String, uint16_t> listenPortMap;
        listenPortMap["*"] = m_listenPort;
        IpNameService::Instance().Enable(TRANSPORT_WFD, listenPortMap, 0, std::map<qcc::String, uint16_t>(), 0, false, false, false, false);
        m_isNsEnabled = false;

        /*
         * If we had the name service running, we must have had listeners
         * waiting for connections due to the name service.  We need to stop
         * them all now.
         */
        for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
            DoStopListen(*i);
        }

        m_isListening = false;
        m_listenPort = 0;
    }

    if (isEmpty) {
        m_isDiscovering = false;
    }
}

/*
 * The default address for use in listen specs.  INADDR_ANY means to listen
 * for WFD connections on any interfaces that are currently up or any that may
 * come up in the future.
 */
static const char* ADDR4_DEFAULT = "0.0.0.0";

/*
 * The default port for use in listen specs.  This port is used by the WFD
 * listener to listen for incoming connection requests.  This is the default
 * port for a "reliable" IPv4 listener since being able to deal with IPv4
 * connection requests is required as part of the definition of the WFD
 * transport.
 *
 * All other mechanisms (unreliable IPv4, reliable IPv6, unreliable IPv6)
 * rely on the presence of an u4port, r6port, and u6port respectively to
 * enable those mechanisms if possible.
 */
static const uint16_t PORT_DEFAULT = 9956;

QStatus WFDTransport::NormalizeListenSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    QCC_DbgTrace(("WFDTransport::NormalizeListenSpec()"));

    qcc::String family;

    /*
     * We don't make any calls that require us to be in any particular state
     * with respect to threading so we don't bother to call IsRunning() here.
     *
     * Take the string in inSpec, which must start with "wfd:" and parse it,
     * looking for comma-separated "key=value" pairs and initialize the
     * argMap with those pairs.
     *
     * There are lots of legal possibilities for an IP-based transport, but
     * all we are going to recognize is the "reliable IPv4 mechanism" and
     * so we will summarily pitch everything else.
     *
     * We expect to end up with a normalized outSpec that looks something
     * like:
     *
     *     "wfd:r4addr=0.0.0.0,r4port=9955"
     *
     * That's all.  We still allow "addr=0.0.0.0,port=9955,family=ipv4" but
     * since the only thing that was ever allowed was really reliable IPv4, we
     * treat addr as synonomous with r4addr, port as synonomous with r4port and
     * ignore family.  The old stuff is normalized to the above.
     *
     * In the future we may want to revisit this and use position/order of keys
     * to imply more information.  For example:
     *
     *     "wfd:addr=0.0.0.0,port=9955,family=ipv4,reliable=true,
     *          addr=0.0.0.0,port=9956,family=ipv4,reliable=false;"
     *
     * might translate into:
     *
     *     "wfd:r4addr=0.0.0.0,r4port=9955,u4addr=0.0.0.0,u4port=9956;"
     *
     * Note the new significance of position.
     */
    QStatus status = ParseArguments(GetTransportName(), inSpec, argMap);
    if (status != ER_OK) {
        return status;
    }

    map<qcc::String, qcc::String>::iterator iter;

    /*
     * We just ignore the family since ipv4 was the only possibld working choice.
     */
    iter = argMap.find("family");
    if (iter != argMap.end()) {
        argMap.erase(iter);
    }

    /*
     * Transports, by definition, may support reliable Ipv4, unreliable IPv4,
     * reliable IPv6 and unreliable IPv6 mechanisms to move bits.  In this
     * incarnation, the WFD transport will only support reliable IPv4; so we
     * log errors and ignore any requests for other mechanisms.
     */
    iter = argMap.find("u4addr");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("WFDTransport::NormalizeListenSpec(): The mechanism implied by \"u4addr\" is not supported."));
        argMap.erase(iter);
    }

    iter = argMap.find("u4port");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("WFDTransport::NormalizeListenSpec(): The mechanism implied by \"u4port\" is not supported."));
        argMap.erase(iter);
    }

    iter = argMap.find("r6addr");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("WFDTransport::NormalizeListenSpec(): The mechanism implied by \"r6addr\" is not supported."));
        argMap.erase(iter);
    }

    iter = argMap.find("r6port");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("WFDTransport::NormalizeListenSpec(): The mechanism implied by \"r6port\" is not supported."));
        argMap.erase(iter);
    }

    iter = argMap.find("u6addr");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("WFDTransport::NormalizeListenSpec(): The mechanism implied by \"u6addr\" is not supported."));
        argMap.erase(iter);
    }

    iter = argMap.find("u6port");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("WFDTransport::NormalizeListenSpec(): The mechanism implied by \"u6port\" is not supported."));
        argMap.erase(iter);
    }

    /*
     * Now, begin normalizing what we want to see in a listen spec.
     *
     * All listen specs must start with the name of the transport followed by
     * a colon.
     */
    outSpec = GetTransportName() + qcc::String(":");

    /*
     * The WFD transport must absolutely support the IPv4 "reliable" mechanism
     * (WFD).  We therefore must provide an r4addr either from explicit keys or
     * generated from the defaults.
     */
    iter = argMap.find("r4addr");
    if (iter == argMap.end()) {
        /*
         * We have no value associated with an "r4addr" key.  Do we have an
         * "addr" which would be synonymous?  If so, save it as an r4addr,
         * erase it and point back to the new r4addr.
         */
        iter = argMap.find("addr");
        if (iter != argMap.end()) {
            argMap["r4addr"] = iter->second;
            argMap.erase(iter);
        }

        iter = argMap.find("r4addr");
    }

    /*
     * Now, deal with the r4addr, possibly replaced by addr.
     */
    if (iter != argMap.end()) {
        /*
         * We have a value associated with the "r4addr" key.  Run it through a
         * conversion function to make sure it's a valid value and to get into
         * in a standard representation.
         */
        IPAddress addr;
        status = addr.SetAddress(iter->second, false);
        if (status == ER_OK) {
            /*
             * The r4addr had better be an IPv4 address, otherwise we bail.
             */
            if (!addr.IsIPv4()) {
                QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                             ("WFDTransport::NormalizeListenSpec(): The r4addr \"%s\" is not a legal IPv4 address.",
                              iter->second.c_str()));
                return ER_BUS_BAD_TRANSPORT_ARGS;
            }
            iter->second = addr.ToString();
            outSpec.append("r4addr=" + addr.ToString());
        } else {
            QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                         ("WFDTransport::NormalizeListenSpec(): The r4addr \"%s\" is not a legal IPv4 address.",
                          iter->second.c_str()));
            return ER_BUS_BAD_TRANSPORT_ARGS;
        }
    } else {
        /*
         * We have no value associated with an "r4addr" key.  Use the default
         * IPv4 listen address for the outspec and create a new key for the
         * map.
         */
        outSpec.append("r4addr=" + qcc::String(ADDR4_DEFAULT));
        argMap["r4addr"] = ADDR4_DEFAULT;
    }

    /*
     * The WFD transport must absolutely support the IPv4 "reliable" mechanism
     * (WFD).  We therefore must provide an r4port either from explicit keys or
     * generated from the defaults.
     */
    iter = argMap.find("r4port");
    if (iter == argMap.end()) {
        /*
         * We have no value associated with an "r4port" key.  Do we have a
         * "port" which would be synonymous?  If so, save it as an r4port,
         * erase it and point back to the new r4port.
         */
        iter = argMap.find("port");
        if (iter != argMap.end()) {
            argMap["r4port"] = iter->second;
            argMap.erase(iter);
        }

        iter = argMap.find("r4port");
    }

    /*
     * Now, deal with the r4port, possibly replaced by port.
     */
    if (iter != argMap.end()) {
        /*
         * We have a value associated with the "r4port" key.  Run it through a
         * conversion function to make sure it's a valid value.  We put it into
         * a 32 bit int to make sure it will actually fit into a 16-bit port
         * number.
         */
        uint32_t port = StringToU32(iter->second);
        if (port <= 0xffff) {
            outSpec.append(",r4port=" + iter->second);
        } else {
            QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                         ("WFDTransport::NormalizeListenSpec(): The key \"r4port\" has a bad value \"%s\".", iter->second.c_str()));
            return ER_BUS_BAD_TRANSPORT_ARGS;
        }
    } else {
        /*
         * We have no value associated with an "r4port" key.  Use the default
         * IPv4 listen port for the outspec and create a new key for the map.
         */
        qcc::String portString = U32ToString(PORT_DEFAULT);
        outSpec += ",r4port=" + portString;
        argMap["r4port"] = portString;
    }

    return ER_OK;
}

QStatus WFDTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    QCC_DbgTrace(("WFDTransport::NormalizeTransportSpec()"));

    /*
     * Wi-Fi Direct pre-association service discovery events are fundamentally
     * layer two events that happen before networks are formed.  Since there
     * is no network, there is no DHCP or DHCP equivalent, so there cannot
     * be an IP address passed in as part of a connect/transport spec.  In
     * order to identify a remote daemon to connect to, we use the daemon's
     * GUID.  If we find anything else, we have run across a "spec" that
     * is not resulting from a Wi-Fi Direct service discovery event.  We
     * reject anything but one of ours.
     *
     * It might not look like we're doing much, but we are ensuring a consistent
     * internal format WRT white space, etc.
     */
    QStatus status = ParseArguments(GetTransportName(), inSpec, argMap);
    if (status != ER_OK) {
        return status;
    }

    map<qcc::String, qcc::String>::iterator iter = argMap.find("guid");
    if (iter != argMap.end()) {
        QCC_DbgPrintf(("WFDTransport::NormalizeTransportSpec(): Found guid"));
        qcc::String guidString = iter->second;
        argMap.clear();
        argMap["guid"] = guidString;
        outSpec = qcc::String(GetTransportName()) + qcc::String(":guid=") + guidString;
        return ER_OK;
    }

    return ER_BUS_BAD_TRANSPORT_ARGS;
}

QStatus WFDTransport::Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newep)
{
    QCC_DbgTrace(("WFDTransport::Connect(): %s", connectSpec));

    QStatus status;
    bool isConnected = false;

    /*
     * Clear the new endpoint pointer so we don't have to do it over and over
     * again in case of the various errors.
     */
    if (newep->IsValid()) {
        newep->Invalidate();
    }

    /*
     * We only want to allow this call to proceed if we have a running server
     * accept thread that isn't in the process of shutting down.  We use the
     * thread response from IsRunning to give us an idea of what our server
     * accept (Run) thread is doing.  See the comment in Start() for details
     * about what IsRunning actually means, which might be subtly different from
     * your intuitition.
     *
     * If we see IsRunning(), the thread might actually have gotten a Stop(),
     * but has not yet exited its Run routine and become STOPPING.  To plug this
     * hole, we need to check IsRunning() and also m_stopping, which is set in
     * our Stop() method.
     */
    if (IsRunning() == false || m_stopping == true) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("WFDTransport::Connect(): Not running or stopping; exiting"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    /*
     * If we pass the IsRunning() gate above, we must have a server accept
     * thread spinning up or shutting down but not yet joined.  Since the name
     * service is started before the server accept thread is spun up, and
     * deleted after it is joined, we must have a started name service or someone
     * isn't playing by the rules; so an assert is appropriate here.
     */
    assert(P2PNameService::Instance().Started() && "WFDTransport::Connect(): P2PNameService not started");

    /*
     * There are two possibilities for the form of the connect spec we have just
     * normalized.  The first is that it contains a key of "guid" (the connect
     * spec looks something like "wfd:guid=2b1188267ee74bc9a910b69435779523")
     * and the second is that it contains IP addressing information as
     * exemplified by the keys "r4addr" and "r4port" (the connect spec would
     * look something like "wfd:r4addr=192.168.1.100,r4port=9956")
     *
     * If the "guid" key is present it indicates that the underlying discovery
     * event happened over Wi-Fi P2P pre-association service discovery.  Since
     * this is a fundamentally layer two process, there is no IP addressing
     * information present before this method is called.  The connection between
     * the GUID and the layer two (MAC) device address is kept in the P2P name
     * service and is available to us.
     *
     * If we found a guid, then we need to actually go and discover the IP
     * address info using our layer three name service, AKA the IP name service.
     * We expect that there will always be a precipitating layer two (P2P)
     * discovery event that drives a JoinSession() which, in turn, causes the
     * WFDTransport::Connect() that brings us here.  This first event will tell
     * us to bring up an initial Wi-Fi connection.  After that initial
     * connection is brought up, the IP name service is always run over the
     * resulting link and we may therefore see layer three discovery events.
     *
     * If the "r4addr", "u4addr", "r6addr", or "u6addr" keys are present in the
     * connect spec it indicates that the JoinSession() driving this Connect()
     * happened due to a layer three (IP) discovery event.  In this case, we
     * do not have to bring up an initial connection and we can proceed directly
     * to the actual connect part of the method.
     *
     * We have two methods to parse the different kinds of connect specs.  The
     * NormalizeTransportSpec() method determines whether the connect spec
     * contains a GUID and if it does, puts it into a standard form and returns
     * ER_OK.
     *
     * The variable preAssociationEvent tells us what kind of discovery event
     * caused the Connect() we are running: either a Wi-Fi Direct pre-
     * association service discovery event (true) or an IP name service event
     * (false).
     */
    bool preAssociationEvent = false;
    qcc::String guid;
    qcc::String device;
    qcc::String normSpec;
    map<qcc::String, qcc::String> argMap;
    status = NormalizeTransportSpec(connectSpec, normSpec, argMap);
    if (status == ER_OK) {
        QCC_DbgPrintf(("WFDTransport::Connect(): Found GUID.  Normalized connect spec is \"%s\"", normSpec.c_str()));

        /*
         * Since we found a GUID in the connect spec, we know we have no layer
         * three addressing information, so we are going to have to discover it
         * before we can do an actual TCP connect to the destination daemon.  We
         * may also need an actual physical network to move the bits over.
         * Neither of these things may exist yet.
         */
        preAssociationEvent = true;
        map<qcc::String, qcc::String>::iterator iter = argMap.find("guid");
        assert(iter != argMap.end() && "WFDTransport::Connect(): Transport spec must provide \"guid\"");
        guid = iter->second;

        /*
         * Since we are doing a Connect() we must want to take on the role of a
         * P2P STA.  A STA can only be connected to one P2P group at a time.  A
         * P2P group has an owner, which we assume to be a remote AllJoyn daemon
         * hosting the service, the advertisement for which got us here in the
         * first place.  When we got the advertisement, we mapped the discovered
         * GUID to the MAC address of the device that did the advertisement.
         *
         * The first thing we need to do in this process is to find the MAC
         * address of the device to which we want to be talking.  There is no
         * guarantee that this device has not been lost during the time it took
         * for the application to get around to asking us to connect, so we
         * return an error if we can no longer find it.
         */
        status = P2PNameService::Instance().GetDeviceForGuid(guid, device);
        if (status != ER_OK) {
            QCC_LogError(status, ("WFDTransport::Connect(): Device corresponding to GUID \"%s\" is gone", guid.c_str()));
            return status;
        }

        /*
         * Unfortunately, this is all fiendishly complicated, so it will be
         * worth your time to read this long comment before you shoot yourself
         * in the foot by making an "obviously correct" change.
         *
         * The major restriction when using the Wi-Fi Direct transport is that a
         * device can be a GO (advertise a service) or a STA (connect to a
         * service) it cannot be both.  There is a fundamental impedance
         * mismatch between this requirement of the underlying implementation
         * and the AllJoyn requirement that pure peer-to-peer applications be
         * supported.  A pure peer-to-peer application is one that is equipotent
         * with other peers and therefore has both a client (STA) and a service
         * (GO) "personality."  In order to make the WFD Transport as useful as
         * possible, we want to maximize the conditions under which we can do
         * something despite this fundamental mismatch.
         *
         * Unfortunately, sorting out what happens and what we need to do means
         * understanding the system down to the lowest levels.  Note that none
         * of this will happen in the client-server (AKA hybrid peer-to-peer)
         * case which is why I like that so much, but such are the vicissitudes
         * of anthromorphic behavior.  The end result is something that is very
         * constrained and not very abstract, but we are ordered to come up with
         * something that will work at least a little in the pure peer-to-peer
         * case.
         *
         * So, we want to appear to higher levels as if we are both client/STA
         * and service/GO even though this is impossible.  As you might expect,
         * this gets a little dicey.
         *
         * Starting with a pair for simplicity, what happens if we have two pure
         * peer-to-peer apps starting up.  Typically, both applications will
         * first start up and advertise their service personalities.  By
         * advertising, they are saying that they want to be services which
         * implies group owner (GO).  We will do a CreateTemporaryNetwork() and
         * enter the CONN_READY state which means we are a service ready to
         * accept inbound TCP connections and want to be the GO in a Wi-Fi
         * Direct group.  This advetisement and readiness to accept connections
         * will happen on both applications.
         *
         * Eventually one or both of the apps will receive an advertisement and
         * try to connect to the other.  By attempting a connection, an app is
         * announcing its intention to become a client.  In order to support the
         * peer-to-peer scenario, we need to allow this; but we can only allow
         * this if the service side is not currently connected, or the
         * contemplated connection attempt will destroy the existing group in
         * favor of a new STA connection to a different GO.  Consider the case
         * where the local P2P device is currently not connected and imagine
         * what happens when we try to connect to a remote device.  First, since
         * we are taking on the role of a client/STA, our advertisements will be
         * immediately silenced by the Android Framework/P2P Helper since a STA
         * cannot accept inbound connections and therefore advertising a service
         * over P2P makes no sense.  We are also a service, however, and we want
         * the remote side to be able to discover us and connect to us even
         * though we are a P2P STA.
         *
         * If the daemon to which we are connecting (the remote daemon from this
         * perspective) is still in the CONN_READY server state (it has not
         * discovered us yet) it will accept the connection and become the GO.
         * It will be allowed to continue advertising since it is still a
         * service/GO.  We will become the STA and our P2P advertisements will
         * be silenced, however, since we have a Wi-Fi link to the daemon the IP
         * name service can run irrespective of whether or not the underlying
         * Wi-Fi device is STA or GO and so the remote client personality can
         * discover our local service personality over IP multicast instead of
         * P2P, which is now forbidden.
         *
         * Because of the magic of distributed systems, there may be a P2P
         * advertisement making its way through the remote client even though
         * our P2P.  So, the remote daemon client personality can discover our
         * local service either due to an outstanding pre-association service
         * discovery event or through the IP name service once the link is
         * actually established.  If the discovery is through the IP name
         * service, the connection will include IP addressing information and we
         * will bypass the whole P2P system, so we don't have to worry about
         * that right here, right now.
         *
         * If, however, the discovery event on the remote side was though P2P, a
         * Connect() will be made on that remote side with a GUID which maps to
         * the device/MAC address our local P2P device -- it will try to connect
         * to us.  However, since the remote daemon is a group owner, it doesn't know the
         * MAC address of every device connected to it and so it actually already
         * be connected to the specified remote device implied by the GUID even
         * though it doesn't know it.  We have to special case this event and
         * try to let the IP name service resolve the GUID since we don't want to
         * fail if we can possibly succeed.  This actually corresponds to case three
         * below (connected && !ourGroupOwner).
         *
         * A degenerate case happens when the connections overlap, i.e., both
         * devices in the pair discover and try to connect at the same time.  In
         * this case, both sides will turn into clients from an AllJoyn
         * perspective and try to form a Wi-Fi P2P group.  Both sides will
         * select the GO intent of zero (want to be a client) and so Wi-Fi P2P
         * will go to the GO negotiation tie-breaker process.  One of the
         * "clients" will be selected as the GO and so network authentication
         * will be performed on a randomly chosedn device.  In this case, one of
         * the client personalities is actually selected to be a GO and it is
         * not notified of this fact.  Even though one of the devices has been
         * chosen to be GO by the underlying system, both have tried to
         * Connect() and so both devices will have chosen to be a client.  This
         * means that both discovery and advertisement will be disabled on both
         * devices.  The devices will become an "island of discovery" which can
         * never be expanded except in a further degeneracy.
         *
         * When a third device enters into the mix, what happens depends on
         * how the first two devices actually ended up connecting.  If both of
         * the original pair ended up doing a Connect() their advertisements
         * were silenced.  If the thrid devices enters proximity after that
         * time, it will not discover either of the previous two devices and
         * the new device will never see the original devices.
         *
         * If the first two devices connected in such a way that one of them
         * remained the service/GO (this requires that the second connect
         * happened driven by an IP name service discovery event) The service/GO
         * will still be advertising and the third device may discover the
         * advertising service of the original pair over P2P.  However, neither
         * of the original pair will be able to see the third device.
         *
         * If the third device sees the advertisement for the remaining
         * service/GO it may connect.  Once connected, the IP name service will
         * run and discover the remaining service, and as above, the two client
         * personalities of the original pair will discover the service
         * personality of the third device.  Thus there is a temporary
         * non-reflexive advertisement sitution that is only resolved when the
         * third device connects.  There may be transient advertisements
         * floating around from the device that became the STA in the
         * relationship between the two original devices.  If the third device
         * receives one of those and tries to connect to the STA it will fail.
         * The interesting fact here is that since the service on the device was
         * discovered, it will not be re-discovered by the client since a
         * subsequent discovery event over the IP name service will be masked
         * since it will look to the daemon as if it is the same service over
         * the same transport (the WFD transport).  In this case, the
         * non-reflexive advertisement situation may remain indefinitely or
         * until the third device cancels its find advertised name and restarts
         * it.
         *
         * So, the following code may seem unusually complex for what it seems
         * to be doing.  What it is actually doing is trying to make a square
         * peg fit in a round hole, so beware of making changes without thinking
         * them through.  It may cost you a toe or two.
         */
        bool connected = P2PConMan::Instance().IsConnected();
        bool ourGroupOwner = P2PConMan::Instance().IsConnected(device);

        QCC_DbgPrintf(("WFDTransport::Connect(): Device \"%s\" corresponds to GUID \"%s\"", device.c_str(), guid.c_str()));

        /*
         * case 1: !connected && !ourGroupOwner:  completely disconnected.
         * case 2: !connected &&  ourGroupOwner:  disconnected but connected to the desired group owner is impossible
         * case 3:  connected && !ourGroupOwner:  already connected but to a different group owner
         * case 4:  connected &&  ourGroupOwner:  already connected to the desired group owner
         */
        if (!connected && ourGroupOwner) {
            /*
             * First, handle case two, disconnected but connected to the desired
             * group owner is impossible.  This is impossible, so we assert that
             * it did not happen.
             */
            QCC_DbgPrintf(("WFDTransport::Connect(): Connection case two"));
            assert(false && "WFDTransport::Connect(): Impossible condition.");
        } else if (connected && !ourGroupOwner) {
            /*
             * Handle case three, already connected but to a different group
             * owner.
             *
             * There is an interesting degenerate case that we need to deal with
             * in the pure peer-to-peer case -- that is, if we are both a client
             * and a service and a remote daemon is both a client and a service.
             * If we have advertised a service and some remote application's
             * client personality has connected to us, we will be in the
             * connected state but the device to which we are connected is the
             * null string.  This is because we are the GO and are not connected
             * to a remote device (MAC) address.  If we also have a client
             * personality, we may have received a P2P pre-association service
             * discovery notification prior to the remote daemon being silenced
             * (see the long comment in case one below for details).  If this
             * happens, we may actually have a connection to the advertising
             * daemon, we just don't know it.  We don't want to arbitrarily fail
             * the Connect() in this case, we want to try to see if we can
             * resolve the GUID using the IP name service in case we can
             * actually reach the daemon.
             *
             * So, as part of case three, we look to see if we are connected to
             * a remote device with a null-string MAC address.  If we are, we
             * are a service attempting a connection to a remote GUID we have
             * heard about through a valid pre-association advertisement (recall
             * that GetDeviceForGuid(guid, device) worked above or we wouldn't
             * be here).  In this case, we just fall through to the IP name
             * service resolution process.
             *
             * If we are connected to a remote daemon via a valid MAC/device
             * address, we are trying to make a second STA connection and this
             * is not supported.  We require an explicit disconnect before we
             * allow this; and so this is an error.
             */
            QCC_DbgPrintf(("WFDTransport::Connect(): Connection case three"));
            qcc::String localDevice("");
            if (P2PConMan::Instance().IsConnected(localDevice) == false) {
                QCC_LogError(ER_P2P_FORBIDDEN, ("WFDTransport::Connect(): Second STA connection forbidden"));
                return ER_P2P_FORBIDDEN;
            }
        } else if (connected && ourGroupOwner) {
            /*
             * Handle case four, already connected to the desired group owner.
             * This means we are done since we are already connected to the
             * device we want to be connected to.
             */
            QCC_DbgPrintf(("WFDTransport::Connect(): Connection case four. Already connected to device \"%s\"", device.c_str()));
        } else {
            /*
             * Handle case one, completely disconnected.
             */
            QCC_DbgPrintf(("WFDTransport::Connect(): Connection case one. Not connected to device \"%s\"", device.c_str()));
            assert(!connected && !ourGroupOwner && "WFDTransport::Connect(): Impossible.");

            /*
             * As mentioned in the extended comment above, we may be completely
             * disconnected, but we may have an outstanding advertisement.  If
             * this is the case, we are trying to be both a service and a
             * client.  When we advertised, we did a CreateTemporaryNetwork()
             * which put the P2P connection manager into a state where it
             * expected to be a GO.  If we are going to try and do a Connect()
             * here, and rely on the IP name service to pick up the slack and
             * advertise our service, we are going to have to undo that
             * temporary network creation and put our connection manager into
             * the idle state so it can deal the request to connect as a STA
             * which will follow.  We've already done all of the tests to ensure
             * that this will be done with as few problems as possible, so we
             * just go for it.
             *
             * It will be the case that after we do this DestroyTemporaryNetwork()
             * we will not be able to accept any new connections for the services
             * we might have advertised.  So while we are off trying to connect to
             * another link, anyone who might connect to us will fail.
             */
            QCC_DbgPrintf(("WFDTransport::Connect(): DestroyTemporaryNetwork()"));
            QStatus status = P2PConMan::Instance().DestroyTemporaryNetwork();
            if (status != ER_OK) {
                QCC_LogError(status, ("WFDTransport::Connect(): Unable to destroy temporary network"));
                return status;
            }

            /*
             * If we are not connected onto a common physical network with the
             * device the first order of business is to make that happen.  Creating
             * a temporary network means bringing up the entire infrastructure of
             * the network, so this may also be a very time-consuming call during
             * which time we will block.  Since human intervention may actually be
             * required on the remote side for Wi-Fi authentication, we may be
             * talking on the order of a couple of minutes here if things happen in
             * the worst case.
             */
            QCC_DbgPrintf(("WFDTransport::Connect(): CreateTemporaryNetwork() with device \"%s\"", device.c_str()));
            status = P2PConMan::Instance().CreateTemporaryNetwork(device, P2PConMan::DEVICE_SHOULD_BE_STA);
            if (status != ER_OK) {
                QCC_LogError(status, ("WFDTransport::Connect(): Unable to CreateTemporaryNetwork() with device \"%s\"", device.c_str()));
                /*
                 * Okay, we've tried to connect as a STA and failed.  It could
                 * be the case that we were a service and in the ready state
                 * (ready to accept new inbound connections) but we are hosting
                 * a pure peer-to-peer app that wants to be both a client and a
                 * service.  If we still want to be a service, we need to return
                 * to the ready state and not just forget about the whole service
                 * thing.
                 */
                if (m_isAdvertising) {
                    qcc::String localDevice("");
                    QStatus status = P2PConMan::Instance().CreateTemporaryNetwork(localDevice, P2PConMan::DEVICE_SHOULD_BE_GO);
                    if (status != ER_OK) {
                        QCC_LogError(status, ("WFDTransport::Connect(): Unable to return to SHOULD_BE_GO"));
                    }
                }
            }
        }
    }

    /*
     * At this point, we are coming from one of three directions.
     *
     * It could be the case that we have just formed a new connection based on a
     * provided GUID in the correctly parsed and normalized transport spec.
     * This case indicates that the discovery event that precipitated the
     * Connect() is a Wi-Fi P2P pre-association service discovery event.  If we
     * find ourselves in that state, we have no layer three (IP) addressing
     * information, and we must discover it before we can proceed.
     *
     * It could be the case that we have a GUID in a correctly parsed and
     * normalized transport spec that refers to a pre-existing connection.  In
     * that case, we expect that we will have already found IP addressing
     * information for the specified device.  We don't remember that address
     * information so this case folds into the previous one.  XXX Should we?
     *
     * It could also be the case that the underlying discovery event was from
     * the IP name service, In that case we expect to have a pre-existing
     * temporary network and we do have layer three addressing information.
     * The IP address may or may not refer to the group owner since the IP
     * name service is a multicast protocol running on all of the nodes in the
     * group.  This is how we can discover and connect to other services
     * advertising as Wi-Fi Direct services even though basic WFD discovery
     * is broken/crippled in Android as of Jellybean.
     *
     * The next goal is to get a connect spec (spec) with IP addressing in it.
     * If the variable preAssociationEvent is true, it means one of the first
     * two cases above, and if it is false, it means the third.
     */
    qcc::String spec;
    if (!preAssociationEvent) {
        qcc::String spec = connectSpec;
        QCC_DbgPrintf(("WFDTransport::Connect(): Provided connect spec is \"%s\"", spec.c_str()));
    } else {
        /*
         * Since preAssociationEvent is true, we know we have a GUID from the
         * original connect spec passed to us.  We also have looked up the
         * device corresponding to that GUID.
         *
         * The ultimate goal now is to essentially create the same connect spec
         * that would have been passed to a TCPTransport::Connect() if the
         * network formation had just happened magically.  We are essentially
         * translating a spec like "wfd:guid=2b1188267ee74bc9a910b69435779523"
         * into a one like "wfd:r4addr=192.168.1.100,r4port=9956".  Note that
         * this is exactly the same form as we would see in the third case
         * above -- one that came directly from the IP name service.
         *
         * This is going to result in IP name service exchanges and may also
         * therefore take a long time to complete.  If the other side misses the
         * original who-has requests, it could take some multiple of 40 seconds
         * for the IP addresses to be found.
         */
        qcc::String newSpec;
        QCC_DbgPrintf(("WFDTransport::Connect(): CreateConnectSpec()"));
        status = P2PConMan::Instance().CreateConnectSpec(device, guid, newSpec);
        if (status != ER_OK) {
            QCC_LogError(status, ("WFDTransport::Connect(): Unable to CreateConnectSpec() with device \"%s\"", device.c_str()));
            return status;
        }

        /*
         * The newSpec is coming almost directly from the IP name service.  The
         * name service will not know to prepend the spec with "wfd:" since it
         * is used across multiple transports, but we need it now.
         */
        spec = qcc::String("wfd:") + newSpec;

        QCC_DbgPrintf(("WFDTransport::Connect(): CreateConnectSpec() says connect spec is \"%s\"", spec.c_str()));
    }

    /*
     * We have now folded all of the different cases down into one.  We have a
     * connect spec just like any other connect spec with layer three (IP)
     * address information in it.  Just like any other spec, we need to make
     * sure it is normalized since it is coming from the "outside world."
     */
    status = NormalizeListenSpec(spec.c_str(), normSpec, argMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("WFDTransport::Connect(): Invalid connect spec \"%s\"", spec.c_str()));
        return status;
    }

    QCC_DbgPrintf(("WFDTransport::Connect(): Final normalized connect spec is \"%s\"", normSpec.c_str()));

    /*
     * From this point on, the Wi-Fi Direct transport connect looks just like
     * the TCP transport connect.
     *
     * The fields r4addr and r4port are guaranteed to be present now (since we
     * successfully ran the spec through NormalizeListenSpec() to check for
     * just that).
     */
    IPAddress ipAddr(argMap.find("r4addr")->second);
    uint16_t port = StringToU32(argMap["r4port"]);

    /*
     * The semantics of the Connect method tell us that we want to connect to a
     * remote daemon.  TCP will happily allow us to connect to ourselves, but
     * this is not always possible in the various transports AllJoyn may use.
     * To avoid unnecessary differences, we do not allow a requested connection
     * to "ourself" to succeed.
     *
     * The code here is not a failsafe way to prevent this since thre are going
     * to be multiple processes involved that have no knowledge of what the
     * other is doing (for example, the wireless supplicant and this daemon).
     * This means we can't synchronize and there will be race conditions that
     * can cause the tests for selfness to fail.  The final check is made in the
     * bus hello protocol, which will abort the connection if it detects it is
     * conected to itself.  We just attempt to short circuit the process where
     * we can and not allow connections to proceed that will be bound to fail.
     *
     * One defintion of a connection to ourself is if we find that a listener
     * has has been started via a call to our own StartListener() with the same
     * connectSpec as we have now.  This is the simple case, but it also turns
     * out to be the uncommon case.
     *
     * It is perfectly legal to start a listener using the INADDR_ANY address,
     * which tells the system to listen for connections on any network interface
     * that happens to be up or that may come up in the future.  This is the
     * default listen address and is the most common case.  If this option has
     * been used, we expect to find a listener with a normalized adresss that
     * looks like "r4addr=0.0.0.0,port=y".  If we detect this kind of connectSpec
     * we have to look at the currently up interfaces and see if any of them
     * match the address provided in the connectSpec.  If so, we are attempting
     * to connect to ourself and we must fail that request.
     */
    char anyspec[64];
    snprintf(anyspec, sizeof(anyspec), "%s:r4addr=0.0.0.0,r4port=%u", GetTransportName(), port);

    qcc::String normAnySpec;
    map<qcc::String, qcc::String> normArgMap;
    status = NormalizeListenSpec(anyspec, normAnySpec, normArgMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("WFDTransport::Connect(): Invalid INADDR_ANY connect spec"));
        return status;
    }

    /*
     * Look to see if we are already listening on the provided connectSpec
     * either explicitly or via the INADDR_ANY address.
     */
    QCC_DbgPrintf(("WFDTransport::Connect(): Checking for connection to self"));
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    bool anyEncountered = false;
    for (list<pair<qcc::String, SocketFd> >::iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
        QCC_DbgPrintf(("WFDTransport::Connect(): Checking listenSpec %s", i->first.c_str()));

        /*
         * If the provided connectSpec is already explicitly listened to, it is
         * an error.  We expect to never see INADDR_ANY in a normSpec.
         */
        if (i->first == normSpec) {
            m_listenFdsLock.Unlock(MUTEX_CONTEXT);
            QCC_DbgPrintf(("WFDTransport::Connect(): Explicit connection to self"));
            return ER_BUS_ALREADY_LISTENING;
        }

        /*
         * If we are listening to INADDR_ANY and the supplied port, then we have
         * to look to the currently UP interfaces to decide if this call is bogus
         * or not.  Set a flag to remind us.
         */
        if (i->first == normAnySpec) {
            QCC_DbgPrintf(("WFDTransport::Connect(): Possible implicit connection to self detected"));
            anyEncountered = true;
        }
    }
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

    /*
     * If we are listening to INADDR_ANY, we are going to have to see if any
     * currently UP interfaces have an address that matches the connectSpec
     * addr.
     */
    if (anyEncountered) {
        QCC_DbgPrintf(("WFDTransport::Connect(): Checking for implicit connection to self"));
        std::vector<qcc::IfConfigEntry> entries;
        QStatus status = qcc::IfConfig(entries);

        /*
         * Only do the check for self-ness if we can get interfaces to check.
         * This is a non-fatal error since we know that there is an end-to-end
         * check happening in the bus hello exchange, so if there is a problem
         * it will simply be detected later.
         */
        if (status == ER_OK) {
            /*
             * Loop through the network interface entries looking for an UP
             * interface that has the same IP address as the one we're trying to
             * connect to.  We know any match on the address will be a hit since
             * we matched the port during the listener check above.  Since we
             * have a listener listening on *any* UP interface on the specified
             * port, a match on the interface address with the connect address
             * is a hit.
             */
            for (uint32_t i = 0; i < entries.size(); ++i) {
                QCC_DbgPrintf(("WFDTransport::Connect(): Checking interface %s", entries[i].m_name.c_str()));
                if (entries[i].m_flags & qcc::IfConfigEntry::UP) {
                    QCC_DbgPrintf(("WFDTransport::Connect(): Interface UP with addresss %s", entries[i].m_addr.c_str()));
                    IPAddress foundAddr(entries[i].m_addr);
                    if (foundAddr == ipAddr) {
                        QCC_DbgPrintf(("WFDTransport::Connect(): Attempted connection to self; exiting"));
                        return ER_BUS_ALREADY_LISTENING;
                    }
                }
            }
        }
    }

    /*
     * This is a new not previously satisfied connection request, so attempt
     * to connect to the remote WFD address and port specified in the connectSpec.
     */
    SocketFd sockFd = qcc::INVALID_SOCKET_FD;
    status = Socket(QCC_AF_INET, QCC_SOCK_STREAM, sockFd);
    if (status == ER_OK) {
        /* Turn off Nagle */
        status = SetNagle(sockFd, false);
    }

    if (status == ER_OK) {
        /*
         * We got a socket, now tell WFD to connect to the remote address and
         * port.
         */
        QCC_DbgPrintf(("WFDTransport::Connect(): Connect()"));
        status = qcc::Connect(sockFd, ipAddr, port);
        if (status == ER_OK) {
            /*
             * We now have a WFD connection established, but DBus (the wire
             * protocol which we are using) requires that every connection,
             * irrespective of transport, start with a single zero byte.  This
             * is so that the Unix-domain socket transport used by DBus can pass
             * SCM_RIGHTS out-of-band when that byte is sent.
             */
            uint8_t nul = 0;
            size_t sent;

            QCC_DbgPrintf(("WFDTransport::Connect(): Send() one byte"));
            status = Send(sockFd, &nul, 1, sent);
            if (status != ER_OK) {
                QCC_LogError(status, ("WFDTransport::Connect(): Failed to send initial NUL byte"));
            }
            isConnected = true;
        } else {
            QCC_LogError(status, ("WFDTransport::Connect(): Failed"));
        }
    } else {
        QCC_LogError(status, ("WFDTransport::Connect(): qcc::Socket() failed"));
    }


    if (status == ER_OK) {
        /*
         * The underlying transport mechanism is started, but we need to create
         * a WFDEndpoint object that will orchestrate the movement of data
         * across the transport.
         */
        QCC_DbgPrintf(("WFDTransport::Connect(): new WFDEndpoint()"));
        bool falsiness = false;
        WFDEndpoint conn(this, m_bus, falsiness, normSpec, sockFd, ipAddr, port, guid);

        /*
         * On the active side of a connection, we don't need an authentication
         * thread to run since we have the caller thread to fill that role.
         */
        conn->SetActive();
        conn->SetAuthenticating();

        /*
         * Initialize the "features" for this endpoint
         */
        conn->GetFeatures().isBusToBus = true;
        conn->GetFeatures().allowRemote = m_bus.GetInternal().AllowRemoteMessages();
        conn->GetFeatures().handlePassing = false;

        qcc::String authName;
        qcc::String redirection;

        /*
         * This is a little tricky.  We usually manage endpoints in one place
         * using the main server accept loop thread.  This thread expects
         * endpoints to have an RX thread and a TX thread running, and these
         * threads are expected to run through the EndpointExit function when
         * they are stopped.  The general endpoint management uses these
         * mechanisms.  However, we are about to get into a state where we are
         * off trying to start an endpoint, but we are using another thread
         * which has called into WFDTransport::Connect().  We are about to do
         * blocking I/O in the authentication establishment dance, but we can't
         * just kill off this thread since it isn't ours for the whacking.  If
         * the transport is stopped, we do however need a way to stop an
         * in-process establishment.  It's not reliable to just close a socket
         * out from uder a thread, so we really need to Alert() the thread
         * making the blocking calls.  So we keep a separate list of Thread*
         * that may need to be Alert()ed and run through that list when the
         * transport is stopping.  This will cause the I/O calls in Establish()
         * to return and we can then allow the "external" threads to return
         * and avoid nasty deadlocks.
         */
        Thread* thread = GetThread();
        m_endpointListLock.Lock(MUTEX_CONTEXT);
        m_activeEndpointsThreadList.insert(thread);
        m_endpointListLock.Unlock(MUTEX_CONTEXT);

        /*
         * Go ahead and do the authentication in the context of this thread.  Even
         * though we don't have the server accept loop thread watching this endpoint
         * we keep we keep the states consistent since the endpoint will eventually
         * to there.
         */
        QCC_DbgPrintf(("WFDTransport::Connect(): Establish()"));
        status = conn->Establish("ANONYMOUS", authName, redirection);
        if (status == ER_OK) {
            conn->SetListener(this);
            status = conn->Start();
            if (status == ER_OK) {
                conn->SetEpStarted();
                conn->SetAuthDone();
            } else {
                conn->SetEpFailed();
                conn->SetAuthDone();
            }
        }

        /*
         * If we have a successful authentication, we pass the connection off to the
         * server accept loop to manage.
         */
        if (status == ER_OK) {
            QCC_DbgPrintf(("WFDTransport::Connect(): Success.  Pass connection."));
            m_endpointListLock.Lock(MUTEX_CONTEXT);
            m_endpointList.insert(conn);
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            newep = BusEndpoint::cast(conn);
        } else {
            QCC_LogError(status, ("WFDTransport::Connect(): Starting the WFDEndpoint failed"));

            /*
             * Although the destructor of a remote endpoint includes a Stop and Join
             * call, there are no running threads since Start() failed.
             */
            conn->Invalidate();
        }

        /*
         * In any case, we are done with blocking I/O on the current thread, so
         * we need to remove its pointer from the list we kept around to break it
         * out of blocking I/O.  If we were successful, the WFDEndpoint was passed
         * to the m_endpointList, where the main server accept loop will deal with
         * it using its RX and TX thread-based mechanisms.  If we were unsuccessful
         * the WFDEndpoint was destroyed and we will return an error below after
         * cleaning up the underlying socket.
         */
        m_endpointListLock.Lock(MUTEX_CONTEXT);
        set<Thread*>::iterator i = find(m_activeEndpointsThreadList.begin(), m_activeEndpointsThreadList.end(), thread);
        assert(i != m_activeEndpointsThreadList.end() && "WFDTransport::Connect(): Thread* not on m_activeEndpointsThreadList");
        m_activeEndpointsThreadList.erase(i);
        m_endpointListLock.Unlock(MUTEX_CONTEXT);
    } else {
        /*
         * If we got an error, and have not created an endpoint, we need to cleanup
         * the socket. If an endpoint was created, the endpoint will be responsible
         * for the cleanup.
         */
        if (isConnected) {
            qcc::Shutdown(sockFd);
        }
        if (sockFd >= 0) {
            qcc::Close(sockFd);
        }

    }

    if (status != ER_OK) {
        /* If we got this connection and its endpoint up without
         * a problem, we return a pointer to the new endpoint.  We aren't going to
         * clean it up since it is an active connection, so we can safely pass the
         * endoint back up to higher layers.
         * Invalidate the endpoint in case of error.
         */
        newep->Invalidate();
    } else {
        assert(newep->IsValid() && "WFDTransport::Connect(): If the conn is up, the conn should be valid");
    }
    QCC_DbgPrintf(("WFDTransport::Connect(): Done."));
    return status;
}

QStatus WFDTransport::Disconnect(const char* connectSpec)
{
    QCC_DbgTrace(("WFDTransport::Disconnect(): %s", connectSpec));

    /*
     * We only want to allow this call to proceed if we have a running server
     * accept thread that isn't in the process of shutting down.  We use the
     * thread response from IsRunning to give us an idea of what our server
     * accept (Run) thread is doing, and by extension the endpoint threads which
     * must be running to properly clean up.  See the comment in Start() for
     * details about what IsRunning actually means, which might be subtly
     * different from your intuitition.
     *
     * If we see IsRunning(), the thread might actually have gotten a Stop(),
     * but has not yet exited its Run routine and become STOPPING.  To plug this
     * hole, we need to check IsRunning() and also m_stopping, which is set in
     * our Stop() method.
     */
    if (IsRunning() == false || m_stopping == true) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("WFDTransport::Disconnect(): Not running or stopping; exiting"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    /*
     * If we pass the IsRunning() gate above, we must have a server accept
     * thread spinning up or shutting down but not yet joined.  Since the name
     * service is started before the server accept thread is spun up, and
     * stopped after it is stopped, we must have a started name service or
     * someone isn't playing by the rules; so an assert is appropriate here.
     */
    assert(P2PNameService::Instance().Started() && "WFDTransport::Disconnect(): P2PNameService not started");

    /*
     * Higher level code tells us which connection is refers to by giving us the
     * same connect spec it used in the Connect() call.  For the Wi-Fi Direct
     * transport, this is going to be the GUID found in the original service
     * discovery event.
     */
    qcc::String normSpec;
    map<qcc::String, qcc::String> argMap;
    QStatus status = NormalizeTransportSpec(connectSpec, normSpec, argMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("WFDTransport::Disconnect(): Invalid WFD connect spec \"%s\"", connectSpec));
        return status;
    }

    map<qcc::String, qcc::String>::iterator iter = argMap.find("guid");
    assert(iter != argMap.end() && "WFDTransport::Connect(): Transport spec must provide \"guid\"");
    qcc::String guid = iter->second;

    /*
     * Now we must stop the remote endpoint(s) associated with the GUID.  Be
     * careful here since calling Stop() on the WFDEndpoint is going to cause
     * the transmit and receive threads of the underlying RemoteEndpoint to
     * exit, which will cause our EndpointExit() to be called, which will walk
     * the list of endpoints and delete the one we are stopping.  Once we poke
     * ep->Stop(), the pointer to ep must be considered dead.
     */
    status = ER_BUS_BAD_TRANSPORT_ARGS;
    m_endpointListLock.Lock(MUTEX_CONTEXT);
    for (set<WFDEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        WFDEndpoint ep = *i;
        if (ep->GetGuid() == guid) {
            ep->SetSuddenDisconnect(false);
            ep->Stop();
            i = m_endpointList.begin();
        }
    }
    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    /*
     * We've started the process of getting rid of any endpoints we may have created.
     * Now we have to actually get rid of the temporary network itself.  Since the
     * network is related to the device, not the GUID, we have to get the device
     * (MAC address) first.
     */
    qcc::String device;
    status = P2PNameService::Instance().GetDeviceForGuid(guid, device);
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::Disconnect(): Device corresponding to GUID \"%s\" is gone", guid.c_str()));
        return status;
    }

    /*
     * Now, leave the P2P Group that our device is participating in.
     */
    status = P2PConMan::Instance().DestroyTemporaryNetwork();
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::Disconnect(): Unable to DestroyTemporaryNetwork"));
        return status;
    }

    return status;
}

QStatus WFDTransport::StartListen(const char* listenSpec)
{
    QCC_DbgTrace(("WFDTransport::StartListen()"));

    /*
     * We only want to allow this call to proceed if we have a running server
     * accept thread that isn't in the process of shutting down.  We use the
     * thread response from IsRunning to give us an idea of what our server
     * accept (Run) thread is doing.  See the comment in Start() for details
     * about what IsRunning actually means, which might be subtly different from
     * your intuitition.
     *
     * If we see IsRunning(), the thread might actually have gotten a Stop(),
     * but has not yet exited its Run routine and become STOPPING.  To plug this
     * hole, we need to check IsRunning() and also m_stopping, which is set in
     * our Stop() method.
     */
    if (IsRunning() == false || m_stopping == true) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("WFDTransport::StartListen(): Not running or stopping; exiting"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    /*
     * Normalize the listen spec.  Although this looks like a connectSpec it is
     * different in that reasonable defaults are possible.  We do the
     * normalization here so we can report an error back to the caller.
     */
    qcc::String normSpec;
    map<qcc::String, qcc::String> argMap;
    QStatus status = NormalizeListenSpec(listenSpec, normSpec, argMap);
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::StartListen(): Invalid WFD listen spec \"%s\"", listenSpec));
        return status;
    }

    QCC_DbgPrintf(("WFDTransport::StartListen(): r4addr = \"%s\", r4port = \"%s\"",
                   argMap["r4addr"].c_str(), argMap["r4port"].c_str()));

    /*
     * The daemon code is in a state where it lags in functionality a bit with
     * respect to the common code.  Common supports the use of IPv6 addresses
     * but the name service is not quite ready for prime time.  Until the name
     * service can properly distinguish between various cases, we fail any
     * request to listen on an IPv6 address.
     */
    IPAddress ipAddress;
    status = ipAddress.SetAddress(argMap["r4addr"].c_str());
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::StartListen(): Unable to SetAddress(\"%s\")", argMap["r4addr"].c_str()));
        return status;
    }

    if (ipAddress.IsIPv6()) {
        status = ER_INVALID_ADDRESS;
        QCC_LogError(status, ("WFDTransport::StartListen(): IPv6 address (\"%s\") in \"r4addr\" not allowed", argMap["r4addr"].c_str()));
        return status;
    }

    /*
     * Because we are sending a *request* to start listening on a given
     * normalized listen spec to another thread, and the server thread starts
     * and stops listening on given listen specs when it decides to eventually
     * run, it is be possible for a calling thread to send multiple requests to
     * start or stop listening on the same listenSpec before the server thread
     * responds.
     *
     * In order to deal with these two timelines, we keep a list of normalized
     * listenSpecs that we have requested to be started, and not yet requested
     * to be removed.  This list (the mListenSpecs) must be consistent with
     * client requests to start and stop listens.  This list is not necessarily
     * consistent with what is actually being listened on.  That is a separate
     * list called mListenFds.
     *
     * So, check to see if someone has previously requested that the address and
     * port in question be listened on.  We need to do this here to be able to
     * report an error back to the caller.
     */
    m_listenSpecsLock.Lock(MUTEX_CONTEXT);
    for (list<qcc::String>::iterator i = m_listenSpecs.begin(); i != m_listenSpecs.end(); ++i) {
        if (*i == normSpec) {
            m_listenSpecsLock.Unlock(MUTEX_CONTEXT);
            return ER_BUS_ALREADY_LISTENING;
        }
    }
    m_listenSpecsLock.Unlock(MUTEX_CONTEXT);

    QueueStartListen(normSpec);
    return ER_OK;
}

void WFDTransport::QueueStartListen(qcc::String& normSpec)
{
    QCC_DbgTrace(("WFDTransport::QueueStartListen()"));

    /*
     * In order to start a listen, we send the server accept thread a message
     * containing the START_LISTEN_INSTANCE request code and the normalized
     * listen spec which specifies the address and port instance to listen on.
     */
    ListenRequest listenRequest;
    listenRequest.m_requestOp = START_LISTEN_INSTANCE;
    listenRequest.m_requestParam = normSpec;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    m_listenRequests.push(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

    /*
     * Wake the server accept loop thread up so it will process the request we
     * just queued.
     */
    Alert();
}

void WFDTransport::DoStartListen(qcc::String& normSpec)
{
    QCC_DbgTrace(("WFDTransport::DoStartListen()"));

    /*
     * Parse the normalized listen spec.  The easiest way to do this is to
     * re-normalize it.  If there's an error at this point, we have done
     * something wrong since the listen spec was presumably successfully
     * normalized before sending it in -- so we assert.
     */
    qcc::String spec;
    map<qcc::String, qcc::String> argMap;
    QStatus status = NormalizeListenSpec(normSpec.c_str(), spec, argMap);
    assert(status == ER_OK && "WFDTransport::DoStartListen(): Invalid WFD listen spec");

    QCC_DbgPrintf(("WFDTransport::DoStartListen(): r4addr = \"%s\", r4port = \"%s\"",
                   argMap["r4addr"].c_str(), argMap["r4port"].c_str()));

    m_listenFdsLock.Lock(MUTEX_CONTEXT);

    /*
     * Figure out what local address and port the listener should use.
     */
    IPAddress listenAddr(argMap["r4addr"]);
    uint16_t listenPort = StringToU32(argMap["r4port"]);
    bool ephemeralPort = (listenPort == 0);

    /*
     * Create the actual TCP listener sockets and set SO_REUSEADDR/SO_REUSEPORT
     * so we don't have to wait for four minutes to relaunch the daemon if it
     * crashes.
     */
    SocketFd listenFd = qcc::INVALID_SOCKET_FD;
    status = Socket(QCC_AF_INET, QCC_SOCK_STREAM, listenFd);
    if (status != ER_OK) {
        m_listenFdsLock.Unlock(MUTEX_CONTEXT);
        QCC_LogError(status, ("WFDTransport::DoStartListen(): Socket() failed"));
        return;
    }

    /*
     * Set the SO_REUSEADDR socket option so we don't have to wait for four
     * minutes while the endpoint is in TIME_WAIT if we crash (or control-C).
     */
    status = qcc::SetReuseAddress(listenFd, true);
    if (status != ER_OK && status != ER_NOT_IMPLEMENTED) {
        m_listenFdsLock.Unlock(MUTEX_CONTEXT);
        QCC_LogError(status, ("WFDTransport::DoStartListen(): SetReuseAddress() failed"));
        qcc::Close(listenFd);
        return;
    }

    /*
     * We call accept in a loop so we need the listenFd to non-blocking
     */
    status = qcc::SetBlocking(listenFd, false);
    if (status != ER_OK) {
        m_listenFdsLock.Unlock(MUTEX_CONTEXT);
        QCC_LogError(status, ("WFDTransport::DoStartListen(): SetBlocking() failed"));
        qcc::Close(listenFd);
        return;
    }

    /*
     * Bind the socket to the listen address and start listening for incoming
     * connections on it.
     */
    if (ephemeralPort) {
        /*
         * First try binding to the default port
         */
        listenPort = PORT_DEFAULT;
        status = Bind(listenFd, listenAddr, listenPort);
        if (status != ER_OK) {
            listenPort = 0;
            status = Bind(listenFd, listenAddr, listenPort);
        }
    } else {
        status = Bind(listenFd, listenAddr, listenPort);
    }

    if (status == ER_OK) {
        /*
         * If the port was not set (or set to zero) then we will have bound an ephemeral port. If
         * so call GetLocalAddress() to update the connect spec with the port allocated by bind.
         */
        if (ephemeralPort) {
            qcc::GetLocalAddress(listenFd, listenAddr, listenPort);
            normSpec = qcc::String(GetTransportName()) + qcc::String(":r4addr=") + argMap["r4addr"] +
                       qcc::String(",r4port=") + U32ToString(listenPort);
        }
        status = qcc::Listen(listenFd, MAX_LISTEN_CONNECTIONS);
        if (status == ER_OK) {
            QCC_DbgPrintf(("WFDTransport::DoStartListen(): Listening on %s/%d", argMap["r4addr"].c_str(), listenPort));
            m_listenFds.push_back(pair<qcc::String, SocketFd>(normSpec, listenFd));
        } else {
            QCC_LogError(status, ("WFDTransport::DoStartListen(): Listen failed"));
        }
    } else {
        QCC_LogError(status, ("WFDTransport::DoStartListen(): Failed to bind to %s/%d", listenAddr.ToString().c_str(), listenPort));
    }

    /*
     * In the WFDTransport, we only support discovery of services that are
     * advertised over Wi-Fi Direct pre-association service discovery.  We
     * explicitly don't allow the IP name service to insinuate itself onto
     * Wi-Fi Direct established links and begin advertising willy-nilly.
     * we only enable the IP name service for the specific use case of
     * discovering IP address and port from daemon GUID.
     */
    m_listenPort = listenPort;
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

    /*
     * Signal the (probably) waiting run thread so it will wake up and add this
     * new socket to its list of sockets it is waiting for connections on.
     */
    if (status == ER_OK) {
        Alert();
    }
}

QStatus WFDTransport::StopListen(const char* listenSpec)
{
    QCC_DbgTrace(("WFDTransport::StopListen()"));

    /*
     * We only want to allow this call to proceed if we have a running server
     * accept thread that isn't in the process of shutting down.  We use the
     * thread response from IsRunning to give us an idea of what our server
     * accept (Run) thread is doing.  See the comment in Start() for details
     * about what IsRunning actually means, which might be subtly different from
     * your intuitition.
     *
     * If we see IsRunning(), the thread might actually have gotten a Stop(),
     * but has not yet exited its Run routine and become STOPPING.  To plug this
     * hole, we need to check IsRunning() and also m_stopping, which is set in
     * our Stop() method.
     */
    if (IsRunning() == false || m_stopping == true) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("WFDTransport::StopListen(): Not running or stopping; exiting"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    /*
     * Normalize the listen spec.  We are going to use the name string that was
     * put together for the StartListen call to find the listener instance to
     * stop, so we need to do it exactly the same way.
     */
    qcc::String normSpec;
    map<qcc::String, qcc::String> argMap;
    QStatus status = NormalizeListenSpec(listenSpec, normSpec, argMap);
    if (status != ER_OK) {
        QCC_LogError(status, ("WFDTransport::StopListen(): Invalid WFD listen spec \"%s\"", listenSpec));
        return status;
    }

    /*
     * Because we are sending a *request* to stop listening on a given
     * normalized listen spec to another thread, and the server thread starts
     * and stops listening on given listen specs when it decides to eventually
     * run, it is be possible for a calling thread to send multiple requests to
     * start or stop listening on the same listenSpec before the server thread
     * responds.
     *
     * In order to deal with these two timelines, we keep a list of normalized
     * listenSpecs that we have requested to be started, and not yet requested
     * to be removed.  This list (the mListenSpecs) must be consistent with
     * client requests to start and stop listens.  This list is not necessarily
     * consistent with what is actually being listened on.  That is reflected by
     * a separate list called mListenFds.
     *
     * We consult the list of listen spects for duplicates when starting to
     * listen, and we make sure that a listen spec is on the list before
     * queueing a request to stop listening.  Asking to stop listening on a
     * listen spec we aren't listening on is not an error, since the goal of the
     * user is to not listen on a given address and port -- and we aren't.
     */
    m_listenSpecsLock.Lock(MUTEX_CONTEXT);
    for (list<qcc::String>::iterator i = m_listenSpecs.begin(); i != m_listenSpecs.end(); ++i) {
        if (*i == normSpec) {
            m_listenSpecs.erase(i);
            QueueStopListen(normSpec);
            break;
        }
    }
    m_listenSpecsLock.Unlock(MUTEX_CONTEXT);

    return ER_OK;
}

void WFDTransport::QueueStopListen(qcc::String& normSpec)
{
    QCC_DbgTrace(("WFDTransport::QueueStopListen()"));

    /*
     * In order to stop a listen, we send the server accept thread a message
     * containing the STOP_LISTEN_INTANCE request code and the normalized listen
     * spec which specifies the address and port instance to stop listening on.
     */
    ListenRequest listenRequest;
    listenRequest.m_requestOp = STOP_LISTEN_INSTANCE;
    listenRequest.m_requestParam = normSpec;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    m_listenRequests.push(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

    /*
     * Wake the server accept loop thread up so it will process the request we
     * just queued.
     */
    Alert();
}

void WFDTransport::DoStopListen(qcc::String& normSpec)
{
    QCC_DbgTrace(("WFDTransport::DoStopListen()"));

    /*
     * Since the name service is started before the server accept thread is spun
     * up, and stopped after it is stopped, we must have a started name service or
     * someone isn't playing by the rules; so an assert is appropriate here.
     */
    assert(P2PNameService::Instance().Started() && "WFDTransport::DoStopListen(): P2PNameService not started");

    /*
     * Find the (single) listen spec and remove it from the list of active FDs
     * used by the server accept loop (run thread).  This is okay to do since
     * we are assuming that, since we should only be called in the context of
     * the server accept loop, it knows that an FD will be deleted here.
     */
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    qcc::SocketFd stopFd = qcc::INVALID_SOCKET_FD;
    bool found = false;
    for (list<pair<qcc::String, SocketFd> >::iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
        if (i->first == normSpec) {
            stopFd = i->second;
            m_listenFds.erase(i);
            found = true;
            break;
        }
    }
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

    /*
     * If we took a socketFD off of the list of active FDs, we need to tear it
     * down and alert the server accept loop that the list of FDs on which it
     * is listening has changed.
     */
    if (found) {
        qcc::Shutdown(stopFd);
        qcc::Close(stopFd);
    }
}

bool WFDTransport::NewDiscoveryOp(DiscoveryOp op, qcc::String namePrefix, bool& isFirst)
{
    QCC_DbgTrace(("WFDTransport::NewDiscoveryOp()"));

    bool first = false;

    if (op == ENABLE_DISCOVERY) {
        QCC_DbgPrintf(("WFDTransport::NewDiscoveryOp(): Registering discovery of namePrefix \"%s\"", namePrefix.c_str()));
        first = m_advertising.empty();
        m_discovering.push_back(namePrefix);
    } else {
        list<qcc::String>::iterator i = find(m_discovering.begin(), m_discovering.end(), namePrefix);
        if (i == m_discovering.end()) {
            QCC_DbgPrintf(("WFDTransport::NewDiscoveryOp(): Cancel of non-existent namePrefix \"%s\"", namePrefix.c_str()));
        } else {
            QCC_DbgPrintf(("WFDTransport::NewDiscoveryOp(): Unregistering discovery of namePrefix \"%s\"", namePrefix.c_str()));
            m_discovering.erase(i);
        }
    }

    isFirst = first;
    return m_discovering.empty();
}

bool WFDTransport::NewAdvertiseOp(AdvertiseOp op, qcc::String name, bool& isFirst)
{
    QCC_DbgTrace(("WFDTransport::NewAdvertiseOp()"));

    bool first = false;

    if (op == ENABLE_ADVERTISEMENT) {
        QCC_DbgPrintf(("WFDTransport::NewAdvertiseOp(): Registering advertisement of namePrefix \"%s\"", name.c_str()));
        first = m_advertising.empty();
        m_advertising.push_back(name);
    } else {
        list<qcc::String>::iterator i = find(m_advertising.begin(), m_advertising.end(), name);
        if (i == m_advertising.end()) {
            QCC_DbgPrintf(("WFDTransport::NewAdvertiseOp(): Cancel of non-existent name \"%s\"", name.c_str()));
        } else {
            QCC_DbgPrintf(("WFDTransport::NewAdvertiseOp(): Unregistering advertisement of namePrefix \"%s\"", name.c_str()));
            m_advertising.erase(i);
        }
    }

    isFirst = first;
    return m_advertising.empty();
}

bool WFDTransport::NewListenOp(ListenOp op, qcc::String normSpec)
{
    QCC_DbgTrace(("WFDTransport::NewListenOp()"));

    if (op == START_LISTEN) {
        QCC_DbgPrintf(("WFDTransport::NewListenOp(): Registering listen of normSpec \"%s\"", normSpec.c_str()));
        m_listening.push_back(normSpec);

    } else {
        list<qcc::String>::iterator i = find(m_listening.begin(), m_listening.end(), normSpec);
        if (i == m_listening.end()) {
            QCC_DbgPrintf(("WFDTransport::NewAdvertiseOp(): StopListen of non-existent spec \"%s\"", normSpec.c_str()));
        } else {
            QCC_DbgPrintf(("WFDTransport::NewAdvertiseOp(): StopListen of normSpec \"%s\"", normSpec.c_str()));
            m_listening.erase(i);
        }
    }

    return m_listening.empty();
}

void WFDTransport::EnableDiscovery(const char* namePrefix)
{
    QCC_DbgTrace(("WFDTransport::EnableDiscovery()"));

    /*
     * We only want to allow this call to proceed if we have a running server
     * accept thread that isn't in the process of shutting down.  We use the
     * thread response from IsRunning to give us an idea of what our server
     * accept (Run) thread is doing.  See the comment in Start() for details
     * about what IsRunning actually means, which might be subtly different from
     * your intuitition.
     *
     * If we see IsRunning(), the thread might actually have gotten a Stop(),
     * but has not yet exited its Run routine and become STOPPING.  To plug this
     * hole, we need to check IsRunning() and also m_stopping, which is set in
     * our Stop() method.
     */
    if (IsRunning() == false || m_stopping == true) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("WFDTransport::EnableDiscovery(): Not running or stopping; exiting"));
        return;
    }

    QueueEnableDiscovery(namePrefix);
}

void WFDTransport::QueueEnableDiscovery(const char* namePrefix)
{
    QCC_DbgTrace(("WFDTransport::QueueEnableDiscovery()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = ENABLE_DISCOVERY_INSTANCE;
    listenRequest.m_requestParam = namePrefix;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    m_listenRequests.push(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

    /*
     * Wake the server accept loop thread up so it will process the request we
     * just queued.
     */
    Alert();
}

void WFDTransport::DisableDiscovery(const char* namePrefix)
{
    QCC_DbgTrace(("WFDTransport::DisableDiscovery()"));

    /*
     * We only want to allow this call to proceed if we have a running server
     * accept thread that isn't in the process of shutting down.  We use the
     * thread response from IsRunning to give us an idea of what our server
     * accept (Run) thread is doing.  See the comment in Start() for details
     * about what IsRunning actually means, which might be subtly different from
     * your intuitition.
     *
     * If we see IsRunning(), the thread might actually have gotten a Stop(),
     * but has not yet exited its Run routine and become STOPPING.  To plug this
     * hole, we need to check IsRunning() and also m_stopping, which is set in
     * our Stop() method.
     */
    if (IsRunning() == false || m_stopping == true) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("WFDTransport::DisbleDiscovery(): Not running or stopping; exiting"));
        return;
    }

    QueueDisableDiscovery(namePrefix);
}

void WFDTransport::QueueDisableDiscovery(const char* namePrefix)
{
    QCC_DbgTrace(("WFDTransport::QueueDisableDiscovery()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = DISABLE_DISCOVERY_INSTANCE;
    listenRequest.m_requestParam = namePrefix;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    m_listenRequests.push(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

    /*
     * Wake the server accept loop thread up so it will process the request we
     * just queued.
     */
    Alert();
}

QStatus WFDTransport::EnableAdvertisement(const qcc::String& advertiseName, bool quietly, TransportMask transports)
{
    QCC_DbgTrace(("WFDTransport::EnableAdvertisement()"));

    /*
     * We only want to allow this call to proceed if we have a running server
     * accept thread that isn't in the process of shutting down.  We use the
     * thread response from IsRunning to give us an idea of what our server
     * accept (Run) thread is doing.  See the comment in Start() for details
     * about what IsRunning actually means, which might be subtly different from
     * your intuitition.
     *
     * If we see IsRunning(), the thread might actually have gotten a Stop(),
     * but has not yet exited its Run routine and become STOPPING.  To plug this
     * hole, we need to check IsRunning() and also m_stopping, which is set in
     * our Stop() method.
     */
    if (IsRunning() == false || m_stopping == true) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("WFDTransport::EnableAdvertisement(): Not running or stopping; exiting"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    QueueEnableAdvertisement(advertiseName);
    return ER_OK;
}

void WFDTransport::QueueEnableAdvertisement(const qcc::String& advertiseName)
{
    QCC_DbgTrace(("WFDTransport::QueueEnableAdvertisement()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = ENABLE_ADVERTISEMENT_INSTANCE;
    listenRequest.m_requestParam = advertiseName;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    m_listenRequests.push(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

    /*
     * Wake the server accept loop thread up so it will process the request we
     * just queued.
     */
    Alert();
}

void WFDTransport::DisableAdvertisement(const qcc::String& advertiseName, TransportMask transports)
{
    QCC_DbgTrace(("WFDTransport::DisableAdvertisement()"));

    /*
     * We only want to allow this call to proceed if we have a running server
     * accept thread that isn't in the process of shutting down.  We use the
     * thread response from IsRunning to give us an idea of what our server
     * accept (Run) thread is doing.  See the comment in Start() for details
     * about what IsRunning actually means, which might be subtly different from
     * your intuitition.
     *
     * If we see IsRunning(), the thread might actually have gotten a Stop(),
     * but has not yet exited its Run routine and become STOPPING.  To plug this
     * hole, we need to check IsRunning() and also m_stopping, which is set in
     * our Stop() method.
     */
    if (IsRunning() == false || m_stopping == true) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("WFDTransport::DisableAdvertisement(): Not running or stopping; exiting"));
        return;
    }

    QueueDisableAdvertisement(advertiseName);
}

void WFDTransport::QueueDisableAdvertisement(const qcc::String& advertiseName)
{
    QCC_DbgTrace(("WFDTransport::QueueDisableAdvertisement()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = DISABLE_ADVERTISEMENT_INSTANCE;
    listenRequest.m_requestParam = advertiseName;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    m_listenRequests.push(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

    /*
     * Wake the server accept loop thread up so it will process the request we
     * just queued.
     */
    Alert();
}

void WFDTransport::P2PNameServiceCallback(const qcc::String& guid, qcc::String& name, uint8_t timer)
{
    QCC_DbgTrace(("WFDTransport::P2PNameServiceCallback(): guid = \"%s\", timer = %d", guid.c_str(), timer));

    /*
     * Whenever the P2P name service receives a message indicating that a
     * bus-name is out on the network via pre-association service discovery, it
     * sends a message back to us via this callback that will conatin the GUID
     * of the remote daemon thad did the advertisement, the list of well-known
     * names being advertised and a validation timer.
     *
     * Although it may seem that this method and the P2PConManNameCallback() are
     * redundant, they actually come from different places and serve different
     * functions.  This method receives pre-association service discovery callbacks
     * and contains no layer three addressing information.  The other callback
     * receives IP name service-related information and does include layer three
     * addressing information.
     *
     * Because they are fundamentally different (layer two versus layer three)
     * the busAddr/connectSpec provided back to clients is different.  Here, we
     * have no busAddr since there is no layer three information, so we provide
     * our layer two mapping key (the GUID of the remote daemon that advertised
     * the name we just found) back to the client.
     *
     * If the client decides to JoinSession as a result of the advertisement we
     * are about to pass on, the daemon does a Connect() where we notice that
     * the connectSpec provides a GUID.  This tells us that we are bringing up a
     * new link and we need to discover the layer three addressing before
     * continuing.
     */
    qcc::String connectSpec = qcc::String(GetTransportName()) + qcc::String(":guid=") + guid;

    /*
     * Let AllJoyn know that we've found a service.
     */
    if (m_listener) {
        QCC_DbgPrintf(("WFDTransport::P2PNameServiceCallback(): Call listener with busAddr \"%s\", timer %d.", connectSpec.c_str(), timer));

        std::vector<qcc::String> wkns;
        wkns.push_back(name);

        m_listener->FoundNames(connectSpec, guid, TRANSPORT_WFD, &wkns, timer);
    }
}

void WFDTransport::P2PConManNameCallback(const qcc::String& busAddr, const qcc::String& guid, std::vector<qcc::String>& nameList, uint8_t timer)
{
    QCC_DbgTrace(("WFDTransport::P2PConManNameCallback(): busAddr = \"%s\", guid = \"%s\", timer = %d", busAddr.c_str(), guid.c_str(), timer));

    QCC_DEBUG_ONLY(
        for (uint32_t i = 0; i < nameList.size(); ++i) {
            QCC_DbgPrintf(("WFDTransport::P2PConManNameCallback(): nameList[%d] = \"%s\"", i, nameList[i].c_str()));
        }
        );

    /*
     * Whenever the P2P connection manager receives a message indicating that a
     * bus-name is out on the network via the IP name service, it sends a
     * message back to us via this callback that will conatin a bus address
     * (with an IP address and port0 the GUID of the remote daemon thad did the
     * advertisement, the list of well-known names being advertised and a
     * validation timer.
     *
     * Although it may seem that this method and the P2PNameServiceCallback()
     * are redundant, they actually come from different places and serve
     * different functions.  This method receives IP name service-related
     * information that includes layer three addressing information while the
     * other method gets pre-association service discovery callbacks that no
     * layer three addressing information.
     *
     * Because they are fundamentally different (layer three here versus layer
     * two there) the busAddr/connectSpec provided back to clients is different.
     * Here, we have a busAddr containing layer three information, so we pass it
     * on back to the client.
     *
     * If the client decides to JoinSession as a result of the advertisement we
     * are about to pass on, the daemon does a Connect() where we notice that
     * the connectSpec provides IP addressing information.  This tells us that
     * we are "borrowing" an existing link and we don't need to go through the
     * gyrations of discovering the layer three addressing.
     */
    qcc::String connectSpec = qcc::String(GetTransportName()) + qcc::String(":") + busAddr;

    /*
     * Let AllJoyn know that we've found a service through our "alternate
     * IP channel."
     */
    if (m_listener) {
        m_listener->FoundNames(connectSpec, guid, TRANSPORT_WFD, &nameList, timer);
    }
}

void WFDTransport::P2PConManStateCallback(P2PConMan::LinkState state, const qcc::String& interface)
{
    QCC_DbgTrace(("WFDTransport::P2PConManStateCallback(): state = %d, interface = \"%s\"", state, interface.c_str()));

    /*
     * Whenever the P2P connection manager notices a link coming up or going down
     * it calls us back here to let us know.
     */
}

} // namespace ajn
