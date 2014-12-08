/**
 * @file
 * A mechanism to get network interface configurations a la Unix/Linux ifconfig.
 */

/******************************************************************************
 * Copyright (c) 2010-2012,2014, AllSeen Alliance. All rights reserved.
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

#include <list>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Socket.h>
#include <qcc/windows/utility.h>

#include <qcc/IfConfig.h>

#define QCC_MODULE "IFCONFIG"

namespace qcc {

//
// Sidebar on general functionality:
//
// We need to provide a way to get a list of interfaces on the system.  We need
// to be able to find interfaces irrespective of whether or not they are up.  We
// also need to be able to deal with multiple IP addresses assigned to
// interfaces, and also with IPv4 and IPv6 at the same time.
//
// There are a bewildering number of ways to get information about network
// devices in Windows.  Unfortunately, most of them only give us access to pieces
// of the information we need; and different versions of Windows keep info in
// different places.  That makes this code somewhat surprisingly complicated.
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
// Linux systems specifically arrange for separating out link layer and network
// layer information, but Windows does not.  Windows is more friendly to us in
// that it provides most of what we want in one place.  The problem is "most."
// We have to jump through some hoops because various versions of Windows return
// different amounts of what we need.
//
// In order to keep the general behavior consistent with the Posix implementation
// we group returned interface/address combinations sorted by address family.  Thus
// we provide this function which returns entries of a given IP address family
// (AF_INET or AF_INET6).
//
void IfConfigByFamily(uint32_t family, std::vector<IfConfigEntry>& entries)
{
    QCC_DbgPrintf(("IfConfigByFamily()"));

    //
    // Windows is from another planet, but we can't blame multiple calls to get
    // interface info on them.  It's a legacy of *nix sockets since BSD 4.2
    //
    IP_ADAPTER_ADDRESSES info, * parray = 0, * pinfo = 0;
    ULONG infoLen = sizeof(info);

    //
    // Call into Windows and it will tell us how much memory it needs, if
    // more than we provide.
    //
    GetAdaptersAddresses(family,
                         GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
                         0, &info, &infoLen);

    //
    // Allocate enough memory to hold the adapter information array.
    //
    parray = pinfo = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(new uint8_t[infoLen]);

    //
    // Now, get the interesting information about the net devices with IPv4 addresses
    //
    if (GetAdaptersAddresses(family,
                             GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
                             0, pinfo, &infoLen) == NO_ERROR) {

        //
        // pinfo is a linked list of adapter information records
        //
        for (; pinfo; pinfo = pinfo->Next) {

            //
            // Since there can be multiple IP addresses associated with an
            // adapter, we have to loop through them as well.  Each IP address
            // corresponds to a name service IfConfigEntry.
            //
            for (IP_ADAPTER_UNICAST_ADDRESS* paddr = pinfo->FirstUnicastAddress; paddr; paddr = paddr->Next) {

                IfConfigEntry entry;

                //
                // Get the adapter name.  This will be a GUID, but the
                // friendly name which would look something like
                // "Wireless Network Connection 3" in an English
                // localization can also be Unicode Chinese characters
                // which AllJoyn may not deal with well.  We choose
                // the better part of valor and just go with the GUID.
                //
                entry.m_name = qcc::String(pinfo->AdapterName);

                //
                // Fill in the rest of the entry, translating the Windows constants into our
                // more platform-independent constants.  Note that we just assume that a
                // Windows interface supports broadcast.  We should really
                //
                //
                entry.m_flags = pinfo->OperStatus == IfOperStatusUp ? IfConfigEntry::UP : 0;
                entry.m_flags |= pinfo->Flags & IP_ADAPTER_NO_MULTICAST ? 0 : IfConfigEntry::MULTICAST;
                entry.m_flags |= pinfo->IfType == IF_TYPE_SOFTWARE_LOOPBACK ? IfConfigEntry::LOOPBACK : 0;
                entry.m_family = TranslateFamily(family);
                entry.m_mtu = pinfo->Mtu;

                if (family == AF_INET) {
                    entry.m_index = pinfo->IfIndex;
                } else {
                    entry.m_index = pinfo->Ipv6IfIndex;
                }

                //
                // Get the IP address in presentation form.
                //
                char buffer[NI_MAXHOST];
                memset(buffer, 0, NI_MAXHOST);

                int result = getnameinfo(paddr->Address.lpSockaddr, paddr->Address.iSockaddrLength,
                                         buffer, sizeof(buffer), NULL, 0, NI_NUMERICHOST);
                if (result != 0) {
                    QCC_LogError(ER_FAIL, ("IfConfigByFamily(): getnameinfo error %d", result));
                }

                //
                // Windows appends the IPv6 link scope (after a
                // percent character) to the end of the IPv6 address
                // which we can't deal with.  We chop it off if it
                // is there.
                //
                char* p = strchr(buffer, '%');
                if (p) {
                    *p = '\0';
                }

                entry.m_addr = buffer;

                //
                // There are a couple of really annoying things we have to deal
                // with here.  First, Windows provides its equivalent of
                // IFF_MULTICAST in the IP_ADAPTER_ADDRESSES structure, but it
                // provides its version of IFF_BROADCAST in the INTERFACE_INFO
                // structure.  It also turns out that Windows XP doesn't provide
                // the OnLinkPrefixLength information we need to construct a
                // subnet directed broadcast in the IP_ADAPTER_UNICAST_ADDRESS
                // structure, but later versions of Windows do.  For another
                // treat, in XP the network prefix is stored as a network mask
                // in the INTERFACE_INFO, but in later versions it is also
                // stored as a prefix in the IP_ADAPTER_UNICAST_ADDRESS
                // structure.  You get INTERFACE_INFO from an ioctl applied to a
                // socket, not a simple library call.
                //
                // So, like we have to do complicated things using netlink to
                // get everything we need in Linux and Android, we have to do
                // complicated things in Windows in different ways to get what
                // we want.
                //
                if (family == AF_INET) {
                    //
                    // Get a socket through qcc::Socket to keep its reference
                    // counting squared away.
                    //
                    qcc::SocketFd socketFd;

                    QStatus status = qcc::Socket(qcc::QCC_AF_INET, qcc::QCC_SOCK_DGRAM, socketFd);

                    if (status == ER_OK) {
                        //
                        // Like many interfaces that do similar things, there's no
                        // clean way to figure out beforehand how big of a buffer we
                        // are going to eventually need.  Typically user code just
                        // picks buffers that are "big enough."  On the Linux side
                        // of things, we run into a similar situation.  There we
                        // chose a buffer that could handle about 150 interfaces, so
                        // we just do the same thing here.  Hopefully 150 will be
                        // "big enough" and an INTERFACE_INFO is not that big since
                        // it holds a long flags and three sockaddr_gen structures
                        // (two shorts, two longs and sixteen btyes).  We're then
                        // looking at allocating 13,200 bytes on the stack which
                        // doesn't see too terribly outrageous.
                        //
                        INTERFACE_INFO interfaces[150];
                        uint32_t nBytes;

                        //
                        // Make the WinSock call to get the address information about
                        // the various interfaces in the system.  If the ioctl fails
                        // then we set the prefix length to some absurd value and
                        // don't enable broadcast.
                        //
                        if (WSAIoctl(socketFd, SIO_GET_INTERFACE_LIST, 0, 0, &interfaces,
                                     sizeof(interfaces), (LPDWORD)&nBytes, 0, 0) == SOCKET_ERROR) {
                            QCC_LogError(status, ("IfConfigByFamily: WSAIoctl(SIO_GET_INTERFACE_LIST) failed: affects %s",
                                                  entry.m_name.c_str()));
                            entry.m_prefixlen = static_cast<uint32_t>(-1);
                        } else {
                            //
                            // Walk the array of interface address information
                            // looking for one with the same address as the adapter
                            // we are currently inspecting.  It is conceivable that
                            // we might see a system presenting us with multiple
                            // adapters with the same IP address but different
                            // netmasks, but that will confuse more modules than us.
                            // For example, someone might have multiple wireless
                            // interfaces connected to multiple access points which
                            // dole out the same DHCP address with different network
                            // parts.  This is expected to be extraordinarily rare,
                            // but if it happens, we'll just form an incorrect
                            // broadcast address.  This is minor in the grand scheme
                            // of things.
                            //
                            uint32_t nInterfaces = nBytes / sizeof(INTERFACE_INFO);
                            for (uint32_t i = 0; i < nInterfaces; ++i) {
                                struct in_addr* addr = &interfaces[i].iiAddress.AddressIn.sin_addr;
                                //
                                // XP doesn't have inet_ntop, so we fall back to inet_ntoa
                                //
                                char* buffer = inet_ntoa(*addr);

                                if (entry.m_addr == qcc::String(buffer)) {
                                    //
                                    // This is the address we want modulo the corner
                                    // case discussed above.  Grown-up systems
                                    // recognize that CIDR is the way to go and give
                                    // us a prefix length, but XP is going to give
                                    // us a netmask.  We have to convert the mask to
                                    // a prefix since we consider ourselves all
                                    // grown-up.
                                    //
                                    // So get the 32-bits of netmask returned by
                                    // Windows (remembering endianness issues) and
                                    // convert it to a prefix length.
                                    //
                                    uint32_t mask = ntohl(interfaces[i].iiNetmask.AddressIn.sin_addr.s_addr);

                                    uint32_t prefixlen = 0;
                                    while (mask & 0x80000000) {
                                        ++prefixlen;
                                        mask <<= 1;
                                    }
                                    entry.m_prefixlen = prefixlen;
                                    entry.m_flags |= interfaces[i].iiFlags & IFF_BROADCAST ? IfConfigEntry::BROADCAST : 0;
                                    break;
                                }
                            }
                        }
                        Close(socketFd);
                    } else {
                        QCC_LogError(status, ("IfConfigByFamily: Socket(QCC_AF_INET) failed: affects %s", entry.m_name.c_str()));
                        entry.m_prefixlen = static_cast<uint32_t>(-1);
                    }
                } else if (family == AF_INET6) {
                    //
                    // Get a socket through qcc::Socket to keep its reference
                    // counting squared away.
                    //
                    qcc::SocketFd socketFd;
                    QStatus status = qcc::Socket(qcc::QCC_AF_INET6, qcc::QCC_SOCK_DGRAM, socketFd);

                    if (status == ER_OK) {
                        //
                        // Like many interfaces that do similar things, there's no
                        // clean way to figure out beforehand how big of a buffer we
                        // are going to eventually need.  Typically user code just
                        // picks buffers that are "big enough."  On the Linux side
                        // of things, we run into a similar situation.  There we
                        // chose a buffer that could handle about 150 interfaces, so
                        // we just do the same thing here.  Hopefully 150 will be
                        // "big enough" and an INTERFACE_INFO is not that big since
                        // it holds a long flags and three sockaddr_gen structures
                        // (two shorts, two longs and sixteen btyes).  We're then
                        // looking at allocating 13,200 bytes
                        //

                        // initialize the prefix to an invalid value in case a matching address is not found
                        entry.m_prefixlen = static_cast<uint32_t>(-1);

                        DWORD nBytes;
                        uint64_t bytes = sizeof(INT) + (sizeof(SOCKET_ADDRESS) * 150);
                        char* addressBuffer = new char[bytes];
                        //
                        // Make the WinSock call to get the address information about
                        // the various addresses that can be bound to this socket.
                        // If the ioctl fails then we set the prefix length to some absurd value
                        //

                        if (WSAIoctl(socketFd, SIO_ADDRESS_LIST_QUERY, NULL, 0, addressBuffer,
                                     bytes, &nBytes, 0, 0) == SOCKET_ERROR) {
                            QCC_LogError(status, ("IfConfigByFamily: WSAIoctl(SIO_GET_INTERFACE_LIST) failed: affects %s; %d",
                                                  entry.m_name.c_str(), WSAGetLastError()));
                        } else {
                            LPSOCKET_ADDRESS_LIST addresses = reinterpret_cast<LPSOCKET_ADDRESS_LIST>(addressBuffer);
                            for (int32_t i = 0; i < addresses->iAddressCount; ++i) {
                                const SOCKET_ADDRESS* address = &addresses->Address[i];

                                if (address->lpSockaddr->sa_family == AF_INET6) {
                                    char addr_str[INET6_ADDRSTRLEN];
                                    DWORD size = sizeof(addr_str);
                                    if (WSAAddressToStringA(address->lpSockaddr, address->iSockaddrLength, NULL, addr_str, &size) != 0) {
                                        QCC_LogError(status, ("IfConfigByFamily: WSAAddressToStringA() failed: %d", WSAGetLastError()));
                                    } else {
                                        // split the string into the ip and prefix
                                        char* percent = strchr(addr_str, '%');

                                        if (percent != NULL) {
                                            *percent = '\0';

                                            if (entry.m_addr == addr_str) {
                                                entry.m_prefixlen = strtoul(percent + 1, NULL, 10);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        delete [] addressBuffer;
                        Close(socketFd);
                    } else {
                        QCC_LogError(status, ("IfConfigByFamily: Socket(QCC_AF_INET6) failed: affects %s", entry.m_name.c_str()));
                        entry.m_prefixlen = static_cast<uint32_t>(-1);
                    }
                } else {
                    // this should never happen
                    entry.m_prefixlen = static_cast<uint32_t>(-1);
                }
                entries.push_back(entry);
            }
        }
    }

    delete [] parray;
}

QStatus IfConfig(std::vector<IfConfigEntry>& entries)
{
    QCC_DbgPrintf(("IfConfig(): The Windows way"));

    //
    // It turns out that there are calls to functions that depend on winsock
    // made here.  We need to make sure that winsock is initialized before
    // making those calls.  Socket.cc has a convenient function to do this.
    //
    WinsockCheck();
    IfConfigByFamily(AF_INET, entries);
    IfConfigByFamily(AF_INET6, entries);
    return ER_OK;
}

} // namespace qcc
