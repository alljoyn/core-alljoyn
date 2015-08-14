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

using namespace ajn::securitymgr;

/** @file IdentityTests.cc */

namespace secmgr_tests {
class IdentityTests :
    public SecurityAgentTest {
  private:

  protected:

  public:
    IdentityTests()
    {
    }
};
/**
 * @test Update the identity certificate of an application and check that it
 *        gets installed correctly.
 *       -# Start the application.
 *       -# Make sure the application is in a CLAIMABLE state.
 *       -# Create and store an IdentityInfo.
 *       -# Claim the application using the IdentityInfo.
 *       -# Check whether the application becomes CLAIMED.
 *       -# Create and store another IdentityInfo.
 *       -# Update the identity certificate of the application.
 *       -# Wait for the updates to be completed.
 *       -# Check whether the identity certificate was installed successfully.
 *       -# Remove the latest identity and make sure the app is removed and
 *          that it becomes claimable again.
 *       -# Use the original identity to claim 2 apps successfully.
 *       -# Remove the original identity and verify that the applications
 *          are removed and they are claimable again.
 *       -# Get all managed applications and verify that none exists.
 **/
TEST_F(IdentityTests, DISABLED_SuccessfulInstallIdentity) {//Requires solution for ASACORE-2342
    /* Start the application */
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));
    IdentityInfo info;
    info.name = "MyName";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(info));

    /* Claim! */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, info));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, SYNC_OK));
    ASSERT_TRUE(CheckIdentity(info, aa.lastManifest));

    /* Try to install another identity */
    IdentityInfo info2;
    info2.name = "AnotherName";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(info2));
    ASSERT_EQ(ER_OK, storage->UpdateIdentity(lastAppInfo, info2, aa.lastManifest));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_TRUE(CheckIdentity(info2, aa.lastManifest));

    /* Remove the identity info and make sure the app is claimable again*/
    ASSERT_EQ(ER_OK, storage->RemoveIdentity(info2));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, SYNC_OK));
    ASSERT_EQ(ER_END_OF_DATA, storage->GetManagedApplication(lastAppInfo));

    /*
     * Use the original identity to claim 2 apps and make sure when the
     * identity is removed those apps are also removed
     * */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, info));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, SYNC_OK));
    ASSERT_TRUE(CheckIdentity(info, aa.lastManifest));

    TestApplication testApp1("NewApp");
    ASSERT_EQ(ER_OK, testApp1.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, SYNC_OK));
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, info));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, SYNC_OK));
    ASSERT_TRUE(CheckIdentity(info, aa.lastManifest));

    ASSERT_EQ(ER_OK, storage->RemoveIdentity(info)); // Should remove testApp and testApp1
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, SYNC_OK)); //First app is claimable again
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, SYNC_OK)); //Second app is claimable again

    vector<Application> apps;
    ASSERT_EQ(ER_OK, storage->GetManagedApplications(apps));
    ASSERT_TRUE(apps.empty());
}

/**
 * @test Update the identity certificate chain.
 *       -# Pending AS-1573 (and implementation in core?)
 **/
TEST_F(IdentityTests, DISABLED_SuccessfulInstallIdentityChain) {
}
} // namespace
