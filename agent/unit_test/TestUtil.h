/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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
                            Mutex& _lock);

    vector<OnlineApplication> events;

  private:
    Condition& sem;
    Mutex& lock;

    void OnApplicationStateChange(const OnlineApplication* old,
                                  const OnlineApplication* updated);

    void OnSyncError(const SyncError* syncError)
    {
        QCC_UNUSED(syncError);
    }

    /* To avoid compilation warning that the assignment operator can not be generated */
    TestApplicationListener& operator=(const TestApplicationListener&);
};

class AutoAccepter :
    public ManifestListener {
    bool ApproveManifest(const OnlineApplication& app,
                         const Manifest& manifest)
    {
        QCC_UNUSED(app);

        lastManifest = manifest;

        return true;
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

    BasicTest();

    /* When updatesPending is -1, we will just ignore checking the updatesPending flag,
     * otherwise it acts as a bool that needs to be checked/waited-for akin to states.
     * hasBusName is true if the application is expected to have a busname, i.e., online.
     * */
    bool WaitForState(PermissionConfigurator::ApplicationState newApplicationState,
                      const bool hasBusName,
                      const int updatesPending = -1);

    bool CheckRemotePolicy(PermissionPolicy& expectedPolicy);

    bool CheckStoredPolicy(PermissionPolicy& expectedPolicy);

    bool CheckPolicy(PermissionPolicy& expectedPolicy);

    bool CheckDefaultPolicy();

    bool CheckIdentity(IdentityInfo& expectedIdentity,
                       Manifest& expectedManifest);

    bool CheckMemberships(vector<GroupInfo> expectedGroups);

    bool CheckUpdatesPending(bool expected);

    bool WaitForUpdatesCompleted();

    virtual shared_ptr<AgentCAStorage>& GetAgentCAStorage()
    {
        return ca;
    }
};

class ClaimedTest :
    public BasicTest {
  public:

    IdentityInfo idInfo;
    TestApplication testApp;

    void SetUp()
    {
        BasicTest::SetUp();

        idInfo.guid = GUID128(0xEF);
        idInfo.name = "MyTest ID Name";
        storage->StoreIdentity(idInfo);

        ASSERT_EQ(ER_OK, testApp.Start());
        ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
        secMgr->Claim(lastAppInfo, idInfo);
        ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
        ASSERT_EQ(ER_OK, secMgr->GetApplication(lastAppInfo));
    }
};

class DefaultAgentStorageWrapper :
    public AgentCAStorage {
  public:
    DefaultAgentStorageWrapper(shared_ptr<AgentCAStorage>& _ca) : ca(_ca) { }

    virtual ~DefaultAgentStorageWrapper() { }

    virtual QStatus GetManagedApplication(Application& app) const
    {
        return ca->GetManagedApplication(app);
    }

    virtual QStatus RegisterAgent(const KeyInfoNISTP256& agentKey,
                                  const Manifest& manifest,
                                  GroupInfo& adminGroup,
                                  IdentityCertificateChain& identityCertificates,
                                  vector<MembershipCertificateChain>& adminGroupMemberships)
    {
        return ca->RegisterAgent(agentKey, manifest, adminGroup, identityCertificates, adminGroupMemberships);
    }

    virtual QStatus StartApplicationClaiming(const Application& app,
                                             const IdentityInfo& idInfo,
                                             const Manifest& manifest,
                                             GroupInfo& adminGroup,
                                             IdentityCertificate& identityCertificate)
    {
        return ca->StartApplicationClaiming(app, idInfo, manifest, adminGroup, identityCertificate);
    }

    virtual QStatus FinishApplicationClaiming(const Application& app,
                                              bool status)
    {
        return ca->FinishApplicationClaiming(app, status);
    }

    virtual QStatus UpdatesCompleted(Application& app, uint64_t& updateID)
    {
        return ca->UpdatesCompleted(app, updateID);
    }

    virtual QStatus StartUpdates(Application& app, uint64_t& updateID)
    {
        return ca->StartUpdates(app, updateID);
    }

    virtual QStatus GetCaPublicKeyInfo(KeyInfoNISTP256& keyInfoOfCA) const
    {
        return ca->GetCaPublicKeyInfo(keyInfoOfCA);
    }

    virtual QStatus GetMembershipCertificates(const Application& app,
                                              MembershipCertificateChain& membershipCertificates) const
    {
        return ca->GetMembershipCertificates(app, membershipCertificates);
    }

    virtual QStatus GetIdentityCertificatesAndManifest(const Application& app,
                                                       IdentityCertificateChain& identityCertificates,
                                                       Manifest& manifest) const
    {
        return ca->GetIdentityCertificatesAndManifest(app, identityCertificates, manifest);
    }

    virtual QStatus GetPolicy(const Application& app, PermissionPolicy& policy) const
    {
        return ca->GetPolicy(app, policy);
    }

    virtual void RegisterStorageListener(StorageListener* listener)
    {
        return ca->RegisterStorageListener(listener);
    }

    virtual void UnRegisterStorageListener(StorageListener* listener)
    {
        return ca->UnRegisterStorageListener(listener);
    }

  protected:
    shared_ptr<AgentCAStorage> ca;
};
}
#endif /* ALLJOYN_SECMGR_TESTUTIL_H_ */
