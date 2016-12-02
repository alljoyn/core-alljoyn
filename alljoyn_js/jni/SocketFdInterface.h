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
#ifndef _SOCKETFDINTERFACE_H
#define _SOCKETFDINTERFACE_H

#include "HttpServer.h"
#include "ScriptableObject.h"
#include <qcc/ManagedObj.h>

class _SocketFdInterface : public ScriptableObject {
  public:
    _SocketFdInterface(Plugin& plugin);
    virtual ~_SocketFdInterface();
    virtual bool Construct(const NPVariant* args, uint32_t argCount, NPVariant* result);

  private:
    HttpServer httpServer;

    bool createObjectURL(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool revokeObjectURL(const NPVariant* args, uint32_t argCount, NPVariant* result);
};

typedef qcc::ManagedObj<_SocketFdInterface> SocketFdInterface;

#endif // _SOCKETFDINTERFACE_H