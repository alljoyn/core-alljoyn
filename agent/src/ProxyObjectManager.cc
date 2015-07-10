/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#include "ProxyObjectManager.h"

#include <qcc/Debug.h>
#include <alljoyn/AllJoynStd.h>

#define QCC_MODULE "SECMGR_AGENT"

#define MSG_REPLY_TIMEOUT 5000

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
AuthListener* ProxyObjectManager::listener = nullptr;

ProxyObjectManager::ProxyObjectManager(BusAttachment* ba) :
    bus(ba)
{
}

ProxyObjectManager::~ProxyObjectManager()
{
}

QStatus ProxyObjectManager::GetProxyObject(const OnlineApplication app,
                                           SessionType sessionType,
                                           SecurityApplicationProxy** remoteObject)
{
    QStatus status = ER_FAIL;
    if (app.busName == "") {
        status = ER_FAIL;
        QCC_DbgRemoteError(("Application is offline"));
        return status;
    }

    lock.Lock(__FILE__, __LINE__);

    if (sessionType == ECDHE_NULL) {
        bus->EnablePeerSecurity(KEYX_ECDHE_NULL, listener, AJNKEY_STORE, true);
    } else if (sessionType == ECDHE_DSA) {
        bus->EnablePeerSecurity(ECDHE_KEYX, listener, AJNKEY_STORE, true);
    }

    const char* busName = app.busName.c_str();

    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                     SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = bus->JoinSession(busName, ALLJOYN_SESSIONPORT_PERMISSION_MGMT,
                              this, sessionId, opts);
    if (status != ER_OK) {
        QCC_DbgRemoteError(("Could not join session with %s", busName));
        lock.Unlock(__FILE__, __LINE__);
        return status;
    }

    SecurityApplicationProxy* remoteObj = new SecurityApplicationProxy(*bus, busName, sessionId);
    if (remoteObj == nullptr) {
        status = ER_FAIL;
        QCC_LogError(status, ("Could not create ProxyBusObject for %s", busName));
        lock.Unlock(__FILE__, __LINE__);
        return status;
    }

    *remoteObject = remoteObj;
    return status;
}

QStatus ProxyObjectManager::ReleaseProxyObject(SecurityApplicationProxy* remoteObject)
{
    lock.Unlock(__FILE__, __LINE__);

    SessionId sessionId = remoteObject->GetSessionId();
    delete remoteObject;
    return bus->LeaveSession(sessionId);
}

QStatus ProxyObjectManager::Claim(const OnlineApplication& app,
                                  KeyInfoNISTP256& certificateAuthority,
                                  GroupInfo& adminGroup,
                                  IdentityCertificate* identityCertChain,
                                  size_t identityCertChainSize,
                                  const Manifest& manifest)
{
    QStatus status;

    SecurityApplicationProxy* remoteObj;
    status = GetProxyObject(app, ECDHE_NULL, &remoteObj);
    if (ER_OK != status) {
        // Errors logged in GetProxyObject.
        return status;
    }

    PermissionPolicy::Rule* manifestRules;
    size_t manifestRulesCount;
    status = manifest.GetRules(&manifestRules, &manifestRulesCount);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to get manifest rules"));
        return status;
    }

    status = remoteObj->Claim(certificateAuthority,
                              adminGroup.guid, adminGroup.authority,
                              identityCertChain, identityCertChainSize,
                              manifestRules, manifestRulesCount);

    ReleaseProxyObject(remoteObj);

    return status;
}

QStatus ProxyObjectManager::GetIdentity(const OnlineApplication& app,
                                        IdentityCertificate* cert)
{
    QStatus status;

    SecurityApplicationProxy* remoteObj;
    status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // Errors logged in GetProxyObject.
        return status;
    }

    MsgArg certChainArg;
    status = remoteObj->GetIdentity(certChainArg);
    ReleaseProxyObject(remoteObj);

    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetIndentity"));
        return status;
    }

    status = SecurityApplicationProxy::MsgArgToIdentityCertChain(certChainArg, cert, 1);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to MsdArgToIdentityCertChain"));
        return status;
    }

    return status;
}

QStatus ProxyObjectManager::UpdateIdentity(const OnlineApplication& app,
                                           IdentityCertificate* certChain,
                                           size_t certChainSize,
                                           const Manifest& mf)
{
    QStatus status;

    SecurityApplicationProxy* remoteObj;
    status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // Errors logged in GetProxyObject.
        return status;
    }

    PermissionPolicy::Rule* manifestRules;
    size_t manifestRuleCount;
    status = mf.GetRules(&manifestRules, &manifestRuleCount);
    if (ER_OK == status) {
        status = remoteObj->UpdateIdentity(certChain, certChainSize, manifestRules, manifestRuleCount);
        delete[] manifestRules;
    }

    ReleaseProxyObject(remoteObj);

    return status;
}

QStatus ProxyObjectManager::InstallMembership(const OnlineApplication& app,
                                              const MembershipCertificate* certChain,
                                              size_t certChainSize)
{
    QStatus status;

    SecurityApplicationProxy* remoteObj;
    status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // errors logged in GetProxyObject
        return status;
    }

    status = remoteObj->InstallMembership(certChain, certChainSize);

    ReleaseProxyObject(remoteObj);

    return status;
}

QStatus ProxyObjectManager::GetPolicy(const OnlineApplication& app,
                                      PermissionPolicy& policy)
{
    SecurityApplicationProxy* remoteObj;
    QStatus status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // errors logged in GetProxyObject
        return status;
    }

    status = remoteObj->GetPolicy(policy);
    ReleaseProxyObject(remoteObj);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetPolicy"));
    }

    return status;
}

QStatus ProxyObjectManager::GetPolicyVersion(const OnlineApplication& app,
                                             uint32_t&  policyVersion)
{
    SecurityApplicationProxy* remoteObj;
    QStatus status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // errors logged in GetProxyObject
        return status;
    }

    status = remoteObj->GetPolicyVersion(policyVersion);
    ReleaseProxyObject(remoteObj);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetPolicyVersion"));
    }

    return status;
}

QStatus ProxyObjectManager::GetDefaultPolicy(const OnlineApplication& app,
                                             PermissionPolicy& policy)
{
    SecurityApplicationProxy* remoteObj;
    QStatus status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // errors logged in GetProxyObject
        return status;
    }

    status = remoteObj->GetDefaultPolicy(policy);
    ReleaseProxyObject(remoteObj);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetDefaultPolicy"));
    }

    return status;
}

QStatus ProxyObjectManager::UpdatePolicy(const OnlineApplication& app,
                                         const PermissionPolicy& policy)
{
    SecurityApplicationProxy* remoteObj;
    QStatus status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // errors logged in GetProxyObject
        return status;
    }

    status = remoteObj->UpdatePolicy(policy);
    ReleaseProxyObject(remoteObj);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to UpdatePolicy"));
    }

    return status;
}

QStatus ProxyObjectManager::ResetPolicy(const OnlineApplication& app)
{
    SecurityApplicationProxy* remoteObj;
    QStatus status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // errors logged in GetProxyObject
        return status;
    }

    status = remoteObj->ResetPolicy();
    ReleaseProxyObject(remoteObj);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to ResetPolicy"));
    }

    return status;
}

QStatus ProxyObjectManager::GetManifestTemplate(const OnlineApplication& app,
                                                Manifest& manifest)
{
    QStatus status;

    SecurityApplicationProxy* remoteObj;
    status = GetProxyObject(app, ECDHE_NULL, &remoteObj);
    if (ER_OK != status) {
        // errors logged in GetProxyObject
        return status;
    }

    MsgArg rulesMsgArg;
    status = remoteObj->GetManifestTemplate(rulesMsgArg);

    ReleaseProxyObject(remoteObj);

    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetManifestTemplate"));
        return status;
    }

    PermissionPolicy::Rule* manifestRules;
    size_t manifestRulesCount;
    status = PermissionPolicy::ParseRules(rulesMsgArg, &manifestRules, &manifestRulesCount);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to ParseRules"));
        goto Exit;
    }

    status = manifest.SetFromRules(manifestRules, manifestRulesCount);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to SetFromRules"));
        goto Exit;
    }

Exit:
    delete[] manifestRules;
    return status;
}

QStatus ProxyObjectManager::Reset(const OnlineApplication& app)
{
    QStatus status;

    SecurityApplicationProxy* remoteObj;
    status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // Errors logged in GetProxyObject.
        return status;
    }

    status = remoteObj->Reset();

    ReleaseProxyObject(remoteObj);

    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to Reset"));
    }

    return status;
}

QStatus ProxyObjectManager::RemoveMembership(const OnlineApplication& app,
                                             const string& serial,
                                             const KeyInfoNISTP256& issuerKeyInfo)
{
    QStatus status;

    SecurityApplicationProxy* remoteObj;
    status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // Errors logged in GetProxyObject.
        return status;
    }

    status = remoteObj->RemoveMembership(serial.c_str(), issuerKeyInfo);

    ReleaseProxyObject(remoteObj);

    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to RemoveMembership"));
    }

    return status;
}

QStatus ProxyObjectManager::GetMembershipSummaries(const OnlineApplication& app,
                                                   vector<MembershipSummary>& summaries)
{
    QStatus status = ER_FAIL;

    SecurityApplicationProxy* remoteObj;
    status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // Errors logged in GetProxyObject.
        return status;
    }

    MsgArg replyMsgArg;
    status = remoteObj->GetMembershipSummaries(replyMsgArg);
    ReleaseProxyObject(remoteObj);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetMembershipSummaries"));
    }

    size_t count = replyMsgArg.v_array.GetNumElements();
    KeyInfoNISTP256* keyInfos = new KeyInfoNISTP256[count];
    String* serials = new String[count];
    status = SecurityApplicationProxy::MsgArgToCertificateIds(replyMsgArg,
                                                              serials,
                                                              keyInfos,
                                                              count);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to MsgArgToCertificateIds"));
    }

    for (size_t cnt = 0; cnt < count; cnt++) {
        MembershipSummary summary;
        summary.issuer = keyInfos[cnt];
        summary.serial = serials[cnt].c_str();
        summaries.push_back(summary);
    }

    delete[] keyInfos;
    delete[] serials;

    return status;
}

QStatus ProxyObjectManager::GetManifest(const OnlineApplication& app,
                                        Manifest& manifest)
{
    QStatus status = ER_OK;

    SecurityApplicationProxy* remoteObj;
    status = GetProxyObject(app, ECDHE_DSA, &remoteObj);
    if (ER_OK != status) {
        // Errors logged in GetProxyObject.
        return status;
    }

    MsgArg rulesMsgArg;
    status = remoteObj->GetManifest(rulesMsgArg);
    ReleaseProxyObject(remoteObj);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetManifest"));
        return status;
    }

    PermissionPolicy::Rule* manifestRules;
    size_t manifestRulesCount;
    status = PermissionPolicy::ParseRules(rulesMsgArg, &manifestRules, &manifestRulesCount);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to ParseRules"));
        goto Exit;
    }

    status = manifest.SetFromRules(manifestRules, manifestRulesCount);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to SetFromRules"));
        goto Exit;
    }

Exit:
    delete[] manifestRules;
    return status;
}

void ProxyObjectManager::SessionLost(SessionId sessionId,
                                     SessionLostReason reason)
{
    QCC_UNUSED(reason);

    QCC_DbgPrintf(("Lost session %lu", (unsigned long)sessionId));
}
}
}
#undef QCC_MODULE
