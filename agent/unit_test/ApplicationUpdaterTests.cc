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

using namespace secmgrcoretest_unit_testutil;

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
 * \test Reset an offline application and check its claimable state when it
 *       comes back online.
 *       -# Stop remote application.
 *       -# Reset the application using the security agent.
 *       -# Restart the remote application.
 *       -# Check whether the remote application is CLAIMABLE.
 **/

TEST_F(ApplicationUpdaterTests, Reset) {
    // stop the stub
    delete stub;
    stub = nullptr;

    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // reset the stub
    ASSERT_EQ(ER_OK, storage->RemoveApplication(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false, true));
    ASSERT_TRUE(CheckUpdatesPending(true)); // app was offline

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
}

/**
 * \test Install a membership certificate for an offline application and bring
 *       it back online.
 *       -# Stop remote application.
 *       -# Install a membership certificate using the security agent.
 *       -# Restart the remote application.
 **/
TEST_F(ApplicationUpdaterTests, InstallMembership) {
    // stop the stub
    delete stub;
    stub = nullptr;
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // change security configuration
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    ASSERT_EQ(ER_OK, storage->InstallMembership(lastAppInfo, groupInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false, true));
    ASSERT_TRUE(CheckUpdatesPending(true)); // app was offline

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
    vector<GroupInfo> memberships;
    memberships.push_back(groupInfo);
    ASSERT_TRUE(CheckRemoteMemberships(memberships));
}

/**
 * \test Update a policy for an offline application and bring it back online.
 *       -# Stop remote application.
 *       -# Install a policy using the security agent.
 *       -# Restart the remote application.
 **/
TEST_F(ApplicationUpdaterTests, UpdatePolicy) {
    // stop the stub
    delete stub;
    stub = nullptr;
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // change security configuration
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    vector<GroupInfo> groups;
    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, policy));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false, true));
    ASSERT_TRUE(CheckUpdatesPending(true)); // app was offline

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
    ASSERT_TRUE(CheckRemotePolicy(policy));
}

/**
 * \test Install an identity certificate for an offline application and bring
 *       it back online.
 *       -# Stop remote application.
 *       -# Install an identity certificate using the security agent.
 *       -# Restart the remote application.
 **/
TEST_F(ApplicationUpdaterTests, InstallIdentity) {
    // stop the stub
    delete stub;
    stub = nullptr;
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // change security configuration
    IdentityInfo identityInfo2;
    identityInfo2.name = "Updated test name";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(identityInfo2));
    ASSERT_EQ(ER_OK, storage->UpdateIdentity(lastAppInfo, identityInfo2));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false, true));
    ASSERT_TRUE(CheckUpdatesPending(true)); // app was offline

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
}

/**
 * \test Change the complete security configuration of an offline application
 *       and bring it back online.
 *       -# Stop remote application.
 *       -# Install a membership certificate using the security agent.
 *       -# Install a policy using the security agent.
 *       -# Install an identity certificate using the security agent.
 *       -# Restart the remote application.
 *       -# Wait for the updates to complete.
 *       -# Stop the remote application again.
 *       -# Reset the remote application using the security agent.
 *       -# Restart the remote application.
 *       -# Check whether the remote application is CLAIMABLE.
 **/
TEST_F(ApplicationUpdaterTests, UpdateAll) {
    // stop the stub
    delete stub;
    stub = nullptr;
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

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
    ASSERT_TRUE(CheckRemotePolicy(policy));
    vector<GroupInfo> memberships;
    memberships.push_back(groupInfo);
    ASSERT_TRUE(CheckRemoteMemberships(memberships));

    // stop the stub
    delete stub;
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));

    // reset the stub
    ASSERT_EQ(ER_OK, storage->RemoveApplication(lastAppInfo));

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, false));
}
