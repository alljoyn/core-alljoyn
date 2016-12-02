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
#include "AcceptSessionJoinerListenerNative.h"

#include "SessionOptsHost.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

AcceptSessionJoinerListenerNative::AcceptSessionJoinerListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

AcceptSessionJoinerListenerNative::~AcceptSessionJoinerListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool AcceptSessionJoinerListenerNative::onAccept(ajn::SessionPort sessionPort, const qcc::String& joiner, SessionOptsHost& opts)
{
    QCC_DbgTrace(("%s(sessionPort=%d,joiner=%s)", __FUNCTION__, sessionPort, joiner.c_str()));

    NPVariant npargs[3];
    ToUnsignedShort(plugin, sessionPort, npargs[0]);
    ToDOMString(plugin, joiner, npargs[1]);
    ToHostObject<SessionOptsHost>(plugin, opts, npargs[2]);

    bool accepted = false;
    NPVariant result = NPVARIANT_VOID;
    if (NPN_InvokeDefault(plugin->npp, objectValue, npargs, 3, &result)) {
        bool ignore; /* Can convert any JS type into a boolean type. */
        accepted = ToBoolean(plugin, result, ignore);
    }
    NPN_ReleaseVariantValue(&result);

    NPN_ReleaseVariantValue(&npargs[2]);
    NPN_ReleaseVariantValue(&npargs[1]);
    return accepted;
}