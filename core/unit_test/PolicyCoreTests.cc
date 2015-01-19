/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#include <semaphore.h>
#include <PolicyGenerator.h>

namespace secmgrcoretest_unit_nominaltests {
using namespace secmgrcoretest_unit_testutil;

using namespace ajn::securitymgr;

class PolicyCoreTests :
    public ClaimTest {
  private:

  protected:

  public:
    PolicyCoreTests()
    {
        idInfo.guid = GUID128();
        idInfo.name = "TestIdentity";
    }

    IdentityInfo idInfo;

    GUID128 guild;
    GUID128 guild2;
    PermissionPolicy policy;
    PermissionPolicy policy2;
};

TEST_F(PolicyCoreTests, SuccessfulInstallPolicy) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);

    vector<GUID128> policyGuilds;
    policyGuilds.push_back(guild);
    PolicyGenerator::DefaultPolicy(policyGuilds, secMgr->GetPublicKey(), policy);
    policyGuilds.push_back(guild2);
    PolicyGenerator::DefaultPolicy(policyGuilds, secMgr->GetPublicKey(), policy2);

    /* Start the stub */
    Stub* stub = new Stub(&tcl);

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));

    /* Installing/retrieving policy before claiming should fail */
    ApplicationInfo appInfo = lastAppInfo;
    PermissionPolicy policyRemote;
    PermissionPolicy policyLocal;
    ASSERT_EQ(ER_PERMISSION_DENIED, secMgr->InstallPolicy(appInfo, policy));
    ASSERT_EQ(ER_PERMISSION_DENIED, secMgr->InstallPolicy(appInfo, policy2));
    ASSERT_EQ(ER_PERMISSION_DENIED, secMgr->GetPolicy(appInfo, policyRemote, true));
    ASSERT_EQ(ER_FAIL, secMgr->GetPolicy(appInfo, policyLocal, false));

    /* Create identity */
    ASSERT_EQ(secMgr->StoreIdentity(idInfo, false), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->ClaimApplication(lastAppInfo, idInfo, &AutoAcceptManifest));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING));

    /* Check default policy */
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, secMgr->GetPolicy(appInfo, policyRemote, true));
    ASSERT_EQ(ER_OK, secMgr->GetPolicy(appInfo, policyLocal, false));

    /* Install policy and check retrieved policy */
    ASSERT_EQ(ER_OK, secMgr->InstallPolicy(appInfo, policy));
    ASSERT_EQ(ER_OK, secMgr->GetPolicy(appInfo, policyRemote, true));
    ASSERT_EQ(ER_OK, secMgr->GetPolicy(appInfo, policyLocal, false));
    ASSERT_EQ((size_t)1, policyRemote.GetTermsSize());
    ASSERT_EQ((size_t)1, policyLocal.GetTermsSize());

    /* Install another policy and check retrieved policy */
    ASSERT_EQ(ER_OK, secMgr->InstallPolicy(appInfo, policy2));
    ASSERT_EQ(ER_OK, secMgr->GetPolicy(appInfo, policyRemote, true));
    ASSERT_EQ(ER_OK, secMgr->GetPolicy(appInfo, policyLocal, false));
    ASSERT_EQ((size_t)2, policyRemote.GetTermsSize());
    ASSERT_EQ((size_t)2, policyLocal.GetTermsSize());

    /* Clear the keystore of the stub */
    stub->Reset();

    /* Stop the stub */
    delete stub;

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));
}
} // namespace
