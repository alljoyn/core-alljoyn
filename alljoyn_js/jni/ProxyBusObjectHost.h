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
#ifndef _PROXYBUSOBJECTHOST_H
#define _PROXYBUSOBJECTHOST_H

#include "BusAttachment.h"
#include "ProxyBusObject.h"
#include "ScriptableObject.h"
#include <qcc/ManagedObj.h>
class _ProxyBusObjectHostImpl;

class _ProxyBusObjectHost : public ScriptableObject {
  public:
    _ProxyBusObjectHost(Plugin& plugin, BusAttachment& busAttachment, const char* serviceName, const char* path, ajn::SessionId sessionId);
    _ProxyBusObjectHost(Plugin& plugin, BusAttachment& busAttachment, ajn::ProxyBusObject* proxyBusObject);
    virtual ~_ProxyBusObjectHost();

  private:
    BusAttachment busAttachment;
    ProxyBusObject proxyBusObject;
    _ProxyBusObjectHostImpl* impl; /* Hide declaration of ProxyChildrenHost to get around recursive include. */
    _ProxyBusObjectHost& operator=(const _ProxyBusObjectHost&);
    _ProxyBusObjectHost(const _ProxyBusObjectHost&);

    void Initialize();

    bool getPath(NPVariant* result);
    bool getServiceName(NPVariant* result);
    bool getSessionId(NPVariant* result);
    bool getSecure(NPVariant* result);

    bool getChildren(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool getInterface(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool getInterfaces(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool introspect(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool methodCall(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool parseXML(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool secureConnection(const NPVariant* args, uint32_t argCount, NPVariant* result);
};

typedef qcc::ManagedObj<_ProxyBusObjectHost> ProxyBusObjectHost;

#endif // _PROXYBUSOBJECTHOST_H