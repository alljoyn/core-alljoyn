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

#include <gtest/gtest.h>

#include <qcc/GUID.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/Status.h>

#include <alljoyn/securitymgr/PolicyGenerator.h>

#include "TestUtil.h"

/** @file ApplicationUpdaterTests.cc */

namespace secmgr_tests {
class ApplicationUpdaterTests :
    public ClaimedTest {
  public:
    ApplicationUpdaterTests()
    {
        string groupName = "Test";
        string groupDesc = "This is a test group";

        groupInfo.name = groupName;
        groupInfo.desc = groupDesc;

        policyGroups.push_back(groupInfo.guid);
    }

  public:
    GroupInfo groupInfo;
    PermissionPolicy policy;
    vector<GUID128> policyGroups;
};

/**
 * @test Reset an offline application and check its claimable state when it
 *       comes back online.
 *       -# Claim and stop a remote application.
 *       -# Reset the application from storage.
 *       -# Restart the remote application.
 *       -# Wait for the updates to complete.
 *       -# Check whether the remote application is CLAIMABLE.
 **/

TEST_F(ApplicationUpdaterTests, Reset) {
    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // reset the test application
    ASSERT_EQ(ER_OK, storage->RemoveApplication(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false, true));
    ASSERT_TRUE(CheckUpdatesPending(true)); // app was offline

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
}

/**
 * @test Install a membership certificate for an offline application and check
 *       whether it was successfully installed when it comes back online.
 *       -# Claim and stop remote application.
 *       -# Store a membership certificate for the application.
 *       -# Restart the remote application.
 *       -# Wait for the updates to complete.
 *       -# Ensure the membership certificate is correctly installed.
 **/
TEST_F(ApplicationUpdaterTests, InstallMembership) {
    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // change security configuration
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    ASSERT_EQ(ER_OK, storage->InstallMembership(lastAppInfo, groupInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false, true));
    ASSERT_TRUE(CheckUpdatesPending(true)); // app was offline

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
    vector<GroupInfo> memberships;
    memberships.push_back(groupInfo);
    ASSERT_TRUE(CheckMemberships(memberships));
}

/**
 * @test Update a policy for an offline application and check whether it
 *       was successfully updated when it comes back online.
 *       -# Claim and stop remote application.
 *       -# Update the stored policy of the application.
 *       -# Restart the remote application.
 *       -# Wait for the updates to complete.
 *       -# Ensure the policy is correctly installed.
 **/
TEST_F(ApplicationUpdaterTests, UpdatePolicy) {
    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // change security configuration
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    vector<GroupInfo> groups;
    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, policy));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false, true));
    ASSERT_TRUE(CheckUpdatesPending(true)); // app was offline

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
    ASSERT_TRUE(CheckPolicy(policy));
}

/**
 * @test Reset a policy for an offline application and check whether it
 *       was successfully reset when it comes back online.
 *       -# Claim and install a policy on the application.
 *       -# Stop remote application.
 *       -# Reset the stored policy of the application.
 *       -# Restart the remote application.
 *       -# Wait for the updates to complete.
 *       -# Ensure the policy is correctly reset.
 **/
TEST_F(ApplicationUpdaterTests, DISABLED_ResetPolicy) { // see ASACORE-2200
    // generate and install a policy
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    vector<GroupInfo> groups;
    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, policy));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_TRUE(CheckPolicy(policy));

    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // reset the policy
    ASSERT_EQ(ER_OK, storage->RemovePolicy(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false, true));
    ASSERT_TRUE(CheckUpdatesPending(true)); // app was offline

    // restart the test application and check for default policy
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
    ASSERT_TRUE(CheckDefaultPolicy());
}

/**
 * @test Update the identity certificate for an offline application and check
 *       whether it was successfully updated when it comes back online.
 *       -# Claim and stop remote application.
 *       -# Update the stored identity certificate of the application.
 *       -# Restart the remote application.
 *       -# Wait for the updates to complete.
 *       -# Ensure the identity certificate is correctly installed.
 **/
TEST_F(ApplicationUpdaterTests, InstallIdentity) {
    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());;
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // change security configuration
    IdentityInfo identityInfo2;
    identityInfo2.name = "Updated test name";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(identityInfo2));
    ASSERT_EQ(ER_OK, storage->UpdateIdentity(lastAppInfo, identityInfo2));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false, true));
    ASSERT_TRUE(CheckUpdatesPending(true)); // app was offline

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
}

/**
 * @test Change the complete security configuration of an offline application
 *       and check whether it was successfully updated when it comes back
 *       online.
 *       -# Claim and stop remote application.
 *       -# Store a membership certificate for the application.
 *       -# Update stored policy for the application.
 *       -# Update stored identity certificate for the application.
 *       -# Restart the remote application.
 *       -# Wait for the updates to complete.
 *       -# Ensure the membership certificate is installed correctly.
 *       -# Ensure the policy is updated correctly.
 *       -# Ensure the identity certificate is updated correctly.
 *       -# Stop the remote application again.
 *       -# Reset the remote application from storage.
 *       -# Restart the remote application.
 *       -# Check whether the remote application is CLAIMABLE again.
 **/
TEST_F(ApplicationUpdaterTests, UpdateAll) {
    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // change security configuration
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    ASSERT_EQ(ER_OK, storage->InstallMembership(lastAppInfo, groupInfo));
    vector<GroupInfo> groups;
    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, policy));
    IdentityInfo identityInfo2;
    identityInfo2.name = "Updated test name";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(identityInfo2));
    ASSERT_EQ(ER_OK, storage->UpdateIdentity(lastAppInfo, identityInfo2));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false, true));
    ASSERT_TRUE(CheckUpdatesPending(true)); // app was offline

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
    ASSERT_TRUE(CheckPolicy(policy));
    vector<GroupInfo> memberships;
    memberships.push_back(groupInfo);
    ASSERT_TRUE(CheckMemberships(memberships));

    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // reset the test application
    ASSERT_EQ(ER_OK, storage->RemoveApplication(lastAppInfo));

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());;
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, false));
}

/**
 * @test Make sure resetting of an application fails, and check if a sync error
 *       of type SYNC_ER_RESET is triggered.
 *       -# Claim the remote application.
 *       -# Stop the security agent.
 *       -# Reset/remove the keystore of the remote application.
 *       -# Restart the security agent.
 *       -# Restart the remote application.
 *       -# Check if a sync error of type SYNC_ER_RESET is triggered.
 **/
TEST_F(ApplicationUpdaterTests, DISABLED_SyncErReset) {
}

/**
 * @test Install a permission policy with an older version than the one
 *       currently installed, and check if a sync error of type SYNC_ER_POLICY
 *       is triggered.
 *       -# Claim the remote application and stop it.
 *       -# Update the policy of the application to a previous version.
 *       -# Restart the remote application.
 *       -# Check if a sync error of type SYNC_ER_POLICY is triggered.
 **/
TEST_F(ApplicationUpdaterTests, DISABLED_SyncErPolicy) {
}

/**
 * @test Update the identity certificate of an application with an invalid
 *       certificate, and check whether a sync error of type SYNC_ER_POLICY
 *       is triggered.
 *       -# Claim the remote application and stop it.
 *       -# Make sure CAStorage returns an invalid certificate (e.g.
 *          digest mismatching the associated manifest).
 *       -# Restart the remote application.
 *       -# Check if a sync error of type SYNC_ER_IDENTITY is triggered.
 **/
TEST_F(ApplicationUpdaterTests, DISABLED_SyncErIdentity) {
}

/**
 * @test Install a membership certificate of an application with an invalid
 *       certificate, and check whether a sync error of type SYNC_ER_MEMBERSHIP
 *       is triggered.
 *       -# Claim the remote application and stop it.
 *       -# Make sure CAStorage returns an invalid certificate (e.g.
 *          signed with a different private key).
 *       -# Restart the remote application.
 *       -# Check if a sync error of type SYNC_ER_MEMBERSHIP is triggered.
 **/
TEST_F(ApplicationUpdaterTests, DISABLED_SyncErMembership) {
}

/**
 * @test Stop the CAStorage, and make sure the application updater starts
 *       eventing some SYNC_ER_STORAGE errors when updating an application.
 *       -# Claim the remote application and stop it.
 *       -# Stop the CAStorage layer (or make sure that some basic functions
 *          like GetManagedApplication start returing errors).
 *       -# Restart the remote application.
 *       -# Check if a sync error of type SYNC_ER_STORAGE is triggered.
 **/
TEST_F(ApplicationUpdaterTests, DISABLED_SyncErStorage) {
}
}
