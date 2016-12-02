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
#include "VersionInterface.h"

#include "npn.h"
#include "TypeMapping.h"
#include <alljoyn/version.h>
#include <qcc/Debug.h>
#include <string.h>

#define QCC_MODULE "ALLJOYN_JS"

_VersionInterface::_VersionInterface(Plugin& plugin) :
    ScriptableObject(plugin)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ATTRIBUTE("buildInfo", &_VersionInterface::getBuildInfo, 0);
    ATTRIBUTE("numericVersion", &_VersionInterface::getNumericVersion, 0);
    ATTRIBUTE("arch", &_VersionInterface::getArch, 0);
    ATTRIBUTE("apiLevel", &_VersionInterface::getApiLevel, 0);
    ATTRIBUTE("release", &_VersionInterface::getRelease, 0);
    ATTRIBUTE("version", &_VersionInterface::getVersion, 0);
}

_VersionInterface::~_VersionInterface()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool _VersionInterface::getBuildInfo(NPVariant* result)
{
    ToDOMString(plugin, ajn::GetBuildInfo(), strlen(ajn::GetBuildInfo()), *result);
    return true;
}

bool _VersionInterface::getNumericVersion(NPVariant* result)
{
    ToUnsignedLong(plugin, ajn::GetNumericVersion(), *result);
    return true;
}

bool _VersionInterface::getArch(NPVariant* result)
{
    ToUnsignedLong(plugin, GetVersionArch(ajn::GetNumericVersion()), *result);
    return true;
}

bool _VersionInterface::getApiLevel(NPVariant* result)
{
    ToUnsignedLong(plugin, GetVersionAPILevel(ajn::GetNumericVersion()), *result);
    return true;
}

bool _VersionInterface::getRelease(NPVariant* result)
{
    ToUnsignedLong(plugin, GetVersionRelease(ajn::GetNumericVersion()), *result);
    return true;
}

bool _VersionInterface::getVersion(NPVariant* result)
{
    ToDOMString(plugin, ajn::GetVersion(), strlen(ajn::GetVersion()), *result);
    return true;
}