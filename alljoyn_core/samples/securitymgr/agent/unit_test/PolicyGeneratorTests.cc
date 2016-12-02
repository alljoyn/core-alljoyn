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

#include <gtest/gtest.h>

#include <stdio.h>

#include <qcc/StringUtil.h>

#include <alljoyn/PermissionPolicyUtil.h>
#include <alljoyn/securitymgr/PolicyGenerator.h>
#include <alljoyn/securitymgr/GroupInfo.h>

#include "TestUtil.h"

using namespace std;
using namespace ajn;
using namespace qcc;
using namespace securitymgr;

/** @file PolicyGeneratorTest.cc */

namespace secmgr_tests {
class PolicyGeneratorTest :
    public BasicTest {
};

/**
 * @test Basic test for the sample policy generator.
 *       -# Create a security group and store it.
 *       -# Generate a policy for this group.
 *       -# Make sure this policy has two ACLs (including one for the admin
 *          group).
 *       -# Create another security group and store it.
 *       -# Generate another policy for both groups.
 *       -# Make sure this policy has three ACLs (including one for the admin
 *          group).
 **/
TEST_F(PolicyGeneratorTest, BasicTest) {
    PermissionPolicy pol;
    GroupInfo group1;

    ASSERT_EQ(ER_OK, storage->StoreGroup(group1));
    vector<GroupInfo> groups;
    groups.push_back(group1);

    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, pol));
    ASSERT_EQ((size_t)2, pol.GetAclsSize());

    GroupInfo group2;
    ASSERT_EQ(ER_OK, storage->StoreGroup(group2));
    groups.push_back(group2);

    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, pol));

    ASSERT_EQ((size_t)3, pol.GetAclsSize());
    ASSERT_TRUE(PermissionPolicyUtil::HasValidDenyRules(pol));
}

/**
 * @test Basic test for illegal argument in policy generator.
 *       -# Create an empty vector of GroupInfo; groupInfoEmptyVec
 *       -# Use the existing PolicyGenerator instance (pg)
 *          to get a DefaultPolicy using the groupInfoEmptyVec
 *          and make sure this does not fail but return a
 *          default policy with one admin rule.
 **/
TEST_F(PolicyGeneratorTest, BasicIllegalArgTest) {
    vector<GroupInfo> groups; // Intentionally empty
    PermissionPolicy pol;
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, pol));
    ASSERT_EQ((size_t)1, pol.GetAclsSize()) << "Policy is: " << pol.ToString().c_str();
    ASSERT_TRUE(PermissionPolicyUtil::HasValidDenyRules(pol));
}

/**
 * @test Validate the generation of a policy with deny rules.
 *       -# Create a policy generator and add a random application to its blacklist.
 *       -# Generate a default policy for an empty vector of groups.
 *       -# Check that the resulting policy has 2 ACLs.
 *       -# Check that the resulting policy only contains valid deny rules.
 **/
TEST_F(PolicyGeneratorTest, DenyRules) {
    OnlineApplication app;
    pg->deniedKeys.push_back(app.keyInfo);
    vector<GroupInfo> groups; // Intentionally empty
    PermissionPolicy pol;
    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, pol));
    ASSERT_EQ((size_t)2, pol.GetAclsSize());
    ASSERT_TRUE(PermissionPolicyUtil::HasValidDenyRules(pol));
}
}