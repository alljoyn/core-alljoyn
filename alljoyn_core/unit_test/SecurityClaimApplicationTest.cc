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
#include <alljoyn/ApplicationStateListener.h>
#include <alljoyn/AuthListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/PermissionConfigurator.h>
#include <alljoyn/SecurityApplicationProxy.h>
#include <qcc/Thread.h>
#include <qcc/Timer.h>

#include <queue>

#include "PermissionMgmtObj.h"
#include "PermissionMgmtTest.h"
#include "InMemoryKeyStore.h"

using namespace ajn;
using namespace qcc;
using namespace std;

/*
 * The unit test use many busy wait loops.  The busy wait loops were chosen
 * over thread sleeps because of the ease of understanding the busy wait loops.
 * Also busy wait loops do not require any platform specific threading code.
 */
#define WAIT_MSECS 5

class SecurityClaimApplicationTest : public testing::Test {
  public:
    SecurityClaimApplicationTest() :
        securityManagerBus("SecurityClaimApplicationManager"),
        peer1Bus("SecurityClaimApplicationPeer1"),
        peer2Bus("SecurityClaimApplicationPeer2"),
        interfaceName("org.allseen.test.SecurityApplication.claim"),
        securityManagerKeyListener(NULL),
        peer1KeyListener(NULL),
        peer2KeyListener(NULL)
    {
    }

    virtual void SetUp() {
        EXPECT_EQ(ER_OK, securityManagerBus.Start());
        EXPECT_EQ(ER_OK, securityManagerBus.Connect());
        EXPECT_EQ(ER_OK, peer1Bus.Start());
        EXPECT_EQ(ER_OK, peer1Bus.Connect());
        EXPECT_EQ(ER_OK, peer2Bus.Start());
        EXPECT_EQ(ER_OK, peer2Bus.Connect());

        // Register in memory keystore listeners
        EXPECT_EQ(ER_OK, securityManagerBus.RegisterKeyStoreListener(securityManagerKeyStoreListener));
        EXPECT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
        EXPECT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));

    }

    virtual void TearDown() {
        securityManagerBus.Stop();
        securityManagerBus.Join();

        peer1Bus.Stop();
        peer1Bus.Join();

        peer2Bus.Stop();
        peer2Bus.Join();

        delete securityManagerKeyListener;
        delete peer1KeyListener;
        delete peer2KeyListener;
    }

    void SetManifestTemplate(BusAttachment& bus)
    {
        // All Inclusive manifest template
        PermissionPolicy::Rule::Member member[1];
        member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        const size_t manifestSize = 1;
        PermissionPolicy::Rule manifestTemplate[manifestSize];
        manifestTemplate[0].SetObjPath("*");
        manifestTemplate[0].SetInterfaceName("*");
        manifestTemplate[0].SetMembers(1, member);
        EXPECT_EQ(ER_OK, bus.GetPermissionConfigurator().SetPermissionManifest(manifestTemplate, manifestSize));
    }

    void InstallMembershipOnManager() {
        //Get manager key
        KeyInfoNISTP256 managerKey;
        PermissionConfigurator& pcManager = securityManagerBus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

        String membershipSerial = "1";
        qcc::MembershipCertificate managerMembershipCertificate[1];
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                        securityManagerBus,
                                                                        securityManagerBus.GetUniqueName(),
                                                                        managerKey.GetPublicKey(),
                                                                        managerGuid,
                                                                        false,
                                                                        3600,
                                                                        managerMembershipCertificate[0]
                                                                        ));
        SecurityApplicationProxy sapWithManagerBus(securityManagerBus, securityManagerBus.GetUniqueName().c_str());
        EXPECT_EQ(ER_OK, sapWithManagerBus.InstallMembership(managerMembershipCertificate, 1));
    }


    BusAttachment securityManagerBus;
    BusAttachment peer1Bus;
    BusAttachment peer2Bus;

    InMemoryKeyStoreListener securityManagerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;

    String interface;
    const char* interfaceName;

    DefaultECDHEAuthListener* securityManagerKeyListener;
    DefaultECDHEAuthListener* peer1KeyListener;
    DefaultECDHEAuthListener* peer2KeyListener;

    GUID128 managerGuid;

};

static void GetAppPublicKey(BusAttachment& bus, ECCPublicKey& publicKey)
{
    KeyInfoNISTP256 keyInfo;
    bus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
    publicKey = *keyInfo.GetPublicKey();
}

TEST_F(SecurityClaimApplicationTest, IsUnclaimableByDefault)
{
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    SecurityApplicationProxy saWithSecurityManager(securityManagerBus, securityManagerBus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStateSecurityManager;
    EXPECT_EQ(ER_OK, saWithSecurityManager.GetApplicationState(applicationStateSecurityManager));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStateSecurityManager);

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    SecurityApplicationProxy saWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, saWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStatePeer1);

    peer2KeyListener = new DefaultECDHEAuthListener();
    peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer2KeyListener);

    SecurityApplicationProxy saWithPeer2(securityManagerBus, peer2Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer2;
    EXPECT_EQ(ER_OK, saWithPeer2.GetApplicationState(applicationStatePeer2));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStatePeer2);
}

class Claim_ApplicationStateListener : public ApplicationStateListener {
  public:
    Claim_ApplicationStateListener() {
        stateChanged = false;
    }

    virtual void State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state) {
        QCC_UNUSED(busName);
        QCC_UNUSED(publicKeyInfo);
        QCC_UNUSED(state);
        stateChanged = true;
    }

    bool stateChanged;
};

/*
 * Claim using ECDHE_NULL
 * Verify that claim is succesful using an ECDHE_NULL based session, where the
 * CA public key and the group public key are the same.
 *
 * Test Case:
 * Claim using ECDHE_NULL
 * caPublic key == adminGroupSecurityPublicKey
 * Identity = Single certificate signed by CA
 */
TEST_F(SecurityClaimApplicationTest, Claim_using_ECDHE_NULL_session_successful)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // peer public key used to generate the identity certificate chain
    ECCPublicKey peer1PublicKey;
    GetAppPublicKey(peer1Bus, peer1PublicKey);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;
    /*
     * Claim Peer1
     * the certificate authority is self signed so the certificateAuthority
     * key is the same as the adminGroup key.
     * For this test the adminGroupId is a randomly generated GUID as long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(securityManagerKey,
                                        securityManagerGuid,
                                        securityManagerKey,
                                        identityCertChain, 1,
                                        manifest, manifestSize));

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(appStateListener.stateChanged);
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMED, applicationStatePeer1);
}

/*
 * Claim fails when using an empty public key identifier
 * Verify that claim fails.
 *
 * Test Case:
 * Claim using ECDHE_NULL
 * Claim using empty caPublicKeyIdentifier.
 * caPublic key == adminGroupSecurityPublicKey
 * Identity = Single certificate signed by CA
 */
TEST_F(SecurityClaimApplicationTest, claim_fails_using_empty_caPublicKeyIdentifier)
{
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStatePeer1);

    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    /*
     * For this test the authorityKeyIdentifier needs to be null
     * the rest of the information should be valid.
     */
    KeyInfoNISTP256 caKey;
    caKey = securityManagerKey;
    caKey.SetKeyId(NULL, 0);
    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // peer public key used to generate the identity certificate chain
    ECCPublicKey peer1PublicKey;
    GetAppPublicKey(peer1Bus, peer1PublicKey);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "1215",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;

    /* set claimable */
    peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    /*
     * Claim Peer1
     * the CA key is empty.
     * For this test the adminGroupId is a randomly generated GUID as long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    EXPECT_NE(ER_OK, sapWithPeer1.Claim(caKey,
                                        securityManagerGuid,
                                        securityManagerKey,
                                        identityCertChain, 1,
                                        manifest, manifestSize));

    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);
}

/*
 * Claim using ECDHE_NULL
 * Verify that claim is succesful using an ECDHE_NULL based session, where the
 * CA public key and the group public key are the same.
 *
 * Test Case:
 * Claim using ECDHE_NULL
 * Claim using empty adminGroupSecurityPublicKeyIdentifier.
 * caPublic key == adminGroupSecurityPublicKey
 * Identity = Single certificate signed by CA
 */
TEST_F(SecurityClaimApplicationTest, claim_fails_using_empty_adminGroupSecurityPublicKeyIdentifier)
{
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStatePeer1);

    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    KeyInfoNISTP256 caKey;
    caKey = securityManagerKey;

    /*
     * For this test the adminGroupAuthorityKeyIdentifier should be null
     * This is the KeyId of the securityManagerKey.
     */
    securityManagerKey.SetKeyId(NULL, 0);

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // peer public key used to generate the identity certificate chain
    ECCPublicKey peer1PublicKey;
    GetAppPublicKey(peer1Bus, peer1PublicKey);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "1215",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;

    /* set claimable */
    peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    /*
     * Claim Peer1
     * the CA key is empty.
     * For this test the adminGroupId is a randomly generated GUID as long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    EXPECT_NE(ER_OK, sapWithPeer1.Claim(caKey,
                                        securityManagerGuid,
                                        securityManagerKey,
                                        identityCertChain, 1,
                                        manifest, manifestSize));

    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);
}

/*
 * Claim using ECDHE_NULL
 * Verify that Claim is successful using an ECDHE_NULL based session, where the
 * CA public key and the admin security group public key are different.
 *
 * Test Case:
 * caPublicKey != adminGroupSecurityPublicKey
 * Identity = Single certificate signed by CA
 */
TEST_F(SecurityClaimApplicationTest, Claim_using_ECDHE_NULL_caKey_not_same_as_adminGroupKey)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer2KeyListener = new DefaultECDHEAuthListener();
    peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer2KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer2Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Use peer2 key as the caKey
    KeyInfoNISTP256 caKey;
    PermissionConfigurator& permissionConfigurator2 = peer2Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator2.GetSigningPublicKey(caKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;
    GUID128 caGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // peer public key used to generate the identity certificate chain
    ECCPublicKey peer1PublicKey;
    GetAppPublicKey(peer1Bus, peer1PublicKey);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                  PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                  PermissionPolicy::Rule::Member::ACTION_MODIFY |
                  PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(peer2Bus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    // peer2 will become the the one signing the identity certificate.
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(peer2Bus,
                                                                  "1215",
                                                                  caGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;
    //Verify the caPublicKey != adminGroupSecurityPublicKey.
    EXPECT_NE(caKey, securityManagerKey);
    /*
     * Claim Peer1
     * the certificate authority is self signed by peer2 using the
     * CreateIdentityCert method
     *
     * For this test the adminGroupId is a randomly generated GUID as long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by peer2
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(caKey,
                                        securityManagerGuid,
                                        securityManagerKey,
                                        identityCertChain, 1,
                                        manifest, manifestSize));

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(appStateListener.stateChanged);
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMED, applicationStatePeer1);
}

/*
 * Claim using ECDHE_PSK
 * Verify that Claim is successful using an ECDHE_PSK based session, where the
 * CA public key and the admin security group public key are the same.
 *
 * Test Case:
 * Claim using ECDHE_PSK
 * caPublic key == adminGroupSecurityPublicKey
 * Identity = Single certificate signed by CA
 */
TEST_F(SecurityClaimApplicationTest, Claim_using_ECDHE_PSK_session_successful)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;
    //EnablePeerSecurity
    const uint8_t psk[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    securityManagerKeyListener = new DefaultECDHEAuthListener(psk, 16);
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_PSK", securityManagerKeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer1KeyListener = new DefaultECDHEAuthListener(psk, 16);
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_PSK", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // peer public key used to generate the identity certificate chain
    ECCPublicKey peer1PublicKey;
    GetAppPublicKey(peer1Bus, peer1PublicKey);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;
    /*
     * Claim Peer1
     * the certificate authority is self signed so the certificateAuthority
     * key is the same as the adminGroup key.
     * For this test the adminGroupId is a randomly generated GUID as long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(securityManagerKey,
                                        securityManagerGuid,
                                        securityManagerKey,
                                        identityCertChain, 1,
                                        manifest, manifestSize));

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(appStateListener.stateChanged);
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMED, applicationStatePeer1);
}

/*
 * Verify that Calim fails when the digest in the Identity certificate does not
 * match the digest of the manifest.
 *
 * Test Case:
 * Claim
 * Manifest digest != digest in the identity certificate */
TEST_F(SecurityClaimApplicationTest, Claim_fails_if_identity_cert_digest_not_equal_claim_manifest)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // peer public key used to generate the identity certificate chain
    ECCPublicKey peer1PublicKey;
    GetAppPublicKey(peer1Bus, peer1PublicKey);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    // Manifest that is different then the All Inclusive manifest passed in as
    // the digest.
    PermissionPolicy::Rule::Member member2[1];
    member2[0].Set("*", PermissionPolicy::Rule::Member::METHOD_CALL, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    PermissionPolicy::Rule manifest2[manifestSize];
    manifest2[0].SetInterfaceName("*");
    manifest2[0].SetMembers(1, member2);

    uint8_t digest2[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest2, manifestSize,
                                                               digest2, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_TRUE(memcmp(digest, digest2, Crypto_SHA256::DIGEST_SIZE) != 0);

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    /*
     * Claim Peer1
     * the certificate authority is self signed so the certificateAuthority
     * key is the same as the adminGroup key.
     * For this test the adminGroupId is a randomly generated GUID as long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    EXPECT_EQ(ER_DIGEST_MISMATCH, sapWithPeer1.Claim(securityManagerKey,
                                                     securityManagerGuid,
                                                     securityManagerKey,
                                                     identityCertChain, 1,
                                                     manifest2, manifestSize));
}

/*
 * Verify the Claim fails when you try to claim the app. bus again with the same
 * set of parameters.
 *
 * Test Case:
 * Try to claim an already claimed application with the same set of parameters
 * as before.
 *
 * We will make a successful ECDHE_NULL claim then claim again.
 */
TEST_F(SecurityClaimApplicationTest, fail_second_claim)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // peer public key used to generate the identity certificate chain
    ECCPublicKey peer1PublicKey;
    GetAppPublicKey(peer1Bus, peer1PublicKey);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;
    /*
     * Claim Peer1
     * the certificate authority is self signed so the certificateAuthority
     * key is the same as the adminGroup key.
     * For this test the adminGroupId is a randomly generated GUID as long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(securityManagerKey,
                                        securityManagerGuid,
                                        securityManagerKey,
                                        identityCertChain, 1,
                                        manifest, manifestSize));

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(appStateListener.stateChanged);
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMED, applicationStatePeer1);

    appStateListener.stateChanged = false;
    EXPECT_EQ(ER_PERMISSION_DENIED, sapWithPeer1.Claim(securityManagerKey,
                                                       securityManagerGuid,
                                                       securityManagerKey,
                                                       identityCertChain, 1,
                                                       manifest, manifestSize));
}

/*
 * Verify the Claim fails when you try to claim the app. bus again with the
 * different  set of parameters.
 *
 * Test Case:
 * Try to claim an already claimed application with a different set of
 * parameters as before.
 *
 * We will make a successful ECDHE_NULL claim then claim again.
 */
TEST_F(SecurityClaimApplicationTest, fail_second_claim_with_different_parameters)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // peer public key used to generate the identity certificate chain
    ECCPublicKey peer1PublicKey;
    GetAppPublicKey(peer1Bus, peer1PublicKey);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;
    /*
     * Claim Peer1
     * the certificate authority is self signed so the certificateAuthority
     * key is the same as the adminGroup key.
     * For this test the adminGroupId is a randomly generated GUID as long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(securityManagerKey,
                                        securityManagerGuid,
                                        securityManagerKey,
                                        identityCertChain, 1,
                                        manifest, manifestSize));

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(appStateListener.stateChanged);
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMED, applicationStatePeer1);


    //Create identityCertChain
    IdentityCertificate identityCertChain2[1];

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member2[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::METHOD_CALL,
                  PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                  PermissionPolicy::Rule::Member::ACTION_MODIFY);
    PermissionPolicy::Rule manifest2[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member2);

    uint8_t digest2[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest2, manifestSize,
                                                               digest2, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain2[0],
                                                                  digest2, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    EXPECT_EQ(ER_PERMISSION_DENIED, sapWithPeer1.Claim(securityManagerKey,
                                                       securityManagerGuid,
                                                       securityManagerKey,
                                                       identityCertChain2, 1,
                                                       manifest2, manifestSize));
}

/*
 * Verify that Claim fails when you try to Claim a "Non-Claimable" application.
 *
 * Test Case:
 * Try to claim a "Non-Claimable" application
 */
TEST_F(SecurityClaimApplicationTest, fail_when_claiming_non_claimable)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    PermissionConfigurator& peer1PermissionConfigurator = peer1Bus.GetPermissionConfigurator();
    peer1PermissionConfigurator.SetApplicationState(PermissionConfigurator::NOT_CLAIMABLE);

    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStatePeer1);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // peer public key used to generate the identity certificate chain
    ECCPublicKey peer1PublicKey;
    GetAppPublicKey(peer1Bus, peer1PublicKey);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;
    /*
     * Claim Peer1
     * the certificate authority is self signed so the certificateAuthority
     * key is the same as the adminGroup key.
     * For this test the adminGroupId is a randomly generated GUID as long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    EXPECT_EQ(ER_PERMISSION_DENIED, sapWithPeer1.Claim(securityManagerKey,
                                                       securityManagerGuid,
                                                       securityManagerKey,
                                                       identityCertChain, 1,
                                                       manifest, manifestSize));
}

/*
 * Verify that Claim fails when the Claimer does not have security enabled.
 *
 * Test Case:
 * Claimer does not have security enabled.
 * Claimer makes a claim call.
 */
TEST_F(SecurityClaimApplicationTest, fail_claimer_security_not_enabled)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);


    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    peer2KeyListener = new DefaultECDHEAuthListener();
    peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer2KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer2Bus);

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());

    //Create admin group key
    KeyInfoNISTP256 caKey;
    PermissionConfigurator& permissionConfigurator = peer2Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(caKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // peer public key used to generate the identity certificate chain
    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& peer1PermissionConfigurator = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.GetSigningPublicKey(peer1Key));

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(peer2Bus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(peer2Bus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  peer1Key.GetPublicKey(),
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    EXPECT_EQ(ER_BUS_SECURITY_NOT_ENABLED, sapWithPeer1.Claim(caKey,
                                                              securityManagerGuid,
                                                              caKey,
                                                              identityCertChain, 1,
                                                              manifest, manifestSize));
}


/*
 * Verify that Claim fails when the Claimant does not have security enabled.
 *
 * Test Case:
 * Claimant does not have security enabled.
 * Claimer makes a claim call.
 */
TEST_F(SecurityClaimApplicationTest, fail_when_peer_being_claimed_is_not_security_enabled)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }


    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    // Create identityCertChain CertChain is only valid for the SecurityManager
    // Not for Peer1.  Since Peer1 has not enabled PeerSecurity it is unable to
    // provide a public key.  We use the securityManagersKey to create an
    // identity certificate.  We expect the resulting failure to be due to the
    // fact that peer1 has not enabled peer security not due to the publicKey
    // mismatch. Either way the result is the same, claim fails.
    IdentityCertificate identityCertChain[1];

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  securityManagerKey.GetPublicKey(),
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;
    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    /*
     * Claim Peer1
     * the certificate authority is self signed so the certificateAuthority
     * key is the same as the adminGroup key.
     * For this test the adminGroupId is a randomly generated GUID as long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    EXPECT_EQ(ER_AUTH_FAIL, sapWithPeer1.Claim(securityManagerKey,
                                               securityManagerGuid,
                                               securityManagerKey,
                                               identityCertChain, 1,
                                               manifest, manifestSize));
}

class ClaimThread1 : public Thread {
  public:
    ClaimThread1() : status(ER_FAIL) { };
    QStatus status;
  protected:
    ThreadReturn STDCALL Run(void* arg) {
        SecurityClaimApplicationTest* thiz = (SecurityClaimApplicationTest*)arg;
        SecurityApplicationProxy sapWithPeer1(thiz->securityManagerBus, thiz->peer1Bus.GetUniqueName().c_str());

        //Create admin group key
        KeyInfoNISTP256 securityManagerKey;
        PermissionConfigurator& permissionConfigurator = thiz->securityManagerBus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

        //Random GUID used for the SecurityManager
        GUID128 securityManagerGuid;

        //Create identityCertChain
        IdentityCertificate identityCertChain[1];

        // peer public key used to generate the identity certificate chain
        ECCPublicKey peer1PublicKey;
        GetAppPublicKey(thiz->peer1Bus, peer1PublicKey);

        // All Inclusive manifest
        PermissionPolicy::Rule::Member member[1];
        member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        const size_t manifestSize = 1;
        PermissionPolicy::Rule manifest[manifestSize];
        manifest[0].SetInterfaceName("*");
        manifest[0].SetMembers(1, member);

        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(thiz->securityManagerBus,
                                                                   manifest, manifestSize,
                                                                   digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(thiz->securityManagerBus,
                                                                      "0",
                                                                      securityManagerGuid.ToString(),
                                                                      &peer1PublicKey,
                                                                      "Alias",
                                                                      3600,
                                                                      identityCertChain[0],
                                                                      digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

        status = sapWithPeer1.Claim(securityManagerKey,
                                    securityManagerGuid,
                                    securityManagerKey,
                                    identityCertChain, 1,
                                    manifest, manifestSize);

        return static_cast<ThreadReturn>(0);
    }
};

class ClaimThread2 : public Thread {
  public:
    ClaimThread2() : status(ER_FAIL) { };
    QStatus status;
  protected:
    ThreadReturn STDCALL Run(void* arg) {
        SecurityClaimApplicationTest* thiz = (SecurityClaimApplicationTest*)arg;
        SecurityApplicationProxy sapWithPeer1(thiz->peer2Bus, thiz->peer1Bus.GetUniqueName().c_str());

        //Create admin group key
        KeyInfoNISTP256 securityManagerKey;
        PermissionConfigurator& permissionConfigurator = thiz->peer2Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

        //Random GUID used for the SecurityManager
        GUID128 securityManagerGuid;

        //Create identityCertChain
        IdentityCertificate identityCertChain[1];

        // peer public key used to generate the identity certificate chain
        ECCPublicKey peer1PublicKey;
        GetAppPublicKey(thiz->peer1Bus, peer1PublicKey);

        // All Inclusive manifest
        PermissionPolicy::Rule::Member member[1];
        member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        const size_t manifestSize = 1;
        PermissionPolicy::Rule manifest[manifestSize];
        manifest[0].SetInterfaceName("*");
        manifest[0].SetMembers(1, member);

        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(thiz->peer2Bus,
                                                                   manifest, manifestSize,
                                                                   digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(thiz->peer2Bus,
                                                                      "0",
                                                                      securityManagerGuid.ToString(),
                                                                      &peer1PublicKey,
                                                                      "Alias",
                                                                      3600,
                                                                      identityCertChain[0],
                                                                      digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

        status = sapWithPeer1.Claim(securityManagerKey,
                                    securityManagerGuid,
                                    securityManagerKey,
                                    identityCertChain, 1,
                                    manifest, manifestSize);
        return static_cast<ThreadReturn>(0);
    }
};
/*
 * Two buses try to claim an application simultaneously.
 *
 * Test Case:
 * Verify that one Claim call is successful and the other one fails.
 */
TEST_F(SecurityClaimApplicationTest, two_peers_claim_application_simultaneously)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer2KeyListener = new DefaultECDHEAuthListener();
    peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer2Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    SecurityApplicationProxy peer2SapWithPeer1(peer2Bus, peer1Bus.GetUniqueName().c_str());
    EXPECT_EQ(ER_OK, peer2SapWithPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    appStateListener.stateChanged = false;

    ClaimThread1 claimThread1;
    ClaimThread2 claimThread2;

    claimThread1.Start(this, NULL);
    claimThread2.Start(this, NULL);

    claimThread1.Join();
    claimThread2.Join();

    //one of the claim threads must pass while the other must fail with Permission denied
    EXPECT_NE(claimThread1.status, claimThread2.status);
    EXPECT_TRUE(claimThread1.status == ER_OK || claimThread2.status == ER_OK);
    EXPECT_TRUE(claimThread1.status == ER_PERMISSION_DENIED || claimThread2.status == ER_PERMISSION_DENIED);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(appStateListener.stateChanged);
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMED, applicationStatePeer1);
}

/*
 * Verify that Claim operation fails when the Claimer and Claimant have
 * different secirity mechanisms enabled.
 *
 * Test Case:
 * Claimer has security enabled for ECDHE_PSK
 * Claimant has security enabled for ECDHE_NULL
 */
TEST_F(SecurityClaimApplicationTest, fail_when_admin_and_peer_use_different_security_mechanisms)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;
    //EnablePeerSecurity
    const uint8_t psk[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    securityManagerKeyListener = new DefaultECDHEAuthListener(psk, 16);
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_PSK", securityManagerKeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& peer1PermissionConfigurator = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.GetSigningPublicKey(peer1Key));

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  peer1Key.GetPublicKey(),
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;

    EXPECT_EQ(ER_AUTH_FAIL, sapWithPeer1.Claim(securityManagerKey,
                                               securityManagerGuid,
                                               securityManagerKey,
                                               identityCertChain, 1,
                                               manifest, manifestSize));
}

/*
 * Verify that Claim fails when the identity certificate's subject is different
 * than the device's public key.
 *
 * Test Case:
 * Generate an identity certificate which has a different public key than that
 * of the device. The device's public key can be found from the Application
 * State notification signal.
 */
TEST_F(SecurityClaimApplicationTest, fail_if_incorrect_publickey_used_in_identity_cert)
{
    Claim_ApplicationStateListener appStateListener;
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    appStateListener.stateChanged = false;
    //EnablePeerSecurity
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    appStateListener.stateChanged = false;

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    /*
     * Get KeyInfo that is not associated with Peer1 to create bad Identity Cert
     * must enable peer security for peer2 so it has a publicKey.
     */
    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& peer1PermissionConfigurator = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.GetSigningPublicKey(peer1Key));

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    // securityManagerKey used instead of Peer1 key to make sure we create an
    // invalid cert.
    EXPECT_NE(*peer1Key.GetPublicKey(), *securityManagerKey.GetPublicKey());
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  securityManagerKey.GetPublicKey(),
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;

    EXPECT_EQ(ER_UNKNOWN_CERTIFICATE, sapWithPeer1.Claim(securityManagerKey,
                                                         securityManagerGuid,
                                                         securityManagerKey,
                                                         identityCertChain, 1,
                                                         manifest, manifestSize));
}

class StateNotification_ApplicationStateListener : public ApplicationStateListener {
  public:

    StateNotification_ApplicationStateListener(const char*busName, PermissionConfigurator::ApplicationState state) :
        busNames(),
        publicKeys(),
        states(),
        busName(busName),
        stateToCheck(state)
    {
        stateChanged = false;
    }


    virtual void State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state) {
        if ((strcmp(busName, this->busName) == 0) && state == stateToCheck) {
            busNames.push(busName);
            publicKeys.push(publicKeyInfo);
            states.push(state);
            stateChanged = true;
        }
    }

    queue<String> busNames;
    queue<KeyInfoNISTP256> publicKeys;
    queue<PermissionConfigurator::ApplicationState> states;
    bool stateChanged;
    const char*busName;
    PermissionConfigurator::ApplicationState stateToCheck;

};

/*
 * TestCase:
 * In factory reset mode, app should emit the state notification.
 *
 * Procedure:
 * Application does not have a keystore.
 * Application bus calls enable peer security with ECDHE_NULL authentication mechanism.
 * Bus does an add match rule for the state notification.
 * Verify that Bus gets the state notification.
 * The state should be "Claimable"
 * publickey algorithm should be equal to 0
 * publickey curveIdentifier should be equal to 0
 * publickey xCo-ordinate and yCo-ordinate are populated and are non-empty
 */
TEST_F(SecurityClaimApplicationTest, get_application_state_signal)
{
    StateNotification_ApplicationStateListener appStateListener(securityManagerBus.GetUniqueName().c_str(), PermissionConfigurator::CLAIMABLE);
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();
    //EnablePeerSecurity
    // the DSA Key Pair should be generated as soon as Enable PeerSecurity is
    // called.
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    EXPECT_FALSE(appStateListener.stateChanged);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(appStateListener.stateChanged);
    EXPECT_EQ(0, appStateListener.publicKeys.front().GetAlgorithm());
    EXPECT_EQ(0, appStateListener.publicKeys.front().GetCurve());
    EXPECT_TRUE(NULL != appStateListener.publicKeys.front().GetPublicKey()->GetX());
    EXPECT_TRUE(NULL != appStateListener.publicKeys.front().GetPublicKey()->GetY());
    EXPECT_EQ(PermissionConfigurator::CLAIMABLE, appStateListener.states.front());
}

/*
 * TestCase:
 * After Claim operation is successful, app should emit the state notification.
 *
 * Procedure:
 * Application does not have a keystore.
 * Application bus calls enable peer security with ECDHE_NULL authentication mechanism.
 *
 * Secondary bus does an add match rule for the state notification.
 *
 * Verify that Secondary bus gets the state notification.
 * The state should be "Claimable"
 * publickey algorithm = 0
 * publickey curveIdentifier = 0
 * publickey xCo-ordinate and yCo-ordinate are populated and are non-empty
 *
 * Standard bus claims application bus successfully.
 *
 * Verify that the Secondary bus gets the Sessionless signal.
 * The state should be "Claimed"
 * publickey algorithm = 0
 * publickey curveIdentifier = 0
 * publickey xCo-ordinate and yCo-ordinate are populated and are same as before.
 */
TEST_F(SecurityClaimApplicationTest, get_application_state_signal_for_claimed_peer)
{
    StateNotification_ApplicationStateListener appStateListener(securityManagerBus.GetUniqueName().c_str(), PermissionConfigurator::CLAIMABLE);
    securityManagerBus.RegisterApplicationStateListener(appStateListener);

    //EnablePeerSecurity
    // the DSA Key Pair should be generated as soon as Enable PeerSecurity is
    // called.
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    EXPECT_FALSE(appStateListener.stateChanged);
    securityManagerBus.AddApplicationStateRule();

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(appStateListener.stateChanged);

    EXPECT_EQ(securityManagerBus.GetUniqueName(), appStateListener.busNames.front());
    appStateListener.busNames.pop();
    EXPECT_EQ(0, appStateListener.publicKeys.front().GetAlgorithm());
    EXPECT_EQ(0, appStateListener.publicKeys.front().GetCurve());
    EXPECT_TRUE(NULL != appStateListener.publicKeys.front().GetPublicKey()->GetX());
    EXPECT_TRUE(NULL != appStateListener.publicKeys.front().GetPublicKey()->GetY());
    appStateListener.publicKeys.pop();
    EXPECT_EQ(PermissionConfigurator::CLAIMABLE, appStateListener.states.front());
    appStateListener.states.pop();

    appStateListener.stateChanged = false;

    //verify we read all the signals
    EXPECT_TRUE(appStateListener.busNames.size() == 0 && appStateListener.publicKeys.size() == 0 && appStateListener.states.size() == 0);

    StateNotification_ApplicationStateListener peer1AppStateListener(peer1Bus.GetUniqueName().c_str(), PermissionConfigurator::CLAIMABLE);
    securityManagerBus.RegisterApplicationStateListener(peer1AppStateListener);
    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (peer1AppStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(peer1AppStateListener.stateChanged);

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());

    EXPECT_EQ(peer1Bus.GetUniqueName(), peer1AppStateListener.busNames.front());
    peer1AppStateListener.busNames.pop();
    EXPECT_EQ(0, peer1AppStateListener.publicKeys.front().GetAlgorithm());
    EXPECT_EQ(0, peer1AppStateListener.publicKeys.front().GetCurve());
    EXPECT_TRUE(NULL != peer1AppStateListener.publicKeys.front().GetPublicKey()->GetX());
    EXPECT_TRUE(NULL != peer1AppStateListener.publicKeys.front().GetPublicKey()->GetY());
    ECCPublicKey peer1PublicKey = *(peer1AppStateListener.publicKeys.front().GetPublicKey());
    peer1AppStateListener.publicKeys.pop();
    EXPECT_EQ(PermissionConfigurator::CLAIMABLE, peer1AppStateListener.states.front());
    peer1AppStateListener.states.pop();

    //verify we read all the signals
    EXPECT_TRUE(peer1AppStateListener.busNames.size() == 0 && peer1AppStateListener.publicKeys.size() == 0 && peer1AppStateListener.states.size() == 0);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    peer1AppStateListener.stateChanged = false;
    peer1AppStateListener.stateToCheck = PermissionConfigurator::CLAIMED;

    EXPECT_TRUE(peer1AppStateListener.busNames.size() == 0 && peer1AppStateListener.publicKeys.size() == 0 && peer1AppStateListener.states.size() == 0);
    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(securityManagerKey,
                                        securityManagerGuid,
                                        securityManagerKey,
                                        identityCertChain, 1,
                                        manifest, manifestSize));

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (peer1AppStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(peer1AppStateListener.stateChanged);

    peer1AppStateListener.busNames.pop();
    EXPECT_EQ(0, peer1AppStateListener.publicKeys.back().GetAlgorithm());
    EXPECT_EQ(0, peer1AppStateListener.publicKeys.back().GetCurve());
    EXPECT_TRUE(NULL != peer1AppStateListener.publicKeys.back().GetPublicKey()->GetX());
    EXPECT_TRUE(NULL != peer1AppStateListener.publicKeys.back().GetPublicKey()->GetY());

    EXPECT_TRUE(memcmp(peer1PublicKey.GetX(), peer1AppStateListener.publicKeys.back().GetPublicKey()->GetX(), qcc::ECC_COORDINATE_SZ) == 0);
    EXPECT_TRUE(memcmp(peer1PublicKey.GetY(), peer1AppStateListener.publicKeys.back().GetPublicKey()->GetY(), qcc::ECC_COORDINATE_SZ) == 0);

    peer1AppStateListener.publicKeys.pop();
    EXPECT_EQ(PermissionConfigurator::CLAIMED, peer1AppStateListener.states.back());
    peer1AppStateListener.states.pop();

    //verify we read all the signals
    EXPECT_TRUE(peer1AppStateListener.busNames.size() == 0 && peer1AppStateListener.publicKeys.size() == 0 && peer1AppStateListener.states.size() == 0) << "The Notification State signal was sent more times than expected.";
}


/*
 * TestCase:
 * After Reset operation, app should emit the state notification and the public
 * key should be preserved.
 *
 * Procedure:
 * Verify that when admin resets the app. bus, the state notification is emitted
 *     and is received by the secondary bus.
 * Verify that Secondary bus gets the state notification.
 * The state should be "Claimable"
 * publickey algorithm = 0
 * publickey curveIdentifier = 0
 * publickey xCo-ordinate and yCo-ordinate are populated and are non-empty and
 *     are preserved and are same as before.
 */
TEST_F(SecurityClaimApplicationTest, DISABLED_get_application_state_signal_for_claimed_then_reset_peer)
{

    //EnablePeerSecurity
    // the DSA Key Pair should be generated as soon as Enable PeerSecurity is
    // called.
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);
    securityManagerBus.AddApplicationStateRule();

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithManager(securityManagerBus, securityManagerBus.GetUniqueName().c_str());

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(securityManagerBus);
    SetManifestTemplate(peer1Bus);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    KeyInfoNISTP256 peer1PublicKey;
    PermissionConfigurator& peer1pc = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, peer1pc.GetSigningPublicKey(peer1PublicKey));

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    // Manager bus claims itself
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  securityManagerKey.GetPublicKey(),
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    EXPECT_EQ(ER_OK, sapWithManager.Claim(securityManagerKey,
                                          managerGuid,
                                          securityManagerKey,
                                          identityCertChain, 1,
                                          manifest, manifestSize));


    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  peer1PublicKey.GetPublicKey(),
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";


    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(securityManagerKey,
                                        managerGuid,
                                        securityManagerKey,
                                        identityCertChain, 1,
                                        manifest, manifestSize));



    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", securityManagerKeyListener);
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", peer1KeyListener);

    InstallMembershipOnManager();

    StateNotification_ApplicationStateListener appStateListener(peer1Bus.GetUniqueName().c_str(), PermissionConfigurator::CLAIMABLE);
    peer1Bus.RegisterApplicationStateListener(appStateListener);

    // Call Reset
    EXPECT_EQ(ER_OK, sapWithPeer1.Reset());

    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", peer1KeyListener);
    SetManifestTemplate(peer1Bus);

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(appStateListener.stateChanged);
}

/*
 * TestCase:
 * When admin calls UpdateIdentity() with a new manifest, the state notification should not be emitted
 *
 * Procedure:
 * Verify that when the admin calls UpdateIdentity() on the app. Bus with a new manifest, the state notification is NOT emitted.
 */
TEST_F(SecurityClaimApplicationTest, no_state_signal_after_update_identity)
{

    //EnablePeerSecurity
    // the DSA Key Pair should be generated as soon as Enable PeerSecurity is
    // called.
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener, NULL, true);
    SetManifestTemplate(securityManagerBus);

    securityManagerBus.AddApplicationStateRule();

    // Setup the test peer
    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);
    SetManifestTemplate(peer1Bus);

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithManager(securityManagerBus, securityManagerBus.GetUniqueName().c_str());

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Create peer 1 public key
    KeyInfoNISTP256 peer1PublicKey;
    PermissionConfigurator& peer1PermissionConfigurator = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.GetSigningPublicKey(peer1PublicKey));

    //Create identityCertChain for the admin to claim the test peer
    IdentityCertificate identityCertChain[1];

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetObjPath("*");
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    // Manager bus claims itself
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  securityManagerKey.GetPublicKey(),
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    EXPECT_EQ(ER_OK, sapWithManager.Claim(securityManagerKey,
                                          managerGuid,
                                          securityManagerKey,
                                          identityCertChain, 1,
                                          manifest, manifestSize));

    // Manager claims peer1
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  peer1PublicKey.GetPublicKey(),
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(securityManagerKey,
                                        managerGuid,
                                        securityManagerKey,
                                        identityCertChain, 1,
                                        manifest, manifestSize));



    //Create identityCertChain
    IdentityCertificate identityCertChain1[1];

    // All Inclusive manifest
    PermissionPolicy::Rule::Member updatedMember[1];
    updatedMember[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED,  PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t updatedManifestSize = 1;
    PermissionPolicy::Rule updatedManifest[updatedManifestSize];
    updatedManifest[0].SetObjPath("*");
    updatedManifest[0].SetInterfaceName("*");
    updatedManifest[0].SetMembers(1, updatedMember);

    uint8_t updatedDigest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               updatedManifest, updatedManifestSize,
                                                               updatedDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "1",
                                                                  managerGuid.ToString(),
                                                                  peer1PublicKey.GetPublicKey(),
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain1[0],
                                                                  updatedDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    EXPECT_EQ(ER_OK, securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", securityManagerKeyListener));
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", peer1KeyListener));

    InstallMembershipOnManager();

    // Call updateIdentity
    StateNotification_ApplicationStateListener appStateListener(peer1Bus.GetUniqueName().c_str(), PermissionConfigurator::CLAIMABLE);
    peer1Bus.RegisterApplicationStateListener(appStateListener);

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChain1, 1,
                                                 updatedManifest, updatedManifestSize));

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_FALSE(appStateListener.stateChanged);
}


/*
 * TestCase:
 * When the manifest template changes in an app, the state notification will be emitted after the app
 * makes a call to SetApplicationState on the application side
 *
 * Procedure:
 * Application changes the manifest template using the permission configurator.

 * The device needs to be Claimed first before changing the manifest template.

 * Verify that Secondary bus gets the state notification.
 * The state should be "Need update"
 * publickey algorithm = 0
 * publickey curveIdentifier = 0
 * publickey xCo-ordinate and yCo-ordinate are populated and are non-empty and are preserved and are same as before.
 */
TEST_F(SecurityClaimApplicationTest, get_state_signal_after_manifest_changes)
{

    //EnablePeerSecurity
    // the DSA Key Pair should be generated as soon as Enable PeerSecurity is
    // called.
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener, NULL, true);

    /* The State signal is only emitted if manifest template is installed */


    // Setup the test peer
    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    KeyInfoNISTP256 peer1PublicKey;
    PermissionConfigurator& peer1PermissionConfigurator = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.GetSigningPublicKey(peer1PublicKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain for the admin to claim the test peer
    IdentityCertificate identityCertChain[1];

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetObjPath("*");
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  peer1PublicKey.GetPublicKey(),
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    StateNotification_ApplicationStateListener appStateListener(peer1Bus.GetUniqueName().c_str(), PermissionConfigurator::CLAIMED);
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(securityManagerKey,
                                        securityManagerGuid,
                                        securityManagerKey,
                                        identityCertChain, 1,
                                        manifest, manifestSize));
    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(appStateListener.stateChanged);

    // Change the manifest
    PermissionPolicy::Rule::Member updatedMember[1];
    updatedMember[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    PermissionPolicy::Rule updatedManifest[manifestSize];
    updatedManifest[0].SetObjPath("*");
    updatedManifest[0].SetInterfaceName("*");
    updatedManifest[0].SetMembers(1, updatedMember);

    StateNotification_ApplicationStateListener needUpateStateListener(peer1Bus.GetUniqueName().c_str(), PermissionConfigurator::NEED_UPDATE);
    securityManagerBus.RegisterApplicationStateListener(needUpateStateListener);

    peer1PermissionConfigurator.SetPermissionManifest(updatedManifest, 1);
    peer1PermissionConfigurator.SetApplicationState(PermissionConfigurator::NEED_UPDATE);
    // Verify that the security manager saw the "Needs Update" notification

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (needUpateStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }
    EXPECT_TRUE(needUpateStateListener.stateChanged);
}

/*
 * Test Case:
 * When the claim operation fails, the state notification should not be emitted.
 *
 * Procedure:
 * Verify that when the claim operation fails, the state notification is not emitted.
 * Verify that the Secondary bus does not get the state notification.
 */
TEST_F(SecurityClaimApplicationTest, no_state_notification_on_claim_fail)
{
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener);

    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    /* The State signal is only emitted if manifest template is installed */
    SetManifestTemplate(peer1Bus);

    StateNotification_ApplicationStateListener appStateListener(peer1Bus.GetUniqueName().c_str(), PermissionConfigurator::CLAIMABLE);
    securityManagerBus.RegisterApplicationStateListener(appStateListener);
    securityManagerBus.AddApplicationStateRule();

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_TRUE(appStateListener.stateChanged);

    SecurityApplicationProxy sapWithPeer1(securityManagerBus, peer1Bus.GetUniqueName().c_str());
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    // peer public key used to generate the identity certificate chain
    ECCPublicKey peer1PublicKey;
    GetAppPublicKey(peer1Bus, peer1PublicKey);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    // Manifest that is different then the All Inclusive manifest passed in as the digest to make the claim call fail
    PermissionPolicy::Rule::Member member2[1];
    member2[0].Set("*", PermissionPolicy::Rule::Member::METHOD_CALL, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    PermissionPolicy::Rule manifest2[manifestSize];
    manifest2[0].SetInterfaceName("*");
    manifest2[0].SetMembers(1, member2);

    uint8_t digest2[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(securityManagerBus,
                                                               manifest2, manifestSize,
                                                               digest2, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    EXPECT_TRUE(memcmp(digest, digest2, Crypto_SHA256::DIGEST_SIZE) != 0);


    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to creat identity certificate.";

    appStateListener.stateChanged = false;
    EXPECT_EQ(ER_DIGEST_MISMATCH, sapWithPeer1.Claim(securityManagerKey,
                                                     securityManagerGuid,
                                                     securityManagerKey,
                                                     identityCertChain, 1,
                                                     manifest2, manifestSize));


    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    EXPECT_FALSE(appStateListener.stateChanged);
}

/*
 * TestCase:
 *  Using PermissionConfigurator, set the claimable field to "Not Claimable".
 *
 * Procedure:
 * Verify that the state notification is emitted with the state = "Not Claimable".
 */
TEST_F(SecurityClaimApplicationTest, not_claimable_state_signal)
{

    //EnablePeerSecurity
    // the DSA Key Pair should be generated as soon as Enable PeerSecurity is
    // called.
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener, NULL, true);


    // Setup the test peer
    peer1KeyListener = new DefaultECDHEAuthListener();
    peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1KeyListener);

    StateNotification_ApplicationStateListener peer1StateListener(peer1Bus.GetUniqueName().c_str(), PermissionConfigurator::NOT_CLAIMABLE);
    peer1Bus.RegisterApplicationStateListener(peer1StateListener);
    peer1Bus.AddApplicationStateRule();

    // Get the Permission configurator for the application
    PermissionConfigurator& peer1PermissionConfigurator = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.SetApplicationState(PermissionConfigurator::NOT_CLAIMABLE));

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (peer1StateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }
    EXPECT_TRUE(peer1StateListener.stateChanged);
}


/*
 * TestCase:
 * State notification is not emitted when Security is not enabled on the app.

 * Procedure:
 * Application does not have a keystore.
 * Application does not enable security.
 * Secondary bus does an add match rule for the state notification.
 *
 * Verify that the Secondary bus does not get the state notification.
 */
TEST_F(SecurityClaimApplicationTest, no_state_notification_when_peer_security_off)
{

    //EnablePeerSecurity
    // the DSA Key Pair should be generated as soon as Enable PeerSecurity is
    // called.
    securityManagerKeyListener = new DefaultECDHEAuthListener();
    securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerKeyListener, NULL, true);

    // Setup the test peer
    peer1KeyListener = new DefaultECDHEAuthListener();

    StateNotification_ApplicationStateListener appStateListener(peer1Bus.GetUniqueName().c_str(), PermissionConfigurator::CLAIMABLE);
    peer1Bus.RegisterApplicationStateListener(appStateListener);
    peer1Bus.AddApplicationStateRule();

    //Wait for a maximum of 10 sec for the Application.State Signal.
    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.stateChanged) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }
    EXPECT_FALSE(appStateListener.stateChanged);



}
