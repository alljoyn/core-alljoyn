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
// (IfConfigLinux.cc) using the completely different mechanisms of that OS under
// the inverse of the ifdef below.
//
#if defined(QCC_OS_DARWIN)

#include <list>
#include <ifaddrs.h>

#include <errno.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <qcc/Debug.h>
#include <qcc/String.h>

#include <qcc/IfConfig.h>

#include <fcntl.h>
#include <net/if.h>

#ifndef RTM_NEWADDR
#define RTM_NEWADDR 0xc
#endif

#ifndef RTM_DELADDR
#define RTM_DELADDR 0xd
#endif

#define QCC_MODULE "IFCONFIG"

namespace qcc {

//
// Sidebar on general functionality.
//
// We need to provide a way to get a list of interfaces on the system.  We need
// to be able to find interfaces irrespective of whether or not they are up.  We
// also need to be able to deal with multiple IP addresses assigned to
// interfaces, and also with IPv4 and IPv6 at the same time.
//
// There are a bewildering number of ways to get information about network
// devices in Posix.  Unfortunately, most of them only give us access to pieces
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
// translate Darwin-specific flag values into some abstract representation.
//
// Note that Darwin excludes some unused or rarely used definitions as compared
// to Linux; but we'll include the outliers if they happen to be defined on a
// given distribution.
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
    if (flags & IFF_MULTICAST) {
        ourFlags |= IfConfigEntry::MULTICAST;
    }

#if defined(IFF_MASTER)
    if (flags & IFF_MASTER) {
        ourFlags |= IfConfigEntry::MASTER;
    }
#endif

#if defined(IFF_SLAVE)
    if (flags & IFF_SLAVE) {
        ourFlags |= IfConfigEntry::SLAVE;
    }
#endif

#if defined(IFF_PORTSEL)
    if (flags & IFF_PORTSEL) {
        ourFlags |= IfConfigEntry::PORTSEL;
    }
#endif

#if defined(IFF_AUTOMEDIA)
    if (flags & IFF_AUTOMEDIA) {
        ourFlags |= IfConfigEntry::AUTOMEDIA;
    }
#endif

#if defined(IFF_DYNAMIC)
    if (flags & IFF_DYNAMIC) {
        ourFlags |= IfConfigEntry::DYNAMIC;
    }
#endif

    return ourFlags;
}

//
// This is the high-level function that provides the functionality similar to
// the Unix/linux ifconfig command.
//
// When compared to the Linux and Windows hoops we have to jump through, the
// BSD-derived functions of Darwin are refreshingly simple and allow us to
// fairly easily get what we need in one place.
//
QStatus IfConfig(std::vector<IfConfigEntry>& entries)
{
    QCC_DbgPrintf(("IfConfig(): The Darwin way"));

    //
    // We need a socket for the ioctl used to get the MTU of the interface.
    //
    int sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0) {
        QCC_LogError(ER_OS_ERROR, ("Ifconfig(): Error opening socket: %s", strerror(errno)));
        return ER_OS_ERROR;
    }

    //
    // BSD-derived systems have getifaddrs which has been extended to provide
    // IPv6-related information.  This puts all of the stuff we need in one
    // place, with everything that comes back in a linked list so we don't
    // have to worry about buffer sizing.
    //
    struct ifaddrs* iflist = NULL;
    if (getifaddrs(&iflist) < 0) {
        QCC_LogError(ER_OS_ERROR, ("Ifconfig(): getifaddrs() failed: %s", strerror(errno)));
        close(sockFd);
        return ER_OS_ERROR;
    }

    //
    // Walk the list of interface addresses.  Note that the resulting list of
    // addresses will not ordereed according to address family (IPv4 or IPv6)
    // like the other (Linux, Windows) versions of this function.
    //
    for (struct ifaddrs* if_addr = iflist; if_addr != NULL; if_addr = if_addr->ifa_next) {
        //
        // Build a new IfConfigEntry for each entry in the ifaddrs list.
        //
        IfConfigEntry entry;

        //
        // Add the interface name (e.g., "eth0" or "wlan0"
        //
        entry.m_name = qcc::String(if_addr->ifa_name);

        //
        // Pick out the address family (AF_INET, AF_INET6).  We assume that if
        // no IP address is actually assigned the family will be AF_UNSPEC.
        //
        entry.m_family = TranslateFamily(if_addr->ifa_addr->sa_family);

        //
        // Translate the interface flags (e.g., IFF_UP, IFF_MULTICAST) into an
        // abstract representation.
        //
        entry.m_flags = TranslateFlags(if_addr->ifa_flags);

        //
        // Provide the interface index to the client since some higher level
        // calls, especially IPv6 socket options will need it.
        //
        entry.m_index = if_nametoindex(if_addr->ifa_name);

        //
        // There may or may not be an address returned in the structure.  If the
        // current interface does not have an address assigned, for example if
        // DHCP loses its lease, the ifa_addr for the interface may be NULL.  We
        // need to admit that possibility.  In this case, we want to provide the
        // interface back to the client, so it is not an error per se.
        //
        // Convert the discovered IP address into its appropriate presentation
        // format if we have one.
        //
        if (if_addr->ifa_addr) {
            char buf[INET6_ADDRSTRLEN];
            buf[0] = '\0';
            char const* pBuf = NULL;
            if (if_addr->ifa_addr->sa_family == AF_INET) {
                struct in_addr* p =  &((struct sockaddr_in*)if_addr->ifa_addr)->sin_addr;
                pBuf = inet_ntop(AF_INET, p, buf, sizeof(buf));
            } else if (if_addr->ifa_addr->sa_family == AF_INET6) {
                struct in6_addr* p =  &((struct sockaddr_in6*)if_addr->ifa_addr)->sin6_addr;
                pBuf = inet_ntop(AF_INET6, p, buf, sizeof(buf));
            }

            //
            // If the address conversion works, inet_ntop will return a pointer to
            // the buffer it filled.  If the conversion fails, we just treat the
            // situation as if the address doesn't exist.
            //
            if (pBuf == buf) {
                entry.m_addr = qcc::String(buf);
            }
        }

        //
        // BSD is an old-timer, so it doesn't provide a CIDR prefixlen directly
        // but gives us an old-fashioned netmask.  We have to convert this
        // netmask to a prefix length here.
        //
        // Just as in the case of the IP address, there may or may not be a
        // netmask provided, so we have to deal with that possibility.  If there
        // is no netmask, we cannot find the prefixlen and we need to set it to
        // some obviously wrong (error) value.
        //
        if (if_addr->ifa_netmask) {
            uint32_t prefixlen = 0;
            if (if_addr->ifa_netmask->sa_family == AF_INET) {
                uint32_t mask = ntohl(((struct sockaddr_in*)(if_addr->ifa_netmask))->sin_addr.s_addr);
                while (mask & 0x80000000) {
                    ++prefixlen;
                    mask <<= 1;
                }
            } else if (if_addr->ifa_netmask->sa_family == AF_INET6) {
                for (uint32_t i = 0; i < 16; ++i) {
                    uint8_t mask = ((struct sockaddr_in6*)(if_addr->ifa_netmask))->sin6_addr.s6_addr[i];
                    while (mask & 0x80) {
                        ++prefixlen;
                        mask <<= 1;
                    }
                }
            }
            entry.m_prefixlen = prefixlen;
        } else {
            entry.m_prefixlen = static_cast<uint32_t>(-1);
        }

        //
        // The MTU is the only tidbit we need that is not conveniently found in
        // the ifaddr structure.  We use the usual ioctl to get at it.
        //
        struct ifreq if_item;
        memset(&if_item, 0, sizeof(if_item));

        //
        // The ioctl needs the interface name, so copy it in, being careful to
        // zero terminate the string.
        //
        strncpy(if_item.ifr_name, if_addr->ifa_name, IFNAMSIZ);
        if_item.ifr_name[IFNAMSIZ - 1] = '\0';

        //
        // Make the ioctl.  If we get an error, we bail on the entry, but
        // continue to try and get as much info out as we can.
        //
        if (ioctl(sockFd, SIOCGIFMTU, &if_item) < 0) {
            QCC_LogError(ER_OS_ERROR, ("Ifconfig(): ioctl() failed: %s", strerror(errno)));
            continue;
        }
        entry.m_mtu = if_item.ifr_mtu;

        //
        // Add the completed interface entry to the list of entries we're
        // giving back to the user.
        //
        entries.push_back(entry);
    }

    close(sockFd);
    freeifaddrs(iflist);

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
    struct ifa_msghdr* networkEvent =  reinterpret_cast<struct ifa_msghdr*>(buffer);
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
        if (sizeof(struct ifa_msghdr) <= nBytes) {
            if (networkEvent->ifam_type == RTM_DELADDR) {
                newEventType = QCC_RTM_DELADDR;
            } else if (networkEvent->ifam_type == RTM_NEWADDR) {
                newEventType = QCC_RTM_NEWADDR;
                uint32_t indexFamily = 0;
                indexFamily |= (networkEvent->ifam_index << 2);
                networkEvents.insert(indexFamily);
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

    if ((sockFd = socket(AF_ROUTE, SOCK_RAW, 0)) < 0) {
        QCC_LogError(ER_FAIL, ("NetworkChangeEventSocket(): Error obtaining socket: %s", strerror(errno)));
        return -1;
    }

    fcntl(sockFd, F_SETFL, O_NONBLOCK);

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

#endif // defined(QCC_OS_DARWIN)
