/**
 * @file
 */
/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include <alljoyn_c/AboutIconProxy.h>
#include <alljoyn/AboutIconProxy.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_abouticonproxy {
    /* Empty by design */
};

alljoyn_abouticonproxy AJ_CALL alljoyn_abouticonproxy_create(alljoyn_busattachment bus,
                                                             const char* busName,
                                                             alljoyn_sessionid sessionId)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_abouticonproxy) new ajn::AboutIconProxy(*(ajn::BusAttachment*)bus, busName, (ajn::SessionId)sessionId);
}

void AJ_CALL alljoyn_abouticonproxy_destroy(alljoyn_abouticonproxy proxy)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete (ajn::AboutIconProxy*)proxy;
}

QStatus AJ_CALL alljoyn_abouticonproxy_geticon(alljoyn_abouticonproxy proxy,
                                               alljoyn_abouticon icon)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutIconProxy*)proxy)->GetIcon(*(ajn::AboutIcon*)icon);
}

AJ_API QStatus AJ_CALL alljoyn_abouticonproxy_getversion(alljoyn_abouticonproxy proxy,
                                                         uint16_t* version)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutIconProxy*)proxy)->GetVersion(*version);
}