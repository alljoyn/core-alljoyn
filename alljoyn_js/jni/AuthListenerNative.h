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
#ifndef _AUTHLISTENERNATIVE_H
#define _AUTHLISTENERNATIVE_H

#include "CredentialsHost.h"
#include "MessageHost.h"
#include "NativeObject.h"
#include <alljoyn/AuthListener.h>
#include <qcc/String.h>

class AuthListenerNative : public NativeObject {
  public:
    AuthListenerNative(Plugin& plugin, NPObject* objectValue);
    virtual ~AuthListenerNative();

    bool onRequest(qcc::String& authMechanism, qcc::String& peerName, uint16_t authCount, qcc::String& userName,
                   uint16_t credMask, CredentialsHost& credentials);
    bool onVerify(qcc::String& authMechanism, qcc::String& peerName, CredentialsHost& credentials);
    void onSecurityViolation(QStatus status, MessageHost& message);
    void onComplete(qcc::String& authMechanism, qcc::String& peerName, bool success);
};

#endif // _AUTHLISTENERNATIVE_H