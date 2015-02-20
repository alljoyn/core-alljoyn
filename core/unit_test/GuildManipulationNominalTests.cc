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

    ASSERT_EQ(secMgr->RemoveGuild(guildInfo), ER_OK);
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
        stringstream tmp;
        tmp << name << i;
        guildInfo.name = tmp.str().c_str();

        guildInfo.guid = GUID128();

        tmp.clear();
        tmp << desc << i;
        guildInfo.desc = tmp.str().c_str();
        ASSERT_EQ(secMgr->StoreGuild(guildInfo), ER_OK);
    }

    ASSERT_EQ(secMgr->GetGuilds(guilds), ER_OK);
    ASSERT_EQ(guilds.size(), (size_t)times);

    for (std::vector<GuildInfo>::iterator g = guilds.begin(); g != guilds.end(); g++) {
        int i = g - guilds.begin();

        stringstream tmp;
        tmp << name << i;
        compareToGuild.name = tmp.str().c_str();
        tmp.clear();
        tmp << desc << i;
        compareToGuild.desc = tmp.str().c_str();

        ASSERT_EQ(g->name, compareToGuild.name);
        ASSERT_EQ(g->desc, compareToGuild.desc);
        ASSERT_EQ(secMgr->RemoveGuild(*g), ER_OK);
    }

    guilds.clear();

    ASSERT_EQ(secMgr->GetGuilds(guilds), ER_OK);
    ASSERT_TRUE(guilds.empty());
}

/**
 * \test Check whether the default guild authority is added on all Guild
 *       methods.
 *       -# Create a GuildInfo object
 *       -# Store the GuildInfo object and verify the authority is set
 *       -# Create another GuildInfo object and fill in only the guid
 *       -# Check if the original GuildInfo object can be retrieved
 *       -# Create another GuildInfo object and fill in only the guid
 *       -# Check if the original GuildInfo object can be removed
 **/
TEST_F(GuildManipulationNominalTests, DefaultAuthority) {
    GuildInfo guild;
    guild.name = "Test";
    guild.desc = "This is a test guild";

    ASSERT_TRUE(guild.authority.empty());
    ASSERT_EQ(ER_OK, secMgr->StoreGuild(guild));
    ASSERT_FALSE(guild.authority.empty());

    ECCPublicKey securityManagerPubKey;
    securityManagerPubKey = secMgr->GetPublicKey();
    ASSERT_TRUE(guild.authority == securityManagerPubKey);

    GuildInfo guild2;
    guild2.guid = guild.guid;
    ASSERT_EQ(ER_OK, secMgr->GetGuild(guild2));
    ASSERT_TRUE(guild == guild2);
    ASSERT_TRUE(guild.name == guild2.name);
    ASSERT_TRUE(guild.desc == guild2.desc);

    GuildInfo guild3;
    guild3.guid = guild.guid;
    ASSERT_EQ(ER_OK, secMgr->RemoveGuild(guild3));
    ASSERT_EQ(ER_END_OF_DATA, secMgr->GetGuild(guild));
}

/**
 * \test Check whether more than one guild authority can be supported.
 *       -# Create a GuildInfo object
 *       -# Store the GuildInfo object and verify the authority is set
 *       -# Create another GuildInfo object with the same guid, but a different
 *          authority
 *       -# Store the second GuildInfo object
 *       -# Create another GuildInfo object and fill in the required fields to
 *          retrieve the second GuildInfo object
 *       -# Check whether the second GuildInfo object can be retrieved.
 *       -# Create another GuildInfo object and fill in the required fields to
 *          retrieve the first GuildInfo object
 *       -# Check whether the first GuildInfo object can be retrieved.
 **/
TEST_F(GuildManipulationNominalTests, MultipleAuthorities) {
    GuildInfo guild;
    guild.name = "Test";
    guild.desc = "This is a test guild";

    ASSERT_TRUE(guild.authority.empty());
    ASSERT_EQ(ER_OK, secMgr->StoreGuild(guild));
    ASSERT_FALSE(guild.authority.empty());

    Crypto_ECC crypto;
    crypto.GenerateDHKeyPair();
    GuildInfo guild3;
    guild3.name = "TestAuth2";
    guild3.desc = "This is a test guild from another authority";
    guild3.guid = guild.guid;
    guild3.authority = *(crypto.GetDHPublicKey());
    ASSERT_EQ(ER_OK, secMgr->StoreGuild(guild3));

    GuildInfo guild4;
    guild4.authority = guild3.authority;
    guild4.guid = guild3.guid;
    ASSERT_EQ(ER_OK, secMgr->GetGuild(guild4));
    ASSERT_TRUE(guild3 == guild4);
    ASSERT_TRUE(guild3.name == guild4.name);
    ASSERT_TRUE(guild3.desc == guild4.desc);

    GuildInfo guild2;
    guild2.guid = guild.guid;
    ASSERT_EQ(ER_OK, secMgr->GetGuild(guild2));
    ASSERT_TRUE(guild == guild2);
    ASSERT_TRUE(guild.name == guild2.name);
    ASSERT_TRUE(guild.desc == guild2.desc);
}
}
//namespace
