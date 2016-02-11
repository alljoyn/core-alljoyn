/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

using namespace ajn::securitymgr;
using namespace std;

/** @file GroupManagementTests.cc */

namespace secmgr_tests {
class GroupManagementTests :
    public SecurityAgentTest {
};

/**
 * @test Store, retrieve and delete a group from storage.
 *       -# Define a valid security group and store it.
 *       -# Retrieve the group from storage and check whether it matches the
 *          stored group.
 *       -# Remove the security group.
 *       -# Retrieving the security group should fail.
 **/
TEST_F(GroupManagementTests, GroupManipBasic) {
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
 * @test Store, retrieve and delete many security groups from storage.
 *       -# Define a series of security groups and store them one by one.
 *       -# Retrieve all groups from storage and count whether all have
 *          been stored correctly.
 *       -# Remove all groups one by one from storage.
 *       -# Retrieve all groups from storage and make sure none are returned.
 **/
TEST_F(GroupManagementTests, GroupManipManyGroups) {
    GroupInfo groupInfo;
    GroupInfo compareToGroup;
    int times = 10;
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
        ASSERT_EQ(ER_OK, storage->RemoveGroup(*g));
    }

    groups.clear();

    ASSERT_EQ(storage->GetGroups(groups), ER_OK);
    ASSERT_TRUE(groups.empty());
}

/**
 * @test Check whether the default group authority is added on all Group
 *       methods.
 *       -# Create a GroupInfo object.
 *       -# Store the GroupInfo object and verify the authority is set.
 *       -# Create another GroupInfo object and fill in only the guid.
 *       -# Check if the original GroupInfo object can be retrieved.
 *       -# Create another GroupInfo object and fill in only the guid.
 *       -# Check if the original GroupInfo object can be removed.
 **/
TEST_F(GroupManagementTests, DefaultAuthority) {
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
 * @test Check whether more than one group authority can be supported.
 *       -# Create a GroupInfo (group0) object with an empty authority
 *       -# Store it successfully and make sure it gets an authority set.
 *       -# Create another GroupInfo (group1) and assign it group0's
 *          GUID but a different authority.
 *       -# Verify storing of group1 fails as it uses group0's GUID.
 *       -# Change group1 to use the same authority as in group0.
 *       -# Verify updating storage with group1 succeeds and
 *          that both groups are identical.
 *       -# Create a different group2 with a different GUID and Authority.
 *       -# Make sure its storage and retrieval succeed and that
 *          it is different than both previous groups.
 **/
TEST_F(GroupManagementTests, AuthoritiesCheck) {
    GroupInfo group0;
    group0.name = "Test";
    group0.desc = "This is a test group";

    ASSERT_TRUE(group0.authority.empty());
    ASSERT_EQ(ER_OK, storage->StoreGroup(group0));
    ASSERT_FALSE(group0.authority.empty()); // StoreGroup fills-in the authority if it was empty

    Crypto_ECC crypto;
    crypto.GenerateDHKeyPair();
    GroupInfo group1;
    group1.name = "TestAuth2";
    group1.desc = "This is a test group from another authority";
    group1.guid = group0.guid;
    group1.authority.SetPublicKey(crypto.GetDHPublicKey());

    ASSERT_NE(ER_OK, storage->StoreGroup(group1)); // Same GUID used but not the same authority
    group1.authority = group0.authority;
    ASSERT_EQ(ER_OK, storage->StoreGroup(group1)); // Update operation
    ASSERT_EQ(ER_OK, storage->GetGroup(group0));
    ASSERT_TRUE(group1 == group0);

    GroupInfo group2; // Different GUID than other groups
    group2.authority.SetPublicKey(crypto.GetDHPublicKey()); // Different authority than group1 and group0
    ASSERT_EQ(ER_OK, storage->StoreGroup(group2));
    ASSERT_EQ(ER_OK, storage->GetGroup(group2));
    ASSERT_FALSE(group2 == group0);
    ASSERT_FALSE(group2 == group1);
}

/**
 * @test Retrieval and deletion of unknown groups should fail.
 *       -# Try to get an unknown group and make sure this fails.
 *       -# Try to remove an unknown group and make sure this fails.
 *       -# Try to get all managed groups and make sure the vector is empty.
 **/
TEST_F(GroupManagementTests, FailedBasicGroupOperations) {
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
 * @test Update an existing group and make sure it can be retrieved correctly.
 *       -# Create a valid security group.
 *       -# Store the group and make sure this is successful.
 *       -# Retrieve the group from storage and make sure this is successful.
 *       -# Change the name and description of the group.
 *       -# Store the group and make sure this is successful.
 *       -# Retrieve the group and make sure it matches the updated info.
 **/
TEST_F(GroupManagementTests, GroupUpdate) {
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
