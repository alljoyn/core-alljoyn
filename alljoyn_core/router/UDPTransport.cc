/**
 * @file
 * UDPTransport is an implementation of UDPTransportBase for daemons.
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
#include <qcc/Thread.h>
#include <qcc/Condition.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/IfConfig.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Session.h>
#include <alljoyn/TransportMask.h>

#include <BusUtil.h>
#include "BusInternal.h"
#include "BusController.h"
#include "ConfigDB.h"
#include "RemoteEndpoint.h"
#include "Router.h"
#include "DaemonRouter.h"

#include "ArdpProtocol.h"
#include "ns/IpNameService.h"
#include "UDPTransport.h"

#if ARDP_TESTHOOKS
#include "ScatterGatherList.h"
#endif

/*
 * How the transport fits into the system
 * ======================================
 *
 * AllJoyn provides the concept of a Transport which provides a relatively
 * abstract way for the daemon to use different network mechanisms for getting
 * Messages from place to another.  Conceptually, think of, for example, a Unix
 * transport that moves bits using unix domain sockets or a TCP transport that
 * moves Messages over a TCP connection.  A UDP transport moves Messages over
 * UDP datagrams using a reliability layer.
 *
 * BSD sockets is oriented toward clients and servers.  There are different
 * sockets calls required for a program implementing a server-side part and a
 * client side part.  The server-side listens for incoming connection requests
 * and the client-side initiates the requests.  AllJoyn clients are bus
 * attachments that our Applications may use and these can only initiate
 * connection requests to AllJoyn daemons.  Although dameons may at first blush
 * appear as the service side of a typical BSD sockets client-server pair, it
 * turns out that while daemons obviously must listen for incoming connections,
 * they also must be able to initiate connection requests to other daemons.
 * This explains the presence of both connect-like methods and listen-like
 * methods here.
 *
 * A fundamental idiom in the AllJoyn system is that of a thread.  Active
 * objects in the system that have threads wandering through them will implement
 * Start(), Stop() and Join() methods.  These methods work together to manage
 * the autonomous activities that can happen in a UDPTransport.  These
 * activities are carried out by so-called hardware threads.  POSIX defines
 * functions used to control hardware threads, which it calls pthreads.  Many
 * threading packages use similar constructs.
 *
 * In a threading package, a start method asks the underlying system to arrange
 * for the start of thread execution.  Threads are not necessarily running when
 * the start method returns, but they are being *started*.  Some time later, a
 * thread of execution appears in a thread run function, at which point the
 * thread is considered *running*.  In the case of the UDPTransport, the Start()
 * method spins up a thread to run the basic maintenance operations such as
 * deciding when to listen and advertise.  Another thread(s) is started to deal
 * with handling callbacks for deadlock avoidance.  The AllJoyn daemon is a
 * fundamentally multithreaded environemnt, so multiple threads may be trying to
 * connect, disconnect, and write from the daemon side, and at the same time
 * connect, disconnect, read and write callbacks may be coming from the network
 * side.  This means that as soon as UDPTransport::Start() is executed, multiple
 * threads, originating both in the transport and from outside, may be wandering
 * around in objects used by the tansport; and so one must be very careful about
 * resource management.  This is the source of much of the complexity in this
 * module.
 *
 * In generic threads packages, executing a stop method asks the underlying
 * system to arrange for a thread to end its execution.  The system typically
 * sends a message to the thread to ask it to stop doing what it is doing.  The
 * thread is running until it responds to the stop message, at which time the
 * run method exits and the thread is considered *stopping*.  The UDPTransport
 * provides a Stop() method to do exactly that.  Note that neither of Start()
 * nor Stop() are synchronous in the sense that one has actually accomplished
 * the desired effect upon the return from a call.  Of particular interest is
 * the fact that after a call to Stop(), threads will still be *running* for
 * some non-deterministic time.  In order to wait until all of the threads have
 * actually stopped, a blocking call is required.  In threading packages this is
 * typically called join, and our corresponding method is called Join().  A user
 * of the UDPTransport must assume that immediately after a call to Start() is
 * begun, and until a call to Join() returns, there may be threads of execution
 * wandering anywhere in the transport and in any callback registered by the
 * caller.  The same model applies to connection endpoints (_UDPEndpoint)
 * instances.  Further complicating _UDPEndpoint design is that the thread
 * lifetime methods may be called repeatedly or never (in the case of some forms
 * of timeout); and so the transport needs to ensure that all combinations of
 * these state transitions occur in an orderly and deterministic manner.
 *
 * The high-level process regarding how an advertisement translates into a
 * transport Connect() is a bit opaque, so we paint a high-level picture here.
 * First, a service (that will be *handling* RPC calls and *emitting* signals)
 * acquires a name on the bus, binds a session port and calls AdvertiseName.
 * This filters down (possibly through language bindings) to the AllJoyn Object.
 * The AllJoynObj essentially turns a DBus into an AllJoyn bus.  The AllJoyn
 * Object consults the transports on the transport list (the UDP transport is
 * one of those) and eventually sends an advertisement request to each specified
 * transport by calling each transport's EnableAdvertisement() method.  We
 * transnslate this call to a call to the the IpNameService::AdvertiseName()
 * method we call since we are an IP-based transport.  The IP name service will
 * multicast the advertisements to other daemons listening on our device's
 * connected networks.
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
 * find which have the associated services.  When a new advertisement is
 * received, the name service fires a callback into the transport, and it, in turn, calls
 * into its associated BusListener to pass the information back to the daemon.
 *
 * The callback includes information about the discovered name, the IP address,
 * port and daemon GUID of the remote daemon (now Routing Node).  This bus
 * address is "hidden" from interested clients and replaced with a more generic
 * name and TransportMask bit (for us it will be TRANSPORT_UDP).  The client
 * either responds by (1) ignoring the advertisement; (2) waiting to accumulate
 * more answers to see what the options are; or (3) joins a session to the
 * implied daemon/service.  A reference to a SessionOpts object is provided as a
 * parameter to a JoinSession call if the client wants to connect.  This
 * SessionOpts reference is passed down into the transport (selected by the
 * TransportMask) into the Connect() method which is used to establish the
 * connection and can be used to determine if the discovered name posesses
 * certain desired characteristics (to aid in determine the course of action
 * of the client
 *
 * There are four basic connection mechanisms that are described by the options.
 * These can be viewed as a matrix;
 *
 *                                                      IPv4               IPv6
 *                                                 ---------------    ---------------
 *     TRAFFIC MESSAGES | TRAFFIC_RAW_RELIABLE  |   Reliable IPv4      Reliable IPv6
 *     TRAFFIC_RAW_UNRELIABLE                   |  Unreliable IPv4    Unreliable IPv6
 *
 * Note that although the UDP protocol is unreliable, the AllJoyn Reliable Datagram
 * Protocol is an additional reliability layer, so that TRAFFIC_MESSAGES are actually
 * sent over the UDP protocol.

 * The bits in the provided SessionOpts select the row, but the column is left
 * free (unspecified).  This means that it is up to the transport to figure out
 * which one to use.  Clearly, if only one of the two address flavors is
 * possible (known from examining the returned bus address which is called a
 * connect spec in the Connect() method) the transport should choose that one.
 * If both IPv4 or IPv6 are available, it is up to the transport (again, us) to
 * choose the "best" method since we don't bother clients with that level of
 * detail.
 *
 * Perhaps somewhat counter-intuitively, advertisements relating to the
 * UDP Transport use the addr (unreliable IPv4 or IPv6 address), and port
 * unreliable IPv4 or IPv6 port).
 * At the same time, the UDP Transport tells clients of the transport that it
 * supports TRAFFIC MESSAGES only.  This is because the underlying network
 * protocol used is UDP which is inherently unreliable.  We provide a
 * reliability layer to translate the unreliable UDP4 and UDP6 datagrams into
 * reliable AllJoyn messages.  The UDP Transpot does not provide RAW sockets
 * which is a deprecated traffic type.
 *
 * Internals
 * =========
 *
 * We spend a lot of time on the threading aspects of the transport since they
 * are often the hardest part to get right and are complicated.  This is where
 * the bugs live.
 *
 * As mentioned above, the AllJoyn system uses the concept of a Transport.  You
 * are looking at the UDPTransport.  Each transport also has the concept of an
 * Endpoint.  The most important function fo an endpoint is to provide (usually)
 * non-blocking semantics to higher level code.  If the source thread overruns
 * the ability of the transport to move bits (reliably), we must apply
 * back-pressure by blocking the calling thread, but usually a call to PushBytes
 * results in an immediate UDP datagram sendto.  In the UDP transport there are
 * separate worker threads assigned to reading UDP datagrams, running the
 * reliability layer and dispatching received AllJoyn messages.
 *
 * Endpoints are specialized into the LocalEndpoint and the RemoteEndpoint
 * classes.  LocalEndpoint represents a connection from a router to the local
 * bus attachment or daemon (within the "current" process).  A RemoteEndpoint
 * represents a connection from a router to a remote attachment or daemon.  By
 * definition, the UDPTransport provides RemoteEndpoint functionality.
 *
 * RemoteEndpoints are further specialized according to the flavor of the
 * corresponding transport, and so you will see a UDPEndpoint class defined
 * below which provides functionality to send messages from the local router to
 * a destination off of the local process using a UDP transport mechanism.
 *
 * RemoteEndpoints use AllJoyn stream objects to actually move bits.  In UDP
 * this is a bit of an oxymoron, however an AllJoyn stream is a thin layer on
 * top of a Socket (which is another thin layer on top of a BSD socket) that
 * provides a PushBytes() method.  Although UDP is not a stream-based protocol,
 * we treat each received datagram as a separate stream for the purposes of
 * passing back to the AllJoyn core which expectes to be able to read bytes from
 * a message backing object.
 *
 * Unlike a TCP transport, there are no dedicated receive threads.  Receive
 * operations in UDP are not associted with a particular endpoint at all, other
 * than using the required endpoint as a convencient place holder for a
 * connection data structure.  The UDP Transport operates more in an
 * Asynchronous IO-like fashion.  Received datagrams appear out of the ARDP
 * protocol as callbacks and are sent into a callback dispatcher thread.  Once
 * the dispatcher has an inbound datagram(s) it reassembles and unmarshals the
 * datagrams into an AllJoyn Message.  It then calls into the daemon
 * (PushMessage) to arrange for delivery.  A separate thread runs the
 * maintenance aspects of the UDP reliability layer (to drive retransmissions,
 * timeouts, etc.) and the endpoint management code (to drive the lifetime state
 * transitions of endpoints).
 *
 * The UDPEndpoint inherits some infrastructure from the more generic
 * RemoteEndpoint class.  Since the UDP transport is a not a stream-based
 * protocol, it does redefine some of the basic operation of the RemoteEndpoint
 * to suit its needs.  The RemoteEndpoint is also somewhat bound to the concept
 * of stream and receive thread, so we have to jump through some hoops to
 * coexist.
 *
 * The UDP endpoint does not use SASL for authentication and implements required
 * dameon exchanges in the SYN, SYN + ACK echanges of the underlying ARDP
 * protocol.  Although there is no authentication, per se, we still call this
 * handshake phase authentication since the BusHello is part of the
 * authentication phase of the TCP Transport.  Authentication can, of course,
 * succeed or fail based on timely interaction between the two sides, but it can
 * also be abused in a denial of service attack.  If a client simply starts the
 * process but never responds, it could tie up a daemon's resources, and
 * coordinated action could bring down a daemon.  Because of this, we provide a
 * way to reach in and abort authentications that are "taking too long" and free
 * the associated resources.
 *
 * As described above, a daemon can listen for inbound connections and it can
 * initiate connections to remote daemons.  Authentication must happen in both
 * cases and so we need to worry about denial of service in both directions and
 * recover gracefully.
 *
 * When the daemon is brought up, its TransportList is Start()ed.  The transport
 * specs string (e.g., "unix:abstract=alljoyn;udp:;tcp:") is provided to
 * TransportList::Start() as a parameter.  The transport specs string is parsed
 * and in the example above, results in "unix" transports, "tcp" transports and
 * "udp" transports being instantiated and started.  As mentioned previously
 * "udp:" in the daemon translates into UDPTransport.  Once the desired
 * transports are instantiated, each is Start()ed in turn.  In the case of the
 * UDPTransport, this will start the maintenance loop.  Initially there are no
 * sockets to listen on.
 *
 * The daemon then needs to start listening on inbound addresses and ports.
 * This is done by the StartListen() command.  This also takes the same kind of
 * server args string shown above but this time the address and port information
 * are used.  For example, one might use the string
 * "udp:addr=0.0.0.0,port=9955;" to specify which address and port to listen
 * to.  This Bus::StartListen() call is translated into a transport
 * StartListen() call which is provided with the string described above, which
 * we call a "listen spec".  Our UDPTransport::StartListen() will arange to
 * create a Socket, bind the socket to the address and port provided and save
 * the new socket on a list of "listenFds" (we may listen on separate sockets
 * corresponding to multiple network interfaces).  Another of the many
 * complications we have to deal with is that the Android Compatibility Test
 * Suite (CTS) requires that an idle phone not have any sockets listening for
 * inbound data.  In order to pass the CTS in the case of the pre-installed
 * daemon, we must only have open name service sockets when actively advertising
 * or discovering.  This implies that we need to track the adveritsement state
 * and enable or disable the name service depending on that state.
 *
 * An inbound connection request in the UDP transport is consists of receiving a
 * SYN datagram.  The AcceptCb() is called from the reliability layer (on
 * reception of a SYN packet) in order to ask whether or not the connection
 * should be accepted.  If AcceptCb() determines there are enough resources for
 * a new connection it will call ARDP_Accept to provide a BusHello reply and
 * return true indicating acceptance, or false which means rejection.  If the
 * connection is accepted, a ConnectCb() is fired and the callback dispatcher
 * thread will ultimately handle the incoming request and create a UDPEndpoint
 * for the *proposed* new connection.
 *
 * Recall that an endpoint is not brought up immediately, but an authentication
 * step must be performed.  The required information (BusHello reply) is
 * provided back in the SYN + ACK packet.  The final ACK of the three-way
 * handshake completes the inbound connection establishment process.
 * If the authentication takes "too long" we assume that a denial of service
 * attack in in progress.  We fail such partial connections and the endpoint
 * management code removes them.
 *
 * A daemon transport can accept incoming connections, and it can make outgoing
 * connections to another daemon.  This case is simpler than the accept case
 * since it is expected that a socket connect can block higner level code, so it
 * is possible to do authentication in the context of the thread calling
 * Connect().  Connect() is provided a so-called "connect spec" which provides
 * an IP address ("addr=xxxx"), port ("port=yyyy") in a String.  A check is
 * always made to catch an attempt for the daemon to connect to itself which is
 * a system-defined error (it causes the daemon grief, so we avoid it here by
 * looking to see if one of the listenFds is listening on an interface that
 * corresponds to the address in the connect spec).  If the connect is allowed,
 * ee kick off a process in the underlying UDP reliability layer that
 * corresponds to the 3-way handshake of TCP.
 *
 * Shutting the UDPTransport down involves orchestrating the orderly termination
 * of:
 *
 *   1) Threads that may be running in the maintenance loop with associated Events
 *      and their dependent socketFds stored in the listenFds list;
 *   3) The callback dispatcher thread that may be out wandering around in the
 *      daemon doing its work;
 *   2) Threads that may be running around in endpoints and streams trying to write
 *      Mesages to the network.
 *
 * We have to be careful to follow the AllJoyn threading model transitions in
 * both the UDPTransport and all of its associated _UdpEndpoints.  There are
 * reference counts of endpoints to be respected as well.  In order to ensure
 * orderly termination of endoints and deterministic disposition of threads
 * which may be executing in those endpoints, We want the last reference count
 * held on an endpoint to be the one held by the transport.  There is much
 * work (see IncrementAndFetch, DecrementAndFetch, ManagedObj for example)
 * done to ensure this outcome.
 *
 * There are a lot of very carefully managed relationships here, so be careful
 * when making changes to the thread and resource management aspects of any
 * transport.  Taking lock order lightly is a recipe for disaster.  Always
 * consider what locks are taken where and in what order.  It's quite easy to
 * shoot yourself in multiple feet you never knew you had if you make an unwise
 * modification, and this can sometimes result in tiny little time-bombs set to
 * go off in seemingly completely unrelated code.
 *
 * A note on connection establishment
 * ==================================
 *
 * In the TCP transport, a separate synchronous sequence is executed before
 * AllJoyn messages can begin flowing.  First a NUL byte is sent as is required
 * in the DBus spec.  In order to get a destination address for the BusHello
 * message, the local side relies on the SASL three-way handshake exchange:
 *
 *     SYN ------------>
 *                       <- SYN + ACK
 *     ACK ------------>
 *     NUL ------------>
 *     AUTH ANONYMOUS ->
 *                       <- OK <GUID>
 *     BEGIN ---------->
 *
 * Once this is done, the active connector sends a BusHello Message and the
 * passive side sends a response
 *
 *     BusHello ------->
 *                       <- BusHello reply
 *
 * In the UDP Transport, we get rid of basically the whole Authentication
 * process and exchange required information in the SYN, SYN + ACK and
 * ACK packets of the protocol three-way handshake.
 *
 * The initial ARDP SYN packet *implies* AUTH_ANONYMOUS and contains the
 * BusHello message data from the Local (initiating/active) side of the
 * connection.  The SYN + ACK segment in response from the remote side contains
 * the response to the BusHello that was sent in the SYN packet.
 *
 *     SYN + BusHello -->
 *                        <- SYN + ACK + BusHello Reply
 *     ACK ------------->
 *
 * This all happens in a TCP-like SYN, SYN + ACK, ACK exchange with AllJoyn
 * data.  At the reception of the final ACK, the connection is up and running.
 *
 * This exchange is implemented using a number of callback functions that
 * fire on the local (active) and remote (passive) side of the connection.
 *
 * 1) The actively connecting side provides a BusHello message in call to
 *    ARDP_Connect().  As described above, ARDP provides this message as data in
 *    the SYN segment which is the first part of the three-way handshake;
 *
 * 2) When the passive side receives the SYN segment, its AcceptCb() callback is
 *    fired.  The data provided in the accept callback contains the BusHello
 *    message from the actively opening side.  The passive side, if it chooses
 *    to accept the connection, makes a call to ARDP_Accept() with its reply to
 *    the BusHello from the active side as data.  ARDP provides this data back
 *    in the SYN + ACK segment as the second part of its three-way handshake;
 *
 * 3) The actively connecting side receives a ConnectCb() callback as a result
 *    of the SYN + ACK coming back from the passive side.  This indicates that
 *    the newly established connection is going into the OPEN state from the
 *    local side's (ARDP) perspective.  Prior to firing the callback, ARDP
 *    automatically sends the final ACK and completes the three-way handshake.
 *    The ConectCb() with the active indication means that a SYN + ACK has been
 *    received that includes the reply to the original BusHello message.
 *
 * 4) When the final ACK of the three-way handshake is delivered to the passive
 *    opener side, it transitions the passive side to the OPEN state and fires
 *    a ConnectCb() callback with the passive indication meaning that the final
 *    ACK of the three-way handhake has arrived.
 *
 * From the perspective of the UDP Transport, this translates into the following
 * sequence diagram that reflects the three-way handshake that is going on under
 * the whole thing.
 *
 *                  Active Side                          Passive Side
 *                  ===========                          ============
 *      ARDP_Connect([out]BusHello message) --> AcceptCb([in]BusHello message) -----+
 *                                                                                  |
 * +--- ConnectCb([in]BusHello reply) <-------- ARDP_Accept([out]BusHello reply) <--+
 * |
 * +------------------------------------------> ConnectCb(NULL)
 *
 */

#define QCC_MODULE "UDP"

#define SENT_SANITY 0   /**< If non-zero make sure ARDP is returning buffers correctly (expensive) */
#define BYTEDUMPS 0     /**< If non-zero do byte-by-byte debug dumps of sent and received buffers (super-expensive) */
#define RETURN_ORPHAN_BUFS 0 /**< If non-zero, call ARDP_RecvReady on any buffers we can't forward (processed after endpoint is torn down) */

using namespace std;
using namespace qcc;

const uint32_t UDP_STALL_REPORT_INTERVAL = 10000; /** Minimum time between stall warning log messages */

const uint32_t UDP_ENDPOINT_MANAGEMENT_TIMER = 1000;  /** Minimum time between calls to ManageEndpoints -- a watchdog */
const uint32_t UDP_WATCHDOG_TIMEOUT = 30000;  /**< How long to wait before printing an error if an endpoint is not going away */
const uint32_t UDP_MESSAGE_PUMP_TIMEOUT = 10000;  /**< How long to keep a message pump thread running with nothing to do */

const uint32_t UDP_CONNECT_TIMEOUT = 1000;  /**< How long before we expect a connection to complete */
const uint32_t UDP_CONNECT_RETRIES = 10;  /**< How many times do we retry a connection before giving up */
const uint32_t UDP_INITIAL_DATA_TIMEOUT = 1000;  /**< Initial value for how long do we wait before retrying sending data */
const uint32_t UDP_TOTAL_DATA_RETRY_TIMEOUT = 30000;  /**< Initial total amount of time to try and send data before giving up */
const uint32_t UDP_MIN_DATA_RETRIES = 5;  /**< Minimum number of times to try and send data before giving up */
const uint32_t UDP_PERSIST_INTERVAL = 1000;  /**< How long do we wait before pinging the other side due to a zero window */
const uint32_t UDP_TOTAL_APP_TIMEOUT = 30000;  /**< How long to we try to ping for window opening before deciding app is not pulling data */
const uint32_t UDP_LINK_TIMEOUT = 30000;  /**< How long before we decide a link is down (with no reponses to keepalive probes */
const uint32_t UDP_KEEPALIVE_RETRIES = 5;  /**< How many times do we try to probe on an idle link before terminating the connection */
const uint32_t UDP_FAST_RETRANSMIT_ACK_COUNTER = 1;  /**< How many duplicate acknowledgements to we need to trigger a data retransmission */
const uint32_t UDP_DELAYED_ACK_TIMEOUT = 100; /**< How long do we wait until acknowledging received segments */
const uint32_t UDP_TIMEWAIT = 1000;  /**< How long do we stay in TIMWAIT state before releasing the per-connection resources */

/*
 * Note that UDP_SEGBMAX * UDP_SEGMAX must be greater than or equal to
 * ALLJOYN_MAX_PACKET_LEN (135168) to ensure we can deliver a maximally sized AllJoyn
 * message.
 */
const uint32_t UDP_SEGBMAX = 4440;  /**< Maximum size of an ARDP segment (quantum of reliable transmission) */
const uint32_t UDP_SEGMAX = 93;  /**< Maximum number of ARDP segment in-flight (bandwidth-delay product sizing) */

namespace ajn {

/**
 * Name of transport used in transport specs.
 */
const char* UDPTransport::TransportName = "udp";

/**
 * Default router advertisement prefix.  Currently Thin Library devices cannot
 * connect to routing nodes over UDP.
 */
#if ADVERTISE_ROUTER_OVER_UDP
const char* const UDPTransport::ALLJOYN_DEFAULT_ROUTER_ADVERTISEMENT_PREFIX = "org.alljoyn.BusNode.";
#endif

/**
 * Prefix to quietly advertise when working around ASACORE-1298.  Keeps an
 * advertisement outstanding to prevent closing listen sockets out from under
 * UDP Endpoints.
 */
#if WORKAROUND_1298
const char* const WORKAROUND_1298_PREFIX = "org.alljoyn.wa1298.";
#endif

const char* TestConnStr = "ARDP TEST CONNECT REQUEST";
const char* TestAcceptStr = "ARDP TEST ACCEPT";

#ifndef NDEBUG
#if BYTEDUMPS
static void DumpLine(uint8_t* buf, uint32_t len, uint32_t width)
{
    for (uint32_t i = 0; i < width; ++i) {
        if (i > len) {
            printf("   ");
        } else {
            printf("%02x ", buf[i]);
        }
    }
    printf(": ");
    for (uint32_t i = 0; i < len && i < width; ++i) {
        if (iscntrl(buf[i]) || !isascii(buf[i])) {
            printf(".");
        } else {
            printf("%c", buf[i]);
        }
    }
    printf("\n");
}

static void DumpBytes(uint8_t* buf, uint32_t len)
{
    if (_QCC_DbgPrintCheck(DBG_GEN_MESSAGE, QCC_MODULE)) {
        for (uint32_t i = 0; i < len; i += 16) {
            DumpLine(buf + i, len - i > 16 ? 16 : len - i, 16);
        }
    }
}
#endif // BYTEDUMPS
#endif // NDEBUG

#ifndef NDEBUG
void qdtm(void)
{
    printf("******** %u: ", GetTimestamp());
}

/*
 * Quick Debug Print for focused printf debugging outside QCC framework.  Useful
 * if you want to get your bearings but don't want to see the entire debug spew.
 * Use just like QCC_DebugPrintf.
 */
#define QDP(x)        \
    do {              \
        qdtm();       \
        printf x;     \
        printf("\n"); \
    } while (0)
#else
#define QDP(x) do { } while (0)
#endif

#ifndef NDEBUG
#define SEAL_SIZE 4

static void SealBuffer(uint8_t* p)
{
    *p++ = 'S';
    *p++ = 'E';
    *p++ = 'A';
    *p++ = 'L';
}

static void CheckSeal(uint8_t* p)
{
    assert(*p++ == 'S' && *p++ == 'E' && *p++ == 'A' && *p++ == 'L' && "CheckSeal(): Seal blown");
}
#endif

class _UDPEndpoint;

static bool IsUdpEpStarted(_UDPEndpoint* ep);
static void UdpEpStateLock(_UDPEndpoint* ep);
static void UdpEpStateUnlock(_UDPEndpoint* ep);

/**
 * A skeletal variety of a Stream used to fake the system into believing that
 * there is a stream-based protocol at work here.  This is not intended to be
 * wired into IODispatch but is used to allow the daemon to to run in a
 * threadless, streamless environment without major changes.
 */
class ArdpStream : public qcc::Stream {
  public:

    ArdpStream()
        : m_transport(NULL),
        m_endpoint(NULL),
        m_handle(NULL),
        m_conn(NULL),
        m_lock(),
        m_disc(false),
        m_discSent(false),
        m_discStatus(ER_OK),
        m_writeCondition(NULL),
        m_sendsOutstanding(0)
    {
        QCC_DbgTrace(("ArdpStream::ArdpStream()"));
        m_writeCondition = new qcc::Condition();
    }

    virtual ~ArdpStream()
    {
        QCC_DbgTrace(("ArdpStream::~ArdpStream()"));

        QCC_DbgPrintf(("ArdpStream::~ArdpStream(): delete events"));
        delete m_writeCondition;
        m_writeCondition = NULL;
    }

    /**
     * Get a pointer to the associated UDP transport instance.
     */
    UDPTransport* GetTransport() const
    {
        QCC_DbgTrace(("ArdpStream::GetTransport(): => %p", m_transport));
        return m_transport;
    }

    /**
     * Set the pointer to the associated UDP transport instance.
     */
    void SetTransport(UDPTransport* transport)
    {
        QCC_DbgTrace(("ArdpStream::SetTransport(transport=%p)", transport));
        m_transport = transport;
    }

    /**
     * Get a pointer to the associated UDP endpoint.
     */
    _UDPEndpoint* GetEndpoint() const
    {
        QCC_DbgTrace(("ArdpStream::GetEndpoint(): => %p", m_endpoint));
        return m_endpoint;
    }

    /**
     * Set the pointer to the associated UDP endpoint instance.
     */
    void SetEndpoint(_UDPEndpoint* endpoint)
    {
        QCC_DbgTrace(("ArdpStream::SetEndpoint(endpoint=%p)", endpoint));
        m_endpoint = endpoint;
    }

    /**
     * Get the information that describes the underlying ARDP protocol connection.
     */
    ArdpHandle* GetHandle() const
    {
        QCC_DbgTrace(("ArdpStream::GetHandle(): => %p", m_handle));
        return m_handle;
    }

    /**
     * Set the handle to the underlying ARDP protocol instance.
     */
    void SetHandle(ArdpHandle* handle)
    {
        QCC_DbgTrace(("ArdpStream::SetHandle(handle=%p)", handle));
        m_handle = handle;
    }

    /**
     * Get the information that describes the underlying ARDP protocol
     * connection.
     */
    ArdpConnRecord* GetConn() const
    {
        QCC_DbgTrace(("ArdpStream::GetConn(): => %p", m_conn));
        return m_conn;
    }

    /**
     * Set the information that describes the underlying ARDP protocol
     * connection.
     */
    void SetConn(ArdpConnRecord* conn)
    {
        QCC_DbgTrace(("ArdpStream::SetConn(conn=%p)", conn));
        m_conn = conn;
    }

    /**
     * Add the currently running thread to a set of threads that may be
     * currently referencing the internals of the stream.  We need this list to
     * make sure we don't try to delete the stream if there are threads
     * currently using the stream, and to wake those threads in case the threads
     * are blocked waiting for a send to complete when the associated endpoint
     * is shut down.
     */
    void AddCurrentThread()
    {
        QCC_DbgTrace(("ArdpStream::AddCurrentThread()"));

        ThreadEntry entry;
        entry.m_thread = qcc::Thread::GetThread();
        entry.m_stream = this;
        m_lock.Lock(MUTEX_CONTEXT);
        m_threads.insert(entry);
        m_lock.Unlock(MUTEX_CONTEXT);
    }

    /**
     * Remove the currently running thread from the set of threads that may be
     * currently referencing the internals of the stream.
     */
    void RemoveCurrentThread()
    {
        QCC_DbgTrace(("ArdpStream::RemoveCurrentThread()"));

        ThreadEntry entry;
        entry.m_thread = qcc::Thread::GetThread();
        entry.m_stream = this;

        m_lock.Lock(MUTEX_CONTEXT);
        set<ThreadEntry>::iterator i = m_threads.find(entry);
        assert(i != m_threads.end() && "ArdpStream::RemoveCurrentThread(): Thread not on m_threads");
        m_threads.erase(i);
        m_lock.Unlock(MUTEX_CONTEXT);
    }

    /**
     * Wake all of the treads that may be waiting on the condition for a chance
     * to contend for the resource.  We expect this function to be used to wake
     * all of the threads in the case that the stream is shutting down.
     * Presumably they will all wake up and notice that the stream is going away
     * and exit
     */
    void WakeThreadSet()
    {
        QCC_DbgTrace(("ArdpStream::WakeThreadSet()"));
        m_lock.Lock(MUTEX_CONTEXT);
        if (m_writeCondition) {
            m_writeCondition->Broadcast();
        }
        m_lock.Unlock(MUTEX_CONTEXT);
    }

    /**
     * Determine whether or not there is a thread waiting on the stream for a write
     * operation to complete.
     */
    bool ThreadSetEmpty()
    {
        QCC_DbgTrace(("ArdpStream::ThreadSetEmpty()"));

        m_lock.Lock(MUTEX_CONTEXT);
        bool empty = m_threads.empty();
        m_lock.Unlock(MUTEX_CONTEXT);

        QCC_DbgTrace(("ArdpStream::ThreadSetEmpty(): -> %s", empty ? "true" : "false"));
        return empty;
    }

    /**
     * Get the number of outstanding send buffers -- the number of sends that have been
     * called, but have not completed and had the buffers returned by ARDP.
     */
    uint32_t GetSendsOutstanding()
    {
        QCC_DbgTrace(("ArdpStream::GetSendsOutstanding() -> %d.", m_sendsOutstanding));
        m_transport->m_cbLock.Lock(MUTEX_CONTEXT);
        uint32_t sendsOutstanding = m_sendsOutstanding;
        m_transport->m_cbLock.Unlock(MUTEX_CONTEXT);
        return sendsOutstanding;
    }

    /**
     * Set the stream's write condition if it exists.  This will wake exactly
     * one waiting thread which will then loop back around and try to do its
     * work again.
     */
    void SignalWriteCondition()
    {
        QCC_DbgTrace(("ArdpStream::SignalWriteCondition()"));
        m_lock.Lock(MUTEX_CONTEXT);
        if (m_writeCondition) {
            m_writeCondition->Signal();
        }
        m_lock.Unlock(MUTEX_CONTEXT);
    }

    /**
     * Send some bytes to the other side of the conection described by the
     * m_conn member variable.
     *
     * The caller of this function is most likely the daemon router that is
     * moving a message to a remote destination.  It was written expecting this
     * call to copy bytes into TCP or block when TCP applies backpressure.  As
     * soon as the call returns, the router expects to be able to delete the
     * message backing buffer (our buf) and go on about its business.
     *
     * That means we basically have to do the same thing here unless we start
     * ripping the guts out of the system.  That means the daemon router expects
     * to see and endpoint with a stream in it that has this PushBytes method.
     *
     * we need to copy the data in and return immediately if there is no
     * backpressure from the protocol; or copy the data in and block the caller
     * if there is backpressure.  Backpressure is indicated by the
     * ER_ARDP_BACKPRESSURE return.  If this happens, we cannot send any more
     * data until we get a send callback indicating the other side has consumed
     * some data.  In this case we need to block the calling thread until it can
     * continue.
     *
     * When a buffer is sent, the ARDP protocol takes ownership of it until it
     * is ACKed by the other side or it times out.  When the ACK happens, a send
     * callback is fired that will record the actual status of the send and free
     * the buffer.  The status of the write is not known until the next read or
     * write operation.
     */
    QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent, uint32_t ttl)
    {
        QCC_DbgTrace(("ArdpStream::PushBytes(buf=%p, numBytes=%d., numSent=%p)", buf, numBytes, &numSent));
        QStatus status;

        /*
         * Start out by assuming that nothing worked and we have sent nothing.
         */
        numSent = 0;

        /*
         * If either the transport or the endpoint is not in a state where it
         * indicates that it is ready to send, we need to return an error.
         *
         * Unfortunately, the system will try to do all kinds of things to an
         * endpoint even though it has just stopped it.  To prevent cascades of
         * error logs, we need to return a "magic" error code and not log an
         * error if we detect that either the transport or the endpoint is
         * shutting down.  Higher level code (especially AllJoynObj) will look
         * for this error and not do any logging if it is a transient error
         * during shutdown, as identified by the error return value
         * ER_BUS_ENDPOINT_CLOSING.
         *
         * There is a window between the start of a remote disconnect event and
         * the endpoint state change where the endpoint can think it is up and
         * started but the stream can think it has started going down.  We'll
         * catch that case before actually starting the send.
         */
        if (m_transport->IsRunning() == false || m_transport->m_stopping == true) {
            return ER_BUS_ENDPOINT_CLOSING;
        }
        if (IsUdpEpStarted(m_endpoint) == false) {
            return ER_BUS_ENDPOINT_CLOSING;
        }

        /*
         * We can proceed, but we are a new thread that is going to be wandering
         * down into the stream code, so add ourselves to the list of threads
         * wandering around in the associated endpoint.  We need to keep track
         * of this in case the endpoint is stopped while the current thread is
         * wandering around in the stream trying to get the send done.
         */
        AddCurrentThread();

#ifndef NDEBUG
#if BYTEDUMPS
        DumpBytes((uint8_t*)buf, numBytes);
#endif
#endif
        /*
         * Copy in the bytes to preserve the buffer management approach expected by
         * higher level code.
         */
        QCC_DbgPrintf(("ArdpStream::PushBytes(): Copy in"));
#ifndef NDEBUG
        uint8_t* buffer = new uint8_t[numBytes + SEAL_SIZE];
        SealBuffer(buffer + numBytes);
#else
        uint8_t* buffer = new uint8_t[numBytes];
#endif
        memcpy(buffer, buf, numBytes);

        /*
         * Set up a timeout on the write.  If we call ARDP_Send, we expect it to
         * come back with some a send callback if it accepts the data.  As a
         * double-check, we add our own timeout that expires in twice the we
         * expect ARDP to time out.  This is effectively a watchdog that barks
         * when ARDP is not doing what we expect it to.
         */
        uint32_t timeout;
        Timespec tStart;

        m_transport->m_ardpLock.Lock();
        timeout = 2 * ARDP_GetDataTimeout(m_handle, m_conn);
        m_transport->m_ardpLock.Unlock();

        GetTimeNow(&tStart);
        QCC_DbgPrintf(("ArdpStream::PushBytes(): Start time is %" PRIu64 ".%03d.", tStart.seconds, tStart.mseconds));

        /*
         * This is the point at which a classic condition vairable wait idiom
         * comes into play.  Extracted from all of the dependencies here, it
         * would look something like:
         *
         *     mutex.Lock()
         *     while (condition != met) {
         *         condition.Wait(mutex);
         *     }
         *     mutex.Unlock();
         *
         * The mutex in question is the cbLock, which synchronizes this thread
         * (think a consumer contending for the protected resource) with the
         * callback thread from ARDP (think producer -- making the ARDP resource
         * available).  The condition in question is whether or not are done trying
         * to write to the stream (we can either succeed or error out).
         */
        bool done = false;
        m_transport->m_cbLock.Lock();
        while (done != true) {
            /*
             * We're in a loop here that could possibly run for many seconds
             * trying to get data out, so we need to check to make sure that the
             * transport is not shutting down each time through the loop.
             */
            if (m_transport->IsRunning() == false || m_transport->m_stopping == true) {
                status = ER_BUS_ENDPOINT_CLOSING;
                done = true;
                continue;
            }

            /*
             * Check the watchdog to make sure it has not timed out.
             */
            Timespec tNow;
            GetTimeNow(&tNow);
            int32_t tRemaining = tStart + timeout - tNow;
            QCC_DbgPrintf(("ArdpStream::PushBytes(): tRemaining is %d.", tRemaining));
            if (tRemaining <= 0) {
                status = ER_TIMEOUT;
                QCC_LogError(status, ("ArdpStream::PushBytes(): Timed out"));
                done = true;
                continue;
            }

            /*
             * Above, we did a coarse granularity checks for whether or not the
             * endpoint was ready to accept data.  There is one case for which
             * we have to be more careful though.  That's the shutdown case.
             * When we speak about shutdown, think socket shutdown.  We have
             * queued up a bunch of data, and we want to do the equivalent of a
             * shutdown(SHUT_WR) when we stop our endpoint.  This means we want
             * to wait for existing data to be sent, and then we want to shut
             * down the endpoint.
             *
             * The equivalent of a shutdown(SHUT_WR) in our case is calling
             * Stop() on an endpoint.  If the Stop() is a result of a local
             * shutdown, the endpoint will immediately transition to the
             * EP_STOPPING state.  This change will cause the endpoint
             * management thread to run.  The management thread will notice the
             * local stop and transition immediately to EP_WAITING state.  The
             * endpoint remains in EP_WAITING until all of the in-process sends
             * complete, at which time an ARDP_Disconnect() is sent and the
             * endpoint continues the destruction process.
             *
             * We need to make sure that we are in EP_STARTED state when we make
             * the ARDP_Send() call, and that we do not allow a change of state
             * out of EP_STARTED while the call is executing, otherwise we risk
             * making a send call and queueing a write during the time at which
             * the management thread is deciding when to execute the
             * ARDP_Disconnect.
             *
             * There at least three threads that may be interested in the
             * instantaneous value of the state: there's us, who is trying to
             * send bytes to the endpoint; there's the management thread who may
             * be wanting to change the state of the endpoint; and there's a
             * LeaveSession thread that may be driving a Stop().  We take the
             * endpoint state change lock and hold it during the ARDP call to
             * prevent possible inconsistencies.
             *
             * There is a window between the start of a remote disconnect event
             * and the endpoint state change where the endpoint can think it is
             * up and started but the stream can think it has started going
             * down.  If we start a send during that time it is doomed to fail,
             * so we check for this corner case at the last possible time.
             */
            UdpEpStateLock(m_endpoint);

            if (IsUdpEpStarted(m_endpoint)) {
                if (m_disc || m_discSent) {
                    /*
                     * The corner case mentioned above has happened, so don't
                     * bother to start the ARDP_Send since we'll just get an
                     * error.
                     *
                     * Note that there used to be a consistency checking assert
                     * right here.  It was removed since the disconnect callback
                     * doesn't update the state of the stream and the state of
                     * the endpoint atomically.  The "obvious" consistency check
                     * here failed under stress.  As discussed in a comment in
                     * DisconnectCb(), the inconsistency is harmless, but we
                     * minimize its impact here.
                     */
                    UdpEpStateUnlock(m_endpoint);
                    status = ER_BUS_ENDPOINT_CLOSING;
                    done = true;
                    continue;
                } else {
                    /*
                     * We think everything is up and ready in ARDP-land, so we
                     * can go ahead and start a send.
                     */
                    m_transport->m_ardpLock.Lock();
                    status = ARDP_Send(m_handle, m_conn, buffer, numBytes, ttl);
                    m_transport->m_ardpLock.Unlock();
                }
            } else {
                /*
                 * The endpoint is not in started state, which means it is
                 * disconnecting which means it is closing from the point of
                 * view of higher level code.  The AllJoynObj, for example,
                 * special-cases a return of ER_BUS_ENDPOINT_CLOSING since
                 * it is "expected."
                 */
                UdpEpStateUnlock(m_endpoint);
                status = ER_BUS_ENDPOINT_CLOSING;
                done = true;
                continue;
            }

            UdpEpStateUnlock(m_endpoint);

            /*
             * If we do something that is going to bug the ARDP protocol, we need
             * to call back into ARDP ASAP to get it moving.  This is done in the
             * main thread, which we need to wake up.  Note that we don't set
             * m_manage so we don't trigger endpoint management, we just trigger
             * ARDP_Run to happen.
             */
            m_transport->Alert();

            /*
             * If the send succeeded, then the bits are on their way off to the
             * destination.  The send callback associated with this PushBytes()
             * will take care of freeing the buffer we allocated.  We return back
             * to the caller as if we were TCP and had copied the bytes into the
             * kernel.
             */
            if (status == ER_OK) {
                /*
                 * The bytes are in flight, so as far as we are concerned they
                 * were successfully written; so indicate that to the caller.
                 *
                 * If we have successfully sent the bytes, the ARDP protocol has
                 * taken responsibility for them, so the buffer is now gone.  We
                 * can't ever touch this buffer so we need to rid ourselves of
                 * the reference to it.
                 */
                numSent = numBytes;
                m_transport->m_cbLock.Lock(MUTEX_CONTEXT);
                ++m_sendsOutstanding;
#if SENT_SANITY
                m_sentSet.insert(buffer);
#endif
                m_transport->m_cbLock.Unlock(MUTEX_CONTEXT);
                buffer = NULL;
                done = true;
                continue;
            }

            /*
             * If the send failed, and the failure was not due to the
             * application of backpressure by the protocol, we have a hard
             * failure and we must give up.  Since the buffer wasn't sent, the
             * callback won't happen and we need to dispose of it here and now.
             */
            if (status != ER_ARDP_BACKPRESSURE) {
                QCC_LogError(status, ("ArdpStream::PushBytes(): Hard failure"));
                done = true;
                continue;
            }

            /*
             * If backpressure has been applied by the ARDP protocol, we can't
             * send another message on this connection until the other side ACKs
             * one of the outstanding datagrams.  It communicates this to us by
             * a send callback which, in turn, signals the condition variable
             * we'll be waiting on and wakes us up. The Wait here corresponds to
             * the Wait in the condition idiom outlined above.
             *
             * What if the backpressure error happens and there are no
             * outstanding messages?  That is, what if the UDP buffer is full
             * and we cant' send even the one message.  Since there is no
             * message in process, there will never be a SendCb to bug the
             * condition to wake us up and therefore the send will time out.  We
             * assume that ARDP will allow us to send at least one message
             * before blocking us due to backpressure; so this is therefore
             * assumed to be a non-problem.
             */
            if (status == ER_ARDP_BACKPRESSURE) {
                QCC_DbgPrintf(("ArdpStream::PushBytes(): Backpressure. Condition::Wait()."));
                assert(m_writeCondition && "ArdpStream::PushBytes(): m_writeCondition must be set");
                status = m_writeCondition->TimedWait(m_transport->m_cbLock, tRemaining);

                /*
                 * We expect that the wait will return either ER_OK if it
                 * succeeds, or ER_TIMEOUT.  If the Wait fails in other ways
                 * then we will stop trying and communicate the failure back to
                 * the caller since we really don't know what might have
                 * happened and whether or not this is recoverable.
                 */
                if (status != ER_OK && status != ER_TIMEOUT) {
                    QCC_LogError(status, ("ArdpStream::PushBytes(): Condition::Wait() returned unexpected error"));
                    done = true;
                    continue;
                }

                /*
                 * If there was a disconnect in the underlying connection, there's
                 * nothing we can do but return the error.
                 */
                if (m_disc) {
                    status = ER_UDP_DISCONNECT;
                    QCC_LogError(status, ("ArdpStream::PushBytes(): Stream disconnected"));
                    done = true;
                    continue;
                }

                QCC_DbgPrintf(("ArdpStream::PushBytes(): Backpressure loop"));
                assert(done == false && "ArdpStream::PushBytes(): loop error");
            }

            /*
             * We are at the end of the while (condition != true) piece of the
             * condition variable wait idiom.  To get here, we must have tried
             * to send a message.  That send either did or did not work out.  If
             * it succeeded, done will have been set to true and we'll break out
             * of the while loop.  If there was a hard failure, done will have
             * also been set to true and we'll break out.  If We detected
             * backpressure, we blocked on the condition variable until it was
             * set.  This indicates that backpressure may have been relieved.
             * we need to loop back and try the ARDP_Send again, maybe
             * encountering backpressure again.
             */
        }

        /*
         * If the buffer was successfully sent off to ARDP, then we no longer
         * have ownership of the buffer and the pointer will have been set to
         * NULL.  If it is not NULL we own it and must dispose of it.
         */
        if (buffer) {
#ifndef NDEBUG
            CheckSeal(buffer + numBytes);
#endif
            delete[] buffer;
            buffer = NULL;
        }

        /*
         * This is the last unlock in the condition wait idiom outlined above.
         * We are all done here.
         */
        m_transport->m_cbLock.Unlock();
        RemoveCurrentThread();
        return status;
    }

    /*
     * A version of PushBytes that doesn't care about TTL.
     */
    QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent)
    {
        QCC_DbgTrace(("ArdpStream::PushBytes(buf=%p, numBytes=%d., numSent=%p)", buf, numBytes, &numSent));
        return PushBytes(buf, numBytes, numSent, 0);
    }

    /**
     * Get some bytes from the other side of the conection described by the
     * m_conn member variable.  Data must be present in the message buffer
     * list since we expect that a RecvCb that added a buffer to that list is
     * what is going to be doing the read that will eventually call PullBytes.
     * In that case, since the data is expected to be present, <timeout> will
     * be zero.
     */
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
    {
        QCC_DbgTrace(("ArdpStream::PullBytes(buf=%p, reqBytes=%d., actualBytes=%d., timeout=%d.)",
                      buf, reqBytes, actualBytes, timeout));
        assert(0 && "ArdpStream::PullBytes(): Should never be called");
        return ER_FAIL;
    }

    /**
     * Set the stram up for being torn down before going through the expected
     * lifetime state transitions.
     */
    void EarlyExit()
    {
        QCC_DbgTrace(("ArdpStream::EarlyExit()"));

        /*
         * An EarlyExit is one when a stream has been created in the expectation
         * that an endpoint will be brought up, but the system changed its mind
         * in mid-"stream" and therefore there is no disconnect processing needed
         * and there must be no threads waiting.
         */
        m_lock.Lock(MUTEX_CONTEXT);
        m_disc = true;
        m_conn = NULL;
        m_discStatus = ER_UDP_EARLY_EXIT;
        m_lock.Unlock(MUTEX_CONTEXT);
    }

    /**
     * Get the disconnected status.  If the stream has been disconnected, return
     * true otherwise false.
     */
    bool GetDisconnected()
    {
        QCC_DbgTrace(("ArdpStream::Disconnected(): -> %s", m_disc ? "true" : "false"));
        return m_disc;
    }

    /**
     * In the case of a local disconnect, disc sent means that ARDP_Disconnect()
     * has been called.  Determine if this call has been made or not.
     */
    bool GetDiscSent()
    {
        QCC_DbgTrace(("ArdpStream::GetDiscSent(): -> %s", m_discSent ? "true" : "false"));
        return m_discSent;
    }

    /**
     * In the case of a remote disconnect, disc status reflects the reason the
     * stream was disconnected.  In the local case, will be ER_OK until the
     * disconnect happens, then will be ER_UDP_LOCAL_DISCONNECT.
     */
    QStatus GetDiscStatus()
    {
        QCC_DbgTrace(("ArdpStream::GetDiscStatus(): -> \"%s\"", QCC_StatusText(m_discStatus)));
        return m_discStatus;
    }

    /**
     * Process a disconnect event, either local or remote.
     */
    void Disconnect(bool sudden, QStatus status)
    {
        QCC_DbgTrace(("ArdpStream::Disconnect(sudden==%d., status==\"%s\")", sudden, QCC_StatusText(status)));

        /*
         * A "sudden" disconnect is an unexpected or unsolicited disconnect
         * initiated from the remote side.  In this case, we will have have
         * gotten an ARDP DisconnectCb() which tells us that the connection is
         * gone and we shouldn't use it again.
         *
         * If sudden is not true, then this is as a result of a local request to
         * terminate the connection.  This means we need to call ARDP and let it
         * know we are disconnecting.  We wait for the DisconnectCb() that must
         * happen as a result of the ARDP_Disconnect() to declare the connection
         * completely gone.
         *
         * The details can get very intricate becuase once a remote side has
         * disconnected, we can get a flood of disconnects from different local
         * users of the endopint as the daemon figures out what it no longer
         * needs as a result of a remote endpoint going away.  We just have to
         * harden ourselves against many duplicate calls.  There are three bits
         * to worry about (sudden, m_discSent, and m_disc) and so there are
         * eight possibile conditions/states here.  We just break them all out.
         */
#ifndef NDEBUG
        if (status == ER_OK) {
            assert(sudden == false);
        }
        if (sudden) {
            assert(status != ER_OK);
        }
#endif

        QCC_DbgPrintf(("ArdpStream::Disconnect(): sudden==%d., m_disc==%d., m_discSent==%d., status==\"%s\"",
                       sudden, m_disc, m_discSent, QCC_StatusText(status)));
        m_lock.Lock(MUTEX_CONTEXT);
        if (sudden == false) {
            if (m_disc == false) {
                if (m_discSent == false) {
                    /*
                     * sudden = false, m_disc = false, m_discSent == false
                     *
                     * This is a new solicited local disconnect event that is
                     * happening on a stream that has never seen a disconnect
                     * event.  We need to do an ARDP_Disconnect() to start the
                     * disconnect process.  We expect status to be
                     * ER_UDP_LOCAL_DISCONNECT by contract.  If we fail to send
                     * the ARDP_Disconnect() the disconnect status is updated
                     * to the reason we couldn't send it.
                     */
                    assert(status == ER_UDP_LOCAL_DISCONNECT && "ArdpStream::Disconnect(): Unexpected status");
                    m_transport->m_ardpLock.Lock();
                    status = ARDP_Disconnect(m_handle, m_conn);
                    m_transport->m_ardpLock.Unlock();
                    if (status == ER_OK) {
                        m_discSent = true;
                        m_discStatus = ER_UDP_LOCAL_DISCONNECT;
                    } else {
                        QCC_LogError(status, ("ArdpStream::Disconnect(): Cannot send ARDP_Disconnect()"));
                        m_disc = true;
                        m_conn = NULL;
                        m_discSent = true;
                        m_discStatus = status;
                    }

                    /*
                     * Tell the endpoint manager that something interesting has
                     * happened
                     */
                    m_transport->m_manage = UDPTransport::STATE_MANAGE;
                    m_transport->Alert();
                } else {
                    /*
                     * sudden = false, m_disc = false, m_discSent == true
                     *
                     * This disconnect event is happening as a result of the
                     * ARDP disconnect callback.  We expect that the status
                     * passed in is ER_OK to confirm that this is the response
                     * to the ARDP_Disconnect().  This completes the locally
                     * initiated disconnect process.  If this happens, we expect
                     * status to be ER_OK by contract and we expect the
                     * disconnect status to have been set to
                     * ER_UDP_LOCAL_DISCONNECT by us.
                     */
                    assert(status == ER_OK && "ArdpStream::Disconnect(): Unexpected status");
                    assert(m_discStatus == ER_UDP_LOCAL_DISCONNECT && "ArdpStream::Disconnect(): Unexpected status");
                    m_disc = true;
                    m_conn = NULL;

                    /*
                     * Tell the endpoint manager that something interesting has
                     * happened
                     */
                    m_transport->m_manage = UDPTransport::STATE_MANAGE;
                    m_transport->Alert();
                }
            } else {
                if (m_discSent == false) {
                    /*
                     * sudden = false, m_disc = true, m_discSent == false
                     *
                     * This is a locally initiated disconnect that happens as a
                     * result of a previously received remote disconnect.  This
                     * can happen when the daemon begins dereferencing
                     * (Stopping) endpoints as a result of a previously reported
                     * disconnect.
                     *
                     * The connection should already be gone.
                     */
                    assert(m_conn == NULL && "ArdpStream::Disconnect(): m_conn unexpectedly live");
                    assert(m_disc == true && "ArdpStream::Disconnect(): unexpectedly not disconnected");
                } else {
                    /*
                     * sudden = false, m_disc = true, m_discSent == true
                     *
                     * This is a locally initiated disconnect that happens after
                     * a local disconnect that has completed (ARDP_Disconnect()
                     * has been called and its DisconnectCb() has been received.
                     * This can happen when the daemon begins dereferencing
                     * (Stopping) endpoints as a result of a previously reported
                     * disconnect but is a little slow at doing so.
                     *
                     * The connection should already be gone.
                     */
                    assert(m_conn == NULL && "ArdpStream::Disconnect(): m_conn unexpectedly live");
                    assert(m_disc == true && "ArdpStream::Disconnect(): unexpectedly not disconnected");
                }
            }
        } else {
            if (m_disc == false) {
                if (m_discSent == false) {
                    /*
                     * sudden = true, m_disc = false, m_discSent == false
                     *
                     * This is a new unsolicited remote disconnect event that is
                     * happening on a stream that has never seen a disconnect
                     * event.
                     */
                    m_conn = NULL;
                    m_disc = true;
                    m_discStatus = status;
                } else {
                    /*
                     * sudden = true, m_disc = false, m_discSent == true
                     *
                     * This is an unsolicited remote disconnect event that is
                     * happening on a stream that has previously gotten a local
                     * disconnect event and called ARDP_Disconnect() but has not
                     * yet received the DisconnectCb() as a result of that
                     * ARDP_Disconnect().
                     *
                     * This indicates a race between the local disconnect and a
                     * remote disconnect.  Any sudden disconnect means the
                     * connection is gone; so a remote disconnect trumps an
                     * in-process local disconnect.  This means we go right to
                     * m_disc = true.  We'll leave the original m_discStatus
                     * alone.
                     */
                    m_conn = NULL;
                    m_disc = true;
                }
            } else {
                if (m_discSent == false) {
                    /*
                     * sudden = true, m_disc = true, m_discSent == false
                     *
                     * This is a second unsolicited remote disconnect event that
                     * is happening on a stream that has previously gotten a
                     * remote disconnect event -- a duplicate in other words.
                     * We'll leave the original m_discStatus alone.
                     *
                     * The connection should already be gone.
                     */
                    assert(m_conn == NULL && "ArdpStream::Disconnect(): m_conn unexpectedly live");
                    assert(m_disc == true && "ArdpStream::Disconnect(): unexpectedly not disconnected");
                } else {
                    /*
                     * sudden = true, m_disc = true, m_discSent == true
                     *
                     * This is an unsolicited remote disconnect event that is
                     * happening on a stream that has previously gotten a local
                     * disconnect event that has completed.  We have already
                     * called ARDP_Disconnect() and gotten the DisconnectCb()
                     * and the connection is gone.  This can happen if both
                     * sides decide to take down connections at about the same
                     * time.  We'll leave the original m_discStatus alone.
                     *
                     * The connection should already be gone.
                     */
                    assert(m_conn == NULL && "ArdpStream::Disconnect(): m_conn unexpectedly live");
                    assert(m_disc == true && "ArdpStream::Disconnect(): unexpectedly not disconnected");
                }
            }
        }

#ifndef NDEBUG
        if (sudden) {
            assert(m_disc == true);
        }
#endif

        m_lock.Unlock(MUTEX_CONTEXT);
    }

    /**
     * This is the data sent callback which is plumbed from the ARDP protocol up
     * to this stream.  This callback means that the buffer is no longer
     * required and may be freed.  The ARDP protocol only had temporary custody
     * of the buffer.
     */
    void SendCb(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status)
    {
        QCC_DbgTrace(("ArdpStream::SendCb(handle=%p, conn=%p, buf=%p, len=%d.)", handle, conn, buf, len));
        m_transport->m_cbLock.Lock();
        --m_sendsOutstanding;

#if SENT_SANITY
        set<uint8_t*>::iterator i = find(m_sentSet.begin(), m_sentSet.end(), buf);
        if (i == m_sentSet.end()) {
            QCC_LogError(ER_FAIL, ("ArdpStream::SendCb(): Callback for unexpected buffer (%p, %d.): Ignored.", buf, len));
        } else {
            m_sentSet.erase(i);
        }
#endif

        m_transport->m_cbLock.Unlock();

#ifndef NDEBUG
        CheckSeal(buf + len);
#endif
        delete[] buf;

        /*
         * If there are any threads waiting for a chance to send bits, wake them
         * up.  They will retry their sends when this event gets set.  If the
         * send callbacks are part of normal operation, the sends may succeed
         * the next time around.  If this callback is part of disconnect
         * processing the next send will fail with an error; and PushBytes()
         * will manage the outstanding write count.
         */
        if (m_writeCondition) {
            QCC_DbgPrintf(("ArdpStream::SendCb(): Condition::Signal()"));
            m_writeCondition->Signal();
        }
    }

    class ThreadEntry {
      public:
        qcc::Thread* m_thread;
        ArdpStream* m_stream;
    };

  private:
    ArdpStream(const ArdpStream& other);
    ArdpStream operator=(const ArdpStream& other);

    UDPTransport* m_transport;         /**< The transport that created the endpoint that created the stream */
    _UDPEndpoint* m_endpoint;          /**< The endpoint that created the stream */
    ArdpHandle* m_handle;              /**< The handle to the ARDP protocol instance this stream works with */
    ArdpConnRecord* m_conn;            /**< The ARDP connection associated with this endpoint / stream combination */
    qcc::Mutex m_lock;                 /**< Mutex that protects m_threads and disconnect state */
    bool m_disc;                       /**< Set to true when ARDP fires the DisconnectCb on the associated connection */
    bool m_discSent;                   /**< Set to true when the endpoint calls ARDP_Disconnect */
    QStatus m_discStatus;              /**< The status code that was the reason for the last disconnect */
    qcc::Condition* m_writeCondition;  /**< The write event that callers are blocked on to apply backpressure */
    int32_t m_sendsOutstanding;        /**< The number of Message sends that are outstanding (in-flight) with ARDP */
    std::set<ThreadEntry> m_threads;   /**< Threads that are wandering around in the stream and possibly associated endpoint */

#if SENT_SANITY
    std::set<uint8_t*> m_sentSet;
#endif

    class BufEntry {
      public:
        BufEntry() : m_buf(NULL), m_len(0), m_pulled(0), m_rcv(NULL), m_cnt(0) { }
        uint8_t* m_buf;
        uint16_t m_len;
        uint16_t m_pulled;
        ArdpRcvBuf* m_rcv;
        uint16_t m_cnt;
    };
};

bool operator<(const ArdpStream::ThreadEntry& lhs, const ArdpStream::ThreadEntry& rhs)
{
    return lhs.m_thread < rhs.m_thread;
}

/*
 * A class to encapsulate a queue of messages to be dispatched and an associated
 * thread to pump the messages.
 *
 * Whevever we have an AllJoyn Message that is ready for delivery, ideally we
 * would just hand it off to the daemon router by calling PushMessage and the
 * router would just deliver it.  There is a problem though.  What the router
 * does is to look up the destination endpoint(s) -- there may be more than one
 * if we have a signal -- and it will stick the messages on the receive queue
 * for the endpoint(s).  The catch is that if the endpoint receive queue is full
 * (because the endpoint is not consuming messages fast enough) the router will
 * block the calling thread.  If we were to allow the UDP Transport message
 * dispatcher thread itselfto block, it would essentially stop the entire UDP
 * Transport until the destination endpoint in question got around to doing
 * something about its messages.  We cannot allow that to happen.
 *
 * We want to apply backpressure directly to the source if a message cannot be
 * delivered.  The way to do that is to avoid calling ARDP_RecvReady() until the
 * message is delivered.  Backpressure will not be applied at the other side
 * until the maximum number of in-flight messages are queued.  This means that
 * we need to be able to accommodate ardpConfig.segmax messages queued up for
 * each endpoint.
 *
 * The phenomenon of the destination queue filling up has been called the "slow
 * reader problem."  Daemon Router-based solutions are being considered, but we
 * have to do something now; so we solve the problem by having a thread other
 * than the message dispatcher thread dispatch the PushMessage to the router.
 * The RemoteEndpoint has a pool of up to 100 threads available, but they are
 * deeply wired down into the concept of a stream socket, which we don't use.
 * So the thread in question lives here.
 *
 * It is relatively cheap to spin up a thread, but this behavior is highly
 * platform dependent so we don't want to do this arbitrarily frequently.  Since
 * our threads consume at least one qcc::Event which, in turn, consumes one or
 * two FDs, we need to be careful about leaving lots of them around.  This leads
 * to a design wjere we associate a thread with an endpoint in an on-demand
 * basis, with the thread exiting after some relatively short period of non-use,
 * but hanging around for bursts of messages.
 */
class MessagePump {
  public:
    MessagePump(UDPTransport* transport)
        : m_transport(transport), m_lock(), m_activeThread(NULL), m_pastThreads(), m_queue(), m_condition(), m_spawnedThreads(0), m_stopping(false)
    {
        QCC_DbgTrace(("MessagePump::MessagePump()"));
    }

    virtual ~MessagePump()
    {
        QCC_DbgTrace(("MessagePump::~MessagePump()"));
        QCC_DbgTrace(("MessagePump::~MessagePump(): Dealing with threads"));
        Stop();
        Join();

        QCC_DbgTrace(("MessagePump::~MessagePump(): Dealing with remaining queued messages"));
        while (m_queue.empty() == false) {
            QueueEntry entry = m_queue.front();
            m_queue.pop();
            m_transport->m_ardpLock.Lock();
            ARDP_RecvReady(entry.m_handle, entry.m_conn, entry.m_rcv);
            m_transport->m_ardpLock.Unlock();
        }

        assert(m_queue.empty() && "MessagePump::~MessagePump(): Message queue must be empty here");
        assert(m_activeThread == NULL && "MessagePump::~MessagePump(): Active thread must be gone here");
        assert(m_pastThreads.empty() && "MessagePump::~MessagePump(): Past threads must be gone here");

        QCC_DbgTrace(("MessagePump::~MessagePump(): Done"));
    }

    /*
     * Determine whether or not there is an active pump thread that may be wanting to
     * call out to the daemon.
     */
    bool IsActive()
    {
        QCC_DbgTrace(("MessagePump::IsActive()"));
        bool ret = m_activeThread != NULL;
        QCC_DbgPrintf(("MessagePump::IsActive() => \"%s\"", ret ? "true" : "false"));
        return ret;
    }

    /*
     * The Message Pump doesn't inherit from qcc::Thread so it isn't going to
     * Start(), Stop() and Join() direcly.  It HASA Thread That is Start()ed
     * on-demand.  We do implement Stop() to allow us to do something when we
     * need to cause the pump thread to stop.
     */
    QStatus Stop()
    {
        QCC_DbgTrace(("MessagePump::Stop()"));
        QStatus status = ER_OK;

        /*
         * There may be at most one m_activeThread running.  We need to set its
         * state to STOPPING.  This thread isn't going to be waiting on its stop
         * event though.  In order to minimize the number of events we create,
         * it will be waiting on a condition variable and then test for stopping
         * whenever it wakes up.  So we have to call Stop() to set the state and
         * then Signal() our condition variable to wake up the thread.
         */
        m_lock.Lock();
        m_stopping = true;
        if (m_activeThread) {
            QCC_DbgPrintf(("MessagePump::Stop(): m_activeThread->Stop()"));
            m_activeThread->Stop();
            QCC_DbgPrintf(("MessagePump::Stop(): m_condition.Signal()"));
            m_condition.Signal();
        }
#ifndef NDEBUG
        else {
            QCC_DbgPrintf(("MessagePump::Stop(): m_activeThread is NULL"));
        }
#endif
        m_lock.Unlock();

        QCC_DbgTrace(("MessagePump::Stop() => \"%s\"", QCC_StatusText(status)));
        return status;
    }

    /*
     * The Message Pump doesn't inherit from qcc::Thread so it isn't going to
     * Start(), Stop() and Join() direcly.  It HASA Thread That is Start()ed
     * on-demand and can be Stop()ped.  It also HASA list of threads that have
     * decided to shut down.  We implement Join() to allow us to join *all* of
     * those threads when we need the pump to shut down
     */
    QStatus Join()
    {
        QCC_DbgTrace(("MessagePump::Join()"));
        return DoJoin(true);
    }

    /*
     * The Message Pump doesn't inherit from qcc::Thread so it isn't going to
     * Start(), Stop() and Join() direcly.  It HASA Thread That is Start()ed
     * on-demand and can be Stop()ped.  It also HASA list of threads that have
     * decided to shut down.  We implement JoinPast() to allow the endpoint
     * management thread to join those shutdown threads but allow an active
     * thread to continue running.
     */
    QStatus JoinPast()
    {
        QCC_DbgTrace(("MessagePump::JoinPast()"));
        return DoJoin(false);
    }

    /*
     * Common internal method implementing both flavors of join.
     */
    QStatus DoJoin(bool both)
    {
        QCC_DbgTrace(("MessagePump::DoJoin(both=\"%s\")", both ? "true" : "false"));
        QStatus status = ER_OK;

        /*
         * There may be at most one m_activeThread running and there may be a
         * set of past threads that need Join()ing.  We expect that the active
         * thread will stop and put itself on the past threads queue.  So we
         * need to join all of the threads on the past threads queue until the
         * spawned thread count goes to zero.  We also expect that pump threads
         * will time out and exit if unused and the endpoint manager will have
         * periodically joined any past threads so this may not happen much in
         * practice.
         */
        m_lock.Lock();
        while (m_spawnedThreads) {
            QCC_DbgPrintf(("MessagePump::DoJoin(): m_spawnedThreads=%d.", m_spawnedThreads));
            assert((m_activeThread ? 1 : 0) + m_pastThreads.size() == m_spawnedThreads && "MessagePump::DoJoin(): m_spawnedThreads count inconsistent");

            if (m_pastThreads.size()) {
                PumpThread* pt = m_pastThreads.front();
                m_pastThreads.pop();
                --m_spawnedThreads;
                m_lock.Unlock();
                QStatus status = pt->Join();
                if (status != ER_OK) {
                    QCC_LogError(status, ("MessagePump::DoJoin: PumpThread Join() error"));
                }
                delete pt;
                pt = NULL;
                m_lock.Lock();
            } else {
                /*
                 * If there is a spawned thread left and no threads on the
                 * past threads list, it means that there is an acive thread
                 * that has not yet stopped.  Since the endpoint manager wants
                 * to clean up zombies periodically, it needs to be able to
                 * Join() past threads but not the active thread.  It calls
                 * JoinPast() to do this, and JoinPast() calls DoJoin(both=false)
                 * which sets both to false.  The endpoint Join() needs to
                 * wait for all threads to be joined so it calls Join() which
                 * calls DoJoin(both=true).
                 */
                if (both) {

                    /*
                     * We need to wait for the active thread to put itself on
                     * the past threads queue.
                     *
                     * TODO: use condition variable and thread exit routine
                     * instead of sleeping/polling.
                     */
                    assert(m_stopping == true && "MessagePump::DoJoin(): m_stopping must be true if both=true)");

                    m_lock.Unlock();
                    qcc::Sleep(10);
                    m_lock.Lock();

                    /*
                     * We are taking and giving the mutex lock constantly in
                     * order to call out to Join() in particular which might
                     * block.  Since we are turning loose of the lock, it could
                     * be the case that there are race conditions between the
                     * Stop() and condition Signal that is going to drive the
                     * active thread to exit, and the spinning up of new active
                     * threads.  In order to prevent waiting around forever, we
                     * keep poking the active thread as we wait for it to go
                     * away.
                     */
                    if (m_activeThread) {
                        m_activeThread->Stop();
                        m_condition.Signal();
                    }
                } else {
                    /*
                     * Okay to leave the m_activeThread unJoin()ed.
                     */
                    break;
                }
            }
        }
#ifndef NDEBUG
        if (both) {
            assert(m_spawnedThreads == 0 && "MessagePump::DoJoin(): m_spawnedThreads must be 0 after DoJoin(true)");
        } else {
            assert(m_spawnedThreads <= 1 && "MessagePump::DoJoin(): m_spawnedThreads must be 0 or 1 after DoJoin(false)");
        }
#endif
        m_lock.Unlock();

        QCC_DbgPrintf(("MessagePump::DoJoin(): m_spawnedThreads=%d. at return", m_spawnedThreads));
        QCC_DbgPrintf(("MessagePump::DoJoin() => \"%s\"", QCC_StatusText(status)));
        return status;
    }

    void RecvCb(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t connId, ArdpRcvBuf* rcv, QStatus status)
    {
        QCC_DbgTrace(("MessagePump::RecvCb(handle=%p, conn=%p, connId=%d., rcv=%p, status=%s)", handle, conn, connId, rcv, QCC_StatusText(status)));

        assert(status == ER_OK && "MessagePump::RecvCb(): Asked to dispatch an error!?");

        /*
         * We always want to pump the message in the callback so create a queue
         * entry and push it onto the queue.  If we start a thread, the first
         * thing it will do is to check the condition (queue is not empty) so it
         * will handle this entry.  If it turns out that there is a thread that
         * has stopped we don't want to spin up a new thread, but we can queue
         * up the message which will be handled when the pump is cleaned up.
         */
        m_lock.Lock();
        QueueEntry entry(handle, conn, connId, rcv, status);
        m_queue.push(entry);

        /*
         * RecvCb() callbacks can continue to happen after we have stopped the
         * active thread, so make sure we don't spin up a new thread that might
         * after we acknowledge a Stop() request.
         */
        if (m_stopping) {
            QCC_DbgPrintf(("MessagePump::RecvCb(): Stopping"));
            m_lock.Unlock();
            return;
        }

        /*
         * The thread rules: If there is no pump thread in existence, create one
         * and start it running.  Put the PumpThread* into m_activeThread.  If
         * there is a PumpThread* in m_activeThread, it must be running.  If you
         * want to stop a pump thread you must remove it from m_activeThread
         * before you do the stop.  If a thread detects it is no longer needed
         * (the wait times out) it will remove itself as the active thread and
         * put itself on the past threads list so it can be Join()ed.
         *
         * Here's a tricky question: what happens if we can allocate a pump
         * thread object but we can't start the pump thread?  We need the thread
         * to do something with the data, but we have no thread.  We could sit
         * in a loop trying to spawn a thread here. but that would block ARDP
         * that is trying to get rid of incoming messages.  We can wait for a
         * next incoming message to try again, but since the thread is going to
         * open the receive window we will only get SEGMAX tries and then the
         * process stalls.  If no new connection happens to start driving the
         * process again, the messages simply remain on the input queue.  We
         * choose bailing and waiting for a new message to drive the process
         * since if we cannot spin up a thread, we have bigger problems than
         * a temporarily stalled message.  The bytes aren't lost, they are
         * temporarily stashed until things maybe get better.  We hope ARDP
         * is understanding.
         */
        if (m_activeThread == NULL) {
            QCC_DbgPrintf(("MessagePump::RecvCb(): Spin up new PumpThread"));
            m_activeThread = new PumpThread(this);
            QStatus status = m_activeThread->Start(NULL, NULL);

            /*
             * The choice described in the above comment.
             */
            if (status != ER_OK) {
                delete m_activeThread;
                m_activeThread = NULL;
                m_lock.Unlock();
                return;
            } else {
                ++m_spawnedThreads;
            }
        }

        QCC_DbgPrintf(("MessagePump::RecvCb(): m_spawnedThreads=%d.", m_spawnedThreads));
        assert(m_activeThread && "MessagePump::RecvCb(): Expecting an active thread at this time.");

        /*
         * It may be the case that the active thread is stopping but it has not
         * yet moved its pointer m_activeThread to the list of past threads.  In
         * this case, there will be nobody to read and dispatch the queue entry
         * correcponding to the RecvCb.  We expect that if the thread
         * IsStopping() it must be because someone has called Stop() on the
         * message pump, and the place where this is done is in the endpoint
         * Stop() method.  This means that the endpoint is in the process of
         * being torn down.  When the message pump is eventually deleted, the
         * queued messages will be pulled off the queue and returned to ARDP.
         * The upshot is that it is okay that the thread is stopping.  We return
         * any queued messages in the order that we received them in the
         * destructor.  We just print a message that this is happening in debug
         * mode.
         */
#ifndef NDEBUG
        if (m_activeThread->IsStopping()) {
            QCC_DbgPrintf(("MessagePump::RecvCb(): Thread stopping."));
        }
#endif

        /*
         * Signal the condition to wake up an existing thread that may be asleep
         * waiting for something to do.
         */
        m_condition.Signal();
        m_lock.Unlock();
    }

  private:
    class QueueEntry {
      public:
        QueueEntry(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t connId, ArdpRcvBuf* rcv, QStatus status)
            : m_handle(handle), m_conn(conn), m_connId(connId), m_rcv(rcv), m_status(status) { }

        ArdpHandle* m_handle;
        ArdpConnRecord* m_conn;
        uint32_t m_connId;
        ArdpRcvBuf* m_rcv;
        QStatus m_status;
    };

    class PumpThread : public qcc::Thread {
      public:
        PumpThread(MessagePump* pump) : qcc::Thread(qcc::String("PumpThread")), m_pump(pump) { }
        void ThreadExit(Thread* thread);

      protected:
        qcc::ThreadReturn STDCALL Run(void* arg);

      private:
        MessagePump* m_pump;
    };

    UDPTransport* m_transport;              /**< The UDP Transport instance associated with this pump */
    qcc::Mutex m_lock;                      /**< Mutex that protects multithread access to the queue */
    PumpThread* m_activeThread;             /**< Thread doing the actual work */
    std::queue<PumpThread*> m_pastThreads;  /**< Queue of transient threads have shut down or are shutting and need to be joined */
    std::queue<QueueEntry> m_queue;         /**< Queue of received messages and associated data to dispatch to the router */
    qcc::Condition m_condition;             /**< Condition variable coordinating consumption of queue entries by the pump thread */
    uint32_t m_spawnedThreads;              /**< The number of threads that have been spawned but not joined */
    bool m_stopping;                        /**< True if Stop() has been called and we shouldn't spin up new threads */
};

/*
 * An endpoint class to abstract the notion of an addressible point in the
 * network.  Endpoints are used by the Routing Node to deal with the details of
 * moving bits from here to there.
 */
class _UDPEndpoint : public _RemoteEndpoint {
  public:
    /**
     * The UDP Transport is a flavor of a RemoteEndpoint.  The daemon thinks of
     * remote endpoints as moving through a number of states, some that have
     * threads wandering around and some that do not.  In order to make sure we
     * are in agreement with what the daemon things we will be doing we keep
     * state regarding what threads would be doing if they were actually here
     * and running.  We also keep track of how endpoints were started and how
     * they move through the process from construction to running.
     *
     *
     *                     +----------------+
     *                     | EP_INITIALIZED |
     *                     +----------------+
     *                           |    |
     *                 Connect() |    | AcceptCb()
     *                           |    |
     *               +-----------+    +-----------+
     *               |                            |
     *               v                            v
     *     +-------------------+        +--------------------+
     *     | EP_ACTIVE_STARTED |        | EP_PASSIVE_STARTED |
     *     +-------------------+        +--------------------+
     *               |                            |
     *    Handshake  |                            | Handshake
     *               +-----------+    +-----------+
     *          Daemon Register  |    |  Daemon Register
     *                           v    v
     *                     +----------------+
     *                     |   EP_STARTED   |
     *                     +----------------+
     *                              |
     *                   Shut down  | <----------------------+
     *                              |                        | Idle
     *                              v                        |
     *                     +----------------+          +----------------+
     *                     |   EP_STOPPING  | -------> |   EP_WAITING   |
     *                     +----------------+  local   +----------------+
     *                              |        disconnect
     *                 Thread join  |
     *                              v
     *                     +----------------+
     *                     |   EP_JOINED    |
     *                     +----------------+
     *                              |
     *           Daemon Unregister  |
     *                              v
     *                     +----------------+
     *                     |     EP_DONE    |
     *                     +----------------+
     *
     *
     * When an endoint is constructed, it begins its live in EP_INITIALIZED.
     * endpoints are constructed for the purpose of service passive or active
     * connections.  A passive connection is one that is driven from a connection
     * coming in from a remote host.  An active connection is one that is driven
     * by a local thread calling UDPTransport::Connect().
     *
     * When an inbound connection causes an accept callback to fire, AcceptCb()
     * will create a UDP Endpoint and set the state to EP_PASSIVE_STARTED.
     * These endoints are put on the m_preList for deadlock avoidance and are
     * then quickly moved to the m_authList where they wait for authentication
     * to complete.  When the three-way ARDP handshake completes, as indicated
     * by a DoConnectcb() firing for that connection, the endpoint is removed
     * from the m_authList and put on the running endpoint list.  Since Start()
     * can't be called with locks held (it calls out to the daemon router), it
     * is actually called after the endpoint goes onto the endpoint list,
     * meaning we will see endpoints in state EP_PASSIVE_STARTED for short
     * periods of time on the running endpoints list.
     *
     * When an outbound connection causes a UDP Endpoint to be created the endpoint
     * is set to state EP_ACTIVE_STARTED.  This is done in the DoConnectCB() function
     * that indicates that the three-way handshake has completed.  For the same
     * deadlock-avoidance reason as in the passive case, the endpoint is placed on the
     * running endpoint list and then Start() is called.  This means will will see
     * endpoints on the running endpoint list for transient periods of time in state
     * EP_ACTIVE_STARTED.
     *
     * Once started, it means that the endpoint is connected (registered) to the
     * daemon router and is happily running and capable of pumping messages in and
     * out.
     *
     * As soon as either the local side or the remote side decides that the
     * endpoint has been around long enough, Stop() is called.  This may happen
     * either due to a Disconnect callback in the case of the remote side, or a
     * Stop() called as the result of a LeaveSession on the local side.  It is
     * possible that there are multiple threads running around in the endpoint
     * due to unconsumed remote messages making their way through the router,
     * unsent messages of local origin making their way down to ARDP, or threads
     * trying to PushMessage into the endpoint.  The transition into state
     * EP_STOPPING means the no new operations are allowed on the endpoint and
     * the process of causing all threads to abort and leave the endpoint are
     * started.
     *
     * If the source of the disconnect is a local LeaveSession, we need to
     * simulate socket shutdown behavior.  Specifically, we simulate
     * shutdown(sockfd, SHUT_RDWR) which is what is done in the TCP Transport.
     * In this state, we wait until all buffers sent to ARDP have been disposed
     * of one way or another.  If the other side receives all queued buffers and
     * acknowledges them we have a clean endpoint shutdown.  This additional state
     * is called EP_WAITING and is enabled only if a local (expected) disconnect
     * has been received.
     *
     * After the endpoint is EP_STOPPING and EP_WAITING is not enabled, or if
     * the endpoint is EP_WAITING and queued buffers are fully sent, the
     * endpoint management function notices the situation and then calls Join()
     * to mark the endpoint as being thread-free, buffer-free ande ready to
     * close.  This causes the endpoint to enter the EP_JOINED state.
     *
     * Once an endpoint is in state EP_JOINED, an exit function is scheduled to
     * be run to make sure the endpoint is completely disconnected from the
     * daemon router.  This Exit() function will set the endpoint state to
     * EP_DONE when its job is done.
     *
     * If the endpoint is in state EP_DONE, the endpoint management thread will
     * remove it from the running endpoint list and delete it.
     */
    enum EndpointState {
        EP_ILLEGAL = 0,
        EP_INITIALIZED,      /**< The endpoint structure has been allocated but not used */
        EP_ACTIVE_STARTED,   /**< The endpoint has begun the process of coming up, due to a passive connection request */
        EP_PASSIVE_STARTED,  /**< The endpoint has begun the process of coming up, due to a passive connection request */
        EP_STARTED,          /**< The endpoint is ready for use, registered with daemon, maybe threads wandering thorugh */
        EP_STOPPING,         /**< The endpoint is stopping but join has not been called */
        EP_WAITING,          /**< Waiting for ARDP to send any queued data before closing (disconnecting) */
        EP_JOINED,           /**< The endpoint is stopping and join has been called */
        EP_DONE              /**< Threads have been shut down and joined */
    };

#ifndef NDEBUG
    void PrintEpState(const char* prefix)
    {
        switch (m_epState) {
        case EP_INITIALIZED:
            QCC_DbgPrintf(("%s: EP_INITIALIZED", prefix));
            break;

        case EP_ACTIVE_STARTED:
            QCC_DbgPrintf(("%s: EP_ACTIVE_STARTED", prefix));
            break;

        case EP_PASSIVE_STARTED:
            QCC_DbgPrintf(("%s: EP_PASSIVE_STARTED", prefix));
            break;

        case EP_STARTED:
            QCC_DbgPrintf(("%s: EP_STARTED", prefix));
            break;

        case EP_STOPPING:
            QCC_DbgPrintf(("%s: EP_STOPPING", prefix));
            break;

        case EP_WAITING:
            QCC_DbgPrintf(("%s: EP_WAITING", prefix));
            break;

        case EP_JOINED:
            QCC_DbgPrintf(("%s: EP_JOINED", prefix));
            break;

        case EP_DONE:
            QCC_DbgPrintf(("%s: EP_DONE", prefix));
            break;

        default:
            QCC_DbgPrintf(("%s: Bad state", prefix));
            assert(false && "_Endpoint::PrintEpState(): Bad state");
            break;
        }
    }
#endif

    /**
     * Connections can either be created as a result of incoming or outgoing
     * connection requests.  If a connection happens as a result of a Connect()
     * it is the active side of a connection.  If a connection happens because
     * of an accept of an inbound ARDP SYN it is the passive side of an ARDP
     * connection.  This is important because of reference counting of
     * bus-to-bus endpoints.  The daemon calls Connect() or ARDP calls
     * AcceptCb() to form connections.  The daemon actually never calls
     * disconnect, it removes a final reference to a remote endpoint.  ARDP
     * does, however call a disconnect callback.
     */
    enum SideState {
        SIDE_ILLEGAL = 0,
        SIDE_INITIALIZED,    /**< This endpoint structure has been allocated but don't know if active or passive yet */
        SIDE_ACTIVE,         /**< This endpoint is the active side of a connection */
        SIDE_PASSIVE         /**< This endpoint is the passive side of a connection */
    };

    /**
     * Construct a remote endpoint suitable for the UDP transport.
     */
    _UDPEndpoint(UDPTransport* transport,
                 BusAttachment& bus,
                 bool incoming,
                 const qcc::String connectSpec) :
        _RemoteEndpoint(bus, incoming, connectSpec, NULL, transport->GetTransportName(), false, true),
        m_transport(transport),
        m_stream(NULL),
        m_handle(NULL),
        m_conn(NULL),
        m_id(0),
        m_ipAddr(),
        m_ipPort(0),
        m_suddenDisconnect(false),
        m_registered(false),
        m_sideState(SIDE_INITIALIZED),
        m_epState(EP_INITIALIZED),
        m_tStart(qcc::Timespec(0)),
        m_remoteExited(false),
        m_exitScheduled(false),
        m_disconnected(false),
        m_refCount(0),
        m_pushCount(0),
        m_stateLock(),
        m_wait(true)
    {
        QCC_DbgHLPrintf(("_UDPEndpoint::_UDPEndpoint(transport=%p, bus=%p, incoming=%d., connectSpec=\"%s\")",
                         transport, &bus, incoming, connectSpec.c_str()));
    }

    /**
     * Destroy a UDP transport remote endpoint.
     */
    virtual ~_UDPEndpoint()
    {
        QCC_DbgHLPrintf(("_UDPEndpoint::~_UDPEndpoint()"));
        QCC_DbgHLPrintf(("_UDPEndpoint::~_UDPEndpoint(): m_refCount==%d.", m_refCount));
        QCC_DbgHLPrintf(("_UDPEndpoint::~_UDPEndpoint(): m_pushCount==%d.", m_pushCount));

        /*
         * Double check that the remote endpoint is sure that its threads are gone,
         * since our destructor is going to call its Stop() and Join() anyway.
         * before deleting it.
         */
        _RemoteEndpoint::Stop();
        _RemoteEndpoint::Exited();
        _RemoteEndpoint::Join();

        assert(IncrementAndFetch(&m_refCount) == 1 && "_UDPEndpoint::~_UDPEndpoint(): non-zero reference count");
        assert(IncrementAndFetch(&m_pushCount) == 1 && "_UDPEndpoint::~_UDPEndpoint(): non-zero PushMessage count");

        /*
         * Make sure that the endpoint isn't in a condition where a thread might
         * conceivably be wandering around in it.  At this point, if everything
         * is working as expected there should be no reason for taking a lock,
         * but then again, if everything is working there also should be no
         * reason for an assert.
         */
        if (m_stream) {
            /*
             * If we have gotten to this point, there certainly must have been a
             * call to Stop() which must have called the stream Disconnect().  This
             * means that it is safe to call delete the stream.  We double-check
             * that there are no threads waiting on completions of send operations
             * and the ARDP connection is/was disconnected.
             */

            assert(m_stream->ThreadSetEmpty() != false && "_UDPEndpoint::~_UDPEndpoint(): Threads present during destruction");
            assert(m_stream->GetDisconnected() && "_UDPEndpoint::~_UDPEndpoint(): Not disconnected");
        }

        DestroyStream();
    }

    /**
     * This is to absolutely, positively ensure that there are no threads
     * wandering around in an endpoint, or are about to start wandering around
     * in an endpoint as it gets destroyed.
     */
    uint32_t IncrementRefs()
    {
        return IncrementAndFetch(&m_refCount);
    }

    /**
     * This is to absolutely, positively ensure that there are no threads
     * wandering around in an endpoint, or are about to start wandering around
     * in an endpoint as it gets destroyed.
     */
    uint32_t DecrementRefs()
    {
        return DecrementAndFetch(&m_refCount);
    }

    /**
     * Return the number of threads currently exectuting in and under
     * PushMessage().  The endpoint management code cannot make a decision about
     * sending an ARDP disconnect with the PushMessage count nonzero, since even
     * though there are no messages in-flight, PushMessage might continue
     * running and do a send which would be lost if an ARDP disconnect was done
     * before the ARDP_Send actually acknowledged the bits.
     */
    uint32_t GetPushMessageCount()
    {
        return m_pushCount;
    }

    /**
     * Override Start() since we are not going to hook in IOdispatch or start TX and
     * RX threads or anything like that.
     */
    QStatus Start()
    {
        IncrementAndFetch(&m_refCount);

        /*
         * Whenever we change state, we need to protect against multiple threads
         * trying to do something at the same time.  Since state changes may be
         * initiated on threads that know nothing about our endpionts and what
         * state they are really in, we need to lock the endpoint list to make
         * sure nothing is changed out from under us.  We are careful to keep
         * this lock order the same "everywhere."  Since we are often called
         * from endpoint management code that holds the endpoint list lock, we
         * take that one first (reentrancy is enabled so we get it if we already
         * hold it).
         */
        m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);
        m_stateLock.Lock(MUTEX_CONTEXT);

        /*
         * What state are are we coming from?
         */
#ifndef NDEBUG
        PrintEpState("_UdpEndpoint::Start()");
#endif

        QCC_DbgHLPrintf(("_UDPEndpoint::Start()"));
        QCC_DbgPrintf(("_UDPEndpoint::Start(): isBusToBus = %s, allowRemote = %s)",
                       GetFeatures().isBusToBus ? "true" : "false",
                       GetFeatures().allowRemote ? "true" : "false"));

        if (m_stream) {
            bool empty = m_stream->ThreadSetEmpty();
            assert(empty && "_UDPEndpoint::Start(): Threads present during Start()");
            if (empty == false) {
                QCC_LogError(ER_FAIL, ("_UDPEndpoint::Start(): Threads present during Start()"));
                m_stateLock.Unlock(MUTEX_CONTEXT);
                m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
                DecrementAndFetch(&m_refCount);
                return ER_FAIL;
            }
        }

        if (GetFeatures().isBusToBus) {
            QCC_DbgPrintf(("_UDPEndpoint::Start(): endpoint switching to ENDPOINT_TYPE_BUS2BUS"));
            SetEndpointType(ENDPOINT_TYPE_BUS2BUS);
        }

#ifndef NDEBUG
        /*
         * Debug consistency check.  If we are starting an endpoint it must be
         * on either the m_authList or the m_endpointList exactly once, and it
         * must be associated with an ARDP connection.
         */
        uint32_t found = 0;
        for (set<UDPEndpoint>::iterator i = m_transport->m_authList.begin(); i != m_transport->m_authList.end(); ++i) {
            UDPEndpoint ep = *i;
            if (GetConnId() == ep->GetConnId()) {
                QCC_DbgPrintf(("_UDPEndpoint::Start(): found endpoint with conn ID == %d. on m_authList", GetConnId()));
                ++found;
            }
        }
        for (set<UDPEndpoint>::iterator i = m_transport->m_endpointList.begin(); i != m_transport->m_endpointList.end(); ++i) {
            UDPEndpoint ep = *i;
            if (GetConnId() == ep->GetConnId()) {
                QCC_DbgPrintf(("_UDPEndpoint::Start(): found endpoint with conn ID == %d. on m_endpointList", GetConnId()));
                ++found;
            }
        }

        assert(found == 1 && "_UDPEndpoint::Start(): Endpoint not on exactly one pending list");
#endif

        /*
         * No threads to Start(), so we jump right to started state.
         */
        assert((IsEpPassiveStarted() || IsEpActiveStarted()) && "_UDPEndpoint::Start(): Endpoint not pre-started corrrectly");
        SetEpStarted();

        /*
         * We need to hook back into the router and do what RemoteEndpoint would have
         * done had we really started RX and TX threads.  Since we know an instance of
         * this object is on exactly one of our endpoint lists, we'll get a reference
         * to a valid object here.
         */
        SetStarted(true);
        BusEndpoint bep = BusEndpoint::wrap(this);

        /*
         * We know we hold a reference, so now we can call out to the daemon
         * with it.  We also never call back out to the daemon with a lock held
         * since you really never know what it might do.  We do keep the thread
         * reference count bumped since there is a thread that will wander back
         * out through here eventually.
         */
        m_stateLock.Unlock(MUTEX_CONTEXT);
        m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

        QCC_DbgPrintf(("_UDPEndpoint::Start(): RegisterEndpoint()"));
        QStatus status = m_transport->m_bus.GetInternal().GetRouter().RegisterEndpoint(bep);
        if (status == ER_OK) {
            m_registered = true;
        }

        DecrementAndFetch(&m_refCount);
        return status;
    }

    /**
     * Perform the AllJoyn thread lifecycle Stop() operation.  Unlike the
     * standard method, Stop() can be called multiple times in this transport
     * since not all operations are serialized through a single RemoteEndpoint
     * ThreadExit.
     *
     * Override RemoteEndpoint::Stop() since we are not going to unhook
     * IOdispatch or stop TX and RX threads or anything like that.
     */
    QStatus Stop()
    {
        IncrementAndFetch(&m_refCount);
        QCC_DbgHLPrintf(("_UDPEndpoint::Stop()"));
        QCC_DbgPrintf(("_UDPEndpoint::Stop(): Unique name == %s", GetUniqueName().c_str()));

        /*
         * Whenever we change state, we need to protect against multiple threads
         * trying to do something at the same time.  Since state changes may be
         * initiated on threads that know nothing about our endpionts and what
         * state they are really in, we need to lock the endpoint list to make
         * sure nothing is changed out from under us.  We are careful to keep
         * this lock order the same "everywhere."  Since we are often called
         * from endpoint management code that holds the endpoint list lock, we
         * take that one first (reentrancy is enabled so we get it if we already
         * hold it).
         */
        m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);
        m_stateLock.Lock(MUTEX_CONTEXT);

        /*
         * What state are are we coming from?
         */
#ifndef NDEBUG
        PrintEpState("_UdpEndpoint::Stop()");
#endif

        /*
         * If we've never been completely started, there are not threads running
         * around inside us so all we have to do is to perform an EarlyExit() to
         * ensure that disconnect status is correct and wake up the endpoint
         * management thread whose resposibility it is for actually dealing with
         * Stop()ped endpoints.  If we're doing an early exit we do not expect
         * there to be any threads waiting to be stopped.
         */
        if (IsEpInitialized() || IsEpActiveStarted() || IsEpPassiveStarted()) {
            QCC_DbgPrintf(("_UDPEndpoint::Stop(): Never Start()ed"));
            if (m_stream) {
                assert(m_stream->ThreadSetEmpty() == true && "_UDPEndpoint::Stop(): Inactive endpoint with threads?");
                m_stream->EarlyExit();
            }

            /*
             * We were never started, so presumably there is no data buffered
             * and waiting to be sent.  Disable the EP_WAITING state since it is
             * completely unnecessary.
             */
            SetEpWaitEnable(false);
            SetEpStopping();
            m_stateLock.Unlock(MUTEX_CONTEXT);
            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

            m_transport->m_manage = UDPTransport::STATE_MANAGE;
            m_transport->Alert();

            DecrementAndFetch(&m_refCount);
            return ER_OK;
        }

        /*
         * If we're already on the way toward being shut down or are actually
         * shut down, there's nothing to do since this function has already been
         * called at some earlier time.  We will bug the endpoint managment
         * thread again to make sure it knows about us.  Don't set the state to
         * EP_STOPPING here or it will possibly overwrite other completed work
         * if Stop() is called multiple times.
         */
        if (IsEpStopping() || IsEpWaiting() || IsEpJoined() || IsEpDone()) {
            QCC_DbgPrintf(("_UDPEndpoint::Stop(): Already stopping, waiting, joined or done"));
            m_stateLock.Unlock(MUTEX_CONTEXT);
            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

            m_transport->m_manage = UDPTransport::STATE_MANAGE;
            m_transport->Alert();

            DecrementAndFetch(&m_refCount);
            return ER_OK;
        }

        /*
         * By a process of elimination, the only possible state we could be in
         * is EP_STARTED.  Did someone add a state and forget to handle it here?
         */
        assert(IsEpStarted() == true && "_UDPEndpoint::Stop(): Endpoint expected to be in EP_STARTED state at this point");
        QCC_DbgPrintf(("_UDPEndpoint::Stop(): Stopping while IsEpStarted()"));

#ifndef NDEBUG
        /*
         * Debug consistency check.  If we are stopping an endpoint it must be
         * on either the m_authList or the m_endpointList exactly once, and it
         * must be associated with an ARDP connection.
         */
        uint32_t found = 0;
        for (set<UDPEndpoint>::iterator i = m_transport->m_authList.begin(); i != m_transport->m_authList.end(); ++i) {
            UDPEndpoint ep = *i;
            if (GetConnId() == ep->GetConnId()) {
                QCC_DbgPrintf(("_UDPEndpoint::Stop(): found endpoint with conn ID == %d. on m_authList", GetConnId()));
                ++found;
            }
        }
        for (set<UDPEndpoint>::iterator i = m_transport->m_endpointList.begin(); i != m_transport->m_endpointList.end(); ++i) {
            UDPEndpoint ep = *i;
            if (GetConnId() == ep->GetConnId()) {
                QCC_DbgPrintf(("_UDPEndpoint::Stop(): found endpoint with conn ID == %d. on m_endpointList", GetConnId()));
                ++found;
            }
        }

        assert(found == 1 && "_UDPEndpoint::Stop(): Endpoint not on exactly one pending list");
#endif
        /*
         * Set the state of the endpoint to stopping.  This will prevent any
         * more PushMessage calls from starting operations in the endpoint and
         * will prevent any additional inbound data from being dispatched to the
         * Daemon Router.
         */
        SetEpStopping();

        m_transport->m_manage = UDPTransport::STATE_MANAGE;
        m_transport->Alert();

        m_stateLock.Unlock(MUTEX_CONTEXT);
        m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return ER_OK;
    }

    /**
     * Perform the AllJoyn thread lifecycle Join() operation.  Join() can be called
     * multiple times.
     */
    QStatus Join()
    {
        IncrementAndFetch(&m_refCount);
        QCC_DbgHLPrintf(("_UDPEndpoint::Join()"));
        QCC_DbgPrintf(("_UDPEndpoint::Join(): Unique name == %s", GetUniqueName().c_str()));

        /*
         * Whenever we change state, we need to protect against multiple threads
         * trying to do something at the same time.  Since state changes may be
         * initiated on threads that know nothing about our endpionts and what
         * state they are really in, we need to lock the endpoint list to make
         * sure nothing is changed out from under us.  We are careful to keep
         * this lock order the same "everywhere."  Since we are often called
         * from endpoint management code that holds the endpoint list lock, we
         * take that one first (reentrancy is enabled so we get it if we already
         * hold it).
         */
        m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);
        m_stateLock.Lock(MUTEX_CONTEXT);

        /*
         * What state are are we coming from?
         */
#ifndef NDEBUG
        PrintEpState("_UdpEndpoint::Join()");
#endif

        /*
         * If we've never been completely started, there are not threads running
         * around inside us so all we have to do is to perform an (actually
         * redundant) EarlyExit() to ensure that disconnect status is correct and
         * wake up the endpoint management thread whose resposibility it is for
         * actually dealing with Join()ed endpoints.  If we're doing an early exit
         * we do not expect there to be any threads waiting to be stopped.
         */
        if (IsEpInitialized() || IsEpActiveStarted() | IsEpPassiveStarted()) {
            QCC_DbgPrintf(("_UDPEndpoint::Join(): Never Start()ed"));
            if (m_stream) {
                m_stream->EarlyExit();
                assert(m_stream->ThreadSetEmpty() == true && "_UDPEndpoint::Join(): Inactive endpoint with threads?");
            }
            SetEpJoined();

            /*
             * Tell the endpoint management code that something has happened
             * that it may be concerned about.
             */
            m_transport->m_manage = UDPTransport::STATE_MANAGE;
            m_transport->Alert();

            m_stateLock.Unlock(MUTEX_CONTEXT);
            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
            DecrementAndFetch(&m_refCount);
            return ER_OK;
        }

        /*
         * The AllJoyn threading model requires that we allow multiple calls to
         * Join().  We expect that the first time through the state will be
         * EP_STOPPING, in which case we may have things to do.  Once we have
         * done a successful Join(), the state will be EP_JOINED or eventually
         * EP_DONE all of which mean we have nothing left to do.  Don't update
         * state here or we may overwrite other work that has previously taken
         * us to EP_DONE.
         */
        if (IsEpJoined() || IsEpDone()) {
            QCC_DbgPrintf(("_UDPEndpoint::Join(): Already Join()ed"));
            m_stateLock.Unlock(MUTEX_CONTEXT);
            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
            DecrementAndFetch(&m_refCount);
            return ER_OK;
        }

        /*
         * By a process of elimination, we are either in EP_STARTED, EP_WAITING
         * or EP_STOPPING state.  It is a programming error to call Join()
         * without first calling Stop().  Calling Stop() will put us in either
         * EP_STOPPING or EP_WAITING state.
         */
        assert((IsEpStopping() || IsEpWaiting()) && "_UDPEndpoint::Join(): Stop() not previously called or unexpected state");
        QCC_DbgPrintf(("_UDPEndpoint::Join(): Join() from IsEpStopping() or IsEpWaiting()"));

        /*
         * The only way Join() will be called while we are in EP_WAITING state
         * is if the Transport itself starts shutting down.  In this case, it is
         * pointless to wait until any outstanding writes are finished, but we
         * do need to do the disconnect and switch to EP_STOPPED.  Calling
         * Disconnect will deal with causing ARDP to abort any outstanding
         * transactions.  This last minute change of plans may cause us to poll
         * for completion of this cleanup in the sleep loop below.
         */
        if (IsEpWaiting()) {
            QCC_DbgPrintf(("_UDPEndpoint::Join(): Join() from IsEpWaiting()"));
            ArdpStream* stream = GetStream();
            if (stream) {
                QCC_DbgPrintf(("_UDPEndpoint::Join(): Local stream->Disconnect()"));
                stream->Disconnect(false, ER_UDP_LOCAL_DISCONNECT);
            }
            SetEpWaitEnable(false);
            SetEpStopping();
        }

        /*
         * If there were any threads blocked waiting to get bytes through to a
         * remote host, they should have been woken up in Stop() and in the
         * normal course of events they should have woken up and left of their
         * own accord.  ManageEndpoints should have waited for that to happen
         * before calling Join().  If we happen to get caught with active
         * endpoints alive when the TRANSPORT is shutting down, however, we may
         * have to wait for that to happen here.  We can't do anything until
         * there are no threads wandering around in the endpoint or risk
         * crashing the daemon.  We just have to trust that they will cooperate.
         */
        int32_t timewait = m_transport->m_ardpConfig.timewait;
        while (m_stream && m_stream->ThreadSetEmpty() == false) {
            QCC_DbgPrintf(("_UDPEndpoint::Join(): Waiting for threads to exit"));

            /*
             * Make sure the threads are "poked" to wake them up.
             */
            m_stream->WakeThreadSet();

            /*
             * Note that we are calling Sleep() with both the endpoint list lock
             * and the state lock taken.  This is dangerous from a deadlock
             * point of view, but the threads that we want to wake up are
             * waiting on an event in the ArdpStream associated with the
             * endpoint.  They will never ask for one of our locks, so they
             * won't deadlock.  What can happen is that we block either the
             * maintenance thread or the dispatcher for timwait milliseconds
             * (typically one second).  This sounds bad, but we have added code
             * in the maintenance theread to wait until these threads are gone
             * beore calling Join() so in the normal course of events, this code
             * should never be executed.  It is here to make sure threads are
             * gone before deleting the endpoint in the case of the UDP
             * Transport being torn down unexpectedly.  In that case, it will
             * not be a problem to bock other threads, since they are going away
             * or are already gone.
             */
            qcc::Sleep(10);

            timewait -= 10;
            if (timewait <= 0) {
                QCC_DbgPrintf(("_UDPEndpoint::Join(): TIMWAIT expired with threads pending"));
                break;
            }
        }

        /*
         * The same story as in the comment above applies to the disconnect callback.
         * We expect that in the normal course of events, the endpoint management
         * thread will wait until the disconnection process is complete before
         * actually calling Join.
         */
        if (m_stream && m_stream->GetDisconnected() == false) {
            QCC_LogError(ER_UDP_NOT_DISCONNECTED, ("_UDPEndpoint::Join(): Not disconnected"));
            m_stream->EarlyExit();
        }

        QCC_DbgPrintf(("_UDPEndpoint::Join(): Going to EP_JOINED"));
        SetEpJoined();

        /*
         * Tell the endpoint management code that something has happened that
         * it may be concerned about.
         */
        m_transport->m_manage = UDPTransport::STATE_MANAGE;
        m_transport->Alert();

        m_stateLock.Unlock(MUTEX_CONTEXT);
        m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
        DecrementAndFetch(&m_refCount);
        return ER_OK;
    }

    /*
     * Stop() and Join() are relly internal to the UDP Transport threading model.
     * We can consider ourselves free to call Stop() and Join() from everywhere
     * and anywhere just so long as we don't release our reference to the endpoint
     * until after we are sure that the daemon has no more references to the
     * endpoint.
     *
     * The last thing we need to do is to arrange for all references to the
     * endpoint to be removed by calling DaemonRouter::UnregisterEndpoint().
     * Unfortunately, the RemoteEndpoint has the idea of TX and RX threading
     * calling out into streams ingrained into it, so we can't just do this
     * shutdown ourselves.  We really have to call into the RemoteEndpoint().
     * We have an Exit() function there that does what needs to be done in the
     * case of no threads to call an exit callback, but we also have to be very
     * careful about calling it from any old thread context since
     * UnregisterEndoint() will need to take the daemon name table lock, which
     * is often held during call-outs.  To avoid deadlocks, we need to esure
     * that calls to Exit() are done on our dispatcher thread which we know will
     * not be holding any locks.
     */
    QStatus Exit()
    {
        IncrementAndFetch(&m_refCount);
        QCC_DbgHLPrintf(("_UDPEndpoint::Exit()"));

        /*
         * Whenever we change state, we need to protect against multiple threads
         * trying to do something at the same time.  We have to be careful since
         * _RemoteEndpoint can happily call out to the daemon or call back into
         * our endpoint.  Don't take any locks while the possibility exists of
         * the daemon wandering off and doing something.  Whatever it is doing
         * the endpoint management code will hold a reference to the endpoint
         * until Exit() completes so we let the daemon go and grab the locks
         * after it returns.  Note that we do increment the thread reference
         * count that indicates that a thread is wandering around and will pop
         * back up through this function sometime later.
         */

        _RemoteEndpoint::Exit();
        _RemoteEndpoint::Stop();
        m_remoteExited = true;
        m_registered = false;

        m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);
        m_stateLock.Lock(MUTEX_CONTEXT);

        /*
         * Jump to done state.  Our ManageEndpoints() will pick up on the fact
         * that this endpoint is done and deal with it by releasing any
         * references to it.
         */
        QCC_DbgPrintf(("_UDPEndpoint::Exit(): Going to EP_DONE state"));
        SetEpDone();

        /*
         * Tell the endpoint management code that something has happened that
         * it may be concerned about.
         */
        m_transport->m_manage = UDPTransport::STATE_MANAGE;
        m_transport->Alert();

        m_stateLock.Unlock(MUTEX_CONTEXT);
        m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
        DecrementAndFetch(&m_refCount);
        return ER_OK;
    }

    /**
     * Get a pointer to the UDP Transport that owns this Endpoint.
     */
    UDPTransport* GetTransport()
    {
        return m_transport;
    }

    /**
     * Get the boolean indication that the RemoteEndpoint exit function has been
     * called.
     */
    bool GetExited()
    {
        QCC_DbgHLPrintf(("_UDPEndpoint::GetExited(): -> %s", m_remoteExited ? "true" : "false"));
        return m_remoteExited;
    }

    /**
     * Set the boolean indication that the RemoteEndpoint exit function has been
     * scheduled.
     */
    void SetExitScheduled()
    {
        QCC_DbgHLPrintf(("_UDPEndpoint::SetExitScheduled()"));
        m_exitScheduled = true;
    }

    /**
     * Get the boolean indication that the RemoteEndpoint exit function has been
     * scheduled.
     */
    bool GetExitScheduled()
    {
        QCC_DbgHLPrintf(("_UDPEndpoint::GetExitScheduled(): -> %s", m_exitScheduled ? "true" : "false"));
        return m_exitScheduled;
    }

    /**
     * Get a boolean indication that the endpoint has been registered with the
     * daemon.
     */
    bool GetRegistered()
    {
        QCC_DbgHLPrintf(("_UDPEndpoint::GetRegistered(): -> %s", m_registered ? "true" : "false"));
        return m_registered;
    }

    /**
     * Create a skeletal stream that we'll use basically as a place to hold some
     * connection information.
     */
    void CreateStream(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t dataTimeout, uint32_t dataRetries)
    {
        IncrementAndFetch(&m_refCount);
        QCC_DbgHLPrintf(("_UDPEndpoint::CreateStream(handle=%p, conn=%p)", handle, conn));

        m_transport->m_ardpLock.Lock();
        assert(m_stream == NULL && "_UDPEndpoint::CreateStream(): stream already exists");

        /*
         * The stream for a UDP endpoint is basically just a convenient place to
         * stick the connection identifier.  For the TCP transport it is a real
         * stream that connects to an underlying socket stream.
         */
        m_stream = new ArdpStream();
        m_stream->SetTransport(m_transport);
        m_stream->SetEndpoint(this);
        m_stream->SetHandle(handle);
        m_stream->SetConn(conn);

        /*
         * This is actually a call to the underlying endpoint that provides the
         * stream for Marshaling and unmarshaling.  This is what hooks our
         * PushMessage() back into the ArdpStream PushBytes().
         */
        SetStream(m_stream);
        m_transport->m_ardpLock.Unlock();
        DecrementAndFetch(&m_refCount);
    }

    /**
     * Get the ArdpStream* pointer to the skeletal stream associated with this
     * endpoint.
     */
    ArdpStream* GetStream()
    {
        QCC_DbgTrace(("_UDPEndpoint::GetStream() => %p", m_stream));
        return m_stream;
    }

    /**
     * Delete the skeletal stream that we used to stash our connection
     * information.
     */
    void DestroyStream()
    {
        IncrementAndFetch(&m_refCount);
        QCC_DbgHLPrintf(("_UDPEndpoint::DestroyStream()"));
        if (m_stream) {
            assert(m_stream->GetConn() == NULL && "_UDPEndpoint::DestroyStream(): Cannot destroy stream unless stream's m_conn is NULL");
            m_stream->SetHandle(NULL);
            delete m_stream;
        }
        m_stream = NULL;
        m_conn = NULL;
        DecrementAndFetch(&m_refCount);
    }

    /**
     * Take a Message destined to be send over the connection represented
     * by the UDP Endpoint and ask it to Deliver() itself though this
     * remote endpoint (we are a descendent).  DeliverNonBlocking() will
     * end up calling PushBytes() on the Stream Sink associated with the
     * endpoint.  This will find its way down to the PushBytes() defined
     * in our ARDP Stream.
     *
     * We have a bit of a tricky situation in that AllJoyn likes to send
     * fire-and-forget messages -- signals.  It sometimes wants to make a
     * connection and send a bunch of signals and then disconnect.  We can't
     * just call ARDP_Disconnect with signals queued for transmission which
     * would send RST and tear down the connection before those signals get
     * sent.  Method calls are okay, since the caller needs to wait for the
     * return values, which indicates they were sent (and responded to) but
     * signals are a different beast.  We need to simulate TCP's half-close
     * behavior.  This means our version of a close needs to wait for ARDP to
     * send any queued messages before calling ARDP_Disconnect.
     *
     * A client will use JoinSession to drive the UDPTransport::Connect() that
     * we, in turn, use to drive ARDP_Connect().  When an AllJoyn client is
     * done with a connection it will call LeaveSession().  This will not call
     * a disconnect function, but will cause the associated endpoint to be
     * invalidated and will call UDPEndpoint::Stop().  This call to Stop()
     * begins the endpoint shutdown process, which must disallow any further
     * writes (enforce a SHUTDOWN_WR-like behavior) but keep the underlying
     * connection up until all data has been sent and acknowledged.  At that
     * point, the ARDP_Disconnect() can be called and the endpoint actually
     * torn down.  There is no Shutdown() on an Endpoint that causes it to
     * behave like a TCP state machine, so we get to implement that behavior
     * ourselves.
     *
     * This affects us here in that we have to respect a stopping indication on
     * the UDPEndpoint (as well as the transport).  Since the message passed to
     * us is going to wander down through the endpoint into the stream and then
     * on out ARDP, we have to admit the possibility that even though ARDP may
     * not have heard about a message passing through here, it is in our system.
     * Therefore, we have a counter that indicates how many calls to
     * PushMessage() are in process on this endpoint.  If this counter is
     * nonzero we have to treat this case as if a Message were in flight.  It
     * may turn out that down in the ARDPStream we detect that the endpoint is
     * closing, but we need to include coverage for the in-process case here.
     *
     * The Endpoint Management thread is what processes the closing of
     * connections and the disposition of endpoints, so it will have to wait
     * until the in-flight message count goes to zero and also the
     * PushMessage()s in-process goes to zero before executing a
     * ARDP_Disconnect().
     */
    QStatus PushMessage(Message& msg)
    {
        /*
         * Increment the counter indicating that another thread is running
         * inside PushMessage for the current endpoint.  THis is different from
         * the counter indicating "something is executing inside the endpoint"
         * bumped below.
         */
        IncrementAndFetch(&m_pushCount);

        IncrementAndFetch(&m_refCount);
        QCC_DbgTrace(("_UDPEndpoint::PushMessage(msg=%p)", &msg));

        /*
         * If either the transport or the endpoint is not in a state where it
         * indicates that it is ready to send, we need to return an error.
         *
         * Unfortunately, the system will try to do all kinds of things to an
         * endpoint even though it has just stopped it.  To prevent cascades of
         * error logs, we need to return a "magic" error code and not log an
         * error if we detect that either the transport or the endpoint is
         * shutting down.  Higher level code (especially AllJoynObj) will look
         * for this error and not do any logging if it is a transient error
         * during shutdown, as identified by the error return value
         * ER_BUS_ENDPOINT_CLOSING.
         */
        if (m_transport->IsRunning() == false || m_transport->m_stopping == true) {
            QStatus status = ER_BUS_ENDPOINT_CLOSING;
            DecrementAndFetch(&m_refCount);
            DecrementAndFetch(&m_pushCount);
            return status;
        }

        if (IsEpStarted() == false) {
            QStatus status = ER_BUS_ENDPOINT_CLOSING;
            DecrementAndFetch(&m_refCount);
            DecrementAndFetch(&m_pushCount);
            return status;
        }

        /*
         * We need to make sure that this endpoint stays on one of our endpoint
         * lists while we figure out what to do with it.  If we are taken off
         * the endpoint list we could actually be deleted while doing this
         * operation, so take the lock to make sure at least the UDP transport
         * holds a reference during this process.
         */
        m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);

        /*
         * Find the managed endpoint to which the connection ID of the current
         * object refers.  If the endpoint state was EP_STARTED above, and we
         * hold the endpoint lock, we should find the endpoint on the list.
         */
        uint32_t found = 0;
        for (set<UDPEndpoint>::iterator i = m_transport->m_endpointList.begin(); i != m_transport->m_endpointList.end(); ++i) {
            UDPEndpoint ep = *i;
            if (GetConnId() == ep->GetConnId()) {
                QCC_DbgPrintf(("_UDPEndpoint::PushMessage(): found endpoint with conn ID == %d. on m_endpointList", GetConnId()));
                ++found;
            }
        }

        if (found == 0) {
            QCC_LogError(ER_UDP_ENDPOINT_REMOVED, ("_UDPEndpoint::PushMessage(): Endpoint is gone"));
            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
            DecrementAndFetch(&m_refCount);
            DecrementAndFetch(&m_pushCount);
            return ER_UDP_ENDPOINT_REMOVED;
        }

        /*
         * Since we know an instance of this object is on our endpoint list,
         * we'll get a reference to a valid object here.
         */
        RemoteEndpoint rep = RemoteEndpoint::wrap(this);

        /*
         * If we are going to pass the Message off to be delivered, the act of
         * delivering will change the write state of the message.  Since
         * delivering to a multipoint session is done by taking a Message and
         * sending it off to multiple endpoints for delivery, if we just use the
         * Message we are given, we will eventually change the writeState of the
         * message to MESSAGE_COMPLETE when we've pushed all of the bits.  That
         * would cause any subsequent PushMessage calls to complete before
         * actually writing any bits since they would think they are done.  This
         * means we have to do a deep copy of every message before we send it.
         */
        Message msgCopy = Message(msg, true);

        /*
         * We know we hold a reference, so now we can call out to the daemon
         * with it.  Even if we release the endpoint list lock, our thread will
         * be registered in the endpoint so it won't go away.  The message
         * handler should call right back into our stream and we should pop back
         * out in short order.
         */
        m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
        QCC_DbgPrintf(("_UDPEndpoint::PushMessage(): DeliverNonBlocking()"));
        QStatus status = msgCopy->DeliverNonBlocking(rep);
        QCC_DbgPrintf(("_UDPEndpoint::PushMessage(): DeliverNonBlocking() returns \"%s\"", QCC_StatusText(status)));
        DecrementAndFetch(&m_refCount);
        DecrementAndFetch(&m_pushCount);
        return status;
    }

    /**
     * Callback letting us know that our connection has been disconnected for
     * some reason.
     *
     * For deadlock avoidance purposes, this callback always comes from the
     * transport dispatcher thread.
     *
     * It is important to realize that this callback is not called in the
     * context of the ARDP thread, it is dispatched from that callback on
     * another thread.  Therefore it is possible that during the dispatch time,
     * the connection may have beome invalid.  In order to remove ourselves from
     * this possibility, we plumb through the connection ID that was given to us
     * in the original callback.  Since the dispatcher used that ID to look us
     * up via our saved connection ID they should be the same and we can treat
     * this as a unique identifier for the purposes of aligning, looking up and
     * releasing resources even if the underlying connection that corresponded
     * to this ID may have dropped out from under us.
     */
    void DisconnectCb(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t connId, QStatus status)
    {
        IncrementAndFetch(&m_refCount);
        QCC_DbgHLPrintf(("_UDPEndpoint::DisconnectCb(handle=%p, conn=%p)", handle, conn));

        /*
         * We need to look and see if this endpoint is on the endopint list
         * and then make sure that it stays on the list, so take the lock to make sure
         * at least the UDP transport holds a reference during this process.
         */
        m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);

#ifndef NDEBUG
        /*
         * The callback dispatcher looked to see if the endpoint was on the
         * endpoint list before it made the call here, and it incremented the
         * thread reference count before calling.  We should find an endpoint
         * still on the endpoint list since the management thread should not
         * remove the endpoint with threads wandering around in it.
         */
        uint32_t found = 0;
        for (set<UDPEndpoint>::iterator i = m_transport->m_endpointList.begin(); i != m_transport->m_endpointList.end(); ++i) {
            UDPEndpoint ep = *i;
            if (GetConnId() == ep->GetConnId()) {
                assert(connId == GetConnId() && "_UDPEndpoint::DisconnectCb(): Inconsistent connId");
                QCC_DbgPrintf(("_UDPEndpoint::DisconnectCb(): found endpoint with conn ID == %d. on m_endpointList", GetConnId()));
                ++found;
            }
        }
        assert(found == 1 && "_UDPEndpoint::DisconnectCb(): Endpoint is gone");
#endif

        /*
         * We need to figure out if this disconnect callback is due to an
         * unforeseen event on the network (coming out of the protocol) or if it
         * is a callback in response to a local disconnect.  The key is the
         * reported status will only be ER_OK if the callback is in response to
         * a local disconnect that has already begun through a call to
         * _UDPEndpoint::Stop().  We turn the fact that this is a part of a
         * local disconnect in to the fact that it is not a "sudden" or
         * unexpected callback and give it to the stream Disconnect() function
         * that is going to handle the details of managing the state of the
         * connection.
         */
        bool sudden = (status != ER_OK);
        SetSuddenDisconnect(sudden);
        QCC_DbgPrintf(("_UDPEndpoint::DisconnectCb(): sudden==\"%s\"", sudden ? "true" : "false"));

        /*
         * Always let the stream see the disconnect event.  It is the piece of
         * code that is really dealing with the hard details.
         *
         * Note that calling Disconnect() here and Stop() at the bottom of the
         * function (making a call out to BusConnectionLost() in the middle)
         * creates a transient situation in which the stream thinks it is
         * disconnected and the endpoint thinks it is connected in the sudden
         * (remote) disconnect case.  These two calls, m_stream->Disconnect()
         * and Stop() should really be done "together" and with the stream lock
         * and state lock held.  This was noticed very late in the game, and
         * is now special-cased (checked for) in ArdpStream::PushBytes().
         * This is harmless since ARDP_Send() will return an error since we are
         * trying to send to a disconnected connection.  The point is, though,
         * that the state of the endoint and the state of the stream will be
         * transiently incoherent, so consistency checks will probably fail
         * under stress.
         */
        if (m_stream) {
            QCC_DbgPrintf(("_UDPEndpoint::DisconnectCb(): Disconnect(): m_stream=%p", m_stream));
            QCC_DbgPrintf(("_UDPEndpoint::DisconnectCb(): m_stream->Disconnect() on endpoint with conn ID == %d.", GetConnId()));
            m_stream->Disconnect(sudden, status);
        }

        /*
         * We believe that the connection must go away here since this is either
         * an unsolicited remote disconnection which always results in the
         * connection going away or a confirmation of a local disconnect that
         * also results in the connection going away.
         */
        m_conn = NULL;

#ifndef NDEBUG
        /*
         * If there is no connection, we expect the stream to report that it is
         * disconnected.  If it does not now, there is no way for it to ever
         * become "more disconnected."
         */
        if (m_stream) {
            assert(m_stream->GetDisconnected() && "_UDPEndpoint::DisconnectCb(): Stream not playing by the rules of the game");
        }
#endif

        /*
         * Since we know an instance of this object is on exactly one of our
         * endpoint lists we'll get a reference to a valid object here.  The
         * thread reference count being non-zero means we will not be deleted
         * out from under the call.
         */
        RemoteEndpoint rep = RemoteEndpoint::wrap(this);
        /*
         * Since this is a disconnect it will eventually require endpoint
         * management, so we make a note to run the endpoint management code.
         */
        m_transport->m_manage = UDPTransport::STATE_MANAGE;
        m_transport->Alert();

        /*
         * Never, ever call out to the daemon with a lock taken.  You will
         * eventually regret it.
         */
        m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

        /*
         * Tell any listeners that the connection was lost.  Since we have a
         * disconnect callback we know that the connection is gone, one way
         * or the other.
         */
        if (m_transport->m_listener) {
            m_transport->m_listener->BusConnectionLost(rep->GetConnectSpec());
        }

        /*
         * The connection is gone, so Stop() so it can continue being torn down
         * by the daemon router (and us).  This may have already been done in
         * the case of a local disconnect callback.  It was the original Stop()
         * that must have happened that precipitated the confirmation callback
         * from ARDP -- the DisconnectCb that we are running now.
         */
        Stop();

        DecrementAndFetch(&m_refCount);
    }

    /**
     * Callback letting us know that we received data over our connection.  We
     * are passed responsibility for the buffer in this callback.
     *
     * For deadlock avoidance purposes, this callback always comes from the
     * transport dispatcher thread.
     *
     * It is important to realize that this callback is not called in the
     * context of the ARDP thread, it is dispatched from that callback on
     * another thread.  Therefore it is possible that during the dispatch time,
     * the connection may have beome invalid.  In order to remove ourselves from
     * this possibility, we plumb through the connection ID that was given to us
     * in the original callback.  Since the dispatcher used that ID to look us
     * up via our saved connection ID they should be the same and we can treat
     * this as a unique identifier for the purposes of aligning, looking up and
     * releasing resources even if the underlying connection that corresponded
     * to this ID may have dropped out from under us.
     */
    void RecvCb(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t connId, ArdpRcvBuf* rcv, QStatus status)
    {
        IncrementAndFetch(&m_refCount);
        QCC_DbgHLPrintf(("_UDPEndpoint::RecvCb(handle=%p, conn=%p, connId=%d., rcv=%p, status=%s)",
                         handle, conn, connId, rcv, QCC_StatusText(status)));

        /*
         * Our contract with ARDP says that it will provide us with valid data
         * if it calls us back.
         */
        assert(rcv != NULL && rcv->data != NULL && rcv->datalen != 0 && "_UDPEndpoint::RecvCb(): No data from ARDP in RecvCb()");

        /*
         * We need to look and see if this endpoint is on the endopint list and
         * then make sure that it stays on the list, so take the lock to make
         * sure at least the UDP transport holds a reference during this
         * process.
         */
        QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): Taking m_endpointListLock"));
        m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);

#ifndef NDEBUG
        /*
         * The callback dispatcher looked to see if the endpoint was on the
         * endpoint list before it made the call here, and it incremented the
         * thread reference count before calling.  We should find an endpoint
         * still on the endpoint list since the management thread should not
         * remove the endpoint with threads wandering around in it.
         */
        uint32_t found = 0;
        for (set<UDPEndpoint>::iterator i = m_transport->m_endpointList.begin(); i != m_transport->m_endpointList.end(); ++i) {
            UDPEndpoint ep = *i;
            if (GetConnId() == ep->GetConnId()) {
                assert(connId == GetConnId() && "_UDPEndpoint::RecvCb(): Inconsistent connId");
                QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): found endpoint with conn ID == %d. on m_endpointList", GetConnId()));
                ++found;
            }
        }
        assert(found == 1 && "_UDPEndpoint::RecvCb(): Endpoint is gone");
#endif

        if (IsEpStarted() == false) {
            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): Not accepting inbound messages"));

#if RETURN_ORPHAN_BUFS

            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): ARDP_RecvReady()"));
            m_transport->m_ardpLock.Lock();

            /*
             * We got a receive callback that includes data destined for an
             * endpoint that is no longer accepting messages.  Most likely it is
             * shutting down.  The details of shutting down the endpoint and
             * discussing those details with ARDP is handled elsewhere, so all
             * we need to do is to return the buffer that we are ingoring.
             *
             * We're in a receive callback, but the callback function is
             * dispatched on another thread that is unrelated to the thread on
             * which the original ARDP callback was fired.  This is important
             * since if we are in the ARDP callback thread the conn is
             * guaranteed to be valid since the function is beign called from
             * ARDP.  In a thread that may be dispatched later this is no longer
             * the case.  The conn may have been torn down between the time the
             * original callback was fired and when the dipatcher has actually
             * run.  If this is the case, we can only trust that ARDP has done
             * the right thing and cleaned up its buffers.  ARDP owns the
             * disposition of the buffers and we did our best -- we called back.
             * If an error is returned, pretty much all we can do is just log or
             * print an error message.  Since we aren't in charge at all, we
             * just print a message if in debug mode.
             */
#ifndef NDEBUG
            QStatus alternateStatus =
#endif
            ARDP_RecvReady(handle, conn, rcv);
#ifndef NDEBUG
            if (alternateStatus != ER_OK) {
                QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): ARDP_RecvReady() returns status==\"%s\"", QCC_StatusText(status)));
            }
#endif
            m_transport->m_ardpLock.Unlock();

#else // not RETURN_ORPHAN_BUFS

            /*
             * Since we are getting an indication that the endpoint is going
             * down, we assume that the endpoint will continue the process and
             * be destroyed.  In this case, we assume that ARDP is going to
             * particpate in closing down the connection and released any
             * pending buffers.  This means that we can just ignore the buffer
             * here.  We'll just print a message saying that's what we did.
             */
            QCC_DbgPrintf(("UDPEndpoint::RecvCb(): Orphaned RECV_CB for conn ID == %d. ignored", connId));

#endif // not RETURN_ORPHAN_BUFS

            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
            DecrementAndFetch(&m_refCount);
            return;
        }

        if (rcv->fcnt == 0) {
            QCC_LogError(ER_UDP_INVALID, ("_UDPEndpoint::RecvCb(): Unexpected rcv->fcnt==%d.", rcv->fcnt));

            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): ARDP_RecvReady()"));
            m_transport->m_ardpLock.Lock();
            /*
             * We got a bogus fragment count and so we will assert this is a
             * bogus condition below.  Don't bother printing an error if ARDP
             * also doesn't take the bogus buffers back.
             */
            ARDP_RecvReady(handle, conn, rcv);
            m_transport->m_ardpLock.Unlock();
            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

            DecrementAndFetch(&m_refCount);
            assert(false && "_UDPEndpoint::RecvCb(): unexpected rcv->fcnt");
            return;
        }

        /*
         * The daemon knows nothing about message fragments, so we must
         * reassemble the fragements into a contiguous buffer before doling it
         * out to the daemon router.  What we get is a singly linked list of
         * ArdpRcvBuf* that we have to walk.  There is no cumulative length, so
         * we have to do two passes through the list: one pass to calculate the
         * length so we can allocate a contiguous buffer, and one to copy the
         * data into the buffer.
         */
        uint8_t* msgbuf = NULL;
        uint32_t mlen = 0;
        if (rcv->fcnt != 1) {
            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): Calculating message length"));
            ArdpRcvBuf* tmp = rcv;
            for (uint32_t i = 0; i < rcv->fcnt; ++i) {
                QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): Found fragment of %d. bytes", tmp->datalen));

                if (tmp->datalen == 0 || tmp->datalen > 65535) {
                    QCC_LogError(ER_UDP_INVALID, ("_UDPEndpoint::RecvCb(): Unexpected tmp->datalen==%d.", tmp->datalen));
                    m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

                    QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): ARDP_RecvReady()"));
                    m_transport->m_ardpLock.Lock();
                    /*
                     * We got a bogus fragment count and so we will assert this
                     * is a bogus condition below.  Don't bother printing an
                     * error if ARDP also doesn't take the bogus buffers back.
                     */
                    ARDP_RecvReady(handle, conn, rcv);
                    m_transport->m_ardpLock.Unlock();

                    DecrementAndFetch(&m_refCount);
                    assert(false && "_UDPEndpoint::RecvCb(): unexpected rcv->fcnt");
                    return;
                }

                mlen += tmp->datalen;
                tmp = tmp->next;
            }

            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): Found Message of %d. bytes", mlen));
#ifndef NDEBUG
            msgbuf  = new uint8_t[mlen + SEAL_SIZE];
            SealBuffer(msgbuf + mlen);
#else
            msgbuf = new uint8_t[mlen];
#endif
            uint32_t offset = 0;
            tmp = rcv;
            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): Reassembling fragements"));
            for (uint32_t i = 0; i < rcv->fcnt; ++i) {
                QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): Copying fragment of %d. bytes", tmp->datalen));
                memcpy(msgbuf + offset, tmp->data, tmp->datalen);
                offset += tmp->datalen;
                tmp = tmp->next;
            }

            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): Message of %d. bytes reassembled", mlen));
        }

        uint8_t* messageBuf = msgbuf ? msgbuf : rcv->data;
        uint32_t messageLen = mlen ? mlen : rcv->datalen;

#ifndef NDEBUG
#if BYTEDUMPS
        DumpBytes(messageBuf, messageLen);
#endif
#endif

        /*
         * Since we know the callback dispatcher verified it could find an
         * intance of this object is on an endpoint lists, and it bumped the
         * thread reference count, we know we'll get a reference to a
         * still-valid object here.
         */
        RemoteEndpoint rep = RemoteEndpoint::wrap(this);
        BusEndpoint bep  = BusEndpoint::cast(rep);

        /*
         * We know we hold a reference that will stay alive until we leave this
         * function, so now we can call out to the daemon all we want.
         */
        m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

        /*
         * The point here is to create an AllJoyn Message from the
         * inbound bytes which we know a priori to contain exactly one
         * Message if present.  We have a back door in the Message code
         * that lets us load our bytes directly into the message.  Note
         * that this LoadBytes does a buffer copy, so we are free to
         * release ownership of the incoming buffer at any time after
         * that.
         */
        Message msg(m_transport->m_bus);
        QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): LoadBytes()"));
        status = msg->LoadBytes(messageBuf, messageLen);
        if (status != ER_OK) {
            QCC_LogError(status, ("_UDPEndpoint::RecvCb(): Cannot load bytes"));

            /*
             * If there's some kind of problem, we have to give the buffer
             * back to the protocol now.
             */
            m_transport->m_ardpLock.Lock();

#ifndef NDEBUG
            QStatus alternateStatus =
#endif
            ARDP_RecvReady(handle, conn, rcv);
#ifndef NDEBUG
            if (alternateStatus != ER_OK) {
                QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): ARDP_RecvReady() returns status==\"%s\"", QCC_StatusText(alternateStatus)));
            }
#endif
            m_transport->m_ardpLock.Unlock();

            /*
             * If we allocated a reassembly buffer, free it too.
             */
            if (msgbuf) {
#ifndef NDEBUG
                CheckSeal(msgbuf + mlen);
#endif
                delete[] msgbuf;
                msgbuf = NULL;
            }

            /*
             * If we do something that is going to bug the ARDP protocol, we
             * need to call back into ARDP ASAP to get it moving.  This is done
             * in the main thread, which we need to wake up.
             */
            m_transport->Alert();
            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * The bytes are now loaded into what amounts to a backing buffer for
         * the Message.  With the exception of the Message header, these are
         * still the raw bytes from the wire, so we have to Unmarshal() them
         * before proceeding (remembering to free the reassembly buffer if it
         * exists.
         */
        if (msgbuf) {
#ifndef NDEBUG
            CheckSeal(msgbuf + mlen);
#endif
            delete[] msgbuf;
            msgbuf = NULL;
        }

        qcc::String endpointName(rep->GetUniqueName());
        QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): Unmarshal()"));
        status = msg->Unmarshal(endpointName, false, false, true, 0);
        if (status != ER_OK) {
            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): Can't Unmarhsal() Message.  Probably duplicate signal delivery"));

            /*
             * If there's some kind of problem, we have to give the buffer
             * back to the protocol now.
             */
            m_transport->m_ardpLock.Lock();

#ifndef NDEBUG
            QStatus alternateStatus =
#endif
            ARDP_RecvReady(handle, conn, rcv);
#ifndef NDEBUG
            if (alternateStatus != ER_OK) {
                QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): ARDP_RecvReady() returns status==\"%s\"", QCC_StatusText(alternateStatus)));
            }
#endif

            m_transport->m_ardpLock.Unlock();

            /*
             * If we do something that is going to bug the ARDP protocol, we
             * need to call back into ARDP ASAP to get it moving.  This is done
             * in the main thread, which we need to wake up.
             */
            m_transport->Alert();
            DecrementAndFetch(&m_refCount);
            return;
        }

        QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): PushMessage()"));
        status = m_transport->m_bus.GetInternal().GetRouter().PushMessage(msg, bep);
        if (status != ER_OK) {
            QCC_LogError(status, ("_UDPEndpoint::RecvCb(): PushMessage failed"));
        }

        /*
         * We're all done with the message we got from ARDP, so we need to let
         * it know that it can reuse the buffer (and open its receive window).
         */
        QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): ARDP_RecvReady()"));
        m_transport->m_ardpLock.Lock();

#ifndef NDEBUG
        QStatus alternateStatus =
#endif
        ARDP_RecvReady(handle, conn, rcv);
#ifndef NDEBUG
        if (alternateStatus != ER_OK) {
            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): ARDP_RecvReady() returns status==\"%s\"", QCC_StatusText(alternateStatus)));
        }
#endif
        m_transport->m_ardpLock.Unlock();

        /*
         * If we do something that is going to bug the ARDP protocol, we need to
         * call back into ARDP ASAP to get it moving.  This is done in the main
         * thread, which we need to wake up.
         */
        m_transport->Alert();
        DecrementAndFetch(&m_refCount);
    }

    /**
     * Callback letting us know that the remote side has acknowledged reception
     * of our data and the buffer can be recycled/freed
     *
     * For deadlock avoidance purposes, this callback always comes from the
     * transport dispatcher thread.
     *
     * It is important to realize that this callback is not called in the
     * context of the ARDP thread, it is dispatched from that callback on
     * another thread.  Therefore it is possible that during the dispatch time,
     * the connection may have beome invalid.  In order to remove ourselves from
     * this possibility, we plumb through the connection ID that was given to us
     * in the original callback.  Since the dispatcher used that ID to look us
     * up via our saved connection ID they should be the same and we can treat
     * this as a unique identifier for the purposes of aligning, looking up and
     * releasing resources even if the underlying connection that corresponded
     * to this ID may have dropped out from under us.
     */
    void SendCb(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t connId, uint8_t* buf, uint32_t len, QStatus status)
    {
        IncrementAndFetch(&m_refCount);
        QCC_DbgHLPrintf(("_UDPEndpoint::SendCb(handle=%p, conn=%p, buf=%p, len=%d.)", handle, conn, buf, len));

        /*
         * We need to look and see if this endpoint is on the endopint list
         * and then make sure that it stays on the list, so take the lock to make sure
         * at least the UDP transport holds a reference during this process.
         */
        m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);

#ifndef NDEBUG
        /*
         * The callback dispatcher looked to see if the endpoint was on the
         * endpoint list before it made the call here, and it incremented the
         * thread reference count before calling.  We should find an endpoint
         * still on the endpoint list since the management thread should not
         * remove the endpoint with threads wandering around in it.
         */
        uint32_t found = 0;
        for (set<UDPEndpoint>::iterator i = m_transport->m_endpointList.begin(); i != m_transport->m_endpointList.end(); ++i) {
            UDPEndpoint ep = *i;
            if (GetConnId() == ep->GetConnId()) {
                assert(connId == GetConnId() && "_UDPEndpoint::SendCb(): Inconsistent connId");
                QCC_DbgPrintf(("_UDPEndpoint::SendCb(): found endpoint with conn ID == %d. on m_endpointList", GetConnId()));
                ++found;
            }
        }
        assert(found == 1 && "_UDPEndpoint::SendCb(): Endpoint is gone");
#endif

        /*
         * We know we are still on the endpoint list and we know we have the
         * thread reference count bumped so it is safe to release the lock.
         */
        m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

        /*
         * If there is a thread trying to send bytes in this in this endpoint,
         * it first calls into PushMessage() and this indirectly calls into the
         * underlying stream's PushBytes().  If there is a pending PushBytes() a
         * thread will be blocked waiting for its ARDP send to complete.  In
         * that case, we must call back into the stream to unblock that pending
         * thread.  The stream actually does some more involved checking of the
         * returned buffers, so we let it do the free unless it is gone.
         *
         * If there is no stream, we are guaranteed there is no thread waiting
         * for something and so we can just proceed to free the memory since the
         * failure will have already been communicated up to the caller by
         * another mechanism, e.g., DisconnectCb().
         */
        if (m_stream) {
            m_stream->SendCb(handle, conn, buf, len, status);
        } else {
#ifndef NDEBUG
            CheckSeal(buf + len);
#endif
            delete[] buf;
        }

        DecrementAndFetch(&m_refCount);
    }

    /**
     * Get the handle to the underlying ARDP protocol implementation.
     */
    ArdpHandle* GetHandle()
    {
        QCC_DbgTrace(("_UDPEndpoint::GetHandle() => %p", m_handle));
        return m_handle;
    }

    /**
     * Set the handle to the underlying ARDP protocol implementation.
     */
    void SetHandle(ArdpHandle* handle)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetHandle(handle=%p)", handle));
        m_handle = handle;
    }

    /**
     * Get the pointer to the underlying ARDP protocol connection information.
     */
    ArdpConnRecord* GetConn()
    {
        QCC_DbgTrace(("_UDPEndpoint::GetConn(): => %p", m_conn));
        return m_conn;
    }

    /**
     * Set the pointer to the underlying ARDP protocol connection information.
     */
    void SetConn(ArdpConnRecord* conn)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetConn(conn=%p)", conn));
        m_conn = conn;
        m_transport->m_ardpLock.Lock();
        uint32_t cid = ARDP_GetConnId(m_handle, conn);

#ifndef NDEBUG
        if (cid == ARDP_CONN_ID_INVALID) {
            QCC_DbgPrintf(("_UDPEndpoint::SetConn(conn=%p): conn is invalid!", conn));
        }
#endif

        SetConnId(cid);
        m_transport->m_ardpLock.Unlock();
    }

    /**
     * Get the connection ID of the original ARDP protocol connection.  We keep
     * this around for debugging purposes after the connection goes away.
     */
    uint32_t GetConnId()
    {
//      QCC_DbgTrace(("_UDPEndpoint::GetConnId(): => %d.", m_id));
        return m_id;
    }

    /**
     * Set the connection ID of the original ARDP protocol connection.  We keep
     * this around for debugging purposes after the connection goes away.
     */
    void SetConnId(uint32_t id)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetConnId(id=%d.)", id));
        m_id = id;
    }

    /**
     * Get the IP address of the remote side of the connection.
     */
    qcc::IPAddress GetIpAddr()
    {
        QCC_DbgTrace(("_UDPEndpoint::GetIpAddr(): => \"%s\"", m_ipAddr.ToString().c_str()));
        return m_ipAddr;
    }

    /**
     * Set the IP address of the remote side of the connection.
     */
    void SetIpAddr(qcc::IPAddress& ipAddr)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetIpAddr(ipAddr=\"%s\")", ipAddr.ToString().c_str()));
        m_ipAddr = ipAddr;
    }

    /**
     * Get the UDP/IP port of the remote side of the connection.
     */
    uint16_t GetIpPort()
    {
        QCC_DbgTrace(("_UDPEndpoint::GetIpPort(): => %d.", m_ipPort));
        return m_ipPort;
    }

    /**
     * Set the UDP/IP port of the remote side of the connection.
     */
    void SetIpPort(uint16_t ipPort)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetIpPort(ipPort=%d.)", ipPort));
        m_ipPort = ipPort;
    }

    /**
     * Get the sudden disconnect indication.  If true, it means that the
     * connection was unexpectedly disconnected.  If false, it means we
     * are still connected, or we initiated the disconnection.
     */
    bool GetSuddenDisconnect()
    {
        QCC_DbgTrace(("_UDPEndpoint::GetSuddenDisconnect(): => %d.", m_suddenDisconnect));
        return m_suddenDisconnect;
    }

    /**
     * Get the sudden disconnect indication.  If true, it means that the
     * connection was unexpectedly disconnected.  If false, it means we
     * are still connected, or we initiated the disconnection.
     */
    void SetSuddenDisconnect(bool suddenDisconnect)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetSuddenDisconnect(suddenDisconnect(suddenDisconnect=%d.)", suddenDisconnect));
        m_suddenDisconnect = suddenDisconnect;
    }

    /**
     * Getting the local IP is not supported
     */
    QStatus GetLocalIp(qcc::String& ipAddrStr)
    {
        // Can get this through conn if it remembers local address to which its socket was bound
        assert(0);
        return ER_UDP_NOT_IMPLEMENTED;
    };

    /**
     * Get the IP address of the remote side of the connection.
     */
    QStatus GetRemoteIp(qcc::String& ipAddrStr)
    {
        QCC_DbgTrace(("_UDPEndpoint::GetRemoteIp(ipAddrStr=%p): => \"%s\"", &ipAddrStr, m_ipAddr.ToString().c_str()));
        ipAddrStr = m_ipAddr.ToString();
        return ER_OK;
    };

    /**
     * Set the time at which authentication was started.
     */
    void SetStartTime(qcc::Timespec tStart)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetStartTime()"));
        m_tStart = tStart;
    }

    /**
     * Get the time at which authentication was started.
     */
    qcc::Timespec GetStartTime(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::GetStartTime(): => %" PRIu64 ".%03d.", m_tStart.seconds, m_tStart.mseconds));
        return m_tStart;
    }

    /**
     * Set the time at which the stop process for the endpoint was begun.
     */
    void SetStopTime(qcc::Timespec tStop)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetStopTime()"));
        m_tStop = tStop;
    }

    /**
     * Get the time at which the stop process for the endpoint was begun.
     */
    qcc::Timespec GetStopTime(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::GetStopTime(): => %" PRIu64 ".%03d.", m_tStop.seconds, m_tStop.mseconds));
        return m_tStop;
    }

    /**
     * Set the time at which the last stall warning was emitted.
     */
    void SetStallTime(qcc::Timespec tStall)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetStallTime()"));
        m_tStall = tStall;
    }

    /**
     * Get the time at which the lst stall warning was emitted.
     */
    qcc::Timespec GetStallTime(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::GetStallTime(): => %" PRIu64 ".%03d.", m_tStall.seconds, m_tStall.mseconds));
        return m_tStall;
    }

    /**
     * Which side of a connection are we -- active or passive
     */
    SideState GetSideState(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::GetSideState(): => %d.", m_sideState));
        return m_sideState;
    }

    /**
     * Note that we are the active side of a connection
     */
    void SetActive(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetActive()"));
        m_sideState = SIDE_ACTIVE;
    }

    /**
     * Note that we are the passive side of a connection
     */
    void SetPassive(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetPassive()"));
        m_sideState = SIDE_PASSIVE;
    }

    /**
     * Get the state of the overall endpoint.  Failed, starting, stopping, etc.
     */
    EndpointState GetEpState(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::GetEpState(): => %d.", m_epState));
        return m_epState;
    }

    /**
     * Check the state of the endpoint for initialized
     */
    bool IsEpInitialized(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::IsEpInitialized() <= \"%s\"", m_epState == EP_INITIALIZED ? "true" : "false"));
        return m_epState == EP_INITIALIZED;
    }

    /**
     * Set the state of the endpoint to active started
     */
    void SetEpActiveStarted(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpActiveStarted()"));
        assert(m_epState == EP_INITIALIZED);
        m_epState = EP_ACTIVE_STARTED;
    }

    /**
     * Check the state of the endpoint for active started
     */
    bool IsEpActiveStarted(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::IsEpActiveStarted() <= \"%s\"", m_epState == EP_ACTIVE_STARTED ? "true" : "false"));
        return m_epState == EP_ACTIVE_STARTED;
    }

    /**
     * Set the state of the endpoint to passive started
     */
    void SetEpPassiveStarted(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpPassiveStarted()"));
        assert(m_epState == EP_INITIALIZED);
        m_epState = EP_PASSIVE_STARTED;
    }

    /**
     * Check the state of the endpoint for passive started
     */
    bool IsEpPassiveStarted(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::IsEpPassiveStarted() <= \"%s\"", m_epState == EP_PASSIVE_STARTED ? "true" : "false"));
        return m_epState == EP_PASSIVE_STARTED;
    }

    /**
     * Set the state of the endpoint to started
     */
    void SetEpStarted(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpStarted()"));
        assert(m_epState == EP_ACTIVE_STARTED || m_epState == EP_PASSIVE_STARTED);
        m_epState = EP_STARTED;
    }

    /**
     * Check the state of the endpoint for started
     */
    bool IsEpStarted(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::IsEpStarted() <= \"%s\"", m_epState == EP_STARTED ? "true" : "false"));
        return m_epState == EP_STARTED;
    }


    /**
     * Enable the optional EP_WAITING state
     */
    void SetEpWaitEnable(bool wait)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpWaitEnable(wait=\"%s\")", wait ? "true" : "false"));
        m_wait = wait;
    }

    /**
     * Check to see if the EP_WAITING state is enabled
     */
    bool IsEpWaitEnabled(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::IsEpWaitEnabled() <= \"%s\"", m_wait ? "true" : "false"));
        return m_wait;
    }

    /**
     * Set the state of the endpoint to stopping.
     *
     * If wait is true, the endpoint is stopping because of a local disconnect
     * and the EP_WAITING state should follow EP_STOPPING to wait for the ARDP
     * write queue to empty before disconnecting.
     */
    void SetEpStopping()
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpStopping()"));

#ifndef NDEBUG
        PrintEpState("_UdpEndpoint::SetEpStopping()");
#endif

        /*
         * Don't allow backward progress.
         */
        assert(m_epState != EP_JOINED || m_epState == EP_DONE);

        Timespec tNow;
        GetTimeNow(&tNow);
        SetStopTime(tNow);
        SetStallTime(tNow);

        m_epState = EP_STOPPING;
    }

    /**
     * Check the state of the endpoint for stopping
     */
    bool IsEpStopping(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::IsEpStopping() <= \"%s\"", m_epState == EP_STOPPING ? "true" : "false"));
        return m_epState == EP_STOPPING;
    }

    /**
     * Set the state of the endpoint to waiting
     */
    void SetEpWaiting(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpWaiting()"));
        assert(m_epState == EP_STOPPING);
        m_epState = EP_WAITING;
    }

    /**
     * Check the state of the endpoint for stopping
     */
    bool IsEpWaiting(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::IsEpWaiting() <= \"%s\"", m_epState == EP_WAITING ? "true" : "false"));
        return m_epState == EP_WAITING;
    }

    /**
     * Set the state of the endpoint to joined
     */
    void SetEpJoined(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpJoined()"));

        /*
         * It is illegal to set Join()ed directly from a started state without calling Stop() first.
         */
        assert(m_epState != EP_STARTED);

        /*
         * Don't allow backward progress.
         */
        assert(m_epState != EP_DONE);

        m_epState = EP_JOINED;
    }

    /**
     * Check the state of the endpoint for joined
     */
    bool IsEpJoined(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::IsEpJoined() <= \"%s\"", m_epState == EP_JOINED ? "true" : "false"));
        return m_epState == EP_JOINED;
    }

    /**
     * Set the state of the endpoint to done
     */
    void SetEpDone(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpDone()"));
        assert(m_epState == EP_JOINED);
        m_epState = EP_DONE;
    }

    /**
     * Check the state of the endpoint for doneness
     */
    bool IsEpDone(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::IsEpDone() <= \"%s\"", m_epState == EP_DONE ? "true" : "false"));
        return m_epState == EP_DONE;
    }

    /**
     * Set the boolean indicating that the disconenct logic has happened.  This
     * is provided so that the transport can manually override this logic if an
     * error is detected prior to calling start (and where the disconnect
     * callback that drives that logic will never be called).
     */
    void SetDisconnected()
    {
        QCC_DbgTrace(("_UDPEndpoint::SetDisconnected()"));
        m_disconnected = true;
    }

    /**
     * Set the link timeout for this connection
     *
     * TODO: How does the link timeout set by the application play with the
     * default link timeout managed by the protocol.  We certainly don't want to
     * trigger the link timeout functionality of the remote endpoint since it is
     * going to expect all of the usual stream, thread, event functionality.
     *
     * For now, we just silently ignore SetLinkTimeout() and use the underlhing
     * ARDP mechanism.
     */
    QStatus SetLinkTimeout(uint32_t& linkTimeout)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetLinkTimeout(linkTimeout=%d.)", linkTimeout));
        QCC_DbgPrintf(("_UDPEndpoint::SetLinkTimeout(): Ignored", linkTimeout));
        return ER_OK;
    }

    void StateLock()
    {
        m_stateLock.Lock();
    }

    void StateUnlock()
    {
        m_stateLock.Unlock();
    }

  private:
    UDPTransport* m_transport;        /**< The server holding the connection */
    ArdpStream* m_stream;             /**< Convenient pointer to the underlying stream */
    ArdpHandle* m_handle;             /**< The handle to the underlying protocol */
    ArdpConnRecord* m_conn;           /**< The connection record for the underlying protocol */
    uint32_t m_id;                    /**< The ID of the connection record for the underlying protocol */
    qcc::IPAddress m_ipAddr;          /**< Remote IP address. */
    uint16_t m_ipPort;                /**< Remote port. */
    bool m_suddenDisconnect;          /**< If true, assumption is that any disconnect will be/was unexpected */
    bool m_registered;                /**< If true, a call-out to the daemon has been made to register this endpoint */
    volatile SideState m_sideState;   /**< Is this an active or passive connection */
    volatile EndpointState m_epState; /**< The state of the endpoint itself */
    qcc::Timespec m_tStart;           /**< Timestamp indicating when the authentication process started */
    qcc::Timespec m_tStop;            /**< Timestamp indicating when the stop process for the endpoint was begun */
    qcc::Timespec m_tStall;           /**< Timestamp indicating when the last stall warning was logged */
    bool m_remoteExited;              /**< Indicates if the remote endpoint exit function has been run.  Cannot delete until true. */
    bool m_exitScheduled;             /**< Indicates if the remote endpoint exit function has been scheduled. */
    volatile bool m_disconnected;     /**< Indicates an interlocked handling of the ARDP_Disconnect has happened */
    volatile int32_t m_refCount;      /**< Incremented if a thread is wandering through the endpoint, decrememted when it leaves */
    volatile int32_t m_pushCount;     /**< Incremented if a thread is wandering through the endpoint, decrememted when it leaves */
    qcc::Mutex m_stateLock;           /**< Mutex protecting the endpoint state against multiple threads attempting changes */
    bool m_wait;                      /**< If true, follow EP_STOPPING state with EP_WAITING state */
};

/*
 * A thread function to dispatch all of the callbacks from the ARDP protocol.
 */
ThreadReturn STDCALL MessagePump::PumpThread::Run(void* arg)
{
    QCC_DbgTrace(("MessagePump::PumpThread::Run()"));
    assert(m_pump && "MessagePump::PumpThread::Run(): pointer to enclosing pump must be specified");

    /*
     * We need to implement a classic condition variable idiom.  Here the mutex
     * is the m_lock mutex member variable and the condition that needs to be
     * met is "something in the queue."  This is complicated a bit (but not
     * much) since we need to live inside a while loop and execute until we are
     * asked to stop or have hung around for some time without something useful
     * to do (see the class doxygen for more description).
     *
     * Note that IsStopping() is a call to the thread base class that indicates
     * the underlying thread has received a Stop() request.  The member variable
     * m_stopping is a boolean we use to synchronize the run thread and any
     * RecvCb() callbacks we might get from the UDP Transport dispatcher thread.
     * We don't want to have to understand the details of how and when the
     * internal stopping member of a thread is set, so we use our own with know
     * semantics for synchronization up here.
     */
    QStatus status = ER_OK;
    m_pump->m_lock.Lock();
    while (!m_pump->m_stopping && !IsStopping() && status != ER_TIMEOUT) {
        QCC_DbgPrintf(("MessagePump::PumpThread::Run(): Top."));
        /*
         * Note that if the condition returns an unexpected status we loop in
         * the hope that it is recoverable.
         */
        while (m_pump->m_queue.empty() && !m_pump->m_stopping && !IsStopping() && status != ER_TIMEOUT) {
            QCC_DbgPrintf(("MessagePump::PumpThread::Run(): TimedWait for condition"));
            status = m_pump->m_condition.TimedWait(m_pump->m_lock, UDP_MESSAGE_PUMP_TIMEOUT);
            QCC_DbgPrintf(("MessagePump::PumpThread::Run(): TimedWait returns \"%s\"", QCC_StatusText(status)));
        }

        QCC_DbgPrintf(("MessagePump::PumpThread::Run(): Done with wait."));

        /*
         * One of three things has happened: 1) the condition has been signaled
         * because there is something on the queue; 2) the condition has been
         * signaled because we have been asked to stop; 3) the timed wait has
         * timed out and we have had nothing to do for that amount of time.  If
         * we've been asked to stop, or we time out, we just loop back to the
         * top and break out of the while loop -- we just need to work if the
         * queue is not empty.
         */
        if (m_pump->m_stopping || IsStopping() || status == ER_TIMEOUT) {
            continue;
        } else if (m_pump->m_queue.empty() == false) {
            QCC_DbgTrace(("MessagePump::PumpThread::Run(): Have work."));

            /*
             * Pull the entry describing the message off the work queue.
             */
            QueueEntry entry = m_pump->m_queue.front();
            m_pump->m_queue.pop();

            /*
             * We need to call out to the daemon to have it go ahead and push
             * the message off to the destination.  This gets tricky since we
             * have several threads that need to deal with the endpoint
             * including those that may want to delete it.
             *
             * While we hold the endpoint lock we are going to call
             * IncrementRefs() on the endpoint to ensure it isn't deleted before
             * the call out to the endpoint is made.  Then we call out to the
             * RecvCb() ahd then finally we DecrementRefs() to allow deletion.
             */
            bool handled = false;
            m_pump->m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);
            for (set<UDPEndpoint>::iterator i = m_pump->m_transport->m_endpointList.begin(); i != m_pump->m_transport->m_endpointList.end(); ++i) {
                UDPEndpoint ep = *i;
                if (entry.m_connId == ep->GetConnId()) {
                    QCC_DbgPrintf(("MessagePump::PumpThread::Run(): found endpoint with conn ID == %d. on m_endpointList", entry.m_connId));
                    /*
                     * We have a managed object reference to ep, but also
                     * increment the endpoint reference count (different from
                     * the managed object reference count) to make sure the
                     * endpoint manager doesn't try to delete the endpoint
                     * before our call actually makes it there.
                     */
                    ep->IncrementRefs();
                    m_pump->m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
                    m_pump->m_lock.Unlock();
                    QCC_DbgPrintf(("MessagePump::PumpThread::Run(): Call out to endopint with connId=%d.", entry.m_connId));
                    ep->RecvCb(entry.m_handle, entry.m_conn, entry.m_connId, entry.m_rcv, entry.m_status);
                    QCC_DbgPrintf(("MessagePump::PumpThread::Run(): Back from endpoint RecvCb()"));
                    handled = true;
                    m_pump->m_lock.Lock();
                    m_pump->m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);
                    /*
                     * Since we held a reference to ep and we incremented the
                     * reference count, we expect this reference to remain
                     * valid.  We use it to decrement the refs and then forget
                     * it.  We can never use the iterator again since the moment
                     * we released the lock it could have become invalid.
                     */
                    ep->DecrementRefs();
                    break;
                }
            }
            m_pump->m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

            /*
             * If we were able to find an endpoint on which to dispatch this
             * callback then the endpoint RecvCb() took responsibility for the
             * message.
             *
             * If we were unable to find an endpoint to dispatch this callback
             * to we still need to do something about the message we were given.
             * This means returning the bytes back to ARDP.
             */
            if (handled == false) {
#if RETURN_ORPHAN_BUFS

                QCC_DbgPrintf(("MessagePump::PumpThread::Run(): Unable to find endpoint with conn ID == %d. on m_endpointList", entry.m_connId));
                m_pump->m_transport->m_ardpLock.Lock();
                ARDP_RecvReady(entry.m_handle, entry.m_conn, entry.m_rcv);
                m_pump->m_transport->m_ardpLock.Unlock();

#else // not RETURN_ORPHAN_BUFS

                QCC_DbgPrintf(("MessagePump::PumpThread::Run(): Unable to find endpoint with conn ID == %d. on m_endpointList. Ignore message", entry.m_connId));

#endif // not RETURN_ORPHAN_BUFS
            }
        }
    }


    /*
     * If we are exiting, we need to make sure that there are no race conditions
     * between our decision to exit and a RecvCb() decision to use an existing
     * active thread.  We also need to recognize that if we are returning because
     * of an inactivity timeout we need to allow RecvCb() to spin up a new thread.
     *
     * We hold the pump lock here, and RecvCb() must also hold the lock thread
     * when it makes its decisions.  If we know RecvCb() will run after we are
     * done.  If we are exiting because of a Stop(), then IsStopping() will be
     * true and our version (m_stopping) will also be true.  RecvCb() will not
     * spin up a new thread if m_stopping is true.  If we are exiting because of
     * inactivity at this point IsStopping() and m_stopping may or may not be
     * true depending on races.  We don't care, since m_stopping is going to
     * determine what happens after we are gone.  We just exit.
     *
     * If we are exiting, we must be the active thread.  The rules say we are
     * the one who must remove our own pointer from the activeThread, zero it
     * out and add ourselves to the pastThread queue.  If we do this atomically
     * with the pump lock held, then RecvCb() can do what it needs to do also
     * atomically.
     */
    QCC_DbgPrintf(("MessagePump::PumpThread::Run(): Exiting"));
    PumpThread* i = (PumpThread*)GetThread();
    assert(m_pump->m_activeThread == i && "MessagePump::PumpThread::Run(): I should be the active thread");
    m_pump->m_pastThreads.push(i);
    m_pump->m_activeThread = NULL;
    m_pump->m_lock.Unlock();
    QCC_DbgPrintf(("MessagePump::PumpThread::Run(): Return"));

    /*
     * The last thing we need to do is to bug the endpoint so its endpoint
     * management function will run and Join() our thread.  We've pushed our
     * thread ID onto the list of past threads, and the endpoint will assume
     * that we have exited and join us if our ID is on pastThreads.  This will
     * happen even if we get swapped out "between" the Alert() and the return
     * (especially during the closing printfs in debug mode).
     */
    QCC_DbgPrintf(("MessagePump::PumpThread::Run(): Alert()"));
    m_pump->m_transport->Alert();
    QCC_DbgPrintf(("MessagePump::PumpThread::Run(): Return"));
    return 0;
}

static void UdpEpStateLock(_UDPEndpoint* ep)
{
    ep->StateLock();
}

static void UdpEpStateUnlock(_UDPEndpoint* ep)
{
    ep->StateUnlock();
}

static bool IsUdpEpStarted(_UDPEndpoint* ep)
{
    return ep->IsEpStarted();
}

/**
 * Construct a UDP Transport object.
 */
UDPTransport::UDPTransport(BusAttachment& bus) :
    Thread("UDPTransport"),
    m_bus(bus), m_stopping(false), m_listener(0), m_foundCallback(m_listener), m_networkEventCallback(*this),
    m_isAdvertising(false), m_isDiscovering(false), m_isListening(false),
    m_isNsEnabled(false),
    m_reload(STATE_RELOADING),
    m_manage(STATE_MANAGE),
    m_nsReleaseCount(0), m_wildcardIfaceProcessed(false),
    m_routerName(), m_maxUntrustedClients(0), m_numUntrustedClients(0),
    m_authTimeout(0), m_sessionSetupTimeout(0),
    m_maxAuth(0), m_maxConn(0), m_currAuth(0), m_currConn(0), m_connLock(),
    m_ardpLock(), m_cbLock(), m_handle(NULL),
    m_dispatcher(NULL), m_exitDispatcher(NULL),
    m_workerCommandQueue(), m_workerCommandQueueLock(), m_exitWorkerCommandQueue(), m_exitWorkerCommandQueueLock()
#if WORKAROUND_1298
    , m_done1298(false)
#endif
{
    QCC_DbgTrace(("UDPTransport::UDPTransport()"));

    /*
     * We know we are daemon code, so we'd better be running with a daemon
     * router.  This is assumed elsewhere.
     */
    assert(m_bus.GetInternal().GetRouter().IsDaemon());

    /*
     * We need to find the defaults for our connection limits.  These limits
     * can be specified in the configuration database with corresponding limits
     * used for DBus.  If any of those are present, we use them, otherwise we
     * provide some hopefully reasonable defaults.
     */
    ConfigDB* config = ConfigDB::GetConfigDB();

    m_authTimeout = config->GetLimit("auth_timeout", ALLJOYN_AUTH_TIMEOUT_DEFAULT);
    m_sessionSetupTimeout = config->GetLimit("session_setup_timeout", ALLJOYN_SESSION_SETUP_TIMEOUT_DEFAULT);
    m_maxAuth = config->GetLimit("max_incomplete_connections", ALLJOYN_MAX_INCOMPLETE_CONNECTIONS_UDP_DEFAULT);
    m_maxConn = config->GetLimit("max_completed_connections", ALLJOYN_MAX_COMPLETED_CONNECTIONS_UDP_DEFAULT);

    ArdpGlobalConfig ardpConfig;
    ardpConfig.connectTimeout = config->GetLimit("udp_connect_timeout", UDP_CONNECT_TIMEOUT);
    ardpConfig.connectRetries = config->GetLimit("udp_connect_retries", UDP_CONNECT_RETRIES);
    ardpConfig.initialDataTimeout = config->GetLimit("udp_initial_data_timeout", UDP_INITIAL_DATA_TIMEOUT);
    ardpConfig.totalDataRetryTimeout = config->GetLimit("udp_total_data_retry_timeout", UDP_TOTAL_DATA_RETRY_TIMEOUT);
    ardpConfig.minDataRetries = config->GetLimit("udp_min_data_retries", UDP_MIN_DATA_RETRIES);
    ardpConfig.persistInterval = config->GetLimit("udp_persist_interval", UDP_PERSIST_INTERVAL);
    ardpConfig.totalAppTimeout = config->GetLimit("udp_total_app_timeout", UDP_TOTAL_APP_TIMEOUT);
    ardpConfig.linkTimeout = config->GetLimit("udp_link_timeout", UDP_LINK_TIMEOUT);
    ardpConfig.keepaliveRetries = config->GetLimit("udp_keepalive_retries", UDP_KEEPALIVE_RETRIES);
    ardpConfig.fastRetransmitAckCounter = config->GetLimit("udp_fast_retransmit_ack_counter", UDP_FAST_RETRANSMIT_ACK_COUNTER);
    ardpConfig.delayedAckTimeout = config->GetLimit("udp_delayed_ack_timeout", UDP_DELAYED_ACK_TIMEOUT);
    ardpConfig.timewait = config->GetLimit("udp_timewait", UDP_TIMEWAIT);
    ardpConfig.segbmax = config->GetLimit("udp_segbmax", UDP_SEGBMAX);
    ardpConfig.segmax = config->GetLimit("udp_segmax", UDP_SEGMAX);
    if (ardpConfig.segmax * ardpConfig.segbmax < ALLJOYN_MAX_PACKET_LEN) {
        QCC_LogError(ER_INVALID_CONFIG, ("UDPTransport::UDPTransport(): udp_segmax (%d) * udp_segbmax (%d) < ALLJOYN_MAX_PACKET_LEN (%d) ignored", ardpConfig.segbmax, ardpConfig.segmax, ALLJOYN_MAX_PACKET_LEN));
        ardpConfig.segbmax = UDP_SEGBMAX;
        ardpConfig.segmax = UDP_SEGMAX;
    }
    memcpy(&m_ardpConfig, &ardpConfig, sizeof(ArdpGlobalConfig));

    for (uint32_t i = 0; i < N_PUMPS; ++i) {
        m_messagePumps[i] = new MessagePump(this);
    }

    /*
     * User configured UDP-specific values trump defaults if longer.
     */
    qcc::Timespec t = Timespec(ardpConfig.connectTimeout * ardpConfig.connectRetries);
    if (m_authTimeout < t) {
        m_authTimeout = m_sessionSetupTimeout = t;
    }

    /*
     * Initialize the hooks to and from the ARDP protocol.  Note that
     * ARDP_AllocHandle is expected to "never fail."
     */
    m_ardpLock.Lock();
    m_handle = ARDP_AllocHandle(&ardpConfig);
    ARDP_SetHandleContext(m_handle, this);
    ARDP_SetAcceptCb(m_handle, ArdpAcceptCb);
    ARDP_SetConnectCb(m_handle, ArdpConnectCb);
    ARDP_SetDisconnectCb(m_handle, ArdpDisconnectCb);
    ARDP_SetRecvCb(m_handle, ArdpRecvCb);
    ARDP_SetSendCb(m_handle, ArdpSendCb);
    ARDP_SetSendWindowCb(m_handle, ArdpSendWindowCb);

#if ARDP_TESTHOOKS
    /*
     * Initialize some testhooks as an example of how to do this.
     */
    ARDP_HookSendToSG(m_handle, ArdpSendToSGHook);
    ARDP_HookSendTo(m_handle, ArdpSendToHook);
    ARDP_HookRecvFrom(m_handle, ArdpRecvFromHook);
#endif

    /*
     * Call into ARDP and ask it to start accepting connections passively.
     * Since we are running in a constructor, there's not much we can do if it
     * fails.
     */
#ifndef NDEBUG
    QStatus status =
#endif
    ARDP_StartPassive(m_handle);

#ifndef NDEBUG
    if (status != ER_OK) {
        QCC_DbgPrintf(("UDPTransport::UDPTransport(): ARDP_StartPassive() returns status==\"%s\"", QCC_StatusText(status)));
    }
#endif

    m_ardpLock.Unlock();
}

/**
 * Destroy a UDP Transport object.
 */
UDPTransport::~UDPTransport()
{
    QCC_DbgTrace(("UDPTransport::~UDPTransport()"));

    Stop();
    Join();

    for (uint32_t i = 0; i < N_PUMPS; ++i) {
        assert(m_messagePumps[i]->IsActive() == false && "UDPTransport::~UDPTransport(): Destroying with active message pump");
        delete m_messagePumps[i];
        m_messagePumps[i] = NULL;
    }

    ARDP_FreeHandle(m_handle);
    m_handle = NULL;

    QCC_DbgPrintf(("UDPTransport::~UDPTransport(): m_mAuthList.size() == %d", m_authList.size()));
    QCC_DbgPrintf(("UDPTransport::~UDPTransport(): m_mEndpointList.size() == %d", m_endpointList.size()));
    assert(m_preList.size() + m_authList.size() + m_endpointList.size() == 0 &&
           "UDPTransport::~UDPTransport(): Destroying with enlisted endpoints");
    //assert(IncrementAndFetch(&m_refCount) == 1 && "UDPTransport::~UDPTransport(): non-zero reference count");
}

/**
 * Define an EndpointExit function even though it is not used in the UDP
 * Transport.  This virtual function is expected by the daemon and must be
 * defined even though we will not use it.  We short-circuit the EndpointExit
 * functionality by defining a new RemoteEndpoint::Stop() function that doesn't
 * require the functionality.
 */
void UDPTransport::EndpointExit(RemoteEndpoint& ep)
{
    QCC_DbgTrace(("UDPTransport::EndpointExit()"));
}

/*
 * A thread to dispatch all of the callbacks from the ARDP protocol.
 */
ThreadReturn STDCALL UDPTransport::DispatcherThread::Run(void* arg)
{
    IncrementAndFetch(&m_transport->m_refCount);
    QCC_DbgTrace(("UDPTransport::DispatcherThread::Run()"));

    vector<Event*> checkEvents, signaledEvents;
    checkEvents.push_back(&stopEvent);

    while (!IsStopping()) {
        QCC_DbgTrace(("UDPTransport::DispatcherThread::Run(): Wait for some action"));

        signaledEvents.clear();

        /*
         * We should never have a status returned from Event::Wait other than
         * ER_OK since we don't set a timeout.  It is, however, the case that
         * Windows Event implementations may return ER_TIEMOUT even though no
         * timeout was set, and Posix systems may return ER_TIMER_NOT_ALLOWED.
         * If we were to exit in the face of such errors, the whole transport
         * would go down since the dispatcher thread would exit.  All we can do
         * is to hope that any errors are transient and try again.  In that
         * case, we will recover if the system does.  We'll log an error, but
         * there's not really much we can do here.
         */
        QStatus status = Event::Wait(checkEvents, signaledEvents);
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::DispatcherThread::Run(): Event::Wait failed"));
            continue;
        }

        for (vector<Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {
            if (*i == &stopEvent) {
                QCC_DbgTrace(("UDPTransport::DispatcherThread::Run(): Reset stopEvent"));
                stopEvent.ResetEvent();
            }
        }

        bool drained = false;
        do {
            WorkerCommandQueueEntry entry;

            /*
             * Pull an entry that describes what it is we need to do from the
             * queue.
             */
            m_transport->m_workerCommandQueueLock.Lock(MUTEX_CONTEXT);

            QCC_DbgTrace(("UDPTransport::DispatcherThread::Run(): m_workerCommandQueue.size()=%d.", m_transport->m_workerCommandQueue.size()));

            if (m_transport->m_workerCommandQueue.empty()) {
                drained = true;
            } else {
                entry = m_transport->m_workerCommandQueue.front();
                m_transport->m_workerCommandQueue.pop();
            }
            m_transport->m_workerCommandQueueLock.Unlock(MUTEX_CONTEXT);

            /*
             * We keep at it until we completely drain this queue every time we
             * wake up.
             */
            if (drained == false) {
                QCC_DbgTrace(("UDPTransport::DispatcherThread::Run(): command=%d., handle=%p, conn=%p., connId=%d.,"
                              "rcv=%p, passive=%d., buf=%p, len=%d., status=\"%s\"",
                              entry.m_command, entry.m_handle, entry.m_conn, entry.m_connId,
                              entry.m_rcv, entry.m_passive, entry.m_buf, entry.m_len, QCC_StatusText(entry.m_status)));

                /*
                 * If the command is a connect callback, we may not have an
                 * endpoint created yet.  Otherwise we have a connection ID in
                 * our command entry, and we expect it to refer to an endpoint
                 * that is on the endpoint list.  If it has been deleted out
                 * from under us we shouldn't use it.  Use the connection ID to
                 * make the connection between ArdpConnRecord* and UDPEndpoint
                 * (which also serves as our protocol demux functionality).
                 */
                if (entry.m_command == WorkerCommandQueueEntry::CONNECT_CB) {
                    /*
                     * We can't call out to some possibly windy code path out
                     * through the daemon router with the m_endpointListLock
                     * taken.
                     */
                    QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): CONNECT_CB: DoConnectCb()"));
                    m_transport->DoConnectCb(entry.m_handle, entry.m_conn, entry.m_connId, entry.m_passive, entry.m_buf, entry.m_len, entry.m_status);

                    /*
                     * The UDPTransport::ConnectCb() handler allocated a copy of
                     * the data buffer from ARDP since ARDP expected its buffer
                     * back immediately.  We need to delete this copy now that
                     * we're done with it.
                     */
#ifndef NDEBUG
                    CheckSeal(entry.m_buf + entry.m_len);
#endif
                    delete[] entry.m_buf;
                    entry.m_buf = NULL;
                    entry.m_len = 0;
                } else {
                    bool haveLock = true;
                    m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);
                    for (set<UDPEndpoint>::iterator i = m_transport->m_endpointList.begin(); i != m_transport->m_endpointList.end(); ++i) {
                        UDPEndpoint ep = *i;
                        if (entry.m_connId == ep->GetConnId()) {
                            /*
                             * We can't call out to some possibly windy code path
                             * out through the daemon router with the
                             * m_endpointListLock taken.  But since we are going to
                             * call into the endpoint, we'll bump the reference
                             * count to indicate a thread is coming.  If the ref
                             * count bumped, the endpoint management code will not
                             * kill the endpoint out from under us.
                             */
                            ep->IncrementRefs();
                            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
                            haveLock = false;

                            /*
                             * This probably seems like a lot of trouble to make a
                             * single method call.  The problem is that if we don't
                             * go through the trouble, we do the calls in an ARDP
                             * callback.  If we do it in a callback, that callback
                             * must have been driven by a call to ARDP_Run() which
                             * must have been called with the ardpLock taken.  When
                             * Start() (for example) does its RegisterEndpoint() the
                             * daemon wants to take the name table lock to add the
                             * endpoint to the name table.
                             *
                             * If another thread is sending a message through the
                             * daemon, it wants to call into daemon router which
                             * takes the nameTableLock to figure out which endpoint
                             * to send to.  If that destination endpoint happens to
                             * be a UDP endpoint, it will need to take the ardpLock
                             * to actually send the bits using ARDP_send.
                             *
                             * In once case the lock order is ardpLock, then
                             * nameTableLock; in the other case the lock order is
                             * nameTableLock, then ardpLock. Deadlock.
                             *
                             * Similar situations abound if we call out to the
                             * daemon, so we cannot call into the daemon directly
                             * from a callback which would imply the ardpLock is
                             * taken.  Instead of playing with fire, we route
                             * callbacks to this thread to get the one call done.
                             *
                             * This approach does have the benefit of keeping all of
                             * the call-outs from the ARDP protocol very quick,
                             * amounting to usually a push onto a message queue.
                             */
                            switch (entry.m_command) {
                            case WorkerCommandQueueEntry::EXIT:
                                {
                                    QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): EXIT: Exit()"));
                                    ep->Exit();
                                    break;
                                }

                            case WorkerCommandQueueEntry::SEND_CB:
                                {
                                    QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): SEND_CB: SendCb()"));
                                    ep->SendCb(entry.m_handle, entry.m_conn, entry.m_connId, entry.m_buf, entry.m_len, entry.m_status);
                                    break;
                                }

                            case WorkerCommandQueueEntry::RECV_CB:
                                {
                                    QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): RECV_CB: Call RecvCb() on endpoint message pump"));
                                    /*
                                     * We have N_PUMPS worth of what amounts to
                                     * worker threads to handle making the calls
                                     * out to the daemon router.  This is to
                                     * handle cases where the reader of the
                                     * messages is slow, that then leads to the
                                     * callout to the PushMessage to block.
                                     *
                                     * We assume entry.connId to be selected
                                     * from a uniform distribution and so simply
                                     * selecting a message pump based on a mod
                                     * operation on the connection ID will
                                     * uniformly distribute the load among the
                                     * threads.
                                     *
                                     * We always select the same pump for a
                                     * given connection ID since that preserves
                                     * message ordering at the destination.
                                     *
                                     * There are complications here because
                                     * there is a possibility that the endpoint
                                     * we recently verified to exist may
                                     * actually not exist anymore as soon as we
                                     * let go of the endpoint list lock above.
                                     * The MessagePump deals with the
                                     * possibility that things are disappearing
                                     * out from underneath it.
                                     *
                                     * We assume that the message pumps array
                                     * will not be changed out from underneath
                                     * us while we are running at least.
                                     */
                                    MessagePump* mp = m_transport->m_messagePumps[entry.m_connId % N_PUMPS];
                                    assert(mp != NULL && "UDPTransport::DispatcherThread::Run(): Pumps array not initialized");
                                    mp->RecvCb(entry.m_handle, entry.m_conn, entry.m_connId, entry.m_rcv, entry.m_status);
                                    break;
                                }

                            case WorkerCommandQueueEntry::DISCONNECT_CB:
                                {
                                    QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): DISCONNECT_CB: DisconnectCb()"));
                                    ep->DisconnectCb(entry.m_handle, entry.m_conn, entry.m_connId, entry.m_status);
                                    break;
                                }

                            default:
                                {
                                    assert(false && "UDPTransport::DispatcherThread::Run(): Unexpected command");
                                    break;
                                }

                            } // switch(entry.m_command)

                            /*
                             * At this point, we assume that we have given the
                             * m_endpointListLock and decremented the thread count
                             * in the endpoint; and our iterator can no longer be
                             * trusted (or is not there in the case of a CONNECT_CB
                             * request.
                             */
                            ep->DecrementRefs();
                            assert(haveLock == false && "UDPTransport::DispatcherThread::Run(): Should not have m_endpointListLock here");
                            break;
                        } // if (entry.m_connId == ep->GetConnId())
                    } // for (set<UDPEndpoint>::iterator i ...

                    /*
                     * If we found an endpoint, we gave the lock, did the
                     * operation and broke out of the iterator loop assuming the
                     * iterator was no good any more.  If we did not find the
                     * endpoint, we still have the lock and we need to give it
                     * up.  Also, if we did not find an endpoint, we may have
                     * a receive buffer we have to dispose of.  We assume that
                     * when the endpoint bailed, any callers waiting to write
                     * were ejected and so there are no transmit buffers to
                     * deal with.
                     */
                    if (haveLock) {
                        m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

                        switch (entry.m_command) {
                        case WorkerCommandQueueEntry::RECV_CB:
                            {
#if RETURN_ORPHAN_BUFS
                                /*
                                 * If we get here, we have a receive callback
                                 * from ARDP but we don't have an endpoint to
                                 * route the message to.  This is expected if
                                 * the endpoint has gone down but there are
                                 * messages queued up to be delivered to it.
                                 * Since there's nowhere to send such messages,
                                 * we just have to drop them.  It is also
                                 * possible that ARDP has closed down the
                                 * connection under the sheets, so it may
                                 * consider the connection invalid.  In that
                                 * case, we try to give it back, but if ARDP
                                 * doesn't take it back, we have to assume that
                                 * it has disposed of the buffers on its own.
                                 * We'll print a message if we are in debug mode
                                 * if that happens.
                                 */
                                QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): Orphaned RECV_CB: ARDP_RecvReady()"));
                                m_transport->m_ardpLock.Lock();

#ifndef NDEBUG
                                QStatus alternateStatus =
#endif
                                ARDP_RecvReady(entry.m_handle, entry.m_conn, entry.m_rcv);
#ifndef NDEBUG
                                if (alternateStatus != ER_OK) {
                                    QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): ARDP_RecvReady() returns status==\"%s\"", QCC_StatusText(alternateStatus)));
                                }
#endif
                                m_transport->m_ardpLock.Unlock();
#else // not RETURN_ORPHAN_BUFS
                                /*
                                 * If we get here, we have a receive callback
                                 * from ARDP but we don't have an endpoint to
                                 * route the message to.  This is expected if
                                 * the endpoint has gone down but there are
                                 * messages queued up to be delivered to it.
                                 * Since there's nowhere to send such messages,
                                 * we just have to drop them.  Since the
                                 * endpoint has been destroyed, we assume that
                                 * ARDP has closed down the connection and
                                 * released any pending buffers.  This means
                                 * that we can just ignore the buffer.
                                 */
                                QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): Orphaned RECV_CB for conn ID == %d. ignored",
                                               entry.m_connId));
#endif // not RETURN_ORPHAN_BUFS
                                break;
                            }

                        default:
                            break;
                        }
                    }
                } // else not CONNECT_CB
            } // if (drained == false)
        } while (drained == false);
    }

    QCC_DbgTrace(("UDPTransport::DispatcherThread::Run(): Exiting"));
    DecrementAndFetch(&m_transport->m_refCount);
    return 0;
}

/*
 * A thread dedicated to dispatching endpoint exit functions to avoid deadlock.
 */
ThreadReturn STDCALL UDPTransport::ExitDispatcherThread::Run(void* arg)
{
    IncrementAndFetch(&m_transport->m_refCount);
    QCC_DbgTrace(("UDPTransport::ExitDispatcherThread::Run()"));

    vector<Event*> checkEvents, signaledEvents;
    checkEvents.push_back(&stopEvent);

    while (!IsStopping()) {
        QCC_DbgTrace(("UDPTransport::ExitDispatcherThread::Run(): Wait for some action"));

        signaledEvents.clear();

        /*
         * We should never have a status returned from Event::Wait other than
         * ER_OK since we don't set a timeout.  It is, however, the case that
         * Windows Event implementations may return ER_TIEMOUT even though no
         * timeout was set, and Posix systems may return ER_TIMER_NOT_ALLOWED.
         * If we were to exit in the face of such errors, the whole transport
         * would go down since the dispatcher thread would exit.  All we can do
         * is to hope that any errors are transient and try again.  In that
         * case, we will recover if the system does.  We'll log an error, but
         * there's not really much we can do here.
         */
        QStatus status = Event::Wait(checkEvents, signaledEvents);
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::ExitDispatcherThread::Run(): Event::Wait failed"));
            continue;
        }

        for (vector<Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {
            if (*i == &stopEvent) {
                QCC_DbgTrace(("UDPTransport::ExitDispatcherThread::Run(): Reset stopEvent"));
                stopEvent.ResetEvent();
            }
        }

        bool drained = false;
        do {
            WorkerCommandQueueEntry entry;

            /*
             * Pull an entry that describes what it is we need to do from the
             * queue.
             */
            m_transport->m_exitWorkerCommandQueueLock.Lock(MUTEX_CONTEXT);

            QCC_DbgTrace(("UDPTransport::ExitDispatcherThread::Run(): m_exitWorkerCommandQueue.size()=%d.", m_transport->m_exitWorkerCommandQueue.size()));

            if (m_transport->m_exitWorkerCommandQueue.empty()) {
                drained = true;
            } else {
                entry = m_transport->m_exitWorkerCommandQueue.front();
                m_transport->m_exitWorkerCommandQueue.pop();
            }
            m_transport->m_exitWorkerCommandQueueLock.Unlock(MUTEX_CONTEXT);

            /*
             * We keep at it until we completely drain this queue every time we
             * wake up.
             */
            if (drained == false) {
                QCC_DbgTrace(("UDPTransport::ExitDispatcherThread::Run(): command=%d., handle=%p, conn=%p., connId=%d.,"
                              "rcv=%p, passive=%d., buf=%p, len=%d., status=\"%s\"",
                              entry.m_command, entry.m_handle, entry.m_conn, entry.m_connId,
                              entry.m_rcv, entry.m_passive, entry.m_buf, entry.m_len, QCC_StatusText(entry.m_status)));

                bool haveLock = true;
                m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);
                for (set<UDPEndpoint>::iterator i = m_transport->m_endpointList.begin(); i != m_transport->m_endpointList.end(); ++i) {
                    UDPEndpoint ep = *i;
                    if (entry.m_connId == ep->GetConnId()) {
                        /*
                         * We can't call out to some possibly windy code path
                         * out through the daemon router with the
                         * m_endpointListLock taken.  But since we are going to
                         * call into the endpoint, we'll bump the reference
                         * count to indicate a thread is coming.  If the ref
                         * count bumped, the endpoint management code will not
                         * kill the endpoint out from under us.
                         */
                        ep->IncrementRefs();
                        m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
                        haveLock = false;

                        switch (entry.m_command) {
                        case WorkerCommandQueueEntry::EXIT:
                            {
                                QCC_DbgPrintf(("UDPTransport::ExitDispatcherThread::Run(): EXIT: Exit()"));
                                ep->Exit();
                                break;
                            }

                        default:
                            {
                                assert(false && "UDPTransport::ExitDispatcherThread::Run(): Unexpected command");
                                break;
                            }
                        }

                        /*
                         * At this point, we assume that we have given the
                         * m_endpointListLock and decremented the thread count
                         * in the endpoint; and our iterator can no longer be
                         * trusted.
                         */
                        ep->DecrementRefs();
                        assert(haveLock == false && "UDPTransport::ExitDispatcherThread::Run(): Should not have m_endpointListLock here");
                        break;
                    } // if (entry.m_connId == ep->GetConnId())
                } // for (set<UDPEndpoint>::iterator i ...

                /*
                 * If we found an endpoint, we gave the lock, did the operation
                 * and broke out of the iterator loop assuming the iterator was
                 * no good any more.  If we did not find the endpoint, we still
                 * have the lock and we need to give it up.
                 */
                if (haveLock) {
                    m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
                }
            } // if (drained == false)
        } while (drained == false);
    }

    QCC_DbgTrace(("UDPTransport::ExitDispatcherThread::Run(): Exiting"));
    DecrementAndFetch(&m_transport->m_refCount);
    return 0;
}

/**
 * Start the UDP Transport and prepare it for accepting inbound connections or
 * forming outbound connections.
 */
QStatus UDPTransport::Start()
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::Start()"));

    /*
     * The AllJoyn threading model says exactly one Start() can be done.
     */
    if (IsRunning()) {
        QCC_LogError(ER_BUS_BUS_ALREADY_STARTED, ("UDPTransport::Start(): Already started"));
        DecrementAndFetch(&m_refCount);
        return ER_BUS_BUS_ALREADY_STARTED;
    }

    m_stopping = false;

    /*
     * Get the guid from the bus attachment which will act as the globally unique
     * ID of the daemon.
     */
    qcc::String guidStr = m_bus.GetInternal().GetGlobalGUID().ToString();

    /*
     * We're a UDP transport, and UDP is an IP protocol, so we want to use the IP
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
    QCC_DbgPrintf(("UDPTransport::Start(): Aquire instance of NS"));
    m_nsReleaseCount = 0;
    IpNameService::Instance().Acquire(guidStr);

    /*
     * Tell the name service to call us back on our FoundCallback method when
     * we hear about a new well-known bus name.
     */
    QCC_DbgPrintf(("UDPTransport::Start(): Set NS callback"));
    IpNameService::Instance().SetCallback(TRANSPORT_UDP,
                                          new CallbackImpl<FoundCallback, void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint32_t>
                                              (&m_foundCallback, &FoundCallback::Found));

    IpNameService::Instance().SetNetworkEventCallback(TRANSPORT_UDP,
                                                      new CallbackImpl<NetworkEventCallback, void, const std::map<qcc::String, qcc::IPAddress>&>
                                                          (&m_networkEventCallback, &NetworkEventCallback::Handler));

    QCC_DbgPrintf(("UDPTransport::Start(): Spin up message dispatcher thread"));
    m_dispatcher = new DispatcherThread(this);
    QStatus status = m_dispatcher->Start(NULL, NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::Start(): Failed to Start() message dispatcher thread"));
        DecrementAndFetch(&m_refCount);
        return status;
    }

    QCC_DbgPrintf(("UDPTransport::Start(): Spin up exit dispatcher thread"));
    m_exitDispatcher = new ExitDispatcherThread(this);
    status = m_exitDispatcher->Start(NULL, NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::Start(): Failed to Start() exit dispatcher thread"));
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * Start the maintenance loop through the thread base class.  This will
     * close or open the IsRunning() gate we use to control access to our public
     * API.
     */
    QCC_DbgPrintf(("UDPTransport::Start(): Spin up main thread"));
    status = Thread::Start();
    DecrementAndFetch(&m_refCount);
    return status;
}

bool operator<(const UDPTransport::ConnectEntry& lhs, const UDPTransport::ConnectEntry& rhs)
{
    return lhs.m_connId < rhs.m_connId;
}

/**
 * Ask all of the threads that may be wandering around in the UDP Transport or
 * its associated endpoints to begin leaving.
 */
QStatus UDPTransport::Stop(void)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::Stop()"));

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
    QCC_DbgPrintf(("UDPTransport::Stop(): Gratuitously clean out advertisements."));
    for (list<qcc::String>::iterator i = m_advertising.begin(); i != m_advertising.end(); ++i) {
        IpNameService::Instance().CancelAdvertiseName(TRANSPORT_UDP, *i, TRANSPORT_UDP);
    }
    m_advertising.clear();
    m_isAdvertising = false;
    QCC_DbgPrintf(("UDPTransport::Stop(): Gratuitously clean out discoveries."));
    for (list<qcc::String>::iterator i = m_discovering.begin(); i != m_discovering.end(); ++i) {
        IpNameService::Instance().CancelFindAdvertisement(TRANSPORT_UDP, *i, TRANSPORT_UDP);
    }
    m_discovering.clear();
    m_isDiscovering = false;
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);

    /*
     * Tell the name service to stop calling us back if it's there (we may get
     * called more than once in the chain of destruction) so the pointer is not
     * required to be non-NULL.
     */
    QCC_DbgPrintf(("UDPTransport::Stop(): Clear NS callback"));
    IpNameService::Instance().SetCallback(TRANSPORT_UDP, NULL);

    IpNameService::Instance().SetNetworkEventCallback(TRANSPORT_UDP, NULL);

    /*
     * Ask any running endpoints to shut down and stop allowing routing to
     * happen through this transport.  The endpoint needs to wake any threads
     * that may be waiting for I/O and arrange for itself to be cleaned up by
     * the maintenance thread.
     */
    QCC_DbgPrintf(("UDPTransport::Stop(): Stop endpoints"));
    QCC_DbgPrintf(("UDPTransport::Stop(): Taking pre-auth list lock"));
    m_preListLock.Lock(MUTEX_CONTEXT);
    for (set<UDPEndpoint>::iterator i = m_preList.begin(); i != m_preList.end(); ++i) {
        UDPEndpoint ep = *i;
        /*
         * Since the UDP Transport is going down, we assume the entire routing
         * node is going down for a reason, and we should not be waiting for
         * queued data to make its way out to the remote side.  We don't delay
         * the shutdown process for this reason at all.
         */
        ep->SetEpWaitEnable(false);
        ep->Stop();
    }
    QCC_DbgPrintf(("UDPTransport::Stop(): Giving pre-auth list lock"));
    m_preListLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::Stop(): Taking endpoint list lock"));
    m_endpointListLock.Lock(MUTEX_CONTEXT);
    for (set<UDPEndpoint>::iterator i = m_authList.begin(); i != m_authList.end(); ++i) {
        UDPEndpoint ep = *i;
        ep->SetEpWaitEnable(false);
        ep->Stop();
    }
    for (set<UDPEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        UDPEndpoint ep = *i;
        ep->SetEpWaitEnable(false);
        ep->Stop();
    }

    /*
     * If there are any threads blocked trying to connect to a remote host, we
     * need to wake them up so they leave before we actually go away.  We are
     * guaranteed by contract that if there is an entry in the connect threads
     * set, the thread is still there and the event has not been destroyed.
     * This is critical since the event was created on the stack of the
     * connecting thread.  These entries will be verified as being gone in
     * Join().
     */
    QCC_DbgPrintf(("UDPTransport::Stop(): Alert connectThreads"));
    for (set<ConnectEntry>::const_iterator i = m_connectThreads.begin(); i != m_connectThreads.end(); ++i) {
        i->m_event->SetEvent();
    }

    QCC_DbgPrintf(("UDPTransport::Stop(): Giving endpoint list lock"));
    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    /*
     * This is the logical place to stop the dispatcher thread and the main
     * server thread, but we might have outstanding writes queued up with ARDP
     * at this point.  The writes come back through callbacks which are driven
     * by the main thread and picked up and executed by the dispatcher thread.
     * We need to keep those puppies running until the number of outstanding
     * writes for all streams drop to zero -- so we do the stop in Join() where
     * we can block until this all happens.  The endpoint Stop() calls above
     * will start the process.
     */
    DecrementAndFetch(&m_refCount);
    return ER_OK;
}

/**
 * Wait for all of the threads that may be wandering around in the UDP Transport
 * or its associated endpoints to complete their cleanup process and leave the
 * transport.  When this method completes, it must be safe to delete the object.
 * Note that this method may be called multiple times.
 */
QStatus UDPTransport::Join(void)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::Join()"));

    /*
     * The logical place to stop the dispatcher thread and the main server
     * thread would be in Stop(), but we might have outstanding writes queued up
     * with ARDP at that point.  The writes come back through callbacks which
     * are driven by the main thread and picked up and executed by the
     * dispatcher thread.  We need to keep those puppies running until the
     * number of outstanding writes for all streams drop to zero -- so we do the
     * stop here in Join() where we can block until this all happens.  The
     * endpoint Stop() calls in UDPTransport::Stop() will begin the process.  We
     * need to wait for it to complete here.
     */
    QCC_DbgPrintf(("UDPTransport::Join(): Taking endpoint list lock"));
    m_endpointListLock.Lock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::Join(): Taking pre-auth list lock"));
    m_preListLock.Lock(MUTEX_CONTEXT);

    bool sendOutstanding;
    do {
        sendOutstanding = false;

        for (set<UDPEndpoint>::iterator i = m_preList.begin(); i != m_preList.end(); ++i) {
            UDPEndpoint ep = *i;
            ArdpStream* stream = ep->GetStream();
            if (stream && stream->GetSendsOutstanding()) {
                sendOutstanding = true;
            }
        }
        for (set<UDPEndpoint>::iterator i = m_authList.begin(); i != m_authList.end(); ++i) {
            UDPEndpoint ep = *i;
            ArdpStream* stream = ep->GetStream();
            if (stream && stream->GetSendsOutstanding()) {
                sendOutstanding = true;
            }
        }
        for (set<UDPEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
            UDPEndpoint ep = *i;
            ArdpStream* stream = ep->GetStream();
            if (stream && stream->GetSendsOutstanding()) {
                sendOutstanding = true;
            }
        }

        if (sendOutstanding) {
            QCC_DbgPrintf(("UDPTransport::Join(): Giving pre-auth list lock"));
            m_preListLock.Unlock(MUTEX_CONTEXT);

            QCC_DbgPrintf(("UDPTransport::Join(): Giving endpoint list lock"));
            m_endpointListLock.Unlock(MUTEX_CONTEXT);

            qcc::Sleep(10);

            QCC_DbgPrintf(("UDPTransport::Join(): Taking endpoint list lock"));
            m_endpointListLock.Lock(MUTEX_CONTEXT);

            QCC_DbgPrintf(("UDPTransport::Join(): Taking pre-auth list lock"));
            m_preListLock.Lock(MUTEX_CONTEXT);
        }
    } while (sendOutstanding);

    QCC_DbgPrintf(("UDPTransport::Join(): Giving pre-auth list lock"));
    m_preListLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::Join(): Giving endpoint list lock"));
    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    /*
     * Now that there are no more sends outstanding in ARDP, we can go ahead and
     * take down the message pumps and dispatcher threads since thre is no more
     * work.
     */
    QCC_DbgPrintf(("UDPTransport::Stop(): Stop() message pumps"));
    for (uint32_t i = 0; i < N_PUMPS; ++i) {
        m_messagePumps[i]->Stop();
    }

    QCC_DbgPrintf(("UDPTransport::Join(): Stop message dispatcher thread"));
    if (m_dispatcher) {
        m_dispatcher->Stop();
    }

    QCC_DbgPrintf(("UDPTransport::Join(): Stop exit dispatcher thread"));
    if (m_exitDispatcher) {
        m_exitDispatcher->Stop();
    }

    /*
     * Tell the main server loop thread to shut down.  We'll wait for it to do
     * so below.
     */
    QCC_DbgPrintf(("UDPTransport::Join(): Stop main thread"));
    QStatus status = Thread::Stop();
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::Join(): Failed to Stop() server thread"));
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * Join() all of the message pumps.
     */
    QCC_DbgPrintf(("UDPTransport::Join(): Join() message pumps"));
    for (uint32_t i = 0; i < N_PUMPS; ++i) {
        m_messagePumps[i]->Join();
    }

    /*
     * We waited for the dispatcher thread to finish dispatching all in-process
     * sends above, so it has nothing to do now and we can get rid of it.
     */
    QCC_DbgPrintf(("UDPTransport::Join(): Join and delete message dispatcher thread"));
    if (m_dispatcher) {
        m_dispatcher->Join();
        delete m_dispatcher;
        m_dispatcher = NULL;
    }

    QCC_DbgPrintf(("UDPTransport::Join(): Join and delete exit dispatcher thread"));
    if (m_exitDispatcher) {
        m_exitDispatcher->Join();
        delete m_exitDispatcher;
        m_exitDispatcher = NULL;
    }

    /*
     * Now, if we have any message buffers queued up in our own internals it is
     * time to get rid of them.
     */
    QCC_DbgPrintf(("UDPTransport::Join(): Return unused message buffers to ARDP"));
    while (m_workerCommandQueue.empty() == false) {
        WorkerCommandQueueEntry entry = m_workerCommandQueue.front();
        m_workerCommandQueue.pop();
        /*
         * The ARDP module will have allocated memory (in some private way) for
         * any messages that are waiting to be routed.  We can't just ignore
         * that situation or we may leak memory.  Give any buffers back to the
         * protocol before leaving.  The assumption here is that ARDP will do
         * the right think in ARDP_REcvReady() and not require a subsequent
         * call to ARDP_Run() which will not happen since the main thread is
         * Stop()ped.
         */
        if (entry.m_command == WorkerCommandQueueEntry::RECV_CB) {
            m_ardpLock.Lock();

#ifndef NDEBUG
            QStatus alternateStatus =
#endif
            ARDP_RecvReady(entry.m_handle, entry.m_conn, entry.m_rcv);
#ifndef NDEBUG
            if (alternateStatus != ER_OK) {
                QCC_DbgPrintf(("UDPTransport::Join(): ARDP_RecvReady() returns status==\"%s\"", QCC_StatusText(alternateStatus)));
            }
#endif
            m_ardpLock.Unlock();
        }

        /*
         * Similarly, we may have copied out the BusHello in a connect callback
         * so we need to delete that buffer if it's there.
         */
        if (entry.m_command == WorkerCommandQueueEntry::CONNECT_CB) {
#ifndef NDEBUG
            CheckSeal(entry.m_buf + entry.m_len);
#endif
            delete[] entry.m_buf;
        }
    }

    /*
     * It is legal to call Join() more than once, so it must be possible to
     * call Join() on a joined transport and also on a joined name service.
     * Note that the thread we are joining here is the single UDP Transport
     * maintenance thread.  When it finally closes, all of the threads
     * previously wandering around in the transport must be gone.
     */
    QCC_DbgPrintf(("UDPTransport::Join(): Join main thread"));
    status = Thread::Join();
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::Join(): Failed to Join() server thread"));
        DecrementAndFetch(&m_refCount);
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
     * We must have asked any running endpoints to shut down and to wake any
     * threads that may be waiting for I/O.  Before we delete the endpoints
     * out from under those threads, we need to wait until they actually
     * all leave the endpoints.  We are in a Join() so it's okay if we take
     * our time and since the transport is shutting down, no new endpoints
     * will be formed, so it is okay to hold the endpoint lock during the
     * Join()s.
     */
    QCC_DbgPrintf(("UDPTransport::Join(): Taking pre-auth list lock"));
    m_preListLock.Lock(MUTEX_CONTEXT);
    for (set<UDPEndpoint>::iterator i = m_preList.begin(); i != m_preList.end(); ++i) {
        UDPEndpoint ep = *i;
        ep->Join();
    }
    QCC_DbgPrintf(("UDPTransport::Join(): Giving pre-auth list lock"));
    m_preListLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::Join(): Taking endpoint list lock"));
    m_endpointListLock.Lock(MUTEX_CONTEXT);
    for (set<UDPEndpoint>::iterator i = m_authList.begin(); i != m_authList.end(); ++i) {
        UDPEndpoint ep = *i;
        ep->Join();
    }
    for (set<UDPEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        UDPEndpoint ep = *i;
        ep->Join();
    }

    /*
     * If there were any threads blocked waiting to connect through to a
     * remote host, they should have been woken up in Stop() and they should
     * now wake up and be leaving of their own accord.  We need to wait
     * until they are all actually done and gone before proceeding to what
     * will ultimately mean the destruction of the transport.
     */
    while (m_connectThreads.empty() == false) {
        QCC_DbgTrace(("UDPTransport::Join(): Waiting for %d. threads to exit", m_connectThreads.size()));

        /*
         * Okay, this is the last call.  If there are still any threads waiting
         * in UDPTransport::Connect() we need to convince them to leave.  Bug
         * Their events again.
         */
        for (set<ConnectEntry>::iterator j = m_connectThreads.begin(); j != m_connectThreads.end(); ++j) {
            j->m_event->SetEvent();
        }

        /*
         * Wait for "a while."  This means long enough to get all of the
         * threads scheduled and run so they can wander out of the endpoint.
         * We would like to wait on an event that is bugged when all threads
         * have left the endpoint, but that would mean an expensive event
         * per endpoint only to optimize during shutdown and we just can't
         * afford that.  So we poll, waiting long enough to ensure that our
         * thread is rescheduled.
         *
         * Some Linux boxes will busy-wait if the time is two milliseconds
         * or less, and most will round up to jiffy resolution (defaults to
         * 10 ms) and then bump again to the next higher Jiffy to ensure
         * that at least the requested time has elapsed. So we pick 10 ms
         * and expect the loop to run every 20 ms in the usual case,
         * ensuring that the waiting threads get time to run and leave.
         */
        QCC_DbgPrintf(("UDPTransport::Join(): Giving endpoint list lock"));
        m_endpointListLock.Unlock(MUTEX_CONTEXT);
        qcc::Sleep(10);
        QCC_DbgPrintf(("UDPTransport::Join(): Taking endpoint list lock"));
        m_endpointListLock.Lock(MUTEX_CONTEXT);
    }

    /*
     * The above loop will not terminate until all connecting threads are gone.
     * There are now no threads running in UDP endpoints or in the transport and
     * since we already Join()ed the maintenance thread we can delete all of the
     * endpoints here.
     */
    set<UDPEndpoint>::iterator i;
    QCC_DbgPrintf(("UDPTransport::Join(): Taking pre-auth list lock"));
    m_preListLock.Lock(MUTEX_CONTEXT);
    while ((i = m_preList.begin()) != m_preList.end()) {
#ifndef NDEBUG
        UDPEndpoint ep = *i;
        QCC_DbgTrace(("UDPTransport::Join(): Erasing endpoint with conn ID == %d. from m_preList", ep->GetConnId()));
#endif
        m_preList.erase(i);
    }
    QCC_DbgPrintf(("UDPTransport::Join(): Giving pre-auth list lock"));
    m_preListLock.Unlock(MUTEX_CONTEXT);

    while ((i = m_authList.begin()) != m_authList.end()) {
#ifndef NDEBUG
        UDPEndpoint ep = *i;
        QCC_DbgTrace(("UDPTransport::Join(): Erasing endpoint with conn ID == %d. from m_authList", ep->GetConnId()));
#endif
        m_authList.erase(i);
        /*
         * If the endpoint is on the auth list, it occupies a slot on the
         * authentication count and the total connection count.  Update both
         * counts.
         */
        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);
    }

    while ((i = m_endpointList.begin()) != m_endpointList.end()) {
#ifndef NDEBUG
        UDPEndpoint ep = *i;
        QCC_DbgTrace(("UDPTransport::Join(): Erasing endpoint with conn ID == %d. from m_endpointList", ep->GetConnId()));
#endif

        /*
         * If the endpoint is on the endpoint list, it occupies a slot only on
         * the total connection count.  Update that count.
         */
        m_connLock.Lock(MUTEX_CONTEXT);
        m_endpointList.erase(i);
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);
    }

    QCC_DbgPrintf(("UDPTransport::Join(): Giving endpoint list lock"));
    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    m_stopping = false;
    DecrementAndFetch(&m_refCount);
    return ER_OK;
}

/**
 * This is a convenience function that tells a caller whether or not this
 * transport will support a set of options for a connection.  Lets the caller
 * decide up front whether or not a connection will succeed due to options
 * conflicts.
 */
bool UDPTransport::SupportsOptions(const SessionOpts& opts) const
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::SupportsOptions()"));

    bool rc = true;

    /*
     * UDP does not support raw sockets, since there is only one socket that
     * must be shared among all users; so we only return true if the traffic
     * type is TRAFFIC_MESSAGES.  It's not an error if we don't match, we just
     * don't have anything to offer.
     */
    if (opts.traffic != SessionOpts::TRAFFIC_MESSAGES) {
        QCC_DbgPrintf(("UDPTransport::SupportsOptions(): traffic type mismatch"));
        rc = false;
    }

    /*
     * The other session option that we need to filter on is the transport
     * bitfield.  If you are explicitly looking for something other than UDP we
     * can't help you.
     */
    if ((opts.transports & TRANSPORT_UDP) == false) {
        QCC_DbgPrintf(("UDPTransport::SupportsOptions(): transport mismatch"));
        rc = false;
    }

    QCC_DbgPrintf(("UDPTransport::SupportsOptions(): returns \"%s\"", rc == true ? "true" : "false"));
    DecrementAndFetch(&m_refCount);
    return rc;
}

/*
 * The default interface for the name service to use.  The wildcard character
 * means to listen and transmit over all interfaces that are up and multicast
 * capable, with any IP address they happen to have.  This default also applies
 * to the search for listen address interfaces.
 */
static const char* INTERFACES_DEFAULT = "*";

/**
 * This is a somewhat obscure method used by the AllJoyn object to determine if
 * there are possibly multiple ways to connect to an advertised bus address.
 * Our goal is to enumerate all of the possible interfaces over which we can be
 * contacted -- for example, eth0, wlan0 -- and construct bus address strings
 * matching each one.
 */
QStatus UDPTransport::GetListenAddresses(const SessionOpts& opts, std::vector<qcc::String>& busAddrs) const
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::GetListenAddresses()"));

    /*
     * We are given a session options structure that defines the kind of
     * transports that are being sought.  It's not an error if we don't match
     * requested options, we just don't have anything to offer.
     */
    if (SupportsOptions(opts) == false) {
        QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): Supported options mismatch"));
        DecrementAndFetch(&m_refCount);
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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("UDPTransport::GetListenAddresses(): NameService not started"));
        DecrementAndFetch(&m_refCount);
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
    QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): IfConfig()"));

    std::vector<qcc::IfConfigEntry> entries;
    QStatus status = qcc::IfConfig(entries);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::GetListenAddresses(): IfConfig() failed"));
        DecrementAndFetch(&m_refCount);
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
     * Walk the comma separated list from the configuration file and and try
     * to mach it up with interfaces actually found in the system.
     */
    for (std::set<qcc::String>::const_iterator it = interfaceSet.begin(); it != interfaceSet.end(); it++) {

        /*
         * We got a set of interfaces, so we need to work our way through
         * the set.  Each entry in the list  may be  an interface name, or a
         * wildcard.
         */
        qcc::String currentInterface = *it;

        QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): looking for interface %s", currentInterface.c_str()));

        /*
         * Walk the list of interfaces that we got from the system and see if
         * we find a match.
         */
        for (uint32_t i = 0; i < entries.size(); ++i) {
            QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): matching %s", entries[i].m_name.c_str()));

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
                QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): %s has correct state", entries[i].m_name.c_str()));
                if (haveWildcard || entries[i].m_name == currentInterface) {
                    QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): %s has correct name", entries[i].m_name.c_str()));
                    /*
                     * This entry matches our search criteria, so we need to
                     * turn the IP address that we found into a busAddr.  We
                     * must be a UDP transport, and we have an IP address
                     * already in a string, so we can easily put together the
                     * desired busAddr.
                     */
                    QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): %s match found", entries[i].m_name.c_str()));

                    /*
                     * We know we have an interface that speaks IP and which has
                     * an IP address we can pass back. We know it is capable of
                     * receiving incoming connections, but the $64,000 questions
                     * are, does it have a listener and what port is that
                     * listener listening on.
                     *
                     * There is one name service associated with the daemon UDP
                     * transport.  It may be advertising a different port on each
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
                    IpNameService::Instance().Enabled(TRANSPORT_UDP,
                                                      reliableIpv4PortMap, reliableIpv6Port,
                                                      unreliableIpv4PortMap, unreliableIpv6port);

                    /*
                     * If no listening port corresponding to this network interface is found in the map,
                     * then it hasn't been set and this implies that there is no listener for this transport
                     * on this network interface. We should only return a bus address corresponding to this
                     * network interface if we have a listener on this network interface.
                     *
                     * Note that if we find a "*" in the unreliableIPv4PortMap it is a wildcard and therefore
                     * matches the entry we are comparing to, in which case we are not comparing the entry to
                     * what's in the port map, we are using what's in the port map to confirm the entry.
                     */
                    bool portMapWildcard = unreliableIpv4PortMap.find(qcc::String("*")) != unreliableIpv4PortMap.end();
                    bool portMapExplicit = unreliableIpv4PortMap.find(entries[i].m_name) != unreliableIpv4PortMap.end();

                    if (portMapWildcard || portMapExplicit) {
                        uint16_t port = 0;
                        if (portMapWildcard) {
                            port = unreliableIpv4PortMap[qcc::String("*")];
                        } else {
                            port = unreliableIpv4PortMap[entries[i].m_name];
                        }

                        /*
                         * Now put this information together into a bus address
                         * that the rest of the AllJoyn world can understand.
                         */
                        if (!entries[i].m_addr.empty() && (entries[i].m_family == QCC_AF_INET)) {
                            qcc::String busAddr = "udp:addr=" + entries[i].m_addr + ","
                                                  "port=" + U32ToString(port) + ","
                                                  "family=ipv4";
                            QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): push busAddr=\"%s\"", busAddr.c_str()));
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
    QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): done"));
    DecrementAndFetch(&m_refCount);
    return ER_OK;
}

void UDPTransport::EmitStallWarnings(UDPEndpoint& ep)
{
    QCC_DbgTrace(("UDPTransport::EmitStallWarnings()"));

    ArdpStream* stream = ep->GetStream();
    assert(stream && "UDPTransport::EmitStallWarnings(): stream must exist");

    bool threadSetEmpty = stream->ThreadSetEmpty();
    bool disconnected = stream->GetDisconnected();

    Timespec tNow;
    GetTimeNow(&tNow);
    Timespec tStop = ep->GetStopTime();
    int32_t tRemaining = tStop + UDP_WATCHDOG_TIMEOUT - tNow;

    /*
     * If tRemaining is less than zero, the endpoint is stalled.
     */
    if (tRemaining < 0) {

        /*
         * Once stalled we don't want to print an error every time thought the
         * management loop, so we limit ourselves to logging errors every few
         * seconds
         */
        Timespec tStalled = ep->GetStallTime();
        int32_t tDelay = tStalled + UDP_STALL_REPORT_INTERVAL - tNow;

        if (tDelay > 0) {
            return;
        }

        ep->SetStallTime(tNow);

        QCC_LogError(ER_UDP_ENDPOINT_STALLED, ("UDPTransport::EmitStallWarnings(): Endpoint with conn ID == %d stalled", ep->GetConnId()));

        if (threadSetEmpty == false) {
            QCC_LogError(ER_UDP_ENDPOINT_STALLED, ("UDPTransport::EmitStallWarnings(): stalled not threadSetEmpty"));
        }

        if (disconnected == false) {
            QCC_LogError(ER_UDP_ENDPOINT_STALLED, ("UDPTransport::EmitStallWarnings(): stalled not disconnected"));
            ArdpStream* stream = ep->GetStream();
            if (stream) {
#ifndef NDEBUG
                bool disc = stream->GetDisconnected();
                bool discSent = stream->GetDiscSent();
                ArdpConnRecord* conn = stream->GetConn();
                bool suddenDisconnect = ep->GetSuddenDisconnect();
                QCC_DbgPrintf(("UDPTransport::EmitStallWarnings(): stalled not disconneccted. disc=\"%s\", discSent=\"%s\", conn=%p, suddendisconnect=\"%s\"", disc ? "true" : "false", discSent ? "true" : "false", conn, suddenDisconnect ? "true" : "false"));
#endif
                QCC_LogError(ER_UDP_ENDPOINT_STALLED, ("UDPTransport::EmitStallWarnings(): stalled not disconneccted."));
            } else {
                QCC_LogError(ER_UDP_ENDPOINT_STALLED, ("UDPTransport::EmitStallWarnings(): stalled not disconnected. No stream"));
            }
        }

        if (threadSetEmpty == false && disconnected == false) {
            QCC_LogError(ER_UDP_ENDPOINT_STALLED, ("UDPTransport::EmitStallWarnings(): stalled with threadSetEmpty and disconnected"));
        }
    }
}

/**
 * This method is used to deal with the lifecycle of all endpoints created by
 * the UDP Transport.  It is called on-demand and periodically by the main run
 * loop in order to detect connections / endpoints that are taking too long to
 * authenticate and also to deal with endpoints that are being torn down.
 *
 * The main complexities here are to ensure that there are no threads wandering
 * around in endpoints before we remove them, ensuring that the endpoints are
 * completely detached from the router and that the UDP Transport holds the final
 * reference to endpoints to make absolutely sure that there are going to be no
 * suprise threads popping up in a deleted object.  We also cannot block waiting
 * for things to happen, since we would block the protocol (as it stands now there
 * is one thread managing endpoints and driving ARDP).
 */
void UDPTransport::ManageEndpoints(Timespec authTimeout, Timespec sessionSetupTimeout)
{
    /*
     * We have an array of message pumps running in order to handle messages
     * that need to be pushed into the daemon router.  There is a chance that
     * any one of these pumps has had a message pump thread running, but that
     * pump decided that it could stop the thread due to a lack of message
     * traffic.  These threads that are stopped are called past threads.
     * Something needs to join those defunct pump threads and that something is
     * us.
     */
    QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): MessagePump::JoinPast()"));
    for (uint32_t i = 0; i < N_PUMPS; ++i) {
        m_messagePumps[i]->JoinPast();
    }

    set<UDPEndpoint>::iterator i;

    QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Taking endpoint list lock"));
    m_endpointListLock.Lock(MUTEX_CONTEXT);

    /*
     * If there are any endpoints on the preList, move them to the authList.
     */
    QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Taking pre-auth list lock"));
    m_preListLock.Lock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): There are %d. endpoints on m_preList", m_preList.size()));
    i = m_preList.begin();
    while (i != m_preList.end()) {
        QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Moving endpoint from m_preList to m_authList"));
        UDPEndpoint ep = *i;
#ifndef NDEBUG
        ep->PrintEpState("UDPTransport::ManageEndpoints(): m_preList");
#endif
        m_authList.insert(ep);
        m_preList.erase(i);
        i = m_preList.begin();
    }

    QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Giving pre-auth list lock"));
    m_preListLock.Unlock(MUTEX_CONTEXT);

    /*
     * This is an opportune time to deal with authenticating endpoints that have
     * snuck in (asynchronously completed) due to race conditions if we are
     * stopping.  If the transport is stopping, all of its endpoints must be in
     * the process of stopping, so we walk the list of active endpoints and call
     * Stop() on them.  If they are already stopping, no harm no foul.
     *
     * In the case of authenticating endpoints, we don't want to wait for some
     * large number of seconds to time out, so we Stop() them and move them out
     * of authenticating state.  If the endpoint is really in the middle of
     * authenticating, in-process authentication responses will just be dropped
     * at least until ARDP is finally killed, at which we will be all done
     * anyway.
     *
     * Calling Stop() on an active or authenticating endpoint will begin the
     * process of causing any waiting threads, etc., to be dislodged and remove
     * depenencies on the endpoints in question.  When we look to do the Join()
     * below, we will actually wait for the actions we start here to be
     * completed.
     */
    if (IsRunning() == false || m_stopping == true) {
        QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): m_stopping: Stopping endpoints on m_endpointList"));
        for (i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
            UDPEndpoint ep = *i;
#ifndef NDEBUG
            ep->PrintEpState("UDPTransport::ManageEndpoints(): m_stopping m_endpointList");
#endif
            ep->Stop();
        }

        i = m_authList.begin();
        while (i != m_authList.end()) {
            UDPEndpoint ep = *i;
#ifndef NDEBUG
            ep->PrintEpState("UDPTransport::ManageEndpoints(): m_stopping m_authList");
#endif
            ep->SetEpWaitEnable(false);
            ep->Stop();
            m_endpointList.insert(ep);
            m_authList.erase(i);
            i = m_authList.begin();
        }
    }

    /*
     * Run through the list of connections on the authList and cleanup any that
     * are taking too long to authenticate.  These are connections that are in
     * the middle of the three-way handshake.
     */
    QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): There are %d. endpoints on m_authList", m_authList.size()));
    bool changeMade = false;
    i = m_authList.begin();
    while (i != m_authList.end()) {
        UDPEndpoint ep = *i;

#ifndef NDEBUG
        ep->PrintEpState("UDPTransport::ManageEndpoints(): Managing m_authList");
#endif

        Timespec tNow;
        GetTimeNow(&tNow);

        if (ep->GetStartTime() + authTimeout < tNow) {
            QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Scavenging slow authenticator"));

            /*
             * If the authentication doesn't happen, the three-way handshake
             * doesn't complete and the endpoint just goes quiescent without
             * ever starting up.  If an endpoint sits on the list of endpoints
             * currently authenticating for too long, we need to just whack it.
             * If the endpoint was created during a passive accept, there is no
             * problem, but if the endpoint was created as part of an active
             * connection, there is a thread waiting for the Connect to finish,
             * so we need to wake it and let it leave before getting rid of the
             * endpoint.
             */
            bool threadWaiting = false;
            set<ConnectEntry>::iterator j = m_connectThreads.begin();
            for (; j != m_connectThreads.end(); ++j) {
                /*
                 * The question of the moment will be: is the endpoint referred
                 * to by the endpoint iterator i the same one referred to by the
                 * connect thread entry referred to by the entry iterator j.  If
                 * it is, then we have a thread blocked on that endpoint and we
                 * must wake it.  We rely on matching the connection IDs to make
                 * that call.
                 *
                 * Once we set the event to wake the thread, we still can't do
                 * anything until the thread actually leaves, at which point we
                 * will no longer find it in the connect threads set.
                 */
                if (j->m_connId == ep->GetConnId()) {
                    QCC_DbgTrace(("UDPTransport::ManageEndpoints(): Waking thread on slow authenticator with conn ID == %d.", j->m_connId));
                    j->m_event->SetEvent();
                    threadWaiting = true;
                    changeMade = true;
                }
            }

            /*
             * No threads waiting in this endpoint.  Just take it off of the
             * authList, make sure it is at least stopping and put it on the
             * endpoint list where it will be picked up and done away with.
             */
            if (threadWaiting == false) {
                QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Moving slow authenticator with conn ID == %d. to m_endpointList", ep->GetConnId()));

                /*
                 * If the endpoint is on the auth list, it occupies a slot on
                 * both the authentication count and the total conneciton count.
                 * If we remove it from the authentication list and put it on
                 * the endpoint list, we are saying we are done with
                 * authentication.  Update that count, but leave the endpoint as
                 * occupying a slot in the total connection count since it is an
                 * endpoint on a list.
                 */
                m_connLock.Lock(MUTEX_CONTEXT);
                m_authList.erase(i);

                /*
                 * If we are shutting down this endpoint because it hasn't
                 * authenticated we presume there cannot be any interesting
                 * information buffered for transmission to the remote side.  We
                 * therefore disable the EP_WAITING state.
                 */
                ep->SetEpWaitEnable(false);
                ep->Stop();
                m_endpointList.insert(ep);
                --m_currAuth;
                m_connLock.Unlock(MUTEX_CONTEXT);

                i = m_authList.upper_bound(ep);
                changeMade = true;
                continue;
            }
        }
        ++i;
    }

    /*
     * We've handled the authList, so now run through the list of connections on
     * the endpointList and cleanup any that are no longer running.
     */
    QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): There are %d. endpoints on m_endpointList", m_endpointList.size()));
    i = m_endpointList.begin();
    while (i != m_endpointList.end()) {
        UDPEndpoint ep = *i;

#ifndef NDEBUG
        ep->PrintEpState("UDPTransport::ManageEndpoints(): Managing m_endpointList");
#endif

        /*
         * We expect endpoints to be in one of the following states if they are
         * on the m_endpointList:
         *
         * EP_PASSIVE_STARTING: The endpoint was created as a result of an
         *     passive connection and was placed on the endpoint after making
         *     its way throught the pre_list and authList.  At this point the
         *     three-way handshake completed and the endpoint is in a transient
         *     state waiting for the daemon router registration process to
         *     complete.
         *
         * EP_ACTIVE_STARTING: The endpoint was created as a result of an
         *     active connection and was placed on the endpoint after the
         *     three-way handshake completed in a transient state waiting for
         *     the daemon router registration process to complete.
         *
         * EP_STARTED: The endpoint has been created and authenticated and
         *     connected to the daemon router.  It is acive and may be pumping
         *     messages back and forth.
         *
         * EP_STOPPING: Stop() has been called on this endpoint because of a
         *     locally or remotely initiated disconnect event.  Endpoints in
         *     this state must be Join()ed by this management thread.
         *
         * EP_WAITING: The endpoint has been Stop()ped, but the original event
         *     leading up to the Stop() was a local disconnect.  In this case
         *     we need to simulate shutdown(sockfd, SHUTRDWR) behavior and so
         *     we enter the EP_WAITING state until any outstanding sends to
         *     ARDP complete.
         *
         * EP_JOINED: Both Stop() and Join() have been called on this endpoint
         *     and there are no threads wandering through the endpoint.  In the
         *     case of a local disconnect, all outstanding ARDP sends have
         *     completed.  Once we reach the EP_JOINED state, a final Exit()
         *     function is scheduled which will drive the transition to EP_DONE
         *     state.
         *
         * EP_DONE: The endpoint has been Stop()ped, Join()ed and the Exit
         *     function has been called to ensure that the endpoint is
         *     completely detached from the daemon router.  We are free to
         *     delete endpoints in EP_DONE state.
         *
         * These are our expectations, so we assert that it is all true.
         */
        assert((ep->IsEpPassiveStarted() ||
                ep->IsEpActiveStarted() ||
                ep->IsEpStarted() ||
                ep->IsEpStopping() ||
                ep->IsEpWaiting() ||
                ep->IsEpJoined() ||
                ep->IsEpDone()) && "UDPTransport::ManageEndpoints(): Endpoint in unexpected state");

        /*
         * If the current endpoint is starting becuase of either an active or
         * passive connection request, it is in a transient state because a call
         * to the daemon router is outstanding.  We can't just delete the
         * endpoint out from under the daemon router which may be at any
         * arbitrarily vulnerable point.  We have to just let it finish.  If it
         * never does, we have bigger problems than a stalled endpoint.
         */
        if (ep->IsEpActiveStarted() || ep->IsEpPassiveStarted()) {
            Timespec tNow;
            GetTimeNow(&tNow);
            Timespec tStart = ep->GetStartTime();
            if (tNow - tStart > UDP_WATCHDOG_TIMEOUT) {
                QCC_LogError(ER_UDP_ENDPOINT_STALLED,
                             ("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d stalled (\"%s\")",
                              ep->GetConnId(), ep->IsEpActiveStarted() ? "EP_ACTIVE_STARTED" : "EP_PASSIVE_STARTED"));
#ifndef NDEBUG
                ep->PrintEpState("UDPTransport::ManageEndpoints(): Stalled");
#endif
            }
        }

        /*
         * Inside the following if statement is where the heavy lifting of the
         * state machine is done.  This can be a little tricky since the daemon
         * wants to reference count the endpoints and will call Stop() but not
         * Join() in RemoveSessionRef() for example.  This is because it doesn't
         * want to take possibly unbounded time to wait for the threads in the
         * endpoint to exit.  What that means is that we can arbitrarily find
         * ourselves with endpoints in EP_STOPPING state that were happily
         * running the last time we looked; and we may have to wait for queued
         * messages to be transmitted (if IsEpWaitEnabled() is true); and we
         * will have to do the Join() to finish tearing down the endpoint; and
         * we will have to Schedule the Exit() function to disconnect from the
         * Daemon Router; and we will have to ultimately delete the endpoints.
         * We must also be sensitive to blocking for long periods of time since
         * we are called out of the main loop of the transport.  This means
         * spending a lot of effort checking to make sure that functions will
         * complete without blocking before calling them.
         */
        if (ep->IsEpStopping() || ep->IsEpWaiting() || ep->IsEpJoined()) {
#ifndef NDEBUG
            if (ep->IsEpStopping()) {
                QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d is EP_STOPPING", ep->GetConnId()));
            } else if (ep->IsEpWaiting()) {
                QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d is EP_WAITING", ep->GetConnId()));
            } else {
                QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d is EP_JOINED", ep->GetConnId()));
            }
#endif

            /*
             * Since we are basically waiting for other things to do what
             * they're supposed to at this point, we call a method that prints
             * out warnings and errors if these things are not happening at a
             * reasonable pace.  There's not much we can do, but it is useful to
             * print this information.  The function itself limits the amount
             * of spew.
             */
            EmitStallWarnings(ep);

            ArdpStream* stream = ep->GetStream();
            assert(stream && "UDPTransport::ManageEndpoints(): stream must exist in states EP_STOPPING, EP_WAITING and EP_JOINED");

            /*
             * If we are in state EP_STOPPING, not too surprisingly, Stop() must
             * have been called by someone.
             *
             * When the endpoint transitions to EP_STOPPING, any new message
             * send requests are prevented from being passed on to the endpoint
             * and then to the underlying ArdpStream.
             *
             * We want to simulate shutdown behavior on a local disconnect; so
             * if the cause of the Stop() was a local disconnect, we need to
             * enter the EP_WAITING state, which is the shutdown state, and not
             * start the teardown process quite yet.
             *
             * If the cause of the Stop() was a remote disconnect, we go we want
             * to tear down the endpoint immediately.  At this point, that means
             * telling any threads waiting for PushMessage() to wake up, notice
             * that the endpoint is stopping and leave.
             *
             * There are possibly many threads that may be interested in the
             * instantaneous value of the state: there's us, who is trying to
             * manage the state change; there may be some number of threads
             * trying to write data into the endpoint, and there may be a
             * LeaveSession thread that may be causing an initial Stop().  We
             * take the endpoint state change lock and hold it while we consider
             * makeing state changes to prevent possible inconsistencies.
             *
             * Lock order is always endpointListLock then stateLock.  We have
             * the endpointListLock so we are okay.
             */
            ep->StateLock();
            if (ep->IsEpStopping()) {
                if (ep->IsEpWaitEnabled()) {
                    QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. entering EP_WAITING", ep->GetConnId()));
                    ep->SetEpWaiting();
                } else {
                    QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. WakeThreadSet()", ep->GetConnId()));
                    stream->WakeThreadSet();
                    /*
                     * If the reason we got to EP_STOPPING is because of a local
                     * disconnnect during transport shutdown, we need to make
                     * sure that a local disconnect is started so ARDP gets the
                     * word.  We do this if GetDiscSent() returns false
                     * indicating that we've never started a local Disconnect,
                     * and if GetDiscStatus is ER_OK indicating that a
                     * coincident remote disconnect did not start.
                     */
                    if (stream->GetDiscSent() == false && stream->GetDiscStatus() == ER_OK) {
                        QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. EP_STOPPING", ep->GetConnId()));
                        stream->Disconnect(false, ER_UDP_LOCAL_DISCONNECT);
                    }
                }
            }

            /*
             * The difference between EP_STOPPING and EP_WAITING at this point
             * is that if we are in EP_STOPPING, we and ARDP have already
             * started the disconnect dance and all we have to do is wait for
             * disconnected status to become true and threads to leave before
             * moving forward with the state transitions to EP_DONE.
             *
             * In the case of a local disconnect (EP_WAITING) we have to wait
             * for the send buffers to drain and any pending PushMessage calls
             * to finish.  When the shutdown is then complete, we need to
             * initiate the local disconnect to the stream, assert that there
             * are no further PushMessage()s in process and wait for the
             * disconnect dance with ARDP to complete.
             *
             * Here threadSetEmpty, if true, indicates that there are no threads
             * blocked waiting for operations involving the endpoint to
             * complete.  quiescent means that there are no ARDP send operations
             * in progress.  This is different from threadSetEmpty since threads
             * and ARDP operations may have different timelines.  For example, a
             * thread can start a send, but be alerted and exit leaving an ARDP
             * operation in progress.  Similarly, an ARDP operation may
             * complete, but a scheduled callback may not have executed to
             * notify a waiting thread.  disconnected means that the disconnect
             * call and callback dance between the UDP Transport and ARDP has
             * finished.
             */
            bool threadSetEmpty = stream->ThreadSetEmpty();
            bool disconnected = stream->GetDisconnected();

#ifndef NDEBUG
            if (ep->IsEpWaiting()) {
                QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. is EP_WAITING", ep->GetConnId()));
                QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): EP_WAITING with push message count %d", ep->GetPushMessageCount()));
                QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): EP_WAITING with sends outstanding %d.", stream->GetSendsOutstanding()));
            }
#endif

            /*
             * If we are in EP_WAITING state and 1) there are no threads that
             * may possibly be starting sends; and 2) ARDP has no queued
             * outstanding sends; then we are finished with the shutdown
             * behavior and we need to start the actual ARDP disconnect process
             * and transition back to EP_STOPPED where we wait for the process
             * to complete.
             */
            if (ep->IsEpWaiting() && ep->GetPushMessageCount() == 0 && stream->GetSendsOutstanding() == 0) {
                QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. exiting EP_WAITING", ep->GetConnId()));
                ep->SetEpWaitEnable(false);
                ep->SetEpStopping();
                stream->Disconnect(false, ER_UDP_LOCAL_DISCONNECT);
                disconnected = stream->GetDisconnected();
            }
            ep->StateUnlock();

            /*
             * If we are in EP_STOPPING state then we have 1) either
             * transitioned through EP_WAITING which means we have waited for
             * all outstanding sends to complete and then done the
             * stream->Disconnect(); or 2) we have bypassed EP_WAITING and gone
             * directly to EP_STOPPING since the disconnect is a result of a
             * control-C equivalent.  In this case we need to do the Join as
             * soon as possible, but no sooner.  We did a stream->Disconnect()
             * above which should have started the process but we need to wait
             * until the disconnect is complete and all buffers have been
             * returned from ARDP before actually destroying the endpoint.  So
             * we wait in Stopping state until this happens.  The Join() call
             * will make the endpoint completely unusable so we need to make
             * sure we are done.
             */
            if (ep->IsEpStopping() && ep->GetPushMessageCount() == 0 && stream->GetSendsOutstanding() == 0 && threadSetEmpty && disconnected) {
                QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Join()ing stopping endpoint with conn ID == %d.", ep->GetConnId()));

                /*
                 * We know we are in EP_STOPPING state here.  We just verified
                 * that we've dislodged any threads which may be waiting for the
                 * endpoint, and we've gone through our disconnect dance with
                 * ARDP.  We now expect that Join() will complete without having
                 * to wait for anything so we take that opportunity.
                 */
                QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Join() endpoint with conn ID == %d.", ep->GetConnId()));
                ep->Join();
                changeMade = true;

                /*
                 * At this point, we have Join()ed the endpoint, but the
                 * endpoint may still have tendrils connecting it to the Daemon
                 * Router.  Since disconnecting the Daemon Router involves a
                 * call out to some foreign land, we don't want to make that
                 * call with locks taken, first to avoid blocking the endpoint
                 * management thread and second in case the router decides to
                 * call back into ARDP (which could cause a deadlock).  So we
                 * shedule a function to make that happen; which then runs the
                 * Exit() function in the context of the dispatcher thread.
                 *
                 * Note: since the call out to register endpoints cannot be made
                 * with our locks held, we are never completely certain that a
                 * registration has succeeded or not.  For example, we could
                 * have put the endpoint on the endpoint list and begun the call
                 * out to the Daemon Router to register the endpoint, but
                 * received an ARDP_Disconnect() before the register call
                 * completed.  When we get the ARDP_Disconnect() we will start
                 * the process of tearing down the endpoint by calling Stop(),
                 * then Join() above.  Clearly, the Stop(), Join() and
                 * scheduling of the Exit() happens asynchronously with respect
                 * to the Daemon Registration process.  The registration process
                 * is a lengthy one involving creation of a virtual endpoint and
                 * starting the ExchangeNames process.  We expect that in a
                 * networked system having a connection go down in the middle of
                 * this process is not an unheard of situation and so we expect
                 * the Daemon Router to gracefully handle the case that an
                 * endpoint is invalidated and unregister is called before
                 * during or after a call to register.
                 *
                 * So, if we have not scheduled an Exit() function to be called
                 * on the endpoint, we do so irrespective of whether or not a
                 * register has completed or is in process.
                 *
                 * ExitEndpoint will schedule the endpoint Exit() method to be
                 * called which will set the state to EP_DONE when run, which
                 * then opens the final gate to the endpoint destruction process.
                 */
                if (ep->GetExitScheduled() == false) {
                    QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Schedule Exit() on endpoint with conn ID == %d.", ep->GetConnId()));
                    ep->SetExitScheduled();
                    ExitEndpoint(ep->GetConnId());
                    changeMade = true;
                }
#ifndef NDEBUG
            } else {
                QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. is not idle", ep->GetConnId()));
                QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): conn ID == %d. ep->IsEpStopping()=\"%s\"",
                               ep->GetConnId(), ep->IsEpStopping() ? "true" : "false"));
                QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): conn ID == %d. threadSetEmpty=\"%s\"",
                               ep->GetConnId(), threadSetEmpty ? "true" : "false"));
                QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): conn ID == %d. disconnected=\"%s\"",
                               ep->GetConnId(), disconnected ? "true" : "false"));
#endif
            }
        }

        /*
         * If we find the endpoint in the EP_DONE state, the endpoint is ready
         * to go away and there must be no pending operations of any sort.
         * Given that caveat, we can just pitch it.  When the reference count
         * goes to zero as a result of removing it from the endpoint list it
         * will be destroyed.  We should be the only thread looking at the
         * endoint if it has been removed from the router.
         */
        if (ep->IsEpDone()) {
            QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. is EP_DONE", ep->GetConnId()));
            if (ep->GetExited()) {
                QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. is exited", ep->GetConnId()));
                QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Removing reference for failed or done endpoint with conn ID == %d.",
                                 ep->GetConnId()));
                int32_t refs = ep->IncrementRefs();
                if (refs == 1) {
                    ep->DecrementRefs();
                    QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. is histoire", ep->GetConnId()));

                    /*
                     * If the endpoint is on the endpoint list, it occupies a
                     * slot only on the total connection count.  Update that
                     * count.
                     */
                    m_connLock.Lock(MUTEX_CONTEXT);
                    m_endpointList.erase(i);
                    --m_currConn;
                    m_connLock.Unlock(MUTEX_CONTEXT);

                    i = m_endpointList.upper_bound(ep);
                    changeMade = true;
                    continue;
                }
                ep->DecrementRefs();
            } else {
                QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. NOT exited", ep->GetConnId()));
            }
        }
        ++i;
    }

    if (changeMade) {
        m_manage = STATE_MANAGE;
        Alert();
    }

    QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Giving endpoint list lock"));
    m_endpointListLock.Unlock(MUTEX_CONTEXT);
}

#if ARDP_TESTHOOKS

/**
 * Provide do-nothing testhooks from the ARDP Protocol as a how-to example.
 */
void UDPTransport::ArdpSendToSGHook(ArdpHandle* handle, ArdpConnRecord* conn, TesthookSource source, qcc::ScatterGatherList& msgSG)
{
    QCC_DbgTrace(("UDPTransport::ArdpSendToSGHook(handle=%p, conn=%p, source=%d., msgSG=%p)", handle, conn, source, &msgSG));
}

void UDPTransport::ArdpSendToHook(ArdpHandle* handle, ArdpConnRecord* conn, TesthookSource source, void* buf, uint32_t len)
{
    QCC_DbgTrace(("UDPTransport::ArdpSendToHook(handle=%p, conn=%p, source=%d., buf=%p, len=%d.)", handle, conn, buf, len));
}

void UDPTransport::ArdpRecvFromHook(ArdpHandle* handle, ArdpConnRecord* conn, TesthookSource source, void* buf, uint32_t len)
{
    QCC_DbgTrace(("UDPTransport::ArdpSendToSGHook(handle=%p, conn=%p, source=%d., buf=%p, len=%d.)", handle, conn, buf, len));
}

#endif

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into
 * the transport.  Note we don't take the ARDP re-entrancy protection lock since
 * we are already executing in the context of a thread that has called ARDP_Run
 * and must already have taken the lock.
 *
 * Since we are called directly from ARDP here, we assume that the handle and
 * connection must be valid since ARDP wouldn't have called us with them set to
 * an invalid value.
 */
bool UDPTransport::ArdpAcceptCb(ArdpHandle* handle, qcc::IPAddress ipAddr, uint16_t ipPort, ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpAcceptCb(handle=%p, ipAddr=\"%s\", port=%d., conn=%p, buf =%p, len = %d)",
                  handle, ipAddr.ToString().c_str(), ipPort, conn, buf, len));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    return transport->AcceptCb(handle, ipAddr, ipPort, conn, buf, len, status);
}

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into
 * the transport.  Note we don't take the ARDP re-entrancy protection lock when
 * we call back out into ARDP since we are already executing in the context of a
 * thread that has called ARDP_Run and must already have taken the lock.
 *
 * Since we are called directly from ARDP here, we assume that the handle and
 * connection must be valid since ARDP wouldn't have called us with them set to
 * an invalid value.
 */
void UDPTransport::ArdpConnectCb(ArdpHandle* handle, ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpConnectCb(handle=%p, conn=%p, passive=%s, buf = %p, len = %d, status=%s)",
                  handle, conn, passive ? "true" : "false", buf, len, QCC_StatusText(status)));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    transport->ConnectCb(handle, conn, passive, buf, len, status);
}

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into
 * the transport.  Note we don't take the ARDP re-entrancy protection lock when
 * we call back out into ARDP since we are already executing in the context of a
 * thread that has called ARDP_Run and must already have taken the lock.
 *
 * Since we are called directly from ARDP here, we assume that the handle and
 * connection must be valid since ARDP wouldn't have called us with them set to
 * an invalid value.
 */
void UDPTransport::ArdpDisconnectCb(ArdpHandle* handle, ArdpConnRecord* conn, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpDisconnectCb(handle=%p, conn=%p, foreign=%d.)", handle, conn));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    transport->DisconnectCb(handle, conn, status);
}

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into
 * the transport.  Note we don't take the ARDP re-entrancy protection lock when
 * we call back out into ARDP since we are already executing in the context of a
 * thread that has called ARDP_Run and must already have taken the lock.
 *
 * Since we are called directly from ARDP here, we assume that the handle and
 * connection must be valid since ARDP wouldn't have called us with them set to
 * an invalid value.
 */
void UDPTransport::ArdpRecvCb(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* rcv, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpRecvCb(handle=%p, conn=%p, buf=%p, status=%s)",
                  handle, conn, rcv, QCC_StatusText(status)));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    transport->RecvCb(handle, conn, rcv, status);
}

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into
 * the transport.  Note we don't take the ARDP re-entrancy protection lock when
 * we call back out into ARDP since we are already executing in the context of a
 * thread that has called ARDP_Run and must already have taken the lock.
 *
 * Since we are called directly from ARDP here, we assume that the handle and
 * connection must be valid since ARDP wouldn't have called us with them set to
 * an invalid value.
 */
void UDPTransport::ArdpSendCb(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpSendCb(handle=%p, conn=%p, buf=%p, len=%d.)", handle, conn, buf, len));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    transport->SendCb(handle, conn, buf, len, status);
}

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into
 * the transport.  Note we don't take the ARDP re-entrancy protection lock when
 * we call back out into ARDP since we are already executing in the context of a
 * thread that has called ARDP_Run and must already have taken the lock.
 *
 * Since we are called directly from ARDP here, we assume that the handle and
 * connection must be valid since ARDP wouldn't have called us with them set to
 * an invalid value.
 */
void UDPTransport::ArdpSendWindowCb(ArdpHandle* handle, ArdpConnRecord* conn, uint16_t window, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpSendWindowCb(handle=%p, conn=%p, window=%d.)", handle, conn, window));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    transport->SendWindowCb(handle, conn, window, status);
}

/*
 * ARDP, being C, calls into the static member function ArdpAcceptCb().  That
 * function finds the transport "this" pointer in the handle and uses that to
 * call here, into the UDPTransport member method.
 *
 * Since we are called in the same thread context as ARDP here, we assume that
 * the handle and connection must be valid since ARDP wouldn't have called us
 * with them set to an invalid value.
 *
 * See "A note on connection establishment" earlier in this file to really make
 * sense of the following.
 *
 * This callback indicates that we are receiving a passive open request.  We are
 * in LISTEN state and are responding to another side that has done an
 * ARDP_Connect().  We expect it to have provided a Hello message which we get
 * in the data that comes along with the SYN segment.  Status should always be
 * ER_OK since it had to be to successfully get us to this point.
 *
 * If we can accept a new connection, we send a reply to the incoming Hello
 * message by calling ARDP_Accept() and we return true indicating that we have,
 * in fact, accepted the connection.
 *
 * Note we don't take the ARDP re-entrancy protection lock when we call back out
 * into ARDP since we are already executing in the context of a thread that has
 * called ARDP_Run and into the static handler and how we have been vectored
 * back to the transport object.  This means that the current thread must have
 * already have taken the lock.
 */
bool UDPTransport::AcceptCb(ArdpHandle* handle, qcc::IPAddress ipAddr, uint16_t ipPort, ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::AcceptCb(handle=%p, ipAddr=\"%s\", ipPort=%d., conn=%p)", handle, ipAddr.ToString().c_str(), ipPort, conn));

    /*
     * We never want to accept connections if we are shutting down.
     */
    if (IsRunning() == false || m_stopping == true) {
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("UDPTransport::AcceptCb(): Stopping or not running"));
        DecrementAndFetch(&m_refCount);
        return false;
    }

    if (buf == NULL || len == 0) {
        QCC_LogError(ER_UDP_INVALID, ("UDPTransport::AcceptCb(): No BusHello with SYN"));
        DecrementAndFetch(&m_refCount);
        return false;
    }

    /*
     * Here's the difficulty.  It is very common for external threads to call
     * into the UDP transport and take the endpoint list lock to locate an
     * endpoint and then take the ARDP lock to do do something with the network
     * protocol based on the stream in that endpoint.  The lock order here is
     * endpointListLock, then ardpLock.  It is also equally common for the main
     * thread to take the ardpLock and then call into ARDP_Run(), which can call
     * out into a callback.  Those callbacks then want to take the
     * enpointListLock to figure out how to deal with the callback.  The lock
     * order there is ardpLock, then endpointListLock.
     *
     * We usually get around that problem by dispatching all callbacks on the
     * dispatcher thread which can also use the endpointListLock, then ardpLock
     * lock order just like external threads.
     *
     * The problem here is that AcceptCb() needs to return a boolean indicating
     * whether or not it can accept a connection, and this depends on the number
     * of endpoints.  In order to look at the size of the lists of endpoints,
     * you need to take the endpoint list lock, which would result in a
     * potential deadlock.  Also, if we create an endpoint, we need to take the
     * endpointListLock in order to add it to the auth list.  In other words, we
     * must take the endpoint list lock, but we cannot take the endpoint list
     * lock.
     *
     * To work around the number of available endpoints issue, we keep an
     * atomically incremented and decremented number of available endpoints
     * around so we don't have to take a lock and call a size() method on a
     * couple of lists, as the TCP Transport would.  To work around the second
     * problem we do the addition of the new endpoint to a "pre" queue protected
     * by a third lock that must never held while either holding or taking the
     * ARDP lock (which is held here since we are in a callback).
     *
     * Bringing up a connection means transiently adding a connection to the
     * count of currently authenticating connections, and also to the list of
     * current connections.  Once the connectton is authenticated (complete) we
     * drop the currently-authenticating number back down.  Neither of these
     * numbers are allowed to exceed configured values.
     */
    m_connLock.Lock(MUTEX_CONTEXT);

    if (m_currAuth + 1U > m_maxAuth || m_currConn + 1U > m_maxConn) {
        QCC_LogError(ER_CONNECTION_LIMIT_EXCEEDED, ("UDPTransport::AcceptCb(): No slot for new connection"));
        m_connLock.Unlock(MUTEX_CONTEXT);
        DecrementAndFetch(&m_refCount);
        return false;
    }

    ++m_currAuth;
    ++m_currConn;

    m_connLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::AcceptCb(): Inbound connection accepted"));

    /*
     * We expect to get an org.alljoyn.Bus.BusHello message from the active side
     * in the data.
     */
    Message activeHello(m_bus);
    status = activeHello->LoadBytes(buf, len);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Can't LoadBytes() BusHello Message"));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    /*
     * Unmarshal the message.  We need to provide and endpoint unique name for
     * error reporting purposes, in order to to affix blame here if something
     * goes awry.  If we don't pass true in checkSender Unmarshal won't validate
     * the endpoint name and will just print it out in case of problems.  We
     * make (an illegal) one up since we don't have an endpoint yet.
     */
    qcc::String endpointName(":0.0");
    status = activeHello->Unmarshal(endpointName, false, false, true, 0);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Can't Unmarhsal() BusHello Message"));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    /*
     * Validate the fields in the incoming BusHello Message
     */
    if (strcmp(activeHello->GetInterface(), org::alljoyn::Bus::InterfaceName) != 0) {
        status = ER_BUS_ESTABLISH_FAILED;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Unexpected interface=\"%s\" in BusHello Message",
                              activeHello->GetInterface()));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    if (activeHello->GetCallSerial() == 0) {
        status = ER_BUS_ESTABLISH_FAILED;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Unexpected zero serial in BusHello Message"));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    if (strcmp(activeHello->GetDestination(), org::alljoyn::Bus::WellKnownName) != 0) {
        status = ER_BUS_ESTABLISH_FAILED;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Unexpected destination=\"%s\" in BusHello Message",
                              activeHello->GetDestination()));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    if (strcmp(activeHello->GetObjectPath(), org::alljoyn::Bus::ObjectPath) != 0) {
        status = ER_BUS_ESTABLISH_FAILED;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Unexpected object path=\"%s\" in BusHello Message",
                              activeHello->GetObjectPath()));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    if (strcmp(activeHello->GetMemberName(), "BusHello") != 0) {
        status = ER_BUS_ESTABLISH_FAILED;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Unexpected member name=\"%s\" in BusHello Message",
                              activeHello->GetMemberName()));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    /*
     * The remote name of the endpoint on the passive side of the connection is
     * the sender of the BusHello Message, presumably the local bus attachment
     * of the remote daemon doing the imlied Connect().
     */
    qcc::String remoteName = activeHello->GetSender();
    QCC_DbgPrintf(("UDPTransport::AcceptCb(): BusHello Message from sender=\"%s\"", remoteName.c_str()));

    status = activeHello->UnmarshalArgs("su");
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Can't UnmarhsalArgs() BusHello Message"));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    /*
     * We expect two arguments in the message: a remoteGUID and a protocol
     * version.  The high order two bits of the protocol version are the
     * nameTransfer bits that will tell the allJoyn obj how many names to
     * exchange during ExchangeNames.
     */
    size_t numArgs;
    const MsgArg* args;
    activeHello->GetArgs(numArgs, args);
    if (numArgs != 2 || args[0].typeId != ALLJOYN_STRING || args[1].typeId != ALLJOYN_UINT32) {
        status = ER_BUS_ESTABLISH_FAILED;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Unexpected number or type of arguments in BusHello Message"));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    qcc::String remoteGUID = args[0].v_string.str;
    uint32_t protocolVersion = args[1].v_uint32 & 0x3FFFFFFF;
    uint32_t nameTransfer = args[1].v_uint32 >> 30;

    QCC_DbgPrintf(("UDPTransport::AcceptCb(): Got BusHello(). remoteGuid=\"%s\", protocolVersion=%d., nameTransfer=%d.",
                   remoteGUID.c_str(), protocolVersion, nameTransfer));

    if (remoteGUID == m_bus.GetInternal().GetGlobalGUID().ToString()) {
        status = ER_BUS_SELF_CONNECT;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): BusHello was sent to self"));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    /*
     * We need to reply to the hello from the other side.  In order to do so we
     * need the unique name of the endpoint we are creating.  This means that it
     * is now time to create that new endpoint.
     */
    static const bool truthiness = true;
    UDPTransport* ptr = this;
    String normSpec = "udp:guid=" + remoteGUID + ",addr=" + ipAddr.ToString() + ",port=" + U32ToString(ipPort);
    UDPEndpoint udpEp(ptr, m_bus, truthiness, normSpec);

    /*
     * Some of this would "normally" be handled by EndpointAuth, but since we
     * are short-circuiting the process, we have to do the bookkeeping
     * ourselves.
     */
    udpEp->GetFeatures().isBusToBus = true;
    udpEp->GetFeatures().allowRemote = true;
    udpEp->GetFeatures().protocolVersion = protocolVersion;
    udpEp->GetFeatures().trusted = false;
    udpEp->GetFeatures().nameTransfer = static_cast<SessionOpts::NameTransferType>(nameTransfer);
    udpEp->SetRemoteGUID(remoteGUID);
    udpEp->SetPassive();
    udpEp->SetIpAddr(ipAddr);
    udpEp->SetIpPort(ipPort);
    udpEp->CreateStream(handle, conn, m_ardpConfig.initialDataTimeout, m_ardpConfig.totalDataRetryTimeout / m_ardpConfig.initialDataTimeout);
    udpEp->SetHandle(handle);
    udpEp->SetConn(conn);

    /*
     * Check any application connecting over UDP to see if it is running on the same machine and
     * set the group ID appropriately if so.
     */
    CheckEndpointLocalMachine(udpEp);

    /*
     * The unique name of the endpoint on the passive side of the connection is
     * a unique name generated on the passive side.  We are calling out to the
     * daemon but only to construct a GUID, so this is okay.
     */
    udpEp->SetUniqueName(m_bus.GetInternal().GetRouter().GenerateUniqueName());

    /*
     * the remote name of the endpoint on the passive side of the connection is
     * the sender of the BusHello, which is the local bus attachement on the
     * remote side that did the impled Connect().
     */
    udpEp->SetRemoteName(remoteName);

    /*
     * Now, we have an endpoint that we need to keep alive but not fully
     * connected and ready to flow AllJoyn Messages until we get the expected
     * response to our Hello.  That will come in as the ConnectCb we get in
     * passive mode that marks the end of the connection establishment phase.
     *
     * Set a timestamp in case this never comes for some reason.
     */
    Timespec tNow;
    GetTimeNow(&tNow);
    udpEp->SetStartTime(tNow);
    udpEp->SetStopTime(tNow);

    /*
     * Note that our endpoint isn't actually connected to anything yet or saved
     * anywhere.  Send a hello reply from our local endpoint.  The unique name
     * in the BusHello reponse is the unique name of our UDP endpoint we just
     * allocated above.
     */
    QCC_DbgPrintf(("UDPTransport::AcceptCb(): HelloReply(true, \"%s\")", udpEp->GetUniqueName().c_str()));
    status = activeHello->HelloReply(true, udpEp->GetUniqueName());
    if (status != ER_OK) {
        status = ER_UDP_BUSHELLO;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Can't make a BusHello Reply Message"));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    /*
     * The Function HelloReply creates and marshals the BusHello reply for the
     * remote side.  Once it is marshaled, there is a buffer associated with the
     * message that contains the on-the-wire version of the messsage.  The ARDP
     * code copies this data in to deal with the possibility of having to
     * retransmit it; So we need to remember to deal with freeing the buffer.
     */
    size_t helloReplyBufLen = activeHello->GetBufferSize();

#ifndef NDEBUG
    uint8_t* helloReplyBuf = new uint8_t[helloReplyBufLen + SEAL_SIZE];
    SealBuffer(helloReplyBuf + helloReplyBufLen);
#else
    uint8_t* helloReplyBuf = new uint8_t[helloReplyBufLen];
#endif

    memcpy(helloReplyBuf, const_cast<uint8_t*>(activeHello->GetBuffer()), helloReplyBufLen);

    /*
     * Since we are in a callback from ARDP we can note a few assumptions.
     * First, that callback must have been driven by a call to ARDP_Run() which
     * must be called with the ARDP lock taken; so we don't have to do it again.
     * Second, since ARDP is calling out to us, and it is the UDP transport main
     * thread that drives ARDP, the only thing that is going to happen is that
     * the SYN + ACK will be sent.  We don't have to deal with any possibility
     * of ARDP processing any inbound data while it is doing the accept.  We
     * take advantage of this by not putting the endpoint on the auth list until
     * we get status back from ARDP_Accept.
     */
    QCC_DbgPrintf(("UDPTransport::AcceptCb(): ARDP_Accept()"));
    status = ARDP_Accept(handle, conn, m_ardpConfig.segmax, m_ardpConfig.segbmax, helloReplyBuf, helloReplyBufLen);

    /*
     * No matter what, we need to free the buffer holding the Hello reply
     * message.
     */
    delete[] helloReplyBuf;
    helloReplyBuf = NULL;

    if (status != ER_OK) {
        /*
         * If ARDP_Accept returns an error, most likely it is becuase the underlying
         * SYN + ACK didn't go out.  The contract with ARDP says that if an error
         * happens here, we shouldn't expect a disconnect, so we just don't bother
         * to finish seting up the endpoint.
         *
         * Even though we haven't actually started the endpoint, we call Stop() to
         * set it up for the destruction process to make sure its state is changed
         * to be deletable (when we release our reference to it and the end of the
         * current scope).
         *
         * If we are stopping the endpoint before it ever really gets started, we
         * presume there can be no data queued for transmission, so we disable the
         * EP_WAITING state.
         */
        udpEp->SetEpWaitEnable(false);
        udpEp->Stop();
        QCC_LogError(status, ("UDPTransport::AcceptCb(): ARDP_Accept() failed"));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return false;
    }

    /*
     * Okay, this is now where we need to work around problem number two.  We
     * are going to tell ARDP to proceed with the connection shortly and we need
     * the endpoint we just created to make it onto the list of currently
     * authenticating endpoints, and we need this to happen without taking the
     * endpointListLock.  What we do is to put it on a "pre" authenticating list
     * that is dealt with especially carefully with respect to locks.
     *
     * Once we put the endpoint on a list, it is the responsibility of the code
     * managing the lists to deal with the authentication slots and total
     * connection slots.
     */
    QCC_DbgPrintf(("UDPTransport::AcceptCb(): Taking pre-auth list lock"));
    m_preListLock.Lock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::AcceptCb(): Adding endpoint with conn ID == %d. to m_preList", udpEp->GetConnId()));
    assert(udpEp->IsEpInitialized() && "UDPTransport::AcceptCb(): Unexpected endpoint state");

    /*
     * Set the endpoint state to indicate that it is starting up becuase of a
     * passive (begun by a remote host) connection attempt.
     */
    udpEp->SetEpPassiveStarted();
    m_preList.insert(udpEp);

    QCC_DbgPrintf(("UDPTransport::AcceptCb(): giving pre-auth list lock"));
    m_preListLock.Unlock(MUTEX_CONTEXT);

    /*
     * If we do something that is going to bug the ARDP protocol, we need to
     * call back into ARDP ASAP to get it moving.  This is done in the main
     * thread, which we need to wake up.  Since this is an accept it will
     * eventually require endpoint management, so we make a note to run the
     * endpoint management code.
     */
    m_manage = STATE_MANAGE;
    Alert();
    DecrementAndFetch(&m_refCount);
    return true;
}

#ifndef NDEBUG
void UDPTransport::DebugAuthListCheck(UDPEndpoint uep)
{
    QCC_DbgTrace(("UDPTransport::DebugAuthListCheck()"));
    m_endpointListLock.Lock(MUTEX_CONTEXT);
    for (set<UDPEndpoint>::iterator i = m_authList.begin(); i != m_authList.end(); ++i) {
        UDPEndpoint ep = *i;
        if (uep->GetConnId() == ep->GetConnId()) {
            QCC_DbgPrintf(("UDPTransport::DebugAuthListCheck(): Endpoint with conn ID == %d. already on m_authList", uep->GetConnId()));
            assert(0 && "UDPTransport::DebugAuthListCheck(): Endpoint already on m_authList");
        }
    }
    m_endpointListLock.Unlock(MUTEX_CONTEXT);
}

void UDPTransport::DebugEndpointListCheck(UDPEndpoint uep)
{
    QCC_DbgTrace(("UDPTransport::DebugEndpointListCheck()"));
    m_endpointListLock.Lock(MUTEX_CONTEXT);
    for (set<UDPEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        UDPEndpoint ep = *i;
        if (uep->GetConnId() == ep->GetConnId()) {
            QCC_DbgPrintf(("UDPTransport::DebugEndpointListCheck(): Endpoint with conn ID == %d. already on m_endpointList", uep->GetConnId()));
            assert(0 && "UDPTransport::DebugAuthListCheck(): Endpoint already on m_endpointList");
        }
    }
    m_endpointListLock.Unlock(MUTEX_CONTEXT);
}
#endif

/*
 * ARDP, being C, calls into the static member function ArdpConnectCb().  That
 * function finds the transport "this" pointer in the handle and uses that to
 * call here, into the UDPTransport member method ConnectCb().  Since we need to
 * call out to the daemon code in order to plumb in a new connection, we cannot
 * just call out from the ARDP callback.  We have to arrange for the connection
 * to really be handled in another thread.  The result of that process is that
 * we are called here.
 *
 * Since we are not called in the same thread context as ARDP here, we cannot
 * assume that the handle and connection are valid since ARDP has since run off
 * and possibly done many other things before we get scheduled and run.  What
 * this means to us is that we cannot make any assumptions about the connection
 * referred to being valid.  We have to admit the possibility that the "conn"
 * has gone away for some reason.
 *
 * See "A note on connection establishment" earlier in this file to really make
 * sense of the following.
 *
 * If passive is true, and status = ER_OK, this callback indicates that we are
 * getting the final callback as a result of the final part of the three-way
 * handshake.  The passive connection was started with a SYN (+ BusHello),
 * replied with a SYN + ACK (+ BusHello) and is now getting the final ACK back.
 *
 * If passive is false, and status = ER_OK, this callback indicates that the
 * passive (other) side has accepted the connection and has returned the SYN +
 * ACK.  We should see a BusHello message and a BusHello reply from the (other)
 * passive side in the data provided.
 *
 * If status != ER_OK, the status should be ER_TIMEOUT indicating that for some
 * reason the three-way handshake did not complete in the expected time/retries.
 *
 * Note we must take the ARDP re-entrancy protection lock if we call back out
 * into ARDP since we are executing in the context of a dispatcher thread and
 * not the callback directly.
 */
void UDPTransport::DoConnectCb(ArdpHandle* handle, ArdpConnRecord* conn, uint32_t connId, bool passive, uint8_t* buf, uint16_t len, QStatus status)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::DoConnectCb(handle=%p, conn=%p)", handle, conn));

    /*
     * If the transport has been asked to stop, we don't want to take any more
     * actions that could cause endpoints to be brought up.  We expect the
     * process of stopping will cause threads waiting active connections to
     * complete will be stopped in other ways, so it is safe to kill possible
     * responses here.
     */
    if (IsRunning() == false || m_stopping == true) {
        status = ER_UDP_STOPPING;
        QCC_LogError(status, ("ArdpStream::PushBytes(): UDP Transport not running or stopping"));
        DecrementAndFetch(&m_refCount);
        return;
    }

    /*
     * We are in DoConnectCb() which is always run off of the dispatcher thread.
     * If we are going to take the preListLock and munge the preList we
     * absolutely, positively must not try to take ardpLock with preListLock
     * taken or we risk deadlock.  In AcceptCb() which did have ardpLock taken,
     * we put any endpoints starting the authentication process on the
     * m_preList, so we have to move those to the m_authList where they are
     * really expected to be from now on.  ManageEndpoints touches both lists
     * using the lock order endpointList, preList; so we must do the same.
     */
    QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Taking endpoint list lock"));
    m_endpointListLock.Lock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Taking pre-auth list lock"));
    m_preListLock.Lock(MUTEX_CONTEXT);

    set<UDPEndpoint>::iterator i = m_preList.begin();
    while (i != m_preList.end()) {
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Moving endpoint from m_preList to m_authList"));
        UDPEndpoint ep = *i;
        m_authList.insert(ep);
        m_preList.erase(i);
        i = m_preList.begin();
    }

    QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Giving pre-auth list lock"));
    m_preListLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Giving endpoint list lock"));
    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    /*
     * The current connection ID is useful to have laying around in case we need
     * it for UDPTransport purposes.  The transport uses the connection ID as a
     * sort of unique number to tie things together; so it will work for us even
     * though the underlying ARDP connection from which we get it goes down.
     * The tricky part is taht since we are dispatched and not called in the
     * context of an ARDP connect callback, the underlying conn may actually be
     * gone and the connId might be ARDP_CONN_ID_INVALID right now.  Because of
     * this, we pass the connection ID (connId) given in the original callback
     * and don't try to get it now.
     *
     * Now we need to determine if this callback is a result of an active
     * (outgoing) connection request or a passive (incoming) request.
     */
    if (passive) {
        /*
         * On the passive side, when we get a ConnectCb, we're done with the
         * three-way handshake if no error is returned.  This marks the end of
         * the connection establishment phase and after we return, we should
         * expect AllJoyn messages to be flowing on the connection.
         *
         * If this is happening, we should have a UDPEndpoint on the m_authList
         * that reflects the ARDP connection that is in the process of being
         * formed.  We need to find that endpoint (based on the provided conn),
         * take it off of the m_authlist and put it on the active enpoint list.
         *
         * If an error has been returned, we are getting the one notification
         * that the connection has failed.  We nedd to find the endpoint that
         * has failed, so the error case looks pretty much like the success
         * case except for the final disposition.
         */
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): passive connection callback with conn ID == %d.", connId));
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Finding endpoint with conn ID == %d. in m_authList", connId));

        /*
         * We need to locate the endpoint that the provided conn parameter
         * refers to.  Since this is a passive connection, we must have
         * accepted the connection, created an endpoint, and placed that
         * endpoint on the list of currently authenticating endpoints
         * (m_authList).
         *
         * We loop through the endpoints on the auth list, looking for
         * a match in the connection ID between the provided connection
         * and the connection ID provided in the conn.
         *
         * In a perfect world, the connection we got from ARDP is still
         * valid, and the connection stored in the endpoint is still valid.
         * Unfortunatly, this is not guaranteed to be the case since time
         * has passed since the endpoint was created and time has also passed
         * since we got the callback from ARDP that drove the process to get
         * us here.
         *
         * If either of the following calls returns ARDP_CONN_ID_INVALID it
         * means that ARDP has decided to remove the connection.  In that case
         * we can no longer make any decision based on the conn ID.  If the
         * connection ID in the endpoint is no longer valid, it will never
         * receive a callback to indicate the connection it is waiting for has
         * completed.  That is okay, since it probably means a timeout has
         * happened.  We just ignore the endpoints with invalid connections and
         * let the endpoint manager time them out and clean them up.  If the
         * connection provided by ARDP is invalid it means that ARDP has removed
         * the connection between the time the precipitating ConnectCb() was
         * called and when we actually got run out of the dispatcher thread.  In
         * this case, there may be an endpoint waiting to hear the connect
         * result, but since ARDP changed its mind, it never will.  We just
         * ignore the callback with the (now) invalid connection and let the
         * endpoint manager time out the endpoint that may still be waiting for
         * the result and clean it up.  Long explanation, but simple
         * implementation.  Here, If cidFromConn is ARDP_CONN_ID_INVALID just
         * return since it is pointless to continue to bring up something that
         * will be unusable.
         */
        m_ardpLock.Lock();
        uint32_t cidFromConn = ARDP_GetConnId(m_handle, conn);
        m_ardpLock.Unlock();
        if (cidFromConn == ARDP_CONN_ID_INVALID) {
            DecrementAndFetch(&m_refCount);
            return;
        }

        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Taking endpoint list lock"));
        m_endpointListLock.Lock(MUTEX_CONTEXT);
        bool haveLock = true;
        set<UDPEndpoint>::iterator i;
        for (i = m_authList.begin(); i != m_authList.end(); ++i) {
            UDPEndpoint ep = *i;

            /*
             * If cidFromEp is ARDP_CONN_ID_INVALID there's nothing we can do fo
             * this endpoint.  Ignore it.  If it was the one referred to by the
             * now defunct conn, it will time out on its own.
             */
            m_ardpLock.Lock();
            uint32_t cidFromEp = ARDP_GetConnId(m_handle, ep->GetConn());
            m_ardpLock.Unlock();
            if (cidFromEp == ARDP_CONN_ID_INVALID) {
                continue;
            }

            if (ep->GetConn() == conn && cidFromEp == cidFromConn) {
                QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Moving endpoint with conn ID == %d to m_endpointList", connId));

                /*
                 * If the endpoint is on the auth list, it occupies a slot on
                 * both the authentication count and the total connection count.
                 * If we remove it from the authentication list and put it on
                 * the endpoint list, we are saying we are done with
                 * authentication, so we update that count, but leave the
                 * endpoint as occupying a slot in the total connection count
                 * since it is an endpoint on a list.
                 */
                m_connLock.Lock(MUTEX_CONTEXT);
                m_authList.erase(i);
                --m_currAuth;
#ifndef NDEBUG
                DebugEndpointListCheck(ep);
#endif
                m_endpointList.insert(ep);
                m_connLock.Unlock(MUTEX_CONTEXT);

                QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Start()ing endpoint with conn ID == %d.", connId));
                /*
                 * Cannot call out with the endpoint list lock taken.
                 */
                QCC_DbgPrintf(("UDPTransport::DoConnectCb(): giving endpoint list lock"));

                /*
                 * If the inbound connection succeeded, we need to tell the daemon
                 * that a new connection is ready to go.  If the connection failed
                 * we need to mark the connection for deletion and bug the endpoint
                 * management code so it can purge the endpoint without delay.
                 */
                if (status == ER_OK) {
                    m_endpointListLock.Unlock(MUTEX_CONTEXT);
                    haveLock = false;
                    ep->SetListener(this);
                    ep->Start();
                } else {
                    /*
                     * If the passive connection fails, it means that the
                     * three-way handshake did not complete successfully.  In
                     * this case, the contract is that ARDP will call us back
                     * and tell us; and we are supposed to acknowledge that fact
                     * by releasing the connection and promising not to use it
                     * again.
                     */
                    ArdpStream* stream = ep->GetStream();
                    assert(stream && "UDPTransport::DoConnectCb(): must have a stream at this point");
                    QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Disconnect() stream for endpoint with conn ID == %d.", connId));
                    stream->Disconnect(false, ER_UDP_LOCAL_DISCONNECT);

                    /*
                     * If the connection faied before it was actually set up, we
                     * presume that there can be no buffered data waiting to go
                     * out so we disable the EP_WAITING state.
                     */
                    ep->SetEpWaitEnable(false);
                    ep->Stop();
                    m_endpointListLock.Unlock(MUTEX_CONTEXT);
                    haveLock = false;

                    m_ardpLock.Lock();
                    ARDP_ReleaseConnection(handle, conn);
                    m_ardpLock.Unlock();
                    m_manage = UDPTransport::STATE_MANAGE;
                    Alert();
                }
                break;
            }
        }

        /*
         * If we didn't find the endpoint for the connection, we still have the
         * lock taken.
         */
        if (haveLock) {
            QCC_DbgPrintf(("UDPTransport::DoConnectCb(): giving endpoint list lock"));
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
        }
        DecrementAndFetch(&m_refCount);
        return;
    } else {
        /*
         * On the active side, we expect to be getting this callback when the
         * passive side does a SYN + ACK and provides a reply to our Hello
         * message that we sent in ARDP_Connect().
         *
         * Since this is an active connection, we expect there to be a thread
         * driving the connection and it will be waiting for something to happen
         * good or bad so we need to remember to wake it up if we can.
         *
         * We can only wake up the thread if we have a valid connection, from
         * which we can extract a context in which we stashed a qcc::Event* that
         * we use to bug the thread.  If ARDP reports the connection as valid
         * then we expect the event we find in the connection context to be
         * valid.
         *
         * An event used to wake the thread up is provided in the connection,
         * but we also need to make sure that the thread hasn't timed out or
         * been stopped for some other reason, in which case the event will not
         * be valid.
         */
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): active connection callback with conn ID == %d.", connId));
        m_ardpLock.Lock();
        bool connValid = ARDP_IsConnValid(m_handle, conn, connId);
        qcc::Event* event = static_cast<qcc::Event*>(ARDP_GetConnContext(m_handle, conn));
        m_ardpLock.Unlock();

        /*
         * We need to remember in the following code that we have a contract
         * with Connect() that says we take over ownership of the connection
         * limit counters after a successful call to ARDP_Connect().  Connect()
         * will have incremented both m_currAuth and m_currConn.  The
         * authentication actually happened in the ARDP connect process, so we
         * need to keep these couters updated as we move things around,
         * succeeding or failing.
         *
         * If the connection is no longer valid, there's nothing we can do in
         * terms of waking any connecting thread up.  The thread that started
         * all of this may or may not be there waiting.  If it is there, we
         * can't wake it up.  If it not there we are too late and even if we
         * proceed, that thread will not be satisfied and will be off trying to
         * figure out how to deal with the error.  We will assume the worst and
         * let that thread continue its ruminations in the case that it is gone.
         * If it is still there, we and just let it time out.  In either case,
         * we have it easy -- we just bail.
         */
        if (connValid == false) {
            QCC_LogError(status, ("UDPTransport::DoConnectCb(): Provided connection no longer valid"));
            m_ardpLock.Lock();
            ARDP_ReleaseConnection(handle, conn);
            m_ardpLock.Unlock();

            m_connLock.Lock(MUTEX_CONTEXT);
            --m_currAuth;
            --m_currConn;
            m_connLock.Unlock(MUTEX_CONTEXT);

            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * If the connection was valid, we expect to have an event still there
         * in its context.  The thread originally waiting for this event may not
         * be there, but since we put the event in, we assert that it (at least)
         * is still there.
         */
        assert(event && "UDPTransport::DoConnectCb(): Connection context did not provide an event");

        /*
         * Is there still a thread with an event on its stack waiting for us
         * here?  If there is, we need to bug it.  If the thread is gone, we go
         * there is no point in going though the motions, and creating an
         * endpoint we would just have to fail.
         */
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Taking endpoint list lock"));
        m_endpointListLock.Lock(MUTEX_CONTEXT);

        bool eventValid = false;
        for (set<ConnectEntry>::iterator j = m_connectThreads.begin(); j != m_connectThreads.end(); ++j) {
            if (j->m_connId == connId) {
                assert(j->m_event == event && "UDPTransport::DoConnectCb(): event != j->m_event");
                eventValid = true;
                break;
            }
        }

        /*
         * If there is no valid event, there is no thread waiting for the
         * connect to complete.  It must be gone if it has removed its
         * ConnectEntry; or there is a very unlikely chance that the connection
         * is down now, and was down when the thread created its entry and it
         * therefore used ARDP_CONN_ID_INVALID as its ID.  In that case, it will
         * just have to time out.  If the thread removed its valid ConnectEntry
         * it must have done so with the endpoint list lock taken, so we know it
         * is there if this test passes.
         */
        if (eventValid == false) {
            QCC_LogError(status, ("UDPTransport::DoConnectCb(): No thread waiting for Connect() to complete"));
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            m_ardpLock.Lock();
            ARDP_ReleaseConnection(handle, conn);
            m_ardpLock.Unlock();

            m_connLock.Lock(MUTEX_CONTEXT);
            --m_currAuth;
            --m_currConn;
            m_connLock.Unlock(MUTEX_CONTEXT);

            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * If the connect failed, wake up the thread waiting for completion
         * without creating an endpoint for it.
         */
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::DoConnectCb(): Connect error"));
            event->SetEvent();
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            m_ardpLock.Lock();
            ARDP_ReleaseConnection(handle, conn);
            m_ardpLock.Unlock();

            m_connLock.Lock(MUTEX_CONTEXT);
            --m_currAuth;
            --m_currConn;
            m_connLock.Unlock(MUTEX_CONTEXT);

            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * If we cannot find a BusHello, wake up the thread waiting for
         * completion without creating an endpoint for it.
         */
        if (buf == NULL || len == 0) {
            QCC_LogError(ER_UDP_INVALID, ("UDPTransport::DoConnectCb(): No BusHello reply with SYN + ACK"));
            event->SetEvent();
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            m_ardpLock.Lock();
            ARDP_ReleaseConnection(handle, conn);
            m_ardpLock.Unlock();

            m_connLock.Lock(MUTEX_CONTEXT);
            --m_currAuth;
            --m_currConn;
            m_connLock.Unlock(MUTEX_CONTEXT);

            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * Load the bytes from the BusHello reply into a Message.
         */
        Message helloReply(m_bus);
        status = helloReply->LoadBytes(buf, len);
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::DoConnectCb(): Can't Unmarhsal() BusHello Reply Message"));
            event->SetEvent();
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            m_ardpLock.Lock();
            ARDP_ReleaseConnection(handle, conn);
            m_ardpLock.Unlock();

            m_connLock.Lock(MUTEX_CONTEXT);
            --m_currAuth;
            --m_currConn;
            m_connLock.Unlock(MUTEX_CONTEXT);

            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * Unmarshal the message.  We need to provide and endpoint unique name
         * for error reporting purposes, in order to to affix blame here if
         * something goes awry.  If we don't pass true in checkSender Unmarshal
         * won't validate the endpoint name and will just print it out in case
         * of problems.  We make (an illegal) one up since we don't have an
         * endpoint yet.
         */
        qcc::String endpointName(":0.0");
        status = helloReply->Unmarshal(endpointName, false, false, true, 0);
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::DoConnectCb(): Can't Unmarhsal() BusHello Message"));
            event->SetEvent();
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            m_ardpLock.Lock();
            ARDP_ReleaseConnection(handle, conn);
            m_ardpLock.Unlock();

            m_connLock.Lock(MUTEX_CONTEXT);
            --m_currAuth;
            --m_currConn;
            m_connLock.Unlock(MUTEX_CONTEXT);

            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * Validate the fields in the incoming BusHello Reply Message
         */
        if (helloReply->GetType() != MESSAGE_METHOD_RET) {
            status = ER_BUS_ESTABLISH_FAILED;
            QCC_LogError(status, ("UDPTransport::DoConnectCb(): Response was not a reply Message"));
            event->SetEvent();
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            m_ardpLock.Lock();
            ARDP_ReleaseConnection(handle, conn);
            m_ardpLock.Unlock();

            m_connLock.Lock(MUTEX_CONTEXT);
            --m_currAuth;
            --m_currConn;
            m_connLock.Unlock(MUTEX_CONTEXT);

            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * The remote name is the sender of the BusHello reply message,
         * presumably the local bus attachment of the remote daemon doing
         * the implied Accept()
         */
        qcc::String remoteName = helloReply->GetSender();
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): BusHello reply from sender=\"%s\"", remoteName.c_str()));

        status = helloReply->UnmarshalArgs("ssu");
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::DoConnectCb(): Can't UnmarhsalArgs() BusHello Reply Message"));
            event->SetEvent();
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            m_ardpLock.Lock();
            ARDP_ReleaseConnection(handle, conn);
            m_ardpLock.Unlock();

            m_connLock.Lock(MUTEX_CONTEXT);
            --m_currAuth;
            --m_currConn;
            m_connLock.Unlock(MUTEX_CONTEXT);

            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * We expect three arguments in the message: the unique name of the
         * remote side, the remoteGUID and a protocol version.  The high order
         * two bits of the protocol version are the nameTransfer bits that will
         * tell the allJoyn obj how many names to exchange during ExchangeNames.
         */
        size_t numArgs;
        const MsgArg* args;
        helloReply->GetArgs(numArgs, args);
        if (numArgs != 3 || args[0].typeId != ALLJOYN_STRING || args[1].typeId != ALLJOYN_STRING || args[2].typeId != ALLJOYN_UINT32) {
            status = ER_BUS_ESTABLISH_FAILED;
            QCC_LogError(status, ("UDPTransport::DoConnectCb(): Unexpected number or type of arguments in BusHello Reply Message"));
            event->SetEvent();
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            m_ardpLock.Lock();
            ARDP_ReleaseConnection(handle, conn);
            m_ardpLock.Unlock();

            m_connLock.Lock(MUTEX_CONTEXT);
            --m_currAuth;
            --m_currConn;
            m_connLock.Unlock(MUTEX_CONTEXT);

            DecrementAndFetch(&m_refCount);
            return;
        }

        qcc::String uniqueName = args[0].v_string.str;
        qcc::String remoteGUID = args[1].v_string.str;
        uint32_t protocolVersion = args[2].v_uint32 & 0x3FFFFFFF;
        uint32_t nameTransfer = args[1].v_uint32 >> 30;

        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Got BusHello() reply. uniqueName=\"%s\", remoteGuid=\"%s\", protocolVersion=%d., nameTransfer=%d.",
                       uniqueName.c_str(), remoteGUID.c_str(), protocolVersion, nameTransfer));

        /*
         * We have everything we need to start up, so it is now time to create
         * our new endpoint.
         */
        m_ardpLock.Lock();
        qcc::IPAddress ipAddr = ARDP_GetIpAddrFromConn(handle, conn);
        uint16_t ipPort = ARDP_GetIpPortFromConn(handle, conn);
        m_ardpLock.Unlock();

        static const bool truthiness = true;
        UDPTransport* ptr = this;
        String normSpec = "udp:guid=" + remoteGUID + ",addr=" + ipAddr.ToString() + ",port=" + U32ToString(ipPort);
        UDPEndpoint udpEp(ptr, m_bus, truthiness, normSpec);

        /*
         * Make a note of when we started this process in case something
         * goes wrong.
         */
        Timespec tNow;
        GetTimeNow(&tNow);
        udpEp->SetStartTime(tNow);
        udpEp->SetStopTime(tNow);

        /*
         * Some of this would "normally" be handled by EndpointAuth, but since we
         * are short-circuiting the process, we have to do the bookkeeping
         * ourselves.
         */
        udpEp->GetFeatures().isBusToBus = true;
        udpEp->GetFeatures().allowRemote = true;
        udpEp->GetFeatures().protocolVersion = protocolVersion;
        udpEp->GetFeatures().trusted = false;
        udpEp->GetFeatures().nameTransfer = static_cast<SessionOpts::NameTransferType>(nameTransfer);
        udpEp->SetRemoteGUID(remoteGUID);
        udpEp->SetActive();
        udpEp->SetIpAddr(ipAddr);
        udpEp->SetIpPort(ipPort);
        udpEp->CreateStream(handle, conn, m_ardpConfig.initialDataTimeout, m_ardpConfig.totalDataRetryTimeout / m_ardpConfig.initialDataTimeout);
        udpEp->SetHandle(handle);
        udpEp->SetConn(conn);

        /*
         * Check any application connecting over UDP to see if it is running on the same machine and
         * set the group ID appropriately if so.
         */
        CheckEndpointLocalMachine(udpEp);

        /*
         * The unique name of the endpoint on the active side of the connection is
         * the unique name generated on the passive side.
         */
        udpEp->SetUniqueName(uniqueName);

        /*
         * The remote name of the endpoint on the active side of the connection
         * is the sender of the BusHello reply message, which is presumably the
         * local bus attachement on the remote side.
         */
        udpEp->SetRemoteName(remoteName);

        /*
         * From our perspective as the active opener of the connection, we are
         * done.
         */
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Adding endpoint with conn ID == %d. to m_endpointList", connId));

#ifndef NDEBUG
        DebugEndpointListCheck(udpEp);
#endif

        /*
         * We have a contract with Connect() that says we take over ownership of
         * the connection limit counters after a successful call to
         * ARDP_Connect().  Connect() will have incremented both m_currAuth and
         * m_currConn.  The authentication actually happened in the ARDP connect
         * process, so what we need to do here is to decrement m_currAuth, but
         * leave m_currConn as it is to reflect the now fully authenticated and
         * active connection.
         */
        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;

        /*
         * Before putting the endpoint on the endpoint list, set the state to
         * indicate that it is starting up becuase of an active (begun by this
         * local host) connection attempt.
         */
        udpEp->SetEpActiveStarted();
        m_endpointList.insert(udpEp);
        m_connLock.Unlock(MUTEX_CONTEXT);

        /*
         * We cannot call out to the daemon (which Start() will do) with the
         * endpointListLock taken.  This means that we will have to re-verify
         * that the thread originally attempting the connect is still there when
         * we come back.  If it is going, we need to arrange to undo the
         * following work, but that's the way the threading-cookie crumbles.
         */
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): giving endpoint list lock"));
        m_endpointListLock.Unlock(MUTEX_CONTEXT);

        /*
         * We now have a UDPEndpoint that needs to be Start()ed and put on the
         * active endpint list and hooked up to the demux so it can receive
         * inbound data.  It needs to be Start()ed not because there are threads
         * that need to be started, but that is where we register our endpoint
         * with the router, and that is what will start the ExchangeNames
         * process.
         */
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Start()ing endpoint with conn ID == %d.", connId));
        udpEp->SetListener(this);
        udpEp->Start();
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Taking endpoint list lock"));
        m_endpointListLock.Lock(MUTEX_CONTEXT);

        /*
         * There may be a thread waiting for this process to finish, so we need
         * to wake it up or understand what its disposition was or will be.  The
         * moment we gave up the m_endpointListLock above in order to keep from
         * holding it though some possibly lengthy daemon operations, the
         * endpoint management thread could have decided to tear down the
         * endpoint and invalidate all of our work.  The connection we so
         * carefully considered above could have been torn down by ARDP if the
         * other side decided to close the connection or RST it for some unknown
         * (to us) reason.
         *
         * At this point, we don't need to talk to ARDP, so the connection
         * identifier is all we really care about there.  We are using it to tie
         * our stuff together so the one we remembered at the start of this
         * method is fine to use in order to find the waiting thread.  If we got
         * to this piont, we have pulled an event out of a connection context
         * previously.  This is a pointer to an event, but it points to an event
         * that was allocated on the heap by the thread that may or may not be
         * still waiting for use to wake it.  We know the endpoint that was
         * formed as a result of this work is still there since we hold a
         * managed object reference to it.
         *
         * So, what we need to do is to find the thread that started this whole
         * long and involved process in order to make sure that the event is
         * still valid and bug that event.
         */
        eventValid = false;
        for (set<ConnectEntry>::iterator j = m_connectThreads.begin(); j != m_connectThreads.end(); ++j) {
            if (j->m_connId == connId) {
                /*
                 * We believe that looking at the connection ID is sufficient to
                 * idenfity the thread.  Double check this assumption by
                 * asserting that the pointer to the event the thread is waiting
                 * on is the same as the pointer to the event we discovered
                 * through the connection.  If it is not, our assumption that
                 * the connection ID is sufficient is broken.
                 */
                assert(j->m_event == event && "UDPTransport::DoConnectCb(): event != j->m_event");
                eventValid = true;
                break;
            }
        }

        /*
         * We're all done cranking up the endpoint.  If there's someone waiting,
         * we found them, so wake them up.  If there's nobody there, stop the
         * endpoint since someone changed their mind and we need to begin the
         * process of taking it down (and undoing all of that work we just did).
         */
        if (eventValid) {
            QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Waking thread waiting for endpoint"));
            event->SetEvent();
        } else {
            QCC_DbgPrintf(("UDPTransport::DoConnectCb(): No thread waiting for endpoint"));

            /*
             * If there is no thread waiting for the endpoint to connect, it cannot
             * have reported back success, so we presume there can be no buffered
             * data waiting go be transmitted over this endpoint.  Therefore we
             * disable the EP_WAITING state.
             */
            udpEp->SetEpWaitEnable(false);
            udpEp->Stop();
        }

        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): giving endpoint list lock"));
        m_endpointListLock.Unlock(MUTEX_CONTEXT);
        DecrementAndFetch(&m_refCount);
        return;
    }
}

/*
 * This is method that is called in order to begin the process of detaching from
 * the router.  We do this bu ultimately calling _RemoteEndpoint::Exit() in our
 * Exit() function. The _RemoteEndpoint::Exit() function calls out to to the
 * Daemon Router UnregisterEndpoint() function.  This function will, in turn,
 * remove the endpoint which will cause virtual endpoints to be removed.  This
 * means NameChanged signals will be generated which are then sent out other
 * endpoints.
 *
 * NameOwnerChanged messages are sent using PushMessage which may turn right
 * around and translate into a PushBytes() back into an ARDP stream.  If we run
 * this method out of the message dispatcher thread, and UnregisterEndpoint()
 * happens to cause a NameChanged signal that is sent out a UDP Endpoint, the
 * message dispatcher will block waiting for the PushBytes to complete, but the
 * SendCb will need to be processed by the dispatcher thread which is blocked.
 * This results in deadlock that is broken by the PushBytes watchdog timer.  Not
 * good.
 *
 * On the other hand, if we drive the UnregisterEndpoint() call out of the
 * endpoint manager, and the PushBytes encounters transmission errors, the
 * call to PushBytes may time out, in which UnregisterEndpoint will be delayed
 * by the entire retransmission time.  This would block the endpoint management
 * process for possibly a long time.
 *
 * So the only real option is to dedicate a separate thread to making our
 * EndpointExit calls, which we do.
 */
void UDPTransport::ExitEndpoint(uint32_t connId)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::ExitEndpoint(connId=%d.)", connId));

    /*
     * If m_exitDispatcher is NULL, it means we are shutting down and the
     * dispatcher has gone away before the endpoint management thread has
     * actually stopped running.  This is rare, but possible.
     */
    if (m_exitDispatcher == NULL) {
        QCC_DbgPrintf(("UDPTransport::ExitEndpoint(): m_exitDispatcher is NULL"));
        DecrementAndFetch(&m_refCount);
        return;
    }

    UDPTransport::WorkerCommandQueueEntry entry;
    entry.m_command = UDPTransport::WorkerCommandQueueEntry::EXIT;
    entry.m_connId = connId;

    QCC_DbgPrintf(("UDPTransport::ExitEndpoint(): sending EXIT request to exit dispatcher"));
    m_exitWorkerCommandQueueLock.Lock(MUTEX_CONTEXT);
    m_exitWorkerCommandQueue.push(entry);
    m_exitWorkerCommandQueueLock.Unlock(MUTEX_CONTEXT);
    m_exitDispatcher->Alert();
    DecrementAndFetch(&m_refCount);
}

/*
 * ARDP, being C, calls into the static member function ArdpConnectCb().  That
 * function finds the transport "this" pointer in the handle and uses that to
 * call here, into the UDPTransport member method.
 *
 * Since we are called in the same thread context as ARDP here, we assume that
 * the handle and connection must be valid since ARDP wouldn't have called us
 * with them set to an invalid value.
 *
 * This is the indication from the ARDP protocol that a connection is in the
 * process of being formed.  We want to spend as little time as possible here
 * (and avoid deadlocks as much as possible here) so we just immediately ask the
 * transport dispatcher to do something with this message and return.  This case
 * is unlike the others because there may not be an endpoint to demux to yet.
 *
 * Note that we do not have to take the ARDP re-entrancy lock here because we
 * are in a callback from ARDP (via the associated static member function),
 * which must have been driven by a call to ARDP_Run which is required to be
 * protected by the re-entrancy lock.
 */
void UDPTransport::ConnectCb(ArdpHandle* handle, ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::ConnectCb(handle=%p, conn=%p, passive=%d., buf=%p, len=%d., status=\"%s\")",
                     handle, conn, passive, buf, len, QCC_StatusText(status)));

    /*
     * If m_dispatcher is NULL, it means we are shutting down and the message
     * dispatcher has gone away before the endpoint management thread has
     * actually stopped running.  This is rare, but possible.
     */
    if (m_dispatcher == NULL) {
        QCC_DbgPrintf(("UDPTransport::ConnectCb(): m_dispatcher is NULL"));
        DecrementAndFetch(&m_refCount);
        return;
    }

    UDPTransport::WorkerCommandQueueEntry entry;
    entry.m_command = UDPTransport::WorkerCommandQueueEntry::CONNECT_CB;
    entry.m_handle = handle;
    entry.m_conn = conn;
    entry.m_connId = ARDP_GetConnId(handle, conn);
    entry.m_passive = passive;
#ifndef NDEBUG
    entry.m_buf = new uint8_t[len + SEAL_SIZE];
    SealBuffer(entry.m_buf + len);
#else
    entry.m_buf = new uint8_t[len];
#endif
    entry.m_len = len;
    memcpy(entry.m_buf, buf, len);
    entry.m_status = status;

    QCC_DbgPrintf(("UDPTransport::ConnectCb(): sending CONNECT_CB request to dispatcher)"));
    m_workerCommandQueueLock.Lock(MUTEX_CONTEXT);
    m_workerCommandQueue.push(entry);
    m_workerCommandQueueLock.Unlock(MUTEX_CONTEXT);
    m_dispatcher->Alert();
    DecrementAndFetch(&m_refCount);
}

/*
 * ARDP, being C, calls into the static member function ArdpDisconnectCb().
 * That function finds the transport "this" pointer in the handle and uses that
 * to call here, into the UDPTransport member method.
 *
 * Since we are called in the same thread context as ARDP here, we assume that
 * the handle and connection must be valid since ARDP wouldn't have called us
 * with them set to an invalid value.
 *
 * This is the indication from the ARDP protocol that a connection has been
 * disconnected.  We want to spend as little time as possible here (and avoid
 * deadlocks as much as possible here) so we just immediately ask the transport
 * dispatcher to do something with this message and return.
 *
 * Note that we do not have to take the ARDP re-entrancy lock here because we
 * are in a callback from ARDP (via the associated static member function),
 * which must have been driven by a call to ARDP_Run which is required to be
 * protected by the re-entrancy lock.
 */
void UDPTransport::DisconnectCb(ArdpHandle* handle, ArdpConnRecord* conn, QStatus status)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::DisconnectCb(handle=%p, conn=%p, foreign=%d.)", handle, conn));

    /*
     * If m_dispatcher is NULL, it means we are shutting down and the dispatcher
     * has gone away before the endpoint management thread has actually stopped
     * running.  This is rare, but possible.
     */
    if (m_dispatcher == NULL) {
        QCC_DbgPrintf(("UDPTransport::DisconnectCb(): m_dispatcher is NULL"));
        DecrementAndFetch(&m_refCount);
        return;
    }

    UDPTransport::WorkerCommandQueueEntry entry;
    entry.m_command = UDPTransport::WorkerCommandQueueEntry::DISCONNECT_CB;
    entry.m_handle = handle;
    entry.m_conn = conn;
    entry.m_connId = ARDP_GetConnId(handle, conn);
    entry.m_status = status;

    QCC_DbgPrintf(("UDPTransport::DisconnectCb(): sending DISCONNECT_CB request to dispatcher)"));
    m_workerCommandQueueLock.Lock(MUTEX_CONTEXT);
    m_workerCommandQueue.push(entry);
    m_workerCommandQueueLock.Unlock(MUTEX_CONTEXT);
    m_dispatcher->Alert();
    DecrementAndFetch(&m_refCount);
}

/*
 * ARDP, being C, calls into the static member function ArdpRecvCb().  That
 * function finds the transport "this" pointer in the handle and uses that to
 * call here, into the UDPTransport member method.
 *
 * Since we are called in the same thread context as ARDP here, we assume that
 * the handle and connection must be valid since ARDP wouldn't have called us
 * with them set to an invalid value.
 *
 * This is the indication from the ARDP protocol that we have received bytes.
 * We want to spend as little time as possible here (and avoid deadlocks as much
 * as possible here) so we just immediately ask the transport dispatcher to do
 * something with this message and return.  The dispatcher will figure out which
 * endpoint this callback is destined for and demultiplex it accordingly.
 *
 * Note that we do not have to take the ARDP re-entrancy lock here because we
 * are in a callback from ARDP (via the associated static member function),
 * which must have been driven by a call to ARDP_Run which is required to be
 * protected by the re-entrancy lock.
 */
void UDPTransport::RecvCb(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* rcv, QStatus status)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::RecvCb(handle=%p, conn=%p, rcv=%p, status=%s)",
                     handle, conn, rcv, QCC_StatusText(status)));

    /*
     * If m_dispatcher is NULL, it means we are shutting down and the dispatcher
     * has gone away before the endpoint management thread has actually stopped
     * running.  This is rare, but possible.
     */
    if (m_dispatcher == NULL) {
        QCC_DbgPrintf(("UDPTransport::RecvCb(): m_dispatcher is NULL"));

#if RETURN_ORPHAN_BUFS

        QCC_DbgPrintf(("UDPTransport::RecvCb(): ARDP_RecvReady()"));
        m_ardpLock.Lock();
        ARDP_RecvReady(handle, conn, rcv);
        m_ardpLock.Unlock();

#else // not RETURN_ORPHAN_BUFS

        /*
         * If we get here, we have a receive callback from ARDP but we don't
         * have a message dispatcher to route the message to.  This is expected
         * (rarely) if the transport is going down (think control-c) but ARDP
         * still has messages queued up to be delivered.  Since there's nowhere
         * to send such messages, we just have to drop them.  Since the endpoint
         * has been or is being destroyed, we assume that ARDP will close down
         * the connection and release any pending buffers as part of the
         * endpoint teardown. This means that we can just ignore the buffer
         * here.
         */
        QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): Orphaned Recv_Cb for conn == %p ignored", conn));

#endif // not RETURN_ORPHAN_BUFS

        DecrementAndFetch(&m_refCount);
        return;
    }

    UDPTransport::WorkerCommandQueueEntry entry;
    entry.m_command = UDPTransport::WorkerCommandQueueEntry::RECV_CB;
    entry.m_handle = handle;
    entry.m_conn = conn;
    entry.m_connId = ARDP_GetConnId(handle, conn);
    entry.m_rcv = rcv;
    entry.m_status = status;

    QCC_DbgPrintf(("UDPTransport::RecvCb(): sending RECV_CB request to dispatcher)"));
    m_workerCommandQueueLock.Lock(MUTEX_CONTEXT);
    m_workerCommandQueue.push(entry);
    m_workerCommandQueueLock.Unlock(MUTEX_CONTEXT);
    m_dispatcher->Alert();
    DecrementAndFetch(&m_refCount);
}

/*
 * ARDP, being C, calls into the static member function ArdpSendCb().  That
 * function finds the transport "this" pointer in the handle and uses that to
 * call here, into the UDPTransport member method.
 *
 * Since we are called in the same thread context as ARDP here, we assume that
 * the handle and connection must be valid since ARDP wouldn't have called us
 * with them set to an invalid value.
 *
 * This is the indication from the ARDP protocol that we have (usually)
 * successfully sent bytes.  We want to spend as little time as possible here
 * (and avoid deadlocks as much as possible here) so we just immediately ask the
 * transport dispatcher to do something with this message and return.  The
 * dispatcher will figure out which endpoint this callback is destined for and
 * demultiplex it accordingly.
 *
 * Note that we do not have to take the ARDP re-entrancy lock here because we
 * are in a callback from ARDP (via the associated static member function),
 * which must have been driven by a call to ARDP_Run which is required to be
 * protected by the re-entrancy lock.
 */
void UDPTransport::SendCb(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::SendCb(handle=%p, conn=%p, buf=%p, len=%d.)", handle, conn, buf, len));

    /*
     * If m_dispatcher is NULL, it means we are shutting down and the dispatcher
     * has gone away before the endpoint management thread has actually stopped
     * running.  This is rare, but possible.
     */
    if (m_dispatcher == NULL) {
        QCC_DbgPrintf(("UDPTransport::SendCb(): m_dispatcher is NULL"));
        DecrementAndFetch(&m_refCount);
        return;
    }

    UDPTransport::WorkerCommandQueueEntry entry;
    entry.m_command = UDPTransport::WorkerCommandQueueEntry::SEND_CB;
    entry.m_handle = handle;
    entry.m_conn = conn;
    entry.m_connId = ARDP_GetConnId(handle, conn);
    entry.m_buf = buf;
    entry.m_len = len;
    entry.m_status = status;

    QCC_DbgPrintf(("UDPTransport::SendCb(): sending SEND_CB request to dispatcher)"));
    m_workerCommandQueueLock.Lock(MUTEX_CONTEXT);
    m_workerCommandQueue.push(entry);
    m_workerCommandQueueLock.Unlock(MUTEX_CONTEXT);
    m_dispatcher->Alert();
    DecrementAndFetch(&m_refCount);
}

/**
 * ARDP, being C, calls into the static member function ArdpSendWindowCb().
 * That function finds the transport "this" pointer in the handle and uses that
 * to call here, into the UDPTransport member method.
 *
 * Since we are called in the same thread context as ARDP here, we assume that
 * the handle and connection must be valid since ARDP wouldn't have called us
 * with them set to an invalid value.
 *
 * This is an indication from the ARDP Procotol that the send window has
 * changed.
 *
 * Note that we do not have to take the ARDP re-entrancy lock here because we
 * are in a callback from ARDP (via the associated static member function),
 * which must have been driven by a call to ARDP_Run which is required to be
 * protected by the re-entrancy lock.
 */
void UDPTransport::SendWindowCb(ArdpHandle* handle, ArdpConnRecord* conn, uint16_t window, QStatus status)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::SendWindowCb(handle=%p, conn=%p, window=%d.)", handle, conn, window));
    QCC_DbgPrintf(("UDPTransport::SendWindowCb(): callback from conn ID == %d", ARDP_GetConnId(handle, conn)));
    DecrementAndFetch(&m_refCount);
}

/**
 * A struct to hold information about IO_WRITE events that can be used
 * transiently to enable ARDP to wait for sockets to become writable after
 * it discovers the socket queue is full.
 *
 * Would be nice if it could be a local type to UDPTransport::Run() but Android
 * balks.
 */
struct WriteEntry {
    WriteEntry(bool active, SocketFd socket, Event* event) : m_active(active), m_socket(socket), m_event(event) { }
    bool m_active;      /**< If true indicates that ARDP wants to know when m_socket is writeable */
    SocketFd m_socket;  /**< The socket FD of interest */
    Event* m_event;     /**< The IO_WRITE event used to wait for writeable state */
};

/**
 * This is the run method of the main loop of the UDP Transport maintenance
 * thread -- the center of the UDP Transport universe.
 */
void* UDPTransport::Run(void* arg)
{
    QCC_DbgTrace(("UDPTransport::Run()"));

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
    while (IpNameService::Instance().Started() == false) {
        QCC_DbgPrintf(("UDPTransport::Run(): Wait for IP name service"));
        qcc::Sleep(10);
    }

    /*
     * ARDP wants to write its output to a datagram socket.  This socket has a
     * buffer.  If this buffer fills up, ARDP will begin getting ER_WOULDBLOCK
     * on its sendto calls.  ARDP will return ER_ARDP_WRITE_BLOCKED in this
     * case.
     *
     * When this happens, ARDP needs to wait until the underlying socket becomes
     * writable.  Since the select or epoll that is underlying our Event::Wait
     * is level, not edge triggered, we don't want to naively add write events,
     * which would then mostly be signaled causing the main loop to effectively
     * poll for readable sockets.
     *
     * In theory, we can have multiple listen specs for multiple interfaces,
     * which leads to multiple sockets coming ready and various times.  We call
     * ARDP sequentially with these different sockets, so When ARDP_Run()
     * returns ER_ARDP_WRITE_BLOCKED on a call in which a specific socket is
     * supplied, we should assume that it is this socket ARDP wants to know
     * about becoming writable again.  Since there can be multiple sockets, ARDP
     * can be interested in hearing about multiple sockets being writable.
     *
     * We don't want to be newing up annd deleting events all the time, so we
     * want to do the event creation when the list of listen FDs changes.  We
     * are always interested in the IO_READ events, but sometimes interested in
     * the IO_WRITE events.  Whenever we notice the list of listen FDs change,
     * we create a list of IO_READ events which will always be used, and a list
     * of corresponding IO_WRITE events from which we can pick and choose
     * depending on whether or not the look for the being-writable state.
     */
    vector<Event*> checkEvents, signaledEvents;
    vector<WriteEntry> writeEvents;

    qcc::Event ardpTimerEvent(qcc::Event::WAIT_FOREVER, 0);
    qcc::Event maintenanceTimerEvent(qcc::Event::WAIT_FOREVER, 0);

    checkEvents.push_back(&stopEvent);
    checkEvents.push_back(&ardpTimerEvent);
    checkEvents.push_back(&maintenanceTimerEvent);

    Timespec tLastManage;
    GetTimeNow(&tLastManage);

    QStatus status = ER_OK;

    /*
     * The purpose of this thread is to (1) manage all of our endpoints going
     * through the various states they do; (2) watch for the various sockets
     * corresponding to endpoints on sundry networks for becoming ready; and
     * (3) drive/whip the ARDP protocol to do our bidding.
     */
    while (true) {
        bool passive = m_preList.empty() && m_authList.empty() && m_endpointList.empty();

        if (IsStopping() && passive) {
            QCC_DbgPrintf(("UDPTransport::Run(): IsStopping() && passive: exiting"));
            break;
        }

        /*
         * Each time through the loop we need to wait on the stop event and all
         * of the SocketFds of the addresses and ports we are listening on.  We
         * expect the list of FDs to change rarely, so we want to spend most of
         * our time just driving the ARDP protocol and moving bits.  We only
         * redo the list if we notice the state changed from STATE_RELOADED.
         *
         * Instead of trying to figure out which of the socket FDs that may have
         * changed between runs, we just reload the whole bunch.  Since this is
         * rarely done, it is okay.
         */
        m_listenFdsLock.Lock(MUTEX_CONTEXT);
        if (m_reload != STATE_RELOADED) {
            QCC_DbgPrintf(("UDPTransport::Run(): Not STATE_RELOADED.  Deleting events"));
            for (vector<Event*>::iterator i = checkEvents.begin(); i != checkEvents.end(); ++i) {
                if (*i != &stopEvent && *i != &ardpTimerEvent && *i != &maintenanceTimerEvent) {
                    delete *i;
                }
            }

            for (vector<WriteEntry>::iterator i = writeEvents.begin(); i != writeEvents.end(); ++i) {
                delete i->m_event;
            }

            checkEvents.clear();
            writeEvents.clear();

            QCC_DbgPrintf(("UDPTransport::Run(): Not STATE_RELOADED. Creating events"));
            checkEvents.push_back(&stopEvent);
            checkEvents.push_back(&ardpTimerEvent);
            checkEvents.push_back(&maintenanceTimerEvent);

            QCC_DbgPrintf(("UDPTransport::Run(): Not STATE_RELOADED. Creating socket events"));
            for (list<pair<qcc::String, SocketFd> >::const_iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
                QCC_DbgPrintf(("UDPTransport::Run(): Not STATE_RELOADED. Creating event for socket %d", i->second));

                qcc::Event* eventRd = new Event(i->second, Event::IO_READ);
                checkEvents.push_back(eventRd);

                qcc::Event* eventWr = new Event(i->second, Event::IO_WRITE);
                WriteEntry entry(false, i->second, eventWr);
                writeEvents.push_back(entry);
            }

            m_reload = STATE_RELOADED;
        }
        m_listenFdsLock.Unlock(MUTEX_CONTEXT);

        /*
         * In order to rationalize management of resources, we manage the
         * various lists in one place on one thread.  This isn't super-expensive
         * but can add up if there are lots of endpoings, so We don't want to do
         * this resource management exercise every time throught the socket read
         * loop, so limit the number of times it will be called.
         */
        Timespec tNow;
        GetTimeNow(&tNow);

        int32_t tRemaining = tLastManage + UDP_ENDPOINT_MANAGEMENT_TIMER - tNow;

        if (m_manage != STATE_MANAGED || tRemaining < 0) {
            /*
             * Set m_manage to STATE_MANAGED before calling ManageEndpoints to
             * allow ManageEndpoints the possibility of causing itself to run
             * again immediately.
             */
            m_manage = STATE_MANAGED;
            ManageEndpoints(m_authTimeout, m_sessionSetupTimeout);
            tLastManage = tNow;
            uint32_t tManage = UDP_ENDPOINT_MANAGEMENT_TIMER;
            maintenanceTimerEvent.ResetTime(tManage, 0);
        }

        /*
         * We now have a list of checkEvents that includes all of the IO_READ
         * socket events and maybe some write events.  We have to decide which
         * write events should be tacked on the end and remove the others.
         * Since we expect the typical case will be to not be interested in
         * write events, we bias toward that case.  Every time through the loop
         * we remove all write events from checkEvents.  Then, we scan through
         * writeEvents looking for active events (which we expect to find only
         * rarely) and add them to checkEvents.  We expect to be working with
         * very small vectors here.
         */
        for (vector<Event*>::iterator i = checkEvents.begin(); i != checkEvents.end();) {
            Event* checkEvent = *i;

            if (checkEvent->GetEventType() == Event::IO_WRITE) {
                i = checkEvents.erase(i);
            } else {
                ++i;
            }
        }

        /*
         * Add any write events we are actively seeking.
         */
        for (vector<WriteEntry>::iterator i = writeEvents.begin(); i != writeEvents.end(); ++i) {
            if (i->m_active) {
                QCC_DbgPrintf(("UDPTransport::Run(): Looking for socket %d. to become writable"));
                checkEvents.push_back(i->m_event);
            }
        }

        signaledEvents.clear();

        status = Event::Wait(checkEvents, signaledEvents);
        if (status == ER_TIMEOUT) {
            // QCC_LogError(status, ("UDPTransport::Run(): Catching Windows returning ER_TIMEOUT from Event::Wait()"));
            continue;
        }

        if (ER_OK != status) {
            QCC_LogError(status, ("UDPTransport::Run(): Event::Wait failed"));
            break;
        }

        /*
         * We're back from our Wait() so one of four things has happened.  Our
         * thread has been asked to Stop(), our thread has been Alert()ed, our
         * timer has expired, or one of the socketFds we are listening on has
         * becomed signalled.
         *
         * If we have been asked to Stop(), or our thread has been Alert()ed,
         * the stopEvent will be on the list of signalled events.  The way we
         * tell the difference is by looking at IsStopping() which we do up at
         * the top of the loop.  In either case, we need to deal with managing
         * the endpoints.
         */
        for (vector<Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {

            /*
             * Reset stop and timer events since we've heard them.
             */
            if (*i == &stopEvent) {
                stopEvent.ResetEvent();
            } else if (*i == &maintenanceTimerEvent) {
                maintenanceTimerEvent.ResetEvent();
            } else if (*i == &ardpTimerEvent) {
                ardpTimerEvent.ResetEvent();
            }

            /*
             * Events of interest to ARDP is if the time requested has expired
             * or if some socket has become readable or writable.  First,
             * determine if this was a socket event (the socket became ready) or
             * if it was a timer event.  If it was a socket ready event,
             * determine if the socket became readable or writable.
             */
            bool socketReady = (*i != &ardpTimerEvent && *i != &maintenanceTimerEvent && *i != &stopEvent);
            bool readReady = (*i)->GetEventType() == Event::IO_READ;
            bool writeReady = (*i)->GetEventType() == Event::IO_WRITE;

            QCC_DbgPrintf(("UDPTransport::Run(): ARDP_Run(): readReady=\"%s\", writeReady=\"%s\"",
                           readReady ? "true" : "false", writeReady ? "true" : "false"));

            uint32_t ms;
            QStatus ardpStatus;
            m_ardpLock.Lock();
            if (socketReady) {
                ardpStatus = ARDP_Run(m_handle, (*i)->GetFD(), readReady, writeReady, &ms);
            } else {
                ardpStatus = ARDP_Run(m_handle, qcc::INVALID_SOCKET_FD, false, false, &ms);
            }
            m_ardpLock.Unlock();

            /*
             * Every time we call ARDP_Run(), it lets us know when its next
             * timer will expire, so we tell our event to set itself in that
             * number of milliseconds so we can call back then.  If it doesn't
             * have anything to do it returns -1 (WAIT_FOREVER).  Just because
             * it doesn't know about something happening doesn't mean something
             * will not happen on this side.  We need to bug this thread (send
             * an Alert() to wake us up) if we do anything that may require
             * deferred action.  Since we don't know what that might be, it
             * means we need to do the Alert() whenever we call into ARDP and
             * do something we expect to require a retransmission or callback.
             */
            ardpTimerEvent.ResetTime(ms, 0);

            /*
             * As described above, whenever we pass a socket into ARDP_Run() it
             * will return ER_ARDP_WRITE_BLOCKED if it wants us to call it back
             * when that socket becomes writable.  If ARDP_run() does not return
             * ER_ARDP_WRITE_BLOCKED it is telling us that it is able to write or that
             * we should not worry about that socket being writable.
             */
            if (socketReady) {
                for (vector<WriteEntry>::iterator j = writeEvents.begin(); j != writeEvents.end(); ++j) {
                    if ((*i)->GetFD() == (*j).m_socket) {
                        if (ardpStatus == ER_ARDP_WRITE_BLOCKED) {
                            QCC_DbgPrintf(("UDPTransport::Run(): ARDP_Run(): ER_ARDP_WRITE_BLOCKED for SocketFd=%d.",
                                           (*i)->GetFD()));
                            (*j).m_active = true;
                        } else {
                            QCC_DbgPrintf(("UDPTransport::Run(): ARDP_Run(): SocketFd=%d. not blocked for write",
                                           (*i)->GetFD()));
                            (*j).m_active = false;
                        }
                    }
                }
            }
        }
    }

    /*
     * Don't leak events when stopping.
     */
    for (vector<Event*>::iterator i = checkEvents.begin(); i != checkEvents.end(); ++i) {
        if (*i != &stopEvent && *i != &ardpTimerEvent && *i != &maintenanceTimerEvent) {
            delete *i;
        }
    }
    checkEvents.clear();

    for (vector<WriteEntry>::iterator i = writeEvents.begin(); i != writeEvents.end(); ++i) {
        delete i->m_event;
    }
    writeEvents.clear();

    /*
     * If we're stopping, it is our responsibility to clean up the list of FDs
     * we are listening to.  Since at this point we've Stop()ped and Join()ed
     * the protocol handlers, all we have to do is to close them down.
     *
     * Set m_reload to STATE_EXITED to indicate that the UDPTransport::Run
     * thread has exited.
     */
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    for (list<pair<qcc::String, SocketFd> >::iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
        qcc::Close(i->second);
    }
    m_listenFds.clear();
    m_reload = STATE_EXITED;
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::Run is exiting status=%s", QCC_StatusText(status)));
    return (void*) status;
}

/*
 * The purpose of this code is really to ensure that we don't have any listeners
 * active on Android systems if we have no ongoing advertisements.  This is to
 * satisfy a requirement driven from the Android Compatibility Test Suite (CTS)
 * which fails systems that have processes listening for UDP connections when
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
 * our UDP listeners, and therefore remove all listeners from the system.  Since
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
 *     information from the name service.  Finally, we shut down our UDP
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
void UDPTransport::RunListenMachine(ListenRequest& listenRequest)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::RunListenMachine()"));
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
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::StartListenInstance(ListenRequest& listenRequest)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::StartListenInstance()"));

    /*
     * We have a new StartListen request, so save the listen spec so we
     * can restart the listen if we stop advertising.
     */
    NewListenOp(START_LISTEN, listenRequest.m_requestParam);

    /*
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

#if ADVERTISE_ROUTER_OVER_UDP
    m_routerName = config->GetProperty("router_advertisement_prefix", ALLJOYN_DEFAULT_ROUTER_ADVERTISEMENT_PREFIX);
#endif

    if (m_isAdvertising || m_isDiscovering || (!m_routerName.empty() && (m_numUntrustedClients < m_maxUntrustedClients))) {
        m_routerName.append(m_bus.GetInternal().GetGlobalGUID().ToShortString());
        DoStartListen(listenRequest.m_requestParam);
    }
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::StopListenInstance(ListenRequest& listenRequest)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::StopListenInstance()"));

    /*
     * We have a new StopListen request, so we need to remove this
     * particular listen spec from our lists so it will not be
     * restarted.
     */
    bool empty = NewListenOp(STOP_LISTEN, listenRequest.m_requestParam);

    /*
     * If we have just removed the last listener, we have a problem if we have
     * advertisements.  This is because we will be advertising soon to be
     * non-existent endpoints.  The question is, what do we want to do about it.
     * We could just ignore it since since clients receiving advertisements may
     * just try to connect to a non-existent endpoint and fail.  It does seem
     * better to log an error and then cancel any outstanding advertisements
     * since they are soon to be meaningless.
     */
    if (empty && m_isAdvertising) {
        QCC_LogError(ER_UDP_NO_LISTENER, ("UDPTransport::StopListenInstance(): No listeners with outstanding advertisements"));
        for (list<qcc::String>::iterator i = m_advertising.begin(); i != m_advertising.end(); ++i) {
            IpNameService::Instance().CancelAdvertiseName(TRANSPORT_UDP, *i, TRANSPORT_UDP);
        }
    }

    /*
     * Execute the code that will actually tear down the specified
     * listening endpoint.  Note that we always stop listening
     * immediately since that is Good (TM) from a power and CTS point of
     * view.  We only delay starting to listen.
     */
    DoStopListen(listenRequest.m_requestParam);
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::EnableAdvertisementInstance(ListenRequest& listenRequest)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::EnableAdvertisementInstance()"));

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
                IpNameService::Instance().Enable(TRANSPORT_UDP, std::map<qcc::String, uint16_t>(), 0, m_listenPortMap, 0, false, false, true, false);
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
        QCC_DbgPrintf(("UDPTransport::EnableAdvertisementInstance(): Advertise with no UDP listeners"));
        if (!m_pendingAdvertisements.empty()) {
            for (std::list<ListenRequest>::iterator it = m_pendingAdvertisements.begin(); it != m_pendingAdvertisements.end(); it++) {
                if (listenRequest.m_requestParam == it->m_requestParam) {
                    DecrementAndFetch(&m_refCount);
                    return;
                }
            }
        }
        m_pendingAdvertisements.push_back(listenRequest);
        DecrementAndFetch(&m_refCount);
        return;
    }

    /*
     * We think we're ready to send the advertisement.  Are we really?
     */
    assert(m_isListening);
    assert(!m_listenPortMap.empty());
    assert(m_isNsEnabled);
    assert(IpNameService::Instance().Started() && "UDPTransport::EnableAdvertisementInstance(): IpNameService not started");

    QStatus status = IpNameService::Instance().AdvertiseName(TRANSPORT_UDP, listenRequest.m_requestParam, listenRequest.m_requestParamOpt, listenRequest.m_requestTransportMask);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::EnableAdvertisementInstance(): Failed to advertise \"%s\"", listenRequest.m_requestParam.c_str()));
    }

    QCC_DbgPrintf(("UDPTransport::EnableAdvertisementInstance(): Done"));
    m_isAdvertising = true;
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::DisableAdvertisementInstance(ListenRequest& listenRequest)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::DisableAdvertisementInstance()"));

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
    QStatus status = IpNameService::Instance().CancelAdvertiseName(TRANSPORT_UDP, listenRequest.m_requestParam, listenRequest.m_requestTransportMask);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::DisableAdvertisementInstance(): Failed to Cancel \"%s\"", listenRequest.m_requestParam.c_str()));
    }

    /* We check to make sure that this cancellation is not for an
     * advertisement that has not yet gone into effect because we
     * are still waiting on the specified network interface to
     * become IFF_UP. We don't want to send out advertisements
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
        IpNameService::Instance().Enable(TRANSPORT_UDP, std::map<qcc::String, uint16_t>(), 0, m_listenPortMap, 0, false, false, false, false);
        m_isNsEnabled = false;

        /*
         * If we had the name service running, we must have had listeners
         * waiting for connections due to the name service.  We need to stop
         * them all now, but only if we are not running on a Windows box.
         * Windows needs the listeners running at all times since it uses
         * UDP for the client to daemon connections. The listen spec may
         * involve network interfaces or IP addresses and we need to do
         * some translation from a specified network interface to the
         * corresponding IP address.
         */
        for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
            map<qcc::String, qcc::String> argMap;
            qcc::String spec;
            QStatus status = NormalizeListenSpec(i->c_str(), spec, argMap);
            assert(status == ER_OK && "UDPTransport::DisableAdvertisementInstance(): Invalid UDP listen spec");
            (void)status;  // Suppress unused warning from G++ when building is release mode.
            if (argMap.find("iface") != argMap.end()) {
                qcc::String interface = argMap["iface"];
                qcc::String normSpec = "udp:addr=" + m_requestedInterfaces[interface].GetAddress().ToString() + ",port=" + U32ToString(m_requestedInterfaces[interface].GetPort());
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
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::EnableDiscoveryInstance(ListenRequest& listenRequest)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::EnableDiscoveryInstance()"));

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
                IpNameService::Instance().Enable(TRANSPORT_UDP, std::map<qcc::String, uint16_t>(), 0, m_listenPortMap, 0, false, false, true, false);
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
        QCC_DbgPrintf(("UDPTransport::EnableDiscoveryInstance(): Discover with no UDP listeners"));
        if (!m_pendingDiscoveries.empty()) {
            for (std::list<ListenRequest>::iterator it = m_pendingDiscoveries.begin(); it != m_pendingDiscoveries.end(); it++) {
                if (listenRequest.m_requestParam == it->m_requestParam) {
                    DecrementAndFetch(&m_refCount);
                    return;
                }
            }
        }
        m_pendingDiscoveries.push_back(listenRequest);
        DecrementAndFetch(&m_refCount);
        return;
    }

    /*
     * We think we're ready to send the FindAdvertisement.  Are we really?
     */
    assert(m_isListening);
    assert(!m_listenPortMap.empty());
    assert(m_isNsEnabled);
    assert(IpNameService::Instance().Started() && "UDPTransport::EnableDiscoveryInstance(): IpNameService not started");

    QStatus status = IpNameService::Instance().FindAdvertisement(TRANSPORT_UDP, listenRequest.m_requestParam, listenRequest.m_requestTransportMask);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::EnableDiscoveryInstance(): Failed to begin discovery with multicast NS \"%s\"", listenRequest.m_requestParam.c_str()));
    }

    m_isDiscovering = true;
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::DisableDiscoveryInstance(ListenRequest& listenRequest)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::DisableDiscoveryInstance()"));

    /*
     * We have a new disable discovery request to deal with.  The first
     * order of business is to remove the well-known name from our saved list.
     */
    bool isFirst;
    bool isEmpty = NewDiscoveryOp(DISABLE_DISCOVERY, listenRequest.m_requestParam, isFirst);

    if (m_isListening && !m_listenPortMap.empty() && m_isNsEnabled && IpNameService::Instance().Started()) {
        QStatus status = IpNameService::Instance().CancelFindAdvertisement(TRANSPORT_UDP, listenRequest.m_requestParam, listenRequest.m_requestTransportMask);
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::DisableDiscoveryInstance(): Failed to cancel discovery with \"%s\"", listenRequest.m_requestParam.c_str()));
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

        IpNameService::Instance().Enable(TRANSPORT_UDP, std::map<qcc::String, uint16_t>(), 0, m_listenPortMap, 0, false, false, false, false);
        m_isNsEnabled = false;

        /*
         * If we had the name service running, we must have had listeners
         * waiting for connections due to the name service.  We need to stop
         * them all now, but only if we are not running on a Windows box.
         * Windows needs the listeners running at all times since it uses
         * UDP for the client to daemon connections.  The listen spec may
         * involve network interfaces or IP addresses and we need to do
         * some translation from a specified network interface to the
         * corresponding IP address.
         */
        for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
            map<qcc::String, qcc::String> argMap;
            qcc::String spec;
            QStatus status = NormalizeListenSpec(i->c_str(), spec, argMap);
            assert(status == ER_OK && "UDPTransport::DisableDiscoveryInstance(): Invalid UDP listen spec");
            (void)status;  // Suppress unused warning from G++ when building is release mode.
            if (argMap.find("iface") != argMap.end()) {
                qcc::String interface = argMap["iface"];
                qcc::String normSpec = "udp:addr=" + m_requestedInterfaces[interface].GetAddress().ToString() + ",port=" + U32ToString(m_requestedInterfaces[interface].GetPort());
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
    DecrementAndFetch(&m_refCount);
}

/*
 * The default address for use in listen specs.  INADDR_ANY means to listen
 * for UDP connections on any interfaces that are currently up or any that may
 * come up in the future.
 */
static const char* ADDR4_DEFAULT = "0.0.0.0";

/*
 * The default port for use in listen specs.
 */
static const uint16_t PORT_DEFAULT = 9955;

QStatus UDPTransport::NormalizeListenSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    qcc::String family;

    /*
     * We don't make any calls that require us to be in any particular state
     * with respect to threading so we don't bother to call IsRunning() here.
     *
     * Take the string in inSpec, which must start with "udp:" and parse it,
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
     *     "udp:addr=0.0.0.0,port=9955"
     *
     * That's all.  We still allow "addr=0.0.0.0,port=9955,family=ipv4" but
     * ignore family.
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
     * incarnation, the UDP transport will only support unrreliable IPv4; so we
     * log errors and ignore any requests for other mechanisms.
     */
    iter = argMap.find("r4addr");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("UDPTransport::NormalizeListenSpec(): The mechanism implied by \"r4addr\" is not supported"));
        argMap.erase(iter);
    }

    iter = argMap.find("r4port");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("UDPTransport::NormalizeListenSpec(): The mechanism implied by \"r4port\" is not supported"));
        argMap.erase(iter);
    }

    iter = argMap.find("r6addr");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("UDPTransport::NormalizeListenSpec(): The mechanism implied by \"r6addr\" is not supported"));
        argMap.erase(iter);
    }

    iter = argMap.find("r6port");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("UDPTransport::NormalizeListenSpec(): The mechanism implied by \"r6port\" is not supported"));
        argMap.erase(iter);
    }

    iter = argMap.find("u6addr");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("UDPTransport::NormalizeListenSpec(): The mechanism implied by \"u6addr\" is not supported"));
        argMap.erase(iter);
    }

    iter = argMap.find("u6port");
    if (iter != argMap.end()) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("UDPTransport::NormalizeListenSpec(): The mechanism implied by \"u6port\" is not supported"));
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
     * The UDP transport must absolutely support the IPv4 "unreliable" mechanism
     * (UDP).  We therefore must provide an addr either from explicit keys or
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
             * "u4addr" which would be synonymous?  If so, save it as an addr,
             * erase it and point back to the new addr.
             */
            iter = argMap.find("u4addr");
            if (iter != argMap.end()) {
                argMap["addr"] = iter->second;
                argMap.erase(iter);
            }

            iter = argMap.find("addr");
        }

        /*
         * Now, deal with the addr, possibly derived from u4addr.
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
                                 ("UDPTransport::NormalizeListenSpec(): The addr \"%s\" is not a legal IPv4 address.",
                                  iter->second.c_str()));
                    return ER_BUS_BAD_TRANSPORT_ARGS;
                }
                iter->second = addr.ToString();
                outSpec.append("addr=" + addr.ToString());
            } else {
                QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                             ("UDPTransport::NormalizeListenSpec(): The addr \"%s\" is not a legal IPv4 address.",
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
     * The UDP transport must absolutely support the IPv4 "unreliable" mechanism
     * (UDP).  We therefore must provide a port either from explicit keys or
     * generated from the defaults.
     */
    iter = argMap.find("port");
    if (iter == argMap.end()) {
        /*
         * We have no value associated with a "port" key.  Do we have a
         * "u4port" which would be synonymous?  If so, save it as a port,
         * erase it and point back to the new port.
         */
        iter = argMap.find("u4port");
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
                         ("UDPTransport::NormalizeListenSpec(): The key \"port\" has a bad value \"%s\"", iter->second.c_str()));
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

QStatus UDPTransport::NormalizeTransportSpec(const char* inSpec, qcc::String& outSpec, map<qcc::String, qcc::String>& argMap) const
{
    QCC_DbgTrace(("UDPTransport::NormalizeTransportSpec()"));

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
                     ("UDPTransport::NormalizeTransportSpec(): The addr may not be the default address"));
        return ER_BUS_BAD_TRANSPORT_ARGS;
    }

    return ER_OK;
}

/**
 * This is the method that is called in order to initiate an outbound (active)
 * connection.  This is called from the AllJoyn Object in the course of
 * processing a JoinSession request in the context of a JoinSessionThread.
 */
QStatus UDPTransport::Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newEp)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::Connect(connectSpec=%s, opts=%p, newEp-%p)", connectSpec, &opts, &newEp));

    /*
     * We are given a session options structure that defines the kind of
     * connection that is being sought.  Can we support the connection being
     * requested?  If not, don't even try.
     */
    if (SupportsOptions(opts) == false) {
        QStatus status = ER_BUS_BAD_SESSION_OPTS;
        QCC_LogError(status, ("UDPTransport::Connect(): Supported options mismatch"));
        DecrementAndFetch(&m_refCount);
        return status;
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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("UDPTransport::Connect(): Not running or stopping; exiting"));
        DecrementAndFetch(&m_refCount);
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

    /*
     * If we pass the IsRunning() gate above, we must have a server accept
     * thread spinning up or shutting down but not yet joined.  Since the name
     * service is started before the server accept thread is spun up, and
     * deleted after it is joined, we must have a started name service or someone
     * isn't playing by the rules; so an assert is appropriate here.
     */
    assert(IpNameService::Instance().Started() && "UDPTransport::Connect(): IpNameService not started");

    /*
     * UDP Transport does not support raw sockets of any flavor.
     */
    QStatus status;
    if (opts.traffic & SessionOpts::TRAFFIC_RAW_RELIABLE || opts.traffic & SessionOpts::TRAFFIC_RAW_UNRELIABLE) {
        status = ER_UDP_UNSUPPORTED;
        QCC_LogError(status, ("UDPTransport::Connect(): UDP Transport does not support raw traffic"));
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * Parse and normalize the connectArgs.  When connecting to the outside
     * world, there are no reasonable defaults and so the addr and port keys
     * MUST be present.
     */
    qcc::String normSpec;
    map<qcc::String, qcc::String> argMap;
    status = NormalizeTransportSpec(connectSpec, normSpec, argMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("UDPTransport::Connect(): Invalid UDP connect spec \"%s\"", connectSpec));
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * These fields (addr, port) are all guaranteed to be present now and an
     * underlying network (even if it is Wi-Fi P2P) is assumed to be up and
     * functioning.
     */
    assert(argMap.find("addr") != argMap.end() && "UDPTransport::Connect(): addr not present in argMap");
    assert(argMap.find("port") != argMap.end() && "UDPTransport::Connect(): port not present in argMap");

    IPAddress ipAddr(argMap.find("addr")->second);
    uint16_t ipPort = StringToU32(argMap["port"]);

#ifndef NDEBUG
    if (ipAddr.IsLoopback()) {
        QCC_DbgPrintf(("UDPTransport::Connect(): \"%s\" is a loopback address", ipAddr.ToString().c_str()));
    }
#endif
    /*
     * The semantics of the Connect method tell us that we want to connect to a
     * remote daemon.  UDP will happily allow us to connect to ourselves, but
     * this is not always possible in the various transports AllJoyn may use.
     * To avoid unnecessary differences, we do not allow a requested connection
     * to "ourself" to succeed.
     *
     * The code here is not a failsafe way to prevent this since thre are going
     * to be multiple processes involved that have no knowledge of what the
     * other is doing (for example, the wireless supplicant and this daemon).
     * This means we can't synchronize and there will be race conditions that
     * can cause the tests for selfness to fail.  The final check is made in the
     * BusHello protocol, which will abort the connection if it detects it is
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
    snprintf(anyspec, sizeof(anyspec), "%s:addr=0.0.0.0,port=%u", GetTransportName(), ipPort);

    qcc::String normAnySpec;
    map<qcc::String, qcc::String> normArgMap;
    status = NormalizeListenSpec(anyspec, normAnySpec, normArgMap);
    if (ER_OK != status) {
        QCC_LogError(status, ("UDPTransport::Connect(): Invalid INADDR_ANY connect spec"));
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * Look to see if we are already listening on the provided connectSpec
     * explicitly.  If we see that we are listening on the INADDR_ANY address
     * make a note of it.  We sort that out in the next bit of code.
     */
    QCC_DbgPrintf(("UDPTransport::Connect(): Checking for connection to self (\"%s\"", normSpec.c_str()));
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    bool anyEncountered = false;
    for (list<pair<qcc::String, SocketFd> >::iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
        QCC_DbgPrintf(("UDPTransport::Connect(): Checking listenSpec %s", i->first.c_str()));

        /*
         * If the provided connectSpec is already explicitly listened to, it is
         * an error.
         */
        if (i->first == normSpec) {
            m_listenFdsLock.Unlock(MUTEX_CONTEXT);
            QCC_DbgPrintf(("UDPTransport::Connect(): Explicit connection to self"));
            DecrementAndFetch(&m_refCount);
            return ER_BUS_ALREADY_LISTENING;
        }

        /*
         * If we are listening to INADDR_ANY and the supplied port, then we have
         * to take a closer look to decide if this call is bogus or not.  Set a
         * flag to remind us.
         */
        if (i->first == normAnySpec) {
            QCC_DbgPrintf(("UDPTransport::Connect(): Possible implicit connection to self detected"));
            anyEncountered = true;
        }
    }
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

    std::vector<qcc::IfConfigEntry> entries;
    status = qcc::IfConfig(entries);
    if (ER_OK != status) {
        QCC_LogError(status, ("UDPTransport::Connect(): Unable to read network interface configuration"));
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * If we are listening to INADDR_ANY, we are going to have to see if any
     * currently UP interfaces have an IP address that matches the connectSpec
     * addr.
     */
    if (anyEncountered) {
        /*
         * If anyEncountered is set to true, it means we discovered an INADDR_ANY listen spec with
         * a specified port that matches the one in the connect spec.  We have to make sure
         * that we are not connecting to this INADDR_ANY listener through a loopback address.
         */
        QCC_DbgPrintf(("UDPTransport::Connect(): Checking for implicit connection to self through loopback"));

        if (ipAddr.IsLoopback()) {
            QCC_DbgPrintf(("UDPTransport::Connect(): Attempted connection to self through loopback; exiting"));
            DecrementAndFetch(&m_refCount);
            return ER_BUS_ALREADY_LISTENING;
        }

        /*
         * Loop through the network interface entries looking for an UP
         * interface that has the same IP address as the one we're trying to
         * connect to.  We know any match on the address will be a hit since we
         * matched the port during the listener check above.  Since we have a
         * listener listening on *any* UP interface on the specified port, a
         * match on the interface address with the connect address is a hit.
         */
        QCC_DbgPrintf(("UDPTransport::Connect(): Checking for implicit connection to self through INADDR_ANY"));

        for (uint32_t i = 0; i < entries.size(); ++i) {
            QCC_DbgPrintf(("UDPTransport::Connect(): Checking interface %s", entries[i].m_name.c_str()));
            if (entries[i].m_flags & qcc::IfConfigEntry::UP) {
                QCC_DbgPrintf(("UDPTransport::Connect(): Interface UP with addresss %s", entries[i].m_addr.c_str()));
                IPAddress foundAddr(entries[i].m_addr);
                if (foundAddr == ipAddr) {
                    QCC_DbgPrintf(("UDPTransport::Connect(): Attempted connection to self; exiting"));
                    DecrementAndFetch(&m_refCount);
                    return ER_BUS_ALREADY_LISTENING;
                }
            }
        }
    }

    /*
     * Now, we have to figure out which socket to use for the connect.  In the
     * TCP Transport we would typcially create a new socket here; but we want to
     * re-use an existing socket to save resources.
     *
     * In the Routing Node configuration, we expect to see some number of listen
     * specs.  TCP interprets these as having IP addresses or interfaces and
     * ports on which we want to listen for incoming connections.  In the UDP
     * Transport, we create a socket for each of these interfaces and listen for
     * incoming UDP datagrams on them.  Currently, the only configuration used
     * is one listen spec that specifies INADDR_ANY (accept datagrams from all
     * interfaces).  There is the possibility that multiple listen specs could
     * be used to bind only to specific interfaces, though.  While we aren't
     * listening for inbound connections as TCP would, the effect is the same.
     * The action we take should therefore be conceptually similar to what TCP
     * would do.
     *
     * There are therefore two cases to consider: First, if there is a socket
     * bound to INADDR_ANY (which means that we got a listen on 0.0.0.0); and
     * second, there are possibly multiple sockets bound to specific addresses.
     * The upshot is that if we have a listening socket bound to INADDR_ANY we
     * just use it.  If we don't listen to INADDR_ANY we look for a socket bound
     * to an IP address that puts it on the same network as the destination
     * specified for the connect.  We don't actually have to do that since UDP
     * will route to the correct outgoing interface, but we keep the coherency
     * to avoid nasty surprises.
     *
     * Note: There is a check for INADDR_ANY above, but that is a check to make
     * sure we don't connect to ourselves inadvertently -- it includes the port
     * number so it is not a generic test for interface.  Below, we are checking
     * to see if any of our sockets is bound to INADDR_ANY irrespective of port.
     */
    qcc::SocketFd sock = 0;
    bool foundSock = false;

    QCC_DbgPrintf(("UDPTransport::Connect(): Look for socket corresponding to destination network"));
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    for (list<pair<qcc::String, SocketFd> >::iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
        /*
         * Get the local address of the socket in question.
         */
        IPAddress listenAddr;
        uint16_t listenPort;
        qcc::GetLocalAddress(i->second, listenAddr, listenPort);
        QCC_DbgPrintf(("UDPTransport::Connect(): Check out local address \"%s\"", listenAddr.ToString().c_str()));

        /*
         * If we encounter a socket bound to INADDR_ANY, we use it per the
         * simple "rules" above.
         */
        if (listenAddr.ToString() == "0.0.0.0") {
            sock = i->second;
            foundSock = true;
            QCC_DbgPrintf(("UDPTransport::Connect(): Found socket (%d.) listening on INADDR_ANY", sock));
            break;
        }

        /*
         * If this isn't a socket bound to INADDR_ANY it must be bound to a
         * specific interface.  Find the corresponding interface information in
         * the IfConfig entries.  We need the network mask from that entry so we
         * can see if the network numbers match.
         *
         * TODO: what if we have multiple interfaces with the same network
         * number i.e. 192.168.1.x?  The advertisement will have come in over
         * one of them but we lose track of the source of the advertisement that
         * precipitated the JoinSession that got us here.  We need to remember
         * that info (perhaps as a "zone index" equivalent) in the connect spec,
         * but that has to be plumbed in from the name service and allowed all
         * the way up into the AllJoyn obj and back down!
         */
        uint32_t prefixLen = 0;
        for (uint32_t j = 0; j < entries.size(); ++j) {
            if (entries[j].m_addr == listenAddr.ToString()) {
                prefixLen = entries[j].m_prefixlen;
            }
        }

        QCC_DbgPrintf(("UDPTransport::Connect(): prefixlen=%d.", prefixLen));

        /*
         * Create a netmask with a one in the leading bits for each position
         * implied by the prefix length.
         */
        uint32_t mask = 0;
        for (uint32_t j = 0; j < prefixLen; ++j) {
            mask >>= 1;
            mask |= 0x80000000;
        }

        QCC_DbgPrintf(("UDPTransport::Connect(): net mask is 0x%x", mask));

        /*
         * Is local address of the currently indexed listenFd on the same
         * network as the destination address supplied as a parameter to the
         * connect?  If so, we use this listenFD as the socket to use when we
         * try to connect to the remote daemon.
         */
        uint32_t network1 = listenAddr.GetIPv4AddressCPUOrder() & mask;
        uint32_t network2 = ipAddr.GetIPv4AddressCPUOrder() & mask;
        if (network1 == network2) {
            QCC_DbgPrintf(("UDPTransport::Connect(): network \"%s\" matches network \"%s\"",
                           IPAddress(network1).ToString().c_str(), IPAddress(network2).ToString().c_str()));
            /*
             * Mark the socket as found, but don't break here in case there is a
             * socket bound to INADDR_ANY that happens to be later in the list.
             * We prefer to use that one since we assume a user configured it in
             * order to be more generic.  We do the break there so that generic
             * choice other specific choices.
             */
            sock = i->second;
            foundSock = true;
        } else {
            QCC_DbgPrintf(("UDPTransport::Connect(): network \"%s\" does not match network \"%s\"",
                           IPAddress(network1).ToString().c_str(), IPAddress(network2).ToString().c_str()));
        }
    }

    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

    if (foundSock == false) {
        status = ER_UDP_NO_NETWORK;
        QCC_LogError(status, ("UDPTransport::Connect(): Not listening on network implied by \"%s\"", ipAddr.ToString().c_str()));
        DecrementAndFetch(&m_refCount);
        return status;
    }
#ifndef NDEBUG
    else {
        QCC_DbgPrintf(("UDPTransport::Connect(): Socket %d. talks to network implied by \"%s\"", sock, normSpec.c_str()));
    }
#endif

    /*
     * We've done the basic homework to see if this is a reasonable request.
     * Now we need to make sure that we have the resources to allow the connect
     * to succeed.  Specifically, we need to check to see if the connection
     * limits will be violated if we proceed.
     */
    m_connLock.Lock(MUTEX_CONTEXT);

    if (m_currAuth + 1U > m_maxAuth || m_currConn + 1U > m_maxConn) {
        status = ER_CONNECTION_LIMIT_EXCEEDED;
        QCC_LogError(status, ("UDPTransport::Connect(): No slot for new connection"));
        m_connLock.Unlock(MUTEX_CONTEXT);
        DecrementAndFetch(&m_refCount);
        return status;
    }

    ++m_currAuth;
    ++m_currConn;
    m_connLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::Connect(): Compose BusHello"));
    Message hello(m_bus);
    status = hello->HelloMessage(true, m_bus.GetInternal().AllowRemoteMessages(), opts.nameTransfer);
    if (status != ER_OK) {
        status = ER_UDP_BUSHELLO;
        QCC_LogError(status, ("UDPTransport::Connect(): Can't make a BusHello Message"));

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * The Function HelloMessage creates and marshals the BusHello Message for
     * the remote side.  Once it is marshaled, there is a buffer associated with
     * the message that contains the on-the-wire version of the messsage.
     */
    size_t buflen = hello->GetBufferSize();
#ifndef NDEBUG
    uint8_t* buf = new uint8_t[buflen + SEAL_SIZE];
    SealBuffer(buf + buflen);
#else
    uint8_t* buf = new uint8_t[buflen];
#endif
    memcpy(buf, const_cast<uint8_t*>(hello->GetBuffer()), buflen);

    /*
     * We are about to get into a state where we are off trying to start up an
     * endpoint, but we are executing in the context of an arbitrary thread that
     * has called into UDPTransport::Connect().  We want to block this thread,
     * but we will be needing to wake it up when the connection process
     * completes and also up in case the UDP transport is shut down during the
     * connection process.  We need the ArdpConnRecord* in order to associate
     * the current thread and the event we'll be using with the connection
     * completion callback that will happen.
     *
     * As soon as we call ARDP_Connect() we are enabling the callback to happen,
     * but we don't have the ArdpConnRecord* we need until after ARDP_Connect()
     * returns.  In order to keep the connect callback from happening, we take
     * the ARDP lock which prevents the callback from being run and we don't
     * give it back until we have the ArdpConnRecord* stashed away in a place
     * where it can be found by the callback.
     *
     * N.B. The event in question *must* remain in scope and valid during the
     * entire time the ConnectEntry we're about to make is on the set we're
     * about to put it on.
     */
    qcc::Event event;
    ArdpConnRecord* conn;

    /*
     * We need to take the endpoint list lock which is going to protect the
     * std::set of entries identifying the connecting thread; and we need to
     * take the ARDP lock to hold off the callback.  When holding two locks,
     * always consider lock order.  Unfortunately, it is a common operation
     * for an external thread to take the endpoint lock, wander around in the
     * endpoints and figure out what to do then take the ARDP lock in order
     * to do it.  It's also a common operation for our main thread to take the
     * ARDP lock and call into ARDP which calls out in a callback
     * and   We'll keep that order.
     */
    m_endpointListLock.Lock(MUTEX_CONTEXT);
    m_ardpLock.Lock();
    QCC_DbgPrintf(("UDPTransport::Connect(): ARDP_Connect()"));
    status = ARDP_Connect(m_handle, sock, ipAddr, ipPort, m_ardpConfig.segmax, m_ardpConfig.segbmax, &conn, buf, buflen, &event);

    /*
     * The ARDP code takes the hello buffer and copies it into its internal
     * retry buffer, so we are responsible for disposition of the buffer no
     * matter if the connect succeeds or fails.
     */
    delete[] buf;
    buf = NULL;

    /*
     * If the ARDP_Connect fails, we haven't started the connection process, so
     * we haven't passed responsibility for the connection counts off to the
     * callback (see note below).
     */
    if (status != ER_OK) {
        assert(conn == NULL && "UDPTransport::Connect(): ARDP_Connect() failed but returned ArdpConnRecord");
        QCC_LogError(status, ("UDPTransport::Connect(): ARDP_Connect() failed"));
        m_ardpLock.Unlock();
        m_endpointListLock.Unlock(MUTEX_CONTEXT);

        m_connLock.Lock(MUTEX_CONTEXT);
        --m_currAuth;
        --m_currConn;
        m_connLock.Unlock(MUTEX_CONTEXT);

        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * Important note: Once we make a successful call to ARDP_Connect, we are
     * transferring responsibility for managing the connection limit counters to
     * the callback.  If ARDP accepts the connection request it guarantees that
     * it will call back with status, and so we rely on that guarantee to manage
     * the counters in one place -- the callback.  That means that we are no
     * longer allowed to touch the counters.  Especially do not decrement them
     * if a future error is detected.  The slots are occupied until the callback
     * responds either positively or negatively.
     */
    Thread* thread = GetThread();
    QCC_DbgPrintf(("UDPTransport::Connect(): Add thread=%p to m_connectThreads", thread));
    assert(thread && "UDPTransport::Connect(): GetThread() returns NULL");
    uint32_t cid = ARDP_GetConnId(m_handle, conn);
    ConnectEntry entry(thread, conn, cid, &event);

    /*
     * It looks like we just did a connect and so if it succeeded, we must have
     * a valid connection and connection ID.  Time has passed between the time
     * we did the ARDP_Connect() and now, though.  We must consider what might
     * happen if it turns out that ARDP has decided to invalidate the connection
     * now that it is off trying to connect.  It guarantees that it will call
     * back with that connection when it is done trying to connect.  What if
     * that already happened and we haven't had a chance to get our thread onto
     * the list of waiting threads.  Admittedly, the chances of that happening
     * are extrememly low, but we can't get the conn until we do the connect and
     * the connection (actually ID) is all we have to tie the threads to the
     * connection process.  Since this is such a wildly unlikely event, we don't
     * do anything to catch it, just describe why it will not hurt.
     *
     * If we are delayed inserting our thread on the list of waiting threads
     * until after the callback happens, the callback will have not been able to
     * find our thread and will have exited without doing anything, or at worst
     * it may have created an endpoint and then stopped it.  No harm no foul.
     * This endpoint will be cleaned up by the endpoint manager.  If the
     * connection is invalid by the time we create the ConnectEntry above, we
     * will create an entry with the connection ID set to ARDP_CONN_ID_INVALID.
     * In this case, we will insert an entry on the list which can never be
     * found.  We will time out below.  We'll remove the entry by finding what
     * we put in, which includes the inavlid conn ID.  No harm no foul.  There
     * is an extremely unlikely chance that we may do some work that could
     * strictly have been avoided if we went to some lengths here.
     */
    m_connectThreads.insert(entry);

    /*
     * If we do something that is going to bug the ARDP protocol (in this case
     * start connect timers), we need to call back into ARDP ASAP to get it
     * moving.  This is done in the main thread, which we need to wake up.
     * Since this is a connect it will eventually require endpoint management,
     * so we make a note to run the endpoint management code.
     */
    m_manage = STATE_MANAGE;
    Alert();

    /*
     * All done with the tricky part, so release the locks in inverse order
     */
    m_ardpLock.Unlock();
    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    /*
     * Set up a watchdog timeout on the connect.  If the other side plays by the
     * rules, we should get a callback.  If there are authentication games played
     * during the connect, we need to detect that and time out ourselves, so the
     * endpoint can be scavenged.  We add our own timeout that expires some time after we
     * expect ARDP to time out.  On a connect that would be at
     *
     *    connectTimeout * (1 + connectRetries)
     *
     * To give ARDP a chance, we timeout one retry interval later, at
     *
     *    connectTimeout * (2 + connectRetries)
     *
     */
    uint32_t timeout = m_ardpConfig.connectTimeout * (2 + m_ardpConfig.connectRetries);

    QCC_DbgPrintf(("UDPTransport::Connect(): qcc::Event::Wait(): timeout=%d.", timeout));

    /*
     * We fired off the connect request.  If the connect succeeds, when we wake
     * up we will find a UDPEndpoint on the m_endpointList with an ARDP
     * connection pointer matching the connection we got above.  If this doesn't
     * happen, the process must've failed.
     */
    status = qcc::Event::Wait(event, timeout);

    /*
     * Whether we succeeded or failed, we are done with blocking I/O on the
     * current thread, so we need to remove the connectEntry from the set we
     * kept it in.  We still have the thread reference count set indicating we
     * are in the endpoint, but we are awake and headed out now.
     */
    m_endpointListLock.Lock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::Connect(): Removing thread=%p from m_connectThreads", thread));
    set<ConnectEntry>::iterator j = m_connectThreads.find(entry);
    assert(j != m_connectThreads.end() && "UDPTransport::Connect(): Thread not on m_connectThreads");
    m_connectThreads.erase(j);

    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::Connect(): Event::Wait() failed"));
        m_endpointListLock.Unlock(MUTEX_CONTEXT);
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * The way we figure out if the connect succeded is by looking for an
     * endpoint with a connection ID that is the same as the one returned to us
     * by the original call to ARDP_Connect().
     *
     * The conn pointer and its associated ID will be plumbed from the call to
     * connect through to the creation of the endpoint in the connect callback;
     * so if an endpoint was created as a result of the call to ARDP_Connect()
     * that returned us a valid conn so long ago.  That doesn't mean that the
     * connection must still be up and running now that we get around to looking
     * for it.  It could actually be the case that ARDP has acutally
     * disconnected the connection and notified us through a disconnect callback
     * that has been queued up for processing.  In that (again, unlikely but
     * real) case, the conn we have could now be invalid even though we haven't
     * heard about it yet since that fact is queued behind us in the dispatcher.
     *
     * All we want to do is to find an endpoint with the connection ID we got at
     * the start.  If everything worked according to the textbooks, we got a
     * valid connection when we did the ARDP_Connect(), got a valid connection
     * ID from that connection, put a valid connection ID into the thread set
     * entry, a valid connection ID was put into the endpoint when it was
     * created in a connect callback, the connect callback found our thread
     * entry with the correct ID and woke us up.  Now we look to make sure that
     * an endpoint was created by looking for one with the correct ID and all of
     * those IDs will match now.
     *
     * How can they not match?  The process works from the ARDP_Connect()
     * through the ConnectCb().  The conn and ID are guaranteed by ARDP to match
     * during that time.  Because we have to call out to the daemon, we need to
     * dispatch DoConnectCb() and any work there can refer to a defunct
     * connection.  We are careful to pass the valid connection ID to
     * DoConnectCb() so it can refer back to use even though a connection may
     * have gone down.  The upshot is that if we don't find an endpoint, it is
     * because our connection ID is ARDP_CONN_ID_INVALID and we missed the boat,
     * or the endpoint was never made.
     *
     * The case to worry about is if the endoint was made successfully and we
     * have connection ID ARDP_CONN_ID_INVALID.  This could allow an unuseable
     * endpoint to hang around until the transport is shut down.  For this to
     * happen, the connect would have to have been successful and an endpoint be
     * created and almost immediately receive a disconnect.  ARDP would have to
     * notify via DisconnectCb() and this would have to happen before the call
     * to ARDP_GetConnId() that happens immediately after ARDP_Connect().  The
     * DisconnectCb() plumbs the original connection ID through to the callback
     * dispatcher, though, and so the DisconnnectCb() will start the process of
     * invalidating and releasing the endpoint even though we never saw any of
     * that happen here.  We are okay even though we might have a connection ID
     * of ID ARDP_CONN_ID_INVALID -- the connect will fail because we can't find
     * an endpoint, but it already failed as evidenced by the disconnect event
     * we were worried about.
     */
    QCC_DbgPrintf(("UDPTransport::Connect(): Finding endpoint with conn ID = %d. in m_endpointList", cid));
    set<UDPEndpoint>::iterator i;
    for (i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        UDPEndpoint ep = *i;
        if (ep->GetConnId() == cid) {
            QCC_DbgPrintf(("UDPTransport::Connect(): Success."));
            /*
             * We know that we found an endpoint on the endpoint list so it has
             * a valid reference count, and we are doing this with the endpoint
             * list lock taken so nothing will be deleted out from under us.
             * This assignment to newEp will result in a new reference to a
             * valid object.
             */
            newEp = BusEndpoint::cast(ep);
            break;
        }
    }

    m_endpointListLock.Unlock(MUTEX_CONTEXT);
    DecrementAndFetch(&m_refCount);
    return status;
}

/**
 * This is a (surprisingly) unused method call.  One would expect that since it
 * is defined, it would be the symmetrical opposigte of Connect.  That turns out
 * not to be the case.  Some transports define implementations as if it was
 * used, but it is not.  Our implementation is to simply assert.
 */
QStatus UDPTransport::Disconnect(const char* connectSpec)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::Disconnect(): %s", connectSpec));

    /*
     * Disconnect is actually not used in the transports architecture.  It is
     * misleading and confusing to have it implemented.
     */
    assert(0 && "UDPTransport::Disconnect(): Unexpected call");
    QCC_LogError(ER_FAIL, ("UDPTransport::Disconnect(): Unexpected call"));
    DecrementAndFetch(&m_refCount);
    return ER_FAIL;
}

/**
 * Start listening for inbound connections over the ARDP Protocol using the
 * address and port information provided in the listenSpec.
 */
QStatus UDPTransport::StartListen(const char* listenSpec)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::StartListen(\"%s\")", listenSpec));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("UDPTransport::StartListen(): Not running or stopping; exiting"));
        DecrementAndFetch(&m_refCount);
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
        QCC_LogError(status, ("UDPTransport::StartListen(): Invalid UDP listen spec \"%s\"", listenSpec));
        DecrementAndFetch(&m_refCount);
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

    /*
     * The daemon code is in a state where it lags in functionality a bit with
     * respect to the common code.  Common supports the use of IPv6 addresses
     * but the name service is not quite ready for prime time.  Until the name
     * service can properly distinguish between various cases, we fail any
     * request to listen on an IPv6 address.
     */
    if (key == "addr") {
        IPAddress ipAddress;
        status = ipAddress.SetAddress(argMap["addr"].c_str());
        if (ipAddress.IsIPv6()) {
            status = ER_INVALID_ADDRESS;
            QCC_LogError(status, ("UDPTransport::StartListen(): IPv6 address (\"%s\") in \"u4addr\" not allowed", argMap["u4addr"].c_str()));
            DecrementAndFetch(&m_refCount);
            return status;
        }
    }
    QCC_DbgPrintf(("UDPTransport::StartListen(): %s = \"%s\", port = \"%s\"",
                   key.c_str(), argMap[key].c_str(), argMap["port"].c_str()));

    QCC_DbgTrace(("UDPTransport::StartListen(): Final normSpec=\"%s\"", normSpec.c_str()));

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
            DecrementAndFetch(&m_refCount);
            return ER_BUS_ALREADY_LISTENING;
        }
    }
    m_listenSpecsLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgTrace(("UDPTransport::StartListen(): QueueStartListen(\"%s\")", normSpec.c_str()));
    QueueStartListen(normSpec);
    DecrementAndFetch(&m_refCount);
    return ER_OK;
}

void UDPTransport::QueueStartListen(qcc::String& normSpec)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::QueueStartListen(\"%s\")", normSpec.c_str()));

    /*
     * In order to start a listen, we send the maintenance thread a message
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
    DecrementAndFetch(&m_refCount);
}

QStatus UDPTransport::DoStartListen(qcc::String& normSpec)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgPrintf(("UDPTransport::DoStartListen(\"%s\")", normSpec.c_str()));

    /*
     * Since the name service is created before the server accept thread is spun
     * up, and stopped when it is stopped, we must have a started name service or
     * someone isn't playing by the rules; so an assert is appropriate here.
     */
    assert(IpNameService::Instance().Started() && "UDPTransport::DoStartListen(): IpNameService not started");

    qcc::String interfaces = ConfigDB::GetConfigDB()->GetProperty("ns_interfaces");
    if (interfaces.size()) {
        QCC_LogError(ER_WARNING, ("UDPTransport::DoStartListen(): The mechanism implied by \"ns_interfaces\" is no longer supported."));
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
    assert(status == ER_OK && "UDPTransport::DoStartListen(): Invalid UDP listen spec");

    qcc::String key = "";
    if (argMap.find("iface") != argMap.end()) {
        key = "iface";
    } else if (argMap.find("addr") != argMap.end()) {
        key = "addr";
    }
    QCC_DbgPrintf(("UDPTransport::DoStartListen(): %s = \"%s\", port = \"%s\"",
                   key.c_str(), argMap[key].c_str(), argMap["port"].c_str()));

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
     *     iface                  Action
     *     ----------             -----------------------------------------
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
     *     address                Action
     *     --------               -----------------------------------------
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
     * This is much harder to describe than to implement; but the upshot is that
     * we listen on the primary IPv4 address of the named network interface that
     * comes in with the listenSpec and we enable the name service on that same
     * interface. It is up to the person doing the configuration to understand
     * what he or she is trying to do and the impact of choosing those values.
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

    /* We first determine whether a network interface name or an IP address was
     * specified and then we invoke the appropriate name service method.
     */
    if (!interface.empty()) {
        m_requestedInterfaces[interface] = qcc::IPEndpoint("0.0.0.0", listenPort);
        m_listenPortMap[interface] = listenPort;
    } else if (addr.Size() && addr.IsIPv4()) {
        m_requestedAddresses[addr.ToString()] = "";
        m_requestedAddressPortMap[addr.ToString()] = listenPort;
    }
    if (!interface.empty()) {
        status = IpNameService::Instance().OpenInterface(TRANSPORT_UDP, interface);
    } else if (addr.Size() && addr.IsIPv4()) {
        status = IpNameService::Instance().OpenInterface(TRANSPORT_UDP, addr.ToString());
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::DoStartListen(): OpenInterface() failed for %s", (interface.empty() ? addr.ToString().c_str() : interface.c_str())));
    }

    DecrementAndFetch(&m_refCount);
    return status;
}

/**
 * Since untrusted clients are only Thin Library clients, and the Thin Library
 * only supports TCP, this is a NOP here.
 */
void UDPTransport::UntrustedClientExit()
{
    QCC_DbgTrace((" UDPTransport::UntrustedClientExit()"));
}

/**
 * Since untrusted clients are only Thin Library clients, and the Thin Library
 * only supports TCP, this is a NOP here.
 */
QStatus UDPTransport::UntrustedClientStart()
{
    QCC_DbgTrace((" UDPTransport::UntrustedClientStart()"));
    return ER_UDP_NOT_IMPLEMENTED;
}

/**
 * Stop listening for inbound connections over the ARDP Protocol using the
 * address and port information provided in the listenSpec.  Must match a
 * previously started listenSpec.
 */
QStatus UDPTransport::StopListen(const char* listenSpec)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::StopListen()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("UDPTransport::StopListen(): Not running or stopping; exiting"));
        DecrementAndFetch(&m_refCount);
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
        QCC_LogError(status, ("UDPTransport::StopListen(): Invalid UDP listen spec \"%s\"", listenSpec));
        DecrementAndFetch(&m_refCount);
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

    DecrementAndFetch(&m_refCount);
    return ER_OK;
}

void UDPTransport::QueueStopListen(qcc::String& normSpec)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::QueueStopListen()"));

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
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::DoStopListen(qcc::String& normSpec)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::DoStopListen()"));

    /*
     * Since the name service is started before the server accept thread is spun
     * up, and stopped after it is stopped, we must have a started name service or
     * someone isn't playing by the rules; so an assert is appropriate here.
     */
    assert(IpNameService::Instance().Started() && "UDPTransport::DoStopListen(): IpNameService not started");

    /*
     * Find the (single) listen spec and remove it from the list of active FDs
     * used by the maintenance thread.
     */
    QCC_DbgPrintf(("UDPTransport::DoStopListen(): Looking for listen FD with normspec \"%s\"", normSpec.c_str()));
    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    qcc::SocketFd stopFd = qcc::INVALID_SOCKET_FD;
    bool found = false;
    for (list<pair<qcc::String, SocketFd> >::iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
        if (i->first == normSpec) {
            QCC_DbgPrintf(("UDPTransport::DoStopListen(): Found normspec \"%s\"", normSpec.c_str()));
            stopFd = i->second;
            m_listenFds.erase(i);
            found = true;
            break;
        }
    }

    if (found) {
        if (m_reload != STATE_EXITED) {
            QCC_DbgPrintf(("UDPTransport::DoStopListen(): m_reload != STATE_EXITED"));

            /*
             * If the UDPTransport::Run thread is still running, set m_reload to
             * STATE_RELOADING, unlock the mutex, alert the main Run thread that
             * there is a change and wait for the Run thread to finish any
             * connections it may be accepting and then reload the set of
             * events.
             */
            m_reload = STATE_RELOADING;

            QCC_DbgPrintf(("UDPTransport::DoStopListen(): Alert()"));
            Alert();

            /*
             * Wait until UDPTransport::Run thread has reloaded the set of
             * events or exited.
             */
            QCC_DbgPrintf(("UDPTransport::DoStopListen(): Wait for STATE_RELOADING()"));
            while (m_reload == STATE_RELOADING) {
                m_listenFdsLock.Unlock(MUTEX_CONTEXT);
                qcc::Sleep(10);
                m_listenFdsLock.Lock(MUTEX_CONTEXT);
            }
            QCC_DbgPrintf(("UDPTransport::DoStopListen(): Done waiting for STATE_RELOADING()"));
        }

        /*
         * If we took a socketFD off of the list of active FDs, we need to tear it
         * down.
         */
        QCC_DbgPrintf(("UDPTransport::DoStopListen(): Close socket %d.", stopFd));
        qcc::Close(stopFd);
    }

    m_listenFdsLock.Unlock(MUTEX_CONTEXT);
    DecrementAndFetch(&m_refCount);
}

bool UDPTransport::NewDiscoveryOp(DiscoveryOp op, qcc::String namePrefix, bool& isFirst)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::NewDiscoveryOp()"));

    bool first = false;

    if (op == ENABLE_DISCOVERY) {
        QCC_DbgPrintf(("UDPTransport::NewDiscoveryOp(): Registering discovery of namePrefix \"%s\"", namePrefix.c_str()));
        first = m_discovering.empty();
        if (find(m_discovering.begin(), m_discovering.end(), namePrefix) == m_discovering.end()) {
            m_discovering.push_back(namePrefix);
        }
    } else {
        list<qcc::String>::iterator i = find(m_discovering.begin(), m_discovering.end(), namePrefix);
        if (i == m_discovering.end()) {
            QCC_DbgPrintf(("UDPTransport::NewDiscoveryOp(): Cancel of non-existent namePrefix \"%s\"", namePrefix.c_str()));
        } else {
            QCC_DbgPrintf(("UDPTransport::NewDiscoveryOp(): Unregistering discovery of namePrefix \"%s\"", namePrefix.c_str()));
            m_discovering.erase(i);
        }
    }

    isFirst = first;
    bool rc = m_discovering.empty();
    DecrementAndFetch(&m_refCount);
    return rc;
}

bool UDPTransport::NewAdvertiseOp(AdvertiseOp op, qcc::String name, bool& isFirst)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::NewAdvertiseOp()"));

    bool first = false;

    if (op == ENABLE_ADVERTISEMENT) {
        QCC_DbgPrintf(("UDPTransport::NewAdvertiseOp(): Registering advertisement of namePrefix \"%s\"", name.c_str()));
        first = m_advertising.empty();
        if (find(m_advertising.begin(), m_advertising.end(), name) == m_advertising.end()) {
            m_advertising.push_back(name);
        }
    } else {
        list<qcc::String>::iterator i = find(m_advertising.begin(), m_advertising.end(), name);
        if (i == m_advertising.end()) {
            QCC_DbgPrintf(("UDPTransport::NewAdvertiseOp(): Cancel of non-existent name \"%s\"", name.c_str()));
        } else {
            QCC_DbgPrintf(("UDPTransport::NewAdvertiseOp(): Unregistering advertisement of namePrefix \"%s\"", name.c_str()));
            m_advertising.erase(i);
        }
    }

    isFirst = first;
    bool rc = m_advertising.empty();
    DecrementAndFetch(&m_refCount);
    return rc;
}

bool UDPTransport::NewListenOp(ListenOp op, qcc::String normSpec)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::NewListenOp()"));

    if (op == START_LISTEN) {
        QCC_DbgPrintf(("UDPTransport::NewListenOp(): Registering listen of normSpec \"%s\"", normSpec.c_str()));
        m_listening.push_back(normSpec);
    } else {
        list<qcc::String>::iterator i = find(m_listening.begin(), m_listening.end(), normSpec);
        if (i == m_listening.end()) {
            QCC_DbgPrintf(("UDPTransport::NewAdvertiseOp(): StopListen of non-existent spec \"%s\"", normSpec.c_str()));
        } else {
            QCC_DbgPrintf(("UDPTransport::NewAdvertiseOp(): StopListen of normSpec \"%s\"", normSpec.c_str()));
            m_listening.erase(i);
        }
    }

    bool rc = m_listening.empty();
    DecrementAndFetch(&m_refCount);
    return rc;
}

void UDPTransport::EnableDiscovery(const char* namePrefix, TransportMask transports)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::EnableDiscovery()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("UDPTransport::EnableDiscovery(): Not running or stopping; exiting"));
        DecrementAndFetch(&m_refCount);
        return;
    }

#if WORKAROUND_1298
    if (!m_done1298) {
        QCC_DbgPrintf(("UDPTransport::EnableDiscovery(): Implementing workaround for ASACORE-1298"));
        qcc::String name = WORKAROUND_1298_PREFIX;
        name.append(m_bus.GetInternal().GetGlobalGUID().ToShortString());
        QueueEnableAdvertisement(name, true, TRANSPORT_UDP);
        m_done1298 = true;
    }
#endif

    QueueEnableDiscovery(namePrefix, transports);
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::QueueEnableDiscovery(const char* namePrefix, TransportMask transports)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::QueueEnableDiscovery()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = ENABLE_DISCOVERY_INSTANCE;
    listenRequest.m_requestParam = namePrefix;
    listenRequest.m_requestTransportMask = transports;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::DisableDiscovery(const char* namePrefix, TransportMask transports)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::DisableDiscovery()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("UDPTransport::DisbleDiscovery(): Not running or stopping; exiting"));
        DecrementAndFetch(&m_refCount);
        return;
    }

    QueueDisableDiscovery(namePrefix, transports);
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::QueueDisableDiscovery(const char* namePrefix, TransportMask transports)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::QueueDisableDiscovery()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = DISABLE_DISCOVERY_INSTANCE;
    listenRequest.m_requestParam = namePrefix;
    listenRequest.m_requestTransportMask = transports;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);
    DecrementAndFetch(&m_refCount);
}

QStatus UDPTransport::EnableAdvertisement(const qcc::String& advertiseName, bool quietly, TransportMask transports)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::EnableAdvertisement()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("UDPTransport::EnableAdvertisement(): Not running or stopping; exiting"));
        DecrementAndFetch(&m_refCount);
        return ER_BUS_TRANSPORT_NOT_STARTED;
    }

#if WORKAROUND_1298
    if (!m_done1298) {
        QCC_DbgPrintf(("UDPTransport::EnableAdvertisement(): Implementing workaround for ASACORE-1298"));
        qcc::String name = WORKAROUND_1298_PREFIX;
        name.append(m_bus.GetInternal().GetGlobalGUID().ToShortString());
        QueueEnableAdvertisement(name, true, TRANSPORT_UDP);
        m_done1298 = true;
    }
#endif

    QueueEnableAdvertisement(advertiseName, quietly, transports);
    DecrementAndFetch(&m_refCount);
    return ER_OK;
}


void UDPTransport::QueueEnableAdvertisement(const qcc::String& advertiseName, bool quietly, TransportMask transports)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::QueueEnableAdvertisement()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = ENABLE_ADVERTISEMENT_INSTANCE;
    listenRequest.m_requestParam = advertiseName;
    listenRequest.m_requestParamOpt = quietly;
    listenRequest.m_requestTransportMask = transports;
    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::DisableAdvertisement(const qcc::String& advertiseName, TransportMask transports)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::DisableAdvertisement()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("UDPTransport::DisableAdvertisement(): Not running or stopping; exiting"));
        DecrementAndFetch(&m_refCount);
        return;
    }

    QueueDisableAdvertisement(advertiseName, transports);
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::QueueDisableAdvertisement(const qcc::String& advertiseName, TransportMask transports)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::QueueDisableAdvertisement()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = DISABLE_ADVERTISEMENT_INSTANCE;
    listenRequest.m_requestParam = advertiseName;
    listenRequest.m_requestTransportMask = transports;
    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);
    DecrementAndFetch(&m_refCount);
}

void UDPTransport::FoundCallback::Found(const qcc::String& busAddr, const qcc::String& guid,
                                        std::vector<qcc::String>& nameList, uint32_t timer)
{
//  Makes lots of noise!
    //QCC_DbgTrace(("UDPTransport::FoundCallback::Found(): busAddr = \"%s\" nameList %d", busAddr.c_str(), nameList.size()));

    qcc::String addr("addr=");
    qcc::String port("port=");
    qcc::String comma(",");

    size_t i = busAddr.find(addr);
    if (i == qcc::String::npos) {
        QCC_DbgPrintf(("UDPTransport::FoundCallback::Found(): No addr in busaddr."));
        return;
    }
    i += addr.size();

    size_t j = busAddr.find(comma, i);
    if (j == qcc::String::npos) {
        QCC_DbgPrintf(("UDPTransport::FoundCallback::Found(): No comma after addr in busaddr."));
        return;
    }

    size_t k = busAddr.find(port);
    if (k == qcc::String::npos) {
        QCC_DbgPrintf(("UDPTransport::FoundCallback::Found(): No port in busaddr."));
        return;
    }
    k += port.size();

    /*
     * "addr=192.168.1.1,port=9955"
     *       ^          ^     ^
     *       i          j     k
     */
    qcc::String newBusAddr = qcc::String("udp:guid=") + guid + "," + addr + busAddr.substr(i, j - i) + "," + port + busAddr.substr(k);

//    QCC_DbgPrintf(("UDPTransport::FoundCallback::Found(): newBusAddr = \"%s\".", newBusAddr.c_str()));

    if (m_listener) {
        m_listener->FoundNames(newBusAddr, guid, TRANSPORT_UDP, &nameList, timer);
    }
}

void UDPTransport::NetworkEventCallback::Handler(const std::map<qcc::String, qcc::IPAddress>& ifMap)
{
    IncrementAndFetch(&m_transport.m_refCount);
    QCC_DbgPrintf(("UDPTransport::NetworkEventCallback::Handler()"));

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
        QCC_LogError(ER_BUS_TRANSPORT_NOT_STARTED, ("UDPTransport::NetworkEventCallback::Handler(): Not running or stopping; exiting"));
        return;
    }

    m_transport.QueueHandleNetworkEvent(ifMap);
    DecrementAndFetch(&m_transport.m_refCount);
}

void UDPTransport::QueueHandleNetworkEvent(const std::map<qcc::String, qcc::IPAddress>& ifMap)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgPrintf(("UDPTransport::QueueHandleNetworkEvent()"));

    ListenRequest listenRequest;
    listenRequest.m_requestOp = HANDLE_NETWORK_EVENT;
    listenRequest.ifMap = ifMap;

    m_listenRequestsLock.Lock(MUTEX_CONTEXT);
    /* Process the request */
    RunListenMachine(listenRequest);
    m_listenRequestsLock.Unlock(MUTEX_CONTEXT);
    DecrementAndFetch(&m_refCount);
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
void UDPTransport::HandleNetworkEventInstance(ListenRequest& listenRequest)
{
    QCC_DbgTrace(("UDPTransport::HandleNetworkEventInstance()"));
    std::map<qcc::String, qcc::IPAddress>& ifMap = listenRequest.ifMap;
    QStatus status = ER_OK;
    IncrementAndFetch(&m_refCount);
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
        DecrementAndFetch(&m_refCount);
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
                qcc::String replacedSpec = "udp:addr=" + m_requestedInterfaces[interface].GetAddress().ToString() + ",port=" + U32ToString(m_requestedInterfaces[interface].GetPort());
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
        QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): Setting up socket"));
        SocketFd listenFd = INVALID_SOCKET_FD;
        status = Socket(QCC_AF_INET, QCC_SOCK_DGRAM, listenFd);
        if (status != ER_OK) {
            continue;
        }

        QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): listenFd=%d.", listenFd));

        /*
         * ARDP expects us to use select and non-blocking sockets.
         */
        QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): SetBlocking(listenFd=%d, false)", listenFd));;
        status = qcc::SetBlocking(listenFd, false);
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::HandleNetworkEventInstance(): SetBlocking() failed"));
            qcc::Close(listenFd);
            continue;
        }

        /*
         * We are going to use UDP, and the socket buffers for UDP sockets are
         * most likely set unnecessarily low by default.  IN the optimal case,
         * we are going to want to spew out SEGBMAX * SEGMAX bytes of data on
         * each connection.  Typical values might be 65535 bytes per buffer * 50
         * buffers = 3,276,800 bytes.  Since there may be N connections running
         * over that single socket, it would imply a requirement to buffer N * 3
         * megabytes total.  Unlikely.  We set the buffer large enough to buffer
         * enough in-flight data to service one fully utilized connection
         * completely.  After the buffer fills, we share capacity.
         *
         * Just because we ask for this amount of buffer does not mean we will
         * get it.  Systems provide a maximum number.  On Linux, for example,
         * you can find the maximum value in /proc/sys/net/core/wmem_max.  On my
         * Fedora 20- box it is set to 131071, so the doubled value returned by
         * GetSndBuf() would be 262142.  Consider changing this value on a
         * server-class system.
         *
         * Note that this is not a fatal error if it fails.  We just may get less
         * buffer capacity than we might like.
         */
        size_t sndSize = m_ardpConfig.segmax * m_ardpConfig.segbmax;
        QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): SetSndBuf(listenFd=%d, %u.)", listenFd, sndSize));
        status = qcc::SetSndBuf(listenFd, sndSize);
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::HandleNetworkEventInstance(): SetSndbuf() failed"));
        }

#ifndef NDEBUG
        sndSize = 0;
        qcc::GetSndBuf(listenFd, sndSize);
        QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): GetSndBuf(listenFd=%d) <= %u. bytes)", listenFd, sndSize));
#endif

        /*
         * Ditto for the receive side.  The remote sides of connections may
         * conspire against us to have N fully utilized connections inbound from
         * different remotes to our single instance, but we cannot simply
         * allocate 300 megabytes of buffering, so we set the receive buffer to
         * SEGBMAX * SEGMAX as well, and limit buffer usage by letting UDP drop
         * inbound messages to rate limit overrunning connections.
         *
         * Just because we ask for this amount of buffer does not mean we will
         * get it.  Systems provide a maximum number.  On Linux, for example,
         * you can find the maximum value in /proc/sys/net/core/rmem_max.  On my
         * Fedora 20- box it is set to 131071, so the doubled value returned by
         * GetRcvBuf() would be 262142.  Consider changing this value on a
         * server-class system.
         *
         * Note that this is not a fatal error if it fails.  We just may get less
         * buffer capacity than we might like.
         */
        size_t rcvSize = m_ardpConfig.segmax * m_ardpConfig.segbmax;
        QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): SetSndBuf(listenFd=%d, %u.)", listenFd, rcvSize));
        status = qcc::SetRcvBuf(listenFd, rcvSize);
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::HandleNetworkEventInstance(): SetRcvBuf() failed"));
        }

#ifndef NDEBUG
        rcvSize = 0;
        qcc::GetRcvBuf(listenFd, rcvSize);
        QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): GetRcvBuf(listenFd=%d) <= %u. bytes)", listenFd, rcvSize));
#endif

        /*
         * If ephemeralPort is set, it means that the listen spec did not provide a
         * specific port and wants us to choose one.  In this case, we first try the
         * default port; but it that port is already taken in the system, we let the
         * system assign a new one from the ephemeral port range.
         */
        if (ephemeralPort) {
            QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): ephemeralPort"));
            listenPort = PORT_DEFAULT;
            QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): Bind(listenFd=%d., listenAddr=\"%s\", listenPort=%d.)",
                           listenFd, listenAddr.ToString().c_str(), listenPort));
            status = Bind(listenFd, listenAddr, listenPort);
            if (status != ER_OK) {
                listenPort = 0;
                QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): Bind() failed.  Bind(listenFd=%d., listenAddr=\"%s\", listenPort=%d.)",
                               listenFd, listenAddr.ToString().c_str(), listenPort));
                status = Bind(listenFd, listenAddr, listenPort);
            }
        } else {
            QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): Bind(listenFd=%d., listenAddr=\"%s\", listenPort=%d.)",
                           listenFd, listenAddr.ToString().c_str(), listenPort));
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
            qcc::String normSpec = "udp:addr=" + listenAddr.ToString() + ",port=" + U32ToString(listenPort);
            QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): ephemeralPort.  New normSpec=\"%s\"", normSpec.c_str()));

            /*
             * Okay, we're ready to receive datagrams on this socket now.  Tell the
             * maintenance thread that something happened here and it needs to reload
             * its FDs.
             */
            QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): addeList.push_back(normSpec=\"%s\", listenFd=%d)", normSpec.c_str(), listenFd));

            /* We make a list of the new listen specs on which we are listening so
             * that we can add those to the m_listenFds when we're done procesing
             * the list of network interfaces.
             */
            addedList.push_back(pair<qcc::String, SocketFd>(normSpec, listenFd));
        } else {
            QCC_LogError(status, ("UDPTransport::HandleNetworkEventInstance(): Failed to bind to %s/%d", listenAddr.ToString().c_str(), listenPort));
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
         * data is sent using TCP and unreliable data is sent using UDP (we use UDP
         * but build a reliability layer on top of it).  Also, IP implies either
         * IPv4 or IPv6 addressing.
         *
         * In the UDPTransport, we only support unreliable data transfer over IPv4
         * addresses, so we leave all of the other possibilities turned off (provide
         * a zero port).  Remember the port we enabled so we can re-enable the name
         * service if listeners come and go.
         */
        QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): IpNameService::Instance().Enable()"));
        IpNameService::Instance().Enable(TRANSPORT_UDP, std::map<qcc::String, uint16_t>(), 0, m_listenPortMap, 0, false, false, true, false);

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
        if (!m_routerName.empty() && (m_numUntrustedClients < m_maxUntrustedClients)) {
            QCC_DbgPrintf(("UDPTransport::HandleNetworkEventInstance(): Advertise m_routerName=\"%s\"", m_routerName.c_str()));
            bool isFirst;
            NewAdvertiseOp(ENABLE_ADVERTISEMENT, m_routerName, isFirst);
            QStatus status = IpNameService::Instance().AdvertiseName(TRANSPORT_UDP, m_routerName, true, TRANSPORT_UDP);
            if (status != ER_OK) {
                QCC_LogError(status, ("UDPTransport::HandleNetworkEventInstance(): Failed to AdvertiseNameQuietly \"%s\"", m_routerName.c_str()));
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
                    qcc::String replacedSpec = "udp:addr=" + m_requestedInterfaces[iter->first].GetAddress().ToString() + ",port=" + U32ToString(m_requestedInterfaces[iter->first].GetPort());
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
                    qcc::String replacedSpec = "udp:addr=" + iter->first + ",port=" + U32ToString(m_listenPortMap[iter->first]);
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
    if (!addedList.empty()) {
        m_listenFdsLock.Lock(MUTEX_CONTEXT);
        for (list<pair<qcc::String, SocketFd> >::iterator it = addedList.begin(); it != addedList.end(); it++) {
            m_listenFds.push_back(*it);
        }
        m_reload = STATE_RELOADING;
        m_listenFdsLock.Unlock(MUTEX_CONTEXT);
    }

    /*
     * Signal the (probably) waiting run thread so it will wake up and add this
     * new socket to its list of sockets it is waiting for connections on.
     */
    Alert();

    DecrementAndFetch(&m_refCount);

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
        IpNameService::Instance().Enable(TRANSPORT_UDP, std::map<qcc::String, uint16_t>(), 0, m_listenPortMap, 0, false, false, false, false);
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

void UDPTransport::CheckEndpointLocalMachine(UDPEndpoint endpoint)
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
