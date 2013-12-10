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
#include "SessionMemberRemovedListenerNative.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

SessionMemberRemovedListenerNative::SessionMemberRemovedListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

SessionMemberRemovedListenerNative::~SessionMemberRemovedListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void SessionMemberRemovedListenerNative::onMemberRemoved(ajn::SessionId id, const qcc::String& uniqueName)
{
    QCC_DbgTrace(("%s(id=%u,uniqueName=%s)", __FUNCTION__, id, uniqueName.c_str()));

    NPVariant npargs[2];
    ToUnsignedLong(plugin, id, npargs[0]);
    ToDOMString(plugin, uniqueName, npargs[1]);

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result);
    NPN_ReleaseVariantValue(&result);
}
