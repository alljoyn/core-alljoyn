#ifndef _ALLJOYN_TRANPORTMASK_H
#define _ALLJOYN_TRANPORTMASK_H
/**
 * @file
 * Transport type definitions
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

#include <qcc/platform.h>

namespace ajn {

/** Bitmask of all transport types */
typedef uint16_t TransportMask;

const TransportMask TRANSPORT_NONE      = 0x0000;   /**< no transports */

const TransportMask TRANSPORT_LOCAL     = 0x0001;   /**< Local (same device) transport */
const TransportMask TRANSPORT_TCP       = 0x0004;   /**< Transport using TCP (same as TRANSPORT_WLAN) */
const TransportMask TRANSPORT_WLAN      = 0x0004;   /**< Wireless local-area network transport (same as TRANSPORT_TCP) */
const TransportMask TRANSPORT_WWAN      = 0x0008;   /**< Wireless wide-area network transport */
const TransportMask TRANSPORT_LAN       = 0x0010;   /**< Wired local-area network transport */
const TransportMask TRANSPORT_WFD       = 0x0080;   /**< Transport using Wi-Fi Direct transport (currently unused) */
const TransportMask TRANSPORT_UDP       = 0x0100;   /**< Transport using the AllJoyn Reliable Datagram Protocol (flavor of reliable UDP) */

/**
 * A constant indicating that any transport is acceptable.
 *
 * It is the case that (1) certain topologies of AllJoyn distributed
 * applications can cause problems when run on Wi-Fi Direct substrate networks;
 * (2) the specifics of authentication in Wi-Fi Direct networks can also produce
 * surprising results in some AllJoyn topologies; and (3) there are
 * implementation problems in existing Wi-Fi Direct systems that prevent certain
 * AllJoyn topologies from forming.  Because these issues might produce
 * surprising results in existing applications that are unaware of the
 * limitations, we do no enable Wi-Fi Direct automatically.
 *
 * Selecting ANY transport really means selecting ANY but Wi-Fi Direct.
 */
const TransportMask TRANSPORT_ANY       = (0xFFFF & ~TRANSPORT_WFD);

/**
 * A constant indicating that literally any transport is acceptable.  It is
 * typically used in the routing node to enable advertisements over all
 * transports.
 */
const TransportMask TRANSPORT_ALL       = (0xFFFF);

/**
 * A constant indicating that any IP-based transport is acceptable.  It is left
 * up to the system to decide which of the available transports is best suited
 * to the implied situation.
 */
const TransportMask TRANSPORT_IP        = (TRANSPORT_TCP | TRANSPORT_UDP);

}

#endif
