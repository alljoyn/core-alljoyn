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

class MembershipCoreTests :
    public BasicTest {
  private:

  protected:

  public:
    MembershipCoreTests()
    {
        idInfo.guid = GUID128();
        idInfo.name = "TestIdentity";

        guildInfo1.guid = GUID128();
        guildInfo1.name = "MyGuild 1";
        guildInfo1.desc = "My test guild 1 description";

        guildInfo2.guid = GUID128();
        guildInfo2.name = "MyGuild 2";
        guildInfo2.desc = "My test guild 2 description";
    }

    IdentityInfo idInfo;
    GuildInfo guildInfo1;
    GuildInfo guildInfo2;
};

TEST_F(MembershipCoreTests, SuccessfulInstallMembership) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);

    /* Create guilds */
    ASSERT_EQ(ER_OK, secMgr->StoreGuild(guildInfo1));
    ASSERT_EQ(ER_OK, secMgr->StoreGuild(guildInfo2));

    /* Start the stub */
    Stub* stub = new Stub(&tcl);

    /* Wait for signals */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));

    /* Installing or removing membership before claiming should fail */
    ApplicationInfo appInfo = lastAppInfo;
    ASSERT_EQ(ER_END_OF_DATA, secMgr->InstallMembership(appInfo, guildInfo2)); // fails due to manifest missing in persistency
    ASSERT_NE(ER_OK, secMgr->RemoveMembership(appInfo, guildInfo2)); // fails due to certificate missing in persistency

    /* Create identity */
    ASSERT_EQ(secMgr->StoreIdentity(idInfo), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));

    stub->SetDSASecurity(true);

    /* Check security signal */
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING));

    ASSERT_EQ(ER_OK, secMgr->InstallMembership(appInfo, guildInfo1));
    ASSERT_EQ(ER_OK, secMgr->InstallMembership(appInfo, guildInfo2));

    ASSERT_EQ(ER_OK, secMgr->RemoveMembership(appInfo, guildInfo1));
    ASSERT_EQ(ER_OK, secMgr->RemoveMembership(appInfo, guildInfo2));

    /* Clear the keystore of the stub */
    stub->Reset();

    /* Stop the stub */
    delete stub;

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));
}
} // namespace
