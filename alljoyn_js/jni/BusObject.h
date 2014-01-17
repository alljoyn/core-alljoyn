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
#ifndef _BUSOBJECT_H
#define _BUSOBJECT_H

#include "BusAttachment.h"
#include <alljoyn/BusObject.h>

class _BusObjectListener {
  public:
    virtual ~_BusObjectListener() { }
    virtual QStatus Get(const char* ifcName, const char* propName, ajn::MsgArg& val) = 0;
    virtual QStatus Set(const char* ifcName, const char* propName, ajn::MsgArg& val) = 0;
    virtual QStatus GenerateIntrospection(bool deep, size_t indent, qcc::String& introspection) const = 0;
    virtual void ObjectRegistered() = 0;
    virtual void ObjectUnregistered() = 0;
    virtual void MethodHandler(const ajn::InterfaceDescription::Member* member, ajn::Message& message) = 0;
};

class _BusObject : public ajn::BusObject {
  public:
    _BusObject(BusAttachment& busAttachment, const char* path) :
        ajn::BusObject(path),
        busAttachment(busAttachment),
        busObjectListener(NULL) { }
    virtual ~_BusObject() { }
    void SetBusObjectListener(_BusObjectListener* busObjectListener) {
        this->busObjectListener = busObjectListener;
    }
    BusAttachment busAttachment;

    QStatus AddInterface(const ajn::InterfaceDescription& iface) {
        return ajn::BusObject::AddInterface(iface);
    }
    QStatus AddMethodHandler(const ajn::InterfaceDescription::Member* member) {
        return ajn::BusObject::AddMethodHandler(member, static_cast<ajn::MessageReceiver::MethodHandler>(&_BusObject::MethodHandler));
    }
    QStatus MethodReply(ajn::Message& msg, const ajn::MsgArg* args = NULL, size_t numArgs = 0) {
        return ajn::BusObject::MethodReply(msg, args, numArgs);
    }
    QStatus MethodReply(ajn::Message& msg, const char* error, const char* errorMessage = NULL) {
        return ajn::BusObject::MethodReply(msg, error, errorMessage);
    }
    QStatus MethodReply(ajn::Message& msg, QStatus status) {
        return ajn::BusObject::MethodReply(msg, status);
    }
    QStatus Signal(const char* destination, ajn::SessionId sessionId, const ajn::InterfaceDescription::Member& signal, const ajn::MsgArg* args = NULL, size_t numArgs = 0, uint16_t timeToLive = 0, uint8_t flags = 0) {
        return ajn::BusObject::Signal(destination, sessionId, signal, args, numArgs, timeToLive, flags);
    }

  private:
    _BusObjectListener* busObjectListener;

    virtual QStatus Get(const char* ifcName, const char* propName, ajn::MsgArg& val) {
        return busObjectListener ? busObjectListener->Get(ifcName, propName, val) : ER_FAIL;
    }
    virtual QStatus Set(const char* ifcName, const char* propName, ajn::MsgArg& val) {
        return busObjectListener ? busObjectListener->Set(ifcName, propName, val) : ER_FAIL;
    }
    virtual qcc::String GenerateIntrospection(bool deep = false, size_t indent = 0) const {
        qcc::String introspection;
        if (!busObjectListener || (ER_OK != busObjectListener->GenerateIntrospection(deep, indent, introspection))) {
            return ajn::BusObject::GenerateIntrospection(deep, indent);
        }
        return introspection;
    }
    virtual void ObjectRegistered() {
        ajn::BusObject::ObjectRegistered();
        if (busObjectListener) { busObjectListener->ObjectRegistered(); }
    }
    virtual void ObjectUnregistered() {
        ajn::BusObject::ObjectUnregistered();
        if (busObjectListener) { busObjectListener->ObjectUnregistered(); }
    }
    void MethodHandler(const ajn::InterfaceDescription::Member* member, ajn::Message& message) {
        if (busObjectListener) { busObjectListener->MethodHandler(member, message); } else { MethodReply(message, ER_FAIL); }
    }
};

typedef qcc::ManagedObj<_BusObject> BusObject;

#endif
