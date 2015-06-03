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

#ifndef TESTUTIL_H_
#define TESTUTIL_H_

#include "gtest/gtest.h"

#include <alljoyn/securitymgr/ApplicationInfo.h>
#include <alljoyn/securitymgr/SecurityManagerFactory.h>
#include <alljoyn/securitymgr/SyncError.h>
#include <alljoyn/securitymgr/sqlstorage/SQLStorageFactory.h>

#include <PermissionMgmt.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Condition.h>
#include "Stub.h"

using namespace ajn::securitymgr;
using namespace std;
namespace secmgrcoretest_unit_testutil {
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
        QCC_UNUSED(version);
        QCC_UNUSED(port);
        QCC_UNUSED(objectDescriptionArg);
        QCC_UNUSED(aboutDataArg);

        printf("Found About data @%s\n", busName);
    }
};

class TestApplicationListener :
    public ApplicationListener {
  public:
    TestApplicationListener(qcc::Condition& _sem,
                            qcc::Mutex& _lock);

    ApplicationInfo _lastAppInfo;
    bool event;

  private:
    qcc::Condition& sem;
    qcc::Mutex& lock;

    void OnApplicationStateChange(const ApplicationInfo* old,
                                  const ApplicationInfo* updated);

    void OnSyncError(const SyncError* syncError)
    {
        QCC_UNUSED(syncError);
    }

    /* To avoid compilation warning that the assignment operator can not be generated */
    TestApplicationListener& operator=(const TestApplicationListener&);
};

class AutoAccepter :
    public ManifestListener {
    bool ApproveManifest(const ApplicationInfo& appInfo,
                         const PermissionPolicy::Rule* manifestRules,
                         const size_t manifestRulesCount)
    {
        QCC_UNUSED(appInfo);
        QCC_UNUSED(manifestRules);
        QCC_UNUSED(manifestRulesCount);

        return true;
    }
};

class BasicTest :
    public::testing::Test {
  private:
    void UpdateLastAppInfo();

  protected:
    qcc::Condition sem;
    qcc::Mutex lock;
    TestApplicationListener* tal;
    TestAboutListener testAboutListener;
    Stub* stub;
    virtual void SetUp();

    virtual void TearDown();

  public:

    SecurityManager* secMgr;
    BusAttachment* ba;
    Storage* storage;
    ApplicationInfo lastAppInfo;
    AutoAccepter aa;

    BasicTest();

    /* When updatesPending is -1, we will just ignore checking the updatesPending flag,
     * otherwise it acts as a bool that needs to be checked/waited-for akin to states.
     * */
    bool WaitForState(ajn::PermissionConfigurator::ClaimableState newClaimState,
                      ajn::securitymgr::ApplicationRunningState newRunningState,
                      const int updatesPending = -1);
};

class TestClaimListener :
    public ClaimListener {
  public:
    TestClaimListener(bool& _claimAnswer) :
        claimAnswer(_claimAnswer)
    {
    }

  private:
    bool& claimAnswer;

    bool OnClaimRequest(const qcc::ECCPublicKey* pubKeyRot, void* ctx)
    {
        QCC_UNUSED(pubKeyRot);
        QCC_UNUSED(ctx);

        return claimAnswer;
    }

    /* This function is called when the claiming process has completed successfully */
    void OnClaimed(void* ctx)
    {
        QCC_UNUSED(ctx);
    }

    /* To avoid compilation warning that the assignment operator can not be generated */
    TestClaimListener& operator=(const TestClaimListener&);
};

class ClaimedTest :
    public BasicTest {
  public:

    Stub* stub;
    IdentityInfo idInfo;
    TestClaimListener* tcl;

    void SetUp()
    {
        BasicTest::SetUp();

        bool claimAnswer = true;
        tcl = new TestClaimListener(claimAnswer);
        stub = new Stub(tcl);
        /* Open claim window */
        stub->OpenClaimWindow();
        ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));
        /* Claim ! */
        idInfo.guid = lastAppInfo.peerID;
        idInfo.name = "MyTest ID Name";
        secMgr->StoreIdentity(idInfo);
        secMgr->Claim(lastAppInfo, idInfo);
        ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING));
        stub->SetDSASecurity(true);
        ASSERT_EQ(ER_OK, secMgr->GetApplication(lastAppInfo));
    }

    void TearDown()
    {
        BasicTest::TearDown();

        if (stub) {
            destroy();
        }
        delete tcl;
        tcl = NULL;
    }

    void destroy()
    {
        delete stub;
        stub = NULL;
    }

    ClaimedTest() :
        stub(NULL), tcl(NULL)
    {
    }
};
}
#endif /* TESTUTIL_H_ */
