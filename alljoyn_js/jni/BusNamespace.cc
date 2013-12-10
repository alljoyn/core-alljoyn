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
#include "BusNamespace.h"

#include "FeaturePermissions.h"
#include "HostObject.h"
#include "SuccessListenerNative.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

std::map<qcc::String, int32_t> _BusNamespace::constants;

std::map<qcc::String, int32_t>& _BusNamespace::Constants()
{
    if (constants.empty()) {
        CONSTANT("USER_ALLOWED", USER_ALLOWED);
        CONSTANT("DEFAULT_ALLOWED", DEFAULT_ALLOWED);
        CONSTANT("DEFAULT_DENIED", DEFAULT_DENIED);
        CONSTANT("USER_DENIED", USER_DENIED);
    }
    return constants;
}

_BusNamespace::_BusNamespace(Plugin& plugin) :
    ScriptableObject(plugin, Constants()),
    busAttachmentInterface(plugin),
    busErrorInterface(plugin),
    credentialsInterface(plugin),
    ifcSecurityInterface(plugin),
    messageInterface(plugin),
    sessionLostReasonInterface(plugin),
    sessionOptsInterface(plugin),
    socketFdInterface(plugin),
    versionInterface(plugin)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ATTRIBUTE("BusAttachment", &_BusNamespace::getBusAttachment, 0);
    ATTRIBUTE("BusError", &_BusNamespace::getBusError, 0);
    ATTRIBUTE("Credentials", &_BusNamespace::getCredentials, 0);
    ATTRIBUTE("IfcSecurity", &_BusNamespace::getIfcSecurity, 0);
    ATTRIBUTE("Message", &_BusNamespace::getMessage, 0);
    ATTRIBUTE("SessionLostReason", &_BusNamespace::getSessionLostReason, 0);
    ATTRIBUTE("SessionOpts", &_BusNamespace::getSessionOpts, 0);
    ATTRIBUTE("SocketFd", &_BusNamespace::getSocketFd, 0);
    ATTRIBUTE("Version", &_BusNamespace::getVersion, 0);
    ATTRIBUTE("privilegedFeatures", &_BusNamespace::getPrivilegedFeatures, 0);

    OPERATION("permissionLevel", &_BusNamespace::permissionLevel);
    OPERATION("requestPermission", &_BusNamespace::requestPermission);
}

_BusNamespace::~_BusNamespace()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool _BusNamespace::getBusAttachment(NPVariant* result)
{
    ToHostObject<BusAttachmentInterface>(plugin, busAttachmentInterface, *result);
    return true;
}

bool _BusNamespace::getBusError(NPVariant* result)
{
    ToHostObject<BusErrorInterface>(plugin, busErrorInterface, *result);
    return true;
}

bool _BusNamespace::getCredentials(NPVariant* result)
{
    ToHostObject<CredentialsInterface>(plugin, credentialsInterface, *result);
    return true;
}

bool _BusNamespace::getIfcSecurity(NPVariant* result)
{
    ToHostObject<IfcSecurityInterface>(plugin, ifcSecurityInterface, *result);
    return true;
}

bool _BusNamespace::getMessage(NPVariant* result)
{
    ToHostObject<MessageInterface>(plugin, messageInterface, *result);
    return true;
}

bool _BusNamespace::getSessionLostReason(NPVariant* result)
{
    ToHostObject<SessionLostReasonInterface>(plugin, sessionLostReasonInterface, *result);
    return true;
}

bool _BusNamespace::getSessionOpts(NPVariant* result)
{
    ToHostObject<SessionOptsInterface>(plugin, sessionOptsInterface, *result);
    return true;
}

bool _BusNamespace::getSocketFd(NPVariant* result)
{
    ToHostObject<SocketFdInterface>(plugin, socketFdInterface, *result);
    return true;
}

bool _BusNamespace::getVersion(NPVariant* result)
{
    ToHostObject<VersionInterface>(plugin, versionInterface, *result);
    return true;
}

bool _BusNamespace::getPrivilegedFeatures(NPVariant* result)
{
    VOID_TO_NPVARIANT(*result);
    if (NewArray(plugin, *result)) {
        NPVariant element = NPVARIANT_VOID;
        ToDOMString(plugin, ALLJOYN_FEATURE, element);
        if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(*result), NPN_GetIntIdentifier(0), &element)) {
            QCC_LogError(ER_FAIL, ("NPN_SetProperty failed"));
        }
        NPN_ReleaseVariantValue(&element);
    } else {
        QCC_LogError(ER_FAIL, ("NewArray failed"));
    }
    return true;
}

bool _BusNamespace::permissionLevel(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status = ER_OK;
    bool typeError = false;
    qcc::String feature;
    int32_t level;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }
    feature = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    status = PluginData::PermissionLevel(plugin, feature, level);

exit:
    if ((ER_OK == status) && !typeError) {
        ToLong(plugin, level, *result);
        return true;
    } else {
        if (ER_OK != status) {
            plugin->RaiseBusError(status);
        }
        return false;
    }
}

class RequestPermissionAsyncCB : public RequestPermissionListener {
  public:
    class _Env {
      public:
        Plugin plugin;
        qcc::String feature;
        SuccessListenerNative* callbackNative;
        _Env(Plugin& plugin, qcc::String& feature, SuccessListenerNative* callbackNative) :
            plugin(plugin),
            feature(feature),
            callbackNative(callbackNative) { }
        ~_Env() {
            delete callbackNative;
        }
    };
    typedef qcc::ManagedObj<_Env> Env;
    Env env;
    RequestPermissionAsyncCB(Plugin& plugin, qcc::String& feature, SuccessListenerNative* callbackNative) :
        env(plugin, feature, callbackNative) { QCC_DbgTrace(("%s", __FUNCTION__)); }
    virtual ~RequestPermissionAsyncCB() { QCC_DbgTrace(("%s", __FUNCTION__)); }

    class RequestPermissionCBContext : public PluginData::CallbackContext {
      public:
        Env env;
        int32_t level;
        bool remember;
        RequestPermissionCBContext(Env& env, int32_t level, bool remember) :
            env(env),
            level(level),
            remember(remember) { }
    };
    virtual void RequestPermissionCB(int32_t level, bool remember) {
        PluginData::Callback callback(env->plugin, _RequestPermissionCB);
        callback->context = new RequestPermissionCBContext(env, level, remember);
        delete this;
        PluginData::DispatchCallback(callback);
    }
    static void _RequestPermissionCB(PluginData::CallbackContext* ctx) {
        RequestPermissionCBContext* context = static_cast<RequestPermissionCBContext*>(ctx);
        PluginData::SetPermissionLevel(context->env->plugin, context->env->feature, context->level, context->remember);
        context->env->callbackNative->onSuccess();
    }
};

bool _BusNamespace::requestPermission(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status = ER_OK;
    bool typeError = false;
    qcc::String feature;
    SuccessListenerNative* callbackNative = NULL;
    RequestPermissionAsyncCB* callback = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }
    feature = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }
    callbackNative = ToNativeObject<SuccessListenerNative>(plugin, args[1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 1 is not an object");
        goto exit;
    }

    callback = new RequestPermissionAsyncCB(plugin, feature, callbackNative);
    callbackNative = NULL; /* callback now owns callbackNative */
    status = RequestPermission(plugin, feature, callback);
    if (ER_OK == status) {
        callback = NULL; /* request now owns callback */
    }

exit:
    delete callback;
    delete callbackNative;
    if ((ER_OK == status) && !typeError) {
        VOID_TO_NPVARIANT(*result);
        return true;
    } else {
        if (ER_OK != status) {
            plugin->RaiseBusError(status);
        }
        return false;
    }
}
