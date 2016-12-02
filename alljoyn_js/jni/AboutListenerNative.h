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
#ifndef _ABOUTLISTENERNATIVE_H
#define _ABOUTLISTENERNATIVE_H

#include "BusAttachmentHost.h"
#include "NativeObject.h"
#include <alljoyn/AboutListener.h>
#include <qcc/String.h>

class AboutListenerNative : public NativeObject {
  public:
    AboutListenerNative(Plugin& plugin, NPObject* objectValue);
    virtual ~AboutListenerNative();

    void onAnnounced(const qcc::String& busName, uint16_t version, ajn::SessionPort port, const ajn::MsgArg& objectDescriptionArg, const ajn::MsgArg& aboutDataArg);
};

#endif // _ABOUTLISTENERNATIVE_H