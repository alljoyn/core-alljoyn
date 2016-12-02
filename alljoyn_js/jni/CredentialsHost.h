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
#ifndef _CREDENTIALSHOST_H
#define _CREDENTIALSHOST_H

#include "BusAttachment.h"
#include "ScriptableObject.h"
#include <alljoyn/AuthListener.h>

class _CredentialsHost : public ScriptableObject {
  public:
    _CredentialsHost(Plugin& plugin, ajn::AuthListener::Credentials& credentials);
    virtual ~_CredentialsHost();

  protected:
    ajn::AuthListener::Credentials& credentials;

    bool getPassword(NPVariant* npresult);
    bool setPassword(const NPVariant* npvalue);
    bool getUserName(NPVariant* npresult);
    bool setUserName(const NPVariant* npvalue);
    bool getCertChain(NPVariant* npresult);
    bool setCertChain(const NPVariant* npvalue);
    bool getPrivateKey(NPVariant* npresult);
    bool setPrivateKey(const NPVariant* npvalue);
    bool getLogonEntry(NPVariant* npresult);
    bool setLogonEntry(const NPVariant* npvalue);
    bool getExpiration(NPVariant* npresult);
    bool setExpiration(const NPVariant* npvalue);
};

typedef qcc::ManagedObj<_CredentialsHost> CredentialsHost;

#endif // _CREDENTIALSHOST_H