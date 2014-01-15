/**
 * @file
 * This class is to manage the permission of an endpoint on using transports or invoking method/signal calls on another peer.
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
    typedef enum {
        STDBUSCALL_ALLOW_ACCESS_SERVICE_ANY = 0,         /**< A standard daemon bus call is allowed to interact with any local or remote service */
        STDBUSCALL_ALLOW_ACCESS_SERVICE_LOCAL = 1,       /**< A standard daemon bus call is allowed, but it can only interact with local service */
        STDBUSCALL_SHOULD_REJECT = 2                     /**< A standard daemon bus call should always be rejected */
    } DaemonBusCallPolicy;

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

    /**
     * Get the policy for a bus endpoint's permission of invoking the standard DBus and AllJoyn inteface.
     * @param   endpoint   The bus endpoint.
     * @return  the policy associated with the bus endpoint for invoking the standard DBus and AllJoyn inteface.
     */
    static DaemonBusCallPolicy GetDaemonBusCallPolicy(BusEndpoint sender);

};

} // namespace ajn

#endif //_PERMISSION_MGR_H
