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
#include <qcc/Thread.h>

using namespace ajn;
using namespace ajn::securitymgr;

/** @file MultiAgentAppTests.cc */

namespace secmgr_tests {
class TApplicationListener :
    public ApplicationListener {
    void OnApplicationStateChange(const OnlineApplication* old,
                                  const OnlineApplication* updated)
    {
        cout << "AGENT2>> Old Application info = " << (old == nullptr ? "null" : old->ToString().c_str()) << endl;
        cout << "AGENT2>> New Application info = " <<
        (updated == nullptr ? "null" : updated->ToString().c_str()) << endl;
    }

    void OnSyncError(const SyncError* syncError)
    {
        QCC_UNUSED(syncError);
    }

    void OnManifestUpdate(const ManifestUpdate* manifestUpdate)
    {
        QCC_UNUSED(manifestUpdate);
    }
};

class MultiAgentAppTests :
    public BasicTest {
  public:
    bool CheckAgentAppState(
        const PermissionConfigurator::ApplicationState applicationState = PermissionConfigurator::CLAIMABLE,
        ApplicationSyncState state = SYNC_UNMANAGED)
    {
        for (size_t i = 0; i < apps.size(); i++) {
            OnlineApplication app2 = apps[i];
            if (ER_OK != agent2->GetApplication(app2)) {
                return false;
            }
            if (state != app2.syncState) {
                return false;
            }
            if (applicationState != app2.applicationState) {
                return false;
            }
            OnlineApplication app = apps[i];
            if (ER_OK != secMgr->GetApplication(app)) {
                return false;
            }
            if (state != app.syncState) {
                return false;
            }
            if (applicationState != app.applicationState) {
                return false;
            }
        }
        return true;
    }

    bool ClaimApplications()
    {
        IdentityInfo info;
        info.name = "testAppName";
        QStatus status = storage->StoreIdentity(info);
        EXPECT_EQ(ER_OK, status);
        if (ER_OK == status) {
            for (size_t i = 0; i < apps.size(); i++) {
                status = agent2->Claim(apps[i], info);
                EXPECT_EQ(ER_OK, status) << "Loop  " << i;
                if (ER_OK != status) {
                    return status;
                }
                bool result = WaitForState(apps[i], PermissionConfigurator::CLAIMED);
                EXPECT_TRUE(result) << "Loop " << i;
                if (!result) {
                    return false;
                }
            }
        }
        return ER_OK == status;
    }

    void SetUp()
    {
        BasicTest::SetUp();
        InitSecAgent();
        ASSERT_EQ(ER_OK, ba2.Start());
        ASSERT_EQ(ER_OK, ba2.Connect());
        if (ER_OK != ba2.WhoImplements(nullptr)) {
            printf("WhoImplements nullptr failed.\n");
        }

        ASSERT_EQ(ER_OK, storage->GetCaStorage(caStorage));
        ASSERT_EQ(ER_OK, SecurityAgentFactory::GetInstance().GetSecurityAgent(caStorage, agent2, &ba2));
        agent2->RegisterApplicationListener(&tal2);
        agent2->SetClaimListener(&aa2);
        for (size_t i = 0; i < 3; i++) {
            string appname = "testapp";
            char buf[] = { (char)('0' + ((char)i)), 0 };
            appname.append(buf);
            TestApplication* testAppP = new TestApplication(appname);
            shared_ptr<TestApplication> testapp = shared_ptr<TestApplication>(testAppP);
            ASSERT_EQ(ER_OK, testapp->Start());
            OnlineApplication app;
            ASSERT_EQ(ER_OK, GetPublicKey(*testAppP, app));
            ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
            testapps.push_back(testapp);
            apps.push_back(app);
        }
        qcc::Sleep(1250);
    }

    void TearDown()
    {
        //Clean up the agents first. Make sure all updates are done before destructing the applications.
        //Destructing the applications first can lead to 2 minute time-outs.
        cout << __FILE__ << ".TearDown: " << "stopping agent 2" << endl;
        agent2 = nullptr;
        DefaultECDHEAuthListener dal;
        ba2.EnablePeerSecurity(ECDHE_KEYX, &dal); //keep bus2 available
        cout << __FILE__ << ".TearDown: " << "stopping main agent" << endl;
        RemoveSecAgent();
        cout << __FILE__ << ".TearDown: " << "Cleaning up ba2" << endl;
        ba2.Disconnect();
        ba2.EnablePeerSecurity("", nullptr);
        ba2.Stop();
        ba2.ClearKeyStore();
        ba2.Join();
        cout << __FILE__ << ".TearDown: " << "Cleaning up the test apps" << endl;
        testapps.clear();
        caStorage = nullptr;
        cout << __FILE__ << ".TearDown: " << "Calling BasicTest::TearDown()" << endl;
        BasicTest::TearDown();
    }

    MultiAgentAppTests() : ba2("agent2_bus"), agent2(nullptr), caStorage(nullptr),
        tal2()
    {
    }

    BusAttachment ba2;
    shared_ptr<SecurityAgent> agent2;
    shared_ptr<AgentCAStorage> caStorage;
    vector<shared_ptr<TestApplication> > testapps;
    vector<OnlineApplication> apps;
    TApplicationListener tal2;
    AutoAccepter aa2;
};

/**
 * @test Verify that where multi-agents are active, the applications
 *       needing to be claimed and be managed are dealt with consistently.
 *       -# Start an number of security agents which share the same storage.
 *       -# Start a number of applications and make sure that all agents
 *          see those applications in the CLAIMABLE state.
 *       -# Let the security agents start claiming claimable
 *          applications.
 *       -# Verify that all applications have been claimed successfully.
 *       -# Let the security agents start unclaiming claimed
 *          applications.
 *       -# Verify that all applications have changed state to CLAIMABLE
 **/
TEST_F(MultiAgentAppTests, DISABLED_SuccessfulClaimAndUnclaimManyApps) { //See ASACORE-2576
    vector<OnlineApplication> agent2Apps;
    ASSERT_EQ(ER_OK, agent2->GetApplications(agent2Apps));
    ASSERT_EQ(apps.size(), agent2Apps.size());
    ASSERT_TRUE(CheckAgentAppState(PermissionConfigurator::CLAIMABLE, SYNC_UNMANAGED));
    ASSERT_TRUE(ClaimApplications());

    for (size_t i = 0; i < apps.size(); i++) {
        ASSERT_EQ(ER_OK, storage->ResetApplication(apps[i]));
    }

    for (size_t i = 0; i < apps.size(); i++) {
        ASSERT_TRUE(WaitForState(apps[i], PermissionConfigurator::CLAIMABLE));
    }
    ASSERT_TRUE(CheckAgentAppState(PermissionConfigurator::CLAIMABLE, SYNC_UNMANAGED));
}

/**
 * @test Verify that where multi-agents are active, claimed applications
 *       can be updated correctly after a restart.
 *       -# Start an number of security agents which share the same storage.
 *       -# Start a number of applications and make sure that all agents
 *          see those applications in the CLAIMABLE state.
 *       -# Claim successfully all applications...could be using
 *          one security agent.
 *       -# Verify that all applications have been claimed successfully.
 *       -# Stop all claimed applications.
 *       -# Make sure all security agents see all applications as off-line
 *          (i.e., no busName).
 *       -# Update all applications with predefined membership and policy
 *       -# Restart all the applications.
 *       -# Verify that all applications have been updated correctly and
 *          that no sync errors have been encountered.
 *       -# Verify all applications are in the CLAIMED state.
 **/
TEST_F(MultiAgentAppTests, DISABLED_SuccessfulUpdateManyApps) { //See ASACORE-2576
    ASSERT_TRUE(ClaimApplications());

    // Stop all applications.
    for (size_t i = 0; i < testapps.size(); i++) {
        ASSERT_EQ(ER_OK, testapps[i]->Stop());
    }

    GroupInfo group;
    group.name = "testgroup";
    ASSERT_EQ(ER_OK, storage->StoreGroup(group));

    vector<GroupInfo> groups;
    groups.push_back(group);
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));

    for (size_t i = 0; i < testapps.size(); i++) {
        ASSERT_EQ(ER_OK, storage->InstallMembership(apps[i], group));
        ASSERT_EQ(ER_OK, storage->UpdatePolicy(apps[i], policy));
        ASSERT_TRUE(WaitForState(apps[i], PermissionConfigurator::CLAIMED, SYNC_PENDING));
    }

    CheckAgentAppState(PermissionConfigurator::CLAIMED, SYNC_PENDING);

    qcc::Sleep(500);

    for (size_t i = 0; i < testapps.size(); i++) {
        ASSERT_EQ(ER_OK, testapps[i]->Start());
    }

    for (size_t i = 0; i < testapps.size(); i++) {
        ASSERT_TRUE(WaitForState(apps[i], PermissionConfigurator::CLAIMED, SYNC_OK));
    }
    CheckAgentAppState(PermissionConfigurator::CLAIMED, SYNC_OK);
}

/**
 * @test Verify that a restarted Security Agent (SA) after claiming many
 *       applications will maintain a consistent view.
 *       -# Start a number of applications and make sure they are claimable.
 *       -# Start 2 SAs and verify that it could see all
 *          the online applications in a CLAIMABLE state.
 *       -# Ask a SA to start claiming (at no specific order) all
 *          claimable applications.
 *       -# Verify that all applications have been claimed; state change to
 *          CLAIMED.
 *       -# Kill the SAs.
 *       -# Install/update all claimed applications with a
 *          predefined membership certificate and policy.
 *       -# Start the SA again and make sure that all previously
 *          claimed applications are in a consistent online state and
 *          no sync errors will have occurred.
 **/
TEST_F(MultiAgentAppTests, DISABLED_SuccessfulRestartSecAgentWithManyApps) { //See ASACORE-2576
    ASSERT_TRUE(ClaimApplications());
    agent2 = nullptr;
    RemoveSecAgent();
    // Stop all applications.
    for (size_t i = 0; i < testapps.size(); i++) {
        ASSERT_EQ(ER_OK, testapps[i]->Stop());
    }

    GroupInfo group;
    group.name = "testgroup";
    ASSERT_EQ(ER_OK, storage->StoreGroup(group));

    Util::Init(ba);
    vector<GroupInfo> groups;
    groups.push_back(group);
    PermissionPolicy policy;
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, policy));

    for (size_t i = 0; i < apps.size(); i++) {
        ASSERT_EQ(ER_OK, storage->InstallMembership(apps[i], group));
        ASSERT_EQ(ER_OK, storage->UpdatePolicy(apps[i], policy)) << apps[i].busName << " loop " << i;
    }

    // Stop all security managers.
    for (size_t i = 0; i < testapps.size(); i++) {
        ASSERT_EQ(ER_OK, testapps[i]->Start());
    }

    cout << endl << endl << "Restarting managers" << endl << endl << endl;

    ASSERT_EQ(ER_OK, ba2.Disconnect());
    ASSERT_EQ(ER_OK, ba2.Connect());
    ASSERT_EQ(ER_OK, SecurityAgentFactory::GetInstance().GetSecurityAgent(caStorage, agent2, &ba2));
    agent2->RegisterApplicationListener(&tal2);
    InitSecAgent();

    for (size_t i = 0; i < testapps.size(); i++) {
        ASSERT_TRUE(WaitForState(apps[i], PermissionConfigurator::CLAIMED, SYNC_OK));
    }
    CheckAgentAppState(PermissionConfigurator::CLAIMED, SYNC_OK);
}
}
