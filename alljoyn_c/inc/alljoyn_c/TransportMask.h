/**
 * @file
 * Transport type definitions
 */

/******************************************************************************
 * Copyright (c) 2009-2010, 2014 AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_C_TRANPORTMASK_H
#define _ALLJOYN_C_TRANPORTMASK_H

#include <qcc/platform.h>
#include <alljoyn_c/AjAPI.h>

/** Bitmask of all transport types */
typedef uint16_t alljoyn_transportmask;

const alljoyn_transportmask ALLJOYN_TRANSPORT_NONE      = 0x0000;   /**< no transports */
const alljoyn_transportmask ALLJOYN_TRANSPORT_ANY       = 0xFFFF;   /**< ANY transport */
const alljoyn_transportmask ALLJOYN_TRANSPORT_LOCAL     = 0x0001;   /**< Local (same device) transport */
const alljoyn_transportmask ALLJOYN_TRANSPORT_BLUETOOTH = 0x0002;   /**< Bluetooth transport */
const alljoyn_transportmask ALLJOYN_TRANSPORT_WLAN      = 0x0004;   /**< Wireless local-area network transport */
const alljoyn_transportmask ALLJOYN_TRANSPORT_WWAN      = 0x0008;   /**< Wireless wide-area network transport */
const alljoyn_transportmask ALLJOYN_TRANSPORT_LAN       = 0x0010;   /**< Wired local-area network transport */
const alljoyn_transportmask ALLJOYN_TRANSPORT_PROXIMITY = 0x0040;   /**< Transport using WinRT Proximity Framework */
const alljoyn_transportmask ALLJOYN_TRANSPORT_WFD       = 0x0080;   /**< Transport using Wi-Fi Direct transport */
const alljoyn_transportmask ALLJOYN_TRANSPORT_TCP       = 0x0004;   /**< TCP/IP transport */
const alljoyn_transportmask ALLJOYN_TRANSPORT_UDP       = 0x0100;   /**< UDP/IP transport */
const alljoyn_transportmask ALLJOYN_TRANSPORT_IP        = 0x0004;   /**< IP transport (system chooses actual IP-based transport */

#endif
