/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
Mutex ProxyObjectManager::lock;

ProxyObjectManager::ProxyObjectManager(BusAttachment* ba) :
    bus(ba)
{
}

ProxyObjectManager::~ProxyObjectManager()
{
}

QStatus ProxyObjectManager::GetProxyObject(ManagedProxyObject& managedProxy,
                                           SessionType sessionType,
                                           AuthListener* authListener)
{
    QStatus status = ER_FAIL;
    const char* busName = managedProxy.remoteApp.busName.c_str();
    if (strlen(busName) == 0) {
        status = ER_FAIL;
        QCC_DbgRemoteError(("Application is offline"));
        return status;
    }

    lock.Lock(__FILE__, __LINE__);

    if (sessionType == ECDHE_NULL) {
        bus->EnablePeerSecurity(KEYX_ECDHE_NULL, listener, AJNKEY_STORE, true);
    } else if (sessionType == ECDHE_DSA) {
        bus->EnablePeerSecurity(ECDHE_KEYX, listener, AJNKEY_STORE, true);
    } else if (sessionType == ECDHE_PSK) {
        bus->EnablePeerSecurity(KEYX_ECDHE_PSK, authListener ? authListener : listener, AJNKEY_STORE, true);
    }

    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                     SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = bus->JoinSession(busName, ALLJOYN_SESSIONPORT_PERMISSION_MGMT,
                              this, sessionId, opts);
    if (status != ER_OK) {
        QCC_DbgRemoteError(("Could not join session with %s", busName));
        if (authListener) {
            bus->EnablePeerSecurity(KEYX_ECDHE_NULL, listener, AJNKEY_STORE, true);
        }
        lock.Unlock(__FILE__, __LINE__);
        return status;
    }

    managedProxy.remoteObj = new SecurityApplicationProxy(*bus, busName, sessionId);
    managedProxy.resetAuthListener = (authListener != nullptr);
    managedProxy.proxyObjectManager = this;
    return status;
}

QStatus ProxyObjectManager::ReleaseProxyObject(SecurityApplicationProxy* remoteObject,
                                               bool resetListener)
{
    SessionId sessionId = remoteObject->GetSessionId();
    delete remoteObject;
    remoteObject = nullptr;
    QStatus status =  bus->LeaveSession(sessionId);
    if (resetListener) {
        bus->EnablePeerSecurity(KEYX_ECDHE_NULL, listener, AJNKEY_STORE, true);
    }
    lock.Unlock(__FILE__, __LINE__);
    return status;
}

ProxyObjectManager::ManagedProxyObject::~ManagedProxyObject()
{
    if (remoteObj != nullptr) {
        assert(proxyObjectManager);
        proxyObjectManager->ReleaseProxyObject(remoteObj, resetAuthListener);
    }
}

QStatus ProxyObjectManager::ManagedProxyObject::Claim(KeyInfoNISTP256& certificateAuthority,
                                                      GroupInfo& adminGroup,
                                                      IdentityCertificateChain identityCertChain,
                                                      const Manifest& manifest)
{
    CheckReAuthenticate();
    PermissionPolicy::Rule* manifestRules;
    size_t manifestRulesCount;
    QStatus status = manifest.GetRules(&manifestRules, &manifestRulesCount);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to get manifest rules"));
    }
    if (ER_OK == status) {
        size_t identityCertChainSize = identityCertChain.size();
        IdentityCertificate* identityCertChainArray = new IdentityCertificate[identityCertChainSize];
        for (size_t i = 0; i < identityCertChainSize; i++) {
            identityCertChainArray[i] = identityCertChain[i];
        }

        status = remoteObj->Claim(certificateAuthority,
                                  adminGroup.guid, adminGroup.authority,
                                  identityCertChainArray, identityCertChainSize,
                                  manifestRules, manifestRulesCount);
        delete[] identityCertChainArray;
        identityCertChainArray = nullptr;
    }

    delete[] manifestRules;
    manifestRules = nullptr;

    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::GetIdentity(IdentityCertificateChain& certChain)
{
    CheckReAuthenticate();
    MsgArg certChainArg;
    QStatus status = remoteObj->GetIdentity(certChainArg);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetIndentity"));
        return status;
    }

    if (qcc::String("a(yay)") != certChainArg.Signature()) {
        status = ER_BUS_SIGNATURE_MISMATCH;
        QCC_LogError(status, ("Failed to GetIndentity - signature mismatch"));
        return status;
    }

    size_t size = certChainArg.v_array.GetNumElements();
    IdentityCertificate* certs = new IdentityCertificate[size];
    status = SecurityApplicationProxy::MsgArgToIdentityCertChain(certChainArg, certs, size);
    if (ER_OK == status) {
        for (size_t i = 0; i < size; i++) {
            certChain.push_back(certs[i]);
        }
    } else {
        QCC_LogError(status, ("Failed to MsdArgToIdentityCertChain"));
    }
    delete[] certs;
    certs = nullptr;
    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::UpdateIdentity(IdentityCertificateChain certChain,
                                                               const Manifest& mf)
{
    CheckReAuthenticate();
    size_t chainSize = certChain.size();
    IdentityCertificate* certArray = new IdentityCertificate[chainSize];

    for (size_t i = 0; i < chainSize; i++) {
        certArray[i] = certChain[i];
    }

    PermissionPolicy::Rule* manifestRules;
    size_t manifestRuleCount;
    QStatus status = mf.GetRules(&manifestRules, &manifestRuleCount);

    if (ER_OK == status) {
        status = remoteObj->UpdateIdentity(certArray, chainSize, manifestRules, manifestRuleCount);
        if (ER_OK == status) {
            needReAuth = true;
        }
    }

    delete[] certArray;
    certArray = nullptr;
    delete[] manifestRules;
    manifestRules = nullptr;

    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::InstallMembership(const MembershipCertificateChain certChainVector)
{
    CheckReAuthenticate();
    size_t size = certChainVector.size();
    MembershipCertificate* certChain = new MembershipCertificate[size];
    for (size_t i = 0; i < size; i++) {
        certChain[i] = certChainVector[i];
    }

    QStatus status = remoteObj->InstallMembership(certChain, size);

    if (ER_OK == status) {
        needReAuth = true;
    }
    delete[] certChain;
    certChain = nullptr;
    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::GetPolicy(PermissionPolicy& policy)
{
    CheckReAuthenticate();
    QStatus status = remoteObj->GetPolicy(policy);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetPolicy"));
    }

    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::GetPolicyVersion(uint32_t&  policyVersion)
{
    CheckReAuthenticate();
    QStatus status = remoteObj->GetPolicyVersion(policyVersion);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetPolicyVersion"));
    }

    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::GetDefaultPolicy(PermissionPolicy& policy)
{
    CheckReAuthenticate();
    QStatus status = remoteObj->GetDefaultPolicy(policy);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetDefaultPolicy"));
    }

    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::UpdatePolicy(const PermissionPolicy& policy)
{
    CheckReAuthenticate();
    QStatus status = remoteObj->UpdatePolicy(policy);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to UpdatePolicy"));
    } else {
        needReAuth = true;
    }

    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::ResetPolicy()
{
    CheckReAuthenticate();
    QStatus status = remoteObj->ResetPolicy();
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to ResetPolicy"));
    } else {
        needReAuth = true;
    }

    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::GetClaimCapabilities(
    PermissionConfigurator::ClaimCapabilities& claimCapabilities,
    PermissionConfigurator::ClaimCapabilityAdditionalInfo& claimCapInfo)
{
    CheckReAuthenticate();
    MsgArg rulesMsgArg;
    QStatus status = remoteObj->GetClaimCapabilities(claimCapabilities);
    if (ER_OK != status) {
        QCC_LogError(status, ("GetClaimCapabilities failed"));
    } else {
        status = remoteObj->GetClaimCapabilityAdditionalInfo(claimCapInfo);
        if (ER_OK != status) {
            QCC_LogError(status, ("GetClaimCapabilityAdditionalInfo failed"));
        }
    }
    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::GetManifestTemplate(Manifest& manifest)
{
    CheckReAuthenticate();
    MsgArg rulesMsgArg;
    QStatus status = remoteObj->GetManifestTemplate(rulesMsgArg);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetManifestTemplate"));
        return status;
    }

    PermissionPolicy::Rule* manifestRules = nullptr;
    size_t manifestRulesCount;
    status = PermissionPolicy::ParseRules(rulesMsgArg, &manifestRules, &manifestRulesCount);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to ParseRules"));
        goto Exit;
    }

    if (0 == manifestRulesCount) {
        status = ER_MANIFEST_NOT_FOUND;
        QCC_LogError(status, ("Manifest does not contain rules"));
        goto Exit;
    }

    status = manifest.SetFromRules(manifestRules, manifestRulesCount);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to SetFromRules"));
        goto Exit;
    }

Exit:
    delete[] manifestRules;
    manifestRules = nullptr;
    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::Reset()
{
    CheckReAuthenticate();
    QStatus status = remoteObj->Reset();
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to Reset"));
    }

    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::RemoveMembership(const string& serial,
                                                                 const KeyInfoNISTP256& issuerKeyInfo)
{
    CheckReAuthenticate();
    QStatus status = remoteObj->RemoveMembership(serial.c_str(), issuerKeyInfo);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to RemoveMembership"));
    } else {
        needReAuth = true;
    }

    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::GetMembershipSummaries(vector<MembershipSummary>& summaries)
{
    CheckReAuthenticate();
    QStatus status = ER_FAIL;
    MsgArg replyMsgArg;
    status = remoteObj->GetMembershipSummaries(replyMsgArg);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetMembershipSummaries"));
        return status;
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
    } else {
        for (size_t cnt = 0; cnt < count; cnt++) {
            MembershipSummary summary;
            summary.issuer = keyInfos[cnt];
            summary.serial = serials[cnt].c_str();
            summaries.push_back(summary);
        }
    }
    delete[] keyInfos;
    keyInfos = nullptr;
    delete[] serials;
    serials = nullptr;

    return status;
}

QStatus ProxyObjectManager::ManagedProxyObject::GetManifest(Manifest& manifest)
{
    CheckReAuthenticate();
    MsgArg rulesMsgArg;
    QStatus status = remoteObj->GetManifest(rulesMsgArg);
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
    manifestRules = nullptr;
    return status;
}

void ProxyObjectManager::ManagedProxyObject::CheckReAuthenticate()
{
    if (needReAuth) {
        remoteObj->SecureConnection(true);
        needReAuth = false;
    }
}

void ProxyObjectManager::SessionLost(SessionId sessionId,
                                     SessionLostReason reason)
{
    QCC_UNUSED(reason);
    QCC_UNUSED(sessionId);  // For Release builds.

    QCC_DbgPrintf(("Lost session %lu", (unsigned long)sessionId));
}
}
}
#undef QCC_MODULE
