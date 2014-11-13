/******************************************************************************
 * Copyright (c) 2010-2014, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/
#ifndef _ALLJOYN_C_PROXYBUSOBJECTLISTENERC_H
#define _ALLJOYN_C_PROXYBUSOBJECTLISTENERC_H
#include <alljoyn/ProxyBusObject.h>
namespace ajn {
/*
 * When setting up a asynchronous Introspection call a callback handler for C
 * alljoyn_proxybusobject_listener_introspectcb_ptr function pointer will be
 * passed in as the callback handler.  AllJoyn expects a
 * ProxyBusObject::Listener::IntrospectCB method handler.  The function handler
 * will be passed as the part of the void* context that is passed to the internal
 * callback handler and will then be used to map the internal callback handler
 * to the user defined alljoyn_proxybusobject_listener_introspectcb_ptr callback
 * function pointer.
 */
class IntrospectCallbackContext {
  public:
    IntrospectCallbackContext(alljoyn_proxybusobject_listener_introspectcb_ptr replyhandler_ptr, void* context) :
        replyhandler_ptr(replyhandler_ptr), context(context) { }

    alljoyn_proxybusobject_listener_introspectcb_ptr replyhandler_ptr;
    void* context;
};

class GetPropertyCallbackContext {
  public:
    GetPropertyCallbackContext(alljoyn_proxybusobject_listener_getpropertycb_ptr replyhandler_ptr, void* context) :
        replyhandler_ptr(replyhandler_ptr), context(context) { }

    alljoyn_proxybusobject_listener_getpropertycb_ptr replyhandler_ptr;
    void* context;
};

class GetAllPropertiesCallbackContext {
  public:
    GetAllPropertiesCallbackContext(alljoyn_proxybusobject_listener_getallpropertiescb_ptr replyhandler_ptr, void* context) :
        replyhandler_ptr(replyhandler_ptr), context(context) { }

    alljoyn_proxybusobject_listener_getallpropertiescb_ptr replyhandler_ptr;
    void* context;
};

class SetPropertyCallbackContext {
  public:
    SetPropertyCallbackContext(alljoyn_proxybusobject_listener_setpropertycb_ptr replyhandler_ptr, void* context) :
        replyhandler_ptr(replyhandler_ptr), context(context) { }

    alljoyn_proxybusobject_listener_setpropertycb_ptr replyhandler_ptr;
    void* context;
};

class PropertieschangedCallbackContext {
  public:
    PropertieschangedCallbackContext(alljoyn_proxybusobject_listener_propertieschanged_ptr signalhandler_ptr, void* context) :
        signalhandler_ptr(signalhandler_ptr), context(context) { }

    alljoyn_proxybusobject_listener_propertieschanged_ptr signalhandler_ptr;
    void* context;
};

class ProxyBusObjectListenerC : public ajn::ProxyBusObject::Listener {
  public:
    void IntrospectCB(QStatus status, ajn::ProxyBusObject* obj, void* context)
    {
        /*
         * The IntrospectCallbackContext that found in the context pointer is
         * allocated in when the user calls alljoyn_proxybusobject_introspectremoteobjectasync
         * as soon as the IntrospectCB is received this context pointer will not be
         * used again and must be freed.  we don't call delete here the instance
         * of the IntrospectCallbackContext will cause a memory leak.
         */
        IntrospectCallbackContext* in = (IntrospectCallbackContext*)context;
        in->replyhandler_ptr(status, (alljoyn_proxybusobject)obj, in->context);
        in->replyhandler_ptr = NULL;
        delete in;
    }

    void GetPropertyCB(QStatus status, ProxyBusObject* obj, const MsgArg& value, void* context)
    {
        GetPropertyCallbackContext* in = (GetPropertyCallbackContext*)context;
        in->replyhandler_ptr(status, (alljoyn_proxybusobject)obj, (alljoyn_msgarg)(&value), in->context);
        in->replyhandler_ptr = NULL;
        delete in;
    }

    void GetAllPropertiesCB(QStatus status, ProxyBusObject* obj, const MsgArg& value, void* context)
    {
        GetAllPropertiesCallbackContext* in = (GetAllPropertiesCallbackContext*)context;
        in->replyhandler_ptr(status, (alljoyn_proxybusobject)obj, (alljoyn_msgarg)(&value), in->context);
        in->replyhandler_ptr = NULL;
        delete in;
    }

    void SetPropertyCB(QStatus status, ProxyBusObject* obj, void* context)
    {
        SetPropertyCallbackContext* in = (SetPropertyCallbackContext*)context;
        in->replyhandler_ptr(status, (alljoyn_proxybusobject)obj, in->context);
        in->replyhandler_ptr = NULL;
        delete in;
    }
};

class ProxyBusObjectPropertiesChangedListenerC : public ajn::ProxyBusObject::PropertiesChangedListener {
  public:
    ProxyBusObjectPropertiesChangedListenerC(alljoyn_proxybusobject_listener_propertieschanged_ptr signalhandler_ptr) : signalhandler_ptr(signalhandler_ptr) { }
    void PropertiesChanged(ProxyBusObject& obj, const char* ifaceName, const MsgArg& changed, const MsgArg& invalidated, void* context)
    {
        signalhandler_ptr((alljoyn_proxybusobject)(&obj), ifaceName, (alljoyn_msgarg)(&changed), (alljoyn_msgarg)(&invalidated), context);
    }

    const alljoyn_proxybusobject_listener_propertieschanged_ptr GetSignalHandler() const { return signalhandler_ptr; }

  private:
    alljoyn_proxybusobject_listener_propertieschanged_ptr signalhandler_ptr;
};
}
#endif
