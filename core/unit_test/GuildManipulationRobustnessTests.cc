/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include "gtest/gtest.h"
#include "TestUtil.h"
#include <StorageConfig.h>
#include "SecurityManagerFactory.h"
#include <AppGuildInfo.h>
#include <string>

using namespace secmgrcoretest_unit_testutil;
using namespace ajn::securitymgr;
using namespace std;

/**
 * Several guild manipulation (i.e., create, delete, retrieve, list guild(s), etc.) robustness tests.
 */
namespace secmgrcoretest_unit_robustnesstests {
class GuildManipulationRobustnessTests :
    public BasicTest {
};

/**
 * \test The test should make sure that basic guild manipulation can fail gracefully.
 *       -# Try to get an unknown guild and make sure this fails
 *       -# Try to remove an unknown guild and make sure this fails
 *       -# Try to get all managed guilds and make sure the vector is empty
 *       -# change the guildInfo to some dummy info
 *       -# Store it and make sure this was successful
 *       -# Try to store it again and make sure this fails
 * */
TEST_F(GuildManipulationRobustnessTests, FailedBasicGuildOperations) {
    vector<GuildInfo> empty;

    GuildInfo guildInfo;

    guildInfo.name = "Wrong Guild";
    guildInfo.desc = "This is should never be there";

    ASSERT_NE(secMgr->GetGuild(guildInfo), ER_OK);
    ASSERT_NE(secMgr->RemoveGuild(guildInfo.guid), ER_OK);
    ASSERT_EQ(secMgr->GetManagedGuilds(empty), ER_OK);
    ASSERT_TRUE(empty.empty());

    guildInfo.name = "Dummy Guild";
    guildInfo.desc = "This is a dummy Guild";

    ASSERT_EQ(secMgr->StoreGuild(guildInfo), ER_OK);
    ASSERT_NE(secMgr->StoreGuild(guildInfo), ER_OK);
}

/**
 * \test The test should make sure that basic guild update works.
 *       -# Create a guildInfo with some Guild ID (guid)
 *       -# Try to store the guild with update=true and make sure this fails as the guild was not added before
 *       -# Try to store the guild with update=false and make sure this is successful
 *       -# Get the guild and make sure this is successful
 *       -# Try to store the guild without the update flag and make sure this fails as the guild already exists
 *       -# Change the name and description of the guild and try to store with update=true and make sure this succeeds
 *       -# Get the guild and compare the updated fields with the new info and make sure this is successful
 * */
TEST_F(GuildManipulationRobustnessTests, GuildUpdate) {
    vector<GuildInfo> empty;

    GuildInfo guildInfo;

    qcc::String name = "Hello Guild";
    qcc::String desc = "This is a hello world test guild";

    guildInfo.name = name;
    guildInfo.desc = desc;

    ASSERT_NE(secMgr->StoreGuild(guildInfo, true), ER_OK);
    ASSERT_EQ(secMgr->StoreGuild(guildInfo, false), ER_OK);
    ASSERT_EQ(secMgr->GetGuild(guildInfo), ER_OK);

    ASSERT_NE(secMgr->StoreGuild(guildInfo), ER_OK);

    name += " - updated";
    desc += " - updated";

    guildInfo.name = name;
    guildInfo.desc = desc;

    ASSERT_EQ(secMgr->StoreGuild(guildInfo, true), ER_OK);
    ASSERT_EQ(secMgr->GetGuild(guildInfo), ER_OK);

    ASSERT_EQ(guildInfo.name, name);
    ASSERT_EQ(guildInfo.desc, desc);
}
}
//namespace
