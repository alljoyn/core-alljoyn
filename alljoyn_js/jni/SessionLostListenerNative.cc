/*
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
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
#include "SessionLostListenerNative.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

SessionLostListenerNative::SessionLostListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

SessionLostListenerNative::~SessionLostListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void SessionLostListenerNative::onLost(ajn::SessionId id, ajn::SessionListener::SessionLostReason reason)
{
    QCC_DbgTrace(("%s(id=%u, reason=%u)", __FUNCTION__, id, reason));

    NPVariant npargs[2];
    ToUnsignedLong(plugin, id, npargs[0]);
    ToUnsignedLong(plugin, reason, npargs[1]);

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result);
    NPN_ReleaseVariantValue(&result);
}
