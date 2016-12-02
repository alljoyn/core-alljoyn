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
#include "HttpListenerNative.h"

#include "HttpRequestHost.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

HttpListenerNative::HttpListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

HttpListenerNative::~HttpListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void HttpListenerNative::onRequest(HttpRequestHost& request)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    NPVariant nparg;
    ToHostObject<HttpRequestHost>(plugin, request, nparg);

    NPVariant result = NPVARIANT_VOID;
    if (!NPN_InvokeDefault(plugin->npp, objectValue, &nparg, 1, &result)) {
        QCC_LogError(ER_FAIL, ("NPN_InvokeDefault failed"));
    }

    NPN_ReleaseVariantValue(&result);
    NPN_ReleaseVariantValue(&nparg);
}