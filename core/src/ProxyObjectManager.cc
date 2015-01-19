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
#include <qcc/Debug.h>

#include "ProxyObjectManager.h"
#include "SecLibDef.h"

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
ProxyObjectManager::ProxyObjectManager(ajn::BusAttachment* ba,
                                       const SecurityManagerConfig& config) :
    bus(ba), objectPath(config.pmObjectPath.c_str()), interfaceName(config.pmIfn.c_str())
{
    methodToSessionType["Claim"] = ECDHE_NULL;
    methodToSessionType["GetIdentity"] = ECDHE_NULL;
    methodToSessionType["GetManifest"] = ECDHE_NULL;
    methodToSessionType["GetPolicy"] = ECDHE_DSA;
    methodToSessionType["InstallIdentity"] = ECDHE_DSA;
    methodToSessionType["InstallMembership"] = ECDHE_DSA;
    methodToSessionType["InstallMembershipAuthData"] = ECDHE_DSA;
    methodToSessionType["InstallPolicy"] = ECDHE_DSA;
    methodToSessionType["RemoveMembership"] = ECDHE_DSA;
    methodToSessionType["Reset"] = ECDHE_DSA;
}

ProxyObjectManager::~ProxyObjectManager()
{
}

QStatus ProxyObjectManager::GetProxyObject(const ApplicationInfo appInfo,
                                           SessionType type,
                                           ajn::ProxyBusObject** remoteObject)
{
    QStatus status = ER_FAIL;
    if (appInfo.busName == "") {
        status = ER_FAIL;
        QCC_DbgRemoteError(("Application is offline"));
        return status;
    }
    const char* busName = appInfo.busName.c_str();

    ajn::SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                     SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = bus->JoinSession(busName, MNGT_SERVICE_PORT,
                              this, sessionId, opts);
    if (status != ER_OK) {
        QCC_DbgRemoteError(("Could not join session with %s", busName));
        return status;
    }

    const InterfaceDescription* remoteIntf = bus->GetInterface(interfaceName);
    if (NULL == remoteIntf) {
        status = ER_FAIL;
        QCC_LogError(status, ("Could not find interface %s", interfaceName));
        return status;
    }

    ajn::ProxyBusObject* remoteObj = new ajn::ProxyBusObject(*bus, busName,
                                                             objectPath, sessionId);
    if (remoteObj == NULL) {
        status = ER_FAIL;
        QCC_LogError(status, ("Could not create ProxyBusObject for %s", busName));
        return status;
    }

    status = remoteObj->AddInterface(*remoteIntf);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to add interface %s to ProxyBusObject", interfaceName));
        delete remoteObj;
        return status;
    }

    *remoteObject = remoteObj;
    return status;
}

QStatus ProxyObjectManager::ReleaseProxyObject(ajn::ProxyBusObject* remoteObject)
{
    SessionId sessionId = remoteObject->GetSessionId();
    delete remoteObject;
    return bus->LeaveSession(sessionId);
}

bool ProxyObjectManager::IsPermissionDeniedError(QStatus status, Message& msg)
{
    if (ER_PERMISSION_DENIED == status) {
        return true;
    }
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (msg->GetErrorName() == NULL) {
            return false;
        }
        if (strcmp(msg->GetErrorName(), "org.alljoyn.Bus.ER_PERMISSION_DENIED") == 0) {
            return true;
        }
    }
    return false;
}

QStatus ProxyObjectManager::MethodCall(const ApplicationInfo app,
                                       const qcc::String methodName,
                                       const MsgArg* args,
                                       const size_t numArgs,
                                       Message& replyMsg)
{
    QStatus status = ER_FAIL;
    const char* method = methodName.c_str();

    std::map<qcc::String, SessionType>::iterator it = methodToSessionType.begin();
    it = methodToSessionType.find(methodName);
    if (it == methodToSessionType.end()) {
        status = ER_FAIL;
        QCC_LogError(status, ("Could not determine session type for %s method", method));
        return status;
    }
    SessionType sessionType = it->second;

    ProxyBusObject* remoteObj;
    status = GetProxyObject(app, sessionType, &remoteObj);
    if (ER_OK != status) {
        // errors logged in GetProxyObject
        return status;
    }

    status = remoteObj->MethodCall(interfaceName, method, args, numArgs, replyMsg, MSG_REPLY_TIMEOUT);
    ReleaseProxyObject(remoteObj);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, replyMsg)) {
            status = ER_PERMISSION_DENIED;
            QCC_DbgRemoteError(("Permission denied to call %s method", method));
            return status;
        }
        QCC_DbgRemoteError(("Failed to call %s method", method));
        return status;
    }

    return status;
}

void ProxyObjectManager::SessionLost(ajn::SessionId sessionId,
                                     SessionLostReason reason)
{
    QCC_DbgPrintf(("Lost session %lu", (unsigned long)sessionId));
}
}
}
#undef QCC_MODULE
