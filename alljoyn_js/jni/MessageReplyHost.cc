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
#include "MessageReplyHost.h"

#include "CallbackNative.h"
#include "SignatureUtils.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

_MessageReplyHost::_MessageReplyHost(Plugin& plugin, BusAttachment& busAttachment, BusObject& busObject, ajn::Message& message, qcc::String replySignature) :
    _MessageHost(plugin, busAttachment, message),
    busObject(busObject),
    replySignature(replySignature)
{
    QCC_DbgTrace(("%s(replySignature=%s)", __FUNCTION__, replySignature.c_str()));

    OPERATION("reply", &_MessageReplyHost::reply);
    OPERATION("replyError", &_MessageReplyHost::replyError);
}

_MessageReplyHost::~_MessageReplyHost()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool _MessageReplyHost::reply(const NPVariant* npargs, uint32_t npargCount, NPVariant* npresult)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    size_t numArgs;
    const char* begin;
    ajn::MsgArg* args = NULL;
    CallbackNative* callbackNative = NULL;
    QStatus status = ER_OK;
    bool typeError = false;

    numArgs = ajn::SignatureUtils::CountCompleteTypes(replySignature.c_str());
    if (npargCount < numArgs) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    args = new ajn::MsgArg[numArgs];
    begin = replySignature.c_str();
    for (size_t i = 0; i < numArgs; ++i) {
        const char* end = begin;
        status = ajn::SignatureUtils::ParseCompleteType(end);
        if (ER_OK != status) {
            goto exit;
        }
        qcc::String typeSignature(begin, end - begin);
        ToAny(plugin, npargs[i], typeSignature, args[i], typeError);
        if (typeError) {
            char message[128];
            snprintf(message, sizeof(message), "argument %lu is not a '%s'", (unsigned long)i, typeSignature.c_str());
            plugin->RaiseTypeError(message);
            goto exit;
        }
        begin = end;
    }
    if (npargCount > numArgs) {
        callbackNative = ToNativeObject<CallbackNative>(plugin, npargs[npargCount - 1], typeError);
        if (typeError) {
            typeError = true;
            plugin->RaiseTypeError("argument 1 is not an object");
            goto exit;
        }
    }

#if !defined(NDEBUG)
    {
        qcc::String str = ajn::MsgArg::ToString(args, numArgs);
        QCC_DbgTrace(("%s", str.c_str()));
    }
#endif
    status = busObject->MethodReply(message, args, numArgs);

exit:
    if (!typeError && callbackNative) {
        CallbackNative::DispatchCallback(plugin, callbackNative, status);
        callbackNative = NULL;
    }
    delete callbackNative;
    delete[] args;
    VOID_TO_NPVARIANT(*npresult);
    return !typeError;
}

/*
 * There several   possible replyError method calls
 *  - replyError( <QStatus> ) : status error
 *  - replyError( <QStatus>, <callback> ) : status error with callback
 *  - replyError( <string> ) : error name
 *  - replyError( <string>, <string> ) : error name and message
 *  - replyError( <string>, <callback> ) : error name and callback
 *  - replyError( <string>, <string>, <callback> ) : error name, message, and callback
 */
bool _MessageReplyHost::replyError(const NPVariant* npargs, uint32_t npargCount, NPVariant* npresult)
{
    QCC_DbgTrace(("%s, npargCount : %d", __FUNCTION__, npargCount));
    CallbackNative* callbackNative = NULL;
    QStatus status = ER_OK;
    bool typeError = false;
    if (npargCount == 1 && NPVARIANT_IS_STRING(npargs[0])) {
        qcc::String errorName = ToDOMString(plugin, npargs[0], typeError);
        if (typeError) {
            plugin->RaiseTypeError("argument 0 is not a string");
            goto exit;
        }
        status = busObject->MethodReply(message, errorName.c_str(), 0);
    } else if (npargCount == 2 && (NPVARIANT_IS_INT32(npargs[0]) || NPVARIANT_IS_DOUBLE(npargs[0]))) {
        unsigned short code = ToUnsignedShort(plugin, npargs[0], typeError);
        if (typeError) {
            plugin->RaiseTypeError("argument 0 is not a number");
            goto exit;
        }
        status = busObject->MethodReply(message, static_cast<QStatus>(code));
        callbackNative = ToNativeObject<CallbackNative>(plugin, npargs[npargCount - 1], typeError);
        if (typeError) {
            typeError = true;
            plugin->RaiseTypeError("argument 1 is not an object");
            goto exit;
        }
    } else if (npargCount > 1 && NPVARIANT_IS_STRING(npargs[0])) {
        qcc::String errorName = ToDOMString(plugin, npargs[0], typeError);
        if (typeError) {
            plugin->RaiseTypeError("argument 0 is not a string");
            goto exit;
        }
        qcc::String errorMessage;
        if (npargCount > 1 && NPVARIANT_IS_STRING(npargs[1])) {
            errorMessage = ToDOMString(plugin, npargs[1], typeError);
            if (typeError) {
                plugin->RaiseTypeError("argument 1 is not a string");
                goto exit;
            }
        }
        status = busObject->MethodReply(message, errorName.c_str(), (npargCount > 1) ? errorMessage.c_str() : 0);
        if (npargCount > 1 && NPVARIANT_IS_OBJECT(npargs[npargCount - 1])) {
            callbackNative = ToNativeObject<CallbackNative>(plugin, npargs[npargCount - 1], typeError);
            if (typeError) {
                typeError = true;
                plugin->RaiseTypeError("argument 1 is not an object");
                goto exit;
            }
        }
    } else {
        typeError = true;
        plugin->RaiseTypeError("incorrect argument types");
        goto exit;
    }

exit:
    if (!typeError && callbackNative) {
        CallbackNative::DispatchCallback(plugin, callbackNative, status);
        callbackNative = NULL;
    }
    delete callbackNative;
    VOID_TO_NPVARIANT(*npresult);
    return !typeError;
}

