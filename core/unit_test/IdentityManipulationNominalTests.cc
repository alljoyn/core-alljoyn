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

#include "TestUtil.h"
#include <string>

using namespace secmgrcoretest_unit_testutil;
using namespace ajn::securitymgr;
using namespace std;

/**
 * Several identity manipulation (i.e., create, delete, retrieve, list identity(s), etc.) nominal tests.
 */
namespace secmgrcoretest_unit_nominaltests {
class IdentityManipulationNominalTests :
    public BasicTest {
};

/**
 * \test The test should verify that the security manager is able to add, delete and retrieve
 *               an identity.
 *       -# Define valid identityinfo fields
 *       -# Add an Identity using those details and verify that it was a successful operation
 *       -# Reset the name and desc fields, try to get the identity and verify that the retrieved info matches the original details
 *       -# Ask the security manager to remove the identity
 *       -# Try to retrieve the identity and verify that it does not exist anymore
 * */
TEST_F(IdentityManipulationNominalTests, IdentityManipBasic) {
    IdentityInfo identityInfo;

    qcc::GUID128 guid("B509480EE7B5A000B82A7E37E");
    qcc::String name = "Hello Identity";
    qcc::String desc = "This is a hello world test identity";

    identityInfo.guid = guid;
    identityInfo.name = name;

    ASSERT_EQ(secMgr->StoreIdentity(identityInfo), ER_OK);

    identityInfo.name.clear();
    ASSERT_EQ(identityInfo.name, qcc::String(""));
    ASSERT_EQ(secMgr->GetIdentity(identityInfo), ER_OK);
    ASSERT_EQ(identityInfo.guid, guid);
    ASSERT_EQ(identityInfo.name, name);

    ASSERT_EQ(secMgr->RemoveIdentity(identityInfo), ER_OK);
    ASSERT_EQ(secMgr->GetIdentity(identityInfo), ER_END_OF_DATA);
}

/**
 * \test The test should verify that the security manager is able to add a number of identities and retrieve them afterwards.
 *       -# Define valid identityinfo fields that could be adjusted later on
 *       -# Add many Identities using those iteratively amended details and verify that it was a successful operation each time
 *       -# Ask the Security Manager for all managed identities and verify the number as well as the content match those that were added
 *       -# Remove all identities
 *       -# Ask the manager for all identities and verify that the returned vector is empty
 * */
TEST_F(IdentityManipulationNominalTests, IdentityManipManyIdentities) {
    IdentityInfo identityInfo;
    IdentityInfo compareToIdentity;
    int times = 200;
    std::vector<IdentityInfo> identities;

    qcc::String name = "Hello Identity";
    qcc::String desc = "This is a hello world test identity";

    identityInfo.name = name;

    for (int i = 0; i < times; i++) {
        stringstream tmp;
        tmp << name << i;
        identityInfo.name = tmp.str().c_str();

        identityInfo.guid = GUID128();

        tmp.clear();
        tmp << desc << i;
        ASSERT_EQ(secMgr->StoreIdentity(identityInfo), ER_OK);
    }

    ASSERT_EQ(secMgr->GetIdentities(identities), ER_OK);
    ASSERT_EQ(identities.size(), (size_t)times);

    for (std::vector<IdentityInfo>::iterator g = identities.begin(); g != identities.end(); g++) {
        int i = g - identities.begin();

        stringstream tmp;
        tmp << name << i;
        compareToIdentity.name = tmp.str().c_str();
        tmp.clear();
        tmp << desc << i;

        ASSERT_EQ(g->name, compareToIdentity.name);
        ASSERT_EQ(secMgr->RemoveIdentity(*g), ER_OK);
    }

    identities.clear();

    ASSERT_EQ(secMgr->GetIdentities(identities), ER_OK);
    ASSERT_TRUE(identities.empty());
}
}
//namespace
