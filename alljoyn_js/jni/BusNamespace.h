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
#ifndef _BUSNAMESPACE_H
#define _BUSNAMESPACE_H

#include "BusAttachmentInterface.h"
#include "BusErrorInterface.h"
#include "CredentialsInterface.h"
#include "MessageInterface.h"
#include "Plugin.h"
#include "ScriptableObject.h"
#include "IfcSecurityInterface.h"
#include "SessionLostReasonInterface.h"
#include "SessionOptsInterface.h"
#include "SocketFdInterface.h"
#include "VersionInterface.h"
#include <map>
#include <qcc/ManagedObj.h>

class _BusNamespace : public ScriptableObject {
  public:
    static std::map<qcc::String, int32_t>& Constants();
    _BusNamespace(Plugin& plugin);
    virtual ~_BusNamespace();

  private:
    static std::map<qcc::String, int32_t> constants;

    BusAttachmentInterface busAttachmentInterface;
    BusErrorInterface busErrorInterface;
    CredentialsInterface credentialsInterface;
    IfcSecurityInterface ifcSecurityInterface;
    MessageInterface messageInterface;
    SessionLostReasonInterface sessionLostReasonInterface;
    SessionOptsInterface sessionOptsInterface;
    SocketFdInterface socketFdInterface;
    VersionInterface versionInterface;
    bool getBusAttachment(NPVariant* result);
    bool getBusError(NPVariant* result);
    bool getCredentials(NPVariant* result);
    bool getIfcSecurity(NPVariant* result);
    bool getMessage(NPVariant* result);
    bool getSessionOpts(NPVariant* result);
    bool getSessionLostReason(NPVariant* result);
    bool getSocketFd(NPVariant* result);
    bool getVersion(NPVariant* result);

    /*
     * Fallback for FeaturePermissions APIs.
     * (http://dev.w3.org/2009/dap/perms/FeaturePermissions.html).
     */
    bool getPrivilegedFeatures(NPVariant* result);
    bool permissionLevel(const NPVariant* args, uint32_t argCount, NPVariant* result);
    bool requestPermission(const NPVariant* args, uint32_t argCount, NPVariant* result);
};

typedef qcc::ManagedObj<_BusNamespace> BusNamespace;

#endif // _BUSNAMESPACE_H