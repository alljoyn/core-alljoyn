/**
 * @file
 * Utility classes for the BlueZ implementation of BT transport.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

#include <ctype.h>
#include <vector>
#include <errno.h>
#include <fcntl.h>

#include <qcc/Event.h>
#include <qcc/Socket.h>
#include <qcc/String.h>
#include <qcc/time.h>
#include <qcc/BDAddress.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>

#include "AdapterObject.h"
#include "BlueZIfc.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN_BT"

using namespace ajn;
using namespace std;
using namespace qcc;

namespace ajn {
namespace bluez {

_AdapterObject::_AdapterObject(BusAttachment& bus, const qcc::String& path) :
    ProxyBusObject(bus, bzBusName, path.c_str(), 0),
    id(0),
    discovering(false),
    powered(false)
{
    size_t i = path.size();
    while (i > 0) {
        --i;
        char c = path[i];
        if (!isdigit(c)) {
            break;
        }
        id *= 10;
        id += c - '0';
    }
}

void _AdapterObject::PropertiesChanged(ProxyBusObject& obj,
                                       const char* ifaceName,
                                       const MsgArg& changed,
                                       const MsgArg& invalidated,
                                       void* context)
{
}

} // namespace bluez
} // namespace ajn
