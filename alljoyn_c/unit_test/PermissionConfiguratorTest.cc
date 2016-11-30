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
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/PermissionConfigurator.h>
#include <alljoyn_c/SecurityApplicationProxy.h>
#include <qcc/Util.h>
#include <qcc/GUID.h>
#include <qcc/CryptoECC.h>
#include "ajTestCommon.h"
#include "InMemoryKeyStore.h"
#include "SecurityApplicationProxyTestHelper.h"

using namespace ajn;
using namespace qcc;

#define NULL_AUTH_MECHANISM "ALLJOYN_ECDHE_NULL"
#define SAMPLE_MANAGED_APP_NAME "SampleManagedApp"

#define VALID_ALLOW_ALL_RULES \
    "<rules>" \
    "<node>" \
    "<interface>" \
    "<method>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" \
    "</method>" \
    "<property>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>" \
    "</propert>" \
    "<signal>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>" \
    "</signal>" \
    "</interface>" \
    "</node>" \
    "</rules>"

static AJ_PCSTR s_validAllowAllManifestTemplate =
    "<manifest>"
    "<node>"
    "<interface>"
    SECURITY_LEVEL_ANNOTATION(PRIVILEGED_SECURITY_LEVEL)
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</property>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_invalidManifestTemplate =
    "<manifest>"
    "</manifest>";

static AJ_PCSTR s_validNewerPolicy =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>200</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    VALID_ALLOW_ALL_RULES
    "</acl>"
    "</acls>"
    "</policy>";

static AJ_PCSTR s_MembershipCertName = "TestApp";

static void BasicBusSetup(alljoyn_busattachment* bus, AJ_PCSTR busName, InMemoryKeyStoreListener* keyStoreListener)
{
    *bus = alljoyn_busattachment_create(busName, QCC_FALSE);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_deletedefaultkeystore(busName));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerkeystorelistener(*bus, (alljoyn_keystorelistener)keyStoreListener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_start(*bus));
}

static void BasicBusTearDown(alljoyn_busattachment bus)
{
    ASSERT_EQ(ER_OK, alljoyn_busattachment_stop(bus));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_join(bus));
    alljoyn_busattachment_destroy(bus);
}

class PermissionConfiguratorTestWithoutSecurity : public testing::Test {

  public:

    PermissionConfiguratorTestWithoutSecurity() :
        m_configuratorUnderTest(nullptr),
        m_appUnderTest(nullptr)
    { }

  protected:

    virtual void SetUp()
    {
        BasicBusSetup(&m_appUnderTest, "AppWithoutPeerSecurity", &m_inMemoryKeyStore);
        m_configuratorUnderTest = alljoyn_busattachment_getpermissionconfigurator(m_appUnderTest);
    }

    virtual void TearDown()
    {
        BasicBusTearDown(m_appUnderTest);
    }

  protected:

    alljoyn_permissionconfigurator m_configuratorUnderTest;
    alljoyn_busattachment m_appUnderTest;

  private:

    InMemoryKeyStoreListener m_inMemoryKeyStore;
};

class PermissionConfiguratorTestWithSecurity : public PermissionConfiguratorTestWithoutSecurity {

  public:

    PermissionConfiguratorTestWithSecurity() :
        PermissionConfiguratorTestWithoutSecurity(),
        m_callbacks()
    { }

  protected:

    virtual void SetUp()
    {
        PermissionConfiguratorTestWithoutSecurity::SetUp();

        SetUpCallbacks();
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(m_appUnderTest,
                                                                  NULL_AUTH_MECHANISM,
                                                                  nullptr,
                                                                  nullptr,
                                                                  QCC_FALSE));

        m_configuratorUnderTest = alljoyn_busattachment_getpermissionconfigurator(m_appUnderTest);
    }

    void PassFlagsToCallbacks(bool* policyChanged, bool* factoryResetHappened)
    {
        m_callbacksContext.factoryResetHappened = factoryResetHappened;
        m_callbacksContext.policyChanged = policyChanged;
        alljoyn_permissionconfigurationlistener listener = alljoyn_permissionconfigurationlistener_create(&m_callbacks, &m_callbacksContext);
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecuritywithpermissionconfigurationlistener(m_appUnderTest,
                                                                                                     NULL_AUTH_MECHANISM,
                                                                                                     nullptr,
                                                                                                     nullptr,
                                                                                                     QCC_FALSE,
                                                                                                     listener));
        FlushUnwantedCallback(policyChanged);
    }

  private:

    alljoyn_permissionconfigurationlistener_callbacks m_callbacks;

    struct CallbacksContext {
        bool* policyChanged;
        bool* factoryResetHappened;
    } m_callbacksContext;

    static void AJ_CALL PolicyChangedCallback(const void* context)
    {
        ASSERT_NE(nullptr, context);

        CallbacksContext* passedContext = (CallbacksContext*)context;

        if (nullptr != passedContext->policyChanged) {
            *(passedContext->policyChanged) = true;
        }
    }

    static QStatus AJ_CALL FactoryResetCallback(const void* context)
    {
        /*
         * ASSERT_* returns from the method without any value so it cannot be used here.
         */
        EXPECT_NE(nullptr, context);

        if (nullptr != context) {
            CallbacksContext* passedContext = (CallbacksContext*)context;
            if (nullptr != passedContext->factoryResetHappened) {
                *(passedContext->factoryResetHappened) = true;
            }
        }

        return ER_OK;
    }

    void SetUpCallbacks()
    {
        memset(&m_callbacks, 0, sizeof(m_callbacks));
        m_callbacks.factory_reset = FactoryResetCallback;
        m_callbacks.policy_changed = PolicyChangedCallback;
        m_callbacks.start_management = nullptr;
        m_callbacks.end_management = nullptr;

        memset(&m_callbacksContext, 0, sizeof(m_callbacksContext));
    }

    void FlushUnwantedCallback(bool* policyChanged)
    {
        /*
         * Enabling peer security also triggers policy_changed callback.
         */
        if (nullptr != policyChanged) {
            ASSERT_TRUE(*policyChanged);
            *policyChanged = false;
        }
    }
};

class PermissionConfiguratorPreClaimTest : public PermissionConfiguratorTestWithSecurity {

  public:

    PermissionConfiguratorPreClaimTest() : PermissionConfiguratorTestWithSecurity(),
        m_identityCertificate(nullptr),
        m_altIdentityCertificate(nullptr),
        m_publicKey(nullptr),
        m_privateKey(nullptr),
        m_signedManifestXmls(),
        m_adminGroupGuid(),
        m_adminGroupId(nullptr),
        m_retrievedManifestTemplate(nullptr),
        m_retrievedPublicKey(nullptr)
    {
        for (size_t i = 0; i < ArraySize(m_signedManifestXmls); i++) {
            m_signedManifestXmls[i] = nullptr;
        }
    }

  protected:

    virtual void SetUp()
    {
        PermissionConfiguratorTestWithSecurity::SetUp();

        SecurityApplicationProxyTestHelper::CreateIdentityCert(m_appUnderTest, m_appUnderTest, &m_identityCertificate);
        SecurityApplicationProxyTestHelper::RetrieveDSAPublicKeyFromKeyStore(m_appUnderTest, &m_publicKey);
        SecurityApplicationProxyTestHelper::RetrieveDSAPrivateKeyFromKeyStore(m_appUnderTest, &m_privateKey);

        m_adminGroupId = m_adminGroupGuid.GetBytes();

        ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_signmanifest(s_validAllowAllManifestTemplate,
                                                                       m_identityCertificate,
                                                                       m_privateKey,
                                                                       &m_signedManifestXmls[0]));

        ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifesttemplatefromxml(m_configuratorUnderTest, s_validAllowAllManifestTemplate));
    }

    virtual void TearDown()
    {
        SecurityApplicationProxyTestHelper::DestroyCertificate(m_identityCertificate);
        SecurityApplicationProxyTestHelper::DestroyCertificate(m_altIdentityCertificate);
        SecurityApplicationProxyTestHelper::DestroyKey(m_publicKey);
        SecurityApplicationProxyTestHelper::DestroyKey(m_privateKey);

        if (nullptr != m_retrievedManifestTemplate) {
            alljoyn_permissionconfigurator_manifesttemplate_destroy(m_retrievedManifestTemplate);
            m_retrievedManifestTemplate = nullptr;
        }

        if (nullptr != m_retrievedPublicKey) {
            alljoyn_permissionconfigurator_publickey_destroy(m_retrievedPublicKey);
            m_retrievedPublicKey = nullptr;
        }

        for (size_t i = 0; i < ArraySize(m_signedManifestXmls); i++) {
            if (nullptr != m_signedManifestXmls[i]) {
                alljoyn_securityapplicationproxy_manifest_destroy(m_signedManifestXmls[i]);
                m_signedManifestXmls[i] = nullptr;
            }
        }

        PermissionConfiguratorTestWithSecurity::TearDown();
    }


    void CreateAltIdentityCertificate(const std::string& subject)
    {
        alljoyn_busattachment differentApp;
        InMemoryKeyStoreListener differentKsl;

        /* Provision a different bus, which will create a different public key. */
        BasicBusSetup(&differentApp, subject.c_str(), &differentKsl);
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(differentApp,
                                                                  NULL_AUTH_MECHANISM,
                                                                  nullptr,
                                                                  nullptr,
                                                                  QCC_FALSE));
        SecurityApplicationProxyTestHelper::CreateIdentityCert(differentApp, differentApp, &m_altIdentityCertificate);
        BasicBusTearDown(differentApp);
    }

    AJ_PSTR m_identityCertificate;
    AJ_PSTR m_altIdentityCertificate;
    AJ_PSTR m_publicKey;
    AJ_PSTR m_privateKey;
    AJ_PSTR m_signedManifestXmls[1];
    GUID128 m_adminGroupGuid;
    const uint8_t* m_adminGroupId;
    AJ_PSTR m_retrievedManifestTemplate;
    AJ_PSTR m_retrievedPublicKey;
};

class PermissionConfiguratorPostClaimTest : public PermissionConfiguratorPreClaimTest {

  public:

    PermissionConfiguratorPostClaimTest() : PermissionConfiguratorPreClaimTest(),
        m_membershipCertificate(nullptr),
        m_certificateId(),
        m_certificateIdArray(),
        m_policyXml(nullptr),
        m_defaultPolicyXml(nullptr),
        m_newPolicyXml(nullptr),
        m_manifestArray(),
        m_identityCertificateChain(nullptr)
    { }

  protected:

    virtual void SetUp()
    {
        PermissionConfiguratorPreClaimTest::SetUp();

        ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_claim(m_configuratorUnderTest,
                                                              m_publicKey,
                                                              m_identityCertificate,
                                                              m_adminGroupId,
                                                              GUID128::SIZE,
                                                              m_publicKey,
                                                              (AJ_PCSTR*)m_signedManifestXmls,
                                                              ArraySize(m_signedManifestXmls)));
    }

    virtual void TearDown()
    {
        SecurityApplicationProxyTestHelper::DestroyCertificate(m_membershipCertificate);

        alljoyn_permissionconfigurator_certificateid_cleanup(&m_certificateId);
        alljoyn_permissionconfigurator_certificateidarray_cleanup(&m_certificateIdArray);

        if (nullptr != m_policyXml) {
            alljoyn_permissionconfigurator_policy_destroy(m_policyXml);
            m_policyXml = nullptr;
        }

        if (nullptr != m_defaultPolicyXml) {
            alljoyn_permissionconfigurator_policy_destroy(m_defaultPolicyXml);
            m_defaultPolicyXml = nullptr;
        }

        if (nullptr != m_newPolicyXml) {
            alljoyn_permissionconfigurator_policy_destroy(m_newPolicyXml);
            m_newPolicyXml = nullptr;
        }

        alljoyn_permissionconfigurator_manifestarray_cleanup(&m_manifestArray);

        if (nullptr != m_identityCertificateChain) {
            alljoyn_permissionconfigurator_certificatechain_destroy(m_identityCertificateChain);
            m_identityCertificateChain = nullptr;
        }

        PermissionConfiguratorPreClaimTest::TearDown();
    }

    void CreateMembershipCertificate(const std::string& subject)
    {
        SecurityApplicationProxyTestHelper::CreateMembershipCert(m_appUnderTest,
                                                                 m_appUnderTest,
                                                                 m_adminGroupId,
                                                                 true,
                                                                 subject,
                                                                 &m_membershipCertificate);
    }


    AJ_PSTR m_membershipCertificate;
    alljoyn_certificateid m_certificateId;
    alljoyn_certificateidarray m_certificateIdArray;
    AJ_PSTR m_policyXml;
    AJ_PSTR m_defaultPolicyXml;
    AJ_PSTR m_newPolicyXml;
    alljoyn_manifestarray m_manifestArray;
    AJ_PSTR m_identityCertificateChain;
};

class PermissionConfiguratorApplicationStateTest : public testing::TestWithParam<alljoyn_applicationstate> {
  public:
    alljoyn_permissionconfigurator m_configuratorUnderTest;
    alljoyn_applicationstate m_expectedState;

    PermissionConfiguratorApplicationStateTest() :
        m_configuratorUnderTest(nullptr),
        m_expectedState(GetParam()),
        m_managedAppUnderTest(nullptr)
    { }

    virtual void SetUp()
    {
        BasicBusSetup(&m_managedAppUnderTest, SAMPLE_MANAGED_APP_NAME, &m_managedAppKeyStoreListener);
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(m_managedAppUnderTest,
                                                                  NULL_AUTH_MECHANISM,
                                                                  nullptr,
                                                                  nullptr,
                                                                  QCC_FALSE));
        m_configuratorUnderTest = alljoyn_busattachment_getpermissionconfigurator(m_managedAppUnderTest);
    }

    virtual void TearDown()
    {
        BasicBusTearDown(m_managedAppUnderTest);
    }

  private:
    alljoyn_busattachment m_managedAppUnderTest;
    InMemoryKeyStoreListener m_managedAppKeyStoreListener;
};

class PermissionConfiguratorClaimCapabilitiesTest : public testing::TestWithParam<alljoyn_claimcapabilities> {
  public:
    alljoyn_permissionconfigurator m_configuratorUnderTest;
    alljoyn_claimcapabilities m_expectedValue;

    PermissionConfiguratorClaimCapabilitiesTest() :
        m_configuratorUnderTest(nullptr),
        m_expectedValue(GetParam()),
        m_managedAppUnderTest(nullptr)
    { }

    virtual void SetUp()
    {
        BasicBusSetup(&m_managedAppUnderTest, SAMPLE_MANAGED_APP_NAME, &m_managedAppKeyStoreListener);
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(m_managedAppUnderTest,
                                                                  NULL_AUTH_MECHANISM,
                                                                  nullptr,
                                                                  nullptr,
                                                                  QCC_FALSE));
        m_configuratorUnderTest = alljoyn_busattachment_getpermissionconfigurator(m_managedAppUnderTest);
    }

    virtual void TearDown()
    {
        BasicBusTearDown(m_managedAppUnderTest);
    }

  private:
    alljoyn_busattachment m_managedAppUnderTest;
    InMemoryKeyStoreListener m_managedAppKeyStoreListener;
};

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenGettingApplicationStateWithoutPeerSecurity)
{
    alljoyn_applicationstate state;
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_getapplicationstate(m_configuratorUnderTest, &state));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenGettingClaimCapabilitiesWithoutPeerSecurity)
{
    alljoyn_claimcapabilities claimCapabilities;
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_getclaimcapabilities(m_configuratorUnderTest, &claimCapabilities));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenSettingClaimCapabilitiesWithoutPeerSecurity)
{
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_setclaimcapabilities(m_configuratorUnderTest, CAPABLE_ECDHE_NULL));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenGettingClaimCapabilitiesAdditionalInfoWithoutPeerSecurity)
{
    alljoyn_claimcapabilitiesadditionalinfo claimCapabilitiesAdditionalInfo;
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_getclaimcapabilitiesadditionalinfo(m_configuratorUnderTest, &claimCapabilitiesAdditionalInfo));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenSettingClaimCapabilitiesAdditionalInfoWithoutPeerSecurity)
{
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_setclaimcapabilitiesadditionalinfo(m_configuratorUnderTest, PASSWORD_GENERATED_BY_APPLICATION));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenSettingManifestTemplateWithoutPeerSecurity)
{
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_setmanifesttemplatefromxml(m_configuratorUnderTest, s_validAllowAllManifestTemplate));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenResetingWithoutPeerSecurity)
{
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_reset(m_configuratorUnderTest));
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldReturnErrorWhenSettingManifestTemplateWithEmptyString)
{
    EXPECT_EQ(ER_EOF, alljoyn_permissionconfigurator_setmanifesttemplatefromxml(m_configuratorUnderTest, ""));
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldReturnErrorWhenSettingManifestTemplateWithInvalidXml)
{
    EXPECT_EQ(ER_XML_INVALID_ELEMENT_CHILDREN_COUNT, alljoyn_permissionconfigurator_setmanifesttemplatefromxml(m_configuratorUnderTest, s_invalidManifestTemplate));
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldInitiallyBeNotClaimable)
{
    alljoyn_applicationstate state;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getapplicationstate(m_configuratorUnderTest, &state));

    EXPECT_EQ(NOT_CLAIMABLE, state);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldInitiallyHaveDefaultClaimCapability)
{
    alljoyn_claimcapabilities claimCapabilities;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilities(m_configuratorUnderTest, &claimCapabilities));

    EXPECT_EQ(alljoyn_permissionconfigurator_getdefaultclaimcapabilities(), claimCapabilities);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldInitiallyHaveNoClaimCapabilityAdditionalInfo)
{
    alljoyn_claimcapabilitiesadditionalinfo claimCapabilitiesAdditionalInfo;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilitiesadditionalinfo(m_configuratorUnderTest, &claimCapabilitiesAdditionalInfo));

    EXPECT_EQ(0, claimCapabilitiesAdditionalInfo);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldPassWhenSettingManifestTemplateWithValidXml)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifesttemplatefromxml(m_configuratorUnderTest, s_validAllowAllManifestTemplate));
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldChangeStateToClaimableAfterSettingManifestTemplate)
{
    alljoyn_applicationstate state;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifesttemplatefromxml(m_configuratorUnderTest, s_validAllowAllManifestTemplate));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getapplicationstate(m_configuratorUnderTest, &state));

    EXPECT_EQ(CLAIMABLE, state);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldPassReset)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(m_configuratorUnderTest));
}

/*
 * Disabled until ASACORE-2708 is fixed.
 */
TEST_F(PermissionConfiguratorTestWithSecurity, DISABLED_ShouldNotMakeAppClaimableAfterResetForNotSetTemplate)
{
    alljoyn_applicationstate state;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(m_configuratorUnderTest));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getapplicationstate(m_configuratorUnderTest, &state));

    EXPECT_EQ(NOT_CLAIMABLE, state);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldMakeAppClaimableAfterResetForSetTemplateAndInNeedOfUpdate)
{
    alljoyn_applicationstate state;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifesttemplatefromxml(m_configuratorUnderTest, s_validAllowAllManifestTemplate));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setapplicationstate(m_configuratorUnderTest, NEED_UPDATE));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(m_configuratorUnderTest));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getapplicationstate(m_configuratorUnderTest, &state));

    EXPECT_EQ(CLAIMABLE, state);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldNotResetClaimCapabilitiesToInitialValue)
{
    alljoyn_claimcapabilities claimCapabilities;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilities(m_configuratorUnderTest, CAPABLE_ECDHE_ECDSA));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(m_configuratorUnderTest));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilities(m_configuratorUnderTest, &claimCapabilities));

    EXPECT_EQ(CAPABLE_ECDHE_ECDSA, claimCapabilities);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldNotResetClaimCapabilitiesAdditionalInfoToInitialValue)
{
    alljoyn_claimcapabilitiesadditionalinfo claimCapabilitiesAdditionalInfo;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilitiesadditionalinfo(m_configuratorUnderTest, PASSWORD_GENERATED_BY_APPLICATION));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(m_configuratorUnderTest));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilitiesadditionalinfo(m_configuratorUnderTest, &claimCapabilitiesAdditionalInfo));

    EXPECT_EQ(PASSWORD_GENERATED_BY_APPLICATION, claimCapabilitiesAdditionalInfo);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldCallFactoryResetCallbackAfterReset)
{
    bool factoryResetHappened = false;
    PassFlagsToCallbacks(nullptr, &factoryResetHappened);
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(m_configuratorUnderTest));

    EXPECT_TRUE(factoryResetHappened);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldCallPolicyChangedCallbackAfterReset)
{
    bool policyChanged = false;
    PassFlagsToCallbacks(&policyChanged, nullptr);
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(m_configuratorUnderTest));

    EXPECT_TRUE(policyChanged);
}

INSTANTIATE_TEST_CASE_P(PermissionConfiguratorSetApplicationState,
                        PermissionConfiguratorApplicationStateTest,
                        ::testing::Values(NOT_CLAIMABLE,
                                          CLAIMABLE,
                                          NEED_UPDATE,
                                          CLAIMED));
/*
 * Disabled until ASACORE-2708 is fixed.
 */
TEST_P(PermissionConfiguratorApplicationStateTest, DISABLED_ShouldReturnErrorWhenSettingApplicationStateWithoutManifestTemplate)
{
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_setapplicationstate(m_configuratorUnderTest, m_expectedState));
}

TEST_P(PermissionConfiguratorApplicationStateTest, ShouldSetApplicationState)
{
    alljoyn_applicationstate state;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setapplicationstate(m_configuratorUnderTest, m_expectedState));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getapplicationstate(m_configuratorUnderTest, &state));

    EXPECT_EQ(m_expectedState, state);
}

INSTANTIATE_TEST_CASE_P(PermissionConfiguratorSetClaimCapabilities,
                        PermissionConfiguratorClaimCapabilitiesTest,
                        ::testing::Values(0,
                                          CAPABLE_ECDHE_NULL,
                                          CAPABLE_ECDHE_ECDSA,
                                          CAPABLE_ECDHE_SPEKE,
                                          (CAPABLE_ECDHE_NULL | CAPABLE_ECDHE_ECDSA | CAPABLE_ECDHE_SPEKE)));
TEST_P(PermissionConfiguratorClaimCapabilitiesTest, ShouldPassWhenSettingClaimCapabilities)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilities(m_configuratorUnderTest, m_expectedValue));
}

TEST_P(PermissionConfiguratorClaimCapabilitiesTest, ShouldSetClaimCapabilities)
{
    alljoyn_claimcapabilities claimCapabilities;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilities(m_configuratorUnderTest, m_expectedValue));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilities(m_configuratorUnderTest, &claimCapabilities));

    EXPECT_EQ(m_expectedValue, claimCapabilities);
}

INSTANTIATE_TEST_CASE_P(PermissionConfiguratorSetClaimCapabilitiesAdditionalInfo,
                        PermissionConfiguratorClaimCapabilitiesTest,
                        ::testing::Values(0,
                                          PASSWORD_GENERATED_BY_APPLICATION,
                                          PASSWORD_GENERATED_BY_SECURITY_MANAGER,
                                          (PASSWORD_GENERATED_BY_APPLICATION | PASSWORD_GENERATED_BY_SECURITY_MANAGER)));
TEST_P(PermissionConfiguratorClaimCapabilitiesTest, ShouldPassWhenSettingClaimCapabilitiesAdditionalInfo)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilitiesadditionalinfo(m_configuratorUnderTest, m_expectedValue));
}

TEST_P(PermissionConfiguratorClaimCapabilitiesTest, ShouldSetClaimCapabilitiesAdditionalInfo)
{
    alljoyn_claimcapabilitiesadditionalinfo claimCapabilitiesAdditionalInfo;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilitiesadditionalinfo(m_configuratorUnderTest, m_expectedValue));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilitiesadditionalinfo(m_configuratorUnderTest, &claimCapabilitiesAdditionalInfo));

    EXPECT_EQ(m_expectedValue, claimCapabilitiesAdditionalInfo);
}

#ifdef NDEBUG

TEST_F(PermissionConfiguratorPreClaimTest, shouldReturnErrorWhenSigningManifestWithNullCertificate)
{
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_securityapplicationproxy_signmanifest(s_validAllowAllManifestTemplate,
                                                                             nullptr,
                                                                             m_privateKey,
                                                                             &m_signedManifestXmls[0]));
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldReturnErrorWhenSigningManifestWithNullPrivateKey)
{
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_securityapplicationproxy_signmanifest(s_validAllowAllManifestTemplate,
                                                                             m_identityCertificate,
                                                                             nullptr,
                                                                             &m_signedManifestXmls[0]));
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldReturnErrorWhenClaimingWithNullPublicKey)
{
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_permissionconfigurator_claim(m_configuratorUnderTest,
                                                                    nullptr,
                                                                    m_identityCertificate,
                                                                    m_adminGroupId,
                                                                    GUID128::SIZE,
                                                                    m_publicKey,
                                                                    (AJ_PCSTR*)m_signedManifestXmls,
                                                                    ArraySize(m_signedManifestXmls)));
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldReturnErrorWhenClaimingWithNullCertificate)
{
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_permissionconfigurator_claim(m_configuratorUnderTest,
                                                                    m_publicKey,
                                                                    nullptr,
                                                                    m_adminGroupId,
                                                                    GUID128::SIZE,
                                                                    m_publicKey,
                                                                    (AJ_PCSTR*)m_signedManifestXmls,
                                                                    ArraySize(m_signedManifestXmls)));
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldReturnErrorWhenClaimingWithNullGroupAuthority)
{
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_permissionconfigurator_claim(m_configuratorUnderTest,
                                                                    m_publicKey,
                                                                    m_identityCertificate,
                                                                    m_adminGroupId,
                                                                    GUID128::SIZE,
                                                                    nullptr,
                                                                    (AJ_PCSTR*)m_signedManifestXmls,
                                                                    ArraySize(m_signedManifestXmls)));
}
#endif

TEST_F(PermissionConfiguratorPreClaimTest, shouldPassSignCertificate)
{
    CertificateX509 cert;

    qcc::String unSignedCert =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
        "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
        "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n"
        "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
        "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
        "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n"
        "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n"
        "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAKfmglMgl67L5ALF\n"
        "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n"
        "IizUeK0oI5c=\n"
        "-----END CERTIFICATE-----";

    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_signcertificate(m_configuratorUnderTest, unSignedCert.c_str(), &m_altIdentityCertificate));
    EXPECT_EQ(ER_OK, cert.LoadPEM(qcc::String(m_altIdentityCertificate)));
    EXPECT_STRNE(m_altIdentityCertificate, unSignedCert.c_str());
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldPassSignManifest)
{
    AJ_PSTR signedManifestXmls[1];
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_signmanifest(m_configuratorUnderTest, m_identityCertificate, s_validAllowAllManifestTemplate, &signedManifestXmls[0]));
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldReturnErrorWhenClaimingWithInvalidPublicKey)
{
    AJ_PCSTR invalidPublicKey = m_privateKey;
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_permissionconfigurator_claim(m_configuratorUnderTest,
                                                                    invalidPublicKey,
                                                                    m_identityCertificate,
                                                                    m_adminGroupId,
                                                                    GUID128::SIZE,
                                                                    m_publicKey,
                                                                    (AJ_PCSTR*)m_signedManifestXmls,
                                                                    ArraySize(m_signedManifestXmls)));
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldReturnErrorWhenClaimingWithInvalidCertificate)
{
    AJ_PCSTR invalidIdentityCert = m_privateKey;
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_permissionconfigurator_claim(m_configuratorUnderTest,
                                                                    m_publicKey,
                                                                    invalidIdentityCert,
                                                                    m_adminGroupId,
                                                                    GUID128::SIZE,
                                                                    m_publicKey,
                                                                    (AJ_PCSTR*)m_signedManifestXmls,
                                                                    ArraySize(m_signedManifestXmls)));
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldIgnoreAndPassWhenClaimingWithInvalidGroupId)
{
    uint8_t invalidGroupId[] = { 1 };
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_claim(m_configuratorUnderTest,
                                                          m_publicKey,
                                                          m_identityCertificate,
                                                          invalidGroupId,
                                                          GUID128::SIZE,
                                                          m_publicKey,
                                                          (AJ_PCSTR*)m_signedManifestXmls,
                                                          ArraySize(m_signedManifestXmls)));
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldReturnErrorWhenClaimingWithInvalidGroupIdSize)
{
    size_t invalidGroupIdSize = GUID128::SIZE + 1;
    EXPECT_EQ(ER_INVALID_GUID, alljoyn_permissionconfigurator_claim(m_configuratorUnderTest,
                                                                    m_publicKey,
                                                                    m_identityCertificate,
                                                                    m_adminGroupId,
                                                                    invalidGroupIdSize,
                                                                    m_publicKey,
                                                                    (AJ_PCSTR*)m_signedManifestXmls,
                                                                    ArraySize(m_signedManifestXmls)));
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldReturnErrorWhenClaimingWithInvalidGroupAuthority)
{
    AJ_PCSTR invalidGroupAuthority = m_privateKey;
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_permissionconfigurator_claim(m_configuratorUnderTest,
                                                                    m_publicKey,
                                                                    m_identityCertificate,
                                                                    m_adminGroupId,
                                                                    GUID128::SIZE,
                                                                    invalidGroupAuthority,
                                                                    (AJ_PCSTR*)m_signedManifestXmls,
                                                                    ArraySize(m_signedManifestXmls)));
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldReturnErrorWhenClaimingWithIdentityCertificateThumbprintMismatch)
{
    CreateAltIdentityCertificate("DifferentApp");

    EXPECT_EQ(ER_UNKNOWN_CERTIFICATE, alljoyn_permissionconfigurator_claim(m_configuratorUnderTest,
                                                                           m_publicKey,
                                                                           m_altIdentityCertificate,
                                                                           m_adminGroupId,
                                                                           GUID128::SIZE,
                                                                           m_publicKey,
                                                                           (AJ_PCSTR*)m_signedManifestXmls,
                                                                           ArraySize(m_signedManifestXmls)));

}

TEST_F(PermissionConfiguratorPreClaimTest, shouldPassWhenClaimingWithValidInput)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_claim(m_configuratorUnderTest,
                                                          m_publicKey,
                                                          m_identityCertificate,
                                                          m_adminGroupId,
                                                          GUID128::SIZE,
                                                          m_publicKey,
                                                          (AJ_PCSTR*)m_signedManifestXmls,
                                                          ArraySize(m_signedManifestXmls)));
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldPassGetManifestTemplateAsXml)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_getmanifesttemplate(m_configuratorUnderTest, &m_retrievedManifestTemplate));
    EXPECT_NE(nullptr, m_retrievedManifestTemplate);
}

TEST_F(PermissionConfiguratorPreClaimTest, shouldPassGetPublicKey)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_getpublickey(m_configuratorUnderTest, &m_retrievedPublicKey));
    EXPECT_NE(nullptr, m_retrievedPublicKey);
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassStartManagementCall)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_startmanagement(m_configuratorUnderTest));
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassEndManagementCallAfterStartManagementCall)
{
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_startmanagement(m_configuratorUnderTest));
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_endmanagement(m_configuratorUnderTest));
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldFailEndManagementCallWithoutStartManagementCall)
{
    EXPECT_EQ(ER_MANAGEMENT_NOT_STARTED, alljoyn_permissionconfigurator_endmanagement(m_configuratorUnderTest));
}

TEST_F(PermissionConfiguratorPostClaimTest, secondStartManagementCallShouldFail)
{
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_startmanagement(m_configuratorUnderTest));
    EXPECT_EQ(ER_MANAGEMENT_ALREADY_STARTED, alljoyn_permissionconfigurator_startmanagement(m_configuratorUnderTest));
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassUpdateIdentity)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_updateidentity(m_configuratorUnderTest,
                                                                   m_identityCertificate,
                                                                   (AJ_PCSTR*)m_signedManifestXmls,
                                                                   ArraySize(m_signedManifestXmls)));
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassGetIdentity)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_getidentity(m_configuratorUnderTest, &m_identityCertificateChain));
    EXPECT_NE(nullptr, m_identityCertificateChain);

}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassGetManifests)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_getmanifests(m_configuratorUnderTest, &m_manifestArray));
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassGetManifestsAfterInstallManifests)
{
    const uint8_t additionalManifests = 10;

    for (uint8_t i = 0; i < additionalManifests; i++) {
        ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_installmanifests(m_configuratorUnderTest,
                                                                         (AJ_PCSTR*)m_signedManifestXmls,
                                                                         ArraySize(m_signedManifestXmls),
                                                                         QCC_TRUE));
    }

    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_getmanifests(m_configuratorUnderTest, &m_manifestArray));
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldGetCorrectNumberOfManifestsAfterInstallManifests)
{
    const uint8_t additionalManifests = 10;

    for (uint8_t i = 0; i < additionalManifests; i++) {
        ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_installmanifests(m_configuratorUnderTest,
                                                                         (AJ_PCSTR*)m_signedManifestXmls,
                                                                         ArraySize(m_signedManifestXmls),
                                                                         QCC_TRUE));
    }

    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getmanifests(m_configuratorUnderTest, &m_manifestArray));
    EXPECT_EQ(additionalManifests + 1U, m_manifestArray.count);
    for (uint8_t i = 0; i < additionalManifests + 1; i++) {
        EXPECT_NE(nullptr, m_manifestArray.xmls[i]) << "Manifest XML " << i << " is null";
    }
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassInstallManifests)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_installmanifests(m_configuratorUnderTest,
                                                                     (AJ_PCSTR*)m_signedManifestXmls,
                                                                     ArraySize(m_signedManifestXmls),
                                                                     QCC_FALSE));
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassInstallManifestsAppendMode)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_installmanifests(m_configuratorUnderTest,
                                                                     (AJ_PCSTR*)m_signedManifestXmls,
                                                                     ArraySize(m_signedManifestXmls),
                                                                     QCC_TRUE));
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassGetIdentityCertificateId)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_getidentitycertificateid(m_configuratorUnderTest, &m_certificateId));
    EXPECT_NE(nullptr, m_certificateId.serial);
    EXPECT_NE(nullptr, m_certificateId.issuerPublicKey);
    EXPECT_EQ(nullptr, m_certificateId.issuerAki);
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassUpdatePolicy)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_updatepolicy(m_configuratorUnderTest, s_validNewerPolicy));
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassGetPolicy)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_getpolicy(m_configuratorUnderTest, &m_policyXml));
    EXPECT_NE(nullptr, m_policyXml);
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassGetDefaultPolicy)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_getdefaultpolicy(m_configuratorUnderTest, &m_policyXml));
    EXPECT_NE(nullptr, m_policyXml);
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassResetPolicy)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_resetpolicy(m_configuratorUnderTest));
}

TEST_F(PermissionConfiguratorPostClaimTest, policiesShouldBeDifferentAfterResetPolicy)
{
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_updatepolicy(m_configuratorUnderTest, s_validNewerPolicy));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getpolicy(m_configuratorUnderTest, &m_policyXml));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getdefaultpolicy(m_configuratorUnderTest, &m_defaultPolicyXml));
    ASSERT_STRNE(m_policyXml, m_defaultPolicyXml);

    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_resetpolicy(m_configuratorUnderTest));

    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getpolicy(m_configuratorUnderTest, &m_newPolicyXml));

    EXPECT_STRNE(m_policyXml, m_newPolicyXml);
    EXPECT_STREQ(m_defaultPolicyXml, m_newPolicyXml);

}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassInstallMembership)
{
    CreateMembershipCertificate(s_MembershipCertName);

    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_installmembership(m_configuratorUnderTest, m_membershipCertificate));
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassGetMembershipSummariesNoCertsInstalled)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_getmembershipsummaries(m_configuratorUnderTest,
                                                                           &m_certificateIdArray));
    EXPECT_EQ(0U, m_certificateIdArray.count);
    EXPECT_EQ(nullptr, m_certificateIdArray.ids);
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassGetMembershipSummaries)
{
    CreateMembershipCertificate(s_MembershipCertName);

    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_installmembership(m_configuratorUnderTest, m_membershipCertificate));

    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_getmembershipsummaries(m_configuratorUnderTest,
                                                                           &m_certificateIdArray));

    EXPECT_LT(0U, m_certificateIdArray.count);
    for (size_t i = 0; i < m_certificateIdArray.count; i++) {
        EXPECT_NE(nullptr, m_certificateIdArray.ids[i].serial) << "Serial " << i << " is nullptr";
        EXPECT_NE(nullptr, m_certificateIdArray.ids[i].issuerPublicKey) << "Issuer public key " << i << " is nullptr";
        EXPECT_NE(nullptr, m_certificateIdArray.ids[i].issuerAki) << "Issuer AKI " << i << " is nullptr";
    }
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldPassRemoveMembershipAfterInstallMembership)
{
    CreateMembershipCertificate(s_MembershipCertName);

    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_installmembership(m_configuratorUnderTest, m_membershipCertificate));

    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getmembershipsummaries(m_configuratorUnderTest,
                                                                           &m_certificateIdArray));

    ASSERT_EQ(1U, m_certificateIdArray.count);

    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_removemembership(m_configuratorUnderTest,
                                                                     m_certificateIdArray.ids[0].serial,
                                                                     m_certificateIdArray.ids[0].serialLen,
                                                                     m_certificateIdArray.ids[0].issuerPublicKey,
                                                                     m_certificateIdArray.ids[0].issuerAki,
                                                                     m_certificateIdArray.ids[0].issuerAkiLen));
}

TEST_F(PermissionConfiguratorPostClaimTest, shouldFailRemoveMembershipSecondCall)
{
    CreateMembershipCertificate(s_MembershipCertName);

    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_installmembership(m_configuratorUnderTest, m_membershipCertificate));

    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getmembershipsummaries(m_configuratorUnderTest,
                                                                           &m_certificateIdArray));

    ASSERT_EQ(1U, m_certificateIdArray.count);

    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_removemembership(m_configuratorUnderTest,
                                                                     m_certificateIdArray.ids[0].serial,
                                                                     m_certificateIdArray.ids[0].serialLen,
                                                                     m_certificateIdArray.ids[0].issuerPublicKey,
                                                                     m_certificateIdArray.ids[0].issuerAki,
                                                                     m_certificateIdArray.ids[0].issuerAkiLen));

    EXPECT_EQ(ER_CERTIFICATE_NOT_FOUND, alljoyn_permissionconfigurator_removemembership(m_configuratorUnderTest,
                                                                                        m_certificateIdArray.ids[0].serial,
                                                                                        m_certificateIdArray.ids[0].serialLen,
                                                                                        m_certificateIdArray.ids[0].issuerPublicKey,
                                                                                        m_certificateIdArray.ids[0].issuerAki,
                                                                                        m_certificateIdArray.ids[0].issuerAkiLen));

}

TEST_F(PermissionConfiguratorPostClaimTest, shouldFailGetConnectedPeerPublicKeyForNonconnectedPeer)
{
    AJ_PSTR pubKey = nullptr;
    EXPECT_EQ(ER_BUS_KEY_UNAVAILABLE, alljoyn_permissionconfigurator_getconnectedpeerpublickey(m_configuratorUnderTest, m_adminGroupId, &pubKey));
}
