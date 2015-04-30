/**
 * @file
 * Data structure and declaration of function for getting network interface configurations
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

#ifndef _IFCONFIG_H
#define _IFCONFIG_H

#ifndef __cplusplus
#error Only include IfConfig.h in C++ code.
#endif

#include <set>
#include <vector>
#include <Status.h>
#include <qcc/String.h>
#include <qcc/SocketTypes.h>

namespace qcc {

/* An entry for a usable IP address.  Note that there can be multiple IP addresses on the same interface,
 * so the same interface can appear in multiple IfConfigEntry instances.
 */
class IfConfigEntry {
  public:
    qcc::String m_name;     /**< The operating system-assigned name of the interface (e.g., "eth0" or "wlan0"). */
    qcc::String m_altname;  /**< An operating system-assigned alias for the interface (e.g., GUID on Windows). */
    qcc::String m_addr;     /**< A string representation of an IP address on the interface. */
    uint32_t m_prefixlen;   /**< The network prefix length, in the sense of CIDR, for the IP address. */
    AddressFamily m_family; /**< The address family of the IP address (AF_UNSPEC, AF_INET or AF_INET6). */

    static const uint32_t UP = 1;               /**< The interface is running and routes are in place. */
    static const uint32_t BROADCAST = 2;        /**< The interface has a valid broadcast address (can broadcast). */
    static const uint32_t DEBUG = 4;            /**< The underlying interface is in debug mode. */
    static const uint32_t LOOPBACK = 8;         /**< This is a loopback interface. */
    static const uint32_t POINTOPOINT = 16;     /**< This interface runs over a point to point link. */
    static const uint32_t RUNNING = 32;         /**< The hardware is running and can send and receive packets. */
    static const uint32_t NOARP = 64;           /**< There is no Address Resolution Protocol required or running. */
    static const uint32_t PROMISC = 128;        /**< The underlying device is in promiscuous mode. */
    static const uint32_t NOTRAILERS = 256;     /**< Avoid the use of trailers in BSD. */
    static const uint32_t ALLMULTI = 512;       /**< Receive all multicast packets.  Useful for multicast routing. */
    static const uint32_t MASTER = 1024;        /**< Load equalization code flag. */
    static const uint32_t SLAVE = 2048;         /**< Load equalization code flag. */
    static const uint32_t MULTICAST = 4096;     /**< The interface as capable of multicast transmission. */
    static const uint32_t PORTSEL = 8192;       /**< Marks the interface as capable of switching between media types. */
    static const uint32_t AUTOMEDIA = 16384;    /**< The interface is capable of automatically choosing media type. */
    static const uint32_t DYNAMIC = 32768;      /**< This interface has an IP address that can change (currently unused). */

    uint32_t m_flags;   /**< The combined interface flags for the interface. */
    uint32_t m_mtu;     /**< The maximum transmission unit (MTU) for the interface. */
    uint32_t m_index;   /**< The operating system generated interface index for the interface. */
};

typedef enum {
    QCC_RTM_IGNORED = -1,
    QCC_RTM_DELADDR = 0,
    QCC_RTM_NEWADDR = 1,
    QCC_RTM_SUSPEND = 2
} NetworkEventType;

const uint32_t QCC_AF_UNSPEC_INDEX = 0x0;
const uint32_t QCC_AF_INET_INDEX = 0x1;
const uint32_t QCC_AF_INET6_INDEX = 0x2;

typedef uint32_t NetworkEvent;
typedef std::set<NetworkEvent> NetworkEventSet;
#define NETWORK_EVENT_IF_INDEX(x) ((x) >> 2)
#define NETWORK_EVENT_IF_FAMILY(x) ((x) & 0x3)

/**
 * @brief Get information regarding the network interfaces on the
 * host.
 *
 * In the mobile device environment, it is often the case that network
 * interfaces will come up and go down unpredictably as the underlying Wi-Fi is
 * associated with or disassociated from access points as the device physically
 * moves.
 *
 * Different operating systems return different tidbits of information regarding
 * their network interfaces using sometimes wildly differing mechanisms, and
 * reporting what is conceptually the same information in sometimes wildly
 * differing formats.
 *
 * It is often desired to get information about network interfaces irrespective
 * of whether or not IP addresses are assigned or whether IPv4 or IPv6 addresses
 * may be present.
 *
 * This function provides an OS-indepenent way of reporting network interface
 * information in a relatively abstract way, providing as much or as little
 * information as may be available at any time.
 *
 * @param entries A vector of IfConfigEntry that will be filled out
 *     with information on the found network interfaces.
 *
 * @return Status of the operation.  Returns ER_OK on success.
 *
 * @see IfConfigEntry
 */
QStatus IfConfig(std::vector<IfConfigEntry>& entries);

/**
 * @brief Watch for network event notifications.
 *
 */
SocketFd NetworkEventSocket();

/**
 * @brief Process network event notifications.
 *
 */
NetworkEventType NetworkEventReceive(SocketFd sockFd, NetworkEventSet& networkEvents);

} // namespace ajn

#endif // _IFCONFIG_H
