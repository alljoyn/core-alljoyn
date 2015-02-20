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

#include "gtest/gtest.h"
#include "TestUtil.h"
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
 * */
TEST_F(GuildManipulationRobustnessTests, FailedBasicGuildOperations) {
    vector<GuildInfo> empty;

    GuildInfo guildInfo;

    guildInfo.name = "Wrong Guild";
    guildInfo.desc = "This is should never be there";

    ASSERT_EQ(secMgr->GetGuild(guildInfo), ER_END_OF_DATA);
    ASSERT_NE(secMgr->RemoveGuild(guildInfo), ER_OK);
    ASSERT_EQ(secMgr->GetGuilds(empty), ER_OK);
    ASSERT_TRUE(empty.empty());
}

/**
 * \test The test should make sure that basic guild update works.
 *       -# Create a guildInfo with some guid
 *       -# Try to store the guild and make sure this is successful
 *       -# Get the guild and make sure this is successful
 *       -# Change the name and description of the guild
 *       -# Try to store the guild and make sure this is successful
 *       -# Get the guild and compare the updated fields with the new info and
 *          make sure this is successful
 * */
TEST_F(GuildManipulationRobustnessTests, GuildUpdate) {
    GuildInfo guildInfo;

    qcc::String name = "Hello Guild";
    qcc::String desc = "This is a hello world test guild";

    guildInfo.name = name;
    guildInfo.desc = desc;

    ASSERT_EQ(secMgr->StoreGuild(guildInfo), ER_OK);
    ASSERT_EQ(secMgr->GetGuild(guildInfo), ER_OK);

    name += " - updated";
    desc += " - updated";

    guildInfo.name = name;
    guildInfo.desc = desc;

    ASSERT_EQ(secMgr->StoreGuild(guildInfo), ER_OK);
    ASSERT_EQ(secMgr->GetGuild(guildInfo), ER_OK);

    ASSERT_EQ(guildInfo.name, name);
    ASSERT_EQ(guildInfo.desc, desc);
}
}
//namespace
