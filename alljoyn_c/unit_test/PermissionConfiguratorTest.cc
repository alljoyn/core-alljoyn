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
#include <qcc/Util.h>
#include "ajTestCommon.h"
#include "InMemoryKeyStore.h"

using namespace ajn;

#define NULL_AUTH_MECHANISM "ALLJOYN_ECDHE_NULL"
#define SAMPLE_MANAGED_APP_NAME "SampleManagedApp"

static AJ_PCSTR s_validAllowAllManifestTemplate =
    "<manifest>"
    "<node>"
    "<interface>"
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

static void BasicBusSetup(alljoyn_busattachment* bus, AJ_PCSTR busName, InMemoryKeyStoreListener* keyStoreListener)
{
    *bus = alljoyn_busattachment_create(busName, QCC_FALSE);
    EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest(busName));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerkeystorelistener(*bus, (alljoyn_keystorelistener)keyStoreListener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_start(*bus));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_connect(*bus, getConnectArg().c_str()));
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

  protected:

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
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_setmanifestfromxml(m_configuratorUnderTest, s_validAllowAllManifestTemplate));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenResetingWithoutPeerSecurity)
{
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_reset(m_configuratorUnderTest));
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldReturnErrorWhenSettingManifestTemplateWithEmptyString)
{
    EXPECT_EQ(ER_EOF, alljoyn_permissionconfigurator_setmanifestfromxml(m_configuratorUnderTest, ""));
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldReturnErrorWhenSettingManifestTemplateWithInvalidXml)
{
    EXPECT_EQ(ER_XML_MALFORMED, alljoyn_permissionconfigurator_setmanifestfromxml(m_configuratorUnderTest, s_invalidManifestTemplate));
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

    EXPECT_EQ(CLAIM_CAPABILITIES_DEFAULT, claimCapabilities);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldInitiallyHaveNoClaimCapabilityAdditionalInfo)
{
    alljoyn_claimcapabilitiesadditionalinfo claimCapabilitiesAdditionalInfo;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilitiesadditionalinfo(m_configuratorUnderTest, &claimCapabilitiesAdditionalInfo));

    EXPECT_EQ(0, claimCapabilitiesAdditionalInfo);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldPassWhenSettingManifestTemplateWithValidXml)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifestfromxml(m_configuratorUnderTest, s_validAllowAllManifestTemplate));
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldChangeStateToClaimableAfterSettingManifestTemplate)
{
    alljoyn_applicationstate state;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifestfromxml(m_configuratorUnderTest, s_validAllowAllManifestTemplate));
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
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifestfromxml(m_configuratorUnderTest, s_validAllowAllManifestTemplate));
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
