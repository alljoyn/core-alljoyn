/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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
#include <alljoyn/securitymgr/PolicyGenerator.h>

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

    GUID128 guildGUID;
    GUID128 guildGUID2;
    PermissionPolicy policy;
    PermissionPolicy policy2;
};

TEST_F(PolicyCoreTests, SuccessfulInstallPolicy) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);

    vector<GuildInfo> policyGuilds;
    GuildInfo guild;
    guild.guid = guildGUID;
    policyGuilds.push_back(guild);
    PolicyGenerator::DefaultPolicy(policyGuilds, policy);

    GuildInfo guild2;
    guild2.guid = guildGUID2;
    policyGuilds.push_back(guild2);
    PolicyGenerator::DefaultPolicy(policyGuilds, policy2);

    /* Start the stub */
    Stub* stub = new Stub(&tcl);

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));

    /* Installing/retrieving policy before claiming should fail */
    ApplicationInfo appInfo = lastAppInfo;
    PermissionPolicy policyLocal;
    ASSERT_NE(ER_OK, secMgr->UpdatePolicy(appInfo, policy));
    ASSERT_NE(ER_OK, secMgr->UpdatePolicy(appInfo, policy2));
    ASSERT_NE(ER_OK, secMgr->GetPolicy(appInfo, policyLocal));

    /* Create identity */
    ASSERT_EQ(secMgr->StoreIdentity(idInfo), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING));

    /* Check default policy */
    ASSERT_EQ(ER_END_OF_DATA, secMgr->GetPolicy(appInfo, policyLocal));

    /* Install policy and check retrieved policy */
    ASSERT_EQ(ER_OK, secMgr->UpdatePolicy(appInfo, policy));
    ASSERT_EQ(ER_OK, secMgr->GetPolicy(appInfo, policyLocal));
    ASSERT_EQ((size_t)1, policyLocal.GetTermsSize());

    /* Install another policy and check retrieved policy */
    ASSERT_EQ(ER_OK, secMgr->UpdatePolicy(appInfo, policy2));
    ASSERT_EQ(ER_OK, secMgr->GetPolicy(appInfo, policyLocal));
    ASSERT_EQ((size_t)2, policyLocal.GetTermsSize());

    /* Clear the keystore of the stub */
    stub->Reset();

    /* Stop the stub */
    delete stub;

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));
}
} // namespace
