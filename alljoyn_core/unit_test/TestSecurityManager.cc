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

#include <string>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/SecurityApplicationProxy.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include "TestSecurityManager.h"

#include <gtest/gtest.h>
#include "ajTestCommon.h"

using namespace std;
using namespace ajn;
using namespace qcc;

#define QCC_MODULE "SECURITY_TEST"

static QStatus GetAppPublicKey(BusAttachment& bus, ECCPublicKey& publicKey)
{
    KeyInfoNISTP256 keyInfo;
    QStatus status = bus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
    if (ER_OK != status) {
        return status;
    }
    publicKey = *keyInfo.GetPublicKey();
    return status;
}

TestSecurityManager::TestSecurityManager(string appName) :
    bus(appName.c_str()),
    opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY),
    authListener(), caKeyPair(), caPublicKeyInfo(), adminGroup(), identityGuid(),
    identityName("testIdentity"), certSerialNumber(0), policyVersion(0)
{
    QCC_VERIFY(ER_OK == caKeyPair.GenerateDSAKeyPair());

    caPublicKeyInfo.SetPublicKey(caKeyPair.GetDSAPublicKey());
    String aki;
    QCC_VERIFY(ER_OK == CertificateX509::GenerateAuthorityKeyId(caKeyPair.GetDSAPublicKey(), aki));
    caPublicKeyInfo.SetKeyId((uint8_t*)aki.data(), aki.size());
    bus.RegisterKeyStoreListener(keyStoreListener);
    IssueCertificate(*caKeyPair.GetDSAPublicKey(), caCertificate, true);
}

QStatus TestSecurityManager::Init() {
    QStatus status = ER_FAIL;

    status = bus.Start();
    if (ER_OK != status) {
        return status;
    }
    status = bus.Connect();
    if (ER_OK != status) {
        return status;
    }

    QCC_DbgHLPrintf(("%s: bus name = %s", __FUNCTION__, GetUniqueName().c_str()));

    status = bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &authListener);
    if (ER_OK != status) {
        return status;
    }

    status = ClaimSelf();
    if (ER_OK != status) {
        return status;
    }

    return InstallMembership(bus, adminGroup);
}

TestSecurityManager::~TestSecurityManager()
{
    bus.ClearKeyStore();
    bus.Disconnect();
    bus.Stop();
    bus.Join();
}

void TestSecurityManager::SessionLost(SessionId sessionId,
                                      SessionLostReason reason)
{
    QCC_UNUSED(sessionId);
    QCC_UNUSED(reason);
}

QStatus TestSecurityManager::ClaimSelf()
{
    PermissionPolicy::Rule::Member members[1];
    members[0].SetMemberName("*");
    members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                             PermissionPolicy::Rule::Member::ACTION_MODIFY |
                             PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    PermissionPolicy::Rule rules[1];
    rules[0].SetInterfaceName("*");
    rules[0].SetMembers(1, members);

    PermissionPolicy::Acl manifest;
    manifest.SetRules(1, rules);

    return Claim(bus, manifest);
}

void TestSecurityManager::IssueCertificate(const ECCPublicKey& appPubKey,
                                           CertificateX509& cert,
                                           bool isCA) {
    cert.SetSubjectPublicKey(&appPubKey);
    qcc::String aki;
    CertificateX509::GenerateAuthorityKeyId(&appPubKey, aki);
    cert.SetSubjectCN((uint8_t*)aki.data(), aki.size());

    cert.SetCA(isCA);

    CertificateX509::ValidPeriod period;
    uint64_t currentTime = GetEpochTimestamp() / 1000;
    period.validFrom = currentTime;
    period.validTo = period.validFrom + 3600 * 24 * 10 * 365;
    period.validFrom = period.validFrom - 3600;
    cert.SetValidity(&period);

    char buffer[33];
    snprintf(buffer, 32, "%x", ++certSerialNumber);
    cert.SetSerial((const uint8_t*)buffer, strlen(buffer));
    cert.SetIssuerCN(caPublicKeyInfo.GetKeyId(), caPublicKeyInfo.GetKeyIdLen());
    QCC_VERIFY(ER_OK == cert.SignAndGenerateAuthorityKeyId(caKeyPair.GetDSAPrivateKey(), caKeyPair.GetDSAPublicKey()));
}

void TestSecurityManager::GenerateIdentityCertificate(const ECCPublicKey& appPubKey,
                                                      const PermissionPolicy::Acl& manifest,
                                                      IdentityCertificate& cert)
{
    QCC_UNUSED(manifest);
    cert.SetAlias(identityGuid.ToString());
    cert.SetSubjectOU((const uint8_t*)identityName.data(), identityName.size());

    IssueCertificate(appPubKey, cert);
}

void TestSecurityManager::GenerateMembershipCertificate(const ECCPublicKey& appPubKey,
                                                        const GUID128& group,
                                                        MembershipCertificate& cert)
{
    cert.SetGuild(group);

    IssueCertificate(appPubKey, cert);
}

void TestSecurityManager::AddAdminAcl(const PermissionPolicy& in,
                                      PermissionPolicy& out) {
    PermissionPolicy::Acl adminAcl;

    PermissionPolicy::Peer peers[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
    peers[0].SetSecurityGroupId(adminGroup);
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(caPublicKeyInfo.GetPublicKey());
    peers[0].SetKeyInfo(&keyInfo);
    adminAcl.SetPeers(1, peers);

    PermissionPolicy::Rule rules[1];
    rules[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member prms[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(
        PermissionPolicy::Rule::Member::ACTION_PROVIDE |
        PermissionPolicy::Rule::Member::ACTION_OBSERVE |
        PermissionPolicy::Rule::Member::ACTION_MODIFY
        );
    rules[0].SetMembers(1, prms);
    adminAcl.SetRules(1, rules);

    vector<PermissionPolicy::Acl> acls;
    acls.push_back(adminAcl);
    for (size_t i = 0; i < in.GetAclsSize(); i++) {
        acls.push_back(in.GetAcls()[i]);
    }
    out.SetAcls(acls.size(), &acls[0]);
}

QStatus TestSecurityManager::Claim(BusAttachment& peerBus, const PermissionPolicy::Acl& manifest)
{
    QStatus status = ER_FAIL;

    SessionId sessionId;
    qcc::String peerBusName = peerBus.GetUniqueName();

    EXPECT_EQ(ER_OK, (status = bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &authListener)));
    if (ER_OK != status) {
        return status;
    }

    EXPECT_EQ(ER_OK, (status = peerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE)));

    EXPECT_EQ(ER_OK, (status = bus.JoinSession(peerBusName.c_str(), ALLJOYN_SESSIONPORT_PERMISSION_MGMT, this, sessionId, opts)));
    if (ER_OK != status) {
        return status;
    }

    SecurityApplicationProxy peerProxy(bus, peerBusName.c_str(), sessionId);

    ECCPublicKey appPublicKey;
    EXPECT_EQ(ER_OK, (status = GetAppPublicKey(peerBus, appPublicKey)));
    if (ER_OK != status) {
        return status;
    }

    IdentityCertificate identityCert;
    CertificateX509 identityCertChain[2];
    GenerateIdentityCertificate(appPublicKey, manifest, identityCert);

    identityCertChain[0] = identityCert;
    identityCertChain[1] = caCertificate;

    const PermissionPolicy::Rule* manifestRules = manifest.GetRules();
    size_t manifestSize = manifest.GetRulesSize();

    Manifest manifests[1];
    EXPECT_EQ(ER_OK, (status = manifests[0]->SetRules(manifestRules, manifestSize)));
    if (ER_OK != status) {
        return status;
    }
    EXPECT_EQ(ER_OK, (status = manifests[0]->ComputeThumbprintAndSign(identityCert, caKeyPair.GetDSAPrivateKey())));
    if (ER_OK != status) {
        return status;
    }

    EXPECT_EQ(ER_OK, (status = peerProxy.Claim(caPublicKeyInfo, adminGroup, caPublicKeyInfo, identityCertChain, ArraySize(identityCertChain), manifests, ArraySize(manifests))));
    if (ER_OK != status) {
        return status;
    }

    // returns ER_ALLJOYN_LEAVESESSION_REPLY_NO_SESSION during ClaimSelf
    status = bus.LeaveSession(sessionId);
    EXPECT_TRUE((ER_OK == status) || (ER_ALLJOYN_LEAVESESSION_REPLY_NO_SESSION == status));
    if (ER_ALLJOYN_LEAVESESSION_REPLY_NO_SESSION == status) {
        status = ER_OK;
    }

    return status;
}

QStatus TestSecurityManager::UpdateIdentity(BusAttachment& peerBus,
                                            const PermissionPolicy::Acl& manifest)
{
    QStatus status = ER_FAIL;

    SessionId sessionId;
    qcc::String peerBusName = peerBus.GetUniqueName();

    status = bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &authListener);
    if (ER_OK != status) {
        return status;
    }

    status = bus.JoinSession(peerBusName.c_str(), ALLJOYN_SESSIONPORT_PERMISSION_MGMT,
                             this, sessionId, opts);
    if (ER_OK != status) {
        return status;
    }

    SecurityApplicationProxy peerProxy(bus, peerBusName.c_str(), sessionId);

    ECCPublicKey appPublicKey;
    status = GetAppPublicKey(peerBus, appPublicKey);
    if (ER_OK != status) {
        return status;
    }

    IdentityCertificate identityCert;
    CertificateX509 identityCertChain[2];
    GenerateIdentityCertificate(appPublicKey, manifest, identityCert);
    identityCertChain[0] = identityCert;
    identityCertChain[1] = caCertificate;

    const PermissionPolicy::Rule* manifestRules = manifest.GetRules();
    size_t manifestSize = manifest.GetRulesSize();

    Manifest manifests[1];
    status = manifests[0]->SetRules(manifestRules, manifestSize);
    if (ER_OK != status) {
        return status;
    }
    status = manifests[0]->ComputeThumbprintAndSign(identityCert, caKeyPair.GetDSAPrivateKey());
    if (ER_OK != status) {
        return status;
    }

    status = peerProxy.UpdateIdentity(identityCertChain, ArraySize(identityCertChain), manifests, ArraySize(manifests));
    if (ER_OK != status) {
        return status;
    }
    status = peerProxy.SecureConnection(true);
    if (ER_OK != status) {
        return status;
    }
    return bus.LeaveSession(sessionId);
}

QStatus TestSecurityManager::InstallMembership(BusAttachment& peerBus, const GUID128& group)
{
    QStatus status = ER_FAIL;

    SessionId sessionId;
    qcc::String peerBusName = peerBus.GetUniqueName();

    status = bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &authListener);
    if (ER_OK != status) {
        return status;
    }

    status = bus.JoinSession(peerBusName.c_str(), ALLJOYN_SESSIONPORT_PERMISSION_MGMT,
                             this, sessionId, opts);
    if (ER_OK != status) {
        return status;
    }

    SecurityApplicationProxy peerProxy(bus, peerBusName.c_str(), sessionId);

    ECCPublicKey appPublicKey;
    status = GetAppPublicKey(peerBus, appPublicKey);
    if (ER_OK != status) {
        return status;
    }

    CertificateX509 membershipCertChain[2];
    MembershipCertificate membershipCert;
    GenerateMembershipCertificate(appPublicKey, group, membershipCert);
    membershipCertChain[0] = membershipCert;
    membershipCertChain[1] = caCertificate;

    status = peerProxy.InstallMembership(membershipCertChain, ArraySize(membershipCertChain));
    if (ER_OK != status) {
        return status;
    }

    status = bus.LeaveSession(sessionId);
    // returns ER_ALLJOYN_LEAVESESSION_REPLY_NO_SESSION during InstallMembershipSelf
    if (ER_ALLJOYN_LEAVESESSION_REPLY_NO_SESSION == status) {
        status = ER_OK;
    }

    return status;
}

QStatus TestSecurityManager::UpdatePolicy(const BusAttachment& peerBus, const PermissionPolicy& policy)
{
    QStatus status = ER_FAIL;

    SessionId sessionId;
    qcc::String peerBusName = peerBus.GetUniqueName();

    EXPECT_EQ(ER_OK, (status = bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &authListener)));
    if (ER_OK != status) {
        return status;
    }

    EXPECT_EQ(ER_OK, (status = bus.JoinSession(peerBusName.c_str(), ALLJOYN_SESSIONPORT_PERMISSION_MGMT, this, sessionId, opts)));
    if (ER_OK != status) {
        return status;
    }

    SecurityApplicationProxy peerProxy(bus, peerBusName.c_str(), sessionId);

    PermissionPolicy copy;
    copy.SetVersion(++policyVersion);
    AddAdminAcl(policy, copy);

    EXPECT_EQ(ER_OK, (status = peerProxy.StartManagement()));
    if (ER_OK != status) {
        return status;
    }

    EXPECT_EQ(ER_OK, (status = peerProxy.UpdatePolicy(copy)));
    if (ER_OK != status) {
        return status;
    }

    EXPECT_EQ(ER_OK, (status = peerProxy.SecureConnection(true)));
    if (ER_OK != status) {
        return status;
    }

    EXPECT_EQ(ER_OK, (status = peerProxy.EndManagement()));
    if (ER_OK != status) {
        return status;
    }

    EXPECT_EQ(ER_OK, (status = bus.LeaveSession(sessionId)));
    /*
     * The peer app doesn't have a session listener for this security management session.
     * Therefore there is no good way to wait here for the peer app to finish processing
     * LeaveSession.
     *
     * However, it is important to finish that work on the peer app's side because
     * otherwise the peer app might have not updated yet its ConnectedPeers information.
     * The peer app can then call SecureConnection(nullptr) and that will try to secure
     * the session that is going away here, with unpredictable results.
     *
     * In an attempt to mitigate the problems described above, sleep for a little while,
     * hoping that the peer app gets a chance to finish updating its ConnectedPeers.
     */
    qcc::Sleep(1000);

    return status;
}

QStatus TestSecurityManager::Reset(const BusAttachment& peerBus)
{
    QStatus status = ER_FAIL;

    SessionId sessionId;
    qcc::String peerBusName = peerBus.GetUniqueName();

    status = bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &authListener);
    if (ER_OK != status) {
        return status;
    }

    status = bus.JoinSession(peerBusName.c_str(), ALLJOYN_SESSIONPORT_PERMISSION_MGMT,
                             this, sessionId, opts);
    if (ER_OK != status) {
        return status;
    }

    SecurityApplicationProxy peerProxy(bus, peerBusName.c_str(), sessionId);

    status = peerProxy.Reset();
    if (ER_OK != status) {
        return status;
    }

    return bus.LeaveSession(sessionId);
}
