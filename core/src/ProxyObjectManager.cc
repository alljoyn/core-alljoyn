/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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
ajn::AuthListener* ProxyObjectManager::listener = NULL;

ProxyObjectManager::ProxyObjectManager(ajn::BusAttachment* ba) :
    bus(ba), objectPath("/org/allseen/Security/PermissionMgmt"),
    interfaceName("org.allseen.Security.PermissionMgmt")
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

QStatus ProxyObjectManager::GetStatus(const QStatus status, const Message& msg) const
{
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        qcc::String errorMessage;
        msg->GetErrorName(&errorMessage);

        if (strcmp(errorMessage.c_str(), "ER_DUPLICATE_CERTIFICATE") == 0) {
            return ER_DUPLICATE_CERTIFICATE;
        }

        if (msg->GetErrorName() == NULL) {
            return ER_FAIL;
        }

        if (strcmp(msg->GetErrorName(), "org.alljoyn.Bus.ER_PERMISSION_DENIED") == 0) {
            return ER_PERMISSION_DENIED;
        }
    }

    return status;
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

    lock.Lock(__FILE__, __LINE__);

    if (sessionType == ECDHE_NULL) {
        bus->EnablePeerSecurity(KEYX_ECDHE_NULL, listener, AJNKEY_STORE, true);
    } else if (sessionType == ECDHE_DSA) {
        bus->EnablePeerSecurity(ECDHE_KEYX, listener, AJNKEY_STORE, true);
    }

    ProxyBusObject* remoteObj;
    status = GetProxyObject(app, sessionType, &remoteObj);
    if (ER_OK != status) {
        // errors logged in GetProxyObject
        lock.Unlock(__FILE__, __LINE__);
        return status;
    }

    status = remoteObj->MethodCall(interfaceName, method, args, numArgs, replyMsg, MSG_REPLY_TIMEOUT);

    lock.Unlock(__FILE__, __LINE__);

    ReleaseProxyObject(remoteObj);

    if (ER_OK != status) {
        status = GetStatus(status, replyMsg);

        qcc::String errorDescription = replyMsg->GetErrorDescription();
        QCC_DbgRemoteError(("Failed to call %s method: %s", method, errorDescription.c_str()));
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
