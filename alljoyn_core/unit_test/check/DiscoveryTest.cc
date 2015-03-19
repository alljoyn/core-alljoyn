/**
 * @file
 * This program tests the discovery features of Alljoyn.It uses google test as the test
 * automation framework.
 */
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
#include "ns/IpNameServiceImpl.h"

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "../ajTestCommon.h"

using namespace qcc;
using namespace ajn;

class StaticParams {
  public:
    uint32_t powerSource;
    uint32_t mobility;
    uint32_t availability;
    uint32_t nodeConnection;
    uint32_t staticScore;
    StaticParams(uint32_t powerSource, uint32_t mobility, uint32_t availability, uint32_t nodeConnection, uint32_t staticScore) :
        powerSource(powerSource), mobility(mobility), availability(availability), nodeConnection(nodeConnection), staticScore(staticScore) { }
};

class DynamicParams {
  public:
    uint32_t tcpAvail;
    uint32_t tcpMax;
    uint32_t udpAvail;
    uint32_t udpMax;
    uint32_t tclAvail;
    uint32_t tclMax;
    uint32_t dynamicScore;
    DynamicParams(uint32_t tcpAvail, uint32_t tcpMax, uint32_t udpAvail, uint32_t udpMax, uint32_t tclAvail, uint32_t tclMax, uint32_t dynamicScore) :
        tcpAvail(tcpAvail), tcpMax(tcpMax), udpAvail(udpAvail), udpMax(udpMax), tclAvail(tclAvail), tclMax(tclMax), dynamicScore(dynamicScore) { }
};

class TestParams {
  public:
    StaticParams staticParams;
    DynamicParams dynamicParams;
    uint16_t priority;
    TestParams(StaticParams staticParams, DynamicParams dynamicParams, uint16_t priority) : staticParams(staticParams), dynamicParams(dynamicParams), priority(priority) { }
};

class DiscoveryTest : public testing::TestWithParam<TestParams> {
    virtual void SetUp() { }
    virtual void TearDown() { }
};

class TestEnumerationParams {
  public:
    qcc::String param;
    qcc::String configXml;
    uint32_t enumValue;
    TestEnumerationParams(qcc::String param, qcc::String configXml, uint32_t enumValue) : param(param), configXml(configXml), enumValue(enumValue) { }
};

class ConfigEnumerationTest : public testing::TestWithParam<TestEnumerationParams> {
    virtual void SetUp() { }
    virtual void TearDown() { }
};

TEST_P(ConfigEnumerationTest, CheckEnumeration)
{
    TestEnumerationParams tp = GetParam();
    ajn::ConfigDB* config = new ajn::ConfigDB(tp.configXml);
    config->LoadConfig();
    qcc::String param = tp.param;
    uint32_t enumValue = ajn::IpNameServiceImpl::LoadParam(config, param);
    ASSERT_EQ(tp.enumValue, enumValue);
    delete config;
}

TEST_P(DiscoveryTest, ComputeStaticScore)
{
    TestParams tp = GetParam();
    // ComputeStaticScore using power_source, mobility, availability and node_type values
    uint32_t powerSource = tp.staticParams.powerSource;
    uint32_t mobility = tp.staticParams.mobility;
    uint32_t availability = tp.staticParams.availability;
    uint32_t nodeConnection = tp.staticParams.nodeConnection;
    uint32_t staticScore = ajn::IpNameServiceImpl::ComputeStaticScore(powerSource, mobility, availability, nodeConnection);
    ASSERT_EQ(tp.staticParams.staticScore, staticScore);
}

TEST_P(DiscoveryTest, ComputeDynamicScore)
{
    // ComputeDynamicScore using tcpAvail, tcpMax, udpAvail, udpMax, tclAvail and tclMax values
    TestParams tp = GetParam();
    uint32_t tcpAvail = tp.dynamicParams.tcpAvail;
    uint32_t tcpMax = tp.dynamicParams.tcpMax;
    uint32_t udpAvail = tp.dynamicParams.udpAvail;
    uint32_t udpMax = tp.dynamicParams.udpMax;
    uint32_t tclAvail = tp.dynamicParams.tclAvail;
    uint32_t tclMax = tp.dynamicParams.tclMax;
    uint16_t dynamicScore = ajn::IpNameServiceImpl::ComputeDynamicScore(tcpAvail, tcpMax, udpAvail, udpMax, tclAvail, tclMax);
    ASSERT_EQ(tp.dynamicParams.dynamicScore, dynamicScore);
}

TEST_P(DiscoveryTest, ComputePriority)
{
    // ComputePriority using staticRank and dynamicRank values
    TestParams tp = GetParam();
    uint32_t staticScore = tp.staticParams.staticScore;
    uint32_t dynamicScore = tp.dynamicParams.dynamicScore;
    uint16_t priority = ajn::IpNameServiceImpl::ComputePriority(staticScore, dynamicScore);
    ASSERT_EQ(tp.priority, priority);
}

INSTANTIATE_TEST_CASE_P(Discovery, DiscoveryTest,
                        testing::Values(TestParams(StaticParams(ajn::IpNameServiceImpl::ROUTER_POWER_SOURCE_MIN, ajn::IpNameServiceImpl::ROUTER_MOBILITY_MIN, ajn::IpNameServiceImpl::ROUTER_AVAILABILITY_MIN, ajn::IpNameServiceImpl::ROUTER_NODE_CONNECTION_MIN, 7987),
                                                   DynamicParams(1, 16, 2, 16, 2, 8, 1379), 56169),
                                        TestParams(StaticParams(ajn::IpNameServiceImpl::ROUTER_POWER_SOURCE_MAX, ajn::IpNameServiceImpl::ROUTER_MOBILITY_MAX, ajn::IpNameServiceImpl::ROUTER_AVAILABILITY_MAX, ajn::IpNameServiceImpl::ROUTER_NODE_CONNECTION_MAX, 27000),
                                                   DynamicParams(16, 16, 16, 16, 8, 8, 9345), 29190)));

INSTANTIATE_TEST_CASE_P(DiscoveryEnumeration, ConfigEnumerationTest,
                        testing::Values(TestEnumerationParams("router_power_source", "<busconfig> <property name=\"router_power_source\">Always AC powered</property> </busconfig>", 2700),
                                        TestEnumerationParams("router_power_source", "<busconfig> <property name=\"router_power_source\">Battery powered and chargeable</property> </busconfig>", 1800),
                                        TestEnumerationParams("router_power_source", "<busconfig> <property name=\"router_power_source\">Battery powered and not chargeable</property> </busconfig>", 900),
                                        TestEnumerationParams("router_power_source", "<busconfig> <property name=\"router_power_source\">Invalid string</property> </busconfig>", 1800),
                                        TestEnumerationParams("router_power_source", "", 1800),
                                        TestEnumerationParams("router_mobility", "<busconfig> <property name=\"router_mobility\">Always Stationary</property> </busconfig>", 8100),
                                        TestEnumerationParams("router_mobility", "<busconfig> <property name=\"router_mobility\">Low mobility</property> </busconfig>", 6075),
                                        TestEnumerationParams("router_mobility", "<busconfig> <property name=\"router_mobility\">Intermediate mobility</property> </busconfig>", 4050),
                                        TestEnumerationParams("router_mobility", "<busconfig> <property name=\"router_mobility\">High mobility</property> </busconfig>", 2025),
                                        TestEnumerationParams("router_mobility", "<busconfig> <property name=\"router_mobility\">Invalid string</property> </busconfig>", 4050),
                                        TestEnumerationParams("router_mobility", "", 4050),
                                        TestEnumerationParams("router_availability", "<busconfig> <property name=\"router_availability\">0-3 hr</property> </busconfig>", 1012),
                                        TestEnumerationParams("router_availability", "<busconfig> <property name=\"router_availability\">3-6 hr</property> </busconfig>", 2025),
                                        TestEnumerationParams("router_availability", "<busconfig> <property name=\"router_availability\">6-9 hr</property> </busconfig>", 3037),
                                        TestEnumerationParams("router_availability", "<busconfig> <property name=\"router_availability\">9-12 hr</property> </busconfig>", 4050),
                                        TestEnumerationParams("router_availability", "<busconfig> <property name=\"router_availability\">12-15 hr</property> </busconfig>", 5062),
                                        TestEnumerationParams("router_availability", "<busconfig> <property name=\"router_availability\">15-18 hr</property> </busconfig>", 6075),
                                        TestEnumerationParams("router_availability", "<busconfig> <property name=\"router_availability\">18-21 hr</property> </busconfig>", 7087),
                                        TestEnumerationParams("router_availability", "<busconfig> <property name=\"router_availability\">21-24 hr</property> </busconfig>", 8100),
                                        TestEnumerationParams("router_availability", "<busconfig> <property name=\"router_availability\">Invalid string</property> </busconfig>", 2025),
                                        TestEnumerationParams("router_availability", "", 2025),
                                        TestEnumerationParams("router_node_connection", "<busconfig> <property name=\"router_node_connection\">Access Point</property> </busconfig>", 8100),
                                        TestEnumerationParams("router_node_connection", "<busconfig> <property name=\"router_node_connection\">Wired</property> </busconfig>", 8100),
                                        TestEnumerationParams("router_node_connection", "<busconfig> <property name=\"router_node_connection\">Wireless</property> </busconfig>", 4050),
                                        TestEnumerationParams("router_node_connection", "<busconfig> <property name=\"router_node_connection\">Invalid string</property> </busconfig>", 4050),
                                        TestEnumerationParams("router_node_connection", "", 4050),
                                        TestEnumerationParams("invalid param", "<busconfig> <property name=\"invalid param\">Invalid string</property> </busconfig>", std::numeric_limits<uint32_t>::max())));
