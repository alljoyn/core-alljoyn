/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include "TestUtil.h"

using namespace ajn::securitymgr;

/** @file DiscoveryTests.cc */
namespace secmgr_tests {
class DiscoveryTests :
    public BasicTest {
};

TEST_F(DiscoveryTests, LateJoiningSecurityAgent) {
    // start test app
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    // start security agent
    InitSecAgent();

    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));
    // wait for signal
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));
}
} // namespace