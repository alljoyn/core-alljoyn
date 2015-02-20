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

using namespace ajn;
using namespace ajn::securitymgr;
using namespace secmgrcoretest_unit_testutil;

namespace secmgrcoretest_unit_nominaltests {
class ResetCoreTests :
    public BasicTest {
  private:

  protected:

  public:
    ResetCoreTests()
    {
    }
};

TEST_F(ResetCoreTests, SuccessfulReset) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);

    Stub* stub = new Stub(&tcl);
    ASSERT_TRUE(WaitForState(PermissionConfigurator::STATE_CLAIMABLE, STATE_RUNNING));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(secMgr->StoreIdentity(idInfo), ER_OK);

    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::STATE_CLAIMED, STATE_RUNNING));

    stub->SetDSASecurity(true);

    ASSERT_EQ(ER_OK, secMgr->Reset(lastAppInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::STATE_CLAIMABLE, STATE_RUNNING));

    stub->SetDSASecurity(false);

    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::STATE_CLAIMED, STATE_RUNNING));

    delete stub;
    ASSERT_TRUE(WaitForState(PermissionConfigurator::STATE_CLAIMED, STATE_NOT_RUNNING));
}
} // namespace
