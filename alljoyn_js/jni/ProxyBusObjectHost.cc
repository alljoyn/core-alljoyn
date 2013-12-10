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
#include "ProxyBusObjectHost.h"

//#include "BusUtil.h"
#include "CallbackNative.h"
#include "InterfaceDescriptionNative.h"
#include "SignatureUtils.h"
#include "TypeMapping.h"
#include <alljoyn/AllJoynStd.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

class IntrospectRemoteObjectAsyncCB : public ajn::ProxyBusObject::Listener {
  public:
    class _Env {
      public:
        Plugin plugin;
        BusAttachment busAttachment;
        ProxyBusObject proxyBusObject;
        CallbackNative* callbackNative;
        _Env(Plugin& plugin, BusAttachment& busAttachment, ProxyBusObject& proxyBusObject, CallbackNative* callbackNative) :
            plugin(plugin),
            busAttachment(busAttachment),
            proxyBusObject(proxyBusObject),
            callbackNative(callbackNative) { }
        ~_Env() {
            delete callbackNative;
        }
    };
    typedef qcc::ManagedObj<_Env> Env;
    Env env;
    IntrospectRemoteObjectAsyncCB(Plugin& plugin, BusAttachment& busAttachment, ProxyBusObject& proxyBusObject, CallbackNative* callbackNative) :
        env(plugin, busAttachment, proxyBusObject, callbackNative) { }
    virtual ~IntrospectRemoteObjectAsyncCB() { }

    class IntrospectCBContext : public PluginData::CallbackContext {
      public:
        Env env;
        QStatus status;
        IntrospectCBContext(Env& env, QStatus status) :
            env(env),
            status(status) { }
    };
    virtual void IntrospectCB(QStatus status, ajn::ProxyBusObject*, void*) {
        PluginData::Callback callback(env->plugin, _IntrospectCB);
        callback->context = new IntrospectCBContext(env, status);
        delete this;
        PluginData::DispatchCallback(callback);
    }
    static void _IntrospectCB(PluginData::CallbackContext* ctx) {
        IntrospectCBContext* context = static_cast<IntrospectCBContext*>(ctx);
        if (ER_OK == context->status) {
            context->env->callbackNative->onCallback(context->status);
        } else {
            BusErrorHost busError(context->env->plugin, context->status);
            context->env->callbackNative->onCallback(busError);
        }
    }
};

class ReplyReceiver : public ajn::ProxyBusObject::Listener, public ajn::MessageReceiver {
  public:
    class _Env {
      public:
        ReplyReceiver* thiz;
        Plugin plugin;
        BusAttachment busAttachment;
        ProxyBusObject proxyBusObject;
        qcc::String interfaceName;
        qcc::String methodName;
        CallbackNative* callbackNative;
        NPVariant* npargs;
        uint32_t npargCount;

        QStatus status;
        qcc::String errorMessage;

        _Env(ReplyReceiver* thiz, Plugin& plugin, BusAttachment& busAttachment, ProxyBusObject& proxyBusObject, qcc::String& interfaceName, qcc::String& methodName, CallbackNative* callbackNative, const NPVariant* npargs, uint32_t npargCount) :
            thiz(thiz),
            plugin(plugin),
            busAttachment(busAttachment),
            proxyBusObject(proxyBusObject),
            interfaceName(interfaceName),
            methodName(methodName),
            callbackNative(callbackNative) {
            this->npargCount = npargCount;
            this->npargs = new NPVariant[this->npargCount];
            for (uint32_t i = 0; i < this->npargCount; ++i) {
                NPN_RetainVariantValue(&npargs[i], &this->npargs[i]);
            }
        }
        ~_Env() {
            for (uint32_t i = 0; i < npargCount; ++i) {
                NPN_ReleaseVariantValue(&npargs[i]);
            }
            delete[] npargs;
            delete callbackNative;
        }
    };
    typedef qcc::ManagedObj<_Env> Env;
    Env env;
    ReplyReceiver(Plugin& plugin, BusAttachment& busAttachment, ProxyBusObject& proxyBusObject, qcc::String& interfaceName, qcc::String& methodName, CallbackNative* callbackNative, const NPVariant* npargs, uint32_t npargCount) :
        env(this, plugin, busAttachment, proxyBusObject, interfaceName, methodName, callbackNative, npargs, npargCount) { }
    virtual ~ReplyReceiver() { }

    class IntrospectCBContext : public PluginData::CallbackContext {
      public:
        Env env;
        QStatus status;
        IntrospectCBContext(Env& env, QStatus status) :
            env(env),
            status(status) { }
        virtual ~IntrospectCBContext() {
            if (ER_OK != env->status) {
                ajn::Message message(*env->busAttachment);
                env->thiz->ReplyHandler(message, 0);
            }
        }
    };
    virtual void IntrospectCB(QStatus status, ajn::ProxyBusObject* obj, void* context) {
        PluginData::Callback callback(env->plugin, _IntrospectCB);
        callback->context = new IntrospectCBContext(env, status);
        PluginData::DispatchCallback(callback);
    }
    static void _IntrospectCB(PluginData::CallbackContext* ctx) {
        IntrospectCBContext* context = static_cast<IntrospectCBContext*>(ctx);
        Env& env = context->env;
        if (ER_OK != context->status) {
            QCC_LogError(context->status, ("IntrospectRemoteObjectAsync failed"));
        }

        const ajn::InterfaceDescription* iface;
        const ajn::InterfaceDescription::Member* method;
        size_t numArgs;
        ajn::MsgArg* args = NULL;
        const char* begin;
        uint32_t timeout = ajn::ProxyBusObject::DefaultCallTimeout;
        uint8_t flags = 0;
        qcc::String noReply = "false";
        bool typeError = false;

        iface = env->busAttachment->GetInterface(env->interfaceName.c_str());
        if (!iface) {
            env->status = ER_BUS_NO_SUCH_INTERFACE;
            QCC_LogError(env->status, ("%s", env->interfaceName.c_str()));
            goto exit;
        }
        method = iface->GetMember(env->methodName.c_str());
        if (!method) {
            env->status = ER_BUS_INTERFACE_NO_SUCH_MEMBER;
            QCC_LogError(env->status, ("%s", env->methodName.c_str()));
            goto exit;
        }

        numArgs = ajn::SignatureUtils::CountCompleteTypes(method->signature.c_str());
        if (env->npargCount < numArgs) {
            env->status = ER_BAD_ARG_COUNT;
            QCC_LogError(env->status, (""));
            goto exit;
        }
        args = new ajn::MsgArg[numArgs];
        begin = method->signature.c_str();
        for (size_t i = 0; i < numArgs; ++i) {
            const char* end = begin;
            env->status = ajn::SignatureUtils::ParseCompleteType(end);
            if (ER_OK != env->status) {
                QCC_LogError(env->status, (""));
                goto exit;
            }
            qcc::String typeSignature(begin, end - begin);
            ToAny(env->plugin, env->npargs[i], typeSignature, args[i], typeError);
            if (typeError) {
                env->status = ER_BUS_BAD_VALUE;
                char errorMessage[128];
                snprintf(errorMessage, sizeof(errorMessage), "argument %lu is not a '%s'", (unsigned long)(i + 2), typeSignature.c_str());
                env->errorMessage = errorMessage;
                QCC_LogError(env->status, (""));
                goto exit;
            }
            begin = end;
        }

        if (numArgs != env->npargCount) {
            NPVariant params = env->npargs[env->npargCount - 1];
            if (!NPVARIANT_IS_OBJECT(params)) {
                env->status = ER_BUS_BAD_VALUE;
                char errorMessage[128];
                snprintf(errorMessage, sizeof(errorMessage), "argument %d is not an object", env->npargCount - 1 + 2);
                env->errorMessage = errorMessage;
                QCC_LogError(env->status, (""));
                goto exit;
            }

            NPVariant result;
            VOID_TO_NPVARIANT(result);
            NPN_GetProperty(env->plugin->npp, NPVARIANT_TO_OBJECT(params), NPN_GetStringIdentifier("timeout"), &result);
            if (!NPVARIANT_IS_VOID(result)) {
                timeout = ToUnsignedLong(env->plugin, result, typeError);
            }
            NPN_ReleaseVariantValue(&result);
            if (typeError) {
                env->status = ER_BUS_BAD_VALUE;
                env->errorMessage = "'timeout' is not a number";
                QCC_LogError(env->status, (""));
                goto exit;
            }

            VOID_TO_NPVARIANT(result);
            NPN_GetProperty(env->plugin->npp, NPVARIANT_TO_OBJECT(params), NPN_GetStringIdentifier("flags"), &result);
            if (!NPVARIANT_IS_VOID(result)) {
                flags = ToOctet(env->plugin, result, typeError);
            }
            NPN_ReleaseVariantValue(&result);
            if (typeError) {
                env->status = ER_BUS_BAD_VALUE;
                env->errorMessage = "'flags' is not a number";
                QCC_LogError(env->status, (""));
                goto exit;
            }
        }

#if !defined(NDEBUG)
        {
            qcc::String str = ajn::MsgArg::ToString(args, numArgs);
            QCC_DbgTrace(("%s", str.c_str()));
            QCC_DbgTrace(("flags=0x%x", flags));
        }
#endif
        if (flags & ajn::ALLJOYN_FLAG_NO_REPLY_EXPECTED) {
            env->status = env->proxyBusObject->MethodCallAsync(*method, 0, 0, args, numArgs, 0, 0, flags);
            ajn::Message message(*env->busAttachment);
            env->thiz->ReplyHandler(message, 0);
        } else {
            env->status = env->proxyBusObject->MethodCallAsync(*method, env->thiz, static_cast<ajn::MessageReceiver::ReplyHandler>(&ReplyReceiver::ReplyHandler), args, numArgs, 0, timeout, flags);
        }

    exit:
        delete[] args;
    }

    class ReplyHandlerContext : public PluginData::CallbackContext {
      public:
        Env env;
        ajn::Message message;
        ReplyHandlerContext(Env& env, ajn::Message& message) :
            env(env),
            message(message) { }
    };
    virtual void ReplyHandler(ajn::Message& message, void*) {
        PluginData::Callback callback(env->plugin, _ReplyHandler);
        callback->context = new ReplyHandlerContext(env, message);
        env->thiz = NULL;
        delete this;
        PluginData::DispatchCallback(callback);
    }
    static void _ReplyHandler(PluginData::CallbackContext* ctx) {
        ReplyHandlerContext* context = static_cast<ReplyHandlerContext*>(ctx);
        Env& env = context->env;
        if (ER_OK != env->status) {
            if (!env->errorMessage.empty()) {
                BusErrorHost busError(env->plugin, "BusError", env->errorMessage, env->status);
                env->callbackNative->onCallback(busError);
            } else {
                BusErrorHost busError(env->plugin, env->status);
                env->callbackNative->onCallback(busError);
            }
        } else if (ajn::MESSAGE_ERROR == context->message->GetType()) {
            QStatus status = ER_BUS_REPLY_IS_ERROR_MESSAGE;
            qcc::String errorMessage;
            const char* errorName = context->message->GetErrorName(&errorMessage);
            if (errorName && !strcmp(ajn::org::alljoyn::Bus::ErrorName, errorName) && context->message->GetArg(1)) {
                status = static_cast<QStatus>(context->message->GetArg(1)->v_uint16);
            }
            /*
             * Technically, an empty error message field is not the same as no error message field,
             * but treat them the same here.
             */
            if (errorName) {
                qcc::String name(errorName);
                BusErrorHost busError(env->plugin, name, errorMessage, status);
                env->callbackNative->onCallback(busError);
            } else {
                BusErrorHost busError(env->plugin, status);
                env->callbackNative->onCallback(busError);
            }
        } else {
            MessageHost messageHost(env->plugin, env->busAttachment, context->message);
            size_t numArgs;
            const ajn::MsgArg* args;
            context->message->GetArgs(numArgs, args);
#if !defined(NDEBUG)
            qcc::String str = ajn::MsgArg::ToString(args, numArgs);
            QCC_DbgTrace(("%s", str.c_str()));
#endif
            env->callbackNative->onCallback(env->status, messageHost, args, numArgs);
        }
    }
};

class _ProxyBusObjectHostImpl {
  public:
    std::map<qcc::String, ProxyBusObjectHost> children;
};

_ProxyBusObjectHost::_ProxyBusObjectHost(Plugin& plugin, BusAttachment& busAttachment, const char* serviceName, const char* path, ajn::SessionId sessionId) :
    ScriptableObject(plugin),
    busAttachment(busAttachment),
    proxyBusObject(*busAttachment, serviceName, path, sessionId)
{
    QCC_DbgTrace(("%s(serviceName=%s,path=%s,sessionId=%u)", __FUNCTION__, serviceName, path, sessionId));
    Initialize();
}

_ProxyBusObjectHost::_ProxyBusObjectHost(Plugin& plugin, BusAttachment& busAttachment, ajn::ProxyBusObject* proxyBusObject) :
    ScriptableObject(plugin),
    busAttachment(busAttachment),
    proxyBusObject(*proxyBusObject)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    Initialize();
}

void _ProxyBusObjectHost::Initialize()
{
    impl = new _ProxyBusObjectHostImpl();

    ATTRIBUTE("path", &_ProxyBusObjectHost::getPath, 0);
    ATTRIBUTE("serviceName", &_ProxyBusObjectHost::getServiceName, 0);
    ATTRIBUTE("sessionId", &_ProxyBusObjectHost::getSessionId, 0);
    ATTRIBUTE("secure", &_ProxyBusObjectHost::getSecure, 0);

    OPERATION("getChildren", &_ProxyBusObjectHost::getChildren);
    OPERATION("getInterface", &_ProxyBusObjectHost::getInterface);
    OPERATION("getInterfaces", &_ProxyBusObjectHost::getInterfaces);
    OPERATION("introspect", &_ProxyBusObjectHost::introspect);
    OPERATION("methodCall", &_ProxyBusObjectHost::methodCall);
    OPERATION("parseXML", &_ProxyBusObjectHost::parseXML);
    OPERATION("secureConnection", &_ProxyBusObjectHost::secureConnection);
}

_ProxyBusObjectHost::~_ProxyBusObjectHost()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete impl;
}

bool _ProxyBusObjectHost::getPath(NPVariant* result)
{
    ToDOMString(plugin, proxyBusObject->GetPath(), *result);
    return true;
}

bool _ProxyBusObjectHost::getChildren(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CallbackNative* callbackNative = NULL;
    size_t numChildren;
    ajn::ProxyBusObject** children = NULL;
    std::vector<ProxyBusObjectHost> hostChildren;
    QStatus status = ER_OK;
    bool typeError = false;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[0], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    numChildren = proxyBusObject->GetChildren();
    children = new ajn::ProxyBusObject *[numChildren];
    proxyBusObject->GetChildren(children, numChildren);

    for (uint32_t i = 0; i < numChildren; ++i) {
        ajn::ProxyBusObject* child = children[i];
        qcc::String name = child->GetServiceName() + child->GetPath();
        if (child->GetSessionId()) {
            char sessionId[32];
            snprintf(sessionId, 32, ":sessionId=%u", child->GetSessionId());
            name += sessionId;
        }
        if (impl->children.find(name) == impl->children.end()) {
            std::pair<qcc::String, ProxyBusObjectHost> element(name, ProxyBusObjectHost(plugin, busAttachment, child));
            impl->children.insert(element);
        }
        std::map<qcc::String, ProxyBusObjectHost>::iterator it = impl->children.find(name);
        hostChildren.push_back(it->second);
    }

    CallbackNative::DispatchCallback(plugin, callbackNative, status, hostChildren);
    callbackNative = NULL;

exit:
    delete callbackNative;
    delete[] children;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _ProxyBusObjectHost::getInterface(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String name;
    CallbackNative* callbackNative = NULL;
    InterfaceDescriptionNative* interfaceDescriptionNative = NULL;
    QStatus status = ER_OK;
    bool typeError = false;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    name = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    if (proxyBusObject->ImplementsInterface(name.c_str())) {
        interfaceDescriptionNative = InterfaceDescriptionNative::GetInterface(plugin, busAttachment, name);
    }

    CallbackNative::DispatchCallback(plugin, callbackNative, status, interfaceDescriptionNative);
    interfaceDescriptionNative = NULL;
    callbackNative = NULL;

exit:
    delete interfaceDescriptionNative;
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _ProxyBusObjectHost::getInterfaces(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CallbackNative* callbackNative = NULL;
    size_t numIfaces;
    const ajn::InterfaceDescription** ifaces = NULL;
    InterfaceDescriptionNative** descs = NULL;
    QStatus status = ER_OK;
    bool typeError = false;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[0], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    numIfaces = proxyBusObject->GetInterfaces();
    ifaces = new const ajn::InterfaceDescription *[numIfaces];
    proxyBusObject->GetInterfaces(ifaces, numIfaces);
    descs = new InterfaceDescriptionNative *[numIfaces];
    for (uint32_t i = 0; i < numIfaces; ++i) {
        descs[i] = InterfaceDescriptionNative::GetInterface(plugin, busAttachment, ifaces[i]->GetName());
    }

    CallbackNative::DispatchCallback(plugin, callbackNative, status, descs, numIfaces);
    descs = NULL;
    callbackNative = NULL;

exit:
    delete callbackNative;
    delete[] descs;
    delete[] ifaces;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _ProxyBusObjectHost::getServiceName(NPVariant* result)
{
    ToDOMString(plugin, proxyBusObject->GetServiceName(), *result);
    return true;
}

bool _ProxyBusObjectHost::getSessionId(NPVariant* result)
{
    ToUnsignedShort(plugin, proxyBusObject->GetSessionId(), *result);
    return true;
}

bool _ProxyBusObjectHost::getSecure(NPVariant* result)
{
    ToUnsignedShort(plugin, proxyBusObject->IsSecure(), *result);
    return true;
}

bool _ProxyBusObjectHost::introspect(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    CallbackNative* callbackNative = NULL;
    IntrospectRemoteObjectAsyncCB* callback = NULL;
    bool typeError = false;
    QStatus status = ER_OK;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[0], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    callback = new IntrospectRemoteObjectAsyncCB(plugin, busAttachment, proxyBusObject, callbackNative);
    callbackNative = NULL; /* callback now owns callbackNative */

    status = proxyBusObject->IntrospectRemoteObjectAsync(callback, static_cast<ajn::ProxyBusObject::Listener::IntrospectCB>(&IntrospectRemoteObjectAsyncCB::IntrospectCB), 0);
    if (ER_OK == status) {
        callback = NULL; /* alljoyn owns callback */
    } else {
        callback->IntrospectCB(status, 0, 0);
        callback = NULL; /* IntrospectCB will delete callback */
    }

exit:
    delete callbackNative;
    delete callback;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _ProxyBusObjectHost::methodCall(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String interfaceName;
    qcc::String methodName;
    CallbackNative* callbackNative = NULL;
    ReplyReceiver* replyReceiver = NULL;
    QStatus status = ER_OK;
    bool typeError = false;

    if (argCount < 3) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    interfaceName = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    methodName = ToDOMString(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[argCount - 1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 0 is not an object");
        goto exit;
    }

    replyReceiver = new ReplyReceiver(plugin, busAttachment, proxyBusObject, interfaceName, methodName, callbackNative, &args[2], argCount - 3);
    callbackNative = NULL; /* replyReceiver now owns callbackNative */
    if (proxyBusObject->ImplementsInterface(interfaceName.c_str())) {
        replyReceiver->IntrospectCB(ER_OK, 0, 0);
    } else if (busAttachment->GetInterface(interfaceName.c_str())) {
        status = proxyBusObject->AddInterface(*busAttachment->GetInterface(interfaceName.c_str()));
        replyReceiver->IntrospectCB(status, 0, 0);
    } else {
        status = proxyBusObject->IntrospectRemoteObjectAsync(replyReceiver, static_cast<ajn::ProxyBusObject::Listener::IntrospectCB>(&ReplyReceiver::IntrospectCB), 0);
        if (ER_OK != status) {
            replyReceiver->IntrospectCB(status, 0, 0);
        }
    }
    replyReceiver = NULL; /* alljoyn now owns replyReceiver */

exit:
    delete callbackNative;
    delete replyReceiver;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _ProxyBusObjectHost::parseXML(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String source;
    CallbackNative* callbackNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    source = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    status = proxyBusObject->ParseXml(source.c_str());

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _ProxyBusObjectHost::secureConnection(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    bool typeError = false;
    QStatus status = ER_OK;
    bool forceAuth = false;
    CallbackNative* callbackNative = NULL;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }

    if (argCount > 1) {
        forceAuth = ToBoolean(plugin, args[0], typeError);
        if (typeError) {
            plugin->RaiseTypeError("argument 0 is not a boolean");
            goto exit;
        }
    }

    callbackNative = ToNativeObject<CallbackNative>(plugin, args[argCount - 1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    status = proxyBusObject->SecureConnectionAsync(forceAuth);

    CallbackNative::DispatchCallback(plugin, callbackNative, status);
    callbackNative = NULL;

exit:
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}
