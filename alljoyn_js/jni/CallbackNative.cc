/*
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
#include "CallbackNative.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

CallbackNative::CallbackNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

CallbackNative::~CallbackNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void CallbackNative::onCallback(BusErrorHost& busError)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    NPVariant nparg;
    ToHostObject<BusErrorHost>(plugin, busError, nparg);

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, &nparg, 1, &result);
    NPN_ReleaseVariantValue(&result);

    NPN_ReleaseVariantValue(&nparg);
}

void CallbackNative::onCallback(QStatus status)
{
    QCC_DbgTrace(("%s(status=%s)", __FUNCTION__, QCC_StatusText(status)));

    NPVariant nparg;
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(nparg);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, nparg);
    }

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, &nparg, 1, &result);
    NPN_ReleaseVariantValue(&result);
}

void CallbackNative::onCallback(QStatus status, qcc::String& s)
{
    QCC_DbgTrace(("%s(status=%s,s=%s)", __FUNCTION__, QCC_StatusText(status), s.c_str()));

    NPVariant npargs[2];
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(npargs[0]);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, npargs[0]);
    }
    ToDOMString(plugin, s, npargs[1]);

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result);
    NPN_ReleaseVariantValue(&result);

    NPN_ReleaseVariantValue(&npargs[1]);
}

void CallbackNative::onCallback(QStatus status, uint32_t u)
{
    QCC_DbgTrace(("%s(status=%s,u=%d)", __FUNCTION__, QCC_StatusText(status), u));

    NPVariant npargs[2];
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(npargs[0]);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, npargs[0]);
    }
    ToUnsignedLong(plugin, u, npargs[1]);

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result);
    NPN_ReleaseVariantValue(&result);
}

void CallbackNative::onCallback(QStatus status, bool b)
{
    QCC_DbgTrace(("%s(status=%s,b=%d)", __FUNCTION__, QCC_StatusText(status), b));

    NPVariant npargs[2];
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(npargs[0]);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, npargs[0]);
    }
    ToBoolean(plugin, b, npargs[1]);

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result);
    NPN_ReleaseVariantValue(&result);
}

void CallbackNative::onCallback(QStatus status, ajn::SessionId id, SessionOptsHost& opts)
{
    QCC_DbgTrace(("%s(status=%s,id=%d)", __FUNCTION__, QCC_StatusText(status), id));

    NPVariant npargs[3];
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(npargs[0]);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, npargs[0]);
    }
    ToUnsignedLong(plugin, id, npargs[1]);
    ToHostObject<SessionOptsHost>(plugin, opts, npargs[2]);

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, npargs, 3, &result);
    NPN_ReleaseVariantValue(&result);

    NPN_ReleaseVariantValue(&npargs[2]);
}

void CallbackNative::onCallback(QStatus status, ajn::SessionPort port)
{
    QCC_DbgTrace(("%s(status=%s,port=%d)", __FUNCTION__, QCC_StatusText(status), port));

    NPVariant npargs[2];
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(npargs[0]);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, npargs[0]);
    }
    ToUnsignedShort(plugin, port, npargs[1]);

    NPVariant result = NPVARIANT_VOID;
    NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result);
    NPN_ReleaseVariantValue(&result);
}

void CallbackNative::onCallback(QStatus status, MessageHost& message, const ajn::MsgArg* args, size_t numArgs)
{
    QCC_DbgTrace(("%s(status=%s,args=%p,numArgs=%d)", __FUNCTION__, QCC_StatusText(status), args, numArgs));
#if !defined(NDEBUG)
    qcc::String str = ajn::MsgArg::ToString(args, numArgs);
    QCC_DbgTrace(("%s", str.c_str()));
#endif

    QStatus sts = ER_OK;
    uint32_t npargCount = 2 + numArgs;
    NPVariant* npargs = new NPVariant[npargCount];
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(npargs[0]);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, npargs[0]);
    }
    ToHostObject<MessageHost>(plugin, message, npargs[1]);
    size_t i;
    for (i = 0; (ER_OK == sts) && (i < numArgs); ++i) {
        ToAny(plugin, args[i], npargs[2 + i], sts);
    }

    NPVariant result = NPVARIANT_VOID;
    if (ER_OK == sts) {
        if (!NPN_InvokeDefault(plugin->npp, objectValue, npargs, npargCount, &result)) {
            sts = ER_FAIL;
            QCC_LogError(sts, ("NPN_InvokeDefault failed"));
        }
    } else {
        npargCount = 2 + i;
    }

    for (uint32_t j = 0; j < npargCount; ++j) {
        NPN_ReleaseVariantValue(&npargs[j]);
    }
    delete[] npargs;
    NPN_ReleaseVariantValue(&result);
}

void CallbackNative::onCallback(QStatus status, ProxyBusObjectHost& proxyBusObject)
{
    QCC_DbgTrace(("%s(status=%s)", __FUNCTION__, QCC_StatusText(status)));

    NPVariant npargs[2];
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(npargs[0]);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, npargs[0]);
    }
    ToHostObject<ProxyBusObjectHost>(plugin, proxyBusObject, npargs[1]);

    NPVariant result = NPVARIANT_VOID;
    if (!NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result)) {
        QCC_LogError(ER_FAIL, ("NPN_InvokeDefault failed"));
    }

    NPN_ReleaseVariantValue(&npargs[1]);
    NPN_ReleaseVariantValue(&result);
}

void CallbackNative::onCallback(QStatus status, SocketFdHost& socketFd)
{
    QCC_DbgTrace(("%s(status=%s)", __FUNCTION__, QCC_StatusText(status)));

    NPVariant npargs[2];
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(npargs[0]);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, npargs[0]);
    }
    ToHostObject<SocketFdHost>(plugin, socketFd, npargs[1]);

    NPVariant result = NPVARIANT_VOID;
    if (!NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result)) {
        QCC_LogError(ER_FAIL, ("NPN_InvokeDefault failed"));
    }

    NPN_ReleaseVariantValue(&npargs[1]);
    NPN_ReleaseVariantValue(&result);
}

void CallbackNative::onCallback(QStatus status, InterfaceDescriptionNative* interfaceDescription)
{
    QCC_DbgTrace(("%s(status=%s)", __FUNCTION__, QCC_StatusText(status)));

    NPVariant npargs[2];
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(npargs[0]);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, npargs[0]);
    }
    ToNativeObject<InterfaceDescriptionNative>(plugin, interfaceDescription, npargs[1]);

    NPVariant result = NPVARIANT_VOID;
    if (!NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result)) {
        QCC_LogError(ER_FAIL, ("NPN_InvokeDefault failed"));
    }

    NPN_ReleaseVariantValue(&npargs[1]);
    NPN_ReleaseVariantValue(&result);
}

void CallbackNative::onCallback(QStatus status, InterfaceDescriptionNative** interfaceDescriptions, size_t numInterfaces)
{
    QCC_DbgTrace(("%s(status=%s,numInterfaces=%d)", __FUNCTION__, QCC_StatusText(status), numInterfaces));

    NPVariant element = NPVARIANT_VOID;
    NPVariant npargs[2];
    if (!NewArray(plugin, npargs[1])) {
        status = ER_FAIL;
        QCC_LogError(status, ("NewArray failed"));
    }
    for (size_t i = 0; (ER_OK == status) && (i < numInterfaces); ++i) {
        ToNativeObject<InterfaceDescriptionNative>(plugin, interfaceDescriptions[i], element);
        if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(npargs[1]), NPN_GetIntIdentifier(i), &element)) {
            status = ER_FAIL;
            QCC_LogError(status, ("NPN_SetProperty failed"));
        }
        NPN_ReleaseVariantValue(&element);
        VOID_TO_NPVARIANT(element);
    }
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(npargs[0]);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, npargs[0]);
    }

    NPVariant result = NPVARIANT_VOID;
    if (!NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result)) {
        QCC_LogError(ER_FAIL, ("NPN_InvokeDefault failed"));
    }

    NPN_ReleaseVariantValue(&element);
    NPN_ReleaseVariantValue(&npargs[1]);
    NPN_ReleaseVariantValue(&result);
}

void CallbackNative::onCallback(QStatus status, std::vector<ProxyBusObjectHost>& children)
{
    QCC_DbgTrace(("%s(status=%s,children.size()=%d)", __FUNCTION__, QCC_StatusText(status), children.size()));

    NPVariant element = NPVARIANT_VOID;
    NPVariant npargs[2];
    if (!NewArray(plugin, npargs[1])) {
        status = ER_FAIL;
        QCC_LogError(status, ("NewArray failed"));
    }
    for (size_t i = 0; (ER_OK == status) && (i < children.size()); ++i) {
        ToHostObject<ProxyBusObjectHost>(plugin, children[i], element);
        if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(npargs[1]), NPN_GetIntIdentifier(i), &element)) {
            status = ER_FAIL;
            QCC_LogError(status, ("NPN_SetProperty failed"));
        }
        NPN_ReleaseVariantValue(&element);
        VOID_TO_NPVARIANT(element);
    }
    if (ER_OK == status) {
        VOID_TO_NPVARIANT(npargs[0]);
    } else {
        BusErrorHost busError(plugin, status);
        ToHostObject<BusErrorHost>(plugin, busError, npargs[0]);
    }

    NPVariant result = NPVARIANT_VOID;
    if (!NPN_InvokeDefault(plugin->npp, objectValue, npargs, 2, &result)) {
        QCC_LogError(ER_FAIL, ("NPN_InvokeDefault failed"));
    }

    NPN_ReleaseVariantValue(&element);
    NPN_ReleaseVariantValue(&npargs[1]);
    NPN_ReleaseVariantValue(&result);
}

class CallbackContext : public PluginData::CallbackContext {
  public:
    CallbackNative* callbackNative;
    QStatus status;
    CallbackContext(CallbackNative* callbackNative, QStatus status) :
        callbackNative(callbackNative),
        status(status) { }
    virtual ~CallbackContext() {
        delete callbackNative;
    }
};

class StatusCallbackContext : public CallbackContext {
  public:
    StatusCallbackContext(CallbackNative* callbackNative, QStatus status) :
        CallbackContext(callbackNative, status) { }
};

void CallbackNative::DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status)
{
    PluginData::Callback callback(plugin, _StatusCallbackCB);
    callback->context = new StatusCallbackContext(callbackNative, status);
    PluginData::DispatchCallback(callback);
}

void CallbackNative::_StatusCallbackCB(PluginData::CallbackContext* ctx)
{
    StatusCallbackContext* context = static_cast<StatusCallbackContext*>(ctx);
    context->callbackNative->onCallback(context->status);
}

class StringCallbackContext : public CallbackContext {
  public:
    qcc::String s;
    StringCallbackContext(CallbackNative* callbackNative, QStatus status, qcc::String& s) :
        CallbackContext(callbackNative, status),
        s(s) { }
};

void CallbackNative::DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, qcc::String& s)
{
    PluginData::Callback callback(plugin, _StringCallbackCB);
    callback->context = new StringCallbackContext(callbackNative, status, s);
    PluginData::DispatchCallback(callback);
}

void CallbackNative::_StringCallbackCB(PluginData::CallbackContext* ctx)
{
    StringCallbackContext* context = static_cast<StringCallbackContext*>(ctx);
    context->callbackNative->onCallback(context->status, context->s);
}

class UnsignedLongCallbackContext : public CallbackContext {
  public:
    uint32_t u;
    UnsignedLongCallbackContext(CallbackNative* callbackNative, QStatus status, uint32_t u) :
        CallbackContext(callbackNative, status),
        u(u) { }
};

void CallbackNative::DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, uint32_t u)
{
    PluginData::Callback callback(plugin, _UnsignedLongCallbackCB);
    callback->context = new UnsignedLongCallbackContext(callbackNative, status, u);
    PluginData::DispatchCallback(callback);
}

void CallbackNative::_UnsignedLongCallbackCB(PluginData::CallbackContext* ctx)
{
    UnsignedLongCallbackContext* context = static_cast<UnsignedLongCallbackContext*>(ctx);
    context->callbackNative->onCallback(context->status, context->u);
}

class BoolCallbackContext : public CallbackContext {
  public:
    bool b;
    BoolCallbackContext(CallbackNative* callbackNative, QStatus status, bool b) :
        CallbackContext(callbackNative, status),
        b(b) { }
};

void CallbackNative::DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, bool b)
{
    PluginData::Callback callback(plugin, _BoolCallbackCB);
    callback->context = new BoolCallbackContext(callbackNative, status, b);
    PluginData::DispatchCallback(callback);
}

void CallbackNative::_BoolCallbackCB(PluginData::CallbackContext* ctx)
{
    BoolCallbackContext* context = static_cast<BoolCallbackContext*>(ctx);
    context->callbackNative->onCallback(context->status, context->b);
}

class BindSessionPortCallbackContext : public CallbackContext {
  public:
    ajn::SessionPort port;
    BindSessionPortCallbackContext(CallbackNative* callbackNative, QStatus status, ajn::SessionPort port) :
        CallbackContext(callbackNative, status),
        port(port) { }
};

void CallbackNative::DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, ajn::SessionPort port)
{
    PluginData::Callback callback(plugin, _BindSessionPortCallbackCB);
    callback->context = new BindSessionPortCallbackContext(callbackNative, status, port);
    PluginData::DispatchCallback(callback);
}

void CallbackNative::_BindSessionPortCallbackCB(PluginData::CallbackContext* ctx)
{
    BindSessionPortCallbackContext* context = static_cast<BindSessionPortCallbackContext*>(ctx);
    context->callbackNative->onCallback(context->status, context->port);
}

class GetProxyBusObjectCallbackContext : public CallbackContext {
  public:
    ProxyBusObjectHost proxyBusObject;
    GetProxyBusObjectCallbackContext(CallbackNative* callbackNative, QStatus status, ProxyBusObjectHost& proxyBusObject) :
        CallbackContext(callbackNative, status),
        proxyBusObject(proxyBusObject) { }
};

void CallbackNative::DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, ProxyBusObjectHost& proxyBusObject)
{
    PluginData::Callback callback(plugin, _GetProxyBusObjectCallbackCB);
    callback->context = new GetProxyBusObjectCallbackContext(callbackNative, status, proxyBusObject);
    PluginData::DispatchCallback(callback);
}

void CallbackNative::_GetProxyBusObjectCallbackCB(PluginData::CallbackContext* ctx)
{
    GetProxyBusObjectCallbackContext* context = static_cast<GetProxyBusObjectCallbackContext*>(ctx);
    context->callbackNative->onCallback(context->status, context->proxyBusObject);
}

class GetSessionFdCallbackContext : public CallbackContext {
  public:
    SocketFdHost socketFd;
    GetSessionFdCallbackContext(CallbackNative* callbackNative, QStatus status, SocketFdHost& socketFd) :
        CallbackContext(callbackNative, status),
        socketFd(socketFd) { }
};

void CallbackNative::DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, SocketFdHost& socketFd)
{
    PluginData::Callback callback(plugin, _GetSessionFdCallbackCB);
    callback->context = new GetSessionFdCallbackContext(callbackNative, status, socketFd);
    PluginData::DispatchCallback(callback);
}

void CallbackNative::_GetSessionFdCallbackCB(PluginData::CallbackContext* ctx)
{
    GetSessionFdCallbackContext* context = static_cast<GetSessionFdCallbackContext*>(ctx);
    context->callbackNative->onCallback(context->status, context->socketFd);
}

class GetInterfaceCallbackContext : public CallbackContext {
  public:
    InterfaceDescriptionNative* interfaceDescription;
    GetInterfaceCallbackContext(CallbackNative* callbackNative, QStatus status, InterfaceDescriptionNative* interfaceDescription) :
        CallbackContext(callbackNative, status),
        interfaceDescription(interfaceDescription) { }
    ~GetInterfaceCallbackContext() {
        delete interfaceDescription;
    }
};

void CallbackNative::DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, InterfaceDescriptionNative* interfaceDescription)
{
    PluginData::Callback callback(plugin, _GetInterfaceCallbackCB);
    callback->context = new GetInterfaceCallbackContext(callbackNative, status, interfaceDescription);
    PluginData::DispatchCallback(callback);
}

void CallbackNative::_GetInterfaceCallbackCB(PluginData::CallbackContext* ctx)
{
    GetInterfaceCallbackContext* context = static_cast<GetInterfaceCallbackContext*>(ctx);
    context->callbackNative->onCallback(context->status, context->interfaceDescription);
}

class GetInterfacesCallbackContext : public CallbackContext {
  public:
    InterfaceDescriptionNative** interfaceDescriptions;
    size_t numInterfaces;
    GetInterfacesCallbackContext(CallbackNative* callbackNative, QStatus status, InterfaceDescriptionNative** interfaceDescriptions, size_t numInterfaces) :
        CallbackContext(callbackNative, status),
        numInterfaces(numInterfaces) {
        this->interfaceDescriptions = new InterfaceDescriptionNative *[numInterfaces];
        for (size_t i = 0; i < numInterfaces; ++i) {
            this->interfaceDescriptions[i] = interfaceDescriptions[i];
        }
    }
    ~GetInterfacesCallbackContext() {
        for (size_t i = 0; i < numInterfaces; ++i) {
            delete interfaceDescriptions[i];
        }
        delete[] interfaceDescriptions;
    }
};

void CallbackNative::DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, InterfaceDescriptionNative** interfaceDescriptions, size_t numInterfaces)
{
    PluginData::Callback callback(plugin, _GetInterfacesCallbackCB);
    callback->context = new GetInterfacesCallbackContext(callbackNative, status, interfaceDescriptions, numInterfaces);
    PluginData::DispatchCallback(callback);
}

void CallbackNative::_GetInterfacesCallbackCB(PluginData::CallbackContext* ctx)
{
    GetInterfacesCallbackContext* context = static_cast<GetInterfacesCallbackContext*>(ctx);
    context->callbackNative->onCallback(context->status, context->interfaceDescriptions, context->numInterfaces);
}

class GetChildrenCallbackContext : public CallbackContext {
  public:
    std::vector<ProxyBusObjectHost> children;
    GetChildrenCallbackContext(CallbackNative* callbackNative, QStatus status, std::vector<ProxyBusObjectHost>& children) :
        CallbackContext(callbackNative, status),
        children(children) { }
};

void CallbackNative::DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, std::vector<ProxyBusObjectHost>& children)
{
    PluginData::Callback callback(plugin, _GetChildrenCallbackCB);
    callback->context = new GetChildrenCallbackContext(callbackNative, status, children);
    PluginData::DispatchCallback(callback);
}

void CallbackNative::_GetChildrenCallbackCB(PluginData::CallbackContext* ctx)
{
    GetChildrenCallbackContext* context = static_cast<GetChildrenCallbackContext*>(ctx);
    context->callbackNative->onCallback(context->status, context->children);
}

