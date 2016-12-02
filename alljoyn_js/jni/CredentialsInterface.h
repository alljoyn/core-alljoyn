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
#ifndef _CREDENTIALSINTERFACE_H
#define _CREDENTIALSINTERFACE_H

#include "ScriptableObject.h"
#include <qcc/ManagedObj.h>

class _CredentialsInterface : public ScriptableObject {
  public:
    static std::map<qcc::String, int32_t>& Constants();
    _CredentialsInterface(Plugin& plugin);
    virtual ~_CredentialsInterface();

  private:
    static std::map<qcc::String, int32_t> constants;
};

typedef qcc::ManagedObj<_CredentialsInterface> CredentialsInterface;

#endif // _CREDENTIALSINTERFACE_H