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
#include "SessionMemberRemovedListenerNative.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

SessionMemberRemovedListenerNative::SessionMemberRemovedListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

SessionMemberRemovedListenerNative::~SessionMemberRemovedListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void SessionMemberRemovedListenerNative::onMemberRemoved(ajn::SessionId id, const qcc::String& uniqueName)
{
    QCC_DbgTrace(("%s(id=%u,uniqueName=%s)", __FUNCTION__, id, uniqueName.c_str()));

    NPVariant npargs[2];
    ToUnsignedLong(plugin, id, npargs[0]);
    ToDOMString(plugin, uniqueName, npargs[1]);

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result);
    NPN_ReleaseVariantValue(&result);
}