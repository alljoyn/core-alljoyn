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
#ifndef _ACCEPTSESSIONJOINERLISTENERNATIVE_H
#define _ACCEPTSESSIONJOINERLISTENERNATIVE_H

#include "NativeObject.h"
#include "SessionOptsHost.h"
#include <qcc/String.h>

class AcceptSessionJoinerListenerNative : public NativeObject {
  public:
    AcceptSessionJoinerListenerNative(Plugin& plugin, NPObject* objectValue);
    virtual ~AcceptSessionJoinerListenerNative();

    bool onAccept(ajn::SessionPort port, const qcc::String& joiner, SessionOptsHost& opts);
};

#endif // _ACCEPTSESSIONJOINERLISTENERNATIVE_H