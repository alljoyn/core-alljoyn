/**
 * @file
 * The lightweight name service implementation
 */

/******************************************************************************
 * Copyright (c) 2010-2014, AllSeen Alliance. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <algorithm>
#include <iterator>

#if defined(QCC_OS_GROUP_WINDOWS)
#define close closesocket
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <climits>
#endif

#include <qcc/Debug.h>
#include <qcc/Event.h>
#include <qcc/atomic.h>
#include <qcc/Socket.h>
#include <qcc/SocketTypes.h>
#include <qcc/IfConfig.h>
#include <qcc/time.h>
#include <qcc/StringUtil.h>
#include <qcc/GUID.h>
#include <qcc/Event.h>

#include "BusUtil.h"
#include "ConfigDB.h"
#include "IpNameServiceImpl.h"

#define QCC_MODULE "IPNS"

using namespace std;
using namespace qcc;

namespace ajn {
int32_t INCREMENTAL_PACKET_ID;
#define RESET_SCHEDULE_ALERTCODE  1
#define PACKET_TIME_ACCURACY_MS 20

// ============================================================================
// Long sidebar on why this looks so complicated:
//
// In order to understand all of the trouble we are going to go through below,
// it is helpful to thoroughly understand what is done on our platforms in the
// presence of multicast.  This is long reading, but worthwhile reading if you
// are trying to understand what is going on.  I don't know of anywhere you
// can find all of this written in one place.
//
// The first thing to grok is that all platforms are implemented differently.
// Windows and Linux use IGMP to enable and disable multicast, and use other
// multicast-related socket calls to do the fine-grained control.  Android
// doesn't bother to compile its kernel with CONFIG_IP_MULTICAST set.  This
// doesn't mean that there is no multicast code in the Android kernel, it means
// there is no IGMP code in the kernel.  Since IGMP isn't implemented, Android
// can't use it to enable and disable multicast at the driver level, so it uses
// wpa_supplicant driver-private commands instead.  This means that you will
// probably get three different answers if you ask how some piece of the
// multicast puzzle works.
//
// On the send side, multicast is controlled by the IP_MULTICAST_IF (or for
// IPv6 IPV6_MULTICAST_IF) socket.  In IPv4 you provide an IP address and in
// IPv6 you provide an interface index.  These differences are abstracted in
// the qcc code and there you are asked to provide an interface name, which the
// abstraction function uses to figure out the appropriate address or index
// depending on the address family.  Unfortunately, you can't abstract away
// the operating system differences in how they interpret the calls; so you
// really need to understand what is happening at a low level in order to get
// the high level multicast operations to do what you really want.
//
// If you do nothing (leave the sockets as you find them), or set the interface
// address to 0.0.0.0 for IPv4 or the interface index to 0 for IPv6 the
// multicast output interface is essentially selected by the system routing
// code.
//
// In Linux (and Android), multicast packets are sent out the interface that is
// used for the default route (the default interface).  You can see this if you
// type "ip ro sh".  In Windows, however, the system chooses its default
// interface by looking for the lowest value for the routing metric for a
// destination IP address of 224.0.0.0 in its routing table.  You can see this
// in the output of "route print".
//
// We want all of our multicast code to work in the presence of IP addresses
// changing when phones move from one Wifi access point to another, or when our
// desktop access point changes when someone with a mobile access point walks
// by; so it is also important to know what will happen when these addresses
// change (or come up or go down).
//
// On Linux, if you set the IP_MULTICAST_IF to 0.0.0.0 (or index 0 in IPv6) and
// bring down the default interface or change the IP address on the default
// interface, you will begin to fail the multicast sends with "network
// unreachable" errors since the default route goes away when you change the IP
// address (e.g, just do somthing like "sudo ifconfig eth1 10.4.108.237 netmask
// 255.255.255.0 up to change the address).  Until you provide a new default
// route (e.g., "route add default gw 10.4.108.1") the multicast packets will be
// dropped, but as soon as a new default route is set, they will begin flowing
// again.
//
// In Windows, if you set the IP_MULTICAST_IF address to 0.0.0.0 and release the
// ip address (e.g., "ipconfig /release") the sends may still appear to work at
// the level of the program but nothing goes out the original interface.  The
// sends fail silently.  This is because Windows will dynamically change the
// default multicast route according to its internal multicast routing table.
// It selects another interface based on a routing metric, and it could, for
// example, just switch to a VMware virtual interface silently.  The name
// service would never know it just "broke" and is no longer sending packets out
// the interface it thinks it is.
//
// When we set up multicast advertisements in our system, we most likely do not
// want to route our advertisements only to the default adapter.  For example,
// on a desktop system, the default interface is probably one of the wired
// Ethernets.  We may or many not want to advertise on that interface, but we
// may also want to advertise on other wired interfaces and other wireless
// interfaces as well.
//
// We do not want the system to start changing multicast destinations out from
// under us, EVER.  Because of this, the only time using INADDR_ANY would be
// appropriate in the IP_MULTICAST_IF socket option is in the simplest, static
// network situations.  For the general case, we really need to keep multiple
// sockets that are each talking to an INTERFACE of interest (not an IP address
// of interest, since they can change at any time because of normal access point
// dis-associations, for example).
//
// Since we determined that we needed to use IP_MULTICAST_IF to control which
// interfaces are used for discovery, we needed to understand exactly what
// changing an IP address out from under a corresponding interface would do.
//
// The first thing we observed is that IP_MULTICAST_IF takes an IP address in
// the case of IPv4, but we wanted to specify an interface index as in IPv6 or
// for mere mortal human beings, a name (e.g., "wlan0").  It may be the case
// that the interface does not have an IP address assigned (is not up or
// connected to an access point) at the time we want to start our name service,
// so a call to set the IP_MULTICAST_IF (via the appropriate abstract qcc call)
// would not be possible until an address is available, perhaps an arbitrary
// unknowable time later.  If sendto() operations are attempted and the IP
// address is not valid one will see "network unreachable" errors.  As we will
// discuss shortly, joining a multicast group also requires an IP address in the
// case of IPv4 (need to send IGMP Join messages), so it is not possible to
// express interest in receiving multicast packets until an IP address is
// available.
//
// So we needed to provide an API that allows a user to specify a network
// interface over which she is interested in advertising.  This explains the
// method OpenInterface(qcc::String interface) defined below.  The client is
// expected to figure out which interfaces it wants to do discovery over (e.g.,
// "wlan0", "eth0") and explicitly tell the name service which interfaces it is
// interested in.  We clearly need a lazy evaluation mechanism in the name
// service to look at the interfaces which the client expresses interest in, and
// when IP addresses are available, or change, we begin using those interfaces.
// If the interfaces go down, or change out from under the name service, we need
// to deal with that fact and make things right.
//
// We can either hook system "IP address changed" or "interface state changed"
// events to drive the re-evaluation process as described above, or we can poll
// for those changes.  Since the event systems in our various target platforms
// are wildly different, creating an abstract event system is non-trivial (for
// example, a DBus-based network manager exists on Linux, but even though
// Android is basically Linux and has DBus, it doesn't use it.  You'd need to
// use Netlink sockets on most Posix systems, but Darwin doesn't have Netlink.
// Windows is from another planet.
//
// Because of all of these complications, we just choose the better part of
// valor and poll for changes using a maintenance thread that fires off every
// second and looks for changes in the networking environment and adjusts
// accordingly.
//
// We could check for IP address changes on the interfaces and re-evaluate and
// open new sockets bound to the correct interfaces whenever an address change
// happens.  It is possible, however, that we could miss the fact that we have
// switched access points if DHCP gives us the same IP address.  Windows, for
// example, could happily begin rerouting packets to other interfaces if one
// goes down.  If the interface comes back up on a different access point, which
// gives out the same IP address, Windows could bring us back up but leave the
// multicast route pointing somewhere else and we would never notice.  Because
// of these byzantine kinds of errors, we chose the better part of valor and
// decided to close all of our multicast sockets down and restart them in a
// known state periodically.
//
// The receive side has similar kinds of issues.
//
// In order to receive multicast datagrams sent to a particular port, it is
// necessary to bind that local port leaving the local address unspecified
// (i.e., INADDR_ANY or in6addr_any).  What you might think of as binding is
// then actually handled by the Internet Group Management Protocol (IGMP) or its
// ICMPv6 equivalent.  Recall that Android does not implement IGMP, so we have
// yet another complication.
//
// Using IGMP, we join the socket to the multicast group instead of binding the
// socket to a specific interface (address) and port.  Binding the socket to
// INADDR_ANY or in6addr_any may look strange, but it is actually the right
// thing to do.  Since joining a multicast group requires sending packets over
// the IGMP protocol, we need a valid IP address in order to do the join.  As
// mentioned above, an interface must be IFF_UP with an assigned IP address in
// order to join a multicast group.
//
// The socket option for joining a multicast group, of course, works differently
// for IPv4 and IPv6.  IP_ADD_MEMBERSHIP (for IPv4) has a provided IP address
// that can be either INADDR_ANY or a specific address.  If INADDR_ANY is
// provided, the interface of the default route is added to the group, and the
// IGMP join is sent out that interface.  IPV6_ADD_MEMBERSHIP (for IPv6) has a
// provided interface index that can be either 0 or a specific interface.  If 0
// is provided, the interface of the default route is added to the group, and
// the IGMP Join (actually an ICMPv6 equivalent) is sent out that interface.  If
// a specific interface index is that interface is added to the group and the
// IGMP join is sent out that interface.  Note that since an ICMP packet is sent,
// the interface must be IFF_UP with an assigned IP address even though the
// interface is specified by an index.
//
// A side effect of the IGMP join deep down in the kernel is to enable reception
// of multicast MAC addresses in the device driver.  Since there is no IGMP in
// Android, we must rely on a multicast (Java) lock being taken by some external
// code on phones that do not leave multicast always enabled (HTC Desire, for
// example).  When the Java multicast lock is taken, a private driver command is
// sent to the wpa_supplicant which, in turn, calls into the appropriate network
// device driver(s) to enable reception of multicast MAC packets.  This is
// completely out of our control here.
//
// Similar to the situation on the send side, we most likely do not want to rely
// on the system routing tables to configure which network interfaces our name
// service receives over; so we really need to provide a specific address.
//
// If a specific IP address is provided, then that address must be an address
// assigned to a currently-UP interface.  This is the same catch-22 as we have
// on the send side.  We need to lazily evaluate the interface in order to find
// if an IP address has appeared on that interface and then join the multicast
// group to enable multicast on the underlying network device.
//
// It turns out that in Linux, the IP address passed to the join multicast group
// socket option call is actually not significant after the initial call.  It is
// used to look up an interface and its associated net device and to then set
// the PACKET_MULTICAST filter on the net device to receive packets destined for
// the specified multicast address.  If the IP address associated with the
// interface changes, multicast messages will continue to be received.
//
// Of course, Windows does it differently.  They look at the IP address passed
// to the socket option as being significant, and so if the underlying IP
// address changes on a Windows system, multicast packets will no longer be
// delivered.  Because of this, the receive side of the multicast name service
// has also got to look for changes to IP address configuration and re-set
// itself whenever it finds a change.
//
// So the code you find below may look overly complicated, but (hopefully most
// of it, anyway) needs to be that way.
//
// As an aside, the daemon that owns us can be happy as a clam by simply binding
// to INADDR_ANY since the semantics of this action, as interpreted by both
// Windows and Linux, are to listen for connections on all current and future
// interfaces and their IP addresses.  The complexity is fairly well contained
// here.
// ============================================================================

//
// There are configurable attributes of the name service which are determined
// by the configuration database.  A module name is required and is defined
// here.  An example of how to use this is in setting the interfaces the name
// service will use for discovery.
//
//   <busconfig>
//       <property name="ns_interfaces">*</property>
//       <flag name="ns_disable_directed_broadcast">false</flag>
//       <flag name="ns_disable_ipv4">false</flag>
//       <flag name="ns_disable_ipv6">false</flag>
//   </busconfig>
//
//
// The value of the interfaces property used to configure the name service
// to run discovery over all interfaces in the system.
//
const char* IpNameServiceImpl::INTERFACES_WILDCARD = "*";

//
// Define WORKAROUND_2_3_BUG to send name service messages over the old site
// administered addresses to work around a forward compatibility bug introduced
// in version 2.3 daemons.  They neglect to join the new IANA assigned multicast
// groups and so cannot receive advertisements on those groups.  In order to
// workaround this problem, we send version zero name service messages over the
// old groups.  The old versions can send new IANA multicast group messages so
// we can receive advertisements from them.  They just can't hear our new
// messages
//
#define WORKAROUND_2_3_BUG 0
#if WORKAROUND_2_3_BUG

//
// This is just a random IPv4 multicast group chosen out of the defined site
// administered block of addresses.  This was a temporary choice while an IANA
// reservation was in process, and remains for backward compatibility.
//
const char* IpNameServiceImpl::IPV4_MULTICAST_GROUP = "239.255.37.41";

//
// This is an IPv6 version of the temporary IPv4 multicast address described
// above.  IPv6 multicast groups are composed of a prefix containing 0xff and
// then flags (4 bits) followed by the IPv6 Scope (4 bits) and finally the IPv4
// group, as in "ff03::239.255.37.41".  The Scope corresponding to the IPv4
// Local Scope group is defined to be "3" by RFC 2365.  Unfortunately, the
// qcc::IPAddress code can't deal with "ff03::239.255.37.41" so we have to
// translate it.
//
const char* IpNameServiceImpl::IPV6_MULTICAST_GROUP = "ff03::efff:2529";

#endif


#if 1
//
// This is the IANA assigned IPv4 multicast group for AllJoyn.  This is
// a Local Network Control Block address.
//
// See IPv4 Multicast Address space Registry IANA
//
const char* IpNameServiceImpl::IPV4_ALLJOYN_MULTICAST_GROUP = "224.0.0.113";

const char* IpNameServiceImpl::IPV4_MDNS_MULTICAST_GROUP = "224.0.0.251";
#endif

//
// This is the IANA assigned UDP port for the AllJoyn Name Service.  See
// see Service Name and Transport Protocol Port Number Registry IANA
//
const uint16_t IpNameServiceImpl::MULTICAST_PORT = 9956;
const uint16_t IpNameServiceImpl::BROADCAST_PORT = IpNameServiceImpl::MULTICAST_PORT;

const uint16_t IpNameServiceImpl::MULTICAST_MDNS_PORT = IpNameService::MULTICAST_MDNS_PORT;
const uint16_t IpNameServiceImpl::BROADCAST_MDNS_PORT = IpNameServiceImpl::MULTICAST_MDNS_PORT;

//
// This is the IANA assigned IPv6 multicast group for AllJoyn.  The assigned
// address is a variable scope address (ff0x) but we always use the link local
// scope (ff02).  See IPv4 Multicast Address space Registry IANA
//
const char* IpNameServiceImpl::IPV6_ALLJOYN_MULTICAST_GROUP = "ff02::13a";

const char* IpNameServiceImpl::IPV6_MDNS_MULTICAST_GROUP = "ff02::fb";

const uint32_t IpNameServiceImpl::RETRY_INTERVALS[] = { 1, 2, 6, 18 };


IpNameServiceImpl::IpNameServiceImpl()
    : Thread("IpNameServiceImpl"), m_state(IMPL_SHUTDOWN), m_isProcSuspending(false),
    m_terminal(false), m_protect_callback(false), m_protect_net_callback(false), m_timer(0),
    m_tDuration(DEFAULT_DURATION), m_tRetransmit(RETRANSMIT_TIME), m_tQuestion(QUESTION_TIME),
    m_modulus(QUESTION_MODULUS), m_retries(sizeof(RETRY_INTERVALS) / sizeof(RETRY_INTERVALS[0])),
    m_loopback(false), m_enableIPv4(false), m_enableIPv6(false), m_enableV1(false),
    m_wakeEvent(), m_forceLazyUpdate(false), m_refreshAdvertisements(false),
    m_enabled(false), m_doEnable(false), m_doDisable(false),
    m_ipv4QuietSockFd(qcc::INVALID_SOCKET_FD), m_ipv6QuietSockFd(qcc::INVALID_SOCKET_FD),
    m_ipv4UnicastSockFd(qcc::INVALID_SOCKET_FD), m_unicastEvent(NULL),
    m_protectListeners(false), m_packetScheduler(*this),
    m_networkChangeScheduleCount(m_retries + 1)
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::IpNameServiceImpl()"));
    TRANSPORT_INDEX_TCP = IndexFromBit(TRANSPORT_TCP);
    TRANSPORT_INDEX_UDP = IndexFromBit(TRANSPORT_UDP);

    memset(&m_any[0], 0, sizeof(m_any));
    memset(&m_callback[0], 0, sizeof(m_callback));
    memset(&m_networkEventCallback[0], 0, sizeof(m_networkEventCallback));

    memset(&m_enabledReliableIPv4[0], 0, sizeof(m_enabledReliableIPv4));
    memset(&m_enabledUnreliableIPv4[0], 0, sizeof(m_enabledUnreliableIPv4));
    memset(&m_enabledReliableIPv6[0], 0, sizeof(m_enabledReliableIPv6));
    memset(&m_enabledUnreliableIPv6[0], 0, sizeof(m_enabledUnreliableIPv6));

    memset(&m_reliableIPv6Port[0], 0, sizeof(m_reliableIPv6Port));
    memset(&m_unreliableIPv6Port[0], 0, sizeof(m_unreliableIPv6Port));

    memset(&m_processTransport[0], 0, sizeof(m_processTransport));
    memset(&m_doNetworkCallback[0], 0, sizeof(m_doNetworkCallback));
}

QStatus IpNameServiceImpl::Init(const qcc::String& guid, bool loopback)
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::Init()"));

    //
    // Can only call Init() if the object is not running or in the process
    // of initializing
    //
    if (m_state != IMPL_SHUTDOWN) {
        return ER_FAIL;
    }

    //
    // There should be no queued packets between IMPL_SHUTDOWN to
    // IMPL_INITIALIZING.
    //
    assert(m_outbound.size() == 0);
    assert(m_burstQueue.size() == 0);

    m_state = IMPL_INITIALIZING;

    ConfigDB* config = ConfigDB::GetConfigDB();

    //
    // We enable outbound traffic on a per-interface basis.  Whether or not we
    // will consider using a network interface address to send name service
    // packets depends on the configuration.
    //
    m_enableIPv4 = !config->GetFlag("ns_disable_ipv4");
    m_enableIPv6 = !config->GetFlag("ns_disable_ipv6");
    m_broadcast = !config->GetFlag("ns_disable_directed_broadcast");

    //
    // We enable v0 and v1 traffic unless explicitly configured not to do so.
    //
    m_enableV1 = config->GetFlag("ns_enable_v1", true);

    //
    // Set the broadcast bit to true for WinRT. For all other platforms,
    // this field should be derived from the property disable_directed_broadcast
    //

    m_guid = guid;
    m_loopback = loopback;
    m_terminal = false;

    m_networkChangeScheduleCount = m_retries + 1;
    return ER_OK;
}

//
// When we moved the name service out of the TCP transport and promoted it to a
// singleton, we opened a bit of a can of worms because of the C++ static
// destruction order fiasco and our interaction with the bundled daemon.
//
// Since the bundled daemon may be destroyed after the IP name service singleton
// it is possible that multiple threads (transports) may be still accessing the
// name service as it is being destroyed.  This horrific situation will be
// resolved when we accomplish strict destructor ordering, but for now, we have
// the possibility.
//
// This object was never intended to provide multithread safe destruction and so
// we are exposed in the case where the object destroys itself around a thread
// that is executing in one of its methods.  The chances are small that this
// happens, but the chance is non-zero; and the result might be a crash after
// the process main() function exits!
//
IpNameServiceImpl::~IpNameServiceImpl()
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::~IpNameServiceImpl()"));

    //
    // Stop the worker thread to get things calmed down.
    //
    if (IsRunning()) {
        Stop();
        Join();
    }

    //
    // We may have some open sockets.  Windows boxes may have Winsock shut down
    // by the time we get to this destructor so we are out of luck trying to
    // make the necessary calls.
    //
#if !defined(QCC_OS_GROUP_WINDOWS)
    ClearLiveInterfaces();
#endif

    for (uint32_t i = 0; i < N_TRANSPORTS; ++i) {
        //
        // Delete any callbacks that any users of this class may have set.  We
        // assume we are not multithreaded at this point.
        //
        delete m_callback[i];
        m_callback[i] = NULL;

        delete m_networkEventCallback[i];
        m_networkEventCallback[i] = NULL;

        //
        // We can just blow away the requested interfaces without a care since
        // nobody else clears them and we are obviously done with them.
        //
        m_requestedInterfaces[i].clear();
    }

    //
    // If we opened a socket to send quiet responses (unicast, not over the
    // multicast channel) we need to close it.
    //
    if (m_ipv4QuietSockFd != qcc::INVALID_SOCKET_FD) {
        qcc::Close(m_ipv4QuietSockFd);
        m_ipv4QuietSockFd = qcc::INVALID_SOCKET_FD;
    }

    if (m_ipv6QuietSockFd != qcc::INVALID_SOCKET_FD) {
        qcc::Close(m_ipv6QuietSockFd);
        m_ipv6QuietSockFd = qcc::INVALID_SOCKET_FD;
    }

    //
    // m_unicastEvent must be deleted before closing m_ipv4UnicastSockFd,
    // because m_unicastEvent's destructor code path is using
    // m_ipv4UnicastSockFd.
    //
    if (m_unicastEvent) {
        delete m_unicastEvent;
        m_unicastEvent = NULL;
    }

    if (m_ipv4UnicastSockFd != qcc::INVALID_SOCKET_FD) {
        qcc::Close(m_ipv4UnicastSockFd);
        m_ipv4UnicastSockFd = qcc::INVALID_SOCKET_FD;
    }
    //
    // All shut down and ready for bed.
    //
    m_state = IMPL_SHUTDOWN;
}

QStatus IpNameServiceImpl::CreateVirtualInterface(const qcc::IfConfigEntry& entry)
{
    QCC_DbgPrintf(("IpNameServiceImpl::CreateVirtualInterface(%s)", entry.m_name.c_str()));

    std::vector<qcc::IfConfigEntry>::const_iterator it = m_virtualInterfaces.begin();
    for (; it != m_virtualInterfaces.end(); it++) {
        if (it->m_name == entry.m_name) {
            QCC_DbgPrintf(("Interface(%s) already exists", entry.m_name.c_str()));
            return ER_FAIL;
        }
    }
    m_virtualInterfaces.push_back(entry);
    m_forceLazyUpdate = true;
    m_wakeEvent.SetEvent();
    return ER_OK;
}

QStatus IpNameServiceImpl::DeleteVirtualInterface(const qcc::String& ifceName)
{
    QCC_DbgPrintf(("IpNameServiceImpl::DeleteVirtualInterface(%s)", ifceName.c_str()));

    std::vector<qcc::IfConfigEntry>::iterator it = m_virtualInterfaces.begin();
    for (; it != m_virtualInterfaces.end(); it++) {
        if (it->m_name == ifceName) {
            m_virtualInterfaces.erase(it);
            m_forceLazyUpdate = true;
            m_wakeEvent.SetEvent();
            return ER_OK;
        }
    }
    QCC_DbgPrintf(("Interface(%s) does not exist", ifceName.c_str()));
    return ER_FAIL;
}

QStatus IpNameServiceImpl::OpenInterface(TransportMask transportMask, const qcc::String& name)
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::OpenInterface(%s)", name.c_str()));

    //
    // Exactly one bit must be set in a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::OpenInterface(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    //
    // Can only call OpenInterface() if the object is running.
    //
    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("IpNameServiceImpl::OpenInterface(): Not running"));
        return ER_FAIL;
    }

    //
    // If the user specifies the wildcard interface name, this trumps everything
    // else.
    //
    if (name == INTERFACES_WILDCARD) {
        qcc::IPAddress wildcard("0.0.0.0");
        return OpenInterface(transportMask, wildcard);
    }

    qcc::IPAddress addr;
    QStatus status = addr.SetAddress(name, false);
    if (status == ER_OK) {
        return OpenInterface(transportMask, addr);
    }
    uint32_t transportIndex = IndexFromBit(transportMask);
    assert(transportIndex < 16 && "IpNameServiceImpl::OpenInterface(): Bad transport index");

    if (transportIndex >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }
    //
    // There are at least two threads that can wander through the vector below
    // so we need to protect access to the list with a convenient mutex.
    //
    m_mutex.Lock();

    for (uint32_t i = 0; i < m_requestedInterfaces[transportIndex].size(); ++i) {
        if (m_requestedInterfaces[transportIndex][i].m_interfaceName == name) {
            QCC_DbgPrintf(("IpNameServiceImpl::OpenInterface(): Already opened."));
            // We need to be idempotent. It is possible that one of the
            // transports has been shut down, but some other transports
            // are still up. We want to allow the transport that was shut
            // down the possibility of being revived and refreshing its
            // network state.
            m_processTransport[transportIndex] = true;
            m_forceLazyUpdate = true;
            m_wakeEvent.SetEvent();
            m_mutex.Unlock();
            return ER_OK;
        }
    }

    InterfaceSpecifier specifier;
    specifier.m_interfaceName = name;
    specifier.m_interfaceAddr = qcc::IPAddress("0.0.0.0");
    specifier.m_transportMask = transportMask;

    m_processTransport[transportIndex] = true;
    m_requestedInterfaces[transportIndex].push_back(specifier);
    m_forceLazyUpdate = true;
    m_wakeEvent.SetEvent();
    m_mutex.Unlock();
    return ER_OK;
}

QStatus IpNameServiceImpl::OpenInterface(TransportMask transportMask, const qcc::IPAddress& addr)
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::OpenInterface(%s)", addr.ToString().c_str()));

    //
    // Exactly one bit must be set in a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::OpenInterface(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    uint32_t transportIndex = IndexFromBit(transportMask);
    assert(transportIndex < 16 && "IpNameServiceImpl::OpenInterface(): Bad transport index");

    if (transportIndex >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    //
    // Can only call OpenInterface() if the object is running.
    //
    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("IpNameServiceImpl::OpenInterface(): Not running"));
        return ER_FAIL;
    }

    //
    // There are at least two threads that can wander through the vector below
    // so we need to protect access to the list with a convenient mutex.
    //
    m_mutex.Lock();

    //
    // We treat the INADDR_ANY address (and the equivalent IPv6 address as a
    // wildcard.  To have the same semantics as using INADDR_ANY in the TCP
    // transport listen spec, and avoid resulting user confusion, we need to
    // interpret this as "use any interfaces that are currently up, or may come
    // up in the future to send and receive name service messages over."  This
    // trumps anything else the user might throw at us.  We set a global flag to
    // indicate this mode of operation and clear it if we see a CloseInterface()
    // on INADDR_ANY.  These calls are not reference counted.
    //
    m_any[transportIndex] = false;
    if (addr == qcc::IPAddress("0.0.0.0") ||
        addr == qcc::IPAddress("0::0") ||
        addr == qcc::IPAddress("::")) {
        QCC_DbgPrintf(("IpNameServiceImpl::OpenInterface(): Wildcard address"));
        m_any[transportIndex] = true;
        m_processTransport[transportIndex] = true;
        m_forceLazyUpdate = true;
        m_wakeEvent.SetEvent();
        m_mutex.Unlock();
        return ER_OK;
    }

    for (uint32_t i = 0; i < m_requestedInterfaces[transportIndex].size(); ++i) {
        if (m_requestedInterfaces[transportIndex][i].m_interfaceAddr == addr) {
            QCC_DbgPrintf(("IpNameServiceImpl::OpenInterface(): Already opened."));
            // We need to be idempotent. It is possible that one of the
            // transports has been shut down, but some other transports
            // are still up. We want to allow the transport that was shut
            // down the possibility of being revived and refreshing its
            // network state.
            m_processTransport[transportIndex] = true;
            m_forceLazyUpdate = true;
            m_wakeEvent.SetEvent();
            m_mutex.Unlock();
            return ER_OK;
        }
    }

    InterfaceSpecifier specifier;
    specifier.m_interfaceName = "";
    specifier.m_interfaceAddr = addr;
    specifier.m_transportMask = transportMask;

    m_processTransport[transportIndex] = true;
    m_requestedInterfaces[transportIndex].push_back(specifier);
    m_forceLazyUpdate = true;
    m_wakeEvent.SetEvent();
    m_mutex.Unlock();
    return ER_OK;
}

QStatus IpNameServiceImpl::CloseInterface(TransportMask transportMask, const qcc::String& name)
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::CloseInterface(%s)", name.c_str()));

    //
    // Exactly one bit must be in set a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::CloseInterface(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    //
    // Can only call CloseInterface() if the object is running.
    //
    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("IpNameServiceImpl::CloseInterface(): Not running"));
        return ER_FAIL;
    }

    uint32_t transportIndex = IndexFromBit(transportMask);
    assert(transportIndex < 16 && "IpNameServiceImpl::CloseInterface(): Bad transport index");

    if (transportIndex >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    //
    // There are at least two threads that can wander through the vector below
    // so we need to protect access to the list with a convenient mutex.
    //
    m_mutex.Lock();

    //
    // use Meyers' idiom to keep iterators sane.  Note that we don't close the
    // socket in this call, we just remove the request and the lazy updator will
    // just not use it when it re-evaluates what to do.
    //
    for (vector<InterfaceSpecifier>::iterator i = m_requestedInterfaces[transportIndex].begin(); i != m_requestedInterfaces[transportIndex].end();) {
        if ((*i).m_interfaceName == name) {
            i = m_requestedInterfaces[transportIndex].erase(i);
        } else {
            ++i;
        }
    }

    m_forceLazyUpdate = true;
    m_wakeEvent.SetEvent();
    m_mutex.Unlock();
    return ER_OK;
}

QStatus IpNameServiceImpl::CloseInterface(TransportMask transportMask, const qcc::IPAddress& addr)
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::CloseInterface(%s)", addr.ToString().c_str()));

    //
    // Exactly one bit must be set in a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::CloseInterface(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    //
    // Can only call CloseInterface() if the object is running.
    //
    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("IpNameServiceImpl::CloseInterface(): Not running"));
        return ER_FAIL;
    }

    uint32_t transportIndex = IndexFromBit(transportMask);
    assert(transportIndex < 16 && "IpNameServiceImpl::CloseInterface(): Bad transport index");

    if (transportIndex >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    //
    // There are at least two threads that can wander through the vector below
    // so we need to protect access to the list with a convenient mutex.
    //
    m_mutex.Lock();

    //
    // We treat the INADDR_ANY address (and the equivalent IPv6 address as a
    // wildcard.  We set a global flag in OpenInterface() to indicate this mode
    // of operation and clear it here.  These calls are not reference counted
    // so one call to CloseInterface(INADDR_ANY) will stop this mode
    // irrespective of how many opens are done.
    //
    if (addr == qcc::IPAddress("0.0.0.0") ||
        addr == qcc::IPAddress("0::0") ||
        addr == qcc::IPAddress("::")) {
        QCC_DbgPrintf(("IpNameServiceImpl::CloseInterface(): Wildcard address"));
        m_any[transportIndex] = false;
        m_mutex.Unlock();
        return ER_OK;
    }

    //
    // use Meyers' idiom to keep iterators sane.  Note that we don't close the
    // socket in this call, we just remove the request and the lazy updator will
    // just not use it when it re-evaluates what to do.
    //
    for (vector<InterfaceSpecifier>::iterator i = m_requestedInterfaces[transportIndex].begin(); i != m_requestedInterfaces[transportIndex].end();) {
        if ((*i).m_interfaceAddr == addr) {
            i = m_requestedInterfaces[transportIndex].erase(i);
        } else {
            ++i;
        }
    }

    m_forceLazyUpdate = true;
    m_wakeEvent.SetEvent();
    m_mutex.Unlock();
    return ER_OK;
}

void IpNameServiceImpl::ClearLiveInterfaces(void)
{
    QCC_DbgPrintf(("IpNameServiceImpl::ClearLiveInterfaces()"));

    //
    // ClearLiveInterfaces is not called with the mutex taken so we need to
    // grab it.
    //
    m_mutex.Lock();

    for (uint32_t i = 0; i < m_liveInterfaces.size(); ++i) {
        if (m_liveInterfaces[i].m_multicastMDNSsockFd != qcc::INVALID_SOCKET_FD || m_liveInterfaces[i].m_multicastsockFd != qcc::INVALID_SOCKET_FD) {

            QCC_DbgPrintf(("IpNameServiceImpl::ClearLiveInterfaces(): clear interface %d", i));

            //
            // If the multicast bit is set, we have done an IGMP join.  In this
            // case, we must arrange an IGMP drop via the appropriate socket option
            // (via the qcc absraction layer). Android doesn't bother to compile its
            // kernel with CONFIG_IP_MULTICAST set.  This doesn't mean that there is
            // no multicast code in the Android kernel, it means there is no IGMP
            // code in the kernel.  What this means to us is that even through we
            // are doing an IP_DROP_MEMBERSHIP request, which is ultimately an IGMP
            // operation, the request will filter through the IP code before being
            // ignored and will do useful things in the kernel even though
            // CONFIG_IP_MULTICAST was not set for the Android build -- i.e., we
            // have to do it anyway.
            //
            if (m_liveInterfaces[i].m_flags & qcc::IfConfigEntry::MULTICAST ||
                m_liveInterfaces[i].m_flags & qcc::IfConfigEntry::LOOPBACK) {
                if (m_liveInterfaces[i].m_address.IsIPv4()) {
#if 1
                    if (m_liveInterfaces[i].m_multicastMDNSsockFd != qcc::INVALID_SOCKET_FD) {
                        qcc::LeaveMulticastGroup(m_liveInterfaces[i].m_multicastMDNSsockFd, qcc::QCC_AF_INET, IPV4_MDNS_MULTICAST_GROUP,
                                                 m_liveInterfaces[i].m_interfaceName);
                    }
                    if (m_liveInterfaces[i].m_multicastsockFd != qcc::INVALID_SOCKET_FD) {
                        qcc::LeaveMulticastGroup(m_liveInterfaces[i].m_multicastsockFd, qcc::QCC_AF_INET, IPV4_ALLJOYN_MULTICAST_GROUP,
                                                 m_liveInterfaces[i].m_interfaceName);
                    }
#endif
                } else if (m_liveInterfaces[i].m_address.IsIPv6()) {
                    if (m_liveInterfaces[i].m_multicastMDNSsockFd != qcc::INVALID_SOCKET_FD) {
                        qcc::LeaveMulticastGroup(m_liveInterfaces[i].m_multicastMDNSsockFd, qcc::QCC_AF_INET6, IPV6_MDNS_MULTICAST_GROUP,
                                                 m_liveInterfaces[i].m_interfaceName);
                    }
                    if (m_liveInterfaces[i].m_multicastsockFd != qcc::INVALID_SOCKET_FD) {
                        qcc::LeaveMulticastGroup(m_liveInterfaces[i].m_multicastsockFd, qcc::QCC_AF_INET6, IPV6_ALLJOYN_MULTICAST_GROUP,
                                                 m_liveInterfaces[i].m_interfaceName);
                    }

                }
            }

            //
            // Always delete the event before closing the socket because the event
            // is monitoring the socket state and therefore has a reference to the
            // socket.  One the socket is closed the FD can be reused and our event
            // can end up monitoring the wrong socket and interfere with the correct
            // operation of other unrelated event/socket pairs.
            //
            if (m_liveInterfaces[i].m_multicastMDNSsockFd != qcc::INVALID_SOCKET_FD) {
                delete m_liveInterfaces[i].m_multicastMDNSevent;
                m_liveInterfaces[i].m_multicastMDNSevent = NULL;
                qcc::Close(m_liveInterfaces[i].m_multicastMDNSsockFd);
                m_liveInterfaces[i].m_multicastMDNSsockFd = qcc::INVALID_SOCKET_FD;
            }

            if (m_liveInterfaces[i].m_multicastsockFd != qcc::INVALID_SOCKET_FD) {
                delete m_liveInterfaces[i].m_multicastevent;
                m_liveInterfaces[i].m_multicastevent = NULL;
                qcc::Close(m_liveInterfaces[i].m_multicastsockFd);
                m_liveInterfaces[i].m_multicastsockFd = qcc::INVALID_SOCKET_FD;
            }
        }
    }

    QCC_DbgPrintf(("IpNameServiceImpl::ClearLiveInterfaces(): Clear interfaces"));
    m_liveInterfaces.clear();

    m_mutex.Unlock();

    QCC_DbgPrintf(("IpNameServiceImpl::ClearLiveInterfaces(): Done"));
}

QStatus IpNameServiceImpl::CreateUnicastSocket()
{
    if (m_ipv4UnicastSockFd == qcc::INVALID_SOCKET_FD) {
        QStatus status = qcc::Socket(qcc::QCC_AF_INET, qcc::QCC_SOCK_DGRAM, m_ipv4UnicastSockFd);
        if (status != ER_OK) {
            QCC_LogError(status, ("CreateUnicastSocket: qcc::Socket(%d) failed: %d - %s", qcc::QCC_AF_INET,
                                  qcc::GetLastError(), qcc::GetLastErrorString().c_str()));
            m_ipv4UnicastSockFd = qcc::INVALID_SOCKET_FD;
            return status;
        }
        status = qcc::SetRecvPktAncillaryData(m_ipv4UnicastSockFd, qcc::QCC_AF_INET, true);
        if (status != ER_OK) {
            QCC_LogError(status, ("CreateUnicastSocket: enable recv ancillary data"
                                  " failed for sockFd %d", m_ipv4UnicastSockFd));
            qcc::Close(m_ipv4UnicastSockFd);
            m_ipv4UnicastSockFd = qcc::INVALID_SOCKET_FD;
            return status;
        }
        //
        // We must be able to reuse the address/port combination so other
        // AllJoyn daemon instances on the same host can listen in if desired.
        // This will set the SO_REUSEPORT socket option if available or fall
        // back onto SO_REUSEADDR if not.
        //
        status = qcc::SetReusePort(m_ipv4UnicastSockFd, true);
        if (status != ER_OK && status != ER_NOT_IMPLEMENTED) {
            QCC_LogError(status, ("CreateUnicastSocket(): SetReusePort() failed"));
            qcc::Close(m_ipv4UnicastSockFd);
            m_ipv4UnicastSockFd = qcc::INVALID_SOCKET_FD;
            return status;
        }
        //
        // We bind to an ephemeral port.
        //
        status = qcc::Bind(m_ipv4UnicastSockFd, qcc::IPAddress("0.0.0.0"), 0);
        if (status != ER_OK) {
            QCC_LogError(status, ("CreateUnicastSocket(): bind failed"));
            qcc::Close(m_ipv4UnicastSockFd);
            m_ipv4UnicastSockFd = qcc::INVALID_SOCKET_FD;
            return status;
        }
    }
    return ER_OK;
}

QStatus CreateMulticastSocket(IfConfigEntry entry, const char* ipv4_multicast_group, const char* ipv6_multicast_group, uint16_t port, bool broadcast, SocketFd& sockFd)
{
    QStatus status = qcc::Socket(entry.m_family, qcc::QCC_SOCK_DGRAM, sockFd);
    if (status != ER_OK) {
        QCC_LogError(status, ("CreateMulticastSocket: qcc::Socket(%d) failed: %d - %s", entry.m_family,
                              qcc::GetLastError(), qcc::GetLastErrorString().c_str()));
        return status;
    }

    status = qcc::SetRecvPktAncillaryData(sockFd, entry.m_family, true);
    if (status != ER_OK) {
        QCC_LogError(status, ("CreateMulticastSocket: enable recv ancillary data"
                              " failed for sockFd %d", sockFd));
        qcc::Close(sockFd);
        return status;
    }

    if (entry.m_family == qcc::QCC_AF_INET6) {
        status = qcc::SetRecvIPv6Only(sockFd, true);
        if (status != ER_OK) {
            QCC_LogError(status, ("CreateMulticastSocket: enable recv IPv6 only"
                                  " failed for sockFd %d", sockFd));
            qcc::Close(sockFd);
            return status;
        }
    }

    if (broadcast && entry.m_flags & qcc::IfConfigEntry::BROADCAST) {
        //
        // If we're going to send broadcasts, we have to ask for
        // permission for the multicast NS socket FD.
        //

        status = qcc::SetBroadcast(sockFd, true);
        if (status != ER_OK && status != ER_NOT_IMPLEMENTED) {
            QCC_LogError(status, ("CreateMulticastSocket: enable broadcast failed"));
            qcc::Close(sockFd);
            return status;
        }
    }

    //
    // We must be able to reuse the address/port combination so other
    // AllJoyn daemon instances on the same host can listen in if desired.
    // This will set the SO_REUSEPORT socket option if available or fall
    // back onto SO_REUSEADDR if not.
    //

    status = qcc::SetReusePort(sockFd, true);
    if (status != ER_OK && status != ER_NOT_IMPLEMENTED) {
        QCC_LogError(status, ("CreateMulticastSocket(): SetReusePort() failed"));
        qcc::Close(sockFd);
        return status;
    }
    //
    // If the MULTICAST or LOOPBACK flag is set, we are going to try and
    // multicast out over the interface in question.  If one of the flags is
    // not set, then we want to fall back to IPv4 subnet directed broadcast,
    // so we optionally do all of the multicast games and take the interface
    // live even if it doesn't support multicast.
    //

    if (entry.m_flags & qcc::IfConfigEntry::MULTICAST ||
        entry.m_flags & qcc::IfConfigEntry::LOOPBACK) {
        //
        // Restrict the scope of the sent muticast packets to the local subnet.
        //
        status = qcc::SetMulticastHops(sockFd, entry.m_family, 1);
        if (status != ER_OK && status != ER_NOT_IMPLEMENTED) {
            QCC_LogError(status, ("CreateMulticastSocket(): SetMulticastHops() failed"));
            qcc::Close(sockFd);
            return status;
        }

        //
        // In order to control which interfaces get our multicast datagrams, it
        // is necessary to do so via a socket option.  See the Long Sidebar above.
        // Yes, you have to do it differently depending on whether or not you're
        // using IPv4 or IPv6.
        //
        status = qcc::SetMulticastInterface(sockFd, entry.m_family, entry.m_name);
        if (status != ER_OK && status != ER_NOT_IMPLEMENTED) {
            QCC_LogError(status, ("CreateMulticastSocket(): SetMulticastInterface() failed"));
            qcc::Close(sockFd);
            return status;
        }
    }
    if (entry.m_family == qcc::QCC_AF_INET) {

        status = qcc::Bind(sockFd, qcc::IPAddress("0.0.0.0"), port);
        if (status != ER_OK) {
            QCC_LogError(status, ("CreateMulticastSocket(): bind(0.0.0.0) failed"));
            qcc::Close(sockFd);
            return status;

        }
    } else if (entry.m_family == qcc::QCC_AF_INET6) {

        status = qcc::Bind(sockFd, qcc::IPAddress("::"), port);
        if (status != ER_OK) {
            QCC_LogError(status, ("CreateMulticastSocket(): bind(::) failed"));
            qcc::Close(sockFd);
            return status;

        }
    }
    //
    // The IGMP join must be done after the bind for Windows XP.  Other
    // OSes are fine with it, but XP balks.
    //
    if (entry.m_flags & qcc::IfConfigEntry::MULTICAST ||
        entry.m_flags & qcc::IfConfigEntry::LOOPBACK) {
        //
        // Arrange an IGMP join via the appropriate socket option (via the
        // qcc abstraction layer). Android doesn't bother to compile its
        // kernel with CONFIG_IP_MULTICAST set.  This doesn't mean that
        // there is no multicast code in the Android kernel, it means there
        // is no IGMP code in the kernel.  What this means to us is that
        // even through we are doing an IP_ADD_MEMBERSHIP request, which is
        // ultimately an IGMP operation, the request will filter through the
        // IP code before being ignored and will do useful things in the
        // kernel even though CONFIG_IP_MULTICAST was not set for the
        // Android build -- i.e., we have to do it anyway.
        //
        if (entry.m_family == qcc::QCC_AF_INET) {
#if 1
            status = qcc::JoinMulticastGroup(sockFd, qcc::QCC_AF_INET, ipv4_multicast_group, entry.m_name);
#endif
        } else if (entry.m_family == qcc::QCC_AF_INET6) {
            status = qcc::JoinMulticastGroup(sockFd, qcc::QCC_AF_INET6, ipv6_multicast_group, entry.m_name);
        }
        if (status != ER_OK) {
            QCC_LogError(status, ("CreateMulticastSocket(): JoinMulticastGroup failed"));

            qcc::Close(sockFd);
            return status;
        }
    }
    return ER_OK;
}
//
// N.B. This function must be called with m_mutex locked since we wander through
// the list of requested interfaces that can also be modified by the user in the
// context of her thread(s).
//
void IpNameServiceImpl::LazyUpdateInterfaces(const qcc::NetworkEventSet& networkEvents)
{
    QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces()"));

    //
    // However desirable it may be, the decision to simply use an existing
    // open socket exposes us to system-dependent behavior.  For example,
    // In Linux and Windows, an IGMP join must be done on an interface that
    // is currently IFF_UP and IFF_MULTICAST with an assigned IP address.
    // On Linux, that join remains in effect (net devices will continue to
    // recieve multicast packets destined for our group) even if the net
    // device goes down and comes back up with a different IP address.  On
    // Windows, however, if the interface goes down, an IGMP drop is done
    // and multicast receives will stop.  Since the socket never returns
    // any status unless we actually send data, it is very possible that
    // the state of the system can change out from underneath us without
    // our knowledge, and we would simply stop receiving multicasts. This
    // behavior is not specified anywhere that I am aware of, so Windows
    // cannot really be said to be broken.  It is just different, like it
    // is in so many other ways.  In Android, IGMP isn't even compiled into
    // the kernel, and so an out-of-band mechanism is used (wpa_supplicant
    // private driver commands called by the Java multicast lock).
    //
    // It can be argued that since we are using Android phones (sort-of Linux)
    // when mobility is a concern, and Windows boxes would be relatively static,
    // we could get away with ignoring the possibility of missing interface
    // state changes.  Since we are really talking an average of a couple of
    // IGMP packets every 30 seconds we take the conservative approach and tear
    // down all of our sockets and restart them every time through.
    //
    ClearLiveInterfaces();

    //
    // If m_enable is false, we need to make sure that no packets are sent
    // and no sockets are listening for connections.  This is for Android
    // Compatibility Test Suite (CTS) conformance.  The only way we can talk
    // to the outside world is via one of the live interfaces, so if we don't
    // make any new ones, this will accomplish the requirement.
    //
    bool processAnyTransport = false;
    for (uint32_t i = 0; i < N_TRANSPORTS; ++i) {
        if (m_processTransport[i] || m_doNetworkCallback[i]) {
            processAnyTransport = true;
            break;
        }
    }

    if (m_enabled == false && processAnyTransport == false) {
        QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): Communication with the outside world is forbidden"));
        if (m_unicastEvent) {
            delete m_unicastEvent;
            m_unicastEvent = NULL;
        }
        if (m_ipv4UnicastSockFd != qcc::INVALID_SOCKET_FD) {
            qcc::Close(m_ipv4UnicastSockFd);
            m_ipv4UnicastSockFd = qcc::INVALID_SOCKET_FD;
        }
        return;
    }

    if (m_isProcSuspending) {
        QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): The process is suspending. Stop communicating with the outside world"));
        if (m_unicastEvent) {
            delete m_unicastEvent;
            m_unicastEvent = NULL;
        }
        if (m_ipv4UnicastSockFd != qcc::INVALID_SOCKET_FD) {
            qcc::Close(m_ipv4UnicastSockFd);
            m_ipv4UnicastSockFd = qcc::INVALID_SOCKET_FD;
        }
        return;
    }
    //
    // Call IfConfig to get the list of interfaces currently configured in the
    // system.  This also pulls out interface flags, addresses and MTU.  If we
    // can't get the system interfaces, we give up for now and hope the error
    // is transient.
    //
    QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): IfConfig()"));
    std::vector<qcc::IfConfigEntry> entries;
    QStatus status = qcc::IfConfig(entries);
    if (status != ER_OK) {
        QCC_LogError(status, ("LazyUpdateInterfaces: IfConfig() failed"));
        if (m_unicastEvent) {
            delete m_unicastEvent;
            m_unicastEvent = NULL;
        }
        if (m_ipv4UnicastSockFd != qcc::INVALID_SOCKET_FD) {
            qcc::Close(m_ipv4UnicastSockFd);
            m_ipv4UnicastSockFd = qcc::INVALID_SOCKET_FD;
        }
        return;
    }

    // add the virtual network interfaces if any
    if (m_virtualInterfaces.size() > 0) {
        entries.insert(entries.end(), m_virtualInterfaces.begin(), m_virtualInterfaces.end());
    }

    //
    // There are two fundamental ways we can look for interfaces to use.  We
    // can either walk the list of IfConfig entries (real interfaces on the
    // system) looking for any that match our list of user-requested
    // interfaces; or we can walk the list of user-requested interfaces looking
    // for any that match the list of real IfConfig entries.  Since we have an
    // m_any mode that means match all real IfConfig entries, we need to walk
    // the real IfConfig entries.
    //
    for (uint32_t i = 0; (m_state == IMPL_RUNNING || m_terminal) && (i < entries.size()); ++i) {
        //
        // We expect that every device in the system must have a name.
        // It might be some crazy random GUID in Windows, but it will have
        // a name.
        //
        assert(entries[i].m_name.size());
        QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): Checking out interface %s", entries[i].m_name.c_str()));

        //
        // We are never interested in interfaces that are not UP.
        //
        if ((entries[i].m_flags & qcc::IfConfigEntry::UP) == 0) {
            QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): not UP"));
            continue;
        }

#if defined(QCC_OS_LINUX)
        if ((entries[i].m_flags & qcc::IfConfigEntry::RUNNING) == 0) {
            QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): not RUNNING"));
            continue;
        }
#endif
        //
        // LOOPBACK interfaces are a special case: sending messages to
        // the local host is handled by the MULTICAST_LOOP socket
        // option which is enabled by default.  However we must stil
        // use the loopback interface in the case there are no other
        // interfaces UP.  Furthermore, multicast LOOPBACK over IPv6
        // doesn't appear to work consistently, so we are only
        // interested in IPv4 multicast interfaces.
        //
        if ((entries[i].m_flags & qcc::IfConfigEntry::LOOPBACK) != 0 &&
            (entries[i].m_family != qcc::QCC_AF_INET)) {
            QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): ignoring non-IPv4 loopback"));
            continue;
        }

        //
        // When initializing the name service, the user can decide whether or
        // not she wants to advertise and listen over IPv4 or IPv6.  We need
        // to check for that configuration here.  Since the rest of the code
        // just works with the live interfaces irrespective of address family,
        // this is the only place we need to do this check.
        //
        if ((m_enableIPv4 == false && entries[i].m_family == qcc::QCC_AF_INET) ||
            (m_enableIPv6 == false && entries[i].m_family == qcc::QCC_AF_INET6)) {
            QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): family %d not enabled", entries[i].m_family));
            continue;
        }

        //
        // The current real interface entry is a candidate for use.  We need to
        // decide if we are actually going to use it either based on the
        // wildcard mode or the list of requestedInterfaces provided by each of
        // the transports.
        //
        bool useEntry = false;
        for (uint32_t j = 0; j < N_TRANSPORTS; ++j) {
            QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): Check out interface cantidates for transport %d", j));

            if (m_any[j]) {
                QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): Wildcard set mode for transport %d", j));

                //
                // All interfaces means all except for "special use" interfaces
                // like Wi-Fi Direct interfaces on Android.  We don't know what
                // interfaces are actually in use by the Wi-Fi Direct subsystem
                // but it does seem that any P2P-based interface will begin with
                // the string "p2p" as in "p2p0" or "p2p-p2p0-0".
                //
                // Note that this assumes that the Wi-Fi Direct transport will
                // never try to open an interface with a wild-card.
                //
#if defined(QCC_OS_ANDROID)
                if (entries[i].m_name.find("p2p") == qcc::String::npos) {
                    QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): Use entry \"%s\" since not a P2P interface",
                                   entries[i].m_name.c_str()));
                    useEntry = true;
                }
#else
                //
                // There is no such thing as a "special use" interface on any of
                // our other platforms, so we always use them.
                //
                QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): Use entry \"%s\" since P2P not supported",
                               entries[i].m_name.c_str()));
                useEntry = true;
#endif
            } else {
                m_mutex.Lock();

                QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): m_any not set, look for explicitly requested interfaces for transport %d (%d currently requested)", j, m_requestedInterfaces[j].size()));
                for (uint32_t k = 0; k < m_requestedInterfaces[j].size(); ++k) {

                    QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): Check out requested interfaces \"%s\"",
                                   m_requestedInterfaces[j][k].m_interfaceName.c_str()));
                    //
                    // If the current real interface name matches the name in the
                    // requestedInterface list, we will try to use it.
                    //
                    if (m_requestedInterfaces[j][k].m_interfaceName.size() != 0 &&
                        m_requestedInterfaces[j][k].m_interfaceName == entries[i].m_name) {
                        QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): Use because found requestedInterface name "
                                       " \"%s\" for transport %d", entries[i].m_name.c_str(), j));
                        useEntry = true;
                        break;
                    }

                    //
                    // If the current real interface IP Address matches the name in
                    // the requestedInterface list, we will try to use it.
                    //
                    if (m_requestedInterfaces[j][k].m_interfaceName.size() == 0 &&
                        m_requestedInterfaces[j][k].m_interfaceAddr == qcc::IPAddress(entries[i].m_addr)) {
                        QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): Use because found requestedInterface address "
                                       "\"%s\" for transport %d.", entries[i].m_addr.c_str(), i));
                        useEntry = true;
                        break;
                    }
                }

                m_mutex.Unlock();
            }
        }

        //
        // If we aren't configured to use this entry, or have no idea how to use
        // this entry (not AF_INET or AF_INET6), try the next one.
        //
        if (useEntry == false || (entries[i].m_family != qcc::QCC_AF_INET && entries[i].m_family != qcc::QCC_AF_INET6)) {
            QCC_DbgPrintf(("IpNameServiceImpl::LazyUpdateInterfaces(): Won't use this IfConfig entry"));
            continue;
        }

        //
        // If we fall through to here, we have decided that the host configured
        // entries[i] interface describes an interface we want to use to send
        // and receive our name service messages over.  We keep a list of "live"
        // interfaces that reflect the interfaces we've previously made the
        // decision to use, so we'll set up a socket and move it there.  We have
        // to be careful about what kind of socket we are going to use for each
        // entry (IPv4 or IPv6) and whether or not multicast is actually supported
        // on the interface.
        //
        // This next condition may be a bit confusing, so we break it out a bit
        // for clarity.  We can posibly use an interface if it supports either
        // loopback, multicast, or broadcast.  What we want to do is to detect
        // the condition when we cannot use it, so we invert the logic.  That
        // means !multicast && !broadcast && !loopback.  Not being able to
        // support broadcast is also true if we don't want to (i.e., m_broadcast
        // is false).  This expression then looks like !loopback && !multicast &&
        // (!broadcast || !m_broadcast).  broadcast really implies AF_INET since
        // there is no broadcast in IPv6 but we double-check this condition and
        // come up with:
        //
        //   !loopback && !multicast && (!broadcast || !m_broadcast || !AF_INET).
        //
        // To avoid a horribly complicated if statement, we make it look like
        // the above explanation.  The resulting debug print is intimidating,
        // but it says exactly the right thing for those in the know.
        //
        bool loopback = (entries[i].m_flags & qcc::IfConfigEntry::LOOPBACK) != 0;
        bool multicast = (entries[i].m_flags & qcc::IfConfigEntry::MULTICAST) != 0;
        bool broadcast = (entries[i].m_flags & qcc::IfConfigEntry::BROADCAST) != 0;
        bool af_inet = entries[i].m_family == qcc::QCC_AF_INET;

        if (!loopback && !multicast && (!broadcast || !m_broadcast || !af_inet)) {
            QCC_DbgPrintf(("LazyUpdateInterfaces: !loopback && !multicast && (!broadcast || !m_broadcast || !af_inet).  Ignoring"));
            continue;
        }

        //
        // We've decided the interface in question is interesting and we want to
        // use it to send and receive name service messages.  Now we need to
        // start the long process of convincing the network to do what we want.
        // This is going to mostly be done by setting a series of socket
        // options.  The small number of the ones we need are absracted in the
        // qcc package.
        // We set up 3 sockets - one to listen for Multicast NS packets, one for MDNS packets
        // and 1 for unicast MDNS packets.
        //
        qcc::SocketFd multicastMDNSsockFd = qcc::INVALID_SOCKET_FD;
        qcc::SocketFd multicastsockFd = qcc::INVALID_SOCKET_FD;

        if (entries[i].m_family != qcc::QCC_AF_INET && entries[i].m_family != qcc::QCC_AF_INET6) {
            assert(!"IpNameServiceImpl::LazyUpdateInterfaces(): Unexpected value in m_family (not AF_INET or AF_INET6");
            continue;
        }

        status = CreateMulticastSocket(entries[i], IPV4_MDNS_MULTICAST_GROUP, IPV6_MDNS_MULTICAST_GROUP, MULTICAST_MDNS_PORT,
                                       m_broadcast, multicastMDNSsockFd);
        if (status != ER_OK) {
            QCC_DbgPrintf(("Failed to create multicast socket for MDNS packets."));
            continue;
        }

        status = CreateMulticastSocket(entries[i], IPV4_ALLJOYN_MULTICAST_GROUP, IPV6_ALLJOYN_MULTICAST_GROUP, MULTICAST_PORT,
                                       m_broadcast, multicastsockFd);
        if (status != ER_OK) {
            QCC_DbgPrintf(("Failed to create multicast socket for NS packets."));
            qcc::Close(multicastMDNSsockFd);
            continue;
        }

        //
        // Now take the interface "live."
        //
        LiveInterface live;
        live.m_interfaceName = entries[i].m_name;
        live.m_interfaceAddr = entries[i].m_addr;
        live.m_prefixlen = entries[i].m_prefixlen;
        live.m_address = qcc::IPAddress(entries[i].m_addr);
        live.m_flags = entries[i].m_flags;
        live.m_mtu = entries[i].m_mtu;
        live.m_index = entries[i].m_index;

        live.m_multicastsockFd = multicastsockFd;
        live.m_multicastMDNSsockFd = multicastMDNSsockFd;

        live.m_multicastPort = MULTICAST_PORT;
        live.m_multicastMDNSPort = MULTICAST_MDNS_PORT;

        if (multicastsockFd != qcc::INVALID_SOCKET_FD) {
            live.m_multicastevent = new qcc::Event(multicastsockFd, qcc::Event::IO_READ);
        }
        if (multicastMDNSsockFd != qcc::INVALID_SOCKET_FD) {
            live.m_multicastMDNSevent = new qcc::Event(multicastMDNSsockFd, qcc::Event::IO_READ);
        }

        QCC_DbgPrintf(("Pushing back interface %s addr %s", live.m_interfaceName.c_str(), entries[i].m_addr.c_str()));
        //
        // Lazy update is called with the mutex taken, so this is safe here.
        //
        m_liveInterfaces.push_back(live);
    }
    if (m_liveInterfaces.size() > 0) {
        if (m_ipv4UnicastSockFd == qcc::INVALID_SOCKET_FD) {
            CreateUnicastSocket();
            m_unicastEvent = new qcc::Event(m_ipv4UnicastSockFd, qcc::Event::IO_READ);
        }
        IPAddress listenAddr;
        uint16_t listenPort = 0;
        if (m_ipv4UnicastSockFd != qcc::INVALID_SOCKET_FD) {
            qcc::GetLocalAddress(m_ipv4UnicastSockFd, listenAddr, listenPort);
        }
        for (uint32_t i = 0; (m_state == IMPL_RUNNING || m_terminal) && (i < m_liveInterfaces.size()); ++i) {
            m_liveInterfaces[i].m_unicastPort = listenPort;
        }
    } else {
        if (m_unicastEvent) {
            delete m_unicastEvent;
            m_unicastEvent = NULL;
        }
        if (m_ipv4UnicastSockFd != qcc::INVALID_SOCKET_FD) {
            qcc::Close(m_ipv4UnicastSockFd);
            m_ipv4UnicastSockFd = qcc::INVALID_SOCKET_FD;
        }
    }

    // Schedule the processing of the transports'
    // network event callbacks on the network event
    // packet scheduler thread.
    processAnyTransport = false;
    for (uint32_t i = 0; i < N_TRANSPORTS; ++i) {
        if (m_processTransport[i]) {
            m_doNetworkCallback[i] = true;
            m_processTransport[i] = false;
            processAnyTransport = true;
        }
    }
    if (processAnyTransport) {
        m_packetScheduler.Alert();
    }

    if (m_refreshAdvertisements) {
        QCC_DbgHLPrintf(("Now refreshing advertisements on interface event"));
        m_timer = m_tRetransmit + 1;
        m_networkChangeScheduleCount = 0;
        std::map<qcc::String, qcc::IPAddress> ifMap;
        for (std::set<uint32_t>::const_iterator it = networkEvents.begin(); it != networkEvents.end(); it++) {
            m_networkEvents.insert(*it);
        }
        m_packetScheduler.Alert();
        m_refreshAdvertisements = false;
    }
}

QStatus IpNameServiceImpl::Enable(TransportMask transportMask,
                                  const std::map<qcc::String, uint16_t>& reliableIPv4PortMap, uint16_t reliableIPv6Port,
                                  const std::map<qcc::String, uint16_t>& unreliableIPv4PortMap, uint16_t unreliableIPv6Port,
                                  bool enableReliableIPv4, bool enableReliableIPv6,
                                  bool enableUnreliableIPv4, bool enableUnreliableIPv6)
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::Enable(0x%x, %d., %d., %d., %d., %d, %d, %d, %d )", transportMask,
                     reliableIPv4PortMap.size(), reliableIPv6Port, unreliableIPv4PortMap.size(), unreliableIPv6Port,
                     enableReliableIPv4, enableReliableIPv6, enableUnreliableIPv4, enableUnreliableIPv6));

    //
    // Exactly one bit must be set in a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::Enable(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    uint32_t i = IndexFromBit(transportMask);
    assert(i < 16 && "IpNameServiceImpl::Enable(): Bad callback index");

    if (i >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    //
    // This is a bit non-intuitive.  We have to disable the name service (stop
    // listening on the sockets for the multicast groups) to pass the Android
    // Compatibility Test.  We have to make sure that if we are disabling the
    // name service by removing its last advertisement, we leave ourselves up
    // for long enough to get the last cancel advertised name out.
    //
    // We synchronize with the main run thread which will do that work by
    // requesting it to enable or disable, and it figures out the right thing
    // to do with respect to the advertised names.
    //
    // We keep track of what is going on with two variables:
    //
    //     <somethingWasEnabled> tells us if there was an enabled port somewhere
    //         before we started.
    //
    //     <enabling> tells us if this operation is to enable or disable some
    //         port.
    //
    m_mutex.Lock();
    bool somethingWasEnabled = false;
    for (uint32_t j = 0; j < N_TRANSPORTS; ++j) {
        if (m_enabledReliableIPv4[j] || m_enabledUnreliableIPv4[j] || m_enabledReliableIPv6[j] || m_enabledUnreliableIPv6[j]) {
            somethingWasEnabled = true;
        }
    }

    bool enabling = enableReliableIPv4 || enableUnreliableIPv4 || enableReliableIPv6 || enableUnreliableIPv6;

    //
    // If enabling is true, then we need to cancel any pending disables since
    // the name service needs to be alive and we absolutely don't want to do a
    // pending shutdown sequence if it is queued.
    //
    if (enabling) {
        m_doDisable = false;

        //
        // If we weren't already enabled, then we certainly want to be so
        // since we know we're going to add a port listener in a moment.
        //
        if (somethingWasEnabled == false) {
            m_doEnable = true;
        }
    }

    // Keep a backup copy of the state so we can correctly
    // send out cancel advertisements. By the time cancel
    // advertise packets are scheduled for transmission and
    // the packets are rewritten, the relevant transport may
    // no longer be enabled.
    m_priorReliableIPv4PortMap[i] = m_reliableIPv4PortMap[i];
    m_priorUnreliableIPv4PortMap[i] = m_unreliableIPv4PortMap[i];

    std::map<qcc::String, uint16_t>::const_iterator it = reliableIPv4PortMap.find("*");
    if (it != reliableIPv4PortMap.end()) {
        if (enableReliableIPv4) {
            m_reliableIPv4PortMap[i].clear();
            m_reliableIPv4PortMap[i]["*"] = it->second;
        } else {
            m_reliableIPv4PortMap[i].clear();
        }
    } else {
        for (it = reliableIPv4PortMap.begin(); it != reliableIPv4PortMap.end(); it++) {
            if (enableReliableIPv4) {
                m_reliableIPv4PortMap[i][it->first] = it->second;
            } else {
                m_reliableIPv4PortMap[i].erase(it->first);
            }
        }
    }

    it = unreliableIPv4PortMap.find("*");
    if (it != unreliableIPv4PortMap.end()) {
        if (enableUnreliableIPv4) {
            m_unreliableIPv4PortMap[i].clear();
            m_unreliableIPv4PortMap[i]["*"] = it->second;
        } else {
            m_unreliableIPv4PortMap[i].clear();
        }
    } else {
        for (it = unreliableIPv4PortMap.begin(); it != unreliableIPv4PortMap.end(); it++) {
            if (enableUnreliableIPv4) {
                m_unreliableIPv4PortMap[i][it->first] = it->second;
            } else {
                m_unreliableIPv4PortMap[i].erase(it->first);
            }
        }
    }

    m_reliableIPv6Port[i] = reliableIPv6Port;
    m_unreliableIPv6Port[i] = reliableIPv6Port;

    m_enabledReliableIPv4[i] = !m_reliableIPv4PortMap[i].empty();
    m_enabledUnreliableIPv4[i] = !m_unreliableIPv4PortMap[i].empty();
    m_enabledReliableIPv6[i] = enableReliableIPv6;
    m_enabledUnreliableIPv6[i] = enableUnreliableIPv6;
    //
    // We might be wanting to disable the name service depending on whether we
    // end up disabling the last of the enabled ports.
    //
    bool somethingIsEnabled = false;
    for (uint32_t j = 0; j < N_TRANSPORTS; ++j) {
        if (m_enabledReliableIPv4[j] || m_enabledUnreliableIPv4[j] || m_enabledReliableIPv6[j] || m_enabledUnreliableIPv6[j]) {
            somethingIsEnabled = true;
        }
    }

    //
    // If the end result of doing the operation above ends up that there are no
    // longer any enabled ports, the name service definitely needs to end up
    // disabled.  Therefore we need to cancel any any pending enable requests.
    //
    if (somethingIsEnabled == false) {
        m_doEnable = false;

        //
        // If we weren't already disabled, and we are then we certainly want to be so
        // since we know we just deleted the  going to add a port listener in a moment.
        //
        if (somethingWasEnabled == true) {
            m_doDisable = true;
        }
    }
    m_mutex.Unlock();

    m_forceLazyUpdate = true;
    m_wakeEvent.SetEvent();

    return ER_OK;
}

QStatus IpNameServiceImpl::Enabled(TransportMask transportMask,
                                   std::map<qcc::String, uint16_t>& reliableIPv4PortMap, uint16_t& reliableIPv6Port,
                                   std::map<qcc::String, uint16_t>& unreliableIPv4PortMap, uint16_t& unreliableIPv6Port)
{
    QCC_DbgPrintf(("IpNameServiceImpl::Enabled()"));

    //
    // Exactly one bit must be set in a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::Enable(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    uint32_t i = IndexFromBit(transportMask);
    assert(i < 16 && "IpNameServiceImpl::Enabled(): Bad callback index");

    if (i >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    m_mutex.Lock();
    reliableIPv4PortMap = m_reliableIPv4PortMap[i];
    unreliableIPv4PortMap = m_unreliableIPv4PortMap[i];
    reliableIPv6Port = m_reliableIPv6Port[i];
    unreliableIPv6Port = m_unreliableIPv6Port[i];
    m_mutex.Unlock();

    return ER_OK;
}

void IpNameServiceImpl::TriggerTransmission(Packet packet)
{
    BurstResponseHeader brh(packet);

    uint32_t nsVersion, msgVersion;
    packet->GetVersion(nsVersion, msgVersion);
    assert(m_enableV1 || (msgVersion != 0 && msgVersion != 1));

    //Queue one instance of the packet, the rest will be taken care of by the PacketScheduler thread
    //QueueProtocolMessage limits the maximum number of outstanding packets to MAX_IPNS_MESSAGES.
    //Limiting m_burstQueue size could posssibly lead to stalls of up to 18 seconds (RETRY_INTERVALS).
    QueueProtocolMessage(packet);
    m_mutex.Lock();
    Timespec now;
    GetTimeNow(&now);

    brh.nextScheduleTime = now + BURST_RESPONSE_INTERVAL;
    m_burstQueue.push_back(brh);

    m_packetScheduler.Alert();
    m_mutex.Unlock();
}

QStatus IpNameServiceImpl::FindAdvertisement(TransportMask transportMask, const qcc::String& matchingStr, LocatePolicy policy, TransportMask completeTransportMask)
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::FindAdvertisement(0x%x, \"%s\", %d)", transportMask, matchingStr.c_str(), policy));

    //
    // Exactly one bit must be set in a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::FindAdvertisement(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    uint32_t transportIndex = IndexFromBit(transportMask);

    if (transportIndex >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    MatchMap matching;
    ParseMatchRule(matchingStr, matching);

    //
    // Only version 2 supports more than just the name key.
    //
    uint8_t type = TRANSMIT_V2;
    MatchMap::iterator name = matching.find("name");
    if (m_enableV1 && (matching.size() == 1) && (name != matching.end())) {
        type |= TRANSMIT_V0_V1;
    }

    //
    // Send a request to the network over our multicast channel, asking for
    // anyone who supports the specified well-known name.
    //
    // We are now at version one of the protocol.  There is no significant
    // difference between version zero and version one messages, but down-version
    // (version zero) clients don't know that, so they will ignore version one
    // messages.  This means that if we want to have clients running older daemons
    // be able to hear our discovery requests, we need to send both flavors of
    // message.  Since the version is located in the message header, this means
    // two messages.

    //
    // Do it once for version two.
    //
    if (type & TRANSMIT_V2) {
        m_v2_queries[transportIndex].insert(matchingStr);
        uint32_t secondOfPairIndex = IndexFromBit(TRANSPORT_SECOND_OF_PAIR);
        bool isFirstOfPair = (transportMask == TRANSPORT_FIRST_OF_PAIR);
        bool isSecondOfPair = (transportMask == TRANSPORT_SECOND_OF_PAIR);
        bool isFirstOfPairRequested = ((completeTransportMask & TRANSPORT_FIRST_OF_PAIR) == TRANSPORT_FIRST_OF_PAIR);
        bool isSecondOfPairRequested = ((completeTransportMask & TRANSPORT_SECOND_OF_PAIR) == TRANSPORT_SECOND_OF_PAIR);

        // If this is the first of the pair, only send if second is not requested in the complete transport mask
        bool sendForFirstOfPair = (isFirstOfPair && !isSecondOfPairRequested);

        // If this is the second of the pair of transports, send if this transport is enabled or the first transport was requested.
        bool sendForSecondOfPair = (isSecondOfPair && (isFirstOfPairRequested || m_enabledUnreliableIPv4[secondOfPairIndex]));

        if (sendForFirstOfPair || sendForSecondOfPair) {


            MDNSPacket query;

            MDNSSearchRData* searchRData = new MDNSSearchRData();
            for (MatchMap::iterator it = matching.begin(); it != matching.end(); ++it) {
                searchRData->SetValue(it->first, it->second);
            }
            MDNSResourceRecord searchRecord("search." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, searchRData);
            query->AddAdditionalRecord(searchRecord);

            Query(completeTransportMask, query);
            delete searchRData;
        }
    }
    //
    // Do it once for version zero.
    //
    if ((type & TRANSMIT_V0_V1) && (transportMask != TRANSPORT_UDP)) {
        m_v0_v1_queries[transportIndex].insert(name->second);

        WhoHas whoHas;

        //
        // We understand all messages from version zero to version one, but we
        // are sending a version zero message.  The whole point of sending a
        // version zero message is that can be understood by down-level code
        // so we can't use the new versioning scheme.  We have to use some
        // sneaky way to tell an in-the know version one client that the
        // packet is from a version one client and that is through the setting
        // of the UDP flag.
        //
        whoHas.SetVersion(0, 0);
        whoHas.SetTransportMask(transportMask);

        //
        // We have to use some sneaky way to tell an in-the know version one
        // client that the packet is from a version one client and that is
        // through the setting of the UDP flag.  TCP transports are the only
        // possibility for version zero packets and it always sets the TCP
        // flag, of course.
        //
        whoHas.SetTcpFlag(true);
        whoHas.SetUdpFlag(true);

        whoHas.SetIPv4Flag(true);
        whoHas.AddName(name->second);

        NSPacket nspacket;
        nspacket->SetVersion(0, 0);
        nspacket->SetTimer(m_tDuration);
        nspacket->AddQuestion(whoHas);

        m_mutex.Lock();
        // Search for the same name in the burstQueue.
        // If present, remove the entry to preserve the ordering of outgoing packets.
        std::list<BurstResponseHeader>::iterator it = m_burstQueue.begin();
        while (it != m_burstQueue.end()) {
            uint32_t nsVersion;
            uint32_t msgVersion;

            (*it).packet->GetVersion(nsVersion, msgVersion);
            if (nsVersion == 0 && msgVersion == 0) {
                NSPacket temp = NSPacket::cast((*it).packet);
                if (temp->GetQuestion(0).GetName(0) == name->second) {
                    m_burstQueue.erase(it++);
                    continue;
                }
            }
            it++;
        }
        m_mutex.Unlock();
        TriggerTransmission(Packet::cast(nspacket));
    }
    //
    // Do it again for version one.
    //
    if ((type & TRANSMIT_V0_V1) && (transportMask != TRANSPORT_UDP)) {
        WhoHas whoHas;

        //
        // We understand all messages from version zero to version one, and we
        // are sending a version one message.
        //
        whoHas.SetVersion(1, 1);
        whoHas.SetTransportMask(transportMask);
        whoHas.AddName(name->second);

        NSPacket nspacket;
        nspacket->SetVersion(1, 1);
        nspacket->SetTimer(m_tDuration);
        nspacket->AddQuestion(whoHas);

        m_mutex.Lock();
        // Search for the same name in the burstQueue.
        // If present, remove the entry to preserve the ordering of outgoing packets.
        std::list<BurstResponseHeader>::iterator it = m_burstQueue.begin();
        while (it != m_burstQueue.end()) {
            uint32_t nsVersion;
            uint32_t msgVersion;

            (*it).packet->GetVersion(nsVersion, msgVersion);
            if (nsVersion == 1 && msgVersion == 1) {
                NSPacket temp = NSPacket::cast((*it).packet);
                if (temp->GetQuestion(0).GetName(0) == name->second) {
                    m_burstQueue.erase(it++);
                    continue;
                }
            }
            it++;
        }
        m_mutex.Unlock();
        TriggerTransmission(Packet::cast(nspacket));
    }



    return ER_OK;
}

QStatus IpNameServiceImpl::CancelFindAdvertisement(TransportMask transportMask, const qcc::String& matchingStr, LocatePolicy policy, TransportMask completeTransportMask)
{
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::CancelFindAdvertisement(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    uint32_t transportIndex = IndexFromBit(transportMask);
    if (transportIndex >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    MatchMap matching;
    ParseMatchRule(matchingStr, matching);
    //
    // Only version 2 supports more than just the name key.
    //
    bool nameOnly = false;
    MatchMap::iterator name = matching.find("name");
    if ((matching.size() == 1) && (name != matching.end())) {
        nameOnly = true;
    }

    m_mutex.Lock();
    if (m_enableV1 && nameOnly) {
        m_v0_v1_queries[transportIndex].erase(name->second);
    }

    m_v2_queries[transportIndex].erase(matchingStr);

    m_mutex.Unlock();
    return ER_OK;
}
const uint32_t MIN_THRESHOLD_CACHE_REFRESH_MS = 1000;

// Purge entries from PeerInfo map that havent recieved a response
// for 3 Cache refresh cycles i.e. 3 * 120 seconds.
const uint32_t PEER_INFO_MAP_PURGE_TIMEOUT = 3 * 120 * 1000;
QStatus IpNameServiceImpl::RefreshCache(TransportMask transportMask, const qcc::String& guid, const qcc::String& matchingStr, LocatePolicy policy, bool ping) {
    QCC_DbgHLPrintf(("IpNameServiceImpl::RefreshCache(0x%x, \"%s\", %d)", transportMask, matchingStr.c_str(), policy));
    QCC_DbgPrintf(("IpNameServiceImpl::RefreshCache %s", matchingStr.c_str()));
    String longGuid;
    MatchMap matching;
    ParseMatchRule(matchingStr, matching);
    //
    // We first retrieve the destination for the guid from the PeerInfoMap and set the destination for the
    // MDNS packet that we will be sending out over unicast to this guid
    //
    m_mutex.Lock();
    std::unordered_map<qcc::String, std::set<PeerInfo>, Hash, Equal>::iterator it = m_peerInfoMap.end();
    if (!ping) {
        it = m_peerInfoMap.find(guid);
        longGuid = guid;
    } else {
        for (std::unordered_map<qcc::String, std::set<PeerInfo>, Hash, Equal>::iterator i = m_peerInfoMap.begin();
             i != m_peerInfoMap.end(); ++i) {
            if (qcc::GUID128(i->first).ToShortString() == guid) {
                it = i;
                longGuid = it->first;
                break;
            }
        }
    }
    // the guid was not found in the m_peerInfoMap the name is unknown.
    if (it != m_peerInfoMap.end()) {
        std::set<PeerInfo>::iterator pit = it->second.begin();
        PrintPeerInfoMap();
        // The check here is because we could be in a session with a name and there could be no valid peer info for it
        // The name will be removed by layer above when we are no longer in a session with that name and it is no longer advertised
        if (!it->second.empty()) {
            Timespec now;
            GetTimeNow(&now);
            QCC_DbgPrintf(("Entry found in Peer Info Map. Setting unicast destination"));

            while (pit != it->second.end()) {
                PeerInfo peerInfo = *pit;
                if (!ping && ((now - (*pit).lastQueryTimeStamp) < MIN_THRESHOLD_CACHE_REFRESH_MS)) {
                    ++pit;
                    continue;
                }
                if (!ping) {
                    // Purge entries from PeerInfo map that havent recieved a response for 3 Cache refresh cycles
                    if ((now - (*pit).lastResponseTimeStamp) >= PEER_INFO_MAP_PURGE_TIMEOUT) {
                        it->second.erase(pit++);
                        continue;
                    }
                    (*pit).lastQueryTimeStamp = now;
                }

                MDNSPacket query;
                query->SetDestination((*pit).unicastInfo);
                MDNSSearchRData* searchRData = new MDNSSearchRData();
                for (MatchMap::iterator it1 = matching.begin(); it1 != matching.end(); ++it1) {
                    searchRData->SetValue(it1->first, it1->second);
                }

                if (ping) {
                    MDNSPingRData* pingRData = new MDNSPingRData();
                    for (MatchMap::iterator it1 = matching.begin(); it1 != matching.end(); ++it1) {
                        pingRData->SetValue("n", it1->second);
                    }
                    MDNSResourceRecord pingRecord("ping." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, pingRData);
                    query->AddAdditionalRecord(pingRecord);
                    delete pingRData;
                }

                MDNSResourceRecord searchRecord("search." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, searchRData);
                query->AddAdditionalRecord(searchRecord);
                delete searchRData;
                m_mutex.Unlock();
                Query(transportMask, query);
                m_mutex.Lock();
                it = m_peerInfoMap.find(longGuid);
                if (it == m_peerInfoMap.end()) {
                    break;
                }
                pit = it->second.upper_bound(peerInfo);
            }
        }
    } else {
        if (ping) {
            m_mutex.Unlock();
            return ER_ALLJOYN_PING_REPLY_INCOMPATIBLE_REMOTE_ROUTING_NODE;
        }
        QCC_DbgPrintf((" IpNameServiceImpl::RefreshCache(): Entry not found in PeerInfoMap"));
    }
    m_mutex.Unlock();

    return ER_OK;
}

void IpNameServiceImpl::SetCriticalParameters(
    uint32_t tDuration,
    uint32_t tRetransmit,
    uint32_t tQuestion,
    uint32_t modulus,
    uint32_t retries)
{
    m_tDuration = tDuration;
    m_tRetransmit = tRetransmit;
    m_tQuestion = tQuestion;
    m_modulus = modulus;
    m_retries = retries;
}

QStatus IpNameServiceImpl::SetCallback(TransportMask transportMask,
                                       Callback<void, const qcc::String&, const qcc::String&, vector<qcc::String>&, uint32_t>* cb)
{
    QCC_DbgPrintf(("IpNameServiceImpl::SetCallback()"));

    //
    // Exactly one bit must be set in a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::SetCallback(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    uint32_t i = IndexFromBit(transportMask);
    assert(i < 16 && "IpNameServiceImpl::SetCallback(): Bad callback index");
    if (i >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    m_mutex.Lock();
    // Wait till the callback is in use.
    while (m_protect_callback) {
        m_mutex.Unlock();
        qcc::Sleep(2);
        m_mutex.Lock();
    }

    Callback<void, const qcc::String&, const qcc::String&, vector<qcc::String>&, uint32_t>*  goner = m_callback[i];
    m_callback[i] = NULL;
    delete goner;
    m_callback[i] = cb;

    m_mutex.Unlock();

    return ER_OK;
}

QStatus IpNameServiceImpl::SetNetworkEventCallback(TransportMask transportMask,
                                                   Callback<void, const std::map<qcc::String, qcc::IPAddress>&>* cb)
{
    QCC_DbgPrintf(("IpNameServiceImpl::SetNetworkEventCallback()"));

    //
    // Exactly one bit must be set in a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::SetNetworkEventCallback(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    uint32_t i = IndexFromBit(transportMask);
    assert(i < 16 && "IpNameServiceImpl::SetNetworkEventCallback(): Bad callback index");
    if (i >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    m_mutex.Lock();
    // Wait till the callback is in use.
    while (m_protect_net_callback) {
        m_mutex.Unlock();
        qcc::Sleep(2);
        m_mutex.Lock();
    }

    Callback<void, const std::map<qcc::String, qcc::IPAddress>&>*  goner = m_networkEventCallback[i];
    m_networkEventCallback[i] = NULL;
    delete goner;
    m_networkEventCallback[i] = cb;

    m_mutex.Unlock();

    return ER_OK;
}

void IpNameServiceImpl::ClearCallbacks(void)
{
    QCC_DbgPrintf(("IpNameServiceImpl::ClearCallbacks()"));

    m_mutex.Lock();
    // Wait till the callback is in use.
    while (m_protect_callback) {
        m_mutex.Unlock();
        qcc::Sleep(2);
        m_mutex.Lock();
    }

    //
    // Delete any callbacks that any users of this class may have set.
    //
    for (uint32_t i = 0; i < N_TRANSPORTS; ++i) {
        Callback<void, const qcc::String&, const qcc::String&, vector<qcc::String>&, uint32_t>*  goner = m_callback[i];
        m_callback[i] = NULL;
        delete goner;
    }

    m_mutex.Unlock();
}

void IpNameServiceImpl::ClearNetworkEventCallbacks(void)
{
    QCC_DbgPrintf(("IpNameServiceImpl::ClearNetworkEventCallbacks()"));

    m_mutex.Lock();
    // Wait till the callback is in use.
    while (m_protect_net_callback) {
        m_mutex.Unlock();
        qcc::Sleep(2);
        m_mutex.Lock();
    }

    //
    // Delete any callbacks that any users of this class may have set.
    //
    for (uint32_t i = 0; i < N_TRANSPORTS; ++i) {
        Callback<void, const std::map<qcc::String, qcc::IPAddress>&>*  goner = m_networkEventCallback[i];
        m_networkEventCallback[i] = NULL;
        delete goner;
    }

    m_mutex.Unlock();
}

size_t IpNameServiceImpl::NumAdvertisements(TransportMask transportMask)
{
    QCC_DbgPrintf(("IpNameServiceImpl::NumAdvertisements()"));

    //
    // Exactly one bit must be set in a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::NumAdvertisements(): Bad transport mask"));
        return 0;
    }

    uint32_t i = IndexFromBit(transportMask);
    assert(i < 16 && "IpNameServiceImpl::NumAdvertisements(): Bad callback index");
    if (i >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    return m_advertised[i].size();
}

QStatus IpNameServiceImpl::AdvertiseName(TransportMask transportMask, const qcc::String& wkn, bool quietly, TransportMask completeTransportMask)
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::AdvertiseName(0x%x, \"%s\", %d)", transportMask, wkn.c_str(), quietly));

    vector<qcc::String> wknVector;
    wknVector.push_back(wkn);

    return AdvertiseName(transportMask, wknVector, quietly, completeTransportMask);
}

QStatus IpNameServiceImpl::AdvertiseName(TransportMask transportMask, vector<qcc::String>& wkn, bool quietly, TransportMask completeTransportMask)
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::AdvertiseName(0x%x, 0x%p, %d)", transportMask, &wkn, quietly));

    //
    // Exactly one bit must be set in a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::AdvertiseName(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    uint32_t transportIndex = IndexFromBit(transportMask);
    assert(transportIndex < 16 && "IpNameServiceImpl::AdvertiseName(): Bad transport index");
    if (transportIndex >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("IpNameServiceImpl::AdvertiseName(): Not IMPL_RUNNING"));
        return ER_FAIL;
    }

    //
    // There are at least two threads wandering through the advertised list.
    // We are running short on toes, so don't shoot any more off by not being
    // thread-unaware.
    //
    m_mutex.Lock();

    //
    // Make a note to ourselves which services we are advertising so we can
    // respond to protocol questions in the future.  Only allow one entry per
    // name.  We keep separate lists of quietly advertised names and actively
    // advertised names since it makes it easy to decide which names go in
    // periodic keep-alive advertisements.
    //
    if (quietly) {
        for (uint32_t i = 0; i < wkn.size(); ++i) {
            set<qcc::String>::iterator j = find(m_advertised_quietly[transportIndex].begin(), m_advertised_quietly[transportIndex].end(), wkn[i]);
            if (j == m_advertised_quietly[transportIndex].end()) {
                m_advertised_quietly[transportIndex].insert(wkn[i]);
            } else {
                //
                // Nothing has changed, so don't bother.
                //
                QCC_DbgPrintf(("IpNameServiceImpl::AdvertiseName(): Duplicate advertisement"));
                m_mutex.Unlock();
                return ER_OK;
            }
        }

        //
        // Since we are advertising quietly, we need to quietly return without
        // advertising the name, which would happen if we just fell out of the
        // if-else.
        //
        m_mutex.Unlock();
        return ER_OK;
    } else {
        for (uint32_t i = 0; i < wkn.size(); ++i) {
            set<qcc::String>::iterator j = find(m_advertised[transportIndex].begin(), m_advertised[transportIndex].end(), wkn[i]);
            if (j == m_advertised[transportIndex].end()) {
                m_advertised[transportIndex].insert(wkn[i]);
            } else {
                //
                // Nothing has changed, so don't bother.
                //
                QCC_DbgPrintf(("IpNameServiceImpl::AdvertiseName(): Duplicate advertisement"));

                m_mutex.Unlock();
                return ER_OK;
            }
        }


        //
        // If the advertisement retransmission timer is cleared, then set us
        // up to retransmit.  This has to be done with the mutex locked since
        // the main thread is playing with this value as well.
        //
        if (m_timer == 0) {
            m_timer = m_tDuration;
        }
    }

    m_mutex.Unlock();

    //
    // We are now at version one of the protocol.  There is a significant
    // difference between version zero and version one messages, so down-version
    // (version zero) clients will not know what to do with version one
    // messages.  This means that if we want to have clients running older
    // daemons be able to hear our advertisements, we need to send both flavors
    // of message.  Since the version is located in the message header, this
    // means two messages.

    //
    // Do it once for version two.
    //
    uint32_t secondOfPairIndex = IndexFromBit(TRANSPORT_SECOND_OF_PAIR);

    bool isFirstOfPair = (transportMask == TRANSPORT_FIRST_OF_PAIR);
    bool isSecondOfPair = (transportMask == TRANSPORT_SECOND_OF_PAIR);
    bool isFirstOfPairRequested = ((completeTransportMask & TRANSPORT_FIRST_OF_PAIR) == TRANSPORT_FIRST_OF_PAIR);
    bool isSecondOfPairRequested = ((completeTransportMask & TRANSPORT_SECOND_OF_PAIR) == TRANSPORT_SECOND_OF_PAIR);

    // If this is the first of the pair, only send if second is not requested in the complete transport mask
    bool sendForFirstOfPair = (isFirstOfPair && !isSecondOfPairRequested);

    // If this is the second of the pair of transports, send if this transport is enabled or the first transport was requested.
    bool sendForSecondOfPair = (isSecondOfPair && (isFirstOfPairRequested || m_enabledUnreliableIPv4[secondOfPairIndex]));

    if (sendForFirstOfPair || sendForSecondOfPair) {
        //version two
        MDNSAdvertiseRData* advRData = new MDNSAdvertiseRData();
        advRData->SetTransport(completeTransportMask & (TRANSPORT_TCP | TRANSPORT_UDP));
        for (uint32_t i = 0; i < wkn.size(); ++i) {
            advRData->SetValue("name", wkn[i]);
        }
        MDNSResourceRecord advRecord("advertise." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, advRData);

        MDNSPacket mdnsPacket;
        mdnsPacket->AddAdditionalRecord(advRecord);
        mdnsPacket->SetVersion(2, 2);
        Response(completeTransportMask, 120, mdnsPacket);
        delete advRData;
    }
    //
    // Do it once for version zero.
    //
    if (m_enableV1 && (transportMask != TRANSPORT_UDP)) {
        //
        // The underlying protocol is capable of identifying both TCP and UDP
        // services.  Right now, the only possibility is TCP, so this is not
        // exposed to the user unneccesarily.
        //
        IsAt isAt;

        //
        // We understand all messages from version zero to version one, and we
        // are sending a version zero message.  The whole point of sending a
        // version zero message is that can be understood by down-level code
        // so we can't use the new versioning scheme.
        //
        isAt.SetVersion(0, 0);

        //
        // We don't actually send the transport mask in version zero packets
        // but we make a note to ourselves to let us know on behalf ow what
        // transport we will be sending.
        //
        isAt.SetTransportMask(transportMask);

        //
        // We have to use some sneaky way to tell an in-the know version one
        // client that the packet is from a version one client and that is
        // through the setting of the UDP flag.  TCP transports are the only
        // possibility for version zero packets and it always sets the TCP
        // flag, of course.
        //
        isAt.SetTcpFlag(true);
        isAt.SetUdpFlag(true);

        //
        // Always send the provided daemon GUID out with the reponse.
        //
        isAt.SetGuid(m_guid);

        //
        // Send a protocol message describing the entire list of names we have
        // for the provided protocol.
        //
        isAt.SetCompleteFlag(true);

        //
        // The only possibility for version zero is that the port is the
        // reliable IPv4 port.  When the message goes out a selected interface,
        // the protocol handler will write out the addresses according to its
        // rules.
        //
        isAt.SetPort(0);

        //
        // Add the provided names to the is-at message that will be sent out on the
        // network.
        //
        for (uint32_t i = 0; i < wkn.size(); ++i) {
            isAt.AddName(wkn[i]);
        }

        //
        // The header ties the whole protocol message together.  By setting the
        // timer, we are asking for everyone who hears the message to remember
        // the advertisements for that number of seconds.
        //
        NSPacket nspacket;
        nspacket->SetVersion(0, 0);
        nspacket->SetTimer(m_tDuration);
        nspacket->AddAnswer(isAt);

        //
        // We don't want allow the caller to advertise an unlimited number of
        // names and consume all available network resources.  We expect
        // AdvertiseName() to typically be called once per advertised name, but
        // since we allow a vector of names we need to limit that size somehow.
        // The easy way is to assume that all of the names are the maximum size
        // and just limit based on the maximum NS packet size and the maximum
        // name size of 256 bytes.  This, however, leaves just five names which
        // seems too restrictive.  So, we do it the more time-consuming way and
        // put together the message and then see if it's "too big."
        //
        // This isn't terribly elegant, but we don't know the IP address(es) over
        // which the message will be sent.  These are added in the loop that
        // actually does the packet sends, with the interface addresses dynamically
        // added onto the message.  We have no clue here if an IPv4 or IPv6 or both
        // flavors of address will exist on a given interface, nor how many
        // interfaces there are.  All we can do here is to assume the worst case for
        // the size (both exist) and add the 20 bytes (four for IPv4, sixteen for
        // IPv6) that the addresses may consume in the final packet.
        //
        if (nspacket->GetSerializedSize() + 20 <= NS_MESSAGE_MAX) {
            //
            // Queue this message for transmission out on the various live interfaces.
            //
            QueueProtocolMessage(Packet::cast(nspacket));
        } else {
            QCC_LogError(ER_PACKET_TOO_LARGE, ("IpNameServiceImpl::AdvertiseName(): Resulting NS message too large"));
            return ER_PACKET_TOO_LARGE;
        }
    }

    //
    // Do it once for version one.
    //
    if (m_enableV1 && (transportMask != TRANSPORT_UDP)) {
        IsAt isAt;

        //
        // We understand all messages from version zero to version one, and we
        // are sending a version one message;
        //
        isAt.SetVersion(1, 1);
        isAt.SetTransportMask(transportMask);

        //
        // Version one allows us to provide four possible endpoints.  The
        // address will be rewritten on the way out with the address of the
        // appropriate interface. We delay the checks for the listening ports
        // to the point at which the packet is re-written on  per-interface.
        // basis.
        //
        isAt.SetReliableIPv4("", 0);
        isAt.SetUnreliableIPv4("", 0);

        // This is a trick to make V2 NS ignore V1 packets. We set the IPv6 reliable bit,
        // that tells version two capable NS that a version two message will follow, and
        // to ignore the version one messages.
        isAt.SetReliableIPv6("", m_reliableIPv6Port[transportIndex]);

        if (m_unreliableIPv6Port[transportIndex]) {
            isAt.SetUnreliableIPv6("", m_unreliableIPv6Port[transportIndex]);
        }

        //
        // Always send the provided daemon GUID out with the reponse.
        //
        isAt.SetGuid(m_guid);

        //
        // Send a protocol message describing the entire list of names we have
        // for the provided protocol.
        //
        isAt.SetCompleteFlag(true);

        //
        // Add the provided names to the is-at message that will be sent out on the
        // network.
        //
        for (uint32_t i = 0; i < wkn.size(); ++i) {
            isAt.AddName(wkn[i]);
        }

        //
        // The header ties the whole protocol message together.  By setting the
        // timer, we are asking for everyone who hears the message to remember
        // the advertisements for that number of seconds.
        //
        NSPacket nspacket;
        nspacket->SetVersion(1, 1);
        nspacket->SetTimer(m_tDuration);
        nspacket->AddAnswer(isAt);

        //
        // We don't want allow the caller to advertise an unlimited number of
        // names and consume all available network resources.  We expect
        // AdvertiseName() to typically be called once per advertised name, but
        // since we allow a vector of names we need to limit that size somehow.
        // The easy way is to assume that all of the names are the maximum size
        // and just limit based on the maximum NS packet size and the maximum
        // name size of 256 bytes.  This, however, leaves just five names which
        // seems too restrictive.  So, we do it the more time-consuming way and
        // put together the message and then see if it's "too big."
        //
        // This isn't terribly elegant, but we don't know the IP address(es) over
        // which the message will be sent.  These are added in the loop that
        // actually does the packet sends, with the interface addresses dynamically
        // added onto the message.  We have no clue here if an IPv4 or IPv6 or both
        // flavors of address will exist on a given interface, nor how many
        // interfaces there are.  All we can do here is to assume the worst case for
        // the size (both exist) and add the 20 bytes (four for IPv4, sixteen for
        // IPv6) that the addresses may consume in the final packet.
        //
        if (nspacket->GetSerializedSize() + 20 <= NS_MESSAGE_MAX) {
            //
            // Queue this message for transmission out on the various live interfaces.
            //
            QueueProtocolMessage(Packet::cast(nspacket));
        } else {
            QCC_LogError(ER_PACKET_TOO_LARGE, ("IpNameServiceImpl::AdvertiseName(): Resulting NS message too large"));
            return ER_PACKET_TOO_LARGE;
        }
    }


    return ER_OK;
}

QStatus IpNameServiceImpl::CancelAdvertiseName(TransportMask transportMask, const qcc::String& wkn, TransportMask completeTransportMask)
{
    QCC_DbgPrintf(("IpNameServiceImpl::CancelAdvertiseName(0x%x, \"%s\")", transportMask, wkn.c_str()));

    vector<qcc::String> wknVector;
    wknVector.push_back(wkn);

    return CancelAdvertiseName(transportMask, wknVector, completeTransportMask);
}

QStatus IpNameServiceImpl::CancelAdvertiseName(TransportMask transportMask, vector<qcc::String>& wkn, TransportMask completeTransportMask)
{
    QCC_DbgPrintf(("IpNameServiceImpl::CancelAdvertiseName(0x%x, 0x%p)", transportMask, &wkn));

    //
    // Exactly one bit must be set in a transport mask in order to identify the
    // one transport (in the AllJoyn sense) that is making the request.
    //
    if (CountOnes(transportMask) != 1) {
        QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::CancelAdvertiseName(): Bad transport mask"));
        return ER_BAD_TRANSPORT_MASK;
    }

    uint32_t transportIndex = IndexFromBit(transportMask);
    assert(transportIndex < 16 && "IpNameServiceImpl::CancelAdvertiseName(): Bad transport index");

    if (transportIndex >= 16) {
        return ER_BAD_TRANSPORT_MASK;
    }

    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("IpNameServiceImpl::CancelAdvertiseName(): Not IMPL_RUNNING"));
        return ER_FAIL;
    }

    //
    // There are at least two threads wandering through the advertised list.
    // We are running short on toes, so don't shoot any more off by not being
    // thread-unaware.
    //
    m_mutex.Lock();

    //
    // Remove the given services from our list of services we are advertising.
    //
    bool changed = false;

    //
    // We cancel advertisements in either the quietly or actively advertised
    // lists through this method.  Note that it is only actively advertised
    // names that have changes in status reflected out on the network.  The
    // variable <changed> drives this network operation and so <changed> is not
    // set in the quietly advertised list even though the list was changed.
    //
    for (uint32_t i = 0; i < wkn.size(); ++i) {
        set<qcc::String>::iterator j = find(m_advertised[transportIndex].begin(), m_advertised[transportIndex].end(), wkn[i]);
        if (j != m_advertised[transportIndex].end()) {
            m_advertised[transportIndex].erase(j);
            changed = true;
        }

        set<qcc::String>::iterator k = find(m_advertised_quietly[transportIndex].begin(), m_advertised_quietly[transportIndex].end(), wkn[i]);
        if (k != m_advertised_quietly[transportIndex].end()) {
            m_advertised_quietly[transportIndex].erase(k);
        }
    }

    //
    // If we have no more advertisements, there is no need to repeatedly state
    // this so turn off the retransmit timer.  The main thread is playing with
    // this number too, so this must be done with the mutex locked.  Note that
    // the timer only reflects the presence of active advertisements.
    //
    bool activeAdvertisements = false;
    for (uint32_t i = 0; i < N_TRANSPORTS; ++i) {
        if (m_advertised[i].size()) {
            activeAdvertisements = true;
        }
    }

    if (activeAdvertisements == false) {
        m_timer = 0;
    }

    m_mutex.Unlock();

    //
    // Even though changed may be false, we may still need to send out the packet
    // since TCP is enabled.
    //

    //
    // Do it once for version two.
    //
    uint32_t secondOfPairIndex = IndexFromBit(TRANSPORT_SECOND_OF_PAIR);

    bool isFirstOfPair = (transportMask == TRANSPORT_FIRST_OF_PAIR);
    bool isSecondOfPair = (transportMask == TRANSPORT_SECOND_OF_PAIR);
    bool isFirstOfPairRequested = ((completeTransportMask & TRANSPORT_FIRST_OF_PAIR) == TRANSPORT_FIRST_OF_PAIR);
    bool isSecondOfPairRequested = ((completeTransportMask & TRANSPORT_SECOND_OF_PAIR) == TRANSPORT_SECOND_OF_PAIR);

    // If this is the first of the pair, only send if second is not requested in the complete transport mask
    bool sendForFirstOfPair = (isFirstOfPair && !isSecondOfPairRequested);

    // If this is the second of the pair of transports, send if this transport is enabled or the first transport was requested.
    bool sendForSecondOfPair = (isSecondOfPair && (isFirstOfPairRequested || m_enabledUnreliableIPv4[secondOfPairIndex]));

    if (sendForFirstOfPair || sendForSecondOfPair) {

        MDNSAdvertiseRData* advRData = new MDNSAdvertiseRData();
        advRData->SetTransport(completeTransportMask & (TRANSPORT_TCP | TRANSPORT_UDP));
        for (uint32_t i = 0; i < wkn.size(); ++i) {
            advRData->SetValue("name", wkn[i]);
        }
        MDNSResourceRecord advRecord("advertise." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 0, advRData);

        MDNSPacket mdnsPacket;
        mdnsPacket->AddAdditionalRecord(advRecord);
        mdnsPacket->SetVersion(2, 2);
        Response(completeTransportMask, 0, mdnsPacket);
        delete advRData;
    }

    //
    // If we didn't actually make a change that needs to be sent out on the
    // network, just return.
    //
    if (changed == false) {
        return ER_OK;
    }

    //
    // We are now at version one of the protocol.  There is a significant
    // difference between version zero and version one messages, so down-version
    // (version zero) clients will not know what to do with version one
    // messages.  This means that if we want to have clients running older
    // daemons be able to hear our advertisements, we need to send both flavors
    // of message.  Since the version is located in the message header, this
    // means two messages.
    //
    // Do it once for version zero.
    //
    if (m_enableV1 && (transportMask != TRANSPORT_UDP)) {
        //
        // Send a protocol answer message describing the list of names we have just
        // been asked to withdraw.
        //
        // This code assumes that the daemon talks over TCP.  True for now.
        //
        IsAt isAt;

        //
        // We understand all messages from version zero to version one, and we
        // are sending a version zero message.  The whole point of sending a
        // version zero message is that can be understood by down-level code
        // so we can't use the new versioning scheme.  We have to use some
        // sneaky way to tell an in-the know version one client that the
        // packet is from a version one client and that is through the setting
        // of the UDP flag.
        //
        isAt.SetVersion(0, 0);

        //
        // We don't actually send the transport mask in version zero packets
        // but we make a note to ourselves to let us know on behalf of what
        // transport we will be sending.
        //
        isAt.SetTransportMask(transportMask);

        //
        // We have to use some sneaky way to tell an in-the know version one
        // client that the packet is from a version one client and that is
        // through the setting of the UDP flag.  TCP transports are the only
        // possibility for version zero packets and it always sets the TCP
        // flag, of course.
        //
        isAt.SetTcpFlag(true);
        isAt.SetUdpFlag(true);

        //
        // Always send the provided daemon GUID out with the response.
        //
        isAt.SetGuid(m_guid);

        //
        // The only possibility in version zero is that the port is the reliable
        // IPv4 port.  When the message goes out a selected interface, the
        // protocol handler will write out the addresses according to its rules.
        //
        isAt.SetPort(0);

        //
        // Copy the names we are withdrawing the advertisement for into the
        // protocol message object.
        //
        for (uint32_t i = 0; i < wkn.size(); ++i) {
            isAt.AddName(wkn[i]);
        }

        //
        // When withdrawing advertisements, a complete flag means that we are
        // withdrawing all of the advertisements.  If the complete flag is
        // not set, we have some advertisements remaining.
        //
        if (m_advertised[transportIndex].size() == 0) {
            isAt.SetCompleteFlag(true);
        }

        //
        // The header ties the whole protocol message together.  We're at version
        // zero of the protocol.
        //
        NSPacket nspacket;
        nspacket->SetVersion(0, 0);

        //
        // We want to signal that everyone can forget about these names
        // so we set the timer value to 0.
        //
        nspacket->SetTimer(0);
        nspacket->AddAnswer(isAt);

        //
        // Queue this message for transmission out on the various live interfaces.
        //
        QueueProtocolMessage(Packet::cast(nspacket));
    }

    //
    // Do it once for version one.
    //
    if (m_enableV1 && (transportMask != TRANSPORT_UDP)) {
        //
        // Send a protocol answer message describing the list of names we have just
        // been asked to withdraw.
        //
        IsAt isAt;

        //
        // We understand all messages from version zero to version one, and we
        // are sending a version one message;
        //
        isAt.SetVersion(1, 1);

        //
        // Tell the other side what transport is no longer advertising these
        // names.
        //
        isAt.SetTransportMask(transportMask);

        //
        // Version one allows us to provide four possible endpoints.  The
        // address will be rewritten on the way out with the address of the
        // appropriate interface.  We delay the checks for the listening ports
        // to the point at which the packet is re-written on  per-interface.
        //
        isAt.SetReliableIPv4("", 0);
        isAt.SetUnreliableIPv4("", 0);
        // This is a trick to make V2 NS ignore V1 packets. We set the IPv6 reliable bit,
        // that tells version two capable NS that a version two message will follow, and
        // to ignore the version one messages.

        isAt.SetReliableIPv6("", m_reliableIPv6Port[transportIndex]);

        if (m_unreliableIPv6Port[transportIndex]) {
            isAt.SetUnreliableIPv6("", m_unreliableIPv6Port[transportIndex]);
        }

        //
        // Always send the provided daemon GUID out with the reponse.
        //
        isAt.SetGuid(m_guid);

        //
        // Copy the names we are withdrawing the advertisement for into the
        // protocol message object.
        //
        for (uint32_t i = 0; i < wkn.size(); ++i) {
            isAt.AddName(wkn[i]);
        }

        //
        // When withdrawing advertisements, a complete flag means that we are
        // withdrawing all of the advertisements.  If the complete flag is
        // not set, we have some advertisements remaining.
        //
        if (m_advertised[transportIndex].size() == 0) {
            isAt.SetCompleteFlag(true);
        }

        //
        // The header ties the whole protocol message together.  We're at version
        // one of the protocol.
        //
        NSPacket nspacket;
        nspacket->SetVersion(1, 1);

        //
        // We want to signal that everyone can forget about these names
        // so we set the timer value to 0.
        //
        nspacket->SetTimer(0);
        nspacket->AddAnswer(isAt);

        //
        // Queue this message for transmission out on the various live interfaces.
        //
        QueueProtocolMessage(Packet::cast(nspacket));
    }


    return ER_OK;
}

QStatus IpNameServiceImpl::Ping(TransportMask transportMask, const qcc::String& guid, const qcc::String& name)
{
    qcc::String pingString = "name='" + name + "'";
    return RefreshCache(transportMask, guid, pingString, ALWAYS_RETRY, true);
}

QStatus IpNameServiceImpl::Query(TransportMask completeTransportMask, MDNSPacket mdnsPacket)
{
    QCC_DbgPrintf(("IpNameServiceImpl::Query(0x%x, ...)", completeTransportMask));

    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("IpNameServiceImpl::Query(): Not running"));
        return ER_FAIL;
    }

    //
    // Fill in mandatory sections of query
    //
    mdnsPacket->SetVersion(2, 2);

    int32_t id = IncrementAndFetch(&INCREMENTAL_PACKET_ID);
    MDNSHeader mdnsHeader(id, MDNSHeader::MDNS_QUERY);
    mdnsPacket->SetHeader(mdnsHeader);
    if (completeTransportMask & TRANSPORT_TCP) {
        MDNSQuestion mdnsQuestion("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, MDNSResourceRecord::INTERNET);
        mdnsPacket->AddQuestion(mdnsQuestion);
    }
    if (completeTransportMask & TRANSPORT_UDP) {
        MDNSQuestion mdnsQuestion("_alljoyn._udp.local.", MDNSResourceRecord::PTR, MDNSResourceRecord::INTERNET);
        mdnsPacket->AddQuestion(mdnsQuestion);
    }
    MDNSSenderRData* refRData =  new MDNSSenderRData();
    refRData->SetSearchID(id);

    MDNSResourceRecord refRecord("sender-info." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, refRData);
    mdnsPacket->AddAdditionalRecord(refRecord);
    delete refRData;

    if (mdnsPacket->DestinationSet()) {
        QueueProtocolMessage(Packet::cast(mdnsPacket));
    } else {

        m_mutex.Lock();
        // Search for the same name in the burstQueue.
        // If present, remove the entry to preserve the ordering of outgoing packets.
        std::list<BurstResponseHeader>::iterator it = m_burstQueue.begin();
        while (it != m_burstQueue.end()) {
            uint32_t nsVersion;
            uint32_t msgVersion;

            (*it).packet->GetVersion(nsVersion, msgVersion);
            if (msgVersion == 2) {
                MDNSPacket temp = MDNSPacket::cast((*it).packet);
                if (temp->GetHeader().GetQRType() == MDNSHeader::MDNS_QUERY) {

                    if ((completeTransportMask & temp->GetTransportMask()) == completeTransportMask) {
                        MDNSResourceRecord* tmpSearchRecord;
                        temp->GetAdditionalRecord("search.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &tmpSearchRecord);
                        MDNSSearchRData* tmpSearchRData = static_cast<MDNSSearchRData*>(tmpSearchRecord->GetRData());

                        MDNSResourceRecord* searchRecord;
                        mdnsPacket->GetAdditionalRecord("search.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &searchRecord);
                        MDNSSearchRData* searchRData = static_cast<MDNSSearchRData*>(searchRecord->GetRData());

                        if (tmpSearchRData->GetNumSearchCriteria() == 1) {
                            if (searchRData->GetSearchCriterion(0) == tmpSearchRData->GetSearchCriterion(0)) {
                                m_burstQueue.erase(it++);
                                continue;
                            }
                        }
                    }
                }
            }
            it++;
        }
        m_mutex.Unlock();
        TriggerTransmission(Packet::cast(mdnsPacket));
    }

    return ER_OK;
}

QStatus IpNameServiceImpl::Response(TransportMask completeTransportMask, uint32_t ttl,  MDNSPacket mdnsPacket)
{
    QCC_DbgHLPrintf(("IpNameServiceImpl::Response(0x%x, ...)", completeTransportMask));

    if (m_state != IMPL_RUNNING) {
        QCC_DbgPrintf(("IpNameServiceImpl::Response(): Not running"));
        return ER_FAIL;
    }

    //
    // Fill in the mandatory sections of the response
    //
    mdnsPacket->SetVersion(2, 2);

    int32_t id = IncrementAndFetch(&INCREMENTAL_PACKET_ID);
    MDNSHeader mdnsHeader(id, MDNSHeader::MDNS_RESPONSE);
    mdnsPacket->SetHeader(mdnsHeader);

    // We defer the checks for the listening ports to the point when the packet is re-written.

    if (completeTransportMask & TRANSPORT_TCP) {
        MDNSPtrRData* ptrRDataTcp = new MDNSPtrRData();
        ptrRDataTcp->SetPtrDName(m_guid + "._alljoyn._tcp.local.");
        MDNSResourceRecord ptrRecordTcp("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, MDNSResourceRecord::INTERNET, 120, ptrRDataTcp);
        delete ptrRDataTcp;

        MDNSSrvRData* srvRDataTcp = new MDNSSrvRData(1 /*priority */, 1 /* weight */,
                                                     0 /* port */, m_guid + ".local." /* target */);
        MDNSResourceRecord srvRecordTcp(m_guid + "._alljoyn._tcp.local.", MDNSResourceRecord::SRV, MDNSResourceRecord::INTERNET, 120, srvRDataTcp);
        delete srvRDataTcp;

        MDNSTextRData* txtRDataTcp = new MDNSTextRData();
        if (m_reliableIPv6Port[TRANSPORT_INDEX_TCP]) {
            txtRDataTcp->SetValue("r6port", U32ToString(m_reliableIPv6Port[TRANSPORT_INDEX_TCP]));
        }

        MDNSResourceRecord txtRecordTcp(m_guid + "._alljoyn._tcp.local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, txtRDataTcp);
        delete txtRDataTcp;

        mdnsPacket->AddAnswer(ptrRecordTcp);
        mdnsPacket->AddAnswer(srvRecordTcp);
        mdnsPacket->AddAnswer(txtRecordTcp);
    }

    // We defer the checks for the listening ports to the point when the packet is re-written.
    if (completeTransportMask & TRANSPORT_UDP) {
        MDNSPtrRData* ptrRDataUdp = new MDNSPtrRData();
        ptrRDataUdp->SetPtrDName(m_guid + "._alljoyn._udp.local.");
        MDNSResourceRecord ptrRecordUdp("_alljoyn._udp.local.", MDNSResourceRecord::PTR, MDNSResourceRecord::INTERNET, 120, ptrRDataUdp);
        delete ptrRDataUdp;

        MDNSSrvRData* srvRDataUdp = new MDNSSrvRData(1 /*priority */, 1 /* weight */,
                                                     0 /* port */, m_guid + ".local." /* target */);
        MDNSResourceRecord srvRecordUdp(m_guid + "._alljoyn._udp.local.", MDNSResourceRecord::SRV, MDNSResourceRecord::INTERNET, 120, srvRDataUdp);
        delete srvRDataUdp;

        MDNSTextRData* txtRDataUdp = new MDNSTextRData();
        if (m_unreliableIPv6Port[TRANSPORT_INDEX_UDP]) {
            txtRDataUdp->SetValue("u6port", U32ToString(m_unreliableIPv6Port[TRANSPORT_INDEX_UDP]));
        }

        MDNSResourceRecord txtRecordUdp(m_guid + "._alljoyn._udp.local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, txtRDataUdp);
        delete txtRDataUdp;

        mdnsPacket->AddAnswer(ptrRecordUdp);
        mdnsPacket->AddAnswer(srvRecordUdp);
        mdnsPacket->AddAnswer(txtRecordUdp);
    }

    MDNSSenderRData* refRData =  new MDNSSenderRData();
    refRData->SetSearchID(id);
    MDNSResourceRecord refRecord("sender-info." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, ttl, refRData);
    mdnsPacket->AddAdditionalRecord(refRecord);
    delete refRData;

    //
    // We don't want allow the caller to advertise an unlimited number of
    // names and consume all available network resources.  We expect
    // AdvertiseName() to typically be called once per advertised name, but
    // since we allow a vector of names we need to limit that size somehow.
    // The easy way is to assume that all of the names are the maximum size
    // and just limit based on the maximum NS packet size and the maximum
    // name size of 256 bytes.  This, however, leaves just five names which
    // seems too restrictive.  So, we do it the more time-consuming way and
    // put together the message and then see if it's "too big."
    //
    // This isn't terribly elegant, but we don't know the IP address(es) over
    // which the message will be sent.  These are added in the loop that
    // actually does the packet sends, with the interface addresses dynamically
    // added onto the message.  We have no clue here if an IPv4 or IPv6 or both
    // flavors of address will exist on a given interface, nor how many
    // interfaces there are.  All we can do here is to assume the worst case for
    // the size (both exist) and add the 20 bytes (four for IPv4, sixteen for
    // IPv6) that the addresses may consume in the final packet.
    //
    if (mdnsPacket->GetSerializedSize() + 20 <= NS_MESSAGE_MAX) {
        //
        // Queue this message for transmission out on the various live interfaces.
        //
        if (mdnsPacket->DestinationSet()) {
            QueueProtocolMessage(Packet::cast(mdnsPacket));
        } else {
            MDNSResourceRecord* advRecord;

            if (mdnsPacket->GetAdditionalRecord("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &advRecord)) {
                MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());

                m_mutex.Lock();
                // Search for the same name in the burstQueue.
                // If present, remove the entry to preserve the ordering of outgoing packets.
                std::list<BurstResponseHeader>::iterator it = m_burstQueue.begin();
                while (it != m_burstQueue.end()) {
                    uint32_t nsVersion;
                    uint32_t msgVersion;

                    (*it).packet->GetVersion(nsVersion, msgVersion);
                    if (msgVersion == 2) {
                        MDNSPacket temp = MDNSPacket::cast((*it).packet);
                        if (temp->GetHeader().GetQRType() == MDNSHeader::MDNS_RESPONSE) {

                            if (completeTransportMask == temp->GetTransportMask()) {
                                MDNSResourceRecord* tmpAdvRecord;
                                if (temp->GetAdditionalRecord("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &tmpAdvRecord)) {
                                    MDNSAdvertiseRData* tmpAdvRData = static_cast<MDNSAdvertiseRData*>(tmpAdvRecord->GetRData());
                                    if ((tmpAdvRData->GetNumTransports() == 1) && (advRData->GetNumNames(completeTransportMask) == tmpAdvRData->GetNumNames(completeTransportMask))) {
                                        bool matching = true;
                                        for (uint32_t k = 0; k < advRData->GetNumNames(completeTransportMask); k++) {
                                            if (advRData->GetNameAt(completeTransportMask, k) != tmpAdvRData->GetNameAt(completeTransportMask, k)) {
                                                matching  = false;
                                            }
                                        }
                                        if (matching) {
                                            m_burstQueue.erase(it++);
                                            continue;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    it++;
                }
                m_mutex.Unlock();

            }
            TriggerTransmission(Packet::cast(mdnsPacket));
        }
    } else {
        QCC_LogError(ER_PACKET_TOO_LARGE, ("IpNameServiceImpl::AdvertiseName(): Resulting NS message too large"));
        return ER_PACKET_TOO_LARGE;
    }


    return ER_OK;
}

QStatus IpNameServiceImpl::OnProcSuspend()
{
    if (!m_isProcSuspending) {
        m_isProcSuspending = true;
        m_forceLazyUpdate = true;
        m_wakeEvent.SetEvent();
    }
    return ER_OK;
}

QStatus IpNameServiceImpl::OnProcResume()
{
    if (m_isProcSuspending) {
        m_isProcSuspending = false;
        m_forceLazyUpdate = true;
        m_wakeEvent.SetEvent();
    }
    return ER_OK;
}

void IpNameServiceImpl::RegisterListener(IpNameServiceListener& listener)
{
    m_mutex.Lock();
    m_listeners.push_back(&listener);
    m_mutex.Unlock();
}

void IpNameServiceImpl::UnregisterListener(IpNameServiceListener& listener)
{
    m_mutex.Lock();
    // Wait till the listeners are not in use.
    while (m_protectListeners) {
        m_mutex.Unlock();
        qcc::Sleep(2);
        m_mutex.Lock();
    }
    m_listeners.remove(&listener);
    m_mutex.Unlock();
}

void IpNameServiceImpl::QueueProtocolMessage(Packet packet)
{
    // Maximum number of IpNameService protocol messages that can be queued.
    static const size_t MAX_IPNS_MESSAGES = 50;
    QCC_DbgPrintf(("IpNameServiceImpl::QueueProtocolMessage()"));

    uint32_t nsVersion, msgVersion;
    packet->GetVersion(nsVersion, msgVersion);
    assert(m_enableV1 || (msgVersion != 0 && msgVersion != 1));

    m_mutex.Lock();
    while (m_outbound.size() >= MAX_IPNS_MESSAGES) {
        m_mutex.Unlock();
        qcc::Sleep(10);
        m_mutex.Lock();
    }
    if (m_state == IMPL_RUNNING) {
        m_outbound.push_back(packet);
        m_wakeEvent.SetEvent();
    }
    m_mutex.Unlock();
}

//
// If you set HAPPY_WANDERER to 1, it will enable a test behavior that
// simulates the daemon happily wandering in and out of range of an
// imaginary access point.
//
// It is essentially a trivial one-dimensional random walk across a fixed
// domain.  When Wander() is called, der froliche wandering daemon moves
// in a random direction for one meter.  When the daemon "walks" out of
// range, Wander() returns false and the test will arrange that name
// service messages are discarded.  When the daemon "walks" back into
// range, messages are delivered again.  We generally call Wander() out
// DoPeriodicMaintenance() which ticks every second, but also out of
// HandleProtocolAnswer() so the random walk is at a non-constant rate
// driven by network activity.  Very nasty.
//
// The environment is 100 meters long, and the range of the access point
// is 50 meters.  The daemon starts right at the edge of the range and is
// expected to hover around that point, but wander random distances in and
// out.
//
//   (*)                       X                         |
//    |                     <- D ->                      |
//    ---------------------------------------------------
//    0                        50                       100
//
// Since this is a very dangerous setting, turning it on is a two-step
// process (set the #define and enable the bool); and we log every action
// as an error.  It will be hard to ignore this and accidentally leave it
// turned on.
//
#define HAPPY_WANDERER 0

#if HAPPY_WANDERER

static const uint32_t WANDER_LIMIT = 100;
static const uint32_t WANDER_RANGE = WANDER_LIMIT / 2;
static const uint32_t WANDER_START = WANDER_RANGE;

bool g_enableWander = false;

void WanderInit(void)
{
    srand(time(NULL));
}

bool Wander(void)
{
    //
    // If you don't explicitly enable this behavior, Wander() always returns
    // "in-range".
    //
    if (g_enableWander == false) {
        return true;
    }

    static uint32_t x = WANDER_START;
    static bool xyzzy = false;

    if (xyzzy == false) {
        WanderInit();
        xyzzy = true;
    }

    switch (x) {
    case 0:
        // Valderi
        ++x;
        break;

    case WANDER_LIMIT:
        // Valdera
        --x;
        break;

    default:
        // Valderahahahahahaha
        x += rand() & 1 ? 1 : -1;
        break;
    }

    QCC_LogError(ER_FAIL, ("Wander(): Wandered to %d which %s in-range", x, x < WANDER_RANGE ? "is" : "is NOT"));

    return x < WANDER_RANGE;
}

#endif

void IpNameServiceImpl::SendProtocolMessage(
    qcc::SocketFd sockFd,
    qcc::IPAddress interfaceAddress,
    uint32_t interfaceAddressPrefixLen,
    uint32_t flags,
    bool sockFdIsIPv4,
    Packet packet,
    uint32_t interfaceIndex,
    const qcc::IPAddress& localAddress)
{
    QCC_DbgPrintf(("**********IpNameServiceImpl::SendProtocolMessage()"));

#if HAPPY_WANDERER
    if (Wander() == false) {
        QCC_LogError(ER_FAIL, ("IpNameServiceImpl::SendProtocolMessage(): Wander(): out of range"));
        return;
    } else {
        QCC_LogError(ER_FAIL, ("IpNameServiceImpl::SendProtocolMessage(): Wander(): in range"));
    }
#endif

    uint32_t nsVersion, msgVersion;
    packet->GetVersion(nsVersion, msgVersion);

    size_t size = packet->GetSerializedSize();
    if (size > NS_MESSAGE_MAX) {
        QCC_LogError(ER_FAIL, ("SendProtocolMessage: Message (%d bytes) is longer than NS_MESSAGE_MAX (%d bytes)",
                               size, NS_MESSAGE_MAX));
        return;
    }

    uint8_t* buffer = new uint8_t[size];
    size = packet->Serialize(buffer);

    size_t sent;

    //
    // We have the concept of a quiet advertisement which means that we don't
    // actively send out is-at packets announcing that we have corresponding
    // well-known names.  We don't announce them gratuitously, but we do respond
    // to queries on the names we are quietly advertising.  With quiet
    // advertisements come quiet responses.  This means that we don't yell our
    // answers over IP multicast, but we politely and quietly respond over
    // unicast.
    //
    // So the first thing to do is to decide whether or not we need to respond
    // quietly or over the multicast channel.  If this protocol message corresponds
    // to a quiet advertisement, the destination address in the header will have
    // been set and we can just respond directly to that address and bail.
    //
    // One complication is that the name service wants to discover all possible
    // interfaces and send all advertisements out all interfaces over all
    // flavors (IPv4 and IPv6) and be completely in control of routing.  In the
    // case of quiet responses we only want to send out responses when the
    // flavor of the address and socket match, and we only want to send messages
    // out on the network number on which the advertisement came in.  Rather
    // than fight with the natural inclination of the bulk of the code, we just
    // quickly open a new socket and let the system route the message out in its
    // usual way.
    //
    // This is a bit of a hack, but then again, this is an experimental change
    // as of now.
    //
    if (packet->DestinationSet()) {
        QStatus status = ER_OK;
        qcc::IPEndpoint destination = packet->GetDestination();
        qcc::AddressFamily family = destination.addr.IsIPv4() ? qcc::QCC_AF_INET : qcc::QCC_AF_INET6;

        if (family == qcc::QCC_AF_INET && m_ipv4QuietSockFd == qcc::INVALID_SOCKET_FD) {
            status = qcc::Socket(family, qcc::QCC_SOCK_DGRAM, m_ipv4QuietSockFd);
        }

        if (family == qcc::QCC_AF_INET6 && m_ipv6QuietSockFd == qcc::INVALID_SOCKET_FD) {
            status = qcc::Socket(family, qcc::QCC_SOCK_DGRAM, m_ipv6QuietSockFd);
        }

        if (status != ER_OK) {
            QCC_LogError(status, ("IpNameServiceImpl::SendProtocolMessage(): Socket() failed: %d - %s", qcc::GetLastError(), qcc::GetLastErrorString().c_str()));
        }

        if (status == ER_OK) {
            QCC_DbgHLPrintf(("IpNameServiceImpl::SendProtocolMessage(): Sending quietly to \"%s\" over \"%s\"", destination.ToString().c_str(), m_liveInterfaces[interfaceIndex].m_interfaceName.c_str()));

            if (family == qcc::QCC_AF_INET) {
                status = qcc::SendTo(m_ipv4QuietSockFd, destination.addr, destination.port, buffer, size, sent);
            } else {
                status = qcc::SendTo(m_ipv6QuietSockFd, destination.addr, destination.port, m_liveInterfaces[interfaceIndex].m_index,
                                     buffer, size, sent);
            }
        }

        if (status != ER_OK) {
            QCC_LogError(status, ("IpNameServiceImpl::SendProtocolMessage(): Error quietly sending to \"%s\"", destination.ToString().c_str()));
        }

        delete [] buffer;
        return;
    }

    //
    // Since we have fallen through to here,
    // Now it's time to send the packets.  Packets is plural since we will try
    // to get our name service information across to peers in as many ways as is
    // reasonably possible since it turns out that discovery is a weak link in
    // the system.  This means we will try broadcast and IPv6 multicast whenever
    // possible.
    //
    if (sockFdIsIPv4) {
        //
        // If the underlying interface told us that it supported multicast, send
        // the packet out on our IPv4 multicast groups (IANA registered and
        // legacy).
        //
#if 1
        if (flags & qcc::IfConfigEntry::MULTICAST ||
            flags & qcc::IfConfigEntry::LOOPBACK) {

#if WORKAROUND_2_3_BUG

            if ((msgVersion == 0) && m_enableV1) {
                qcc::IPAddress ipv4SiteAdminMulticast(IPV4_MULTICAST_GROUP);
                QCC_DbgHLPrintf(("IpNameServiceImpl::SendProtocolMessage():  Sending actively to \"%s\" over \"%s\"",
                                 ipv4SiteAdminMulticast.ToString().c_str(), m_liveInterfaces[interfaceIndex].m_interfaceName.c_str()));
                QStatus status = qcc::SendTo(sockFd, ipv4SiteAdminMulticast, MULTICAST_PORT, buffer, size, sent);
                if (status != ER_OK) {
                    QCC_LogError(status, ("IpNameServiceImpl::SendProtocolMessage():  Error sending to IPv4 Site Administered multicast group"));
                }
            }
#endif
            if (msgVersion == 2) {
                qcc::IPAddress ipv4LocalMulticast(IPV4_MDNS_MULTICAST_GROUP);
                QCC_DbgHLPrintf(("IpNameServiceImpl::SendProtocolMessage():  Sending actively to \"%s\" over \"%s\"",
                                 ipv4LocalMulticast.ToString().c_str(), m_liveInterfaces[interfaceIndex].m_interfaceName.c_str()));
                QStatus status = qcc::SendTo(sockFd, ipv4LocalMulticast, MULTICAST_MDNS_PORT, buffer, size, sent);
                if (status != ER_OK) {
                    QCC_LogError(status, ("IpNameServiceImpl::SendProtocolMessage():  Error sending to IPv4 Local Network Control Block multicast group"));
                }
            } else if (m_enableV1) {
                qcc::IPAddress ipv4LocalMulticast(IPV4_ALLJOYN_MULTICAST_GROUP);
                if (localAddress == qcc::IPAddress("0.0.0.0") || localAddress == ipv4LocalMulticast) {
                    QCC_DbgHLPrintf(("IpNameServiceImpl::SendProtocolMessage():  Sending actively to \"%s\" over \"%s\"",
                                     ipv4LocalMulticast.ToString().c_str(), m_liveInterfaces[interfaceIndex].m_interfaceName.c_str()));
                    QStatus status = qcc::SendTo(sockFd, ipv4LocalMulticast, MULTICAST_PORT, buffer, size, sent);
                    if (status != ER_OK) {
                        QCC_LogError(status, ("IpNameServiceImpl::SendProtocolMessage():  Error sending to IPv4 Local Network Control Block multicast group"));
                    }
                }
            }
        }
#endif

        //
        // If the interface is broadcast-capable, We want to send out a subnet
        // directed broadcast over IPv4.
        //
        if (flags & qcc::IfConfigEntry::BROADCAST) {
            //
            // If there was a problem getting the IP address prefix
            // length, it will come in as -1.  In this case, we can't form
            // a proper subnet directed broadcast and so we don't try.  An
            // error will have been logged when we did the IfConfig, so
            // don't flood out any more, just silently ignore the problem.
            //
            if (m_broadcast && interfaceAddressPrefixLen != static_cast<uint32_t>(-1)) {
                //
                // In order to ensure that our broadcast goes to the correct
                // interface and is not just sent out some default way, we
                // have to form a subnet directed broadcast.  To do this we need
                // the IP address and netmask.
                //
                QCC_DbgPrintf(("IpNameServiceImpl::SendProtocolMessage():  InterfaceAddress %s, prefix %d",
                               interfaceAddress.ToString().c_str(), interfaceAddressPrefixLen));

                //
                // Create a netmask with a one in the leading bits for each position
                // implied by the prefix length.
                //
                uint32_t mask = 0;
                for (uint32_t i = 0; i < interfaceAddressPrefixLen; ++i) {
                    mask >>= 1;
                    mask |= 0x80000000;
                }

                //
                // The subnet directed broadcast address is the address part of the
                // interface address (defined by the mask) with the rest of the bits
                // set to one.
                //
                uint32_t addr = (interfaceAddress.GetIPv4AddressCPUOrder() & mask) | ~mask;
                qcc::IPAddress ipv4Broadcast(addr);
                QCC_DbgHLPrintf(("IpNameServiceImpl::SendProtocolMessage():  Sending actively to \"%s\" over \"%s\"",
                                 ipv4Broadcast.ToString().c_str(), m_liveInterfaces[interfaceIndex].m_interfaceName.c_str()));

                if (msgVersion != 2 && m_enableV1 && (localAddress == qcc::IPAddress("0.0.0.0") || localAddress == ipv4Broadcast)) {
                    QStatus status = qcc::SendTo(sockFd, ipv4Broadcast, BROADCAST_PORT, buffer, size, sent);

                    if (status != ER_OK) {
                        QCC_LogError(ER_FAIL, ("IpNameServiceImpl::SendProtocolMessage():  Error sending to IPv4 (broadcast)"));
                    }
                }
            } else {
                QCC_DbgPrintf(("IpNameServiceImpl::SendProtocolMessage():  Subnet directed broadcasts are disabled"));
            }
        } else {
            QCC_DbgPrintf(("IpNameServiceImpl::SendProtocolMessage():  Interface does not support broadcast"));
        }
    } else {
        if (flags & qcc::IfConfigEntry::MULTICAST ||
            flags & qcc::IfConfigEntry::LOOPBACK) {

#if WORKAROUND_2_3_BUG

            if ((msgVersion == 0) && m_enableV1) {
                qcc::IPAddress ipv6SiteAdmin(IPV6_MULTICAST_GROUP);
                QCC_DbgHLPrintf(("IpNameServiceImpl::SendProtocolMessage():  Sending actively to \"%s\" over \"%s\"",
                                 ipv6SiteAdmin.ToString().c_str(), m_liveInterfaces[interfaceIndex].m_interfaceName.c_str()));
                QStatus status = qcc::SendTo(sockFd, ipv6SiteAdmin, MULTICAST_PORT, buffer, size, sent);
                if (status != ER_OK) {
                    QCC_LogError(status, ("IpNameServiceImpl::SendProtocolMessage():  Error sending to IPv6 Site Administered multicast group "));
                }
            }

#endif
            QStatus status = ER_OK;
            if (msgVersion == 2) {
                qcc::IPAddress ipv6AllJoyn(IPV6_MDNS_MULTICAST_GROUP);
                QCC_DbgHLPrintf(("IpNameServiceImpl::SendProtocolMessage():  Sending actively to \"%s\" over \"%s\"",
                                 ipv6AllJoyn.ToString().c_str(), m_liveInterfaces[interfaceIndex].m_interfaceName.c_str()));
                status = qcc::SendTo(sockFd, ipv6AllJoyn, MULTICAST_MDNS_PORT, buffer, size, sent);
            } else if (m_enableV1) {
                qcc::IPAddress ipv6AllJoyn(IPV6_ALLJOYN_MULTICAST_GROUP);
                QCC_DbgHLPrintf(("IpNameServiceImpl::SendProtocolMessage():  Sending actively to \"%s\" over \"%s\"",
                                 ipv6AllJoyn.ToString().c_str(), m_liveInterfaces[interfaceIndex].m_interfaceName.c_str()));
                status = qcc::SendTo(sockFd, ipv6AllJoyn, MULTICAST_PORT, buffer, size, sent);
            }
            if (status != ER_OK) {
                QCC_LogError(status, ("IpNameServiceImpl::SendProtocolMessage():  Error sending to IPv6 Link-Local Scope multicast group "));
            }
        }
    }

    delete [] buffer;
}

bool IpNameServiceImpl::InterfaceRequested(uint32_t transportIndex, uint32_t liveIndex)
{
    QCC_DbgPrintf(("IpNameServiceImpl::InterfaceRequested()"));

    //
    // Look for the wildcard condition (any interface) and take into account
    // that <any> doesn't mean P2P on Android.
    //
#if defined(QCC_OS_ANDROID)
    if (m_any[transportIndex] && m_liveInterfaces[liveIndex].m_interfaceName.find("p2p") == qcc::String::npos) {
        QCC_DbgPrintf(("IpNameServiceImpl::InterfaceRequested(): Interface \"%s\" approved.", m_liveInterfaces[liveIndex].m_interfaceName.c_str()));
        return true;
    }
#else
    if (m_any[transportIndex]) {
        QCC_DbgPrintf(("IpNameServiceImpl::InterfaceRequested(): Interface \"%s\" approved.", m_liveInterfaces[liveIndex].m_interfaceName.c_str()));
        return true;
    }
#endif

    //
    // Now, the question is whether or not the current interface as indicated by
    // the interface name is on the list of requested interfaces for the
    // transport mask found in the message.  If it is not, we must not send this
    // message out the current interface.
    //
    for (uint32_t i = 0; i < m_requestedInterfaces[transportIndex].size(); ++i) {
        //
        // If the current interface name matches the name in the requestedInterface list,
        // we will send this message out the current interface.
        //
        if (m_requestedInterfaces[transportIndex][i].m_interfaceName == m_liveInterfaces[liveIndex].m_interfaceName) {
            QCC_DbgPrintf(("IpNameServiceImpl::InterfaceRequested(): Interface \"%s\" approved.", m_liveInterfaces[liveIndex].m_interfaceName.c_str()));
            return true;
        }
        //
        // If the current interface IP address matches the IP address in the
        // requestedInterface list, we will send this message out the current interface.
        //
        if (m_requestedInterfaces[transportIndex][i].m_interfaceName.size() == 0 &&
            m_requestedInterfaces[transportIndex][i].m_interfaceAddr == qcc::IPAddress(m_liveInterfaces[liveIndex].m_interfaceAddr)) {
            QCC_DbgPrintf(("IpNameServiceImpl::InterfaceRequested(): Interface \"%s\" approved.", m_liveInterfaces[liveIndex].m_interfaceName.c_str()));
            return true;
        }
    }

    return false;
}

void IpNameServiceImpl::RewriteVersionSpecific(
    uint32_t msgVersion,
    Packet packet,
    bool haveIPv4address, qcc::IPAddress ipv4address,
    bool haveIPv6address, qcc::IPAddress ipv6address,
    uint16_t unicastIpv4Port, const qcc::String& interface,
    uint16_t const reliableTransportPort, const uint16_t unreliableTransportPort)
{
    QCC_DbgPrintf(("IpNameServiceImpl::RewriteVersionSpecific()"));

    //
    // We're modifying answers in-place so clear any state we might have
    // previously added.
    //
    NSPacket nsPacket;
    MDNSPacket mdnspacket;
    switch (msgVersion) {
    case 0:
        QCC_DbgPrintf(("IpNameServiceImpl::RewriteVersionSpecific(): Answer gets version zero"));
        nsPacket  = NSPacket::cast(packet);
        //
        // At this point, we know both of our local IPv4 and IPv6 addresses if they exist.  Now, we have to
        // walk the list of answer (is-at) messages and rewrite the provided addresses
        // that will correspond to the interface we are sending the message out
        // of.
        //
        for (uint8_t j = 0; j < nsPacket->GetNumberAnswers(); ++j) {
            QCC_DbgPrintf(("IpNameServiceImpl::RewriteVersionSpecific(): Rewrite answer %d.", j));

            IsAt* isAt;
            nsPacket->GetAnswer(j, &isAt);
            isAt->ClearIPv4();
            isAt->ClearIPv6();
            isAt->ClearReliableIPv4();
            isAt->ClearUnreliableIPv4();
            isAt->ClearReliableIPv6();
            isAt->ClearUnreliableIPv6();

            QCC_DbgPrintf(("IpNameServiceImpl::RewriteVersionSpecific(): Answer gets version zero"));

            isAt->SetVersion(0, 0);
            isAt->SetTcpFlag(true);

            isAt->SetPort(reliableTransportPort);
            //
            // Remember that we must sneak in the fact that we are a post-zero name
            // service by the old "setting the UDP flag" trick.
            //
            isAt->SetUdpFlag(true);

            //
            // For version zero, the name service was an integral part of the TCP
            // transport.  Because of this, we know implicitly that the only kind of
            // address supported was the reliable IPv4 address.  This means we just
            // need to set the IPv4 address.
            //
            if (haveIPv4address) {
                isAt->SetIPv4(ipv4address.ToString());
            }
        }
        break;

    case 1:
        {
            QCC_DbgPrintf(("IpNameServiceImpl::RewriteVersionSpecific(): Answer gets version one"));

            nsPacket  = NSPacket::cast(packet);
            //
            // At this point, we know both of our local IPv4 and IPv6 addresses if they exist.  Now, we have to
            // walk the list of answer (is-at) messages and rewrite the provided addresses
            // that will correspond to the interface we are sending the message out
            // of.
            //
            for (uint8_t j = 0; j < nsPacket->GetNumberAnswers(); ++j) {
                QCC_DbgPrintf(("IpNameServiceImpl::RewriteVersionSpecific(): Rewrite answer %d.", j));

                IsAt* isAt;
                nsPacket->GetAnswer(j, &isAt);
                isAt->ClearIPv4();
                isAt->ClearIPv6();
                isAt->ClearReliableIPv4();
                isAt->ClearUnreliableIPv4();
                isAt->ClearReliableIPv6();
                isAt->ClearUnreliableIPv6();

                QCC_DbgPrintf(("IpNameServiceImpl::RewriteVersionSpecific(): Answer gets version one"));

                isAt->SetVersion(1, 1);

                uint32_t transportIndex = IndexFromBit(isAt->GetTransportMask());
                assert(transportIndex < 16 && "IpNameServiceImpl::RewriteVersionSpecific(): Bad transport index in messageg");
                if (transportIndex >= 16) {
                    return;
                }

                //
                // Now we can write the various addresses into the
                // packet if they are called for.
                //
                if (haveIPv4address && reliableTransportPort) {
                    isAt->SetReliableIPv4(ipv4address.ToString(), reliableTransportPort);
                }

                if (haveIPv4address && unreliableTransportPort) {
                    isAt->SetUnreliableIPv4(ipv4address.ToString(), unreliableTransportPort);
                }
                // This is a trick to make V2 NS ignore V1 packets. We set the IPv6 reliable bit,
                // that tells version two capable NS that a version two message will follow, and
                // to ignore the version one messages.

                isAt->SetReliableIPv6(ipv6address.ToString(), m_reliableIPv6Port[transportIndex]);

                if (haveIPv6address && m_unreliableIPv6Port[transportIndex]) {
                    isAt->SetUnreliableIPv6(ipv6address.ToString(), m_unreliableIPv6Port[transportIndex]);
                }
            }
            break;
        }

    case 2:
        {
            //Need to rewrite ipv4Address into A record,ipv6address, unicast NS response ports into reference record.
            mdnspacket  = MDNSPacket::cast(packet);
            MDNSHeader mdnsheader = mdnspacket->GetHeader();
            MDNSResourceRecord* refRecord;
            mdnspacket->GetAdditionalRecord("sender-info.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &refRecord);
            MDNSSenderRData* refRData = static_cast<MDNSSenderRData*>(refRecord->GetRData());
            if (mdnsheader.GetQRType() == MDNSHeader::MDNS_QUERY) {
                if (haveIPv4address && (unicastIpv4Port != 0)) {
                    refRData->SetIPV4ResponsePort(unicastIpv4Port);
                    refRData->SetIPV4ResponseAddr(ipv4address.ToString());
                } else {
                    refRData->RemoveEntry("ipv4");
                    refRData->RemoveEntry("upcv4");
                }
            } else {

                //Response packet
                for (int i = 0; i < mdnspacket->GetNumAnswers(); i++) {

                    MDNSResourceRecord* answerRecord;
                    mdnspacket->GetAnswerAt(i, &answerRecord);
                    MDNSResourceRecord* resourceRecord;
                    MDNSARData* addrRData;
                    MDNSTextRData* txtRData;
                    MDNSSrvRData* srvRData;

                    switch (answerRecord->GetRRType()) {
                    case MDNSResourceRecord::SRV:

                        if (answerRecord->GetDomainName().find("._tcp.") != String::npos) {

                            srvRData = static_cast<MDNSSrvRData*>(answerRecord->GetRData());

                            if (haveIPv4address) {
                                if (!mdnspacket->GetAdditionalRecord(srvRData->GetTarget(), MDNSResourceRecord::A, &resourceRecord)) {
                                    // Add an IPv4 address record
                                    addrRData = new MDNSARData();
                                    mdnspacket->AddAdditionalRecord(MDNSResourceRecord(m_guid + ".local.", MDNSResourceRecord::A, MDNSResourceRecord::INTERNET, 120, addrRData));
                                    mdnspacket->GetAdditionalRecord(srvRData->GetTarget(), MDNSResourceRecord::A, &resourceRecord);
                                    delete addrRData;
                                }
                                addrRData = static_cast<MDNSARData*>(resourceRecord->GetRData());
                                if (addrRData) {
                                    addrRData->SetAddr(ipv4address.ToString());
                                    refRData->SetIPV4ResponsePort(unicastIpv4Port);
                                    if (reliableTransportPort) {
                                        srvRData->SetPort(reliableTransportPort);
                                    }
                                }
                            } else {
                                mdnspacket->RemoveAdditionalRecord(m_guid + ".local.", MDNSResourceRecord::A);
                                refRData->RemoveEntry("ipv4");
                                refRData->RemoveEntry("upcv4");
                            }

                        } else if (answerRecord->GetDomainName().find("._udp.") != String::npos) {
                            srvRData = static_cast<MDNSSrvRData*>(answerRecord->GetRData());
                            if (haveIPv4address) {
                                if (!mdnspacket->GetAdditionalRecord(srvRData->GetTarget(), MDNSResourceRecord::A, &resourceRecord)) {
                                    // Add an IPv4 address record
                                    addrRData = new MDNSARData();
                                    mdnspacket->AddAdditionalRecord(MDNSResourceRecord(m_guid + ".local.", MDNSResourceRecord::A, MDNSResourceRecord::INTERNET, 120, addrRData));
                                    delete addrRData;
                                    mdnspacket->GetAdditionalRecord(srvRData->GetTarget(), MDNSResourceRecord::A, &resourceRecord);
                                }
                                addrRData = static_cast<MDNSARData*>(resourceRecord->GetRData());
                                if (addrRData) {
                                    addrRData->SetAddr(ipv4address.ToString());
                                    if (unicastIpv4Port != 0) {
                                        refRData->SetIPV4ResponsePort(unicastIpv4Port);
                                    }
                                    if (unreliableTransportPort) {
                                        srvRData->SetPort(unreliableTransportPort);
                                    }
                                }
                            }  else {
                                mdnspacket->RemoveAdditionalRecord(m_guid + ".local.", MDNSResourceRecord::A);
                                refRData->RemoveEntry("ipv4");
                                refRData->RemoveEntry("upcv4");
                            }

                        }


                        break;

                    case MDNSResourceRecord::TXT:
                        txtRData = static_cast<MDNSTextRData*>(answerRecord->GetRData());
                        if (answerRecord->GetDomainName().find("._tcp.") != String::npos) {

                            if (m_reliableIPv6Port[TRANSPORT_INDEX_TCP]) {
                                txtRData->SetValue("r6port",  U32ToString(m_reliableIPv6Port[TRANSPORT_INDEX_TCP]));
                            }

                        } else if (answerRecord->GetDomainName().find("._udp.") != String::npos) {
                            if (m_unreliableIPv6Port[TRANSPORT_INDEX_UDP]) {
                                txtRData->SetValue("u6port",  U32ToString(m_unreliableIPv6Port[TRANSPORT_INDEX_UDP]));
                            }

                        }

                        break;

                    default:
                        break;
                    }
                }
            }
            break;
        }

    default:
        assert(false && "IpNameServiceImpl::RewriteVersionSpecific(): Bad message version");
        break;
    }

}

bool IpNameServiceImpl::SameNetwork(uint32_t interfaceAddressPrefixLen, qcc::IPAddress addressA, qcc::IPAddress addressB)
{
    QCC_DbgPrintf(("IpNameServiceImpl::SameNetwork(%d, \"%s\", \"%s\")", interfaceAddressPrefixLen,
                   addressA.ToString().c_str(), addressB.ToString().c_str()));

    //
    // If there was a problem getting the IP address prefix length, it will come
    // in as -1.  In this case, we can't determine what part of the addresses
    // are network number so we don't try.
    //
    if (interfaceAddressPrefixLen == static_cast<uint32_t>(-1)) {
        QCC_LogError(ER_FAIL, ("IpNameServiceImpl::SameNetwork(): Bad network prefix"));
        return false;
    }

    if (addressA.IsIPv6()) {
        if (addressB.IsIPv4()) {
            QCC_DbgPrintf(("IpNameServiceImpl::SameNetwork(): Network families are different"));
            return false;
        }

        if (interfaceAddressPrefixLen > 128) {
            QCC_LogError(ER_FAIL, ("IpNameServiceImpl::SameNetwork(): Bad IPv6 network prefix"));
            return false;
        }

        uint8_t addrA[qcc::IPAddress::IPv6_SIZE];
        addressA.RenderIPv6Binary(addrA, qcc::IPAddress::IPv6_SIZE);
        uint8_t addrB[qcc::IPAddress::IPv6_SIZE];
        addressB.RenderIPv6Binary(addrB, qcc::IPAddress::IPv6_SIZE);

        uint32_t nBytes = interfaceAddressPrefixLen / 8;
        for (uint32_t i = 0; i < nBytes; ++i) {
            if (addrA[i] != addrB[i]) {
                QCC_DbgPrintf(("IpNameServiceImpl::SameNetwork(): IPv6 networks are different"));
                return false;
            }
        }

        uint32_t nBits = interfaceAddressPrefixLen % 8;
        uint8_t mask = 0;
        for (uint32_t i = 0; i < nBits; ++i) {
            mask >>= 1;
            mask |= 0x80;
        }

        if (interfaceAddressPrefixLen == 128) {
            return true;
        }

        if ((addrA[nBytes] & mask) == (addrB[nBytes] & mask)) {
            QCC_DbgPrintf(("IpNameServiceImpl::SameNetwork(): IPv6 networks are the same"));
            return true;
        } else {
            QCC_DbgPrintf(("IpNameServiceImpl::SameNetwork(): IPv6 networks are different"));
            return false;
        }
    } else if (addressA.IsIPv4()) {
        if (addressB.IsIPv6()) {
            QCC_DbgPrintf(("IpNameServiceImpl::SameNetwork(): Network families are different"));
            return false;
        }

        if (interfaceAddressPrefixLen > 32) {
            QCC_LogError(ER_FAIL, ("IpNameServiceImpl::SameNetwork(): Bad IPv4 network prefix"));
            return false;
        }

        //
        // Create a netmask with a one in the leading bits for each position
        // implied by the prefix length.
        //
        uint32_t mask = 0;
        for (uint32_t i = 0; i < interfaceAddressPrefixLen; ++i) {
            mask >>= 1;
            mask |= 0x80000000;
        }

        //
        // The subnet directed broadcast address is the address part of the
        // interface address (defined by the mask) with the rest of the bits
        // set to one.
        //
        uint32_t addrA = (addressA.GetIPv4AddressCPUOrder() & mask);
        uint32_t addrB = (addressB.GetIPv4AddressCPUOrder() & mask);

        //
        // If the masked off network bits are the same, the two addresses belong
        // to the same network.
        //
        if (addrA == addrB) {
            QCC_DbgPrintf(("IpNameServiceImpl::SameNetwork(): IPv4 networks are the same"));
            return true;
        } else {
            QCC_DbgPrintf(("IpNameServiceImpl::SameNetwork(): IPv4 networks are different"));
            return false;
        }
    }

    assert(false && "IpNameServiceImpl::SameNetwork(): Not IPv4 or IPv6?");
    return false;
}

void IpNameServiceImpl::SendOutboundMessageQuietly(Packet packet)
{
    QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly()"));
    //
    // Sending messages quietly is a "new thing" so we don't bother to send
    // down-version messages quietly since nobody will have a need for them.
    //
    uint32_t nsVersion, msgVersion;
    packet->GetVersion(nsVersion, msgVersion);

    if (msgVersion == 0) {
        QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): Down-version message ignored"));
        return;
    }

    //
    // If we are doing a quiet response, we'd better have a destination address
    // to use.
    //
    assert(packet->DestinationSet() && "IpNameServiceImpl::SendOutboundMessageQuietly(): No destination IP address");
    qcc::IPEndpoint destination = packet->GetDestination();

    //
    // We have a destination address for the message which came ultimately from
    // the recvfrom that received the who-has message that drove the process
    // that got us here.  Someone figured out how to send us this message, and
    // we already figured out that we have an advertisement that matches that
    // message, but are we sure that we are allowed to send a response out the
    // interface specified by the network part of the destination address?
    //
    // Well, we have obviously opened the interface over which the message was
    // received or it would not have been received; so *some* transport has
    // opened that interface.  The question now is, was it the interface that
    // advertised the name that opened the interface over which we received the
    // who-has that caused us to respond?  It could be that transport A opened
    // the interface, but transport B advertised the name.  the right place to
    // do this check is up when the name was first received since we may have a
    // collection of names here and we aren't really sure which one of those
    // corresponds to the original request, so we simply assume that the right
    // thing to do was done and we just send the message on out the interface
    // corresponding to the destination address.
    //
    // Now, when higher level code queues up messages for us to send, it doesn't
    // know to what interfaces we will eventually send the messages, and
    // therefore what IP addresses we will be need to send as the contact
    // addresses in the messages.  We expect our transport listeners to be
    // listening to the appropriate INADDR_ANY address, and that they are
    // relying on us to get the IP addressing information of the various
    // networks we are talking to correct when we send out address and port
    // information.  What this means is that we are going to have to rewrite the
    // correct addresses into our is-at messages on the fly as we prepare to
    // send them out.
    //
    // So we need to walk the list of live interfaces and figure out which one
    // corresponds to the network part of the destination address.
    //
    for (uint32_t i = 0; (m_state == IMPL_RUNNING) && (i < m_liveInterfaces.size()); ++i) {
        QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): Checking out live interface %d. (\"%s\")",
                       i, m_liveInterfaces[i].m_interfaceName.c_str()));

        //
        // Don't bother to do anything if the socket FD isn't initialized, since
        // we most likely couldn't have actually received this message over that
        // socket (unless we're in a transient state) and so we shouldn't send
        // it out that interface.
        //
        if (m_liveInterfaces[i].m_multicastMDNSsockFd == qcc::INVALID_SOCKET_FD) {
            QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): Interface %d. is not live", i));
            continue;
        }

        QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): Interface %d. is live", i));

        //
        // We need to start doing cuts to figure out where (not) to send this
        // message.  The easiest cut is on address type.  If we have an IPv4
        // destination address we quite obviously aren't going to send it out an
        // interface with an IPv6 address or vice versa.
        //
        if ((destination.addr.IsIPv4() && m_liveInterfaces[i].m_address.IsIPv6()) ||
            (destination.addr.IsIPv6() && m_liveInterfaces[i].m_address.IsIPv4())) {
            QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): Interface %d. is address family mismatched", i));
            continue;
        }

        //
        // Now we know we have a destination address which is the same address family
        // as the current interface address.  Now, we need to see if they are on the
        // same network.
        //
        // The interesting tidbit for us now is the network address prefix
        // length (cf. net mask) that will let us know what part of the
        // destination address corresponds to the network number and will allow
        // us to compare destination address network with interface address
        // network.
        //
        uint32_t interfaceAddressPrefixLen = m_liveInterfaces[i].m_prefixlen;

        if (SameNetwork(interfaceAddressPrefixLen, m_liveInterfaces[i].m_address, destination.addr)) {
            uint32_t flags = m_liveInterfaces[i].m_flags;

            //
            // Okay, we have found the interface that received the who-has message
            // that started this process.
            //
            // When higher level code queues up messages, it doesn't know to
            // what interfaces and therefore over what source IP addresses we
            // will be using to send messages out on.  What this means is that
            // we are going to have to rewrite any IP addresses into is-at
            // messages on the fly as we prepare to send them out.
            //
            // The next thing we need to do is to pull out the IP address of the
            // current interface.  It may be either IPv4 or IPv6 -- all we know
            // now is that it matches the destination.
            //
            uint16_t unicastPortv4 = 0;
            qcc::IPAddress ipv4address;
            bool haveIPv4address = m_liveInterfaces[i].m_address.IsIPv4();
            if (haveIPv4address) {
                QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): Interface %d. is IPv4", i));
                ipv4address = m_liveInterfaces[i].m_address;
                unicastPortv4 = m_liveInterfaces[i].m_unicastPort;

            }
            bool interfaceIsIPv4 = haveIPv4address;

            qcc::IPAddress ipv6address;
            bool haveIPv6address = m_liveInterfaces[i].m_address.IsIPv6();
            if (haveIPv6address) {
                QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): Interface %d. is IPv6", i));
                ipv6address = m_liveInterfaces[i].m_address;
            }

            //
            // Each interface in our list is going to have either an IPv4 or an IPv6
            // address.  When we send the message, we want to send out both flavors
            // (Ipv4 and IPv6) over each interface since we want to maximize the
            // possibility that clients will actually receive this information
            // (i.e. we send IPv4 addressing over an IPv6 packet).  This is because
            // the probability to get a name service packet out is actually greater
            // over IPv6, but TCP transports want to listen on IPv4.  We do the
            // inverse just for consistency and to prepare for when TCP might
            // actually use IPv6.
            //
            // So, if the current address is IPv4, we scan for an IPv6 address on
            // another interface of the same name.  If the current address is IPv6,
            // we for an IPv4 address.
            //
            for (uint32_t j = 0; j < m_liveInterfaces.size(); ++j) {
                if (m_liveInterfaces[i].m_multicastMDNSsockFd == qcc::INVALID_SOCKET_FD ||
                    m_liveInterfaces[j].m_interfaceName != m_liveInterfaces[i].m_interfaceName) {
                    continue;
                }
                if (haveIPv4address == false && m_liveInterfaces[j].m_address.IsIPv4()) {
                    QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): Interface %d. has IPv4 counterpart %d.", i, j));
                    haveIPv4address = true;
                    ipv4address = m_liveInterfaces[j].m_address;
                    unicastPortv4 = m_liveInterfaces[j].m_unicastPort;
                    break;
                }

                if (haveIPv6address == false && m_liveInterfaces[j].m_address.IsIPv6()) {
                    QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): Interface %d. has IPv6 counterpart %d.", i, j));
                    haveIPv6address = true;
                    ipv6address = m_liveInterfaces[j].m_address;
                    break;
                }
            }

            if (!haveIPv4address) {
                QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): Interface %d does not have an IPv4 address.", i));
                continue;
            }

            //
            // Do the version-specific rewriting of the addresses in this NS/MDNS message.
            //
            uint16_t reliableTransportPort = 0;
            uint16_t unreliableTransportPort = 0;
            if (m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].find("*") != m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
                reliableTransportPort = m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP]["*"];
            } else if (m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].find("0.0.0.0") != m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
                reliableTransportPort = m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP]["0.0.0.0"];
            } else if (m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].find(m_liveInterfaces[i].m_interfaceName) != m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
                reliableTransportPort = m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP][m_liveInterfaces[i].m_interfaceName];
            } else if (m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].find(m_liveInterfaces[i].m_interfaceAddr.ToString()) != m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
                reliableTransportPort = m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP][m_liveInterfaces[i].m_interfaceAddr.ToString()];
            }

            if (m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find("*") != m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
                unreliableTransportPort = m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP]["*"];
            } else if (m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find("0.0.0.0") != m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
                unreliableTransportPort = m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP]["0.0.0.0"];
            } else if (m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find(m_liveInterfaces[i].m_interfaceName) != m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
                unreliableTransportPort = m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP][m_liveInterfaces[i].m_interfaceName];
            } else if (m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find(m_liveInterfaces[i].m_interfaceAddr.ToString()) != m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
                unreliableTransportPort = m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP][m_liveInterfaces[i].m_interfaceAddr.ToString()];
            }

            if (msgVersion == 0) {
                NSPacket nsPacket  = NSPacket::cast(packet);
                if (nsPacket->GetNumberAnswers() && !reliableTransportPort) {
                    continue;
                }
            } else if (msgVersion == 1) {
                NSPacket nsPacket  = NSPacket::cast(packet);
                if (nsPacket->GetNumberAnswers() && !reliableTransportPort && !unreliableTransportPort) {
                    continue;
                }
            } else {
                MDNSPacket mdnsPacket = MDNSPacket::cast(packet);
                if (mdnsPacket->GetHeader().GetQRType() == MDNSHeader::MDNS_RESPONSE) {
                    MDNSResourceRecord* ptrRecordTcp = NULL;
                    MDNSResourceRecord* ptrRecordUdp = NULL;
                    bool tcpAnswer = mdnsPacket->GetAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, &ptrRecordTcp);
                    bool udpAnswer = mdnsPacket->GetAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR, &ptrRecordUdp);
                    if (!tcpAnswer && !udpAnswer) {
                        continue;
                    }
                    if (tcpAnswer && !udpAnswer) {
                        if (!reliableTransportPort) {
                            continue;
                        }
                        uint32_t numMatches = mdnsPacket->GetNumMatches("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS);
                        for (uint32_t match = 0; match < numMatches; match++) {
                            MDNSResourceRecord* advRecord;
                            if (!mdnsPacket->GetAdditionalRecordAt("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, match, &advRecord)) {
                                continue;
                            }
                            MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());
                            if (!advRData) {
                                continue;
                            }

                            std::list<String> tcpNames;
                            uint32_t numTcp = advRData->GetNumNames(TRANSPORT_TCP);
                            uint32_t numUdp = advRData->GetNumNames(TRANSPORT_UDP);
                            uint32_t numTcpUdp = advRData->GetNumNames(TRANSPORT_TCP | TRANSPORT_UDP);
                            if (!numUdp && !numTcpUdp) {
                                continue;
                            }
                            for (uint32_t j = 0; j < numTcp; j++) {
                                String name = advRData->GetNameAt(TRANSPORT_TCP, j);
                                tcpNames.push_back(name);
                            }
                            for (uint32_t j = 0; j < numTcpUdp; j++) {
                                String name = advRData->GetNameAt(TRANSPORT_TCP | TRANSPORT_UDP, j);
                                tcpNames.push_back(name);
                            }
                            advRData->Reset();
                            advRData->SetTransport(TRANSPORT_TCP);
                            for (std::list<String>::iterator iter = tcpNames.begin(); iter != tcpNames.end(); iter++) {
                                advRData->SetValue("name", *iter);
                            }
                        }
                    }
                    if (udpAnswer && !tcpAnswer) {
                        if (!unreliableTransportPort) {
                            continue;
                        }
                        uint32_t numMatches = mdnsPacket->GetNumMatches("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS);
                        for (uint32_t match = 0; match < numMatches; match++) {
                            MDNSResourceRecord* advRecord;
                            if (!mdnsPacket->GetAdditionalRecordAt("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, match, &advRecord)) {
                                continue;
                            }
                            MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());
                            if (!advRData) {
                                continue;
                            }

                            std::list<String> udpNames;
                            uint32_t numTcp = advRData->GetNumNames(TRANSPORT_TCP);
                            uint32_t numUdp = advRData->GetNumNames(TRANSPORT_UDP);
                            uint32_t numTcpUdp = advRData->GetNumNames(TRANSPORT_TCP | TRANSPORT_UDP);
                            if (!numTcp && !numTcpUdp) {
                                continue;
                            }
                            for (uint32_t j = 0; j < numUdp; j++) {
                                String name = advRData->GetNameAt(TRANSPORT_UDP, j);
                                udpNames.push_back(name);
                            }
                            for (uint32_t j = 0; j < numTcpUdp; j++) {
                                String name = advRData->GetNameAt(TRANSPORT_TCP | TRANSPORT_UDP, j);
                                udpNames.push_back(name);
                            }
                            advRData->Reset();
                            advRData->SetTransport(TRANSPORT_UDP);
                            for (std::list<String>::iterator iter = udpNames.begin(); iter != udpNames.end(); iter++) {
                                advRData->SetValue("name", *iter);
                            }
                        }
                    }
                    if (tcpAnswer && udpAnswer) {
                        if (!reliableTransportPort && !unreliableTransportPort) {
                            continue;
                        } else if (!reliableTransportPort) {
                            MDNSPtrRData* ptrRData = static_cast<MDNSPtrRData*>(ptrRecordTcp->GetRData());
                            String name = ptrRData->GetPtrDName();
                            mdnsPacket->RemoveAnswer(name, MDNSResourceRecord::SRV);
                            mdnsPacket->RemoveAnswer(name, MDNSResourceRecord::TXT);
                            mdnsPacket->RemoveAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR);

                            uint32_t numMatches = mdnsPacket->GetNumMatches("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS);
                            for (uint32_t match = 0; match < numMatches; match++) {
                                MDNSResourceRecord* advRecord;
                                if (!mdnsPacket->GetAdditionalRecordAt("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, match, &advRecord)) {
                                    continue;
                                }
                                MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());
                                if (!advRData) {
                                    continue;
                                }

                                std::list<String> udpNames;
                                uint32_t numUdp = advRData->GetNumNames(TRANSPORT_UDP);
                                uint32_t numTcpUdp = advRData->GetNumNames(TRANSPORT_TCP | TRANSPORT_UDP);
                                for (uint32_t j = 0; j < numUdp; j++) {
                                    String name = advRData->GetNameAt(TRANSPORT_UDP, j);
                                    udpNames.push_back(name);
                                }
                                for (uint32_t j = 0; j < numTcpUdp; j++) {
                                    String name = advRData->GetNameAt(TRANSPORT_TCP | TRANSPORT_UDP, j);
                                    udpNames.push_back(name);
                                }
                                advRData->Reset();
                                advRData->SetTransport(TRANSPORT_UDP);
                                for (std::list<String>::iterator iter = udpNames.begin(); iter != udpNames.end(); iter++) {
                                    advRData->SetValue("name", *iter);
                                }
                            }
                        } else if (!unreliableTransportPort) {
                            MDNSPtrRData* ptrRData = static_cast<MDNSPtrRData*>(ptrRecordUdp->GetRData());
                            String name = ptrRData->GetPtrDName();
                            mdnsPacket->RemoveAnswer(name, MDNSResourceRecord::SRV);
                            mdnsPacket->RemoveAnswer(name, MDNSResourceRecord::TXT);
                            mdnsPacket->RemoveAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR);
                            MDNSResourceRecord* advRecord;
                            uint32_t numMatches = mdnsPacket->GetNumMatches("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS);
                            for (uint32_t match = 0; match < numMatches; match++) {
                                if (!mdnsPacket->GetAdditionalRecordAt("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, match, &advRecord)) {
                                    continue;
                                }
                                MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());
                                if (!advRData) {
                                    continue;
                                }
                                std::list<String> tcpNames;
                                uint32_t numTcp = advRData->GetNumNames(TRANSPORT_TCP);
                                uint32_t numTcpUdp = advRData->GetNumNames(TRANSPORT_TCP | TRANSPORT_UDP);
                                for (uint32_t j = 0; j < numTcp; j++) {
                                    String name = advRData->GetNameAt(TRANSPORT_TCP, j);
                                    tcpNames.push_back(name);
                                }
                                for (uint32_t j = 0; j < numTcpUdp; j++) {
                                    String name = advRData->GetNameAt(TRANSPORT_TCP | TRANSPORT_UDP, j);
                                    tcpNames.push_back(name);
                                }
                                advRData->Reset();
                                advRData->SetTransport(TRANSPORT_TCP);
                                for (std::list<String>::iterator iter = tcpNames.begin(); iter != tcpNames.end(); iter++) {
                                    advRData->SetValue("name", *iter);
                                }
                            }
                        }
                    }
                } else {
                    MDNSQuestion* questionUdp;
                    MDNSQuestion* questionTcp;
                    bool tcpQuestion = mdnsPacket->GetQuestion("_alljoyn._tcp.local.", &questionTcp);
                    bool udpQuestion = mdnsPacket->GetQuestion("_alljoyn._udp.local.", &questionUdp);
                    bool reliableTransportAllowed = (reliableTransportPort != 0);
                    bool unreliableTransportAllowed = (unreliableTransportPort != 0);
                    if (tcpQuestion && udpQuestion) {
                        if (!reliableTransportAllowed) {
                            mdnsPacket->RemoveQuestion("_alljoyn._tcp.local.");
                        } else if (!unreliableTransportAllowed) {
                            mdnsPacket->RemoveQuestion("_alljoyn._udp.local.");
                        }
                    }
                }
            }
            QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): Rewrite NS/MDNS packet %p", &(*packet)));
            RewriteVersionSpecific(msgVersion, packet, haveIPv4address, ipv4address, haveIPv6address, ipv6address,
                                   unicastPortv4, m_liveInterfaces[i].m_interfaceName, reliableTransportPort, unreliableTransportPort);

            //
            // Send the protocol message described by the header, with its contained
            // rewritten is-at messages out on the socket that corresponds to the
            // live interface we chose for sending.  Note that the actual destination
            //
            QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageQuietly(): SendProtocolMessage()"));
            if (msgVersion == 2) {
                SendProtocolMessage(m_liveInterfaces[i].m_multicastMDNSsockFd, ipv4address, interfaceAddressPrefixLen,
                                    flags, interfaceIsIPv4, packet, i);
            } else if (m_liveInterfaces[i].m_multicastsockFd != qcc::INVALID_SOCKET_FD) {
                SendProtocolMessage(m_liveInterfaces[i].m_multicastsockFd, ipv4address, interfaceAddressPrefixLen,
                                    flags, interfaceIsIPv4, packet, i);
            }
        }
    }
}

void IpNameServiceImpl::SendOutboundMessageActively(Packet packet, const qcc::IPAddress& localAddress)
{
    QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageActively()"));

    //
    // Make a note of what version this message is on, since there is a
    // difference in what information is there that will need to be passed down
    // in order to rewrite the addresses (see below).
    //
    uint32_t nsVersion, msgVersion;
    packet->GetVersion(nsVersion, msgVersion);

    //
    // We walk the list of live interfaces looking for those with IPv4 or IPv6
    // addresses, rewrite the messages as required for those interfaces and send
    // them out if they have been enabled.
    //
    QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageActively(): Walk interfaces"));

    bool removedUdp = false;
    bool removedTcp = false;
    std::list<MDNSResourceRecord> removedTcpAnswers;
    std::list<MDNSResourceRecord> removedUdpAnswers;
    std::list<MDNSQuestion> removedTcpQuestions;
    std::list<MDNSQuestion> removedUdpQuestions;
    std::map<uint32_t, std::list<String> > tcpNames;
    std::map<uint32_t, std::list<String> > udpNames;
    std::map<uint32_t, std::list<String> > tcpUdpNames;
    for (uint32_t i = 0; (m_state == IMPL_RUNNING || m_terminal) && (i < m_liveInterfaces.size()); ++i) {
        if (packet->InterfaceIndexSet()) {
            if (m_liveInterfaces[i].m_index != packet->GetInterfaceIndex()) {
                continue;
            }
            if (packet->AddressFamilySet()) {
                if (m_liveInterfaces[i].m_address.GetAddressFamily() != packet->GetAddressFamily()) {
                    continue;
                }
            }
        }
        QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageActively(): Checking out live interface %d. (\"%s\")",
                       i, m_liveInterfaces[i].m_interfaceName.c_str()));

        //
        // Don't bother to do anything if the socket FD isn't initialized, since
        // we wouldn't be able to send anyway.
        //

        if (m_liveInterfaces[i].m_multicastMDNSsockFd == qcc::INVALID_SOCKET_FD) {
            QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageActively(): Interface %d. is not live", i));
            continue;
        }

        QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageActively(): Interface %d. is live", i));

        //
        // We have a candidate interface to send the message out on.  The
        // question is whether or not the current interface as indicated by the
        // interface name is on the list of requested interfaces for the
        // transport mask found in the message.  If it is not, we must not send
        // this message out the current interface.
        //
        // The requested interfaces are stored on a per-transport basis.  Each
        // transport can open a different list of interfaces, and these lists
        // are selected by the <transportIndex> which is derived from the
        // transport mask passed to the originating advertisement or discovery
        // operation.  The transport mask comes to us in the questions and
        // answers stored in the message (header).
        //
        // To keep things at least slightly simpler, if any of the questions or
        // answers in our message have a transport mask that refers to a
        // transport that, in turn, has either a wildcard or the current
        // specific interface opened, we approve this interface as one to which
        // the message can be sent.
        //
        // We have to be careful about sending messages from transports that
        // open a wildcard interface as well.  Wildcard means all interfaces,
        // but all interfaces really means all except for "special use"
        // interfaces like Wi-Fi Direct interfaces on Android.  We don't know
        // what interfaces are actually in use by the Wi-Fi Direct subsystem but
        // it does seem that any P2P-based interface will begin with the string
        // "p2p" as in "p2p0" or "p2p-p2p0-0".
        //
        bool interfaceApproved = false;
        if (msgVersion <= 1) {
            NSPacket nsPacket = NSPacket::cast(packet);
            //
            // Do we have any questions that need to go out on this interface?
            //
            for (uint8_t j = 0; j < nsPacket->GetNumberQuestions(); ++j) {
                WhoHas* whoHas;
                nsPacket->GetQuestion(j, &whoHas);

                //
                // Get the transport mask referred to by the current question (who-has)
                // and convert the mask into an index into the per-transport data.
                //
                TransportMask transportMask = whoHas->GetTransportMask();
                assert(transportMask != TRANSPORT_NONE && "IpNameServiceImpl::SendOutboundMessageActively(): TransportMask must always be set");

                uint32_t transportIndex = IndexFromBit(transportMask);
                assert(transportIndex < 16 && "IpNameServiceImpl::SendOutboundMessageActively(): Bad transport index");
                if (transportIndex >= 16) {
                    return;
                }

                //
                // If this interface is requested as an outbound interface for this
                // transport, we approve sending it over that interface.
                //
                if (InterfaceRequested(transportIndex, i)) {
                    interfaceApproved = true;
                    break;
                }
            }

            //
            // Do we have any answers that need to go out on this interface?
            //
            for (uint8_t j = 0; j < nsPacket->GetNumberAnswers(); ++j) {
                IsAt* isAt;
                nsPacket->GetAnswer(j, &isAt);

                TransportMask transportMask = isAt->GetTransportMask();
                assert(transportMask != TRANSPORT_NONE && "IpNameServiceImpl::SendOutboundMessageActively(): TransportMask must always be set");

                uint32_t transportIndex = IndexFromBit(transportMask);
                assert(transportIndex < 16 && "IpNameServiceImpl::SendOutboundMessageActively(): Bad transport index");
                if (transportIndex >= 16) {
                    return;
                }

                //
                // If this interface is requested as an outbound interface for this
                // transport, we approve sending it over that interface.
                //
                if (InterfaceRequested(transportIndex, i)) {
                    interfaceApproved = true;
                    break;
                }
            }
        } else {
            //version two

            MDNSPacket mdnspacket = MDNSPacket::cast(packet);
            MDNSResourceRecord* answer;

            if (mdnspacket->GetHeader().GetQRType() == MDNSHeader::MDNS_QUERY) {
                MDNSQuestion* question;
                if (mdnspacket->GetQuestion("_alljoyn._tcp.local.", &question)) {

                    //
                    // If this interface is requested as an outbound interface for this
                    // transport, we approve sending it over that interface.
                    //
                    if (InterfaceRequested(TRANSPORT_INDEX_TCP, i)) {
                        interfaceApproved = true;
                    }

                }
                if (mdnspacket->GetQuestion("_alljoyn._udp.local.", &question)) {

                    //
                    // If this interface is requested as an outbound interface for this
                    // transport, we approve sending it over that interface.
                    //
                    if (InterfaceRequested(TRANSPORT_INDEX_UDP, i)) {
                        interfaceApproved = true;
                    }

                }
            } else {
                if (mdnspacket->GetAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, &answer)) {

                    //
                    // If this interface is requested as an outbound interface for this
                    // transport, we approve sending it over that interface.
                    //
                    if (InterfaceRequested(TRANSPORT_INDEX_TCP, i)) {
                        interfaceApproved = true;
                    }

                }
                if (mdnspacket->GetAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR, &answer)) {

                    //
                    // If this interface is requested as an outbound interface for this
                    // transport, we approve sending it over that interface.
                    //
                    if (InterfaceRequested(TRANSPORT_INDEX_UDP, i)) {
                        interfaceApproved = true;
                    }

                }
            }
        }

        //
        // If no questions nor answers of our message need to go out, then we
        // don't do anything on this interface.
        //
        if (interfaceApproved == false) {
            QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageActively(): No questions or answers for this interface"));
            continue;
        }

        //
        // When higher level code queues up messages, it doesn't know to what
        // interfaces and therefore over what source IP addresses we will be
        // using to send messages out on.  We expect our transport listeners to
        // be listening to the appropriate INADDR_ANY address, and that they are
        // relying on us to get the IP addressing information of the various
        // networks we are talking to correct when we send out address and port
        // information.  What this means is that we are going to rewrite any IP
        // addresses into is-at messages on the fly as we prepare to send them
        // out our sundry interfaces.  who-has messages don't include any source
        // addresses, so we can leave them as-is.
        //
        // The next thing we need to do is to pull out the IP address of the
        // current interface.  It may be either IPv4 or IPv6.
        //
        qcc::IPAddress ipv4address;
        uint16_t unicastPortv4 = 0;
        bool haveIPv4address = m_liveInterfaces[i].m_address.IsIPv4();
        if (haveIPv4address) {
            ipv4address = m_liveInterfaces[i].m_address;
            unicastPortv4 = m_liveInterfaces[i].m_unicastPort;
            QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageActively(): Interface %d. is IPv4", i));
        }
        bool interfaceIsIPv4 = haveIPv4address;

        qcc::IPAddress ipv6address;
        bool haveIPv6address = m_liveInterfaces[i].m_address.IsIPv6();
        if (haveIPv6address) {
            ipv6address = m_liveInterfaces[i].m_address;
            QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageActively(): Interface %d. is IPv6", i));
        }

        //
        // While we're here pulling out IP addresses, take the time to get the
        // network address prefix length (cf. net mask) and flags for the
        // interface.  We'll need to pass them on down to the method that does
        // the sending so it can figure out subnet directed broadcast addresses
        // if it needs to.
        //
        uint32_t interfaceAddressPrefixLen = m_liveInterfaces[i].m_prefixlen;
        uint32_t flags = m_liveInterfaces[i].m_flags;

        //
        // Each interface in our list is going to have either an IPv4 or an IPv6
        // address.  When we send the message, we want to send out both flavors
        // (Ipv4 and IPv6) over each interface since we want to maximize the
        // possibility that clients will actually receive this information
        // (i.e. we send IPv4 addressing over an IPv6 packet).  This is because
        // the probability to get a name service packet out is actually greater
        // over IPv6, but TCP transports want to listen on IPv4.  We do the
        // inverse just for consistency and to prepare for when TCP might
        // actually use IPv6.
        //
        // So, if the current address is IPv4, we scan for an IPv6 address on
        // another interface of the same name.  If the current address is IPv6,
        // we for an IPv4 address.
        //
        for (uint32_t j = 0; j < m_liveInterfaces.size(); ++j) {
            if (m_liveInterfaces[i].m_multicastMDNSsockFd == qcc::INVALID_SOCKET_FD ||
                m_liveInterfaces[j].m_interfaceName != m_liveInterfaces[i].m_interfaceName) {
                continue;
            }
            if (haveIPv4address == false && m_liveInterfaces[j].m_address.IsIPv4()) {
                QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageActively(): Interface %d. has IPv4 counterpart %d.", i, j));
                haveIPv4address = true;
                ipv4address = m_liveInterfaces[j].m_address;
                unicastPortv4 = m_liveInterfaces[j].m_unicastPort;
                break;
            }

            if (haveIPv6address == false && m_liveInterfaces[j].m_address.IsIPv6()) {
                QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageActively(): Interface %d. has IPv6 counterpart %d.", i, j));
                haveIPv6address = true;
                ipv6address = m_liveInterfaces[j].m_address;
                break;
            }
        }

        if (!haveIPv4address) {
            QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessageActively(): Interface %d does not have an IPv4 address.", i));
            continue;
        }
        //
        // At this point, we are ready to multicast out an interface and we know
        // both of our IPv4 and IPv6 addresses if they exist.  Now, we have to
        // walk the list of answer (is-at) messages and rewrite the provided addresses
        // that will correspond to the interface we are sending the message out
        // of.  Recall that until this point, nobody knew the addresses that the
        // message was going out over.  Question (who-has) messages don't have any
        // address information so we don't have to touch them.
        //
        //
        // Do the version-specific rewriting of the addresses in this NS/MDNS packet.
        //
        uint16_t reliableTransportPort = 0;
        uint16_t unreliableTransportPort = 0;
        if (m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].find("*") != m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
            reliableTransportPort = m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP]["*"];
        } else if (m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].find("0.0.0.0") != m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
            reliableTransportPort = m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP]["0.0.0.0"];
        } else if (m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].find(m_liveInterfaces[i].m_interfaceName) != m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
            reliableTransportPort = m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP][m_liveInterfaces[i].m_interfaceName];
        } else if (m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].find(m_liveInterfaces[i].m_interfaceAddr.ToString()) != m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
            reliableTransportPort = m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP][m_liveInterfaces[i].m_interfaceAddr.ToString()];
        }

        if (m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find("*") != m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
            unreliableTransportPort = m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP]["*"];
        } else if (m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find("0.0.0.0") != m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
            unreliableTransportPort = m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP]["0.0.0.0"];
        } else if (m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find(m_liveInterfaces[i].m_interfaceName) != m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
            unreliableTransportPort = m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP][m_liveInterfaces[i].m_interfaceName];
        } else if (m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find(m_liveInterfaces[i].m_interfaceAddr.ToString()) != m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
            unreliableTransportPort = m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP][m_liveInterfaces[i].m_interfaceAddr.ToString()];
        }

        bool ttlZero = false;
        if (msgVersion == 0) {
            NSPacket nsPacket  = NSPacket::cast(packet);
            if (nsPacket->GetNumberAnswers() && !reliableTransportPort) {
                continue;
            }
        } else if (msgVersion == 1) {
            NSPacket nsPacket  = NSPacket::cast(packet);
            if (nsPacket->GetNumberAnswers() && !reliableTransportPort && !unreliableTransportPort) {
                continue;
            }
        } else {
            MDNSPacket mdnsPacket = MDNSPacket::cast(packet);
            if (mdnsPacket->GetHeader().GetQRType() == MDNSHeader::MDNS_RESPONSE) {
                MDNSResourceRecord* ptrRecordTcp = NULL;
                MDNSResourceRecord* ptrRecordUdp = NULL;
                bool tcpAnswer = mdnsPacket->GetAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, &ptrRecordTcp);
                bool udpAnswer = mdnsPacket->GetAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR, &ptrRecordUdp);

                uint32_t numMatches = mdnsPacket->GetNumMatches("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS);
                for (uint32_t match = 0; match < numMatches; match++) {
                    MDNSResourceRecord* advRecord;
                    if (!mdnsPacket->GetAdditionalRecordAt("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, match, &advRecord)) {
                        continue;
                    }
                    uint32_t ttl = advRecord->GetRRttl();
                    if (!ttl) {
                        ttlZero = true;
                    }
                }
                if (!tcpAnswer && !udpAnswer) {
                    continue;
                }
                if (tcpAnswer && !udpAnswer) {
                    if (!reliableTransportPort && !ttlZero) {
                        continue;
                    }
                }
                if (udpAnswer && !tcpAnswer) {
                    if (!unreliableTransportPort && !ttlZero) {
                        continue;
                    }
                }
                if (tcpAnswer && udpAnswer) {
                    if (!reliableTransportPort && !unreliableTransportPort && !ttlZero) {
                        continue;
                    } else if (!reliableTransportPort) {
                        uint32_t numMatches = mdnsPacket->GetNumMatches("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS);
                        for (uint32_t match = 0; match < numMatches; match++) {
                            MDNSResourceRecord* advRecord;
                            if (!mdnsPacket->GetAdditionalRecordAt("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, match, &advRecord)) {
                                continue;
                            }
                            MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());
                            if (!advRData) {
                                continue;
                            }

                            if (!advRecord->GetRRttl()) {
                                continue;
                            }

                            uint32_t numNames = advRData->GetNumNames(TRANSPORT_TCP);
                            for (uint32_t j = 0; j < numNames; j++) {
                                String name = advRData->GetNameAt(TRANSPORT_TCP, j);
                                tcpNames[match].push_back(name);
                            }
                            numNames = advRData->GetNumNames(TRANSPORT_UDP);
                            for (uint32_t j = 0; j < numNames; j++) {
                                String name = advRData->GetNameAt(TRANSPORT_UDP, j);
                                udpNames[match].push_back(name);
                            }
                            numNames = advRData->GetNumNames(TRANSPORT_TCP | TRANSPORT_UDP);
                            for (uint32_t j = 0; j < numNames; j++) {
                                String name = advRData->GetNameAt(TRANSPORT_TCP | TRANSPORT_UDP, j);
                                tcpUdpNames[match].push_back(name);
                            }

                            advRData->Reset();
                            advRData->SetTransport(TRANSPORT_UDP);
                            if (udpNames.find(match) != udpNames.end()) {
                                for (std::list<String>::iterator iter = udpNames[match].begin(); iter != udpNames[match].end(); iter++) {
                                    advRData->SetValue("name", *iter);
                                }
                            }
                            if (tcpUdpNames.find(match) != tcpUdpNames.end()) {
                                for (std::list<String>::iterator iter = tcpUdpNames[match].begin(); iter != tcpUdpNames[match].end(); iter++) {
                                    advRData->SetValue("name", *iter);
                                }
                            }

                            if (packet->InterfaceIndexSet()) {
                                tcpNames.erase(match);
                                udpNames.erase(match);
                                tcpUdpNames.erase(match);
                            }
                        }
                        if (!ttlZero) {
                            removedTcp = true;
                            MDNSPtrRData* ptrRData = static_cast<MDNSPtrRData*>(ptrRecordTcp->GetRData());
                            String name = ptrRData->GetPtrDName();
                            if (!packet->InterfaceIndexSet()) {
                                MDNSResourceRecord* record;
                                if (mdnsPacket->GetAnswer(name, MDNSResourceRecord::SRV, &record)) {
                                    removedTcpAnswers.push_back(*record);
                                }
                                if (mdnsPacket->GetAnswer(name, MDNSResourceRecord::TXT, &record)) {
                                    removedTcpAnswers.push_back(*record);
                                }
                            }
                            removedTcpAnswers.push_back(*ptrRecordTcp);
                            mdnsPacket->RemoveAnswer(name, MDNSResourceRecord::SRV);
                            mdnsPacket->RemoveAnswer(name, MDNSResourceRecord::TXT);
                            mdnsPacket->RemoveAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR);
                        }
                    } else if (!unreliableTransportPort) {
                        MDNSResourceRecord* advRecord;
                        uint32_t numMatches = mdnsPacket->GetNumMatches("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS);
                        for (uint32_t match = 0; match < numMatches; match++) {
                            if (!mdnsPacket->GetAdditionalRecordAt("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, match, &advRecord)) {
                                continue;
                            }
                            MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());
                            if (!advRData) {
                                continue;
                            }
                            if (!advRecord->GetRRttl()) {
                                continue;
                            }
                            uint32_t numNames = advRData->GetNumNames(TRANSPORT_TCP);
                            for (uint32_t j = 0; j < numNames; j++) {
                                String name = advRData->GetNameAt(TRANSPORT_TCP, j);
                                tcpNames[match].push_back(name);
                            }
                            numNames = advRData->GetNumNames(TRANSPORT_UDP);
                            for (uint32_t j = 0; j < numNames; j++) {
                                String name = advRData->GetNameAt(TRANSPORT_UDP, j);
                                udpNames[match].push_back(name);
                            }
                            numNames = advRData->GetNumNames(TRANSPORT_TCP | TRANSPORT_UDP);
                            for (uint32_t j = 0; j < numNames; j++) {
                                String name = advRData->GetNameAt(TRANSPORT_TCP | TRANSPORT_UDP, j);
                                tcpUdpNames[match].push_back(name);
                            }

                            advRData->Reset();
                            advRData->SetTransport(TRANSPORT_TCP);
                            if (tcpNames.find(match) != tcpNames.end()) {
                                for (std::list<String>::iterator iter = tcpNames[match].begin(); iter != tcpNames[match].end(); iter++) {
                                    advRData->SetValue("name", *iter);
                                }
                            }
                            if (tcpUdpNames.find(match) != tcpUdpNames.end()) {
                                for (std::list<String>::iterator iter = tcpUdpNames[match].begin(); iter != tcpUdpNames[match].end(); iter++) {
                                    advRData->SetValue("name", *iter);
                                }
                            }
                            if (packet->InterfaceIndexSet()) {
                                tcpNames.erase(match);
                                udpNames.erase(match);
                                tcpUdpNames.erase(match);
                            }
                        }
                        if (!ttlZero) {
                            removedUdp = true;
                            MDNSPtrRData* ptrRData = static_cast<MDNSPtrRData*>(ptrRecordUdp->GetRData());
                            String name = ptrRData->GetPtrDName();
                            if (!packet->InterfaceIndexSet()) {
                                MDNSResourceRecord* record;
                                if (mdnsPacket->GetAnswer(name, MDNSResourceRecord::SRV, &record)) {
                                    removedUdpAnswers.push_back(*record);
                                }
                                if (mdnsPacket->GetAnswer(name, MDNSResourceRecord::TXT, &record)) {
                                    removedUdpAnswers.push_back(*record);
                                }
                            }
                            removedUdpAnswers.push_back(*ptrRecordUdp);
                            mdnsPacket->RemoveAnswer(name, MDNSResourceRecord::SRV);
                            mdnsPacket->RemoveAnswer(name, MDNSResourceRecord::TXT);
                            mdnsPacket->RemoveAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR);
                        }
                    }
                }
            } else {
                MDNSQuestion* questionUdp;
                MDNSQuestion* questionTcp;
                bool tcpQuestion = mdnsPacket->GetQuestion("_alljoyn._tcp.local.", &questionTcp);
                bool udpQuestion = mdnsPacket->GetQuestion("_alljoyn._udp.local.", &questionUdp);
                bool reliableTransportAllowed = (reliableTransportPort != 0);
                bool unreliableTransportAllowed = (unreliableTransportPort != 0);
                if (tcpQuestion && udpQuestion) {
                    if (!reliableTransportAllowed) {
                        removedTcp = true;
                        if (!packet->InterfaceIndexSet()) {
                            removedTcpQuestions.push_back(*questionTcp);
                        }
                        mdnsPacket->RemoveQuestion("_alljoyn._tcp.local.");
                    } else if (!unreliableTransportAllowed) {
                        removedUdp = true;
                        if (!packet->InterfaceIndexSet()) {
                            removedUdpQuestions.push_back(*questionUdp);
                        }
                        mdnsPacket->RemoveQuestion("_alljoyn._udp.local.");
                    }
                }
            }
        }

        if (ttlZero && !reliableTransportPort) {
            if (m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP].find("*") != m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
                reliableTransportPort = m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP]["*"];
            } else if (m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP].find("0.0.0.0") != m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
                reliableTransportPort = m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP]["0.0.0.0"];
            } else if (m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP].find(m_liveInterfaces[i].m_interfaceName) != m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
                reliableTransportPort = m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP][m_liveInterfaces[i].m_interfaceName];
            } else if (m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP].find(m_liveInterfaces[i].m_interfaceAddr.ToString()) != m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP].end()) {
                reliableTransportPort = m_priorReliableIPv4PortMap[TRANSPORT_INDEX_TCP][m_liveInterfaces[i].m_interfaceAddr.ToString()];
            }
        }

        if (ttlZero && !unreliableTransportPort) {
            if (m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find("*") != m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
                unreliableTransportPort = m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP]["*"];
            } else if (m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find("0.0.0.0") != m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
                unreliableTransportPort = m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP]["0.0.0.0"];
            } else if (m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find(m_liveInterfaces[i].m_interfaceName) != m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
                unreliableTransportPort = m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP][m_liveInterfaces[i].m_interfaceName];
            } else if (m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP].find(m_liveInterfaces[i].m_interfaceAddr.ToString()) != m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP].end()) {
                unreliableTransportPort = m_priorUnreliableIPv4PortMap[TRANSPORT_INDEX_UDP][m_liveInterfaces[i].m_interfaceAddr.ToString()];
            }
        }

        if (!reliableTransportPort && !unreliableTransportPort) {
            continue;
        }

        //
        // Do the version-specific rewriting of the addresses/ports in this NS/MDNS packet.
        //
        RewriteVersionSpecific(msgVersion, packet, haveIPv4address, ipv4address, haveIPv6address, ipv6address, unicastPortv4,
                               m_liveInterfaces[i].m_interfaceName, reliableTransportPort, unreliableTransportPort);

        //
        // Send the protocol message described by the header, with its contained
        // rewritten is-at messages out on the socket that corresponds to the
        // live interface we approved for sending.
        //
        if (msgVersion == 2) {
            SendProtocolMessage(m_liveInterfaces[i].m_multicastMDNSsockFd, ipv4address, interfaceAddressPrefixLen,
                                flags, interfaceIsIPv4, packet, i, localAddress);
        } else if (m_liveInterfaces[i].m_multicastsockFd != qcc::INVALID_SOCKET_FD) {
            SendProtocolMessage(m_liveInterfaces[i].m_multicastsockFd, ipv4address, interfaceAddressPrefixLen,
                                flags, interfaceIsIPv4, packet, i, localAddress);
        }
        if (removedTcp) {
            MDNSPacket mdnsPacket = MDNSPacket::cast(packet);
            for (std::list<MDNSResourceRecord>::const_iterator it = removedTcpAnswers.begin(); it != removedTcpAnswers.end(); it++) {
                mdnsPacket->AddAnswer(*it);
            }
            for (std::list<MDNSQuestion>::const_iterator it = removedTcpQuestions.begin(); it != removedTcpQuestions.end(); it++) {
                mdnsPacket->AddQuestion(*it);
            }
        }
        if (removedUdp) {
            MDNSPacket mdnsPacket = MDNSPacket::cast(packet);
            for (std::list<MDNSResourceRecord>::const_iterator it = removedUdpAnswers.begin(); it != removedUdpAnswers.end(); it++) {
                mdnsPacket->AddAnswer(*it);
            }
            for (std::list<MDNSQuestion>::const_iterator it = removedUdpQuestions.begin(); it != removedUdpQuestions.end(); it++) {
                mdnsPacket->AddQuestion(*it);
            }
        }

        if (removedTcp || removedUdp) {
            MDNSPacket mdnsPacket = MDNSPacket::cast(packet);
            uint32_t numMatches = mdnsPacket->GetNumMatches("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS);
            for (uint32_t match = 0; match < numMatches; match++) {
                if (tcpNames.find(match) == tcpNames.end() &&
                    udpNames.find(match) == udpNames.end() &&
                    tcpUdpNames.find(match) == tcpUdpNames.end()) {
                    continue;
                }

                MDNSResourceRecord* advRecord;
                if (!mdnsPacket->GetAdditionalRecordAt("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, match, &advRecord)) {
                    continue;
                }
                MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());
                if (!advRData) {
                    continue;
                }
                advRData->Reset();
                if (tcpNames.find(match) != tcpNames.end()) {
                    advRData->SetTransport(TRANSPORT_TCP);
                    for (std::list<String>::iterator iter = tcpNames[match].begin(); iter != tcpNames[match].end(); iter++) {
                        advRData->SetValue("name", *iter);
                    }
                }
                if (udpNames.find(match) != udpNames.end()) {
                    advRData->SetTransport(TRANSPORT_UDP);
                    for (std::list<String>::iterator iter = udpNames[match].begin(); iter != udpNames[match].end(); iter++) {
                        advRData->SetValue("name", *iter);
                    }
                }
                if (tcpUdpNames.find(match) != tcpUdpNames.end()) {
                    advRData->SetTransport(TRANSPORT_TCP | TRANSPORT_UDP);
                    for (std::list<String>::iterator iter = tcpUdpNames[match].begin(); iter != tcpUdpNames[match].end(); iter++) {
                        advRData->SetValue("name", *iter);
                    }
                }
            }
        }
        tcpNames.clear();
        udpNames.clear();
        tcpUdpNames.clear();
        removedTcpAnswers.clear();
        removedTcpQuestions.clear();
        removedUdpAnswers.clear();
        removedUdpQuestions.clear();
        removedTcp = false;
        removedUdp = false;
    }
}

void IpNameServiceImpl::SendOutboundMessages(void)
{
    QCC_DbgPrintf(("IpNameServiceImpl::SendOutboundMessages()"));
    int count =  m_outbound.size();
    //
    // Send any messages we have queued for transmission.  We expect to be
    // called with the mutex locked so we can wander around in the various
    // protected data structures freely.
    //
    while ((count) && (m_state == IMPL_RUNNING || m_terminal)) {

        count--;
        //
        // Pull a message off of the outbound queue.  What we get is a
        // header object that will tie together a number of "question"
        // (who-has) objects and a number of "answer" (is-at) objects.
        //
        Packet packet = m_outbound.front();
        //
        // We have the concept of quiet advertisements that imply quiet
        // (unicast) responses.  If we have a quiet response, we know because a
        // destination address will have been set in the header.
        //
        if (packet->DestinationSet()) {
            SendOutboundMessageQuietly(packet);
        } else {
            SendOutboundMessageActively(packet);
        }

        //
        // The current message has been sent to any and all of interfaces that
        // make sense, so we can discard it and loop back for another.
        //
        m_outbound.pop_front();
    }
}

void* IpNameServiceImpl::Run(void* arg)
{
    QCC_DbgPrintf(("IpNameServiceImpl::Run()"));

    //
    // This method is executed by the name service main thread and becomes the
    // center of the name service universe.  All incoming and outgoing messages
    // percolate through this thread because of the way we have to deal with
    // interfaces coming up and going down underneath us in a mobile
    // environment.  See the "Long Sidebar" comment above for some details on
    // the pain this has caused.
    //
    // Ultimately, this means we have a number of sockets open that correspond
    // to the "live" interfaces we are listening to.  We have to listen to all
    // of these sockets in what amounts to a select() below.  That means we
    // have live FDs waiting in the select.  On the other hand, we want to be
    // responsive in the case of a user turning on wireless and immediately
    // doing a Locate().  This requirement implies that we need to update the
    // interface state whenever we do a Locate.  This Locate() will be done in
    // the context of a user thread.  So we have a requirement that we avoid
    // changing the state of the FDs in another thread and the requirement
    // that we change the state of the FDs when the user wants to Locate().
    // Either we play synchronization games and distribute our logic or do
    // everything here.  Because it is easier to manage the process in one
    // place, we have all messages gonig through this thread.
    //
    size_t bufsize = NS_MESSAGE_MAX;
    uint8_t* buffer = new uint8_t[bufsize];

    //
    // Instantiate an event that fires after one second, and once per second
    // thereafter.  Used to drive protocol maintenance functions, especially
    // dealing with interface state changes.
    //
    const uint32_t MS_PER_SEC = 1000;
    qcc::Event timerEvent(MS_PER_SEC, MS_PER_SEC);

    qcc::NetworkEventSet networkEvents;

    qcc::SocketFd networkEventFd = qcc::INVALID_SOCKET_FD;
#ifndef QCC_OS_GROUP_WINDOWS
    networkEventFd = qcc::NetworkEventSocket();
    qcc::Event networkEvent(networkEventFd, qcc::Event::IO_READ);
#else
    qcc::Event networkEvent(true);
#endif

    qcc::Timespec tNow, tLastLazyUpdate;
    GetTimeNow(&tLastLazyUpdate);

    m_mutex.Lock();
    while ((m_state == IMPL_RUNNING) || (m_state == IMPL_STOPPING) || m_terminal) {
        //
        // If we are shutting down, we need to make sure that we send out the
        // terminal is-at messages that correspond to a CancelAdvertiseName for
        // any of the names we are advertising.  These messages are queued while
        // handling the thread stop event (below) and m_terminal is set to true.
        // So, if the thread has been asked to stop and stopEvent is still set,
        // run through the loop, so the messages can be queued and m_terminal
        // can be set to true.
        // The first time through the loop in which we find the m_outbound list
        // empty it means that all of the terminal messages have been sent and
        // we can exit.  So if we find m_terminal true and m_outbound.empty()
        // true, we break out of the loop and exit.
        //
        if (m_terminal && m_outbound.empty()) {
            QCC_DbgPrintf(("IpNameServiceImpl::Run(): m_terminal && m_outbound.empty() -> m_terminal = false"));
            m_terminal = false;
            break;
        }

        GetTimeNow(&tNow);

        //
        // In order to pass the Android Compatibility Test, we need to be able
        // to enable and disable communication with the outside world.  Enabling
        // is straightforward enough, but when we disable, we need to be careful
        // about turning things off before we've sent out all possibly queued
        // packets.
        //
        if (m_doEnable) {
            m_enabled = true;
            m_doEnable = false;
        }

        if (m_doDisable && m_outbound.empty()) {
            QCC_DbgPrintf(("IpNameServiceImpl::Run(): m_doDisable && m_outbound.empty() -> m_enabled = false"));
            m_enabled = false;
            m_doDisable = false;
        }

        //
        // We need to figure out which interfaces we can send and receive
        // protocol messages over.  On one hand, we don't want to get carried
        // away with multicast group joins and leaves since we could get tangled
        // up in IGMP rate limits.  On the other hand we want to do this often
        // enough to appear responsive to the user when she moves into proximity
        // with another device.
        //
        // Some quick measurements indicate that a Linux box can take about 15
        // seconds to associate, Windows and Android about 5 seconds.  Based on
        // the lower limits, it won't do much good to lazy update faster than
        // about once every five seconds; so we take that as an upper limit on
        // how often we allow a lazy update.  On the other hand, we want to
        // make sure we do a lazy update at least every 15 seconds.  We define
        // a couple of constants, LAZY_UPDATE_{MAX,MIN}_INTERVAL to allow this
        // range.
        //
        // What drives the middle ground between MAX and MIN timing?  The
        // presence or absence of FindAdvertisement() and AdvertiseName()
        // calls.  If the application is poked by an impatient user who "knows"
        // she should be able to connect, she may arrange to send out a
        // FindAdvertiseName() or AdvertiseName().  This is indicated to us by a
        // message on the m_outbound queue.
        //
        // So there are three basic cases which cause us to rn the lazy updater:
        //
        //     1) If m_forceLazyUpdate is true, some major configuration change
        //        has happened and we need to update no matter what.
        //
        //     2) If a message is found on the outbound queue, we need to do a
        //        lazy update if LAZY_UPDATE_MIN_INTERVAL has passed since the
        //        last update.
        //
        //     3) If LAZY_UPDATE_MAX_INTERVAL has elapsed since the last lazy
        //        update, we need to update.
        //
        if (m_forceLazyUpdate) {
            QCC_DbgPrintf(("IpNameServiceImpl::Run(): LazyUpdateInterfaces()"));
            LazyUpdateInterfaces(networkEvents);
            networkEvents.clear();
            tLastLazyUpdate = tNow;
            m_forceLazyUpdate = false;
        }
        SendOutboundMessages();

        //
        // Now, worry about what to do next.  Create a set of events to wait on.
        // We always wait on the stop event, the timer event and the event used
        // to signal us when an outging message is queued or a forced wakeup for
        // a lazy update is done.
        //
        vector<qcc::Event*> checkEvents, signaledEvents;
        checkEvents.push_back(&stopEvent);
        if (IsPeriodicMaintenanceTimerNeeded()) {
            checkEvents.push_back(&timerEvent);
        }
        checkEvents.push_back(&m_wakeEvent);
        checkEvents.push_back(&networkEvent);
        if (m_unicastEvent) {
            checkEvents.push_back(m_unicastEvent);
        }

        //
        // We also need to wait on events from all of the sockets that
        // correspond to the "live" interfaces we need to listen for inbound
        // multicast messages on.
        //
        for (uint32_t i = 0; i < m_liveInterfaces.size(); ++i) {
            if (m_liveInterfaces[i].m_multicastMDNSsockFd != qcc::INVALID_SOCKET_FD) {
                checkEvents.push_back(m_liveInterfaces[i].m_multicastMDNSevent);
            }
            if (m_liveInterfaces[i].m_multicastsockFd != qcc::INVALID_SOCKET_FD) {
                checkEvents.push_back(m_liveInterfaces[i].m_multicastevent);
            }
        }

        //
        // We are going to go to sleep for possibly as long as a second, so
        // we definitely need to release other (user) threads that might
        // be waiting to talk to us.
        //
        m_mutex.Unlock();

        //
        // Wait for something to happen.  if we get an error, there's not
        // much we can do about it but bail.
        //
        QStatus status = qcc::Event::Wait(checkEvents, signaledEvents);
        if (status != ER_OK && status != ER_TIMEOUT) {
            QCC_LogError(status, ("IpNameServiceImpl::Run(): Event::Wait(): Failed"));
            m_mutex.Lock();
            break;
        }


        //
        // Loop over the events for which we expect something has happened
        //
        for (vector<qcc::Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {
            if (*i == &stopEvent) {
                QCC_DbgPrintf(("IpNameServiceImpl::Run(): Stop event fired"));

                //
                // We heard the stop event, so reset it.  Our contract is that once
                // we've heard this event, we have to exit the run routine fairly
                // quickly.  We can take some time to clean up, but there will be
                // someone else eventually blocked waiting for us to exit, so we
                // can't get carried away.
                //
                stopEvent.ResetEvent();

                //
                // What we need to do is to send out is-at messages telling
                // anyone interested in our names that they are no longer valid.
                // This is a fairly complicated process that can involve sending
                // multiple packets out multiple interfaces, so we clearly don't
                // want to duplicate code here to make it all happen.  We use a
                // special case of normal operation to prevent new requests from
                // being queued, issue our own terminal requests corresponding to
                // the is-at messages metioned above, and then we run until they
                // are all processed and then we exit.
                //
                // Calling Retransmit(index, true, false) will queue the desired
                // terminal is-at messages from the given transport on the
                // m_outbound list.  To ensure that they are sent before we
                // exit, we set m_terminal to true.  We will have set m_state to
                // IMPL_STOPPING in IpNameServiceImpl::Stop.  This stops new
                // external requests from being acted upon.  We then continue in
                // our loop until the outbound queue is empty and then exit the
                // run routine (above).
                //
                m_terminal = true;

                for (uint32_t index = 0; index < N_TRANSPORTS; ++index) {
                    vector<String> empty;
                    Retransmit(index, true, false, qcc::IPEndpoint("0.0.0.0", 0), TRANSMIT_V0_V1, MaskFromIndex(index), empty);
                    Retransmit(index, true, false, qcc::IPEndpoint("0.0.0.0", 0), TRANSMIT_V2, TRANSPORT_TCP | TRANSPORT_UDP, empty);
                }
                break;
            } else if (*i == &timerEvent) {
                // QCC_DbgPrintf(("IpNameServiceImpl::Run(): Timer event fired"));
                //
                // This is an event that fires every second to give us a chance
                // to do any protocol maintenance, like retransmitting queued
                // advertisements.
                //
                DoPeriodicMaintenance();
            } else if (*i == &m_wakeEvent) {
                QCC_DbgPrintf(("IpNameServiceImpl::Run(): Wake event fired"));
                //
                // This is an event that fires whenever a message has been
                // queued on the outbound name service message queue.  We
                // always check the queue whenever we run through the loop,
                // (it'll happen before we sleep again) but we do have to reset
                // it.
                //
                m_wakeEvent.ResetEvent();
            } else if (*i == &networkEvent) {
                QCC_DbgPrintf(("IpNameServiceImpl::Run(): Network event fired"));
#ifndef QCC_OS_GROUP_WINDOWS
                NetworkEventType eventType = qcc::NetworkEventReceive(networkEventFd, networkEvents);
                if (eventType == QCC_RTM_DELADDR) {
                    m_forceLazyUpdate = true;
                }
                if (eventType == QCC_RTM_NEWADDR) {
                    m_forceLazyUpdate = true;
                    m_refreshAdvertisements = true;
                }
#else
                networkEvent.ResetEvent();
                m_forceLazyUpdate = true;
                m_refreshAdvertisements = true;
#endif
            } else {
                QCC_DbgPrintf(("IpNameServiceImpl::Run(): Socket event fired"));
                //
                // This must be activity on one of our multicast listener sockets.
                //
                qcc::SocketFd sockFd = (*i)->GetFD();

                QCC_DbgPrintf(("IpNameServiceImpl::Run(): Call qcc::RecvFrom()"));

                qcc::IPAddress remoteAddress, localAddress;
                uint16_t remotePort;
                size_t nbytes;
                int32_t localInterfaceIndex;

                QStatus status = qcc::RecvWithAncillaryData(sockFd, remoteAddress, remotePort, localAddress,
                                                            buffer, bufsize, nbytes, localInterfaceIndex);

                if (status != ER_OK) {
                    //
                    // We have a RecvFrom error.  We want to avoid states where
                    // we get repeated read errors and just end up in an
                    // infinite loop getting errors sucking up all available
                    // CPU, so we make sure we sleep for at least a short time
                    // after detecting the error.
                    //
                    // Our basic strategy is to hope that this is a transient
                    // error, or one that will be recovered at the next lazy
                    // update.  We don't want to blindly force a lazy update
                    // or we may get into an infinite lazy update loop, so
                    // the worst that can happen is that we introduce a short
                    // delay here in our handler whenever we detect an error.
                    //
                    // On Windows ER_WOULBLOCK can be expected because it takes
                    // an initial call to recv to determine if the socket is readable.
                    //
                    if (status != ER_WOULDBLOCK) {
                        QCC_LogError(status, ("IpNameServiceImpl::Run(): qcc::RecvFrom(%d, ...): Failed", sockFd));
                        qcc::Sleep(1);
                    }
                    continue;
                }

                QCC_DbgHLPrintf(("IpNameServiceImpl::Run(): Got IPNS message from \"%s\"", remoteAddress.ToString().c_str()));

                // Find out the destination port and interface index for this message.
                uint16_t recvPort = std::numeric_limits<uint16_t>::max();
                int32_t ifIndex = -1;
                bool destIsIPv4Local = false;
                bool destIsIPv6Local = false;
                String ifName;

                for (uint32_t i = 0; i < m_liveInterfaces.size(); ++i) {

                    if (m_liveInterfaces[i].m_multicastMDNSsockFd == sockFd) {
                        recvPort = m_liveInterfaces[i].m_multicastMDNSPort;
                        ifIndex = m_liveInterfaces[i].m_index;
                        ifName = m_liveInterfaces[i].m_interfaceName;
                    }
                    if (m_liveInterfaces[i].m_multicastsockFd == sockFd) {
                        recvPort = m_liveInterfaces[i].m_multicastPort;
                        ifIndex = m_liveInterfaces[i].m_index;
                        ifName = m_liveInterfaces[i].m_interfaceName;
                    }

                    if (!destIsIPv4Local && m_liveInterfaces[i].m_address.IsIPv4()
                        && localAddress == m_liveInterfaces[i].m_address) {
                        destIsIPv4Local = true;
                        recvPort = m_liveInterfaces[i].m_unicastPort;
                        ifIndex =  m_liveInterfaces[i].m_index;
                    }

                    if (!destIsIPv6Local && m_liveInterfaces[i].m_address.IsIPv6()
                        && localAddress == m_liveInterfaces[i].m_address) {
                        destIsIPv6Local = true;
                        recvPort = m_liveInterfaces[i].m_unicastPort;
                        ifIndex =  m_liveInterfaces[i].m_index;
                    }
                }

                if (recvPort != std::numeric_limits<uint16_t>::max() && ifIndex != -1) {
                    QCC_DbgHLPrintf(("Processing packet on interface index %d that was received on index %d from %s:%u to %s:%u",
                                     ifIndex, localInterfaceIndex, remoteAddress.ToString().c_str(), remotePort, localAddress.ToString().c_str(), recvPort));
                }
                if (ifIndex != -1 && !destIsIPv4Local && ifIndex != localInterfaceIndex) {
                    QCC_DbgHLPrintf(("Ignoring non-unicast or unexpected packet that was received on a different interface"));
                    continue;
                }
                //
                // We got a message over the multicast channel.  Deal with it.
                //
                if (recvPort != std::numeric_limits<uint16_t>::max() && ifIndex != -1) {
                    qcc::IPEndpoint endpoint(remoteAddress, remotePort);
                    HandleProtocolMessage(buffer, nbytes, endpoint, recvPort, ifIndex, localAddress);
                }
            }
        }
        m_mutex.Lock();
    }
    m_mutex.Unlock();

    // We took the time to send out a final
    // advertisement(s) above, indicating that we are going away.
    // Clear live interfaces and exit.
    ClearLiveInterfaces();

    if (networkEventFd != qcc::INVALID_SOCKET_FD) {
        qcc::Close(networkEventFd);
    }

    delete [] buffer;
    return 0;
}

void IpNameServiceImpl::GetResponsePackets(std::list<Packet>& packets, bool quietly, const qcc::IPEndpoint destination, uint8_t type,
                                           TransportMask completeTransportMask, const int32_t interfaceIndex, const qcc::AddressFamily family)
{
    m_mutex.Lock();
    bool tcpProcessed = false;
    bool udpProcessed = false;
    for (uint32_t transportIndex = 0; transportIndex < N_TRANSPORTS; ++transportIndex) {
        if ((type & TRANSMIT_V2) && !m_advertised[transportIndex].empty()) {
            MDNSSenderRData senderRData;
            MDNSResourceRecord refRecord("sender-info." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, &senderRData);

            MDNSARData addrRData;
            MDNSResourceRecord aRecord(m_guid + ".local.", MDNSResourceRecord::A, MDNSResourceRecord::INTERNET, 120, &addrRData);

            MDNSAAAARData aaaaRData;
            MDNSResourceRecord aaaaRecord(m_guid + ".local.", MDNSResourceRecord::AAAA, MDNSResourceRecord::INTERNET, 120, &aaaaRData);
            uint32_t aaaaRecordSize = aaaaRecord.GetSerializedSize();

            int32_t id = IncrementAndFetch(&INCREMENTAL_PACKET_ID);

            MDNSHeader mdnsHeader(id, MDNSHeader::MDNS_RESPONSE);

            MDNSPtrRData ptrRDataTcp;
            ptrRDataTcp.SetPtrDName(m_guid + "._alljoyn._tcp.local.");
            MDNSResourceRecord ptrRecordTcp("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, MDNSResourceRecord::INTERNET, 120, &ptrRDataTcp);

            MDNSSrvRData srvRDataTcp(1 /*priority */, 1 /* weight */,
                                     0 /* port */, m_guid + ".local." /* target */);
            MDNSResourceRecord srvRecordTcp(m_guid + "._alljoyn._tcp.local.", MDNSResourceRecord::SRV, MDNSResourceRecord::INTERNET, 120, &srvRDataTcp);

            MDNSTextRData txtRDataTcp;

            MDNSPtrRData ptrRDataUdp;
            ptrRDataUdp.SetPtrDName(m_guid + "._alljoyn._udp.local.");
            MDNSResourceRecord ptrRecordUdp("_alljoyn._udp.local.", MDNSResourceRecord::PTR, MDNSResourceRecord::INTERNET, 120, &ptrRDataUdp);

            MDNSSrvRData srvRDataUdp(1 /*priority */, 1 /* weight */,
                                     0 /* port */, m_guid + ".local." /* target */);
            MDNSResourceRecord srvRecordUdp(m_guid + "._alljoyn._udp.local.", MDNSResourceRecord::SRV, MDNSResourceRecord::INTERNET, 120, &srvRDataUdp);

            MDNSTextRData txtRDataUdp;

            MDNSAdvertiseRData advertiseRData;

            MDNSResourceRecord advertiseRecord("advertise." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, &advertiseRData);

            MDNSPacket pilotPacket;
            pilotPacket->SetHeader(mdnsHeader);
            pilotPacket->SetVersion(2, 2);

            if (m_reliableIPv6Port[TRANSPORT_INDEX_TCP]) {
                txtRDataTcp.SetValue("r6port", U32ToString(m_reliableIPv6Port[TRANSPORT_INDEX_TCP]));
            }
            MDNSResourceRecord txtRecordTcp(m_guid + "._alljoyn._tcp.local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, &txtRDataTcp);

            if (m_unreliableIPv6Port[TRANSPORT_INDEX_UDP]) {
                txtRDataUdp.SetValue("u6port", U32ToString(m_unreliableIPv6Port[TRANSPORT_INDEX_UDP]));
            }

            MDNSResourceRecord txtRecordUdp(m_guid + "._alljoyn._udp.local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, &txtRDataUdp);

            pilotPacket->AddAdditionalRecord(advertiseRecord);
            pilotPacket->AddAdditionalRecord(refRecord);
            pilotPacket->AddAdditionalRecord(aRecord);
            if (quietly) {
                pilotPacket->SetDestination(destination);
            } else {
                pilotPacket->ClearDestination();
                if (interfaceIndex != -1) {
                    pilotPacket->SetInterfaceIndex(interfaceIndex);
                }
                if (family != qcc::QCC_AF_UNSPEC) {
                    pilotPacket->SetAddressFamily(family);
                }
            }

            MDNSResourceRecord* advRecord;
            pilotPacket->GetAdditionalRecord("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &advRecord);
            MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());

            MDNSResourceRecord* refRecord1;
            pilotPacket->GetAdditionalRecord("sender-info.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &refRecord1);
            MDNSSenderRData* refRData = static_cast<MDNSSenderRData*>(refRecord1->GetRData());

            refRData->SetSearchID(id);
            packets.push_back(Packet::cast(pilotPacket));

            TransportMask transportMaskArr[3] = { TRANSPORT_TCP, TRANSPORT_UDP, TRANSPORT_TCP | TRANSPORT_UDP };

            if ((transportIndex == IndexFromBit(TRANSPORT_TCP) && tcpProcessed)  ||
                (transportIndex == IndexFromBit(TRANSPORT_UDP) && udpProcessed)) {
                continue;
            }
            for (int i = 0; i < 3; i++) {
                TransportMask tm = transportMaskArr[i];
                if (completeTransportMask == TRANSPORT_TCP) {
                    if (tm == TRANSPORT_UDP) {
                        continue;
                    }
                } else if (completeTransportMask == TRANSPORT_UDP) {
                    if (tm == TRANSPORT_TCP) {
                        continue;
                    }
                } else if (completeTransportMask != (TRANSPORT_TCP | TRANSPORT_UDP)) {
                    continue;
                }

                set<String> advertising = GetAdvertising(tm);
                int count = 0;
                for (set<qcc::String>::iterator i = advertising.begin(); i != advertising.end(); ++i) {
                    QCC_DbgPrintf(("IpNameServiceImpl::GetResponsePackets(): Accumulating \"%s\"", (*i).c_str()));

                    //
                    // It is possible that we have accumulated more advertisements than will
                    // fit in a UDP IpNameServiceImpl packet.  A name service is-at message is going
                    // to consist of a header and its answer section, which is made from an
                    // IsAt object.  We first ask both of these objects to return their size
                    // so we know how much space is committed already.  Note that we ask the
                    // header for its max possible size since the header may be modified to
                    // add actual IPv4 and IPv6 addresses when it is sent.
                    //
                    size_t currentSize = packets.back()->GetSerializedSize();

                    //
                    // This isn't terribly elegant, but we don't know the IP address(es)
                    // over which the message will be sent.  These are added in the loop
                    // that actually does the packet sends, with the interface addresses
                    // dynamically added onto the message.  We have no clue here if an IPv4
                    // or IPv6 or both flavors of address will exist on a given interface,
                    // nor how many interfaces there are.  All we can do here is to assume
                    // the worst case for the size (both exist) and add the 20 bytes (four
                    // for IPv4, sixteen for IPv6) that the addresses may consume in the
                    // final packet.
                    //
                    currentSize += aaaaRecordSize;

                    MDNSAdvertiseRData currentAdvert;
                    currentAdvert.SetUniqueCount(advRData->GetUniqueCount());
                    if (!count) {
                        currentAdvert.SetTransport(tm);
                    }
                    currentAdvert.SetValue("name", *i);
                    uint32_t currentAdvertSize = currentAdvert.GetSerializedSize() - 2;
                    //
                    // We cheat a little in order to avoid a string copy and use our
                    // knowledge that names are stored as a byte count followed by the
                    // string bytes.  If the current name won't fit into the currently
                    // assembled message, we need to flush the current message and start
                    // again.
                    //
                    if (currentSize + currentAdvertSize > NS_MESSAGE_MAX) {
                        QCC_DbgPrintf(("IpNameServiceImpl::GetResponsePackets(): Message is full"));
                        QCC_DbgPrintf(("IpNameServiceImpl::GetResponsePackets(): Sending partial list"));
                        id = IncrementAndFetch(&INCREMENTAL_PACKET_ID);
                        mdnsHeader.SetId(id);
                        MDNSPacket additionalPacket;
                        additionalPacket->SetHeader(mdnsHeader);

                        if ((tm & TRANSPORT_TCP) && (!m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].empty() || m_reliableIPv6Port[TRANSPORT_INDEX_TCP])) {
                            MDNSResourceRecord txtRecordTcp(m_guid + "._alljoyn._tcp.local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, &txtRDataTcp);
                            additionalPacket->AddAnswer(ptrRecordTcp);
                            additionalPacket->AddAnswer(srvRecordTcp);
                            additionalPacket->AddAnswer(txtRecordTcp);
                        }

                        if (tm & TRANSPORT_UDP && (!m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].empty() || m_unreliableIPv6Port[TRANSPORT_INDEX_UDP])) {
                            MDNSResourceRecord txtRecordUdp(m_guid + "._alljoyn._udp.local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, &txtRDataUdp);
                            additionalPacket->AddAnswer(ptrRecordUdp);
                            additionalPacket->AddAnswer(srvRecordUdp);
                            additionalPacket->AddAnswer(txtRecordUdp);
                        }

                        additionalPacket->AddAdditionalRecord(advertiseRecord);
                        additionalPacket->AddAdditionalRecord(refRecord);
                        additionalPacket->AddAdditionalRecord(aRecord);
                        additionalPacket->SetVersion(2, 2);
                        additionalPacket->GetAdditionalRecord("advertise.*", MDNSResourceRecord::TXT, &advRecord);
                        advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());

                        additionalPacket->GetAdditionalRecord("sender-info.*", MDNSResourceRecord::TXT, &refRecord1);
                        refRData = static_cast<MDNSSenderRData*>(refRecord1->GetRData());
                        advRData->Reset();
                        advRData->SetTransport(tm);
                        advRData->SetValue("name", *i);
                        refRData->SetSearchID(id);
                        if (quietly) {
                            additionalPacket->SetDestination(destination);
                        } else {
                            additionalPacket->ClearDestination();
                            if (interfaceIndex != -1) {
                                additionalPacket->SetInterfaceIndex(interfaceIndex);
                            }
                            if (family != qcc::QCC_AF_UNSPEC) {
                                additionalPacket->SetAddressFamily(family);
                            }
                        }
                        packets.push_back(Packet::cast(additionalPacket));
                        count = 1;
                    } else {
                        QCC_DbgPrintf(("IpNameServiceImpl::GetResponsePackets(): Message has room.  Adding \"%s\"", (*i).c_str()));
                        MDNSResourceRecord* answer;
                        bool tcpAnswer = MDNSPacket::cast(packets.back())->GetAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, &answer);
                        bool udpAnswer = MDNSPacket::cast(packets.back())->GetAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR, &answer);
                        if (!udpAnswer && (tm & TRANSPORT_UDP) && (!m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].empty() || m_unreliableIPv6Port[TRANSPORT_INDEX_UDP])) {
                            MDNSPacket::cast(packets.back())->AddAnswer(ptrRecordUdp);
                            MDNSPacket::cast(packets.back())->AddAnswer(srvRecordUdp);
                            MDNSPacket::cast(packets.back())->AddAnswer(txtRecordUdp);
                        }
                        if (!tcpAnswer && (tm & TRANSPORT_TCP) && (!m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].empty() || m_reliableIPv6Port[TRANSPORT_INDEX_TCP])) {
                            MDNSPacket::cast(packets.back())->AddAnswer(ptrRecordTcp);
                            MDNSPacket::cast(packets.back())->AddAnswer(srvRecordTcp);
                            MDNSPacket::cast(packets.back())->AddAnswer(txtRecordTcp);
                        }
                        if (!count) {
                            advRData->SetTransport(tm);
                        }
                        advRData->SetValue("name", *i);
                        count++;
                    }
                }
                if (quietly) {
                    set<String> advertising_quietly = GetAdvertisingQuietly(tm);

                    for (set<qcc::String>::iterator i = advertising_quietly.begin(); i != advertising_quietly.end(); ++i) {
                        QCC_DbgPrintf(("IpNameServiceImpl::GetResponsePackets(): Accumulating (quiet) \"%s\"", (*i).c_str()));

                        size_t currentSize = packets.back()->GetSerializedSize();
                        currentSize += aaaaRecordSize;

                        MDNSAdvertiseRData currentAdvert;
                        currentAdvert.SetUniqueCount(advRData->GetUniqueCount());
                        if (!count) {
                            currentAdvert.SetTransport(tm);
                        }
                        advRData->SetValue("name", *i);
                        uint32_t currentAdvertSize = currentAdvert.GetSerializedSize() - 2;
                        if (currentSize + currentAdvertSize  > NS_MESSAGE_MAX) {
                            QCC_DbgPrintf(("IpNameServiceImpl::GetResponsePackets(): Message is full"));
                            QCC_DbgPrintf(("IpNameServiceImpl::GetResponsePackets(): Sending partial list"));

                            id = IncrementAndFetch(&INCREMENTAL_PACKET_ID);
                            mdnsHeader.SetId(id);
                            MDNSPacket additionalPacket;
                            additionalPacket->SetHeader(mdnsHeader);

                            if ((tm & TRANSPORT_TCP) && (!m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].empty() || m_reliableIPv6Port[TRANSPORT_INDEX_TCP])) {
                                if (m_reliableIPv6Port[TRANSPORT_INDEX_TCP]) {
                                    txtRDataTcp.SetValue("r6port", U32ToString(m_reliableIPv6Port[TRANSPORT_INDEX_TCP]));
                                }
                                MDNSResourceRecord txtRecordTcp(m_guid + "._alljoyn._tcp.local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, &txtRDataTcp);
                                additionalPacket->AddAnswer(ptrRecordTcp);
                                additionalPacket->AddAnswer(srvRecordTcp);
                                additionalPacket->AddAnswer(txtRecordTcp);
                            }

                            if (tm & TRANSPORT_UDP && (!m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].empty() || m_unreliableIPv6Port[TRANSPORT_INDEX_UDP])) {
                                if (m_unreliableIPv6Port[TRANSPORT_INDEX_UDP]) {
                                    txtRDataUdp.SetValue("u6port", U32ToString(m_unreliableIPv6Port[TRANSPORT_INDEX_UDP]));
                                }
                                MDNSResourceRecord txtRecordUdp(m_guid + "._alljoyn._udp.local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, &txtRDataUdp);
                                additionalPacket->AddAnswer(ptrRecordUdp);
                                additionalPacket->AddAnswer(srvRecordUdp);
                                additionalPacket->AddAnswer(txtRecordUdp);
                            }

                            additionalPacket->AddAdditionalRecord(advertiseRecord);
                            additionalPacket->AddAdditionalRecord(refRecord);
                            additionalPacket->AddAdditionalRecord(aRecord);
                            additionalPacket->SetVersion(2, 2);
                            additionalPacket->GetAdditionalRecord("advertise.*", MDNSResourceRecord::TXT, &advRecord);
                            advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());

                            additionalPacket->GetAdditionalRecord("sender-info.*", MDNSResourceRecord::TXT, &refRecord1);
                            refRData = static_cast<MDNSSenderRData*>(refRecord1->GetRData());
                            advRData->Reset();
                            advRData->SetTransport(tm);
                            advRData->SetValue("name", *i);
                            refRData->SetSearchID(id);
                            additionalPacket->SetDestination(destination);
                            packets.push_back(Packet::cast(additionalPacket));
                            count = 1;
                        } else {
                            MDNSResourceRecord* answer;
                            bool tcpAnswer = MDNSPacket::cast(packets.back())->GetAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, &answer);
                            bool udpAnswer = MDNSPacket::cast(packets.back())->GetAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR, &answer);
                            if (!udpAnswer && (tm & TRANSPORT_UDP) && (!m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].empty() || m_unreliableIPv6Port[TRANSPORT_INDEX_UDP])) {
                                MDNSPacket::cast(packets.back())->AddAnswer(ptrRecordUdp);
                                MDNSPacket::cast(packets.back())->AddAnswer(srvRecordUdp);
                                MDNSPacket::cast(packets.back())->AddAnswer(txtRecordUdp);
                            }
                            if (!tcpAnswer && (tm & TRANSPORT_TCP) && (!m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].empty() || m_reliableIPv6Port[TRANSPORT_INDEX_TCP])) {
                                MDNSPacket::cast(packets.back())->AddAnswer(ptrRecordTcp);
                                MDNSPacket::cast(packets.back())->AddAnswer(srvRecordTcp);
                                MDNSPacket::cast(packets.back())->AddAnswer(txtRecordTcp);
                            }
                            if (!count) {
                                advRData->SetTransport(tm);
                            }
                            QCC_DbgPrintf(("IpNameServiceImpl::GetResponsePackets(): Message has room.  Adding (quiet) \"%s\"", (*i).c_str()));
                            advRData->SetValue("name", *i);
                            count++;
                        }
                    }
                }
            }
            tcpProcessed = true;
            udpProcessed = true;
        }
    }
    m_mutex.Unlock();
}

void IpNameServiceImpl::GetQueryPackets(std::list<Packet>& packets, const uint8_t type, const int32_t interfaceIndex, const qcc::AddressFamily family)
{
    m_mutex.Lock();
    for (uint32_t transportIndex = 0; transportIndex < N_TRANSPORTS; ++transportIndex) {
        if (m_enableV1 && (type & TRANSMIT_V0_V1) && !m_v0_v1_queries[transportIndex].empty()) {

            {
                uint32_t nQuerySent = 0;
                WhoHas whoHas;
                whoHas.SetVersion(0, 0);
                whoHas.SetTransportMask(MaskFromIndex(transportIndex));
                whoHas.SetTcpFlag(true);
                whoHas.SetUdpFlag(true);
                whoHas.SetIPv4Flag(true);

                NSPacket pilotPacket;
                pilotPacket->SetVersion(0, 0);
                pilotPacket->SetTimer(m_tDuration);
                pilotPacket->AddQuestion(whoHas);
                pilotPacket->ClearDestination();
                if (interfaceIndex != -1) {
                    pilotPacket->SetInterfaceIndex(interfaceIndex);
                }
                if (family != qcc::QCC_AF_UNSPEC) {
                    pilotPacket->SetAddressFamily(family);
                }
                packets.push_back(Packet::cast(pilotPacket));
                ++nQuerySent;

                WhoHas* pWhoHas;
                pilotPacket->GetQuestion(0, &pWhoHas);
                for (set<qcc::String>::const_iterator i = m_v0_v1_queries[transportIndex].begin(); i != m_v0_v1_queries[transportIndex].end(); ++i) {
                    size_t currentSize = packets.back()->GetSerializedSize();
                    currentSize += 20;
                    if (currentSize + 1 + i->size() > NS_MESSAGE_MAX) {
                        QCC_DbgPrintf(("IpNameServiceImpl::GetQueryPackets(): Resetting current list"));
                        NSPacket additionalPacket;
                        whoHas.Reset();
                        whoHas.AddName(*i);
                        additionalPacket->SetVersion(0, 0);
                        additionalPacket->SetTimer(m_tDuration);
                        additionalPacket->AddQuestion(whoHas);
                        if (interfaceIndex != -1) {
                            additionalPacket->SetInterfaceIndex(interfaceIndex);
                        }
                        if (family != qcc::QCC_AF_UNSPEC) {
                            additionalPacket->SetAddressFamily(family);
                        }
                        packets.push_back(Packet::cast(additionalPacket));
                        additionalPacket->GetQuestion(0, &pWhoHas);
                        ++nQuerySent;
                    } else {
                        pWhoHas->AddName(*i);
                    }
                }
            }

            {
                WhoHas whoHas;
                whoHas.SetVersion(1, 1);
                whoHas.SetTransportMask(MaskFromIndex(transportIndex));

                NSPacket pilotPacket;
                pilotPacket->SetVersion(1, 1);
                pilotPacket->SetTimer(m_tDuration);
                pilotPacket->AddQuestion(whoHas);
                pilotPacket->ClearDestination();
                if (interfaceIndex != -1) {
                    pilotPacket->SetInterfaceIndex(interfaceIndex);
                }
                if (family != qcc::QCC_AF_UNSPEC) {
                    pilotPacket->SetAddressFamily(family);
                }
                packets.push_back(Packet::cast(pilotPacket));

                WhoHas* pWhoHas;
                pilotPacket->GetQuestion(0, &pWhoHas);
                for (set<qcc::String>::const_iterator i = m_v0_v1_queries[transportIndex].begin(); i != m_v0_v1_queries[transportIndex].end(); ++i) {
                    size_t currentSize = packets.back()->GetSerializedSize();
                    currentSize += 20;
                    if (currentSize + 1 + i->size() > NS_MESSAGE_MAX) {
                        NSPacket additionalPacket;
                        whoHas.Reset();
                        whoHas.AddName(*i);
                        additionalPacket->SetVersion(1, 1);
                        additionalPacket->SetTimer(m_tDuration);
                        additionalPacket->AddQuestion(whoHas);
                        if (interfaceIndex != -1) {
                            additionalPacket->SetInterfaceIndex(interfaceIndex);
                        }
                        if (family != qcc::QCC_AF_UNSPEC) {
                            additionalPacket->SetAddressFamily(family);
                        }
                        packets.push_back(Packet::cast(additionalPacket));
                        additionalPacket->GetQuestion(0, &pWhoHas);
                    } else {
                        pWhoHas->AddName(*i);
                    }
                }
            }
        }
    }

    MDNSQuestion mdnsTCPQuestion("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, MDNSResourceRecord::INTERNET);
    MDNSQuestion mdnsUDPQuestion("_alljoyn._udp.local.", MDNSResourceRecord::PTR, MDNSResourceRecord::INTERNET);

    MDNSAAAARData aaaaRData;
    MDNSResourceRecord aaaaRecord(m_guid + ".local.", MDNSResourceRecord::AAAA, MDNSResourceRecord::INTERNET, 120, &aaaaRData);
    uint32_t aaaaRecordSize = aaaaRecord.GetSerializedSize();

    MDNSPacket pilotPacket;
    int32_t id = IncrementAndFetch(&INCREMENTAL_PACKET_ID);
    MDNSHeader mdnsHeader(id, MDNSHeader::MDNS_QUERY);
    pilotPacket->SetHeader(mdnsHeader);
    pilotPacket->SetVersion(2, 2);

    if (!m_v2_queries[TRANSPORT_INDEX_UDP].empty()) {
        pilotPacket->AddQuestion(mdnsUDPQuestion);
    }
    if (!m_v2_queries[TRANSPORT_INDEX_TCP].empty()) {
        pilotPacket->AddQuestion(mdnsTCPQuestion);
    }

    MDNSSearchRData searchRefData;
    MDNSResourceRecord searchRecord("search." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, &searchRefData);

    MDNSSenderRData senderRData;
    MDNSResourceRecord refRecord("sender-info." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, 120, &senderRData);

    pilotPacket->AddAdditionalRecord(searchRecord);
    pilotPacket->AddAdditionalRecord(refRecord);


    MDNSResourceRecord* searchRecord1;
    pilotPacket->GetAdditionalRecord("search.*", MDNSResourceRecord::TXT, &searchRecord1);
    MDNSSearchRData* searchRData = static_cast<MDNSSearchRData*>(searchRecord1->GetRData());

    MDNSResourceRecord* refRecord1;
    pilotPacket->GetAdditionalRecord("sender-info.*", MDNSResourceRecord::TXT, &refRecord1);
    MDNSSenderRData* refRData = static_cast<MDNSSenderRData*>(refRecord1->GetRData());
    refRData->SetSearchID(id);
    pilotPacket->ClearDestination();
    if (interfaceIndex != -1) {
        pilotPacket->SetInterfaceIndex(interfaceIndex);
    }
    if (family != qcc::QCC_AF_UNSPEC) {
        pilotPacket->SetAddressFamily(family);
    }
    bool pilotAdded = false;
    uint32_t count = 0;
    set<qcc::String> addedQueries;
    for (uint32_t transportIndex = 0; transportIndex < N_TRANSPORTS; ++transportIndex) {
        if ((type & TRANSMIT_V2) && !m_v2_queries[transportIndex].empty()) {
            if (!pilotAdded) {
                packets.push_back(Packet::cast(pilotPacket));
                pilotAdded = true;
            }
            for (set<qcc::String>::const_iterator i = m_v2_queries[transportIndex].begin(); i != m_v2_queries[transportIndex].end(); ++i) {
                if (addedQueries.find(*i) != addedQueries.end() && (transportIndex == TRANSPORT_INDEX_UDP || transportIndex == TRANSPORT_INDEX_TCP)) {
                    continue;
                }
                size_t currentSize = packets.back()->GetSerializedSize();
                currentSize += aaaaRecordSize;
                MatchMap matching;
                ParseMatchRule(*i, matching);
                MDNSSearchRData currentQuery;
                currentQuery.SetUniqueCount(searchRData->GetUniqueCount());
                for (MatchMap::iterator j = matching.begin(); j != matching.end(); ++j) {
                    currentQuery.SetValue(j->first, j->second);
                }
                currentQuery.SetValue(";");
                uint32_t currentQuerySize = currentQuery.GetSerializedSize() - 2;
                if ((currentSize + currentQuerySize) > NS_MESSAGE_MAX) {
                    QCC_DbgPrintf(("IpNameServiceImpl::GetQueryPackets(): Message is full"));
                    QCC_DbgPrintf(("IpNameServiceImpl::GetQueryPackets(): Resetting current list"));
                    id = IncrementAndFetch(&INCREMENTAL_PACKET_ID);
                    mdnsHeader.SetId(id);
                    MDNSPacket additionalPacket;
                    additionalPacket->SetHeader(mdnsHeader);
                    additionalPacket->SetVersion(2, 2);
                    additionalPacket->AddAdditionalRecord(searchRecord);
                    additionalPacket->AddAdditionalRecord(refRecord);
                    additionalPacket->GetAdditionalRecord("search.*", MDNSResourceRecord::TXT, &searchRecord1);
                    searchRData = static_cast<MDNSSearchRData*>(searchRecord1->GetRData());
                    additionalPacket->GetAdditionalRecord("sender-info.*", MDNSResourceRecord::TXT, &refRecord1);
                    refRData = static_cast<MDNSSenderRData*>(refRecord1->GetRData());
                    searchRData->Reset();
                    if (!m_v2_queries[TRANSPORT_INDEX_UDP].empty()) {
                        additionalPacket->AddQuestion(mdnsUDPQuestion);
                    }
                    if (!m_v2_queries[TRANSPORT_INDEX_TCP].empty()) {
                        additionalPacket->AddQuestion(mdnsTCPQuestion);
                    }
                    for (MatchMap::iterator j = matching.begin(); j != matching.end(); ++j) {
                        searchRData->SetValue(j->first, j->second);
                    }
                    count = 1;
                    refRData->SetSearchID(id);
                    additionalPacket->ClearDestination();
                    if (interfaceIndex != -1) {
                        additionalPacket->SetInterfaceIndex(interfaceIndex);
                    }
                    if (family != qcc::QCC_AF_UNSPEC) {
                        additionalPacket->SetAddressFamily(family);
                    }
                    packets.push_back(Packet::cast(additionalPacket));
                    addedQueries.insert(*i);
                } else {
                    if (count > 0) {
                        searchRData->SetValue(";");
                    }
                    for (MatchMap::iterator j = matching.begin(); j != matching.end(); ++j) {
                        searchRData->SetValue(j->first, j->second);
                    }
                    if (transportIndex == TRANSPORT_INDEX_UDP || transportIndex == TRANSPORT_INDEX_TCP) {
                        addedQueries.insert(*i);
                    }
                    count++;
                }
            }
        }
    }
    m_mutex.Unlock();
}

void IpNameServiceImpl::Retransmit(uint32_t transportIndex, bool exiting, bool quietly, const qcc::IPEndpoint& destination, uint8_t type, TransportMask completeTransportMask, vector<qcc::String>& wkns, const int32_t interfaceIndex, const qcc::AddressFamily family, const qcc::IPAddress& localAddress)
{
    //
    // Type can be one of the following 3 values:
    // - TRANSMIT_V0_V1: transmit version zero and version one messages.
    // - TRANSMIT_V2: transmit version two messages.
    // - TRANSMIT_V0_V1 | TRANSMIT_V2: transmit version zero, version one and
    //                                 version two messages.
    //
    // If V1 is not enabled we only respond to queries for quiet names from V1
    // to support legacy thin core leaf nodes looking for router nodes.
    //
    if (!m_enableV1 && !quietly) {
        type &= ~TRANSMIT_V0_V1;
    }

    if (type == 0) {
        //Nothing to transmit
        return;
    }
    QCC_DbgPrintf(("IpNameServiceImpl::Retransmit()"));

    //
    // There are at least two threads wandering through the advertised list.
    // We are running short on toes, so don't shoot any more off by not being
    // thread-unaware.
    //
    m_mutex.Lock();

    //
    // We've been asked to retransmit our advertised names.  There are two main
    // classes of names: those actively advertised and those quietly advertised.
    // The difference is that quietly advertised names only go out when a
    // who-has message is received.  They are not sent periodically.  The
    // reception of a who-has message is indicated by the <quietly> parameter
    // being set to true.  Since we want to allow passive observers to hear our
    // responses, if we get a who-has message, no matter what is being looked
    // or, we take the opportunity to retransmit all of our names whether or not
    // they are quitely or actively advertised.  Since quiet responses are a
    // "new thing," we don't worry about sending down-version packets.  This all
    // means quiet advertisement responses are quite simple as compared to
    // active advertisement responses.  They are a special case though.
    //
    // So, based on these observations, we retransmit our whole list if
    // <quietly> is true and the advertised quietly list is not empty or if the
    // advertised list is not empty -- otherwise we don't have anything to do.
    //
    bool doRetransmit = (quietly && !m_advertised_quietly[transportIndex].empty()) || !m_advertised[transportIndex].empty();
    if (doRetransmit == false) {
        QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Nothing to do for transportIndex %d", transportIndex));
        m_mutex.Unlock();
        return;
    }

    //
    // We are now at version one of the protocol.  There is a significant
    // difference between version zero and version one messages, so down-version
    // (version zero) clients will not know what to do with versino one
    // messages.  This means that if we want to have clients running older
    // daemons be able to hear our advertisements, we need to send both flavors
    // of message.  Since the version is located in the message header, this
    // means two messages.
    //
    // Put together and send response packets for version zero, but only if the
    // transport index corresponds to TRANSPORT_TCP since that was the only
    // possibility in version zero and keeping in mind that we aren't going to
    // send version zero messages over our newly defined "quiet" mechanism.
    //
    if (transportIndex == TRANSPORT_INDEX_TCP && quietly == false && (type & TRANSMIT_V0)) {
        //
        // Keep track of how many messages we actually send in order to get all of
        // the advertisements out.
        //
        uint32_t nSent = 0;

        //
        // The header will tie the whole protocol message together.  By setting the
        // timer, we are asking for everyone who hears the message to remember the
        // advertisements for that number of seconds.  If we are exiting, then we
        // set the timer to zero, which means that the name is no longer valid.
        //
        NSPacket nspacket;

        //
        // We understand all messages from version zero to version one, and we
        // are sending a version zero message.  The whole point of sending a
        // version zero message is that can be understood by down-level code
        // so we can't use the new versioning scheme.
        //
        nspacket->SetVersion(0, 0);

        nspacket->SetTimer(exiting ? 0 : m_tDuration);

        IsAt isAt;
        isAt.SetVersion(0, 0);

        //
        // We don't actually send the transport mask in version zero packets
        // but we make a note to ourselves to let us know on behalf of what
        // transport we will be sending.
        //
        isAt.SetTransportMask(MaskFromIndex(transportIndex));

        //
        // The Complete Flag tells the other side that the message it recieves
        // contains the complete list of well-known names advertised by the
        // source.  We don't know that we fit them all in yet, so this must be
        // initialized to false.
        //
        isAt.SetCompleteFlag(false);

        //
        // We have to use some sneaky way to tell an in-the know version one
        // client that the packet is from a version one client and that is
        // through the setting of the UDP flag.  TCP transports are the only
        // possibility for version zero packets and it always sets the TCP
        // flag, of course.
        //
        isAt.SetTcpFlag(true);
        isAt.SetUdpFlag(true);

        isAt.SetGuid(m_guid);

        //
        // The only possibility in version zero is that the port is the IPv4
        // reliable port.
        //
        isAt.SetPort(0);

        QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Loop through advertised names"));

        //
        // Loop through the list of names we are advertising, constructing as many
        // protocol messages as it takes to get our list of advertisements out.
        //
        // Note that the number of packets that can go out in any given amount of
        // time is effectively throttled in SendProtocolMessage() by a random delay.
        // A user can consume all available resources here by flooding us with
        // advertisements but she will only be shooting herself in the foot.
        //
        for (set<qcc::String>::iterator i = m_advertised[transportIndex].begin(); i != m_advertised[transportIndex].end(); ++i) {
            QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Accumulating \"%s\"", (*i).c_str()));

            //
            // It is possible that we have accumulated more advertisements than will
            // fit in a UDP IpNameServiceImpl packet.  A name service is-at message is going
            // to consist of a header and its answer section, which is made from an
            // IsAt object.  We first ask both of these objects to return their size
            // so we know how much space is committed already.  Note that we ask the
            // header for its max possible size since the header may be modified to
            // add actual IPv4 and IPv6 addresses when it is sent.
            //
            size_t currentSize = nspacket->GetSerializedSize() + isAt.GetSerializedSize();

            //
            // This isn't terribly elegant, but we don't know the IP address(es)
            // over which the message will be sent.  These are added in the loop
            // that actually does the packet sends, with the interface addresses
            // dynamically added onto the message.  We have no clue here if an IPv4
            // or IPv6 or both flavors of address will exist on a given interface,
            // nor how many interfaces there are.  All we can do here is to assume
            // the worst case for the size (both exist) and add the 20 bytes (four
            // for IPv4, sixteen for IPv6) that the addresses may consume in the
            // final packet.
            //
            currentSize += 20;

            //
            // We cheat a little in order to avoid a string copy and use our
            // knowledge that names are stored as a byte count followed by the
            // string bytes.  If the current name won't fit into the currently
            // assembled message, we need to flush the current message and start
            // again.
            //
            if (currentSize + 1 + (*i).size() > NS_MESSAGE_MAX) {
                QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Message is full"));
                //
                // The current message cannot hold another name.  We need to send it
                // out before continuing.
                //
                QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Sending partial list"));
                nspacket->AddAnswer(isAt);

                if (quietly) {
                    nspacket->SetDestination(destination);
                    SendOutboundMessageQuietly(Packet::cast(nspacket));
                } else {
                    nspacket->ClearDestination();
                    if (interfaceIndex != -1) {
                        nspacket->SetInterfaceIndex(interfaceIndex);
                    } else {
                        nspacket->ClearInterfaceIndex();
                    }
                    if (family != qcc::QCC_AF_UNSPEC) {
                        nspacket->SetAddressFamily(family);
                    } else {
                        nspacket->ClearAddressFamily();
                    }
                    if (localAddress != IPAddress("0.0.0.0")) {
                        SendOutboundMessageActively(Packet::cast(nspacket), localAddress);
                    } else {
                        SendOutboundMessageActively(Packet::cast(nspacket));
                    }
                }


                ++nSent;

                //
                // The full message is now on the way out.  Now, we remove all of
                // the entries in the IsAt object, reset the header, which clears
                // out the existing is-at, and start accumulating new names again.
                //
                QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Resetting current list"));
                nspacket->Reset();
                isAt.Reset();
                isAt.AddName(*i);
            } else {
                QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Message has room.  Adding \"%s\"", (*i).c_str()));
                isAt.AddName(*i);
            }
        }

        //
        // We most likely have a partially full message waiting to go out.  If we
        // haven't sent a message, then the one message holds all of the names that
        // are being advertised.  In this case, we set the complete flag to indicate
        // that this packet describes the full extent of advertised well known
        // names.
        //
        if (nSent == 0) {
            QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Single complete message "));
            isAt.SetCompleteFlag(true);
        }

        QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Sending final version zero message "));
        nspacket->AddAnswer(isAt);

        nspacket->ClearDestination();
        if (interfaceIndex != -1) {
            nspacket->SetInterfaceIndex(interfaceIndex);
        } else {
            nspacket->ClearInterfaceIndex();
        }
        if (family != qcc::QCC_AF_UNSPEC) {
            nspacket->SetAddressFamily(family);
        } else {
            nspacket->ClearAddressFamily();
        }
        if (localAddress != IPAddress("0.0.0.0")) {
            SendOutboundMessageActively(Packet::cast(nspacket), localAddress);
        } else {
            SendOutboundMessageActively(Packet::cast(nspacket));
        }
    }

    //
    // Put together and send response packets for version one.
    //

    if ((transportIndex == TRANSPORT_INDEX_TCP) && type & TRANSMIT_V1) {
        //
        // Keep track of how many messages we actually send in order to get all of
        // the advertisements out.
        //
        uint32_t nSent = 0;

        //
        // The header will tie the whole protocol message together.  By setting the
        // timer, we are asking for everyone who hears the message to remember the
        // advertisements for that number of seconds.  If we are exiting, then we
        // set the timer to zero, which means that the name is no longer valid.
        //
        NSPacket nspacket;

        //
        // We understand all messages from version zero to version one, and we
        // are sending a version one message;
        //
        nspacket->SetVersion(1, 1);

        nspacket->SetTimer(exiting ? 0 : m_tDuration);

        //
        // The underlying protocol is capable of identifying both TCP and UDP
        // services.  Right now, the only possibility is TCP.
        //
        IsAt isAt;

        //
        // We understand all messages from version zero to version one, and we
        // are sending a version one message;
        //
        isAt.SetVersion(1, 1);

        //
        // We don't know if this is going to be a complete and final list yet,
        // but we do know which transport we are doing this on behalf of.
        //
        isAt.SetCompleteFlag(false);
        isAt.SetTransportMask(MaskFromIndex(transportIndex));

        //
        // Version one allows us to provide four possible endpoints.  The address
        // will be rewritten on the way out with the address of the appropriate
        // interface.
        //
        if (!m_reliableIPv4PortMap[transportIndex].empty()) {
            isAt.SetReliableIPv4("", 0);
        }
        if (!m_unreliableIPv4PortMap[transportIndex].empty()) {
            isAt.SetUnreliableIPv4("", 0);
        }
        // This is a trick to make V2 NS ignore V1 packets. We set the IPv6 reliable bit,
        // that tells version two capable NS that a version two message will follow, and
        // to ignore the version one messages.

        isAt.SetReliableIPv6("", m_reliableIPv6Port[transportIndex]);

        if (m_unreliableIPv6Port[transportIndex]) {
            isAt.SetUnreliableIPv6("", m_unreliableIPv6Port[transportIndex]);
        }

        isAt.SetGuid(m_guid);

        QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Loop through advertised names"));

        //
        // Loop through the list of names we are advertising, constructing as many
        // protocol messages as it takes to get our list of advertisements out.
        //
        // Note that the number of packets that can go out in any given amount of
        // time is effectively throttled in SendProtocolMessage() by a random delay.
        // A user can consume all available resources here by flooding us with
        // advertisements but she will only be shooting herself in the foot.
        //
        for (set<qcc::String>::iterator i = m_advertised[transportIndex].begin(); i != m_advertised[transportIndex].end(); ++i) {

            //Do not send non-matching names if replying quietly
            if (quietly) {
                bool ignore = true;
                for (vector<String>::iterator itWkn = wkns.begin(); itWkn != wkns.end(); itWkn++) {
                    //Do not send non-matching names if replying quietly
                    if (!(WildcardMatch((*i), (*itWkn)))) {
                        ignore = false;
                        break;
                    }
                }
                if (ignore) {
                    continue;
                }
            }
            QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Accumulating \"%s\"", (*i).c_str()));

            //
            // It is possible that we have accumulated more advertisements than will
            // fit in a UDP IpNameServiceImpl packet.  A name service is-at message is going
            // to consist of a header and its answer section, which is made from an
            // IsAt object.  We first ask both of these objects to return their size
            // so we know how much space is committed already.  Note that we ask the
            // header for its max possible size since the header may be modified to
            // add actual IPv4 and IPv6 addresses when it is sent.
            //
            size_t currentSize = nspacket->GetSerializedSize() + isAt.GetSerializedSize();

            //
            // This isn't terribly elegant, but we don't know the IP address(es)
            // over which the message will be sent.  These are added in the loop
            // that actually does the packet sends, with the interface addresses
            // dynamically added onto the message.  We have no clue here if an IPv4
            // or IPv6 or both flavors of address will exist on a given interface,
            // nor how many interfaces there are.  All we can do here is to assume
            // the worst case for the size (both exist) and add the 20 bytes (four
            // for IPv4, sixteen for IPv6) that the addresses may consume in the
            // final packet.
            //
            currentSize += 20;

            //
            // We cheat a little in order to avoid a string copy and use our
            // knowledge that names are stored as a byte count followed by the
            // string bytes.  If the current name won't fit into the currently
            // assembled message, we need to flush the current message and start
            // again.
            //
            if (currentSize + 1 + (*i).size() > NS_MESSAGE_MAX) {
                QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Message is full"));
                //
                // The current message cannot hold another name.  We need to send it
                // out before continuing.
                //
                QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Sending partial list"));
                nspacket->AddAnswer(isAt);

                if (quietly) {
                    nspacket->SetDestination(destination);
                    SendOutboundMessageQuietly(Packet::cast(nspacket));
                } else {
                    nspacket->ClearDestination();
                    if (interfaceIndex != -1) {
                        nspacket->SetInterfaceIndex(interfaceIndex);
                    } else {
                        nspacket->ClearInterfaceIndex();
                    }
                    if (family != qcc::QCC_AF_UNSPEC) {
                        nspacket->SetAddressFamily(family);
                    } else {
                        nspacket->ClearAddressFamily();
                    }
                    if (localAddress != IPAddress("0.0.0.0")) {
                        SendOutboundMessageActively(Packet::cast(nspacket), localAddress);
                    } else {
                        SendOutboundMessageActively(Packet::cast(nspacket));
                    }
                }


                ++nSent;

                //
                // The full message is now on the way out.  Now, we remove all of
                // the entries in the IsAt object, reset the header, which clears
                // out the existing is-at, and start accumulating new names again.
                //
                QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Resetting current list"));
                nspacket->Reset();
                isAt.Reset();
                isAt.AddName(*i);
            } else {
                QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Message has room.  Adding \"%s\"", (*i).c_str()));
                isAt.AddName(*i);
            }
        }

        if (quietly) {
            for (set<qcc::String>::iterator i = m_advertised_quietly[transportIndex].begin(); i != m_advertised_quietly[transportIndex].end(); ++i) {
                if (quietly) {
                    bool ignore = true;
                    for (vector<String>::iterator itWkn = wkns.begin(); itWkn != wkns.end(); itWkn++) {
                        //Do not send non-matching names if replying quietly
                        if (!(WildcardMatch((*i), (*itWkn)))) {
                            ignore = false;
                            break;
                        }
                    }
                    if (ignore) {
                        continue;
                    }
                }
                QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Accumulating (quiet) \"%s\"", (*i).c_str()));

                size_t currentSize = nspacket->GetSerializedSize() + isAt.GetSerializedSize();
                currentSize += 20;

                if (currentSize + 1 + (*i).size() > NS_MESSAGE_MAX) {
                    QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Message is full"));
                    QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Sending partial list"));
                    nspacket->AddAnswer(isAt);

                    if (quietly) {
                        nspacket->SetDestination(destination);
                        SendOutboundMessageQuietly(Packet::cast(nspacket));
                    } else {
                        nspacket->ClearDestination();
                        SendOutboundMessageActively(Packet::cast(nspacket));
                    }


                    ++nSent;

                    QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Resetting current list"));
                    nspacket->Reset();
                    isAt.Reset();
                    isAt.AddName(*i);
                } else {
                    QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Message has room.  Adding (quiet) \"%s\"", (*i).c_str()));
                    isAt.AddName(*i);
                }
            }
        }

        //
        // We most likely have a partially full message waiting to go out.  If we
        // haven't sent a message, then the one message holds all of the names that
        // are being advertised.  In this case, we set the complete flag to indicate
        // that this packet describes the full extent of advertised well known
        // names.
        //
        if (nSent == 0) {
            QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Single complete message "));
            isAt.SetCompleteFlag(true);
        }

        QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Sending final message "));
        nspacket->AddAnswer(isAt);

        if (quietly) {
            nspacket->SetDestination(destination);
            SendOutboundMessageQuietly(Packet::cast(nspacket));
        } else {
            nspacket->ClearDestination();
            if (interfaceIndex != -1) {
                nspacket->SetInterfaceIndex(interfaceIndex);
            } else {
                nspacket->ClearInterfaceIndex();
            }
            if (family != qcc::QCC_AF_UNSPEC) {
                nspacket->SetAddressFamily(family);
            } else {
                nspacket->ClearAddressFamily();
            }
            if (localAddress != IPAddress("0.0.0.0")) {
                SendOutboundMessageActively(Packet::cast(nspacket), localAddress);
            } else {
                SendOutboundMessageActively(Packet::cast(nspacket));
            }
        }

    }

    if (type & TRANSMIT_V2) {
        //
        // Keep track of how many messages we actually send in order to get all of
        // the advertisements out.
        //
        uint32_t nSent = 0;
        //version two
        int32_t id = IncrementAndFetch(&INCREMENTAL_PACKET_ID);

        MDNSHeader mdnsHeader(id, MDNSHeader::MDNS_RESPONSE);

        MDNSAdvertiseRData* advRData = new MDNSAdvertiseRData();
        MDNSResourceRecord advertiseRecord("advertise." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, exiting ? 0 : m_tDuration, advRData);
        delete advRData;

        MDNSSenderRData* refRData =  new MDNSSenderRData();
        refRData->SetSearchID(id);
        MDNSResourceRecord refRecord("sender-info." + m_guid + ".local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, exiting ? 0 : m_tDuration, refRData);
        delete refRData;

        MDNSARData* addrRData = new MDNSARData();
        MDNSResourceRecord aRecord(m_guid + ".local.", MDNSResourceRecord::A, MDNSResourceRecord::INTERNET, exiting ? 0 : m_tDuration, addrRData);
        delete addrRData;

        MDNSPacket mdnsPacket;
        mdnsPacket->SetHeader(mdnsHeader);

        if ((completeTransportMask & TRANSPORT_TCP) && (!m_reliableIPv4PortMap[TRANSPORT_INDEX_TCP].empty() || m_reliableIPv6Port[TRANSPORT_INDEX_TCP])) {

            MDNSPtrRData* ptrRDataTcp = new MDNSPtrRData();
            ptrRDataTcp->SetPtrDName(m_guid + "._alljoyn._tcp.local.");
            MDNSResourceRecord ptrRecordTcp("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, MDNSResourceRecord::INTERNET, exiting ? 0 : m_tDuration, ptrRDataTcp);
            delete ptrRDataTcp;

            MDNSSrvRData* srvRDataTcp = new MDNSSrvRData(1 /*priority */, 1 /* weight */,
                                                         0 /* port */, m_guid + ".local." /* target */);
            MDNSResourceRecord srvRecordTcp(m_guid + "._alljoyn._tcp.local.", MDNSResourceRecord::SRV, MDNSResourceRecord::INTERNET, exiting ? 0 : m_tDuration, srvRDataTcp);
            delete srvRDataTcp;

            MDNSTextRData* txtRDataTcp = new MDNSTextRData();
            if (m_reliableIPv6Port[TRANSPORT_INDEX_TCP]) {
                txtRDataTcp->SetValue("r6port", U32ToString(m_reliableIPv6Port[TRANSPORT_INDEX_TCP]));
            }

            MDNSResourceRecord txtRecordTcp(m_guid + "._alljoyn._tcp.local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, exiting ? 0 : m_tDuration, txtRDataTcp);
            delete txtRDataTcp;

            mdnsPacket->AddAnswer(ptrRecordTcp);
            mdnsPacket->AddAnswer(srvRecordTcp);
            mdnsPacket->AddAnswer(txtRecordTcp);
        }

        if ((completeTransportMask & TRANSPORT_UDP) && (!m_unreliableIPv4PortMap[TRANSPORT_INDEX_UDP].empty() || m_unreliableIPv6Port[TRANSPORT_INDEX_UDP])) {
            MDNSPtrRData* ptrRDataUdp = new MDNSPtrRData();
            ptrRDataUdp->SetPtrDName(m_guid + "._alljoyn._udp.local.");
            MDNSResourceRecord ptrRecordUdp("_alljoyn._udp.local.", MDNSResourceRecord::PTR, MDNSResourceRecord::INTERNET, exiting ? 0 : m_tDuration, ptrRDataUdp);
            delete ptrRDataUdp;

            MDNSSrvRData* srvRDataUdp = new MDNSSrvRData(1 /*priority */, 1 /* weight */,
                                                         0 /* port */, m_guid + ".local." /* target */);
            MDNSResourceRecord srvRecordUdp(m_guid + "._alljoyn._udp.local.", MDNSResourceRecord::SRV, MDNSResourceRecord::INTERNET, exiting ? 0 : m_tDuration, srvRDataUdp);
            delete srvRDataUdp;

            MDNSTextRData* txtRDataUdp = new MDNSTextRData();
            if (m_unreliableIPv6Port[TRANSPORT_INDEX_UDP]) {
                txtRDataUdp->SetValue("u6port", U32ToString(m_unreliableIPv6Port[TRANSPORT_INDEX_UDP]));
            }

            MDNSResourceRecord txtRecordUdp(m_guid + "._alljoyn._udp.local.", MDNSResourceRecord::TXT, MDNSResourceRecord::INTERNET, exiting ? 0 : m_tDuration, txtRDataUdp);
            delete txtRDataUdp;

            mdnsPacket->AddAnswer(ptrRecordUdp);
            mdnsPacket->AddAnswer(srvRecordUdp);
            mdnsPacket->AddAnswer(txtRecordUdp);
        }
        mdnsPacket->AddAdditionalRecord(advertiseRecord);
        mdnsPacket->AddAdditionalRecord(refRecord);
        mdnsPacket->AddAdditionalRecord(aRecord);
        mdnsPacket->SetVersion(2, 2);
        MDNSResourceRecord* advRecord;
        mdnsPacket->GetAdditionalRecord("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &advRecord);

        advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());

        MDNSResourceRecord* refRecord1;
        mdnsPacket->GetAdditionalRecord("sender-info.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &refRecord1);

        refRData = static_cast<MDNSSenderRData*>(refRecord1->GetRData());

        TransportMask transportMaskArr[3] = { TRANSPORT_TCP, TRANSPORT_UDP, TRANSPORT_TCP | TRANSPORT_UDP };

        for (int i = 0; i < 3; i++) {
            TransportMask tm = transportMaskArr[i];
            set<String> advertising = GetAdvertising(tm);
            set<String> advertising_quietly = GetAdvertisingQuietly(tm);
            //Insert the transport mask if there are any active or quiet advertisements we are sending out.
            if (!advertising.empty() || (quietly && !advertising_quietly.empty())) {
                advRData->SetTransport(tm);
            }
            for (set<qcc::String>::iterator it = advertising.begin(); it != advertising.end(); ++it) {

                //Do not send non-matching names if requestor has set send_matching_only i.e. wkns.size() > 0
                if (wkns.size() > 0) {
                    bool ignore = true;
                    for (vector<String>::iterator itWkn = wkns.begin(); itWkn != wkns.end(); itWkn++) {
                        //Do not send non-matching names if requestor has set send_matching_only i.e. wkns.size() > 0
                        if (!(WildcardMatch((*it), (*itWkn)))) {
                            ignore = false;
                            break;
                        }
                    }
                    if (ignore) {
                        continue;
                    }
                }

                QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Accumulating \"%s\"", (*it).c_str()));

                //
                // It is possible that we have accumulated more advertisements than will
                // fit in a UDP IpNameServiceImpl packet.  A name service is-at message is going
                // to consist of a header and its answer section, which is made from an
                // IsAt object.  We first ask both of these objects to return their size
                // so we know how much space is committed already.  Note that we ask the
                // header for its max possible size since the header may be modified to
                // add actual IPv4 and IPv6 addresses when it is sent.
                //
                size_t currentSize = mdnsPacket->GetSerializedSize();

                //
                // This isn't terribly elegant, but we don't know the IP address(es)
                // over which the message will be sent.  These are added in the loop
                // that actually does the packet sends, with the interface addresses
                // dynamically added onto the message.  We have no clue here if an IPv4
                // or IPv6 or both flavors of address will exist on a given interface,
                // nor how many interfaces there are.  All we can do here is to assume
                // the worst case for the size (both exist) and add the 20 bytes (four
                // for IPv4, sixteen for IPv6) that the addresses may consume in the
                // final packet.
                //
                currentSize += 100;
                //
                // We cheat a little in order to avoid a string copy and use our
                // knowledge that names are stored as a byte count followed by the
                // string bytes.  If the current name won't fit into the currently
                // assembled message, we need to flush the current message and start
                // again.
                //
                if (currentSize + 1 + (*it).size() > NS_MESSAGE_MAX) {
                    QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Message is full"));
                    //
                    // The current message cannot hold another name.  We need to send it
                    // out before continuing.
                    //
                    QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Sending partial list"));

                    if (quietly) {
                        mdnsPacket->SetDestination(destination);
                        SendOutboundMessageQuietly(Packet::cast(mdnsPacket));
                    } else {
                        mdnsPacket->ClearDestination();
                        SendOutboundMessageActively(Packet::cast(mdnsPacket));
                    }


                    ++nSent;

                    //
                    // The full message is now on the way out.  Now, we remove all of
                    // the entries in the IsAt object, reset the header, which clears
                    // out the existing is-at, and start accumulating new names again.
                    //
                    QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Resetting current list"));
                    advRData->Reset();
                    advRData->SetTransport(tm);
                    advRData->SetValue("name", *it);
                    id = IncrementAndFetch(&INCREMENTAL_PACKET_ID);
                    refRData->SetSearchID(id);

                } else {
                    QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Message has room.  Adding \"%s\"", (*it).c_str()));
                    advRData->SetValue("name", *it);
                }
            }



            if (quietly) {

                for (set<qcc::String>::iterator it = advertising_quietly.begin(); it != advertising_quietly.end(); ++it) {
                    //Do not send non-matching names if requestor has set send_matching_only i.e. wkns.size() > 0
                    if (wkns.size() > 0) {
                        bool ignore = true;
                        for (vector<String>::iterator itWkn = wkns.begin(); itWkn != wkns.end(); itWkn++) {
                            //Do not send non-matching names if requestor has set send_matching_only i.e. wkns.size() > 0
                            if (!(WildcardMatch((*it), (*itWkn)))) {
                                ignore = false;
                                break;
                            }
                        }
                        if (ignore) {
                            continue;
                        }
                    }
                    QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Accumulating (quiet) \"%s\"", (*it).c_str()));

                    size_t currentSize = mdnsPacket->GetSerializedSize();
                    currentSize += 100;

                    if (currentSize + 1 + (*it).size() > NS_MESSAGE_MAX) {
                        QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Message is full"));
                        QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Sending partial list"));

                        mdnsPacket->SetDestination(destination);
                        SendOutboundMessageQuietly(Packet::cast(mdnsPacket));


                        ++nSent;

                        QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Resetting current list"));
                        advRData->Reset();
                        advRData->SetTransport(tm);

                        advRData->SetValue("name", *it);
                        id = IncrementAndFetch(&INCREMENTAL_PACKET_ID);
                        refRData->SetSearchID(id);
                    } else {
                        QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Message has room.  Adding (quiet) \"%s\"", (*it).c_str()));
                        advRData->SetValue("name", *it);
                    }
                }
            }
        }
        //
        // We most likely have a partially full message waiting to go out.  If we
        // haven't sent a message, then the one message holds all of the names that
        // are being advertised.  In this case, we set the complete flag to indicate
        // that this packet describes the full extent of advertised well known
        // names.
        //
        if (nSent == 0) {
            QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Single complete message "));
        }

        QCC_DbgPrintf(("IpNameServiceImpl::Retransmit(): Sending final message "));

        if (quietly) {
            mdnsPacket->SetDestination(destination);
            SendOutboundMessageQuietly(Packet::cast(mdnsPacket));
        } else {
            mdnsPacket->ClearDestination();
            SendOutboundMessageActively(Packet::cast(mdnsPacket));
        }

    }
    m_mutex.Unlock();
}

// Note: this function assumes the mutex is locked
bool IpNameServiceImpl::IsPeriodicMaintenanceTimerNeeded(void) const
{
    //
    // The timer is needed when we're in the midst of handling a terminal message,
    // we have an outbound message queued, or we're counting down to send the
    // queued advertisement (in V1 config).
    //
    if (m_terminal || (m_outbound.size() > 0) || (m_enableV1 && (m_timer > 0))) {
        return true;
    } else {
        return false;
    }
}

void IpNameServiceImpl::DoPeriodicMaintenance(void)
{
#if HAPPY_WANDERER
    Wander();
#endif
    m_mutex.Lock();

    //
    // If we have something exported, we will have a retransmit timer value
    // set.  If not, this value will be zero and there's nothing to be done.
    //
    if (m_timer) {
        --m_timer;
        if (m_timer == m_tRetransmit) {
            QCC_DbgPrintf(("IpNameServiceImpl::DoPeriodicMaintenance(): Retransmit()"));
            for (uint32_t index = 0; index < N_TRANSPORTS; ++index) {
                vector<String> empty;
                Retransmit(index, false, false, qcc::IPEndpoint("0.0.0.0", 0), TRANSMIT_V0_V1, MaskFromIndex(index), empty);
            }
            m_timer = m_tDuration;
        }
    }

    m_mutex.Unlock();
}

void IpNameServiceImpl::HandleProtocolQuestion(WhoHas whoHas, const qcc::IPEndpoint& endpoint, int32_t interfaceIndex, const qcc::IPAddress& localAddress)
{
    QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolQuestion(%s)", endpoint.ToString().c_str()));

    //
    // There are at least two threads wandering through the advertised list.
    //
    m_mutex.Lock();

    //
    // We check the version of WhoHas packet
    // If it is version 0 that we got from a routing node capable of sending a
    // version 1 WhoHas then we drop this packet. This reduces the number of
    // IS-AT packets that we send over the wire
    //
    uint32_t nsVersion, msgVersion;
    whoHas.GetVersion(nsVersion, msgVersion);
    if (nsVersion == 0 && msgVersion == 0) {
        if (whoHas.GetUdpFlag()) {
            QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolQuestion(): Ignoring version zero message from version one peer"));
            m_mutex.Unlock();
            return;
        }
    }

    if (nsVersion == 1 && msgVersion == 1) {
        if (whoHas.GetUdpFlag()) {
            QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolQuestion(): Ignoring version one message from version two peer"));
            m_mutex.Unlock();
            return;
        }
    }
    vector<String> wkns;
    //
    // The who-has message doesn't specify which transport is doing the asking.
    // This is an oversight and should be fixed in a subsequent version.  The
    // only reasonable thing to do is to return name matches found in all of
    // the advertising transports.
    //
    for (uint32_t index = 0; index < N_TRANSPORTS; ++index) {

        //
        // If there are no names being advertised by the transport identified by
        // its index (actively or quietly), there is nothing to do.
        //
        if (m_advertised[index].empty() && m_advertised_quietly[index].empty()) {
            continue;
        }

        //
        // Loop through the names we are being asked about, and if we have
        // advertised any of them, we are going to need to respond to this
        // question.  Keep track of whether or not any of our corresponding
        // advertisements are quiet, since we want to respond quietly to a
        // question about a quiet advertisements.  That is, if any of the names
        // the client is asking about corresponds to a quiet advertisement we
        // respond directly to the client and do not multicast the response.
        // The only way we multicast a response is if the client does not ask
        // about any of our quietly advertised names.
        //
        // Becuse of this requirement, we loop through all of the names in the
        // who-has message to see if any of them correspond to quiet
        // advertisements.  We don't just break out and respond if we find any
        // old match since it may be the case that the last name is the quiet
        // one.
        //
        bool respond = false;
        bool respondQuietly = false;
        for (uint32_t i = 0; i < whoHas.GetNumberNames(); ++i) {

            qcc::String wkn = whoHas.GetName(i);
            wkns.push_back(wkn);
            //
            // Zero length strings are unmatchable.  If you want to do a wildcard
            // match, you've got to send a wildcard character.
            //
            if (wkn.size() == 0) {
                continue;
            }

            //
            // Check to see if this name on the list of names we actively
            // advertise.
            //
            // If V1 is not enabled we only respond to queries for quiet names
            // from V1 to support legacy thin core leaf nodes looking for router
            // nodes.
            //
            for (set<qcc::String>::iterator j = m_advertised[index].begin(); m_enableV1 && (j != m_advertised[index].end()); ++j) {

                //
                // The requested name comes in from the WhoHas message and we
                // allow wildcards there.
                //
                if (WildcardMatch((*j), wkn)) {
                    QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolQuestion(): request for %s does not match my %s",
                                   wkn.c_str(), (*j).c_str()));
                    continue;
                } else {
                    respond = true;
                    break;
                }
            }

            //
            // Check to see if this name on the list of names we quietly advertise.
            //
            for (set<qcc::String>::iterator j = m_advertised_quietly[index].begin(); j != m_advertised_quietly[index].end(); ++j) {

                //
                // The requested name comes in from the WhoHas message and we
                // allow wildcards there.
                //
                if (WildcardMatch((*j), wkn)) {
                    QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolQuestion(): request for %s does not match my %s",
                                   wkn.c_str(), (*j).c_str()));
                    continue;
                } else {
                    respond = true;
                    respondQuietly = true;
                    break;
                }
            }
        }

        //
        // Since any response we send must include all of the advertisements we
        // are exporting; this just means to retransmit all of our advertisements.
        //
        if (respond) {
            m_mutex.Unlock();
            qcc::AddressFamily family = qcc::QCC_AF_UNSPEC;
            if (endpoint.GetAddress().IsIPv4()) {
                family = QCC_AF_INET;
            }
            if (endpoint.GetAddress().IsIPv6()) {
                family = QCC_AF_INET6;
            }
            if (nsVersion == 0 && msgVersion == 0) {
                vector<String> empty;
                Retransmit(index, false, respondQuietly, endpoint, TRANSMIT_V0, MaskFromIndex(index), empty, interfaceIndex, family, localAddress);
            }
            if (nsVersion == 1 && msgVersion == 1) {
                Retransmit(index, false, respondQuietly, endpoint, TRANSMIT_V1, MaskFromIndex(index), wkns, interfaceIndex, family, localAddress);
            }
            m_mutex.Lock();
        }
    }

    m_mutex.Unlock();
}

void IpNameServiceImpl::HandleProtocolAnswer(IsAt isAt, uint32_t timer, const qcc::IPEndpoint& endpoint, int32_t interfaceIndex)
{
    QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(%s)", endpoint.ToString().c_str()));

    // Get IPv4 address of interface for this message (message may have been
    // received on the IPv6 address).  This will be used as a sanity check later
    // against the connect spec in the message.
    String ifName;
    int32_t ifIndexV4 = -1;
    if (interfaceIndex != -1) {
        for (uint32_t i = 0; i < m_liveInterfaces.size(); ++i) {
            if ((uint32_t)interfaceIndex == m_liveInterfaces[i].m_index) {
                ifName = m_liveInterfaces[i].m_interfaceName;
                if (m_liveInterfaces[i].m_address.IsIPv4()) {
                    ifIndexV4 = i;
                    break;
                }
            }
        }
    }

    //
    // We have to determine where the transport mask is going to come
    // from.  For version zero messages, we infer it as TRANSPORT_TCP
    // since that was the only possibility.  For version one and greater
    // messages the transport mask is included in the message.
    //
    TransportMask transportMask;
    uint32_t transportIndex;

    uint32_t nsVersion, msgVersion;
    isAt.GetVersion(nsVersion, msgVersion);
    if (msgVersion == 0) {
        transportMask = TRANSPORT_TCP;
        transportIndex = TRANSPORT_INDEX_TCP;
    } else {
        transportMask = isAt.GetTransportMask();

        if (CountOnes(transportMask) != 1) {
            QCC_LogError(ER_BAD_TRANSPORT_MASK, ("IpNameServiceImpl::HandleProtocolAnswer(): Bad transport mask"));
            return;
        }

        transportIndex = IndexFromBit(transportMask);
        assert(transportIndex < 16 && "IpNameServiceImpl::HandleProtocolAnswer(): Bad callback index");
        if (transportIndex >= 16) {
            return;
        }

    }

    //
    // We need protection since other threads can call in and change the
    // callback out from under us if we do not use protection.
    // We want to have a contract that says we won't ever send out a
    // callback after it is cleared.  Taking a lock
    // and holding it during the callback is a bit dangerous, so we grab the lock,
    // set m_protect_callback to true and then release the lock before making the
    // callback. We therefore do expect that callbacks won't do
    // something silly like call back and cancel callbacks or make some other
    // call back into this class from another direction.
    //
    m_mutex.Lock();

    //
    // If there is no callback for the provided transport, we can't tell the
    // user anything about what is going on the net, so it's pointless to go any
    // further.
    //

    if (m_callback[transportIndex] == NULL) {
        QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(): No callback for transport, so nothing to do"));

        m_mutex.Unlock();

        return;
    }

    //
    // For version zero messages from version one transports, we need to
    // disregard the name service messages sent out in compatibility mode
    // (version zero messages).  We know that a version one name service will be
    // following up with a version one packet, so a version zero compatibility
    // message provides incomplete information -- we drop such messages here.
    // The indication that this is the case is both versions being zero with a
    // UDP flag being true.
    //
    if (nsVersion == 0 && msgVersion == 0) {
        if (isAt.GetUdpFlag()) {
            QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(): Ignoring version zero message from version one/version two peer"));

            m_mutex.Unlock();

            return;
        }
    }

    //
    // For version one messages from version two transports, we need to
    // disregard the name service messages sent out in compatibility mode
    // (version one messages).  We know that a version two name service will be
    // following up with a version two packet, so a version one compatibility
    // message provides incomplete information -- we drop such messages here.
    // The indication that this is the case is both versions being one with a
    // IPv6 flag being true.
    //
    if (nsVersion == 1 && msgVersion == 1) {
        if (isAt.GetReliableIPv6Flag()) {
            QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(): Ignoring version one message from version two peer"));
            m_mutex.Unlock();

            return;
        }
    }

    vector<qcc::String> wkn;

    for (uint8_t i = 0; i < isAt.GetNumberNames(); ++i) {
        wkn.push_back(isAt.GetName(i));
    }

    //
    // Life is easier if we keep these things sorted.  Don't rely on the source
    // (even though it is really us) to do so.
    //
    sort(wkn.begin(), wkn.end());

    qcc::String guid = isAt.GetGuid();
    QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(): Got GUID %s", guid.c_str()));

    //
    // How we infer addresses is different between version zero of the protocol
    // and version one.  In version zero, if there are no IP addresses present
    // in the received message, we take the IP address found in the received
    // packet.  This allowed us to optimize out the address in some cases.  We
    // do not do this in version one messages.  The advertised addresses must
    // always be present in the message.
    //
    isAt.GetVersion(nsVersion, msgVersion);
    if (msgVersion == 0) {
        //
        // We always get an address from the system since we got the message
        // over a call to recvfrom().  This will either be an IPv4 or an IPv6
        // address in the case of legacy daemons or only from IPv6 in new
        // daemons.  We can also get an IPv4 or an IPv6 address in the protocol.
        // So we have from one to three addresses of possibly different flavors
        // that we need to communicate back to the daemon.  We have to be very
        // careful to play by the old rules when appropriate to make sure we
        // have backward compatibility.
        //
        // Note that there is no such thing as a TCP transport that is capable
        // of listening on an IPv6 address, so we filter those out here.
        //
        // It is convenient for the daemon to get these addresses in the form of
        // a "listen-spec".  This is a string starting with the transport name,
        // followed by private (to the transport) name=value pairs.  In version
        // zero of the protocol, there was only one possible transport that used
        // the IP name service, and that was the TCP transport.  We used to be
        // integrated into the TCP transport, so, for us here and now these
        // listen specs look like, "tcp:r4addr=x,r4port=y".  The daemon is going
        // to keep track of unique instances of these and must be able to handle
        // multiple identical reports since we will be getting keepalives.  What
        // we need to do then is to send a callback with a listen-spec for every
        // address we find.  If we get all three addresses, we'll do three
        // callbacks with different listen-specs.  This completely changes in
        // version one, BTW.
        //
        qcc::String ipv4address, ipv6address;

        QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(): Got IP %s from recvfrom", endpoint.addr.ToString().c_str()));

        if (isAt.GetIPv4Flag()) {
            ipv4address = isAt.GetIPv4();
            QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(): Got IPv4 %s from message", ipv4address.c_str()));
        }

        if (isAt.GetIPv6Flag()) {
            ipv6address = isAt.GetIPv6();
            QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(): Got IPv6 %s from message", ipv6address.c_str()));
        }

        uint16_t port = isAt.GetPort();
        QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(): Got port %d from message", port));

        //
        //
        // Since version zero had no transport mask, the only transport that can
        // provide a version zero message is tcp.  So, the longest bus address
        // we can generate is going to be the larger of an IPv4 or IPv6 address:
        //
        // "addr=255.255.255.255,port=65535"
        // "addr=ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff,port=65535"
        //
        // or 56 characters long including the trailing '\0'
        //
        char addrbuf[64];

        //
        // Call back with the address we got via recvfrom unless it is
        // overridden by the address in the message. An ipv4 address in the
        // message overrides an ipv4 recvfrom address, an ipv6 address in the
        // message overrides an ipv6 recvfrom address.
        //
        // Note that we no longer prepend the transport name ("tcp:") since we
        // got broken out of the TCP transport.  We expect the transport to do
        // that now.
        //
        if ((endpoint.addr.IsIPv4() && !ipv4address.size())) {
            ipv4address = endpoint.addr.ToString();
        }

        //
        // If we received an IPv4 address in the message, call back with that
        // one.
        //
        if (ipv4address.size()) {
            if (ifIndexV4 != -1 &&
                SameNetwork(m_liveInterfaces[ifIndexV4].m_prefixlen, m_liveInterfaces[ifIndexV4].m_address, ipv4address)) {
                snprintf(addrbuf, sizeof(addrbuf), "addr=%s,port=%d", ipv4address.c_str(), port);
                qcc::String busAddress(addrbuf);

                if (transportIndex == TRANSPORT_INDEX_TCP && m_callback[transportIndex]) {
                    m_protect_callback = true;
                    m_mutex.Unlock();
                    QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(): Calling back with %s", addrbuf));
                    (*m_callback[transportIndex])(busAddress, guid, wkn, timer);
                    m_mutex.Lock();
                    m_protect_callback = false;
                }
            } else {
                //
                // We expect that a v4 addr may be sent via a v6 link local address.  However
                // if a v4 addr is sent via a v4 address then someone is misbehaving, so log
                // a warning.
                //
                if (endpoint.addr.IsIPv4()) {
                    QCC_LogError(ER_WARNING, ("Ignoring advertisement from %s for %s received on %s",
                                              endpoint.addr.ToString().c_str(),
                                              ipv4address.c_str(),
                                              ifName.c_str()));
                }
            }
        }

        //
        // If we received an IPv6 address in the message, call back with that
        // one.
        //
        if (ipv6address.size()) {
            snprintf(addrbuf, sizeof(addrbuf), "r6addr=%s,r6port=%d", ipv6address.c_str(), port);
            qcc::String busAddress(addrbuf);

            if (transportIndex == TRANSPORT_INDEX_TCP && m_callback[transportIndex]) {
                m_protect_callback = true;
                m_mutex.Unlock();
                QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(): Calling back with %s", addrbuf));
                (*m_callback[transportIndex])(busAddress, guid, wkn, timer);
                m_mutex.Lock();
                m_protect_callback = false;

            }
        }
    } else if (msgVersion == 1) {
        //
        // In the version one protocol, the maximum size static buffer for the
        // longest bus address we can generate corresponds to two fully occupied
        // IPv4 addresses and two fully occupied IPV6 addresses.  So, we figure
        // that we need 31 bytes for the IPv4 endpoint information,
        // 55 bytes for the IPv6 endpoint information and one extra
        // comma:
        //
        //     "addr=192.168.100.101,port=65535,"
        //     "addr=ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff,port=65535"
        //
        // Adding a byte for the trailing '\0' we come up with 88 bytes of bus
        // address. C++ purists will object to using the C stdio routines but
        // they are simpler and faster since there are no memory allocations or
        // reallocations.
        //
        // Note that we do not prepend the bus address with the transport name,
        // i.e. "tcp:" since we assume that the transport knows its own name.
        //
        char reliableAddrBuf[88];
        char unreliableAddrBuf[88];
        reliableAddrBuf[0] = '\0';
        unreliableAddrBuf[0] = '\0';

        char reliableAddr6Buf[60];
        char unreliableAddr6Buf[60];

        bool needComma = false;

        if (isAt.GetReliableIPv4Flag()) {
            snprintf(reliableAddrBuf, sizeof(reliableAddrBuf), "addr=%s,port=%d",
                     isAt.GetReliableIPv4Address().c_str(), isAt.GetReliableIPv4Port());

            needComma = true;
        }

        if (isAt.GetUnreliableIPv4Flag()) {
            snprintf(unreliableAddrBuf, sizeof(unreliableAddrBuf), ",addr=%s,port=%d",
                     isAt.GetUnreliableIPv4Address().c_str(), isAt.GetUnreliableIPv4Port());

            needComma = true;
        }

        if (isAt.GetReliableIPv6Flag()) {
            snprintf(reliableAddr6Buf, sizeof(reliableAddr6Buf), ",addr=%s,port=%d",
                     isAt.GetReliableIPv6Address().c_str(), isAt.GetReliableIPv6Port());
            if (needComma) {
                strncat(reliableAddrBuf, &reliableAddr6Buf[0], sizeof(reliableAddr6Buf));
            } else {
                strncat(reliableAddrBuf, &reliableAddr6Buf[1], sizeof(reliableAddr6Buf));
            }
        }

        if (isAt.GetUnreliableIPv6Flag()) {
            snprintf(unreliableAddr6Buf, sizeof(unreliableAddr6Buf), ",addr=%s,port=%d",
                     isAt.GetUnreliableIPv6Address().c_str(), isAt.GetUnreliableIPv6Port());
            if (needComma) {
                strncat(unreliableAddrBuf, &unreliableAddr6Buf[0], sizeof(unreliableAddr6Buf));
            } else {
                strncat(unreliableAddrBuf, &unreliableAddr6Buf[1], sizeof(unreliableAddr6Buf));
            }
        }

        if (!isAt.GetReliableIPv4Flag() || (ifIndexV4 != -1 && SameNetwork(m_liveInterfaces[ifIndexV4].m_prefixlen,
                                                                           m_liveInterfaces[ifIndexV4].m_address,
                                                                           isAt.GetReliableIPv4Address()))) {
            //
            // In version one of the protocol, we always call back with the
            // addresses we find in the message.  We don't bother with the address
            // we got in recvfrom.
            //
            qcc::String busAddress;
            if (transportIndex == TRANSPORT_INDEX_TCP) {
                busAddress = qcc::String(reliableAddrBuf);
            } else if (transportIndex == TRANSPORT_INDEX_UDP) {
                busAddress = qcc::String(unreliableAddrBuf);
            }

            if ((transportIndex == TRANSPORT_INDEX_TCP || transportIndex == TRANSPORT_INDEX_UDP) && m_callback[transportIndex]) {
                m_protect_callback = true;
                m_mutex.Unlock();
                QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolAnswer(): Calling back with %s", busAddress.c_str()));
                (*m_callback[transportIndex])(busAddress, guid, wkn, timer);
                m_mutex.Lock();
                m_protect_callback = false;
            }
        } else {
            //
            // We expect that a v4 addr may be sent via a v6 link local address.  However
            // if a v4 addr is sent via a v4 address then someone is misbehaving, so log
            // a warning.
            //
            if (isAt.GetReliableIPv4Flag() && endpoint.addr.IsIPv4()) {
                QCC_LogError(ER_WARNING, ("Ignoring advertisement from %s for %s received on %s",
                                          endpoint.addr.ToString().c_str(),
                                          isAt.GetReliableIPv4Address().c_str(),
                                          ifName.c_str()));
            }
        }
    }

    m_mutex.Unlock();
}


void IpNameServiceImpl::HandleProtocolMessage(uint8_t const* buffer, uint32_t nbytes, const qcc::IPEndpoint& endpoint, const uint16_t recvPort, int32_t interfaceIndex, const qcc::IPAddress& localAddress)
{
    QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolMessage(0x%x, %d, %s)", buffer, nbytes, endpoint.ToString().c_str()));

#if HAPPY_WANDERER
    if (Wander() == false) {
        QCC_LogError(ER_FAIL, ("IpNameServiceImpl::HandleProtocolMessage(): Wander(): out of range"));
        return;
    } else {
        QCC_LogError(ER_FAIL, ("IpNameServiceImpl::HandleProtocolMessage(): Wander(): in range"));
    }
#endif

    // Any messages received on port 9956 are version zero or version one messages.
    if (recvPort == 9956) {

        NSPacket nsPacket;
        size_t bytesRead = nsPacket->Deserialize(buffer, nbytes);
        if (bytesRead != nbytes) {
            QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolMessage(): Deserialize(): Error"));
            return;
        }

        //
        // We only understand version zero and one messages.
        //
        uint32_t nsVersion, msgVersion;
        nsPacket->GetVersion(nsVersion, msgVersion);

        if (msgVersion != 0 && msgVersion != 1) {
            QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolMessage(): Unknown version: Error"));
            return;
        }

        //
        // If the received packet contains questions, see if we can answer them.
        // We have the underlying device in loopback mode so we can get receive
        // our own questions.  We usually don't have an answer and so we don't
        // reply, but if we do have the requested names, we answer ourselves
        // to pass on this information to other interested bystanders.
        //
        for (uint8_t i = 0; i < nsPacket->GetNumberQuestions(); ++i) {
            HandleProtocolQuestion(nsPacket->GetQuestion(i), endpoint, interfaceIndex, localAddress);
        }

        //
        // Only questions are handled if V1 is not enabled since we are only
        // responding to queries for quiet names from V1 to support legacy thin
        // core leaf nodes looking for router nodes.
        //
        if (!m_enableV1) {
            return;
        }
        //
        // If the received packet contains answers, see if they are answers to
        // questions we think are interesting.  Make sure we are not talking to
        // ourselves unless we are told to for debugging purposes
        //
        for (uint8_t i = 0; i < nsPacket->GetNumberAnswers(); ++i) {
            IsAt isAt = nsPacket->GetAnswer(i);
            //
            // The version isn't actually carried in the is-at message since that
            // would be redundant, so we have to set it from the nsPacket version
            // before passing it off.
            //
            uint32_t nsVersion, msgVersion;
            nsPacket->GetVersion(nsVersion, msgVersion);
            isAt.SetVersion(nsVersion, msgVersion);
            if (m_loopback || (isAt.GetGuid() != m_guid)) {
                HandleProtocolAnswer(isAt, nsPacket->GetTimer(), endpoint, interfaceIndex);
            }
        }
    } else {
        // Messages not received on port 9956 are version two messages.
        MDNSPacket mdnsPacket;
        size_t bytesRead = mdnsPacket->Deserialize(buffer, nbytes);
        if (bytesRead != nbytes) {
            QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolMessage(): Deserialize(): Error."));
            return;
        }

        if (mdnsPacket->GetHeader().GetQRType() == MDNSHeader::MDNS_QUERY) {
            HandleProtocolQuery(mdnsPacket, endpoint, recvPort);
        } else {
            HandleProtocolResponse(mdnsPacket, endpoint, recvPort, interfaceIndex);
        }
    }
}

qcc::String IpNameServiceImpl::PeerInfo::ToString(const qcc::String& guid) const
{
    String s;
    s += "guid=" + guid + "/" + GUID128(guid).ToShortString();
    s += ",ip=" + unicastInfo.ToString();
    return s;
}

void IpNameServiceImpl::PrintPeerInfoMap()
{
    for (std::unordered_map<qcc::String, std::set<PeerInfo>, Hash, Equal>::iterator it = m_peerInfoMap.begin();
         it != m_peerInfoMap.end(); ++it) {
        for (std::set<PeerInfo>::iterator pit = it->second.begin(); pit != it->second.end(); ++pit) {
            QCC_DbgHLPrintf(("  %s", pit->ToString(it->first).c_str()));
        }
    }
}

bool IpNameServiceImpl::AddToPeerInfoMap(const qcc::String& guid, const qcc::IPEndpoint& ipEndpoint)
{
    if (ipEndpoint.GetPort() == 0 || ipEndpoint.GetAddress() == IPAddress()) {
        return false;
    }
    m_mutex.Lock();
    std::unordered_map<qcc::String, std::set<PeerInfo>, Hash, Equal>::iterator it = m_peerInfoMap.find(guid);
    if (it != m_peerInfoMap.end()) {
        bool foundEntry = false;
        for (std::set<PeerInfo>::iterator pit = it->second.begin(); !foundEntry && pit != it->second.end(); ++pit) {
            if (pit->unicastInfo == ipEndpoint) {
                foundEntry = true;
                Timespec now;
                GetTimeNow(&now);
                (*pit).lastResponseTimeStamp = now;
            }
        }
        if (!foundEntry) {
            PeerInfo peerInfo(ipEndpoint);
            it->second.insert(peerInfo);
            QCC_DbgHLPrintf(("Add to peer info map: %s", peerInfo.ToString(it->first).c_str()));
        }
    } else {
        PeerInfo peerInfo(ipEndpoint);
        std::set<PeerInfo> peerInfoList;
        peerInfoList.insert(peerInfo);
        m_peerInfoMap.insert(std::pair<qcc::String, std::set<PeerInfo> >(guid, peerInfoList));
        QCC_DbgHLPrintf(("Add to peer info map: %s", peerInfo.ToString(guid).c_str()));
    }
    m_mutex.Unlock();
    return true;
}

bool IpNameServiceImpl::RemoveFromPeerInfoMap(const qcc::String& guid)
{
    m_mutex.Lock();
    std::unordered_map<qcc::String, std::set<PeerInfo>, Hash, Equal>::iterator it = m_peerInfoMap.find(guid);
    if (it != m_peerInfoMap.end()) {
        for (std::set<PeerInfo>::iterator pit = it->second.begin(); pit != it->second.end(); ++pit) {
            QCC_DbgHLPrintf(("Remove from peer info map: %s", pit->ToString(guid).c_str()));
        }
        QCC_DbgHLPrintf(("Erase from peer info map: guid=%s", guid.c_str()));
        m_peerInfoMap.erase(guid);
        unordered_map<pair<String, IPEndpoint>, uint16_t, HashPacketTracker, EqualPacketTracker>::iterator it1 = m_mdnsPacketTracker.begin();
        while (it1 != m_mdnsPacketTracker.end()) {
            if (it1->first.first == guid) {
                m_mdnsPacketTracker.erase(it1++);
            } else {
                it1++;
            }
        }
        m_mutex.Unlock();
        return true;
    }
    m_mutex.Unlock();
    return false;
}

bool IpNameServiceImpl::UpdateMDNSPacketTracker(qcc::String guid, IPEndpoint endpoint, uint16_t burstId)
{
    //QCC_DbgPrintf(("IpNameServiceImpl::UpdateMDNSPacketTracker(%s, %s,%d)", guid.c_str(), endpoint.ToString().c_str(), burstId));

    //
    // We check for the entry in MDNSPacketTracker
    // If we find it we return false since that implies that we have seen a packet from this burst
    // If we do not find it we return true that implies that we have not seen a packet from this burst.
    //     We add/update the guid with this burst id
    //
    pair<String, IPEndpoint> key(guid, endpoint);
    unordered_map<pair<String, IPEndpoint>, uint16_t, HashPacketTracker, EqualPacketTracker>::iterator it = m_mdnsPacketTracker.find(key);
    // Check if the GUID is present in the Map
    // If Yes check if the incoming burst id is same or lower
    if (it != m_mdnsPacketTracker.end()) {
        // Drop the packet if burst id is lower or same
        if (it->second >= burstId) {
            return false;
        }
        // Update the last seen burst id from this guid
        else {
            it->second = burstId;
            return true;
        }
    }
    // GUID is not present in the Map so we add the entry
    else {
        m_mdnsPacketTracker[key] = burstId;
        return true;
    }
    return true;
}


void IpNameServiceImpl::HandleProtocolResponse(MDNSPacket mdnsPacket, IPEndpoint endpoint, uint16_t recvPort, int32_t interfaceIndex)
{
    // Get IPv4 address of interface for this message (message may have been
    // received on the IPv6 address).  This will be used as a sanity check later
    // against the connect spec in the message.
    String ifName;
    int32_t ifIndexV4 = -1;
    if (interfaceIndex != -1) {
        for (uint32_t i = 0; i < m_liveInterfaces.size(); ++i) {
            if ((uint32_t)interfaceIndex == m_liveInterfaces[i].m_index) {
                ifName = m_liveInterfaces[i].m_interfaceName;
                if (m_liveInterfaces[i].m_address.IsIPv4()) {
                    ifIndexV4 = i;
                    break;
                }
            }
        }
    }

    // Check if someone is providing info. about an alljoyn service.
    MDNSResourceRecord* answerTcp;
    MDNSResourceRecord* answerUdp;
    TransportMask transportMask = TRANSPORT_NONE;
    bool isAllJoynResponse = false;

    if (mdnsPacket->GetAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, &answerTcp)) {
        transportMask |= TRANSPORT_TCP;
        isAllJoynResponse = true;
    }
    if (mdnsPacket->GetAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR, &answerUdp)) {
        transportMask |= TRANSPORT_UDP;
        isAllJoynResponse = true;
    }

    if (!isAllJoynResponse) {
        QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolResponse Ignoring Non-AllJoyn related response"));
        return;
    }
    MDNSResourceRecord* refRecord;
    if (!mdnsPacket->GetAdditionalRecord("sender-info.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &refRecord)) {
        QCC_DbgPrintf(("Ignoring response without sender-info"));
        return;
    }
    MDNSSenderRData* refRData = static_cast<MDNSSenderRData*>(refRecord->GetRData());

    if (!refRData) {
        QCC_DbgPrintf(("Ignoring response with invalid sender-info"));
        return;
    }

    String guid = refRecord->GetDomainName().substr(sizeof("sender-info.") - 1, 32);
    if (guid == m_guid) {
        QCC_DbgPrintf(("Ignoring my own response"));
        return;
    }
    IPEndpoint r4, r6;
    IPEndpoint u4, u6;
    IPEndpoint ns4;
    ns4.port = refRData->GetIPV4ResponsePort();

    if (transportMask & TRANSPORT_TCP) {
        MDNSPtrRData* ptrRDataTcp = static_cast<MDNSPtrRData*>(answerTcp->GetRData());
        if (!ptrRDataTcp) {
            QCC_DbgPrintf(("Ignoring response with invalid sender-info"));
            return;
        }

        MDNSResourceRecord* srvAnswerTcp;
        if (!mdnsPacket->GetAnswer(ptrRDataTcp->GetPtrDName(), MDNSResourceRecord::SRV, &srvAnswerTcp)) {
            QCC_DbgPrintf(("Ignoring response without srv"));
            return;
        }
        MDNSSrvRData* srvRDataTcp = static_cast<MDNSSrvRData*>(srvAnswerTcp->GetRData());
        if (!srvRDataTcp) {
            QCC_DbgPrintf(("Ignoring response with invalid srv"));
            return;
        }
        r4.port = srvRDataTcp->GetPort();
        MDNSResourceRecord* txtAnswerTcp;
        if (mdnsPacket->GetAnswer(ptrRDataTcp->GetPtrDName(), MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &txtAnswerTcp)) {
            MDNSTextRData* txtRDataTcp = static_cast<MDNSTextRData*>(txtAnswerTcp->GetRData());
            if (!txtRDataTcp) {
                QCC_DbgPrintf(("Ignoring response with invalid txt"));
                return;
            }
            r6.port = StringToU32(txtRDataTcp->GetValue("r6port"));
        }
        MDNSResourceRecord* aRecord;
        if (mdnsPacket->GetAdditionalRecord(srvRDataTcp->GetTarget(), MDNSResourceRecord::A, &aRecord)) {
            MDNSARData* aRData = static_cast<MDNSARData*>(aRecord->GetRData());
            if (!aRData) {
                QCC_DbgPrintf(("Ignoring response with invalid ipv4 address"));
                return;
            }
            r4.addr = aRData->GetAddr();
            ns4.addr = aRData->GetAddr();
        }
        MDNSResourceRecord* aaaaRecord;
        if (mdnsPacket->GetAdditionalRecord(srvRDataTcp->GetTarget(), MDNSResourceRecord::AAAA, &aaaaRecord)) {
            MDNSAAAARData* aaaaRData = static_cast<MDNSAAAARData*>(aaaaRecord->GetRData());
            if (!aaaaRData) {
                QCC_DbgPrintf(("Ignoring response with invalid ipv6 address"));
                return;
            }
            r6.addr = aaaaRData->GetAddr();
        }
    }

    if (transportMask & TRANSPORT_UDP) {
        MDNSPtrRData* ptrRDataUdp = static_cast<MDNSPtrRData*>(answerUdp->GetRData());
        if (!ptrRDataUdp) {
            QCC_DbgPrintf(("Ignoring response with invalid sender-info"));
            return;
        }

        MDNSResourceRecord* srvAnswerUdp;
        if (!mdnsPacket->GetAnswer(ptrRDataUdp->GetPtrDName(), MDNSResourceRecord::SRV, &srvAnswerUdp)) {
            QCC_DbgPrintf(("Ignoring response without srv"));
            return;
        }
        MDNSSrvRData* srvRDataUdp = static_cast<MDNSSrvRData*>(srvAnswerUdp->GetRData());
        if (!srvRDataUdp) {
            QCC_DbgPrintf(("Ignoring response with invalid srv"));
            return;
        }
        u4.port = srvRDataUdp->GetPort();
        MDNSResourceRecord* txtAnswerUdp;
        if (mdnsPacket->GetAnswer(ptrRDataUdp->GetPtrDName(), MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &txtAnswerUdp)) {
            MDNSTextRData* txtRDataUdp = static_cast<MDNSTextRData*>(txtAnswerUdp->GetRData());
            if (!txtRDataUdp) {
                QCC_DbgPrintf(("Ignoring response with invalid txt"));
                return;
            }
            u6.port = StringToU32(txtRDataUdp->GetValue("u6port"));
        }
        MDNSResourceRecord* aRecord;
        if (mdnsPacket->GetAdditionalRecord(srvRDataUdp->GetTarget(), MDNSResourceRecord::A, &aRecord)) {
            MDNSARData* aRData = static_cast<MDNSARData*>(aRecord->GetRData());
            if (!aRData) {
                QCC_DbgPrintf(("Ignoring response with invalid ipv4 address"));
                return;
            }
            u4.addr = aRData->GetAddr();
            ns4.addr = aRData->GetAddr();
        }
        MDNSResourceRecord* aaaaRecord;
        if (mdnsPacket->GetAdditionalRecord(srvRDataUdp->GetTarget(), MDNSResourceRecord::AAAA, &aaaaRecord)) {
            MDNSAAAARData* aaaaRData = static_cast<MDNSAAAARData*>(aaaaRecord->GetRData());
            if (!aaaaRData) {
                QCC_DbgPrintf(("Ignoring response with invalid ipv6 address"));
                return;
            }
            u6.addr = aaaaRData->GetAddr();
        }
    }

    m_mutex.Lock();

    //
    // We first check if this packet was received over MDNS multicast port 5353
    // If Yes, only then are we interested in keeping track of the burst ID.
    //     We check if we have seen this packet with burst id from this GUID
    //     If Yes, we do not process this packet
    //     If No, we process this packet
    // If No, This is a unicast response in which case we need not keep track of Burst IDs
    //
    if (recvPort == MULTICAST_MDNS_PORT) {
        // We need to check if this packet is from a burst which we have seen before in which case we will ignore it
        if (!UpdateMDNSPacketTracker(guid, ns4, refRData->GetSearchID())) {
            QCC_DbgPrintf(("Ignoring response with duplicate burst ID"));
            m_mutex.Unlock();
            return;
        }
    }

    if (r4.addr.IsIPv4() && (ifIndexV4 == -1 || !SameNetwork(m_liveInterfaces[ifIndexV4].m_prefixlen,
                                                             m_liveInterfaces[ifIndexV4].m_address,
                                                             r4.addr))) {
        //
        // We expect that a v4 addr may be sent via a v6 link local address.  However
        // if a v4 addr is sent via a v4 address then someone is misbehaving, so log
        // a warning.
        //
        if (endpoint.addr.IsIPv4()) {
            QCC_DbgPrintf(("Ignoring advertisement from %s for %s received on %s",
                           endpoint.addr.ToString().c_str(),
                           r4.addr.ToString().c_str(),
                           ifName.c_str()));
        }
        m_mutex.Unlock();
        return;
    }

    //
    // Handle the advertised names first in case one of the registered response
    // handlers triggers an action that requires the name to be in the name
    // table (e.g. JoinSession).
    //
    HandleAdvertiseResponse(mdnsPacket, recvPort, guid, ns4, r4, r6, u4, u6);

    m_protectListeners = true;
    m_mutex.Unlock();
    bool handled = false;
    for (list<IpNameServiceListener*>::iterator it = m_listeners.begin(); !handled && it != m_listeners.end(); ++it) {
        handled = (*it)->ResponseHandler(transportMask, mdnsPacket, recvPort);
    }
    m_mutex.Lock();
    m_protectListeners = false;

    m_mutex.Unlock();
}

bool IpNameServiceImpl::HandleAdvertiseResponse(MDNSPacket mdnsPacket, uint16_t recvPort,
                                                const qcc::String& guid, const qcc::IPEndpoint& ns4,
                                                const qcc::IPEndpoint& r4, const qcc::IPEndpoint& r6, const qcc::IPEndpoint& u4, const qcc::IPEndpoint& u6)
{
    uint32_t numMatches = mdnsPacket->GetNumMatches("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS);
    for (uint32_t match = 0; match < numMatches; match++) {
        MDNSResourceRecord* advRecord;
        if (!mdnsPacket->GetAdditionalRecordAt("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, match, &advRecord)) {
            return false;
        }

        MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());
        if (!advRData) {
            QCC_DbgPrintf(("Ignoring response with invalid advertisement info"));
            return true;
        }
        uint32_t ttl = advRecord->GetRRttl();

        //
        // We need to populate our structure that keeps track of unicast ports of
        // services so that they can be polled for presence
        //
        if (ttl != 0) {
            AddToPeerInfoMap(guid, ns4);
        }

        vector<qcc::String> namesTcp;
        vector<qcc::String> namesUdp;

        for (uint8_t i = 0; i < advRData->GetNumNames(TRANSPORT_TCP | TRANSPORT_UDP); ++i) {
            String temp = advRData->GetNameAt(TRANSPORT_TCP | TRANSPORT_UDP, i);
            namesTcp.push_back(temp);
            namesUdp.push_back(temp);
        }
        for (uint8_t i = 0; i < advRData->GetNumNames(TRANSPORT_TCP); ++i) {
            String temp = advRData->GetNameAt(TRANSPORT_TCP, i);
            namesTcp.push_back(temp);
        }

        for (uint8_t i = 0; i < advRData->GetNumNames(TRANSPORT_UDP); ++i) {
            String temp = advRData->GetNameAt(TRANSPORT_UDP, i);
            namesUdp.push_back(temp);
        }

        //
        // Life is easier if we keep these things sorted.  Don't rely on the source
        // (even though it is really us) to do so.
        //
        sort(namesTcp.begin(), namesTcp.end());
        sort(namesUdp.begin(), namesUdp.end());

        //
        // In the version two protocol, the maximum size static buffer for the
        // longest bus address we can generate corresponds to two fully occupied
        // IPv4 addresses and two fully occupied IPV6 addresses.  So, we figure
        // that we need 31 bytes for the IPv4 endpoint information,
        // 55 bytes for the IPv6 endpoint information and one extra
        // comma:
        //
        //     "addr=192.168.100.101,port=65535,"
        //     "addr=ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff,port=65535"
        //
        // Adding a byte for the trailing '\0' we come up with 88 bytes of bus
        // address. C++ purists will object to using the C stdio routines but
        // they are simpler and faster since there are no memory allocations or
        // reallocations.
        //
        // Note that we do not prepend the bus address with the transport name,
        // i.e. "tcp:" since we assume that the transport knows its own name.
        //
        char busAddressTcp[88];
        char busAddressUdp[88];
        busAddressTcp[0] = '\0';
        busAddressUdp[0] = '\0';

        char addr6buf[60];
        addr6buf[0] = '\0';

        bool needComma = false;


        if (r4.port != 0 && r4.addr != IPAddress()) {
            snprintf(busAddressTcp, sizeof(busAddressTcp), "addr=%s,port=%d", r4.addr.ToString().c_str(), r4.port);
            needComma = true;
        }
        if (r6.port != 0 && r6.addr != IPAddress()) {
            if (needComma) {
                snprintf(addr6buf, sizeof(addr6buf), ",addr=%s,port=%d", r6.addr.ToString().c_str(), r6.port);
            } else {

                snprintf(addr6buf, sizeof(addr6buf), "addr=%s,port=%d", r6.addr.ToString().c_str(), r6.port);
            }
            strncat(busAddressTcp, &addr6buf[0], sizeof(addr6buf));

        }
        needComma = false;
        if (u4.port != 0 && u4.addr != IPAddress()) {

            snprintf(busAddressUdp, sizeof(busAddressUdp), "addr=%s,port=%d", u4.addr.ToString().c_str(), u4.port);
            needComma = true;
        }

        if (u6.port != 0 && u6.addr != IPAddress()) {
            if (needComma) {
                snprintf(addr6buf, sizeof(addr6buf), ",addr=%s,port=%d", u6.addr.ToString().c_str(), u6.port);
            } else {

                snprintf(addr6buf, sizeof(addr6buf), "addr=%s,port=%d", u6.addr.ToString().c_str(), u6.port);
            }
            strncat(busAddressUdp, &addr6buf[0], sizeof(addr6buf));

        }

        if ((namesUdp.size() > 0) && m_callback[TRANSPORT_INDEX_UDP]) {
            m_protect_callback = true;
            m_mutex.Unlock();
            (*m_callback[TRANSPORT_INDEX_UDP])(busAddressUdp, guid, namesUdp, ttl);
            m_mutex.Lock();
            m_protect_callback = false;
        }

        if ((namesTcp.size() > 0) && m_callback[TRANSPORT_INDEX_TCP]) {
            m_protect_callback = true;
            m_mutex.Unlock();
            (*m_callback[TRANSPORT_INDEX_TCP])(busAddressTcp, guid, namesTcp, ttl);
            m_mutex.Lock();
            m_protect_callback = false;
        }
    }
    return true;
}

void IpNameServiceImpl::HandleProtocolQuery(MDNSPacket mdnsPacket, IPEndpoint endpoint, uint16_t recvPort)
{
    bool isAllJoynQuery = true;
    // Check if someone is asking about an alljoyn service.
    MDNSQuestion* questionTcp;
    MDNSQuestion* questionUdp;
    TransportMask completeTransportMask = TRANSPORT_NONE;
    if (mdnsPacket->GetQuestion("_alljoyn._tcp.local.", &questionTcp)) {
        isAllJoynQuery = true;
        completeTransportMask |= TRANSPORT_TCP;
    }
    if (mdnsPacket->GetQuestion("_alljoyn._udp.local.", &questionUdp)) {
        isAllJoynQuery = true;
        completeTransportMask |= TRANSPORT_UDP;
    }
    if (!isAllJoynQuery) {
        QCC_DbgPrintf(("IpNameServiceImpl::HandleProtocolQuery Ignoring Non-AllJoyn related query"));
        return;
    }
    MDNSResourceRecord* refRecord;
    if (!mdnsPacket->GetAdditionalRecord("sender-info.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &refRecord)) {
        QCC_DbgPrintf(("Ignoring query without sender info"));
        return;
    }
    MDNSSenderRData* refRData = static_cast<MDNSSenderRData*>(refRecord->GetRData());
    if (!refRData) {
        QCC_DbgPrintf(("Ignoring query with invalid sender info"));
        return;
    }
    IPEndpoint ns4(refRData->GetIPV4ResponseAddr(), refRData->GetIPV4ResponsePort());

    String guid = refRecord->GetDomainName().substr(sizeof("sender-info.") - 1, 32);
    if (guid == m_guid) {
        QCC_DbgPrintf(("Ignoring my own query"));
        return;
    }
    m_mutex.Lock();

    //
    // We first check if this packet was received over MDNS multicast port 5353
    // If Yes, only then are we interested in keeping track of the burst ID.
    //     We check if we have seen this packet with burst id from this GUID
    //     If Yes, we do not process this packet
    //     If No, we process this packet
    // If No, This is a unicast response in which case we need not keep track of Burst IDs
    //
    if (recvPort == MULTICAST_MDNS_PORT) {
        // We need to check if this packet is from a burst which we have seen before in which case we will ignore it
        if (!UpdateMDNSPacketTracker(guid, ns4, refRData->GetSearchID())) {
            QCC_DbgPrintf(("Ignoring query with duplicate burst ID"));
            m_mutex.Unlock();
            return;
        }
    }
    m_protectListeners = true;
    m_mutex.Unlock();
    bool handled = false;
    for (list<IpNameServiceListener*>::iterator it = m_listeners.begin(); !handled && it != m_listeners.end(); ++it) {
        handled = (*it)->QueryHandler(completeTransportMask, mdnsPacket, recvPort, ns4);
    }
    m_mutex.Lock();
    m_protectListeners = false;
    if (handled) {
        m_mutex.Unlock();
        return;
    }
    HandleSearchQuery(completeTransportMask, mdnsPacket, recvPort, guid, ns4);

    m_mutex.Unlock();
}

bool IpNameServiceImpl::HandleSearchQuery(TransportMask completeTransportMask, MDNSPacket mdnsPacket, uint16_t recvPort,
                                          const qcc::String& guid, const qcc::IPEndpoint& ns4)
{
    QCC_DbgPrintf(("IpNameServiceImpl::HandleSearchQuery"));
    MDNSResourceRecord* searchRecord;
    if (!mdnsPacket->GetAdditionalRecord("search.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &searchRecord)) {
        return false;
    }

    MDNSSearchRData* searchRData = static_cast<MDNSSearchRData*>(searchRecord->GetRData());
    if (!searchRData) {
        QCC_DbgPrintf(("Ignoring query with invalid search info"));
        return true;
    }

    vector<String> wkns;
    //
    // The who-has message doesn't specify which transport is doing the asking.
    // This is an oversight and should be fixed in a subsequent version.  The
    // only reasonable thing to do is to return name matches found in all of
    // the advertising transports.
    //
    for (uint32_t index = 0; index < N_TRANSPORTS; ++index) {

        //
        // If there are no names being advertised by the transport identified by
        // its index (actively or quietly), there is nothing to do.
        //
        if (m_advertised[index].empty() && m_advertised_quietly[index].empty()) {
            continue;
        }

        //
        // Loop through the names we are being asked about, and if we have
        // advertised any of them, we are going to need to respond to this
        // question.  Keep track of whether or not any of our corresponding
        // advertisements are quiet, since we want to respond quietly to a
        // question about a quiet advertisements.  That is, if any of the names
        // the client is asking about corresponds to a quiet advertisement we
        // respond directly to the client and do not multicast the response.
        // The only way we multicast a response is if the client does not ask
        // about any of our quietly advertised names.
        //
        // Becuse of this requirement, we loop through all of the names in the
        // who-has message to see if any of them correspond to quiet
        // advertisements.  We don't just break out and respond if we find any
        // old match since it may be the case that the last name is the quiet
        // one.
        //
        bool respond = false;
        bool respondQuietly = false;
        for (int i = 0; i < searchRData->GetNumNames(); ++i) {
            String wkn = searchRData->GetNameAt(i);
            if (searchRData->SendMatchOnly()) {
                wkns.push_back(wkn);
            }
            //
            // Zero length strings are unmatchable.  If you want to do a wildcard
            // match, you've got to send a wildcard character.
            //
            if (wkn.size() == 0) {
                continue;
            }

            //
            // Check to see if this name on the list of names we actively advertise.
            //
            for (set<String>::iterator j = m_advertised[index].begin(); j != m_advertised[index].end(); ++j) {

                //
                // The requested name comes in from the WhoHas message and we
                // allow wildcards there.
                //
                if (WildcardMatch((*j), wkn)) {
                    QCC_DbgPrintf(("IpNameServiceImpl::HandleSearchQuery(): request for %s does not match my %s",
                                   wkn.c_str(), (*j).c_str()));
                    continue;
                } else {
                    respond = true;
                    break;
                }
            }

            //
            // Check to see if this name on the list of names we quietly advertise.
            //
            for (set<String>::iterator j = m_advertised_quietly[index].begin(); j != m_advertised_quietly[index].end(); ++j) {

                //
                // The requested name comes in from the WhoHas message and we
                // allow wildcards there.
                //
                if (WildcardMatch((*j), wkn)) {
                    QCC_DbgPrintf(("IpNameServiceImpl::HandleSearchQuery(): request for %s does not match my %s",
                                   wkn.c_str(), (*j).c_str()));
                    continue;
                } else {
                    respond = true;
                    respondQuietly = true;
                    break;
                }
            }
        }
        //
        // Since any response we send must include all of the advertisements we
        // are exporting; this just means to retransmit all of our advertisements.
        //
        if (respond) {
            m_mutex.Unlock();
            if (ns4.GetAddress().IsIPv4()) {
                Retransmit(index, false, respondQuietly, ns4, TRANSMIT_V2, completeTransportMask, wkns);
            }
            m_mutex.Lock();
        }
    }
    return true;
}

QStatus IpNameServiceImpl::Start(void* arg, qcc::ThreadListener* listener)
{
    QCC_DbgPrintf(("IpNameServiceImpl::Start()"));
    m_mutex.Lock();
    assert(IsRunning() == false);
    m_state = IMPL_RUNNING;
    QCC_DbgPrintf(("IpNameServiceImpl::Start(): Starting thread"));
    QStatus status = Thread::Start(this, listener);
    QCC_DbgPrintf(("IpNameServiceImpl::Start(): Started"));
    m_mutex.Unlock();
    m_packetScheduler.Start();
    return status;
}

bool IpNameServiceImpl::Started()
{
    return IsRunning();
}

QStatus IpNameServiceImpl::Stop()
{
    QCC_DbgPrintf(("IpNameServiceImpl::Stop()"));
    m_mutex.Lock();
    if (m_state != IMPL_SHUTDOWN) {
        m_state = IMPL_STOPPING;
    }
    QCC_DbgPrintf(("IpNameServiceImpl::Stop(): Stopping thread"));
    QStatus status = Thread::Stop();
    QCC_DbgPrintf(("IpNameServiceImpl::Stop(): Stopped"));
    m_packetScheduler.Stop();
    m_mutex.Unlock();
    return status;
}

QStatus IpNameServiceImpl::Join()
{
    m_packetScheduler.Join();
    QCC_DbgPrintf(("IpNameServiceImpl::Join()"));
    assert(m_state == IMPL_STOPPING || m_state == IMPL_SHUTDOWN);
    QCC_DbgPrintf(("IpNameServiceImpl::Join(): Joining thread"));
    QStatus status = Thread::Join();
    QCC_DbgPrintf(("IpNameServiceImpl::Join(): Joined"));
    m_state = IMPL_SHUTDOWN;
    return status;
}

//
// Count the number of bits set in a 32-bit word using one of the many
// well-known high-performance algorithms for calculating Population Count while
// determining Hamming Distance.  It's completely obscure and mostly
// incomprehensible at first glance.  Google hamming distance or popcount if you
// dare.
//
// This is a well-investigated operation so similar code snippets are widely
// available on the web and are in the public domain.
//
// We use this method in the process of ensuring that only one bit is set in a
// TransportMask.  This is because there must be a one-to-one correspondence
// between a transport mask bit and a transport.
//
uint32_t IpNameServiceImpl::CountOnes(uint32_t data)
{
    QCC_DbgPrintf(("IpNameServiceImpl::CountOnes(0x%x)", data));

    data = data - ((data >> 1) & 0x55555555);
    data = (data & 0x33333333) + ((data >> 2) & 0x33333333);
    uint32_t result = (((data + (data >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;

    QCC_DbgPrintf(("IpNameServiceImpl::CountOnes(): %d bits are set", result));
    return result;
}

//
// Convert a data word with one bit set to an index into a table corresponding
// to that bit.  This uses one of the many well-known high performance
// algorithms for counting the number of consecutive trailing zero bits in an
// integer.  This is similar to finding log base two of the data word.  Google
// consecutive trailing zero bits if you dare.
//
// This is a well-investigated operation so similar code snippets are widely
// available on the web and are in the public domain.
//
// We use this method to convert from a transport mask to an index into a table
// corresponding to some property of the transport that is using the name service.
// We assume that the data has been verified to contain one bit set in the low
// order word.
//
uint32_t IpNameServiceImpl::IndexFromBit(uint32_t data)
{
    QCC_DbgPrintf(("IpNameServiceImpl::IndexFromBit(0x%x)", data));

    uint32_t c = 32;
    data &= -signed(data);

    if (data) {
        --c;
    }
    if (data & 0x0000ffff) {
        c -= 16;
    }
    if (data & 0x00ff00ff) {
        c -= 8;
    }
    if (data & 0x0f0f0f0f) {
        c -= 4;
    }
    if (data & 0x33333333) {
        c -= 2;
    }
    if (data & 0x55555555) {
        c -= 1;
    }

    //
    // If the number of trailing bits that are set to zero is count, then the
    // first set bit must be at position count + 1.  Since array indices are
    // zero-based, the index into an array corresponding to the first set bit
    // is count (index == number of trailing zero bits).
    //
    QCC_DbgPrintf(("IpNameServiceImpl::IndexFromBit(): Index is %d.", c));
    assert(c < 16 && "IpNameServiceImpl::IndexFromBit(): Bad transport index");
    return c;
}

//
// Convert a data word with one bit set to an index into a table corresponding
// to that bit.  This uses one of the many well-known high performance
// algorithms for counting the number of consecutive trailing zero bits in an
// integer.  This is similar to finding log base two of the data word.  Google
// consecutive trailing zero bits if you dare.
//
// This is a well-investigated operation so similar code snippets are widely
// available on the web and are in the public domain.
//
// We use this method to convert from a transport mask to an index into a table
// corresponding to some property of the transport that is using the name service.
// We assume that the data has been verified to contain one bit set in the low
// order word.
//
TransportMask IpNameServiceImpl::MaskFromIndex(uint32_t index)
{
    QCC_DbgPrintf(("IpNameServiceImpl::MaskFromIndex(%d.)", index));
    uint32_t result = 1 << index;
    QCC_DbgPrintf(("IpNameServiceImpl::MaskFromIndex(): Bit is 0x%x", result));
    return result;
}

set<String> IpNameServiceImpl::GetAdvertising(TransportMask transportMask) {
    set<String> set_common, set_return;
    std::set<String> empty;
    set_intersection(m_advertised[TRANSPORT_INDEX_TCP].begin(), m_advertised[TRANSPORT_INDEX_TCP].end(), m_advertised[TRANSPORT_INDEX_UDP].begin(), m_advertised[TRANSPORT_INDEX_UDP].end(), std::inserter(set_common, set_common.end()));

    if (transportMask == TRANSPORT_TCP || transportMask == TRANSPORT_UDP) {


        uint32_t transportIndex = IndexFromBit(transportMask);
        if (transportIndex >= 16) {
            return empty;
        }

        set_difference(m_advertised[transportIndex].begin(), m_advertised[transportIndex].end(), set_common.begin(), set_common.end(), std::inserter(set_return, set_return.end()));
        return set_return;
    }
    if (transportMask == (TRANSPORT_TCP | TRANSPORT_UDP)) {
        return set_common;
    }

    return empty;

}
set<String> IpNameServiceImpl::GetAdvertisingQuietly(TransportMask transportMask) {
    set<String> set_common, set_return;
    std::set<String> empty;
    set_intersection(m_advertised_quietly[TRANSPORT_INDEX_TCP].begin(), m_advertised_quietly[TRANSPORT_INDEX_TCP].end(), m_advertised_quietly[TRANSPORT_INDEX_UDP].begin(), m_advertised_quietly[TRANSPORT_INDEX_UDP].end(), std::inserter(set_common, set_common.end()));

    if (transportMask == TRANSPORT_TCP || transportMask == TRANSPORT_UDP) {
        uint32_t transportIndex = IndexFromBit(transportMask);
        if (transportIndex >= 16) {
            return empty;
        }

        set_difference(m_advertised_quietly[transportIndex].begin(), m_advertised_quietly[transportIndex].end(), set_common.begin(), set_common.end(), std::inserter(set_return, set_return.end()));
        return set_return;
    }
    if (transportMask == (TRANSPORT_TCP | TRANSPORT_UDP)) {

        return set_common;
    }

    return empty;

}

bool IpNameServiceImpl::PurgeAndUpdatePacket(MDNSPacket mdnspacket, bool updateSid)
{
    MDNSResourceRecord* refRecord;
    mdnspacket->GetAdditionalRecord("sender-info.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &refRecord);
    MDNSSenderRData* refRData = static_cast<MDNSSenderRData*>(refRecord->GetRData());
    int32_t id = IncrementAndFetch(&INCREMENTAL_PACKET_ID);
    if (updateSid) {
        refRData->SetSearchID(id);
    }
    if (mdnspacket->GetHeader().GetQRType() == MDNSHeader::MDNS_QUERY) {
        MDNSResourceRecord* searchRecord;
        mdnspacket->GetAdditionalRecord("search.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &searchRecord);
        MDNSSearchRData* searchRData = static_cast<MDNSSearchRData*>(searchRecord->GetRData());

        set<String> set_union_tcp_udp;
        set_union(m_v2_queries[TRANSPORT_INDEX_TCP].begin(), m_v2_queries[TRANSPORT_INDEX_TCP].end(), m_v2_queries[TRANSPORT_INDEX_UDP].begin(), m_v2_queries[TRANSPORT_INDEX_UDP].end(), std::inserter(set_union_tcp_udp, set_union_tcp_udp.end()));
        uint32_t numSearch = searchRData->GetNumSearchCriteria();
        for (uint32_t k = 0; k < numSearch; k++) {
            String crit = searchRData->GetSearchCriterion(k);
            if (std::find(set_union_tcp_udp.begin(), set_union_tcp_udp.end(), crit) == set_union_tcp_udp.end()) {
                searchRData->RemoveSearchCriterion(k);
                k--;
                numSearch = searchRData->GetNumSearchCriteria();
            }
        }

        if (m_v2_queries[TRANSPORT_INDEX_TCP].size() == 0) {
            //Remove TCP PTR/SRV/TXT records
            MDNSResourceRecord* ptrRecord;
            if (mdnspacket->GetAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, &ptrRecord)) {
                MDNSPtrRData* ptrRData = static_cast<MDNSPtrRData*>(ptrRecord->GetRData());
                String name = ptrRData->GetPtrDName();
                mdnspacket->RemoveAnswer(name, MDNSResourceRecord::SRV);
                mdnspacket->RemoveAnswer(name, MDNSResourceRecord::TXT);
                mdnspacket->RemoveAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR);
            }
        }
        if (m_v2_queries[TRANSPORT_INDEX_UDP].size() == 0) {
            //Remove UDP PTR/SRV/TXT records
            MDNSResourceRecord* ptrRecord;
            if (mdnspacket->GetAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR, &ptrRecord)) {
                MDNSPtrRData* ptrRData = static_cast<MDNSPtrRData*>(ptrRecord->GetRData());
                String name = ptrRData->GetPtrDName();
                mdnspacket->RemoveAnswer(name, MDNSResourceRecord::SRV);
                mdnspacket->RemoveAnswer(name, MDNSResourceRecord::TXT);
                mdnspacket->RemoveAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR);
            }

        }
        return (numSearch > 0);
    } else {
        MDNSResourceRecord* advRecord;
        mdnspacket->GetAdditionalRecord("advertise.*", MDNSResourceRecord::TXT, MDNSTextRData::TXTVERS, &advRecord);
        MDNSAdvertiseRData* advRData = static_cast<MDNSAdvertiseRData*>(advRecord->GetRData());

        TransportMask transportMaskArr[3] = { TRANSPORT_TCP, TRANSPORT_UDP, TRANSPORT_TCP | TRANSPORT_UDP };
        uint32_t numNamesTotal = 0;
        uint32_t ttl = advRecord->GetRRttl();
        uint32_t numNames[3];
        for (int i = 0; i < 3; i++) {
            TransportMask tm = transportMaskArr[i];
            set<String> advertising = GetAdvertising(tm);
            numNames[i] = advRData->GetNumNames(tm);
            for (uint32_t k = 0; k < numNames[i]; k++) {
                if (ttl == 0) {
                    //If this is a packet with ttl == 0, ensure that we are NOT advertising the names mentioned in the packet.

                    if (std::find(advertising.begin(), advertising.end(), advRData->GetNameAt(tm, k)) != advertising.end()) {

                        advRData->RemoveNameAt(tm, k);
                        // a name has been removed from the IsAt response header make
                        // sure the numNames used in the for loop is updated to reflect
                        // the removal of that name.
                        k = k - 1;
                        numNames[i] = advRData->GetNumNames(tm);
                    }
                } else {
                    //If this is a packet with ttl >0, ensure that we are still advertising all the names mentioned in the packet.
                    // If only one of the transports has been enabled because the interface specified for the other transport
                    // is yet to be IFF_UP, then restrict the search space to only the transport that is enabled.

                    if ((tm == (TRANSPORT_TCP | TRANSPORT_UDP)) && m_enabledReliableIPv4[TRANSPORT_INDEX_TCP] && !m_enabledUnreliableIPv4[TRANSPORT_INDEX_UDP]) {
                        advertising = GetAdvertising(TRANSPORT_TCP);
                    }
                    if ((tm == (TRANSPORT_TCP | TRANSPORT_UDP)) && !m_enabledReliableIPv4[TRANSPORT_INDEX_TCP] && m_enabledUnreliableIPv4[TRANSPORT_INDEX_UDP]) {
                        advertising = GetAdvertising(TRANSPORT_UDP);
                    }
                    if (std::find(advertising.begin(), advertising.end(), advRData->GetNameAt(tm, k)) == advertising.end()) {

                        advRData->RemoveNameAt(tm, k);
                        // a name has been removed from the IsAt response header make
                        // sure the numNames used in the for loop is updated to reflect
                        // the removal of that name.
                        k = k - 1;
                        numNames[i] = advRData->GetNumNames(tm);

                    }

                }
            }
            numNamesTotal += numNames[i];


        }

        if (numNames[0] == 0 && numNames[2] == 0) {
            //Remove TCP PTR/SRV/TXT records
            MDNSResourceRecord* ptrRecord;
            if (mdnspacket->GetAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, &ptrRecord)) {
                MDNSPtrRData* ptrRData = static_cast<MDNSPtrRData*>(ptrRecord->GetRData());
                String name = ptrRData->GetPtrDName();
                mdnspacket->RemoveAnswer(name, MDNSResourceRecord::SRV);
                mdnspacket->RemoveAnswer(name, MDNSResourceRecord::TXT);
                mdnspacket->RemoveAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR);
            }
        }
        if (numNames[1] == 0 && numNames[2] == 0) {
            //Remove UDP PTR/SRV/TXT records
            MDNSResourceRecord* ptrRecord;
            if (mdnspacket->GetAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR, &ptrRecord)) {
                MDNSPtrRData* ptrRData = static_cast<MDNSPtrRData*>(ptrRecord->GetRData());
                String name = ptrRData->GetPtrDName();
                mdnspacket->RemoveAnswer(name, MDNSResourceRecord::SRV);
                mdnspacket->RemoveAnswer(name, MDNSResourceRecord::TXT);
                mdnspacket->RemoveAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR);
            }

        }

        return (numNamesTotal > 0);
    }
    return false;
}
ThreadReturn STDCALL IpNameServiceImpl::PacketScheduler::Run(void* arg) {

    m_impl.m_mutex.Lock();
    while (!IsStopping()) {
        Timespec now;
        GetTimeNow(&now);
        uint32_t timeToSleep = -1;
        //Step 1: Collect all packets
        std::list<Packet> subsequentBurstpackets;
        std::list<Packet> initialBurstPackets;
        subsequentBurstpackets.clear();
        initialBurstPackets.clear();

        // If doAnyNetworkCallback is true, then one of the transports
        // is waiting for us to supply the list of live interfaces so
        // it can get things started. We only want to provide the
        // sub-set of live interfaces that have been requested by
        // each transport (by name or addr) when the callback is invoked.
        bool doAnyNetworkCallback = false;
        for (uint32_t transportIndex = 0; transportIndex < N_TRANSPORTS; transportIndex++) {
            if (m_impl.m_doNetworkCallback[transportIndex]) {
                doAnyNetworkCallback = true;
                break;
            }
        }

        if (doAnyNetworkCallback) {
            std::map<qcc::String, qcc::IPAddress> ifMap;
            for (uint32_t i = 0; (m_impl.m_state == IMPL_RUNNING) && (i < m_impl.m_liveInterfaces.size()); ++i) {
                if (m_impl.m_liveInterfaces[i].m_address.IsIPv4()) {
                    ifMap[m_impl.m_liveInterfaces[i].m_interfaceName] = m_impl.m_liveInterfaces[i].m_address;
                }
            }
            if (!ifMap.empty()) {
                for (uint32_t transportIndex = 0; transportIndex < N_TRANSPORTS; transportIndex++) {
                    if (m_impl.m_networkEventCallback[transportIndex] && m_impl.m_doNetworkCallback[transportIndex]) {
                        std::map<qcc::String, qcc::IPAddress> transportIfMap;
                        for (uint32_t j = 0; j < m_impl.m_requestedInterfaces[transportIndex].size(); j++) {
                            for (std::map<qcc::String, qcc::IPAddress>::iterator it = ifMap.begin(); it != ifMap.end(); it++) {
                                qcc::String name = it->first;
                                qcc::IPAddress addr = it->second;
                                if (m_impl.m_requestedInterfaces[transportIndex][j].m_interfaceName == name || m_impl.m_requestedInterfaces[transportIndex][j].m_interfaceAddr == addr) {
                                    transportIfMap[name] = addr;
                                }
                            }
                        }
                        if (m_impl.m_any[transportIndex]) {
                            transportIfMap = ifMap;
                        }
                        if (!transportIfMap.empty()) {
                            m_impl.m_protect_net_callback = true;
                            m_impl.m_mutex.Unlock();
                            (*m_impl.m_networkEventCallback[transportIndex])(ifMap);
                            m_impl.m_mutex.Lock();
                            m_impl.m_protect_net_callback = false;
                        }
                    }
                    m_impl.m_doNetworkCallback[transportIndex] = false;
                }
            }
        }
        //Collect network change burst packets
        if ((m_impl.m_networkChangeScheduleCount <= m_impl.m_retries) && ((m_impl.m_networkChangeScheduleCount == 0) || ((m_impl.m_networkChangeTimeStamp - now) < PACKET_TIME_ACCURACY_MS))) {

#ifndef QCC_OS_GROUP_WINDOWS
            if (!m_impl.m_networkEvents.empty()) {
                for (std::set<uint32_t>::const_iterator iter =  m_impl.m_networkEvents.begin(); iter !=  m_impl.m_networkEvents.end(); iter++) {
                    qcc::AddressFamily family = qcc::QCC_AF_UNSPEC;
                    int32_t interfaceIndex = -1;
                    if (NETWORK_EVENT_IF_FAMILY(*iter) == qcc::QCC_AF_INET_INDEX) {
                        family = qcc::QCC_AF_INET;
                    }
                    if (NETWORK_EVENT_IF_FAMILY(*iter) == qcc::QCC_AF_INET6_INDEX) {
                        family = qcc::QCC_AF_INET6;
                    }
                    interfaceIndex = NETWORK_EVENT_IF_INDEX(*iter);
#if defined(QCC_OS_LINUX)
                    // If this is a loopback interface and we have an event for IPv6
                    // address change, we also add an event for the IPv4 address of
                    // the loopback interface as we don't get an event for IPv4
                    // address changes on ifdown/up on loopback interfaces unless
                    // the IPv4 address is also removed.
                    for (uint32_t i = 0; (m_impl.m_state == IMPL_RUNNING) && (i < m_impl.m_liveInterfaces.size()); ++i) {
                        int currentIndex = m_impl.m_liveInterfaces[i].m_index;
                        if (currentIndex == interfaceIndex && (m_impl.m_liveInterfaces[i].m_flags & qcc::IfConfigEntry::LOOPBACK)) {
                            family = qcc::QCC_AF_UNSPEC;
                            break;
                        }
                    }
#endif
                    m_impl.GetResponsePackets(subsequentBurstpackets, false, qcc::IPEndpoint("0.0.0.0", 0), TRANSMIT_V2, (TRANSPORT_TCP | TRANSPORT_UDP), interfaceIndex, family);
                    m_impl.GetQueryPackets(subsequentBurstpackets, (TRANSMIT_V0_V1 | TRANSMIT_V2), interfaceIndex, family);
                }
            }
#else
            m_impl.GetResponsePackets(subsequentBurstpackets);
            m_impl.GetQueryPackets(subsequentBurstpackets);
#endif
            if (m_impl.m_networkChangeScheduleCount == 0) {
                m_impl.m_networkChangeTimeStamp = now + RETRY_INTERVALS[0] * 1000;
                std::map<qcc::String, qcc::IPAddress> ifMap;
#ifndef QCC_OS_GROUP_WINDOWS
                // For the transport callbacks, we want to include only the
                // interfaces that have changed their IPv4 addresses or the
                // loopback interfaces as these retain their IPv4 addresses
                // on interface down events on Linux.
                // In addition, we want to include the interfaces with IPv4
                // addresses that have changed on platforms where we do not
                // have information about the address family that changed.
                // We also want to include all interfaces with IPv4 addresses
                // on platforms where we do not have information about which
                // interface index/address family has changed.
                for (std::set<uint32_t>::const_iterator it = m_impl.m_networkEvents.begin(); it != m_impl.m_networkEvents.end(); it++) {
                    for (uint32_t i = 0; (m_impl.m_state == IMPL_RUNNING) && (i < m_impl.m_liveInterfaces.size()); ++i) {
                        bool sameInterfaceIndex = (m_impl.m_liveInterfaces[i].m_index == NETWORK_EVENT_IF_INDEX(*it));
                        bool interfaceAddrIsIPv4 = m_impl.m_liveInterfaces[i].m_address.IsIPv4();
                        bool ipv4OrUnspecifiedEvent = (NETWORK_EVENT_IF_FAMILY(*it) == qcc::QCC_AF_INET_INDEX || NETWORK_EVENT_IF_FAMILY(*it) == qcc::QCC_AF_UNSPEC_INDEX);
                        bool loopbackInterface = ((m_impl.m_liveInterfaces[i].m_flags & qcc::IfConfigEntry::LOOPBACK) != 0);
                        if (sameInterfaceIndex && interfaceAddrIsIPv4 && (ipv4OrUnspecifiedEvent || loopbackInterface)) {
                            ifMap[m_impl.m_liveInterfaces[i].m_interfaceName] = m_impl.m_liveInterfaces[i].m_address;
                            break;
                        }
                    }
                }
#else
                for (uint32_t i = 0; (m_impl.m_state == IMPL_RUNNING) && (i < m_impl.m_liveInterfaces.size()); ++i) {
                    if (m_impl.m_liveInterfaces[i].m_address.IsIPv4()) {
                        ifMap[m_impl.m_liveInterfaces[i].m_interfaceName] = m_impl.m_liveInterfaces[i].m_address;
                    }
                }
#endif
                if (!ifMap.empty()) {
                    for (uint32_t transportIndex = 0; transportIndex < N_TRANSPORTS; transportIndex++) {
                        if (m_impl.m_networkEventCallback[transportIndex]) {
                            std::map<qcc::String, qcc::IPAddress> transportIfMap;
                            for (uint32_t j = 0; j < m_impl.m_requestedInterfaces[transportIndex].size(); j++) {
                                for (std::map<qcc::String, qcc::IPAddress>::iterator it = ifMap.begin(); it != ifMap.end(); it++) {
                                    qcc::String name = it->first;
                                    qcc::IPAddress addr = it->second;
                                    if (m_impl.m_requestedInterfaces[transportIndex][j].m_interfaceName == name || m_impl.m_requestedInterfaces[transportIndex][j].m_interfaceAddr == addr) {
                                        transportIfMap[name] = addr;
                                    }
                                }
                            }
                            if (m_impl.m_any[transportIndex]) {
                                transportIfMap = ifMap;
                            }
                            if (!transportIfMap.empty()) {
                                m_impl.m_protect_net_callback = true;
                                m_impl.m_mutex.Unlock();
                                (*m_impl.m_networkEventCallback[transportIndex])(ifMap);
                                m_impl.m_mutex.Lock();
                                m_impl.m_protect_net_callback = false;
                            }
                        }
                    }
                }
            } else {
                //adjust m_networkChangeTimeStamp
                m_impl.m_networkChangeTimeStamp += RETRY_INTERVALS[m_impl.m_networkChangeScheduleCount] * 1000 + (BURST_RESPONSE_RETRIES) *BURST_RESPONSE_INTERVAL;
            }
            if (now < m_impl.m_networkChangeTimeStamp) {
                uint32_t delay = m_impl.m_networkChangeTimeStamp - now;
                if (timeToSleep > delay) {
                    timeToSleep = delay;
                }

            } else {
                timeToSleep = 0;
            }

            //adjust m_networkChangeScheduleCount
            m_impl.m_networkChangeScheduleCount++;
            if (m_impl.m_networkChangeScheduleCount > m_impl.m_retries) {
                m_impl.m_networkEvents.clear();
            }
        }

        //Collect unsolicited Advertise/CancelAdvertise/FindAdvertisement burst packets
        std::list<BurstResponseHeader>::iterator it = m_impl.m_burstQueue.begin();
        it = m_impl.m_burstQueue.begin();

        while (it != m_impl.m_burstQueue.end()) {
            if (((*it).nextScheduleTime - now) < PACKET_TIME_ACCURACY_MS) {
                uint32_t nsVersion;
                uint32_t msgVersion;
                (*it).packet->GetVersion(nsVersion, msgVersion);
                if (msgVersion == 2) {
                    MDNSPacket mdnspacket = MDNSPacket::cast((*it).packet);
                    //PurgeAndUpdatePacket will remove any names that have changed - not being advertised/discovered
                    // and also update the burst ID in the packet.
                    if (!m_impl.PurgeAndUpdatePacket(mdnspacket, (*it).scheduleCount != 0)) {
                        //No names found, remove this packet
                        m_impl.m_burstQueue.erase(it++);
                        continue;
                    }
                }


                if ((*it).scheduleCount == 0) {
                    initialBurstPackets.push_back((*it).packet);
                    (*it).nextScheduleTime += RETRY_INTERVALS[(*it).scheduleCount] * 1000 - BURST_RESPONSE_INTERVAL;
                } else {
                    subsequentBurstpackets.push_back((*it).packet);
                    (*it).nextScheduleTime += RETRY_INTERVALS[(*it).scheduleCount] * 1000 + (BURST_RESPONSE_RETRIES) *BURST_RESPONSE_INTERVAL;

                }


                //if scheduleCount has reached max_retries, get rid of entry and advance iterator.
                if ((*it).scheduleCount == m_impl.m_retries) {
                    m_impl.m_burstQueue.erase(it++);
                    continue;
                }

                (*it).scheduleCount++;

            }

            if (now < (*it).nextScheduleTime) {
                uint32_t delay = (*it).nextScheduleTime - now;
                if (timeToSleep > delay) {
                    timeToSleep = delay;
                }
            } else {
                timeToSleep = 0;
            }
            it++;

        }
        m_impl.m_mutex.Unlock();
        //Step 2: Burst the packets
        uint32_t burstIndex  = 0;
        while (burstIndex < BURST_RESPONSE_RETRIES && (!subsequentBurstpackets.empty() || !initialBurstPackets.empty()) && !IsStopping()) {

            //If this is the first burst in the schedule, queue one less packet, first one is queued by TriggerTransmission
            if (burstIndex != BURST_RESPONSE_RETRIES - 1) {
                for (std::list<Packet>::const_iterator i = initialBurstPackets.begin(); i != initialBurstPackets.end(); i++) {
                    Packet packet = *i;
                    uint32_t nsVersion;
                    uint32_t msgVersion;

                    packet->GetVersion(nsVersion, msgVersion);
                    if (msgVersion == 2) {
                        m_impl.QueueProtocolMessage(*i);
                    }

                }
            }

            for (std::list<Packet>::const_iterator i = subsequentBurstpackets.begin(); i != subsequentBurstpackets.end(); i++) {
                Packet packet = *i;
                uint32_t nsVersion;
                uint32_t msgVersion;

                packet->GetVersion(nsVersion, msgVersion);
                if ((msgVersion == 2) || (burstIndex == 0)) {
                    m_impl.QueueProtocolMessage(*i);
                }

            }
            // Wait for burst interval = BURST_RESPONSE_INTERVAL
            Event::Wait(Event::neverSet, BURST_RESPONSE_INTERVAL);
            GetStopEvent().ResetEvent();
            burstIndex++;
        }
        m_impl.m_mutex.Lock();
        //Step 3: Wait for a specific amount of time
        if (!IsStopping()) {
            m_impl.m_mutex.Unlock();
            Event::Wait(Event::neverSet, timeToSleep);
            GetStopEvent().ResetEvent();
            m_impl.m_mutex.Lock();

        }
    }
    m_impl.m_burstQueue.clear();
    m_impl.m_mutex.Unlock();

    return 0;

}
} // namespace ajn
