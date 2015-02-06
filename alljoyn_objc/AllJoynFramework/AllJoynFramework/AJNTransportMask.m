////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import "AJNTransportMask.h"
#import "AvailabilityMacros.h"

const AJNTransportMask kAJNTransportMaskNone      = 0x0000;   /**< no transports */
const AJNTransportMask kAJNTransportMaskLocal     = 0x0001;   /**< Local (same device) transport */
const AJNTransportMask kAJNTransportMaskTCP       = 0x0004;   /**< TCP/IP transport */
const AJNTransportMask kAJNTransportMaskUDP       = 0x0100;   /**< UDP/IP transport */
const AJNTransportMask kAJNTransportMaskIP        = (kAJNTransportMaskTCP + kAJNTransportMaskUDP); /**< An IP-based transport */
const AJNTransportMask kAJNTransportMaskAny       = (kAJNTransportMaskLocal + kAJNTransportMaskIP); /**< Any non-experimental transport */

/*
 * QCC_DEPRECATED is a macro defined in platform.h on Linux and Windows for C++
 * that is used to mark functions, etc., as deprecated. Windows compilers use
 * the __declspec(deprecated) prefix notation and various flavors of gcc use the
 * __attribute__ ((deprecated)) postfix notation.  Objective-C on the other hand
 * uses a postfix to the initialization value for attribtues which isn't
 * compatible with the use of QCC_DEPRECATED.  Therefore we use the functionality
 * defined for the purpose directly in Objective-C: DEPRECATED_ATTRIBUTE.
 */
const AJNTransportMask kAJNTransportMaskWLAN      = 0x0004 DEPRECATED_ATTRIBUTE;   /**< Wireless local-area network transport */
const AJNTransportMask kAJNTransportMaskWWAN      = 0x0008 DEPRECATED_ATTRIBUTE;   /**< Wireless wide-area network transport */
const AJNTransportMask kAJNTransportMaskLAN       = 0x0010 DEPRECATED_ATTRIBUTE;   /**< Wired local-area network transport */
const AJNTransportMask kAJNTransportMaskWiFiDirect= 0x0080 DEPRECATED_ATTRIBUTE;   /**< Transport using Wi-Fi Direct transport */
