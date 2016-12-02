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
#ifndef _HTTPREQUESTHOST_H
#define _HTTPREQUESTHOST_H

#include "HttpServer.h"
#include "ScriptableObject.h"
#include <qcc/String.h>
#include <map>

class _HttpRequestHost : public ScriptableObject {
  public:
    _HttpRequestHost(Plugin& plugin, HttpServer& httpServer, Http::Headers& requestHeaders, qcc::SocketStream& stream, qcc::SocketFd sessionFd);
    virtual ~_HttpRequestHost();

  private:
    HttpServer httpServer;
    Http::Headers requestHeaders;
    qcc::SocketStream stream;
    qcc::SocketFd sessionFd;
    uint16_t status;
    qcc::String statusText;
    Http::Headers responseHeaders;

    bool getStatus(NPVariant* result);
    bool setStatus(const NPVariant* value);
    bool getStatusText(NPVariant* result);
    bool setStatusText(const NPVariant* value);

    bool getAllRequestHeaders(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool getRequestHeader(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool setResponseHeader(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool send(const NPVariant* args, uint32_t argCount, NPVariant* result);
};

typedef qcc::ManagedObj<_HttpRequestHost> HttpRequestHost;

#endif // _HTTPREQUESTHOST_H