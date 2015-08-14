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
TEST_F(ResetTests, DISABLED_SuccessfulReset) { //Requires solution for ASACORE-2342
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
    ASSERT_TRUE(CheckIdentity(idInfo, aa.lastManifest));

    ASSERT_EQ(ER_OK, storage->RemoveApplication(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true,
                             SYNC_OK));

    ASSERT_TRUE(CheckSyncState(SYNC_OK));

    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
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
TEST_F(ResetTests, DISABLED_RecoveryFromResetFailure) { // see ASACORE-2262
    // create and store identity
    IdentityInfo idInfo;
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    // start and claim test app
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));

    // make sure remote reset will fail
    proxyObjectManager->Reset(lastAppInfo);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    // make sure storage will fail on UpdatesCompleted
    wrappedCA->failOnUpdatesCompleted = true;

    // reset the test application
    ASSERT_EQ(ER_OK, storage->RemoveApplication(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, SYNC_WILL_RESET));

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
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    // check storage
    ASSERT_EQ(ER_END_OF_DATA, storage->GetManagedApplication(lastAppInfo));
    ASSERT_EQ(ER_FAIL, storage->RemoveApplication(lastAppInfo));
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
TEST_F(ResetTests, DISABLED_RecoveryFromResetSuccess) { // see ASACORE-2262
    // create and store identity
    IdentityInfo idInfo;
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    // start and claim test app
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));

    // make sure storage will fail on UpdatesCompleted
    wrappedCA->failOnUpdatesCompleted = true;

    // reset the application
    ASSERT_EQ(ER_OK, storage->RemoveApplication(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, SYNC_WILL_RESET));
    ASSERT_NE(ER_END_OF_DATA, storage->GetManagedApplication(lastAppInfo));

    // stop the test app
    ASSERT_EQ(ER_OK, testApp.Stop());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, false, SYNC_WILL_RESET));

    // restore connectivity to storage
    wrappedCA->failOnUpdatesCompleted = false;

    // restart the app and check whether it is removed from storage
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
    ASSERT_EQ(ER_END_OF_DATA, storage->GetManagedApplication(lastAppInfo));
}
} // namespace
