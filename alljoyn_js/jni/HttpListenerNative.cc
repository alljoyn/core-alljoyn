/*
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
