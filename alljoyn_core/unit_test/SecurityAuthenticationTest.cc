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
#include <gtest/gtest.h>
#include <alljoyn/AuthListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/SecurityApplicationProxy.h>
#include <iostream>
#include <map>
#include <set>

#include "InMemoryKeyStore.h"
#include "PermissionMgmtObj.h"
#include "PermissionMgmtTest.h"

using namespace ajn;
using namespace qcc;
using namespace std;
/*
 * The unit test use many busy wait loops.  The busy wait loops were chosen
 * over thread sleeps because of the ease of understanding the busy wait loops.
 * Also busy wait loops do not require any platform specific threading code.
 */
#define WAIT_MSECS 5

/**
 * This is a collection of misc. test cases that did not fit into another
 * catagory but are still related to security2.0 feature.
 */

class SecurityAuthenticationTestSessionPortListener : public SessionPortListener {
  public:
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
};

class SecurityAuthTestHelper {
  public:
    static QStatus UpdatePolicyWithValuesFromDefaultPolicy(const PermissionPolicy& defaultPolicy,
                                                           PermissionPolicy& policy,
                                                           bool keepCAentry = true,
                                                           bool keepAdminGroupEntry = false,
                                                           bool keepInstallMembershipEntry = false) {

        size_t count = policy.GetAclsSize();
        if (keepCAentry) {
            ++count;
        }
        if (keepAdminGroupEntry) {
            ++count;
        }
        if (keepInstallMembershipEntry) {
            ++count;
        }

        PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[count];
        size_t idx = 0;
        for (size_t cnt = 0; cnt < defaultPolicy.GetAclsSize(); ++cnt) {
            if (defaultPolicy.GetAcls()[cnt].GetPeersSize() > 0) {
                if (defaultPolicy.GetAcls()[cnt].GetPeers()[0].GetType() == PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY) {
                    if (keepCAentry) {
                        acls[idx++] = defaultPolicy.GetAcls()[cnt];
                    }
                } else if (defaultPolicy.GetAcls()[cnt].GetPeers()[0].GetType() == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) {
                    if (keepAdminGroupEntry) {
                        acls[idx++] = defaultPolicy.GetAcls()[cnt];
                    }
                } else if (defaultPolicy.GetAcls()[cnt].GetPeers()[0].GetType() == PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY) {
                    if (keepInstallMembershipEntry) {
                        acls[idx++] = defaultPolicy.GetAcls()[cnt];
                    }
                }
            }

        }
        for (size_t cnt = 0; cnt < policy.GetAclsSize(); ++cnt) {
            assert(idx <= count);
            acls[idx++] = policy.GetAcls()[cnt];
        }
        policy.SetAcls(count, acls);
        delete [] acls;
        return ER_OK;
    }

    /*
     * Creates a PermissionPolicy that allows everything.
     * @policy[out] the policy to set
     * @version[in] the version number for the policy
     */
    static void GeneratePermissivePolicyAll(PermissionPolicy& policy, uint32_t version) {
        policy.SetVersion(version);
        {
            PermissionPolicy::Acl acls[1];
            {
                PermissionPolicy::Peer peers[1];
                peers[0].SetType(PermissionPolicy::Peer::PEER_ALL);
                acls[0].SetPeers(1, peers);
            }
            {
                PermissionPolicy::Rule rules[1];
                rules[0].SetObjPath("*");
                rules[0].SetInterfaceName("*");
                {
                    PermissionPolicy::Rule::Member members[1];
                    members[0].Set("*",
                                   PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                                   PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                                   PermissionPolicy::Rule::Member::ACTION_MODIFY |
                                   PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                    rules[0].SetMembers(1, members);
                }
                acls[0].SetRules(1, rules);
            }
            policy.SetAcls(1, acls);
        }
    }
    /*
     * Creates a PermissionPolicy that allows everything.
     * @policy[out] the policy to set
     * @version[in] the version number for the policy
     */
    static void GeneratePermissivePolicyAnyTrusted(PermissionPolicy& policy, uint32_t version) {
        policy.SetVersion(version);
        {
            PermissionPolicy::Acl acls[1];
            {
                PermissionPolicy::Peer peers[1];
                peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
                acls[0].SetPeers(1, peers);
            }
            {
                PermissionPolicy::Rule rules[1];
                rules[0].SetObjPath("*");
                rules[0].SetInterfaceName("*");
                {
                    PermissionPolicy::Rule::Member members[1];
                    members[0].Set("*",
                                   PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                                   PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                                   PermissionPolicy::Rule::Member::ACTION_MODIFY |
                                   PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                    rules[0].SetMembers(1, members);
                }
                acls[0].SetRules(1, rules);
            }
            policy.SetAcls(1, acls);
        }
    }
};

static const char ecdsaPrivateKeyPEM[] = {
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MDECAQEEIICSqj3zTadctmGnwyC/SXLioO39pB1MlCbNEX04hjeioAoGCCqGSM49\n"
    "AwEH\n"
    "-----END EC PRIVATE KEY-----"
};

static const char ecdsaCertChainX509PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBWjCCAQGgAwIBAgIHMTAxMDEwMTAKBggqhkjOPQQDAjArMSkwJwYDVQQDDCAw\n"
    "ZTE5YWZhNzlhMjliMjMwNDcyMGJkNGY2ZDVlMWIxOTAeFw0xNTAyMjYyMTU1MjVa\n"
    "Fw0xNjAyMjYyMTU1MjVaMCsxKTAnBgNVBAMMIDZhYWM5MjQwNDNjYjc5NmQ2ZGIy\n"
    "NmRlYmRkMGM5OWJkMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEP/HbYga30Afm\n"
    "0fB6g7KaB5Vr5CDyEkgmlif/PTsgwM2KKCMiAfcfto0+L1N0kvyAUgff6sLtTHU3\n"
    "IdHzyBmKP6MQMA4wDAYDVR0TBAUwAwEB/zAKBggqhkjOPQQDAgNHADBEAiAZmNVA\n"
    "m/H5EtJl/O9x0P4zt/UdrqiPg+gA+wm0yRY6KgIgetWANAE2otcrsj3ARZTY/aTI\n"
    "0GOQizWlQm8mpKaQ3uE=\n"
    "-----END CERTIFICATE-----"
};

class SecurityAuthenticationAuthListener : public AuthListener {
  public:
    SecurityAuthenticationAuthListener() :
        requestCredentialsCalled(false),
        verifyCredentialsCalled(false),
        authenticationSuccessfull(false),
        securityViolationCalled(false)
    {
    }

    void ClearFlags() {
        requestCredentialsCalled = false;
        verifyCredentialsCalled = false;
        authenticationSuccessfull = false;
        securityViolationCalled = false;
    }
    QStatus RequestCredentialsAsync(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, void* context)
    {
        QCC_UNUSED(authPeer);
        QCC_UNUSED(authCount);
        QCC_UNUSED(userId);
        QCC_UNUSED(credMask);
        requestCredentialsCalled = true;
        Credentials creds;
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_NULL") == 0) {
            return RequestCredentialsResponse(context, true, creds);
        }
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_PSK") == 0) {
            creds.SetPassword("faaa0af3dd3f1e0379da046a3ab6ca44");
            return RequestCredentialsResponse(context, true, creds);
        }
        if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("123456");
            }
            return RequestCredentialsResponse(context, true, creds);
        }
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
            if ((credMask& AuthListener::CRED_PRIVATE_KEY) == AuthListener::CRED_PRIVATE_KEY) {
                String pk(ecdsaPrivateKeyPEM, strlen(ecdsaPrivateKeyPEM));
                creds.SetPrivateKey(pk);
            }
            if ((credMask& AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN) {
                String cert(ecdsaCertChainX509PEM, strlen(ecdsaCertChainX509PEM));
                creds.SetCertChain(cert);
            }
            return RequestCredentialsResponse(context, true, creds);
        }
        return RequestCredentialsResponse(context, false, creds);
    }
    QStatus VerifyCredentialsAsync(const char* authMechanism, const char* authPeer, const Credentials& creds, void* context) {
        QCC_UNUSED(authMechanism);
        QCC_UNUSED(authPeer);
        QCC_UNUSED(creds);
        verifyCredentialsCalled = true;
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
            if (creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
                return VerifyCredentialsResponse(context, false);
            }
        }
        return VerifyCredentialsResponse(context, false);
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        QCC_UNUSED(authMechanism);
        QCC_UNUSED(authPeer);
        QCC_UNUSED(success);
        if (success) {
            authenticationSuccessfull = true;
        }
    }

    void SecurityViolation(QStatus status, const Message& msg) {
        QCC_UNUSED(status);
        QCC_UNUSED(msg);
        securityViolationCalled = true;
    }
    bool requestCredentialsCalled;
    bool verifyCredentialsCalled;
    bool authenticationSuccessfull;
    bool securityViolationCalled;
};

static void GetAppPublicKey(BusAttachment& bus, ECCPublicKey& publicKey)
{
    KeyInfoNISTP256 keyInfo;
    bus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
    publicKey = *keyInfo.GetPublicKey();
}

class SecurityAuthenticationTest : public testing::Test {
  public:
    SecurityAuthenticationTest() :
        managerBus("SecurityAuthenticationManager", true),
        peer1Bus("SecuritAuthenticationPeer1", true),
        peer2Bus("SecurityAuthenticationPeer2", true),
        managerKeyStoreListener(),
        peer1KeyStoreListener(),
        peer2KeyStoreListener(),
        managerAuthListener(),
        peer1AuthListener(),
        peer2AuthListener(),
        managerSessionPortListener(),
        peer1SessionPortListener(),
        peer2SessionPortListener(),
        managerToPeer1SessionId(0),
        managerToPeer2SessionId(0),
        peer1SessionPort(42),
        peer2SessionPort(42)
    {
    }

    virtual void SetUp() {
        EXPECT_EQ(ER_OK, managerBus.Start());
        EXPECT_EQ(ER_OK, managerBus.Connect());
        EXPECT_EQ(ER_OK, peer1Bus.Start());
        EXPECT_EQ(ER_OK, peer1Bus.Connect());
        EXPECT_EQ(ER_OK, peer2Bus.Start());
        EXPECT_EQ(ER_OK, peer2Bus.Connect());

        // Register in memory keystore listeners
        EXPECT_EQ(ER_OK, managerBus.RegisterKeyStoreListener(managerKeyStoreListener));
        EXPECT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
        EXPECT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));

        EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

        SessionOpts opts1;
        SessionId managerToManagerSessionId;
        SessionPort managerSessionPort = 42;
        EXPECT_EQ(ER_OK, managerBus.BindSessionPort(managerSessionPort, opts1, managerSessionPortListener));

        SessionOpts opts2;
        EXPECT_EQ(ER_OK, peer1Bus.BindSessionPort(peer1SessionPort, opts2, peer1SessionPortListener));

        SessionOpts opts3;
        EXPECT_EQ(ER_OK, peer2Bus.BindSessionPort(peer2SessionPort, opts3, peer2SessionPortListener));

        EXPECT_EQ(ER_OK, managerBus.JoinSession(managerBus.GetUniqueName().c_str(), managerSessionPort, NULL, managerToManagerSessionId, opts1));
        EXPECT_EQ(ER_OK, managerBus.JoinSession(peer1Bus.GetUniqueName().c_str(), peer1SessionPort, NULL, managerToPeer1SessionId, opts2));
        EXPECT_EQ(ER_OK, managerBus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, managerToPeer2SessionId, opts3));

        //-----------------------Claim each bus Attachments------------------
        SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);
        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);


        // All Inclusive manifest
        const size_t manifestSize = 1;
        PermissionPolicy::Rule manifest[manifestSize];
        manifest[0].SetObjPath("*");
        manifest[0].SetInterfaceName("*");
        {
            PermissionPolicy::Rule::Member member[1];
            member[0].Set("*",
                          PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                          PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                          PermissionPolicy::Rule::Member::ACTION_MODIFY |
                          PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            manifest[0].SetMembers(1, member);
        }

        //Get manager key
        KeyInfoNISTP256 managerKey;
        PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

        //Create peer1 key
        KeyInfoNISTP256 peer1Key;
        PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

        //Create peer2 key
        KeyInfoNISTP256 peer2Key;
        PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));

        //------------ Claim self(managerBus), Peer1, and Peer2 --------
        //Random GUID used for the SecurityManager
        GUID128 managerGuid;

        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(managerBus,
                                                                   manifest, manifestSize,
                                                                   digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        //Create identityCert
        const size_t certChainSize = 1;
        IdentityCertificate identityCertChainMaster[certChainSize];

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                      "0",
                                                                      managerGuid.ToString(),
                                                                      managerKey.GetPublicKey(),
                                                                      "ManagerAlias",
                                                                      3600,
                                                                      identityCertChainMaster[0],
                                                                      digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        /* set claimable */
        managerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        EXPECT_EQ(ER_OK, sapWithManager.Claim(managerKey,
                                              managerGuid,
                                              managerKey,
                                              identityCertChainMaster, certChainSize,
                                              manifest, manifestSize));


        ECCPublicKey managerPublicKey;
        GetAppPublicKey(managerBus, managerPublicKey);
        ASSERT_EQ(*managerKey.GetPublicKey(), managerPublicKey);

        //Create peer1 identityCert
        IdentityCertificate identityCertChainPeer1[certChainSize];


        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                      "0",
                                                                      managerGuid.ToString(),
                                                                      peer1Key.GetPublicKey(),
                                                                      "Peer1Alias",
                                                                      3600,
                                                                      identityCertChainPeer1[0],
                                                                      digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        /* set claimable */
        peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        //Manager claims Peers
        EXPECT_EQ(ER_OK, sapWithPeer1.Claim(managerKey,
                                            managerGuid,
                                            managerKey,
                                            identityCertChainPeer1, certChainSize,
                                            manifest, manifestSize));

        //Create peer2 identityCert
        IdentityCertificate identityCertChainPeer2[certChainSize];

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                      "0",
                                                                      managerGuid.ToString(),
                                                                      peer2Key.GetPublicKey(),
                                                                      "Peer2Alias",
                                                                      3600,
                                                                      identityCertChainPeer2[0],
                                                                      digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";
        /* set claimable */
        peer2Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        EXPECT_EQ(ER_OK, sapWithPeer2.Claim(managerKey,
                                            managerGuid,
                                            managerKey,
                                            identityCertChainPeer2, certChainSize,
                                            manifest, manifestSize));

        EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

        //--------- InstallMembership certificates on self, peer1, and peer2

        String membershipSerial = "1";
        qcc::MembershipCertificate managerMembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                        managerBus,
                                                                        managerBus.GetUniqueName(),
                                                                        managerKey.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        managerMembershipCertificate[0]
                                                                        ));
        EXPECT_EQ(ER_OK, sapWithManager.InstallMembership(managerMembershipCertificate, 1));

        qcc::MembershipCertificate peer1MembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                        managerBus,
                                                                        peer1Bus.GetUniqueName(),
                                                                        peer1Key.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        peer1MembershipCertificate[0]
                                                                        ));

        EXPECT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1MembershipCertificate, 1));

        qcc::MembershipCertificate peer2MembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                        managerBus,
                                                                        peer2Bus.GetUniqueName(),
                                                                        peer2Key.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        peer2MembershipCertificate[0]
                                                                        ));
        EXPECT_EQ(ER_OK, sapWithPeer2.InstallMembership(peer2MembershipCertificate, 1));
    }

    virtual void TearDown() {
        EXPECT_EQ(ER_OK, managerBus.Stop());
        EXPECT_EQ(ER_OK, managerBus.Join());
        EXPECT_EQ(ER_OK, peer1Bus.Stop());
        EXPECT_EQ(ER_OK, peer1Bus.Join());
        EXPECT_EQ(ER_OK, peer2Bus.Stop());
        EXPECT_EQ(ER_OK, peer2Bus.Join());
    }
    BusAttachment managerBus;
    BusAttachment peer1Bus;
    BusAttachment peer2Bus;

    InMemoryKeyStoreListener managerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;

    SecurityAuthenticationAuthListener managerAuthListener;
    SecurityAuthenticationAuthListener peer1AuthListener;
    SecurityAuthenticationAuthListener peer2AuthListener;

    SecurityAuthenticationTestSessionPortListener managerSessionPortListener;
    SecurityAuthenticationTestSessionPortListener peer1SessionPortListener;
    SecurityAuthenticationTestSessionPortListener peer2SessionPortListener;

    SessionId managerToPeer1SessionId;
    SessionId managerToPeer2SessionId;

    SessionPort peer1SessionPort;
    SessionPort peer2SessionPort;
};

/*
 * Purpose:
 * Verify that when both sides have one policy ACL with peer type
 * ALL, ECDHE_ECDSA based session cannot be set up. But, all other sessions like
 * NULL, ECDHE_PSK and SRP based sessions can be set.
 *
 * Setup:
 * A and B are claimed.
 * Both their identity certificates are signed by the CA.
 *
 * Peer A has a local policy with ALL Peer Type
 * Peer B has a local policy with ALL Peer Type
 * Policy rules and manifest rules allow everything.
 *
 * Case 1: A and B set up a ECDHE_NULL based session.
 * Case 2: A and B set up a ECDHE_PSK based session.
 * Case 3: A and B set up a SRP based session.
 * Case 4: A and B set up a ECDHE_ECDSA based session.
 *
 * Verification:
 * Case 1: Secure sessions can be set up successfully.
 * Case 2: Secure sessions can be set up successfully.
 * Case 3: Secure sessions can be set up successfully.
 * Case 4: Secure session cannot be set up because the policy does not have any
 *         authorities who can verify the IC of the remote peer.
 */
TEST_F(SecurityAuthenticationTest, authenticate_test1_case1_ECDHE_NULL) {
    //---------------- Install Policy --------------
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));
    {
        peer1AuthListener.ClearFlags();
        peer2AuthListener.ClearFlags();
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

        EXPECT_TRUE(peer1AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer1AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer1AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer1AuthListener.securityViolationCalled);

        EXPECT_TRUE(peer2AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer2AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer2AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer2AuthListener.securityViolationCalled);
    }
}

TEST_F(SecurityAuthenticationTest, authenticate_test1_case2_ECDHE_PSK) {
    //---------------- Install Policy --------------
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));
    {
        peer1AuthListener.ClearFlags();
        peer2AuthListener.ClearFlags();
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_PSK", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_PSK", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

        EXPECT_TRUE(peer1AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer1AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer1AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer1AuthListener.securityViolationCalled);

        EXPECT_TRUE(peer2AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer2AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer2AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer2AuthListener.securityViolationCalled);
    }
}

TEST_F(SecurityAuthenticationTest, authenticate_test1_case3_SRP) {
    //---------------- Install Policy --------------
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));
    {
        peer1AuthListener.ClearFlags();
        peer2AuthListener.ClearFlags();
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

        EXPECT_TRUE(peer1AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer1AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer1AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer1AuthListener.securityViolationCalled);

        EXPECT_TRUE(peer2AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer2AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer2AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer2AuthListener.securityViolationCalled);
    }
}

TEST_F(SecurityAuthenticationTest, authenticate_test1_case4_ECDHE_ECDSA) {
    //---------------- Install Policy --------------
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));
    {
        peer1AuthListener.ClearFlags();
        peer2AuthListener.ClearFlags();
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_AUTH_FAIL, proxy.SecureConnection(true));

        EXPECT_FALSE(peer1AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer1AuthListener.verifyCredentialsCalled);
        EXPECT_FALSE(peer1AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer1AuthListener.securityViolationCalled);

        EXPECT_FALSE(peer2AuthListener.requestCredentialsCalled);
        EXPECT_TRUE(peer2AuthListener.verifyCredentialsCalled);
        EXPECT_FALSE(peer2AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer2AuthListener.securityViolationCalled);
    }
}

/*
 * Purpose:
 * Verify that when both sides have one policy ACL with peer type ANY_TRUSTED,
 * ECDHE_ECDSA based session cannot be set up. But, all other sessions like
 * NULL, ECDHE_PSK and SRP based sessions can be set.
 *
 * Setup:
 * A and B are claimed.
 * Both their identity certificates are signed by the CA.
 *
 * Peer A has a local policy with ANY_TRUSTED Peer Type
 * Peer B has a local policy with ANY_TRUSTED Peer Type
 * Policy rules and manifest rules allow everything.
 *
 * Case 1: A and B set up a ECDHE_NULL based session.
 * Case 2: A and B set up a ECDHE_PSK based session.
 * Case 3: A and B set up a SRP based session.
 * Case 4: A and B set up a ECDHE_ECDSA based session.
 *
 * Verification:
 * Case 1: Secure sessions can be set up successfully.
 * Case 2: Secure sessions can be set up successfully.
 * Case 3: Secure sessions can be set up successfully.
 * Case 4: Secure session cannot be set up because the policy does not have any
 *         authorities who can verify the IC of the remote peer.
 */
TEST_F(SecurityAuthenticationTest, authenticate_test2_case1_ECDHE_NULL) {
    //----------------Install Policy --------------
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));
    {
        peer1AuthListener.ClearFlags();
        peer2AuthListener.ClearFlags();
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

        EXPECT_TRUE(peer1AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer1AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer1AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer1AuthListener.securityViolationCalled);

        EXPECT_TRUE(peer2AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer2AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer2AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer2AuthListener.securityViolationCalled);
    }
}

TEST_F(SecurityAuthenticationTest, authenticate_test2_case2_ECDHE_PSK) {
    //----------------Install Policy --------------
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));
    {
        peer1AuthListener.ClearFlags();
        peer2AuthListener.ClearFlags();
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_PSK", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_PSK", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

        EXPECT_TRUE(peer1AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer1AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer1AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer1AuthListener.securityViolationCalled);

        EXPECT_TRUE(peer2AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer2AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer2AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer2AuthListener.securityViolationCalled);
    }
}

TEST_F(SecurityAuthenticationTest, authenticate_test2_case3_SRP) {
    //----------------Install Policy --------------
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));
    {
        peer1AuthListener.ClearFlags();
        peer2AuthListener.ClearFlags();
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

        EXPECT_TRUE(peer1AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer1AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer1AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer1AuthListener.securityViolationCalled);

        EXPECT_TRUE(peer2AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer2AuthListener.verifyCredentialsCalled);
        EXPECT_TRUE(peer2AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer2AuthListener.securityViolationCalled);
    }
}

TEST_F(SecurityAuthenticationTest, authenticate_test2_case4_ECDHE_ECDSA) {
    //----------------Install Policy --------------
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
        // Don't instantly call SecureConnection we want to control when SecureConnection is called.
    }

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));
    {
        peer1AuthListener.ClearFlags();
        peer2AuthListener.ClearFlags();
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_AUTH_FAIL, proxy.SecureConnection(true));

        EXPECT_FALSE(peer1AuthListener.requestCredentialsCalled);
        EXPECT_FALSE(peer1AuthListener.verifyCredentialsCalled);
        EXPECT_FALSE(peer1AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer1AuthListener.securityViolationCalled);

        EXPECT_FALSE(peer2AuthListener.requestCredentialsCalled);
        EXPECT_TRUE(peer2AuthListener.verifyCredentialsCalled);
        EXPECT_FALSE(peer2AuthListener.authenticationSuccessfull);
        EXPECT_FALSE(peer2AuthListener.securityViolationCalled);
    }
}

class SecurityAuthentication2AuthListener : public DefaultECDHEAuthListener {
  public:
    SecurityAuthentication2AuthListener() {
        uint8_t psk[16] = { 0xfa, 0xaa, 0x0a, 0xf3,
                            0xdd, 0x3f, 0x1e, 0x03,
                            0x79, 0xda, 0x04, 0x6a,
                            0x3a, 0xb6, 0xca, 0x44 };
        SetPSK(psk, 16);
    }

    QStatus RequestCredentialsAsync(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, void* context)
    {
        Credentials creds;
        if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("123456");
            }
            return RequestCredentialsResponse(context, true, creds);
        }
        return DefaultECDHEAuthListener::RequestCredentialsAsync(authMechanism, authPeer, authCount, userId, credMask, context);
    }
};

class SecurityAuthenticationTest2 : public testing::Test {
  public:
    SecurityAuthenticationTest2() :
        managerBus("SecurityAuthenticationManager", true),
        peer1Bus("SecuritAuthenticationPeer1", true),
        peer2Bus("SecurityAuthenticationPeer2", true),

        busUsedAsCA("foo1"),
        busUsedAsCA1("foo2"),
        busUsedAsLivingRoom("foo3"),
        busUsedAsPeerC("foo4"),

        managerKeyStoreListener(),
        peer1KeyStoreListener(),
        peer2KeyStoreListener(),

        listener1(),
        listener2(),
        listener3(),
        listener4(),

        managerAuthListener(),
        peer1AuthListener(),
        peer2AuthListener(),

        managerSessionPortListener(),
        peer1SessionPortListener(),
        peer2SessionPortListener(),

        managerToPeer1SessionId(0),
        managerToPeer2SessionId(0),

        peer1SessionPort(42),
        peer2SessionPort(42),

        managerGuid(),
        caGuid(),
        livingRoomSGID(),

        manifestSize(1)
    {
    }

    virtual void SetUp() {
        EXPECT_EQ(ER_OK, managerBus.Start());
        EXPECT_EQ(ER_OK, managerBus.Connect());
        EXPECT_EQ(ER_OK, peer1Bus.Start());
        EXPECT_EQ(ER_OK, peer1Bus.Connect());
        EXPECT_EQ(ER_OK, peer2Bus.Start());
        EXPECT_EQ(ER_OK, peer2Bus.Connect());

        // Register in memory keystore listeners
        EXPECT_EQ(ER_OK, managerBus.RegisterKeyStoreListener(managerKeyStoreListener));
        EXPECT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
        EXPECT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));

        EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

        busUsedAsCA.Start();
        busUsedAsCA.Connect();
        busUsedAsCA1.Start();
        busUsedAsLivingRoom.Start();
        busUsedAsPeerC.Start();

        EXPECT_EQ(ER_OK, busUsedAsCA.RegisterKeyStoreListener(listener1));
        EXPECT_EQ(ER_OK, busUsedAsCA1.RegisterKeyStoreListener(listener2));
        EXPECT_EQ(ER_OK, busUsedAsLivingRoom.RegisterKeyStoreListener(listener3));
        EXPECT_EQ(ER_OK, busUsedAsPeerC.RegisterKeyStoreListener(listener4));

        EXPECT_EQ(ER_OK, busUsedAsCA.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &managerAuthListener));
        EXPECT_EQ(ER_OK, busUsedAsCA1.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &managerAuthListener));
        EXPECT_EQ(ER_OK, busUsedAsLivingRoom.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &managerAuthListener));
        EXPECT_EQ(ER_OK, busUsedAsPeerC.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &managerAuthListener));

        SessionOpts opts1;
        SessionId managerToManagerSessionId;
        SessionPort managerSessionPort = 42;
        EXPECT_EQ(ER_OK, managerBus.BindSessionPort(managerSessionPort, opts1, managerSessionPortListener));

        SessionOpts opts2;
        EXPECT_EQ(ER_OK, peer1Bus.BindSessionPort(peer1SessionPort, opts2, peer1SessionPortListener));

        SessionOpts opts3;
        EXPECT_EQ(ER_OK, peer2Bus.BindSessionPort(peer2SessionPort, opts3, peer2SessionPortListener));

        EXPECT_EQ(ER_OK, managerBus.JoinSession(managerBus.GetUniqueName().c_str(), managerSessionPort, NULL, managerToManagerSessionId, opts1));
        EXPECT_EQ(ER_OK, managerBus.JoinSession(peer1Bus.GetUniqueName().c_str(), peer1SessionPort, NULL, managerToPeer1SessionId, opts2));
        EXPECT_EQ(ER_OK, managerBus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, managerToPeer2SessionId, opts3));

        //-----------------------Claim each bus Attachments------------------
        SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);


        // All Inclusive manifest
        manifest[0].SetObjPath("*");
        manifest[0].SetInterfaceName("*");
        {
            PermissionPolicy::Rule::Member member[1];
            member[0].Set("*",
                          PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                          PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                          PermissionPolicy::Rule::Member::ACTION_MODIFY |
                          PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            manifest[0].SetMembers(1, member);
        }

        //Get manager key
        KeyInfoNISTP256 managerKey;
        PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

        //Get peer1 key
        KeyInfoNISTP256 peer1Key;
        PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

        //Get peer2 key
        KeyInfoNISTP256 peer2Key;
        PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));

        //Get peer2 key
        KeyInfoNISTP256 caKey;
        PermissionConfigurator& pcCA = busUsedAsCA.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcCA.GetSigningPublicKey(caKey));

        //------------ Claim self(managerBus), Peer1, and Peer2 --------
        uint8_t managerDigest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(managerBus,
                                                                   manifest, manifestSize,
                                                                   managerDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(busUsedAsCA,
                                                                   manifest, manifestSize,
                                                                   caDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        //Create identityCert
        const size_t certChainSize = 1;
        IdentityCertificate identityCertChainMaster[certChainSize];

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                      "0",
                                                                      managerGuid.ToString(),
                                                                      managerKey.GetPublicKey(),
                                                                      "ManagerAlias",
                                                                      3600,
                                                                      identityCertChainMaster[0],
                                                                      managerDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        /* set claimable */
        managerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        EXPECT_EQ(ER_OK, sapWithManager.Claim(caKey,
                                              managerGuid,
                                              managerKey,
                                              identityCertChainMaster, certChainSize,
                                              manifest, manifestSize));


        ECCPublicKey managerPublicKey;
        GetAppPublicKey(managerBus, managerPublicKey);
        ASSERT_EQ(*managerKey.GetPublicKey(), managerPublicKey);

        //Create peer1 identityCert
        IdentityCertificate identityCertChainPeer1[1];

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                      "0",
                                                                      caGuid.ToString(),
                                                                      peer1Key.GetPublicKey(),
                                                                      "Peer1Alias",
                                                                      3600,
                                                                      identityCertChainPeer1[0],
                                                                      caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        //Manager claims Peers
        /* set claimable */
        peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        EXPECT_EQ(ER_OK, sapWithPeer1.Claim(caKey,
                                            managerGuid,
                                            caKey,
                                            identityCertChainPeer1, 1,
                                            manifest, manifestSize));

        //Create peer2 identityCert
        IdentityCertificate identityCertChainPeer2[certChainSize];

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                      "0",
                                                                      caGuid.ToString(),
                                                                      peer2Key.GetPublicKey(),
                                                                      "Peer2Alias",
                                                                      3600,
                                                                      identityCertChainPeer2[0],
                                                                      caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        peer2Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        EXPECT_EQ(ER_OK, sapWithPeer2.Claim(caKey,
                                            managerGuid,
                                            caKey,
                                            identityCertChainPeer2, certChainSize,
                                            manifest, manifestSize));

        EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

        //--------- InstallMembership certificates on self, peer1, and peer2

        String membershipSerial = "1";
        qcc::MembershipCertificate managerMembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                        busUsedAsCA,
                                                                        managerBus.GetUniqueName(),
                                                                        managerKey.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        managerMembershipCertificate[0]
                                                                        ));
        EXPECT_EQ(ER_OK, sapWithManager.InstallMembership(managerMembershipCertificate, 1));

        // Install Membership certificate
        qcc::MembershipCertificate peer1MembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("1",
                                                                        busUsedAsCA,
                                                                        peer1Bus.GetUniqueName(),
                                                                        peer1Key.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        peer1MembershipCertificate[0]
                                                                        ));

        EXPECT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1MembershipCertificate, 1));

        qcc::MembershipCertificate peer2MembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                        busUsedAsCA,
                                                                        peer2Bus.GetUniqueName(),
                                                                        peer2Key.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        peer2MembershipCertificate[0]
                                                                        ));
        EXPECT_EQ(ER_OK, sapWithPeer2.InstallMembership(peer2MembershipCertificate, 1));
    }

    virtual void TearDown() {
        EXPECT_EQ(ER_OK, managerBus.Stop());
        EXPECT_EQ(ER_OK, managerBus.Join());
        EXPECT_EQ(ER_OK, peer1Bus.Stop());
        EXPECT_EQ(ER_OK, peer1Bus.Join());
        EXPECT_EQ(ER_OK, peer2Bus.Stop());
        EXPECT_EQ(ER_OK, peer2Bus.Join());
    }

    PermissionPolicy CreatePeer1Policy(uint32_t version) {
        PermissionPolicy policy;
        policy.SetVersion(version);
        {
            PermissionPolicy::Acl acls[1];
            {
                PermissionPolicy::Peer peers[1];
                peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
                PermissionConfigurator& pcCA = busUsedAsCA.GetPermissionConfigurator();
                KeyInfoNISTP256 caPublicKey;
                EXPECT_EQ(ER_OK, pcCA.GetSigningPublicKey(caPublicKey));
                peers[0].SetKeyInfo(&caPublicKey);
                acls[0].SetPeers(1, peers);
            }
            {
                PermissionPolicy::Rule rules[1];
                rules[0].SetObjPath("*");
                rules[0].SetInterfaceName("*");
                {
                    PermissionPolicy::Rule::Member members[1];
                    members[0].Set("*",
                                   PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                                   PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                                   PermissionPolicy::Rule::Member::ACTION_MODIFY |
                                   PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                    rules[0].SetMembers(1, members);
                }
                acls[0].SetRules(1, rules);
            }
            policy.SetAcls(1, acls);
        }
        return policy;
    }

    // ACL1: Peer type: WITH_MEMBERSHIP, SGID: Living Room
    // ACL2: Peer type: FROM_CERTIFICATE_AUTHORITY: CA1
    // ACL3: Peer Type: WITH_PUBLIC_KEY: Peer C
    PermissionPolicy CreatePeer2PolicyA(uint32_t version) {
        PermissionPolicy policy;
        policy.SetVersion(version);
        {
            PermissionPolicy::Acl acls[3];

            // All of the acls are using the same permissive rules
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[0].SetMembers(1, members);
            }

            // ACL0:  Peer type: WITH_MEMBERSHIP, SGID: Living Room
            {
                PermissionPolicy::Peer peers[1];
                peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
                peers[0].SetSecurityGroupId(livingRoomSGID);
                ////Get manager key
                KeyInfoNISTP256 livingRoomKey;
                PermissionConfigurator& pcLivingRoom = busUsedAsLivingRoom.GetPermissionConfigurator();
                EXPECT_EQ(ER_OK, pcLivingRoom.GetSigningPublicKey(livingRoomKey));
                peers[0].SetKeyInfo(&livingRoomKey); //Is this the public key of Peer2???
                acls[0].SetPeers(1, peers);
                acls[0].SetRules(1, rules);
            }
            //ACL1: Peer type: FROM_CERTIFICATE_AUTHORITY: CA1
            {
                PermissionPolicy::Peer peers[1];
                peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
                KeyInfoNISTP256 ca1Key;
                PermissionConfigurator& pcCA1 = busUsedAsCA1.GetPermissionConfigurator();
                EXPECT_EQ(ER_OK, pcCA1.GetSigningPublicKey(ca1Key));
                peers[0].SetKeyInfo(&ca1Key);
                acls[1].SetPeers(1, peers);
                acls[1].SetRules(1, rules);
            }
            //ACL2: Peer Type: WITH_PUBLIC_KEY: Peer C
            {
                PermissionPolicy::Peer peers[1];
                peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
                //Get manager key
                KeyInfoNISTP256 peerCKey;
                PermissionConfigurator& pcPeerC = busUsedAsPeerC.GetPermissionConfigurator();
                EXPECT_EQ(ER_OK, pcPeerC.GetSigningPublicKey(peerCKey));
                peers[0].SetKeyInfo(&peerCKey); //peerKey of peerC
                acls[2].SetPeers(1, peers);
                acls[2].SetRules(1, rules);
            }
            policy.SetAcls(3, acls);
        }
        return policy;
    }

    // Peer[0]: FROM_CERTIFICATE_AUTHORITY, Public key of CA1
    // Peer[1]: WITH_MEMBERSHIP, SGID: Living Room
    // Peer[2]: WITH_PUBLIC_KEY: Peer C's public key
    PermissionPolicy CreatePeer2PolicyB(uint32_t version) {
        PermissionPolicy policy;
        policy.SetVersion(version);
        {
            PermissionPolicy::Acl acls[1];

            // All of the acls are using the same permissive rules
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[0].SetMembers(1, members);
            }

            {
                PermissionPolicy::Peer peers[3];
                // Peer[0]: FROM_CERTIFICATE_AUTHORITY, Public key of CA1
                peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
                KeyInfoNISTP256 ca1Key;
                PermissionConfigurator& pcCA1 = busUsedAsCA1.GetPermissionConfigurator();
                EXPECT_EQ(ER_OK, pcCA1.GetSigningPublicKey(ca1Key));
                peers[0].SetKeyInfo(&ca1Key);
                // Peer[1]: WITH_MEMBERSHIP, SGID: Living Room
                peers[1].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
                peers[1].SetSecurityGroupId(livingRoomSGID);
                ////Get manager key
                KeyInfoNISTP256 livingRoomKey;
                PermissionConfigurator& pcLivingRoom = busUsedAsLivingRoom.GetPermissionConfigurator();
                EXPECT_EQ(ER_OK, pcLivingRoom.GetSigningPublicKey(livingRoomKey));
                peers[1].SetKeyInfo(&livingRoomKey);
                // Peer[2]: WITH_PUBLIC_KEY: Peer C's public key
                peers[2].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
                //Get manager key
                KeyInfoNISTP256 peerCKey;
                PermissionConfigurator& pcPeerC = busUsedAsPeerC.GetPermissionConfigurator();
                EXPECT_EQ(ER_OK, pcPeerC.GetSigningPublicKey(peerCKey));
                peers[2].SetKeyInfo(&peerCKey); //peerKey of peerC

                acls[0].SetPeers(3, peers);
                acls[0].SetRules(1, rules);
            }
            policy.SetAcls(1, acls);
        }
        return policy;
    }

    /**
     * Base for test case 3.
     * Verify that, for authentication between two peers to succeed, identity trust
     * of the peer is matched against all the authorities as specified in the policy.
     *
     * Setup:
     * Peers A and app. bus are claimed
     * App. bus's IC is signed by the CA.
     *
     * Peer A has a policy with peer type: FROM_CERTIFICATE_AUTHORITY, Public key of CA.
     *
     * App. bus has the following ACLs.
     * ACL1:  Peer type: WITH_MEMBERSHIP, SGID: Living Room
     * ACL2: Peer type: FROM_CERTIFICATE_AUTHORITY: CA1
     * ACL3: Peer Type: WITH_PUBLIC_KEY: Peer C
     *
     * The ACLs does not have any rules.
     *
     *
     * peer1 == PeerA
     * peer2 == App. Bus
     */
    void BaseAuthenticationTest3(BusAttachment& identityIssuer, QStatus expectedStatus, const char* reference)
    {
        //Get peer1 key
        KeyInfoNISTP256 peer1Key;
        PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

        //---------------- Install Policy --------------
        {
            PermissionPolicy policy = CreatePeer1Policy(1);
            PermissionPolicy peer1DefaultPolicy;

            SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
            EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
            /* peer1 will have new CA but managerBus still retains admin group privilege so it can install identity
             * cert to peer1 later */
            SecurityAuthTestHelper::UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, policy, false, true);
            EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
            EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));
        }
        {
            PermissionPolicy policy = CreatePeer2PolicyA(1);
            PermissionPolicy peer2DefaultPolicy;

            SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
            EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
            /* From here on the managerBus has no access to peer2 anymore */
        }

        //Create peer1 identityCert

        // All Inclusive manifest
        const size_t manifestSize = 1;
        PermissionPolicy::Rule manifest[manifestSize];
        manifest[0].SetObjPath("*");
        manifest[0].SetInterfaceName("*");
        {
            PermissionPolicy::Rule::Member member[1];
            member[0].Set("*",
                          PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                          PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                          PermissionPolicy::Rule::Member::ACTION_MODIFY |
                          PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            manifest[0].SetMembers(1, member);
        }

        GUID128 peer1Guid;

        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(busUsedAsCA1,
                                                                   manifest, manifestSize,
                                                                   digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        IdentityCertificate identityCertChainPeer1[1];

        PermissionConfigurator& pcCA = peer1Bus.GetPermissionConfigurator();
        KeyInfoNISTP256 peer1PublicKey;
        EXPECT_EQ(ER_OK, pcCA.GetSigningPublicKey(peer1PublicKey));

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(identityIssuer,
                                                                      "1",
                                                                      peer1Guid.ToString(),
                                                                      peer1PublicKey.GetPublicKey(),
                                                                      "Peer1Alias",
                                                                      3600,
                                                                      identityCertChainPeer1[0],
                                                                      digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChainPeer1, 1, manifest, manifestSize))
            << "Failed to update Identity cert or manifest ";
        uint32_t sessionId;
        SessionOpts opts;
        EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

        {
            EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
            EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
            SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
            EXPECT_EQ(expectedStatus, proxy.SecureConnection(true)) << reference << " expects status " << QCC_StatusText(expectedStatus);
            SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
            EXPECT_EQ(expectedStatus, proxy2.SecureConnection(true)) << reference << " expects status " << QCC_StatusText(expectedStatus);
        }
    }

    /**
     * Base for test case 4.
     * Verify that, for authentication between two peers to succeed, identity
     * trust of the peer is matched against all the authorities as specified in
     * the policy. The policy has only one ACL but the ACL has several peers.
     *
     * Setup:
     * Peers A and app. bus are claimed
     * App. bus's IC is signed by the CA.
     *
     * Peer A has a policy with peer type: FROM_CERTIFICATE_AUTHORITY, Public key of CA.
     *
     * App. bus has the following ACLs.
     * ACL1:  Peer type: WITH_MEMBERSHIP, SGID: Living Room
     * ACL2: Peer type: FROM_CERTIFICATE_AUTHORITY: CA1
     * ACL3: Peer Type: WITH_PUBLIC_KEY: Peer C
     *
     * The ACLs does not have any rules.
     *
     *
     * peer1 == PeerA
     * peer2 == App. Bus
     */
    void BaseAuthenticationTest4(BusAttachment& identityIssuer)
    {
        //Get peer1 key
        KeyInfoNISTP256 peer1Key;
        PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

        //---------------- Install Policy --------------
        {
            PermissionPolicy policy = CreatePeer1Policy(1);
            PermissionPolicy peer1DefaultPolicy;

            SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
            EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
            /* peer1 will have new CA but managerBus still retains admin group privilege so it can install identity
             * cert to peer1 later */
            SecurityAuthTestHelper::UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, policy, false, true);
            EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
            EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));
        }
        {
            PermissionPolicy policy = CreatePeer2PolicyB(1);
            PermissionPolicy peer2DefaultPolicy;

            SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
            EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
            /* From here on the managerBus has no access to peer2 anymore */
        }

        //Create peer1 identityCert

        // All Inclusive manifest
        const size_t manifestSize = 1;
        PermissionPolicy::Rule manifest[manifestSize];
        manifest[0].SetObjPath("*");
        manifest[0].SetInterfaceName("*");
        {
            PermissionPolicy::Rule::Member member[1];
            member[0].Set("*",
                          PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                          PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                          PermissionPolicy::Rule::Member::ACTION_MODIFY |
                          PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            manifest[0].SetMembers(1, member);
        }

        GUID128 peer1Guid;

        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(busUsedAsCA1,
                                                                   manifest, manifestSize,
                                                                   digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        IdentityCertificate identityCertChainPeer1[1];

        PermissionConfigurator& pcCA = peer1Bus.GetPermissionConfigurator();
        KeyInfoNISTP256 peer1PublicKey;
        EXPECT_EQ(ER_OK, pcCA.GetSigningPublicKey(peer1PublicKey));

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(identityIssuer,
                                                                      "1",
                                                                      peer1Guid.ToString(),
                                                                      peer1PublicKey.GetPublicKey(),
                                                                      "Peer1Alias",
                                                                      3600,
                                                                      identityCertChainPeer1[0],
                                                                      digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChainPeer1, 1, manifest, manifestSize))
            << "Failed to update Identity cert or manifest ";
    }

    /**
     * Base for test case 6.
     * Verify that under the default policy, authentication will succeed if the
     * IC is signed by either the CA or the ASGA.
     *
     * Both Peers use default policy
     */
    void BaseAuthenticationTest6(BusAttachment& identityIssuer, QStatus expectedStatus, const char* reference)
    {
        //Get peer1 key
        KeyInfoNISTP256 peer1Key;
        PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

        //Create peer1 identityCert

        // All Inclusive manifest
        const size_t manifestSize = 1;
        PermissionPolicy::Rule manifest[manifestSize];
        manifest[0].SetObjPath("*");
        manifest[0].SetInterfaceName("*");
        {
            PermissionPolicy::Rule::Member member[1];
            member[0].Set("*",
                          PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                          PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                          PermissionPolicy::Rule::Member::ACTION_MODIFY |
                          PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            manifest[0].SetMembers(1, member);
        }

        GUID128 peer1Guid;

        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(busUsedAsCA1,
                                                                   manifest, manifestSize,
                                                                   digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        IdentityCertificate identityCertChainPeer1[1];

        PermissionConfigurator& pcCA = peer1Bus.GetPermissionConfigurator();
        KeyInfoNISTP256 peer1PublicKey;
        EXPECT_EQ(ER_OK, pcCA.GetSigningPublicKey(peer1PublicKey));

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(identityIssuer,
                                                                      "1",
                                                                      peer1Guid.ToString(),
                                                                      peer1PublicKey.GetPublicKey(),
                                                                      "Peer1Alias",
                                                                      3600,
                                                                      identityCertChainPeer1[0],
                                                                      digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChainPeer1, 1, manifest, manifestSize))
            << "Failed to update Identity cert or manifest ";
        uint32_t sessionId;
        SessionOpts opts;
        EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

        {
            EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
            EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
            SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
            EXPECT_EQ(expectedStatus, proxy.SecureConnection(true)) << reference << " expects status " << QCC_StatusText(expectedStatus);
            SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
            EXPECT_EQ(expectedStatus, proxy2.SecureConnection(true)) << reference << " expects status " << QCC_StatusText(expectedStatus);
        }
    }

    BusAttachment managerBus;
    BusAttachment peer1Bus;
    BusAttachment peer2Bus;

    BusAttachment busUsedAsCA;
    BusAttachment busUsedAsCA1;
    BusAttachment busUsedAsLivingRoom;
    BusAttachment busUsedAsPeerC;

    InMemoryKeyStoreListener managerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;

    InMemoryKeyStoreListener listener1;
    InMemoryKeyStoreListener listener2;
    InMemoryKeyStoreListener listener3;
    InMemoryKeyStoreListener listener4;

    SecurityAuthentication2AuthListener managerAuthListener;
    SecurityAuthentication2AuthListener peer1AuthListener;
    SecurityAuthentication2AuthListener peer2AuthListener;

    SecurityAuthenticationTestSessionPortListener managerSessionPortListener;
    SecurityAuthenticationTestSessionPortListener peer1SessionPortListener;
    SecurityAuthenticationTestSessionPortListener peer2SessionPortListener;

    SessionId managerToPeer1SessionId;
    SessionId managerToPeer2SessionId;

    SessionPort peer1SessionPort;
    SessionPort peer2SessionPort;

    GUID128 managerGuid;
    GUID128 caGuid;
    qcc::GUID128 livingRoomSGID;

    const size_t manifestSize;
    PermissionPolicy::Rule manifest[1];
    uint8_t caDigest[Crypto_SHA256::DIGEST_SIZE];
};

/*
 * Purpose:
 * Verify that, for authentication between two peers to succeed, identity trust
 * of the peer is matched against all the authorities as specified in the policy.
 *
 * Setup:
 * Peers A and app. bus are claimed
 * App. bus's IC is signed by the CA.
 *
 * Peer A has a policy with peer type: FROM_CERTIFICATE_AUTHORITY, Public key of CA.
 *
 * App. bus has the following ACLs.
 * ACL1:  Peer type: WITH_MEMBERSHIP, SGID: Living Room
 * ACL2: Peer type: FROM_CERTIFICATE_AUTHORITY: CA1
 * ACL3: Peer Type: WITH_PUBLIC_KEY: Peer C
 *
 * The ACLs does not have any rules.
 *
 * case A. Peer A has the IC signed by the CA and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case B.  Peer A has the IC signed by the ASGA and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case C. Peer A has the IC signed by the Living room authority  and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case D. Peer A has the IC signed by the CA1 and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case E.  Peer A has the IC signed by Peer C and both peers specify the ECDHE_ECDSA auth. mechanism.
 *
 * For each of the cases above, Peer A tries to establish a secure session with the app. bus.
 * For each of the cases above, the app. bus tries to establish a secure session with Peer A.
 *
 * Verification:
 * A. Verify that authentication between both peers is NOT successful.
 * B. Verify that authentication between both peers is  NOT successful.
 * C. Verify that authentication between both peers is successful.
 * D. Verify that authentication between both peers is successful.
 * E. Verify that authentication between both peers is not successful.
 *
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test3_caseA) {
    BaseAuthenticationTest3(busUsedAsCA, ER_AUTH_FAIL, "authenticate_test3_caseA");
}

/*
 * Purpose:
 * Verify that, for authentication between two peers to succeed, identity trust
 * of the peer is matched against all the authorities as specified in the policy.
 *
 * Setup:
 * Peers A and app. bus are claimed
 * App. bus's IC is signed by the CA.
 *
 * Peer A has a policy with peer type: FROM_CERTIFICATE_AUTHORITY, Public key of CA.
 *
 * App. bus has the following ACLs.
 * ACL1:  Peer type: WITH_MEMBERSHIP, SGID: Living Room
 * ACL2: Peer type: FROM_CERTIFICATE_AUTHORITY: CA1
 * ACL3: Peer Type: WITH_PUBLIC_KEY: Peer C
 *
 * The ACLs does not have any rules.
 *
 * case A. Peer A has the IC signed by the CA and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case B.  Peer A has the IC signed by the ASGA and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case C. Peer A has the IC signed by the Living room authority  and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case D. Peer A has the IC signed by the CA1 and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case E.  Peer A has the IC signed by Peer C and both peers specify the ECDHE_ECDSA auth. mechanism.
 *
 * For each of the cases above, Peer A tries to establish a secure session with the app. bus.
 * For each of the cases above, the app. bus tries to establish a secure session with Peer A.
 *
 * Verification:
 * A. Verify that authentication between both peers is NOT successful.
 * B. Verify that authentication between both peers is  NOT successful.
 * C. Verify that authentication between both peers is successful.
 * D. Verify that authentication between both peers is successful.
 * E. Verify that authentication between both peers is not successful.
 *
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test3_caseB) {
    BaseAuthenticationTest3(managerBus, ER_AUTH_FAIL, "authenticate_test3_caseB");
}

/*
 * Purpose:
 * Verify that, for authentication between two peers to succeed, identity trust
 * of the peer is matched against all the authorities as specified in the policy.
 *
 * Setup:
 * Peers A and app. bus are claimed
 * App. bus's IC is signed by the CA.
 *
 * Peer A has a policy with peer type: FROM_CERTIFICATE_AUTHORITY, Public key of CA.
 *
 * App. bus has the following ACLs.
 * ACL1:  Peer type: WITH_MEMBERSHIP, SGID: Living Room
 * ACL2: Peer type: FROM_CERTIFICATE_AUTHORITY: CA1
 * ACL3: Peer Type: WITH_PUBLIC_KEY: Peer C
 *
 * The ACLs does not have any rules.
 *
 * case A. Peer A has the IC signed by the CA and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case B.  Peer A has the IC signed by the ASGA and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case C. Peer A has the IC signed by the Living room authority  and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case D. Peer A has the IC signed by the CA1 and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case E.  Peer A has the IC signed by Peer C and both peers specify the ECDHE_ECDSA auth. mechanism.
 *
 * For each of the cases above, Peer A tries to establish a secure session with the app. bus.
 * For each of the cases above, the app. bus tries to establish a secure session with Peer A.
 *
 * Verification:
 * A. Verify that authentication between both peers is NOT successful.
 * B. Verify that authentication between both peers is  NOT successful.
 * C. Verify that authentication between both peers is successful.
 * D. Verify that authentication between both peers is successful.
 * E. Verify that authentication between both peers is not successful.
 *
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test3_caseC) {
    BaseAuthenticationTest3(busUsedAsLivingRoom, ER_OK, "authenticate_test3_caseC");
}

/*
 * Purpose:
 * Verify that, for authentication between two peers to succeed, identity trust
 * of the peer is matched against all the authorities as specified in the policy.
 *
 * Setup:
 * Peers A and app. bus are claimed
 * App. bus's IC is signed by the CA.
 *
 * Peer A has a policy with peer type: FROM_CERTIFICATE_AUTHORITY, Public key of CA.
 *
 * App. bus has the following ACLs.
 * ACL1:  Peer type: WITH_MEMBERSHIP, SGID: Living Room
 * ACL2: Peer type: FROM_CERTIFICATE_AUTHORITY: CA1
 * ACL3: Peer Type: WITH_PUBLIC_KEY: Peer C
 *
 * The ACLs does not have any rules.
 *
 * case A. Peer A has the IC signed by the CA and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case B.  Peer A has the IC signed by the ASGA and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case C. Peer A has the IC signed by the Living room authority  and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case D. Peer A has the IC signed by the CA1 and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case E.  Peer A has the IC signed by Peer C and both peers specify the ECDHE_ECDSA auth. mechanism.
 *
 * For each of the cases above, Peer A tries to establish a secure session with the app. bus.
 * For each of the cases above, the app. bus tries to establish a secure session with Peer A.
 *
 * Verification:
 * A. Verify that authentication between both peers is NOT successful.
 * B. Verify that authentication between both peers is  NOT successful.
 * C. Verify that authentication between both peers is successful.
 * D. Verify that authentication between both peers is successful.
 * E. Verify that authentication between both peers is not successful.
 *
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test3_caseD) {
    BaseAuthenticationTest3(busUsedAsCA1, ER_OK, "authenticate_test3_caseD");
}

/*
 * Purpose:
 * Verify that, for authentication between two peers to succeed, identity trust
 * of the peer is matched against all the authorities as specified in the policy.
 *
 * Setup:
 * Peers A and app. bus are claimed
 * App. bus's IC is signed by the CA.
 *
 * Peer A has a policy with peer type: FROM_CERTIFICATE_AUTHORITY, Public key of CA.
 *
 * App. bus has the following ACLs.
 * ACL1:  Peer type: WITH_MEMBERSHIP, SGID: Living Room
 * ACL2: Peer type: FROM_CERTIFICATE_AUTHORITY: CA1
 * ACL3: Peer Type: WITH_PUBLIC_KEY: Peer C
 *
 * The ACLs does not have any rules.
 *
 * case A. Peer A has the IC signed by the CA and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case B.  Peer A has the IC signed by the ASGA and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case C. Peer A has the IC signed by the Living room authority  and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case D. Peer A has the IC signed by the CA1 and both peers specify the ECDHE_ECDSA auth. mechanism.
 * case E.  Peer A has the IC signed by Peer C and both peers specify the ECDHE_ECDSA auth. mechanism.
 *
 * For each of the cases above, Peer A tries to establish a secure session with the app. bus.
 * For each of the cases above, the app. bus tries to establish a secure session with Peer A.
 *
 * Verification:
 * A. Verify that authentication between both peers is NOT successful.
 * B. Verify that authentication between both peers is  NOT successful.
 * C. Verify that authentication between both peers is successful.
 * D. Verify that authentication between both peers is successful.
 * E. Verify that authentication between both peers is not successful.
 *
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test3_caseE) {
    BaseAuthenticationTest3(busUsedAsPeerC, ER_AUTH_FAIL, "authenticate_test3_caseE");
}

/*
 * Purpose:
 * Verify that, for authentication between two peers to succeed, identity trust
 * of the peer is matched against all the authorities as specified in the policy.
 * The policy has only one ACL but the ACL has several peers.
 *
 * Setup:
 * Peer A and app. bus are claimed.
 * app bus's IC: signed by CA
 *
 * Peer A has a local policy with FROM_CERTIFICATE_AUTHORITY peer type, public key of CA
 *
 * app bus has the following policy:
 * ACL 1:
 * Peer[0]: FROM_CERTIFICATE_AUTHORITY, Public key of CA1
 * Peer[1]: WITH_MEMBERSHIP, SGID: Living Room
 * Peer[2]: WITH_PUBLIC_KEY: Peer C's public key
 *
 * Case 1: A and app. bus set up a ECDHE_NULL based session.
 * Case 2: A and app. bus set up a ECDHE_PSK based session.
 * Case 3: A and app. bus set up a SRP based session.
 * Case 4: A and app. bus set up a ECDHE_ECDSA based session, IC of A is signed by CA1
 * Case 5: A and app. bus set up a ECDHE_ECDSA based session, IC of A is signed by Living room authority
 * Case 6: A and app. bus set up a ECDHE_ECDSA based session, IC of A is signed by Peer C
 * Case 7: A and app. bus set up a ECDHE_ECDSA based session, IC of A is signed by CA
 *
 * Verification:
 * Cases 1-5: Secure session can be set up successfully.
 * Cases 6-7: Secure session cannot be set up.
 *
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test4_case1) {
    BaseAuthenticationTest4(busUsedAsCA);
    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));
        SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy2.SecureConnection(true));
    }
}

/*
 * See authenticate_test4_case1 above for full test description
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test4_case2) {
    BaseAuthenticationTest4(busUsedAsCA);
    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_PSK", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_PSK", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));
        SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy2.SecureConnection(true));
    }
}

/*
 * See authenticate_test4_case1 above for full test description
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test4_case3) {
    BaseAuthenticationTest4(busUsedAsCA);
    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));
        SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy2.SecureConnection(true));
    }
}

/*
 * See authenticate_test4_case1 above for full test description
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test4_case4) {
    BaseAuthenticationTest4(busUsedAsCA1);
    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));
        SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy2.SecureConnection(true));
    }
}

/*
 * See authenticate_test4_case1 above for full test description
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test4_case5) {
    BaseAuthenticationTest4(busUsedAsLivingRoom);
    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));
        SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy2.SecureConnection(true));
    }
}

/*
 * See authenticate_test4_case1 above for full test description
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test4_case6) {
    BaseAuthenticationTest4(busUsedAsPeerC);
    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_AUTH_FAIL, proxy.SecureConnection(true));
        SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_AUTH_FAIL, proxy2.SecureConnection(true));
    }
}

/*
 * See authenticate_test4_case1 above for full test description
 */
TEST_F(SecurityAuthenticationTest2, authenticate_test4_case7) {
    BaseAuthenticationTest4(busUsedAsCA);
    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
        SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_AUTH_FAIL, proxy.SecureConnection(true));
        SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_AUTH_FAIL, proxy2.SecureConnection(true));
    }
}

class SecurityAuthentication3AuthListener1 : public DefaultECDHEAuthListener {
  public:
    SecurityAuthentication3AuthListener1() {
        uint8_t psk[16] = { 0xfa, 0xaa, 0x0a, 0xf3,
                            0xdd, 0x3f, 0x1e, 0x03,
                            0x79, 0xda, 0x04, 0x6a,
                            0x3a, 0xb6, 0xca, 0x44 };
        SetPSK(psk, 16);
        calledAuthMechanisms.clear();
    }

    QStatus RequestCredentialsAsync(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, void* context)
    {
        Credentials creds;
        if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("123456");
            }
            return RequestCredentialsResponse(context, true, creds);
        }
        return DefaultECDHEAuthListener::RequestCredentialsAsync(authMechanism, authPeer, authCount, userId, credMask, context);
    }
    void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success) {
        calledAuthMechanisms.insert(authMechanism);
        DefaultECDHEAuthListener::AuthenticationComplete(authMechanism, peerName, success);
    }

    std::set<qcc::String> calledAuthMechanisms;
};

class SecurityAuthentication3AuthListener2 : public DefaultECDHEAuthListener {
  public:
    SecurityAuthentication3AuthListener2() {
        uint8_t psk[16] = { 0xfa, 0xaa, 0x0a, 0xf3,
                            0xdd, 0x3f, 0x1e, 0x03,
                            0x79, 0xda, 0x04, 0x6a,
                            0x3a, 0xb6, 0xca, 0x55 };
        SetPSK(psk, 16);
        calledAuthMechanisms.clear();
    }

    QStatus RequestCredentialsAsync(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, void* context)
    {
        Credentials creds;
        if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("654321");
            }
            return RequestCredentialsResponse(context, true, creds);
        }
        return DefaultECDHEAuthListener::RequestCredentialsAsync(authMechanism, authPeer, authCount, userId, credMask, context);
    }

    void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success) {
        calledAuthMechanisms.insert(authMechanism);
        DefaultECDHEAuthListener::AuthenticationComplete(authMechanism, peerName, success);
    }

    std::set<qcc::String> calledAuthMechanisms;
};

/*
 * This test class strongly resembles the SecurityAuthenticationTest2 class
 * except the AuthListeners used in the set up are different. As well as some
 * additional cleanup to remove unused variables.
 */
class SecurityAuthenticationTest3 : public testing::Test {
  public:
    SecurityAuthenticationTest3() :
        managerBus("SecurityAuthenticationManager", true),
        peer1Bus("SecuritAuthenticationPeer1", true),
        peer2Bus("SecurityAuthenticationPeer2", true),

        busUsedAsCA("foo1"),
        busUsedAsLivingRoom("foo3"),

        managerKeyStoreListener(),
        peer1KeyStoreListener(),
        peer2KeyStoreListener(),

        listener1(),
        listener2(),

        managerAuthListener(),
        peer1AuthListener(),
        peer2AuthListener(),

        managerSessionPortListener(),
        peer1SessionPortListener(),
        peer2SessionPortListener(),

        managerToPeer1SessionId(0),
        managerToPeer2SessionId(0),

        peer1SessionPort(42),
        peer2SessionPort(42),

        managerGuid(),
        caGuid(),
        livingRoomSGID(),

        manifestSize(1)
    {
    }

    virtual void SetUp() {
        EXPECT_EQ(ER_OK, managerBus.Start());
        EXPECT_EQ(ER_OK, managerBus.Connect());
        EXPECT_EQ(ER_OK, peer1Bus.Start());
        EXPECT_EQ(ER_OK, peer1Bus.Connect());
        EXPECT_EQ(ER_OK, peer2Bus.Start());
        EXPECT_EQ(ER_OK, peer2Bus.Connect());

        // Register in memory keystore listeners
        EXPECT_EQ(ER_OK, managerBus.RegisterKeyStoreListener(managerKeyStoreListener));
        EXPECT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
        EXPECT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));

        EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

        busUsedAsCA.Start();
        busUsedAsCA.Connect();
        busUsedAsLivingRoom.Start();

        EXPECT_EQ(ER_OK, busUsedAsCA.RegisterKeyStoreListener(listener1));
        EXPECT_EQ(ER_OK, busUsedAsLivingRoom.RegisterKeyStoreListener(listener2));

        EXPECT_EQ(ER_OK, busUsedAsCA.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &managerAuthListener));
        EXPECT_EQ(ER_OK, busUsedAsLivingRoom.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &managerAuthListener));

        SessionOpts opts1;
        SessionId managerToManagerSessionId;
        SessionPort managerSessionPort = 42;
        EXPECT_EQ(ER_OK, managerBus.BindSessionPort(managerSessionPort, opts1, managerSessionPortListener));

        SessionOpts opts2;
        EXPECT_EQ(ER_OK, peer1Bus.BindSessionPort(peer1SessionPort, opts2, peer1SessionPortListener));

        SessionOpts opts3;
        EXPECT_EQ(ER_OK, peer2Bus.BindSessionPort(peer2SessionPort, opts3, peer2SessionPortListener));

        EXPECT_EQ(ER_OK, managerBus.JoinSession(managerBus.GetUniqueName().c_str(), managerSessionPort, NULL, managerToManagerSessionId, opts1));
        EXPECT_EQ(ER_OK, managerBus.JoinSession(peer1Bus.GetUniqueName().c_str(), peer1SessionPort, NULL, managerToPeer1SessionId, opts2));
        EXPECT_EQ(ER_OK, managerBus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, managerToPeer2SessionId, opts3));

        //-----------------------Claim each bus Attachments------------------
        SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);


        // All Inclusive manifest
        manifest[0].SetObjPath("*");
        manifest[0].SetInterfaceName("*");
        {
            PermissionPolicy::Rule::Member member[1];
            member[0].Set("*",
                          PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                          PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                          PermissionPolicy::Rule::Member::ACTION_MODIFY |
                          PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            manifest[0].SetMembers(1, member);
        }

        //Get manager key
        KeyInfoNISTP256 managerKey;
        PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

        //Get peer1 key
        KeyInfoNISTP256 peer1Key;
        PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

        //Get peer2 key
        KeyInfoNISTP256 peer2Key;
        PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));

        //Get peer2 key
        KeyInfoNISTP256 caKey;
        PermissionConfigurator& pcCA = busUsedAsCA.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcCA.GetSigningPublicKey(caKey));

        //------------ Claim self(managerBus), Peer1, and Peer2 --------
        uint8_t managerDigest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(managerBus,
                                                                   manifest, manifestSize,
                                                                   managerDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(busUsedAsCA,
                                                                   manifest, manifestSize,
                                                                   caDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        //Create identityCert
        const size_t certChainSize = 1;
        IdentityCertificate identityCertChainMaster[certChainSize];

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                      "0",
                                                                      managerGuid.ToString(),
                                                                      managerKey.GetPublicKey(),
                                                                      "ManagerAlias",
                                                                      3600,
                                                                      identityCertChainMaster[0],
                                                                      managerDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        /* set claimable */
        managerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        EXPECT_EQ(ER_OK, sapWithManager.Claim(caKey,
                                              managerGuid,
                                              managerKey,
                                              identityCertChainMaster, certChainSize,
                                              manifest, manifestSize));


        ECCPublicKey managerPublicKey;
        GetAppPublicKey(managerBus, managerPublicKey);
        ASSERT_EQ(*managerKey.GetPublicKey(), managerPublicKey);

        //Create peer1 identityCert
        IdentityCertificate identityCertChainPeer1[1];

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                      "0",
                                                                      caGuid.ToString(),
                                                                      peer1Key.GetPublicKey(),
                                                                      "Peer1Alias",
                                                                      3600,
                                                                      identityCertChainPeer1[0],
                                                                      caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        //Manager claims Peers
        /* set claimable */
        peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        EXPECT_EQ(ER_OK, sapWithPeer1.Claim(caKey,
                                            managerGuid,
                                            caKey,
                                            identityCertChainPeer1, 1,
                                            manifest, manifestSize));

        //Create peer2 identityCert
        IdentityCertificate identityCertChainPeer2[certChainSize];

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                      "0",
                                                                      caGuid.ToString(),
                                                                      peer2Key.GetPublicKey(),
                                                                      "Peer2Alias",
                                                                      3600,
                                                                      identityCertChainPeer2[0],
                                                                      caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        peer2Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        EXPECT_EQ(ER_OK, sapWithPeer2.Claim(caKey,
                                            managerGuid,
                                            caKey,
                                            identityCertChainPeer2, certChainSize,
                                            manifest, manifestSize));

        EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

        //--------- InstallMembership certificates on self, peer1, and peer2

        String membershipSerial = "1";
        qcc::MembershipCertificate managerMembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                        busUsedAsCA,
                                                                        managerBus.GetUniqueName(),
                                                                        managerKey.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        managerMembershipCertificate[0]
                                                                        ));
        EXPECT_EQ(ER_OK, sapWithManager.InstallMembership(managerMembershipCertificate, 1));

        // Install Membership certificate
        qcc::MembershipCertificate peer1MembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("1",
                                                                        busUsedAsCA,
                                                                        peer1Bus.GetUniqueName(),
                                                                        peer1Key.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        peer1MembershipCertificate[0]
                                                                        ));

        EXPECT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1MembershipCertificate, 1));

        qcc::MembershipCertificate peer2MembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                        busUsedAsCA,
                                                                        peer2Bus.GetUniqueName(),
                                                                        peer2Key.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        peer2MembershipCertificate[0]
                                                                        ));
        EXPECT_EQ(ER_OK, sapWithPeer2.InstallMembership(peer2MembershipCertificate, 1));
    }

    virtual void TearDown() {
        EXPECT_EQ(ER_OK, managerBus.Stop());
        EXPECT_EQ(ER_OK, managerBus.Join());
        EXPECT_EQ(ER_OK, peer1Bus.Stop());
        EXPECT_EQ(ER_OK, peer1Bus.Join());
        EXPECT_EQ(ER_OK, peer2Bus.Stop());
        EXPECT_EQ(ER_OK, peer2Bus.Join());
    }

    PermissionPolicy CreatePeer1Policy(uint32_t version) {
        PermissionPolicy policy;
        policy.SetVersion(version);
        {
            PermissionPolicy::Acl acls[1];
            {
                PermissionPolicy::Peer peers[1];
                peers[0].SetType(PermissionPolicy::Peer::PEER_ALL);
                acls[0].SetPeers(1, peers);
            }
            policy.SetAcls(1, acls);
        }
        return policy;
    }

    //ACL0:  Peer type: WITH_MEMBERSHIP, SGID: Living Room;
    //ACL1:  Peer type: ALL, peer type
    //ACL2:  ANY_TRUSTED
    //(i.e app. bus has three ACLs.)
    //
    //The ACLs does not have any rules.
    PermissionPolicy CreatePeer2Policy(uint32_t version) {
        PermissionPolicy policy;
        policy.SetVersion(version);
        {
            PermissionPolicy::Acl acls[3];
            // ACL0:  Peer type: WITH_MEMBERSHIP, SGID: Living Room
            {
                PermissionPolicy::Peer peers[1];
                peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
                peers[0].SetSecurityGroupId(livingRoomSGID);
                ////Get manager key
                KeyInfoNISTP256 livingRoomKey;
                PermissionConfigurator& pcLivingRoom = busUsedAsLivingRoom.GetPermissionConfigurator();
                EXPECT_EQ(ER_OK, pcLivingRoom.GetSigningPublicKey(livingRoomKey));
                peers[0].SetKeyInfo(&livingRoomKey); //Is this the public key of Peer2???
                acls[0].SetPeers(1, peers);
            }
            //ACL1:  Peer type: ALL, peer type
            {
                PermissionPolicy::Peer peers[1];
                peers[0].SetType(PermissionPolicy::Peer::PEER_ALL);
                acls[1].SetPeers(1, peers);
            }
            //ACL2:  ANY_TRUSTED
            {
                PermissionPolicy::Peer peers[1];
                peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
                acls[2].SetPeers(1, peers);
            }
            policy.SetAcls(3, acls);
        }
        return policy;
    }

    /**
     * base for test 5
     * Verify that peer should fallback to ECDHE_NULL based authentication in case of failure.
     *
     * Setup:
     * Peers A and app. bus are claimed.
     * App. bus's IC is signed by the CA.
     *
     * Peer A has a policy with ALL peer type.
     * App. bus has the following ACLs.
     * ACL1:  Peer type: WITH_MEMBERSHIP, SGID: Living Room;
     *
     * ACL2:  Peer type: ALL, peer type
     * ACL3:  ANY_TRUSTED
     * (i.e app. bus has three ACLs.)
     *
     * The ACLs does not have any rules.
     *
     * THe ECDHE_PSK key or the SRP key between two peers do not match. (This should cause the ECDHE_PSK and SRP auth. mechanisms to fail.)
     *
     * case A. Peer A has the IC signed by the CA' and both peers specify the ECDHE_ECDSA, ECDHE_PSK, SRP, ECDHE_NULL auth. mechanism.
     *
     * For each of the cases above, Peer A tries to establish a secure session with the app. bus.
     * For each of the cases above, the app. bus tries to establish a secure session with Peer A.
     *
     * peer1 == PeerA
     * peer2 == App. Bus
     */
    void BaseAuthenticationTest5(BusAttachment& identityIssuer)
    {
        //Get peer1 key
        KeyInfoNISTP256 peer1Key;
        PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

        //---------------- Install Policy --------------
        {
            PermissionPolicy policy = CreatePeer1Policy(1);
            PermissionPolicy peer1DefaultPolicy;

            SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
            EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
            /* peer1 will have new CA but managerBus still retains admin group privilege so it can install identity
             * cert to peer1 later */
            SecurityAuthTestHelper::UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, policy, false, true);
            EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
            EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));
        }
        {
            PermissionPolicy policy = CreatePeer2Policy(1);
            PermissionPolicy peer2DefaultPolicy;

            SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
            EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
            /* From here on the managerBus has no access to peer2 anymore */
        }

        //Create peer1 identityCert

        // All Inclusive manifest
        const size_t manifestSize = 1;
        PermissionPolicy::Rule manifest[manifestSize];
        manifest[0].SetObjPath("*");
        manifest[0].SetInterfaceName("*");
        {
            PermissionPolicy::Rule::Member member[1];
            member[0].Set("*",
                          PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                          PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                          PermissionPolicy::Rule::Member::ACTION_MODIFY |
                          PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            manifest[0].SetMembers(1, member);
        }

        GUID128 peer1Guid;

        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(busUsedAsCA,
                                                                   manifest, manifestSize,
                                                                   digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        IdentityCertificate identityCertChainPeer1[1];

        PermissionConfigurator& pcCA = peer1Bus.GetPermissionConfigurator();
        KeyInfoNISTP256 peer1PublicKey;
        EXPECT_EQ(ER_OK, pcCA.GetSigningPublicKey(peer1PublicKey));

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(identityIssuer,
                                                                      "1",
                                                                      peer1Guid.ToString(),
                                                                      peer1PublicKey.GetPublicKey(),
                                                                      "Peer1Alias",
                                                                      3600,
                                                                      identityCertChainPeer1[0],
                                                                      digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChainPeer1, 1, manifest, manifestSize))
            << "Failed to update Identity cert or manifest ";
    }

    BusAttachment managerBus;
    BusAttachment peer1Bus;
    BusAttachment peer2Bus;

    BusAttachment busUsedAsCA;
    BusAttachment busUsedAsLivingRoom;

    InMemoryKeyStoreListener managerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;

    InMemoryKeyStoreListener listener1;
    InMemoryKeyStoreListener listener2;

    DefaultECDHEAuthListener managerAuthListener;
    SecurityAuthentication3AuthListener1 peer1AuthListener;
    SecurityAuthentication3AuthListener2 peer2AuthListener;

    SecurityAuthenticationTestSessionPortListener managerSessionPortListener;
    SecurityAuthenticationTestSessionPortListener peer1SessionPortListener;
    SecurityAuthenticationTestSessionPortListener peer2SessionPortListener;

    SessionId managerToPeer1SessionId;
    SessionId managerToPeer2SessionId;

    SessionPort peer1SessionPort;
    SessionPort peer2SessionPort;

    GUID128 managerGuid;
    GUID128 caGuid;
    qcc::GUID128 livingRoomSGID;

    const size_t manifestSize;
    PermissionPolicy::Rule manifest[1];
    uint8_t caDigest[Crypto_SHA256::DIGEST_SIZE];
};

/**
 * purpose:
 * Verify that peer should fallback to ECDHE_NULL based authentication in case
 * of failure.
 *
 * Setup: (most of the setup done in the Setup and BaseAuthenticationTest5 methods)
 * Peers A and app. bus are claimed.
 * App. bus's IC is signed by the CA.
 *
 * Peer A has a policy with ALL peer type.
 * App. bus has the following ACLs.
 * ACL1:  Peer type: WITH_MEMBERSHIP, SGID: Living Room;
 *
 * ACL2:  Peer type: ALL, peer type
 * ACL3:  ANY_TRUSTED
 * (i.e app. bus has three ACLs.)
 *
 * The ACLs does not have any rules.
 *
 * The ECDHE_PSK key or the SRP key between two peers do not match.
 * (This should cause the ECDHE_PSK and SRP auth. mechanisms to fail.)
 *
 * case A. Peer A has the IC signed by the CA' and both peers specify the
 * ECDHE_ECDSA, ECDHE_PSK, SRP, ECDHE_NULL auth. mechanism.
 *
 * For each of the cases above, Peer A tries to establish a secure session with the app. bus.
 * For each of the cases above, the app. bus tries to establish a secure session with Peer A.
 *
 * Verify:
 * Verify that establishment of secure session fails over ECDHE_ECDSA and then
 * falls over to ECDHE_PSK and fails over and falls to SRP and fails and then it
 * fallbacks over to ECDHE_NULL, where it becomes successful.
 *
 * Peer A == Peer1
 * app. Bus == Peer2
 */
TEST_F(SecurityAuthenticationTest3, DISABLED_authenticate_test5_peer1_tries_to_secure_session) {
    BaseAuthenticationTest5(busUsedAsCA);

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK ALLJOYN_SRP_KEYX ALLJOYN_ECDHE_NULL", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK ALLJOYN_SRP_KEYX ALLJOYN_ECDHE_NULL", &peer2AuthListener));

        peer1AuthListener.calledAuthMechanisms.clear();
        peer2AuthListener.calledAuthMechanisms.clear();

        SecurityApplicationProxy proxy1(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy1.SecureConnection(true));

        EXPECT_TRUE(peer2AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_ECDSA") != peer2AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_ECDSA Auth Mechanism to be requested it was not called.";
        EXPECT_TRUE(peer2AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_PSK") != peer2AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_PSK Auth Mechanism to be requested it was not called.";
        EXPECT_TRUE(peer2AuthListener.calledAuthMechanisms.find("ALLJOYN_SRP_KEYX") != peer2AuthListener.calledAuthMechanisms.end()) << "Expected SRP_KEYX Auth Mechanism to be requested it was not called.";
        EXPECT_TRUE(peer2AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_NULL") != peer2AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_NULL Auth Mechanism to be requested it was not called.";
    }
}

/*
 * see description for authenticate_test5_peer1_tries_to_secure_session
 * This is the same test except peer2 is doing the authentications
 */
TEST_F(SecurityAuthenticationTest3, DISABLED_authenticate_test5_peer2_tries_to_secure_session) {
    BaseAuthenticationTest5(busUsedAsCA);

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK ALLJOYN_SRP_KEYX ALLJOYN_ECDHE_NULL", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK ALLJOYN_SRP_KEYX ALLJOYN_ECDHE_NULL", &peer2AuthListener));

        peer1AuthListener.calledAuthMechanisms.clear();
        peer2AuthListener.calledAuthMechanisms.clear();

        SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy2.SecureConnection(true));

        EXPECT_TRUE(peer1AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_ECDSA") != peer1AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_ECDSA Auth Mechanism to be requested it was not called.";
        EXPECT_TRUE(peer1AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_PSK") != peer1AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_PSK Auth Mechanism to be requested it was not called.";
        EXPECT_TRUE(peer1AuthListener.calledAuthMechanisms.find("ALLJOYN_SRP_KEYX") != peer1AuthListener.calledAuthMechanisms.end()) << "Expected SRP_KEYX Auth Mechanism to be requested it was not called.";
        EXPECT_TRUE(peer1AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_NULL") != peer1AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_NULL Auth Mechanism to be requested it was not called.";
    }
}

/*
 * see description for authenticate_test5_peer1_tries_to_secure_session
 * This is the same test except the list of auth mechanisms is limited to ECDHE
 */
TEST_F(SecurityAuthenticationTest3, authenticate_test5_peer1_tries_to_secure_session_only_ECDHE) {
    BaseAuthenticationTest5(busUsedAsCA);

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_NULL", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_NULL", &peer2AuthListener));

        peer1AuthListener.calledAuthMechanisms.clear();
        peer2AuthListener.calledAuthMechanisms.clear();

        SecurityApplicationProxy proxy1(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy1.SecureConnection(true));

        EXPECT_TRUE(peer2AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_ECDSA") != peer2AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_ECDSA Auth Mechanism to be requested it was not called.";
        EXPECT_TRUE(peer2AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_PSK") != peer2AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_PSK Auth Mechanism to be requested it was not called.";
        EXPECT_TRUE(peer2AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_NULL") != peer2AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_NULL Auth Mechanism to be requested it was not called.";
    }
}

/*
 * see description for authenticate_test5_peer1_tries_to_secure_session
 * This is the same test except the list of auth mechanisms is limited to ECDHE
 * and Peer2 is used for authentication instead of Peer1
 */
TEST_F(SecurityAuthenticationTest3, authenticate_test5_peer2_tries_to_secure_session_only_ECDHE) {
    BaseAuthenticationTest5(busUsedAsCA);

    uint32_t sessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_NULL", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_NULL", &peer2AuthListener));

        peer1AuthListener.calledAuthMechanisms.clear();
        peer2AuthListener.calledAuthMechanisms.clear();

        SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
        EXPECT_EQ(ER_OK, proxy2.SecureConnection(true));

        EXPECT_TRUE(peer1AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_ECDSA") != peer1AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_ECDSA Auth Mechanism to be requested it was not called.";
        EXPECT_TRUE(peer1AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_PSK") != peer1AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_PSK Auth Mechanism to be requested it was not called.";
        EXPECT_TRUE(peer1AuthListener.calledAuthMechanisms.find("ALLJOYN_ECDHE_NULL") != peer1AuthListener.calledAuthMechanisms.end()) << "Expected ECDHE_NULL Auth Mechanism to be requested it was not called.";
    }
}

class SecurityAuthenticationTest4 : public testing::Test {
  public:
    SecurityAuthenticationTest4() :
        managerBus("SecurityAuthenticationManager", true),
        peer1Bus("SecuritAuthenticationPeer1", true),
        peer2Bus("SecurityAuthenticationPeer2", true),

        busUsedAsCA("foo1"),
        busUsedAsCA1("foo2"),
        busUsedAsLivingRoom("foo3"),
        busUsedAsPeerC("foo4"),

        managerKeyStoreListener(),
        peer1KeyStoreListener(),
        peer2KeyStoreListener(),

        listener1(),
        listener2(),
        listener3(),
        listener4(),

        managerAuthListener(),
        peer1AuthListener(),
        peer2AuthListener(),

        managerSessionPortListener(),
        peer1SessionPortListener(),
        peer2SessionPortListener(),

        managerToPeer1SessionId(0),
        managerToPeer2SessionId(0),

        peer1SessionPort(42),
        peer2SessionPort(42),

        managerGuid(),
        caGuid(),
        livingRoomSGID(),

        manifestSize(1)
    {
    }

    virtual void SetUp() {
        EXPECT_EQ(ER_OK, managerBus.Start());
        EXPECT_EQ(ER_OK, managerBus.Connect());
        EXPECT_EQ(ER_OK, peer1Bus.Start());
        EXPECT_EQ(ER_OK, peer1Bus.Connect());
        EXPECT_EQ(ER_OK, peer2Bus.Start());
        EXPECT_EQ(ER_OK, peer2Bus.Connect());

        // Register in memory keystore listeners
        EXPECT_EQ(ER_OK, managerBus.RegisterKeyStoreListener(managerKeyStoreListener));
        EXPECT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
        EXPECT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));

        EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

        busUsedAsCA.Start();
        busUsedAsCA.Connect();
        busUsedAsCA1.Start();
        busUsedAsLivingRoom.Start();
        busUsedAsPeerC.Start();

        EXPECT_EQ(ER_OK, busUsedAsCA.RegisterKeyStoreListener(listener1));
        EXPECT_EQ(ER_OK, busUsedAsCA1.RegisterKeyStoreListener(listener2));
        EXPECT_EQ(ER_OK, busUsedAsLivingRoom.RegisterKeyStoreListener(listener3));
        EXPECT_EQ(ER_OK, busUsedAsPeerC.RegisterKeyStoreListener(listener4));

        EXPECT_EQ(ER_OK, busUsedAsCA.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &managerAuthListener));
        EXPECT_EQ(ER_OK, busUsedAsCA1.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &managerAuthListener));
        EXPECT_EQ(ER_OK, busUsedAsLivingRoom.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &managerAuthListener));
        EXPECT_EQ(ER_OK, busUsedAsPeerC.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &managerAuthListener));

        SessionOpts opts1;
        SessionId managerToManagerSessionId;
        SessionPort managerSessionPort = 42;
        EXPECT_EQ(ER_OK, managerBus.BindSessionPort(managerSessionPort, opts1, managerSessionPortListener));

        SessionOpts opts2;
        EXPECT_EQ(ER_OK, peer1Bus.BindSessionPort(peer1SessionPort, opts2, peer1SessionPortListener));

        SessionOpts opts3;
        EXPECT_EQ(ER_OK, peer2Bus.BindSessionPort(peer2SessionPort, opts3, peer2SessionPortListener));

        EXPECT_EQ(ER_OK, managerBus.JoinSession(managerBus.GetUniqueName().c_str(), managerSessionPort, NULL, managerToManagerSessionId, opts1));
        EXPECT_EQ(ER_OK, managerBus.JoinSession(peer1Bus.GetUniqueName().c_str(), peer1SessionPort, NULL, managerToPeer1SessionId, opts2));
        EXPECT_EQ(ER_OK, managerBus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, managerToPeer2SessionId, opts3));

        //-----------------------Claim each bus Attachments------------------
        SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);


        // All Inclusive manifest
        manifest[0].SetObjPath("*");
        manifest[0].SetInterfaceName("*");
        {
            PermissionPolicy::Rule::Member member[1];
            member[0].Set("*",
                          PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                          PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                          PermissionPolicy::Rule::Member::ACTION_MODIFY |
                          PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            manifest[0].SetMembers(1, member);
        }

        //Get manager key
        KeyInfoNISTP256 managerKey;
        PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

        //Get peer1 key
        KeyInfoNISTP256 peer1Key;
        PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

        //Get peer2 key
        KeyInfoNISTP256 peer2Key;
        PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));

        //Get peer2 key
        KeyInfoNISTP256 caKey;
        PermissionConfigurator& pcCA = busUsedAsCA.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcCA.GetSigningPublicKey(caKey));

        //------------ Claim self(managerBus), Peer1, and Peer2 --------
        uint8_t managerDigest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(managerBus,
                                                                   manifest, manifestSize,
                                                                   managerDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(busUsedAsCA,
                                                                   manifest, manifestSize,
                                                                   caDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        //Create identityCert
        const size_t certChainSize = 1;
        IdentityCertificate identityCertChainMaster[certChainSize];

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                      "0",
                                                                      managerGuid.ToString(),
                                                                      managerKey.GetPublicKey(),
                                                                      "ManagerAlias",
                                                                      3600,
                                                                      identityCertChainMaster[0],
                                                                      managerDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        /* set claimable */
        managerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        EXPECT_EQ(ER_OK, sapWithManager.Claim(caKey,
                                              managerGuid,
                                              managerKey,
                                              identityCertChainMaster, certChainSize,
                                              manifest, manifestSize));


        ECCPublicKey managerPublicKey;
        GetAppPublicKey(managerBus, managerPublicKey);
        ASSERT_EQ(*managerKey.GetPublicKey(), managerPublicKey);

        //Create peer1 identityCert
        IdentityCertificate identityCertChainPeer1[1];

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                      "0",
                                                                      caGuid.ToString(),
                                                                      peer1Key.GetPublicKey(),
                                                                      "Peer1Alias",
                                                                      3600,
                                                                      identityCertChainPeer1[0],
                                                                      caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        //Manager claims Peers
        EXPECT_EQ(ER_OK, sapWithPeer1.Claim(caKey,
                                            managerGuid,
                                            managerKey,
                                            identityCertChainPeer1, 1,
                                            manifest, manifestSize));

        //Create peer2 identityCert
        IdentityCertificate identityCertChainPeer2[certChainSize];

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                      "0",
                                                                      caGuid.ToString(),
                                                                      peer2Key.GetPublicKey(),
                                                                      "Peer2Alias",
                                                                      3600,
                                                                      identityCertChainPeer2[0],
                                                                      caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        peer2Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
        EXPECT_EQ(ER_OK, sapWithPeer2.Claim(caKey,
                                            managerGuid,
                                            managerKey,
                                            identityCertChainPeer2, certChainSize,
                                            manifest, manifestSize));

        EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

        //--------- InstallMembership certificates on self, peer1, and peer2

        String membershipSerial = "1";
        qcc::MembershipCertificate managerMembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                        managerBus,
                                                                        managerBus.GetUniqueName(),
                                                                        managerKey.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        managerMembershipCertificate[0]
                                                                        ));

        EXPECT_EQ(ER_OK, sapWithManager.InstallMembership(managerMembershipCertificate, 1));

        // Install Membership certificate
        qcc::MembershipCertificate peer1MembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("1",
                                                                        managerBus,
                                                                        peer1Bus.GetUniqueName(),
                                                                        peer1Key.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        peer1MembershipCertificate[0]
                                                                        ));

        EXPECT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1MembershipCertificate, 1));

        qcc::MembershipCertificate peer2MembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                        managerBus,
                                                                        peer2Bus.GetUniqueName(),
                                                                        peer2Key.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        peer2MembershipCertificate[0]
                                                                        ));
        EXPECT_EQ(ER_OK, sapWithPeer2.InstallMembership(peer2MembershipCertificate, 1));
    }

    virtual void TearDown() {
        EXPECT_EQ(ER_OK, managerBus.Stop());
        EXPECT_EQ(ER_OK, managerBus.Join());
        EXPECT_EQ(ER_OK, peer1Bus.Stop());
        EXPECT_EQ(ER_OK, peer1Bus.Join());
        EXPECT_EQ(ER_OK, peer2Bus.Stop());
        EXPECT_EQ(ER_OK, peer2Bus.Join());
    }

    /**
     * Base for test case 6.
     * Verify that under the default policy, authentication will succeed if the
     * IC is signed by either the CA or the ASGA.
     *
     * Both Peers use default policy
     */
    void BaseAuthenticationTest6(BusAttachment& identityIssuer, QStatus expectedStatus, const char* reference)
    {
        //Get peer1 key
        KeyInfoNISTP256 peer1Key;
        PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

        //Create peer1 identityCert

        // All Inclusive manifest
        const size_t manifestSize = 1;
        PermissionPolicy::Rule manifest[manifestSize];
        manifest[0].SetObjPath("*");
        manifest[0].SetInterfaceName("*");
        {
            PermissionPolicy::Rule::Member member[1];
            member[0].Set("*",
                          PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                          PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                          PermissionPolicy::Rule::Member::ACTION_MODIFY |
                          PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            manifest[0].SetMembers(1, member);
        }

        GUID128 peer1Guid;

        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(busUsedAsCA1,
                                                                   manifest, manifestSize,
                                                                   digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        IdentityCertificate identityCertChainPeer1[1];

        PermissionConfigurator& pcCA = peer1Bus.GetPermissionConfigurator();
        KeyInfoNISTP256 peer1PublicKey;
        EXPECT_EQ(ER_OK, pcCA.GetSigningPublicKey(peer1PublicKey));

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(identityIssuer,
                                                                      "1",
                                                                      peer1Guid.ToString(),
                                                                      peer1PublicKey.GetPublicKey(),
                                                                      "Peer1Alias",
                                                                      3600,
                                                                      identityCertChainPeer1[0],
                                                                      digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChainPeer1, 1, manifest, manifestSize))
            << "Failed to update Identity cert or manifest ";
        uint32_t sessionId;
        SessionOpts opts;
        EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts));

        {
            EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
            EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
            SecurityApplicationProxy proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), sessionId);
            EXPECT_EQ(expectedStatus, proxy.SecureConnection(true)) << reference << " expects status " << QCC_StatusText(expectedStatus);
            SecurityApplicationProxy proxy2(peer2Bus, peer1Bus.GetUniqueName().c_str(), sessionId);
            EXPECT_EQ(expectedStatus, proxy2.SecureConnection(true)) << reference << " expects status " << QCC_StatusText(expectedStatus);
        }
    }

    BusAttachment managerBus;
    BusAttachment peer1Bus;
    BusAttachment peer2Bus;

    BusAttachment busUsedAsCA;
    BusAttachment busUsedAsCA1;
    BusAttachment busUsedAsLivingRoom;
    BusAttachment busUsedAsPeerC;

    InMemoryKeyStoreListener managerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;

    InMemoryKeyStoreListener listener1;
    InMemoryKeyStoreListener listener2;
    InMemoryKeyStoreListener listener3;
    InMemoryKeyStoreListener listener4;

    SecurityAuthentication2AuthListener managerAuthListener;
    SecurityAuthentication2AuthListener peer1AuthListener;
    SecurityAuthentication2AuthListener peer2AuthListener;

    SecurityAuthenticationTestSessionPortListener managerSessionPortListener;
    SecurityAuthenticationTestSessionPortListener peer1SessionPortListener;
    SecurityAuthenticationTestSessionPortListener peer2SessionPortListener;

    SessionId managerToPeer1SessionId;
    SessionId managerToPeer2SessionId;

    SessionPort peer1SessionPort;
    SessionPort peer2SessionPort;

    GUID128 managerGuid;
    GUID128 caGuid;
    qcc::GUID128 livingRoomSGID;

    const size_t manifestSize;
    PermissionPolicy::Rule manifest[1];
    uint8_t caDigest[Crypto_SHA256::DIGEST_SIZE];
};

/*
 * Purpose:
 * Verify that under the default policy, authentication will succeed if the IC
 * is signed by either the CA or the ASGA.
 *
 * Setup:
 * App. bus is claimed. The CA and ASGA public key are passed as arguments as a part of claim.
 *
 * (The app. bus has a default policy.)
 *
 * Case 1: A and app. bus  set up a ECDHE_ECDSA based session, IC of A is signed by CA.
 * Case 1: A and app. bus  set up a ECDHE_ECDSA based session, IC of A is signed by ASGA.
 *
 * Verification:
 * Cases 1-2: Secure sessions can be set up successfully.
 *
 */
TEST_F(SecurityAuthenticationTest4, authenticate_test6_case1) {
    BaseAuthenticationTest6(busUsedAsCA, ER_OK, "authenticate_test6_case1");
}
/*
 * See authenticate_test6_case1 above for full test description
 */
TEST_F(SecurityAuthenticationTest4, authenticate_test6_case2) {
    BaseAuthenticationTest6(managerBus, ER_OK, "authenticate_test6_case2");
}
