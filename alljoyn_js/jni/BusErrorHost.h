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
#ifndef _BUSERRORHOST_H
#define _BUSERRORHOST_H

#include "ScriptableObject.h"
#include <alljoyn/Status.h>
#include <qcc/ManagedObj.h>
#include <qcc/String.h>

class _BusErrorHost : public ScriptableObject {
  public:
    _BusErrorHost(Plugin& plugin, const qcc::String& name, const qcc::String& message, QStatus code);
    _BusErrorHost(Plugin& plugin, QStatus code);
    virtual ~_BusErrorHost();
    qcc::String ToString();

  private:
    const qcc::String name;
    const qcc::String message;
    QStatus code;

    bool getName(NPVariant* result);
    bool getMessage(NPVariant* result);
    bool getCode(NPVariant* result);
};

typedef qcc::ManagedObj<_BusErrorHost> BusErrorHost;

#endif // _BUSERRORHOST_H