/**
 * @file
 * This code is experimental, and as such has not been fully tested.
 * Please help make it more robust by contributing fixes if you find problems.
 */
/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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

QStatus AJ_CALL alljoyn_aboutobj_unannounce(alljoyn_aboutobj obj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutObj*)obj)->Unannounce();
}