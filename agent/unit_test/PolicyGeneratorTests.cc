/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#include <gtest/gtest.h>

#include <stdio.h>

#include <qcc/StringUtil.h>

#include <alljoyn/securitymgr/PolicyGenerator.h>
#include <alljoyn/securitymgr/GroupInfo.h>

#include "TestUtil.h"

namespace secmgrcoretest_unit_nominaltests {
using namespace std;
using namespace ajn;
using namespace qcc;
using namespace securitymgr;
using namespace secmgrcoretest_unit_testutil;

class PolicyGeneratorTest :
    public BasicTest {
};

TEST_F(PolicyGeneratorTest, BasicTest) {
    ECCPublicKey publicKey;
    PermissionPolicy pol;
    GUID128 groupID;
    string groupIDString = BytesToHexString(groupID.GetBytes(), GUID128::SIZE).c_str();

    GroupInfo group1;
    group1.guid = groupID;
    ASSERT_EQ(ER_OK, storage->StoreGroup(group1));
    vector<GroupInfo> groups;
    groups.push_back(group1);

    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, pol));
    string policyString = pol.ToString().c_str();
    ASSERT_EQ((size_t)2, pol.GetAclsSize());

    GUID128 groupID2;
    string groupID2String = BytesToHexString(groupID2.GetBytes(), GUID128::SIZE).c_str();

    GroupInfo group2;
    group2.guid = groupID2;
    ASSERT_EQ(ER_OK, storage->StoreGroup(group2));
    groups.push_back(group2);

    ASSERT_EQ(ER_OK, pg->DefaultPolicy(groups, pol));
    policyString = pol.ToString().c_str();

    ASSERT_EQ((size_t)3, pol.GetAclsSize());
}
}
