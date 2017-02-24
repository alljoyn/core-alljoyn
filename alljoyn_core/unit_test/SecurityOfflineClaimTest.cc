/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *    
 *    SPDX-License-Identifier: Apache-2.0
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *    
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/
#include <gtest/gtest.h>
#include <alljoyn/ApplicationStateListener.h>
#include <alljoyn/AuthListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/PermissionConfigurator.h>
#include <alljoyn/SecurityApplicationProxy.h>
#include <qcc/Thread.h>
#include <qcc/Timer.h>
#include <qcc/Util.h>

#include "PermissionMgmtObj.h"
#include "PermissionMgmtTest.h"
#include "InMemoryKeyStore.h"
#include "XmlManifestConverter.h"
#include "ajTestCommon.h"

using namespace ajn;
using namespace qcc;
using namespace std;

class SecurityOfflineClaimTest : public testing::Test {
  public:
    SecurityOfflineClaimTest() :
        securityManagerBus("SecurityClaimApplicationManager"),
        peer1Bus("SecurityClaimApplicationPeer1"),
        peer2Bus("SecurityClaimApplicationPeer2"),
        securityManagerAuthListener(nullptr),
        peer1AuthListener(nullptr),
        peer2AuthListener(nullptr) {
    }

    virtual void SetUp() {
        ASSERT_EQ(ER_OK, securityManagerBus.Start());
        ASSERT_EQ(ER_OK, peer1Bus.Start());
        ASSERT_EQ(ER_OK, peer2Bus.Start());

        // Register in memory keystore listeners
        ASSERT_EQ(ER_OK, securityManagerBus.RegisterKeyStoreListener(securityManagerKeyStoreListener));
        ASSERT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
        ASSERT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));

        securityManagerAuthListener = new DefaultECDHEAuthListener();
        ASSERT_EQ(ER_OK, securityManagerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", securityManagerAuthListener));

        peer1AuthListener = new DefaultECDHEAuthListener();
        ASSERT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1AuthListener));

        peer2AuthListener = new DefaultECDHEAuthListener();
        ASSERT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer2AuthListener));
    }

    virtual void TearDown() {
        securityManagerBus.Stop();
        securityManagerBus.Join();

        peer1Bus.Stop();
        peer1Bus.Join();

        peer2Bus.Stop();
        peer2Bus.Join();

        delete securityManagerAuthListener;
        delete peer1AuthListener;
        delete peer2AuthListener;
    }

    void SetManifestTemplate(BusAttachment& bus)
    {
        Manifest manifestTemplate;
        ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifestTemplate));
        ASSERT_EQ(ER_OK, bus.GetPermissionConfigurator().SetPermissionManifestTemplate(const_cast<PermissionPolicy::Rule*>(manifestTemplate->GetRules().data()), manifestTemplate->GetRules().size()));
    }

    BusAttachment securityManagerBus;
    BusAttachment peer1Bus;
    BusAttachment peer2Bus;

    InMemoryKeyStoreListener securityManagerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;

    DefaultECDHEAuthListener* securityManagerAuthListener;
    DefaultECDHEAuthListener* peer1AuthListener;
    DefaultECDHEAuthListener* peer2AuthListener;

    GUID128 managerGuid;
};

static void GetAppPublicKey(BusAttachment& bus, ECCPublicKey& publicKey)
{
    KeyInfoNISTP256 keyInfo;
    bus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
    publicKey = *keyInfo.GetPublicKey();
}

TEST_F(SecurityOfflineClaimTest, IsUnclaimableByDefault)
{
    PermissionConfigurator& pcSecurityManager = securityManagerBus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationStateSecurityManager;
    ASSERT_EQ(ER_OK, pcSecurityManager.GetApplicationState(applicationStateSecurityManager));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStateSecurityManager);

    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStatePeer1);

    PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationStatePeer2;
    ASSERT_EQ(ER_OK, pcPeer2.GetApplicationState(applicationStatePeer2));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStatePeer2);
}

/*
 * Claim using offline provisioning
 * Verify that claim is successful using an offline session, where the
 * CA public key and the group public key are the same.
 *
 * Test Case:
 * Claim using PermissionConfigurator
 * caPublic key == adminGroupSecurityPublicKey
 * Identity = Single certificate signed by CA
 */
TEST_F(SecurityOfflineClaimTest, Claim_using_PermissionConfigurator_session_successful)
{
    SetManifestTemplate(securityManagerBus);
    SetManifestTemplate(peer1Bus);
    SetManifestTemplate(peer2Bus);

    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    ASSERT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

    //Random GUID used for the SecurityManager
    GUID128 securityManagerGuid;

    //Create identityCertChain
    IdentityCertificate identityCertChain[1];

    //Peer public key used to generate the identity certificate chain
    ECCPublicKey peer1PublicKey;
    GetAppPublicKey(peer1Bus, peer1PublicKey);

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0])) << "Failed to create identity certificate.";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(securityManagerBus, identityCertChain[0], manifests[0]));
    /*
     * Claim Peer1
     * the certificate authority is self signed so the certificateAuthority
     * key is the same as the adminGroup key.
     * For this test the adminGroupId is a randomly generated GUID. As long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    vector<std::string> manifestsXmlStrings;
    vector<AJ_PCSTR> manifestsXmls;
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestsToXmlArray(manifests, ArraySize(manifests), manifestsXmlStrings));
    PermissionMgmtTestHelper::UnwrapStrings(manifestsXmlStrings, manifestsXmls);
    EXPECT_EQ(ER_OK, pcPeer1.Claim(securityManagerKey,
                                   securityManagerGuid,
                                   securityManagerKey,
                                   identityCertChain, ArraySize(identityCertChain),
                                   manifestsXmls.data(), manifestsXmls.size()));

    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMED, applicationStatePeer1);

    std::vector<CertificateX509> returnedCertChain;
    ASSERT_EQ(ER_OK, pcPeer1.GetIdentity(returnedCertChain));
    EXPECT_EQ(1U, returnedCertChain.size());
    if (returnedCertChain.size() > 0) {
        String encodedSourceCert, encodedReturnedCert;
        ASSERT_EQ(ER_OK, identityCertChain[0].EncodeCertificateDER(encodedSourceCert));
        ASSERT_EQ(ER_OK, returnedCertChain[0].EncodeCertificateDER(encodedReturnedCert));
        EXPECT_EQ(encodedSourceCert, encodedReturnedCert);
    }

    std::vector<Manifest> returnedManifests;
    ASSERT_EQ(ER_OK, pcPeer1.GetManifests(returnedManifests));
    EXPECT_EQ(1U, returnedManifests.size());
    if (returnedManifests.size() > 0) {
        EXPECT_EQ(manifests[0], returnedManifests[0]);
    }
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
TEST_F(SecurityOfflineClaimTest, claim_fails_using_empty_caPublicKeyIdentifier)
{
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStatePeer1);

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

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "1215",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0])) << "Failed to create identity certificate.";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(securityManagerBus, identityCertChain[0], manifests[0]));

    /* set claimable */
    ASSERT_EQ(ER_OK, peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE));
    /*
     * Claim Peer1
     * the CA key is empty.
     * For this test the adminGroupId is a randomly generated GUID. As long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    vector<std::string> manifestsXmlStrings;
    vector<AJ_PCSTR> manifestsXmls;
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestsToXmlArray(manifests, ArraySize(manifests), manifestsXmlStrings));
    PermissionMgmtTestHelper::UnwrapStrings(manifestsXmlStrings, manifestsXmls);
    EXPECT_NE(ER_OK, pcPeer1.Claim(caKey,
                                   securityManagerGuid,
                                   securityManagerKey,
                                   identityCertChain, ArraySize(identityCertChain),
                                   manifestsXmls.data(), manifestsXmls.size()));

    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);
}

/*
 * Claim fails when using an empty admin key identifier
 * Verify that claim fails
 */
TEST_F(SecurityOfflineClaimTest, claim_fails_using_empty_adminGroupSecurityPublicKeyIdentifier)
{
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStatePeer1);

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

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "1215",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0])) << "Failed to create identity certificate.";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(securityManagerBus, identityCertChain[0], manifests[0]));
    /* set claimable */
    ASSERT_EQ(ER_OK, pcPeer1.SetApplicationState(PermissionConfigurator::CLAIMABLE));
    /*
     * Claim Peer1
     * the CA key is empty.
     * For this test the adminGroupId is a randomly generated GUID. As long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    vector<std::string> manifestsXmlStrings;
    vector<AJ_PCSTR> manifestsXmls;
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestsToXmlArray(manifests, ArraySize(manifests), manifestsXmlStrings));
    PermissionMgmtTestHelper::UnwrapStrings(manifestsXmlStrings, manifestsXmls);
    EXPECT_NE(ER_OK, pcPeer1.Claim(caKey,
                                   securityManagerGuid,
                                   securityManagerKey,
                                   identityCertChain, ArraySize(identityCertChain),
                                   manifestsXmls.data(), manifestsXmls.size()));

    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);
}

/*
 * Claim using offline provisioning
 * Verify that Claim is successful using an offline based session, where the
 * CA public key and the admin security group public key are different.
 *
 * Test Case:
 * caPublicKey != adminGroupSecurityPublicKey
 * Identity = Single certificate signed by CA
 */
TEST_F(SecurityOfflineClaimTest, Claim_caKey_not_same_as_adminGroupKey)
{
    SetManifestTemplate(securityManagerBus);
    SetManifestTemplate(peer1Bus);
    SetManifestTemplate(peer2Bus);

    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
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

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    // peer2 will become the the one signing the identity certificate.
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(peer2Bus,
                                                                  "1215",
                                                                  caGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0])) << "Failed to create identity certificate.";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(peer2Bus, identityCertChain[0], manifests[0]));
    //Verify the caPublicKey != adminGroupSecurityPublicKey.
    ASSERT_NE(caKey, securityManagerKey);
    /*
     * Claim Peer1
     * the certificate authority is self signed by peer2 using the
     * CreateIdentityCert method
     *
     * For this test the adminGroupId is a randomly generated GUID. As long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by peer2
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    vector<std::string> manifestsXmlStrings;
    vector<AJ_PCSTR> manifestsXmls;
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestsToXmlArray(manifests, ArraySize(manifests), manifestsXmlStrings));
    PermissionMgmtTestHelper::UnwrapStrings(manifestsXmlStrings, manifestsXmls);
    EXPECT_EQ(ER_OK, pcPeer1.Claim(caKey,
                                   securityManagerGuid,
                                   securityManagerKey,
                                   identityCertChain, ArraySize(identityCertChain),
                                   manifestsXmls.data(), manifestsXmls.size()));

    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMED, applicationStatePeer1);
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
TEST_F(SecurityOfflineClaimTest, fail_second_claim)
{
    SetManifestTemplate(securityManagerBus);
    SetManifestTemplate(peer1Bus);
    SetManifestTemplate(peer2Bus);

    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
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

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0])) << "Failed to create identity certificate.";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(securityManagerBus, identityCertChain[0], manifests[0]));
    /*
     * Claim Peer1
     * the certificate authority is self signed so the certificateAuthority
     * key is the same as the adminGroup key.
     * For this test the adminGroupId is a randomly generated GUID. As long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    vector<std::string> manifestsXmlStrings;
    vector<AJ_PCSTR> manifestsXmls;
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestsToXmlArray(manifests, ArraySize(manifests), manifestsXmlStrings));
    PermissionMgmtTestHelper::UnwrapStrings(manifestsXmlStrings, manifestsXmls);
    EXPECT_EQ(ER_OK, pcPeer1.Claim(securityManagerKey,
                                   securityManagerGuid,
                                   securityManagerKey,
                                   identityCertChain, ArraySize(identityCertChain),
                                   manifestsXmls.data(), manifestsXmls.size()));

    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMED, applicationStatePeer1);


    EXPECT_EQ(ER_PERMISSION_DENIED, pcPeer1.Claim(securityManagerKey,
                                                  securityManagerGuid,
                                                  securityManagerKey,
                                                  identityCertChain, ArraySize(identityCertChain),
                                                  manifestsXmls.data(), manifestsXmls.size()));
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
TEST_F(SecurityOfflineClaimTest, fail_second_claim_with_different_parameters)
{
    SetManifestTemplate(securityManagerBus);
    SetManifestTemplate(peer1Bus);
    SetManifestTemplate(peer2Bus);

    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
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

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0])) << "Failed to create identity certificate.";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(securityManagerBus, identityCertChain[0], manifests[0]));
    /*
     * Claim Peer1
     * the certificate authority is self signed so the certificateAuthority
     * key is the same as the adminGroup key.
     * For this test the adminGroupId is a randomly generated GUID. As long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    vector<std::string> manifestsXmlStrings;
    vector<AJ_PCSTR> manifestsXmls;
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestsToXmlArray(manifests, ArraySize(manifests), manifestsXmlStrings));
    PermissionMgmtTestHelper::UnwrapStrings(manifestsXmlStrings, manifestsXmls);
    EXPECT_EQ(ER_OK, pcPeer1.Claim(securityManagerKey,
                                   securityManagerGuid,
                                   securityManagerKey,
                                   identityCertChain, ArraySize(identityCertChain),
                                   manifestsXmls.data(), manifestsXmls.size()));

    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::CLAIMED, applicationStatePeer1);

    //Create identityCertChain
    IdentityCertificate identityCertChain2[1];

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain2[0])) << "Failed to create identity certificate.";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(securityManagerBus, identityCertChain2[0], manifests[0]));

    EXPECT_EQ(ER_PERMISSION_DENIED, pcPeer1.Claim(securityManagerKey,
                                                  securityManagerGuid,
                                                  securityManagerKey,
                                                  identityCertChain2, ArraySize(identityCertChain2),
                                                  manifestsXmls.data(), manifestsXmls.size()));
}

/*
 * Verify that Claim fails when you try to Claim a "Non-Claimable" application.
 *
 * Test Case:
 * Try to claim a "Non-Claimable" application
 */
TEST_F(SecurityOfflineClaimTest, fail_when_claiming_non_claimable)
{
    SetManifestTemplate(securityManagerBus);
    SetManifestTemplate(peer1Bus);
    SetManifestTemplate(peer2Bus);

    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    PermissionConfigurator& peer1PermissionConfigurator = peer1Bus.GetPermissionConfigurator();
    peer1PermissionConfigurator.SetApplicationState(PermissionConfigurator::NOT_CLAIMABLE);

    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
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

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  &peer1PublicKey,
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0])) << "Failed to create identity certificate.";


    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(securityManagerBus, identityCertChain[0], manifests[0]));
    /*
     * Claim Peer1
     * the certificate authority is self signed so the certificateAuthority
     * key is the same as the adminGroup key.
     * For this test the adminGroupId is a randomly generated GUID. As long as the
     * GUID is consistent it's unimportant that the GUID is random.
     * Use generated identity certificate signed by the securityManager
     * Since we are only interested in claiming the peer we are using an all
     * inclusive manifest.
     */
    vector<std::string> manifestsXmlStrings;
    vector<AJ_PCSTR> manifestsXmls;
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestsToXmlArray(manifests, ArraySize(manifests), manifestsXmlStrings));
    PermissionMgmtTestHelper::UnwrapStrings(manifestsXmlStrings, manifestsXmls);
    EXPECT_EQ(ER_PERMISSION_DENIED, pcPeer1.Claim(securityManagerKey,
                                                  securityManagerGuid,
                                                  securityManagerKey,
                                                  identityCertChain, ArraySize(identityCertChain),
                                                  manifestsXmls.data(), manifestsXmls.size()));
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
TEST_F(SecurityOfflineClaimTest, fail_if_incorrect_publickey_used_in_identity_cert)
{
    SetManifestTemplate(securityManagerBus);
    SetManifestTemplate(peer1Bus);
    SetManifestTemplate(peer2Bus);

    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    ASSERT_EQ(ER_OK, pcPeer1.GetApplicationState(applicationStatePeer1));
    ASSERT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

    //Create admin group key
    KeyInfoNISTP256 securityManagerKey;
    PermissionConfigurator& permissionConfigurator = securityManagerBus.GetPermissionConfigurator();
    ASSERT_EQ(ER_OK, permissionConfigurator.GetSigningPublicKey(securityManagerKey));

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
    ASSERT_EQ(ER_OK, peer1PermissionConfigurator.GetSigningPublicKey(peer1Key));

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    // securityManagerKey used instead of Peer1 key to make sure we create an
    // invalid cert.
    EXPECT_NE(*peer1Key.GetPublicKey(), *securityManagerKey.GetPublicKey());
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(securityManagerBus,
                                                                  "0",
                                                                  securityManagerGuid.ToString(),
                                                                  securityManagerKey.GetPublicKey(),
                                                                  "Alias",
                                                                  3600,
                                                                  identityCertChain[0])) << "Failed to create identity certificate.";


    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(securityManagerBus, identityCertChain[0], manifests[0]));

    vector<std::string> manifestsXmlStrings;
    vector<AJ_PCSTR> manifestsXmls;
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestsToXmlArray(manifests, ArraySize(manifests), manifestsXmlStrings));
    PermissionMgmtTestHelper::UnwrapStrings(manifestsXmlStrings, manifestsXmls);
    EXPECT_EQ(ER_UNKNOWN_CERTIFICATE, pcPeer1.Claim(securityManagerKey,
                                                    securityManagerGuid,
                                                    securityManagerKey,
                                                    identityCertChain, ArraySize(identityCertChain),
                                                    manifestsXmls.data(), manifestsXmls.size()));
}