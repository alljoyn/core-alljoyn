/*
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
