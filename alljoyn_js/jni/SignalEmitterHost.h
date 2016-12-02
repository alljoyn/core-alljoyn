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
#ifndef _SIGNALEMITTERHOST_H
#define _SIGNALEMITTERHOST_H

#include "BusObject.h"
#include "ScriptableObject.h"
#include <alljoyn/InterfaceDescription.h>

class _SignalEmitterHost : public ScriptableObject {
  public:
    _SignalEmitterHost(Plugin& plugin, BusObject& busObject);
    virtual ~_SignalEmitterHost();

  private:
    BusObject busObject;

    bool emitSignal(const NPVariant* args, uint32_t argCount, NPVariant* result);
};

typedef qcc::ManagedObj<_SignalEmitterHost> SignalEmitterHost;

#endif // _SIGNALEMITTERHOST_H