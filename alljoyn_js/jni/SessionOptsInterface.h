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
#ifndef _SESSIONOPTSINTERFACE_H
#define _SESSIONOPTSINTERFACE_H

#include "ScriptableObject.h"
#include <qcc/ManagedObj.h>

class _SessionOptsInterface : public ScriptableObject {
  public:
    static std::map<qcc::String, int32_t>& Constants();
    _SessionOptsInterface(Plugin& plugin);
    virtual ~_SessionOptsInterface();

  private:
    static std::map<qcc::String, int32_t> constants;
};

typedef qcc::ManagedObj<_SessionOptsInterface> SessionOptsInterface;

#endif // _SESSIONOPTSINTERFACE_H