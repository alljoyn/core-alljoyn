/*
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
#include "SessionOptsHost.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"


_SessionOptsHost::_SessionOptsHost(Plugin& plugin, const ajn::SessionOpts& opts) :
    ScriptableObject(plugin, _SessionOptsInterface::Constants()),
    opts(opts)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ATTRIBUTE("traffic", &_SessionOptsHost::getTraffic, 0);
    ATTRIBUTE("isMultipoint", &_SessionOptsHost::getIsMultipoint, 0);
    ATTRIBUTE("proximity", &_SessionOptsHost::getProximity, 0);
    ATTRIBUTE("transports", &_SessionOptsHost::getTransports, 0);
}

_SessionOptsHost::~_SessionOptsHost()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool _SessionOptsHost::getTraffic(NPVariant* result)
{
    ToOctet(plugin, opts.traffic, *result);
    return true;
}

bool _SessionOptsHost::getIsMultipoint(NPVariant* result)
{
    ToBoolean(plugin, opts.isMultipoint, *result);
    return true;
}

bool _SessionOptsHost::getProximity(NPVariant* result)
{
    ToOctet(plugin, opts.proximity, *result);
    return true;
}

bool _SessionOptsHost::getTransports(NPVariant* result)
{
    ToUnsignedShort(plugin, opts.transports, *result);
    return true;
}

