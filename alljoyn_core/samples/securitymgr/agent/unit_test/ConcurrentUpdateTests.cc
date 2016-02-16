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
#include "TestUtil.h"
#include "AgentStorageWrapper.h"

#include <qcc/GUID.h>
#include <qcc/String.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/Status.h>

#include <alljoyn/securitymgr/PolicyGenerator.h>

/** @file ConcurrentUpdateTests.cc */

namespace secmgr_tests {
enum Action {
    NOTHING,
    RESET,
    MEMBERSHIP,
    POLICY,
    MULTI
};

class CCAgentStorageWrapper :
    public AgentStorageWrapper {
  public:
    CCAgentStorageWrapper(shared_ptr<AgentCAStorage>& _ca, shared_ptr<UIStorage>& _storage)
        : AgentStorageWrapper(_ca), action(NOTHING), storage(_storage), locked(false) { }

    QStatus UpdatesCompleted(Application& app, uint64_t& updateID)
    {
        QStatus status = ER_OK;
        Application copy = app; // to avoid impact on agent to storage feedback
        switch (action) {
        case RESET:
            status = storage->ResetApplication(copy);
            action = NOTHING;
            break;

        case MEMBERSHIP:
            status = storage->InstallMembership(copy, group);
            action = NOTHING;
            break;

        case POLICY:
            status = storage->UpdatePolicy(copy, policy);
            action = NOTHING;
            break;

        case MULTI:
            status = storage->InstallMembership(copy, group);
            action = POLICY;
            break;

        case NOTHING:
            //do nothing.
            lock.Lock();
            lock.Unlock();
            break;

        default:
            break;
        }
        if (status != ER_OK) {
            cerr << "Update action " << action << " failed "  << QCC_StatusText(status) << endl;
        }
        return ca->UpdatesCompleted(app, updateID);
    }

    void SetAction(Application _app, Action _action)
    {
        app = _app;
        action = _action;
    }

    void SetAction(Application _app, PermissionPolicy _policy)
    {
        app = _app;
        policy = _policy;
        action = POLICY;
    }

    void SetAction(Application _app, GroupInfo _group)
    {
        app = _app;
        group = _group;
        action = MEMBERSHIP;
    }

    void BlockNothingAction()
    {
        lock.Lock();
        locked = true;
    }

    void UnblockNothingAction()
    {
        if (locked) {
            locked = false;
            lock.Unlock();
        }
    }

  private:
    CCAgentStorageWrapper& operator=(const CCAgentStorageWrapper);

    Mutex lock;
    Application app;
    GroupInfo group;
    Action action;
    shared_ptr<UIStorage>& storage;
    PermissionPolicy policy;
    bool locked;
};

class ConcurrentUpdateTests :
    public ClaimedTest {
  public:
    ConcurrentUpdateTests() : wrappedCa(nullptr)
    {
        groupInfo.name = "Test";
        groupInfo.desc = "This is a test group";

        policyGroups.push_back(groupInfo.guid);
    }

    void TearDown()
    {
        wrappedCa->UnblockNothingAction();
        ClaimedTest::TearDown();
    }

    shared_ptr<AgentCAStorage>& GetAgentCAStorage()
    {
        wrappedCa = shared_ptr<CCAgentStorageWrapper>(new CCAgentStorageWrapper(ca, storage));
        ca = wrappedCa;
        return ca;
    }

  public:
    GroupInfo groupInfo;
    PermissionPolicy policy;
    vector<GUID128> policyGroups;
    shared_ptr<CCAgentStorageWrapper> wrappedCa;
};

/**
 * @test Reset an application while updating its policy and check whether it is
 *       CLAIMABLE.
 *       -# Claim an application.
 *       -# Update the policy of the application.
 *       -# While updating the policy, reset the application using the security
 *          agent.
 *       -# Check whether the application is CLAIMABLE.
 **/

TEST_F(ConcurrentUpdateTests, ResetAfterPolicy) {
    //Schedule reset.
    wrappedCa->SetAction(testAppInfo, RESET);
    wrappedCa->BlockNothingAction();
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    vector<GroupInfo> groups;
    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(testAppInfo, policy));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_WILL_RESET));
    wrappedCa->UnblockNothingAction();
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE));
}

/**
 * @test Install a membership certificate on an application while updating its
 *       policy and check whether both are installed correctly.
 *       -# Claim an application.
 *       -# Update the policy of the application.
 *       -# While updating the policy, store a membership certificate for the
 *          policy.
 *       -# Wait for the updates to complete.
 *       -# Check whether the policy was updated correctly.
 *       -# Check whether the membership was installed correctly.
 **/
TEST_F(ConcurrentUpdateTests, InstallMembershipAfterPolicy) {
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    wrappedCa->SetAction(testAppInfo, groupInfo);
    vector<GroupInfo> groups;
    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(testAppInfo, policy));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));

    ASSERT_TRUE(CheckSyncState(SYNC_OK));
    vector<GroupInfo> memberships;
    memberships.push_back(groupInfo);
    ASSERT_TRUE(CheckPolicy(policy));
    ASSERT_TRUE(CheckMemberships(memberships));
}

/**
 * @test Update the policy of an application while updating its policy and check
 *       whether the last policy is installed successfully.
 *       -# Claim an application.
 *       -# Update the policy of the application.
 *       -# While updating the policy, store another policy for the application.
 *       -# Wait for the updates to complete.
 *       -# Check whether the last policy is installed correctly.
 **/
TEST_F(ConcurrentUpdateTests, UpdatePolicyAfterPolicy) {
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    vector<GroupInfo> groups;
    PermissionPolicy p;
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, p));

    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    wrappedCa->SetAction(testAppInfo, policy);

    ASSERT_EQ(ER_OK, storage->UpdatePolicy(testAppInfo, p));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));

    ASSERT_TRUE(CheckSyncState(SYNC_OK));
    policy.SetVersion(2);
    ASSERT_TRUE(CheckPolicy(policy));
}

/**
 * @test Install a membership certificate on an application and update its
 *       policy while updating its policy, and check whether the last policy
 *       and the membership certificate have been installed successfully.
 *       -# Claim an application.
 *       -# Update the policy of the application.
 *       -# While updating the policy, install a membership certificate.
 *       -# While installing the membership certificate, update its policy.
 *       -# Wait for the updates to complete.
 *       -# Check whether the last policy is installed correctly.
 *       -# Check whether the membership certificate was installed correctly.
 **/
TEST_F(ConcurrentUpdateTests, UpdateMultiple) {
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    vector<GroupInfo> groups;
    PermissionPolicy p;
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, p));

    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));

    wrappedCa->SetAction(testAppInfo, policy);
    wrappedCa->SetAction(testAppInfo, groupInfo);
    wrappedCa->SetAction(testAppInfo, MULTI);

    ASSERT_EQ(ER_OK, storage->UpdatePolicy(testAppInfo, p));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_PENDING));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, SYNC_OK));

    ASSERT_TRUE(CheckSyncState(SYNC_OK));

    policy.SetVersion(2);
    ASSERT_TRUE(CheckPolicy(policy));

    vector<GroupInfo> memberships;
    memberships.push_back(groupInfo);
    ASSERT_TRUE(CheckMemberships(memberships));
}
}
