/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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
