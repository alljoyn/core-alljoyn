/**
 * @file
 * This class is to manage the permission of an endpoint on using transports or invoking method/signal calls on another peer
 */

/******************************************************************************
 * Copyright (c) 2012, 2014 AllSeen Alliance. All rights reserved.
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
#include "PermissionMgr.h"
#include "PermissionDB.h"
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <map>

using namespace qcc;

#define QCC_MODULE "PERMISSION_MGR"

namespace ajn {

QStatus TransportPermission::FilterTransports(BusEndpoint& srcEp, const qcc::String& sender, TransportMask& transports, const char* callerName)
{
    QCC_DbgPrintf(("TransportPermission::FilterTransports() callerName(%s)", callerName));
    QStatus status = ER_OK;
    if (srcEp->IsValid()) {
        if (transports & TRANSPORT_WLAN) {
            bool allowed = PermissionDB::GetDB().IsWifiAllowed(srcEp->GetUserId());
            if (!allowed) {
                transports ^= TRANSPORT_WLAN;
                QCC_LogError(ER_ALLJOYN_ACCESS_PERMISSION_WARNING, ("AllJoynObj::%s() WARNING: No permission to use Wifi", ((callerName == NULL) ? "" : callerName)));
            }
        }
        if (transports == 0) {
            status = ER_BUS_NO_TRANSPORTS;
        }
    } else {
        status = ER_BUS_NO_ENDPOINT;
        QCC_LogError(ER_BUS_NO_ENDPOINT, ("AllJoynObj::CheckTransportsPermission No Bus Endpoint found for Sender %s", sender.c_str()));
    }
    return status;
}

uint32_t PermissionMgr::AddAliasUnixUser(BusEndpoint& srcEp, qcc::String& sender, uint32_t origUID, uint32_t aliasUID)
{
    QCC_DbgHLPrintf(("PermissionMgr::AddAliasUnixUser() origUID(%d), aliasUID(%d)", origUID, aliasUID));
    QStatus status = ER_OK;
    uint32_t replyCode = ALLJOYN_ALIASUNIXUSER_REPLY_SUCCESS;
    if (!(srcEp->IsValid())) {
        status = ER_BUS_NO_ENDPOINT;
        QCC_LogError(status, ("AliasUnixUser Failed to find endpoint for sender=%s", sender.c_str()));
        replyCode = ALLJOYN_ALIASUNIXUSER_REPLY_FAILED;
    } else {
        origUID = srcEp->GetUserId();
        if (origUID == (uint32_t)-1 || aliasUID == (uint32_t)-1) {
            QCC_LogError(ER_FAIL, ("AliasUnixUser Invalid user id origUID=%d aliasUID=%d", origUID, aliasUID));
            replyCode = ALLJOYN_ALIASUNIXUSER_REPLY_FAILED;
        }
    }

    if (replyCode == ALLJOYN_ALIASUNIXUSER_REPLY_SUCCESS) {
        if (PermissionDB::GetDB().AddAliasUnixUser(origUID, aliasUID) != ER_OK) {
            replyCode = ALLJOYN_ALIASUNIXUSER_REPLY_FAILED;
        }
    }
    return replyCode;
}

QStatus PermissionMgr::CleanPermissionCache(BusEndpoint& endpoint)
{
    QCC_DbgHLPrintf(("PermissionMgr::CleanPermissionCache()"));
    return PermissionDB::GetDB().RemovePermissionCache(endpoint);
}

} // namespace ajn {
