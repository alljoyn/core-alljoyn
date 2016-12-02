/**
 * @file
 * This class is to manage the permission of an endpoint on using transports or invoking method/signal calls on another peer.
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
#ifndef _PERMISSION_MGR_H
#define _PERMISSION_MGR_H

#include "LocalTransport.h"
#include "TransportList.h"
#include <qcc/ThreadPool.h>

namespace ajn {

#define MAX_PERM_CHECKEDCALL_SIZE (512)

class TransportPermission {
  public:
    /**
     * Filter out transports that the endpoint has no permissions to use
     * @param   srcEp         The source endpoint
     * @param   sender        The sender's well-known name string
     * @param   transports    The transport mask
     * @param   callerName    The caller that invokes this method
     */
    static QStatus FilterTransports(BusEndpoint& srcEp, const qcc::String& sender, TransportMask& transports, const char* callerName);
};

class PermissionMgr {
  public:
    /**
     * Add an alias ID to a UnixEndpoint User ID
     * @param srcEp     The source endpoint
     * @param origUID   The unique User ID
     * @param aliasUID  The alias User ID
     */
    static uint32_t AddAliasUnixUser(BusEndpoint& srcEp, qcc::String& sender, uint32_t origUID, uint32_t aliasUID);

    /**
     * Cleanup the permission information cache of an enpoint before it exits.
     */
    static QStatus CleanPermissionCache(BusEndpoint& endpoint);

};

} // namespace ajn

#endif //_PERMISSION_MGR_H