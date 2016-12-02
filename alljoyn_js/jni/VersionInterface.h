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
#ifndef _VERSIONINTERFACE_H
#define _VERSIONINTERFACE_H

#include "ScriptableObject.h"
#include <qcc/ManagedObj.h>

class _VersionInterface : public ScriptableObject {
  public:
    _VersionInterface(Plugin& plugin);
    virtual ~_VersionInterface();

  private:
    bool getBuildInfo(NPVariant* result);
    bool getNumericVersion(NPVariant* result);
    bool getArch(NPVariant* result);
    bool getApiLevel(NPVariant* result);
    bool getRelease(NPVariant* result);
    bool getVersion(NPVariant* result);
    //bool toString(const NPVariant* args, uint32_t npargCount, NPVariant* result);
};

typedef qcc::ManagedObj<_VersionInterface> VersionInterface;

#endif // _VERSIONINTERFACE_H