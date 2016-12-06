/******************************************************************************
 * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

/*
 * Below tests are based on constructing a ConfigDB object with
 * different config XML parameters. A bundled router always creates
 * the ConfigDB singleton, so if we try to construct our ConfigDB
 * from a BR=on test binary, the tests will terminate on a failed singleton
 * assertion. For this reason, the tests below are only compiled when
 * a standalone router is used (BR=off, ROUTER not defined).
 */
#ifndef ROUTER

#include <qcc/platform.h>

#include <gtest/gtest.h>
#include "ajTestCommon.h"

#include <memory>
#include "ConfigDB.h"

static const char defaultConfig[] =
    "<busconfig>"
    "  <type>alljoyn_bundled</type>"
    "  <listen>tcp:iface=*,port=0</listen>"
    "  <listen>udp:iface=*,port=0</listen>"
    "  <limit name=\"auth_timeout\">20000</limit>"
    "  <limit name=\"max_incomplete_connections\">48</limit>"
    "  <limit name=\"max_completed_connections\">64</limit>"
    "  <limit name=\"max_remote_clients_tcp\">48</limit>"
    "  <limit name=\"max_remote_clients_udp\">48</limit>"
    "  <property name=\"router_power_source\">Battery powered and chargeable</property>"
    "  <property name=\"router_mobility\">Intermediate mobility</property>"
    "  <property name=\"router_availability\">3-6 hr</property>"
    "  <property name=\"router_node_connection\">Wireless</property>"
    "</busconfig>";

static const char userConfig[] =
    "<busconfig>"
    "  <type>alljoyn_bundled</type>"
    "  <listen>tcp:iface=*,port=9555</listen>"
    "  <listen>udp:iface=*,port=9559</listen>"
    "  <limit name=\"auth_timeout\">10000</limit>"
    "  <limit name=\"max_incomplete_connections\">20</limit>"
    "  <limit name=\"max_completed_connections\">30</limit>"
    "  <limit name=\"max_remote_clients_tcp\">30</limit>"
    "  <limit name=\"max_remote_clients_udp\">30</limit>"
    "  <property name=\"router_power_source\">Battery powered and chargeable</property>"
    "  <property name=\"router_mobility\">Intermediate mobility</property>"
    "  <property name=\"router_availability\">2-4 hr</property>"
    "  <property name=\"router_node_connection\">Wireless</property>"
    "</busconfig>";

static const char malformedConfig[] =
    "<busconfig>"
    "  <type>alljoyn_bundled</type>"
    "  <listen>tcp:iface=*,port=9555</listen>"
    "  <listen>udp:iface=*,port=9559</listen>"
    "  <limit name=\"auth_timeout\">10000" // Missing: </limit>"
    "  <limit name=\"max_incomplete_connections\">20</limit>"
    "  <limit name=\"max_completed_connections\">30</limit>"
    "  <limit name=\"max_remote_clients_tcp\">30</limit>"
    "  <limit name=\"max_remote_clients_udp\">30</limit>"
    "  <property name=\"router_power_source\">Battery powered and chargeable</property>"
    "  <property name=\"router_mobility\">Intermediate mobility</property>"
    "  <property name=\"router_availability\">2-4 hr</property>"
    "  <property name=\"router_node_connection\">Wireless</property>"
    "</busconfig>";

class ConfigDbTest : public::testing::Test {
  public:
    ConfigDbTest()
    { }

    virtual void SetUp()
    { }

    virtual void TearDown()
    { }

    void ExpectDefaultConfig()
    {
        EXPECT_STREQ("alljoyn_bundled", configDb->GetType().c_str());

        ajn::ConfigDB::ListenList actualListenList = configDb->GetListen();
        ajn::ConfigDB::ListenList expectedListenList;
        expectedListenList->insert("tcp:iface=*,port=0");
        expectedListenList->insert("udp:iface=*,port=0");
        EXPECT_EQ(expectedListenList, actualListenList);

        EXPECT_EQ(20000u, configDb->GetLimit("auth_timeout"));
        EXPECT_EQ(48u, configDb->GetLimit("max_incomplete_connections"));
        EXPECT_EQ(64u, configDb->GetLimit("max_completed_connections"));
        EXPECT_EQ(48u, configDb->GetLimit("max_remote_clients_tcp"));
        EXPECT_EQ(48u, configDb->GetLimit("max_remote_clients_udp"));
        EXPECT_STREQ("Battery powered and chargeable", configDb->GetProperty("router_power_source").c_str());
        EXPECT_STREQ("Intermediate mobility", configDb->GetProperty("router_mobility").c_str());
        EXPECT_STREQ("3-6 hr", configDb->GetProperty("router_availability").c_str());
        EXPECT_STREQ("Wireless", configDb->GetProperty("router_node_connection").c_str());
    }

    void ExpectUserConfig()
    {
        EXPECT_STREQ("alljoyn_bundled", configDb->GetType().c_str());

        ajn::ConfigDB::ListenList actualListenList = configDb->GetListen();
        ajn::ConfigDB::ListenList expectedListenList;
        expectedListenList->insert("tcp:iface=*,port=9555");
        expectedListenList->insert("udp:iface=*,port=9559");
        EXPECT_EQ(expectedListenList, actualListenList);

        EXPECT_EQ(10000u, configDb->GetLimit("auth_timeout"));
        EXPECT_EQ(20u, configDb->GetLimit("max_incomplete_connections"));
        EXPECT_EQ(30u, configDb->GetLimit("max_completed_connections"));
        EXPECT_EQ(30u, configDb->GetLimit("max_remote_clients_tcp"));
        EXPECT_EQ(30u, configDb->GetLimit("max_remote_clients_udp"));
        EXPECT_STREQ("Battery powered and chargeable", configDb->GetProperty("router_power_source").c_str());
        EXPECT_STREQ("Intermediate mobility", configDb->GetProperty("router_mobility").c_str());
        EXPECT_STREQ("2-4 hr", configDb->GetProperty("router_availability").c_str());
        EXPECT_STREQ("Wireless", configDb->GetProperty("router_node_connection").c_str());
    }

    std::unique_ptr<ajn::ConfigDB> configDb;
};

TEST_F(ConfigDbTest, LoadConfig_DefaultXmlProvided_ConfigApplied)
{
    configDb.reset(new ajn::ConfigDB(defaultConfig));

    ASSERT_TRUE(configDb->LoadConfig());
    ExpectDefaultConfig();
}

TEST_F(ConfigDbTest, LoadConfig_DefaultAndUserXmlProvided_UserConfigApplied)
{
    configDb.reset(new ajn::ConfigDB(defaultConfig, "", userConfig));

    ASSERT_TRUE(configDb->LoadConfig());
    ExpectUserConfig();
}

TEST_F(ConfigDbTest, LoadConfig_DefaultAndUserXmlAndFilenameProvided_DefaultConfigApplied)
{
    configDb.reset(new ajn::ConfigDB(defaultConfig, "config.xml", userConfig));

    ASSERT_TRUE(configDb->LoadConfig());
    ExpectDefaultConfig();
}

TEST_F(ConfigDbTest, LoadConfig_MalformedDefaultXmlAndNoUserXmlProvided_ReturnsFalse)
{
    configDb.reset(new ajn::ConfigDB(malformedConfig));

    ASSERT_FALSE(configDb->LoadConfig());
}

TEST_F(ConfigDbTest, LoadConfig_MalformedDefaultXmlAndUserXmlProvided_UserConfigApplied)
{
    configDb.reset(new ajn::ConfigDB(malformedConfig, "", userConfig));

    ASSERT_TRUE(configDb->LoadConfig());
    ExpectUserConfig();
}

TEST_F(ConfigDbTest, LoadConfig_MalformedDefaultXmlAndUserXmlAndFilenameProvided_ReturnsFalse)
{
    configDb.reset(new ajn::ConfigDB(malformedConfig, "config.xml", userConfig));

    EXPECT_FALSE(configDb->LoadConfig());
}

TEST_F(ConfigDbTest, LoadConfig_DefaultAndMalformedUserXmlProvided_DefaultConfigApplied)
{
    configDb.reset(new ajn::ConfigDB(defaultConfig, "", malformedConfig));

    ASSERT_TRUE(configDb->LoadConfig());
    ExpectDefaultConfig();
}

#endif