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

/** @file SecurityAgentFactoryTests.cc */

namespace secmgr_tests {
class SecurityAgentFactoryTests :
    public BasicTest {
};

/**
 * @test Ensure that the security agent factory can return a valid security agent.
 *       -# Get a security agent factory instance.
 *       -# Get a security agent using a connected bus attachment.
 *       -# Validate the security agent returned
 *       -# Get a security agent with using a null bus attachment parameter.
 *       -# Validate the security agent returned.
 **/
TEST_F(SecurityAgentFactoryTests, Basic) {
    shared_ptr<SecurityAgent> localSecMgr;
    SecurityAgentFactory& localSecFac = SecurityAgentFactory::GetInstance();
    ASSERT_EQ(ER_OK, localSecFac.GetSecurityAgent(GetAgentCAStorage(), localSecMgr, ba));
    ASSERT_TRUE(ba->IsConnected());
    ASSERT_TRUE(ba->IsStarted());
    ASSERT_NE(nullptr, localSecMgr);
    secMgr = nullptr;
    ASSERT_EQ(ER_OK, localSecFac.GetSecurityAgent(GetAgentCAStorage(), localSecMgr, nullptr));
    ASSERT_NE(nullptr, localSecMgr);
}
}