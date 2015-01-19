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

namespace secmgrcoretest_unit_nominaltests {
using namespace secmgrcoretest_unit_testutil;

using namespace ajn::securitymgr;

class ClaimingCoreTests :
    public ClaimTest {
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

    /* Start the stub */
    Stub* stub = new Stub(&tcl);

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));

    /* Create identity */
    IdentityInfo idInfo;
    idInfo.guid = GUID128("abcdef123456789");
    idInfo.name = "TestIdentity";
    ASSERT_EQ(secMgr->StoreIdentity(idInfo, false), ER_OK);

    /* Claim */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING));

    /* Try to claim again */
    ASSERT_EQ(ER_PERMISSION_DENIED, secMgr->Claim(lastAppInfo, idInfo));

    /* Clear the keystore of the stub */
    stub->Reset();

    /* Stop the stub */
    delete stub;

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));
}

TEST_F(ClaimingCoreTests, SuccessfulClaimApplication) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);

    /* Start the stub */
    Stub* stub = new Stub(&tcl);

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));

    /* Create identity */
    IdentityInfo idInfo;
    idInfo.guid = GUID128("abcdef123456789");
    idInfo.name = "TestIdentity";
    ASSERT_EQ(secMgr->StoreIdentity(idInfo, false), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->ClaimApplication(lastAppInfo, idInfo, &AutoAcceptManifest));

    /* Check security signal */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING));

    /* Try to claim again */
    ASSERT_EQ(ER_PERMISSION_DENIED, secMgr->ClaimApplication(lastAppInfo, idInfo, &AutoAcceptManifest));

    /* Clear the keystore of the stub */
    stub->Reset();

    /* Stop the stub */
    delete stub;

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));
}
} // namespace
