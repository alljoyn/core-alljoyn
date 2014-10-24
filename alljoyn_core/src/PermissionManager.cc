/**
 * @file
 * This file defines the Permission DB classes that provide the interface to
 * parse the authorization data
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#include <alljoyn/Message.h>
#include <qcc/Debug.h>
#include "PermissionManager.h"

#define QCC_MODULE "PERMISSION_MGMT"

using namespace std;
using namespace qcc;

namespace ajn {

struct MessageHolder {
    Message& msg;
    bool send;
    const char* objPath;
    const char* iName;
    const char* mbrName;

    MessageHolder(Message& msg, bool send) : msg(msg), send(send),
        iName(NULL), mbrName(NULL)
    {
        objPath = msg->GetObjectPath();
    }
};

struct MatchWeight {
    int8_t idx;
    uint8_t primary;
    uint8_t secondary;

    MatchWeight() : idx(-1), primary(0), secondary(0)
    {
    }
};

struct Right {
    bool consumer;
    bool readOnly;

    Right() : consumer(false), readOnly(false)
    {
    }
};

static bool LocalAuthorized(bool send, MessageHolder& msgHolder, PermissionPolicy* policy, GUID128* guilds, size_t guildCount)
{
    QCC_DbgPrintf(("LocalAuthorized with objPath %s iName %s mbrName %s", msgHolder.objPath, msgHolder.iName, msgHolder.mbrName));
    if (policy == NULL) {
        return true;  /* no policy no enforcement */
    }
    Right right;

    if (send) {
        if (msgHolder.msg->GetType() == MESSAGE_METHOD_CALL) {
            /* send a method call */
            right.consumer = true;
        } else if (msgHolder.msg->GetType() == MESSAGE_SIGNAL) {
            /* send a signal */
            right.consumer = false;
        }
    } else {
        if (msgHolder.msg->GetType() == MESSAGE_METHOD_CALL) {
            /* receive a method call */
            right.consumer = false;
        } else if (msgHolder.msg->GetType() == MESSAGE_SIGNAL) {
            /* receive a signal */
            right.consumer = true;
        }
    }

    return true;
}

static bool RemoteAuthorized(bool send, MessageHolder& msgHolder, PermissionPolicy* policy)
{
    return true;
}

static bool IsStdInterface(const char* iName)
{
    if (strcmp(iName, org::alljoyn::Bus::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Daemon::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Daemon::Debug::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Bus::Peer::Authentication::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Bus::Peer::Session::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::allseen::Introspectable::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Bus::Peer::HeaderCompression::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::freedesktop::DBus::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::freedesktop::DBus::Peer::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::freedesktop::DBus::Introspectable::InterfaceName) == 0) {
        return true;
    }
    return false;
}

static bool IsPropertyInterface(const char* iName)
{
    if (strcmp(iName, org::freedesktop::DBus::Properties::InterfaceName) == 0) {
        return true;
    }
    return false;
}

static bool IsPermissionMgmtInterface(const char* iName)
{
    return (strcmp(iName, org::allseen::Security::PermissionMgmt::InterfaceName) == 0);
}

static QStatus ParsePropertiesMessage(bool send, Message& msg, const char** iNameHolder, const char** mbrNameHolder)
{
    QStatus status;
    const char* mbrName = msg->GetMemberName();
    const char* propIName;
    const char* propName;

    if ((strncmp(mbrName, "Get", 3) == 0) || (strncmp(mbrName, "Set", 3) == 0)) {
        const MsgArg* args;
        size_t numArgs;
        if (send) {
            msg->GetRefArgs(numArgs, args);
        } else {
            msg->GetArgs(numArgs, args);
        }
        if (numArgs < 2) {
            return ER_INVALID_DATA;
        }
        /* only interested in the first two arguments */
        status = args[0].Get("s", &propIName);
        if (ER_OK != status) {
            return status;
        }
        status = args[1].Get("s", &propName);
        if (status != ER_OK) {
            return status;
        }

        QCC_DbgPrintf(("PermissionManager::ParsePropertiesMessage %s %s.%s", mbrName, propIName, propName));
    } else if (strncmp(mbrName, "GetAll", 6) == 0) {
        propName = NULL;
        if (send) {
            const MsgArg* args;
            size_t numArgs;
            msg->GetRefArgs(numArgs, args);
            if (numArgs < 1) {
                return ER_INVALID_DATA;
            }
            status = args[0].Get("s", &propIName);
        } else {
            status = msg->GetArgs("s", &propIName);
        }
        if (status != ER_OK) {
            return status;
        }
        QCC_DbgPrintf(("PermissionManager::ParsePropertiesMessage %s %s.%s", mbrName, propIName));
    } else {
        return ER_FAIL;
    }
    *iNameHolder = propIName;
    *mbrNameHolder = propName;
    return ER_OK;
}

bool PermissionManager::PeerHasAdminPriv(const GUID128& peerGuid)
{
    PermissionMgmtObj::TrustAnchor ta;
    QStatus status = permissionMgmtObj->GetTrustAnchor(ta);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionManager::PeerHasAdminPriv does not find trust anchor"));
        return false;  /* not yet claimed so can't allow install policy */
    }

    /* check against trust anchor */
    GUID128 taGuid(0);
    taGuid.SetBytes(ta.guid);
    return (taGuid == peerGuid);
}

bool PermissionManager::AuthorizePermissionMgmt(bool send, const GUID128& peerGuid, Message& msg)
{
    if (send) {
        return true;  /* always allow send action */
    }
    bool authorized = false;
    const char* mbrName = msg->GetMemberName();

    if (!permissionMgmtObj) {
        QCC_DbgPrintf(("PermissionManager::AuthorizePermissionMgmt does not have PermissionMgmtObj initialized"));
        return false;
    }
    if (strncmp(mbrName, "Claim", 5) == 0) {
        PermissionMgmtObj::TrustAnchor ta;
        QStatus status = permissionMgmtObj->GetTrustAnchor(ta);
        if (ER_OK == status) {
            return false;  /* already claim */
        }
        return true;
    } else if (strncmp(mbrName, "InstallPolicy", 14) == 0) {
        /* this action requires admin privilege */
        return PeerHasAdminPriv(peerGuid);
    } else if (strncmp(mbrName, "GetPolicy", 9) == 0) {
        /* this action requires admin privilege */
        return PeerHasAdminPriv(peerGuid);
    } else if (strncmp(mbrName, "NotifyConfig", 12) == 0) {
        return true;
    }
    return authorized;
}

/*
 * the apply order is:
 *  1. applies ANY-USER policy
 *  2. applies all guilds-in-common policies
 *  3. applies peer policies
 */
QStatus PermissionManager::AuthorizeMessage(bool send, const GUID128& peerGuid, Message& msg,
                                            size_t peerGuildCount, GUID128* peerGuilds)
{
    QStatus status = ER_PERMISSION_DENIED;
    bool authorized = false;

    /* only checks for method call and signal */
    if ((msg->GetType() != MESSAGE_METHOD_CALL) &&
        (msg->GetType() != MESSAGE_SIGNAL)) {
        return ER_OK;
    }

    /* skip the AllJoyn Std interfaces */
    if (IsStdInterface(msg->GetInterface())) {
        return ER_OK;
    }
    if (IsPermissionMgmtInterface(msg->GetInterface())) {
        if (AuthorizePermissionMgmt(send, peerGuid, msg)) {
            return ER_OK;
        }
        return status;
    }
    MessageHolder holder(msg, send);
    if (IsPropertyInterface(msg->GetInterface())) {
        status = ParsePropertiesMessage(send, msg, &holder.iName, &holder.mbrName);
        if (status != ER_OK) {
            return status;
        }
    } else {
        holder.iName = msg->GetInterface();
        holder.mbrName = msg->GetMemberName();
    }

    QCC_DbgPrintf(("PermissionManager::AuthorizeMessage with send: %d msg %s\n", send, msg->ToString().c_str()));
    QCC_DbgPrintf(("PermissionManager::AuthorizeMessage calling LocalAuthorized with numOfGuilds %d %d\n", numOfGuilds, GetNumOfGuilds()));
    if (send) {
        authorized = LocalAuthorized(send, holder, policy, guilds, numOfGuilds);
    } else {
        authorized = LocalAuthorized(send, holder, policy, peerGuilds, peerGuildCount);
    }
    if (!authorized) {
        QCC_DbgPrintf(("PermissionManager::AuthorizeMessage LocalAuthorized returns ER_PERMISSION_DENIED\n"));
        return status;
    }
    authorized = RemoteAuthorized(send, holder, policy);
    if (!authorized) {
        QCC_DbgPrintf(("PermissionManager::AuthorizeMessage RemoteAuthorized returns ER_PERMISSION_DENIED\n"));
        return status;
    }
    return ER_OK;
}

} /* namespace ajn */

