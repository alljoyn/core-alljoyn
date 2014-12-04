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

#include <alljoyn_c/AboutIcon.h>
#include <alljoyn/AboutIcon.h>
#include <alljoyn/MsgArg.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_abouticon_handle {
    /* Empty by design */
};

alljoyn_abouticon AJ_CALL alljoyn_abouticon_create()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_abouticon) new ajn::AboutIcon();
}

void AJ_CALL alljoyn_abouticon_destroy(alljoyn_abouticon icon)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete (ajn::AboutIcon*)icon;
}

QStatus AJ_CALL alljoyn_abouticon_setcontent(alljoyn_abouticon icon,
                                             const char* type,
                                             uint8_t* data,
                                             size_t csize,
                                             bool ownsData)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutIcon*)icon)->SetContent(type, data, csize, ownsData);
}

QStatus AJ_CALL alljoyn_abouticon_seturl(alljoyn_abouticon icon,
                                         const char* type,
                                         const char* url)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutIcon*)icon)->SetUrl(type, url);
}

void AJ_CALL alljoyn_abouticon_clear(alljoyn_abouticon icon)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutIcon*)icon)->Clear();
}

QStatus AJ_CALL alljoyn_abouticon_setcontent_frommsgarg(alljoyn_abouticon icon,
                                                        const alljoyn_msgarg arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::AboutIcon*)icon)->SetContent(*(ajn::MsgArg*)arg);
}