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

#include "PermissionMgmt.h"
#include "TestUtil.h"

/**
 * Several claiming robustness tests.
 */

namespace secmgrcoretest_unit_robustnesstests {
using namespace secmgrcoretest_unit_testutil;

using namespace ajn::securitymgr;
using namespace std;

class ClaimingRobustnessTests :
    public BasicTest {
  private:

  protected:

  public:
    ClaimingRobustnessTests()
    {
    }
};

/**
 * \test The test should make sure that the security manager handles properly
 *       the response of a stub client that already has an ROT
 *       -# Create a security manager 1 and announce it.
 *       -# Create a security manager 2 and announce it.
 *       -# Create a stub client and make it claimable.
 *       -# Try to claim the stub client from both security managers at the same time.
 *       -# Verify that exactly one security manager has claimed the stub client
 *          and that the stub client has the right ROT and an identity certificate.

 * */
TEST_F(ClaimingRobustnessTests, DISABLED_FailedClaimingExistingROT) {
    // ASSERT_TRUE(false);
}

/**
 * \test The test should make sure that the security manager handles properly
 *       the response of a stub client that already has an ROT
 *       -# Create a security manager and announce it.
 *       -# Create a stub client and make it claimable.
 *       -# Try to claim the stub client but introduce a network error, e.g., session lost
 *       -# Verify that the security manager did not claim the stub client and that the client
 *          has no ROT nor an identity certificate.
 * */
TEST_F(ClaimingRobustnessTests, DISABLED_FailedClaimingNetError) {
    //   ASSERT_TRUE(false);
}

/**
 * \test The test should make sure that the claim method handles the request in a robust way
 *  -# Try to claim the same application with a valid publicKey and make sure this works even with a bad bus name
 *  -# Make sure it cannot be re-claimed
 *  -# Kill the stub app client
 *  -# Make sure the stub app cannot be claimed
 * */
TEST_F(ClaimingRobustnessTests, DISABLED_InvalidArguments) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);

    Stub* stub = new Stub(&tcl);
    ASSERT_EQ(stub->OpenClaimWindow(), ER_OK);
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));

    ApplicationInfo info = lastAppInfo;
    info.busName = lastAppInfo.busName;

    info.publicKey = lastAppInfo.publicKey;
    info.busName = "My Rubbish BusName";               //the bad busname should be ignored
    IdentityInfo idInfo;
    idInfo.guid = info.peerID;
    idInfo.name = info.appName;
    ASSERT_EQ(ER_OK, secMgr->StoreIdentity(idInfo));

    ASSERT_EQ(ER_OK, secMgr->Claim(info, idInfo));

    ASSERT_NE(ER_OK, secMgr->Claim(lastAppInfo, idInfo));                //already claimed

    delete stub;

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));

    ASSERT_NE(ER_OK, secMgr->Claim(lastAppInfo, idInfo));                //we killed our peer.
}

/**
 * \test Make sure that previously claimed apps can be retrieved when an the SM restarts.
 *		-# Create a stub client with the needed listeners and make it claimable
 *		-# Try to claim the stub and make sure this was successful
 *		-# Teardown the security manager and the busattachment used
 *		-# Get a new security manager
 *		-# Get the previously claimed stub/app from the security manager
 *      -# Make sure the retrieved application info match that of the originally claimed app
 * */
TEST_F(ClaimingRobustnessTests, DISABLED_SMClaimedAppsWarmStart) {
    bool claimAnswer = true;
    TestClaimListener tcl(claimAnswer);

    Stub* stub = new Stub(&tcl);
    ASSERT_EQ(stub->OpenClaimWindow(), ER_OK);
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING));

    IdentityInfo idInfo;
    idInfo.guid = GUID128("abcdef123456789");
    idInfo.name = "MyName";
    ASSERT_EQ(secMgr->StoreIdentity(idInfo), ER_OK);

    ASSERT_EQ(secMgr->Claim(lastAppInfo, idInfo), ER_OK);

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING));

    qcc::String origBusName = lastAppInfo.busName;
    TearDown();                    //Kill secMgr and ba

    SecurityManagerFactory& secFac =
        SecurityManagerFactory::GetInstance();
    ba = new BusAttachment("test", true);
    ASSERT_TRUE(ba != NULL);
    ASSERT_EQ(ER_OK, ba->Start());
    ASSERT_EQ(ER_OK, ba->Connect());

    // create new sec mgr
    storage = SQLStorageFactory::GetInstance().GetStorage();
    ASSERT_TRUE(storage != NULL);

    secMgr = secFac.GetSecurityManager(storage, ba);
    ASSERT_TRUE(secMgr != NULL);

    tal = new TestApplicationListener(sem, lock);
    secMgr->RegisterApplicationListener(tal);
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING));

    ApplicationInfo cmprInfo;
    cmprInfo.busName = origBusName;

    ASSERT_EQ(ER_OK, secMgr->GetApplication(cmprInfo));

    ASSERT_EQ(tal->_lastAppInfo.publicKey, cmprInfo.publicKey);
    ASSERT_EQ(tal->_lastAppInfo.userDefinedName, cmprInfo.userDefinedName);
    ASSERT_EQ(tal->_lastAppInfo.deviceName, cmprInfo.deviceName);
    ASSERT_EQ(tal->_lastAppInfo.appName, cmprInfo.appName);
    ASSERT_EQ(tal->_lastAppInfo.peerID, cmprInfo.peerID);
    ASSERT_EQ(tal->_lastAppInfo.claimState, cmprInfo.claimState);

    ASSERT_EQ(tal->_lastAppInfo.busName, cmprInfo.busName);
    ASSERT_EQ(tal->_lastAppInfo.rootsOfTrust.size(), cmprInfo.rootsOfTrust.size());
    ASSERT_EQ(tal->_lastAppInfo.runningState, cmprInfo.runningState);
    secMgr->UnregisterApplicationListener(tal);

    delete stub;
}
}
//namespace
