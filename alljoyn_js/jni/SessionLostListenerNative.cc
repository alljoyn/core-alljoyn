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
#include "SessionLostListenerNative.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

SessionLostListenerNative::SessionLostListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

SessionLostListenerNative::~SessionLostListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void SessionLostListenerNative::onLost(ajn::SessionId id, ajn::SessionListener::SessionLostReason reason)
{
    QCC_DbgTrace(("%s(id=%u, reason=%u)", __FUNCTION__, id, reason));

    NPVariant npargs[2];
    ToUnsignedLong(plugin, id, npargs[0]);
    ToUnsignedLong(plugin, reason, npargs[1]);

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result);
    NPN_ReleaseVariantValue(&result);
}