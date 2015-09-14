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

#include <qcc/Thread.h>

#include "TestUtil.h"

using namespace ajn::securitymgr;

/** @file MembershipTests.cc */

namespace secmgr_tests {
class MembershipTests :
    public SecurityAgentTest {
  private:

  protected:

  public:
    MembershipTests()
    {
        idInfo.guid = GUID128();
        idInfo.name = "TestIdentity";

        groupInfo1.guid = GUID128();
        groupInfo1.name = "MyGroup 1";
        groupInfo1.desc = "My test group 1 description";

        groupInfo2.guid = GUID128();
        groupInfo2.name = "MyGroup 2";
        groupInfo2.desc = "My test group 2 description";
    }

    IdentityInfo idInfo;
    GroupInfo groupInfo1;
    GroupInfo groupInfo2;
};

/**
 * @test Verify the ability to install several memberships based on different
 *       GroupInfo instances.
 *       -# Store a couple of different GroupInfo instances; groupInfo1 and
 *          groupInfo2 in persistency.
 *       -# Start an application and make sure it's online and CLAIMABLE.
 *       -# Try to install and remove a membership using the lately announced
 *          application and make sure this fails.
 *       -# Successfully store an IdentityInfo instance.
 *       -# Successfully claim the application using the IdentityInfo instance.
 *       -# Make sure the application is online and in the CLAIMED state
 *          with no updates pending.
 *       -# Make sure the remote identity and manifest of the application
 *          match the stored ones.
 *       -# Verify that installing of membership using groupInfo1 is successful.
 *       -# Make sure updates have been completed.
 *       -# Repeat the previous 2 steps for groupInfo2.
 *       -# Verify that removal of membership using groupInfo1 is successful.
 *       -# Make sure updates have been completed.
 *       -# Repeat the previous 2 steps for groupInfo2.
 *       -# Install memberships for both groupInfo1 and groupInfo2 successfully.
 *       -# Verify that deleting groupInfo1 and groupInfo2 will result in syncing the app
 *          again and in removing the memberships associated.
 *       -# Repeat the previous step but verify the removal of memberships associated
 *          immediately after the deletion of each group.
 **/
TEST_F(MembershipTests, SuccessfulInstallMembership) {
    /* Create groups */
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo1));
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo2));

    /* Start the test application */
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE));

    /* Installing or removing membership before claiming should fail */
    Application app = lastAppInfo;
    ASSERT_NE(ER_OK, storage->InstallMembership(app, groupInfo2)); // fails due to manifest missing in persistency
    ASSERT_NE(ER_OK, storage->RemoveMembership(app, groupInfo2)); // fails due to certificate missing in persistency

    /* Create identity */
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_TRUE(CheckIdentity(idInfo, aa.lastManifest));

    ASSERT_EQ(ER_OK, storage->InstallMembership(app, groupInfo1));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    vector<GroupInfo> memberships;
    memberships.push_back(groupInfo1);
    ASSERT_TRUE(CheckMemberships(memberships));

    ASSERT_EQ(ER_OK, storage->InstallMembership(app, groupInfo2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    memberships.push_back(groupInfo2);
    ASSERT_TRUE(CheckMemberships(memberships));

    ASSERT_EQ(ER_OK, storage->RemoveMembership(app, groupInfo1));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    memberships.erase(memberships.begin());
    ASSERT_TRUE(CheckMemberships(memberships));

    ASSERT_EQ(ER_OK, storage->RemoveMembership(app, groupInfo2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    memberships.erase(memberships.begin());
    ASSERT_TRUE(CheckMemberships(memberships));

    /* *
     * Install memberships for groupinfo1 and groupinfo1 then
     * remove both and make sure the app is synced and the
     * memberships associated are deleted.
     * */
    ASSERT_EQ(ER_OK, storage->InstallMembership(app, groupInfo1));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    memberships.push_back(groupInfo1);
    ASSERT_TRUE(CheckMemberships(memberships));

    ASSERT_EQ(ER_OK, storage->InstallMembership(app, groupInfo2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    memberships.push_back(groupInfo2);
    ASSERT_TRUE(CheckMemberships(memberships));

    ASSERT_EQ(ER_OK, storage->RemoveGroup(groupInfo1));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_EQ(ER_OK, storage->RemoveGroup(groupInfo2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    memberships.erase(memberships.begin());
    memberships.erase(memberships.begin());
    ASSERT_TRUE(CheckMemberships(memberships));
    ASSERT_EQ(ER_END_OF_DATA, storage->RemoveMembership(app, groupInfo1));
    ASSERT_EQ(ER_END_OF_DATA, storage->RemoveMembership(app, groupInfo2));

    /* *
     * Install memberships for both groups but remove and
     * verify immediately after each removal.
     */
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo1));
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo2));

    ASSERT_EQ(ER_OK, storage->InstallMembership(app, groupInfo1));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    memberships.push_back(groupInfo1);
    ASSERT_TRUE(CheckMemberships(memberships));

    ASSERT_EQ(ER_OK, storage->InstallMembership(app, groupInfo2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    memberships.push_back(groupInfo2);
    ASSERT_TRUE(CheckMemberships(memberships));

    ASSERT_EQ(ER_OK, storage->RemoveGroup(groupInfo1));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    memberships.erase(memberships.begin());
    ASSERT_TRUE(CheckMemberships(memberships));
    ASSERT_EQ(ER_END_OF_DATA, storage->RemoveMembership(app, groupInfo1));

    ASSERT_EQ(ER_OK, storage->RemoveGroup(groupInfo2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    memberships.erase(memberships.begin());
    ASSERT_TRUE(CheckMemberships(memberships));
    ASSERT_EQ(ER_END_OF_DATA, storage->RemoveMembership(app, groupInfo2));
}

/**
 * @test Verify that installing and removing a membership triggers an increase in the policy version
 *       -# Start an application and make sure it's online and CLAIMABLE.
 *       -# Successfully store an IdentityInfo instance.
 *       -# Successfully claim the application using the IdentityInfo instance.
 *       -# Make sure the application is online and in the CLAIMED state
 *          with no updates pending.
 *       -# Make sure the remote identity and manifest of the application
 *          match the stored ones.
 *       -# Update a policy on an application
 *       -# Verify that installing of membership using groupInfo1 is successful.
 *       -# Make sure updates have been completed.
 *       -# Check that the policy version increased.
 *       -# Verify that removing of membership using groupInfo1 is successful.
 *       -# Make sure updates have been completed.
 *       -# Check that the policy version increased again.
 **/
TEST_F(MembershipTests, InstallRemoveMembershipPolicyUpdate) {
    /* Create groups */
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo1));

    /* Start the test application */
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE));
    OnlineApplication app = lastAppInfo;
    /* Create identity */
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_TRUE(CheckIdentity(idInfo, aa.lastManifest));

    vector<GroupInfo> policyGroups;
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(policyGroups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(app, policy));
    ASSERT_TRUE(WaitForUpdatesCompleted());

    uint32_t currentVersion;
    ASSERT_EQ(ER_OK, GetPolicyVersion(app, currentVersion));
    ASSERT_EQ(ER_OK, storage->InstallMembership(app, groupInfo1));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    uint32_t remoteVersion;
    ASSERT_EQ(ER_OK, GetPolicyVersion(app, remoteVersion));
    ASSERT_EQ(1 + currentVersion, remoteVersion);

    ASSERT_EQ(ER_OK, storage->RemoveMembership(app, groupInfo1));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_EQ(ER_OK, GetPolicyVersion(app, remoteVersion));
    ASSERT_EQ(2 + currentVersion, remoteVersion);
}
} // namespace
