/*
 * Copyright (c) 2011-2012, 2014 AllSeen Alliance. All rights reserved.
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
 */
#include "SessionOptsInterface.h"

#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

std::map<qcc::String, int32_t> _SessionOptsInterface::constants;

std::map<qcc::String, int32_t>& _SessionOptsInterface::Constants()
{
    if (constants.empty()) {
        CONSTANT("TRAFFIC_MESSAGES",       0x01);
        CONSTANT("TRAFFIC_RAW_UNRELIABLE", 0x02);
        CONSTANT("TRAFFIC_RAW_RELIABLE",   0x04);

        CONSTANT("PROXIMITY_ANY",      0xFF);
        CONSTANT("PROXIMITY_PHYSICAL", 0x01);
        CONSTANT("PROXIMITY_NETWORK",  0x02);

        CONSTANT("TRANSPORT_NONE",      0x0000);
        CONSTANT("TRANSPORT_LOCAL",     0x0001);
        CONSTANT("TRANSPORT_WLAN",      0x0004);
        CONSTANT("TRANSPORT_WWAN",      0x0008);
        CONSTANT("TRANSPORT_LAN",       0x0010);
        CONSTANT("TRANSPORT_PROXIMITY", 0x0040);

        CONSTANT("TRANSPORT_TCP",       0x0004);
        CONSTANT("TRANSPORT_UDP",       0x0100);
        CONSTANT("TRANSPORT_IP",        0x0104);

        CONSTANT("TRANSPORT_ANY",       0xFF7F);
    }
    return constants;
}

_SessionOptsInterface::_SessionOptsInterface(Plugin& plugin) :
    ScriptableObject(plugin, Constants())
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_SessionOptsInterface::~_SessionOptsInterface()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

