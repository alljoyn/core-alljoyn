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
#ifndef _SESSIONOPTSHOST_H
#define _SESSIONOPTSHOST_H

#include "ScriptableObject.h"
#include <alljoyn/Session.h>
#include <qcc/ManagedObj.h>

class _SessionOptsHost : public ScriptableObject {
  public:
    _SessionOptsHost(Plugin& plugin, const ajn::SessionOpts& opts);
    virtual ~_SessionOptsHost();

  private:
    const ajn::SessionOpts opts;

    bool getTraffic(NPVariant* result);
    bool getIsMultipoint(NPVariant* result);
    bool getProximity(NPVariant* result);
    bool getTransports(NPVariant* result);
};

typedef qcc::ManagedObj<_SessionOptsHost> SessionOptsHost;

#endif // _SESSIONOPTSHOST_H