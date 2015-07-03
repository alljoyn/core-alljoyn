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

#include <string>

#include "TestUtil.h"

using namespace secmgrcoretest_unit_testutil;
using namespace ajn::securitymgr;
using namespace std;

/**
 * Several group manipulation (i.e., create, delete, retrieve, list group(s), etc.) robustness tests.
 */
namespace secmgrcoretest_unit_robustnesstests {
class GroupManipulationRobustnessTests :
    public BasicTest {
};

/**
 * \test The test should make sure that basic group manipulation can fail gracefully.
 *       -# Try to get an unknown group and make sure this fails
 *       -# Try to remove an unknown group and make sure this fails
 *       -# Try to get all managed groups and make sure the vector is empty
 * */
TEST_F(GroupManipulationRobustnessTests, FailedBasicGroupOperations) {
    vector<GroupInfo> empty;

    GroupInfo groupInfo;

    groupInfo.name = "Wrong Group";
    groupInfo.desc = "This is should never be there";

    ASSERT_EQ(storage->GetGroup(groupInfo), ER_END_OF_DATA);
    ASSERT_NE(storage->RemoveGroup(groupInfo), ER_OK);
    ASSERT_EQ(storage->GetGroups(empty), ER_OK);
    ASSERT_TRUE(empty.empty());
}

/**
 * \test The test should make sure that basic group update works.
 *       -# Create a groupInfo with some guid
 *       -# Try to store the group and make sure this is successful
 *       -# Get the group and make sure this is successful
 *       -# Change the name and description of the group
 *       -# Try to store the group and make sure this is successful
 *       -# Get the group and compare the updated fields with the new info and
 *          make sure this is successful
 * */
TEST_F(GroupManipulationRobustnessTests, GroupUpdate) {
    GroupInfo groupInfo;

    string name = "Hello Group";
    string desc = "This is a hello world test group";

    groupInfo.name = name;
    groupInfo.desc = desc;

    ASSERT_EQ(storage->StoreGroup(groupInfo), ER_OK);
    ASSERT_EQ(storage->GetGroup(groupInfo), ER_OK);

    name += " - updated";
    desc += " - updated";

    groupInfo.name = name;
    groupInfo.desc = desc;

    ASSERT_EQ(storage->StoreGroup(groupInfo), ER_OK);
    ASSERT_EQ(storage->GetGroup(groupInfo), ER_OK);

    ASSERT_EQ(groupInfo.name, name);
    ASSERT_EQ(groupInfo.desc, desc);
}
}
//namespace
