/**
 * @file
 * A mechanism to get network interface configurations a la Unix/Linux ifconfig.
 */

/******************************************************************************
 * Copyright (c) 2010-2011,2014, AllSeen Alliance. All rights reserved.
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

//
// This is supposed to be the Posix version of our OS abstraction library.
// Unfortunately there are a number of functions which we must use, but which
// are not covered under the Posix specs.  Of course, since they are not
// defined, different systems will implement them differently.  This is
// especially apparent in when one looks at the BSD-derived network stack of the
// Darwin operating system.
//
// Unlike Linux, Darwin does not implement Netlink sockets, but does provide the
// functionality we need in different ways.  Additionally, many of the definitions
// are simply not provided in Darwin.  Rather than invent a new subcategory of
// capabilities within the Posiz OS_GROUP, we just "switch" on the particular
// specific OS.
//
// So, we expect the IfConfig functionality to be provided in a separate file
// (IfConfigDarwin.cc) using the completely different mechanisms of that OS under
// the inverse of the ifdef below.
//
#if !defined(QCC_OS_DARWIN)

#include <list>

#include <errno.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Socket.h>

#include <qcc/IfConfig.h>
#include <fcntl.h>

#define QCC_MODULE "IFCONFIG"

namespace qcc {

//
// Sidebar on general functionality and Netlink Sockets:
//
// We need to provide a way to get a list of interfaces on the system.  We need
// to be able to find interfaces irrespective of whether or not they are up.  We
// also need to be able to deal with multiple IP addresses assigned to
// interfaces, and also with IPv4 and IPv6 at the same time.
//
// There are a bewildering number of ways to get information about network
// devices in Linux.  Unfortunately, most of them only give us access to pieces
// of the information we need.  For example, the ioctl SIOCGIFCONF belongs to
// the IP layer and only returns information about IFF_UP interfaces that have
// assigned IP address.  Furthermore, it only returns information about IPv4
// addresses.  Since we need IPv6 addresses, and also interfaces that may not
// have a currently assigned IP address, this won't work.  The function
// getifaddrs() works on Linux, but isn't available on Android since the header
// is private and excludes information we need.  User-space tools are even
// different on different platforms, with ifconfig returning both IPv4 and IPv6
// addresses on Linux but only IPv4 addresses on Android.  Windows is from
// another planet.
//
// One command that pretty much works the same on Android and generic Linux
// is "ip ad sh" which gives us exactly what we want.  This is from the iproute2
// package which uses netlink sockets, which is the new super-whoopty-wow
// control plane for networks in Linux-based systems.  We use netlink sockets
// to get what we want in Linux and Android.  Of course, as you may expect,
// libnetlink is not available on Android so we get to use low-level netlink
// sockets.  This is a bit of a pain, but is manageable since we really need
// so little information.
//
// Netlink has become a rather generic way to transfer information from kernel
// space to user space in Linux.  Netlink is a datagram-oriented service using
// the BSD sockets interface.  There are a number of communication families
// associated with Netlink.  We are concerned with the NETLINK_ROUTE family
// here.  According to the NETLINK(7) manpage, the NETLINK_ROUTE family receives
// routing and link updates and may be used to modify the routing tables (both
// IPv4 and IPv6), IP addresses, link parameters, neighbor setups, queueing
// disciplines, traffic classes and packet classifiers (see rtnetlink(7)).
//
// Since our client is probably in opening separate sockets on each
// interface/address combination as they become available, we organize the
// output as a list of interface/address combinations instead of the more
// OS-like way of providing a list of interfaces each with an associated list of
// addresses.
//
// This file consists of a number of utility functions that are used to get at
// other OS-dependent C functions and is therefore actually a C program written
// in C++.  Because of this, the organization of the module is in the C idiom,
// with the lowest level functions appearing first in the file, leading toward
// the highest level functions in a bottom-up fashion.
//

static AddressFamily TranslateFamily(uint32_t family)
{
    if (family == AF_INET) {
        return QCC_AF_INET;
    }
    if (family == AF_INET6) {
        return QCC_AF_INET6;
    }
    return QCC_AF_UNSPEC;
}

//
// Since the whole point of this package is to provide an OS-independent way to
// describe the underlying resources of the system, we ned to provide a way to
// translate Unix/Linux-specific flag values into some abstract notation.  This
// is essentially a one-to-one operation in Linux since this OS contains the
// more complete version, but it will be quite different in Windows, for example.
//
static uint32_t TranslateFlags(uint32_t flags)
{
    uint32_t ourFlags = 0;
    if (flags & IFF_UP) {
        ourFlags |= IfConfigEntry::UP;
    }
    if (flags & IFF_BROADCAST) {
        ourFlags |= IfConfigEntry::BROADCAST;
    }
    if (flags & IFF_DEBUG) {
        ourFlags |= IfConfigEntry::DEBUG;
    }
    if (flags & IFF_LOOPBACK) {
        ourFlags |= IfConfigEntry::LOOPBACK;
    }
    if (flags & IFF_POINTOPOINT) {
        ourFlags |= IfConfigEntry::POINTOPOINT;
    }
    if (flags & IFF_RUNNING) {
        ourFlags |= IfConfigEntry::RUNNING;
    }
    if (flags & IFF_NOARP) {
        ourFlags |= IfConfigEntry::NOARP;
    }
    if (flags & IFF_PROMISC) {
        ourFlags |= IfConfigEntry::PROMISC;
    }
    if (flags & IFF_NOTRAILERS) {
        ourFlags |= IfConfigEntry::NOTRAILERS;
    }
    if (flags & IFF_ALLMULTI) {
        ourFlags |= IfConfigEntry::ALLMULTI;
    }
    if (flags & IFF_MASTER) {
        ourFlags |= IfConfigEntry::MASTER;
    }
    if (flags & IFF_SLAVE) {
        ourFlags |= IfConfigEntry::SLAVE;
    }
    if (flags & IFF_MULTICAST) {
        ourFlags |= IfConfigEntry::MULTICAST;
    }
    if (flags & IFF_PORTSEL) {
        ourFlags |= IfConfigEntry::PORTSEL;
    }
    if (flags & IFF_AUTOMEDIA) {
        ourFlags |= IfConfigEntry::AUTOMEDIA;
    }
    if (flags & IFF_DYNAMIC) {
        ourFlags |= IfConfigEntry::DYNAMIC;
    }
    return ourFlags;
}

//
// As mentioned in the Sidebar above, we use the NETLINK_ROUTE family to get at
// the information we need about interfaces, addresses and associated
// configuration information.
//
// This function creates a Netlink socket and initializes it to use the
// NETLINK_ROUTE family.  We also set the transmit and receive buffer sizes for
// the socket so we know exactly what data is sent when.
//
// The newly created Netlink socket is bound to to the AF_NETLINK address family
// and is ready for use when this function completes.  Since its use is purely
// internal, we work with the Unix/Linux sockets idioms and return an integer socket file
// descriptor on success or a -1 on error.
//
static int NetlinkRouteSocket(uint32_t bufsize)
{
    int sockFd;

    if ((sockFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
        QCC_LogError(ER_FAIL, ("NetlinkRouteSocket: Error obtaining socket: %s", strerror(errno)));
        return -1;
    }

    if (setsockopt(sockFd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) < 0) {
        QCC_LogError(ER_FAIL, ("NetlinkRouteSocket: Can't setsockopt SO_SNDBUF: %s", strerror(errno)));
        return -1;
    }

    if (setsockopt(sockFd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) < 0) {
        QCC_LogError(ER_FAIL, ("NetlinkRouteSocket: Can't setsockopt SO_RCVBUF: %s", strerror(errno)));
        return -1;
    }

    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = 0;

    if (bind(sockFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        QCC_LogError(ER_FAIL, ("NetlinkRouteSocket: Can't bind to NETLINK_ROUTE socket: %s", strerror(errno)));
        return -1;
    }

    return sockFd;
}

//
// Send a generic Netlink request datagram.
//
// Each datagram sent over netlink is assigned a monotonically increasing
// sequence number which is provided.  The <type> describes what kind of
// message we want to send.  Of particular interest in this module are
// the RTM_GETLINK and RTM_GETADDR messages.
//
// RTM_GETLINK is used to request link layer information about interfaces.  We
// are specifically interested in the interface name and MTU which are found
// using this message.
//
// RTM_GETADDR is used to request network layer information.  We are interested
// in the address family (IPv4 or IPv6), the CIDR network prefix length (cf.
// 192.168.0.0/16, where 16 is the prefix length), the interface flags (e.g.,
// IFF_UP, IFF_MULTICAST), the address scope and the interface index along with,
// of course, the IP address.
//
// We also take an address family parameter which conveys a callers interest in
// seeing IPv4 (AF_INET) or IPv6 (AF_INET6) information.
//
// We expect that a NetlinkSend will be followed by a NetlinkRecv to pull the
// requested information out.
//
static void NetlinkSend(int sockFd, int seq, int type, int family)
{
    //
    // Header with general form of address family-dependent message
    //
    struct {
        struct nlmsghdr nlh;
        struct rtgenmsg g;
    } request;

    memset(&request, 0, sizeof(request));
    request.nlh.nlmsg_len = sizeof(request);
    request.nlh.nlmsg_type = type;

    //
    // NLM_F_REQUEST ... Indicates a request
    // NLM_F_ROOT ...... Return the entire table, not just a single entry.
    // NLM_F_MATCH ..... Return all entries matching criteria.
    //
    request.nlh.nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
    request.nlh.nlmsg_pid = getpid();
    request.nlh.nlmsg_seq = seq;

    request.g.rtgen_family = family;

    send(sockFd, (void*)&request, sizeof(request), 0);
}

//
// For each NetlinkSend, which sends a Netlink sockets NETLINK_ROUTE message
// to the kernel, we must do a NetlinkRecv to get the requested information
// back.  This is going to do a number of BSD sockets recv() calls to pull
// out the requested information and place each received buffer into the
// client-provided buffer at the correct offset.
//
// A common problem with virtually all similar mechanisms is that there is no
// way to discover, prior to the actual call, how big of a buffer will be
// required to hold all of the requested information.  We could perform a
// "dummy" call, the sole purpose of which is to determine how much data will
// actually come back, and then follow that "dummy" call with a "real" call that
// actually stores the data in an ad-hoc allocated buffer.  Instead of taking
// this more complicated and resource-consuming approach, we take the typical
// approach and assume that the client will provide a buffer that is "big
// enough" for us to fit all of the data into.
//
static uint32_t NetlinkRecv(int sockFd, char* buffer, int buflen)
{
    uint32_t nBytes = 0;

    while (1) {
        int tmp = recv(sockFd, &buffer[nBytes], buflen - nBytes, 0);

        if (tmp < 0) {
            return nBytes;
        }

        if (tmp == 0) {
            return nBytes;
        }

        struct nlmsghdr* p = (struct nlmsghdr*)&buffer[nBytes];

        if (p->nlmsg_type == NLMSG_DONE) {
            break;
        }

        nBytes += tmp;
    }

    return nBytes;
}

//
// Netlink sockets has messages oriented according to the OSI layer
// corresponding to the requested information, so we provide corresponding
// classes, one for link layer information (name, MTU, flags, etc.) and one for
// network layer information (network prefix, IP address, etc.).  There is an
// interface index field present in both classes which allows us to combine
// (join in the relational database sense) the information into one unified
// object which is returned to the client.
//
// This is the class that we use to hold the link layer information about a
// particular network interface.
//
class IfEntry {
  public:
    uint32_t m_index;
    qcc::String m_name;
    uint32_t m_mtu;
    uint32_t m_flags;
};

//
// There are two fundamental pieces to the puzzle we want to solve.  We need
// to get a list of interfaces on the system and then we want to get a list
// of all of the addresses on those interfaces.
//
// We don't want to force our clients to think like an OS, so we are going go do
// a "join" of these two functions in the sense of a database join in order to
// put all of the information into a convenient form.  We can use interface
// index to do the "join."  This is the internal function that will provide the
// interface information.
//
// One of the fundamental reasons that we need to do this complicated work is so
// that we can provide a list of interfaces (links) on the system irrespective
// of whether or not they are up or down or what kind of address they may have
// assigned *IPv4 or IPv6).
//
// This function returns a std::list of the link layer information for all
// network interfaces on the system.
//
static std::list<IfEntry> NetlinkGetInterfaces(void)
{
    std::list<IfEntry> entries;

    //
    // Like many interfaces that do similar things, there's no clean way to
    // figure out beforehand how big of a buffer we are going to eventually
    // need.  Typically user code just picks buffers that are "big enough."
    // for example, iproute2 just picks a 16K buffer and fails if its not
    // "big enough."  Experimentally, we see that an 8K buffer holds 19
    // interfaces (because we have overflowed that for some devices).  Instead
    // of trying to dynamically resize the buffer as we go along, we do what
    // everyone else does and just allocate a  "big enough' buffer.  We choose
    // 64K which should handle about 150 interfaces.
    //
    const uint32_t BUFSIZE = 65536;
    char* buffer = new char[BUFSIZE];
    uint32_t len;

    //
    // If we can't get a NETLINK_ROUTE socket, treat it as a transient error
    // since we'll periodically retry looking for interfaces that come up and
    // go down.  The worst thing that will happen is we may not send
    // advertisements during the transient time; but this is guaranteed by
    // construction to be less than the advertisement timeout.
    //
    int sockFd = NetlinkRouteSocket(BUFSIZE);
    if (sockFd < 0) {
        delete[] buffer;
        return entries;
    }

    NetlinkSend(sockFd, 0, RTM_GETLINK, 0);
    len = NetlinkRecv(sockFd, buffer, BUFSIZE);

    for (struct nlmsghdr* nh = (struct nlmsghdr*)buffer; NLMSG_OK(nh, len); nh = NLMSG_NEXT(nh, len)) {
        switch (nh->nlmsg_type) {
        case NLMSG_DONE:
        case NLMSG_ERROR:
            break;

        case RTM_NEWLINK:
            {
                IfEntry entry;

                struct ifinfomsg* ifi = (struct ifinfomsg*)NLMSG_DATA(nh);
                entry.m_index = ifi->ifi_index;
                entry.m_flags = ifi->ifi_flags;

                struct rtattr* rta = IFLA_RTA(ifi);
                uint32_t rtalen = IFLA_PAYLOAD(nh);

                for (; RTA_OK(rta, rtalen); rta = RTA_NEXT(rta, rtalen)) {
                    switch (rta->rta_type) {
                    case IFLA_IFNAME:
                        entry.m_name = qcc::String((char*)RTA_DATA(rta));
                        break;

                    case IFLA_MTU:
                        entry.m_mtu = *(int*)RTA_DATA(rta);
                        break;
                    }
                }
                entries.push_back(entry);
                break;
            }
        }
    }

    delete [] buffer;
    qcc::Close(sockFd);

    return entries;
}

//
// Netlink sockets has messages oriented according to the OSI layer
// corresponding to the requested information, so we provide corresponding
// classes, one for link layer information (name, MTU, flags, etc.) and one for
// network layer information (network prefix, IP address, etc.).  There is an
// interface index field present in both classes which allows us to combine
// (join in the relational database sense) the information into one unified
// object which is returned to the client.
//
// This is the class that we use to hold the network layer information about a
// particular network interface.
//
class AddrEntry {
  public:
    uint32_t m_family;
    uint32_t m_prefixlen;
    uint32_t m_flags;
    uint32_t m_scope;
    uint32_t m_index;
    qcc::String m_addr;
};

//
// There are two fundamental pieces to the puzzle we want to solve.  We need
// to get a list of interfaces on the system and then we want to get a list
// of all of the addresses on those interfaces.
//
// We don't want to force our clients to think like an OS, so we are going go do
// a "join" of these two functions in the sense of a database join in order to
// put all of the information into a convenient form.  We can use interface
// index to do the "join."  This is the internal function that will provide the
// interface information.
//
// One of the fundamental reasons that we need to do this complicated work is so
// that we can provide a list of interfaces (links) on the system irrespective
// of whether or not they are up or down or what kind of address they may have
// assigned *IPv4 or IPv6).
//
// This function returns a std::list of the network layer information for all
// network interfaces on the system.  Since the specific use-case for what the
// client will do with the information may differ according to the address
// family found, we take the family (AF_INET for IPv4, AF_INET6 for IPv6) as
// a parameter.
//
// This function returns a std::list of the network layer information for all
// network interfaces on the system that have addresses for the given family.
//
static std::list<AddrEntry> NetlinkGetAddresses(uint32_t family)
{
    std::list<AddrEntry> entries;

    //
    // Like many interfaces that do similar things, there's no clean way to
    // figure out beforehand how big of a buffer we are going to eventually
    // need.  Typically user code just picks buffers that are "big enough."
    // for example, iproute2 just picks a 16K buffer and fails if its not
    // "big enough."  Experimentally, we see that an 8K buffer holds 19
    // interfaces (because we have overflowed that for some devices).  Instead
    // of trying to dynamically resize the buffer as we go along, we do what
    // everyone else does and just allocate a  "big enough' buffer.  We choose
    // 64K which should handle about 150 interfaces.
    //
    const uint32_t BUFSIZE = 65536;
    char* buffer = new char[BUFSIZE];
    uint32_t len;

    int sockFd = NetlinkRouteSocket(BUFSIZE);
    if (sockFd < 0) {
        delete[] buffer;
        return entries;
    }

    NetlinkSend(sockFd, 0, RTM_GETADDR, family);
    len = NetlinkRecv(sockFd, buffer, BUFSIZE);

    for (struct nlmsghdr* nh = (struct nlmsghdr*)buffer; NLMSG_OK(nh, len); nh = NLMSG_NEXT(nh, len)) {
        switch (nh->nlmsg_type) {
        case NLMSG_DONE:
        case NLMSG_ERROR:
            break;

        case RTM_NEWADDR:
            {
                AddrEntry entry;

                struct ifaddrmsg* ifa = (struct ifaddrmsg*)NLMSG_DATA(nh);

                entry.m_family = ifa->ifa_family;
                entry.m_prefixlen = ifa->ifa_prefixlen;
                entry.m_flags = ifa->ifa_flags;
                entry.m_scope = ifa->ifa_scope;
                entry.m_index = ifa->ifa_index;

                struct rtattr* rta = IFA_RTA(ifa);
                uint32_t rtalen = IFA_PAYLOAD(nh);

                for (; RTA_OK(rta, rtalen); rta = RTA_NEXT(rta, rtalen)) {
                    switch (rta->rta_type) {
                    case IFA_ADDRESS:
                        if (ifa->ifa_family == AF_INET) {
                            struct in_addr* p = (struct in_addr*)RTA_DATA(rta);
                            //
                            // Android seems to stash INADDR_ANY in as an
                            // IFA_ADDRESS in the case of an AF_INET address
                            // for some reason, so we ignore those.
                            //
                            if (p->s_addr != 0) {
                                char buffer[17];
                                inet_ntop(AF_INET, p, buffer, sizeof(buffer));
                                entry.m_addr = qcc::String(buffer);
                            }
                        }
                        if (ifa->ifa_family == AF_INET6) {
                            struct in6_addr* p = (struct in6_addr*)RTA_DATA(rta);
                            char buffer[41];
                            inet_ntop(AF_INET6, p, buffer, sizeof(buffer));
                            entry.m_addr = qcc::String(buffer);
                        }
                        break;

                    default:
                        break;
                    }
                }
                entries.push_back(entry);
            }
            break;
        }
    }

    delete [] buffer;
    qcc::Close(sockFd);

    return entries;
}

//
// This is the high-level function that provides the functionality similar to
// the Unix/linux ifconfig command.  If you've followed the bottom-up Netlink
// sockets trail from the beginning of the file, you know that Netlink sockets
// allows us to get link layer information and network layer information from
// the kernel using two different message types.  The network layer information
// can be further classified according to address family (IPv4 or IPv6).
// Note that Linux will default to AF_UNSPEC if the address family requested
// is not supported on the device. For details see net/core/rtnetlink.c.
//
// Thus, we need to make three Netlink sockets calls here:  One to get the list
// of network interfaces from the link layer, and two calls to get the address
// information from the network layer.
//
// Since our client doesn't care about our issues with having to make multiple
// OS-specific calls, we combine all of this information into a single
// std::vector of objects containing hopefully interesting information about
// each interface/address combination.  Most of the work done here is involved
// with doing the "join" of the three different categories into one unified
// interface/address object.
//
QStatus IfConfig(std::vector<IfConfigEntry>& entries)
{
    QCC_DbgPrintf(("IfConfig(): The Linux way"));

    std::list<IfEntry> ifEntries = NetlinkGetInterfaces();
    std::list<AddrEntry> entriesIpv4 = NetlinkGetAddresses(AF_INET);
    std::list<AddrEntry> entriesIpv6 = NetlinkGetAddresses(AF_INET6);

    for (std::list<IfEntry>::const_iterator i = ifEntries.begin(); i != ifEntries.end(); ++i) {
        uint32_t nAddresses = 0;

        //
        // Are there any IPV4 addresses assigned to the currently described interface
        //
        for (std::list<AddrEntry>::const_iterator j = entriesIpv4.begin(); j != entriesIpv4.end(); ++j) {
            if ((*j).m_index == (*i).m_index && (*j).m_family == AF_INET) {
                IfConfigEntry entry;
                entry.m_name = (*i).m_name.c_str();
                entry.m_flags = TranslateFlags((*i).m_flags);
                entry.m_mtu = (*i).m_mtu;
                entry.m_index = (*i).m_index;
                entry.m_addr = (*j).m_addr.c_str();
                entry.m_prefixlen = (*j).m_prefixlen;
                entry.m_family = TranslateFamily((*j).m_family);

                entries.push_back(entry);
                ++nAddresses;
            }
        }

        //
        // Are there any IPV6 addresses assigned to the currently described interface
        //
        for (std::list<AddrEntry>::const_iterator j = entriesIpv6.begin(); j != entriesIpv6.end(); ++j) {
            if ((*j).m_index == (*i).m_index && (*j).m_family == AF_INET6) {
                IfConfigEntry entry;
                entry.m_name = (*i).m_name.c_str();
                entry.m_flags = TranslateFlags((*i).m_flags);
                entry.m_mtu = (*i).m_mtu;
                entry.m_index = (*i).m_index;
                entry.m_addr = (*j).m_addr.c_str();
                entry.m_prefixlen = (*j).m_prefixlen;
                entry.m_family = TranslateFamily((*j).m_family);

                entries.push_back(entry);
                ++nAddresses;
            }
        }

        //
        // Even if there are no addresses on the interface instantaneously, we
        // need to return it since there may be an address on it in the future.
        // The flags should reflect the fact that there are no addresses
        // assigned.
        //
        if (nAddresses == 0) {
            IfConfigEntry entry;
            entry.m_name = (*i).m_name.c_str();
            entry.m_flags = (*i).m_flags;
            entry.m_mtu = (*i).m_mtu;
            entry.m_index = (*i).m_index;

            entry.m_addr = qcc::String();
            entry.m_family = QCC_AF_UNSPEC;

            entries.push_back(entry);
        }
    }

    return ER_OK;
}

/*
 * This is the high-level function that processes data received on
 * network events and checks if there are any events we are interested in.
 * We limit processing of events to a batch of up to 100 events at a time.
 */
static NetworkEventType NetworkEventRecv(qcc::SocketFd sockFd, char* buffer, int buflen, NetworkEventSet& networkEvents)
{
    uint32_t nBytes = 0;
    struct nlmsghdr* networkEvent =  reinterpret_cast<struct nlmsghdr*>(buffer);
    fd_set rdset;
    struct timeval tval;
    tval.tv_sec = 0;
    tval.tv_usec = 0;
    int32_t count = 0;

    NetworkEventType eventSummary = QCC_RTM_IGNORED;

    FD_ZERO(&rdset);
    FD_SET(sockFd, &rdset);
    do {
        NetworkEventType newEventType = QCC_RTM_IGNORED;
        nBytes = recv(sockFd, buffer, buflen, 0);
        if (sizeof(struct nlmsghdr) <= nBytes && networkEvent->nlmsg_len <= nBytes) {
            if (networkEvent->nlmsg_type == RTM_DELADDR) {
                newEventType = QCC_RTM_DELADDR;
            } else if (networkEvent->nlmsg_type == RTM_NEWADDR) {
                newEventType = QCC_RTM_NEWADDR;
                struct ifaddrmsg* ifa = (struct ifaddrmsg*)NLMSG_DATA(networkEvent);
                uint32_t indexFamily = 0;
                if (ifa->ifa_family == AF_INET) {
                    indexFamily |= QCC_AF_INET_INDEX;
                }
                if (ifa->ifa_family == AF_INET6) {
                    indexFamily |= QCC_AF_INET6_INDEX;
                }
                indexFamily |= (ifa->ifa_index << 2);
                networkEvents.insert(indexFamily);
            } else if (networkEvent->nlmsg_type == NLMSG_DONE) {
                break;
            } else {
                newEventType = QCC_RTM_IGNORED;
            }
            if (eventSummary < newEventType) {
                eventSummary = newEventType;
            }
        } else {
            QCC_LogError(ER_OK, ("NetworkEventRecv(): Error processing network event data"));
        }
    } while (count++ < 100 && select(sockFd + 1, &rdset, NULL, NULL, &tval) > 0);
    QCC_DbgPrintf(("NetworkEventRecv(): Processed %d event(s), %s", count, (eventSummary == QCC_RTM_IGNORED ? "none are relevant" : "some are relevant")));
    return eventSummary;
}

/*
 * This is the high-level function that creates a socket on which
 * to receive network event notifications.
 */
static SocketFd NetworkChangeEventSocket()
{
    int sockFd;
    struct sockaddr_nl addr;

    if ((sockFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
        QCC_LogError(ER_FAIL, ("NetworkChangeEventSocket(): Error obtaining socket: %s", strerror(errno)));
        return -1;
    }

    fcntl(sockFd, F_SETFL, O_NONBLOCK);
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV6_IFADDR | RTMGRP_IPV4_IFADDR;

    if (bind(sockFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        QCC_LogError(ER_FAIL, ("NetworkChangeEventSocket(): Error binding to NETLINK_ROUTE socket: %s", strerror(errno)));
        return -1;
    }

    return sockFd;
}

SocketFd NetworkEventSocket()
{
    return NetworkChangeEventSocket();
}

NetworkEventType NetworkEventReceive(qcc::SocketFd sockFd, NetworkEventSet& networkEvents)
{
    const uint32_t BUFSIZE = 65536;
    char* buffer = new char[BUFSIZE];

    NetworkEventType ret = NetworkEventRecv(sockFd, buffer, BUFSIZE, networkEvents);

    delete[] buffer;
    return ret;
}

} // namespace ajn

#endif // !defined(QCC_OS_DARWIN)
