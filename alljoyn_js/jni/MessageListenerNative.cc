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
#include "MessageListenerNative.h"

#include "MessageHost.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

MessageListenerNative::MessageListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

MessageListenerNative::~MessageListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void MessageListenerNative::onMessage(MessageHost& message, const ajn::MsgArg* args, size_t numArgs)
{
    QCC_DbgTrace(("%s(args=%p,numArgs=%d)", __FUNCTION__, args, numArgs));
#if !defined(NDEBUG)
    qcc::String str = ajn::MsgArg::ToString(args, numArgs);
    QCC_DbgTrace(("%s", str.c_str()));
#endif

    QStatus status = ER_OK;
    uint32_t npargCount = 1 + numArgs;
    NPVariant* npargs = new NPVariant[npargCount];
    ToHostObject<MessageHost>(plugin, message, npargs[0]);
    size_t i;
    for (i = 0; (ER_OK == status) && (i < numArgs); ++i) {
        ToAny(plugin, args[i], npargs[1 + i], status);
    }

    NPVariant result = NPVARIANT_VOID;
    if (ER_OK == status) {
        if (!NPN_InvokeDefault(plugin->npp, objectValue, npargs, npargCount, &result)) {
            status = ER_FAIL;
            QCC_LogError(status, ("NPN_InvokeDefault failed"));
        }
    } else {
        npargCount = 1 + i;
    }

    for (uint32_t j = 0; j < npargCount; ++j) {
        NPN_ReleaseVariantValue(&npargs[j]);
    }
    delete[] npargs;
    NPN_ReleaseVariantValue(&result);
}
