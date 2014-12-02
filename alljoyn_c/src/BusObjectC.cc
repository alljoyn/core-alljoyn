/**
 * @file
 *
 * This file implements a BusObject subclass for use by the C API
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

#include <alljoyn/BusObject.h>
#include <alljoyn_c/BusObject.h>
#include <map>
#include "DeferredCallback.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

using namespace qcc;
using namespace std;

namespace ajn {

class BusObjectC : public BusObject {
  public:
    BusObjectC(const char* path, QCC_BOOL isPlaceholder, \
               const alljoyn_busobject_callbacks* callbacks_in, const void* context_in) :
        BusObject(path, isPlaceholder == QCC_TRUE ? true : false),
        callbacks()
    {
        context = context_in;
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks_in == NULL) {
            callbacks.property_get = NULL;
            callbacks.property_set = NULL;
            callbacks.object_registered = NULL;
            callbacks.object_unregistered = NULL;
        } else {
            memcpy(&callbacks, callbacks_in, sizeof(alljoyn_busobject_callbacks));
        }
    }

    QStatus MethodReplyC(alljoyn_message msg, const alljoyn_msgarg args, size_t numArgs)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        return MethodReply(*((Message*)msg), (const MsgArg*)args, numArgs);
    }

    QStatus MethodReplyC(alljoyn_message msg, const char* error, const char* errorMessage)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        return MethodReply(*((Message*)msg), error, errorMessage);
    }

    QStatus MethodReplyC(alljoyn_message msg, QStatus status)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        return MethodReply(*((Message*)msg), status);
    }

    QStatus SignalC(const char* destination,
                    alljoyn_sessionid sessionId,
                    const InterfaceDescription::Member& signal,
                    const alljoyn_msgarg args,
                    size_t numArgs,
                    uint16_t timeToLive,
                    uint8_t flags,
                    alljoyn_message msg)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        return this->Signal(destination,
                            *(ajn::SessionId*)&sessionId,
                            signal,
                            (const ajn::MsgArg*)args,
                            numArgs,
                            timeToLive,
                            flags,
                            (ajn::Message*)msg);
    }

    void EmitPropChangedC(const char* ifcName, const char* propName, alljoyn_msgarg val, alljoyn_sessionid id)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        this->EmitPropChanged(ifcName, propName, *((ajn::MsgArg*) val), id);
    }

    void EmitPropChangedC(const char* ifcName, const char** propNames, size_t numProps, alljoyn_sessionid id)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        this->EmitPropChanged(ifcName, propNames, numProps, id);
    }

    QStatus AddInterfaceC(const alljoyn_interfacedescription iface, alljoyn_about_announceflag isAnnounced = (alljoyn_about_announceflag)ANNOUNCED)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        return AddInterface(*(const InterfaceDescription*)iface, (BusObject::AnnounceFlag)isAnnounced);
    }

    QStatus AddMethodHandlerC(const alljoyn_interfacedescription_member member, alljoyn_messagereceiver_methodhandler_ptr handler, void* context)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        callbackMap.insert(pair<const ajn::InterfaceDescription::Member*, alljoyn_messagereceiver_methodhandler_ptr>(
                               (const ajn::InterfaceDescription::Member*)member.internal_member, handler));
        QStatus ret = AddMethodHandler((const ajn::InterfaceDescription::Member*)member.internal_member,
                                       static_cast<MessageReceiver::MethodHandler>(&BusObjectC::MethodHandlerRemap),
                                       context);
        return ret;
    }

    QStatus AddMethodHandlersC(const alljoyn_busobject_methodentry* entries, size_t numEntries)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        MethodEntry* proper_entries = new MethodEntry[numEntries];

        for (size_t i = 0; i < numEntries; i++) {
            proper_entries[i].member = (const ajn::InterfaceDescription::Member*)entries[i].member->internal_member;
            callbackMap.insert(pair<const ajn::InterfaceDescription::Member*, alljoyn_messagereceiver_methodhandler_ptr>(
                                   (const ajn::InterfaceDescription::Member*)entries[i].member->internal_member,
                                   entries[i].method_handler));
            proper_entries[i].handler = static_cast<MessageReceiver::MethodHandler>(&BusObjectC::MethodHandlerRemap);
        }
        QStatus ret = AddMethodHandlers(proper_entries, numEntries);
        delete [] proper_entries;
        return ret;
    }

  protected:
    virtual QStatus Get(const char* ifcName, const char* propName, MsgArg& val)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        QStatus ret = ER_BUS_NO_SUCH_PROPERTY;
        if (callbacks.property_get != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                ret = callbacks.property_get(context, ifcName, propName, (alljoyn_msgarg)(&val));
            } else {
                DeferredCallback_4<QStatus, const void*, const char*, const char*, alljoyn_msgarg>* dcb =
                    new DeferredCallback_4<QStatus, const void*, const char*, const char*, alljoyn_msgarg>(callbacks.property_get, context, ifcName, propName, (alljoyn_msgarg)(&val));
                ret = DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
        return ret;
    }

    virtual QStatus Set(const char* ifcName, const char* propName, MsgArg& val)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        QStatus ret = ER_BUS_NO_SUCH_PROPERTY;
        if (callbacks.property_set != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                ret = callbacks.property_set(context, ifcName, propName, (alljoyn_msgarg)(&val));
            } else {
                DeferredCallback_4<QStatus, const void*, const char*, const char*, alljoyn_msgarg>* dcb =
                    new DeferredCallback_4<QStatus, const void*, const char*, const char*, alljoyn_msgarg>(callbacks.property_set, context, ifcName, propName, (alljoyn_msgarg)(&val));
                ret = DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
        return ret;
    }

    // TODO: Need to do GenerateIntrospection?

    virtual void ObjectRegistered(void)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        if (callbacks.object_registered != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.object_registered(context);
            } else {
                DeferredCallback_1<void, const void*>* dcb =
                    new DeferredCallback_1<void, const void*>(callbacks.object_registered, context);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

    virtual void ObjectUnregistered(void)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        // Call parent as per docs
        BusObject::ObjectUnregistered();

        if (callbacks.object_unregistered != NULL) {
            if (!DeferredCallback::sMainThreadCallbacksOnly) {
                callbacks.object_unregistered(context);
            } else {
                DeferredCallback_1<void, const void*>* dcb =
                    new DeferredCallback_1<void, const void*>(callbacks.object_unregistered, context);
                DEFERRED_CALLBACK_EXECUTE(dcb);
            }
        }
    }

    // TODO: Need to do GetProp, SetProp, GetAllProps, Introspect?

  private:
    void MethodHandlerRemap(const InterfaceDescription::Member* member, Message& message)
    {
        QCC_DbgTrace(("%s", __FUNCTION__));
        /* Populate a C member description for use in the callback */
        alljoyn_interfacedescription_member c_member;

        c_member.iface = (alljoyn_interfacedescription)member->iface;
        c_member.memberType = (alljoyn_messagetype)member->memberType;
        c_member.name = member->name.c_str();
        c_member.signature = member->signature.c_str();
        c_member.returnSignature = member->returnSignature.c_str();
        c_member.argNames = member->argNames.c_str();
        // TODO Add back in annotations
        //c_member.annotation = member->annotation;
        c_member.internal_member = member;

        /* Look up the C callback via map and invoke */
        alljoyn_messagereceiver_methodhandler_ptr remappedHandler = callbackMap[member];
        if (!DeferredCallback::sMainThreadCallbacksOnly) {
            remappedHandler((alljoyn_busobject) this, &c_member, (alljoyn_message)(&message));
        } else {
            DeferredCallback_3<void, alljoyn_busobject, const alljoyn_interfacedescription_member*, alljoyn_message>* dcb =
                new DeferredCallback_3<void, alljoyn_busobject, const alljoyn_interfacedescription_member*, alljoyn_message>(remappedHandler, (alljoyn_busobject) this, &c_member, (alljoyn_message)(&message));
            DEFERRED_CALLBACK_EXECUTE(dcb);
        }
    }

    map<const ajn::InterfaceDescription::Member*, alljoyn_messagereceiver_methodhandler_ptr> callbackMap;
    alljoyn_busobject_callbacks callbacks;
    const void* context;
};
}

struct _alljoyn_busobject_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

alljoyn_busobject AJ_CALL alljoyn_busobject_create(const char* path, QCC_BOOL isPlaceholder,
                                                   const alljoyn_busobject_callbacks* callbacks_in, const void* context_in)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (alljoyn_busobject) new ajn::BusObjectC(path, isPlaceholder, callbacks_in, context_in);
}

void AJ_CALL alljoyn_busobject_destroy(alljoyn_busobject bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    delete (ajn::BusObjectC*)bus;
}

const char* AJ_CALL alljoyn_busobject_getpath(alljoyn_busobject bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->GetPath();
}


void AJ_CALL alljoyn_busobject_emitpropertychanged(alljoyn_busobject bus,
                                                   const char* ifcName,
                                                   const char* propName,
                                                   alljoyn_msgarg val,
                                                   alljoyn_sessionid id)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::BusObjectC*)bus)->EmitPropChangedC(ifcName, propName, val, id);
}

void AJ_CALL alljoyn_busobject_emitpropertieschanged(alljoyn_busobject bus,
                                                     const char* ifcName,
                                                     const char** propNames,
                                                     size_t numProps,
                                                     alljoyn_sessionid id)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::BusObjectC*)bus)->EmitPropChangedC(ifcName, propNames, numProps, id);
}

size_t AJ_CALL alljoyn_busobject_getname(alljoyn_busobject bus, char* buffer, size_t bufferSz)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    qcc::String name = ((ajn::BusObjectC*)bus)->GetName();
    if (buffer != NULL && bufferSz > 0) {
        strncpy(buffer, name.c_str(), bufferSz);
        //prevent sting not being nul terminated.
        buffer[bufferSz - 1] = '\0';
    }
    return name.length() + 1;
}

QStatus AJ_CALL alljoyn_busobject_addinterface(alljoyn_busobject bus, const alljoyn_interfacedescription iface)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->AddInterfaceC(iface, UNANNOUNCED);
}

QStatus AJ_CALL alljoyn_busobject_addmethodhandler(alljoyn_busobject bus, const alljoyn_interfacedescription_member member, alljoyn_messagereceiver_methodhandler_ptr handler, void* context)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->AddMethodHandlerC(member, handler, context);
}

QStatus AJ_CALL alljoyn_busobject_addmethodhandlers(alljoyn_busobject bus, const alljoyn_busobject_methodentry* entries, size_t numEntries)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->AddMethodHandlersC(entries, numEntries);
}

QStatus AJ_CALL alljoyn_busobject_methodreply_args(alljoyn_busobject bus, alljoyn_message msg,
                                                   const alljoyn_msgarg args, size_t numArgs)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->MethodReplyC(msg, args, numArgs);
}

QStatus AJ_CALL alljoyn_busobject_methodreply_err(alljoyn_busobject bus, alljoyn_message msg,
                                                  const char* error, const char* errorMessage)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->MethodReplyC(msg, error, errorMessage);
}

QStatus AJ_CALL alljoyn_busobject_methodreply_status(alljoyn_busobject bus, alljoyn_message msg, QStatus status)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->MethodReplyC(msg, status);
}

const alljoyn_busattachment AJ_CALL alljoyn_busobject_getbusattachment(alljoyn_busobject bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (const alljoyn_busattachment) &((ajn::BusObjectC*)bus)->GetBusAttachment();
}

QStatus AJ_CALL alljoyn_busobject_signal(alljoyn_busobject bus,
                                         const char* destination,
                                         alljoyn_sessionid sessionId,
                                         const alljoyn_interfacedescription_member signal,
                                         const alljoyn_msgarg args,
                                         size_t numArgs,
                                         uint16_t timeToLive,
                                         uint8_t flags,
                                         alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    const ajn::InterfaceDescription::Member* member = ((ajn::InterfaceDescription*)signal.iface)->GetMember(signal.name);
    if (!member) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }

    /* must call the Signal Method through BusObjectC since Signal is a protected Method */
    return ((ajn::BusObjectC*)bus)->SignalC(
               destination,
               sessionId,
               *member,
               args,
               numArgs,
               timeToLive,
               flags,
               msg);
}

QStatus AJ_CALL alljoyn_busobject_cancelsessionlessmessage_serial(alljoyn_busobject bus, uint32_t serialNumber)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->CancelSessionlessMessage(serialNumber);
}

QStatus AJ_CALL alljoyn_busobject_cancelsessionlessmessage(alljoyn_busobject bus, const alljoyn_message msg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->CancelSessionlessMessage(*((ajn::Message*)msg));
}

QCC_BOOL AJ_CALL alljoyn_busobject_issecure(alljoyn_busobject bus)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return (((ajn::BusObjectC*)bus)->IsSecure() == true ? QCC_TRUE : QCC_FALSE);
}

/**
 * This function is experimental, and as such has not been fully tested.
 * Please help make it more robust by contributing fixes if you find problems.
 */
size_t AJ_CALL alljoyn_busobject_getannouncedinterfacenames(alljoyn_busobject bus,
                                                            const char** interfaces,
                                                            size_t numInterfaces)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->GetAnnouncedInterfaceNames(interfaces, numInterfaces);
}

/**
 * This function is experimental, and as such has not been fully tested.
 * Please help make it more robust by contributing fixes if you find problems.
 */
QStatus AJ_CALL alljoyn_busobject_setannounceflag(alljoyn_busobject bus,
                                                  const alljoyn_interfacedescription iface,
                                                  alljoyn_about_announceflag isAnnounced)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->SetAnnounceFlag((ajn::InterfaceDescription*)iface, (ajn::BusObject::AnnounceFlag)isAnnounced);
}

/**
 * This function is experimental, and as such has not been fully tested.
 * Please help make it more robust by contributing fixes if you find problems.
 */
QStatus AJ_CALL alljoyn_busobject_addinterface_announced(alljoyn_busobject bus,
                                                         const alljoyn_interfacedescription iface)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::BusObjectC*)bus)->AddInterfaceC(iface, ANNOUNCED);
}


