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

#include <alljoyn_c/AboutObj.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/MsgArg.h>

#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_aboutobj {
    /* Empty by design */
};

alljoyn_aboutobj AJ_CALL alljoyn_aboutobj_create(alljoyn_busattachment bus,
                                                 alljoyn_about_announceflag isAnnounced)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_aboutobj) new ajn::AboutObj(*(ajn::BusAttachment*)bus, (ajn::BusObject::AnnounceFlag)isAnnounced);
}

void AJ_CALL alljoyn_aboutobj_destroy(alljoyn_aboutobj obj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete (ajn::AboutObj*)obj;
}

QStatus AJ_CALL alljoyn_aboutobj_announce(alljoyn_aboutobj obj,
                                          alljoyn_sessionport sessionPort,
                                          alljoyn_aboutdata aboutData)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObj*)obj)->Announce((ajn::SessionPort)sessionPort, *(ajn::AboutDataListener*)aboutData);
}

QStatus AJ_CALL alljoyn_aboutobj_announce_using_datalistener(alljoyn_aboutobj obj,
                                                             alljoyn_sessionport sessionPort,
                                                             alljoyn_aboutdatalistener aboutListener)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObj*)obj)->Announce((ajn::SessionPort)sessionPort, *(ajn::AboutDataListener*)aboutListener);
}

QStatus AJ_CALL alljoyn_aboutobj_unannounce(alljoyn_aboutobj obj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObj*)obj)->Unannounce();
}