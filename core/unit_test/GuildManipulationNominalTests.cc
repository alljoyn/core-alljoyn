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
 * Several guild manipulation (i.e., create, delete, retrieve, list guild(s), etc.) nominal tests.
 */
namespace secmgrcoretest_unit_nominaltests {
class GuildManipulationNominalTests :
    public BasicTest {
};

/**
 * \test The test should verify that the security manager is able to add, delete and retrieve
 *               a guild.
 *       -# Define valid guildinfo fields
 *       -# Add a Guild using those details and verify that it was a successful operation
 *       -# Reset the name and desc fields, try to get the guild and verify that the retrieved info matches the original details
 *       -# Ask the security manager to remove the guild
 *       -# Try to retrieve the guild and verify that it does not exist anymore
 * */
TEST_F(GuildManipulationNominalTests, GuildManipBasic) {
    GuildInfo guildInfo;

    qcc::GUID128 guid("B509480EE7B5A000B82A7E37E");
    qcc::String name = "Hello Guild";
    qcc::String desc = "This is a hello world test guild";

    guildInfo.guid = guid;
    guildInfo.name = name;
    guildInfo.desc = desc;

    ASSERT_EQ(secMgr->StoreGuild(guildInfo), ER_OK);

    guildInfo.name.clear();
    guildInfo.desc.clear();
    ASSERT_EQ(guildInfo.name, qcc::String(""));
    ASSERT_EQ(guildInfo.desc, qcc::String(""));
    ASSERT_EQ(secMgr->GetGuild(guildInfo), ER_OK);
    ASSERT_EQ(guildInfo.guid, guid);
    ASSERT_EQ(guildInfo.name, name);
    ASSERT_EQ(guildInfo.desc, desc);

    ASSERT_EQ(secMgr->RemoveGuild(guildInfo.guid), ER_OK);
    ASSERT_NE(secMgr->GetGuild(guildInfo), ER_OK);
}

/**
 * \test The test should verify that the security manager is able to add a number of guilds and retrieve them afterwards.
 *       -# Define valid guildinfo fields that could be adjusted later on
 *       -# Add many Guilds using those iteratively amended details and verify that it was a successful operation each time
 *       -# Ask the Security Manager for all managed guilds and verify the number as well as the content match those that were added
 *       -# Remove all guilds
 *       -# Ask the manager for all guilds and verify that the returned vector is empty
 * */
TEST_F(GuildManipulationNominalTests, GuildManipManyGuilds) {
    GuildInfo guildInfo;
    GuildInfo compareToGuild;
    int times = 200;
    std::vector<GuildInfo> guilds;

    qcc::String name = "Hello Guild";
    qcc::String desc = "This is a hello world test guild";

    guildInfo.name = name;
    guildInfo.desc = desc;

    for (int i = 0; i < times; i++) {
        guildInfo.guid = GUID128();
        guildInfo.name = name + std::to_string(i).c_str();
        guildInfo.desc = desc + std::to_string(i).c_str();
        ASSERT_EQ(secMgr->StoreGuild(guildInfo), ER_OK);
    }

    ASSERT_EQ(secMgr->GetManagedGuilds(guilds), ER_OK);
    ASSERT_EQ(guilds.size(), (size_t)times);

    for (std::vector<GuildInfo>::const_iterator g = guilds.begin(); g != guilds.end(); g++) {
        int i = g - guilds.begin();

        compareToGuild.name = name + std::to_string(i).c_str();
        compareToGuild.desc = desc + std::to_string(i).c_str();

        ASSERT_EQ(g->name, compareToGuild.name);
        ASSERT_EQ(g->desc, compareToGuild.desc);
        ASSERT_EQ(secMgr->RemoveGuild(g->guid), ER_OK);
    }

    guilds.clear();

    ASSERT_EQ(secMgr->GetManagedGuilds(guilds), ER_OK);
    ASSERT_TRUE(guilds.empty());
}
}
//namespace
