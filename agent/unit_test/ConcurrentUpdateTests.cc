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
#include "TestUtil.h"

#include <qcc/GUID.h>
#include <qcc/String.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/Status.h>

#include <alljoyn/securitymgr/PolicyGenerator.h>

using namespace secmgrcoretest_unit_testutil;

enum Action {
    NOTHING,
    RESET,
    MEMBERSHIP,
    POLICY,
    MULTI
};

class CCAgentStorageWrapper :
    public DefaultAgentStorageWrapper {
  public:
    CCAgentStorageWrapper(shared_ptr<AgentCAStorage>& _ca, shared_ptr<UIStorage>& _storage)
        : DefaultAgentStorageWrapper(_ca), action(NOTHING), storage(_storage), locked(false) { }

    QStatus UpdatesCompleted(Application& app, uint64_t& updateID)
    {
        QStatus status = ER_OK;
        switch (action) {
        case RESET:
            status = storage->RemoveApplication(app);
            action = NOTHING;
            break;

        case MEMBERSHIP:
            status = storage->InstallMembership(app, group);
            action = NOTHING;
            break;

        case POLICY:
            status = storage->UpdatePolicy(app, policy);
            action = NOTHING;
            break;

        case MULTI:
            status = storage->InstallMembership(app, group);
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
 * \test Reset an application during a policy update and check its claimable state.
 *       -# Reset the application using the security agent while update the policy
 *       -# Check whether the remote application is CLAIMABLE.
 **/

TEST_F(ConcurrentUpdateTests, ResetAfterPolicy) {
    //Schedule reset.
    wrappedCa->SetAction(lastAppInfo, RESET);
    wrappedCa->BlockNothingAction();
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    vector<GroupInfo> groups;
    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, policy));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, true));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, true));
    wrappedCa->UnblockNothingAction();
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true, false));
    ASSERT_TRUE(CheckUpdatesPending(false));
}

/**
 * \test Install a membership certificate for an application during an update policy
 *       -# Install a membership certificate using the security agent.
 *       -# check the result
 **/
TEST_F(ConcurrentUpdateTests, InstallMembershipAfterPolicy) {
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    wrappedCa->SetAction(lastAppInfo, groupInfo);
    vector<GroupInfo> groups;
    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, policy));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, true));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));

    ASSERT_TRUE(CheckUpdatesPending(false));
    vector<GroupInfo> memberships;
    memberships.push_back(groupInfo);
    ASSERT_TRUE(CheckRemotePolicy(policy));
    ASSERT_TRUE(CheckRemoteMemberships(memberships));
}

/**
 * \test Install a membership certificate for an application during an update policy
 *       -# Install a membership certificate using the security agent.
 *       -# check the result
 **/
TEST_F(ConcurrentUpdateTests, UpdatePolicyAfterPolicy) {
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    vector<GroupInfo> groups;
    PermissionPolicy p;
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, p));

    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));
    wrappedCa->SetAction(lastAppInfo, policy);

    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, p));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, true));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));

    ASSERT_TRUE(CheckUpdatesPending(false));
    policy.SetVersion(2);
    ASSERT_TRUE(CheckRemotePolicy(policy));
}
/**
 * \test Do multiple updates in a row
 *       -# Install a membership certificate using the security agent.
 *       -# check the result
 **/
TEST_F(ConcurrentUpdateTests, UpdateMultiple) {
    ASSERT_EQ(ER_OK, storage->StoreGroup(groupInfo));
    vector<GroupInfo> groups;
    PermissionPolicy p;
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, p));

    groups.push_back(groupInfo);
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));

    wrappedCa->SetAction(lastAppInfo, policy);
    wrappedCa->SetAction(lastAppInfo, groupInfo);
    wrappedCa->SetAction(lastAppInfo, MULTI);

    ASSERT_EQ(ER_OK, storage->UpdatePolicy(lastAppInfo, p));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, true));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true, false));

    ASSERT_TRUE(CheckUpdatesPending(false));

    policy.SetVersion(2);
    ASSERT_TRUE(CheckRemotePolicy(policy));

    vector<GroupInfo> memberships;
    memberships.push_back(groupInfo);
    ASSERT_TRUE(CheckRemoteMemberships(memberships));
}
