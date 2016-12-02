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
#include "SuccessListenerNative.h"

#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

SuccessListenerNative::SuccessListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

SuccessListenerNative::~SuccessListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void SuccessListenerNative::onSuccess()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, 0, 0, &result);
    NPN_ReleaseVariantValue(&result);
}
