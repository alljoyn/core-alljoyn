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
#include "SignalEmitterHost.h"

#include "CallbackNative.h"
#include "SignatureUtils.h"
#include "TypeMapping.h"
#include <alljoyn/MsgArg.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

_SignalEmitterHost::_SignalEmitterHost(Plugin& plugin, BusObject& busObject) :
    ScriptableObject(plugin),
    busObject(busObject)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CALLER(&_SignalEmitterHost::emitSignal);
}

_SignalEmitterHost::~_SignalEmitterHost()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool _SignalEmitterHost::emitSignal(const NPVariant* npargs, uint32_t npargCount, NPVariant* npresult)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String interfaceName;
    qcc::String signalName;
    CallbackNative* callbackNative = NULL;
    const ajn::InterfaceDescription* iface = NULL;
    const ajn::InterfaceDescription::Member* signal = NULL;
    size_t numArgs;
    const char* begin;
    ajn::MsgArg* args = NULL;
    ajn::SessionId sessionId = 0;
    qcc::String destination;
    uint16_t timeToLive = 0;
    uint8_t flags = 0;
    QStatus status = ER_OK;

    bool typeError = false;
    if (npargCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }
    interfaceName = ToDOMString(plugin, npargs[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }
    signalName = ToDOMString(plugin, npargs[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a string");
        goto exit;
    }
    callbackNative = ToNativeObject<CallbackNative>(plugin, npargs[npargCount - 1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }
    QCC_DbgTrace(("interfaceName=%s,signalName=%s", interfaceName.c_str(), signalName.c_str()));

    iface = busObject->busAttachment->GetInterface(interfaceName.c_str());
    if (!iface) {
        status = ER_BUS_NO_SUCH_INTERFACE;
        goto exit;
    }
    signal = iface->GetMember(signalName.c_str());
    if (!signal) {
        status = ER_BUS_INTERFACE_NO_SUCH_MEMBER;
        goto exit;
    }
    numArgs = ajn::SignatureUtils::CountCompleteTypes(signal->signature.c_str());
    if ((npargCount - 3) < numArgs) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }
    args = new ajn::MsgArg[numArgs];
    begin = signal->signature.c_str();
    for (size_t i = 0; i < numArgs; ++i) {
        const char* end = begin;
        status = ajn::SignatureUtils::ParseCompleteType(end);
        if (ER_OK != status) {
            goto exit;
        }
        qcc::String typeSignature(begin, end - begin);
        ToAny(plugin, npargs[i + 2], typeSignature, args[i], typeError);
        if (typeError) {
            char message[128];
            snprintf(message, sizeof(message), "argument %lu is not a '%s'", (unsigned long)i, typeSignature.c_str());
            plugin->RaiseTypeError(message);
            goto exit;
        }
        begin = end;
    }

    if (numArgs < (npargCount - 3)) {
        NPVariant params = npargs[npargCount - 2];
        if (!NPVARIANT_IS_OBJECT(params)) {
            typeError = true;
            char message[128];
            snprintf(message, sizeof(message), "argument %d is not an object", npargCount - 2);
            plugin->RaiseTypeError(message);
            goto exit;
        }

        NPVariant value;
        VOID_TO_NPVARIANT(value);
        NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(params), NPN_GetStringIdentifier("sessionId"), &value);
        if (!NPVARIANT_IS_VOID(value)) {
            sessionId = ToUnsignedLong(plugin, value, typeError);
        }
        NPN_ReleaseVariantValue(&value);
        if (typeError) {
            plugin->RaiseTypeError("'sessionId' is not a number");
            goto exit;
        }

        VOID_TO_NPVARIANT(value);
        NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(params), NPN_GetStringIdentifier("destination"), &value);
        if (!NPVARIANT_IS_VOID(value)) {
            destination = ToDOMString(plugin, value, typeError);
        }
        NPN_ReleaseVariantValue(&value);
        if (typeError) {
            plugin->RaiseTypeError("'destination' is not a string");
            goto exit;
        }

        VOID_TO_NPVARIANT(value);
        NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(params), NPN_GetStringIdentifier("timeToLive"), &value);
        if (!NPVARIANT_IS_VOID(value)) {
            timeToLive = ToUnsignedShort(plugin, value, typeError);
        }
        NPN_ReleaseVariantValue(&value);
        if (typeError) {
            plugin->RaiseTypeError("'timeToLive' is not a number");
            goto exit;
        }

        VOID_TO_NPVARIANT(value);
        NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(params), NPN_GetStringIdentifier("flags"), &value);
        if (!NPVARIANT_IS_VOID(value)) {
            flags = ToOctet(plugin, value, typeError);
        }
        NPN_ReleaseVariantValue(&value);
        if (typeError) {
            plugin->RaiseTypeError("'flags' is not a number");
            goto exit;
        }
    }

#if !defined(NDEBUG)
    {
        qcc::String str = ajn::MsgArg::ToString(args, numArgs);
        QCC_DbgTrace(("%s", str.c_str()));
    }
#endif
    status = busObject->Signal(destination.empty() ? 0 : destination.c_str(), sessionId, *signal,
                               args, numArgs, timeToLive, flags);

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

