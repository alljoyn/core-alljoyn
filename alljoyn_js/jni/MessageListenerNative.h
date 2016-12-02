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
#ifndef _MESSAGELISTENERNATIVE_H
#define _MESSAGELISTENERNATIVE_H

#include "MessageHost.h"
#include "NativeObject.h"
#include <alljoyn/MsgArg.h>

class MessageListenerNative : public NativeObject {
  public:
    MessageListenerNative(Plugin& plugin, NPObject* objectValue);
    virtual ~MessageListenerNative();

    void onMessage(MessageHost& message, const ajn::MsgArg* args, size_t numArgs);
};

#endif // _MESSAGELISTENERNATIVE_H