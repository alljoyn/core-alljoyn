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

#include <alljoyn_c/AboutProxy.h>
#include <alljoyn/AboutProxy.h>
#include <alljoyn/AboutProxy.h>
#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/AboutData.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_aboutproxy_handle {
    /* Empty by design */
};

alljoyn_aboutproxy AJ_CALL alljoyn_aboutproxy_create(alljoyn_busattachment bus,
                                                     const char* busName,
                                                     alljoyn_sessionid sessionId)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_aboutproxy) new ajn::AboutProxy(*(ajn::BusAttachment*)bus, busName, (ajn::SessionId)sessionId);
}

void AJ_CALL alljoyn_aboutproxy_destroy(alljoyn_aboutproxy proxy)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete (ajn::AboutProxy*)proxy;
}

QStatus AJ_CALL alljoyn_aboutproxy_getobjectdescription(alljoyn_aboutproxy proxy,
                                                        alljoyn_msgarg objectDesc)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutProxy*)proxy)->GetObjectDescription(*(ajn::MsgArg*)objectDesc);
}

QStatus AJ_CALL alljoyn_aboutproxy_getaboutdata(alljoyn_aboutproxy proxy,
                                                const char* language,
                                                alljoyn_msgarg data)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutProxy*)proxy)->GetAboutData(language, *(ajn::MsgArg*)data);
}

QStatus AJ_CALL alljoyn_aboutproxy_getversion(alljoyn_aboutproxy proxy,
                                              uint16_t* version)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutProxy*)proxy)->GetVersion(*version);
}