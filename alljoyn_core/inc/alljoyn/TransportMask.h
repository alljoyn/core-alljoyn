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

const TransportMask TRANSPORT_NONE          = 0x0000;  /**< no transports */
const TransportMask TRANSPORT_LOCAL         = 0x0001;  /**< Local (same device) transport */
const TransportMask TRANSPORT_TCP           = 0x0004;  /**< Transport using TCP as the underlying mechanism */
const TransportMask TRANSPORT_UDP           = 0x0100;  /**< Transport using UDP as the underlying mechanism */

/**
 * A placeholder for an experimental transport that has not yet reached the
 * performance, stability or testing requirements of a commercialized transport.
 *
 * It is expected that each experimental Transport will alias this bit if
 * included in an AllJoyn release and then allocate one of the reserved mask
 * bits upon attaining commercialized status.
 *
 * For example,
 *     const TransportMask TRANSPORT_CAN_AND_STRING = TRANSPORT_EXPERIMENTAL
 */
const TransportMask TRANSPORT_EXPERIMENTAL  = 0x8000;

/**
 * A constant indicating that any IP-based transport is acceptable.  It is left
 * up to the system to decide which of the available transports is "best."
 */
const TransportMask TRANSPORT_IP        = (TRANSPORT_TCP | TRANSPORT_UDP);

/**
 * A constant indicating that any commericalized transport is acceptable.  As
 * more transports are introduced and reach commercial quality, we expect the
 * corresponding mask bits to be added here.
 */
const TransportMask TRANSPORT_ANY       = (TRANSPORT_LOCAL | TRANSPORT_IP);

/**
 * Obsolete mask indicating that any transport as long as it is running over the
 * local wireless local area network is acceptable.
 *
 * @deprecated February 2015 for 15.04 release
 */
QCC_DEPRECATED(const TransportMask TRANSPORT_WLAN) = 0x0004;   /**< Wireless local-area network transport (same as TRANSPORT_TCP) */

/**
 * Obsolete mask indicating that any transport as long as it is running over the
 * local wireless wide area network is acceptable.
 *
 * @deprecated February 2015 for 15.04 release
 */
QCC_DEPRECATED(const TransportMask TRANSPORT_WWAN) = 0x0008;   /**< Wireless wide-area network transport */

/**
 * Obsolete mask indicating that any transport as long as it is running over the
 * (non-wireless) local area network -- Ethernet -- is acceptable.
 *
 * @deprecated February 2015 for 15.04 release
 */
QCC_DEPRECATED(const TransportMask TRANSPORT_LAN) = 0x0010;   /**< Wired local-area network transport */

/**
 * Obsolete mask indicating that the Wi-Fi Direct Transport is acceptable.
 *
 * @deprecated February 2015 for 15.04 release
 */
QCC_DEPRECATED(const TransportMask TRANSPORT_WFD) = 0x0080;   /**< Transport using Wi-Fi Direct transport (currently unused) */

}

#endif
