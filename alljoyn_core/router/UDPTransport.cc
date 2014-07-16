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

/*
 * How the transport fits into the system
 * ======================================
 *
 * AllJoyn provides the concept of a Transport which provides a relatively
 * abstract way for the daemon to use different network mechanisms for getting
 * Messages from place to another.  Conceptually, think of, for example, a Unix
 * transport that moves bits using unix domain sockets, a Bluetooth transport
 * that moves bits over a Bluetooth link, or a TCP transport that moves Messages
 * over a TCP connection.  A UDP transport moves Messages over UDP datagrams
 * using a reliability layer.
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
 * Perhaps somewhat counter-intuitively, advertisements relating to the UDP
 * Transport use the u4addr (unreliable IPv4 address), u4port (unreliable IPv4
 * port), u6addr (unreliable IPv6 address), and u6port (unreliable IPv6 port).
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
 * specs string (e.g., "unix:abstract=alljoyn;udp:;tcp:;bluetooth:") is provided
 * to TransportList::Start() as a parameter.  The transport specs string is
 * parsed and in the example above, results in "unix" transports, "tcp"
 * transports, "udp" transports and "bluetooth" transports being instantiated
 * and started.  As mentioned previously "udp:" in the daemon translates into
 * UDPTransport.  Once the desired transports are instantiated, each is
 * Start()ed in turn.  In the case of the UDPTransport, this will start the
 * maintenance loop.  Initially there are no sockets to listen on.
 *
 * The daemon then needs to start listening on inbound addresses and ports.
 * This is done by the StartListen() command.  This also takes the same kind of
 * server args string shown above but this time the address and port information
 * are used.  For example, one might use the string
 * "udp:u4addr=0.0.0.0,u4port=9955;" to specify which address and port to listen
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
 * an IP address ("u4addr=xxxx"), port ("y4port=yyyy") in a String.  A check is
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

#define SENT_SANITY 1

using namespace std;
using namespace qcc;

/**
 * This is the time between calls to ManageEndpoints if nothing external is
 * happening to drive it.  Usually, something happens to drive ManageEndpoints
 * like a connection starting or stopping or a connection timing out.  This is
 * basically a watchdog to to keep the pump primed.
 */
const uint32_t UDP_ENDPOINT_MANAGEMENT_TIMER = 1000;

const uint32_t UDP_CONNECT_TIMEOUT = 3000;  /**< How long before we expect a connection to complete */
const uint32_t UDP_CONNECT_RETRIES = 3;     /**< How many times do we retry a connection before giving up */
const uint32_t UDP_DATA_TIMEOUT = 3000;     /**< How long do we wait before retrying sending data */
const uint32_t UDP_DATA_RETRIES = 5;        /**< How many times to we try do send data before giving up and terminating a connection */
const uint32_t UDP_PERSIST_TIMEOUT = 5000;  /**< How long do we wait before pinging the other side due to a zero window */
const uint32_t UDP_PERSIST_RETRIES = 5;     /**< How many times do we do a zero window ping before giving up and terminating a connection */
const uint32_t UDP_PROBE_TIMEOUT = 10000;   /**< How long to we wait on an idle link before generating link activity */
const uint32_t UDP_PROBE_RETRIES = 5;       /**< How many times do we try to probe on an idle link before terminating the connection */
const uint32_t UDP_DUPACK_COUNTER = 1;      /**< How many duplicate acknowledgements to we need to trigger a data retransmission */
const uint32_t UDP_TIMEWAIT = 1000;         /**< How long do we stay in TIMWAIT state before releasing the per-connection resources */

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

const char* TestConnStr = "ARDP TEST CONNECT REQUEST";
const char* TestAcceptStr = "ARDP TEST ACCEPT";

#ifndef NDEBUG
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
#endif // NDEBUG


#ifndef NDEBUG
#define SEAL_SIZE 4

void SealBuffer(uint8_t* p)
{
    *p++ = 'S';
    *p++ = 'E';
    *p++ = 'A';
    *p++ = 'L';
}

void CheckSeal(uint8_t* p)
{
    assert(*p++ == 'S' && *p++ == 'E' && *p++ == 'A' && *p++ == 'L' && "CheckSeal(): Seal blown");
}
#endif

class _UDPEndpoint;

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
        m_dataTimeout(0),
        m_dataRetries(0),
        m_lock(),
        m_disc(false),
        m_discSent(false),
        m_discStatus(ER_OK),
        m_writeEvent(NULL),
        m_writesOutstanding(0),
        m_writeWaits(0),
        m_buffers()
    {
        QCC_DbgTrace(("ArdpStream::ArdpStream()"));
        m_writeEvent = new qcc::Event();
    }

    virtual ~ArdpStream()
    {
        QCC_DbgTrace(("ArdpStream::~ArdpStream()"));

        QCC_DbgPrintf(("ArdpStream::~ArdpStream(): delete events"));
        delete m_writeEvent;
        m_writeEvent = NULL;
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
     * Get the number of outstanding write operations in process on the stream.
     * connection.
     */
    uint32_t GetWritesOutstanding()
    {
        QCC_DbgTrace(("ArdpStream::GetWritesOutstanding() => %d.", m_writesOutstanding));
        return m_writesOutstanding;
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

    void WakeThreadSet()
    {
        QCC_DbgTrace(("ArdpStream::WakeThreadSet()"));

        m_lock.Lock(MUTEX_CONTEXT);
        for (set<ThreadEntry>::iterator i = m_threads.begin(); i != m_threads.end(); ++i) {
            QCC_DbgTrace(("ArdpStream::Alert(): Wake thread %p waiting on stream %p", i->m_thread, i->m_stream));
            i->m_stream->m_writeEvent->SetEvent();
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
     * Get the data transmission timeout that the underlying ARDP protocol
     * connection will be using
     */
    uint32_t GetDataTimeout() const
    {
        QCC_DbgTrace(("ArdpStream::GetDataTimeout(): => %d.", m_dataTimeout));
        return m_dataTimeout;
    }

    /**
     * Set the data transmission timeout that the underlying ARDP protocol
     * connection will be using
     */
    void SetDataTimeout(uint32_t dataTimeout)
    {
        QCC_DbgTrace(("ArdpStream::SetDataTimeout(dataTimeout=%d.)", dataTimeout));
        m_dataTimeout = dataTimeout;
    }

    /**
     * Get the data transmission retries that the underlying ARDP protocol
     * connection will be using
     */
    uint32_t GetDataRetries() const
    {
        QCC_DbgTrace(("ArdpStream::GetDataRetries(): => %d.", m_dataRetries));
        return m_dataRetries;
    }

    /**
     * Set the data transmission retries that the underlying ARDP protocol
     * connection will be using
     */
    void SetDataRetries(uint32_t dataRetries)
    {
        QCC_DbgTrace(("ArdpStream::SetDataRetries(dataRetries=%d.)", dataRetries));
        m_dataRetries = dataRetries;
    }

    /**
     * Set the stream's write event if it exists
     */
    void SetWriteEvent()
    {
        QCC_DbgTrace(("ArdpStream::SetWriteEvent()"));
        m_lock.Lock(MUTEX_CONTEXT);
        if (m_writeEvent) {
            m_writeEvent->SetEvent();
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
     * TODO: Note that the blocking is on an endpoint-by-endpoint basis, which
     * means there is a write event per endpoint.  This could be changed to one
     * event per transport, but would mean waking all blocked threads only to
     * have one of them succeed and the rest go back to sleep if the event
     * wasn't directed at them.  This is the classic thundering herd, but trades
     * CPU for event resources which may be a good way to go since our events
     * can be so expensive.  It's a simple change conceptually but there is no
     * bradcast condition variable in common, which would be the way to go.
     *
     * For now, we will take the one event per endpoint approach and optimize
     * that as time permits.
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

        if (m_transport->IsRunning() == false || m_transport->m_stopping == true) {
            status = ER_UDP_STOPPING;
            QCC_LogError(status, ("ArdpStream::PushBytes(): UDP Transport not running or stopping"));
            return status;
        }

        if (m_disc) {
            status = m_discStatus;
            QCC_LogError(status, ("ArdpStream::PushBytes(): ARDP connection found disconnected"));
            return status;
        }

        /*
         * There's a new thread in town, so add it to the list of threads
         * wandering around in the associated endpoint.  We need to keep track
         * of this in case the endpoint is stopped while the current thread is
         * wandering around in the stream trying to get its send done.
         */
        AddCurrentThread();

#ifndef NDEBUG
        DumpBytes((uint8_t*)buf, numBytes);
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
         * double-check, we add our own timeout that expires some time after we
         * expect ARDP to time out.  On a write that would be at
         *
         *    dataTimeout * (1 + dataRetries)
         *
         * To give ARDP a chance, we timeout one retry interval later, at
         *
         *    dataTimeout * (2 + dataRetries)
         *
         */
        uint32_t timeout = GetDataTimeout() * (2 + GetDataRetries());

        Timespec tStart;
        GetTimeNow(&tStart);
        QCC_DbgPrintf(("ArdpStream::PushBytes(): Start time is %d.", tStart));

        /*
         * Now we get down to business.  We are going to enter a loop in which
         * we retry the write until it succeeds.  The write can either be a soft
         * failure which means that the protocol is applying backpressure and we
         * should try again "later" or it can be a hard failure which means the
         * underlying UDP send has failed.  In that case, we give up since
         * presumably something bad has happened, like the Wi-Fi has
         * disassociated or someone has unplugged a cable.
         */
        while (true) {
            if (m_transport->IsRunning() == false || m_transport->m_stopping == true) {
#ifndef NDEBUG
                CheckSeal(buffer + numBytes);
#endif
                delete[] buffer;
                status = ER_UDP_STOPPING;
                QCC_LogError(status, ("ArdpStream::PushBytes(): UDP Transport not running or stopping"));
                RemoveCurrentThread();
                return status;
            }

            Timespec tNow;
            GetTimeNow(&tNow);

            int32_t tRemaining = tStart + timeout - tNow;
            QCC_DbgPrintf(("ArdpStream::PushBytes(): tRemaining is %d.", tRemaining));
            if (tRemaining <= 0) {
#ifndef NDEBUG
                CheckSeal(buffer + numBytes);
#endif
                delete[] buffer;
                status = ER_TIMEOUT;
                QCC_LogError(status, ("ArdpStream::PushBytes(): Timed out"));
                RemoveCurrentThread();
                return status;
            }

            m_transport->m_ardpLock.Lock();
            status = ARDP_Send(m_handle, m_conn, buffer, numBytes, ttl);
            m_transport->m_ardpLock.Unlock();

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
                m_transport->m_cbLock.Lock();
#if SENT_SANITY
                m_sentSet.insert(buffer);
#endif
                ++m_writesOutstanding;
                QCC_DbgPrintf(("ArdpStream::PushBytes(): ARDP_Send(): Success. m_writesOutstanding=%d.", m_writesOutstanding));
                m_transport->m_cbLock.Unlock();
                numSent = numBytes;
                RemoveCurrentThread();
                return status;
            }

            /*
             * If the send failed, and the failure was not due to the application
             * of backpressure by the protocol, we have a hard failure and we need
             * to give up.  Since the buffer wasn't sent, the callback won't happen
             * and we need to dispose of it here and now.
             */
            if (status != ER_ARDP_BACKPRESSURE) {
#ifndef NDEBUG
                CheckSeal(buffer + numBytes);
#endif
                delete[] buffer;
                QCC_LogError(status, ("ArdpStream::PushBytes(): ARDP_Send(): Hard failure"));
                RemoveCurrentThread();
                return status;
            }

            /*
             * Backpressure has been applied.  We can't send another message on
             * this connection until the other side ACKs one of the outstanding
             * datagrams.  It communicates this to us by a send callback which,
             * in turn, sets an event that wakes us up.
             */
            if (status == ER_ARDP_BACKPRESSURE) {
                QCC_DbgPrintf(("ArdpStream::PushBytes(): ER_ARDP_BACKPRESSURE"));

                /*
                 * Multiple threads could conceivably be trying to write at the
                 * same time another thread fires callbacks, so we have to be
                 * careful.  If m_writesOutstanding is non-zero, the ARDP
                 * protocol has a contract with us to call back when writes are
                 * is complete.  To make sure we are synchronized with the
                 * callback thread, we release the callback lock during the call
                 * to Event::Wait().
                 *
                 * To make sure only one of the threads does the reset of the
                 * event (confusing another), we keep track of how many are
                 * waiting at any one time and only let the first one reset the
                 * underlying event.  This means that a second waiter could be
                 * awakened unnecessarily, but it will immediately try agian and
                 * go back to sleep.
                 */
                m_transport->m_cbLock.Lock();
                QCC_DbgPrintf(("ArdpStream::PushBytes(): Backpressure. m_writesOutstanding=%d.", m_writesOutstanding));

                /*
                 * It is possible that between the time we called ARDP_Send and
                 * the time we just took the callback lock immediately above,
                 * all (especially if the window is one) of the previous sends
                 * that caused the rejection of the current send has actually
                 * completed and relieved the backpressure.  Now that we are in
                 * firm control of the process with the lock taken, check to see
                 * if there are any writes outstanding.  If there are not, we
                 * will never get a callback to wake us up, so we need to loop
                 * back around and see if we can write again.  Since there are
                 * no writes outstanding, the answer will be yes.
                 */
                if (m_writesOutstanding == 0) {
                    m_transport->m_cbLock.Unlock();
                    QCC_DbgPrintf(("ArdpStream::PushBytes(): Backpressure relieved"));
                    continue;
                }

                /*
                 * Multiple threads could conceivably be trying to write at the
                 * same time another thread fires callbacks, so we have to be
                 * careful.  To make sure only one of the writer threads does
                 * the reset of the event (confusing another), we keep track of
                 * how many are waiting at any one time and only let the first
                 * one reset the underlying event.  This means that a second
                 * waiter could be awakened unnecessarily, but it will
                 * immediately try again and go back to sleep.  To make sure we
                 * are synchronized with the callback thread, we release the
                 * callback lock during the call to Event::Wait().
                 */
                QCC_DbgPrintf(("ArdpStream::PushBytes(): Backpressure. m_writeWaits=%d.", m_writeWaits));
                if (m_writeWaits == 0) {
                    QCC_DbgPrintf(("ArdpStream::PushBytes(): Backpressure. Reset write event"));
                    m_writeEvent->ResetEvent();
                }
                ++m_writeWaits;
                QCC_DbgPrintf(("ArdpStream::PushBytes(): Backpressure. Event::Wait(). m_writeWaits=%d.", m_writeWaits));
                status = qcc::Event::Wait(*m_writeEvent, m_transport->m_cbLock, tRemaining);
                m_transport->m_cbLock.Lock();
                QCC_DbgPrintf(("ArdpStream::PushBytes(): Backpressure. Back from Event::Wait(). m_writeWaits=%d.", m_writeWaits));
                --m_writeWaits;
                QCC_DbgPrintf(("ArdpStream::PushBytes(): Backpressure. Decremented m_writeWaits=%d.", m_writeWaits));
                m_transport->m_cbLock.Unlock();

                /*
                 * If the wait fails, then there's nothing we can do but bail.  If we
                 * never actually started the send sucessfully, the callback will never
                 * happen and we need to free the buffer we newed here.
                 */
                if (status != ER_OK && status != ER_TIMEOUT) {
#ifndef NDEBUG
                    CheckSeal(buffer + numBytes);
#endif
                    delete[] buffer;
                    QCC_LogError(status, ("ArdpStream::PushBytes(): WaitWriteEvent() failed"));
                    RemoveCurrentThread();
                    return status;
                }

                /*
                 * If there was a disconnect in the underlying connection, there's
                 * nothing we can do but return the error.
                 */
                if (m_disc) {
#ifndef NDEBUG
                    CheckSeal(buffer + numBytes);
#endif
                    delete[] buffer;
                    QCC_LogError(m_discStatus, ("ArdpStream::PushBytes(): Disconnected"));
                    RemoveCurrentThread();
                    return m_discStatus;
                }

                QCC_DbgPrintf(("ArdpStream::PushBytes(): Backpressure loop"));
            }

            /*
             * We detected backpressure and waited until a callback indicated
             * that the backpressure was relieved.  We gave up the cb lock,
             * so now we loop back around and try the ARDP_Send again, maybe
             * waiting again.
             */
        }

        assert(0 && "ArdpStream::PushBytes(): Impossible condition");
        RemoveCurrentThread();
        return ER_FAIL;
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
     * Process a disconnect event, either local or remote.
     */
    void Disconnect(bool sudden, QStatus status)
    {
        if (status == ER_OK) {
            assert(sudden == false);
        }
        if (sudden) {
            assert(status != ER_OK);
        }

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
#if SENT_SANITY
        m_transport->m_cbLock.Lock();
        set<uint8_t*>::iterator i = find(m_sentSet.begin(), m_sentSet.end(), buf);
        if (i == m_sentSet.end()) {
            QCC_LogError(ER_FAIL, ("ArdpStream::SendCb(): Callback for buffer never sent or already freed (%p, %d.).  Ignored", buf, len));
        } else {
            m_sentSet.erase(i);
#ifndef NDEBUG
            CheckSeal(buf + len);
#endif
            delete[] buf;
        }
        m_transport->m_cbLock.Unlock();
#else
#ifndef NDEBUG
        CheckSeal(buf + len);
#endif
        delete[] buf;
#endif

        /*
         * If there are any threads waiting for a chance to send bits, wake them
         * up.  They will retry their sends when this event gets set.  If the
         * send callbacks are part of normal operation, the sends may succeed
         * the next time around.  If this callback is part of disconnect
         * processing the next send will fail with an error; and PushBytes()
         * will manage the outstanding write count.
         */
        if (m_writeEvent) {
            QCC_DbgPrintf(("ArdpStream::SendCb(): SetEvent()"));
            m_transport->m_cbLock.Lock();
            m_writeEvent->SetEvent();
            m_transport->m_cbLock.Unlock();
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

    UDPTransport* m_transport;        /**< The transport that created the endpoint that created the stream */
    _UDPEndpoint* m_endpoint;         /**< The endpoint that created the stream */
    ArdpHandle* m_handle;             /**< The handle to the ARDP protocol instance this stream works with */
    ArdpConnRecord* m_conn;           /**< The ARDP connection associated with this endpoint / stream combination */
    uint32_t m_dataTimeout;           /**< The timeout that the ARDP protocol will use when retrying sends */
    uint32_t m_dataRetries;           /**< The number of retries that the ARDP protocol will use when sending */
    qcc::Mutex m_lock;                /**< Mutex that protects m_threads and disconnect state */
    bool m_disc;                      /**< Set to true when ARDP fires the DisconnectCb on the associated connection */
    bool m_discSent;                  /**< Set to true when the endpoint calls ARDP_Disconnect */
    QStatus m_discStatus;             /**< The status code that was the reason for the last disconnect */
    qcc::Event* m_writeEvent;         /**< The write event that callers are blocked on to apply backpressure */
    int32_t m_writesOutstanding;      /**< The number of writes that are outstanding with ARDP */
    int32_t m_writeWaits;             /**< The number of Threads that are blocked trying to write to an ARDP connection */

    std::set<ThreadEntry> m_threads;  /**< Threads that are wandering around in the stream and possibly associated endpoint */

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

    std::list<BufEntry> m_buffers;
};


bool operator<(const ArdpStream::ThreadEntry& lhs, const ArdpStream::ThreadEntry& rhs)
{
    return lhs.m_thread < rhs.m_thread;
}

/*
 * An endpoint class to handle the details of authenticating a connection in a
 * way that avoids denial of service attacks.
 */
class _UDPEndpoint : public _RemoteEndpoint {
  public:
    /**
     * The UDP Transport is a flavor of a RemoteEndpoint.  The daemon thinks of
     * remote endpoints as moving through a number of states, some that have
     * threads wandering around and some that do not.  In order to make sure we
     * are in agreement with what the daemon things we will be doing we keep
     * state regarding what threads would be doing if they were actually here
     * and running.
     */
    enum EndpointState {
        EP_ILLEGAL = 0,
        EP_INITIALIZED,      /**< This endpoint structure has been allocated but not used */
        EP_FAILED,           /**< Starting has failed and this endpoint is not usable */
        EP_STARTING,         /**< The endpoint is being started, threads would be starting */
        EP_STARTED,          /**< The endpoint is ready for use, threads would be running */
        EP_STOPPING,         /**< The endpoint is stopping but join has not been called */
        EP_JOINED,           /**< The endpoint is stopping and join has been called */
        EP_DONE              /**< Threads have been shut down and joined */
    };

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
        m_suddenDisconnect(incoming),
        m_registered(false),
        m_sideState(SIDE_INITIALIZED),
        m_epState(EP_INITIALIZED),
        m_tStart(qcc::Timespec(0)),
        m_remoteExited(false),
        m_exitScheduled(false),
        m_disconnected(false),
        m_refCount(0),
        m_stateLock()
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

        /*
         * Double check that the remote endpoint is sure that its threads are gone,
         * since our destructor is going to call its Stop() and Join() anyway.
         * before deleting it.
         */
        _RemoteEndpoint::Stop();
        _RemoteEndpoint::Exited();
        _RemoteEndpoint::Join();

        assert(IncrementAndFetch(&m_refCount) == 1 && "_UDPEndpoint::~_UDPEndpoint(): non-zero reference count");

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
     * This is really part of debug code to absolutely, positively ensure that
     * there are no threads wandering around in an endpoint as it gets destroyed.
     */
    uint32_t IncrementRefs()
    {
        return IncrementAndFetch(&m_refCount);
    }

    /**
     * This is really part of debug code to absolutely, positively ensure that
     * there are no threads wandering around in an endpoint as it gets destroyed.
     */
    uint32_t DecrementRefs()
    {
        return DecrementAndFetch(&m_refCount);
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
        assert(GetEpState() == EP_INITIALIZED && "_UDPEndpoint::Start(): Endpoint not in EP_INITIALIZED state");
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
         * If we've never been started, there's nothing to do.
         */
        if (GetEpState() == EP_INITIALIZED) {
            QCC_DbgPrintf(("_UDPEndpoint::Stop(): Never Start()ed"));
            if (m_stream) {
                m_stream->EarlyExit();
            }
            m_stateLock.Unlock(MUTEX_CONTEXT);
            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
            DecrementAndFetch(&m_refCount);
            return ER_OK;
        }

        /*
         * If we're already on the way toward being shut down, there's nothing to do.
         */
        if (GetEpState() != EP_STARTED) {
            QCC_DbgPrintf(("_UDPEndpoint::Stop(): Already stopping or done"));
            m_stateLock.Unlock(MUTEX_CONTEXT);
            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
            DecrementAndFetch(&m_refCount);
            return ER_OK;
        }

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

        assert(found == 1 && "_UDPEndpoint::Stop(): Endpoint not on exactly one pending list");
#endif

        /*
         * If there was a remote (sudden) disconnect, the disconnect callback
         * will disconnect the stream and call Stop.  This may precipitate a
         * flood of events in the daemon with router endpoints being dereferenced
         * and disconnected and destroyed.  This will likely result in Stop()
         * being called multiple times.  The stream remembers what started it all
         * and so it is safe to call it with ER_UDP_LOCAL_DISCONNECT even though
         * this stop may have been called as part of remote disconnect handling
         * just so long as the disconnect callback got there first.
         *
         * The Disconnect() below will call ARDP_Disconnect().  Calling out to
         * ARDP with locks held is dangerous from a deadlock perspective.  For
         * example, we took the endpoint list lock above, and will want to take
         * ARDP lock in Disconnect(); but in the accept callback it will be
         * called with ARDP lock taken and want to get the endpoint list lock.
         * This is a recipe for deadlock.  We must set the state and talk to
         * the management thread, then release the locks and finally call out
         * to Disconnect().  Through this process we leave the thread reference
         * count incremented so we know there is a thread that will reappear
         * popping back out through here eventually.
         */
        SetEpStopping();

        m_transport->m_manage = UDPTransport::STATE_MANAGE;
        m_transport->Alert();

        m_stateLock.Unlock(MUTEX_CONTEXT);
        m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);

        if (m_stream) {
            m_stream->WakeThreadSet();
            m_stream->Disconnect(false, ER_UDP_LOCAL_DISCONNECT);
        }

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
         * If we've never been started, there's nothing to do.
         */
        if (GetEpState() == EP_INITIALIZED) {
            QCC_DbgPrintf(("_UDPEndpoint::Join(): Never Start()ed"));
            if (m_stream) {
                m_stream->EarlyExit();
            }
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
         * EP_DONE or EP_FAILED, all of which mean we have nothing to do.
         */
        if (GetEpState() == EP_JOINED || GetEpState() == EP_DONE || GetEpState() == EP_FAILED) {
            QCC_DbgPrintf(("_UDPEndpoint::Join(): Already Join()ed"));
            m_stateLock.Unlock(MUTEX_CONTEXT);
            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
            DecrementAndFetch(&m_refCount);
            return ER_OK;
        }

        /*
         * Now, down to business.  If there were any threads blocked waiting to
         * get bytes through to a remote host, they should have been woken up in
         * Stop() and in the normal course of events they should have woken up
         * and left of their own accord.  ManageEndpoints should have waited
         * for that to happen before calling Join().  If we happen to get caught
         * with active endpoints alive when the TRANSPORT is shutting down,
         * however, we may have to wait for that to happen here.  We can't do
         * anything until there are no threads wandering around in the endpoint
         * or risk crashing the daemon.  We just have to trust that they will
         * cooperate.
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
             * not be a problem to bock other threads, since they are going
             * away or are already gone.
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
            QCC_LogError(ER_UDP_STOPPING, ("_UDPEndpoint::Join(): Not disconnected"));
            m_stream->EarlyExit();
        }

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
        QCC_DbgHLPrintf(("_UDPEndpoint::CreateStream(handle=0x%0, conn=%p)", handle, conn));

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
        m_stream->SetDataTimeout(dataTimeout);
        m_stream->SetDataRetries(dataRetries);

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
     */
    QStatus PushMessage(Message& msg)
    {
        IncrementAndFetch(&m_refCount);

        /*
         * We need to make sure that this endpoint stays on one of our endpoint
         * lists while we figure out what to do with it.  If we are taken off
         * the endpoint list we could actually be deleted while doing this
         * operation, so take the lock to make sure at least the UDP transport
         * holds a reference during this process.
         */
        m_transport->m_endpointListLock.Lock(MUTEX_CONTEXT);

        QCC_DbgHLPrintf(("_UDPEndpoint::PushMessage(msg=%p)", &msg));
        if (GetEpState() != EP_STARTED) {
            QStatus status = ER_UDP_STOPPING;
            QCC_LogError(status, ("_UDPEndpoint::PushBytes(): UDP Transport stopping"));
            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
            DecrementAndFetch(&m_refCount);
            return status;
        }

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
            QCC_LogError(ER_UDP_STOPPING, ("_UDPEndpoint::PushMessage(): Endpoint is gone"));
            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
            DecrementAndFetch(&m_refCount);
            return ER_UDP_STOPPING;
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
        return status;
    }

    /**
     * Callback (indirectly) from the ARDP implementation letting us know that
     * our connection has been disconnected for some reason.
     */
    void DisconnectCb(ArdpHandle* handle, ArdpConnRecord* conn, QStatus status)
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
         */
        if (m_stream) {
            QCC_DbgPrintf(("_UDPEndpoint::DisconnectCb(): Disconnect(): m_stream=%p", m_stream));
            m_stream->Disconnect(sudden, status);
        }

        /*
         * We believe that the connection must go away here since this is either
         * an unsolicited remote disconnection which always resutls in the
         * connection going away or a confirmation of a local disconnect that
         * also results in the connection going away.
         */
        m_conn = NULL;

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
         * that must have happened that precipitated the confirmation callback.
         */
        Stop();

        DecrementAndFetch(&m_refCount);
    }

    /**
     * Callback letting us know that we received data over our connection.  We
     * are passed responsibility for the buffer in this callback.
     *
     * for deadlock avoidance purposes, this callback always comes from the
     * transport dispatcher thread.
     */
    void RecvCb(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* rcv, QStatus status)
    {
        IncrementAndFetch(&m_refCount);
        QCC_DbgHLPrintf(("_UDPEndpoint::RecvCb(handle=%p, conn=%p, rcv=%p, status=%s)",
                         handle, conn, rcv, QCC_StatusText(status)));

        /*
         * Our contract with ARDP says that it will provide us with valid data
         * if it calls us back.
         */
        assert(rcv != NULL && rcv->data != NULL && rcv->datalen != 0 && "_UDPEndpoint::RecvCb(): No data from ARDP in RecvCb()");

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
                QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): found endpoint with conn ID == %d. on m_endpointList", GetConnId()));
                ++found;
            }
        }
        assert(found == 1 && "_UDPEndpoint::RecvCb(): Endpoint is gone");
#endif

        if (GetEpState() != EP_STARTING && GetEpState() != EP_STARTED) {
            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): Not accepting inbound messages"));

            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): ARDP_RecvReady()"));
            m_transport->m_ardpLock.Lock();
            ARDP_RecvReady(handle, conn, rcv);
            m_transport->m_ardpLock.Unlock();

            m_transport->m_endpointListLock.Unlock(MUTEX_CONTEXT);
            DecrementAndFetch(&m_refCount);
            return;
        }

        if (rcv->fcnt == 0 || rcv->fcnt > 3) {
            QCC_LogError(ER_UDP_INVALID, ("_UDPEndpoint::RecvCb(): Unexpected rcv->fcnt==%d.", rcv->fcnt));

            QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): ARDP_RecvReady()"));
            m_transport->m_ardpLock.Lock();
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
        DumpBytes(messageBuf, messageLen);
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
            ARDP_RecvReady(handle, conn, rcv);
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
            QCC_LogError(status, ("_UDPEndpoint::RecvCb(): Can't Unmarhsal() Message"));

            /*
             * If there's some kind of problem, we have to give the buffer
             * back to the protocol now.
             */
            m_transport->m_ardpLock.Lock();
            ARDP_RecvReady(handle, conn, rcv);
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

        /*
         * Now, we have an AllJoyn Message that is ready for delivery.
         * We just hand it off to the daemon router at this point.  It
         * will try to find the implied destination endpoint and stick
         * it on the receive queue for that endpoint.
         *
         * TODO: If the PushMessage cannot enqueue the message it blocks!  We
         * need it to fail, not to block.
         */
        QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): PushMessage()"));
        status = m_transport->m_bus.GetInternal().GetRouter().PushMessage(msg, bep);
        if (status != ER_OK) {
            QCC_LogError(status, ("_UDPEndpoint::RecvCb(): PushMessage failed"));
        }

        /*
         * TODO: If the daemon router cannot deliver the message, we need to
         * enqueue it on a list and NOT call ARDP_RecvReady().  This opens the
         * receive window for the protocol, so after we enqueue a receive
         * window's full of data the protocol will apply backpressure to the
         * remote side which will stop sending data and further apply
         * backpressure to the ultimate sender.  We either need to retry
         * delivery or get a callback from the destination endpoint telling us
         * to retry.
         */
        QCC_DbgPrintf(("_UDPEndpoint::RecvCb(): ARDP_RecvReady()"));
        m_transport->m_ardpLock.Lock();
        ARDP_RecvReady(handle, conn, rcv);
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
     * Callback from the ARDP implementation letting us know that the remote side
     * has acknowledged reception of our data and the buffer can be recycled/freed
     */
    void SendCb(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status)
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
        SetConnId(ARDP_GetConnId(m_handle, conn));
    }

    /**
     * Get the connection ID of the original ARDP protocol connection.  We keep
     * this around for debugging purposes after the connection goes away.
     */
    uint32_t GetConnId()
    {
        QCC_DbgTrace(("_UDPEndpoint::GetConnId(): => %d.", m_id));
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
        QCC_DbgTrace(("_UDPEndpoint::GetStartTime(): => %d.", m_tStart));
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
        QCC_DbgTrace(("_UDPEndpoint::GetStopTime(): => %d.", m_tStop));
        return m_tStop;
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
     * Set the state of the endpoint to failed
     */
    void SetEpFailed(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::GetEpFailed()"));
        m_epState = EP_FAILED;
    }

    /**
     * Set the state of the endpoint to starting
     */
    void SetEpStarting(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpStarting()"));
        assert(m_epState != EP_STARTING && m_epState != EP_STARTED);
        m_epState = EP_STARTING;
    }

    /**
     * Set the state of the endpoint to started
     */
    void SetEpStarted(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpStarted()"));
        assert(m_epState != EP_STARTED);
        m_epState = EP_STARTED;
    }

    /**
     * Set the state of the endpoint to stopping
     */
    void SetEpStopping(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpStopping()"));
        if (m_epState != EP_STARTING && m_epState == EP_STARTED) {
            QCC_DbgPrintf(("_UDPEndpoint::SetEpStopping(): m_epState == %d", m_epState));
        }
        assert(m_epState == EP_STOPPING || m_epState == EP_STARTING || m_epState == EP_STARTED);

        Timespec tNow;
        GetTimeNow(&tNow);
        SetStopTime(tNow);

        m_epState = EP_STOPPING;
    }

    /**
     * Set the state of the endpoint to joined
     */
    void SetEpJoined(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpJoined()"));
        /*
         * Pretty much any state is legal to call Join() in except started
         * state.  This always requires a Stop() first.
         */
        assert(m_epState != EP_STARTED);
        m_epState = EP_JOINED;
    }

    /**
     * Set the state of the endpoint to done
     */
    void SetEpDone(void)
    {
        QCC_DbgTrace(("_UDPEndpoint::SetEpDone()"));
        assert(m_epState == EP_FAILED || m_epState == EP_JOINED);
        m_epState = EP_DONE;
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
        QStatus status = ER_OK;
        QCC_LogError(status, ("_UDPEndpoint::SetLinkTimeout(): Ignored"));
        return status;
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
    bool m_remoteExited;              /**< Indicates if the remote endpoint exit function has been run.  Cannot delete until true. */
    bool m_exitScheduled;             /**< Indicates if the remote endpoint exit function has been scheduled. */
    volatile bool m_disconnected;     /**< Indicates an interlocked handling of the ARDP_Disconnect has happened */
    volatile int32_t m_refCount;      /**< Incremented if a thread is wandering through the endpoint, decrememted when it leaves */
    qcc::Mutex m_stateLock;           /**< Mutex protecting the endpoint state against multiple threads attempting changes */
};

/**
 * Construct a UDP Transport object.
 */
UDPTransport::UDPTransport(BusAttachment& bus) :
    Thread("UDPTransport"),
    m_bus(bus), m_stopping(false), m_listener(0),
    m_foundCallback(m_listener), m_isAdvertising(false), m_isDiscovering(false), m_isListening(false),
    m_isNsEnabled(false),
    m_reload(STATE_RELOADING),
    m_manage(STATE_MANAGE),
    m_listenPort(0), m_nsReleaseCount(0),
    m_routerName(), m_maxUntrustedClients(0), m_numUntrustedClients(0),
    m_authTimeout(0), m_sessionSetupTimeout(0), m_maxAuth(0), m_currAuth(0), m_maxConn(0), m_currConn(0),
    m_ardpLock(), m_cbLock(), m_handle(NULL),
    m_dispatcher(NULL), m_workerCommandQueue(), m_workerCommandQueueLock()
{
    QCC_DbgHLPrintf(("UDPTransport::UDPTransport()"));

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
    ardpConfig.dataTimeout = config->GetLimit("udp_data_timeout", UDP_DATA_TIMEOUT);
    ardpConfig.dataRetries = config->GetLimit("udp_data_retries", UDP_DATA_RETRIES);
    ardpConfig.persistTimeout = config->GetLimit("udp_persist_timeout", UDP_PERSIST_TIMEOUT);
    ardpConfig.persistRetries = config->GetLimit("udp_persist_retries", UDP_PERSIST_RETRIES);
    ardpConfig.probeTimeout = config->GetLimit("udp_probe_timeout", UDP_PROBE_TIMEOUT);
    ardpConfig.probeRetries = config->GetLimit("udp_probe_retries", UDP_PROBE_RETRIES);
    ardpConfig.dupackCounter = config->GetLimit("udp_dupack_counter", UDP_DUPACK_COUNTER);
    ardpConfig.timewait = config->GetLimit("udp_timewait", UDP_TIMEWAIT);
    memcpy(&m_ardpConfig, &ardpConfig, sizeof(ArdpGlobalConfig));

    /*
     * User configured UDP-specific values trump defaults if longer.
     */
    qcc::Timespec t = Timespec(ardpConfig.connectTimeout * ardpConfig.connectRetries);
    if (m_authTimeout < t) {
        m_authTimeout = m_sessionSetupTimeout = t;
    }

    /*
     * Initialize the hooks to and from the ARDP protocol
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
    ARDP_StartPassive(m_handle);
    m_ardpLock.Unlock();

}

/**
 * Destroy a UDP Transport object.
 */
UDPTransport::~UDPTransport()
{
    QCC_DbgHLPrintf(("UDPTransport::~UDPTransport()"));
    Stop();
    Join();

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

        QStatus status = Event::Wait(checkEvents, signaledEvents);
        /*
         * This should never happen since we provide a timeout value of
         * WAIT_FOREVER by default, but it does.  This means that there is a
         * problem in the Windows implementation of Event::Wait().
         */
        if (status == ER_TIMEOUT) {
//          QCC_LogError(status, ("UDPTransport::DispatcherThread::Run(): Catching Windows returning ER_TIMEOUT from Event::Wait()"));
            continue;
        }

        if (ER_OK != status) {
            QCC_LogError(status, ("UDPTransport::DispatcherThread::Run(): Event::Wait failed"));
            break;
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
                    m_transport->DoConnectCb(entry.m_handle, entry.m_conn, entry.m_passive, entry.m_buf, entry.m_len, entry.m_status);
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
                             * amounting to usually a push onto a messsage queue.
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
                                    ep->SendCb(entry.m_handle, entry.m_conn, entry.m_buf, entry.m_len, entry.m_status);
                                    break;
                                }

                            case WorkerCommandQueueEntry::RECV_CB:
                                {
                                    QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): RECV_CB: RecvCb()"));
                                    ep->RecvCb(entry.m_handle, entry.m_conn, entry.m_rcv, entry.m_status);
                                    break;
                                }

                            case WorkerCommandQueueEntry::DISCONNECT_CB:
                                {
                                    QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): DISCONNECT_CB: DisconnectCb()"));
                                    ep->DisconnectCb(entry.m_handle, entry.m_conn, entry.m_status);
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
                                QCC_DbgPrintf(("UDPTransport::DispatcherThread::Run(): Orphaned RECV_CB: ARDP_RecvReady()"));
                                m_transport->m_ardpLock.Lock();
                                ARDP_RecvReady(entry.m_handle, entry.m_conn, entry.m_rcv);
                                m_transport->m_ardpLock.Unlock();
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
                                          new CallbackImpl<FoundCallback, void, const qcc::String&, const qcc::String&, std::vector<qcc::String>&, uint8_t>
                                              (&m_foundCallback, &FoundCallback::Found));

    QCC_DbgPrintf(("UDPTransport::Start(): Spin up message dispatcher thread"));
    m_dispatcher = new DispatcherThread(this);
    QStatus status = m_dispatcher->Start(NULL, NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::Start(): Failed to Start() message dispatcher thread"));
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
    QCC_DbgHLPrintf(("UDPTransport::Stop()"));

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
    QCC_DbgPrintf(("UDPTransport::Stop(): Clear NS callback"));
    IpNameService::Instance().SetCallback(TRANSPORT_UDP, NULL);

    /*
     * Ask any running endpoints to shut down and stop allowing routing to
     * happen through this transport.  The endpoint needs to wake any threads
     * that may be waiting for I/O and arrange for itself to be cleaned up by
     * the maintenance thread.
     */
    QCC_DbgPrintf(("UDPTransport::Stop(): Stop endpoints"));
    m_endpointListLock.Lock(MUTEX_CONTEXT);
    for (set<UDPEndpoint>::iterator i = m_authList.begin(); i != m_authList.end(); ++i) {
        UDPEndpoint ep = *i;
        ep->Stop();
    }
    for (set<UDPEndpoint>::iterator i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        UDPEndpoint ep = *i;
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
    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::Stop(): Stop dispatcher thread"));
    if (m_dispatcher) {
        m_dispatcher->Stop();
    }

    /*
     * Tell the server maintenance loop thread to shut down.  It needs to wait
     * for all of those threads and endpoints to shut down so it doesn't
     * unexpectedly disappear out from underneath them.  We'll wait for it
     * to actually stop when we do a required Join() below.
     */
    QCC_DbgPrintf(("UDPTransport::Stop(): Stop main thread"));
    QStatus status = Thread::Stop();
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::Stop(): Failed to Stop() server thread"));
        DecrementAndFetch(&m_refCount);
        return status;
    }

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
    QCC_DbgHLPrintf(("UDPTransport::Join()"));

    QCC_DbgPrintf(("UDPTransport::Join(): Join and delete dispatcher thread"));
    if (m_dispatcher) {
        m_dispatcher->Join();
        delete m_dispatcher;
        m_dispatcher = NULL;
    }

    QCC_DbgPrintf(("UDPTransport::Join(): Return unused message buffers to ARDP"));
    while (m_workerCommandQueue.empty() == false) {
        WorkerCommandQueueEntry entry = m_workerCommandQueue.front();
        m_workerCommandQueue.pop();
        /*
         * However, the ARDP module will have allocated memory (in some private
         * way) for any messages that are waiting to be routed.  We can't just
         * ignore that situation or we may leak memory.  Give any buffers back
         * to the protocol before leaving.
         */
        if (entry.m_command == WorkerCommandQueueEntry::RECV_CB) {
            m_ardpLock.Lock();
            ARDP_RecvReady(entry.m_handle, entry.m_conn, entry.m_rcv);
            m_ardpLock.Unlock();

            /*
             * If we do something that is going to bug the ARDP protocol, we
             * need to call back into ARDP ASAP to get it moving.  This is done
             * in the main thread, which we need to wake up.
             */
            Alert();
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
    QStatus status = Thread::Join();
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
        m_endpointListLock.Unlock(MUTEX_CONTEXT);
        qcc::Sleep(10);
        m_endpointListLock.Lock(MUTEX_CONTEXT);
    }

    /*
     * The above loop will not terminate until all connecting threads are gone.
     * There are now no threads running in UDP endpoints or in the transport and
     * since we already Join()ed the maintenance thread we can delete all of the
     * endpoints here.
     */

    set<UDPEndpoint>::iterator i;
    while ((i = m_preList.begin()) != m_preList.end()) {
#ifndef NDEBUG
        UDPEndpoint ep = *i;
        QCC_DbgTrace(("UDPTransport::Join(): Erasing endpoint with conn ID == %d. from m_preList", ep->GetConnId()));
#endif
        m_preList.erase(i);
    }

    while ((i = m_authList.begin()) != m_authList.end()) {
#ifndef NDEBUG
        UDPEndpoint ep = *i;
        QCC_DbgTrace(("UDPTransport::Join(): Erasing endpoint with conn ID == %d. from m_authList", ep->GetConnId()));
#endif
        m_authList.erase(i);
        DecrementAndFetch(&m_currAuth);

    }

    while ((i = m_endpointList.begin()) != m_endpointList.end()) {
#ifndef NDEBUG
        UDPEndpoint ep = *i;
        QCC_DbgTrace(("UDPTransport::Join(): Erasing endpoint with conn ID == %d. from m_endpointList", ep->GetConnId()));
#endif
        m_endpointList.erase(i);
        DecrementAndFetch(&m_currConn);
    }
    m_endpointListLock.Unlock(MUTEX_CONTEXT);

    m_stopping = false;
    DecrementAndFetch(&m_refCount);
    return ER_OK;
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
     * transports that are being sought.  The UDP transport provides reliable
     * traffic as understood by the session options, so we only return someting
     * if the traffic type is TRAFFIC_MESSAGES or TRAFFIC_RAW_RELIABLE.  It's
     * not an error if we don't match, we just don't have anything to offer.
     */
    if (opts.traffic != SessionOpts::TRAFFIC_MESSAGES && opts.traffic != SessionOpts::TRAFFIC_RAW_RELIABLE) {
        QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): traffic mismatch"));
        DecrementAndFetch(&m_refCount);
        return ER_OK;
    }

    /*
     * The other session option that we need to filter on is the transport
     * bitfield.  We have no easy way of figuring out if we are a wireless
     * local-area, wireless wide-area, wired local-area or local transport,
     * but we do exist, so we respond if the caller is asking for any of
     * those: cogito ergo some.
     */
    if (!(opts.transports & (TRANSPORT_WLAN | TRANSPORT_WWAN | TRANSPORT_LAN))) {
        QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): transport mismatch"));
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
        QCC_LogError(status, ("UDPTransport::GetListenAddresses(): ns.IfConfig() failed"));
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * The next thing to do is to get the list of interfaces from the config
     * file.  These are required to be formatted in a comma separated list,
     * with '*' being a wildcard indicating that we want to match any interface.
     * If there is no configuration item, we default to something rational.
     */
    QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): GetProperty()"));
    qcc::String interfaces = ConfigDB::GetConfigDB()->GetProperty("ns_interfaces");
    if (interfaces.size() == 0) {
        interfaces = INTERFACES_DEFAULT;
    }

    /*
     * Check for wildcard anywhere in the configuration string.  This trumps
     * anything else that may be there and ensures we get only one copy of
     * the addresses if someone tries to trick us with "*,*".
     */
    bool haveWildcard = false;
    const char*wildcard = "*";
    size_t i = interfaces.find(wildcard);
    if (i != qcc::String::npos) {
        QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): wildcard search"));
        haveWildcard = true;
        interfaces = wildcard;
    }

    /*
     * Walk the comma separated list from the configuration file and and try
     * to mach it up with interfaces actually found in the system.
     */
    while (interfaces.size()) {
        /*
         * We got a comma-separated list, so we need to work our way through
         * the list.  Each entry in the list  may be  an interface name, or a
         * wildcard.
         */
        qcc::String currentInterface;
        size_t i = interfaces.find(",");
        if (i != qcc::String::npos) {
            currentInterface = interfaces.substr(0, i);
            interfaces = interfaces.substr(i + 1, interfaces.size() - i - 1);
        } else {
            currentInterface = interfaces;
            interfaces.clear();
        }

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
                     * transport, and it is advertising at most one port.  It
                     * may be advertising that port over multiple interfaces,
                     * but there is currently just one port being advertised.
                     * If multiple listeners are created, the name service only
                     * advertises the lastly set port.  In the future we may
                     * need to add the ability to advertise different ports on
                     * different interfaces, but the answer is simple now.  Ask
                     * the name service for the one port it is advertising and
                     * that must be the answer.
                     */
                    qcc::String ipv4address;
                    qcc::String ipv6address;
                    uint16_t reliableIpv4Port, reliableIpv6Port, unreliableIpv4Port, unreliableIpv6port;
                    IpNameService::Instance().Enabled(TRANSPORT_UDP,
                                                      reliableIpv4Port, reliableIpv6Port,
                                                      unreliableIpv4Port, unreliableIpv6port);
                    /*
                     * If the port is zero, then it hasn't been set and this
                     * implies that UDPTransport::StartListen hasn't
                     * been called and there is no listener for this transport.
                     * We should only return an address if we have a listener.
                     */
                    if (unreliableIpv4Port) {
                        /*
                         * Now put this information together into a bus address
                         * that the rest of the AllJoyn world can understand.
                         */
                        if (!entries[i].m_addr.empty() && (entries[i].m_family == QCC_AF_INET)) {
                            qcc::String busAddr = "udp:u4addr=" + entries[i].m_addr + ","
                                                  "u4port=" + U32ToString(unreliableIpv4Port) + ","
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
    QCC_DbgPrintf(("UDPTransport::GetListenAddresses(): done"));
    DecrementAndFetch(&m_refCount);
    return ER_OK;
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
    set<UDPEndpoint>::iterator i;

    m_endpointListLock.Lock(MUTEX_CONTEXT);

    /*
     * If there are any endpoints on the preList, move them to the authList.
     */
    m_preListLock.Lock(MUTEX_CONTEXT);

    i = m_preList.begin();
    while (i != m_preList.end()) {
        QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Moving endpoint from m_preList to m_authList"));
        UDPEndpoint ep = *i;
        m_authList.insert(ep);
        m_preList.erase(i);
        i = m_preList.begin();
    }

    m_preListLock.Unlock(MUTEX_CONTEXT);

    /*
     * Run through the list of connections on the authList and cleanup any that
     * are taking too long to authenticate.  These are connections that are in
     * the middle of the three-way handshake.
     */
    bool changeMade = false;
    i = m_authList.begin();
    while (i != m_authList.end()) {
        UDPEndpoint ep = *i;

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
                m_authList.erase(i);
                DecrementAndFetch(&m_currAuth);
                m_endpointList.insert(ep);
                IncrementAndFetch(&m_currConn);
                ep->Stop();
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
    i = m_endpointList.begin();
    while (i != m_endpointList.end()) {
        UDPEndpoint ep = *i;

        /*
         * Get the internal (UDP) endpoint state.  We use this to figure out the
         * correct strategy for getting rid of endpoints at the end of their
         * lifetimes.
         */
        _UDPEndpoint::EndpointState endpointState = ep->GetEpState();

        /*
         * This can be a little tricky since the daemon wants to reference count
         * the endpoints and will call Stop() but not Join() in
         * RemoveSessionRef() for example.  This is because it doesn't want to
         * take possibly unbounded time to wait for the threads in the endpoint
         * to exit.  What that means is that we can arbitrarily find ourselves
         * with endpoints in EP_STOPPING state, and we will have to do the
         * Join() to finish tearing down the endpoint.  That means we have to do
         * a Join(), but we must also be sensitive to blocking for long periods
         * of time since we are called out of the main loop of the transport and
         * in the thread that is driving ARDP.
         */
        if (endpointState == _UDPEndpoint::EP_STOPPING || endpointState == _UDPEndpoint::EP_JOINED) {
            QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d is EP_STOPPING or EP_JOINED", ep->GetConnId()));

            /*
             * If we are in state EP_STOPPING, not surprisingly Stop() must have
             * been called.  What we need to do is to wait until the
             * after-effects of that Stop() have settled before we call Join()
             * so it doesn't block.  When Stop() was called, it Alerted the set
             * of (daemon) threads that may have been waiting on the endpoint
             * and if no previous sudden disconnect happened it called ARDP_Disconnect
             * to start a local disconnect.  We have to wait for the threads to
             * leave and the disconnect to complete before doing the Join().
             */
            ArdpStream* stream = ep->GetStream();
            assert(stream && "UDPTransport::ManageEndpoints(): stream must exist in state EP_STOPPING");

            /*
             * Wait for the threads blocked on the endpoint for writing to exit,
             * pending writes to finish (or be discarded) and the required
             * disconnect callback to happen.
             */
            bool threadSetEmpty = stream->ThreadSetEmpty();
            bool disconnected = stream->GetDisconnected();

            /*
             * We keep an eye on endpoints that seem to be stalled waiting to
             * have the expected things happen.  There's not much we can do if
             * things have gone wrong, but we log errors in the hope that
             * someone notices.
             */
            Timespec tNow;
            GetTimeNow(&tNow);
            Timespec tStop = ep->GetStopTime();
            int32_t tRemaining = tStop + (m_ardpConfig.connectTimeout * m_ardpConfig.connectRetries) - tNow;
            if (tRemaining < 0) {
                QCC_LogError(ER_UDP_ENDPOINT_STALLED, ("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d stalled", ep->GetConnId()));

                if (threadSetEmpty == false) {
                    QCC_LogError(ER_UDP_ENDPOINT_STALLED, ("UDPTransport::ManageEndpoints(): stalled not threadSetEmpty"));
                }

                if (disconnected == false) {
                    QCC_LogError(ER_UDP_ENDPOINT_STALLED, ("UDPTransport::ManageEndpoints(): stalled not disconnected"));
#ifndef NDEBUG
                    ArdpStream* stream = ep->GetStream();
                    if (stream) {
                        bool disc = stream->GetDisconnected();
                        bool discSent = stream->GetDiscSent();
                        ArdpConnRecord* conn = stream->GetConn();
                        bool suddenDisconnect = ep->GetSuddenDisconnect();
                        QCC_LogError(ER_UDP_ENDPOINT_STALLED, ("UDPTransport::ManageEndpoints(): stalled not disconneccted. disc=\"%s\", discSent=\"%s\", conn=%p, suddendisconnect=\"%s\"", disc ? "true" : "false", discSent ? "true" : "false", conn, suddenDisconnect ? "true" : "false"));
                    } else {
                        QCC_LogError(ER_UDP_ENDPOINT_STALLED, ("UDPTransport::ManageEndpoints(): stalled not disconnected. No stream"));
                    }
#endif
                }
            }

            if (threadSetEmpty && disconnected) {
                QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Join()ing stopping endpoint with conn ID == %d.", ep->GetConnId()));

                /*
                 * We now expect that Join() will complete without having to
                 * wait for anything.
                 */
                if (endpointState != _UDPEndpoint::EP_JOINED) {
                    ep->Join();
                    changeMade = true;
                }

                /*
                 * Now, schedule the endpoint exit function to be run if it has
                 * not been run before.  This will ensure that the endpoint is
                 * detached (unregistered) from the daemon (running in another
                 * thread to avoid deadlocks).  At this point we have stopped
                 * and joined the endpoint, but we must wait until the detach
                 * happens, as indicated by EndpointExited() returning true,
                 * before removing our (last) reference to the endpoint below.
                 * When the endpoint Exit() function is actually run by the
                 * dispatcher, it sets the endpoint state to EP_DONE.
                 */
                if (ep->GetRegistered() && ep->GetExitScheduled() == false) {
                    ep->SetExitScheduled();
                    ExitEndpoint(ep->GetConnId());
                    endpointState = ep->GetEpState();
                    changeMade = true;
                }
#ifndef NDEBUG
            } else {
                QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. is not idle", ep->GetConnId()));
#endif
            }
        }

        /*
         * If we find the endpoint in the EP_FAILED or EP_DONE state, the
         * endpoint is ready to go away and there must be no pending operations
         * of any sort.  Given that caveat, we can just pitch it.  When the
         * reference count goes to zero as a result of removing it from the
         * endpoint list it will be destroyed.
         */
        if (endpointState == _UDPEndpoint::EP_FAILED ||
            endpointState == _UDPEndpoint::EP_DONE) {
            if (ep->GetExited()) {
                QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Removing reference for failed or done endpoint with conn ID == %d.",
                                 ep->GetConnId()));
                int32_t refs = ep->IncrementRefs();
                if (refs == 1) {
                    ep->DecrementRefs();
                    QCC_DbgHLPrintf(("UDPTransport::ManageEndpoints(): Endpoint with conn ID == %d. is histoire", ep->GetConnId()));
                    m_endpointList.erase(i);
                    DecrementAndFetch(&m_currConn);
                    i = m_endpointList.upper_bound(ep);
                    changeMade = true;
                    continue;
                }
                ep->DecrementRefs();
            }
        }
        ++i;
    }

    if (changeMade) {
        m_manage = STATE_MANAGE;
        Alert();
    }

    m_endpointListLock.Unlock(MUTEX_CONTEXT);
}

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into the transport.
 */
bool UDPTransport::ArdpAcceptCb(ArdpHandle* handle, qcc::IPAddress ipAddr, uint16_t ipPort, ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpAcceptCb(handle=%p, ipAddr=\"%s\", port=%d., conn=%p, buf =%p, len = %d)",
                  handle, ipAddr.ToString().c_str(), ipPort, conn, buf, len));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    return transport->AcceptCb(handle, ipAddr, ipPort, conn, buf, len, status);
}

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into the transport.
 */
void UDPTransport::ArdpConnectCb(ArdpHandle* handle, ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpConnectCb(handle=%p, conn=%p, passive=%s, buf = %p, len = %d, status=%s)",
                  handle, conn, passive ? "true" : "false", buf, len, QCC_StatusText(status)));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    transport->ConnectCb(handle, conn, passive, buf, len, status);
}

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into the transport.
 */
void UDPTransport::ArdpDisconnectCb(ArdpHandle* handle, ArdpConnRecord* conn, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpDisconnectCb(handle=%p, conn=%p, foreign=%d.)", handle, conn));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    transport->DisconnectCb(handle, conn, status);
}

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into the transport.
 */
void UDPTransport::ArdpRecvCb(ArdpHandle* handle, ArdpConnRecord* conn, ArdpRcvBuf* rcv, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpRecvCb(handle=%p, conn=%p, buf=%p, status=%s)",
                  handle, conn, rcv, QCC_StatusText(status)));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    transport->RecvCb(handle, conn, rcv, status);
}

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into the transport.
 */
void UDPTransport::ArdpSendCb(ArdpHandle* handle, ArdpConnRecord* conn, uint8_t* buf, uint32_t len, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpSendCb(handle=%p, conn=%p, buf=%p, len=%d.)", handle, conn, buf, len));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    transport->SendCb(handle, conn, buf, len, status);
}

/**
 * Callback from the ARDP Protocol.  We just plumb this callback directly into the transport.
 */
void UDPTransport::ArdpSendWindowCb(ArdpHandle* handle, ArdpConnRecord* conn, uint16_t window, QStatus status)
{
    QCC_DbgTrace(("UDPTransport::ArdpSendWindowCb(handle=%p, conn=%p, window=%d.)", handle, conn, window));
    UDPTransport* const transport = static_cast<UDPTransport* const>(ARDP_GetHandleContext(handle));
    transport->SendWindowCb(handle, conn, window, status);
}

/*
 * See the note on connection establishment to really make sense of this.
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
 */
bool UDPTransport::AcceptCb(ArdpHandle* handle, qcc::IPAddress ipAddr, uint16_t ipPort, ArdpConnRecord* conn, uint8_t* buf, uint16_t len, QStatus status)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::AcceptCb(handle=%p, ipAddr=\"%s\", ipPort=%d., conn=%p)", handle, ipAddr.ToString().c_str(), ipPort, conn));

    if (buf == NULL || len == 0) {
        QCC_LogError(ER_UDP_INVALID, ("UDPTransport::AcceptCb(): No BusHello with SYN"));
        DecrementAndFetch(&m_refCount);
        return false;
    }

    /*
     * Here's the diffulty.  It is very common for external threads to call into
     * the UDP transport and take the endpoint list lock to locate an endpoint
     * and then take the ARDP lock to do do something with the network protocol
     * based on the stream in that endpoint.  The lock order here is
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
     * count of currently authenticating connections.  This number is never
     * allowed to exceed a configured value.  The number of authenticating plus
     * acive connections may never exceed a configured value.
     */
    uint32_t currAuth = IncrementAndFetch(&m_currAuth);
    uint32_t currConn = IncrementAndFetch(&m_currConn);

    if (currAuth > m_maxAuth || currAuth + currConn > m_maxConn + 1) {
        QCC_LogError(ER_BUS_CONNECTION_REJECTED, ("UDPTransport::AcceptCb(): No slot for new connection"));
        DecrementAndFetch(&m_currAuth);
        DecrementAndFetch(&m_refCount);
        return false;
    }

    /*
     * The connection is not actually complete yet and there is no corresponding
     * endpoint on the the endpoint list so we can't claim it as existing yet.
     * We do consider the not yet existing endpoint as existing since we need a
     * placeholder for it.  We just have to be careful about the accounting.
     */
    DecrementAndFetch(&m_currConn);
    QCC_DbgPrintf(("UDPTransport::AcceptCb(): Inbound connection accepted"));

    /*
     * We expect to get an org.alljoyn.Bus.BusHello message from the active side
     * in the data.
     */
    Message activeHello(m_bus);
    status = activeHello->LoadBytes(buf, len);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Can't LoadBytes() BusHello Message"));
        DecrementAndFetch(&m_currAuth);
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
        DecrementAndFetch(&m_currAuth);
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
        DecrementAndFetch(&m_currAuth);
        DecrementAndFetch(&m_refCount);
        return false;
    }

    if (activeHello->GetCallSerial() == 0) {
        status = ER_BUS_ESTABLISH_FAILED;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Unexpected zero serial in BusHello Message"));
        DecrementAndFetch(&m_currAuth);
        DecrementAndFetch(&m_refCount);
        return false;
    }

    if (strcmp(activeHello->GetDestination(), org::alljoyn::Bus::WellKnownName) != 0) {
        status = ER_BUS_ESTABLISH_FAILED;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Unexpected destination=\"%s\" in BusHello Message",
                              activeHello->GetDestination()));
        DecrementAndFetch(&m_currAuth);
        DecrementAndFetch(&m_refCount);
        return false;
    }

    if (strcmp(activeHello->GetObjectPath(), org::alljoyn::Bus::ObjectPath) != 0) {
        status = ER_BUS_ESTABLISH_FAILED;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Unexpected object path=\"%s\" in BusHello Message",
                              activeHello->GetObjectPath()));
        DecrementAndFetch(&m_currAuth);
        DecrementAndFetch(&m_refCount);
        return false;
    }

    if (strcmp(activeHello->GetMemberName(), "BusHello") != 0) {
        status = ER_BUS_ESTABLISH_FAILED;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): Unexpected member name=\"%s\" in BusHello Message",
                              activeHello->GetMemberName()));
        DecrementAndFetch(&m_currAuth);
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
        DecrementAndFetch(&m_currAuth);
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
        DecrementAndFetch(&m_currAuth);
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
        DecrementAndFetch(&m_currAuth);
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
    String normSpec = "udp:guid=" + remoteGUID + ",u4addr=" + ipAddr.ToString() + ",u4port=" + U32ToString(ipPort);
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
    udpEp->CreateStream(handle, conn, m_ardpConfig.dataTimeout, m_ardpConfig.dataRetries);
    udpEp->SetHandle(handle);
    udpEp->SetConn(conn);

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
        DecrementAndFetch(&m_currAuth);
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * The Function HelloReply creates and marshals the BusHello reply for
     * the remote side.  Once it is marshaled, there is a buffer associated
     * with the message that contains the on-the-wire version of the
     * messsage.  The ARDP code expects to take responsibility for the
     * buffer since it may need to retransmit it, so we need to copy out the
     * contents of that (small) buffer.
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
    status = ARDP_Accept(handle, conn, ARDP_SEGMAX, ARDP_SEGBMAX, helloReplyBuf, helloReplyBufLen);
    if (status != ER_OK) {
        /*
         * If ARDP_Accept returns an error, most likely it is becuase the underlying
         * SYN + ACK didn't go out.  The contract with ARDP says that if an error
         * happens here, we shouldn't expect an disconnect, so we just don't bother
         * to finish seting up the endpoint.
         *
         * Even though we haven't actually started the endpoint, we call Stop() to
         * set it up for the destruction process to make sure its state is changed
         * to be deletable (when we release our reference to it and the end of the
         * current scope).
         */
        udpEp->Stop();
        delete[] helloReplyBuf;
        helloReplyBuf = NULL;
        QCC_LogError(status, ("UDPTransport::AcceptCb(): ARDP_Accept() failed"));
        DecrementAndFetch(&m_currAuth);
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * Okay, this is now where we need to work around problem number two.  We
     * are going to tell ARDP to proceed with the connection shortly and we need
     * the endpoint we just created to make it onto the list of currently
     * authenticating endpoints, and we need this to happen without taking the
     * endpointListLock.  What we do is to put it on a "pre" authenticating list
     * that is dealt with especially carefully with respect to locks.
     */

    QCC_DbgPrintf(("UDPTransport::AcceptCb(): Taking pre-auth list lock"));
    m_preListLock.Lock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("UDPTransport::AcceptCb(): Adding endpoint with conn ID == %d. to m_preList", udpEp->GetConnId()));
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
 * See the note on connection establishment in the start of this file to make
 * sense of this.
 *
 * If passive is true, and status = ER_OK, this callback indicates that we are
 * getting the final callback as a result of the ARDP_Acknowledge which drove
 * the ACK back from the active opener as the final part of the three-way
 * handshake.  We should see a BusHello reply from the active side to our
 * passive Hello in the data provided.
 *
 * If passive is false, and status = ER_OK, this callback indicates that the
 * passive side has accepted the connection and has returned the SYN + ACK.  We
 * should see a BusHello message and a BusHello reply from the passive side in
 * the data provided.
 *
 * If status != ER_OK, the status should be ER_TIMEOUT indicating that for some
 * reason the three-way handshake did not complete in the expected time/retries.
 */
void UDPTransport::DoConnectCb(ArdpHandle* handle, ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::DoConnectCb(handle=%p, conn=%p)", handle, conn));

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
        QCC_DbgPrintf(("UDPTransport::ManageEndpoints(): Moving endpoint from m_preList to m_authList"));
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
     * Useful to have laying around for debug prints
     */
#ifndef NDEBUG
    uint32_t connId = ARDP_GetConnId(handle, conn);
#endif

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

        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Taking endpoint list lock"));
        m_endpointListLock.Lock(MUTEX_CONTEXT);
        bool haveLock = true;
        set<UDPEndpoint>::iterator i;
        for (i = m_authList.begin(); i != m_authList.end(); ++i) {
            UDPEndpoint ep = *i;
            if (ep->GetConn() == conn && ARDP_GetConnId(m_handle, ep->GetConn()) == ARDP_GetConnId(m_handle, conn)) {
                QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Moving endpoint with conn ID == %d to m_endpointList", connId));
                m_authList.erase(i);
                DecrementAndFetch(&m_currAuth);

#ifndef NDEBUG
                DebugEndpointListCheck(ep);
#endif
                m_endpointList.insert(ep);
                IncrementAndFetch(&m_currConn);

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
                    ArdpStream* stream = ep->GetStream();
                    assert(stream && "UDPTransport::DoConnectCb(): must have a stream at this point");
                    stream->Disconnect(false, ER_UDP_LOCAL_DISCONNECT);
                    ep->Stop();
                    m_endpointListLock.Unlock(MUTEX_CONTEXT);
                    haveLock = false;
                    ARDP_ReleaseConnection(handle, conn);
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
         * good or bad so we need to remember to wake it up.
         *
         * An event used to wake the thread up is provided in the connection,
         * but we also need to make sure that the thread hasn't timed out or
         * been stopped for some other reason, in which case the event will not
         * be valid.
         */
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): active connection callback with conn ID == %d.", connId));
        qcc::Event* event = static_cast<qcc::Event*>(ARDP_GetConnContext(m_handle, conn));
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
            if (j->m_conn == conn && j->m_connId == ARDP_GetConnId(m_handle, conn)) {
                assert(j->m_event == event && "UDPTransport::DoConnectCb(): event != j->m_event");
                eventValid = true;
                break;
            }
        }

        /*
         * There is no thread waiting for the connect to complete.  It must be
         * gone if it has removed its ConnectEntry.  It must have done so with
         * the endpoint list lock taken, so we know it is there if this test
         * passes; and we know it is not there if it does not.
         */
        if (eventValid == false) {
            QCC_LogError(status, ("UDPTransport::DoConnectCb(): No thread waiting for Connect() to complete"));
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            ARDP_ReleaseConnection(handle, conn);
            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * If the connection failed, wake up the thread waiting for completion
         * without creating an endpoint for it.
         */
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::DoConnectCb(): Connect error"));
            event->SetEvent();
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            ARDP_ReleaseConnection(handle, conn);
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
            ARDP_ReleaseConnection(handle, conn);
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
            ARDP_ReleaseConnection(handle, conn);
            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * The dispatcher thread allocated a copy of the buffer from ARDP since
         * ARDP expected its buffer back, so we need to delete this copy.
         */
#ifndef NDEBUG
        CheckSeal(buf + len);
#endif
        delete[] buf;
        buf = NULL;
        len = 0;

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
            ARDP_ReleaseConnection(handle, conn);
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
            ARDP_ReleaseConnection(handle, conn);
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
            ARDP_ReleaseConnection(handle, conn);
            DecrementAndFetch(&m_refCount);
            return;
        }

        /*
         * We expect three arguments in the message: the unique name of the
         * remote side, the remoteGUID and a protocol version.  The high order two bits of the protocol version are the
         * nameTransfer bits that will tell the allJoyn obj how many names to
         * exchange during ExchangeNames.
         */
        size_t numArgs;
        const MsgArg* args;
        helloReply->GetArgs(numArgs, args);
        if (numArgs != 3 || args[0].typeId != ALLJOYN_STRING || args[1].typeId != ALLJOYN_STRING || args[2].typeId != ALLJOYN_UINT32) {
            status = ER_BUS_ESTABLISH_FAILED;
            QCC_LogError(status, ("UDPTransport::DoConnectCb(): Unexpected number or type of arguments in BusHello Reply Message"));
            event->SetEvent();
            m_endpointListLock.Unlock(MUTEX_CONTEXT);
            ARDP_ReleaseConnection(handle, conn);
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
        qcc::IPAddress ipAddr = ARDP_GetIpAddrFromConn(handle, conn);
        uint16_t ipPort = ARDP_GetIpPortFromConn(handle, conn);
        static const bool truthiness = true;
        UDPTransport* ptr = this;
        String normSpec = "udp:guid=" + remoteGUID + ",u4addr=" + ipAddr.ToString() + ",u4port=" + U32ToString(ipPort);
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
        udpEp->SetActive();
        udpEp->SetIpAddr(ipAddr);
        udpEp->SetIpPort(ipPort);
        udpEp->CreateStream(handle, conn, m_ardpConfig.dataTimeout, m_ardpConfig.dataRetries);
        udpEp->SetHandle(handle);
        udpEp->SetConn(conn);

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
        m_endpointList.insert(udpEp);
        IncrementAndFetch(&m_currConn);

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

        /*
         * There is a thread waiting for this process to finish, so we need to
         * wake it up.  The moment we gave up the m_endpointListLock, though,
         * the endpoint management thread can decide to tear down the endpoint
         * and invalidate all of our work.  That means that the event can
         * be torn down out from underneath us.  So we have got to make sure
         * that the endpoint is still there in order to ensure the event is
         * still valid.
         */
        QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Taking endpoint list lock"));
        m_endpointListLock.Lock(MUTEX_CONTEXT);

        /*
         * We know the endpoint is still there since we hold a managed object
         * reference to it.  What might be missing is the thread that started
         * this whole long and involved process.
         */
        eventValid = false;
        for (set<ConnectEntry>::iterator j = m_connectThreads.begin(); j != m_connectThreads.end(); ++j) {
            if (j->m_conn == conn && j->m_connId == ARDP_GetConnId(m_handle, conn)) {
                assert(j->m_event == event && "UDPTransport::DoConnectCb(): event != j->m_event");
                eventValid = true;
                break;
            }
        }

        /*
         * We're all done cranking up the endpoint.  If there's someone waiting,
         * wake them up.  If there's nobody there, stop the endpoint since
         * someone changed their mind.
         */
        if (eventValid) {
            QCC_DbgPrintf(("UDPTransport::DoConnectCb(): Waking thread waiting for endpoint"));
            event->SetEvent();
        } else {
            QCC_DbgPrintf(("UDPTransport::DoConnectCb(): No thread waiting for endpoint"));
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
 * the router.  We dispatch the call to another thread since we absolutely do
 * not want to hold any locks when we call out to the daemon.
 */
void UDPTransport::ExitEndpoint(uint32_t connId)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::ExitEndpoint(connId=%d.)", connId));

    /*
     * If m_dispatcher is NULL, it means we are shutting down and the dispatcher
     * has gone away before the endpoint management thread has actually stopped
     * running.  This is rare, but possible.
     */
    if (m_dispatcher == NULL) {
        QCC_DbgPrintf(("UDPTransport::ExitEndpoint(): m_dispatcher is NULL"));
        DecrementAndFetch(&m_refCount);
        return;
    }

    UDPTransport::WorkerCommandQueueEntry entry;
    entry.m_command = UDPTransport::WorkerCommandQueueEntry::EXIT;
    entry.m_connId = connId;

    QCC_DbgPrintf(("UDPTransport::ExitEndpoint(): sending EXIT request to dispatcher"));
    m_workerCommandQueueLock.Lock(MUTEX_CONTEXT);
    m_workerCommandQueue.push(entry);
    m_workerCommandQueueLock.Unlock(MUTEX_CONTEXT);
    m_dispatcher->Alert();
    DecrementAndFetch(&m_refCount);
}

/*
 * This is the indication from the ARDP protocol that a connection is in the
 * process of being formed.  We want to spend as little time as possible here
 * (and avoid deadlocks as much as possible here) so we just immediately ask the
 * transport dispatcher to do something with this message and return.  This case
 * is unlike the others because there may not be an endpoint to demux to yet.
 */
void UDPTransport::ConnectCb(ArdpHandle* handle, ArdpConnRecord* conn, bool passive, uint8_t* buf, uint16_t len, QStatus status)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgHLPrintf(("UDPTransport::ConnectCb(handle=%p, conn=%p, passive=%d., buf=%p, len=%d., status=\"%s\")",
                     handle, conn, passive, buf, len, QCC_StatusText(status)));

    /*
     * If m_dispatcher is NULL, it means we are shutting down and the dispatcher
     * has gone away before the endpoint management thread has actually stopped
     * running.  This is rare, but possible.
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
 * This is the indication from the ARDP protocol that a connection has been
 * disconnected.  We want to spend as little time as possible here (and avoid
 * deadlocks as much as possible here) so we just immediately ask the transport
 * dispatcher to do something with this message and return.
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
 * This is the indication from the ARDP protocol that we have received bytes.
 * We want to spend as little time as possible here (and avoid deadlocks as much
 * as possible here) so we just immediately ask the transport dispatcher to do
 * something with this message and return.  The dispatcher will figure out which
 * endpoint this callback is destined for and demultiplex it accordingly.
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

        QCC_DbgPrintf(("UDPTransport::RecvCb(): ARDP_RecvReady()"));
        m_ardpLock.Lock();
        ARDP_RecvReady(handle, conn, rcv);
        m_ardpLock.Unlock();

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
 * This is the indication from the ARDP protocol that we have (usually)
 * successfully sent bytes.  We want to spend as little time as possible here
 * (and avoid deadlocks as much as possible here) so we just immediately ask the
 * transport dispatcher to do something with this message and return.  The
 * dispatcher will figure out which endpoint this callback is destined for and
 * demultiplex it accordingly.
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
 * This is an indication from the ARDP Procotol that the send window has changed.
 */
void UDPTransport::SendWindowCb(ArdpHandle* handle, ArdpConnRecord* conn, uint16_t window, QStatus status)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::SendWindowCb(handle=%p, conn=%p, window=%d.)", handle, conn, window));
    QCC_DbgPrintf(("UDPTransport::SendWindowCb(): callback from conn ID == %d", ARDP_GetConnId(handle, conn)));
    DecrementAndFetch(&m_refCount);
}

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
     * Events driving the main loop execution below.  Always listen for the
     * (thread) stop event firing.  Create a timer event that the ARDP protocol
     * will borrow for its timers -- it never pops unless ARDP says to, so it
     * starts waiting forever.
     */
    vector<Event*> checkEvents, signaledEvents;
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
    while (!IsStopping()) {

        /*
         * Each time through the loop we need to wait on the stop event and all
         * of the SocketFds of the addresses and ports we are listening on.  We
         * expect the list of FDs to change rarely, so we want to spend most of
         * our time just driving the ARDP protocol and moving bits.  We only
         * redo the list if we notice the state changed from STATE_RELOADED.
         *
         * Instead of trying to figure out the delta, we just restart the whole
         * shebang.
         */
        m_listenFdsLock.Lock(MUTEX_CONTEXT);
        if (m_reload != STATE_RELOADED) {
            QCC_DbgPrintf(("UDPTransport::Run(): Not STATE_RELOADED.  Deleting events"));
            for (vector<Event*>::iterator i = checkEvents.begin(); i != checkEvents.end(); ++i) {
                if (*i != &stopEvent && *i != &ardpTimerEvent && *i != &maintenanceTimerEvent) {
                    delete *i;
                }
            }

            checkEvents.clear();

            QCC_DbgPrintf(("UDPTransport::Run(): Not STATE_RELOADED. Creating events"));
            checkEvents.push_back(&stopEvent);
            checkEvents.push_back(&ardpTimerEvent);
            checkEvents.push_back(&maintenanceTimerEvent);

            QCC_DbgPrintf(("UDPTransport::Run(): Not STATE_RELOADED. Creating socket events"));
            for (list<pair<qcc::String, SocketFd> >::const_iterator i = m_listenFds.begin(); i != m_listenFds.end(); ++i) {
                QCC_DbgPrintf(("UDPTransport::Run(): Not STATE_RELOADED. Creating event for socket %d", i->second));
                checkEvents.push_back(new Event(i->second, Event::IO_READ, false));
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
         * We have our list of events, so now wait for something to happen on
         * that list.  The number of events in checkEvents should be 3 + the
         * number of sockets listened (stopEvent, ardpTimerEvent, maintenanceTimerEvent and sockets).
         */
        signaledEvents.clear();

        status = Event::Wait(checkEvents, signaledEvents);
        if (status == ER_TIMEOUT) {
//            QCC_LogError(status, ("UDPTransport::Run(): Catching Windows returning ER_TIMEOUT from Event::Wait()"));
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
             * Determine if this was a socket event (the socket became ready) or
             * if it was a timer event.  If we are calling ARDP because
             * something new came in, let it know by setting a flag.
             *
             * TODO: If we are passing the socket FD in every time,
             * why do we have it stashed in the handle or conn?
             */
            bool socketReady = (*i != &ardpTimerEvent && *i != &maintenanceTimerEvent && *i != &stopEvent);
            uint32_t ms;
            m_ardpLock.Lock();
            ARDP_Run(m_handle, socketReady ? (*i)->GetFD() : -1, socketReady, &ms);
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
     * enabled.  It must be enabled either because we are advertising
     * (actively or quietly) or we are discovering.  If we are
     * advertising(actively or quietly) or discovering, then there
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
     * If we think we are advertising, we'd better have an entry in
     * the advertisements list to advertise, and there must be
     * listeners waiting for inbound connections as a result of those
     * advertisements.  If we are advertising the name service had
     * better be enabled.
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
                assert(m_listenPort);
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
                IpNameService::Instance().Enable(TRANSPORT_UDP, 0, 0, m_listenPort, 0, true, false, false, false);
                m_isNsEnabled = true;
            }
        }
    }

    if (!m_isListening) {
        QCC_LogError(ER_UDP_NO_LISTENER, ("UDPTransport::EnableAdvertisementInstance(): Advertise with no UDP listeners"));
        return;
    }

    /*
     * We think we're ready to send the advertisement.  Are we really?
     */
    assert(m_isListening);
    assert(m_listenPort);
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
        IpNameService::Instance().Enable(TRANSPORT_UDP, 0, 0, m_listenPort, 0, false, false, false, false);
        m_isNsEnabled = false;

        /*
         * If we had the name service running, we must have had listeners
         * waiting for connections due to the name service.  We need to stop
         * them all now, but only if we are not running on a Windows box.
         * Windows needs the listeners running at all times since it uses
         * UDP for the client to daemon connections.
         */
        for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
            DoStopListen(*i);
        }

        m_isListening = false;
        m_listenPort = 0;
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
         */
        if (!m_isListening) {
            for (list<qcc::String>::iterator i = m_listening.begin(); i != m_listening.end(); ++i) {
                QStatus status = DoStartListen(*i);
                if (ER_OK != status) {
                    continue;
                }
                assert(m_listenPort);
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
                IpNameService::Instance().Enable(TRANSPORT_UDP,  0, 0, m_listenPort, 0, true, false, false, false);
                m_isNsEnabled = true;
            }
        }
    }

    if (!m_isListening) {
        QCC_LogError(ER_UDP_NO_LISTENER, ("UDPTransport::EnableDiscoveryInstance(): Discover with no UDP listeners"));
        DecrementAndFetch(&m_refCount);
        return;
    }

    /*
     * We think we're ready to send the FindAdvertisement.  Are we really?
     */
    assert(m_isListening);
    assert(m_listenPort);
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

    if (m_isListening && m_listenPort && m_isNsEnabled && IpNameService::Instance().Started()) {
        QStatus status = IpNameService::Instance().CancelFindAdvertisement(TRANSPORT_UDP, listenRequest.m_requestParam, listenRequest.m_requestTransportMask);
        if (status != ER_OK) {
            QCC_LogError(status, ("TCPTransport::DisableDiscoveryInstance(): Failed to cancel discovery with \"%s\"", listenRequest.m_requestParam.c_str()));
        }
    }

    /*
     * If it turns out that this was the last discovery operation on
     * our list, we need to think about disabling our listeners and turning off
     * the name service.  We only to this if there are no advertisements in
     * progress.
     */
    if (isEmpty && !m_isAdvertising) {

        IpNameService::Instance().Enable(TRANSPORT_UDP,  0, 0, m_listenPort, 0, false, false, false, false);
        m_isNsEnabled = false;

        /*
         * If we had the name service running, we must have had listeners
         * waiting for connections due to the name service.  We need to stop
         * them all now, but only if we are not running on a Windows box.
         * Windows needs the listeners running at all times since it uses
         * UDP for the client to daemon connections.
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
     *     "udp:u4addr=0.0.0.0,u4port=9955"
     *
     * That's all.  We still allow "addr=0.0.0.0,port=9955,family=ipv4" but
     * treat addr as synonomous with u4addr, port as synonomous with u4port and
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
     * (UDP).  We therefore must provide a u4addr either from explicit keys or
     * generated from the defaults.
     */
    iter = argMap.find("u4addr");
    if (iter == argMap.end()) {
        /*
         * We have no value associated with an "u4addr" key.  Do we have an
         * "addr" which would be synonymous?  If so, save it as a u4addr,
         * erase it and point back to the new u4addr.
         */
        iter = argMap.find("addr");
        if (iter != argMap.end()) {
            argMap["u4addr"] = iter->second;
            argMap.erase(iter);
        }

        iter = argMap.find("u4addr");
    }

    /*
     * Now, deal with the u4addr, possibly replaced by addr.
     */
    if (iter != argMap.end()) {
        /*
         * We have a value associated with the "u4addr" key.  Run it through a
         * conversion function to make sure it's a valid value and to get into
         * in a standard representation.
         */
        IPAddress addr;
        status = addr.SetAddress(iter->second, false);
        if (status == ER_OK) {
            /*
             * The u4addr had better be an IPv4 address, otherwise we bail.
             */
            if (!addr.IsIPv4()) {
                QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                             ("UDPTransport::NormalizeListenSpec(): The u4addr \"%s\" is not a legal IPv4 address",
                              iter->second.c_str()));
                return ER_BUS_BAD_TRANSPORT_ARGS;
            }
            iter->second = addr.ToString();
            outSpec.append("u4addr=" + addr.ToString());
        } else {
            QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                         ("UDPTransport::NormalizeListenSpec(): The u4addr \"%s\" is not a legal IPv4 address",
                          iter->second.c_str()));
            return ER_BUS_BAD_TRANSPORT_ARGS;
        }
    } else {
        /*
         * We have no value associated with an "u4addr" key.  Use the default
         * IPv4 listen address for the outspec and create a new key for the
         * map.
         */
        outSpec.append("u4addr=" + qcc::String(ADDR4_DEFAULT));
        argMap["u4addr"] = ADDR4_DEFAULT;
    }

    /*
     * The UDP transport must absolutely support the IPv4 "unreliable" mechanism
     * (UDP).  We therefore must provide a u4port either from explicit keys or
     * generated from the defaults.
     */
    iter = argMap.find("u4port");
    if (iter == argMap.end()) {
        /*
         * We have no value associated with a "u4port" key.  Do we have a
         * "port" which would be synonymous?  If so, save it as a u4port,
         * erase it and point back to the new u4port.
         */
        iter = argMap.find("port");
        if (iter != argMap.end()) {
            argMap["u4port"] = iter->second;
            argMap.erase(iter);
        }

        iter = argMap.find("u4port");
    }

    /*
     * Now, deal with the u4port, possibly replaced by port.
     */
    if (iter != argMap.end()) {
        /*
         * We have a value associated with the "u4port" key.  Run it through a
         * conversion function to make sure it's a valid value.  We put it into
         * a 32 bit int to make sure it will actually fit into a 16-bit port
         * number.
         */
        uint32_t port = StringToU32(iter->second);
        if (port <= 0xffff) {
            outSpec.append(",u4port=" + iter->second);
        } else {
            QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                         ("UDPTransport::NormalizeListenSpec(): The key \"u4port\" has a bad value \"%s\"", iter->second.c_str()));
            return ER_BUS_BAD_TRANSPORT_ARGS;
        }
    } else {
        /*
         * We have no value associated with an "u4port" key.  Use the default
         * IPv4 listen port for the outspec and create a new key for the map.
         */
        qcc::String portString = U32ToString(PORT_DEFAULT);
        outSpec += ",u4port=" + portString;
        argMap["u4port"] = portString;
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
    map<qcc::String, qcc::String>::iterator i = argMap.find("u4addr");
    assert(i != argMap.end());
    if ((i->second == ADDR4_DEFAULT)) {
        QCC_LogError(ER_BUS_BAD_TRANSPORT_ARGS,
                     ("UDPTransport::NormalizeTransportSpec(): The u4addr may not be the default address"));
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
    QCC_DbgHLPrintf(("UDPTransport::Connect(connectSpec=%s, opts=%p, newEp-%p)", connectSpec, &opts, &newEp));

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
    assert(argMap.find("u4addr") != argMap.end() && "UDPTransport::Connect(): u4addr not present in argMap");
    assert(argMap.find("u4port") != argMap.end() && "UDPTransport::Connect(): u4port not present in argMap");

    IPAddress ipAddr(argMap.find("u4addr")->second);
    uint16_t ipPort = StringToU32(argMap["u4port"]);

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
     * looks like "r4addr=0.0.0.0,port=y".  If we detect this kind of connectSpec
     * we have to look at the currently up interfaces and see if any of them
     * match the address provided in the connectSpec.  If so, we are attempting
     * to connect to ourself and we must fail that request.
     */
    char anyspec[64];
    snprintf(anyspec, sizeof(anyspec), "%s:u4addr=0.0.0.0,u4port=%u", GetTransportName(), ipPort);

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
     * either explicitly or via the INADDR_ANY address.
     */
    QCC_DbgPrintf(("UDPTransport::Connect(): Checking for connection to self"));
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
         * to look to the currently UP interfaces to decide if this call is bogus
         * or not.  Set a flag to remind us.
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
        QCC_DbgPrintf(("UDPTransport::Connect(): Checking for implicit connection to self"));

        /*
         * Loop through the network interface entries looking for an UP
         * interface that has the same IP address as the one we're trying to
         * connect to.  We know any match on the address will be a hit since we
         * matched the port during the listener check above.  Since we have a
         * listener listening on *any* UP interface on the specified port, a
         * match on the interface address with the connect address is a hit.
         */
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
     * Now, we have to figure out which of the current sockets we are listening
     * on corresponds to the network of the address in the connect spec in order
     * to send the connect request out on the right network.
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
         * Find the corresponding interface information in the IfConfig entries.
         * We need the network mask from that entry so we can see if
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

    QCC_DbgPrintf(("UDPTransport::Connect(): Compose BusHello"));
    Message hello(m_bus);
    status = hello->HelloMessage(true, m_bus.GetInternal().AllowRemoteMessages(), opts.nameTransfer);
    if (status != ER_OK) {
        status = ER_UDP_BUSHELLO;
        QCC_LogError(status, ("UDPTransport::Connect(): Can't make a BusHello Message"));
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * The Function HelloMessage creates and marshals the BusHello Message for
     * the remote side.  Once it is marshaled, there is a buffer associated with
     * the message that contains the on-the-wire version of the messsage.  The
     * ARDP code expects to take responsibility for the buffer since it may need
     * to retransmit it, so we need to copy out the contents of that (small)
     * buffer.
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
    status = ARDP_Connect(m_handle, sock, ipAddr, ipPort, ARDP_SEGMAX, ARDP_SEGBMAX, &conn, buf, buflen, &event);
    if (status != ER_OK) {
        assert(conn == NULL && "UDPTransport::Connect(): ARDP_Connect() failed but returned ArdpConnRecord");
        QCC_LogError(status, ("UDPTransport::Connect(): ARDP_Connect() failed"));
        m_ardpLock.Unlock();
        m_endpointListLock.Unlock(MUTEX_CONTEXT);
        DecrementAndFetch(&m_refCount);
        return status;
    }

    Thread* thread = GetThread();
    QCC_DbgPrintf(("UDPTransport::Connect(): Add thread=%p to m_connectThreads", thread));
    assert(thread && "UDPTransport::Connect(): GetThread() returns NULL");
    ConnectEntry entry(thread, conn, ARDP_GetConnId(m_handle, conn), &event);

    /*
     * Now, we can safely insert the entry into the set.  We're danger close to
     * a horrible fate unless we get it off that list before <event> goes out of
     * scope.
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
     */
    QCC_DbgPrintf(("UDPTransport::Connect(): Finding endpoint with conn ID = %d. in m_endpointList", ARDP_GetConnId(m_handle, conn)));
    set<UDPEndpoint>::iterator i;
    for (i = m_endpointList.begin(); i != m_endpointList.end(); ++i) {
        UDPEndpoint ep = *i;
        if (ep->GetConn() == conn) {
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
    QCC_DbgTrace(("UDPTransport::StartListen()"));

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

    QCC_DbgPrintf(("UDPTransport::StartListen(): u4addr = \"%s\", u4port = \"%s\"",
                   argMap["u4addr"].c_str(), argMap["u4port"].c_str()));

    /*
     * The daemon code is in a state where it lags in functionality a bit with
     * respect to the common code.  Common supports the use of IPv6 addresses
     * but the name service is not quite ready for prime time.  Until the name
     * service can properly distinguish between various cases, we fail any
     * request to listen on an IPv6 address.
     */
    IPAddress ipAddress;
    status = ipAddress.SetAddress(argMap["u4addr"].c_str());
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::StartListen(): Unable to SetAddress(\"%s\")", argMap["u4addr"].c_str()));
        DecrementAndFetch(&m_refCount);
        return status;
    }

    if (ipAddress.IsIPv6()) {
        status = ER_INVALID_ADDRESS;
        QCC_LogError(status, ("UDPTransport::StartListen(): IPv6 address (\"%s\") in \"u4addr\" not allowed", argMap["u4addr"].c_str()));
        DecrementAndFetch(&m_refCount);
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
            DecrementAndFetch(&m_refCount);
            return ER_BUS_ALREADY_LISTENING;
        }
    }
    m_listenSpecsLock.Unlock(MUTEX_CONTEXT);

    QueueStartListen(normSpec);
    DecrementAndFetch(&m_refCount);
    return ER_OK;
}

void UDPTransport::QueueStartListen(qcc::String& normSpec)
{
    IncrementAndFetch(&m_refCount);
    QCC_DbgTrace(("UDPTransport::QueueStartListen()"));

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
    QCC_DbgPrintf(("UDPTransport::DoStartListen()"));

    /*
     * Since the name service is created before the server accept thread is spun
     * up, and stopped when it is stopped, we must have a started name service or
     * someone isn't playing by the rules; so an assert is appropriate here.
     */
    assert(IpNameService::Instance().Started() && "UDPTransport::DoStartListen(): IpNameService not started");

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

    QCC_DbgPrintf(("UDPTransport::DoStartListen(): u4addr = \"%s\", u4port = \"%s\"",
                   argMap["u4addr"].c_str(), argMap["u4port"].c_str()));

    /*
     * Figure out what local address and port the listener should use.
     */
    IPAddress listenAddr(argMap["u4addr"]);
    uint16_t listenPort = StringToU32(argMap["u4port"]);
    bool ephemeralPort = (listenPort == 0);

    /*
     * If we're going to listen on an address, we are going to listen on a
     * corresponding network interface.  We need to convince the name service to
     * send advertisements out over that interface, or nobody will know to
     * connect to the listening daemon.  The expected use case is that the
     * daemon does exactly one StartListen() which listens to INADDR_ANY
     * (listens for inbound connections over any interface) and the name service
     * is controlled by a separate configuration item that selects which
     * interfaces are used in discovery.  Since IP addresses in a mobile
     * environment are dynamic, listening on the ANY address is the only option
     * that really makes sense, and this is the only case in which the current
     * implementation will really work.
     *
     * So, we need to get the configuration item telling us which network
     * interfaces we should run the name service over.  The item can specify an
     * IP address, in which case the name service waits until that particular
     * address comes up and then uses the corresponding net device if it is
     * multicast-capable.  The item can also specify an interface name.  In this
     * case the name service waits until it finds the interface IFF_UP and
     * multicast capable with an assigned IP address and then starts using the
     * interface.  If the configuration item contains "*" (the wildcard) it is
     * interpreted as meaning all multicast-capable interfaces.  If the
     * configuration item is empty (not assigned in the configuration database)
     * it defaults to "*".
     */
    qcc::String interfaces = ConfigDB::GetConfigDB()->GetProperty("ns_interfaces");
    if (interfaces.size() == 0) {
        interfaces = INTERFACES_DEFAULT;
    }

    while (interfaces.size()) {
        qcc::String currentInterface;
        size_t i = interfaces.find(",");
        if (i != qcc::String::npos) {
            currentInterface = interfaces.substr(0, i);
            interfaces = interfaces.substr(i + 1, interfaces.size() - i - 1);
        } else {
            currentInterface = interfaces;
            interfaces.clear();
        }

        /*
         * We have been given a listenSpec that provides an r4addr and an r4port
         * in the parameters to this method.  We are expected to listen on that
         * address and port for inbound connections.  We have a separate list of
         * network interface names that we are walking through that tell us
         * which interfaces the name service should advertise and discover over.
         * We always listen on the listen address and port, and we always
         * respect the interface names given for the name service.
         *
         * We can either be given a listenAddr of INADDR_ANY or a specific
         * address.  If given INADDR_ANY this means that the TCP Transport will
         * listen for inbound connections on any currently IFF_UP interface or
         * any interface that may come IFF_UP in the future.  If given a
         * specific IP address, we must only listen for connections on that
         * address.
         *
         * We are also given a list of interface names in the ns_interfaces
         * property.  These tell the name service which interfaces to discover and
         * advertise over.  There are four basic situations we will encounter:
         *
         *     listen   ns_interface  Action
         *     -------  ------------  -----------------------------------------
         * 1.  0.0.0.0       *        Listen on 0.0.0.0 and advertise/discover
         *                            over '*'.  This is the default case where
         *                            the system listens on all interfaces and
         *                            advertises / discovers on all interfaces.
         *                            This is the "speak alljoyn over all of
         *                            your interfaces" situation.
         *
         * 2.  a.b.c.d       *        Listen only on a.b.c.d, but advertise and
         *                            discover over '*'.  In this case, the TCP
         *                            transport is told to listen on a specific
         *                            address, but the name service is told to
         *                            advertise and discover over all network
         *                            interfaces.  This may not be exactly what
         *                            is desired, but it may be.  We do what we
         *                            are told.  Note that by doing this, one
         *                            is limiting the number of daemons that can
         *                            be run on a host using the same address
         *                            and port to one.  Other daemons configured
         *                            this way must select another port.
         *
         * 3.  0.0.0.0   'specific'   Listen on INADDR_ANY and so respond to all
         *                            inbound connections.  Only advertise and
         *                            discover over a specific named network
         *                            interface.  This is how we expect people
         *                            to limit AllJoyn to talking only over a
         *                            specific interface.  This allows that
         *                            interface to change IP addresses on the
         *                            fly.
         *
         * 4.  a.b.c.d   'specific'   Listen on a specified address and so
         *                            only respond to inbound connections on
         *                            that interface.  Only advertise and
         *                            discover over a specific named network
         *                            interface (presumably the same interface
         *                            but we don't require it to be set up yet).
         *                            This allows administrators to to limit
         *                            AllJoyn to both listening and advertising
         *                            and discovering only over a specific
         *                            network interface.  This requires that
         *                            the IP address of the network interface
         *                            named 'specific' to be known a-priori.
         *
         * Note that the string 'specific' from the ns_interfaces configuration
         * can also refer to a specific IP address and is not limited to only
         * network interface names.  Thus, if one has a statically configured
         * network address, one could specify both listening and advertisement
         * using IP addresses:
         *
         * 5.  a.b.c.d   'a.b.c.d'    Listen on a specified address and so
         *                            only respond to inbound connections on
         *                            that interface.  Only advertise and
         *                            discover over a specific IP address
         *                            (presumably the same address but we
         *                            don't require it to be set up yet).
         *                            Note that there may, in fact, be a list
         *                            of specific IP addresses, but there can
         *                            be only one listen addresses.  This is
         *                            a degenerate version of the a.b.c.d '*'
         *                            case from above since you can contemplate
         *                            adding all IP addresses in the list.
         *
         * This is much harder to describe than to implement; but the upshot is
         * that we listen on whatever IP address comes in with the listenSpec
         * and we enable the name service on whatever interface name or address
         * that comes in in ns_interfaces.  It is up to the person doing the
         * configuration to understand what he or she is trying to do and the
         * impact of choosing those values.
         *
         * So, the first order of business is to determine whether or not the
         * current ns_interfaces item is an IP address or is a network interface
         * name.  If setting an IPAddress with the current item works, it is an
         * IP Address, otherwise we assume it must be a network interface.  Once
         * we know which overloaded NS function to call, just do it.
         */
        IPAddress currentAddress;
        status = currentAddress.SetAddress(currentInterface, false);
        if (status == ER_OK) {
            status = IpNameService::Instance().OpenInterface(TRANSPORT_TCP, currentAddress);
        } else {
            status = IpNameService::Instance().OpenInterface(TRANSPORT_TCP, currentInterface);
        }

        if (status != ER_OK) {
            QCC_LogError(status, ("TCPTransport::DoStartListen(): OpenInterface() failed for %s", currentInterface.c_str()));
        }
    }

    /*
     * We have the name service work out of the way, so we can now create the
     * UDP listener sockets and set SO_REUSEADDR/SO_REUSEPORT so we don't have
     * to wait for four minutes to relaunch the daemon if it crashes.
     */
    QCC_DbgPrintf(("UDPTransport::DoStartListen(): Setting up socket"));
    SocketFd listenFd = -1;
    status = Socket(QCC_AF_INET, QCC_SOCK_DGRAM, listenFd);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::DoStartListen(): Socket() failed"));
        DecrementAndFetch(&m_refCount);
        return status;
    }

    QCC_DbgPrintf(("UDPTransport::DoStartListen(): listenFd=%d.", listenFd));;

    /*
     * ARDP expects us to use select and non-blocking sockets.
     */
    QCC_DbgPrintf(("UDPTransport::DoStartListen(): SetBlocking(listenFd=%d, false)", listenFd));;
    status = qcc::SetBlocking(listenFd, false);
    if (status != ER_OK) {
        QCC_LogError(status, ("UDPTransport::DoStartListen(): SetBlocking() failed"));
        qcc::Close(listenFd);
        DecrementAndFetch(&m_refCount);
        return status;
    }

    /*
     * If ephemeralPort is set, it means that the listen spec did not provide a
     * specific port and wants us to choose one.  In this case, we first try the
     * default port; but it that port is already taken in the system, we let the
     * system assign a new one from the ephemeral port range.
     */
    if (ephemeralPort) {
        QCC_DbgPrintf(("UDPTransport::DoStartListen(): ephemeralPort"));;
        listenPort = PORT_DEFAULT;
        QCC_DbgPrintf(("UDPTransport::DoStartListen(): Bind(listenFd=%d., listenAddr=\"%s\", listenPort=%d.)",
                       listenFd, listenAddr.ToString().c_str(), listenPort));;
        status = Bind(listenFd, listenAddr, listenPort);
        if (status != ER_OK) {
            listenPort = 0;
            QCC_DbgPrintf(("UDPTransport::DoStartListen(): Bind() failed.  Bind(listenFd=%d., listenAddr=\"%s\", listenPort=%d.)",
                           listenFd, listenAddr.ToString().c_str(), listenPort));;
            status = Bind(listenFd, listenAddr, listenPort);
        }
    } else {
        QCC_DbgPrintf(("UDPTransport::DoStartListen(): Bind(listenFd=%d., listenAddr=\"%s\", listenPort=%d.)",
                       listenFd, listenAddr.ToString().c_str(), listenPort));;
        status = Bind(listenFd, listenAddr, listenPort);
    }

    if (status == ER_OK) {
        /*
         * If the port was not set (or set to zero) then we may have bound an ephemeral port. If
         * so call GetLocalAddress() to update the connect spec with the port allocated by bind.
         */
        if (ephemeralPort) {
            qcc::GetLocalAddress(listenFd, listenAddr, listenPort);
            normSpec = "udp:u4addr=" + argMap["u4addr"] + ",u4port=" + U32ToString(listenPort);
            QCC_DbgPrintf(("UDPTransport::DoStartListen(): ephemeralPort.  New normSpec=\"%s\"", normSpec.c_str()));
        }
    } else {
        QCC_LogError(status, ("UDPTransport::DoStartListen(): Failed to bind to %s/%d", listenAddr.ToString().c_str(), listenPort));
    }

    /*
     * Okay, we're ready to receive datagrams on this socket now.  Tell the
     * maintenance thread that something happened here and it needs to reload
     * its FDs.
     */
    QCC_DbgPrintf(("UDPTransport::DoStartListen(): listenFds.push_back(normSpec=\"%s\", listenFd=%d)", normSpec.c_str(), listenFd));

    m_listenFdsLock.Lock(MUTEX_CONTEXT);
    m_listenFds.push_back(pair<qcc::String, SocketFd>(normSpec, listenFd));
    m_reload = STATE_RELOADING;
    m_listenFdsLock.Unlock(MUTEX_CONTEXT);

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
    QCC_DbgPrintf(("UDPTransport::DoStartListen(): IpNameService::Instance().Enable()"));
    m_listenPort = listenPort;
    IpNameService::Instance().Enable(TRANSPORT_UDP, 0, 0, listenPort, 0, false, false, true, false);
    m_isNsEnabled = true;

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
        QCC_DbgPrintf(("UDPTransport::DoStartListen(): Advertise m_routerName=\"%s\"", m_routerName.c_str()));
        bool isFirst;
        NewAdvertiseOp(ENABLE_ADVERTISEMENT, m_routerName, isFirst);
        QStatus status = IpNameService::Instance().AdvertiseName(TRANSPORT_UDP, m_routerName, true, TRANSPORT_UDP);
        if (status != ER_OK) {
            QCC_LogError(status, ("UDPTransport::DoStartListen(): Failed to AdvertiseNameQuietly \"%s\"", m_routerName.c_str()));
        }
        m_isAdvertising = true;
    }
    m_isListening = true;

    /*
     * Signal the (probably) waiting run thread so it will wake up and add this
     * new socket to its list of sockets it is waiting for connections on.
     */
    if (status == ER_OK) {
        QCC_DbgPrintf(("UDPTransport::DoStartListen(): Alert()"));
        Alert();
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
    qcc::SocketFd stopFd = -1;
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
        first = m_advertising.empty();
        m_discovering.push_back(namePrefix);
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
        m_advertising.push_back(name);
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
                                        std::vector<qcc::String>& nameList, uint8_t timer)
{
//  Makes lots of noise!
    //QCC_DbgTrace(("UDPTransport::FoundCallback::Found(): busAddr = \"%s\" nameList %d", busAddr.c_str(), nameList.size()));

    qcc::String u4addr("u4addr=");
    qcc::String u4port("u4port=");
    qcc::String comma(",");

    size_t i = busAddr.find(u4addr);
    if (i == qcc::String::npos) {
        QCC_DbgPrintf(("UDPTransport::FoundCallback::Found(): No u4addr in busaddr."));
        return;
    }
    i += u4addr.size();

    size_t j = busAddr.find(comma, i);
    if (j == qcc::String::npos) {
        QCC_DbgPrintf(("UDPTransport::FoundCallback::Found(): No comma after u4addr in busaddr."));
        return;
    }

    size_t k = busAddr.find(u4port);
    if (k == qcc::String::npos) {
        QCC_DbgPrintf(("UDPTransport::FoundCallback::Found(): No u4port in busaddr."));
        return;
    }
    k += u4port.size();

    /*
     * "u4addr=192.168.1.1,u4port=9955"
     *         ^          ^       ^
     *         i          j       k
     */
    qcc::String newBusAddr = qcc::String("udp:guid=") + guid + "," + u4addr + busAddr.substr(i, j - i) + "," + u4port + busAddr.substr(k);

//    QCC_DbgPrintf(("UDPTransport::FoundCallback::Found(): newBusAddr = \"%s\".", newBusAddr.c_str()));

    if (m_listener) {
        m_listener->FoundNames(newBusAddr, guid, TRANSPORT_UDP, &nameList, timer);
    }
}

} // namespace ajn
