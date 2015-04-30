/**
 * @file
 * A mechanism to get network interface configurations a la Unix/Linux ifconfig.
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
#include <mstcpip.h>

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
// Since our client might be opening separate sockets on each
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
// of whether or not they are up or down or what kind of addresses they may have
// assigned *IPv4 or IPv6).
//
// Linux systems only provide link-layer and network-layer information separately,
// but Windows is more friendly to us in that it provides most of what we want in
// one place.
//
// In order to keep the general behavior consistent with the POSIX implementation
// we group returned interface/address combinations sorted by address family.  Thus
// we provide this function which returns entries of a given IP address family
// (AF_INET or AF_INET6).  Unfortunately this means we lose information about
// which address family is preferred.
//
void IfConfigByFamily(uint32_t family, std::vector<IfConfigEntry>& entries)
{
    QCC_DbgPrintf(("IfConfigByFamily()"));

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
    // Now, get the interesting information about the adapters with addresses.
    //
    ULONG error = GetAdaptersAddresses(family,
                                       GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
                                       0, pinfo, &infoLen);
    if (error != NO_ERROR) {
        QCC_LogError(ER_OS_ERROR, ("IfConfigByFamily(): GetAdaptersAddresses error %u", error));
    } else {
        //
        // Windows provides its equivalent of IFF_MULTICAST in the
        // IP_ADAPTER_ADDRESSES structure, but it provides its version
        // of IFF_BROADCAST in the INTERFACE_INFO structure.  You get
        // INTERFACE_INFO from an IOCTL applied to a socket, not a simple
        // library call.
        //
        const int broadcastInfoLen = 150 * sizeof(INTERFACE_INFO);
        INTERFACE_INFO* pinterfaces = nullptr;
        uint32_t nInterfaces = 0;

        if (family == AF_INET) {
            qcc::SocketFd socketFd;

            //
            // Like many interfaces that do similar things, there's no
            // clean way to figure out beforehand how big of a buffer
            // SIO_GET_INTERFACE_LIST needs.  Typically user code just
            // picks buffers that are "big enough."  On the Linux side
            // of things, we run into a similar situation.  There we
            // chose a buffer that could handle about 150 interfaces, so
            // we just do the same thing here.  Hopefully 150 will be
            // "big enough" and an INTERFACE_INFO is not that big since
            // it holds a long flags and three sockaddr_gen structures
            // (two shorts, two longs and sixteen btyes).  We're then
            // looking at dynamically allocating 13,200 bytes which
            // doesn't seem too terribly outrageous.
            //
            pinterfaces = reinterpret_cast<INTERFACE_INFO*>(new uint8_t[broadcastInfoLen]);

            QStatus status = qcc::Socket(qcc::QCC_AF_INET, qcc::QCC_SOCK_DGRAM, socketFd);

            if (status != ER_OK) {
                QCC_LogError(status, ("IfConfigByFamily: Socket(QCC_AF_INET) failed"));
            } else {
                uint32_t nBytes;

                //
                // Make the WinSock call to get the address information about
                // the various interfaces in the system.  If the ioctl fails
                // then we don't enable broadcast.
                //
                if (WSAIoctl(socketFd, SIO_GET_INTERFACE_LIST, 0, 0, pinterfaces,
                             broadcastInfoLen, (LPDWORD)&nBytes, 0, 0) == SOCKET_ERROR) {
                    QCC_LogError(ER_OS_ERROR, ("IfConfigByFamily: WSAIoctl(SIO_GET_INTERFACE_LIST) failed"));
                } else {
                    nInterfaces = nBytes / sizeof(INTERFACE_INFO);
                }
                Close(socketFd);
            }
        }

        //
        // pinfo is a linked list of adapter information records
        //
        for (; pinfo; pinfo = pinfo->Next) {
            //
            // Get the adapter name.
            //
            CHAR ifName[IF_NAMESIZE];
            if (!if_indextoname(pinfo->IfIndex, ifName)) {
                QCC_LogError(ER_OS_ERROR, ("IfConfigByFamily(): if_indextoname failed"));
                continue;
            }

            //
            // Since there can be multiple IP addresses associated with an
            // adapter, we have to loop through them as well.  Each IP address
            // corresponds to a name service IfConfigEntry.
            //
            for (IP_ADAPTER_UNICAST_ADDRESS* paddr = pinfo->FirstUnicastAddress; paddr; paddr = paddr->Next) {
                IfConfigEntry entry;

                entry.m_name = qcc::String(ifName);

                //
                // AllJoyn used to use the AdapterName which is much harder for humans to use, but
                // we need to provide backwards compatibility so we still want to support it too.
                //
                entry.m_altname = qcc::String(pinfo->AdapterName);

                //
                // Fill in the rest of the entry, translating the Windows constants into our
                // more platform-independent constants.
                //
                entry.m_flags = pinfo->OperStatus == IfOperStatusUp ? IfConfigEntry::UP : 0;
                entry.m_flags |= pinfo->Flags & IP_ADAPTER_NO_MULTICAST ? 0 : IfConfigEntry::MULTICAST;
                entry.m_flags |= pinfo->IfType == IF_TYPE_SOFTWARE_LOOPBACK ? IfConfigEntry::LOOPBACK : 0;
                entry.m_family = TranslateFamily(family);
                entry.m_mtu = pinfo->Mtu;
                entry.m_index = pinfo->IfIndex;

                //
                // Get the IP address in presentation form.
                //
                char buffer[NI_MAXHOST];
                memset(buffer, 0, NI_MAXHOST);

                int result = getnameinfo(paddr->Address.lpSockaddr, paddr->Address.iSockaddrLength,
                                         buffer, sizeof(buffer), NULL, 0, NI_NUMERICHOST);
                if (result != 0) {
                    QCC_LogError(ER_OS_ERROR, ("IfConfigByFamily(): getnameinfo error %d", result));
                }

                //
                // Other AllJoyn code currently can't deal with the IPv6 scope id (after a
                // percent character) at the end of the IPv6 address, so until
                // the rest of AllJoyn is fixed, we chop it off if it is there.
                //
                char* p = strchr(buffer, '%');
                if (p) {
                    *p = '\0';
                }

                entry.m_addr = buffer;
                entry.m_prefixlen = paddr->OnLinkPrefixLength;

                if (family == AF_INET) {
                    //
                    // Walk the array of interface address information
                    // looking for the same address as
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
                    for (uint32_t i = 0; i < nInterfaces; ++i) {
                        if (memcmp(&pinterfaces[i].iiAddress.AddressIn,
                                   paddr->Address.lpSockaddr,
                                   sizeof(SOCKADDR_IN)) == 0) {
                            entry.m_flags |= pinterfaces[i].iiFlags & IFF_BROADCAST ? IfConfigEntry::BROADCAST : 0;
                            break;
                        }
                    }
                }
                entries.push_back(entry);
            }
        }

        delete [] pinterfaces;
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
