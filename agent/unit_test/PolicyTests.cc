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

#include <alljoyn/securitymgr/PolicyGenerator.h>

#include "TestUtil.h"

using namespace ajn::securitymgr;

/** @file PolicyTests.cc */

namespace secmgr_tests {
class PolicyTests :
    public SecurityAgentTest {
  private:

  protected:

  public:
    PolicyTests()
    {
        idInfo.guid = GUID128();
        idInfo.name = "TestIdentity";
    }

    IdentityInfo idInfo;

    GUID128 groupGUID;
    GUID128 groupGUID2;
    PermissionPolicy policy;
    PermissionPolicy policy2;
};

/**
 * @test Update the policy of an application and check whether it is updated
 *       correctly.
 *       -# Start the application.
 *       -# Installing and retrieving the policy before claiming should fail.
 *       -# Make sure the application is in a CLAIMABLE state.
 *       -# Create and store an IdentityInfo.
 *       -# Claim the application using the IdentityInfo.
 *       -# Accept the manifest of the application.
 *       -# Check whether the application becomes CLAIMED.
 *       -# Make sure the retrieval of the policy returns ER_END_OF_DATA.
 *       -# Update the policy.
 *       -# Wait for updates to complete.
 *       -# Update the policy again.
 *       -# Check whether the remote policy is equal to the installed policy.
 *       -# Check whether the remote policy is equal to the policy that can be
 *          retrieved from storage.
 *       -# Wait for updates to complete.
 *       -# Check whether the remote policy is equal to the installed policy.
 *       -# Check whether the remote policy is equal to the policy that can be
 *          retrieved from storage.
 *       -# Try to install a newer policy (version 100) and verify it was
 *          successful
 *       -# Try to install an older policy (version 1) and verify this fails
 *       -# Try to install a default policy of (version 0) and verify this
 *          was successful.
 *       -# Get the persisted policy and make sure its version is (version 100 +1)
 **/
TEST_F(PolicyTests, SuccessfulInstallPolicyAndUpdatePolicy) {
    vector<GroupInfo> policyGroups;
    GroupInfo group;
    group.guid = groupGUID;
    ASSERT_EQ(ER_OK, storage->StoreGroup(group));
    policyGroups.push_back(group);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(policyGroups, policy));

    GroupInfo group2;
    group2.guid = groupGUID2;
    ASSERT_EQ(ER_OK, storage->StoreGroup(group2));
    policyGroups.push_back(group2);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(policyGroups, policy2));

    /* Start the test application */
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    /* Installing/retrieving policy before claiming should fail */
    Application app = lastAppInfo;
    PermissionPolicy policyLocal;
    ASSERT_NE(ER_OK, storage->UpdatePolicy(app, policy));
    ASSERT_NE(ER_OK, storage->UpdatePolicy(app, policy2));
    ASSERT_NE(ER_OK, storage->GetPolicy(app, policyLocal));
    ASSERT_TRUE(CheckSyncState(SYNC_OK));

    /* Create identity */
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
    ASSERT_TRUE(CheckIdentity(idInfo, aa.lastManifest));

    /* Check default policy */
    ASSERT_TRUE(CheckDefaultPolicy());

    /* Install policy and check retrieved policy */
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(app, policy));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_TRUE(CheckPolicy(policy));

    /* Install another policy and check retrieved policy */
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(app, policy2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_TRUE(CheckPolicy(policy2));

    /* Install a newer policy and check retrieved policy */
    policy2.SetVersion(100);
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(app, policy2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_TRUE(CheckPolicy(policy2));

    /* Install an older policy and ensure failure */
    policy2.SetVersion(1);
    ASSERT_EQ(ER_POLICY_NOT_NEWER, storage->UpdatePolicy(app, policy2));
    ASSERT_FALSE(WaitForUpdatesCompleted());
    ASSERT_FALSE(CheckPolicy(policy2));

    /* Install an default v=0 policy and ensure successful update */
    policy2.SetVersion(0);
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(app, policy2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_TRUE(CheckPolicy(policy2));

    /* Get the persisted policy and compare latest version */
    PermissionPolicy finalPolicy;
    ASSERT_EQ(ER_OK, storage->GetPolicy(app, finalPolicy));
    // Latest successful update was with policy v=100
    ASSERT_EQ((size_t)(100 + 1), finalPolicy.GetVersion());
}

/**
 * @test Verify resetting the policy of an application succeeds.
 *       -# Start the application and make sure it's claimable.
 *       -# Claim the application successfully.
 *       -# Check the default policy.
 *       -# Install a different policy and wait until updates have been
 *          completed.
         -# Check whether the policy was installed successfully.
 *       -# Reset the policy and wait until updates have been completed.
 *       -# Check the default policy.
 */
TEST_F(PolicyTests, SuccessfulResetPolicy) {
    /* Start the test application */
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    /* Store identity */
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
    ASSERT_TRUE(CheckDefaultPolicy());

    /* Install policy */
    vector<GroupInfo> policyGroups;
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(policyGroups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, policy));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_TRUE(CheckPolicy(policy));

    /* Reset policy */
    ASSERT_EQ(ER_OK, storage->RemovePolicy(lastAppInfo));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_TRUE(CheckDefaultPolicy());
}

/**
 * @test Verify the the security agent can handle permission denied response.
 *       -# Start the application and make sure it's claimable.
 *       -# Claim the application successfully.
 *       -# Install a policy that does NOT contain the admin group rule.
 *       -# Check whether the application is in SYNC_PENDING state.
 *       -# Make sure that at least one sync error is triggered.
 */
TEST_F(PolicyTests, PermissionDenied) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    /* Create identity */
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
    ASSERT_TRUE(CheckIdentity(idInfo, aa.lastManifest));

    /* Check default policy */
    ASSERT_TRUE(CheckDefaultPolicy());

    /* No admin group policy*/
    PermissionPolicy emptyPolicy;

    /* Install policy and check retrieved policy */
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, emptyPolicy));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, SYNC_PENDING));
    ASSERT_TRUE(WaitForSyncError(SYNC_ER_REMOTE, ER_PERMISSION_DENIED));
}
} // namespace
