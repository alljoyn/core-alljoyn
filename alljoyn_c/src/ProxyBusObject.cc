/**
 * @file
 *
 * This file implements the ProxyBusObject class.
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

#include <assert.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn_c/ProxyBusObject.h>
#include "BusAttachmentC.h"
#include "MessageReceiverC.h"
#include "ProxyBusObjectListenerC.h"
#include <qcc/Debug.h>
#include <qcc/Mutex.h>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_proxybusobject_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

/* static instances of classes used for Async CallBacks */
static ajn::MessageReceiverC msgReceverC;
static ajn::ProxyBusObjectListenerC proxyObjListener;

namespace std {
// Specialized comparator for properties change listeners.
template <>
struct less<ajn::ProxyBusObjectPropertiesChangedListenerC*> {
    bool operator()(const ajn::ProxyBusObjectPropertiesChangedListenerC* lhs, const ajn::ProxyBusObjectPropertiesChangedListenerC* rhs) const
    {
        return (intptr_t)lhs->GetSignalHandler() < (intptr_t)lhs->GetSignalHandler();
    }
};
}

class ProxyBusObjectC : public ajn::ProxyBusObject {

    // private typedefs to make some of the code lines shorter and more readable.
    typedef std::multimap<ajn::ProxyBusObjectPropertiesChangedListenerC*, char*> ListenerMap;
    typedef std::pair<ajn::ProxyBusObjectPropertiesChangedListenerC*, char*> ListenerMapEntry;

  public:
    ProxyBusObjectC() : ajn::ProxyBusObject() { }
    ProxyBusObjectC(ajn::BusAttachment& bus, const char* service, const char* path, ajn::SessionId sessionId, bool secure = false) :
        ProxyBusObject(bus, service, path, sessionId, secure)
    {
    }

    ~ProxyBusObjectC()
    {
        /*
         * Don't bother locking the mutex in the destructor.  If the
         * mutex were to block then the calling code has much bigger
         * problems since it is destroying an object that is currently
         * in use.
         */
        while (!listeners.empty()) {
            free(listeners.begin()->second);
            delete listeners.begin()->first;
            listeners.erase(listeners.begin());
        }
    }

    ajn::ProxyBusObjectPropertiesChangedListenerC* AddListener(const char* iface, alljoyn_proxybusobject_listener_propertieschanged_ptr callback)
    {
        ajn::ProxyBusObjectPropertiesChangedListenerC* propListener = new ajn::ProxyBusObjectPropertiesChangedListenerC(callback);
        lock.Lock();
        listeners.insert(ListenerMapEntry(propListener, strdup(iface)));
        lock.Unlock();
        return propListener;
    }

    ajn::ProxyBusObjectPropertiesChangedListenerC* RemoveListener(const char* iface, alljoyn_proxybusobject_listener_propertieschanged_ptr callback)
    {
        ajn::ProxyBusObjectPropertiesChangedListenerC* propListener = NULL;
        ajn::ProxyBusObjectPropertiesChangedListenerC lookupPropListener(callback);
        lock.Lock();
        ListenerMap::iterator it = listeners.lower_bound(&lookupPropListener);
        ListenerMap::iterator end = listeners.upper_bound(&lookupPropListener);
        while ((it != end) && (strcmp(iface, it->second) != 0)) {
            ++it;
        }
        if (it != end) {
            free(it->second);
            propListener = it->first;
            listeners.erase(it);
        }
        lock.Unlock();
        return propListener;
    }

  private:

    ListenerMap listeners;
    mutable qcc::Mutex lock;
};


alljoyn_proxybusobject AJ_CALL alljoyn_proxybusobject_create(alljoyn_busattachment bus, const char* service,
                                                             const char* path, alljoyn_sessionid sessionId)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ProxyBusObjectC* ret = new ProxyBusObjectC(*((ajn::BusAttachmentC*)bus), service, path, sessionId);
    return (alljoyn_proxybusobject)ret;
}

alljoyn_proxybusobject AJ_CALL alljoyn_proxybusobject_create_secure(alljoyn_busattachment bus, const char* service,
                                                                    const char* path, alljoyn_sessionid sessionId)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ProxyBusObjectC* ret = new ProxyBusObjectC(*((ajn::BusAttachmentC*)bus), service, path, sessionId, true);
    return (alljoyn_proxybusobject)ret;
}

void AJ_CALL alljoyn_proxybusobject_destroy(alljoyn_proxybusobject proxyObj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(proxyObj != NULL && "NULL parameter passed to alljoyn_proxybusobject_destroy.");

    // Clean up the C++ ProxyBusObject before cleaning up the listeners.
    delete (ProxyBusObjectC*)proxyObj;
}

QStatus AJ_CALL alljoyn_proxybusobject_addinterface(alljoyn_proxybusobject proxyObj, const alljoyn_interfacedescription iface)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->AddInterface(*((const ajn::InterfaceDescription*)iface));
}

QStatus AJ_CALL alljoyn_proxybusobject_addinterface_by_name(alljoyn_proxybusobject proxyObj, const char* name)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->AddInterface(name);
}

size_t AJ_CALL alljoyn_proxybusobject_getchildren(alljoyn_proxybusobject proxyObj, alljoyn_proxybusobject* children, size_t numChildren)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->GetChildren((ajn::ProxyBusObject**)children, numChildren);
}

alljoyn_proxybusobject AJ_CALL alljoyn_proxybusobject_getchild(alljoyn_proxybusobject proxyObj, const char* path)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_proxybusobject)((ProxyBusObjectC*)proxyObj)->GetChild(path);
}

QStatus AJ_CALL alljoyn_proxybusobject_addchild(alljoyn_proxybusobject proxyObj, const alljoyn_proxybusobject child)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->AddChild(*(ProxyBusObjectC*)child);
}

QStatus AJ_CALL alljoyn_proxybusobject_removechild(alljoyn_proxybusobject proxyObj, const char* path)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->RemoveChild(path);
}

QStatus AJ_CALL alljoyn_proxybusobject_introspectremoteobject(alljoyn_proxybusobject proxyObj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->IntrospectRemoteObject();
}

QStatus AJ_CALL alljoyn_proxybusobject_introspectremoteobjectasync(alljoyn_proxybusobject proxyObj, alljoyn_proxybusobject_listener_introspectcb_ptr callback, void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    /*
     * The new ajn::IntrospectCallbackContext must be freed in inside the
     * ajn::ProxyBusObjectListenerC::IntrospectCB.
     */
    return ((ProxyBusObjectC*)proxyObj)->IntrospectRemoteObjectAsync(&proxyObjListener,
                                                                     static_cast<ajn::ProxyBusObject::Listener::IntrospectCB>(&ajn::ProxyBusObjectListenerC::IntrospectCB),
                                                                     (void*) new ajn::IntrospectCallbackContext(callback, context));
}

QStatus AJ_CALL alljoyn_proxybusobject_getproperty(alljoyn_proxybusobject proxyObj, const char* iface, const char* property, alljoyn_msgarg value)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::MsgArg* reply = (ajn::MsgArg*)&(*value);
    return ((ProxyBusObjectC*)proxyObj)->GetProperty(iface, property, *reply);
}

QStatus AJ_CALL alljoyn_proxybusobject_getpropertyasync(alljoyn_proxybusobject proxyObj,
                                                        const char* iface,
                                                        const char* property,
                                                        alljoyn_proxybusobject_listener_getpropertycb_ptr callback,
                                                        uint32_t timeout,
                                                        void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    /*
     * The new ajn::GetPropertyCallbackContext must be freed in inside the
     * ajn::ProxyBusObjectListenerC::GetPropertyCB.
     */
    return ((ProxyBusObjectC*)proxyObj)->GetPropertyAsync(iface, property,
                                                          &proxyObjListener,
                                                          static_cast<ajn::ProxyBusObject::Listener::GetPropertyCB>(&ajn::ProxyBusObjectListenerC::GetPropertyCB),
                                                          (void*) new ajn::GetPropertyCallbackContext(callback, context),
                                                          timeout);

}
QStatus AJ_CALL alljoyn_proxybusobject_getallproperties(alljoyn_proxybusobject proxyObj, const char* iface, alljoyn_msgarg values)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::MsgArg* reply = (ajn::MsgArg*)&(*values);
    return ((ProxyBusObjectC*)proxyObj)->GetAllProperties(iface, *reply);
}

QStatus AJ_CALL alljoyn_proxybusobject_getallpropertiesasync(alljoyn_proxybusobject proxyObj,
                                                             const char* iface,
                                                             alljoyn_proxybusobject_listener_getallpropertiescb_ptr callback,
                                                             uint32_t timeout,
                                                             void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    /*
     * The new ajn::GetAllPropertiesCallbackContext must be freed in inside the
     * ajn::ProxyBusObjectListenerC::GetAllPropertiesCB.
     */
    return ((ProxyBusObjectC*)proxyObj)->GetAllPropertiesAsync(iface,
                                                               &proxyObjListener,
                                                               static_cast<ajn::ProxyBusObject::Listener::GetAllPropertiesCB>(&ajn::ProxyBusObjectListenerC::GetAllPropertiesCB),
                                                               (void*) new ajn::GetPropertyCallbackContext(callback, context),
                                                               timeout);
}

QStatus AJ_CALL alljoyn_proxybusobject_setproperty(alljoyn_proxybusobject proxyObj, const char* iface, const char* property, alljoyn_msgarg value)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::MsgArg* reply = (ajn::MsgArg*)&(*value);
    return ((ProxyBusObjectC*)proxyObj)->SetProperty(iface, property, *reply);
}

QStatus AJ_CALL alljoyn_proxybusobject_setpropertyasync(alljoyn_proxybusobject proxyObj,
                                                        const char* iface,
                                                        const char* property,
                                                        alljoyn_msgarg value,
                                                        alljoyn_proxybusobject_listener_setpropertycb_ptr callback,
                                                        uint32_t timeout,
                                                        void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    /*
     * The new ajn::GetPropertyCallbackContext must be freed in inside the
     * ajn::ProxyBusObjectListenerC::GetPropertyCB.
     */
    return ((ProxyBusObjectC*)proxyObj)->SetPropertyAsync(iface, property,
                                                          *(ajn::MsgArg*)value,
                                                          &proxyObjListener,
                                                          static_cast<ajn::ProxyBusObject::Listener::SetPropertyCB>(&ajn::ProxyBusObjectListenerC::SetPropertyCB),
                                                          (void*) new ajn::SetPropertyCallbackContext(callback, context),
                                                          timeout);
}

QStatus AJ_CALL alljoyn_proxybusobject_registerpropertieschangedlistener(alljoyn_proxybusobject proxyObj,
                                                                         const char* iface,
                                                                         const char** properties,
                                                                         size_t propertiesSize,
                                                                         alljoyn_proxybusobject_listener_propertieschanged_ptr callback,
                                                                         void* context)
{
    ajn::ProxyBusObjectPropertiesChangedListenerC* propChangeListener = ((ProxyBusObjectC*)proxyObj)->AddListener(iface, callback);
    return ((ProxyBusObjectC*)proxyObj)->RegisterPropertiesChangedListener(iface, properties, propertiesSize, *propChangeListener, context);
}

QStatus AJ_CALL alljoyn_proxybusobject_unregisterpropertieschangedlistener(alljoyn_proxybusobject proxyObj,
                                                                           const char* iface,
                                                                           alljoyn_proxybusobject_listener_propertieschanged_ptr callback)
{
    QStatus status = ER_FAIL;
    ajn::ProxyBusObjectPropertiesChangedListenerC* propChangeListener = ((ProxyBusObjectC*)proxyObj)->RemoveListener(iface, callback);

    if (propChangeListener) {
        status = ((ProxyBusObjectC*)proxyObj)->UnregisterPropertiesChangedListener(iface, *propChangeListener);
        delete propChangeListener;
    }
    return status;
}

QStatus AJ_CALL alljoyn_proxybusobject_methodcall(alljoyn_proxybusobject obj,
                                                  const char* ifaceName,
                                                  const char* methodName,
                                                  alljoyn_msgarg args,
                                                  size_t numArgs,
                                                  alljoyn_message replyMsg,
                                                  uint32_t timeout,
                                                  uint8_t flags)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::Message* reply = (ajn::Message*)&(*replyMsg);
    return ((ProxyBusObjectC*)obj)->MethodCall(ifaceName, methodName, (const ajn::MsgArg*)args,
                                               numArgs, *reply, timeout, flags);
}

QStatus AJ_CALL alljoyn_proxybusobject_methodcall_member(alljoyn_proxybusobject proxyObj,
                                                         const alljoyn_interfacedescription_member method,
                                                         const alljoyn_msgarg args,
                                                         size_t numArgs,
                                                         alljoyn_message replyMsg,
                                                         uint32_t timeout,
                                                         uint8_t flags)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::Message* reply = (ajn::Message*)&(*replyMsg);
    return ((ProxyBusObjectC*)proxyObj)->MethodCall(*(const ajn::InterfaceDescription::Member*)(method.internal_member),
                                                    (const ajn::MsgArg*)args, numArgs, *reply, timeout, flags);
}

QStatus AJ_CALL alljoyn_proxybusobject_methodcall_noreply(alljoyn_proxybusobject proxyObj,
                                                          const char* ifaceName,
                                                          const char* methodName,
                                                          const alljoyn_msgarg args,
                                                          size_t numArgs,
                                                          uint8_t flags)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->MethodCall(ifaceName, methodName, (const ajn::MsgArg*)args, numArgs, flags);
}

QStatus AJ_CALL alljoyn_proxybusobject_methodcall_member_noreply(alljoyn_proxybusobject proxyObj,
                                                                 const alljoyn_interfacedescription_member method,
                                                                 const alljoyn_msgarg args,
                                                                 size_t numArgs,
                                                                 uint8_t flags)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->MethodCall(*(const ajn::InterfaceDescription::Member*)(method.internal_member),
                                                    (const ajn::MsgArg*)args, numArgs, flags);
}

QStatus AJ_CALL alljoyn_proxybusobject_methodcallasync(alljoyn_proxybusobject proxyObj,
                                                       const char* ifaceName,
                                                       const char* methodName,
                                                       alljoyn_messagereceiver_replyhandler_ptr replyFunc,
                                                       const alljoyn_msgarg args,
                                                       size_t numArgs,
                                                       void* context,
                                                       uint32_t timeout,
                                                       uint8_t flags)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    /*
     * the instance of the ajn::MessageReceiverReplyHandlerCallbackContext is
     * freed when the message ReplyHandler is called.
     */
    return ((ProxyBusObjectC*)proxyObj)->MethodCallAsync(ifaceName,
                                                         methodName,
                                                         &msgReceverC,
                                                         static_cast<ajn::MessageReceiver::ReplyHandler>(&ajn::MessageReceiverC::ReplyHandler),
                                                         (const ajn::MsgArg*)args,
                                                         numArgs,
                                                         new ajn::MessageReceiverReplyHandlerCallbackContext(replyFunc, context),
                                                         timeout,
                                                         flags);
}

QStatus AJ_CALL alljoyn_proxybusobject_methodcallasync_member(alljoyn_proxybusobject proxyObj,
                                                              const alljoyn_interfacedescription_member method,
                                                              /* MessageReceiver* receiver,*/
                                                              alljoyn_messagereceiver_replyhandler_ptr replyFunc,
                                                              const alljoyn_msgarg args,
                                                              size_t numArgs,
                                                              void* context,
                                                              uint32_t timeout,
                                                              uint8_t flags)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->MethodCallAsync(*(const ajn::InterfaceDescription::Member*)(method.internal_member),
                                                         &msgReceverC,
                                                         static_cast<ajn::MessageReceiver::ReplyHandler>(&ajn::MessageReceiverC::ReplyHandler),
                                                         (const ajn::MsgArg*)args,
                                                         numArgs,
                                                         new ajn::MessageReceiverReplyHandlerCallbackContext(replyFunc, context),
                                                         timeout,
                                                         flags);
}

QStatus AJ_CALL alljoyn_proxybusobject_parsexml(alljoyn_proxybusobject proxyObj, const char* xml, const char* identifier)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->ParseXml(xml, identifier);
}

QStatus AJ_CALL alljoyn_proxybusobject_secureconnection(alljoyn_proxybusobject proxyObj, QCC_BOOL forceAuth)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->SecureConnection(forceAuth);
}

QStatus AJ_CALL alljoyn_proxybusobject_secureconnectionasync(alljoyn_proxybusobject proxyObj, QCC_BOOL forceAuth)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->SecureConnectionAsync(forceAuth);
}
const alljoyn_interfacedescription AJ_CALL alljoyn_proxybusobject_getinterface(alljoyn_proxybusobject proxyObj, const char* iface)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (const alljoyn_interfacedescription)((ProxyBusObjectC*)proxyObj)->GetInterface(iface);
}

size_t AJ_CALL alljoyn_proxybusobject_getinterfaces(alljoyn_proxybusobject proxyObj, const alljoyn_interfacedescription* ifaces, size_t numIfaces)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->GetInterfaces(((const ajn::InterfaceDescription**)ifaces), numIfaces);
}

const char* AJ_CALL alljoyn_proxybusobject_getpath(alljoyn_proxybusobject proxyObj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->GetPath().c_str();
}

const char* AJ_CALL alljoyn_proxybusobject_getservicename(alljoyn_proxybusobject proxyObj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->GetServiceName().c_str();
}

alljoyn_sessionid AJ_CALL alljoyn_proxybusobject_getsessionid(alljoyn_proxybusobject proxyObj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_sessionid)((ProxyBusObjectC*)proxyObj)->GetSessionId();
}

QCC_BOOL AJ_CALL alljoyn_proxybusobject_implementsinterface(alljoyn_proxybusobject proxyObj, const char* iface)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (QCC_BOOL)((ProxyBusObjectC*)proxyObj)->ImplementsInterface(iface);
}

alljoyn_proxybusobject AJ_CALL alljoyn_proxybusobject_copy(const alljoyn_proxybusobject source)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!source) {
        return NULL;
    }
    ProxyBusObjectC* ret = new ProxyBusObjectC;
    *ret = *(ProxyBusObjectC*)source;
    return (alljoyn_proxybusobject) ret;
}

QCC_BOOL AJ_CALL alljoyn_proxybusobject_isvalid(alljoyn_proxybusobject proxyObj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (QCC_BOOL)((ProxyBusObjectC*)proxyObj)->IsValid();
}

QCC_BOOL AJ_CALL alljoyn_proxybusobject_issecure(alljoyn_proxybusobject proxyObj)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ProxyBusObjectC*)proxyObj)->IsSecure() == true ? QCC_TRUE : QCC_FALSE;
}
