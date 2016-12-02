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
#ifndef _HTTPLISTENERNATIVE_H
#define _HTTPLISTENERNATIVE_H

#include "HttpRequestHost.h"
#include "NativeObject.h"
#include <alljoyn/MsgArg.h>

class HttpListenerNative : public NativeObject {
  public:
    HttpListenerNative(Plugin& plugin, NPObject* objectValue);
    virtual ~HttpListenerNative();

    void onRequest(HttpRequestHost& request);
};

#endif // _HTTPLISTENERNATIVE_H