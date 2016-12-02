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
#ifndef _BUSLISTENERNATIVE_H
#define _BUSLISTENERNATIVE_H

#include "BusAttachmentHost.h"
#include "NativeObject.h"
#include <alljoyn/BusListener.h>
#include <qcc/String.h>

class BusListenerNative : public NativeObject {
  public:
    BusListenerNative(Plugin& plugin, NPObject* objectValue);
    virtual ~BusListenerNative();

    void onRegistered(BusAttachmentHost& busAttachment);
    void onUnregistered();
    void onFoundAdvertisedName(const qcc::String& name, ajn::TransportMask transport, const qcc::String& namePrefix);
    void onLostAdvertisedName(const qcc::String& name, ajn::TransportMask transport, const qcc::String& namePrefix);
    void onNameOwnerChanged(const qcc::String& busName, const qcc::String& previousOwner, const qcc::String& newOwner);
    void onPropertyChanged(const qcc::String& propName, const ajn::MsgArg* propValue);
    void onStopping();
    void onDisconnected();
};

#endif // _BUSLISTENERNATIVE_H