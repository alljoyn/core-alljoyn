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

/** @file IdentityManagementTests.cc */

namespace secmgr_tests {
class IdentityManagementTests :
    public BasicTest {
};

/**
 * @test Store, retrieve and delete an identity from storage.
 *       -# Define a valid IdentityInfo and store it.
 *       -# Retrieve the identity from storage and check whether it matches the
 *          stored identity.
 *       -# Remove the identity from storage.
 *       -# Retrieving the identity should fail.
 **/
TEST_F(IdentityManagementTests, IdentityManipBasic) {
    IdentityInfo identityInfo;

    GUID128 guid("B509480EE7B5A000B82A7E37E");
    string name = "Hello Identity";

    identityInfo.name = name;
    identityInfo.guid = guid;

    ASSERT_EQ(storage->StoreIdentity(identityInfo), ER_OK);
    ASSERT_FALSE(identityInfo.authority.empty());

    identityInfo.name.clear();
    ASSERT_EQ(identityInfo.name, string(""));
    ASSERT_EQ(storage->GetIdentity(identityInfo), ER_OK);
    ASSERT_EQ(identityInfo.guid, guid);
    ASSERT_EQ(identityInfo.name, name);

    ASSERT_EQ(storage->RemoveIdentity(identityInfo), ER_OK);
    ASSERT_EQ(storage->GetIdentity(identityInfo), ER_END_OF_DATA);
}

/**
 * @test Store, retrieve and delete many identities from storage.
 *       -# Define a series of identities and store them one by one.
 *       -# Retrieve all identities from storage and count whether all have
 *          been stored correctly.
 *       -# Remove all identities one by one from storage.
 *       -# Retrieve all identities from storage and make sure none are
 *          returned.
 **/
TEST_F(IdentityManagementTests, IdentityManipManyIdentities) {
    IdentityInfo identityInfo;
    IdentityInfo compareToIdentity;
    int times = 10;
    vector<IdentityInfo> identities;

    string name = "Hello Identity";
    string desc = "This is a hello world test identity";

    identityInfo.name = name;

    for (int i = 0; i < times; i++) {
        stringstream tmp;
        tmp << name << i;
        identityInfo.name = tmp.str().c_str();

        identityInfo.guid = GUID128();

        tmp.clear();
        tmp << desc << i;
        ASSERT_EQ(storage->StoreIdentity(identityInfo), ER_OK);
    }

    ASSERT_EQ(storage->GetIdentities(identities), ER_OK);
    ASSERT_EQ(identities.size(), (size_t)times);

    for (vector<IdentityInfo>::iterator g = identities.begin(); g != identities.end(); g++) {
        int i = g - identities.begin();

        stringstream tmp;
        tmp << name << i;
        compareToIdentity.name = tmp.str().c_str();
        tmp.clear();
        tmp << desc << i;

        ASSERT_EQ(g->name, compareToIdentity.name);
        ASSERT_EQ(storage->RemoveIdentity(*g), ER_OK);
    }

    identities.clear();

    ASSERT_EQ(storage->GetIdentities(identities), ER_OK);
    ASSERT_TRUE(identities.empty());
}

/**
 * @test Retrieval and deletion of unknown identities should fail.
 *       -# Try to get an unknown identity and make sure this fails.
 *       -# Try to remove an unknown identity and make sure this fails.
 *       -# Try to get all identities and make sure there are none.
 **/
TEST_F(IdentityManagementTests, FailedBasicIdentityOperations) {
    vector<IdentityInfo> empty;

    IdentityInfo identityInfo;

    identityInfo.name = "Wrong Identity";

    ASSERT_EQ(storage->GetIdentity(identityInfo), ER_END_OF_DATA);
    ASSERT_NE(storage->RemoveIdentity(identityInfo), ER_OK);
    ASSERT_EQ(storage->GetIdentities(empty), ER_OK);
    ASSERT_TRUE(empty.empty());
}

/**
 * @test Update an existing identity and make sure it can be retrieved correctly.
 *       -# Create a valid identity.
 *       -# Store the identity and make sure this is successful.
 *       -# Retrieve the identity from storage and make sure this is successful.
 *       -# Change the name and description of the identity.
 *       -# Store the identity and make sure this is successful.
 *       -# Retrieve the identity and make sure it matches the updated identity.
 **/
TEST_F(IdentityManagementTests, IdentityUpdate) {
    vector<IdentityInfo> empty;

    IdentityInfo identityInfo;

    string name = "Hello Identity";
    string desc = "This is a hello world test identity";

    identityInfo.name = name;

    ASSERT_EQ(storage->StoreIdentity(identityInfo), ER_OK);
    ASSERT_EQ(storage->GetIdentity(identityInfo), ER_OK);

    name += " - updated";
    desc += " - updated";

    identityInfo.name = name;

    ASSERT_EQ(storage->StoreIdentity(identityInfo), ER_OK);
    ASSERT_EQ(storage->GetIdentity(identityInfo), ER_OK);

    ASSERT_EQ(identityInfo.name, name);
}
}
//namespace
