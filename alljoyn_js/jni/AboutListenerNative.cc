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
#include "AboutListenerNative.h"

#include "BusAttachmentHost.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

AboutListenerNative::AboutListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

AboutListenerNative::~AboutListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}


void AboutListenerNative::onAnnounced(const qcc::String& busName, uint16_t version, ajn::SessionPort port, const ajn::MsgArg& objectDescriptionArg, const ajn::MsgArg& aboutDataArg)
{
    QCC_DbgTrace(("%s(busname=%s,version=%d,port=%d)", __FUNCTION__, busName.c_str(), version, port));
    NPIdentifier onAnnounced = NPN_GetStringIdentifier("onAnnounced");
    if (NPN_HasMethod(plugin->npp, objectValue, onAnnounced)) {
        NPVariant npargs[5];
        ToDOMString(plugin, busName, npargs[0]);
        ToUnsignedShort(plugin, version, npargs[1]);
        ToUnsignedShort(plugin, port, npargs[2]);

        QStatus status = ER_OK;
        ToAny(plugin, objectDescriptionArg, npargs[3], status);
        assert(status == ER_OK);
        ToAny(plugin, aboutDataArg, npargs[4], status);
        assert(status == ER_OK);

        NPVariant result = NPVARIANT_VOID;
        NPN_Invoke(plugin->npp, objectValue, onAnnounced, npargs, 5, &result);
        NPN_ReleaseVariantValue(&result);

        NPN_ReleaseVariantValue(&npargs[4]);
        NPN_ReleaseVariantValue(&npargs[3]);
        NPN_ReleaseVariantValue(&npargs[0]);
    }
}