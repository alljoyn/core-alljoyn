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
#include "SessionJoinedListenerNative.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

SessionJoinedListenerNative::SessionJoinedListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

SessionJoinedListenerNative::~SessionJoinedListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void SessionJoinedListenerNative::onJoined(ajn::SessionPort sessionPort, ajn::SessionId id, const qcc::String& joiner)
{
    QCC_DbgTrace(("%s(sessionPort=%d,id=%u,joiner=%s)", __FUNCTION__, sessionPort, id, joiner.c_str()));

    NPVariant npargs[3];
    ToUnsignedShort(plugin, sessionPort, npargs[0]);
    ToUnsignedLong(plugin, id, npargs[1]);
    ToDOMString(plugin, joiner, npargs[2]);

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, npargs, 3, &result);
    NPN_ReleaseVariantValue(&result);

    NPN_ReleaseVariantValue(&npargs[2]);
}