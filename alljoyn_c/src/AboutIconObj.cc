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

#include <alljoyn_c/AboutIconObj.h>
#include <alljoyn/AboutIconObj.h>
#include <alljoyn/BusAttachment.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_abouticonobj_handle {
    /* Empty by design */
};

alljoyn_abouticonobj AJ_CALL alljoyn_abouticonobj_create(alljoyn_busattachment bus,
                                                         alljoyn_abouticon icon)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_abouticonobj) new ajn::AboutIconObj(*(ajn::BusAttachment*)bus, *(ajn::AboutIcon*)icon);
}

void AJ_CALL alljoyn_abouticonobj_destroy(alljoyn_abouticonobj icon)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete (ajn::AboutIconObj*)icon;
}