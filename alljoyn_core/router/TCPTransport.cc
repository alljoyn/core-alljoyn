/**
 * @file
 * TCPTransport is an implementation of TCPTransportBase for daemons.
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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
#include "BusController.h"
#include "ConfigDB.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "DaemonRouter.h"
#include "ns/IpNameService.h"
#include "TCPTransport.h"

/*
 * How the transport fits into the system
 * ======================================
 *
 * AllJoyn provides the concept of a Transport which provides a relatively
 * abstract way for the daemon to use different network mechanisms for getting
 * Messages from place to another.  Conceptually, think of, for example, a Unix
 * transport that moves bits using unix domain sockets and a TCP transport that
 * moves Messages over a TCP connection.
 *
 * In networking 101, one discovers that BSD sockets is oriented toward clients
 * and servers.  There are different sockets calls required for a program
 * implementing a server-side part and a client side part.  The server-side
 * listens for incoming connection requests and the client-side initiates the
 * requests.  AllJoyn clients are bus attachments that our Applications may use
 * and these can only initiate connection requests to AllJoyn daemons.  Although
 * dameons may at first blush appear as the service side of a typical BSD
 * sockets client-server pair, it turns out that while daemons obviously must
 * listen for incoming connections, they also must be able to initiate
 * connection requests to other daemons.  It turns out that there is very little
 * in the way of common code when comparing the client version of a TCP
 * transport and a daemon version.  Therefore you will find a TCPTransport
 * class in here the dameon directory and a client version, called simply
 * TCPTransport, in the src directory.
 *
 * This file is the TCPTransport.  It needs to act as both a client and a
 * server explains the presence of both connect-like methods and listen-like
 * methods here.
 *
 * A fundamental idiom in the AllJoyn system is that of a thread.  Active
 * objects in the system that have threads wandering through them will implement
 * Start(), Stop() and Join() methods.  These methods work together to manage
 * the autonomous activities that can happen in a TCPTransport.  These
 * activities are carried out by so-called hardware threads.  POSIX defines
 * functions used to control hardware threads, which it calls pthreads.  Many
 * threading packages use similar constructs.
 *
 * In a threading package, a start method asks the underlying system to arrange
 * for the start of thread execution.  Threads are not necessarily running when
 * the start method returns, but they are being *started*.  Some time later, a
 * thread of execution appears in a thread run function, at which point the
 * thread is considered *running*.  In the case of the TCPTransport, the Start() method
 * spins up a thread to run the BSD sockets' server accept loop.  This
 * also means that as soon as Start() is executed, a thread may be using underlying
 * socket file descriptors and one must be very careful about convincing the
 * accept loop thread to exit before deleting the resources.
 *
 * In generic threads packages, executing a stop method asks the underlying
 * system to arrange for a thread to end its execution.  The system typically
 * sends a message to the thread to ask it to stop doing what it is doing.  The
 * thread is running until it responds to the stop message, at which time the
 * run method exits and the thread is considered *stopping*.  The
 * TCPTransport provides a Stop() method to do exactly that.
 *
 * Note that neither of Start() nor Stop() are synchronous in the sense that one
 * has actually accomplished the desired effect upon the return from a call.  Of
 * particular interest is the fact that after a call to Stop(), threads will
 * still be *running* for some non-deterministic time.
 *
 * In order to wait until all of the threads have actually stopped, a blocking
 * call is required.  In threading packages this is typically called join, and
 * our corresponding method is called Join().  A user of the DaemonTcpTransport
 * must assume that immediately after a call to Start() is begun, and until a
 * call to Join() returns, there may be threads of execution wandering anywhere
 * in the DaemonTcpTransport and in any callback registered by the caller.
 *
 * The high-level process for how an advertisement translates into a transport
 * Connect() is a bit opaque, so we paint a high-level picture here.
 *
 * First, a service (that will be handling RPC calls and emitting signals)
 * acquires a name on the bus, binds a session and calls AdvertiseName.  This
 * filters down (possibly through language bindings) to the AllJoyn object, into
 * the transports on the transport list (the TCP transport is one of those) and
 * eventually to the IpNameService::AdvertiseName() method we call since we are
 * an IP-based transport.  The IP name service will multicast the advertisements
 * to other daemons listening on our device's connected networks.
 *
 * A client that is interested in using the service calls the discovery
 * method FindAdvertisedName.  This filters down (possibly through
 * language bindings) to the AllJoyn object, into the transports on the
 * transport list (us) and we eventually call IpNameService::FindAdvertisedName()
 * since we are an IP-based transport.  The IP name service multicasts the
 * discovery message to other daemons listening on our networks.
 *
 * The daemon remembers which clients have expressed interest in which services,
 * and expects name services to call back with the bus addresses of daemons they
 * find which have the associated services.  In version zero of the protocol,
 * the only endpoint type supported was a TCP endpoint.  In the case of version
 * one, we have four, so we now see "different" bus addresses coming from the
 * name service and "different" connect specs coming from AllJoyn proper.
 *
 * When a new advertisement is received (because we called our listener's
 * Found() method here, the bus address is "hidden" from interested clients and
 * replaced with a more generic TransportMask bit (for us it will be
 * TRANSPORT_TCP).  The client either responds by ignoring the advertisement,
 * waits to accumulate more answers or joins a session to the implied
 * daemon/service.  A reference to a SessionOpts object is provided as a
 * parameter to a JoinSession call if the client wants to connect.  This
 * SessionOpts reference is passed down into the transport (selected by the
 * TransportMask) into the Connect() method which is used to establish the
 * connection.
 *
 * The four different connection mechanisms can be viewed as a matrix;
 *
 *                                                      IPv4               IPv6
 *                                                 ---------------    ---------------
 *     TRAFFIC MESSAGES | TRAFFIC_RAW_RELIABLE  |   Reliable IPv4      Reliable IPv6
 *     TRAFFIC_RAW_UNRELIABLE                   |  Unreliable IPv4    Unreliable IPv6
 *
 * The bits in the provided SessionOpts select the row, but the column is left
 * free (unspecified).  This means that it is up to the transport to figure out
 * which one to use.  Clearly, if only one of the two address flavors is
 * possible (known from examining the returned bus address which is called a
 * connect spec in the Connect() method) the transport should choose that one.
 * If both IPv4 or IPv6 are available, it is up to the transport (again, us) to
 * choose the "best" method since we don't bother clients with that level of
 * detail.  We (TCP) generally choose IPv6 when given the choice since DHCP on
 * IPv4 is sometimes problematic in some networks.
 *
 * Internals
 * =========
 *
 * We spend a lot of time on the threading aspects of the transport since they
 * are often the hardest part to get right and are complicated.  This is where
 * the bugs live.
 *
 * As mentioned above, the AllJoyn system uses the concept of a Transport.  You
 * are looking at the TCPTransport.  Each transport also has the concept
 * of an Endpoint.  The most important function fo an endpoint is to provide
 * non-blocking semantics to higher level code.  This is provided by a transmit
 * thread on the write side which can block without blocking the higher level
 * code, and a receive thread which can similarly block waiting for data without
 * blocking the higher level code.
 *
 * Endpoints are specialized into the LocalEndpoint and the RemoteEndpoint
 * classes.  LocalEndpoint represents a connection from a router to the local
 * bus attachment or daemon (within the "current" process).  A RemoteEndpoint
 * represents a connection from a router to a remote attachment or daemon.  By
 * definition, the TCPTransport provides RemoteEndpoint functionality.
 *
 * RemoteEndpoints are further specialized according to the flavor of the
 * corresponding transport, and so you will see a TCPEndpoint class
 * defined below which provides functionality to send messages from the local
 * router to a destination off of the local process using a TCP transport
 * mechanism.
 *
 * RemoteEndpoints use AllJoyn stream objects to actually move bits.  This
 * is a thin layer on top of a Socket (which is another thin layer on top of
 * a BSD socket) that provides PushBytes() adn PullBytes() methods.  Remote
 * endpoints read and write data by through a callback mechanism implemented
 * using IODispatch.
 *
 * The ReadCallback is made when there is data available to read from the socket.
 * The job of the ReadCallback is to read bytes that appear on the
 * input side of the stream and to unmarshal them into AllJoyn Messages.  Once
 * an endpoint has a message, it calls into the Message router (PushMessage) to
 * arrange for delivery. The WriteCallback is made when the socket is ready to
 * accept data and is enabled when there are messages in the endpoint's transmit
 * queue. The WriteCallback will pull messages off the endpoint's transmit queue,
 * marshal it, and then it will write the bytes to the transport mechanism.
 *
 * The TCPEndpoint inherits the infrastructure requred to do most of its
 * work from the more generic RemoteEndpoint class.  It needs to do specific
 * TCP-related work and also provide for authenticating the endpoint before it
 * is allowed to start pumping messages.  Authentication means running some
 * mysterious (to us) process that may involve some unknown number of challenge
 * and response messsages being exchanged between the client and server side of
 * the connection.  Since we cannot block a caller waiting for authentication,
 * this must done on another thread; and this must be done before the
 * RemoteEndpoint is Start()ed and registered with IODispatch --
 * before its Read and Write callbacks are
 * set up to occur, lest they start pumping messages and interfering with the
 * authentication process.
 *
 * Authentication can, of course, succeed or fail based on timely interaction
 * between the two sides, but it can also be abused in a denial of service
 * attack.  If a client simply starts the process but never responds, it could
 * tie up a daemon's resources, and coordinated action could bring down a
 * daemon.  Because of this, we need to provide a way to reach in and abort
 * authentications that are "taking too long."
 *
 * As described above, a daemon can listen for inbound connections and it can
 * initiate connections to remote daemons.  Authentication must happen in both
 * cases.
 *
 * If you consider all that is happening, we are talking about a complicated
 * system of many threads that are appearing and disappearing in the system at
 * unpredictable times.  These threads have dependencies in the resources
 * associated with them (sockets and events in particular).  These resources may
 * have further dependencies that must be respected.  For example, Events may
 * have references to Sockets.  The Sockets must not be released before the
 * Events are released, because the events would be left with stale handles.  An
 * even scarier case is if an underlying Socket FD is reused at just the wrong
 * time, it would be possible to switch a Socket FD from one connection to
 * another out from under an Event without its knowledge.
 *
 * To summarize, consider the following "big picture' view of the transport.  A
 * single TCPTransport is constructed if the daemon TransportList indicates that
 * TCP support is required.  The high-level daemon code (see bbdaemon.cc for
 * example) builds a TransportFactoryContainer that is initialized with a
 * factory that knows how to make TCPTransport objects if they are needed, and
 * associates the factory with the string "tcp".  The daemon also constructs
 * "server args" which may contain the string "tcp" or "unix".  If the factory
 * container provides a "tcp" factory and the server args specify a "tcp"
 * transport is needed then a TCPTransport object is instantiated and entered
 * into the daemon's internal transport list (list of available transports).
 * Also provided for each transport is an abstract address to listen for
 * incoming connection requests on.
 *
 * When the daemon is brought up, its TransportList is Start()ed.  The transport
 * specs string (e.g., "unix:abstract=alljoyn;tcp:") is provided to
 * TransportList::Start() as a parameter.  The transport specs string is parsed
 * and in the example above, results in "unix" transports and "tcp" transports
 * being instantiated and started.  As mentioned previously "tcp" in the daemon
 * translates into TCPTransport.  Once the desired transports are instantiated,
 * each is Start()ed in turn.  In the case of the TCPTransport, this will start
 * the server accept loop.  Initially there are no sockets to listen on.
 *
 * The daemon then needs to start listening on some inbound addresses and ports.
 * This is done by the StartListen() command which you can find in bbdaemon, for
 * example.  This alwo takes the same king of server args string shown above but
 * this time the address and port information are used.  For example, one might
 * use the string "tcp:addr=0.0.0.0,port=9955;" to specify which address and
 * port to listen to.  This Bus::StartListen() call is translated into a
 * TCPTransport::StartListen() call which is provided with the string
 * which we call a "listen spec".  Our StartListen() will create a Socket, bind
 * the socket to the address and port provided and save the new socket on a list
 * of "listenFds." It will then Alert() the already running server accept loop
 * thread -- see TCPTransport::Run().  Each time through the server accept
 * loop, Run() will examine the list of listenFds and will associate an Event
 * with the corresponding socketFd and wait for connection requests.
 *
 * There is a complementary call to stop listening on addresses.  Since the
 * server accept loop is depending on the associated sockets, StopListen must
 * not close those Sockets, it must ask the server accept loop to do so in a
 * coordinated way.
 *
 * When an inbound connection request is received, the accept loop will wake up
 * and create a TCPEndpoint for the *proposed* new connection.  Recall
 * that an endpoint is not brought up immediately, but an authentication step
 * must be performed.  The server accept loop starts this process by placing the
 * new TCPEndpoint on an authList, or list of authenticating endpoints.
 * It then calls the endpoint Authenticate() method which spins up an
 * authentication thread and returns immediately.  This process transfers the
 * responsibility for the connection and its resources to the authentication
 * thread.  Authentication can succeed, fail, or take to long and be aborted.
 *
 * If authentication succeeds, the authentication thread calls back into the
 * TCPTransport's Authenticated() method.  Along with indicating that
 * authentication has completed successfully, this transfers ownership of the
 * TCPEndpoint back to the TCPTransport from the authentication
 * thread.  At this time, the TCPEndpoint is Start()ed which spins up
 * the transmit and receive threads and enables Message routing across the
 * transport.
 *
 * If the authentication fails, the authentication thread simply sets a the
 * TCPEndpoint state to FAILED and exits.  The server accept loop looks at
 * authenticating endpoints (those on the authList)each time through its loop.
 * If an endpoint has failed authentication, and its thread has actually gone
 * away (or more precisely is at least going away in such a way that it will
 * never touch the endpoint data structure again).  This means that the endpoint
 * can be deleted.
 *
 * If the authentication takes "too long" we assume that a denial of service
 * attack in in progress.  We call AuthStop() on such an endpoint which will most
 * likely induce a failure (unless we happen to call abort just as the endpoint
 * actually finishes the authentication which is highly unlikely but okay).
 * This AuthStop() will cause the endpoint to be scavenged using the above mechanism
 * the next time through the accept loop.
 *
 * A daemon transport can accept incoming connections, and it can make outgoing
 * connections to another daemon.  This case is simpler than the accept case
 * since it is expected that a socket connect can block, so it is possible to do
 * authentication in the context of the thread calling Connect().  Connect() is
 * provided a so-called "connect spec" which provides an IP address ("addr=xxxx"),
 * port ("port=yyyy") in a String.
 *
 * A check is always made to catch an attempt for the daemon to connect to
 * itself which is a system-defined error (it causes the daemon grief, so we
 * avoid it here by looking to see if one of the listenFds is listening on an
 * interface that corresponds to the address in the connect spec).
 *
 * If the connect is allowed, we do the usual BSD sockets thing where we create
 * a socket and connect to the specified remote address.  The DBus spec says that
 * all connections must begin with one uninterpreted byte so we send that.  This
 * byte is only meaningful in Unix domain sockets transports, but we must send it
 * anyway.
 *
 * The next step is to create a TCPEndpoint and to put it on the endpointList.
 * Note that the endpoint doesn't go on the authList as in the server case, it
 * goes on the list of active endpoints.  This is because a failure to authenticate
 * on the client side results in a call to EndpointExit which is the same code path as
 * a failure when the endpoint is up.  The failing endpoint must be on the endpoint
 * list in order to allow authentication errors to be propagated back to higher-level
 * code in a meaningful context.  Once the endpoint is stored on the list, Connect()
 * starts client-side Authentication with the remote (server) side.  If Authentication
 * succeeds, the endpoint is Start()ed which will register the endpoint with IODispatch
 * that enables callbacks to be made to start Message routing across the link.
 * The endpoint is left on the endpoint list in this case.
 * If authentication fails, the endpoint is removed from the active
 * list.  This is thread-safe since there is no authentication thread running because
 * the authentication was done in the context of the thread calling Connect() which
 * is the one deleting the endpoint; and the endpoint is not registered with IODispatch
 * if the authentication fails.
 *
 * Shutting the TCPTransport down involves orchestrating the orderly termination
 * of:
 *
 *   1) Threads that may be running in the server accept loop with associated Events
 *      and their dependent socketFds stored in the listenFds list.
 *   2) Threads that may be running authentication with associated endpoint objects,
 *      streams and SocketFds.  These threads are accessible through endpoint objects
 *      stored on the authList.
 *   3) Unregistering the endpoint from IODispatch that stops any future read/write callbacks
 *      from occuring and schedules a ExitCallback that can be used for clean up.
 *
 * Note that we also have to understand and deal with the fact that threads
 * running in state (2) above, will exit and depend on the server accept loop to
 * scavenge the associated objects off of the authList and delete them.  This
 * means that the server accept loop cannot be Stop()ped until the authList is
 * empty.  We further have to understand that read/write callbacks running in state (3) above
 * will depend on the hooked EndpointExit function to dispose of associated
 * resources.  This will happen in the context of either the IODispatch callbacks (the last to go).
 * We can't delete the transport until all of its
 * associated endpoint threads/callbacks are Join()ed.  Also, since the server accept loop
 * is looking at the list of listenFDs, we must be careful about deleting those
 * sockets out from under the server thread.  The system should call
 * StopListen() on all of the listen specs it called StartListen() on; but we
 * need to be prepared to clean up any "unstopped" listen specs in a coordinated
 * way.  This, in turn, means that the server accept loop cannot be Stop()ped
 * until all of the listenFds are cleaned up.
 *
 * There are a lot of dependencies here, so be careful when making changes to
 * the thread and resource management here.  It's quite easy to shoot yourself
 * in multiple feet you never knew you had if you make an unwise modification,
 * and sometimes the results are tiny little time-bombs set to go off in
 * completely unrelated code (if, for example, a socket is deleted and reused
 * by another piece of code while the transport still has an event referencing
 * the socket now used by the other module).
 */

#define QCC_MODULE "TCP"

using namespace std;
using namespace qcc;

const uint32_t TCP_LINK_TIMEOUT_PROBE_ATTEMPTS       = 1;
const uint32_t TCP_LINK_TIMEOUT_PROBE_RESPONSE_DELAY = 10;
const uint32_t TCP_LINK_TIMEOUT_MIN_LINK_TIMEOUT     = 40;

namespace ajn {

/**
 * Name of transport used in transport specs.
 */
const char* TCPTransport::TransportName = "tcp";

/**
 * Default router advertisement prefix.
 */
const char* const TCPTransport::ALLJOYN_DEFAULT_ROUTER_ADVERTISEMENT_PREFIX = "org.alljoyn.BusNode.";

/*
 * An endpoint class to handle the details of authenticating a connection in a
 * way that avoids denial of service attacks.
 */
class _TCPEndpoint : public _RemoteEndpoint {
  public:
    friend class TCPTransport;
    /**
     * There are three threads that can be running around in this data
     * structure.  An auth thread is run before the endpoint is started in order
     * to handle the security stuff that must be taken care of before messages
     * can start passing.  This enum reflects the states of the authentication
     * process and the state can be found in m_authState.  Once authentication
     * is complete, the auth thread must go away, but it must also be joined,
     * which is indicated by the AUTH_DONE state.  The state of Read and Write
     * callbacks is dealt with by the EndpointState.
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
     * There is one dedicated thread and two types callbacks that can be running
     * around in this data structure.
     * Read and Write callbacks are used to pump
     * messages through an endpoint.  These callbacks cannot be run until the
     * authentication process has completed.  This enum reflects the states of
     * the endpoint Read and WriteCallbacks and can be found in m_epState.  The auth
     * thread is dealt with by the AuthState enum above.  These callbacks must cease
     * to occur once the endpoint has completely exited, which is indicated by the EP_DONE state.
     */
    enum EndpointState {
        EP_ILLEGAL = 0,
        EP_INITIALIZED,      /**< This endpoint structure has been allocated but not used */
        EP_FAILED,           /**< Starting the Read and WriteCallbacks has failed and this endpoint is not usable */
        EP_STARTING,          /**< The Read and WriteCallbacks are being started */
        EP_STARTED,          /**< The Read and WriteCallbacks have been started (they work as a unit) */
        EP_STOPPING,         /**< The Read and WriteCallbacks are stopping (have run ThreadExit) but have not been joined */
        EP_DONE              /**< The Read and WriteCallbacks have been shut down and joined */
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

    _TCPEndpoint(TCPTransport* transport,
                 BusAttachment& bus,
                 bool incoming,
                 const qcc::String connectSpec,
                 qcc::SocketFd sock,
                 const qcc::IPAddress& ipAddr,
                 uint16_t port) :
        _RemoteEndpoint(bus, incoming, connectSpec, &m_stream, "tcp"),
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

    _TCPEndpoint(TCPTransport* transport,
                 BusAttachment& bus,
                 bool incoming,
                 const qcc::String connectSpec,  const AddressFamily family, const SocketType type,
                 const qcc::IPAddress& ipAddr,
                 uint16_t port) :
        _RemoteEndpoint(bus, incoming, connectSpec, &m_stream, "tcp"),
        m_transport(transport),
        m_sideState(SIDE_INITIALIZED),
        m_authState(AUTH_INITIALIZED),
        m_epState(EP_INITIALIZED),
        m_tStart(qcc::Timespec(0)),
        m_authThread(this),
        m_stream(family, type),
        m_ipAddr(ipAddr),
        m_port(port),
        m_wasSuddenDisconnect(!incoming) { }
    virtual ~_TCPEndpoint() { }

    QStatus GetLocalIp(qcc::String& ipAddrStr) {
        SocketFd sockFd = m_stream.GetSocketFd();
        IPAddress ipaddr;
        uint16_t port;
        QStatus status = GetLocalAddress(sockFd, ipaddr, port);
        if (status == ER_OK) {
            ipAddrStr = ipaddr.ToString();
        }
        return status;
    };

    QStatus GetRemoteIp(qcc::String& ipAddrStr) {
        ipAddrStr = m_ipAddr.ToString();
        return ER_OK;
    };

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
        Timespec tNow;
        GetTimeNow(&tNow);
        SetStartTime(tNow);
        m_authState = AUTH_DONE;
    }

    EndpointState GetEpState(void) { return m_epState; }

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
        assert(m_epState == EP_STARTING || m_epState == EP_STARTED || m_epState == EP_STOPPING || m_epState == EP_FAILED);
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
            uint32_t to = max(linkTimeout, TCP_LINK_TIMEOUT_MIN_LINK_TIMEOUT);
            to -= TCP_LINK_TIMEOUT_PROBE_RESPONSE_DELAY * TCP_LINK_TIMEOUT_PROBE_ATTEMPTS;
            status = _RemoteEndpoint::SetLinkTimeout(to, TCP_LINK_TIMEOUT_PROBE_RESPONSE_DELAY, TCP_LINK_TIMEOUT_PROBE_ATTEMPTS);
            if ((status == ER_OK) && (to > 0)) {
                linkTimeout = to + TCP_LINK_TIMEOUT_PROBE_RESPONSE_DELAY * TCP_LINK_TIMEOUT_PROBE_ATTEMPTS;
            }

        } else {
            _RemoteEndpoint::SetLinkTimeout(0, 0, 0);
        }
        return status;
    }

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

    /*
     * Return true if the auth thread is STARTED, RUNNING or STOPPING.  A true
     * response means the authentication thread is in a state that indicates
     * a possibility it might touch the endpoint data structure.  This means
     * don't delete the endpoint if this method returns true.  This method
     * indicates nothing about endpoint Read and WriteCallback state.
     */
    bool IsAuthThreadRunning(void)
    {
        return m_authThread.IsRunning();
    }
    virtual void ThreadExit(qcc::Thread* thread);

  private:
    class AuthThread : public qcc::Thread {
      public:
        AuthThread(_TCPEndpoint* ep) : Thread("auth"), m_endpoint(ep)  { }
      private:
        virtual qcc::ThreadReturn STDCALL Run(void* arg);

        _TCPEndpoint* m_endpoint;
    };

    TCPTransport* m_transport;        /**< The server holding the connection */
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

void _TCPEndpoint::ThreadExit(qcc::Thread* thread)
{
    /* If the auth thread exits before it even enters the AuthThread::Run() function, set the state to AUTH_FAILED. */
    if (thread == &m_authThread) {
        if (m_authState == AUTH_INITIALIZED) {
            m_authState = AUTH_FAILED;
        }
        m_transport->Alert();
    }
    _RemoteEndpoint::ThreadExit(thread);
}

QStatus _TCPEndpoint::Authenticate(void)
{
    QCC_DbgTrace(("TCPEndpoint::Authenticate()"));
    /*
     * Start the authentication thread.
     */
    QStatus status = m_authThread.Start(this, this);
    if (status != ER_OK) {
        m_authState = AUTH_FAILED;
    }
    return status;
}

void _TCPEndpoint::AuthStop(void)
{
    QCC_DbgTrace(("TCPEndpoint::AuthStop()"));

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

void _TCPEndpoint::AuthJoin(void)
{
    QCC_DbgTrace(("TCPEndpoint::AuthJoin()"));

    /*
     * Join the auth thread to stop executing.  All threads must be joined in
     * order to communicate their return status.  The auth thread is no exception.
     * This is done in a lazy fashion from the main server accept loop, where we
     * cleanup every time through the loop.
     */
    m_authThread.Join();
}

void* _TCPEndpoint::AuthThread::Run(void* arg)
{
    QCC_DbgTrace(("TCPEndpoint::AuthThread::Run()"));

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
     * will quickly go away and will be replaced by the Read and WriteCallbacks of
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

    /* Initialize the features for this endpoint */
    m_endpoint->GetFeatures().isBusToBus = false;
    m_endpoint->GetFeatures().isBusToBus = false;
    m_endpoint->GetFeatures().handlePassing = false;

    /*
     * Check any application connecting over TCP to see if it is running on the same machine and
     * set the group ID appropriately if so.
     */
    TCPEndpoint tcpEp = TCPEndpoint::wrap(m_endpoint);
    TCPTransport::CheckEndpointLocalMachine(tcpEp);

    /* Run the actual connection authentication code. */
    qcc::String authName;
    qcc::String redirection;
    DaemonRouter& router = reinterpret_cast<DaemonRouter&>(m_endpoint->m_transport->m_bus.GetInternal().GetRouter());
    AuthListener* authListener = router.GetBusController()->GetAuthListener();
    /* Since the TCPTransport allows untrusted clients, it must implement UntrustedClientStart and
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
        QCC_LogError(status, ("Failed to establish TCP endpoint"));

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
    m_endpoint->m_transport->Authenticated(tcpEp);

    QCC_DbgTrace(("TCPEndpoint::AuthThread::Run(): Returning"));

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

TCPTransport::TCPTransport(BusAttachment& bus)
    : Thread("TCPTransport"), m_bus(bus), m_stopping(false), m_listener(0),
    m_foundCallback(m_listener), m_networkEventCallback(*this),
    m_isAdvertising(false), m_isDiscovering(false), m_isListening(false),
    m_isNsEnabled(false), m_reload(STATE_RELOADING),
    m_nsReleaseCount(0), m_wildcardIfaceProcessed(false),
    m_maxUntrustedClients(0), m_numUntrustedClients(0)
{
    QCC_DbgTrace(("TCPTransport::TCPTransport()"));
    /*
     * We know we are daemon code, so we'd better be running with a daemon
     * router.  This is assumed elsewhere.
     */
    assert(m_bus.GetInternal().GetRouter().IsDaemon());
}

TCPTransport::~TCPTransport()
{
    QCC_DbgTrace(("TCPTransport::~TCPTransport()"));
    Stop();
    Join();
}

void TCPTransport::Authenticated(TCPEndpoint& conn)
{
    QCC_DbgTrace(("TCPTransport::Authenticated()"));
    /*
     * If the transport is stopping, dont set up the Read and WriteCallback
     * mechanism for endpoints.
     */
    if (m_stopping == true) {
        return;
    }
    /*
     * If Authenticated() is being called, it is as a result of the
     * authentication thread telling us that it has succeeded.  What we need to
     * do here is to try and Start() the endpoint which will set up
     * Read and WriteCallbacks and register the endpoint with the daemon router.
     * As soon as we call Start(), we are transferring responsibility for error reporting
     * through endpoint ThreadExit() function.  This will percolate out our
     * EndpointExit function.  It will expect to find <conn> on the endpoint
     * list so we move it from the authList to the endpointList before calling
     * Start.
     */
    m_endpointListLock.Lock(MUTEX_CONTEXT);

    set<TCPEndpoint>::iterator i = find(m_authList.begin(), m_authList.end(), conn);
    assert(i != m_authList.end() && "TCPTransport::Authenticated(): Conn not on m_authList");

    /*
     * Note here that we have not yet marked the authState as AUTH_SUCCEEDED so
     * this is a point in time where the authState can be AUTH_AUTHENTICATING
     * and the endpoint can be on the endpointList and not the authList.
     */
    m_authList.erase(i);
    m_endpointList.insert(conn);

    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    conn->SetListener(this);

    conn->SetEpStarting();

    QStatus status = conn->Start(m_defaultHbeatIdleTimeout, m_defaultHbeatProbeTimeout, m_numHbeatProbes, m_maxHbeatProbeTimeout);
    if (status != ER_OK) {
        QCC_LogError(status, ("TCPTransport::Authenticated(): Failed to start TCP endpoint"));
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
         * that there are Read and WriteCallbacks wandering around in this endpoint.
         */
        conn->SetEpStarted();
    }
}

QStatus TCPTransport::Start()
{
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
        QCC_LogError(ER_BUS_BUS_ALREADY_STARTED, ("TCPTransport::Start(): Already started"));
        return ER_BUS_BUS_ALREADY_STARTED;
    }

    m_stopping = false;

    /*
     * Get the guid from the bus attachment which will act as the globally unique
     * ID of the daemon.
     */
    qcc::String guidStr = m_bus.GetInternal().GetGlobalGUID().ToString();

    /*
     * We're a TCP transport, and TCP is an IP protocol, so we want to use the IP
     * name service for our advertisement and discovery work.  When we acquire
     * the name service, we are basically bumping a reference count and starting
     * it if required.
     *
     * Start() will legally be called exactly once, but Stop() and Join() may be called
     * multiple times.  Since we are essentially reference counting the name service
     * singleton, we can only call Release() on it once.  So we have a release count
     * variable that allows us to only release the singleton on the first transport
     * Join()
     */
    m_nsReleaseCount = 0;
    IpNameService::Instance().Acquire(guidStr);

    /*
     * Tell the name service to call us back on our FoundCallback method when
     * we hear about a new well-known bus name.
     */
    IpNameService::Instance().SetCallback(TRANSPORT_TCP,
                                          new CallbackImpl<FoundCallback, void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint32_t>
                                              (&m_foundCallback, &FoundCallback::Found));

    /*
     * Tell the name service to call us back on our NetworkEventCallback when it
     * detects that an interface has become IFF_UP or its IP address has changed.
     */
    IpNameService::Instance().SetNetworkEventCallback(TRANSPORT_TCP,
                                                      new CallbackImpl<NetworkEventCallback, void, const std::map<qcc::String, qcc::IPAddress>&>
                                                          (&m_networkEventCallback, &NetworkEventCallback::Handler));

    /*
     * Start the server accept loop through the thread base class.  This will
     * close or open the IsRunning() gate we use to control access to our
     * public API.
     */
    return Thread::Start();
}

QStatus TCPTransport::Stop(void)
{
    QCC_DbgTrace(("TCPTransport::Stop()"));

    /*
     * It is legal to call Stop() more than once, so it must be possible to
     * call Stop() on a stopped transport.
     */
    m_stopping = true;

    /*
     * Tell the name service to disregard all our prior advertisements and
     * discoveries. The internal state will shortly be discarded as well.
     */
    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    QCC_DbgTrace(("TCPTransport::Stop(): Gratuitously clean out advertisements."));
    for (list<qcc::String>::iterator i = m_advertising.begin(); i != m_advertising.end(); ++i) {
        IpNameService::Instance().CancelAdvertiseName(TRANSPORT_TCP, *i, TRANSPORT_TCP);
    }
    m_advertising.clear();
    m_isAdvertising = false;
    QCC_DbgTrace(("TCPTransport::Stop(): Gratuitously clean out discoveries."));
    for (list<qcc::String>::iterator i = m_discovering.begin(); i != m_discovering.end(); ++i) {
        IpNameService::Instance().CancelFindAdvertisement(TRANSPORT_TCP, *i, TRANSPORT_TCP);
    }
    m_discovering.clear();
    m_isDiscovering = false;
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

    /*
     * Tell the name service to stop calling us back if it's there (we may get
     * called more than once in the chain of destruction) so the pointer is not
     * required to be non-NULL.
     */
    IpNameService::Instance().SetCallback(TRANSPORT_TCP, NULL);

    IpNameService::Instance().SetNetworkEventCallback(TRANSPORT_TCP, NULL);

    /*
     * Tell the server accept loop thread to shut down through the thead
     * base class.
     */
    QStatus status = Thread::Stop();
    if (status != ER_OK) {
        QCC_LogError(status, ("TCPTransport::Stop(): Failed to Stop() server thread"));
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
     * endpoint Read and WriteCallbacks will not be running yet.
     */
    for (set<TCPEndpoint>::iterator i = m_authList.begin(); i != m_authList.end(); ++i) {
        TCPEndpoint ep = *i;
        ep->AuthStop();
    }

    /*
     * Ask any running endpoints to shut down and exit their threads.  By its
     * presence on the m_endpointList, we know that authentication is compete and
     * the Read and WriteCallbacks have responsibility for dealing with the endpoint
     * data structure.  We call Stop() to stop those threads from running.  Since
     * the connnection is on the m_endpointList, we know that the authentication
     * thread has handed off responsibility.
     */
    for (set<TCPEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        TCPEndpoint ep = *i;
        ep->Stop();
    }

    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    return ER_OK;
}

QStatus TCPTransport::Join(void)
{
    QCC_DbgTrace(("TCPTransport::Join()"));

    /*
     * It is legal to call Join() more than once, so it must be possible to
     * call Join() on a joined transport and also on a joined name service.
     */
    QStatus status = Thread::Join();
    if (status != ER_OK) {
        return status;
    }

    /*
     * Tell the IP name service instance that we will no longer be making calls
     * and it may shut down if we were the last transport.  This release can
     * be thought of as a reference counted Stop()/Join() so it is appropriate
     * to make it here since we are expecting the possibility of blocking.
     *
     * Since it is reference counted, we can't just call it willy-nilly.  We
     * have to be careful since our Join() can be called multiple times.
     */
    int count = qcc::IncrementAndFetch(&m_nsReleaseCount);
    if (count == 1) {
        IpNameService::Instance().Release();
    }

    /*
     * A required call to Stop() that needs to happen before this Join will ask
     * all of the endpoints to stop; and will also cause any authenticating
     * endpoints to stop.  We still need to wait here until all of the threads
     * running in those endpoints actually stop running.
     *
     * Since Stop() is a request to stop, and this is what has ultimately been
     * done to both authentication threads and Read and WriteCallbacks, it is possible
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
    set<TCPEndpoint>::iterator it = m_authList.begin();
    while (it != m_authList.end()) {
        TCPEndpoint ep = *it;
        m_authList.erase(it);
        m_endpointListLock.Unlock(MUTEX_CONTEXT);
        ep->AuthJoin();
        m_endpointListLock.Lock(MUTEX_CONTEXT);
        it = m_authList.upper_bound(ep);
    }


    /*
     * Any running endpoints have been asked it their threads in a previously
     * required Stop().  We need to Join() all of thesse threads here.  This
     * Join() will wait on the endpoint Read and WriteCallbacks to exit as opposed to
     * the joining of the auth thread we did above.
     */
    it = m_endpointList.begin();
    while (it != m_endpointList.end()) {
        TCPEndpoint ep = *it;
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

/**
 * This is a convenience function that tells a caller whether or not this
 * transport will support a set of options for a connection.  Lets the caller
 * decide up front whether or not a connection will succeed due to options
 * conflicts.
 */
bool TCPTransport::SupportsOptions(const SessionOpts& opts) const
{
    QCC_DbgTrace(("TCPTransport::SupportsOptions()"));
    bool rc = true;

    /*
     * TCP only suports reliable messaging, which means TRAFFIC_RAW_RELIABLE
     * (raw sockets over a reliable underlying transport) or TRAFFIC_MESSAGES
     * (which is AllJoyn Messages over a reliable underlying transport).  It's
     * not an error if we don't match, we just don't have anything to offer.
     */
    if (opts.traffic != SessionOpts::TRAFFIC_MESSAGES && opts.traffic != SessionOpts::TRAFFIC_RAW_RELIABLE) {
        QCC_DbgPrintf(("TCPTransport::SupportsOptions(): traffic type mismatch"));
        rc = false;
    }

    /*
     * The other session option that we need to filter on is the transport
     * bitfield.  This transport supports TRANSPORT_TCP of course, but we allow
     * TRANSPORT_WLAN, TRANSPORT_WWAN, and TRANSPORT_LAN to be synonymous with
     * TRANSPORT_TCP.  If you are explicitly looking for something other than
     * TCP (or one of the aliases) we can't help you.
     */
    if (!(opts.transports & (TRANSPORT_TCP | TRANSPORT_WLAN | TRANSPORT_WWAN | TRANSPORT_LAN))) {
        QCC_DbgPrintf(("TCPTransport::SupportsOptions(): transport mismatch"));
        rc = false;
    }

    QCC_DbgPrintf(("TCPTransport::SupportsOptions(): returns \"%s\"", rc == true ? "true" : "false"));
    return rc;
}

/*
 * The default interface for the name service to use.  The wildcard character
 * means to listen and transmit over all interfaces that are up and multicast
 * capable, with any IP address they happen to have.  This default also applies
 * to the search for listen address interfaces.
 */
static const char* INTERFACES_DEFAULT = "*";

QStatus TCPTransport::GetListenAddresses(const SessionOpts& opts, std::vector<qcc::String>& busAddrs) const
{
    QCC_DbgTrace(("TCPTransport::GetListenAddresses()"));

    /*
     * We are given a session options structure that defines the kind of
     * transports that are being sought.  It's not an error if we don't match
     * requested options, we just don't have anything to offer.
     */
    if (SupportsOptions(opts) == false) {
        QCC_DbgPrintf(("TCPTransport::GetListenAddresses(): Supported options mismatch"));
        return ER_OK;
    }

    /*
     * The name service is initialized by the call to Init() in our Start()
     * method and then started there.  It is Stop()ped in our Stop() method and
     * joined in our Join().  In the case of a call here, the transport will
     * probably be started, and we will probably find the name service started,
     * but there is no requirement to ensure this.  If m_ns is NULL, we need to
     * complain so the user learns to Start() the transport before calling
     * IfConfig.  A call to IsRunning() here is superfluous since we really
     * don't care about anything but the name service in this method.
     */
    if (IpNameService::Instance().Started() == false) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("TCPTransport::GetListenAddresses(): NameService not started"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    /*
     * Our goal is here is to match a list of interfaces provided in the
     * configuration database (or a wildcard) to a list of interfaces that are
     * IFF_UP in the system.  The first order of business is to get the list of
     * interfaces in the system.  We do that using a convenient OS-inependent
     * call into the name service.
     *
     * We can't cache this list since it may change as the phone wanders in
     * and out of range of this and that and the underlying IP addresses change
     * as DHCP doles out whatever it feels like at any moment.
     */
    QCC_DbgPrintf(("TCPTransport::GetListenAddresses(): IfConfig()"));

    std::vector<qcc::IfConfigEntry> entries;
    QStatus status = qcc::IfConfig(entries);
    if (status != ER_OK) {
        QCC_LogError(status, ("TCPTransport::GetListenAddresses(): ns.IfConfig() failed"));
        return status;
    }

    /*
     * The next thing to do is to get the list of requested interfaces that
     * have been processed. A '*' or '0.0.0.0'  being a wildcard indicating
     * that we want to match any interface.  If there is no configuration
     * item, we default to something rational.
     */
    std::set<qcc::String> interfaceSet;
    bool haveWildcard = false;
    /*
     * Check for wildcard anywhere in the configuration string.  This trumps
     * anything else that may be there and ensures we get only one copy of
     * the addresses if someone tries to trick us with duplicate "*".
     * If we have already processed a specification in the configuration
     * database for the wildcard network interface "*" or the wildcard
     * network address we only add the default interface to the set of
     * processed interfaces.  If we haven't processed a wildcard
     * specification, we look for any interfaces that we have processed,
     * these interface names will map to a non-default IP address in the
     * m_requestedInterfaces map.  We also look for any IP addresses that
     * we have processed, these will map to a non-empty network interface
     * name in the m_requestedAddresses map. We add the interface names in
     * both cases to the processed interfaceSet.
     */
    if (m_wildcardIfaceProcessed || m_wildcardAddressProcessed) {
        interfaceSet.insert(INTERFACES_DEFAULT);
        haveWildcard = true;
    } else {
        for (std::map<qcc::String, qcc::IPEndpoint>::const_iterator it = m_requestedInterfaces.begin();
             it != m_requestedInterfaces.end(); it++) {
            if (it->first != "*" && it->second.GetAddress().ToString() != "0.0.0.0") {
                interfaceSet.insert(it->first);
            }
        }
        for (std::map<qcc::String, qcc::String>::const_iterator it = m_requestedAddresses.begin();
             it != m_requestedAddresses.end(); it++) {
            if (it->first != "0.0.0.0" && !it->second.empty()) {
                interfaceSet.insert(it->second);
            }
        }
    }
    if (interfaceSet.size() == 0) {
        interfaceSet.insert(INTERFACES_DEFAULT);
        haveWildcard = true;
    }

    /*
     * Walk the requested set of processed interfaces and try to match it
     * up with interfaces actually found in the system via IfConfig().
     */
    for (std::set<qcc::String>::const_iterator it = interfaceSet.begin(); it != interfaceSet.end(); it++) {
        /*
         * We got a set of interfaces, so we need to work our way through
         * the set.  Each entry in the list  may be  an interface name, or a
         * wildcard.
         */
        qcc::String currentInterface = *it;

        QCC_DbgPrintf(("TCPTransport::GetListenAddresses(): looking for interface %s", currentInterface.c_str()));

        /*
         * Walk the list of interfaces that we got from the system and see if
         * we find a match.
         */
        for (uint32_t i = 0; i < entries.size(); ++i) {
            QCC_DbgPrintf(("TCPTransport::GetListenAddresses(): matching %s", entries[i].m_name.c_str()));
            /*
             * To match a configuration entry, the name of the interface must:
             *
             *   - match the name in the currentInterface (or be wildcarded);
             *   - be UP which means it has an IP address assigned;
             *   - not be the LOOPBACK device and therefore be remotely available.
             */
            uint32_t mask = qcc::IfConfigEntry::UP | qcc::IfConfigEntry::LOOPBACK;

            uint32_t state = qcc::IfConfigEntry::UP;

            if ((entries[i].m_flags & mask) == state) {
                QCC_DbgPrintf(("TCPTransport::GetListenAddresses(): %s has correct state", entries[i].m_name.c_str()));
                if (haveWildcard || entries[i].m_name == currentInterface) {
                    QCC_DbgPrintf(("TCPTransport::GetListenAddresses(): %s has correct name", entries[i].m_name.c_str()));
                    /*
                     * This entry matches our search criteria, so we need to
                     * turn the IP address that we found into a busAddr.  We
                     * must be a TCP transport, and we have an IP address
                     * already in a string, so we can easily put together the
                     * desired busAddr.
                     */
                    QCC_DbgTrace(("TCPTransport::GetListenAddresses(): %s match found", entries[i].m_name.c_str()));
                    /*
                     * We know we have an interface that speaks IP and
                     * which has an IP address we can pass back. We know
                     * it is capable of receiving incoming connections, but
                     * the $64,000 questions are, does it have a listener
                     * and what port is that listener listening on.
                     *
                     * There is one name service associated with the daemon
                     * TCP transport.  It may be advertising a different port on each
                     * network interface.  If multiple listeners are created for
                     * a network interface, the name service only advertises the
                     * lastly set port for that network interface. Ask the name
                     * service for a map that correlates the different interfaces
                     * it is advertising on to the respective ports we are listening on.
                     */
                    qcc::String ipv4address;
                    qcc::String ipv6address;
                    std::map<qcc::String, uint16_t> reliableIpv4PortMap, unreliableIpv4PortMap;
                    uint16_t reliableIpv6Port, unreliableIpv6port;
                    IpNameService::Instance().Enabled(TRANSPORT_TCP,
                                                      reliableIpv4PortMap, reliableIpv6Port,
                                                      unreliableIpv4PortMap, unreliableIpv6port);
                    /*
                     * If no listening port corresponding to this network interface is found in the map,
                     * then it hasn't been set and this implies that there is no listener for this transport
                     * on this network interface. We should only return a bus address corresponding to this
                     * network interface if we have a listener on this network interface.
                     *
                     * Note that if we find a "*" in the reliableIPv4PortMap it is a wildcard and therefore
                     * matches the entry we are comparing to, in which case we are not comparing the entry to
                     * what's in the port map, we are using what's in the port map to confirm the entry.
                     */
                    bool portMapWildcard = reliableIpv4PortMap.find(qcc::String("*")) != reliableIpv4PortMap.end();
                    bool portMapExplicit = reliableIpv4PortMap.find(entries[i].m_name) != reliableIpv4PortMap.end();

                    if (portMapWildcard || portMapExplicit) {
                        uint16_t port = 0;
                        if (portMapWildcard) {
                            port = reliableIpv4PortMap[qcc::String("*")];
                        } else {
                            port = reliableIpv4PortMap[entries[i].m_name];
                        }

                        /*
                         * Now put this information together into a bus address
                         * that the rest of the AllJoyn world can understand.
                         * (Note: only IPv4 "reliable" addresses are supported
                         * at this time.)
                         */
                        if (!entries[i].m_addr.empty() && (entries[i].m_family == QCC_AF_INET)) {
                            qcc::String busAddr = "tcp:addr=" + entries[i].m_addr + ","
                                                  "port=" + U32ToString(port) + ","
                                                  "family=ipv4";
                            busAddrs.push_back(busAddr);
                        }
                    }
                }
            }
        }
    }

    /*
     * If we can get the list and walk it, we have succeeded.  It is not an
     * error to have no available interfaces.  In fact, it is quite expected
     * in a phone if it is not associated with an access point over wi-fi.
     */
    QCC_DbgPrintf(("TCPTransport::GetListenAddresses(): done"));
    return ER_OK;
}

void TCPTransport::EndpointExit(RemoteEndpoint& ep)
{
    /*
     * This is a callback driven from the remote endpoint thread exit function.
     * Our TCPEndpoint inherits from class RemoteEndpoint and so when
     * either of the threads (transmit or receive) of one of our endpoints exits
     * for some reason, we get called back here.  We only get called if either
     * the Read and WriteCallbacks stop taking place, which implies that they
     * have been run.  It turns out that in the case of an endpoint receiving
     * a connection, it means that authentication has succeeded.
     * In the case of an endpoint doing the connect, the EndpointExit may have
     * resulted from an authentication error since authentication is done in the
     * context of the Connect()ing thread and may be reported through EndpointExit.
     */
    QCC_DbgTrace(("TCPTransport::EndpointExit()"));
    TCPEndpoint tep = TCPEndpoint::cast(ep);
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
     * the Read and WriteCallbacks.
     */
    tep->SetEpStopping();

    /*
     * Wake up the server accept loop so that it deals with our passing immediately.
     */
    Alert();
}

void TCPTransport::ManageEndpoints(Timespec authTimeout, Timespec sessionSetupTimeout)
{
    m_endpointListLock.Lock(MUTEX_CONTEXT);

    /*
     * Run through the list of connections on the authList and cleanup
     * any that are no longer running or are taking too long to authenticate
     * (we assume a denial of service attack in this case).
     */
    set<TCPEndpoint>::iterator i = m_authList.begin();
    while (i != m_authList.end()) {
        TCPEndpoint ep = *i;
        _TCPEndpoint::AuthState authState = ep->GetAuthState();

        if (authState == _TCPEndpoint::AUTH_FAILED) {
            /*
             * The endpoint has failed authentication and the auth thread is
             * gone or is going away.  Since it has failed there is no way this
             * endpoint is going to be started so we can get rid of it as soon
             * as we Join() the (failed) authentication thread.
             */
            QCC_DbgHLPrintf(("TCPTransport::ManageEndpoints(): Scavenging failed authenticator"));
            m_authList.erase(i);
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            ep->AuthJoin();
            m_endpointListLock.Lock(MUTEX_CONTEXT);
            i = m_authList.upper_bound(ep);
            continue;
        }

        Timespec tNow;
        GetTimeNow(&tNow);

        if (ep->GetStartTime() + authTimeout < tNow) {
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
            QCC_DbgHLPrintf(("TCPTransport::ManageEndpoints(): Scavenging slow authenticator"));
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
        TCPEndpoint ep = *i;

        /*
         * We are only managing passive connections here, or active connections
         * that are done and are explicitly ready to be cleaned up.
         */
        _TCPEndpoint::SideState sideState = ep->GetSideState();
        if (sideState == _TCPEndpoint::SIDE_ACTIVE) {
            ++i;
            continue;
        }

        _TCPEndpoint::AuthState authState = ep->GetAuthState();
        _TCPEndpoint::EndpointState endpointState = ep->GetEpState();

        if (authState == _TCPEndpoint::AUTH_SUCCEEDED) {
            /*
             * The endpoint has succeeded authentication and the auth thread is
             * gone or is going away.  Take this opportunity to join the auth
             * thread.  Since the auth thread promised not to touch the state
             * after setting AUTH_SUCCEEEDED, we can safely change the state
             * here since we now own the conn.  We do this through a method call
             * to enable this single special case where we are allowed to set
             * the state.
             */
            QCC_DbgHLPrintf(("TCPTransport::ManageEndpoints(): Scavenging failed authenticator"));
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            ep->AuthJoin();
            ep->SetAuthDone();
            m_endpointListLock.Lock(MUTEX_CONTEXT);
            i = m_endpointList.upper_bound(ep);
            continue;
        }
        /*
         * Passive endpoints need to be monitored between the time the endpoint is created via listen/accept
         * up until responsibility for  lifecycle of the endpoint can be transferred to the session management
         * code in AllJoynObj. Otherwise, an endpoint can exist indefinitely if no session related control
         * messages are received over the new endpoint.
         */
        if (authState == _TCPEndpoint::AUTH_DONE) {

            Timespec tNow;
            GetTimeNow(&tNow);
            if ((ep->GetFeatures().isBusToBus && !ep->IsSessionRouteSetUp()) && (ep->GetStartTime() + sessionSetupTimeout < tNow)) {
                /* This is a connection that timedout waiting for routing to be set up. Kill it */
                QCC_DbgHLPrintf(("TCPTransport:: Stopping endpoint that timedout waiting for routing to be set up %s.\n", ep->GetUniqueName().c_str()));
                ep->Stop();
            }
        }

        /*
         * There are two possibilities for the disposition of the Read and
         * WriteCallbacks.  First, they were never successfully started.  In
         * this case, the epState will be EP_FAILED.  If we find this, we
         * can just remove the useless endpoint from the list and delete
         * it.  Since the threads were never started, they must not be
         * joined.
         */
        if (endpointState == _TCPEndpoint::EP_FAILED) {
            m_endpointList.erase(i);
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            ep->AuthJoin();
            m_endpointListLock.Lock(MUTEX_CONTEXT);
            i = m_endpointList.upper_bound(ep);
            continue;
        }

        /*
         * The second possibility for the disposition of the Read and
         * WriteCallbacks is that they were successfully started but
         * have been stopped for some reason, either because of a
         * Disconnect() or a network error.  In this case, the
         * epState will be EP_STOPPING, which was set in the
         * EndpointExit function.  If we find this, we need to Join
         * the endpoint threads, remove the endpoint from the
         * endpoint list and delete it.  Note that we are calling
         * the endpoint Join() to join the TX and RX threads and not
         * the endpoint AuthJoin() to join the auth thread.
         */
        if (endpointState == _TCPEndpoint::EP_STOPPING) {
            m_endpointList.erase(i);
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            ep->AuthJoin();
            ep->Join();
            m_endpointListLock.Lock(MUTEX_CONTEXT);
            i = m_endpointList.upper_bound(ep);
            continue;
        }
        ++i;
    }
    m_endpointListLock.Unlock(MUTEX_CONTEXT);
}

void* TCPTransport::Run(void* arg)
{
    QCC_DbgTrace(("TCPTransport::Run()"));

    /*
     * We need to find the defaults for our connection limits.  These limits
     * can be specified in the configuration database with corresponding limits
     * used for DBus.  If any of those are present, we use them, otherwise we
     * provide some hopefully reasonable defaults.
     */
    ConfigDB* config = ConfigDB::GetConfigDB();

    /*
     * authTimeout is the maximum amount of time we allow incoming connections to
     * mess about while they should be authenticating.  If they take longer
     * than this time, we feel free to disconnect them as deniers of service.
     */
    Timespec authTimeout = config->GetLimit("auth_timeout", ALLJOYN_AUTH_TIMEOUT_DEFAULT);

    /*
     * sessionSetupTimeout is the maximum amount of time we allow incoming connections to
     * mess about while they should be sending messages to set up the session routes.
     * If they take longer than this time, we feel free to disconnect them as deniers of service.
     */
    Timespec sessionSetupTimeout = config->GetLimit("session_setup_timeout", ALLJOYN_SESSION_SETUP_TIMEOUT_DEFAULT);

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

    m_minHbeatIdleTimeout = config->GetLimit("tcp_min_idle_timeout", MIN_HEARTBEAT_IDLE_TIMEOUT_DEFAULT);
    m_maxHbeatIdleTimeout = config->GetLimit("tcp_max_idle_timeout", MAX_HEARTBEAT_IDLE_TIMEOUT_DEFAULT);
    m_defaultHbeatIdleTimeout = config->GetLimit("tcp_default_idle_timeout", DEFAULT_HEARTBEAT_IDLE_TIMEOUT_DEFAULT);

    m_numHbeatProbes = HEARTBEAT_NUM_PROBES;
    m_maxHbeatProbeTimeout = config->GetLimit("tcp_max_probe_timeout", MAX_HEARTBEAT_PROBE_TIMEOUT_DEFAULT);
    m_defaultHbeatProbeTimeout = config->GetLimit("tcp_default_probe_timeout", DEFAULT_HEARTBEAT_PROBE_TIMEOUT_DEFAULT);

    QCC_DbgPrintf(("TCPTransport: Using m_minHbeatIdleTimeout=%u, m_maxHbeatIdleTimeout=%u, m_numHbeatProbes=%u, m_defaultHbeatProbeTimeout=%u m_maxHbeatProbeTimeout=%u", m_minHbeatIdleTimeout, m_maxHbeatIdleTimeout, m_numHbeatProbes, m_defaultHbeatProbeTimeout, m_maxHbeatProbeTimeout));

    QStatus status = ER_OK;

    while (!IsStopping()) {

        /*
         * We did an Acquire on the name service in our Start() method which
         * ultimately caused this thread to run.  If we were the first transport
         * to Acquire() the name service, it will have done a Start() to crank
         * up its own run thread.  Just because we did that Start() before we
         * did our Start(), it does not necessarily mean that thread will come
         * up and run before us.  If we happen to come up before our name service
         * we'll hang around until it starts to run.  After all, nobody is going
         * to attempt to connect until we advertise something, and we need the
         * name service to advertise.
         */
        if (IpNameService::Instance().Started() == false) {
            QCC_DbgTrace(("TCPTransport::Run(): Wait for IP name service"));
            qcc::Sleep(1);
            continue;
        }

        /*
         * Each time through the loop we create a set of events to wait on.
         * We need to wait on the stop event and all of the SocketFds of the
         * addresses and ports we are listening on.  If the list changes, the
         * code that does the change Alert()s this thread and we wake up and
         * re-evaluate the list of SocketFds.
         * Set reload to STATE_RELOADED to indicate that the set of events has been reloaded.
         */
        m_listenFdsLock.Lock(MUTEX_CONTEXT);
        m_reload = STATE_RELOADED;
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
            for (vector<Event*>::iterator i = checkEvents.begin(); i != checkEvents.end(); ++i) {
                if (*i != &stopEvent) {
                    delete *i;
                }
            }
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
             * The stopEvent may get set indirectly by ManageEndpoints below, so
             * make sure to reset it before calling ManageEndpoints.
             */
            if (*i == &stopEvent) {
                stopEvent.ResetEvent();
            }

            /*
             * In order to rationalize management of resources, we manage the
             * various lists in one place on one thread.  This thread is a
             * convenient victim, so we do it here.
             */
            ManageEndpoints(authTimeout, sessionSetupTimeout);

            if (*i == &stopEvent) {
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

                QCC_DbgHLPrintf(("TCPTransport::Run(): Accepting connection newSock=%d", newSock));

                QCC_DbgPrintf(("TCPTransport::Run(): maxAuth == %d", maxAuth));
                QCC_DbgPrintf(("TCPTransport::Run(): maxConn == %d", maxConn));
                QCC_DbgPrintf(("TCPTransport::Run(): mAuthList.size() == %d", m_authList.size()));
                QCC_DbgPrintf(("TCPTransport::Run(): mEndpointList.size() == %d", m_endpointList.size()));
                assert(m_authList.size() + m_endpointList.size() <= maxConn);

                /*
                 * Do we have a slot available for a new connection?  If so, use
                 * it.
                 */
                m_endpointListLock.Lock(MUTEX_CONTEXT);
                if ((m_authList.size() < maxAuth) && (m_authList.size() + m_endpointList.size() < maxConn)) {
                    static const bool truthiness = true;
                    TCPTransport* ptr = this;
                    TCPEndpoint conn(ptr, m_bus, truthiness, TCPTransport::TransportName, newSock, remoteAddr, remotePort);
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
                    std::pair<std::set<TCPEndpoint>::iterator, bool> ins = m_authList.insert(conn);
                    status = conn->Authenticate();
                    if (status != ER_OK) {
                        m_authList.erase(ins.first);
                    }
                    m_endpointListLock.Unlock(MUTEX_CONTEXT);
                } else {
                    m_endpointListLock.Unlock(MUTEX_CONTEXT);
                    qcc::SetLinger(newSock, true, 0);
                    qcc::Shutdown(newSock);
                    qcc::Close(newSock);
                    status = ER_CONNECTION_LIMIT_EXCEEDED;
                    QCC_LogError(status, ("TCPTransport::Run(): No slot for new connection"));
                }
            }

            /*
             * Accept returns ER_WOULDBLOCK when all of the incoming connections have been handled
             */
            if (ER_WOULDBLOCK == status) {
                status = ER_OK;
            }

            if (status != ER_OK) {
                QCC_LogError(status, ("TCPTransport::Run(): Error accepting new connection. Ignoring..."));
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

    }

    /*
     * If we're stopping, it is our responsibility to clean up the list of FDs
     * we are listening to.  Since we've gotten a Stop() and are exiting the
     * server loop, and FDs are added in the server loop, this is the place to
     * get rid of them.  We don't have to take the list lock since a Stop()
     * request to the TCPTransport is required to lock out any new
     * requests that may possibly touch the listen FDs list.
     * Set m_reload to STATE_EXITED to indicate that the TCPTransport::Run
     * thread has exited.
     */
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    for (list<pair<qcc::String, SocketFd> >::iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
        qcc::SetLinger(i->second, true, 0);
        qcc::Shutdown(i->second);
        qcc::Close(i->second);
    }
    m_listenFds.clear();
    m_reload = STATE_EXITED;
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("TCPTransport::Run is exiting status=%s", QCC_StatusText(status)));
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
 * There are five member variables that reflect the state of the transport
 * and name service with respect to this code:
 *
 *   m_isListening:  The list of listeners is reflected by currently listening
 *     sockets.  We have network infrastructure in place to receive inbound
 *     connection requests.
 *
 *   m_isNsEnabled:  The name service is up and running and listening on its
 *     sockets for incoming requests.
 *
 *   m_isAdvertising: We are advertising at least one well-known name either actively or quietly .
 *     If we are m_isAdvertising then m_isNsEnabled must be true.
 *
 *   m_isDiscovering: The list of discovery requests has been sent to the name
 *     service.  If we are m_isDiscovering then m_isNsEnabled must be true.
 */
void TCPTransport::RunListenMachine(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("TCPTransport::RunListenMachine()"));
    /*
     * Do some consistency checks to make sure we're not confused about what
     * is going on.
     *
     * First, if we are not listening, then we had better not think we're
     * advertising(actively or quietly) or discovering.  If we are
     * not listening, then the name service must not be enabled and sending
     * or responding to external daemons.
     */
    if (m_isListening == false) {
        assert(m_isAdvertising == false);
        assert(m_isDiscovering == false);
        assert(m_isNsEnabled == false);
    }

    /*
     * If we think the name service is enabled, it had better think it is
     * enabled.  It must be enabled either because we have requested it to
     * start advertising (actively or quietly) or we are discovering.
     * If there are listeners, then the m_listenPortMap (a map matching
     * the different interfaces to the ports on which we are listening
     * on those interfaces) must be non-empty.
     */
    if (m_isNsEnabled && !m_stopping) {
        assert(m_isListening);
        assert(!m_listenPortMap.empty());
    }

    /*
     * If we think we are advertising, we'd better have an entry in
     * the advertisements list to advertise, and there must be
     * listeners waiting for inbound connections as a result of those
     * advertisements.  If we are advertising the name service had
     * better be enabled.
     */
    if (m_isAdvertising && !m_stopping) {
        assert(!m_advertising.empty());
        assert(m_isListening);
        assert(!m_listenPortMap.empty());
        assert(m_isNsEnabled);
    }

    /*
     * If we are discovering, we'd better have an entry in the discovering
     * list to make us discover, and there must be listeners waiting for
     * inbound connections as a result of session operations driven by those
     * discoveries.  If we are discovering the name service had better be
     * enabled.
     */
    if (m_isDiscovering && !m_stopping) {
        assert(!m_discovering.empty());
        assert(m_isListening);
        assert(!m_listenPortMap.empty());
        assert(m_isNsEnabled);
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

    case HANDLE_NETWORK_EVENT:
        HandleNetworkEventInstance(listenRequest);
        break;
    }
}

void TCPTransport::StartListenInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("TCPTransport::StartListenInstance()"));

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
     *
     * We have a bit of a chicken-and-egg problem when we want to start a quiet
     * advertisement of the daemon router for embedded AllJoyn clients.  We
     * don't want to start the quiet advertisement until we have a listener, but
     * then we don't start listeners until we have advertisements in order to
     * pass the Android Compatibility Test Suite.
     *
     * There is only one quiet advertisement that needs to be done
     * automagically, and this is the daemon router advertisement we do based on
     * configuration.  So, we take a peek at this configuration item and if it
     * is set, we go ahead and execute the DoStartListen to crank up a listener.
     * We actually start the quiet advertisement there in DoStartListen, after
     * we have a valid listener to respond to remote requests.  Note that we are
     * just driving the start listen, and there is no quiet advertisement yet so
     * the corresponding <m_isAdvertising> must not yet be set.
     */
    ConfigDB* config = ConfigDB::GetConfigDB();
    m_maxUntrustedClients = config->GetLimit("max_untrusted_clients", ALLJOYN_MAX_UNTRUSTED_CLIENTS_DEFAULT);

    routerName = config->GetProperty("router_advertisement_prefix", ALLJOYN_DEFAULT_ROUTER_ADVERTISEMENT_PREFIX);

    if (m_isAdvertising || m_isDiscovering || (!routerName.empty() && (m_numUntrustedClients < m_maxUntrustedClients))) {
        routerName.append(m_bus.GetInternal().GetGlobalGUID().ToShortString());
        DoStartListen(listenRequest.m_requestParam);
    }
}

void TCPTransport::StopListenInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("TCPTransport::StopListenInstance()"));

    /*
     * We have a new StopListen request, so we need to remove this
     * particular listen spec from our lists so it will not be
     * restarted.
     */
    bool empty = NewListenOp(STOP_LISTEN, listenRequest.m_requestParam);

    /*
     * If we have just removed the last listener, we have a problem if
     * we have advertisements.  This is because we will be
     * advertising soon to be non-existent endpoints.  The question is,
     * what do we want to do about it.  We could just ignore it since
     * since clients receiving advertisements may just try to connect to
     * a non-existent endpoint and fail.  It does seem better to log an
     * error and then cancel any outstanding advertisements since they
     * are soon to be meaningless.
     *
     */
    if (empty && m_isAdvertising) {
        QCC_LogError(ER_FAIL, ("TCPTransport::StopListenInstance(): No listeners with outstanding advertisements."));
        for (list<qcc::String>::iterator i = m_advertising.begin(); i != m_advertising.end(); ++i) {
            IpNameService::Instance().CancelAdvertiseName(TRANSPORT_TCP, *i, TRANSPORT_TCP);
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

void TCPTransport::EnableAdvertisementInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("TCPTransport::EnableAdvertisementInstance()"));

    /*
     * We have a new advertisement request to deal with.  The first
     * order of business is to save the well-known name away for
     * use later.
     */
    bool isFirst;
    NewAdvertiseOp(ENABLE_ADVERTISEMENT, listenRequest.m_requestParam, isFirst);

    /*
     * If it turned out that is the first advertisement on our list, we
     * need to prepare before actually doing the advertisement.
     */
    if (isFirst) {
        /*
         * If we don't have any listeners up and running, we need to get them
         * up.  If this is a Windows box, the listeners will start running
         * immediately and will never go down, so they may already be running.
         */
        if (!m_isListening) {
            for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
                QStatus status = DoStartListen(*i);
                if (ER_OK != status) {
                    continue;
                }
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
                IpNameService::Instance().Enable(TRANSPORT_TCP, m_listenPortMap, 0, std::map<qcc::String, uint16_t>(), 0, true, false, false, false);
                m_isNsEnabled = true;
            }
        }
    }

    /* If we encounter the situation where there are no listeners it is
     * possible that we don't have any of the specified interfaces IFF_UP
     * yet.  When those interfaces come up the backlog of advertisements
     * that came in will be processed.
     */
    if (!m_isListening) {
        QCC_DbgPrintf(("TCPTransport::EnableAdvertisementInstance(): Advertise with no TCP listeners"));
        if (!m_pendingAdvertisements.empty()) {
            for (std::list<ListenRequest>::iterator it = m_pendingAdvertisements.begin(); it != m_pendingAdvertisements.end(); it++) {
                if (listenRequest.m_requestParam == it->m_requestParam) {
                    return;
                }
            }
        }
        m_pendingAdvertisements.push_back(listenRequest);
        return;
    }

    /*
     * We think we're ready to send the advertisement.  Are we really?
     */
    assert(m_isListening);
    assert(!m_listenPortMap.empty());
    assert(m_isNsEnabled);
    assert(IpNameService::Instance().Started() && "TCPTransport::EnableAdvertisementInstance(): IpNameService not started");

    QStatus status = IpNameService::Instance().AdvertiseName(TRANSPORT_TCP, listenRequest.m_requestParam, listenRequest.m_requestParamOpt, listenRequest.m_requestTransportMask);
    if (status != ER_OK) {
        QCC_LogError(status, ("TCPTransport::EnableAdvertisementInstance(): Failed to advertise \"%s\"", listenRequest.m_requestParam.c_str()));
    }

    QCC_DbgPrintf(("TCPTransport::EnableAdvertisementInstance(): Done"));
    m_isAdvertising = true;
}

void TCPTransport::DisableAdvertisementInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("TCPTransport::DisableAdvertisementInstance()"));

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
    QStatus status = IpNameService::Instance().CancelAdvertiseName(TRANSPORT_TCP, listenRequest.m_requestParam, listenRequest.m_requestTransportMask);
    if (status != ER_OK) {
        QCC_LogError(status, ("TCPTransport::DisableAdvertisementInstance(): Failed to Cancel \"%s\"", listenRequest.m_requestParam.c_str()));
    }

    /* We check to make sure that this cancellation is not for an
     * advertisement that has not yet gone into effect because we
     * are still waiting on the specified network interface to
     * become IFF_UP.  We don't want to send out advertisements
     * whenever the interface comes up. If this is the last
     * advertisement and we are no longer discovering we should
     * not start listening when the interface comes up.
     */
    if (!m_pendingAdvertisements.empty()) {
        for (std::list<ListenRequest>::iterator it = m_pendingAdvertisements.begin(); it != m_pendingAdvertisements.end(); it++) {
            if (listenRequest.m_requestParam == it->m_requestParam) {
                m_pendingAdvertisements.erase(it);
                break;
            }
        }
    }

    /*
     * If it turns out that this was the last advertisement on our list, we need
     * to think about disabling our listeners and turning off the name service.
     * We only to this if there are no discovery instances in progress.
     */
    if (isEmpty && !m_isDiscovering) {

        /*
         * Since the cancel advertised name has been sent, we can disable the
         * name service.  We do this by telling it we don't want it to be
         * enabled on any of the possible ports.
         */
        IpNameService::Instance().Enable(TRANSPORT_TCP, m_listenPortMap, 0, std::map<qcc::String, uint16_t>(), 0, false, false, false, false);
        m_isNsEnabled = false;

        /*
         * If we had the name service running, we must have had listeners
         * waiting for connections due to the name service.  We need to stop
         * them all now, but only if we are not running on a Windows box.
         * Windows needs the listeners running at all times since it uses
         * TCP for the client to daemon connections.  The listen spec may
         * involve network interfaces or IP addresses and we need to do
         * some translation from a specified network interface to the
         * corresponding IP address.
         */
        for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
            map<qcc::String, qcc::String> argMap;
            qcc::String spec;
            QStatus status = NormalizeListenSpec(i->c_str(), spec, argMap);
            assert(status == ER_OK && "TCPTransport::DisableAdvertisementInstance(): Invalid TCP listen spec");
            (void)status;  // Suppress unused warning from G++ when building is release mode.
            if (argMap.find("iface") != argMap.end()) {
                qcc::String interface = argMap["iface"];
                qcc::String normSpec = "tcp:addr=" + m_requestedInterfaces[interface].GetAddress().ToString() + ",port=" + U32ToString(m_requestedInterfaces[interface].GetPort());
                DoStopListen(normSpec);
            } else if (argMap.find("addr") != argMap.end()) {
                DoStopListen(*i);
            }
        }

        m_isListening = false;
        m_listenPortMap.clear();
        m_pendingDiscoveries.clear();
        m_pendingAdvertisements.clear();
        m_wildcardIfaceProcessed = false;
    }

    if (isEmpty) {
        m_isAdvertising = false;
    }
}

void TCPTransport::EnableDiscoveryInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("TCPTransport::EnableDiscoveryInstance()"));

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
         * However, we only set up the m_listenPortMap (a map of the network
         * interfaces to the corresponding ports on which we are listening)
         * when we are sure that the specified network interfaces are IFF_UP.
         */
        if (!m_isListening) {
            for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
                QStatus status = DoStartListen(*i);
                if (ER_OK != status) {
                    continue;
                }
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
                IpNameService::Instance().Enable(TRANSPORT_TCP, m_listenPortMap, 0, std::map<qcc::String, uint16_t>(), 0, true, false, false, false);
                m_isNsEnabled = true;
            }
        }
    }

    /* If we encounter the situation where there are no listeners it is
     * possible that we don't have any of the specified interfaces IFF_UP
     * yet, when those interfaces come up the backlog of discoveries
     * that came in will be processed.
     */
    if (!m_isListening) {
        QCC_DbgPrintf(("TCPTransport::EnableDiscoveryInstance(): Discover with no TCP listeners"));
        if (!m_pendingDiscoveries.empty()) {
            for (std::list<ListenRequest>::iterator it = m_pendingDiscoveries.begin(); it != m_pendingDiscoveries.end(); it++) {
                if (listenRequest.m_requestParam == it->m_requestParam) {
                    return;
                }
            }
        }
        m_pendingDiscoveries.push_back(listenRequest);
        return;
    }

    /*
     * We think we're ready to send the FindAdvertisement.  Are we really?
     */
    assert(m_isListening);
    assert(!m_listenPortMap.empty());
    assert(m_isNsEnabled);
    assert(IpNameService::Instance().Started() && "TCPTransport::EnableDiscoveryInstance(): IpNameService not started");

    QStatus status = IpNameService::Instance().FindAdvertisement(TRANSPORT_TCP, listenRequest.m_requestParam, listenRequest.m_requestTransportMask);
    if (status != ER_OK) {
        QCC_LogError(status, ("TCPTransport::EnableDiscoveryInstance(): Failed to begin discovery with multicast NS \"%s\"", listenRequest.m_requestParam.c_str()));
    }

    m_isDiscovering = true;
}

void TCPTransport::DisableDiscoveryInstance(ListenRequest& listenRequest)
{
    QCC_DbgPrintf(("TCPTransport::DisableDiscoveryInstance()"));

    /*
     * We have a new disable discovery request to deal with.  The first
     * order of business is to remove the well-known name from our saved list.
     */
    bool isFirst;
    bool isEmpty = NewDiscoveryOp(DISABLE_DISCOVERY, listenRequest.m_requestParam, isFirst);

    if (m_isListening && !m_listenPortMap.empty() && m_isNsEnabled && IpNameService::Instance().Started()) {
        QStatus status = IpNameService::Instance().CancelFindAdvertisement(TRANSPORT_TCP, listenRequest.m_requestParam, listenRequest.m_requestTransportMask);
        if (status != ER_OK) {
            QCC_LogError(status, ("TCPTransport::DisableDiscoveryInstance(): Failed to cancel discovery with \"%s\"", listenRequest.m_requestParam.c_str()));
        }
    }

    if (!m_pendingDiscoveries.empty()) {
        for (std::list<ListenRequest>::iterator it = m_pendingDiscoveries.begin(); it != m_pendingDiscoveries.end(); it++) {
            if (listenRequest.m_requestParam == it->m_requestParam) {
                m_pendingDiscoveries.erase(it);
                break;
            }
        }
    }

    /*
     * If it turns out that this was the last discovery operation on
     * our list, we need to think about disabling our listeners and turning off
     * the name service.  We only to this if there are no advertisements in
     * progress.
     */
    if (isEmpty && !m_isAdvertising) {

        IpNameService::Instance().Enable(TRANSPORT_TCP, m_listenPortMap, 0, std::map<qcc::String, uint16_t>(), 0, false, false, false, false);
        m_isNsEnabled = false;

        /*
         * If we had the name service running, we must have had listeners
         * waiting for connections due to the name service.  We need to stop
         * them all now, but only if we are not running on a Windows box.
         * Windows needs the listeners running at all times since it uses
         * TCP for the client to daemon connections.  The listen spec may
         * involve network interfaces or IP addresses and we need to do
         * some translation from a specified network interface to the
         * corresponding IP address.
         */
        for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
            map<qcc::String, qcc::String> argMap;
            qcc::String spec;
            QStatus status = NormalizeListenSpec(i->c_str(), spec, argMap);
            assert(status == ER_OK && "TCPTransport::DisableDiscoveryInstance(): Invalid TCP listen spec");
            (void)status;  // Suppress unused warning from G++ when building is release mode.
            if (argMap.find("iface") != argMap.end()) {
                qcc::String interface = argMap["iface"];
                qcc::String normSpec = "tcp:addr=" + m_requestedInterfaces[interface].GetAddress().ToString() + ",port=" + U32ToString(m_requestedInterfaces[interface].GetPort());
                DoStopListen(normSpec);
            } else if (argMap.find("addr") != argMap.end()) {
                DoStopListen(*i);
            }
        }

        m_isListening = false;
        m_listenPortMap.clear();
        m_pendingDiscoveries.clear();
        m_pendingAdvertisements.clear();
        m_wildcardIfaceProcessed = false;
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

/*
 * The default port for use in listen specs.  This port is used by the TCP
 * listener to listen for incoming connection requests.  This is the default
 * port for a "reliable" IPv4 listener since being able to deal with IPv4
 * connection requests is required as part of the definition of the TCP
 * transport.
 *
 * All other mechanisms (unreliable IPv4, reliable IPv6, unreliable IPv6)
 * rely on the presence of an u4port, r6port, and u6port respectively to
 * enable those mechanisms if possible.
 */
static const uint16_t PORT_DEFAULT = 9955;

QStatus TCPTransport::NormalizeListenSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    qcc::String family;

    /*
     * We don't make any calls that require us to be in any particular state
     * with respect to threading so we don't bother to call IsRunning() here.
     *
     * Take the string in inSpec, which must start with "tcp:" and parse it,
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
     *     "tcp:addr=0.0.0.0,port=9955"
     *
     * That's all.  We still allow "addr=0.0.0.0,port=9955,family=ipv4" but
     * since the only thing that was ever allowed was really reliable IPv4, we
     * ignore family.  The old stuff is normalized to the above.
     *
     * In the future we may want to revisit this and use position/order of keys
     * to imply more information.  For example:
     *
     *     "addr=0.0.0.0,port=9955,family=ipv4,reliable=true,
     *      addr=0.0.0.0,port=9956,family=ipv4,reliable=false;"
     *
     * might translate into:
     *
     *     "tcp:addr=0.0.0.0,port=9955"
     *      udp:addr=0.0.0.0,port=9956;"
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
     * incarnation, the TCP transport will only support reliable IPv4; so we
     * log errors and ignore any requests for other mechanisms.
     */
    iter = argMap.find("u4addr");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("TCPTransport::NormalizeListenSpec(): The mechanism implied by \"u4addr\" is not supported."));
        argMap.erase(iter);
    }

    iter = argMap.find("u4port");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("TCPTransport::NormalizeListenSpec(): The mechanism implied by \"u4port\" is not supported."));
        argMap.erase(iter);
    }

    iter = argMap.find("r6addr");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("TCPTransport::NormalizeListenSpec(): The mechanism implied by \"r6addr\" is not supported."));
        argMap.erase(iter);
    }

    iter = argMap.find("r6port");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("TCPTransport::NormalizeListenSpec(): The mechanism implied by \"r6port\" is not supported."));
        argMap.erase(iter);
    }

    iter = argMap.find("u6addr");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("TCPTransport::NormalizeListenSpec(): The mechanism implied by \"u6addr\" is not supported."));
        argMap.erase(iter);
    }

    iter = argMap.find("u6port");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("TCPTransport::NormalizeListenSpec(): The mechanism implied by \"u6port\" is not supported."));
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
     * The TCP transport must absolutely support the IPv4 "reliable" mechanism
     * (TCP).  We therefore must provide an addr either from explicit keys or
     * generated from the defaults.
     */
    iter = argMap.find("iface");

    /*
     * Now, deal with the iface.
     */
    if (iter != argMap.end()) {
        outSpec.append("iface=" + iter->second);
    } else {
        iter = argMap.find("addr");
        if (iter == argMap.end()) {
            /*
             * We have no value associated with an "addr" key.  Do we have an
             * "r4addr" which would be synonymous?  If so, save it as an addr,
             * erase it and point back to the new addr.
             */
            iter = argMap.find("r4addr");
            if (iter != argMap.end()) {
                argMap["addr"] = iter->second;
                argMap.erase(iter);
            }

            iter = argMap.find("addr");
        }

        /*
         * Now, deal with the addr, possibly derived from r4addr.
         */
        if (iter != argMap.end()) {
            /*
             * We have a value associated with the "addr" key.  Run it through a
             * conversion function to make sure it's a valid value and to get into
             * in a standard representation.
             */
            IPAddress addr;
            status = addr.SetAddress(iter->second, false);
            if (status == ER_OK) {
                /*
                 * The addr had better be an IPv4 address, otherwise we bail.
                 */
                if (!addr.IsIPv4()) {
                    QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                                 ("TCPTransport::NormalizeListenSpec(): The addr \"%s\" is not a legal IPv4 address.",
                                  iter->second.c_str()));
                    return ER_BUS_BAD_TRANSPORT_ARGS;
                }
                iter->second = addr.ToString();
                outSpec.append("addr=" + addr.ToString());
            } else {
                QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                             ("TCPTransport::NormalizeListenSpec(): The addr \"%s\" is not a legal IPv4 address.",
                              iter->second.c_str()));
                return ER_BUS_BAD_TRANSPORT_ARGS;
            }
        }
    }

    if (iter == argMap.end()) {
        /*
         * We have no value associated with an "iface" or "addr" key.  Use the default
         * network interface name for the outspec and create a new key for the map.
         */
        outSpec.append("iface=" + qcc::String(INTERFACES_DEFAULT));
        argMap["iface"] = INTERFACES_DEFAULT;
    }

    /*
     * The TCP transport must absolutely support the IPv4 "reliable" mechanism
     * (TCP).  We therefore must provide an port either from explicit keys or
     * generated from the defaults.
     */
    iter = argMap.find("port");
    if (iter == argMap.end()) {
        /*
         * We have no value associated with a "port" key.  Do we have a
         * "r4port" which would be synonymous?  If so, save it as a port,
         * erase it and point back to the new port.
         */
        iter = argMap.find("r4port");
        if (iter != argMap.end()) {
            argMap["port"] = iter->second;
            argMap.erase(iter);
        }

        iter = argMap.find("port");
    }

    /*
     * Now, deal with the port.
     */
    if (iter != argMap.end()) {
        /*
         * We have a value associated with the "port" key.  Run it through a
         * conversion function to make sure it's a valid value.  We put it into
         * a 32 bit int to make sure it will actually fit into a 16-bit port
         * number.
         */
        uint32_t port = StringToU32(iter->second);
        if (port <= 0xffff) {
            outSpec.append(",port=" + iter->second);
        } else {
            QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                         ("TCPTransport::NormalizeListenSpec(): The key \"port\" has a bad value \"%s\".", iter->second.c_str()));
            return ER_BUS_BAD_TRANSPORT_ARGS;
        }
    } else {
        /*
         * We have no value associated with an "port" key.  Use the default
         * IPv4 listen port for the outspec and create a new key for the map.
         */
        qcc::String portString = U32ToString(PORT_DEFAULT);
        outSpec += ",port=" + portString;
        argMap["port"] = portString;
    }
    return ER_OK;
}

QStatus TCPTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    QCC_DbgPrintf(("TCPTransport::NormalizeTransportSpec"));

    QStatus status;

    /*
     * Aside from the presence of the guid, the only fundamental difference
     * between a listenSpec and a transportSpec (actually a connectSpec) is that
     * a connectSpec must have a valid and specific address IP address to
     * connect to (i.e., INADDR_ANY isn't a valid IP address to connect to).
     * This means that we can just call NormalizeListenSpec to get everything
     * into standard form.
     */
    status = NormalizeListenSpec(inSpec, outSpec, argMap);
    if (status != ER_OK) {
        return status;
    }

    /*
     * Since there is no guid present if we've fallen through to here, the only
     * difference between a connectSpec and a listenSpec is that a connectSpec
     * requires the presence of a non-default IP address.  So we just check for
     * the default addresses and fail if we find one.
     */
    map<qcc::String, qcc::String>::iterator i = argMap.find("addr");
    assert(i != argMap.end());
    if ((i->second == ADDR4_DEFAULT)) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("TCPTransport::NormalizeTransportSpec(): The addr may not be the default address."));
        return ER_BUS_BAD_TRANSPORT_ARGS;
    }

    return ER_OK;
}

QStatus TCPTransport::Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newEp)
{
    QCC_DbgHLPrintf(("TCPTransport::Connect(): %s", connectSpec));

    /*
     * We are given a session options structure that defines the kind of
     * connection that is being sought.  Can we support the connection being
     * requested?  If not, don't even try.
     */
    if (SupportsOptions(opts) == false) {
        QStatus status = ER_BUS_BAD_SESSION_OPTS;
        QCC_LogError(status, ("TCPTransport::Connect(): Supported options mismatch"));
        return status;
    }

    /*
     * We need to find the defaults for our connection limits.  These limits
     * can be specified in the configuration database with corresponding limits
     * used for DBus.  If any of those are present, we use them, otherwise we
     * provide some hopefully reasonable defaults.
     */
    ConfigDB* config = ConfigDB::GetConfigDB();

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

    QStatus status;
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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("TCPTransport::Connect(): Not running or stopping; exiting"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    /*
     * If we pass the IsRunning() gate above, we must have a server accept
     * thread spinning up or shutting down but not yet joined.  Since the name
     * service is started before the server accept thread is spun up, and
     * deleted after it is joined, we must have a started name service or someone
     * isn't playing by the rules; so an assert is appropriate here.
     */
    assert(IpNameService::Instance().Started() && "TCPTransport::Connect(): IpNameService not started");

    /*
     * Parse and normalize the connectArgs.  When connecting to the outside
     * world, there are no reasonable defaults and so the addr and port keys
     * MUST be present.
     */
    qcc::String normSpec;
    map<qcc::String, qcc::String> argMap;
    status = NormalizeTransportSpec(connectSpec, normSpec, argMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("TCPTransport::Connect(): Invalid TCP connect spec \"%s\"", connectSpec));
        return status;
    }

    /*
     * These fields (addr, port) Are all guaranteed to be present now and an
     * underlying network (even if it is Wi-Fi P2P) is assumed to be up and
     * functioning.
     */
    assert(argMap.find("addr") != argMap.end() && "TCPTransport::Connect(): addr not present in argMap");
    assert(argMap.find("port") != argMap.end() && "TCPTransport::Connect(): port not present in argMap");

    IPAddress ipAddr(argMap.find("addr")->second);
    uint16_t port = StringToU32(argMap["port"]);

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
    char anyspec[64];
    snprintf(anyspec, sizeof(anyspec), "%s:addr=0.0.0.0,port=%u", GetTransportName(), port);

    qcc::String normAnySpec;
    map<qcc::String, qcc::String> normArgMap;
    status = NormalizeListenSpec(anyspec, normAnySpec, normArgMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("TCPTransport::Connect(): Invalid INADDR_ANY connect spec"));
        return status;
    }

    /*
     * Look to see if we are already listening on the provided connectSpec
     * either explicitly or via the INADDR_ANY address.
     */
    QCC_DbgHLPrintf(("TCPTransport::Connect(): Checking for connection to self"));
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    bool anyEncountered = false;
    for (list<pair<qcc::String, SocketFd> >::iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
        QCC_DbgHLPrintf(("TCPTransport::Connect(): Checking listenSpec %s", i->first.c_str()));

        /*
         * If the provided connectSpec is already explicitly listened to, it is
         * an error.
         */
        if (i->first == normSpec) {
            m_listenFdsLock.Unlock(MUTEX_CONTEXT);
            QCC_DbgHLPrintf(("TCPTransport::Connect(): Explicit connection to self"));
            return ER_BUS_ALREADY_LISTENING;
        }

        /*
         * If we are listening to INADDR_ANY and the supplied port, then we have
         * to look to the currently UP interfaces to decide if this call is bogus
         * or not.  Set a flag to remind us.
         */
        if (i->first == normAnySpec) {
            QCC_DbgHLPrintf(("TCPTransport::Connect(): Possible implicit connection to self detected"));
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
        QCC_DbgHLPrintf(("TCPTransport::Connect(): Checking for implicit connection to self"));
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
                QCC_DbgHLPrintf(("TCPTransport::Connect(): Checking interface %s", entries[i].m_name.c_str()));
                if (entries[i].m_flags & qcc::IfConfigEntry::UP) {
                    QCC_DbgHLPrintf(("TCPTransport::Connect(): Interface UP with addresss %s", entries[i].m_addr.c_str()));
                    IPAddress foundAddr(entries[i].m_addr);
                    if (foundAddr == ipAddr) {
                        QCC_DbgHLPrintf(("TCPTransport::Connect(): Attempted connection to self; exiting"));
                        return ER_BUS_ALREADY_LISTENING;
                    }
                }
            }
        }
    }



    static const bool falsiness = false;
    TCPTransport* ptr = this;
    AddressFamily family = QCC_AF_INET;
    SocketType type = QCC_SOCK_STREAM;

    TCPEndpoint tcpEp = TCPEndpoint(ptr, m_bus, falsiness, normSpec, family, type, ipAddr, port);
    /*
     * Before starting the underlying transport mechanism, we need to create
     * a TCPEndpoint object that will orchestrate the movement of data
     * across the transport.
     */


    /*
     * On the active side of a connection, we don't need an authentication
     * thread to run since we have the caller thread to fill that role.
     */
    tcpEp->SetActive();

    /*
     * Initialize the "features" for this endpoint
     */
    tcpEp->GetFeatures().isBusToBus = true;
    tcpEp->GetFeatures().allowRemote = m_bus.GetInternal().AllowRemoteMessages();
    tcpEp->GetFeatures().handlePassing = false;
    tcpEp->GetFeatures().nameTransfer = opts.nameTransfer;

    /*
     * Check any application connecting over TCP to see if it is running on the same machine and
     * set the group ID appropriately if so.
     */
    CheckEndpointLocalMachine(tcpEp);

    qcc::String authName;
    qcc::String redirection;

    /*
     * This is a little tricky.  We usually manage endpoints in one place
     * using the main server accept loop thread.  Endpoints read and write data
     * by using a callback mechanism implemented using IODispatch and these
     * callbacks are expected to be made regularly until the EndpointExit function
     * is called when the endpoints are stopped.  The general endpoint management uses these
     * mechanisms.  However, we are about to get into a state where we are
     * off trying to start an endpoint, but we are using another thread
     * which has called into TCPTransport::Connect().  We are about to do
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

    /*
     * Go ahead and do the authentication in the context of this thread.  Even
     * though we don't have the server accept loop thread watching this endpoint
     * we keep we keep the states consistent since the endpoint will eventually
     * to there.
     */
    DaemonRouter& router = reinterpret_cast<DaemonRouter&>(m_bus.GetInternal().GetRouter());
    AuthListener* authListener = router.GetBusController()->GetAuthListener();
    QCC_DbgPrintf(("TCPTransport::Connect(): maxAuth == %d", maxAuth));
    QCC_DbgPrintf(("TCPTransport::Connect(): maxConn == %d", maxConn));
    QCC_DbgPrintf(("TCPTransport::Connect(): mAuthList.size() == %d", m_authList.size()));
    QCC_DbgPrintf(("TCPTransport::Connect(): mEndpointList.size() == %d", m_endpointList.size()));

    /* Ensure that the connection limits for TCPEndpoints have not been hit. */
    if ((m_authList.size() < maxAuth) && (m_authList.size() + m_endpointList.size() < maxConn)) {
        m_authList.insert(tcpEp);
        status = ER_OK;
    } else {
        QCC_LogError(status, ("TCPTransport::Connect(): No slot for new connection"));
        /* Remove this thread from the m_activeEndpointsThreadList */
        set<Thread*>::iterator i = find(m_activeEndpointsThreadList.begin(), m_activeEndpointsThreadList.end(), thread);
        assert(i != m_activeEndpointsThreadList.end() && "TCPTransport::Connect(): Thread* not on m_activeEndpointsThreadList");
        m_activeEndpointsThreadList.erase(i);
        m_endpointListLock.Unlock(MUTEX_CONTEXT);
        return ER_CONNECTION_LIMIT_EXCEEDED;
    }
    m_endpointListLock.Unlock();
    status = tcpEp->m_stream.SetNagle(false);

    if (status == ER_OK) {
        /*
         * We got a socket, created an endpoint and inserted it into the m_authList,
         * now tell TCP to connect to the remote address and
         * port.
         */
        String addrStr = ipAddr.ToString();
        status = tcpEp->m_stream.Connect(addrStr, port);
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

            status = tcpEp->m_stream.PushBytes(&nul, 1, sent);
            if (status != ER_OK) {
                QCC_LogError(status, ("TCPTransport::Connect(): Failed to send initial NUL byte"));
            }
            isConnected = true;
        } else {
            QCC_LogError(status, ("TCPTransport::Connect(): Failed"));
        }
    }
    if (status == ER_OK) {

        status = tcpEp->Establish("ANONYMOUS", authName, redirection, authListener);
        if (status == ER_OK) {
            tcpEp->SetListener(this);
            tcpEp->SetEpStarting();
            status = tcpEp->Start(m_defaultHbeatIdleTimeout, m_defaultHbeatProbeTimeout, m_numHbeatProbes, m_maxHbeatProbeTimeout);
            if (status == ER_OK) {
                tcpEp->SetEpStarted();
                tcpEp->SetAuthDone();
            } else {
                tcpEp->SetEpFailed();
                tcpEp->SetAuthDone();
            }
        }
        /*
         * If we have a successful authentication, we pass the connection off to the
         * server accept loop to manage.
         */
        if (status == ER_OK) {
            m_endpointListLock.Lock(MUTEX_CONTEXT);
            m_authList.erase(tcpEp);
            m_endpointList.insert(tcpEp);
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            newEp = BusEndpoint::cast(tcpEp);
        }
    }
    if (status != ER_OK) {
        if (isConnected) {
            tcpEp->m_stream.Close();
        }
        m_endpointListLock.Lock(MUTEX_CONTEXT);
        m_authList.erase(tcpEp);
        m_endpointListLock.Unlock();
        /* If we got this connection and its endpoint up without
         * a problem, we return a pointer to the new endpoint.  We aren't going to
         * clean it up since it is an active connection, so we can safely pass the
         * endoint back up to higher layers.
         * Invalidate the endpoint in case of error.
         */
        newEp->Invalidate();
    }
    /*
     * In any case, we are done with blocking I/O on the current thread, so
     * we need to remove its pointer from the list we kept around to break it
     * out of blocking I/O.  If we were successful, the TCPEndpoint was passed
     * to the m_endpointList, where the main server accept loop will deal with
     * it using its Read and WriteCallback-based mechanisms.  If we were unsuccessful
     * the TCPEndpoint was destroyed and we will return an error below after
     * cleaning up the underlying socket.
     */
    m_endpointListLock.Lock(MUTEX_CONTEXT);
    set<Thread*>::iterator i = find(m_activeEndpointsThreadList.begin(), m_activeEndpointsThreadList.end(), thread);
    assert(i != m_activeEndpointsThreadList.end() && "TCPTransport::Connect(): Thread* not on m_activeEndpointsThreadList");
    m_activeEndpointsThreadList.erase(i);
    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    return status;
}

QStatus TCPTransport::Disconnect(const char* connectSpec)
{
    QCC_DbgHLPrintf(("TCPTransport::Disconnect(): %s", connectSpec));

    /*
     * Disconnect is actually not used in the transports architecture.  It is
     * misleading and confusing to have it implemented.
     */
    assert(0 && "TCPTransport::Disconnect(): Unexpected call");
    QCC_LogError(ER_FAIL, ("TCPTransport::Disconnect(): Unexpected call"));
    return ER_FAIL;
#if UNUSED_CODE
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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("TCPTransport::Disconnect(): Not running or stopping; exiting"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    /*
     * If we pass the IsRunning() gate above, we must have a server accept
     * thread spinning up or shutting down but not yet joined.  Since the name
     * service is started before the server accept thread is spun up, and
     * stopped after it is stopped, we must have a started name service or
     * someone isn't playing by the rules; so an assert is appropriate here.
     */
    assert(IpNameService::Instance().Started() && "TCPTransport::Disconnect(): IpNameService not started");

    /*
     * Higher level code tells us which connection is refers to by giving us the
     * same connect spec it used in the Connect() call.  We have to determine the
     * address and port in exactly the same way
     */
    qcc::String normSpec;
    map<qcc::String, qcc::String> argMap;
    QStatus status = NormalizeTransportSpec(connectSpec, normSpec, argMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("TCPTransport::Disconnect(): Invalid TCP connect spec \"%s\"", connectSpec));
        return status;
    }

    IPAddress ipAddr(argMap.find("addr")->second); // Guaranteed to be there.
    uint16_t port = StringToU32(argMap["port"]);   // Guaranteed to be there.

    /*
     * Stop the remote endpoint.  Be careful here since calling Stop() on the
     * TCPEndpoint is going to cause the transmit and receive threads of the
     * underlying RemoteEndpoint to exit, which will cause our EndpointExit()
     * to be called, which will walk the list of endpoints and delete the one
     * we are stopping.  Once we poke ep->Stop(), the pointer to ep must be
     * considered dead.
     */
    status = ER_BUS_BAD_TRANSPORT_ARGS;
    m_endpointListLock.Lock(MUTEX_CONTEXT);
    for (set<TCPEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        TCPEndpoint ep = *i;
        if (ep->GetPort() == port && ep->GetIPAddress() == ipAddr) {
            ep->SetSuddenDisconnect(false);
            m_endpointListLock.Unlock(MUTEX_CONTEXT);

            return ep->Stop();
        }
    }
    m_endpointListLock.Unlock(MUTEX_CONTEXT);
    return status;
#endif
}

QStatus TCPTransport::StartListen(const char* listenSpec)
{
    QCC_DbgPrintf(("TCPTransport::StartListen()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("TCPTransport::StartListen(): Not running or stopping; exiting"));
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
        QCC_LogError(status, ("TCPTransport::StartListen(): Invalid TCP listen spec \"%s\"", listenSpec));
        return status;
    }

    /*
     * We allow the listen request to be specified with either
     * a network interface name or an IP address.
     */
    qcc::String key = "";
    if (argMap.find("iface") != argMap.end()) {
        key = "iface";
    } else if (argMap.find("addr") != argMap.end()) {
        key = "addr";
    }
    QCC_DbgPrintf(("TCPTransport::StartListen(): %s = \"%s\", port = \"%s\"",
                   key.c_str(), argMap[key].c_str(), argMap["port"].c_str()));

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

void TCPTransport::QueueStartListen(qcc::String& normSpec)
{
    QCC_DbgPrintf(("TCPTransport::QueueStartListen()"));

    /*
     * In order to start a listen, we send the server accept thread a message
     * containing the START_LISTEN_INSTANCE request code and the normalized
     * listen spec which specifies the address and port instance to listen on.
     */
    ListenRequest listenRequest;
    listenRequest.m_requestOp = START_LISTEN_INSTANCE;
    listenRequest.m_requestParam = normSpec;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

}

QStatus TCPTransport::DoStartListen(qcc::String& normSpec)
{
    QCC_DbgPrintf(("TCPTransport::DoStartListen()"));

    /*
     * Since the name service is created before the server accept thread is spun
     * up, and stopped when it is stopped, we must have a started name service or
     * someone isn't playing by the rules; so an assert is appropriate here.
     */
    assert(IpNameService::Instance().Started() && "TCPTransport::DoStartListen(): IpNameService not started");

    qcc::String interfaces = ConfigDB::GetConfigDB()->GetProperty("ns_interfaces");
    if (interfaces.size()) {
        QCC_LogError(ER_WARNING, ("TCPTransport::DoStartListen(): The mechanism implied by \"ns_interfaces\" is no longer supported."));
    }
    /*
     * Parse the normalized listen spec.  The easiest way to do this is to
     * re-normalize it.  If there's an error at this point, we have done
     * something wrong since the listen spec was presumably successfully
     * normalized before sending it in -- so we assert.
     */
    qcc::String spec;
    map<qcc::String, qcc::String> argMap;
    QStatus status = NormalizeListenSpec(normSpec.c_str(), spec, argMap);
    assert(status == ER_OK && "TCPTransport::DoStartListen(): Invalid TCP listen spec");

    qcc::String key = "";
    if (argMap.find("iface") != argMap.end()) {
        key = "iface";
    } else if (argMap.find("addr") != argMap.end()) {
        key = "addr";
    }
    QCC_DbgPrintf(("TCPTransport::DoStartListen(): %s = \"%s\", port = \"%s\"",
                   key.c_str(), argMap[key].c_str(), argMap["port"].c_str()));

    /*
     * If we're going to listen on an address, we are going to listen on a
     * corresponding network interface.  We need to convince the name service
     * to send advertisements out over that interface, or nobody will know to
     * connect to the listening daemon.
     *
     * So, we need to get the configuration item telling us which network
     * interfaces we should run the name service over and listen on.  The
     * name service waits until it finds the specified interface IFF_UP and
     * multicast capable with an assigned IP address and then starts using
     * the interface.  If the configuration item contains "*" (the wildcard)
     * it is interpreted as meaning all multicast-capable interfaces.  If the
     * configuration item is empty (not assigned in the configuration database)
     * it defaults to "*".
     */

    /*
     * We have been given a listenSpec that provides an iface or addr and a port
     * in the parameters to this method.  We are expected to listen on that
     * network interface's primary IPv4 address and port for inbound connections.
     * The name service will also advertise and discover over this network interface.
     *
     * We can either be given the wildcard iface "*", a specific network
     * interface name, the default address "0.0.0.0" or a specific address.
     * If given "*" or "0.0.0.0", this means that the TCP Transport will
     * listen for inbound connections on the INADDR_ANY address and the name
     * service will advertise and discover over any currently IFF_UP interface
     * or any interface that may come IFF_UP in the future.
     *
     * If given a network interface name, the TCP Transport will listen for
     * inbound connections on the current primary IPv4 address of that network
     * interface and the name service will advertise and discover over that
     * network interface.
     *
     * If given a specific address, the TCP Transport will listen for
     * inbound connections on the specified address and the name service
     * will advertise and discover over the underlying network interface
     * as long as it retains that address.
     *
     *    iface                 Action
     *    --------               -----------------------------------------
     * 1.  *                      Listen on 0.0.0.0 and advertise/discover
     *                            over '*'.  This is the default case where
     *                            the system listens on all interfaces and
     *                            advertises / discovers on all interfaces.
     *                            This is the "speak alljoyn over all of
     *                            your interfaces" situation.
     *
     * 2.  'specific'             Listen only on the primary address of the
     *                            named netowrk interface and advertise and
     *                            discover over that specific interface.
     *                            This may not be exactly what is desired,
     *                            but it may be.  We do what we are told.
     *                            Note that by doing this, one is limiting
     *                            the number of daemons that can be run on
     *                            a host using the same address and port
     *                            to one.  Other daemons configured this
     *                            way must select another port. This is
     *                            how we expect people to limit AllJoyn to
     *                            talking only over a specific interface.
     *                            This allows that interface to change IP
     *                            addresses on the fly. This reqires the
     *                            interface name to be known a-priori but
     *                            does not require the IP address of the
     *                            network interface named 'specific' to be
     *                            known a-priori.
     *
     *    address                Action
     *    --------               -----------------------------------------
     * 1.  0.0.0.0                Listen on 0.0.0.0 and advertise/discover
     *                            over '*'.  This is the default case where
     *                            the system listens on all interfaces and
     *                            advertises / discovers on all interfaces.
     *                            This is the "speak alljoyn over all of
     *                            your interfaces" situation.
     *
     * 2.  'a.b.c.d'              Listen only on the specified address if
     *                            or when it appears on the network and
     *                            advertise and discover over the
     *                            underlying interface for that address,
     *                            so long as it retains the address.
     *                            This may not be exactly what is desired,
     *                            but it may be.  We do what we are told.
     *                            Note that by doing this, one is limiting
     *                            the number of daemons that can be run on
     *                            a host using the same address and port
     *                            to one.  Other daemons configured this
     *                            way must select another port.
     *
     * This is much harder to describe than to implement; but the upshot is
     * that we listen on the primary IPv4 address of the named network interface
     * that comes in with the listenSpec and we enable the name service on that
     * same interface. It is up to the person doing the configuration to
     * understand what he or she is trying to do and the impact of choosing those values.
     */
    uint16_t listenPort = StringToU32(argMap["port"]);
    qcc::String interface = "";
    qcc::IPAddress addr;
    if (argMap.find("iface") != argMap.end()) {
        interface = argMap["iface"];
    }
    if (argMap.find("addr") != argMap.end()) {
        addr = IPAddress(argMap["addr"]);
    }

    /* We first determine whether a network interface name or an IP
     * address was specified and then we invoke the appropriate name
     * service method.
     */
    if (!interface.empty()) {
        m_requestedInterfaces[interface] = qcc::IPEndpoint("0.0.0.0", listenPort);
        m_listenPortMap[interface] = listenPort;
    } else if (addr.Size() && addr.IsIPv4()) {
        m_requestedAddresses[addr.ToString()] = "";
        m_requestedAddressPortMap[addr.ToString()] = listenPort;
    }
    if (!interface.empty()) {
        status = IpNameService::Instance().OpenInterface(TRANSPORT_TCP, interface);
    } else if (addr.Size() && addr.IsIPv4()) {
        status = IpNameService::Instance().OpenInterface(TRANSPORT_TCP, addr.ToString());
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("TCPTransport::DoStartListen(): OpenInterface() failed for %s", (interface.empty() ? addr.ToString().c_str() : interface.c_str())));
    }

    return status;
}

void TCPTransport::UntrustedClientExit() {

    /* An untrusted client has exited, update the counts and re-enable the advertisement if necessary. */
    m_listenRequestsLock.Lock();
    m_numUntrustedClients--;
    QCC_DbgPrintf((" TCPTransport::UntrustedClientExit() m_numUntrustedClients=%d m_maxUntrustedClients=%d", m_numUntrustedClients, m_maxUntrustedClients));
    if (!routerName.empty() && (m_numUntrustedClients == (m_maxUntrustedClients - 1))) {
        EnableAdvertisement(routerName, true, TRANSPORT_TCP);
    }
    m_listenRequestsLock.Unlock();


}

QStatus TCPTransport::UntrustedClientStart() {

    /* An untrusted client Establish has finished, so update the counts and disable the advertisement if necessary */
    QStatus status = ER_OK;
    m_listenRequestsLock.Lock();
    m_numUntrustedClients++;
    QCC_DbgPrintf((" TCPTransport::UntrustedClientStart() m_numUntrustedClients=%d m_maxUntrustedClients=%d", m_numUntrustedClients, m_maxUntrustedClients));

    if (m_numUntrustedClients > m_maxUntrustedClients) {
        /* This could happen in the following situation:
         * The max untrusted clients is set to 1. Two untrusted clients try to
         * connect to this daemon at the same time. When the 2nd one
         * finishes the EndpointAuth::Establish, it will call into this method
         * and hit this case and will be rejected.
         */
        status = ER_BUS_NOT_ALLOWED;
        m_numUntrustedClients--;
    }
    if (m_numUntrustedClients >= m_maxUntrustedClients) {
        if (m_numUntrustedClients == m_maxUntrustedClients) {
            QCC_DbgPrintf(("TCPTransport::UntrustedClientStart(): Last available slot is now filled - no more free slots"));
        } else {
            QCC_LogError(ER_BUS_NOT_ALLOWED, ("TCPTransport::UntrustedClientStart(): Disabling routing node advertisements"));
        }
        DisableAdvertisement(routerName, TRANSPORT_TCP);
    }
    m_listenRequestsLock.Unlock();
    return status;
}

QStatus TCPTransport::StopListen(const char* listenSpec)
{
    QCC_DbgPrintf(("TCPTransport::StopListen()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("TCPTransport::StopListen(): Not running or stopping; exiting"));
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
        QCC_LogError(status, ("TCPTransport::StopListen(): Invalid TCP listen spec \"%s\"", listenSpec));
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

void TCPTransport::QueueStopListen(qcc::String& normSpec)
{
    QCC_DbgPrintf(("TCPTransport::QueueStopListen()"));

    /*
     * In order to stop a listen, we send the server accept thread a message
     * containing the STOP_LISTEN_INTANCE request code and the normalized listen
     * spec which specifies the address and port instance to stop listening on.
     */
    ListenRequest listenRequest;
    listenRequest.m_requestOp = STOP_LISTEN_INSTANCE;
    listenRequest.m_requestParam = normSpec;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

}

void TCPTransport::DoStopListen(qcc::String& normSpec)
{
    QCC_DbgPrintf(("TCPTransport::DoStopListen()"));

    /*
     * Since the name service is started before the server accept thread is spun
     * up, and stopped after it is stopped, we must have a started name service or
     * someone isn't playing by the rules; so an assert is appropriate here.
     */
    assert(IpNameService::Instance().Started() && "TCPTransport::DoStopListen(): IpNameService not started");

    /*
     * Find the (single) listen spec and remove it from the list of active FDs
     * used by the server accept loop (run thread).
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

    if (found) {
        if (m_reload != STATE_EXITED) {
            /* If the TCPTransport::Run thread is still running, set m_reload to
             * STATE_RELOADING, unlock the mutex, alert the main Run thread that
             * there is a change and wait for the Run thread to finish any connections
             * it may be accepting and then reload the set of events.
             */


            m_reload = STATE_RELOADING;

            Alert();

            /* Wait until TCPTransport::Run thread has reloaded the
             * set of events or exited.
             */
            while (m_reload == STATE_RELOADING) {
                m_listenFdsLock.Unlock(MUTEX_CONTEXT);
                qcc::Sleep(2);
                m_listenFdsLock.Lock(MUTEX_CONTEXT);
            }
        }
        /*
         * If we took a socketFD off of the list of active FDs, we need to tear it
         * down.
         */
        qcc::SetLinger(stopFd, true, 0);
        qcc::Shutdown(stopFd);
        qcc::Close(stopFd);
    }
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

}

bool TCPTransport::NewDiscoveryOp(DiscoveryOp op, qcc::String namePrefix, bool& isFirst)
{
    QCC_DbgPrintf(("TCPTransport::NewDiscoveryOp()"));

    bool first = false;

    if (op == ENABLE_DISCOVERY) {
        QCC_DbgPrintf(("TCPTransport::NewDiscoveryOp(): Registering discovery of namePrefix \"%s\"", namePrefix.c_str()));
        first = m_discovering.empty();
        if (find(m_discovering.begin(), m_discovering.end(), namePrefix) == m_discovering.end()) {
            m_discovering.push_back(namePrefix);
        }
    } else {
        list<qcc::String>::iterator i = find(m_discovering.begin(), m_discovering.end(), namePrefix);
        if (i == m_discovering.end()) {
            QCC_DbgPrintf(("TCPTransport::NewDiscoveryOp(): Cancel of non-existent namePrefix \"%s\"", namePrefix.c_str()));
        } else {
            QCC_DbgPrintf(("TCPTransport::NewDiscoveryOp(): Unregistering discovery of namePrefix \"%s\"", namePrefix.c_str()));
            m_discovering.erase(i);
        }
    }

    isFirst = first;
    return m_discovering.empty();
}

bool TCPTransport::NewAdvertiseOp(AdvertiseOp op, qcc::String name, bool& isFirst)
{
    QCC_DbgPrintf(("TCPTransport::NewAdvertiseOp()"));

    bool first = false;

    if (op == ENABLE_ADVERTISEMENT) {
        QCC_DbgPrintf(("TCPTransport::NewAdvertiseOp(): Registering advertisement of namePrefix \"%s\"", name.c_str()));
        first = m_advertising.empty();
        if (find(m_advertising.begin(), m_advertising.end(), name) == m_advertising.end()) {
            m_advertising.push_back(name);
        }
    } else {
        list<qcc::String>::iterator i = find(m_advertising.begin(), m_advertising.end(), name);
        if (i == m_advertising.end()) {
            QCC_DbgPrintf(("TCPTransport::NewAdvertiseOp(): Cancel of non-existent name \"%s\"", name.c_str()));
        } else {
            QCC_DbgPrintf(("TCPTransport::NewAdvertiseOp(): Unregistering advertisement of namePrefix \"%s\"", name.c_str()));
            m_advertising.erase(i);
        }
    }

    isFirst = first;
    return m_advertising.empty();
}

bool TCPTransport::NewListenOp(ListenOp op, qcc::String normSpec)
{
    QCC_DbgPrintf(("TCPTransport::NewListenOp()"));

    if (op == START_LISTEN) {
        QCC_DbgPrintf(("TCPTransport::NewListenOp(): Registering listen of normSpec \"%s\"", normSpec.c_str()));
        m_listening.push_back(normSpec);

    } else {
        list<qcc::String>::iterator i = find(m_listening.begin(), m_listening.end(), normSpec);
        if (i == m_listening.end()) {
            QCC_DbgPrintf(("TCPTransport::NewAdvertiseOp(): StopListen of non-existent spec \"%s\"", normSpec.c_str()));
        } else {
            QCC_DbgPrintf(("TCPTransport::NewAdvertiseOp(): StopListen of normSpec \"%s\"", normSpec.c_str()));
            m_listening.erase(i);
        }
    }

    return m_listening.empty();
}

void TCPTransport::EnableDiscovery(const char* namePrefix, TransportMask transports)
{
    QCC_DbgPrintf(("TCPTransport::EnableDiscovery()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("TCPTransport::EnableDiscovery(): Not running or stopping; exiting"));
        return;
    }

    QueueEnableDiscovery(namePrefix, transports);
}

void TCPTransport::QueueEnableDiscovery(const char* namePrefix, TransportMask transports)
{
    QCC_DbgPrintf(("TCPTransport::QueueEnableDiscovery()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = ENABLE_DISCOVERY_INSTANCE;
    listenRequest.m_requestParam = namePrefix;
    listenRequest.m_requestTransportMask = transports;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);
}

void TCPTransport::DisableDiscovery(const char* namePrefix, TransportMask transports)
{
    QCC_DbgPrintf(("TCPTransport::DisableDiscovery()"));
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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("TCPTransport::DisbleDiscovery(): Not running or stopping; exiting"));
        return;
    }

    QueueDisableDiscovery(namePrefix, transports);
}

void TCPTransport::QueueDisableDiscovery(const char* namePrefix,  TransportMask transports)
{
    QCC_DbgPrintf(("TCPTransport::QueueDisableDiscovery()"));
    ListenRequest listenRequest;
    listenRequest.m_requestOp = DISABLE_DISCOVERY_INSTANCE;
    listenRequest.m_requestParam = namePrefix;
    listenRequest.m_requestTransportMask = transports;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

}

QStatus TCPTransport::EnableAdvertisement(const qcc::String& advertiseName, bool quietly, TransportMask transports)
{
    QCC_DbgPrintf(("TCPTransport::EnableAdvertisement()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("TCPTransport::EnableAdvertisement(): Not running or stopping; exiting"));
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    QueueEnableAdvertisement(advertiseName, quietly, transports);
    return ER_OK;
}


void TCPTransport::QueueEnableAdvertisement(const qcc::String& advertiseName, bool quietly, TransportMask transports)
{
    QCC_DbgPrintf(("TCPTransport::QueueEnableAdvertisement()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = ENABLE_ADVERTISEMENT_INSTANCE;
    listenRequest.m_requestParam = advertiseName;
    listenRequest.m_requestParamOpt = quietly;
    listenRequest.m_requestTransportMask = transports;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);
}

void TCPTransport::DisableAdvertisement(const qcc::String& advertiseName, TransportMask transports)
{
    QCC_DbgPrintf(("TCPTransport::DisableAdvertisement()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("TCPTransport::DisableAdvertisement(): Not running or stopping; exiting"));
        return;
    }

    QueueDisableAdvertisement(advertiseName, transports);
}

void TCPTransport::QueueDisableAdvertisement(const qcc::String& advertiseName, TransportMask transports)
{
    QCC_DbgPrintf(("TCPTransport::QueueDisableAdvertisement()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = DISABLE_ADVERTISEMENT_INSTANCE;
    listenRequest.m_requestParam = advertiseName;
    listenRequest.m_requestTransportMask = transports;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

}

void TCPTransport::FoundCallback::Found(const qcc::String& busAddr, const qcc::String& guid,
                                        std::vector<qcc::String>& nameList, uint32_t timer)
{
    QCC_DbgPrintf(("TCPTransport::FoundCallback::Found(): busAddr = \"%s\"", busAddr.c_str()));

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
     * recieve, also via this callback.  Since we are just a go-between, we
     * pretty much just pass what we find on back to the daemon, modulo some
     * filtering to avoid situations we don't yet support:
     *
     * 1. Currently this transport has no clue how to handle anything but
     *    reliable IPv4 endpoints (addr, port), so we filter everything else
     *    out (by removing the unsupported endpoints from the bus address)
     */
    qcc::String addr("addr=");
    qcc::String port("port=");
    qcc::String comma(",");

    /*
     * Find where the addr name starts.
     */
    size_t i = busAddr.find(addr);
    if (i == String::npos) {
        QCC_DbgPrintf(("TCPTransport::FoundCallback::Found(): No addr in busaddr."));
        return;
    }
    i += addr.size();

    /*
     * We assume that the address is always followed by the port so there must
     * be a comma following the address.
     */
    size_t j = busAddr.find(comma, i);
    if (j == String::npos) {
        QCC_DbgPrintf(("TCPTransport::FoundCallback::Found(): No comma after addr in busaddr."));
        return;
    }

    size_t k = busAddr.find(port);
    if (k == String::npos) {
        QCC_DbgPrintf(("TCPTransport::FoundCallback::Found(): No port in busaddr."));
        return;
    }
    k += port.size();

    size_t l = busAddr.find(comma, k);
    if (l == String::npos) {
        l = busAddr.size();
    }

    /*
     * We have the following situation now.  Either:
     *
     *     "addr=192.168.1.1,port=9955,addr=fe80::1260:4bff:fe74:5de9,port=9955"
     *           ^          ^     ^   ^
     *           i          j     k   l = 30
     *
     * or
     *
     *     "addr=192.168.1.1,port=9955"
     *           ^          ^     ^   ^
     *           i          j     k   l = 30
     *
     * So construct a new bus address with only the reliable IPv4 part pulled
     * out.
     */
    qcc::String newBusAddr = String("tcp:") + addr + busAddr.substr(i, j - i) + "," + port + busAddr.substr(k, l - k);

    QCC_DbgPrintf(("TCPTransport::FoundCallback::Found(): newBusAddr = \"%s\".", newBusAddr.c_str()));

    /*
     * Let AllJoyn know that we've found a service(s).
     */
    if (m_listener) {
        QCC_DbgPrintf(("TCPTransport::FoundCallback::Found(): FoundNames(): %s", newBusAddr.c_str()));
        m_listener->FoundNames(newBusAddr, guid, TRANSPORT_TCP, &nameList, timer);
    }
}

void TCPTransport::NetworkEventCallback::Handler(const std::map<qcc::String, qcc::IPAddress>& ifMap)
{
    QCC_DbgPrintf(("TCPTransport::NetworkEventCallback::Handler()"));

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
    if (m_transport.IsRunning() == false || m_transport.m_stopping == true) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("TCPTransport::NetworkEventCallback::Handler(): Not running or stopping; exiting"));
        return;
    }

    m_transport.QueueHandleNetworkEvent(ifMap);
}

void TCPTransport::QueueHandleNetworkEvent(const std::map<qcc::String, qcc::IPAddress>& ifMap)
{
    QCC_DbgPrintf(("TCPTransport::QueueHandleNetworkEvent()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = HANDLE_NETWORK_EVENT;
    listenRequest.ifMap = ifMap;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

}

/* This is the callback handler that is invoked when the name service detects that a network interface
 * has become IFF_UP or a network interface's IP address has changed. When we invoke the OpenInterface()
 * method of the name service from the DoStartListen() method, it will also trigger the name service to
 * refresh the set of interfaces and invoke this callback to get things started. On network events, a map
 * of interfaces to IP addresses that have changed is provided. Whenever OpenInterface() is called, a map
 * of interfaces to IP addresses for all the interfaces that are currently up is provided. Note that this
 * handler may also be invoked because another transport called the OpenInterface() method of the name
 * service and one or more of the interfaces requested by this transport has become IFF_UP.
 */
void TCPTransport::HandleNetworkEventInstance(ListenRequest& listenRequest)
{
    QCC_DbgTrace(("TCPTransport::HandleNetworkEventInstance()"));
    std::map<qcc::String, qcc::IPAddress>& ifMap = listenRequest.ifMap;
    QStatus status = ER_OK;
    list<String> replacedList;
    list<pair<qcc::String, SocketFd> > addedList;
    bool wildcardIfaceRequested = (m_requestedInterfaces.find("*") != m_requestedInterfaces.end());
    bool wildcardAddressRequested = (m_requestedAddresses.find("0.0.0.0") != m_requestedAddresses.end());

    /* If we don't have any interfaces or addresses that we are required to listen on then we return.
     * If a wildcard interface or wildcard address was specified, once we have processed the request
     * we no longer care about dynamic changes to the state of the network interfaces since we are
     * listening on IN_ADDR_ANY at this point so we return.
     */
    if ((m_requestedInterfaces.empty() && m_requestedAddresses.empty()) ||
        ((wildcardIfaceRequested && m_wildcardIfaceProcessed) ||
         (wildcardAddressRequested && m_wildcardAddressProcessed))) {
        return;
    }

    /*
     * We walk through the list of interfaces that have changed in some way provided to us by the name service.
     * For each interface, we check if that interface is one of the interfaces specified in the configuration
     * database. If we don't have a wildcard interface or wildcard address in the configuration database and
     * the current interface's network interface name or IP address is not specified in the configuration
     * database, then we proceed to the next interface in the list. If the current interface's network name
     * is in the configuration database and its IP address has not changed we proceed to the next interface.
     * Similarly, if a network IP address still corresponds to the same network interface we proceed to the
     * next interface in the list. At this point, we check if the change in IP address for a network interface
     * is a change from the default address "0.0.0.0". If it is, then this is the first time we are learning of
     * the actual IP address of the specified interface. If it isn't, then this is a previously known interface
     * that has just changed its IP address. We save a copy of its listen spec so that we can stop listening on
     * the old IP address once we start listening on the new IP address. If we find an address has previously
     * been specified in the configuration database, we remove that entry so that we only listen on the port
     * specified by a network interface name if both network interface and IP address is specified for the same
     * interface. We then update the IP address to the current one for that network interface.  Similarly, if
     * we are learning of a specified IP address's network name for the first time we update our records. If we
     * have a wildcard in the configuration database that we have not yet processed, we process it the first time
     * while walking the list of interfaces and return.
     */
    for (std::map<qcc::String, qcc::IPAddress>::const_iterator it = ifMap.begin(); it != ifMap.end(); it++) {
        qcc::String interface = it->first;
        qcc::IPAddress address = it->second;
        qcc::String addressStr = address.ToString();
        bool currentIfaceRequested = (m_requestedInterfaces.find(interface) != m_requestedInterfaces.end());
        bool currentAddressRequested = (m_requestedAddresses.find(addressStr) != m_requestedAddresses.end());
        if (!wildcardIfaceRequested && !wildcardAddressRequested &&
            !currentIfaceRequested && !currentAddressRequested) {
            continue;
        }

        if (!wildcardIfaceRequested && currentIfaceRequested &&
            m_requestedInterfaces[interface].GetAddress() == address) {
            continue;
        }

        if (!wildcardAddressRequested && currentAddressRequested &&
            m_requestedAddresses[addressStr] == interface) {
            continue;
        }

        if (!wildcardIfaceRequested && currentIfaceRequested) {
            if (m_requestedInterfaces[interface].GetAddress() != qcc::IPAddress("0.0.0.0")) {
                qcc::String replacedSpec = "tcp:addr=" + m_requestedInterfaces[interface].GetAddress().ToString() + ",port=" + U32ToString(m_requestedInterfaces[interface].GetPort());
                if (m_requestedAddresses.find(m_requestedInterfaces[interface].GetAddress().ToString()) != m_requestedAddresses.end()) {
                    m_requestedAddresses.erase(m_requestedInterfaces[interface].GetAddress().ToString());
                }
                replacedList.push_back(replacedSpec);
            }
            m_requestedInterfaces[interface] = qcc::IPEndpoint(address, m_requestedInterfaces[interface].GetPort());
        }

        if (!wildcardAddressRequested && currentAddressRequested) {
            if (!m_requestedAddresses[addressStr].empty()) {
                m_requestedAddresses[addressStr] = interface;
                continue;
            }
            m_requestedAddresses[addressStr] = interface;
        }

        qcc::IPAddress listenAddr;
        uint16_t listenPort = 0;
        if (wildcardIfaceRequested) {
            listenAddr = qcc::IPAddress("0.0.0.0");
            listenPort = m_requestedInterfaces["*"].GetPort();
        } else if (wildcardAddressRequested) {
            listenAddr = qcc::IPAddress("0.0.0.0");
            listenPort = m_requestedAddressPortMap["0.0.0.0"];
        } else {
            if (!listenAddr.Size() && currentIfaceRequested) {
                listenAddr = m_requestedInterfaces[interface].GetAddress();
                listenPort = m_requestedInterfaces[interface].GetPort();
            } else if (!listenAddr.Size() && currentAddressRequested) {
                listenAddr = address;
                listenPort = m_requestedAddressPortMap[addressStr];
            } else {
                continue;
            }
        }
        if (!listenAddr.Size() || !listenAddr.IsIPv4()) {
            continue;
        }
        bool ephemeralPort = (listenPort == 0);
        /*
         * We have the name service work out of the way, so we can now create the
         * TCP listener sockets and set SO_REUSEADDR/SO_REUSEPORT so we don't have
         * to wait for four minutes to relaunch the daemon if it crashes.
         */
        SocketFd listenFd = INVALID_SOCKET_FD;
        status = Socket(QCC_AF_INET, QCC_SOCK_STREAM, listenFd);
        if (status != ER_OK) {
            continue;
        }

        /*
         * Set the SO_REUSEADDR socket option so we don't have to wait for four
         * minutes while the endpoint is in TIME_WAIT if we crash (or control-C).
         */
        status = qcc::SetReuseAddress(listenFd, true);
        if (status != ER_OK && status != ER_NOT_IMPLEMENTED) {
            qcc::Close(listenFd);
            continue;
        }

        /*
         * We call accept in a loop so we need the listenFd to be non-blocking.
         */
        status = qcc::SetBlocking(listenFd, false);
        if (status != ER_OK) {
            qcc::Close(listenFd);
            continue;
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
            }
            if (wildcardIfaceRequested) {
                m_requestedInterfaces["*"] = qcc::IPEndpoint("0.0.0.0", listenPort);
            } else if (wildcardAddressRequested) {
                m_requestedAddressPortMap["0.0.0.0"] = listenPort;
            } else {
                if (currentIfaceRequested) {
                    m_requestedInterfaces[interface] = qcc::IPEndpoint(m_requestedInterfaces[interface].GetAddress().ToString(), listenPort);
                } else if (currentAddressRequested) {
                    m_requestedAddressPortMap[addressStr] = listenPort;
                }
            }
            qcc::String normSpec = "tcp:addr=" + listenAddr.ToString() + ",port=" + U32ToString(listenPort);

            status = qcc::Listen(listenFd, MAX_LISTEN_CONNECTIONS);
            /* We make a list of the new listen specs on which we are listening so
             * that we can add those to the m_listenFds when we're done procesing
             * the list of network interfaces.
             */
            if (status == ER_OK) {
                addedList.push_back(pair<qcc::String, SocketFd>(normSpec, listenFd));
            } else {
                QCC_LogError(status, ("TCPTransport::HandleNetworkEventInstance(): Listen failed"));
            }
        } else {
            QCC_LogError(status, ("TCPTransport::HandleNetworkEventInstance(): Failed to bind to %s/%d", listenAddr.ToString().c_str(), listenPort));
        }

        /* We update the map of interface names to port numbers
         * here to account for ephemeral ports since only at
         * this point do we know the actual ephemeral port number
         * after we call Bind() and are actually listening.
         */
        if (wildcardIfaceRequested) {
            m_listenPortMap["*"] = listenPort;
        } else if (wildcardAddressRequested) {
            m_listenPortMap["0.0.0.0"] = listenPort;
        } else {
            if (currentIfaceRequested) {
                m_listenPortMap[interface] = listenPort;
            } else if (currentAddressRequested) {
                m_listenPortMap[addressStr] = listenPort;
            }
        }

        /*
         * The IP name service is very flexible about what to advertise.  It assumes
         * that a so-called transport is going to be doing the advertising.  An IP
         * transport, by definition, has a reliable data transmission capability and
         * an unreliable data transmission capability.  In the IP world, reliable
         * data is sent using TCP and unreliable data is sent using UDP (the Packet
         * Engine in the AllJoyn world).  Also, IP implies either IPv4 or IPv6
         * addressing.
         *
         * In the TCPTransport, we only support reliable data transfer over IPv4
         * addresses, so we leave all of the other possibilities turned off (provide
         * a zero port).  Remember the port we enabled so we can re-enable the name
         * service if listeners come and go.
         */
        IpNameService::Instance().Enable(TRANSPORT_TCP, m_listenPortMap, 0, std::map<qcc::String, uint16_t>(), 0, true, false, false, false);

        /*
         * There is a special case in which we respond to embedded AllJoyn bus
         * attachements actively looking for daemons to connect to.  We don't want
         * do blindly do this all the time so we can pass the Android Compatibility
         * Test, so we crank up an advertisement when we do the start listen (which
         * is why we bother to do all of the serialization of DoStartListen work
         * anyway).  We make this a configurable advertisement so users of bundled
         * daemons can change the advertisement and know they are connecting to
         * "their" daemons if desired.
         *
         * We pull the advertisement prefix out of the configuration and if it is
         * there, we append the short GUID of the daemon to make it unique and then
         * advertise it quietly via the IP name service.  The quietly option means
         * that we do not send gratuitous is-at (advertisements) of the name, but we
         * do respond to who-has requests on the name.
         */
        if (!routerName.empty() && (m_numUntrustedClients < m_maxUntrustedClients)) {
            bool isFirst;
            NewAdvertiseOp(ENABLE_ADVERTISEMENT, routerName, isFirst);
            QStatus status = IpNameService::Instance().AdvertiseName(TRANSPORT_TCP, routerName, true, TRANSPORT_TCP);
            if (status != ER_OK) {
                QCC_LogError(status, ("TCPTransport::HandleNetworkEventInstance(): Failed to AdvertiseNameQuietly \"%s\"", routerName.c_str()));
            }
            m_isAdvertising = true;
        }
        m_isListening = true;
        m_isNsEnabled = true;

        /* If we have a wildcard specified in the configuration database, we want to stop
         * listening on all the non-wildcard addresses/ports we may have previously opened
         * and so we add all of those listen specs to the replaced list so we can stop
         * listening on those addresses/ports and just listen on IN_ADDR_ANY alone. We
         * also ensure that our listenPortMap only has a wildcard entry.
         */
        if (wildcardIfaceRequested) {
            m_wildcardIfaceProcessed = true;
            for (std::map<qcc::String, qcc::IPEndpoint>::const_iterator iter = m_requestedInterfaces.begin(); iter != m_requestedInterfaces.end(); iter++) {
                if (iter->first != "*" && iter->second.GetAddress() != qcc::IPAddress("0.0.0.0")) {
                    qcc::String replacedSpec = "tcp:addr=" + m_requestedInterfaces[iter->first].GetAddress().ToString() + ",port=" + U32ToString(m_requestedInterfaces[iter->first].GetPort());
                    m_listenPortMap.erase(iter->first);
                    replacedList.push_back(replacedSpec);
                }
            }
            m_requestedInterfaces.clear();
            m_requestedAddresses.clear();
            m_requestedAddressPortMap.clear();
            m_requestedInterfaces["*"] = qcc::IPEndpoint("0.0.0.0", listenPort);
            break;
        } else if (wildcardAddressRequested) {
            m_wildcardAddressProcessed = true;
            for (std::map<qcc::String, qcc::String>::const_iterator iter = m_requestedAddresses.begin(); iter != m_requestedAddresses.end(); iter++) {
                if (iter->first != "0.0.0.0" && !iter->second.empty()) {
                    qcc::String replacedSpec = "tcp:addr=" + iter->first + ",port=" + U32ToString(m_listenPortMap[iter->first]);
                    m_listenPortMap.erase(iter->first);
                    replacedList.push_back(replacedSpec);
                }
            }
            m_requestedAddresses.clear();
            m_requestedAddressPortMap.clear();
            m_requestedAddresses["0.0.0.0"] = "*";
            break;
        }
    }

    /* We add the listen specs to the m_listenFds at this point */
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    if (!addedList.empty()) {
        for (list<pair<qcc::String, SocketFd> >::iterator it = addedList.begin(); it != addedList.end(); it++) {
            m_listenFds.push_back(*it);
        }
    }

    /*
     * Signal the (probably) waiting run thread so it will wake up and add this
     * new socket to its list of sockets it is waiting for connections on.
     */
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);
    Alert();

    /*
     * We stop listening on all the listen specs that were replaced during the processing.
     * These listen specs usually represent the old IP addresses that are no longer in use.
     * In addition, if the last advertisement or discovery request was cancelled before
     * the relevant network interfaces became IFF_UP, we also stop listening.
     */
    if (m_advertising.empty() && m_discovering.empty()) {
        for (list<pair<qcc::String, SocketFd> >::iterator it = addedList.begin(); it != addedList.end(); it++) {
            replacedList.push_back(it->first);
        }
        IpNameService::Instance().Enable(TRANSPORT_TCP, m_listenPortMap, 0, std::map<qcc::String, uint16_t>(), 0, false, false, false, false);
        m_isListening = false;
        m_isNsEnabled = false;
        m_listenPortMap.clear();
        m_pendingDiscoveries.clear();
        m_pendingAdvertisements.clear();
        m_wildcardIfaceProcessed = false;
    }
    for (list<String>::iterator it = replacedList.begin(); it != replacedList.end(); it++) {
        DoStopListen(*it);
    }

    /*
     * If there were pending advertisements that came in before the network interfaces
     * became IFF_UP, we enable those pending advertisements.
     */
    for (list<ListenRequest>::iterator it = m_pendingAdvertisements.begin(); it != m_pendingAdvertisements.end(); it++) {
        EnableAdvertisementInstance(*it);
    }
    m_pendingAdvertisements.clear();

    /*
     * If there were pending discoveries that came in before the network interfaces
     * became IFF_UP, we enable those pending discoveries.
     */
    for (list<ListenRequest>::iterator it = m_pendingDiscoveries.begin(); it != m_pendingDiscoveries.end(); it++) {
        EnableDiscoveryInstance(*it);
    }
    m_pendingDiscoveries.clear();
}

void TCPTransport::CheckEndpointLocalMachine(TCPEndpoint endpoint)
{
#ifdef QCC_OS_GROUP_WINDOWS
    String ipAddrStr;
    endpoint->GetRemoteIp(ipAddrStr);

    std::vector<IfConfigEntry> entries;
    IfConfig(entries);

    for (uint32_t i = 0; i < entries.size(); i++) {
        if (ipAddrStr == entries[i].m_addr) {
            endpoint->SetGroupId(GetUsersGid(DESKTOP_APPLICATION));
            break;
        }
    }
#endif
}

} // namespace ajn
