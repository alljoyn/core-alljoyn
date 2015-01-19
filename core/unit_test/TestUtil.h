/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <SecurityManagerFactory.h>
#include <PermissionMgmt.h>
#include <qcc/String.h>

#include <semaphore.h>
#include "Stub.h"

using namespace ajn::securitymgr;
using namespace std;
namespace secmgrcoretest_unit_testutil {
class TestApplicationListener :
    public ApplicationListener {
  public:
    TestApplicationListener(sem_t& _sem,
                            sem_t& _lock);

    ApplicationInfo _lastAppInfo;

  private:
    sem_t& sem;
    sem_t& lock;

    void OnApplicationStateChange(const ApplicationInfo* old,
                                  const ApplicationInfo* updated);
};

class BasicTest :
    public::testing::Test {
  private:
    TestApplicationListener* tal;

    void UpdateLastAppInfo();

  protected:
    sem_t sem;
    sem_t lock;

    virtual void SetUp();

    virtual void TearDown();

  public:

    ajn::securitymgr::SecurityManager* secMgr;
    ajn::securitymgr::StorageConfig sc;
    ajn::securitymgr::SecurityManagerConfig smc;
    ajn::BusAttachment* ba;

    ApplicationInfo lastAppInfo;

    BasicTest();
    void SetSmcStub();

    bool WaitForState(ajn::PermissionConfigurator::ClaimableState newClaimState,
                      ajn::securitymgr::ApplicationRunningState newRunningState);
};

class ClaimTest :
    public BasicTest {
  protected:

    static bool AutoAcceptManifest(const ApplicationInfo& appInfo,
                                   const PermissionPolicy::Rule* manifestRules,
                                   const size_t manifestRulesCount,
                                   void* cookie)
    {
        return true;
    }
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
        return claimAnswer;
    }

    /* This function is called when the claiming process has completed successfully */
    void OnClaimed(void* ctx)
    {
    }
};

class ClaimedTest :
    public ClaimTest {
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
        secMgr->ClaimApplication(lastAppInfo, idInfo, &AutoAcceptManifest);
        ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING));
        secMgr->GetApplication(lastAppInfo);
    }

    void TearDown()
    {
        if (stub) {
            destroy();
        }

        BasicTest::TearDown();

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
        SetSmcStub();
    }
};
}
#endif /* TESTUTIL_H_ */
