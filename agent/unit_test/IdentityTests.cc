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
TEST_F(IdentityTests, SuccessfulInstallIdentity) {
    /* Start the application */
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
    IdentityInfo info;
    info.name = "MyName";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(info));

    /* Claim! */
    ASSERT_EQ(ER_OK, secMgr->Claim(app, info));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_TRUE(CheckIdentity(app, info, aa.lastManifest));

    /* Try to install another identity */
    IdentityInfo info2;
    info2.name = "AnotherName";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(info2));
    ASSERT_EQ(ER_OK, storage->UpdateIdentity(app, info2, aa.lastManifest));
    ASSERT_TRUE(WaitForUpdatesCompleted(app));
    ASSERT_TRUE(CheckIdentity(app, info2, aa.lastManifest));

    /* Remove the identity info and make sure the app is claimable again*/
    ASSERT_EQ(ER_OK, storage->RemoveIdentity(info2));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
    ASSERT_EQ(ER_END_OF_DATA, storage->GetManagedApplication(app));

    /*
     * Use the original identity to claim 2 apps and make sure when the
     * identity is removed those apps are also removed
     * */
    ASSERT_EQ(ER_OK, secMgr->Claim(app, info));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_TRUE(CheckIdentity(app, info, aa.lastManifest));

    TestApplication testApp1("NewApp");
    ASSERT_EQ(ER_OK, testApp1.Start());
    OnlineApplication app1;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp1, app1));
    ASSERT_TRUE(WaitForState(app1, PermissionConfigurator::CLAIMABLE));
    ASSERT_EQ(ER_OK, secMgr->Claim(app1, info));
    ASSERT_TRUE(WaitForState(app1, PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_TRUE(CheckIdentity(app1, info, aa.lastManifest));

    ASSERT_EQ(ER_OK, storage->RemoveIdentity(info)); // Should remove testApp and testApp1
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE)); //First app is claimable again
    ASSERT_TRUE(WaitForState(app1, PermissionConfigurator::CLAIMABLE)); //Second app is claimable again

    vector<Application> apps;
    ASSERT_EQ(ER_OK, storage->GetManagedApplications(apps));
    ASSERT_EQ((size_t)1, apps.size()); //Only the app to check the status with.
}

/**
 * @test Verify that update identity triggers an increase of the policy version
 *       -# Start an application and make sure it's online and CLAIMABLE.
 *       -# Successfully store an IdentityInfo instance.
 *       -# Successfully claim the application using the IdentityInfo instance.
 *       -# Make sure the application is online and in the CLAIMED state
 *          with no updates pending.
 *       -# Make sure the remote identity and manifest of the application
 *          match the stored ones.
 *       -# Update a policy on an application
 *       -# Verify that updateIdentity is succesful.
 *       -# Make sure updates have been completed.
 *       -# Check that the policy version increased.
 **/
TEST_F(IdentityTests, UpdateIdentityPolicyUpdate) {
    /* Start the test application */
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
    /* Create identity */
    IdentityInfo info;
    info.name = "MyName";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(info));
    ASSERT_EQ(storage->StoreIdentity(info), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(app, info));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_TRUE(CheckIdentity(app, info, aa.lastManifest));

    vector<GroupInfo> policyGroups;
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(policyGroups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(app, policy));
    ASSERT_TRUE(WaitForUpdatesCompleted(app));

    uint32_t currentVersion;
    ASSERT_EQ(ER_OK, GetPolicyVersion(app, currentVersion));
    /* Try to install another identity */
    IdentityInfo info2;
    info2.name = "AnotherName";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(info2));
    ASSERT_EQ(ER_OK, storage->UpdateIdentity(app, info2, aa.lastManifest));
    ASSERT_TRUE(WaitForUpdatesCompleted(app));
    ASSERT_TRUE(CheckIdentity(app, info2, aa.lastManifest));
    uint32_t remoteVersion;
    ASSERT_EQ(ER_OK, GetPolicyVersion(app, remoteVersion));
    ASSERT_EQ(1 + currentVersion, remoteVersion);
}
} // namespace
