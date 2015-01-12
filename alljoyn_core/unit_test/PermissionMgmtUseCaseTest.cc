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

#include "PermissionMgmtTest.h"
#include <string>

using namespace ajn;
using namespace qcc;

static const char* PERMISSION_MGMT_PATH = "/org/allseen/Security/PermissionMgmt";

static GUID128 membershipGUID1;
static const char* membershipSerial1 = "10001";
static GUID128 membershipGUID2;
static GUID128 membershipGUID3;
static const char* membershipSerial3 = "30003";

static const char sampleCertificatePEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "AAAAAf8thIwHzhCU8qsedyuEldP/TouX6w7rZI/cJYST/kexAAAAAMvbuy8JDCJI\n"
    "Ms8vwkglUrf/infSYMNRYP/gsFvl5FutAAAAAAAAAAD/LYSMB84QlPKrHncrhJXT\n"
    "/06Ll+sO62SP3CWEk/5HsQAAAADL27svCQwiSDLPL8JIJVK3/4p30mDDUWD/4LBb\n"
    "5eRbrQAAAAAAAAAAAAAAAAASgF0AAAAAABKBiQABMa7uTLSqjDggO0t6TAgsxKNt\n"
    "+Zhu/jc3s242BE0drFU12USXXIYQdqps/HrMtqw6q9hrZtaGJS+e9y7mJegAAAAA\n"
    "APpeLT1cHNm3/OupnEcUCmg+jqi4SUEi4WTWSR4OzvCSAAAAAA==\n"
    "-----END CERTIFICATE-----"
};

static PermissionPolicy* GeneratePolicy(qcc::GUID128& guid, qcc::ECCPublicKey& adminPublicKey, qcc::ECCPublicKey& guildAuthority)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(74892317);

    /* add the admin section */
    PermissionPolicy::Peer* admins = new PermissionPolicy::Peer[1];
    admins[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guid.GetBytes(), guid.SIZE);
    keyInfo->SetPublicKey(&adminPublicKey);
    admins[0].SetKeyInfo(keyInfo);
    policy->SetAdmins(1, admins);

    /* add the provider section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[4];

    /* terms record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Off");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    prms[1].SetMemberName("*");
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);
    terms[0].SetRules(1, rules);

    /* terms record 1 GUILD membershipGUID1 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(membershipGUID1.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthority);
    peers[0].SetKeyInfo(keyInfo);
    terms[1].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("Channel");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(3, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);
    terms[1].SetRules(2, rules);

    /* terms record 2 GUILD membershipGUID2 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(membershipGUID2.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthority);
    peers[0].SetKeyInfo(keyInfo);
    terms[2].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[2];
    rules[0].SetObjPath("/control/settings");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    rules[0].SetMembers(1, prms);
    rules[1].SetObjPath("*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);
    terms[2].SetRules(2, rules);

    /* terms record 3 peer specific rule  */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetPublicKey(&guildAuthority);
    peers[0].SetKeyInfo(keyInfo);
    terms[3].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("Mute");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(1, prms);
    terms[3].SetRules(1, rules);

    policy->SetTerms(4, terms);

    return policy;
}

static PermissionPolicy* GenerateMembeshipAuthData()
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(88473);

    /* add the provider section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0 */
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);

    terms[0].SetRules(1, rules);
    policy->SetTerms(1, terms);

    return policy;
}

static QStatus GenerateManifest(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    *retRules = rules;
    return ER_OK;
}

class PermissionMgmtUseCaseTest : public BasePermissionMgmtTest {

  public:
    PermissionMgmtUseCaseTest() : BasePermissionMgmtTest("/app")
    {
    }

    /**
     *  Claim the admin app
     */
    void ClaimAdmin()
    {
        QStatus status = ER_OK;

        /* factory reset */
        PermissionConfigurator& pc = adminBus.GetPermissionConfigurator();
        status = pc.Reset();
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        /* Gen DSA keys */
        status = pc.GenerateSigningKeyPair();
        EXPECT_EQ(ER_OK, status) << "  GenerateSigningKeyPair failed.  Actual Status: " << QCC_StatusText(status);

        /* Retrieve the DSA keys */
        ECCPrivateKey issuerPrivateKey;
        ECCPublicKey issuerPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
        EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(adminProxyBus, adminBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
        ProxyBusObject clientProxyObject(adminProxyBus, adminBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
        ECCPublicKey claimedPubKey;
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);

        qcc::String der;
        status = PermissionMgmtTestHelper::CreateIdentityCert("1010101", issuerGUID, &issuerPrivateKey, issuerGUID, &issuerPubKey, "Admin User", der);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::Claim(adminProxyBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, issuerGUID, der);
        EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

        /* retrieve back the identity cert to compare */
        IdentityCertificate newCert;
        status = PermissionMgmtTestHelper::GetIdentity(adminProxyBus, clientProxyObject, newCert);
        EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String retIdentity;
        status = newCert.EncodeCertificateDER(retIdentity);
        EXPECT_EQ(ER_OK, status) << "  newCert.EncodeCertificateDER failed.  Actual Status: " << QCC_StatusText(status);
        EXPECT_STREQ(der.c_str(), retIdentity.c_str()) << "  GetIdentity failed.  Return value does not equal original";
    }

    /**
     *  Claim the service app
     */
    void ClaimService()
    {
        QStatus status = ER_OK;
        /* factory reset */
        PermissionConfigurator& pc = serviceBus.GetPermissionConfigurator();
        status = pc.Reset();
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(adminBus, serviceBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);

        SetNotifyConfigSignalReceived(false);

        /* setup state unclaimable */
        PermissionConfigurator::ClaimableState claimableState = pc.GetClaimableState();
        EXPECT_EQ(PermissionConfigurator::STATE_CLAIMABLE, claimableState) << "  ClaimableState is not CLAIMABLE";
        status = pc.SetClaimable(false);
        EXPECT_EQ(ER_OK, status) << "  SetClaimable failed.  Actual Status: " << QCC_StatusText(status);
        claimableState = pc.GetClaimableState();
        EXPECT_EQ(PermissionConfigurator::STATE_UNCLAIMABLE, claimableState) << "  ClaimableState is not UNCLAIMABLE";
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        ECCPrivateKey issuerPrivateKey;
        ECCPublicKey issuerPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
        EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

        ECCPublicKey claimedPubKey;
        /* retrieve public key from to-be-claimed app to create identity cert */
        status = PermissionMgmtTestHelper::GetPeerPublicKey(adminBus, clientProxyObject, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  GetPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
        /* create identity cert for the claimed app */
        qcc::String der;
        status = PermissionMgmtTestHelper::CreateIdentityCert("2020202", issuerGUID, &issuerPrivateKey, serviceGUID, &claimedPubKey, "Service Provider", der);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);

        /* try claiming with state unclaimable.  Exptect to fail */
        status = PermissionMgmtTestHelper::Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, serviceGUID, der);
        EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

        /* now switch it back to claimable */
        status = pc.SetClaimable(true);
        EXPECT_EQ(ER_OK, status) << "  SetClaimable failed.  Actual Status: " << QCC_StatusText(status);
        claimableState = pc.GetClaimableState();
        EXPECT_EQ(PermissionConfigurator::STATE_CLAIMABLE, claimableState) << "  ClaimableState is not CLAIMABLE";

        /* try claiming with state laimable.  Exptect to succeed */
        status = PermissionMgmtTestHelper::Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, serviceGUID, der);
        EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

        /* try to claim one more time */
        status = PermissionMgmtTestHelper::Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, serviceGUID, der);
        EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

        ECCPublicKey claimedPubKey2;
        /* retrieve public key from claimed app to validate that it is not changed */
        status = PermissionMgmtTestHelper::GetPeerPublicKey(adminBus, clientProxyObject, &claimedPubKey2);
        EXPECT_EQ(ER_OK, status) << "  GetPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(memcmp(&claimedPubKey2, &claimedPubKey, sizeof(ECCPublicKey)), 0) << "  The public key of the claimed app has changed.";

        /* sleep a second to see whether the NotifyConfig signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetNotifyConfigSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";

        status = adminBus.LeaveSession(sessionId);
        EXPECT_EQ(ER_OK, status) << "  LeaveSession failed.  Actual Status: " << QCC_StatusText(status);
    }
    /**
     *  Claim the consumer
     */
    void ClaimConsumer()
    {
        QStatus status = ER_OK;
        /* factory reset */
        PermissionConfigurator& pc = consumerBus.GetPermissionConfigurator();
        status = pc.Reset();
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(adminBus, consumerBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
        ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
        ECCPublicKey claimedPubKey;

        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        ECCPrivateKey issuerPrivateKey;
        ECCPublicKey issuerPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
        EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);
        /* retrieve public key from to-be-claimed app to create identity cert */
        status = PermissionMgmtTestHelper::GetPeerPublicKey(adminBus, clientProxyObject, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  GetPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
        /* create identity cert for the claimed app */
        qcc::String der;
        status = PermissionMgmtTestHelper::CreateIdentityCert("3030303", issuerGUID, &issuerPrivateKey, serviceGUID, &claimedPubKey, "Consumer", der);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);
        SetNotifyConfigSignalReceived(false);
        status = PermissionMgmtTestHelper::Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, consumerGUID, der);
        EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

        /* try to claim a second time */
        status = PermissionMgmtTestHelper::Claim(adminBus, clientProxyObject, issuerGUID, &issuerPubKey, &claimedPubKey, consumerGUID, der);
        EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

        /* sleep a second to see whether the NotifyConfig signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetNotifyConfigSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
        status = adminBus.LeaveSession(sessionId);
        EXPECT_EQ(ER_OK, status) << "  LeaveSession failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Install policy to service app
     */
    void InstallPolicyToService(PermissionPolicy& policy)
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        SetNotifyConfigSignalReceived(false);
        status = PermissionMgmtTestHelper::InstallPolicy(adminBus, clientProxyObject, policy);
        EXPECT_EQ(ER_OK, status) << "  InstallPolicy failed.  Actual Status: " << QCC_StatusText(status);

        /* retrieve back the policy to compare */
        PermissionPolicy retPolicy;
        status = PermissionMgmtTestHelper::GetPolicy(adminBus, clientProxyObject, retPolicy);
        EXPECT_EQ(ER_OK, status) << "  GetPolicy failed.  Actual Status: " << QCC_StatusText(status);

        EXPECT_EQ(policy.GetSerialNum(), retPolicy.GetSerialNum()) << " GetPolicy failed. Different serial number.";
        EXPECT_EQ(policy.GetAdminsSize(), retPolicy.GetAdminsSize()) << " GetPolicy failed. Different admin size.";
        EXPECT_EQ(policy.GetTermsSize(), retPolicy.GetTermsSize()) << " GetPolicy failed. Different provider size.";
        /* sleep a second to see whether the NotifyConfig signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetNotifyConfigSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
    }
    /*
     *  Replace service app Identity Certificate
     */
    void ReplaceServiceIdentityCert()
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        /* retrieve the current identity cert */
        IdentityCertificate cert;
        status = PermissionMgmtTestHelper::GetIdentity(adminBus, clientProxyObject, cert);
        EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);

        /* create a new identity cert */
        ECCPrivateKey issuerPrivateKey;
        ECCPublicKey issuerPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
        EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        qcc::String der;
        status = PermissionMgmtTestHelper::CreateIdentityCert("4040404", issuerGUID, &issuerPrivateKey, cert.GetSubject(), cert.GetSubjectPublicKey(), "Service Provider", der);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);

        status = PermissionMgmtTestHelper::InstallIdentity(adminBus, clientProxyObject, der);
        EXPECT_EQ(ER_OK, status) << "  InstallIdentity failed.  Actual Status: " << QCC_StatusText(status);

        /* retrieve back the identity cert to compare */
        IdentityCertificate newCert;
        status = PermissionMgmtTestHelper::GetIdentity(adminBus, clientProxyObject, newCert);
        EXPECT_EQ(ER_OK, status) << "  GetIdentity failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String retIdentity;
        status = newCert.EncodeCertificateDER(retIdentity);
        EXPECT_EQ(ER_OK, status) << "  newCert.EncodeCertificateDER failed.  Actual Status: " << QCC_StatusText(status);
        EXPECT_STREQ(der.c_str(), retIdentity.c_str()) << "  GetIdentity failed.  Return value does not equal original";

    }

    /**
     *  Install Membership to service provider
     */
    void InstallMembershipToServiceProvider(PermissionPolicy* membershipAuthData)
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(serviceBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembership RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::InstallMembership(membershipSerial3, adminBus, clientProxyObject, adminBus, serviceGUID, &claimedPubKey, membershipGUID3, membershipAuthData);
        EXPECT_EQ(ER_OK, status) << "  InstallMembership cert1 failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::InstallMembership(membershipSerial3, adminBus, clientProxyObject, adminBus, serviceGUID, &claimedPubKey, membershipGUID3, membershipAuthData);
        EXPECT_NE(ER_OK, status) << "  InstallMembership cert1 again is supposed to fail.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  RemoveMembership from service provider
     */
    void RemoveMembershipFromServiceProvider()
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        EXPECT_EQ(ER_OK, status) << "  GetGuid failed.  Actual Status: " << QCC_StatusText(status);

        status = PermissionMgmtTestHelper::RemoveMembership(adminBus, clientProxyObject, membershipSerial3, issuerGUID);
        EXPECT_EQ(ER_OK, status) << "  RemoveMembershipFromServiceProvider failed.  Actual Status: " << QCC_StatusText(status);

        /* removing it again */
        status = PermissionMgmtTestHelper::RemoveMembership(adminBus, clientProxyObject, membershipSerial3, issuerGUID);
        EXPECT_NE(ER_OK, status) << "  RemoveMembershipFromServiceProvider succeeded.  Expect it to fail.  Actual Status: " << QCC_StatusText(status);

    }

    /**
     *  Install Membership to a consumer
     */
    void InstallMembershipToConsumer(PermissionPolicy* membershipAuthData)
    {
        ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::InstallMembership(membershipSerial1, adminBus, clientProxyObject, adminBus, consumerGUID, &claimedPubKey, membershipGUID1, membershipAuthData);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer cert1 failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Test PermissionMgmt InstallGuildEquivalence method
     */
    void InstallGuildEquivalence()
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
        status = PermissionMgmtTestHelper::InstallGuildEquivalence(adminBus, clientProxyObject, sampleCertificatePEM);
        EXPECT_EQ(ER_OK, status) << "  InstallGuildEquivalence failed.  Actual Status: " << QCC_StatusText(status);

    }

    /**
     *  any user can call TV On but not Off
     */
    void AnyUserCanCallOnAndNotOff()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOn(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AccessOnOffByAnyUser ExcerciseOn failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::ExcerciseOff(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AccessOffByAnyUser ExcersizeOff did not fail.  Actual Status: " << QCC_StatusText(status);
    }
    /**
     *  Guild member can turn up/down but can't specify a channel
     */
    void GuildMemberCanTVUpAndDownAndNotChannel()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVUp(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AccessTVByUserWithGuildMembership ExcerciseTVUp failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::ExcerciseTVDown(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AccessTVByUserWithGuildMembership ExcerciseTVDown failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::ExcerciseTVChannel(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AccessTVByUserWithGuildMembership ExcerciseTVChannel did not fail.  Actual Status: " << QCC_StatusText(status);
    }

    /*
     *  Set the manifest for the service provider
     */
    void SetPermissionManifestOnServiceProvider()
    {

        PermissionPolicy::Rule* rules = NULL;
        size_t count = 0;
        QStatus status = GenerateManifest(&rules, &count);
        EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest GenerateManifest failed.  Actual Status: " << QCC_StatusText(status);
        PermissionConfigurator& pc = serviceBus.GetPermissionConfigurator();
        status = pc.SetPermissionManifest(rules, count);
        EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest SetPermissionManifest failed.  Actual Status: " << QCC_StatusText(status);

        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
        PermissionPolicy::Rule* retrievedRules = NULL;
        size_t retrievedCount = 0;
        status = PermissionMgmtTestHelper::GetManifest(consumerBus, clientProxyObject, &retrievedRules, &retrievedCount);
        EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest GetManifest failed.  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(count, retrievedCount) << "  SetPermissionManifest GetManifest failed to retrieve the same count.";
        delete [] rules;
        delete [] retrievedRules;
    }

    /*
     *  Remove Policy from service provider
     */
    void RemovePolicyFromServiceProvider()
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        /* remove the policy */
        SetNotifyConfigSignalReceived(false);
        status = PermissionMgmtTestHelper::RemovePolicy(adminBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  RemovePolicy failed.  Actual Status: " << QCC_StatusText(status);

        /* get policy again.  Expect it to fail */
        PermissionPolicy retPolicy;
        status = PermissionMgmtTestHelper::GetPolicy(adminBus, clientProxyObject, retPolicy);
        EXPECT_NE(ER_OK, status) << "  GetPolicy did not fail.  Actual Status: " << QCC_StatusText(status);
        /* sleep a second to see whether the NotifyConfig signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetNotifyConfigSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
    }

    /*
     * Remove Membership from consumer.
     */
    void RemoveMembershipFromConsumer()
    {
        ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        status = PermissionMgmtTestHelper::RemoveMembership(adminBus, clientProxyObject, membershipSerial1, issuerGUID);
        EXPECT_EQ(ER_OK, status) << "  RemoveMembershipFromConsumer failed.  Actual Status: " << QCC_StatusText(status);

        /* removing it again */
        status = PermissionMgmtTestHelper::RemoveMembership(adminBus, clientProxyObject, membershipSerial1, issuerGUID);
        EXPECT_NE(ER_OK, status) << "  RemoveMembershipFromConsumer succeeded.  Expect it to fail.  Actual Status: " << QCC_StatusText(status);

    }

    /**
     * Test PermissionMgmt Reset method on service.  The consumer should not be
     * able to reset the service since the consumer is not an admin.
     */
    void FailResetServiceByConsumer()
    {
        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        status = PermissionMgmtTestHelper::Reset(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  Reset is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

    }

    /*
     * Test PermissionMgmt Reset method on service by the admin.  The admin should be
     * able to reset the service.
     */
    void SuccessfulResetServiceByAdmin()
    {
        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);

        status = PermissionMgmtTestHelper::Reset(adminBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        /* retrieve the current identity cert */
        IdentityCertificate cert;
        status = PermissionMgmtTestHelper::GetIdentity(adminBus, clientProxyObject, cert);
        EXPECT_NE(ER_OK, status) << "  GetIdentity is not supposed to succeed since it was removed by Reset.  Actual Status: " << QCC_StatusText(status);

    }

    /*
     * retrieve the peer public key.
     */
    void RetrieveServicePublicKey()
    {
        PermissionConfigurator& pc = consumerBus.GetPermissionConfigurator();
        GUID128 serviceGUID(0);
        String peerName = serviceBus.GetUniqueName();
        status = PermissionMgmtTestHelper::GetPeerGUID(consumerBus, peerName, serviceGUID);
        EXPECT_EQ(ER_OK, status) << "  ca.GetPeerGuid failed.  Actual Status: " << QCC_StatusText(status);
        ECCPublicKey publicKey;
        status = pc.GetConnectedPeerPublicKey(serviceGUID, &publicKey);
        EXPECT_EQ(ER_OK, status) << "  GetConnectedPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
    }
};

/*
 *  Test all the possible calls provided PermissionMgmt interface
 */
TEST_F(PermissionMgmtUseCaseTest, TestAllCalls)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL");
    ClaimAdmin();
    ClaimService();
    ClaimConsumer();

    EnableSecurity("ALLJOYN_ECDHE_ECDSA");
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    ReplaceServiceIdentityCert();
    policy = GenerateMembeshipAuthData();
    InstallMembershipToServiceProvider(policy);
    InstallMembershipToConsumer(policy);
    delete policy;
    RemoveMembershipFromServiceProvider();
    InstallGuildEquivalence();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff();
    GuildMemberCanTVUpAndDownAndNotChannel();
    SetPermissionManifestOnServiceProvider();

    RetrieveServicePublicKey();
    RemovePolicyFromServiceProvider();
    RemoveMembershipFromConsumer();
    FailResetServiceByConsumer();
    SuccessfulResetServiceByAdmin();
}

/*
 *  Simple case: claiming, install policy, install membership, and access
 */
TEST_F(PermissionMgmtUseCaseTest, ClaimPolicyMembershipAccess)
{
    EnableSecurity("ALLJOYN_ECDHE_PSK");
    ClaimAdmin();
    ClaimService();
    ClaimConsumer();

    EnableSecurity("ALLJOYN_ECDHE_ECDSA");
    qcc::GUID128 issuerGUID;
    PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
    ECCPrivateKey issuerPrivateKey;
    ECCPublicKey issuerPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAKeys(adminBus, issuerPrivateKey, issuerPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAKeys failed.  Actual Status: " << QCC_StatusText(status);

    /* generate a policy */
    ECCPublicKey guildAuthorityPubKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicy(issuerGUID, issuerPubKey, guildAuthorityPubKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateMembeshipAuthData();
    InstallMembershipToConsumer(policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff();
    GuildMemberCanTVUpAndDownAndNotChannel();
    SetPermissionManifestOnServiceProvider();

}

