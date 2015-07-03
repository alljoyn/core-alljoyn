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

#include <string>

#include "TestUtil.h"

using namespace secmgrcoretest_unit_testutil;
using namespace ajn::securitymgr;
using namespace std;

/**
 * Several group manipulation (i.e., create, delete, retrieve, list group(s), etc.) nominal tests.
 */
namespace secmgrcoretest_unit_nominaltests {
class GroupManipulationNominalTests :
    public BasicTest {
};

/**
 * \test The test should verify that the security agent is able to add, delete and retrieve
 *               a group.
 *       -# Define valid groupinfo fields
 *       -# Add a Group using those details and verify that it was a successful operation
 *       -# Reset the name and desc fields, try to get the group and verify that the retrieved info matches the original details
 *       -# Ask the security agent to remove the group
 *       -# Try to retrieve the group and verify that it does not exist anymore
 * */
TEST_F(GroupManipulationNominalTests, GroupManipBasic) {
    GroupInfo groupInfo;

    GUID128 guid("B509480EE7B5A000B82A7E37E");
    string name = "Hello Group";
    string desc = "This is a hello world test group";

    groupInfo.guid = guid;
    groupInfo.name = name;
    groupInfo.desc = desc;

    ASSERT_EQ(storage->StoreGroup(groupInfo), ER_OK);

    groupInfo.name.clear();
    groupInfo.desc.clear();
    ASSERT_EQ(groupInfo.name, string(""));
    ASSERT_EQ(groupInfo.desc, string(""));
    ASSERT_EQ(storage->GetGroup(groupInfo), ER_OK);
    ASSERT_EQ(groupInfo.guid, guid);
    ASSERT_EQ(groupInfo.name, name);
    ASSERT_EQ(groupInfo.desc, desc);

    ASSERT_EQ(storage->RemoveGroup(groupInfo), ER_OK);
    ASSERT_NE(storage->GetGroup(groupInfo), ER_OK);
}

/**
 * \test The test should verify that the security agent is able to add a number of groups and retrieve them afterwards.
 *       -# Define valid groupinfo fields that could be adjusted later on
 *       -# Add many Groups using those iteratively amended details and verify that it was a successful operation each time
 *       -# Ask the Security Manager for all managed groups and verify the number as well as the content match those that were added
 *       -# Remove all groups
 *       -# Ask the agent for all groups and verify that the returned vector is empty
 * */
TEST_F(GroupManipulationNominalTests, GroupManipManyGroups) {
    GroupInfo groupInfo;
    GroupInfo compareToGroup;
    int times = 200;
    vector<GroupInfo> groups;

    string name = "Hello Group";
    string desc = "This is a hello world test group";

    groupInfo.name = name;
    groupInfo.desc = desc;

    for (int i = 0; i < times; i++) {
        stringstream tmp;
        tmp << name << i;
        groupInfo.name = tmp.str().c_str();

        groupInfo.guid = GUID128();

        tmp.clear();
        tmp << desc << i;
        groupInfo.desc = tmp.str().c_str();
        ASSERT_EQ(storage->StoreGroup(groupInfo), ER_OK);
    }

    ASSERT_EQ(storage->GetGroups(groups), ER_OK);
    ASSERT_EQ(groups.size(), (size_t)times);

    for (vector<GroupInfo>::iterator g = groups.begin(); g != groups.end(); g++) {
        int i = g - groups.begin();

        stringstream tmp;
        tmp << name << i;
        compareToGroup.name = tmp.str().c_str();
        tmp.clear();
        tmp << desc << i;
        compareToGroup.desc = tmp.str().c_str();

        ASSERT_EQ(g->name, compareToGroup.name);
        ASSERT_EQ(g->desc, compareToGroup.desc);
        ASSERT_EQ(storage->RemoveGroup(*g), ER_OK);
    }

    groups.clear();

    ASSERT_EQ(storage->GetGroups(groups), ER_OK);
    ASSERT_TRUE(groups.empty());
}

/**
 * \test Check whether the default group authority is added on all Group
 *       methods.
 *       -# Create a GroupInfo object
 *       -# Store the GroupInfo object and verify the authority is set
 *       -# Create another GroupInfo object and fill in only the guid
 *       -# Check if the original GroupInfo object can be retrieved
 *       -# Create another GroupInfo object and fill in only the guid
 *       -# Check if the original GroupInfo object can be removed
 **/
TEST_F(GroupManipulationNominalTests, DefaultAuthority) {
    GroupInfo group;
    group.name = "Test";
    group.desc = "This is a test group";

    ASSERT_TRUE(group.authority.empty());
    ASSERT_EQ(ER_OK, storage->StoreGroup(group));
    ASSERT_FALSE(group.authority.empty());

    ECCPublicKey securityManagerPubKey;
    securityManagerPubKey = *(secMgr->GetPublicKeyInfo().GetPublicKey());
    ASSERT_TRUE(*group.authority.GetPublicKey() == securityManagerPubKey);

    GroupInfo group2;
    group2.guid = group.guid;
    ASSERT_EQ(ER_OK, storage->GetGroup(group2));
    ASSERT_TRUE(group == group2);
    ASSERT_TRUE(group.name == group2.name);
    ASSERT_TRUE(group.desc == group2.desc);

    GroupInfo group3;
    group3.guid = group.guid;
    ASSERT_EQ(ER_OK, storage->RemoveGroup(group3));
    ASSERT_EQ(ER_END_OF_DATA, storage->GetGroup(group));
}

/**
 * \test Check whether more than one group authority can be supported.
 *       -# Create a GroupInfo object
 *       -# Store the GroupInfo object and verify the authority is set
 *       -# Create another GroupInfo object with the same guid, but a different
 *          authority
 *       -# Store the second GroupInfo object
 *       -# Create another GroupInfo object and fill in the required fields to
 *          retrieve the second GroupInfo object
 *       -# Check whether the second GroupInfo object can be retrieved.
 *       -# Create another GroupInfo object and fill in the required fields to
 *          retrieve the first GroupInfo object
 *       -# Check whether the first GroupInfo object can be retrieved.
 **/
TEST_F(GroupManipulationNominalTests, MultipleAuthorities) {
    GroupInfo group;
    group.name = "Test";
    group.desc = "This is a test group";

    ASSERT_TRUE(group.authority.empty());
    ASSERT_EQ(ER_OK, storage->StoreGroup(group));
    ASSERT_FALSE(group.authority.empty());

    Crypto_ECC crypto;
    crypto.GenerateDHKeyPair();
    GroupInfo group3;
    group3.name = "TestAuth2";
    group3.desc = "This is a test group from another authority";
    group3.guid = group.guid;
    group3.authority.SetPublicKey(crypto.GetDHPublicKey());
    ASSERT_EQ(ER_OK, storage->StoreGroup(group3));

    GroupInfo group4;
    group4.authority = group3.authority;
    group4.guid = group3.guid;
    ASSERT_EQ(ER_OK, storage->GetGroup(group4));
    ASSERT_TRUE(group3 == group4);
    ASSERT_TRUE(group3.name == group4.name);
    ASSERT_TRUE(group3.desc == group4.desc);

    GroupInfo group2;
    group2.guid = group.guid;
    ASSERT_EQ(ER_OK, storage->GetGroup(group2));
    ASSERT_TRUE(group == group2);
    ASSERT_TRUE(group.name == group2.name);
    ASSERT_TRUE(group.desc == group2.desc);
}
}
//namespace
