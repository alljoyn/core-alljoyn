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

        EXPECT_EQ(ER_OK, sapWithManager.Claim(managerKey,
                                              managerGuid,
                                              managerKey,
                                              identityCertChainMaster, certChainSize,
                                              manifest, manifestSize));


        ECCPublicKey managerPublicKey;
        sapWithManager.GetEccPublicKey(managerPublicKey);
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
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
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
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
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
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
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
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAll(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
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
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
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
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
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
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
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
    }
    {
        PermissionPolicy policy;
        SecurityAuthTestHelper::GeneratePermissivePolicyAnyTrusted(policy, 1);
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
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
