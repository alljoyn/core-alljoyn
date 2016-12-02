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
#ifndef _BUSERRORINTERFACE_H
#define _BUSERRORINTERFACE_H

#include "ScriptableObject.h"
#include <qcc/ManagedObj.h>

class _BusErrorInterface : public ScriptableObject {
  public:
    static std::map<qcc::String, int32_t>& Constants();
    _BusErrorInterface(Plugin& plugin);
    virtual ~_BusErrorInterface();

  private:
    static std::map<qcc::String, int32_t> constants;
    bool getName(NPVariant* result);
    bool getMessage(NPVariant* result);
    bool getCode(NPVariant* result);
};

typedef qcc::ManagedObj<_BusErrorInterface> BusErrorInterface;

#endif // _BUSERRORINTERFACE_H