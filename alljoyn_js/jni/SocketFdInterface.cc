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
#include "SocketFdInterface.h"

#include "CallbackNative.h"
#include "HttpListenerNative.h"
#include "SocketFdHost.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

_SocketFdInterface::_SocketFdInterface(Plugin& plugin) :
    ScriptableObject(plugin),
    httpServer(plugin)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    OPERATION("createObjectURL", &_SocketFdInterface::createObjectURL);
    OPERATION("revokeObjectURL", &_SocketFdInterface::revokeObjectURL);
}

_SocketFdInterface::~_SocketFdInterface()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool _SocketFdInterface::Construct(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status = ER_OK;
    bool typeError = false;
    qcc::String fd;
    const char* nptr;
    char* endptr;
    qcc::SocketFd socketFd;

    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }
    fd = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }
    nptr = fd.c_str();
    socketFd = strtoll(nptr, &endptr, 0);
    typeError = (endptr == nptr);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a socket descriptor");
        goto exit;
    }
    if (qcc::INVALID_SOCKET_FD != socketFd) {
        status = qcc::SocketDup(socketFd, socketFd);
        if (ER_OK != status) {
            QCC_LogError(status, ("SocketDup failed"));
            goto exit;
        }
    }

    {
        SocketFdHost socketFdHost(plugin, socketFd);
        ToHostObject<SocketFdHost>(plugin, socketFdHost, *result);
    }

exit:
    if ((ER_OK == status) && !typeError) {
        return true;
    } else {
        if (ER_OK != status) {
            plugin->RaiseBusError(status);
        }
        return false;
    }
}

bool _SocketFdInterface::createObjectURL(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    QStatus status = ER_OK;
    bool typeError = false;
    SocketFdHost* socketFd = NULL;
    CallbackNative* callbackNative = NULL;
    HttpListenerNative* httpListener = NULL;
    qcc::String url;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }
    socketFd = ToHostObject<SocketFdHost>(plugin, args[0], typeError);
    if (typeError || !socketFd) {
        plugin->RaiseTypeError("argument 0 is not a SocketFd");
        goto exit;
    }
    if (argCount > 2) {
        httpListener = ToNativeObject<HttpListenerNative>(plugin, args[1], typeError);
        if (typeError || !httpListener) {
            plugin->RaiseTypeError("argument 1 is not an object");
            goto exit;
        }
    }
    callbackNative = ToNativeObject<CallbackNative>(plugin, args[argCount - 1], typeError);
    if (typeError || !callbackNative) {
        typeError = true;
        plugin->RaiseTypeError("argument 2 is not an object");
        goto exit;
    }

    status = httpServer->CreateObjectUrl((*socketFd)->GetFd(), httpListener, url);
    if (ER_OK == status) {
        httpListener = NULL; /* httpServer now owns httpListener */
        QCC_DbgTrace(("url=%s", url.c_str()));
    }

exit:
    if (!typeError && callbackNative) {
        CallbackNative::DispatchCallback(plugin, callbackNative, status, url);
        callbackNative = NULL;
    }
    delete callbackNative;
    delete httpListener;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _SocketFdInterface::revokeObjectURL(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    bool typeError = false;
    QStatus status = ER_OK;
    qcc::String url;
    CallbackNative* callbackNative = NULL;

    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }
    url = ToDOMString(plugin, args[0], typeError);
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
    QCC_DbgTrace(("url=%s", url.c_str()));

    httpServer->RevokeObjectUrl(url);

exit:
    if (!typeError && callbackNative) {
        CallbackNative::DispatchCallback(plugin, callbackNative, status);
        callbackNative = NULL;
    }
    delete callbackNative;
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}
