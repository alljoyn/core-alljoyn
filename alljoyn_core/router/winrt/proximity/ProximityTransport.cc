/**
 * @file
 * ProximityTransport is a specialization of class Transport for daemons talking over WinRT proximity API.
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014, AllSeen Alliance. All rights reserved.
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
#include "ProximityTransport.h"
#include "ProximityNameService.h"

#define QCC_MODULE "PROXIMITY"

using namespace std;
using namespace qcc;
namespace ajn {

/**
 * Name of transport used in transport specs.
 */
const char* ProximityTransport::TransportName = "proximity";

/*
 * An endpoint class to handle the details of authenticating a connection in a
 * way that avoids denial of service attacks.
 */
class _ProximityEndpoint : public _RemoteEndpoint {
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

    _ProximityEndpoint(ProximityTransport* transport,
                       BusAttachment& bus,
                       bool incoming,
                       const qcc::String connectSpec,
                       qcc::SocketFd sock,
                       const qcc::IPAddress& ipAddr,
                       uint16_t port)
        : _RemoteEndpoint(bus, incoming, connectSpec, &m_stream, "proximity"),
        m_transport(transport),
        m_sideState(SIDE_INITIALIZED),
        m_authState(AUTH_INITIALIZED),
        m_epState(EP_INITIALIZED),
        m_tStart(qcc::Timespec(0)),
        m_authThread(this),
        m_stream(sock),
        m_ipAddr(ipAddr),
        m_port(port),
        m_wasSuddenDisconnect(!incoming) { }

    virtual ~_ProximityEndpoint() { }

    void SetStartTime(qcc::Timespec tStart) { m_tStart = tStart; }
    qcc::Timespec GetStartTime(void) { return m_tStart; }
    QStatus Authenticate(void);
    void AuthStop(void);
    void AuthJoin(void);
    const qcc::IPAddress& GetIPAddress() { return m_ipAddr; }
    uint16_t GetPort() { return m_port; }

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
        AuthThread(_ProximityEndpoint* conn) : Thread("auth"), m_endpoint(conn) { }
      private:
        virtual qcc::ThreadReturn STDCALL Run(void* arg);

        _ProximityEndpoint* m_endpoint;
    };

    ProximityTransport* m_transport;  /**< The server holding the connection */
    volatile SideState m_sideState;   /**< Is this an active or passive connection */
    volatile AuthState m_authState;   /**< The state of the endpoint authentication process */
    volatile EndpointState m_epState; /**< The state of the endpoint authentication process */
    qcc::Timespec m_tStart;           /**< Timestamp indicating when the authentication process started */
    AuthThread m_authThread;          /**< Thread used to do blocking calls during startup */
    qcc::SocketStream m_stream;       /**< Stream used by authentication code */
    qcc::IPAddress m_ipAddr;          /**< Remote IP address. */
    uint16_t m_port;                  /**< Remote port. */
    bool m_wasSuddenDisconnect;       /**< If true, assumption is that any disconnect is unexpected due to lower level error */
};

QStatus _ProximityEndpoint::Authenticate(void)
{
    QCC_DbgTrace(("ProximityEndpoint::Authenticate()"));
    /*
     * Start the authentication thread.
     */
    QStatus status = m_authThread.Start(this);
    if (status != ER_OK) {
        m_authState = AUTH_FAILED;
    }
    return status;
}

void _ProximityEndpoint::AuthStop(void)
{
    QCC_DbgTrace(("ProximityEndpoint::AuthStop()"));

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

void _ProximityEndpoint::AuthJoin(void)
{
    QCC_DbgTrace(("ProximityEndpoint::AuthJoin()"));

    /*
     * Join the auth thread to stop executing.  All threads must be joined in
     * order to communicate their return status.  The auth thread is no exception.
     * This is done in a lazy fashion from the main server accept loop, where we
     * cleanup every time through the loop.
     */
    m_authThread.Join();
}

void* _ProximityEndpoint::AuthThread::Run(void* arg)
{
    QCC_DbgTrace(("ProximityEndpoint::AuthThread::Run()"));

    m_endpoint->m_authState = AUTH_AUTHENTICATING;

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
    QStatus status = m_endpoint->m_stream.PullBytes(&byte, 1, nbytes);
    if ((status != ER_OK) || (nbytes != 1) || (byte != 0)) {
        m_endpoint->m_stream.Close();
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
        m_endpoint->m_authState = AUTH_FAILED;
        return (void*)ER_FAIL;
    }

    /* Initialized the features for this endpoint */
    m_endpoint->GetFeatures().isBusToBus = false;
    m_endpoint->GetFeatures().isBusToBus = false;
    m_endpoint->GetFeatures().handlePassing = false;

    /* Run the actual connection authentication code. */
    qcc::String authName;
    qcc::String redirection;
    status = m_endpoint->Establish("ANONYMOUS", authName, redirection);
    if (status != ER_OK) {
        m_endpoint->m_stream.Close();
        QCC_LogError(status, ("Failed to establish proximity endpoint"));

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
        return (void*)status;
    }

    /*
     * Tell the transport that the authentication has succeeded and that it can
     * now bring the connection up.
     */
    ProximityEndpoint proxEp = ProximityEndpoint::wrap(m_endpoint);
    m_endpoint->m_transport->Authenticated(proxEp);

    QCC_DbgTrace(("ProximityEndpoint::AuthThread::Run(): Returning"));

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
    if (m_endpoint->m_transport->m_pns != nullptr) {
        m_endpoint->m_transport->m_pns->IncreaseP2PConnectionRef();
    }
    return (void*)status;
}

ProximityTransport::ProximityTransport(BusAttachment& bus)
    : Thread("ProximityTransport"), m_bus(bus), m_pns(nullptr), m_stopping(false), m_listener(0), m_foundCallback(m_listener),
    m_isAdvertising(false), m_isDiscovering(false), m_isListening(false), m_nsReleaseCount(0)
{
    QCC_DbgTrace(("ProximityTransport::ProximityTransport()"));
    /*
     * We know we are daemon code, so we'd better be running with a daemon
     * router.  This is assumed elsewhere.
     */
    assert(m_bus.GetInternal().GetRouter().IsDaemon());
}

ProximityTransport::~ProximityTransport()
{
    QCC_DbgTrace(("ProximityTransport::~ProximityTransport()"));
    Stop();
    Join();
    m_pns = nullptr;
}

void ProximityTransport::Authenticated(ProximityEndpoint& conn)
{
    QCC_DbgTrace(("ProximityTransport::Authenticated()"));

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

    list<ProximityEndpoint>::iterator i = find(m_authList.begin(), m_authList.end(), conn);
    assert(i != m_authList.end() && "ProximityTransport::Authenticated(): Conn not on m_authList");

    /*
     * Note here that we have not yet marked the authState as AUTH_SUCCEEDED so
     * this is a point in time where the authState can be AUTH_AUTHENTICATING
     * and the endpoint can be on the endpointList and not the authList.
     */
    m_authList.erase(i);
    m_endpointList.push_back(conn);

    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    conn->SetListener(this);
    QStatus status = conn->Start();
    if (status != ER_OK) {
        QCC_LogError(status, ("ProximityTransport::Authenticated(): Failed to start proximity endpoint"));
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

QStatus ProximityTransport::Start()
{
    Config* config = ConfigDB::GetConfigDB();
    /*
     * Get configuration item that control whether or not to use IPv4, IPv6 and broadcasts.
     */
    bool enableIPv4 = !config->GetFlag("ns_disable_ipv4");
    bool enableIPv6 = !config->GetFlag("ns_disable_ipv6");
    bool disableBroadcast = config->GetFlag("ns_disable_directed_broadcast");

    QCC_DbgTrace(("ProximityTransport::Start() ipv4=%s ipv6=%s", enableIPv4 ? "true" : "false", enableIPv6 ? "true" : "false"));

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
        QCC_LogError(ER_BUS_BUS_ALREADY_STARTED, ("ProximityTransport::Start(): Already started"));
        return ER_BUS_BUS_ALREADY_STARTED;
    }

    /*
     * In order to pass the IsRunning() gate above, there must be no server
     * accept thread running.  Running includes a thread that has been asked to
     * stop but has not been Join()ed yet.  So we know that there is no thread
     * and that either a Start() has never happened, or a Start() followed by a
     * Stop() and a Join() has happened.  Since Join() does a Thread::Join and
     * then deletes the name service, it is possible that a Join() done on one
     * thread is done enough to pass the gate above, but has not yet finished
     * deleting the name service instance when a Start() comes in on another
     * thread.  Because of this (rare and unusual) possibility we also check the
     * name service instance and return an error if we find it non-zero.  If the
     * name service is NULL, the Stop() and Join() is totally complete and we
     * can safely proceed.
     */
    if (m_pns != nullptr) {
        QCC_LogError(ER_BUS_BUS_ALREADY_STARTED, ("ProximityTransport::Start(): Name service already started"));
        return ER_BUS_BUS_ALREADY_STARTED;
    }
    /*
     * Get the guid from the bus attachment which will act as the globally unique
     * ID of the daemon.
     */
    qcc::String guidStr = m_bus.GetInternal().GetGlobalGUID().ToString();
    m_pns = ref new ProximityNameService(guidStr);
    assert(m_pns != nullptr);
    m_nsReleaseCount = 0;
    m_pns->Start();
    m_pns->RegisterProximityListener(this);
    m_stopping = false;

    /*
     * Tell the name service to call us back on our FoundCallback method when
     * we hear about a new well-known bus name.
     */
    m_pns->SetCallback(
        new CallbackImpl<FoundCallback, void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint8_t>
            (&m_foundCallback, &FoundCallback::Found));

    /*
     * Start the server accept loop through the thread base class.  This will
     * close or open the IsRunning() gate we use to control access to our
     * public API.
     */
    return Thread::Start();
}

QStatus ProximityTransport::Stop(void)
{
    QCC_DbgTrace(("ProximityTransport::Stop()"));

    /*
     * It is legal to call Stop() more than once, so it must be possible to
     * call Stop() on a stopped transport.
     */
    m_stopping = true;

    /*
     * Tell the name service to stop calling us back if it's there (we may get
     * called more than once in the chain of destruction) so the pointer is not
     * required to be non-NULL.
     */
    if (m_pns != nullptr) {
        m_pns->SetCallback(NULL);
    }

    /*
     * Tell the server accept loop thread to shut down through the thead
     * base class.
     */
    QStatus status = Thread::Stop();
    if (status != ER_OK) {
        QCC_LogError(status, ("ProximityTransport::Stop(): Failed to Stop() server thread"));
        return status;
    }

    m_endpointListLock.Lock(MUTEX_CONTEXT);

    /*
     * Ask any authenticating endpoints to shut down and exit their threads.  By its
     * presence on the m_authList, we know that the endpoint is authenticating and
     * the authentication thread has responsibility for dealing with the endpoint
     * data structure.  We call Stop() to stop that thread from running.  The
     * endpoint Rx and Tx threads will not be running yet.
     */
    for (list<ProximityEndpoint>::iterator i = m_authList.begin(); i != m_authList.end(); ++i) {
        (*i)->AuthStop();
    }

    /*
     * Ask any running endpoints to shut down and exit their threads.  By its
     * presence on the m_endpointList, we know that authentication is compete and
     * the Rx and Tx threads have responsibility for dealing with the endpoint
     * data structure.  We call Stop() to stop those threads from running.  Since
     * the connnection is on the m_endpointList, we know that the authentication
     * thread has handed off responsibility.
     */
    for (list<ProximityEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        (*i)->Stop();
    }

    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    /*
     * The use model for ProximityTransport is that it works like a thread.
     * There is a call to Start() that spins up the server accept loop in order
     * to get it running.  When someone wants to tear down the transport, they
     * call Stop() which requests the transport to stop.  This is followed by
     * Join() which waits for all of the threads to actually stop.
     *
     * The name service should play by those rules as well.  We allocate and
     * initialize it in Start(), which will spin up the main thread there.
     * We need to Stop() the name service here and Join its thread in
     * ProximityTransport::Join().  If someone just deletes the transport
     * there is an implied Stop() and Join() so it behaves correctly.
     */

    int count = qcc::IncrementAndFetch(&m_nsReleaseCount);
    if (count == 1) {
        if (m_pns != nullptr) {
            m_pns->Stop();
            m_pns->UnRegisterProximityListener(this);
            delete m_pns;
            m_pns = nullptr;
        }
    }
    return ER_OK;
}

QStatus ProximityTransport::Join(void)
{
    QCC_DbgTrace(("ProximityTransport::Join()"));

    /*
     * It is legal to call Join() more than once, so it must be possible to
     * call Join() on a joined transport.
     *
     * First, wait for the server accept loop thread to exit.
     */
    QStatus status = Thread::Join();
    if (status != ER_OK) {
        QCC_LogError(status, ("ProximityTransport::Join(): Failed to Join() server thread"));
        return status;
    }

    /*
     * A requred call to Stop() that needs to happen before this Join will ask
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
    for (list<ProximityEndpoint>::iterator i = m_authList.begin(); i != m_authList.end(); ++i) {
        (*i)->AuthJoin();
        (*i)->Invalidate();
    }
    m_authList.clear();

    /*
     * Any running endpoints have been asked it their threads in a previously
     * required Stop().  We need to Join() all of thesse threads here.  This
     * Join() will wait on the endpoint rx and tx threads to exit as opposed to
     * the joining of the auth thread we did above.
     */
    for (list<ProximityEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        (*i)->Join();
        (*i)->Invalidate();
    }
    m_endpointList.clear();

    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    /*
     * The use model for ProximityTransport is that it works like a thread.
     * There is a call to Start() that spins up the server accept loop in order
     * to get it running.  When someone wants to tear down the transport, they
     * call Stop() which requests the transport to stop.  This is followed by
     * Join() which waits for all of the threads to actually stop.
     *
     * The name service needs to play by the use model for the transport (see
     * Start()).  We allocate and initialize it in Start() so we need to Join
     * and delete the name service here.  Since there is an implied Join() in
     * the destructor we just delete the name service to play by the rules.
     */
    m_stopping = false;

    return ER_OK;
}

/*
 * The default interface for the name service to use.  The wildcard character
 * means to listen and transmit over all interfaces that are up and multicast
 * capable, with any IP address they happen to have.  This default also applies
 * to the search for listen address interfaces.
 */
static const char* INTERFACES_DEFAULT = "*";

QStatus ProximityTransport::GetListenAddresses(const SessionOpts& opts, std::vector<qcc::String>& busAddrs) const
{
    QCC_DbgTrace(("ProximityTransport::GetListenAddresses()"));

    /*
     * We are given a session options structure that defines the kind of
     * transports that are being sought.  TCP provides reliable traffic as
     * understood by the session options, so we only return someting if
     * the traffic type is TRAFFIC_MESSAGES or TRAFFIC_RAW_RELIABLE.  It's
     * not an error if we don't match, we just don't have anything to offer.
     */
    if (opts.traffic != SessionOpts::TRAFFIC_MESSAGES && opts.traffic != SessionOpts::TRAFFIC_RAW_RELIABLE) {
        QCC_DbgPrintf(("ProximityTransport::GetListenAddresses(): traffic mismatch"));
        return ER_OK;
    }

    /*
     * The other session option that we need to filter on is the transport
     * bitfield.  We have no easy way of figuring out if we are a wireless
     * local-area, wireless wide-area, wired local-area or local transport,
     * but we do exist, so we respond if the caller is asking for any of
     * those: cogito ergo some.
     */
    if (!(opts.transports & (TRANSPORT_WLAN | TRANSPORT_WWAN | TRANSPORT_LAN | TRANSPORT_WFD))) {
        QCC_DbgPrintf(("ProximityTransport::GetListenAddresses(): transport mismatch"));
        return ER_OK;
    }

    /*
     * The name service is allocated in Start(), Started by the call to Init()
     * in Start(), Stopped in our Stop() method and deleted in our Join().  In
     * this case, the transport will probably be started, and we will probably
     * find m_ns set, but there is no requirement to ensure this.  If m_ns is
     * NULL, we need to complain so the user learns to Start() the transport
     * before calling IfConfig.  A call to IsRunning() here is superfluous since
     * we really don't care about anything but the name service in this method.
     */
    if (m_pns == nullptr) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("ProximityTransport::GetListenAddresses(): NameService not initialized"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    qcc::String ipv6address;
    uint16_t port;
    m_pns->GetEndpoints(ipv6address, port);
    if (port) {
        qcc::String busAddr = "proximity:addr=" + ipv6address + ",port=" + U32ToString(port) + ",family=ipv6";
        busAddrs.push_back(busAddr);
    }

    /*
     * If we can get the list and walk it, we have succeeded.  It is not an
     * error to have no available interfaces.  In fact, it is quite expected
     * in a phone if it is not associated with an access point over wi-fi.
     */
    QCC_DbgPrintf(("ProximityTransport::GetListenAddresses(): done"));
    return ER_OK;
}

void ProximityTransport::EndpointExit(RemoteEndpoint& ep)
{
    /*
     * This is a callback driven from the remote endpoint thread exit function.
     * Our ProximityEndpoint inherits from class RemoteEndpoint and so when
     * either of the threads (transmit or receive) of one of our endpoints exits
     * for some reason, we get called back here.  We only get called if either
     * the tx or rx thread exits, which implies that they have been run.  It
     * turns out that in the case of an endpoint receiving a connection, it
     * means that authentication has succeeded.  In the case of an endpoint
     * doing the connect, the EndpointExit may have resulted from an
     * authentication error since authentication is done in the context of the
     * Connect()ing thread and may be reported through EndpointExit.
     */
    QCC_DbgTrace(("ProximityTransport::EndpointExit()"));

    ProximityEndpoint tep = ProximityEndpoint::cast(ep);

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
    if (m_pns != nullptr) {
        m_pns->DecreaseP2PConnectionRef();
    }
}

void ProximityTransport::ManageEndpoints(Timespec tTimeout)
{
    m_endpointListLock.Lock(MUTEX_CONTEXT);

    /*
     * Run through the list of connections on the authList and cleanup
     * any that are no longer running or are taking too long to authenticate
     * (we assume a denial of service attack in this case).
     */
    list<ProximityEndpoint>::iterator i = m_authList.begin();
    while (i != m_authList.end()) {
        ProximityEndpoint ep = *i;
        _ProximityEndpoint::AuthState authState = ep->GetAuthState();

        if (authState == _ProximityEndpoint::AUTH_FAILED) {
            /*
             * The endpoint has failed authentication and the auth thread is
             * gone or is going away.  Since it has failed there is no way this
             * endpoint is going to be started so we can get rid of it as soon
             * as we Join() the (failed) authentication thread.
             */
            QCC_DbgHLPrintf(("ProximityTransport::ManageEndpoints(): Scavenging failed authenticator"));
            ep->AuthJoin();
            i = m_authList.erase(i);
            ep->Invalidate();
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
             * exit.  we will then clean it up the next time through this loop.
             * In the hope that the thread can exit and we can catch its exit
             * here and now, we take our thread off the OS ready list (Sleep)
             * and let the other thread run before looping back.
             */
            QCC_DbgHLPrintf(("ProximityTransport::ManageEndpoints(): Scavenging slow authenticator"));
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
        ProximityEndpoint ep = *i;

        /*
         * We are only managing passive connections here, or active connections
         * that are done and are explicitly ready to be cleaned up.
         */
        _ProximityEndpoint::SideState sideState = ep->GetSideState();
        if (sideState == _ProximityEndpoint::SIDE_ACTIVE) {
            ++i;
            continue;
        }

        _ProximityEndpoint::AuthState authState = ep->GetAuthState();
        _ProximityEndpoint::EndpointState endpointState = ep->GetEpState();

        if (authState == _ProximityEndpoint::AUTH_SUCCEEDED) {
            /*
             * The endpoint has succeeded authentication and the auth thread is
             * gone or is going away.  Take this opportunity to join the auth
             * thread.  Since the auth thread promised not to touch the state
             * after setting AUTH_SUCCEEEDED, we can safely change the state
             * here since we now own the conn.  We do this through a method call
             * to enable this single special case where we are allowed to set
             * the state.
             */
            QCC_DbgHLPrintf(("ProximityTransport::ManageEndpoints(): Scavenging failed authenticator"));
            ep->AuthJoin();
            ep->SetAuthDone();
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
        if (endpointState == _ProximityEndpoint::EP_FAILED) {
            i = m_endpointList.erase(i);
            ep->Invalidate();
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
        if (endpointState == _ProximityEndpoint::EP_STOPPING) {
            ep->Join();
            i = m_endpointList.erase(i);
            ep->Invalidate();
            continue;
        }
        ++i;
    }
    m_endpointListLock.Unlock(MUTEX_CONTEXT);
}

void* ProximityTransport::Run(void* arg)
{
    QCC_DbgTrace(("ProximityTransport::Run()"));
    /*
     * This is the Thread Run function for our server accept loop.  We require
     * that the name service be started before the Thread that will call us
     * here.
     */
    assert(m_pns != nullptr);

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
    uint32_t maxAuth = config->GetLimit("max_incomplete_connections", ALLJOYN_MAX_INCOMPLETE_CONNECTIONS_TCP_DEFAULT);

    /*
     * maxConn is the maximum number of active connections possible over the
     * TCP transport.  If starting to process a new connection would mean
     * exceeding this number, we drop the new connection.
     */
    uint32_t maxConn = config->GetLimit("max_completed_connections", ALLJOYN_MAX_COMPLETED_CONNECTIONS_TCP_DEFAULT);

    QStatus status = ER_OK;

    while (!IsStopping()) {
        /*
         * We require that the name service be created and started before the
         * Thread that called us here; and we require that the name service stay
         * around until after we leave.
         */
        assert(m_pns != nullptr);

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
            checkEvents.push_back(new Event(i->second, Event::IO_READ, false));
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

                QCC_DbgHLPrintf(("ProximityTransport::Run(): Accepting connection newSock=%d", newSock));

                QCC_DbgPrintf(("ProximityTransport::Run(): maxAuth == %d", maxAuth));
                QCC_DbgPrintf(("ProximityTransport::Run(): maxConn == %d", maxConn));
                QCC_DbgPrintf(("ProximityTransport::Run(): mAuthList.size() == %d", m_authList.size()));
                QCC_DbgPrintf(("ProximityTransport::Run(): mEndpointList.size() == %d", m_endpointList.size()));
                assert(m_authList.size() + m_endpointList.size() <= maxConn);

                /*
                 * Do we have a slot available for a new connection?  If so, use
                 * it.
                 */
                m_endpointListLock.Lock(MUTEX_CONTEXT);
                if ((m_authList.size() < maxAuth) && (m_authList.size() + m_endpointList.size() < maxConn)) {
                    bool truthiness = true;
                    ProximityEndpoint conn(this, m_bus, truthiness, "", newSock, remoteAddr, remotePort);
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
                    m_authList.push_front(conn);
                    status = conn->Authenticate();
                    if (status != ER_OK) {
                        m_authList.pop_front();
                        conn->Invalidate();
                    }
                    m_endpointListLock.Unlock(MUTEX_CONTEXT);
                } else {
                    m_endpointListLock.Unlock(MUTEX_CONTEXT);
                    qcc::Shutdown(newSock);
                    qcc::Close(newSock);
                    status = ER_AUTH_FAIL;
                    QCC_LogError(status, ("ProximityTransport::Run(): No slot for new connection"));
                }
            }

            /*
             * Accept returns ER_WOULDBLOCK when all of the incoming connections have been handled
             */
            if (ER_WOULDBLOCK == status) {
                status = ER_OK;
            }

            if (status != ER_OK) {
                QCC_LogError(status, ("ProximityTransport::Run(): Error accepting new connection. Ignoring..."));
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
         * with whether or not to enable TCP listeners and the name service
         * UDP listeners.
         */
        RunListenMachine();
    }

    /*
     * If we're stopping, it is our responsibility to clean up the list of FDs
     * we are listening to.  Since we've gotten a Stop() and are exiting the
     * server loop, and FDs are added in the server loop, this is the place to
     * get rid of them.  We don't have to take the list lock since a Stop()
     * request to the ProximityTransport is required to lock out any new
     * requests that may possibly touch the listen FDs list.
     */
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    for (list<pair<qcc::String, SocketFd> >::iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
        qcc::Shutdown(i->second);
        qcc::Close(i->second);
    }
    m_listenFds.clear();
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("ProximityTransport::Run is exiting status=%s", QCC_StatusText(status)));
    return (void*) status;
}

/*
 * The purpose of this code is really to ensure that we don't have any listeners
 * active on Android systems if we have no ongoing advertisements.  This is to
 * satisfy a requirement driven from the Android Compatibility Test Suite (CTS)
 * which fails systems that have processes listening for TCP connections when
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
 * our TCP listeners, and therefore remove all listeners from the system.  Since
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
 *     information from the name service.  Finally, we shut down our TCP
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
void ProximityTransport::RunListenMachine(void)
{
    QCC_DbgPrintf(("ProximityTransport::RunListenMachine()"));

    while (m_listenRequests.empty() == false) {
        QCC_DbgPrintf(("TCPTransport::RunListenMachine(): Do request."));

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

void ProximityTransport::StartListenInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("ProximityTransport::StartListenInstance()"));

    /*
     * We have a new StartListen request, so save the listen spec so we
     * can restart the listen if we stop advertising.
     */
    NewListenOp(START_LISTEN, listenRequest.m_requestParam);

    /*
     * If we're running on Windows, we always start listening immediately
     * since Windows uses TCP as the client to daemon communication link.
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

void ProximityTransport::StopListenInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("ProximityTransport::StopListenInstance()"));

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
        QCC_LogError(ER_FAIL, ("ProximityTransport::StopListenInstance(): No listeners with outstanding advertisements."));
        vector<qcc::String> names;
        for (list<qcc::String>::iterator i = m_advertising.begin(); i != m_advertising.end(); ++i) {
            names.push_back(*i);
        }
        m_pns->DisableAdvertisement(names);
    }

    /*
     * Execute the code that will actually tear down the specified
     * listening endpoint.  Note that we always stop listening
     * immediately since that is Good (TM) from a power and CTS point of
     * view.  We only delay starting to listen.
     */
    DoStopListen(listenRequest.m_requestParam);
}

void ProximityTransport::EnableAdvertisementInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("ProximityTransport::EnableAdvertisementInstance()"));

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
        } else {
            QCC_LogError(ER_FAIL, ("ProximityTransport::EnableAdvertisementInstance(): Advertise with no TCP listeners"));
            return;
        }
    }

    /*
     * We think we're ready to send the advertisement.  Are we really?
     */
    assert(m_isListening);
    assert(m_pns != nullptr);
    m_pns->EnableAdvertisement(listenRequest.m_requestParam);

    m_isAdvertising = true;
}

void ProximityTransport::DisableAdvertisementInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("ProximityTransport::DisableAdvertisementInstance()"));

    /*
     * We have a new disable advertisement request to deal with.  The first
     * order of business is to remove the well-known name from our saved list.
     */
    bool isFirst;
    bool isEmpty = NewAdvertiseOp(DISABLE_ADVERTISEMENT, listenRequest.m_requestParam, isFirst);

    /*
     * We always cancel any advertisement to allow the name service to
     * send out its lost advertisement message.
     */
    vector<qcc::String> names;
    names.push_back(listenRequest.m_requestParam);
    m_pns->DisableAdvertisement(names);

    /*
     * If it turns out that this was the last advertisement on our list, we need
     * to think about disabling our listeners and turning off the name service.
     * We only to this if there are no discovery instances in progress.
     */
    if (isEmpty && !m_isDiscovering) {

        /*
         * If we had the name service running, we must have had listeners
         * waiting for connections due to the name service.  We need to stop
         * them all now, but only if we are not running on a Windows box.
         * Windows needs the listeners running at all times since it uses
         * TCP for the client to daemon connections.
         */
        for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
            DoStopListen(*i);
        }
        m_isListening = false;
    }

    if (isEmpty) {
        m_isAdvertising = false;
    }
}

void ProximityTransport::EnableDiscoveryInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("ProximityTransport::EnableDiscoveryInstance()"));

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
        } else {
            QCC_LogError(ER_FAIL, ("ProximityTransport::EnableDiscoveryInstance(): Discover with no TCP listeners"));
            return;
        }
    }

    /*
     * We think we're ready to send the locate.  Are we really?
     */
    assert(m_isListening);
    assert(m_pns != nullptr);

    /*
     * When a bus name is advertised, the source may append a string that
     * identifies a specific instance of advertised name.  For example, one
     * might advertise something like
     *
     *   com.mycompany.myproduct.0123456789ABCDEF
     *
     * as a specific instance of the bus name,
     *
     *   com.mycompany.myproduct
     *
     * Clients of the system will want to be able to discover all specific
     * instances, so they need to do a wildcard search for bus name strings
     * that match the non-specific name, for example,
     *
     *   com.mycompany.myproduct*
     *
     * We automatically append the name service wildcard character to the end
     * of the provided string (which we call the namePrefix) before sending it
     * to the name service which forwards the request out over the net.
     */
    String starred = listenRequest.m_requestParam;
    starred.append('*');
    m_pns->EnableDiscovery(starred);
    m_isDiscovering = true;
}

void ProximityTransport::DisableDiscoveryInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("ProximityTransport::DisableDiscoveryInstance()"));

    /*
     * We have a new disable discovery request to deal with.  The first
     * order of business is to remove the well-known name from our saved list.
     */
    bool isFirst;
    bool isEmpty = NewDiscoveryOp(DISABLE_DISCOVERY, listenRequest.m_requestParam, isFirst);

    /*
     * There is no state in the name service with respect to ongoing discovery.
     * A discovery request just causes it to send a WHO-HAS message, so thre
     * is nothing to cancel down there.
     *
     * However, if it turns out that this was the last discovery operation on
     * our list, we need to think about disabling our listeners and turning off
     * the name service.  We only to this if there are no advertisements in
     * progress.
     */
    if (isEmpty && !m_isAdvertising) {

        /*
         * If we had the name service running, we must have had listeners
         * waiting for connections due to the name service.  We need to stop
         * them all now, but only if we are not running on a Windows box.
         * Windows needs the listeners running at all times since it uses
         * TCP for the client to daemon connections.
         */
        for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
            DoStopListen(*i);
        }
        m_isListening = false;
    }

    if (isEmpty) {
        m_isDiscovering = false;
    }
}

/*
 * The default address for use in listen specs.  INADDR_ANY means to listen
 * for TCP connections on any interfaces that are currently up or any that may
 * come up in the future.
 */
static const char* ADDR4_DEFAULT = "0.0.0.0";
static const char* ADDR6_DEFAULT = "0::0";

/*
 * The default port for use in listen specs.  This port is used by the TCP
 * listener to listen for incoming connection requests.
 */
static const uint16_t PORT_DEFAULT = 9957;

QStatus ProximityTransport::NormalizeListenSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    qcc::String family;

    /*
     * We don't make any calls that require us to be in any particular state
     * with respect to threading so we don't bother to call IsRunning() here.
     *
     * Take the string in inSpec, which must start with "proximity:" and parse it,
     * looking for comma-separated "key=value" pairs and initialize the
     * argMap with those pairs.
     */
    QStatus status = ParseArguments(GetTransportName(), inSpec, argMap);
    if (status != ER_OK) {
        return status;
    }
    /*
     * If the family was specified we will check that address matches otherwise
     * we will figure out the family from the address format.
     */
    map<qcc::String, qcc::String>::iterator iter = argMap.find("family");
    if (iter != argMap.end()) {
        family = iter->second;
    }

    iter = argMap.find("addr");
    if (iter == argMap.end()) {
        if (family.empty()) {
            family = "ipv4";
        }
        qcc::String addrString = (family == "ipv6") ? ADDR6_DEFAULT : ADDR4_DEFAULT;
        argMap["addr"] = addrString;
        outSpec = "proximity:addr=" + addrString;
    } else {
        /*
         * We have a value associated with the "addr" key.  Run it through
         * a conversion function to make sure it's a valid value.
         */
        IPAddress addr;
        status = addr.SetAddress(iter->second, false);
        if (status == ER_OK) {
            if (family.empty()) {
                family = addr.IsIPv6() ? "ipv6" : "ipv4";
            } else if (addr.IsIPv6() != (family == "ipv6")) {
                return ER_BUS_BAD_TRANSPORT_ARGS;
            }
            // Normalize address representation
            iter->second = addr.ToString();
            outSpec = "proximity:addr=" + iter->second;
        } else {
            return ER_BUS_BAD_TRANSPORT_ARGS;
        }
    }
    argMap["family"] = family;
    outSpec += ",family=" + family;

    iter = argMap.find("port");
    if (iter == argMap.end()) {
        qcc::String portString = U32ToString(PORT_DEFAULT);
        argMap["port"] = portString;
        outSpec += ",port=" + portString;
    } else {
        /*
         * We have a value associated with the "port" key.  Run it through
         * a conversion function to make sure it's a valid value.
         */
        uint32_t port = StringToU32(iter->second);
        if (port <= 0xffff) {
            iter->second = U32ToString(port);
            outSpec += ",port=" + iter->second;
        } else {
            return ER_BUS_BAD_TRANSPORT_ARGS;
        }
    }

    return ER_OK;
}

QStatus ProximityTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    /*
     * We don't make any calls that require us to be in any particular state
     * with respect to threading so we don't bother to call IsRunning() here.
     *
     * Unlike a listenSpec a transportSpec (actually a connectSpec) must have
     * a specific address (INADDR_ANY isn't a valid IP address to connect to).
     */
    QStatus status = NormalizeListenSpec(inSpec, outSpec, argMap);
    if (status != ER_OK) {
        return status;
    }

    /*
     * Since the only difference between a transportSpec and a listenSpec is
     * the presence of the address, we just check for the default address
     * and fail if we find it.
     */
    map<qcc::String, qcc::String>::iterator i = argMap.find("addr");
    assert(i != argMap.end());
    if ((i->second == ADDR4_DEFAULT) || (i->second == ADDR6_DEFAULT)) {
        return ER_BUS_BAD_TRANSPORT_ARGS;
    }

    return ER_OK;
}

QStatus ProximityTransport::Connect(const char* connSpec, const SessionOpts& opts, BusEndpoint& newEp)
{
    QCC_DbgHLPrintf(("ProximityTransport::Connect(): %s", connSpec));

    QStatus status = ER_OK;
    bool isConnected = false;

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("ProximityTransport::Connect(): Not running or stopping; exiting"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    /*
     * If we pass the IsRunning() gate above, we must have a server accept
     * thread spinning up or shutting down but not yet joined.  Since the name
     * service is created before the server accept thread is spun up, and
     * deleted after it is joined, we must have a valid name service or someone
     * isn't playing by the rules; so an assert is appropriate here.
     */
    qcc::String connectSpec = connSpec;
    assert(m_pns != nullptr);
    map<qcc::String, qcc::String> argMap;
    ParseArguments("proximity", connSpec, argMap);
    if (argMap.find("guid") != argMap.end()) {
        status = m_pns->EstasblishProximityConnection(argMap["guid"]);
        if (status == ER_OK) {
            if (!m_pns->GetPeerConnectSpec(argMap["guid"], connectSpec)) {
                status = ER_OS_ERROR;
            }
        }
    }
    if (status != ER_OK) {
        return status;
    }

    argMap.clear();
    /*
     * Parse and normalize the connectArgs.  When connecting to the outside
     * world, there are no reasonable defaults and so the addr and port keys
     * MUST be present.
     */
    qcc::String normSpec;
    status = NormalizeTransportSpec(connectSpec.c_str(), normSpec, argMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("ProximityTransport::Connect(): Invalid proximity connect spec \"%s\"", connectSpec));
        return status;
    }
    /*
     * These fields (addr, port, family) are all guaranteed to be present
     */
    IPAddress ipAddr(argMap.find("addr")->second);
    uint16_t port = StringToU32(argMap["port"]);
    qcc::AddressFamily family = argMap["family"] == "ipv6" ?  QCC_AF_INET6 : QCC_AF_INET;

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
     * looks like "addr=0.0.0.0,port=y".  If we detect this kind of connectSpec
     * we have to look at the currently up interfaces and see if any of them
     * match the address provided in the connectSpec.  If so, we are attempting
     * to connect to ourself and we must fail that request.
     */
    char anyspec[50];
    if (family == QCC_AF_INET) {
        snprintf(anyspec, sizeof(anyspec), "proximity:addr=0.0.0.0,port=%u,family=ipv4", port);
    } else {
        snprintf(anyspec, sizeof(anyspec), "proximity:addr=0::0,port=%u,family=ipv6", port);
    }
    qcc::String normAnySpec;
    map<qcc::String, qcc::String> normArgMap;
    status = NormalizeListenSpec(anyspec, normAnySpec, normArgMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("ProximityTransport::Connect(): Invalid INADDR_ANY connect spec"));
        return status;
    }

    /*
     * Look to see if we are already listening on the provided connectSpec
     * either explicitly or via the INADDR_ANY address.
     */
    QCC_DbgHLPrintf(("ProximityTransport::Connect(): Checking for connection to self"));
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    bool anyEncountered = false;
    for (list<pair<qcc::String, SocketFd> >::iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
        QCC_DbgHLPrintf(("ProximityTransport::Connect(): Checking listenSpec %s", i->first.c_str()));

        /*
         * If the provided connectSpec is already explicitly listened to, it is
         * an error.
         */
        if (i->first == normSpec) {
            m_listenFdsLock.Unlock(MUTEX_CONTEXT);
            QCC_DbgHLPrintf(("ProximityTransport::Connect(): Explicit connection to self"));
            return ER_BUS_ALREADY_LISTENING;
        }

        /*
         * If we are listening to INADDR_ANY and the supplied port, then we have
         * to look to the currently UP interfaces to decide if this call is bogus
         * or not.  Set a flag to remind us.
         */
        if (i->first == normAnySpec) {
            QCC_DbgHLPrintf(("ProximityTransport::Connect(): Possible implicit connection to self detected"));
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
        QCC_DbgHLPrintf(("ProximityTransport::Connect(): Checking for implicit connection to self"));
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
                QCC_DbgHLPrintf(("ProximityTransport::Connect(): Checking interface %s", entries[i].m_name.c_str()));
                if (entries[i].m_flags & qcc::IfConfigEntry::UP) {
                    QCC_DbgHLPrintf(("ProximityTransport::Connect(): Interface UP with addresss %s", entries[i].m_addr.c_str()));
                    IPAddress foundAddr(entries[i].m_addr);
                    if (foundAddr == ipAddr) {
                        QCC_DbgHLPrintf(("ProximityTransport::Connect(): Attempted connection to self; exiting"));
                        return ER_BUS_ALREADY_LISTENING;
                    }
                }
            }
        }
    }

    /*
     * This is a new not previously satisfied connection request, so attempt
     * to connect to the remote TCP address and port specified in the connectSpec.
     */
    SocketFd sockFd = -1;
    status = Socket(family, QCC_SOCK_STREAM, sockFd);
    if (status == ER_OK) {
        /* Turn off Nagle */
        status = SetNagle(sockFd, false);
    }

    if (status == ER_OK) {
        /*
         * We got a socket, now tell TCP to connect to the remote address and
         * port.
         */
        status = qcc::Connect(sockFd, ipAddr, port);
        if (status == ER_OK) {
            /*
             * We now have a TCP connection established, but DBus (the wire
             * protocol which we are using) requires that every connection,
             * irrespective of transport, start with a single zero byte.  This
             * is so that the Unix-domain socket transport used by DBus can pass
             * SCM_RIGHTS out-of-band when that byte is sent.
             */
            uint8_t nul = 0;
            size_t sent;

            status = Send(sockFd, &nul, 1, sent);
            if (status != ER_OK) {
                QCC_LogError(status, ("ProximityTransport::Connect(): Failed to send initial NUL byte"));
            }
            isConnected = true;
        } else {
            QCC_LogError(status, ("ProximityTransport::Connect(): Failed"));
        }
    } else {
        QCC_LogError(status, ("ProximityTransport::Connect(): qcc::Socket() failed"));
    }

    /*
     * The underlying transport mechanism is started, but we need to create a
     * ProximityEndpoint object that will orchestrate the movement of data across the
     * transport.
     */

    if (status == ER_OK) {
        bool falsiness = false;
        ProximityEndpoint conn(this, m_bus, falsiness, normSpec, sockFd, ipAddr, port);
        /*
         * On the active side of a connection, we don't need an authentication
         * thread to run since we have the caller thread.  We do have to put the
         * endpoint on the endpoint list to be assured that errors get logged.
         * By marking the connection as active, we prevent the server accept thread
         * from cleaning up this endpoint.  For consistency, we mark the endpoint
         * as authenticating to avoid ugly surprises.
         */
        conn->SetActive();
        conn->SetAuthenticating();
        m_endpointListLock.Lock(MUTEX_CONTEXT);
        m_endpointList.push_back(conn);
        m_endpointListLock.Unlock(MUTEX_CONTEXT);

        /* Initialized the features for this endpoint */
        conn->GetFeatures().isBusToBus = true;
        conn->GetFeatures().allowRemote = m_bus.GetInternal().AllowRemoteMessages();
        conn->GetFeatures().handlePassing = false;

        qcc::String authName;
        qcc::String redirection;

        /*
         * Go ahead and to the authentication in the context of this thread.  Even
         * though we have prevented the server accept loop from cleaning up our
         * endpoint by marking it as active, we keep the states consistent.
         */
        status = conn->Establish("ANONYMOUS", authName, redirection);
        if (status == ER_OK) {
            conn->SetListener(this);
            status = conn->Start();
            if (status == ER_OK) {
                if (m_pns != nullptr) {
                    m_pns->IncreaseP2PConnectionRef();
                }
                conn->SetEpStarted();
                conn->SetAuthDone();
            } else {
                conn->SetEpFailed();
                conn->SetAuthDone();
            }
        }
        /*
         * We put the endpoint into our list of active endpoints to make life
         * easier reporting problems up the chain of command behind the scenes
         * if we got an error during the authentication process and the endpoint
         * startup.  If we did get an error, we need to remove the endpoint since
         * we've asked to keep responsibility by doing a SetActive().
         */
        if (status != ER_OK) {
            QCC_LogError(status, ("ProximityTransport::Connect(): Start ProximityEndpoint failed"));

            m_endpointListLock.Lock(MUTEX_CONTEXT);
            list<ProximityEndpoint>::iterator i = find(m_endpointList.begin(), m_endpointList.end(), conn);
            if (i != m_endpointList.end()) {
                m_endpointList.erase(i);
            }
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            conn->Invalidate();
        }       else {
            newEp = BusEndpoint::cast(conn);
        }
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
        newEp->Invalidate();
    }

    return status;
}

QStatus ProximityTransport::Disconnect(const char* connectSpec)
{
    QCC_DbgHLPrintf(("ProximityTransport::Disconnect(): %s", connectSpec));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("ProximityTransport::Disconnect(): Not running or stopping; exiting"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    /*
     * If we pass the IsRunning() gate above, we must have a server accept
     * thread spinning up or shutting down but not yet joined.  Since the name
     * service is created before the server accept thread is spun up, and
     * deleted after it is joined, we must have a valid name service or someone
     * isn't playing by the rules; so an assert is appropriate here.
     */
    assert(m_pns != nullptr);

    /*
     * Higher level code tells us which connection is refers to by giving us the
     * same connect spec it used in the Connect() call.  We have to determine the
     * address and port in exactly the same way
     */
    qcc::String normSpec;
    map<qcc::String, qcc::String> argMap;
    QStatus status = NormalizeTransportSpec(connectSpec, normSpec, argMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("ProximityTransport::Disconnect(): Invalid TCP connect spec \"%s\"", connectSpec));
        return status;
    }

    IPAddress ipAddr(argMap.find("addr")->second); // Guaranteed to be there.
    uint16_t port = StringToU32(argMap["port"]);   // Guaranteed to be there.

    /*
     * Stop the remote endpoint.  Be careful here since calling Stop() on the
     * ProximityEndpoint is going to cause the transmit and receive threads of the
     * underlying RemoteEndpoint to exit, which will cause our EndpointExit()
     * to be called, which will walk the list of endpoints and delete the one
     * we are stopping.  Once we poke ep->Stop(), the pointer to ep must be
     * considered dead.
     */
    status = ER_BUS_BAD_TRANSPORT_ARGS;
    m_endpointListLock.Lock(MUTEX_CONTEXT);
    for (list<ProximityEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        if ((*i)->GetPort() == port && (*i)->GetIPAddress() == ipAddr) {
            ProximityEndpoint ep = *i;
            ep->SetSuddenDisconnect(false);
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            return ep->Stop();
        }
    }
    m_endpointListLock.Unlock(MUTEX_CONTEXT);
    return status;
}

void ProximityTransport::OnProximityDisconnected()
{
    QCC_DbgPrintf(("ProximityTransport::OnProximityDisconnected()"));
    m_endpointListLock.Lock(MUTEX_CONTEXT);
    for (list<ProximityEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        ProximityEndpoint ep = *i;
        ep->SetSuddenDisconnect(false);
        ep->Stop();
    }
    m_endpointListLock.Unlock(MUTEX_CONTEXT);
}

QStatus ProximityTransport::StartListen(const char* listenSpec)
{
    QCC_DbgPrintf(("ProximityTransport::StartListen()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("ProximityTransport::StartListen(): Not running or stopping; exiting"));
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
        QCC_LogError(status, ("ProximityTransport::StartListen(): Invalid TCP listen spec \"%s\"", listenSpec));
        return status;
    }

    QCC_DbgPrintf(("ProximityTransport::StartListen(): addr = \"%s\", port = \"%s\", family=\"%s\"",
                   argMap["addr"].c_str(), argMap["port"].c_str(), argMap["family"].c_str()));

    /*
     * The daemon code is in a state where it lags in functionality a bit with
     * respect to the common code.  Common supports the use of IPv6 addresses
     * but the name service is not quite ready for prime time.  Until the name
     * service can properly distinguish between various cases, we fail any
     * request to listen on an IPv6 address.
     */
    IPAddress ipAddress;
    status = ipAddress.SetAddress(argMap["addr"].c_str());
    if (status != ER_OK) {
        QCC_LogError(status, ("ProximityTransport::StartListen(): Unable to SetAddress(\"%s\")", argMap["addr"].c_str()));
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

void ProximityTransport::QueueStartListen(qcc::String& normSpec)
{
    QCC_DbgPrintf(("ProximityTransport::QueueStartListen()"));

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

void ProximityTransport::DoStartListen(qcc::String& normSpec)
{
    QCC_DbgPrintf(("ProximityTransport::DoStartListen()"));

    /*
     * Since the name service is created before the server accept thread is spun
     * up, and deleted after it is joined, we must have a valid name service or
     * someone isn't playing by the rules; so an assert is appropriate here.
     */
    assert(m_pns != nullptr);

    /*
     * Parse the normalized listen spec.  The easiest way to do this is to
     * re-normalize it.  If there's an error at this point, we have done
     * something wrong since the listen spec was presumably successfully
     * normalized before sending it in -- so we assert.
     */
    qcc::String spec;
    map<qcc::String, qcc::String> argMap;
    QStatus status = NormalizeListenSpec(normSpec.c_str(), spec, argMap);
    assert(status == ER_OK && "ProximityTransport::DoStartListen(): Invalid TCP listen spec");

    QCC_DbgPrintf(("ProximityTransport::DoStartListen(): addr = \"%s\", port = \"%s\", family=\"%s\"",
                   argMap["addr"].c_str(), argMap["port"].c_str(), argMap["family"].c_str()));

    m_listenFdsLock.Lock(MUTEX_CONTEXT);

    /*
     * Figure out what local address and port the listener should use.
     */
    IPAddress listenAddr(argMap["addr"]);
    uint16_t listenPort = StringToU32(argMap["port"]);
    qcc::AddressFamily family = argMap["family"] == "ipv6" ?  QCC_AF_INET6 : QCC_AF_INET;
    bool ephemeralPort = (listenPort == 0);

    /*
     * We have the name service work out of the way, so we can now create the
     * TCP listener sockets and set SO_REUSEADDR/SO_REUSEPORT so we don't have
     * to wait for four minutes to relaunch the daemon if it crashes.
     */
    SocketFd listenFd = -1;
    status = Socket(family, QCC_SOCK_STREAM, listenFd);
    if (status != ER_OK) {
        m_listenFdsLock.Unlock(MUTEX_CONTEXT);
        QCC_LogError(status, ("ProximityTransport::DoStartListen(): Socket() failed"));
        return;
    }
    /*
     * Set the SO_REUSEADDR socket option so we don't have to wait for four
     * minutes while the endpoint is in TIME_WAIT if we crash (or control-C).
     */
    status = qcc::SetReuseAddress(listenFd, true);
    if (status != ER_OK && status != ER_NOT_IMPLEMENTED) {
        m_listenFdsLock.Unlock(MUTEX_CONTEXT);
        QCC_LogError(status, ("ProximityTransport::DoStartListen(): SetReuseAddress() failed"));
        qcc::Close(listenFd);
        return;
    }
    /*
     * We call accept in a loop so we need the listenFd to non-blocking
     */
    status = qcc::SetBlocking(listenFd, false);
    if (status != ER_OK) {
        m_listenFdsLock.Unlock(MUTEX_CONTEXT);
        QCC_LogError(status, ("ProximityTransport::DoStartListen(): SetBlocking() failed"));
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
            normSpec = "proximity:addr=" + argMap["addr"] + "," + argMap["family"] + ",port=" + U32ToString(listenPort);
        }
        status = qcc::Listen(listenFd, MAX_LISTEN_CONNECTIONS);
        if (status == ER_OK) {
            QCC_DbgPrintf(("ProximityTransport::DoStartListen(): Listening on %s/%d", argMap["addr"].c_str(), listenPort));
            m_listenFds.push_back(pair<qcc::String, SocketFd>(normSpec, listenFd));
        } else {
            QCC_LogError(status, ("ProximityTransport::DoStartListen(): Listen failed"));
        }
    } else {
        QCC_LogError(status, ("ProximityTransport::DoStartListen(): Failed to bind to %s/%d", listenAddr.ToString().c_str(), listenPort));
    }
    m_pns->SetEndpoints(String::Empty, listenPort);
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

    /*
     * Signal the (probably) waiting run thread so it will wake up and add this
     * new socket to its list of sockets it is waiting for connections on.
     */
    if (status == ER_OK) {
        Alert();
    }
}

QStatus ProximityTransport::StopListen(const char* listenSpec)
{
    QCC_DbgPrintf(("ProximityTransport::StopListen()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("ProximityTransport::StopListen(): Not running or stopping; exiting"));
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
        QCC_LogError(status, ("ProximityTransport::StopListen(): Invalid TCP listen spec \"%s\"", listenSpec));
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

void ProximityTransport::QueueStopListen(qcc::String& normSpec)
{
    QCC_DbgPrintf(("ProximityTransport::QueueStopListen()"));

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

void ProximityTransport::DoStopListen(qcc::String& normSpec)
{
    QCC_DbgPrintf(("ProximityTransport::DoStopListen()"));

    /*
     * Find the (single) listen spec and remove it from the list of active FDs
     * used by the server accept loop (run thread).  This is okay to do since
     * we are assuming that, since we should only be called in the context of
     * the server accept loop, it knows that an FD will be deleted here.
     */
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    qcc::SocketFd stopFd = -1;
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

bool ProximityTransport::NewDiscoveryOp(DiscoveryOp op, qcc::String namePrefix, bool& isFirst)
{
    QCC_DbgPrintf(("ProximityTransport::NewDiscoveryOp()"));

    bool first = false;

    if (op == ENABLE_DISCOVERY) {
        QCC_DbgPrintf(("ProximityTransport::NewDiscoveryOp(): Registering discovery of namePrefix \"%s\"", namePrefix.c_str()));
        first = m_advertising.empty();
        m_discovering.push_back(namePrefix);
    } else {
        list<qcc::String>::iterator i = find(m_discovering.begin(), m_discovering.end(), namePrefix);
        if (i == m_discovering.end()) {
            QCC_DbgPrintf(("ProximityTransport::NewDiscoveryOp(): Cancel of non-existent namePrefix \"%s\"", namePrefix.c_str()));
        } else {
            QCC_DbgPrintf(("ProximityTransport::NewDiscoveryOp(): Unregistering discovery of namePrefix \"%s\"", namePrefix.c_str()));
            m_discovering.erase(i);
        }
    }

    isFirst = first;
    return m_discovering.empty();
}

bool ProximityTransport::NewAdvertiseOp(AdvertiseOp op, qcc::String name, bool& isFirst)
{
    QCC_DbgPrintf(("ProximityTransport::NewAdvertiseOp()"));

    bool first = false;

    if (op == ENABLE_ADVERTISEMENT) {
        QCC_DbgPrintf(("ProximityTransport::NewAdvertiseOp(): Registering advertisement of namePrefix \"%s\"", name.c_str()));
        first = m_advertising.empty();
        m_advertising.push_back(name);
    } else {
        list<qcc::String>::iterator i = find(m_advertising.begin(), m_advertising.end(), name);
        if (i == m_advertising.end()) {
            QCC_DbgPrintf(("ProximityTransport::NewAdvertiseOp(): Cancel of non-existent name \"%s\"", name.c_str()));
        } else {
            QCC_DbgPrintf(("ProximityTransport::NewAdvertiseOp(): Unregistering advertisement of namePrefix \"%s\"", name.c_str()));
            m_advertising.erase(i);
        }
    }

    isFirst = first;
    return m_advertising.empty();
}

bool ProximityTransport::NewListenOp(ListenOp op, qcc::String normSpec)
{
    QCC_DbgPrintf(("ProximityTransport::NewListenOp()"));

    if (op == START_LISTEN) {
        QCC_DbgPrintf(("ProximityTransport::NewListenOp(): Registering listen of normSpec \"%s\"", normSpec.c_str()));
        m_listening.push_back(normSpec);

    } else {
        list<qcc::String>::iterator i = find(m_listening.begin(), m_listening.end(), normSpec);
        if (i == m_listening.end()) {
            QCC_DbgPrintf(("ProximityTransport::NewAdvertiseOp(): StopListen of non-existent spec \"%s\"", normSpec.c_str()));
        } else {
            QCC_DbgPrintf(("ProximityTransport::NewAdvertiseOp(): StopListen of normSpec \"%s\"", normSpec.c_str()));
            m_listening.erase(i);
        }
    }

    return m_listening.empty();
}

void ProximityTransport::EnableDiscovery(const char* namePrefix)
{
    QCC_DbgPrintf(("ProximityTransport::EnableDiscovery()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("ProximityTransport::EnableDiscovery(): Not running or stopping; exiting"));
        return;
    }

    QueueEnableDiscovery(namePrefix);
}

void ProximityTransport::QueueEnableDiscovery(const char* namePrefix)
{
    QCC_DbgPrintf(("ProximityTransport::QueueEnableDiscovery()"));

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

void ProximityTransport::DisableDiscovery(const char* namePrefix)
{
    QCC_DbgPrintf(("ProximityTransport::DisableDiscovery()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("ProximityTransport::DisbleDiscovery(): Not running or stopping; exiting"));
        return;
    }

    QueueDisableDiscovery(namePrefix);
}

void ProximityTransport::QueueDisableDiscovery(const char* namePrefix)
{
    QCC_DbgPrintf(("ProximityTransport::QueueDisableDiscovery()"));

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

QStatus ProximityTransport::EnableAdvertisement(const qcc::String& advertiseName, bool quietly, TransportMask transports)
{
    QCC_DbgPrintf(("ProximityTransport::EnableAdvertisement()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("ProximityTransport::EnableAdvertisement(): Not running or stopping; exiting"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    QueueEnableAdvertisement(advertiseName);
    return ER_OK;
}

void ProximityTransport::QueueEnableAdvertisement(const qcc::String& advertiseName)
{
    QCC_DbgPrintf(("ProximityTransport::QueueEnableAdvertisement()"));

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

void ProximityTransport::DisableAdvertisement(const qcc::String& advertiseName, TransportMask transports)
{
    QCC_DbgPrintf(("ProximityTransport::DisableAdvertisement()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("ProximityTransport::DisableAdvertisement(): Not running or stopping; exiting"));
        return;
    }

    QueueDisableAdvertisement(advertiseName);
}

void ProximityTransport::QueueDisableAdvertisement(const qcc::String& advertiseName)
{
    QCC_DbgPrintf(("ProximityTransport::QueueDisableAdvertisement()"));

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

void ProximityTransport::FoundCallback::Found(const qcc::String& busAddr, const qcc::String& guid,
                                              std::vector<qcc::String>& nameList, uint8_t timer)
{
    /*
     * Whenever the name service receives a message indicating that a bus-name
     * is out on the network somewhere, it sends a message back to us via this
     * callback.  In order to avoid duplication of effort, the name service does
     * not manage a cache of names, but delegates that to the daemon having this
     * transport.  If the timer parameter is non-zero, it indicates that the
     * nameList (actually a vector of bus-name Strings) can be expected to be
     * valid for the value of timer in seconds.  If timer is zero, it means that
     * the bus names in the nameList are no longer available and should be
     * flushed out of the daemon name cache.
     *
     * The name service does not have a cache and therefore cannot time out
     * entries, but also delegates that task to the daemon.  It is expected that
     * remote daemons will send keepalive messages that the local daemon will
     * recieve, also via this callback.
     *
     * XXX Currently this transport has no clue how to handle an advertised
     * IPv6 address so we filter them out.  We should support IPv6.
     */

    if (m_listener) {
        m_listener->FoundNames(busAddr, guid, TRANSPORT_WFD, &nameList, timer);
    }
}

} // namespace ajn
