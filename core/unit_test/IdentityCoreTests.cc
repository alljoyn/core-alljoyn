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

class IdentityCoreTests :
    public ClaimTest {
  private:

  protected:

  public:
    IdentityCoreTests()
    {
    }
};

TEST_F(IdentityCoreTests, SuccessfulInstallIdentity) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);

    /* Start the stub */
    Stub* stub = new Stub(&tcl);

    /* Wait for signals */
    sem_wait(&sem);
    if (tal->_lastAppInfo.appName.size() == 0) {
        sem_wait(&sem); // about signal
    } else {
        sem_trywait(&sem);
    }
    ASSERT_EQ(ajn::securitymgr::STATE_RUNNING, tal->_lastAppInfo.runningState);
    ASSERT_EQ(ajn::PermissionConfigurator::STATE_CLAIMABLE, tal->_lastAppInfo.claimState);

    IdentityInfo info;
    info.name = "MyName";
    ASSERT_EQ(ER_OK, secMgr->StoreIdentity(info, false));

    /* Claim! */
    ASSERT_EQ(ER_OK, secMgr->Claim(tal->_lastAppInfo, info));
    sem_wait(&sem);
    ASSERT_EQ(ajn::securitymgr::STATE_RUNNING, tal->_lastAppInfo.runningState);
    ASSERT_EQ(ajn::PermissionConfigurator::STATE_CLAIMED, tal->_lastAppInfo.claimState);

    /* Try to install identity again */
    //QCC_SetLogLevels("PERMISSION_MGMT=7");
    //QCC_SetLogLevels("CRYPTO=7");
    ASSERT_EQ(ER_OK, secMgr->InstallIdentity(tal->_lastAppInfo, info));

    qcc::IdentityCertificate certificate;
    ASSERT_EQ(ER_OK, secMgr->GetIdentityCertificate(tal->_lastAppInfo, certificate));

    printf("Retrieved id certificate = '%s'\n", certificate.GetPEM().c_str());

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
