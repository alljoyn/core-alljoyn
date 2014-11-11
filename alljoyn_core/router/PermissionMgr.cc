/**
 * @file
 * This class is to manage the permission of an endpoint on using transports or invoking method/signal calls on another peer
 */

/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/AllJoynStd.h>
#include "ConfigDB.h"
#include "PermissionMgr.h"
#include "RemoteEndpoint.h"

#define QCC_MODULE "PERMISSION_MGR"

namespace ajn {

PermissionMgr::DaemonBusCallPolicy PermissionMgr::GetDaemonBusCallPolicy(BusEndpoint sender)
{
    static bool enableRestrict = ConfigDB::GetConfigDB()->GetFlag("restrict_untrusted_clients");

    QCC_DbgTrace(("PermissionMgr::GetDaemonBusCallPolicy(send=%s)", sender->GetUniqueName().c_str()));
    DaemonBusCallPolicy policy = STDBUSCALL_ALLOW_ACCESS_SERVICE_ANY;
    if (enableRestrict) {
        if (sender->GetEndpointType() == ENDPOINT_TYPE_NULL || sender->GetEndpointType() == ENDPOINT_TYPE_LOCAL) {
            policy = STDBUSCALL_ALLOW_ACCESS_SERVICE_ANY;
        } else if (sender->GetEndpointType() == ENDPOINT_TYPE_REMOTE) {
            RemoteEndpoint rEndpoint = RemoteEndpoint::cast(sender);
            QCC_DbgPrintf(("This is a RemoteEndpoint. ConnSpec = %s", rEndpoint->GetConnectSpec().c_str()));

            if ((rEndpoint->GetConnectSpec() == "unix") ||
                (rEndpoint->GetConnectSpec() == "npipe") ||
                (rEndpoint->GetConnectSpec() == "localhost") ||
                (rEndpoint->GetConnectSpec() == "slap")) {
                policy = STDBUSCALL_ALLOW_ACCESS_SERVICE_ANY;
            } else if (rEndpoint->GetConnectSpec() == "tcp") {
                if (!rEndpoint->IsTrusted()) {
                    policy = STDBUSCALL_ALLOW_ACCESS_SERVICE_LOCAL;
                } else {
                    policy = STDBUSCALL_ALLOW_ACCESS_SERVICE_ANY;
                }
            } else {
                policy = STDBUSCALL_SHOULD_REJECT;
                QCC_LogError(ER_FAIL, ("Unrecognized connect spec for endpoint:%s. connectspec=%s", sender->GetUniqueName().c_str(), rEndpoint->GetConnectSpec().c_str()));
            }
        } else if (sender->GetEndpointType() == ENDPOINT_TYPE_BUS2BUS || sender->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
            policy = STDBUSCALL_SHOULD_REJECT;
            QCC_LogError(ER_FAIL, ("Bus-to-bus endpoint(%s) is not ALLOW_ACCESSed to invoke daemon standard method call", sender->GetUniqueName().c_str()));
        } else {
            policy = STDBUSCALL_SHOULD_REJECT;
            QCC_LogError(ER_FAIL, ("Unexpected endponit type(%d)", sender->GetEndpointType()));
        }
    }
    return policy;
}

} // namespace ajn {
