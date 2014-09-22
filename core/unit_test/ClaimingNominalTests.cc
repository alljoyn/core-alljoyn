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

#include "gtest/gtest.h"
#include "Common.h"
#include "SecurityManagerFactory.h"
#include "PermissionMgmt.h"
#include "TestUtil.h"
#include "Stub.h"
#include <semaphore.h>
#include <stdio.h>

/**
 * Several claiming nominal tests.
 */
namespace secmgrcoretest_unit_nominaltests {
using namespace secmgrcoretest_unit_testutil;

using namespace ajn::securitymgr;
using namespace std;

class ClaimingNominalTests :
    public ClaimTest {
  private:

  protected:

  public:
    ClaimingNominalTests()
    {
    }
};

/**
 * \test The test should verify that a factory-reset device not using an Out Of Band (OOB) secret
 *       can be successfully claimed using the security manager.
 *       -# Create a security manager and announce it.
 *       -# Create a stub client and make it claimable.
 *       -# Ask the security manager to see apps ready to be claimed.
 *       -# The discovered stub client can now be claimed, i.e., install ROT on it, generate
 *          and install Identity certificate.
 *       -# Make sure that the stub client has the right ROT and Identity certificate as well as
 *          verify that it was tracked by the security manager as a claimed application.
 * */
TEST_F(ClaimingNominalTests, SuccessfulClaimingWithoutOOB) {
    sem_t sem;
    sem_init(&sem, 0, 0);
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);
    TestApplicationListener tal(sem);

    secMgr->RegisterApplicationListener(&tal);

    Stub* stub = new Stub(&tcl);
    ASSERT_EQ(stub->GetInstalledIdentityCertificate(), "");
    sem_wait(&sem);
    ASSERT_EQ(tal._lastAppInfo.runningState, ApplicationRunningState::RUNNING);
    ASSERT_EQ(tal._lastAppInfo.claimState, ApplicationClaimState::NOT_CLAIMED);
    ASSERT_EQ(tal._lastAppInfo, secMgr->GetApplications()[0]);
    ASSERT_EQ(tal._lastAppInfo.rootOfTrustList.size(), (size_t)0);
    ASSERT_EQ(secMgr->GetApplications(ApplicationClaimState::CLAIMED).size(), (size_t)0);

    /* make sure we cannot claim yet */
    ASSERT_NE(secMgr->ClaimApplication(tal._lastAppInfo, &AutoAcceptManifest), ER_OK);
    ASSERT_EQ(tal._lastAppInfo.runningState, ApplicationRunningState::RUNNING);
    ASSERT_EQ(tal._lastAppInfo.claimState, ApplicationClaimState::NOT_CLAIMED);
    ASSERT_EQ(tal._lastAppInfo, secMgr->GetApplications()[0]);
    ASSERT_EQ(secMgr->GetApplications(ApplicationClaimState::CLAIMED).size(), (size_t)0);
    //ASSERT_EQ(secMgr->GetIdentityForApplication(_lastAppInfo), NULL);
    ASSERT_EQ(stub->GetRoTKeys().size(), (size_t)0);

    /* Open claim window */
    ASSERT_EQ(stub->OpenClaimWindow(), ER_OK);
    sem_wait(&sem);
    ASSERT_EQ(tal._lastAppInfo.runningState, ApplicationRunningState::RUNNING);
    ASSERT_EQ(tal._lastAppInfo.claimState, ApplicationClaimState::CLAIMABLE);
    ASSERT_EQ(tal._lastAppInfo, secMgr->GetApplications()[0]);
    ASSERT_EQ(secMgr->GetApplications(ApplicationClaimState::CLAIMED).size(), (size_t)0);
    //ASSERT_EQ(secMgr->GetIdentityForApplication(_lastAppInfo), NULL);
    ASSERT_EQ(stub->GetRoTKeys().size(), (size_t)0);

    /* Claim ! */
    ASSERT_EQ(secMgr->ClaimApplication(tal._lastAppInfo, &AutoAcceptManifest), ER_OK);
    sem_wait(&sem);
    ASSERT_EQ(tal._lastAppInfo.runningState, ApplicationRunningState::RUNNING);
    ASSERT_EQ(tal._lastAppInfo.claimState, ApplicationClaimState::CLAIMED);
    ASSERT_EQ(tal._lastAppInfo, secMgr->GetApplications()[0]);
    ASSERT_EQ(tal._lastAppInfo, secMgr->GetApplications(ApplicationClaimState::CLAIMED)[0]);
    ASSERT_EQ(tal._lastAppInfo.rootOfTrustList.size(), (size_t)1);
    qcc::ECCPublicKey pb = secMgr->GetRootOfTrust().GetPublicKey();
    printf("'%s'\n", tal._lastAppInfo.rootOfTrustList.at(0).ToString().c_str());
    char buf[sizeof(pb.data) * 2 + 1];
    char* ptr = &buf[0];
    for (unsigned int i = 0; i < sizeof(pb.data); i++, ptr += 2) {
        sprintf(ptr, "%02x", pb.data[i]);
    }
    printf("'%s'\n", qcc::String(buf).c_str());
    ASSERT_TRUE(tal._lastAppInfo.rootOfTrustList.at(0).ToString() ==  qcc::String(buf));
    //ASSERT_NE(secMgr->GetIdentityForApplication(_lastAppInfo), NULL);
    ASSERT_EQ(stub->GetRoTKeys().size(), (size_t)1);

    ASSERT_TRUE(0 ==
                memcmp(stub->GetRoTKeys()[0]->data, secMgr->GetRootOfTrust().GetPublicKey().data,
                       sizeof(secMgr->GetRootOfTrust().GetPublicKey().data)));

    ASSERT_NE(stub->GetInstalledIdentityCertificate(), "");

    /* make sure we cannot claim again */
    ASSERT_NE(secMgr->ClaimApplication(tal._lastAppInfo, &AutoAcceptManifest), ER_OK);

    /* Stop the stub */
    delete stub;
    sem_wait(&sem);
    ASSERT_EQ(tal._lastAppInfo.runningState, ApplicationRunningState::NOT_RUNNING);
    ASSERT_EQ(tal._lastAppInfo.claimState, ApplicationClaimState::CLAIMED);
    ASSERT_EQ(tal._lastAppInfo, secMgr->GetApplications()[0]);
    ASSERT_EQ(tal._lastAppInfo, secMgr->GetApplications(ApplicationClaimState::CLAIMED)[0]);

    sem_destroy(&sem);
}

/**
 * \test The test should verify that a factory-reset device not using an Out Of Band (OOB) secret
 *       can be successfully claimed using the security manager.
 *       -# Create a security manager and announce it with support of OOB credentials.
 *       -# Create a stub client and make it claimable with an OOB.
 *       -# Ask security manager to see apps ready to be claimed.
 *       -# The discovered stub client can now be claimed, i.e., install ROT on it, generate
 *          and install Identity certificate.
 *       -# Make sure that the stub client has the right ROT and Identity certificate as well as
 *          verify that it was tracked by the security manager as a claimed application.
 * */
TEST_F(ClaimingNominalTests, DISABLED_SuccessfulClaimingWithOOB) {
}
}
//namespace
