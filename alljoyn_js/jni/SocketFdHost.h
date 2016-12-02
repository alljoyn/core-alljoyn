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
#ifndef _SOCKETFDHOST_H
#define _SOCKETFDHOST_H

#include "ScriptableObject.h"

class _SocketFdHost : public ScriptableObject {
  public:
    _SocketFdHost(Plugin& plugin, qcc::SocketFd& socketFd);
    virtual ~_SocketFdHost();
    qcc::SocketFd GetFd() { return socketFd; }

  private:
    qcc::SocketFd socketFd;

    bool getFd(NPVariant* result);

    bool close(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool shutdown(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool recv(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool send(const NPVariant* args, uint32_t argCount, NPVariant* result);
};

typedef qcc::ManagedObj<_SocketFdHost> SocketFdHost;

#endif // _SOCKETFDHOST_H