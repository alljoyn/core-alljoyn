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
#ifndef _SESSIONLOSTLISTENERNATIVE_H
#define _SESSIONLOSTLISTENERNATIVE_H

#include "NativeObject.h"
#include <alljoyn/SessionPortListener.h>
#include <qcc/String.h>

class SessionLostListenerNative : public NativeObject {
  public:
    SessionLostListenerNative(Plugin& plugin, NPObject* objectValue);
    virtual ~SessionLostListenerNative();

    void onLost(ajn::SessionId id, ajn::SessionListener::SessionLostReason reason);
};

#endif // _SESSIONLOSTLISTENERNATIVE_H