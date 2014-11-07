////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013 - 2014, AllSeen Alliance. All rights reserved.
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

const AJNTransportMask kAJNTransportMaskNone      = 0x0000;   /**< no transports */
const AJNTransportMask kAJNTransportMaskLocal     = 0x0001;   /**< Local (same device) transport */
const AJNTransportMask kAJNTransportMaskWLAN      = 0x0004;   /**< Wireless local-area network transport */
const AJNTransportMask kAJNTransportMaskWWAN      = 0x0008;   /**< Wireless wide-area network transport */
const AJNTransportMask kAJNTransportMaskLAN       = 0x0010;   /**< Wired local-area network transport */
const AJNTransportMask kAJNTransportMaskWiFiDirect= 0x0080;   /**< Transport using Wi-Fi Direct transport */
const AJNTransportMask kAJNTransportMaskTCP       = 0x0004;   /**< TCP/IP transport */
const AJNTransportMask kAJNTransportMaskUDP       = 0x0100;   /**< UDP/IP transport */
const AJNTransportMask kAJNTransportMaskIP        = 0x0104;   /**< IP transport (system chooses betwen TCP and UDP) */
const AJNTransportMask kAJNTransportMaskAny       = 0xFF7F;   /**< ANY transport (but Wi-Fi Direct) */
