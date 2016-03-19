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

static AJ_PCSTR VALID_ALLOW_ALL_MANIFEST_TEMPLATE =
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

static AJ_PCSTR INVALID_MANIFEST_TEMPLATE =
    "<manifest>"
    "</manifest>";

static void basicBusSetup(alljoyn_busattachment* bus, AJ_PCSTR busName, ajn::InMemoryKeyStoreListener* keyStoreListener)
{
    *bus = alljoyn_busattachment_create(busName, QCC_FALSE);
    ASSERT_EQ(ER_OK, alljoyn_busattachment_registerkeystorelistener(*bus, (alljoyn_keystorelistener)keyStoreListener));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_start(*bus));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_connect(*bus, ajn::getConnectArg().c_str()));
}

static void basicBusTearDown(alljoyn_busattachment bus)
{
    ASSERT_EQ(ER_OK, alljoyn_busattachment_stop(bus));
    ASSERT_EQ(ER_OK, alljoyn_busattachment_join(bus));
    alljoyn_busattachment_destroy(bus);
}

class PermissionConfiguratorTestWithoutSecurity : public testing::Test {

  public:

    virtual void SetUp()
    {
        basicBusSetup(&appUnderTest, "AppWithoutPeerSecurity", &inMemoryKeyStore);
        configuratorUnderTest = alljoyn_busattachment_getpermissionconfigurator(appUnderTest);
    }

    virtual void TearDown()
    {
        basicBusTearDown(appUnderTest);
    }

  protected:

    alljoyn_permissionconfigurator configuratorUnderTest;
    alljoyn_busattachment appUnderTest;

  private:

    ajn::InMemoryKeyStoreListener inMemoryKeyStore;
};

class PermissionConfiguratorTestWithSecurity : public PermissionConfiguratorTestWithoutSecurity {

    virtual void SetUp()
    {
        PermissionConfiguratorTestWithoutSecurity::SetUp();

        setUpCallbacks();
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(appUnderTest,
                                                                  "ALLJOYN_ECDHE_NULL",
                                                                  nullptr,
                                                                  nullptr,
                                                                  QCC_FALSE));

        configuratorUnderTest = alljoyn_busattachment_getpermissionconfigurator(appUnderTest);
    }

  protected:

    void passFlagsToCallbacks(bool* policyChanged, bool* factoryResetHappened)
    {
        callbacksContext.factoryResetHappened = factoryResetHappened;
        callbacksContext.policyChanged = policyChanged;
        alljoyn_permissionconfigurationlistener listener = alljoyn_permissionconfigurationlistener_create(&callbacks, &callbacksContext);
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecuritywithpermissionconfigurationlistener(appUnderTest,
                                                                                                     "ALLJOYN_ECDHE_NULL",
                                                                                                     nullptr,
                                                                                                     nullptr,
                                                                                                     QCC_FALSE,
                                                                                                     listener));
        flushUnwantedCallback(policyChanged);
    }

  private:

    alljoyn_permissionconfigurationlistener_callbacks callbacks;

    struct CallbacksContext {
        bool* policyChanged;
        bool* factoryResetHappened;
        bool* startManagementHappened;
        bool* endManagementHappened;
    } callbacksContext;

    static void AJ_CALL policyChangedCallback(const void* context)
    {
        ASSERT_NE(nullptr, context);

        CallbacksContext* passedContext = (CallbacksContext*)context;

        if (nullptr != passedContext->policyChanged) {
            *(passedContext->policyChanged) = true;
        }
    }

    static QStatus AJ_CALL factoryResetCallback(const void* context)
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

    void setUpCallbacks()
    {
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.factory_reset = factoryResetCallback;
        callbacks.policy_changed = policyChangedCallback;
        callbacks.start_management = nullptr;
        callbacks.end_management = nullptr;

        memset(&callbacksContext, 0, sizeof(callbacksContext));
    }

    void flushUnwantedCallback(bool* policyChanged)
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
    alljoyn_permissionconfigurator configuratorUnderTest;
    alljoyn_applicationstate expectedState;

    PermissionConfiguratorApplicationStateTest() :
        expectedState(GetParam())
    { }

    virtual void SetUp()
    {
        basicBusSetup(&managedAppUnderTest, "SampleManagedApp", &managedAppKeyStoreListener);
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(managedAppUnderTest,
                                                                  "ALLJOYN_ECDHE_NULL",
                                                                  nullptr,
                                                                  nullptr,
                                                                  QCC_FALSE));
        configuratorUnderTest = alljoyn_busattachment_getpermissionconfigurator(managedAppUnderTest);
    }

  private:
    alljoyn_busattachment managedAppUnderTest;
    ajn::InMemoryKeyStoreListener managedAppKeyStoreListener;
};

class PermissionConfiguratorClaimCapabilitiesTest : public testing::TestWithParam<alljoyn_claimcapabilities> {
  public:
    alljoyn_permissionconfigurator configuratorUnderTest;
    alljoyn_claimcapabilities expectedValue;

    PermissionConfiguratorClaimCapabilitiesTest() :
        expectedValue(GetParam())
    { }

    virtual void SetUp()
    {
        basicBusSetup(&managedAppUnderTest, "SampleManagedApp", &managedAppKeyStoreListener);
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecurity(managedAppUnderTest,
                                                                  "ALLJOYN_ECDHE_NULL",
                                                                  nullptr,
                                                                  nullptr,
                                                                  QCC_FALSE));
        configuratorUnderTest = alljoyn_busattachment_getpermissionconfigurator(managedAppUnderTest);
    }

  private:
    alljoyn_busattachment managedAppUnderTest;
    ajn::InMemoryKeyStoreListener managedAppKeyStoreListener;
};

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenGettingApplicationStateWithoutPeerSecurity)
{
    alljoyn_applicationstate state;
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_getapplicationstate(configuratorUnderTest, &state));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenGettingClaimCapabilitiesWithoutPeerSecurity)
{
    alljoyn_claimcapabilities claimCapabilities;
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_getclaimcapabilities(configuratorUnderTest, &claimCapabilities));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenSettingClaimCapabilitiesWithoutPeerSecurity)
{
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_setclaimcapabilities(configuratorUnderTest, CAPABLE_ECDHE_NULL));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenGettingClaimCapabilitiesAdditionalInfoWithoutPeerSecurity)
{
    alljoyn_claimcapabilitiesadditionalinfo claimCapabilitiesAdditionalInfo;
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_getclaimcapabilitiesadditionalinfo(configuratorUnderTest, &claimCapabilitiesAdditionalInfo));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenSettingClaimCapabilitiesAdditionalInfoWithoutPeerSecurity)
{
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_setclaimcapabilitiesadditionalinfo(configuratorUnderTest, PASSWORD_GENERATED_BY_APPLICATION));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenSettingManifestTemplateWithoutPeerSecurity)
{
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_setmanifestfromxml(configuratorUnderTest, VALID_ALLOW_ALL_MANIFEST_TEMPLATE));
}

TEST_F(PermissionConfiguratorTestWithoutSecurity, ShouldReturnErrorWhenResetingWithoutPeerSecurity)
{
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_reset(configuratorUnderTest));
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldReturnErrorWhenSettingManifestTemplateWithEmptyString)
{
    EXPECT_EQ(ER_EOF, alljoyn_permissionconfigurator_setmanifestfromxml(configuratorUnderTest, ""));
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldReturnErrorWhenSettingManifestTemplateWithInvalidXml)
{
    EXPECT_EQ(ER_XML_MALFORMED, alljoyn_permissionconfigurator_setmanifestfromxml(configuratorUnderTest, INVALID_MANIFEST_TEMPLATE));
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldInitiallyBeNotClaimable)
{
    alljoyn_applicationstate state;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getapplicationstate(configuratorUnderTest, &state));

    EXPECT_EQ(NOT_CLAIMABLE, state);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldInitiallyHaveDefaultClaimCapability)
{
    alljoyn_claimcapabilities claimCapabilities;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilities(configuratorUnderTest, &claimCapabilities));

    EXPECT_EQ(CLAIM_CAPABILITIES_DEFAULT, claimCapabilities);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldInitiallyHaveNoClaimCapabilityAdditionalInfo)
{
    alljoyn_claimcapabilitiesadditionalinfo claimCapabilitiesAdditionalInfo;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilitiesadditionalinfo(configuratorUnderTest, &claimCapabilitiesAdditionalInfo));

    EXPECT_EQ(0, claimCapabilitiesAdditionalInfo);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldPassWhenSettingManifestTemplateWithValidXml)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifestfromxml(configuratorUnderTest, VALID_ALLOW_ALL_MANIFEST_TEMPLATE));
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldChangeStateToClaimableAfterSettingManifestTemplate)
{
    alljoyn_applicationstate state;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifestfromxml(configuratorUnderTest, VALID_ALLOW_ALL_MANIFEST_TEMPLATE));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getapplicationstate(configuratorUnderTest, &state));

    EXPECT_EQ(CLAIMABLE, state);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldPassReset)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(configuratorUnderTest));
}

/*
 * Disabled until ASACORE-2708 is fixed.
 */
TEST_F(PermissionConfiguratorTestWithSecurity, DISABLED_ShouldNotMakeAppClaimableAfterResetForNotSetTemplate)
{
    alljoyn_applicationstate state;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(configuratorUnderTest));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getapplicationstate(configuratorUnderTest, &state));

    EXPECT_EQ(NOT_CLAIMABLE, state);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldMakeAppClaimableAfterResetForSetTemplateAndInNeedOfUpdate)
{
    alljoyn_applicationstate state;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifestfromxml(configuratorUnderTest, VALID_ALLOW_ALL_MANIFEST_TEMPLATE));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setapplicationstate(configuratorUnderTest, NEED_UPDATE));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(configuratorUnderTest));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getapplicationstate(configuratorUnderTest, &state));

    EXPECT_EQ(CLAIMABLE, state);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldNotResetClaimCapabilitiesToInitialValue)
{
    alljoyn_claimcapabilities claimCapabilities;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilities(configuratorUnderTest, CAPABLE_ECDHE_ECDSA));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(configuratorUnderTest));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilities(configuratorUnderTest, &claimCapabilities));

    EXPECT_EQ(CAPABLE_ECDHE_ECDSA, claimCapabilities);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldNotResetClaimCapabilitiesAdditionalInfoToInitialValue)
{
    alljoyn_claimcapabilitiesadditionalinfo claimCapabilitiesAdditionalInfo;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilitiesadditionalinfo(configuratorUnderTest, PASSWORD_GENERATED_BY_APPLICATION));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(configuratorUnderTest));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilitiesadditionalinfo(configuratorUnderTest, &claimCapabilitiesAdditionalInfo));

    EXPECT_EQ(PASSWORD_GENERATED_BY_APPLICATION, claimCapabilitiesAdditionalInfo);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldCallFactoryResetCallbackAfterReset)
{
    bool factoryResetHappened = false;
    passFlagsToCallbacks(nullptr, &factoryResetHappened);
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(configuratorUnderTest));

    EXPECT_TRUE(factoryResetHappened);
}

TEST_F(PermissionConfiguratorTestWithSecurity, ShouldCallPolicyChangedCallbackAfterReset)
{
    bool policyChanged = false;
    passFlagsToCallbacks(&policyChanged, nullptr);
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_reset(configuratorUnderTest));

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
    EXPECT_EQ(ER_FEATURE_NOT_AVAILABLE, alljoyn_permissionconfigurator_setapplicationstate(configuratorUnderTest, expectedState));
}

TEST_P(PermissionConfiguratorApplicationStateTest, ShouldSetApplicationState)
{
    alljoyn_applicationstate state;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setapplicationstate(configuratorUnderTest, expectedState));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getapplicationstate(configuratorUnderTest, &state));

    EXPECT_EQ(expectedState, state);
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
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilities(configuratorUnderTest, expectedValue));
}

TEST_P(PermissionConfiguratorClaimCapabilitiesTest, ShouldSetClaimCapabilities)
{
    alljoyn_claimcapabilities claimCapabilities;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilities(configuratorUnderTest, expectedValue));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilities(configuratorUnderTest, &claimCapabilities));

    EXPECT_EQ(expectedValue, claimCapabilities);
}

INSTANTIATE_TEST_CASE_P(PermissionConfiguratorSetClaimCapabilitiesAdditionalInfo,
                        PermissionConfiguratorClaimCapabilitiesTest,
                        ::testing::Values(0,
                                          PASSWORD_GENERATED_BY_APPLICATION,
                                          PASSWORD_GENERATED_BY_SECURITY_MANAGER,
                                          (PASSWORD_GENERATED_BY_APPLICATION | PASSWORD_GENERATED_BY_SECURITY_MANAGER)));
TEST_P(PermissionConfiguratorClaimCapabilitiesTest, ShouldPassWhenSettingClaimCapabilitiesAdditionalInfo)
{
    EXPECT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilitiesadditionalinfo(configuratorUnderTest, expectedValue));
}

TEST_P(PermissionConfiguratorClaimCapabilitiesTest, ShouldSetClaimCapabilitiesAdditionalInfo)
{
    alljoyn_claimcapabilitiesadditionalinfo claimCapabilitiesAdditionalInfo;
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setclaimcapabilitiesadditionalinfo(configuratorUnderTest, expectedValue));
    ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_getclaimcapabilitiesadditionalinfo(configuratorUnderTest, &claimCapabilitiesAdditionalInfo));

    EXPECT_EQ(expectedValue, claimCapabilitiesAdditionalInfo);
}
