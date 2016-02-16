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

#ifndef ALLJOYN_SECMGR_TESTUTIL_H_
#define ALLJOYN_SECMGR_TESTUTIL_H_

#include <gtest/gtest.h>

#include <memory>

#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Condition.h>

#include <ProxyObjectManager.h>

#include <alljoyn/securitymgr/Application.h>
#include <alljoyn/securitymgr/SecurityAgentFactory.h>
#include <alljoyn/securitymgr/SyncError.h>
#include <alljoyn/securitymgr/storage/StorageFactory.h>
#include <alljoyn/securitymgr/PolicyGenerator.h>

#include "TestApplication.h"

using namespace ajn;
using namespace ajn::securitymgr;
using namespace std;

/** @file TestUtil.h */

namespace secmgr_tests {
#define TEST_STORAGE_NAME "testCaKeystore"

class TestSessionListener :
    public SessionListener {
    void SessionLost(SessionId sessionId, SessionLostReason reason)
    {
        printf("SessionLost sessionId = %u, Reason = %d\n", sessionId, reason);
    }
};

class TestAboutListener :
    public AboutListener {
    virtual void Announced(const char* busName,
                           uint16_t version,
                           SessionPort port,
                           const MsgArg& objectDescriptionArg,
                           const MsgArg& aboutDataArg)
    {
        QCC_UNUSED(busName);
        QCC_UNUSED(version);
        QCC_UNUSED(port);
        QCC_UNUSED(objectDescriptionArg);
        QCC_UNUSED(aboutDataArg);
    }
};

class TestApplicationListener :
    public ApplicationListener {
  public:
    TestApplicationListener(Condition& _sem,
                            Mutex& _lock,
                            Condition& _errorSem,
                            Mutex& _errorLock,
                            Condition& _manifestSem,
                            Mutex& _manifestLock);

    vector<OnlineApplication> events;
    vector<SyncError> syncErrors;
    vector<ManifestUpdate> manifestUpdates;

  private:
    Condition& sem;
    Mutex& lock;
    Condition& errorSem;
    Mutex& errorLock;
    Condition& manifestSem;
    Mutex& manifestLock;

    void OnApplicationStateChange(const OnlineApplication* old,
                                  const OnlineApplication* updated);

    void OnSyncError(const SyncError* syncError);

    void OnManifestUpdate(const ManifestUpdate* manifestUpdate);

    /* To avoid compilation warning that the assignment operator can not be generated */
    TestApplicationListener& operator=(const TestApplicationListener&);
};

class AutoAccepter :
    public ClaimListener {
    QStatus ApproveManifestAndSelectSessionType(ClaimContext& ctx)
    {
        ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_NULL);
        ctx.ApproveManifest();
        lastManifest = ctx.GetManifest();

        return ER_OK;
    }

  public:
    Manifest lastManifest;
};

class BasicTest :
    public::testing::Test {
  private:
    bool UpdateLastAppInfo(OnlineApplication& app,
                           const OnlineApplication& needed,
                           bool useBusName);

    BusAttachment* ownBus;

  protected:
    shared_ptr<ProxyObjectManager> proxyObjectManager;
    Condition sem;
    Mutex lock;
    Condition errorSem;
    Mutex errorLock;
    Condition manifestSem;
    Mutex manifestLock;
    TestApplicationListener* tal;
    TestAboutListener testAboutListener;
    virtual void SetUp();

    virtual void TearDown();

    QStatus CreateProxyObjectManager();

  public:

    shared_ptr<SecurityAgent> secMgr;
    BusAttachment* ba;
    shared_ptr<UIStorage> storage;
    shared_ptr<AgentCAStorage> ca;

    AutoAccepter aa;
    PolicyGenerator* pg;
    Mutex secAgentLock;

    BasicTest();

    QStatus GetPublicKey(const TestApplication& app,
                         OnlineApplication& appInfo);

    bool CheckRemotePolicy(const OnlineApplication& app,
                           PermissionPolicy& expectedPolicy);

    bool CheckStoredPolicy(const OnlineApplication& app,
                           PermissionPolicy& expectedPolicy);

    bool CheckPolicy(const OnlineApplication& app,
                     PermissionPolicy& expectedPolicy);

    bool CheckDefaultPolicy(const OnlineApplication& app);

    bool CheckRemoteIdentity(const OnlineApplication& app,
                             IdentityInfo& expectedIdentity,
                             Manifest& expectedManifest,
                             IdentityCertificate& remoteIdentity,
                             Manifest& remoteManifest);

    bool CheckIdentity(const OnlineApplication& app,
                       IdentityInfo& expectedIdentity,
                       Manifest& expectedManifest);

    bool CheckMemberships(const OnlineApplication& app,
                          vector<GroupInfo> expectedGroups);

    bool CheckSyncState(const OnlineApplication& app,
                        ApplicationSyncState updateState);

    bool WaitForState(const OnlineApplication& app,
                      PermissionConfigurator::ApplicationState newApplicationState,
                      ApplicationSyncState updateState = SYNC_UNKNOWN,
                      bool useBusName = false);

    bool WaitForUpdatesCompleted(const OnlineApplication& app);

    bool WaitForSyncError(SyncErrorType type,
                          QStatus status);

    bool WaitForEvents(size_t numOfEvents);

    bool CheckUnexpectedSyncErrors();

    bool WaitForManifestUpdate(ManifestUpdate& manifestUpdate);

    bool CheckUnexpectedManifestUpdates();

    QStatus Reset(const OnlineApplication& app);

    QStatus GetMembershipSummaries(const OnlineApplication& app,
                                   vector<MembershipSummary>& summaries);

    QStatus GetPolicyVersion(const OnlineApplication& app,
                             uint32_t& version);

    QStatus GetIdentity(const OnlineApplication& app,
                        IdentityCertificateChain& idCertChain);

    QStatus GetClaimCapabilities(const OnlineApplication& app,
                                 PermissionConfigurator::ClaimCapabilities& claimCaps,
                                 PermissionConfigurator::ClaimCapabilityAdditionalInfo& claimCapInfo);

    virtual shared_ptr<AgentCAStorage>& GetAgentCAStorage()
    {
        return ca;
    }

    void InitSecAgent();

    void RemoveSecAgent();
};

class SecurityAgentTest :
    public BasicTest {
  public:

    void SetUp()
    {
        BasicTest::SetUp();
        InitSecAgent();
    }
};

class ClaimedTest :
    public SecurityAgentTest {
  public:

    IdentityInfo idInfo;
    TestApplication testApp;
    OnlineApplication testAppInfo;

    void SetUp()
    {
        SecurityAgentTest::SetUp();

        ASSERT_EQ(ER_OK, CreateProxyObjectManager());

        idInfo.guid = GUID128(0xEF);
        idInfo.name = "MyTest ID Name";
        storage->StoreIdentity(idInfo);

        ASSERT_EQ(ER_OK, testApp.Start());
        ASSERT_EQ(ER_OK, BasicTest::GetPublicKey(testApp, testAppInfo));
        ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE));
        ASSERT_EQ(ER_OK, secMgr->Claim(testAppInfo, idInfo));
        ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED));
        ASSERT_EQ(ER_OK, secMgr->GetApplication(testAppInfo));
        ASSERT_TRUE(CheckIdentity(idInfo, aa.lastManifest));
    }

    bool CheckRemotePolicy(PermissionPolicy& expectedPolicy)
    {
        UpdateBusname();
        return BasicTest::CheckRemotePolicy(testAppInfo, expectedPolicy);
    }

    bool CheckStoredPolicy(PermissionPolicy& expectedPolicy)
    {
        UpdateBusname();
        return BasicTest::CheckStoredPolicy(testAppInfo, expectedPolicy);
    }

    bool CheckPolicy(PermissionPolicy& expectedPolicy)
    {
        UpdateBusname();
        return BasicTest::CheckPolicy(testAppInfo, expectedPolicy);
    }

    bool CheckDefaultPolicy()
    {
        UpdateBusname();
        return BasicTest::CheckDefaultPolicy(testAppInfo);
    }

    bool CheckRemoteIdentity(IdentityInfo& expectedIdentity,
                             Manifest& expectedManifest,
                             IdentityCertificate& remoteIdentity,
                             Manifest& remoteManifest)
    {
        UpdateBusname();
        return BasicTest::CheckRemoteIdentity(testAppInfo,
                                              expectedIdentity,
                                              expectedManifest,
                                              remoteIdentity,
                                              remoteManifest);
    }

    bool CheckIdentity(IdentityInfo& expectedIdentity,
                       Manifest& expectedManifest)
    {
        UpdateBusname();
        return BasicTest::CheckIdentity(testAppInfo, expectedIdentity, expectedManifest);
    }

    bool CheckMemberships(vector<GroupInfo> expectedGroups)
    {
        UpdateBusname();
        return BasicTest::CheckMemberships(testAppInfo, expectedGroups);
    }

    bool CheckSyncState(ApplicationSyncState updateState)
    {
        UpdateBusname();
        return BasicTest::CheckSyncState(testAppInfo, updateState);
    }

    bool WaitForState(PermissionConfigurator::ApplicationState newApplicationState,
                      ApplicationSyncState updateState = SYNC_UNKNOWN)
    {
        UpdateBusname();
        return BasicTest::WaitForState(testAppInfo, newApplicationState, updateState);
    }

    bool WaitForUpdatesCompleted()
    {
        UpdateBusname();
        return BasicTest::WaitForUpdatesCompleted(testAppInfo);
    }

    void UpdateBusname()
    {
        testAppInfo.busName = testApp.GetBusName();
    }
};
}
#endif /* ALLJOYN_SECMGR_TESTUTIL_H_ */
