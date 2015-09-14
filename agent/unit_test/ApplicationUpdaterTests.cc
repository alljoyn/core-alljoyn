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

#include <gtest/gtest.h>

#include <qcc/GUID.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/Status.h>

#include <alljoyn/securitymgr/PolicyGenerator.h>

#include "TestUtil.h"
#include "AgentStorageWrapper.h"

/** @file ApplicationUpdaterTests.cc */

namespace secmgr_tests {
class SyncErrorStorageWrapper :
    public AgentStorageWrapper {
  public:
    SyncErrorStorageWrapper(shared_ptr<AgentCAStorage>& _ca,
                            shared_ptr<UIStorage>& _storage) :
        AgentStorageWrapper(_ca),
        failOnStartUpdates(false),
        returnEmptyMembershipCert(false),
        storage(_storage),
        returnWrappedPolicy(false),
        returnWrappedManifest(false)
    {
    }

    QStatus StartUpdates(Application& app, uint64_t& updateID)
    {
        if (failOnStartUpdates) {
            return ER_FAIL;
        }
        return ca->StartUpdates(app, updateID);
    }

    QStatus GetPolicy(const Application& app,
                      PermissionPolicy& _policy) const
    {
        if (returnWrappedPolicy) {
            _policy = policy;
            return ER_OK;
        }
        return ca->GetPolicy(app, _policy);
    }

    QStatus GetIdentityCertificatesAndManifest(const Application& app,
                                               IdentityCertificateChain& identityCertificates,
                                               Manifest& _manifest) const
    {
        QStatus status = ca->GetIdentityCertificatesAndManifest(app, identityCertificates, _manifest);

        if (returnWrappedManifest) {
            _manifest = manifest;
        }

        return status;
    }

    QStatus GetMembershipCertificates(const Application& app,
                                      vector<MembershipCertificateChain>& certs) const
    {
        if (returnEmptyMembershipCert) {
            MembershipCertificate emptyCert;
            MembershipCertificateChain emptyCertChain;
            emptyCertChain.push_back(emptyCert);
            certs.push_back(emptyCertChain);
            return ER_OK;
        }

        return ca->GetMembershipCertificates(app, certs);
    }

    void SetPolicy(PermissionPolicy _policy)
    {
        policy = _policy;
        returnWrappedPolicy = true;
    }

    void UnsetPolicy()
    {
        returnWrappedPolicy = false;
    }

    void SetManifest(Manifest _manifest)
    {
        manifest = _manifest;
        returnWrappedManifest = true;
    }

    void UnsetManifest()
    {
        returnWrappedManifest = false;
    }

  public:
    bool failOnStartUpdates;
    bool returnEmptyMembershipCert;

  private:
    SyncErrorStorageWrapper& operator=(const SyncErrorStorageWrapper);

    shared_ptr<UIStorage>& storage;
    PermissionPolicy policy;
    bool returnWrappedPolicy;
    Manifest manifest;
    bool returnWrappedManifest;
};

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

    shared_ptr<AgentCAStorage>& GetAgentCAStorage()
    {
        wrappedCA = shared_ptr<SyncErrorStorageWrapper>(new SyncErrorStorageWrapper(ca, storage));
        ca = wrappedCA;
        return ca;
    }

  public:
    GroupInfo groupInfo;
    PermissionPolicy policy;
    vector<GUID128> policyGroups;
    shared_ptr<SyncErrorStorageWrapper> wrappedCA;
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

    // reset the test application
    ASSERT_EQ(ER_OK, storage->ResetApplication(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_WILL_RESET));

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE));
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

    // change security configuration
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    ASSERT_EQ(ER_OK, storage->InstallMembership(lastAppInfo, groupInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(CheckSyncState(SYNC_PENDING)); // app was offline

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_TRUE(CheckSyncState(SYNC_OK));
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

    // change security configuration
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    vector<GroupInfo> groups;
    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, policy));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(CheckSyncState(SYNC_PENDING)); // app was offline

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_TRUE(CheckSyncState(SYNC_OK));
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
TEST_F(ApplicationUpdaterTests, ResetPolicy) {
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

    // reset the policy
    ASSERT_EQ(ER_OK, storage->RemovePolicy(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(CheckSyncState(SYNC_PENDING)); // app was offline

    // restart the test application and check for default policy
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_TRUE(CheckSyncState(SYNC_OK));
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

    // change security configuration
    IdentityInfo identityInfo2;
    identityInfo2.name = "Updated test name";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(identityInfo2));
    ASSERT_EQ(ER_OK, storage->UpdateIdentity(lastAppInfo, identityInfo2, aa.lastManifest));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(CheckSyncState(SYNC_PENDING)); // app was offline

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_TRUE(CheckSyncState(SYNC_OK));
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
    ASSERT_EQ(ER_OK, storage->UpdateIdentity(lastAppInfo, identityInfo2, aa.lastManifest));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(CheckSyncState(SYNC_PENDING)); // app was offline

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));
    ASSERT_TRUE(CheckSyncState(SYNC_OK));
    ASSERT_TRUE(CheckPolicy(policy));
    vector<GroupInfo> memberships;
    memberships.push_back(groupInfo);
    ASSERT_TRUE(CheckMemberships(memberships));

    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());

    // reset the test application
    ASSERT_EQ(ER_OK, storage->ResetApplication(lastAppInfo));

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE));
}

/**
 * @test Make sure resetting of an application fails, and check if a sync error
 *       of type SYNC_ER_RESET is triggered.
 *       -# Claim the remote application.
 *       -# Update the policy with a different admin group.
 *       -# Stop the remote application.
 *       -# Remove the application from the CA.
 *       -# Restart the remote application.
 *       -# Check if a sync error of type SYNC_ER_RESET is triggered.
 **/
TEST_F(ApplicationUpdaterTests, SyncErReset) {
    // install invalid policy
    GroupInfo invalidAdminGroup;
    PolicyGenerator invalidPolicyGenerator(invalidAdminGroup);
    PermissionPolicy invalidPolicy;
    vector<GroupInfo> invalidGuilds;
    invalidPolicyGenerator.DefaultPolicy(invalidGuilds, invalidPolicy);
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, invalidPolicy));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));
    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());

    // reset the test application
    ASSERT_EQ(ER_OK, storage->ResetApplication(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_WILL_RESET));

    // restart the remote application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForSyncError(SYNC_ER_RESET, ER_AUTH_FAIL));

    // check remote state
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_WILL_RESET));
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
TEST_F(ApplicationUpdaterTests, SyncErPolicy) {
    // install a policy
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    vector<GroupInfo> groups;
    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    policy.SetVersion(42);
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, policy));
    ASSERT_TRUE(WaitForUpdatesCompleted());

    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());

    // make sure wrapper returns an older policy
    PermissionPolicy olderPolicy = policy;
    olderPolicy.SetVersion(1);
    wrappedCA->SetPolicy(olderPolicy);

    // start the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForSyncError(SYNC_ER_POLICY, ER_POLICY_NOT_NEWER));

    // check remote state
    wrappedCA->UnsetPolicy();
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED));
    ASSERT_TRUE(CheckPolicy(policy));
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
TEST_F(ApplicationUpdaterTests, SyncErIdentity) {
    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());

    // make sure wrapper will return a different manifest
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("this.should.never.match*");
    PermissionPolicy::Rule::Member*  prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(1, prms);
    Manifest testManifest(rules, 1);
    delete[] rules;
    rules = nullptr;
    delete[] prms;
    prms = nullptr;
    wrappedCA->SetManifest(testManifest);

    // change security configuration
    IdentityInfo identityInfo2;
    identityInfo2.name = "Updated test name";
    ASSERT_EQ(ER_OK, storage->StoreIdentity(identityInfo2));
    ASSERT_EQ(ER_OK, storage->UpdateIdentity(lastAppInfo, identityInfo2, aa.lastManifest));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(CheckSyncState(SYNC_PENDING)); // app was offline

    // start the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForSyncError(SYNC_ER_IDENTITY, ER_DIGEST_MISMATCH));

    // check remote state
    wrappedCA->UnsetManifest();
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    IdentityCertificate remoteIdCert;
    Manifest remoteManifest;
    ASSERT_TRUE(CheckRemoteIdentity(idInfo, aa.lastManifest, remoteIdCert, remoteManifest));
}

/**
 * @test Install a membership certificate of an application with an invalid
 *       certificate, and check whether a sync error of type SYNC_ER_MEMBERSHIP
 *       is triggered.
 *       -# Claim the remote application and stop it.
 *       -# Make sure CAStorage returns an invalid certificate (e.g.
 *          empty guid).
 *       -# Restart the remote application.
 *       -# Check if a sync error of type SYNC_ER_MEMBERSHIP is triggered.
 **/
TEST_F(ApplicationUpdaterTests, SyncErMembership) {
    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());

    // make sure wrapper returns empty membership certificates
    wrappedCA->returnEmptyMembershipCert = true;

    // change security configuration
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    ASSERT_EQ(ER_OK, storage->InstallMembership(lastAppInfo, groupInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(CheckSyncState(SYNC_PENDING)); // app was offline

    // restart the test application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForSyncError(SYNC_ER_MEMBERSHIP, ER_FAIL));

    // check remote state
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    vector<GroupInfo> memberships;
    ASSERT_TRUE(CheckMemberships(memberships));
}

/**
 * @test Stop the CAStorage, and make sure the application updater starts
 *       notifying its listeners of some SYNC_ER_STORAGE errors when updating
 *       an application.
 *       -# Claim the remote application and stop it.
 *       -# Stop the CAStorage layer (or make sure that some basic functions
 *          like GetManagedApplication start returning errors).
 *       -# Restart the remote application.
 *       -# Check if a sync error of type SYNC_ER_STORAGE is triggered.
 **/
TEST_F(ApplicationUpdaterTests, SyncErStorage) {
    // stop the test application
    ASSERT_EQ(ER_OK, testApp.Stop());

    // configure ca to fail
    wrappedCA->failOnStartUpdates = true;

    // restart the remote application
    ASSERT_EQ(ER_OK, testApp.Start());
    ASSERT_TRUE(WaitForSyncError(SYNC_ER_STORAGE, ER_FAIL));
}
}
