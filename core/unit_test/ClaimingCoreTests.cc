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

namespace secmgrcoretest_unit_nominaltests {
using namespace secmgrcoretest_unit_testutil;

using namespace ajn::securitymgr;

class AutoRejector :
    public ManifestListener {
    bool ApproveManifest(const ApplicationInfo& appInfo,
                         const PermissionPolicy::Rule* manifestRules,
                         const size_t manifestRulesCount)
    {
        return false;
    }
};

class ClaimingCoreTests :
    public BasicTest {
  private:

  protected:

  public:
    ClaimingCoreTests()
    {
    }
};

TEST_F(ClaimingCoreTests, SuccessfulClaim) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);

    /* Check that the app is not there yet */
    ASSERT_EQ(ER_END_OF_DATA, secMgr->GetApplication(lastAppInfo));

    /* Start the stub */
    Stub* stub = new Stub(&tcl);

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));

    /* Create identity */
    IdentityInfo idInfo;
    idInfo.guid = GUID128("abcdef123456789");
    idInfo.name = "TestIdentity";
    ASSERT_EQ(secMgr->StoreIdentity(idInfo), ER_OK);

    /* Check root of trust */
    ASSERT_EQ((size_t)0, lastAppInfo.rootsOfTrust.size());

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING));

    /* Check root of trust */
    ASSERT_EQ((size_t)1, lastAppInfo.rootsOfTrust.size());
    ECCPublicKey secMgrPubKey = secMgr->GetPublicKey();
    ASSERT_TRUE(secMgrPubKey == lastAppInfo.rootsOfTrust[0]);

    /* Try to claim again */
    ASSERT_EQ(ER_PERMISSION_DENIED, secMgr->Claim(lastAppInfo, idInfo));

    /* Clear the keystore of the stub */
    stub->Reset();

    /* Stop the stub */
    delete stub;

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));
}

/**
 * \test Reject the manifest during claiming and check whether the application
 *       becomes CLAIMABLE again.
 *       -# Claim the remote application.
 *       -# Reject the manifest.
 *       -# Check whether the application becomes CLAIMABLE again.
 * */

TEST_F(ClaimingCoreTests, RejectManifest) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);
    Stub stub(&tcl);

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(secMgr->StoreIdentity(idInfo), ER_OK);

    AutoRejector ar;
    secMgr->SetManifestListener(&ar);

    ASSERT_EQ(ER_MANIFEST_REJECTED, secMgr->Claim(lastAppInfo, idInfo));

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));
    secMgr->SetManifestListener(NULL);
}

/**
 * \test Set the user defined name of an application and check whether it can be retrieved.
 *       -# Claim the remote application.
 *       -# Set a user defined name.
 *       -# Retrieve the application info from the security manager.
 *       -# Check whether the retrieved user defined name matches the one that was set.
 * */

TEST_F(ClaimingCoreTests, SetApplicationName) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);
    Stub stub(&tcl);

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(secMgr->StoreIdentity(idInfo), ER_OK);

    ASSERT_EQ(ER_END_OF_DATA, secMgr->SetApplicationName(lastAppInfo));

    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING));

    qcc::String userDefinedName = "User-defined test name";
    lastAppInfo.userDefinedName = userDefinedName;
    ASSERT_EQ(ER_OK, secMgr->SetApplicationName(lastAppInfo));

    ApplicationInfo appInfo;
    appInfo.busName = lastAppInfo.busName;
    ASSERT_EQ(ER_OK, secMgr->GetApplication(appInfo));
    printf("udn = %s\n", appInfo.userDefinedName.c_str());
    ASSERT_TRUE(appInfo.userDefinedName == userDefinedName);
}
} // namespace
