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

using namespace ajn;
using namespace ajn::securitymgr;

/** @file ResetTests.cc */

namespace secmgr_tests {
class ResetTests :
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
 * @test Reset an application and make sure it becomes CLAIMABLE again.
 *       -# Start the application.
 *       -# Make sure the application is in a CLAIMABLE state.
 *       -# Create and store an IdentityInfo.
 *       -# Claim the application using the IdentityInfo.
 *       -# Accept the manifest of the application.
 *       -# Check whether the application becomes CLAIMED.
 *       -# Remove the application from storage.
 *       -# Check whether it becomes CLAIMABLE again.
 *       -# Claim the application again.
 *       -# Check whether it becomes CLAIMED again.
 **/
TEST_F(ResetTests, SuccessfulReset) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED));
    ASSERT_TRUE(CheckIdentity(app, idInfo, aa.lastManifest));

    ASSERT_EQ(ER_OK, storage->ResetApplication(app));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED));
}

/**
 * @test Recovery from failure of notifying the CA of failure to reset an
 *       application should be graceful.
 *       -# Start a test application and claim it.
 *       -# Make sure remote reset fails.
 *       -# Stop the application.
 *       -# Make sure the UpdatesCompleted to storage fails.
 *       -# Reset the application and check that this succeeds.
 *       -# Restart the test application and make sure it is removed from
 *          storage.
 */
TEST_F(ResetTests, RecoveryFromResetFailure) {
    // create and store identity
    IdentityInfo idInfo;
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    // start and claim test app
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));

    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED));

    // make sure remote reset will fail
    Reset(app);
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE, SYNC_OK));

    // make sure storage will fail on UpdatesCompleted
    wrappedCA->failOnUpdatesCompleted = true;

    // reset the test application
    ASSERT_EQ(ER_OK, storage->ResetApplication(app));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE, SYNC_WILL_RESET));

    // stop agent to make sure update is completed
    RemoveSecAgent();

    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());

    // make sure storage will succeed on UpdatesCompleted
    wrappedCA->failOnUpdatesCompleted = false;

    // restart agent
    InitSecAgent();

    // start the remote application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    // check storage
    ASSERT_EQ(ER_END_OF_DATA, storage->GetManagedApplication(app));
    ASSERT_EQ(ER_FAIL, storage->ResetApplication(app));
}

/**
 * @test Recovery from failure of notifying the CA of successful resetting an
 *       application should be graceful.
 *       -# Start a test application and claim it.
 *       -# Make sure the UpdatesCompleted to storage fails.
 *       -# Reset the application and check that this succeeds.
 *       -# Restart the test application and make sure it is removed from
 *          storage.
 */
TEST_F(ResetTests, RecoveryFromResetSuccess) {
    // create and store identity
    IdentityInfo idInfo;
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    // start and claim test app
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED));

    // make sure storage will fail on UpdatesCompleted
    wrappedCA->failOnUpdatesCompleted = true;

    // reset the application
    ASSERT_EQ(ER_OK, storage->ResetApplication(app));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE, SYNC_WILL_RESET));
    ASSERT_NE(ER_END_OF_DATA, storage->GetManagedApplication(app));

    // stop the test app
    ASSERT_EQ(ER_OK, testApp.Stop());

    // restore connectivity to storage
    wrappedCA->failOnUpdatesCompleted = false;

    // restart the app and check whether it is removed from storage
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
    ASSERT_EQ(ER_END_OF_DATA, storage->GetManagedApplication(app));
}

/**
 * @test Discovery of an application that has been remotely reset, should result in a sync error and
 *       should be reclaimable after removing it from storage.
 *       -# Start a test application and claim it.
 *       -# Reset the application behind the back of the security manager.
 *       -# Check that the security manager reports a SYNC_ER_UNEXPECTED_STATE.
 *       -# Check that reclaiming the application would fail.
 *       -# Forcibly remove the application from storage.
 *       -# Check that reclaiming the application now succeeds.
 */
TEST_F(ResetTests, RecoveryFromRemoteReset) {
    // create and store identity
    IdentityInfo idInfo;
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    // start and claim test app
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));

    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED));

    // reset the application behind the back of the security manager
    Reset(app);
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE, SYNC_OK));

    // restart test application and wait for sync error
    ASSERT_EQ(ER_OK, testApp.Stop());
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForSyncError(SYNC_ER_UNEXPECTED_STATE, ER_FAIL));

    // check that claim fails
    ASSERT_EQ(ER_FAIL, secMgr->Claim(app, idInfo));

    // claim should succeed after removing application from storage
    ASSERT_EQ(ER_OK, storage->RemoveApplication(app));
    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
}
} // namespace
