////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
const AJNTransportMask kAJNTransportMaskAny       = 0xFFFF;   /**< ANY transport */
const AJNTransportMask kAJNTransportMaskLocal     = 0x0001;   /**< Local (same device) transport */
const AJNTransportMask kAJNTransportMaskBluetooth = 0x0002;   /**< Bluetooth transport */
const AJNTransportMask kAJNTransportMaskWLAN      = 0x0004;   /**< Wireless local-area network transport */
const AJNTransportMask kAJNTransportMaskWWAN      = 0x0008;   /**< Wireless wide-area network transport */
const AJNTransportMask kAJNTransportMaskLAN       = 0x0010;   /**< Wired local-area network transport */
const AJNTransportMask kAJNTransportMaskICE       = 0x0020; /**< Transport using ICE protocol */
const AJNTransportMask kAJNTransportMaskProximity = 0x0040;/**< Transport using WinRT Proximity Framework */
const AJNTransportMask kAJNTransportMaskWiFiDirect= 0x0080;/**< Transport using Wi-Fi Direct transport */
