/**
 * @file
 * This program tests the basic features of Alljoyn.It uses google test as the test
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

#include "ajTestCommon.h"
#include <ns/IpNameServiceImpl.h>

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
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
        tcpAvail(tcpAvail), tcpMax(tcpMax), udpAvail(udpAvail), tclAvail(tclAvail), tclMax(tclMax), dynamicScore(dynamicScore) { }
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
                        testing::Values(TestParams(StaticParams(2700, 8100, 8100, 8100, 27000),
                                                   DynamicParams(16, 16, 16, 16, 8, 8, 9504), 29031),
                                        TestParams(StaticParams(900, 2025, 1012, 7987, 11924),
                                                   DynamicParams(1, 16, 2, 16, 2, 8, 1505), 52106)));
