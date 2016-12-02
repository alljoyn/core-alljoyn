/**
 * @file
 * This class is to manage the permission of an endpoint on using transports or invoking method/signal calls on another peer
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#include "PermissionMgr.h"
#include <alljoyn/AllJoynStd.h>

#define QCC_MODULE "PERMISSION_MGR"

namespace ajn {

QStatus TransportPermission::FilterTransports(BusEndpoint& srcEp, const qcc::String& sender, TransportMask& transports, const char* callerName)
{
    QCC_UNUSED(srcEp);
    QCC_UNUSED(sender);
    QCC_UNUSED(transports);
    QCC_UNUSED(callerName);
    return ER_OK;
}

uint32_t PermissionMgr::AddAliasUnixUser(BusEndpoint& srcEp, qcc::String& sender, uint32_t origUID, uint32_t aliasUID)
{
    QCC_UNUSED(srcEp);
    QCC_UNUSED(sender);
    QCC_UNUSED(origUID);
    QCC_UNUSED(aliasUID);
    return ALLJOYN_ALIASUNIXUSER_REPLY_NO_SUPPORT;
}

QStatus PermissionMgr::CleanPermissionCache(BusEndpoint& endpoint)
{
    QCC_UNUSED(endpoint);
    return ER_OK;
}

} // namespace ajn {