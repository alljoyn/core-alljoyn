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
#define TEST_STORAGE_NAME "testcakeystore"

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
    void UpdateLastAppInfo();

  protected:
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

  public:

    shared_ptr<SecurityAgent> secMgr;
    BusAttachment* ba;
    shared_ptr<UIStorage> storage;
    shared_ptr<AgentCAStorage> ca;
    OnlineApplication lastAppInfo;
    AutoAccepter aa;
    PolicyGenerator* pg;
    ProxyObjectManager* proxyObjectManager;
    Mutex secAgentLock;

    BasicTest();

    bool WaitForState(PermissionConfigurator::ApplicationState newApplicationState,
                      ApplicationSyncState updateState = SYNC_UNKNOWN);

    bool CheckRemotePolicy(PermissionPolicy& expectedPolicy);

    bool CheckStoredPolicy(PermissionPolicy& expectedPolicy);

    bool CheckPolicy(PermissionPolicy& expectedPolicy);

    bool CheckDefaultPolicy();

    bool CheckRemoteIdentity(IdentityInfo& expectedIdentity,
                             Manifest& expectedManifest,
                             IdentityCertificate& remoteIdentity,
                             Manifest& remoteManifest);

    bool CheckIdentity(IdentityInfo& expectedIdentity,
                       Manifest& expectedManifest);

    bool CheckMemberships(vector<GroupInfo> expectedGroups);

    bool CheckSyncState(ApplicationSyncState updateState);

    bool WaitForUpdatesCompleted();

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

    void SetUp()
    {
        SecurityAgentTest::SetUp();

        idInfo.guid = GUID128(0xEF);
        idInfo.name = "MyTest ID Name";
        storage->StoreIdentity(idInfo);

        ASSERT_EQ(ER_OK, testApp.Start());
        ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE));
        secMgr->Claim(lastAppInfo, idInfo);
        ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED));
        ASSERT_EQ(ER_OK, secMgr->GetApplication(lastAppInfo));
        ASSERT_TRUE(CheckIdentity(idInfo, aa.lastManifest));
    }
};
}
#endif /* ALLJOYN_SECMGR_TESTUTIL_H_ */
