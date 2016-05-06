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
#include <algorithm>
#include <gtest/gtest.h>
#include <time.h>
#include <alljoyn/AuthListener.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/PermissionConfigurator.h>
#include <alljoyn_c/SecurityApplicationProxy.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/StringUtil.h>
#include <qcc/XmlElement.h>
#include "ajTestCommon.h"
#include "InMemoryKeyStore.h"
#include "SecurityApplicationProxyTestHelper.h"

using namespace ajn;
using namespace qcc;
using namespace std;

#define WAIT_MS 5
#define CALLBACK_TIMEOUT_MS 2000
#define ACLS_INDEX 2

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

#define GROUP_ID_PLACEHOLDER ":groupId"
#define GROUP_PUBKEY_PLACEHOLDER ":groupPubKey"
#define SECURITY_MANAGER_BUS_NAME "securityManager"
#define MANAGED_APP_BUS_NAME "managedApp"
#define INVALID_BUS_NAME "invalidBusName"
#define NULL_AND_ECDSA_AUTH_MECHANISM "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA"

static AJ_PCSTR s_validSecurityManagerManifestTemplate =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Modify\"/>"
    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>"
    "</method>"
    "<property>"
    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Modify\"/>"
    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>"
    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Observe\"/>"
    "</property>"
    "<signal>"
    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>"
    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Observe\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_validManagedAppManifestTemplate =
    "<manifest>"
    "<node name=\"/Node0\">"
    "<interface name=\"org.test.alljoyn.Interface\">"
    "<method name=\"MethodName\">"
    "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
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

static AJ_PCSTR s_validOlderPolicy =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>100</serialNumber>"
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

static AJ_PCSTR s_defaultPolicyFixForAsacore2755 =
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>" GROUP_PUBKEY_PLACEHOLDER "</publicKey>"
    "<sgID>" GROUP_ID_PLACEHOLDER "</sgID>"
    "</peer>"
    "</peers>"
    VALID_ALLOW_ALL_RULES
    "</acl>";

static AJ_PCSTR s_invalidManifest =
    "<manifest>"
    "</manifest>";

class SecurityApplicationProxyPreProxyTest : public testing::Test {
  public:

    SecurityApplicationProxyPreProxyTest() :
        m_securityManagerSecurityApplicationProxy(nullptr),
        m_securityManager(nullptr),
        m_securityManagerIdentityCertificate(nullptr),
        m_securityManagerPublicKey(nullptr),
        m_securityManagerPrivateKey(nullptr),
        m_signedManifestXml(nullptr),
        m_defaultEcdheAuthListener(nullptr),
        m_securityManagerPermissionConfigurationListener(nullptr)
    { }

    virtual void SetUp()
    {
        SetUpCallbacks();

        m_defaultEcdheAuthListener = (alljoyn_authlistener) new DefaultECDHEAuthListener();
        m_securityManagerPermissionConfigurationListener = alljoyn_permissionconfigurationlistener_create(&m_callbacks, nullptr);
        BasicBusSetup(&m_securityManager,
                      SECURITY_MANAGER_BUS_NAME,
                      &securityManagerKeyStore,
                      s_validSecurityManagerManifestTemplate,
                      m_securityManagerPermissionConfigurationListener);

        m_securityManagerUniqueName = alljoyn_busattachment_getuniquename(m_securityManager);
        OpenManagementSession(m_securityManager, m_securityManagerUniqueName, &m_securityManagerSessionId);

        SecurityApplicationProxyTestHelper::CreateIdentityCert(m_securityManager, m_securityManager, &m_securityManagerIdentityCertificate);
        SecurityApplicationProxyTestHelper::RetrieveDSAPublicKeyFromKeyStore(m_securityManager, &m_securityManagerPublicKey);
        SecurityApplicationProxyTestHelper::RetrieveDSAPrivateKeyFromKeyStore(m_securityManager, &m_securityManagerPrivateKey);
    }

    virtual void TearDown()
    {
        if (nullptr != m_securityManagerSecurityApplicationProxy) {
            alljoyn_securityapplicationproxy_destroy(m_securityManagerSecurityApplicationProxy);
        }

        if (nullptr != m_signedManifestXml) {
            alljoyn_securityapplicationproxy_manifest_destroy(m_signedManifestXml);
        }

        SecurityApplicationProxyTestHelper::DestroyCertificate(m_securityManagerIdentityCertificate);
        SecurityApplicationProxyTestHelper::DestroyKey(m_securityManagerPublicKey);
        SecurityApplicationProxyTestHelper::DestroyKey(m_securityManagerPrivateKey);
        alljoyn_authlistenerasync_destroy(m_defaultEcdheAuthListener);

        alljoyn_permissionconfigurationlistener_destroy(m_securityManagerPermissionConfigurationListener);
        BasicBusTearDown(m_securityManager);
    }

  protected:
    static bool s_policyChangeHappened;
    static bool s_factoryResetHappened;
    static bool s_startManagementHappened;
    static bool s_endManagementHappened;

    alljoyn_securityapplicationproxy m_securityManagerSecurityApplicationProxy;
    alljoyn_busattachment m_securityManager;
    alljoyn_sessionid m_securityManagerSessionId;
    alljoyn_permissionconfigurationlistener_callbacks m_callbacks;
    AJ_PCSTR m_securityManagerUniqueName;
    AJ_PSTR m_securityManagerIdentityCertificate;
    AJ_PSTR m_securityManagerPublicKey;
    AJ_PSTR m_securityManagerPrivateKey;
    AJ_PSTR m_signedManifestXml;

    void BasicBusSetup(alljoyn_busattachment* bus,
                       AJ_PCSTR busName,
                       InMemoryKeyStoreListener* keyStoreListener,
                       AJ_PCSTR manifestTemplate,
                       alljoyn_permissionconfigurationlistener configurationListener)
    {
        *bus = alljoyn_busattachment_create(busName, QCC_FALSE);
        ASSERT_EQ(ER_OK, alljoyn_busattachment_start(*bus));
        ASSERT_EQ(ER_OK, alljoyn_busattachment_connect(*bus, getConnectArg().c_str()));
        ASSERT_EQ(ER_OK, alljoyn_busattachment_registerkeystorelistener(*bus, (alljoyn_keystorelistener)keyStoreListener));
        ASSERT_EQ(ER_OK, alljoyn_busattachment_enablepeersecuritywithpermissionconfigurationlistener(*bus,
                                                                                                     NULL_AND_ECDSA_AUTH_MECHANISM,
                                                                                                     m_defaultEcdheAuthListener,
                                                                                                     nullptr,
                                                                                                     QCC_FALSE,
                                                                                                     configurationListener));
        SetUpManifest(*bus, manifestTemplate);
    }

    void BasicBusTearDown(alljoyn_busattachment bus)
    {
        ASSERT_EQ(ER_OK, alljoyn_busattachment_stop(bus));
        ASSERT_EQ(ER_OK, alljoyn_busattachment_join(bus));
        alljoyn_busattachment_destroy(bus);
    }

    void OpenManagementSession(alljoyn_busattachment fromBus, AJ_PCSTR toBusUniqueName, alljoyn_sessionid* sessionId)
    {
        alljoyn_sessionopts sessionOpts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES,
                                                                     QCC_FALSE,
                                                                     ALLJOYN_PROXIMITY_ANY,
                                                                     ALLJOYN_TRANSPORT_ANY);
        ASSERT_EQ(ER_OK, alljoyn_busattachment_joinsession(fromBus,
                                                           toBusUniqueName,
                                                           PERMISSION_MANAGEMENT_SESSION_PORT,
                                                           nullptr,
                                                           sessionId,
                                                           sessionOpts));

        alljoyn_sessionopts_destroy(sessionOpts);
    }

  private:

    alljoyn_authlistener m_defaultEcdheAuthListener;
    alljoyn_permissionconfigurationlistener m_securityManagerPermissionConfigurationListener;
    InMemoryKeyStoreListener securityManagerKeyStore;

    static void AJ_CALL PolicyChangedCallback(const void* context)
    {
        QCC_UNUSED(context);
        s_policyChangeHappened = true;
    }

    static QStatus AJ_CALL FactoryResetCallback(const void* context)
    {
        QCC_UNUSED(context);
        s_factoryResetHappened = true;

        return ER_OK;
    }

    static void AJ_CALL StartManagementCallback(const void* context)
    {
        QCC_UNUSED(context);
        s_startManagementHappened = true;
    }

    static void AJ_CALL EndManagementCallback(const void* context)
    {
        QCC_UNUSED(context);
        s_endManagementHappened = true;
    }

    void SetUpCallbacks()
    {
        memset(&m_callbacks, 0, sizeof(m_callbacks));
        m_callbacks.factory_reset = FactoryResetCallback;
        m_callbacks.policy_changed = PolicyChangedCallback;
        m_callbacks.start_management = StartManagementCallback;
        m_callbacks.end_management = EndManagementCallback;

        s_policyChangeHappened = false;
        s_factoryResetHappened = false;
        s_startManagementHappened = false;
        s_endManagementHappened = false;
    }

    void SetUpManifest(alljoyn_busattachment bus, AJ_PCSTR manifestTemplate)
    {
        alljoyn_permissionconfigurator configurator = alljoyn_busattachment_getpermissionconfigurator(bus);
        ASSERT_EQ(ER_OK, alljoyn_permissionconfigurator_setmanifestfromxml(configurator, manifestTemplate));
    }
};

class SecurityApplicationProxySelfClaimTest : public SecurityApplicationProxyPreProxyTest {
  public:
    SecurityApplicationProxySelfClaimTest() : SecurityApplicationProxyPreProxyTest(),
        m_invalidProxy(nullptr),
        m_managedApp(nullptr),
        m_managedAppPermissionConfigurationListener(nullptr)
    {
        InitializeArrayElements(m_securityManagerSignedManifests, ArraySize(m_securityManagerSignedManifests));
    }

    virtual void SetUp()
    {
        SecurityApplicationProxyPreProxyTest::SetUp();

        m_managedAppPermissionConfigurationListener = alljoyn_permissionconfigurationlistener_create(&m_callbacks, nullptr);
        BasicBusSetup(&m_managedApp,
                      MANAGED_APP_BUS_NAME,
                      &m_managedAppKeyStore,
                      s_validManagedAppManifestTemplate,
                      m_managedAppPermissionConfigurationListener);

        m_adminGroupId = m_adminGroupGuid.GetBytes();

        m_invalidProxy = alljoyn_securityapplicationproxy_create(m_securityManager, INVALID_BUS_NAME, m_securityManagerSessionId);

        m_securityManagerSecurityApplicationProxy = alljoyn_securityapplicationproxy_create(m_securityManager, m_securityManagerUniqueName, m_securityManagerSessionId);

        ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_signmanifest(s_validSecurityManagerManifestTemplate,
                                                                       m_securityManagerIdentityCertificate,
                                                                       m_securityManagerPrivateKey,
                                                                       &(m_securityManagerSignedManifests[0])));
    }

    virtual void TearDown()
    {
        BasicBusTearDown(m_managedApp);
        DeleteManifestsArray(m_securityManagerSignedManifests, ArraySize(m_securityManagerSignedManifests));

        alljoyn_permissionconfigurationlistener_destroy(m_managedAppPermissionConfigurationListener);

        alljoyn_securityapplicationproxy_destroy(m_invalidProxy);

        SecurityApplicationProxyPreProxyTest::TearDown();
    }

  protected:
    const uint8_t* m_adminGroupId;
    alljoyn_securityapplicationproxy m_invalidProxy;
    alljoyn_busattachment m_managedApp;
    AJ_PSTR m_securityManagerSignedManifests[1];
    GUID128 m_adminGroupGuid;

    void InitializeArrayElements(AJ_PSTR* someArray, size_t elementCount)
    {
        for (size_t index = 0; index < elementCount; index++) {
            someArray[index] = nullptr;
        }
    }

    void DeleteManifestsArray(AJ_PSTR* someArray, size_t elementCount)
    {
        for (size_t index = 0; index < elementCount; index++) {
            alljoyn_securityapplicationproxy_manifest_destroy(someArray[index]);
        }
    }

  private:
    alljoyn_permissionconfigurationlistener m_managedAppPermissionConfigurationListener;
    InMemoryKeyStoreListener m_managedAppKeyStore;
};

class SecurityApplicationProxyPreClaimTest : public SecurityApplicationProxySelfClaimTest {
  public:
    SecurityApplicationProxyPreClaimTest() : SecurityApplicationProxySelfClaimTest(),
        m_adminGroupMembershipCertificate(nullptr),
        m_managedAppIdentityCertificate(nullptr),
        m_managedAppIdentityCertificateChain(nullptr),
        m_retrievedManagedAppManifestTemplate(nullptr),
        m_retrievedManagedAppEccPublicKey(nullptr),
        m_managedAppSecurityApplicationProxy(nullptr)
    {
        InitializeArrayElements(m_managedAppSignedManifests, ArraySize(m_managedAppSignedManifests));
    }

    virtual void SetUp()
    {
        SecurityApplicationProxySelfClaimTest::SetUp();

        SelfClaimSecurityManager();
        GetAdminGroupMembershipCertificate();
        GetManagedAppSecurityApplicationProxy();
        GetManagedAppIdentityCertAndManifest();
    }

    virtual void TearDown()
    {
        if (nullptr != m_retrievedManagedAppManifestTemplate) {
            alljoyn_securityapplicationproxy_manifesttemplate_destroy(m_retrievedManagedAppManifestTemplate);
        }

        if (nullptr != m_retrievedManagedAppEccPublicKey) {
            alljoyn_securityapplicationproxy_eccpublickey_destroy(m_retrievedManagedAppEccPublicKey);
        }

        alljoyn_securityapplicationproxy_destroy(m_managedAppSecurityApplicationProxy);

        SecurityApplicationProxyTestHelper::DestroyCertificate(m_managedAppIdentityCertificateChain);
        SecurityApplicationProxyTestHelper::DestroyCertificate(m_managedAppIdentityCertificate);
        SecurityApplicationProxyTestHelper::DestroyCertificate(m_adminGroupMembershipCertificate);

        DeleteManifestsArray(m_managedAppSignedManifests, ArraySize(m_managedAppSignedManifests));

        SecurityApplicationProxySelfClaimTest::TearDown();
    }

  protected:
    AJ_PSTR m_adminGroupMembershipCertificate;
    AJ_PSTR m_managedAppIdentityCertificate;
    AJ_PSTR m_managedAppIdentityCertificateChain;
    AJ_PSTR m_retrievedManagedAppManifestTemplate;
    AJ_PSTR m_retrievedManagedAppEccPublicKey;
    AJ_PSTR m_managedAppSignedManifests[1];
    alljoyn_securityapplicationproxy m_managedAppSecurityApplicationProxy;
    alljoyn_applicationstate m_retrievedManagedAppApplicationState;
    alljoyn_claimcapabilities m_retrievedManagedAppClaimCapabilities;
    alljoyn_claimcapabilities m_retrievedManagedAppClaimCapabilitiesAdditionalInfo;

    string RemoveNewLines(string input)
    {
        input.erase(remove(input.begin(), input.end(), '\n'), input.end());
        return input;
    }

    bool WaitForTrueOrTimeout(bool* isTrue)
    {
        time_t startTime = time(nullptr);
        int timeDiffMs = difftime(time(nullptr), startTime) * 1000;
        while (!(*isTrue)
               && (timeDiffMs < CALLBACK_TIMEOUT_MS)) {
            qcc::Sleep(WAIT_MS);
            timeDiffMs = difftime(time(nullptr), startTime) * 1000;
        }

        return *isTrue;
    }

  private:
    alljoyn_sessionid m_managedAppSessionId;
    AJ_PCSTR m_managedAppUniqueName;

    void SelfClaimSecurityManager()
    {
        ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_claim(m_securityManagerSecurityApplicationProxy,
                                                                m_securityManagerPublicKey,
                                                                m_securityManagerIdentityCertificate,
                                                                m_adminGroupId,
                                                                GUID128::SIZE,
                                                                m_securityManagerPublicKey,
                                                                (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                                1U));
    }

    void GetAdminGroupMembershipCertificate()
    {
        SecurityApplicationProxyTestHelper::CreateMembershipCert(m_securityManager,
                                                                 m_securityManager,
                                                                 m_adminGroupId,
                                                                 true,
                                                                 &m_adminGroupMembershipCertificate);
    }

    void GetManagedAppSecurityApplicationProxy()
    {
        m_managedAppUniqueName = alljoyn_busattachment_getuniquename(m_managedApp);

        OpenManagementSession(m_securityManager, m_managedAppUniqueName, &m_managedAppSessionId);
        m_managedAppSecurityApplicationProxy = alljoyn_securityapplicationproxy_create(m_securityManager, m_managedAppUniqueName, m_managedAppSessionId);
    }

    void GetManagedAppIdentityCertAndManifest()
    {
        SecurityApplicationProxyTestHelper::CreateIdentityCert(m_securityManager,
                                                               m_managedApp,
                                                               &m_managedAppIdentityCertificate,
                                                               false);
        ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_signmanifest(s_validManagedAppManifestTemplate,
                                                                       m_managedAppIdentityCertificate,
                                                                       m_securityManagerPrivateKey,
                                                                       &(m_managedAppSignedManifests[0])));
    }
};

class SecurityApplicationProxyPostClaimTest : public SecurityApplicationProxyPreClaimTest {
  public:
    SecurityApplicationProxyPostClaimTest() : SecurityApplicationProxyPreClaimTest()
    { }

    virtual void SetUp()
    {
        SecurityApplicationProxyPreClaimTest::SetUp();
        ClaimManagedApp();
    }

  private:

    void ClaimManagedApp()
    {
        SecurityApplicationProxyTestHelper::CreateIdentityCertChain(m_securityManagerIdentityCertificate, m_managedAppIdentityCertificate, &m_managedAppIdentityCertificateChain);
        ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_claim(m_managedAppSecurityApplicationProxy,
                                                                m_securityManagerPublicKey,
                                                                m_managedAppIdentityCertificateChain,
                                                                m_adminGroupId,
                                                                GUID128::SIZE,
                                                                m_securityManagerPublicKey,
                                                                (AJ_PCSTR*)m_managedAppSignedManifests,
                                                                1U));
    }
};

class SecurityApplicationProxyFullSetupTest : public SecurityApplicationProxyPostClaimTest {
  public:
    SecurityApplicationProxyFullSetupTest() : SecurityApplicationProxyPostClaimTest(),
        m_oldPolicy(nullptr),
        m_newPolicy(nullptr)
    { }

    virtual void SetUp()
    {
        SecurityApplicationProxyPostClaimTest::SetUp();
        InstallAdminGroupMembership();

        SetUpPolicies();
    }

    virtual void TearDown()
    {
        DestroyStringCopy(m_newPolicy);
        DestroyStringCopy(m_oldPolicy);

        SecurityApplicationProxyPostClaimTest::TearDown();
    }

  protected:

    AJ_PSTR m_oldPolicy;
    AJ_PSTR m_newPolicy;

    void ModifyManagedAppIdentityCertAndManifests()
    {
        SecurityApplicationProxyTestHelper::DestroyCertificate(m_managedAppIdentityCertificate);
        alljoyn_securityapplicationproxy_manifest_destroy(m_managedAppSignedManifests[0]);

        SecurityApplicationProxyTestHelper::CreateIdentityCert(m_securityManager,
                                                               m_managedApp,
                                                               &m_managedAppIdentityCertificate,
                                                               true);

        ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_signmanifest(s_validSecurityManagerManifestTemplate,
                                                                       m_managedAppIdentityCertificate,
                                                                       m_securityManagerPrivateKey,
                                                                       &(m_managedAppSignedManifests[0])));
    }

  private:

    void SetUpPolicies()
    {
        UpdatePolicyWithTrustAnchor(s_validOlderPolicy, &m_oldPolicy);
        UpdatePolicyWithTrustAnchor(s_validNewerPolicy, &m_newPolicy);
    }

    /**
     * A fix for ASACORE-2755
     */
    void UpdatePolicyWithTrustAnchor(AJ_PCSTR originalPolicy, AJ_PSTR* fixedPolicy)
    {
        XmlElement* fixedPolicyXml = nullptr;
        XmlElement* fixXml = nullptr;
        string policyFixTemplate = s_defaultPolicyFixForAsacore2755;

        SecurityApplicationProxyTestHelper::ReplaceString(policyFixTemplate, GROUP_PUBKEY_PLACEHOLDER, m_securityManagerPublicKey);
        SecurityApplicationProxyTestHelper::ReplaceString(policyFixTemplate, GROUP_ID_PLACEHOLDER, m_adminGroupGuid.ToString().c_str());

        ASSERT_EQ(ER_OK, XmlElement::GetRoot(originalPolicy, &fixedPolicyXml));
        ASSERT_EQ(ER_OK, XmlElement::GetRoot(policyFixTemplate.c_str(), &fixXml));

        fixedPolicyXml->GetChildren()[ACLS_INDEX]->AddChild(fixXml);

        *fixedPolicy = CreateStringCopy(fixedPolicyXml->Generate());
        delete fixedPolicyXml;
    }


    void InstallAdminGroupMembership()
    {
        ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_installmembership(m_securityManagerSecurityApplicationProxy, m_adminGroupMembershipCertificate));
        ASSERT_EQ(ER_OK, alljoyn_proxybusobject_secureconnection((alljoyn_proxybusobject)m_managedAppSecurityApplicationProxy, true));
    }
};

bool SecurityApplicationProxyPreProxyTest::s_policyChangeHappened;
bool SecurityApplicationProxyPreProxyTest::s_factoryResetHappened;
bool SecurityApplicationProxyPreProxyTest::s_startManagementHappened;
bool SecurityApplicationProxyPreProxyTest::s_endManagementHappened;


TEST_F(SecurityApplicationProxyPreProxyTest, shouldPassWhenCreatingWithNonExistingRemoteApp)
{
    m_securityManagerSecurityApplicationProxy = alljoyn_securityapplicationproxy_create(m_securityManager, INVALID_BUS_NAME, m_securityManagerSessionId);
    EXPECT_NE(nullptr, m_securityManagerSecurityApplicationProxy);
}

TEST_F(SecurityApplicationProxyPreProxyTest, shouldPassWhenCreatingWithInvalidSessionId)
{
    m_securityManagerSecurityApplicationProxy = alljoyn_securityapplicationproxy_create(m_securityManager, m_securityManagerUniqueName, 0);
    EXPECT_NE(nullptr, m_securityManagerSecurityApplicationProxy);
}

TEST_F(SecurityApplicationProxyPreProxyTest, shouldPassWhenDestroyingNull)
{
    alljoyn_securityapplicationproxy_destroy(nullptr);
}

TEST_F(SecurityApplicationProxyPreProxyTest, shouldPassCreationgWithValidInput)
{
    m_securityManagerSecurityApplicationProxy = alljoyn_securityapplicationproxy_create(m_securityManager, m_securityManagerUniqueName, m_securityManagerSessionId);

    EXPECT_NE(nullptr, m_securityManagerSecurityApplicationProxy);
}

TEST_F(SecurityApplicationProxyPreProxyTest, shouldPassDestroyingValidProxy)
{
    m_securityManagerSecurityApplicationProxy = alljoyn_securityapplicationproxy_create(m_securityManager, m_securityManagerUniqueName, m_securityManagerSessionId);
    ASSERT_NE(nullptr, m_securityManagerSecurityApplicationProxy);

    alljoyn_securityapplicationproxy_destroy(m_securityManagerSecurityApplicationProxy);
    m_securityManagerSecurityApplicationProxy = nullptr;
}

TEST_F(SecurityApplicationProxyPreProxyTest, shouldReturnErrorWhenSigningInvalidManifest)
{
    EXPECT_EQ(ER_XML_MALFORMED, alljoyn_securityapplicationproxy_signmanifest(s_invalidManifest,
                                                                              m_securityManagerIdentityCertificate,
                                                                              m_securityManagerPrivateKey,
                                                                              &m_signedManifestXml));
}

#ifdef NDEBUG
TEST_F(SecurityApplicationProxyPreProxyTest, shouldReturnErrorWhenSigningManifestWithNullCertificate)
{
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_securityapplicationproxy_signmanifest(s_validSecurityManagerManifestTemplate,
                                                                             nullptr,
                                                                             m_securityManagerPrivateKey,
                                                                             &m_signedManifestXml));
}

TEST_F(SecurityApplicationProxyPreProxyTest, shouldReturnErrorWhenSigningManifestWithNullPrivateKey)
{
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_securityapplicationproxy_signmanifest(s_validSecurityManagerManifestTemplate,
                                                                             m_securityManagerIdentityCertificate,
                                                                             nullptr,
                                                                             &m_signedManifestXml));
}

TEST_F(SecurityApplicationProxySelfClaimTest, shouldReturnErrorWhenClaimingWithNullPublicKey)
{
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_securityapplicationproxy_claim(m_securityManagerSecurityApplicationProxy,
                                                                      nullptr,
                                                                      m_securityManagerIdentityCertificate,
                                                                      m_adminGroupId,
                                                                      GUID128::SIZE,
                                                                      m_securityManagerPublicKey,
                                                                      (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                                      1U));
}

TEST_F(SecurityApplicationProxySelfClaimTest, shouldReturnErrorWhenClaimingWithNullCertificate)
{
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_securityapplicationproxy_claim(m_securityManagerSecurityApplicationProxy,
                                                                      m_securityManagerPublicKey,
                                                                      nullptr,
                                                                      m_adminGroupId,
                                                                      GUID128::SIZE,
                                                                      m_securityManagerPublicKey,
                                                                      (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                                      1U));
}

TEST_F(SecurityApplicationProxySelfClaimTest, shouldReturnErrorWhenClaimingWithNullGroupAuthority)
{
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_securityapplicationproxy_claim(m_securityManagerSecurityApplicationProxy,
                                                                      m_securityManagerPublicKey,
                                                                      m_securityManagerIdentityCertificate,
                                                                      m_adminGroupId,
                                                                      GUID128::SIZE,
                                                                      nullptr,
                                                                      (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                                      1U));
}
#endif

TEST_F(SecurityApplicationProxyPreProxyTest, shouldReturnErrorWhenSigningManifestWithPublicKey)
{
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_securityapplicationproxy_signmanifest(s_validSecurityManagerManifestTemplate,
                                                                             m_securityManagerIdentityCertificate,
                                                                             m_securityManagerPublicKey,
                                                                             &m_signedManifestXml));
}

TEST_F(SecurityApplicationProxyPreProxyTest, shouldPassWhenSigningManifestWithValidInput)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_signmanifest(s_validSecurityManagerManifestTemplate,
                                                                   m_securityManagerIdentityCertificate,
                                                                   m_securityManagerPrivateKey,
                                                                   &m_signedManifestXml));
}

TEST_F(SecurityApplicationProxyPreProxyTest, shouldPassWhenDestroyingNullSignedManifest)
{
    alljoyn_securityapplicationproxy_manifesttemplate_destroy(nullptr);
}

TEST_F(SecurityApplicationProxyPreProxyTest, shouldPassWhenDestroyingSignedManifest)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_signmanifest(s_validSecurityManagerManifestTemplate,
                                                                   m_securityManagerIdentityCertificate,
                                                                   m_securityManagerPrivateKey,
                                                                   &m_signedManifestXml));

    alljoyn_securityapplicationproxy_manifesttemplate_destroy(m_signedManifestXml);
    m_signedManifestXml = nullptr;
}

TEST_F(SecurityApplicationProxySelfClaimTest, shouldReturnErrorWhenClaimingWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_claim(m_invalidProxy,
                                                                   m_securityManagerPublicKey,
                                                                   m_securityManagerIdentityCertificate,
                                                                   m_adminGroupId,
                                                                   GUID128::SIZE,
                                                                   m_securityManagerPublicKey,
                                                                   (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                                   1U));
}

TEST_F(SecurityApplicationProxySelfClaimTest, shouldReturnErrorWhenClaimingWithInvalidPublicKey)
{
    AJ_PCSTR invalidPublicKey = m_securityManagerPrivateKey;
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_securityapplicationproxy_claim(m_securityManagerSecurityApplicationProxy,
                                                                      invalidPublicKey,
                                                                      m_securityManagerIdentityCertificate,
                                                                      m_adminGroupId,
                                                                      GUID128::SIZE,
                                                                      m_securityManagerPublicKey,
                                                                      (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                                      1U));
}

TEST_F(SecurityApplicationProxySelfClaimTest, shouldReturnErrorWhenClaimingWithInvalidCertificate)
{
    AJ_PCSTR invalidIdentityCert = m_securityManagerPrivateKey;
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_securityapplicationproxy_claim(m_securityManagerSecurityApplicationProxy,
                                                                      m_securityManagerPublicKey,
                                                                      invalidIdentityCert,
                                                                      m_adminGroupId,
                                                                      GUID128::SIZE,
                                                                      m_securityManagerPublicKey,
                                                                      (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                                      1U));
}

TEST_F(SecurityApplicationProxySelfClaimTest, shouldIgnoreAndPassWhenClaimingWithInvalidGroupId)
{
    uint8_t invalidGroupId[] = { 1 };
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_claim(m_securityManagerSecurityApplicationProxy,
                                                            m_securityManagerPublicKey,
                                                            m_securityManagerIdentityCertificate,
                                                            invalidGroupId,
                                                            GUID128::SIZE,
                                                            m_securityManagerPublicKey,
                                                            (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                            1U));
}

TEST_F(SecurityApplicationProxySelfClaimTest, shouldReturnErrorWhenClaimingWithInvalidGroupIdSize)
{
    size_t invalidGroupIdSize = GUID128::SIZE + 1;
    EXPECT_EQ(ER_INVALID_GUID, alljoyn_securityapplicationproxy_claim(m_securityManagerSecurityApplicationProxy,
                                                                      m_securityManagerPublicKey,
                                                                      m_securityManagerIdentityCertificate,
                                                                      m_adminGroupId,
                                                                      invalidGroupIdSize,
                                                                      m_securityManagerPublicKey,
                                                                      (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                                      1U));
}

TEST_F(SecurityApplicationProxySelfClaimTest, shouldReturnErrorWhenClaimingWithInvalidGroupAuthority)
{
    AJ_PCSTR invalidGroupAuthority = m_securityManagerPrivateKey;
    EXPECT_EQ(ER_INVALID_DATA, alljoyn_securityapplicationproxy_claim(m_securityManagerSecurityApplicationProxy,
                                                                      m_securityManagerPublicKey,
                                                                      m_securityManagerIdentityCertificate,
                                                                      m_adminGroupId,
                                                                      GUID128::SIZE,
                                                                      invalidGroupAuthority,
                                                                      (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                                      1U));
}

TEST_F(SecurityApplicationProxySelfClaimTest, shouldReturnErrorWhenClaimingWithIdentityCertificateThumbprintMismatch)
{
    AJ_PSTR differentIdentityCertificate;
    SecurityApplicationProxyTestHelper::CreateIdentityCert(m_securityManager, m_managedApp, &differentIdentityCertificate);

    EXPECT_EQ(ER_UNKNOWN_CERTIFICATE, alljoyn_securityapplicationproxy_claim(m_securityManagerSecurityApplicationProxy,
                                                                             m_securityManagerPublicKey,
                                                                             differentIdentityCertificate,
                                                                             m_adminGroupId,
                                                                             GUID128::SIZE,
                                                                             m_securityManagerPublicKey,
                                                                             (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                                             1U));

    delete[] differentIdentityCertificate;
}

TEST_F(SecurityApplicationProxySelfClaimTest, shouldPassWhenClaimingWithValidInput)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_claim(m_securityManagerSecurityApplicationProxy,
                                                            m_securityManagerPublicKey,
                                                            m_securityManagerIdentityCertificate,
                                                            m_adminGroupId,
                                                            GUID128::SIZE,
                                                            m_securityManagerPublicKey,
                                                            (AJ_PCSTR*)m_securityManagerSignedManifests,
                                                            1U));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldReturnErrorWhileGettingApplicationStateWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_getapplicationstate(m_invalidProxy, &m_retrievedManagedAppApplicationState));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldPassGetApplicationState)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_getapplicationstate(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppApplicationState));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldGetClaimableApplicationState)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_getapplicationstate(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppApplicationState));

    EXPECT_EQ(CLAIMABLE, m_retrievedManagedAppApplicationState);
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldReturnErrorWhileGettingClaimCapabilitiesWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_getclaimcapabilities(m_invalidProxy, &m_retrievedManagedAppClaimCapabilities));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldPassGetClaimCapabilities)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_getclaimcapabilities(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppClaimCapabilities));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldGetDefaultClaimCapabilities)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_getclaimcapabilities(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppClaimCapabilities));

    EXPECT_EQ(CLAIM_CAPABILITIES_DEFAULT, m_retrievedManagedAppClaimCapabilities);
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldReturnErrorWhileGettingClaimCapabilitiesAdditionalInfoWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_getclaimcapabilitiesadditionalinfo(m_invalidProxy, &m_retrievedManagedAppClaimCapabilitiesAdditionalInfo));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldPassGetClaimCapabilitiesAdditionalInfo)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_getclaimcapabilitiesadditionalinfo(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppClaimCapabilitiesAdditionalInfo));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldGetDefaultClaimCapabilitiesAdditionalInfo)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_getclaimcapabilitiesadditionalinfo(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppClaimCapabilitiesAdditionalInfo));

    EXPECT_EQ(0, m_retrievedManagedAppClaimCapabilitiesAdditionalInfo);
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldReturnErrorWhileGettingManifestTemplateWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_getmanifesttemplate(m_invalidProxy, &m_retrievedManagedAppManifestTemplate));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldPassGetManifestTemplate)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_getmanifesttemplate(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppManifestTemplate));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldGetValidManifestTemplate)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_getmanifesttemplate(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppManifestTemplate));

    EXPECT_STREQ(s_validManagedAppManifestTemplate, RemoveNewLines(m_retrievedManagedAppManifestTemplate).c_str());
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldDestroyNullManifestTemplate)
{
    alljoyn_securityapplicationproxy_manifesttemplate_destroy(nullptr);
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldDestroyRetrievedManifestTemplate)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_getmanifesttemplate(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppManifestTemplate));

    alljoyn_securityapplicationproxy_manifesttemplate_destroy(m_retrievedManagedAppManifestTemplate);
    m_retrievedManagedAppManifestTemplate = nullptr;
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldReturnErrorWhileGettingEccPublicKeyWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_geteccpublickey(m_invalidProxy, &m_retrievedManagedAppEccPublicKey));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldPassGetEccPublicKey)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_geteccpublickey(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppEccPublicKey));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldGetValidEccPublicKey)
{
    ECCPublicKey publicKey;
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_geteccpublickey(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppEccPublicKey));

    EXPECT_EQ(ER_OK, CertificateX509::DecodePublicKeyPEM(m_retrievedManagedAppEccPublicKey, &publicKey));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldDestroyNullEccPublicKey)
{
    alljoyn_securityapplicationproxy_manifesttemplate_destroy(nullptr);
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldDestroyRetrievedEccPublicKey)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_geteccpublickey(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppEccPublicKey));

    alljoyn_securityapplicationproxy_eccpublickey_destroy(m_retrievedManagedAppEccPublicKey);
    m_retrievedManagedAppEccPublicKey = nullptr;
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldPassClaimManagedAppWithIdentityCertChainHavingOneCert)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_claim(m_managedAppSecurityApplicationProxy,
                                                            m_securityManagerPublicKey,
                                                            m_managedAppIdentityCertificate,
                                                            m_adminGroupId,
                                                            GUID128::SIZE,
                                                            m_securityManagerPublicKey,
                                                            (AJ_PCSTR*)m_managedAppSignedManifests,
                                                            1U));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldPassClaimManagedAppWithIdentityCertChainHavingTwoCerts)
{
    SecurityApplicationProxyTestHelper::CreateIdentityCertChain(m_securityManagerIdentityCertificate, m_managedAppIdentityCertificate, &m_managedAppIdentityCertificateChain);

    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_claim(m_managedAppSecurityApplicationProxy,
                                                            m_securityManagerPublicKey,
                                                            m_managedAppIdentityCertificateChain,
                                                            m_adminGroupId,
                                                            GUID128::SIZE,
                                                            m_securityManagerPublicKey,
                                                            (AJ_PCSTR*)m_managedAppSignedManifests,
                                                            1U));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldChangeAppStateAfterClaimingManagedAppWithIdentityCertChainHavingTwoCerts)
{
    SecurityApplicationProxyTestHelper::CreateIdentityCertChain(m_securityManagerIdentityCertificate, m_managedAppIdentityCertificate, &m_managedAppIdentityCertificateChain);
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_claim(m_managedAppSecurityApplicationProxy,
                                                            m_securityManagerPublicKey,
                                                            m_managedAppIdentityCertificateChain,
                                                            m_adminGroupId,
                                                            GUID128::SIZE,
                                                            m_securityManagerPublicKey,
                                                            (AJ_PCSTR*)m_managedAppSignedManifests,
                                                            1U));
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_getapplicationstate(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppApplicationState));

    EXPECT_EQ(CLAIMED, m_retrievedManagedAppApplicationState);
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldCallPolicyChangedCallbackAfterClaim)
{
    s_policyChangeHappened = false;
    SecurityApplicationProxyTestHelper::CreateIdentityCertChain(m_securityManagerIdentityCertificate, m_managedAppIdentityCertificate, &m_managedAppIdentityCertificateChain);
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_claim(m_managedAppSecurityApplicationProxy,
                                                            m_securityManagerPublicKey,
                                                            m_managedAppIdentityCertificateChain,
                                                            m_adminGroupId,
                                                            GUID128::SIZE,
                                                            m_securityManagerPublicKey,
                                                            (AJ_PCSTR*)m_managedAppSignedManifests,
                                                            1U));

    EXPECT_TRUE(WaitForTrueOrTimeout(&s_policyChangeHappened));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldPassInstallMembership)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_installmembership(m_securityManagerSecurityApplicationProxy, m_adminGroupMembershipCertificate));
}

TEST_F(SecurityApplicationProxyPreClaimTest, shouldReturnErrorWhenInstallingSameMembershipTwice)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_installmembership(m_securityManagerSecurityApplicationProxy, m_adminGroupMembershipCertificate));

    EXPECT_EQ(ER_DUPLICATE_CERTIFICATE, alljoyn_securityapplicationproxy_installmembership(m_securityManagerSecurityApplicationProxy, m_adminGroupMembershipCertificate));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnErrorForStartManagementWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_startmanagement(m_invalidProxy));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnErrorForStartManagementForValidProxyButMissingMembership)
{
    EXPECT_EQ(ER_PERMISSION_DENIED, alljoyn_securityapplicationproxy_startmanagement(m_managedAppSecurityApplicationProxy));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnErrorForEndManagementWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_endmanagement(m_invalidProxy));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnErrorForEndManagementCallForValidProxyButMissingMembership)
{
    EXPECT_EQ(ER_PERMISSION_DENIED, alljoyn_securityapplicationproxy_endmanagement(m_managedAppSecurityApplicationProxy));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnErrorForResetWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_reset(m_invalidProxy));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnErrorForResetForValidProxyButMissingMembership)
{
    EXPECT_EQ(ER_PERMISSION_DENIED, alljoyn_securityapplicationproxy_reset(m_managedAppSecurityApplicationProxy));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnPolicyErrorForResetWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_resetpolicy(m_invalidProxy));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnErrorForResetPolicyForValidProxyButMissingMembership)
{
    EXPECT_EQ(ER_PERMISSION_DENIED, alljoyn_securityapplicationproxy_resetpolicy(m_managedAppSecurityApplicationProxy));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnErrorForUpdatePolicyWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_updatepolicy(m_invalidProxy, s_validNewerPolicy));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnErrorForUpdatePolicyForValidProxyButMissingMembership)
{
    EXPECT_EQ(ER_PERMISSION_DENIED, alljoyn_securityapplicationproxy_updatepolicy(m_managedAppSecurityApplicationProxy, s_validNewerPolicy));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnErrorForUpdateIdentityWithInvalidProxy)
{
    EXPECT_EQ(ER_AUTH_FAIL, alljoyn_securityapplicationproxy_updateidentity(m_invalidProxy,
                                                                            m_managedAppIdentityCertificateChain,
                                                                            (AJ_PCSTR*)m_managedAppSignedManifests,
                                                                            1U));
}

TEST_F(SecurityApplicationProxyPostClaimTest, shouldReturnErrorForUpdateIdentityForValidProxyButMissingMembership)
{
    EXPECT_EQ(ER_PERMISSION_DENIED, alljoyn_securityapplicationproxy_updateidentity(m_managedAppSecurityApplicationProxy,
                                                                                    m_managedAppIdentityCertificateChain,
                                                                                    (AJ_PCSTR*)m_managedAppSignedManifests,
                                                                                    1U));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldPassStartManagementCallForValidProxyAndInstalledMembership)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_startmanagement(m_managedAppSecurityApplicationProxy));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldReturnErrorWhenCallingStartManagementTwice)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_startmanagement(m_managedAppSecurityApplicationProxy));

    EXPECT_EQ(ER_MANAGEMENT_ALREADY_STARTED, alljoyn_securityapplicationproxy_startmanagement(m_managedAppSecurityApplicationProxy));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldCallStartManagementCallbackAfterStartManagement)
{
    s_startManagementHappened = false;
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_startmanagement(m_managedAppSecurityApplicationProxy));

    EXPECT_TRUE(WaitForTrueOrTimeout(&s_startManagementHappened));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldReturnErrorWhenCallingEndManagementBeforeStart)
{
    EXPECT_EQ(ER_MANAGEMENT_NOT_STARTED, alljoyn_securityapplicationproxy_endmanagement(m_managedAppSecurityApplicationProxy));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldPassEndManagementCallForValidProxyAndInstalledMembership)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_startmanagement(m_managedAppSecurityApplicationProxy));

    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_endmanagement(m_managedAppSecurityApplicationProxy));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldCallEndManagementCallbackAfterEndManagement)
{
    s_endManagementHappened = false;
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_startmanagement(m_managedAppSecurityApplicationProxy));
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_endmanagement(m_managedAppSecurityApplicationProxy));

    EXPECT_TRUE(WaitForTrueOrTimeout(&s_endManagementHappened));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldPassStartManagementAfterEnd)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_startmanagement(m_managedAppSecurityApplicationProxy));
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_endmanagement(m_managedAppSecurityApplicationProxy));

    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_startmanagement(m_managedAppSecurityApplicationProxy));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldPassUpdatePolicyForValidProxyAndInstalledMembership)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_updatepolicy(m_managedAppSecurityApplicationProxy, s_validNewerPolicy));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldCallPolicyChangedCallbackAfterPolicyUpdate)
{
    s_policyChangeHappened = false;
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_updatepolicy(m_managedAppSecurityApplicationProxy, s_validNewerPolicy));

    EXPECT_TRUE(WaitForTrueOrTimeout(&s_policyChangeHappened));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldReturnErrorWhenUpdatingPolicyWithNotNewerNumber)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_updatepolicy(m_managedAppSecurityApplicationProxy, m_newPolicy));
    ASSERT_EQ(ER_OK, alljoyn_proxybusobject_secureconnection((alljoyn_proxybusobject)m_managedAppSecurityApplicationProxy, true));

    EXPECT_EQ(ER_POLICY_NOT_NEWER, alljoyn_securityapplicationproxy_updatepolicy(m_managedAppSecurityApplicationProxy, m_oldPolicy));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldPassWhenUpdatingPolicyWithNewerNumber)
{
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_updatepolicy(m_managedAppSecurityApplicationProxy, m_oldPolicy));
    ASSERT_EQ(ER_OK, alljoyn_proxybusobject_secureconnection((alljoyn_proxybusobject)m_managedAppSecurityApplicationProxy, true));

    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_updatepolicy(m_managedAppSecurityApplicationProxy, m_newPolicy));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldPassUpdateIdentityForValidProxyAndInstalledMembership)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_updateidentity(m_managedAppSecurityApplicationProxy,
                                                                     m_managedAppIdentityCertificate,
                                                                     (AJ_PCSTR*)m_managedAppSignedManifests,
                                                                     1U));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldNotCallPolicyChangedCallbackAfterIdentityCertUpdate)
{
    ModifyManagedAppIdentityCertAndManifests();
    s_policyChangeHappened = false;
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_updateidentity(m_managedAppSecurityApplicationProxy,
                                                                     m_managedAppIdentityCertificate,
                                                                     (AJ_PCSTR*)m_managedAppSignedManifests,
                                                                     1U));

    EXPECT_FALSE(WaitForTrueOrTimeout(&s_policyChangeHappened));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldPassResetForValidProxyAndInstalledMembership)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_reset(m_managedAppSecurityApplicationProxy));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldCallResetCallbackAfterReset)
{
    s_factoryResetHappened = false;
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_reset(m_managedAppSecurityApplicationProxy));

    EXPECT_TRUE(WaitForTrueOrTimeout(&s_factoryResetHappened));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldResetAppToClaimableStateAfterReset)
{
    s_factoryResetHappened = false;
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_reset(m_managedAppSecurityApplicationProxy));
    ASSERT_TRUE(WaitForTrueOrTimeout(&s_factoryResetHappened));
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_getapplicationstate(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppApplicationState));

    EXPECT_EQ(CLAIMABLE, m_retrievedManagedAppApplicationState);
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldPassResetPolicyForValidProxyAndInstalledMembership)
{
    EXPECT_EQ(ER_OK, alljoyn_securityapplicationproxy_resetpolicy(m_managedAppSecurityApplicationProxy));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldCallResetPolicyCallbackAfterReset)
{
    s_policyChangeHappened = false;
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_resetpolicy(m_managedAppSecurityApplicationProxy));

    EXPECT_TRUE(WaitForTrueOrTimeout(&s_policyChangeHappened));
}

TEST_F(SecurityApplicationProxyFullSetupTest, shouldNotResetAppToClaimableStateAfterResetPolicy)
{
    s_policyChangeHappened = false;
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_resetpolicy(m_managedAppSecurityApplicationProxy));
    ASSERT_TRUE(WaitForTrueOrTimeout(&s_policyChangeHappened));
    ASSERT_EQ(ER_OK, alljoyn_securityapplicationproxy_getapplicationstate(m_managedAppSecurityApplicationProxy, &m_retrievedManagedAppApplicationState));

    EXPECT_EQ(CLAIMED, m_retrievedManagedAppApplicationState);
}