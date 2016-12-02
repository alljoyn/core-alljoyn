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

#ifndef _ABOUTOBJHOST_H
#define _ABOUTOBJHOST_H

#include "BusAttachment.h"
#include "ScriptableObject.h"
#include "AboutObj.h"
#include <qcc/GUID.h>

class _AboutObjHost : public ScriptableObject {
  public:
    _AboutObjHost(Plugin& plugin, BusAttachment& busAttachment);
    virtual ~_AboutObjHost();

  private:
    BusAttachment busAttachment;
    AboutObj aboutObj;
    ajn::AboutData* aboutData;

    bool announce(const NPVariant* args, uint32_t argCount, NPVariant* result);
};

typedef qcc::ManagedObj<_AboutObjHost> AboutObjHost;

#endif