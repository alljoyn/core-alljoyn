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
#ifndef _CALLBACKNATIVE_H
#define _CALLBACKNATIVE_H

#include "BusErrorHost.h"
#include "InterfaceDescriptionNative.h"
#include "MessageHost.h"
#include "NativeObject.h"
#include "PluginData.h"
#include "ProxyBusObjectHost.h"
#include "SessionOptsHost.h"
#include "SocketFdHost.h"
#include <alljoyn/Session.h>
#include <vector>

class CallbackNative : public NativeObject {
  public:
    CallbackNative(Plugin& plugin, NPObject* objectValue);
    virtual ~CallbackNative();

    void onCallback(QStatus status);
    void onCallback(QStatus status, bool b);
    void onCallback(QStatus status, qcc::String& s);
    void onCallback(QStatus status, uint32_t u);
    void onCallback(QStatus status, ajn::SessionId id, SessionOptsHost& opts);
    void onCallback(QStatus status, ajn::SessionPort port);
    void onCallback(QStatus status, MessageHost& message, const ajn::MsgArg* args, size_t numArgs);
    void onCallback(QStatus status, ProxyBusObjectHost& proxyBusObject);
    void onCallback(QStatus status, SocketFdHost& socketFd);
    void onCallback(QStatus status, InterfaceDescriptionNative* interfaceDescription);
    void onCallback(QStatus status, InterfaceDescriptionNative** interfaceDescription, size_t numInterfaces);
    void onCallback(QStatus status, std::vector<ProxyBusObjectHost>& children);
    void onCallback(BusErrorHost& busError);

    static void DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status);
    static void DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, bool b);
    static void DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, qcc::String& s);
    static void DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, uint32_t u);
    static void DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, ajn::SessionPort port);
    static void DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, ProxyBusObjectHost& proxyBusObject);
    static void DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, SocketFdHost& socketFd);
    static void DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, InterfaceDescriptionNative* interfaceDescription);
    static void DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, InterfaceDescriptionNative** interfaceDescription, size_t numInterfaces);
    static void DispatchCallback(Plugin& plugin, CallbackNative* callbackNative, QStatus status, std::vector<ProxyBusObjectHost>& children);

  private:
    static void _StatusCallbackCB(PluginData::CallbackContext* ctx);
    static void _BoolCallbackCB(PluginData::CallbackContext* ctx);
    static void _StringCallbackCB(PluginData::CallbackContext* ctx);
    static void _UnsignedLongCallbackCB(PluginData::CallbackContext* ctx);
    static void _BindSessionPortCallbackCB(PluginData::CallbackContext* ctx);
    static void _GetProxyBusObjectCallbackCB(PluginData::CallbackContext* ctx);
    static void _GetSessionFdCallbackCB(PluginData::CallbackContext* ctx);
    static void _GetInterfaceCallbackCB(PluginData::CallbackContext* ctx);
    static void _GetInterfacesCallbackCB(PluginData::CallbackContext* ctx);
    static void _GetChildrenCallbackCB(PluginData::CallbackContext* ctx);
};

#endif // _CALLBACKNATIVE_H
