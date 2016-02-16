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

#include "TestUtil.h"
#include "AgentStorageWrapper.h"
#include <qcc/Thread.h>

using namespace ajn::securitymgr;

/** @file ClaimingTests.cc */

namespace secmgr_tests {
class AutoRejector :
    public ClaimListener {
    QStatus ApproveManifestAndSelectSessionType(ClaimContext& ctx)
    {
        ctx.ApproveManifest(false);
        return ER_OK;
    }
};

class RejectAfterAcceptListener :
    public ClaimListener {
  public:
    RejectAfterAcceptListener(const shared_ptr<SecurityAgent>& _secMgr) : secMgr(_secMgr)
    {
    }

    QStatus ApproveManifestAndSelectSessionType(ClaimContext& ctx)
    {
        ctx.ApproveManifest();
        ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_NULL);
        secMgr->SetClaimListener(&ar);

        return ER_OK;
    }

  private:
    AutoRejector ar;
    shared_ptr<SecurityAgent> secMgr;
};

class StopBeforeAcceptListener :
    public ClaimListener {
  public:
    StopBeforeAcceptListener(TestApplication& _testApp) : testApp(_testApp)
    {
    }

    QStatus ApproveManifestAndSelectSessionType(ClaimContext& ctx)
    {
        ctx.ApproveManifest();
        ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_NULL);

        testApp.Stop();

        return ER_OK;
    }

  private:
    StopBeforeAcceptListener& operator=(const StopBeforeAcceptListener);

    TestApplication& testApp;
};

class ClaimingTests :
    public SecurityAgentTest {
  public:
    shared_ptr<AgentCAStorage>& GetAgentCAStorage()
    {
        wrappedCA = shared_ptr<FailingStorageWrapper>(new FailingStorageWrapper(ca, storage));
        ca = wrappedCA;
        return ca;
    }

  public:
    shared_ptr<FailingStorageWrapper> wrappedCA;
};

/**
 * @test Claim an application and check that it becomes CLAIMED.
 *       -# Start the application.
 *       -# Make sure the application is in a CLAIMABLE state.
 *       -# Claim the application.
 *       -# Accept the manifest of the application.
 *       -# Check whether the application becomes CLAIMED.
 **/
TEST_F(ClaimingTests, SuccessfulClaim) {
    /* Start the application */
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    /* Create identity */
    IdentityInfo idInfo;
    idInfo.guid = GUID128("abcdef123456789");
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED));
    ASSERT_TRUE(CheckIdentity(app, idInfo, aa.lastManifest));

    ASSERT_EQ(ER_OK, storage->GetManagedApplication(app));

    /* Try to claim again */
    ASSERT_NE(ER_OK, secMgr->Claim(app, idInfo));
}

/**
 * @test Reject the manifest during claiming and check whether the application
 *       becomes CLAIMABLE again.
 *       -# Claim the remote application.
 *       -# Reject the manifest.
 *       -# Check whether the agent returns an ER_MANIFEST_REJECTED error.
 *       -# Check whether the application remains CLAIMABLE.
 **/

TEST_F(ClaimingTests, RejectManifest) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));

    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    AutoRejector ar;
    secMgr->SetClaimListener(&ar);

    ASSERT_EQ(ER_MANIFEST_REJECTED, secMgr->Claim(app, idInfo));
    secMgr->SetClaimListener(nullptr);
}

/**
 * @test Basic robustness tests for the claiming, including input validation,
 *       unavailability of manifest listener/CA.
 *      -# Claiming an off-line/unknown application should fail.
 *      -# Claiming using an unknown identity should fail.
 *      -# Claiming an application that is NOT_CLAIMABALE should fail.
 *      -# Claiming an application that is CLAIMED should fail.
 *      -# Claiming an application that NEED_UPDATE should fail.
 *
 *      -# Claiming when no ClaimListener is set should fail.
 */
TEST_F(ClaimingTests, BasicRobustness) {
    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "StoredTestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);
    OnlineApplication app;
    ASSERT_EQ(ER_FAIL, secMgr->Claim(app, idInfo));     // No test app exists (or offline)

    TestApplication testApp;
    IdentityInfo inexistentIdInfo;
    inexistentIdInfo.guid = GUID128();
    inexistentIdInfo.name = "InexistentTestIdentity";
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
    ASSERT_EQ(ER_FAIL, secMgr->Claim(app, inexistentIdInfo)); // Claim a claimable app with inexistent ID

    testApp.SetApplicationState(PermissionConfigurator::NOT_CLAIMABLE);
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::NOT_CLAIMABLE));
    ASSERT_NE(ER_OK, secMgr->Claim(app, idInfo));

    testApp.SetApplicationState(PermissionConfigurator::CLAIMED);
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED, SYNC_UNMANAGED));
    ASSERT_NE(ER_OK, secMgr->Claim(app, idInfo));

    testApp.SetApplicationState(PermissionConfigurator::NEED_UPDATE);
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::NEED_UPDATE, SYNC_UNMANAGED));
    ASSERT_NE(ER_OK, secMgr->Claim(app, idInfo));

    secMgr->SetClaimListener(nullptr);
    testApp.SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
    ASSERT_EQ(ER_FAIL, secMgr->Claim(app, idInfo)); // Secmgr has no mft listener

    ASSERT_EQ(ER_OK, testApp.Stop());
    testApp.Reset();
}

/**
 * @test Recovery from failure of notifying the CA of failure of claiming an
 *       application should be graceful.
 *       -# Start a test application.
 *       -# Install a manifest listener that stops the application before
 *          accepting the manifest, which will make the claiming fail.
 *       -# Make sure the UpdatesCompleted to storage fails.
 *       -# Claim the application and check that this fails.
 *       -# Wait for a sync error for the application.
 */
TEST_F(ClaimingTests, RecoveryFromClaimingFailure) {
    // create and store identity
    IdentityInfo idInfo;
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    // start and claim test app
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    // install manifest listener
    StopBeforeAcceptListener sbal(testApp);
    secMgr->SetClaimListener(&sbal);

    ASSERT_NE(ER_OK, secMgr->Claim(app, idInfo));

    // check whether the app now gets claimed successfully
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
}

/**
 * @test Changing the manifest listener when being in the callback of the
 *       original manifest listener should work.
 *       -# Claim a CLAIMABLE application with a known identity.
 *       -# While the claim listener is called to approve the manifest, a
 *          new manifest listener is installed to reject the manifest.
 *       -# The original listener accepts the manifest.
 *       -# The application should be claimed.
 *       -# Start a new application and try claiming it.
 *       -# The manifest should be rejected and the claiming should fail.
 *       -# Make sure the new application is still claimable.
 */
TEST_F(ClaimingTests, ConcurrentClaimListenerUpdate) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));

    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    RejectAfterAcceptListener rejectAfterAccept(secMgr);
    secMgr->SetClaimListener(&rejectAfterAccept);

    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED));

    TestApplication testApp2("NewTestApp");
    ASSERT_EQ(ER_OK, testApp2.Start());
    OnlineApplication app2;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp2, app2));

    ASSERT_TRUE(WaitForState(app2, PermissionConfigurator::CLAIMABLE));
    ASSERT_EQ(ER_MANIFEST_REJECTED, secMgr->Claim(app2, idInfo));

    testApp2.SetApplicationState(PermissionConfigurator::CLAIMABLE); // Trigger another event
    ASSERT_TRUE(WaitForState(app2, PermissionConfigurator::CLAIMABLE));
}

class PSKClaimListener :
    public ClaimListener {
  public:
    PSKClaimListener(const GUID128& psk) : localPsk(psk)
    {
    }

    QStatus ApproveManifestAndSelectSessionType(ClaimContext& ctx)
    {
        ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_PSK);
        ctx.ApproveManifest();
        ctx.SetPreSharedKey(localPsk.GetBytes(), GUID128::SIZE);
        return ER_OK;
    }

    GUID128 localPsk;
};

/**
 * @test Verify claiming with Out-Of-Band (OOB) succeeds.
 *       -# Start an application and make sure it's in the CLAIMABLE state
 *          with PSK preference; i.e., OOB.
 *       -# Make sure that the application has generated the PSK.
 *       -# Verify the the security agent uses the same PSK for OOB claiming
 *          and accepts the manifest.
 *       -# Verify that the application is CLAIMED and online.
 *       -# Reset/remove the application and make sure it's claimable again and
 *          repeat the scenario.
 *       -# Verify that claiming was successful and that the application is in
 *          CLAIMED state and online.
 *
 */
TEST_F(ClaimingTests, OOBSuccessfulClaiming) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    //Sanity checks. Make sure that the claim caps are as expected.
    PermissionConfigurator::ClaimCapabilities claimCaps;
    PermissionConfigurator::ClaimCapabilityAdditionalInfo claimCapInfo;
    ASSERT_EQ(ER_OK, GetClaimCapabilities(app, claimCaps, claimCapInfo));
    ASSERT_EQ(PermissionConfigurator::CAPABLE_ECDHE_NULL, claimCaps);
    ASSERT_EQ((size_t)0, claimCapInfo);

    ASSERT_EQ(ER_OK, testApp.SetClaimByPSK());
    ASSERT_EQ(ER_OK, GetClaimCapabilities(app, claimCaps, claimCapInfo));
    ASSERT_EQ(PermissionConfigurator::CAPABLE_ECDHE_PSK, claimCaps);
    ASSERT_EQ(PermissionConfigurator::PSK_GENERATED_BY_APPLICATION, claimCapInfo);

    PSKClaimListener pcl(testApp.GetPsk());
    secMgr->SetClaimListener(&pcl);
    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(idInfo));

    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_EQ(string("ALLJOYN_ECDHE_PSK"), testApp.GetLastAuthMechanism());

    ASSERT_EQ(ER_OK, storage->ResetApplication(app));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_EQ(string("ALLJOYN_ECDHE_PSK"), testApp.GetLastAuthMechanism());
}

class BadPSKClaimListener :
    public ClaimListener {
  public:
    BadPSKClaimListener() : localPsk(0xaf)
    {
    }

    QStatus ApproveManifestAndSelectSessionType(ClaimContext& ctx)
    {
        ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_PSK);
        ctx.ApproveManifest();
        ctx.SetPreSharedKey(localPsk.GetBytes(), GUID128::SIZE);
        return ER_OK;
    }

    GUID128 localPsk;
};

/**
 * @test Verify claiming with Out-Of-Band (OOB) fails when wrong PSK is used.
 *       -# Start an application and make sure it's in the CLAIMABLE state
 *          with PSK preference; i.e., OOB.
 *       -# Make sure that the security agent has generated the PSK.
 *       -# Verify the the application uses a different PSK for OOB claiming.
 *       -# Verify that the application is still CLAIMABLE and online and that
 *          claiming has failed.
 *       -# Repeat the scenario where the PSK is generated by the application
 *          instead of the security agent and make sure PSK claiming fails.
 */
TEST_F(ClaimingTests, OOBFailedClaiming) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    ASSERT_EQ(ER_OK, testApp.SetClaimByPSK());
    BadPSKClaimListener bcl;
    secMgr->SetClaimListener(&bcl);
    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(idInfo));

    ASSERT_NE(ER_OK, secMgr->Claim(app, idInfo));
}

class BadClaimListener :
    public ClaimListener {
  public:
    BadClaimListener(const GUID128& guid) : callSetClaimType(false), callApproveManifest(true), setPsk(false), retVal(
            ER_OK), psk(guid)
    {
    }

    QStatus ApproveManifestAndSelectSessionType(ClaimContext& ctx)
    {
        if (callSetClaimType) {
            EXPECT_EQ(ER_OK, ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_PSK));
            if (setPsk) {
                EXPECT_EQ(ER_OK, ctx.SetPreSharedKey(psk.GetBytes(), GUID128::SIZE));
            }
        }
        if (callApproveManifest) {
            ctx.ApproveManifest();
        }
        return retVal;
    }

    bool callSetClaimType;
    bool callApproveManifest;
    bool setPsk;
    QStatus retVal;
    GUID128 psk;
};

/**
 * @test Verify when the ClaimListener returns errors, these are handled correctly.
 *       -# Start an application and make sure it's in the CLAIMABLE state
 *       -# Try to claim it and trigger error conditions in the ClaimListener.
 *       -# Verify that the application remains CLAIMABLE.
 */
TEST_F(ClaimingTests, ClaimListenerErrors) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    testApp.SetClaimByPSK();
    BadClaimListener bcl(testApp.GetPsk());
    secMgr->SetClaimListener(&bcl);
    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(idInfo));

    ASSERT_EQ(ER_FAIL, secMgr->Claim(app, idInfo));
    bcl.callSetClaimType = true;
    bcl.retVal = ER_BAD_ARG_8;
    ASSERT_EQ(bcl.retVal, secMgr->Claim(app, idInfo));
    bcl.retVal = ER_OK;
    bcl.callApproveManifest = false;
    ASSERT_EQ(ER_MANIFEST_REJECTED, secMgr->Claim(app, idInfo));
    bcl.callApproveManifest = true;
    ASSERT_NE(ER_OK, secMgr->Claim(app, idInfo)); //No psk set.

    testApp.Stop();
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    bcl.setPsk = true;
    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED));
}

class NestedPSKClaimListener :
    public ClaimListener {
  public:
    NestedPSKClaimListener(vector<OnlineApplication>& _apps,
                           vector<shared_ptr<TestApplication> >& _testapps,
                           IdentityInfo& _idInfo,
                           shared_ptr<SecurityAgent>& _secMgr)
        : idInfo(_idInfo), apps(_apps), testapps(_testapps), secMgr(_secMgr)
    {
    }

    QStatus ApproveManifestAndSelectSessionType(ClaimContext& ctx)
    {
        EXPECT_EQ(ER_OK,
                  ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_PSK)) << "loop " <<
            ctx.GetApplication().busName;
        ctx.ApproveManifest();
        string busName = ctx.GetApplication().busName;
        for (size_t i = 0; i < testapps.size(); i++) {
            if (busName == testapps[i]->GetBusName()) {
                EXPECT_EQ(ER_OK, ctx.SetPreSharedKey(testapps[i]->GetPsk().GetBytes(), GUID128::SIZE)) << "loop " << i;

                if (++i < testapps.size()) {
                    return secMgr->Claim(apps[i], idInfo);
                }
                return ER_OK;
            }
        }
        return ER_BAD_ARG_1;
    }

    IdentityInfo idInfo;;
    vector<OnlineApplication> apps;
    vector<shared_ptr<TestApplication> > testapps;
    shared_ptr<SecurityAgent> secMgr;
};

/**
 * @test Verify when the ClaimListener claim another application, these extra claims are handled correctly.
 *       -# Start multiple application and make sure it's in the CLAIMABLE state
 *       -# Try to claim them nesting the claim calls
 *       -# Verify that the applications are CLAIMED.
 */
TEST_F(ClaimingTests, NestedPSKClaims) {
    vector<OnlineApplication> apps;
    vector<shared_ptr<TestApplication> > testapps;
    size_t nr_off_apps = 5;
    for (size_t i = 0; i < nr_off_apps; i++) {
        TestApplication* testapp = new TestApplication(string("NestedTestApp") +
                                                       std::to_string(i));
        testapps.push_back(shared_ptr<TestApplication>(testapp));
        testapps[i]->Start();
        testapps[i]->SetClaimByPSK();
        OnlineApplication app;
        ASSERT_EQ(ER_OK, GetPublicKey(*testapp, app));
        ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
        apps.push_back(app);
    }
    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(idInfo));
    NestedPSKClaimListener bcl(apps, testapps, idInfo, secMgr);
    secMgr->SetClaimListener(&bcl);
    ASSERT_EQ(ER_OK, secMgr->Claim(apps[0], idInfo));

    for (size_t i = 0; i < nr_off_apps; i++) {
        ASSERT_TRUE(WaitForState(apps[i], PermissionConfigurator::CLAIMED));
    }
}

class ClaimThread :
    public Thread {
  public:

    ClaimThread(const IdentityInfo& _idInfo,
                const OnlineApplication& _app,
                const shared_ptr<SecurityAgent>& _secMgr) : idInfo(_idInfo), app(_app), secMgr(_secMgr)
    {
        Start();
    }

    virtual ThreadReturn STDCALL Run(void* arg)
    {
        QCC_UNUSED(arg);
        EXPECT_EQ(ER_OK, secMgr->Claim(app, idInfo));
        return nullptr;
    }

  private:
    IdentityInfo idInfo;
    OnlineApplication app;
    shared_ptr<SecurityAgent> secMgr;
};

class ConcurrentPSKClaimListener :
    public ClaimListener {
  public:
    ConcurrentPSKClaimListener(vector<shared_ptr<TestApplication> >& _testapps)
        :  testapps(_testapps)
    {
    }

    QStatus ApproveManifestAndSelectSessionType(ClaimContext& ctx)
    {
        EXPECT_EQ(ER_OK,
                  ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_PSK)) << "loop " <<
            ctx.GetApplication().busName;
        ctx.ApproveManifest();
        string busName = ctx.GetApplication().busName;
        for (size_t i = 0; i < testapps.size(); i++) {
            if (busName == testapps[i]->GetBusName()) {
                EXPECT_EQ(ER_OK, ctx.SetPreSharedKey(testapps[i]->GetPsk().GetBytes(), GUID128::SIZE)) << "loop " << i;
                return ER_OK;
            }
        }
        return ER_BAD_ARG_1;
    }

    vector<shared_ptr<TestApplication> > testapps;
};

/**
 * @test Verify that the agent concurrently can claim multiple applications.
 *       -# Start multiple applications and try to claim them in parallel
 *       -# Verify that all applications become claimed.
 */
TEST_F(ClaimingTests, ConcurrentPSKClaims) {
    vector<OnlineApplication> apps;
    vector<shared_ptr<TestApplication> > testapps;
    size_t nr_off_apps = 3;
    for (size_t i = 0; i < nr_off_apps; i++) {
        TestApplication* testapp = new TestApplication(string("NestedTestApp") +
                                                       std::to_string(i));
        testapps.push_back(shared_ptr<TestApplication>(testapp));
        testapps[i]->Start();
        testapps[i]->SetClaimByPSK();
        OnlineApplication app;
        ASSERT_EQ(ER_OK, GetPublicKey(*testapp, app));

        ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
        apps.push_back(app);
    }
    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(idInfo));
    ConcurrentPSKClaimListener cl(testapps);
    secMgr->SetClaimListener(&cl);

    vector<shared_ptr<ClaimThread> > threads;
    for (size_t i = 0; i < nr_off_apps; i++) {
        threads.push_back(shared_ptr<ClaimThread>(new ClaimThread(idInfo, apps[i], secMgr)));
    }

    for (size_t i = 0; i < nr_off_apps; i++) {
        ASSERT_TRUE(WaitForState(apps[i], PermissionConfigurator::CLAIMED));
    }

    for (size_t i = 0; i < nr_off_apps; i++) {
        threads[i]->Join();
    }
}

/**
 * @test Verify that the agent resets the application after it claims it, but receives
 *      an error from the storage
 *       -# Start an application and make sure it is claimable.
 *       -# Claim it, but make sure that the storage FinishApplicationClaiming fails
 *       -# Verify that the application becomes claimed for a short while and then claimable again
 */
TEST_F(ClaimingTests, ResetAfterReportClaimFails) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    /* Create identity */
    IdentityInfo idInfo;
    idInfo.guid = GUID128("abcdef123456789");
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    /* Claim application */
    wrappedCA->failOnFinishApplicationClaiming = true;
    ASSERT_EQ(ER_FAIL, secMgr->Claim(app, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED, SYNC_UNMANAGED));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    /* Claim application again*/
    wrappedCA->failOnFinishApplicationClaiming = false;
    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED));
}

class ConcurrentSameClaimListener :
    public ClaimListener {
  public:
    ConcurrentSameClaimListener(const IdentityInfo& _idInfo, const shared_ptr<SecurityAgent>& _secMgr)
        :  checked(false), idInfo(_idInfo), secMgr(_secMgr)
    {
    }

    QStatus ApproveManifestAndSelectSessionType(ClaimContext& ctx)
    {
        EXPECT_FALSE(checked);
        if (!checked) {
            checked = true;
            for (int i = 0; i < 5; i++) {
                QStatus status = secMgr->Claim(ctx.GetApplication(), idInfo);
                EXPECT_EQ(ER_BAD_ARG_1, status);
                if (ER_BAD_ARG_1 != status) {
                    return ER_FAIL;
                }
            }
        }
        EXPECT_EQ(ER_OK, ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_NULL));
        ctx.ApproveManifest();
        return ER_OK;
    }

    bool checked;
    IdentityInfo idInfo;;
    shared_ptr<SecurityAgent> secMgr;
};

/**
 * @test Verify that the agent rejects claim  of an application when it is already claiming
 * that application
 *       -# Start an application and make sure it is claimable.
 *       -# Claim it, but make sure to claim it again while the first claim is ongoing
 *       -# Verify that the application becomes claimed and the second claim fails
 */
TEST_F(ClaimingTests, ConcurrentClaimOfSameApp) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    /* Create identity */
    IdentityInfo idInfo;
    idInfo.guid = GUID128("abcdef123456789");
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);
    ConcurrentSameClaimListener cl(idInfo, secMgr);
    secMgr->SetClaimListener(&cl);
    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
    ASSERT_TRUE(cl.checked);

    /* Check security signal */
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED));
}
} // namespace
