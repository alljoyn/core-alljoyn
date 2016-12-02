/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
#ifndef _BUSOBJECTNATIVE_H
#define _BUSOBJECTNATIVE_H

#include "MessageReplyHost.h"
#include "NativeObject.h"
#include <alljoyn/Status.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/MsgArg.h>

class BusObjectNative : public NativeObject {
  public:
    BusObjectNative(Plugin& plugin, NPObject* objectValue);
    virtual ~BusObjectNative();

    void onRegistered();
    void onUnregistered();
    QStatus get(const ajn::InterfaceDescription* interface, const ajn::InterfaceDescription::Property* property, ajn::MsgArg& val);
    QStatus set(const ajn::InterfaceDescription* interface, const ajn::InterfaceDescription::Property* property, const ajn::MsgArg& val);
    QStatus toXML(bool deep, size_t indent, qcc::String& xml);
    void onMessage(const char* interfaceName, const char* methodName, MessageReplyHost& message, const ajn::MsgArg* args, size_t numArgs);
};

#endif // _BUSOBJECTNATIVE_H