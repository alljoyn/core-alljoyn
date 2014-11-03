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

class MembershipCoreTests :
    public ClaimTest {
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

    /* Start the stub */
    Stub* stub = new Stub(&tcl);

    /* Wait for signals */
    sem_wait(&sem); // security signal
    if (tal->_lastAppInfo.appName.size() == 0) {
        sem_wait(&sem); // about signal
    } else {
        sem_trywait(&sem);
    }

    ASSERT_EQ(ajn::securitymgr::STATE_RUNNING, tal->_lastAppInfo.runningState);
    ASSERT_EQ(ajn::PermissionConfigurator::STATE_CLAIMABLE, tal->_lastAppInfo.claimState);

    /* Create identity */
    ASSERT_EQ(secMgr->StoreIdentity(idInfo, false), ER_OK);

    /* Claim application */
    ASSERT_EQ(ER_OK, secMgr->ClaimApplication(tal->_lastAppInfo, idInfo, &AutoAcceptManifest));

    /* Check security signal */
    sem_wait(&sem);
    ASSERT_EQ(ajn::securitymgr::STATE_RUNNING, tal->_lastAppInfo.runningState);
    ASSERT_EQ(ajn::PermissionConfigurator::STATE_CLAIMED, tal->_lastAppInfo.claimState);

    ApplicationInfo appInfo = tal->_lastAppInfo;

    ASSERT_EQ(secMgr->StoreGuild(guildInfo1, false), ER_OK);
    ASSERT_EQ(secMgr->StoreGuild(guildInfo2, false), ER_OK);

    ASSERT_EQ(secMgr->InstallMembership(appInfo, guildInfo1), ER_OK);
    ASSERT_EQ(secMgr->InstallMembership(appInfo, guildInfo2), ER_OK);

    // ASSERT_EQ(secMgr->RemoveMembership(appInfo, guildInfo1), ER_OK);
    // ASSERT_EQ(secMgr->RemoveMembership(appInfo, guildInfo2), ER_OK);

    /* Clear the keystore of the stub */
    stub->Reset();

    /* Stop the stub */
    delete stub;

    while (sem_trywait(&sem) == 0) {
        ;
    }
    sem_wait(&sem);

    ASSERT_EQ(ajn::securitymgr::STATE_NOT_RUNNING, tal->_lastAppInfo.runningState);
    ASSERT_EQ(ajn::PermissionConfigurator::STATE_CLAIMED, tal->_lastAppInfo.claimState);
}
} // namespace
