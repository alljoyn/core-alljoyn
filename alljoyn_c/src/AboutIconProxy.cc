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