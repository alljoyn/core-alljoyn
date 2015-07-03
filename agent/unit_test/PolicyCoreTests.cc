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

namespace secmgrcoretest_unit_nominaltests {
using namespace secmgrcoretest_unit_testutil;

using namespace ajn::securitymgr;

class PolicyCoreTests :
    public BasicTest {
  private:

  protected:

  public:
    PolicyCoreTests()
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

TEST_F(PolicyCoreTests, SuccessfulInstallPolicy) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);

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

    /* Start the stub */
    stub = new Stub(&tcl);

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    /* Installing/retrieving policy before claiming should fail */
    Application app = lastAppInfo;
    PermissionPolicy policyLocal;
    ASSERT_NE(ER_OK, storage->UpdatePolicy(app, policy));
    ASSERT_NE(ER_OK, storage->UpdatePolicy(app, policy2));
    ASSERT_NE(ER_OK, storage->GetPolicy(app, policyLocal));

    OnlineApplication checkUpdatesPendingInfo;
    checkUpdatesPendingInfo.keyInfo = app.keyInfo;
    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_FALSE(checkUpdatesPendingInfo.updatesPending);

    /* Create identity */
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
    ASSERT_TRUE(CheckRemoteIdentity(idInfo, aa.lastManifest));

    /* Check default policy */
    ASSERT_EQ(ER_END_OF_DATA, storage->GetPolicy(app, policyLocal));

    /* Install policy and check retrieved policy */
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(app, policy));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_EQ(ER_OK, storage->GetPolicy(app, policyLocal));
    ASSERT_TRUE(CheckRemotePolicy(policy));
    ASSERT_TRUE(CheckRemotePolicy(policyLocal));

    /* Install another policy and check retrieved policy */
    ASSERT_EQ(ER_OK, storage->UpdatePolicy(app, policy2));
    ASSERT_TRUE(WaitForUpdatesCompleted());
    ASSERT_EQ(ER_OK, storage->GetPolicy(app, policyLocal));
    ASSERT_TRUE(CheckRemotePolicy(policy2));
    ASSERT_TRUE(CheckRemotePolicy(policyLocal));

    /* Clear the keystore of the stub */
    stub->Reset();

    /* Stop the stub */
    delete stub;
    stub = nullptr;
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, false));
}
} // namespace
