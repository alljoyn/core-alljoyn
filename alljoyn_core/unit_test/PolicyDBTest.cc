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
#include <qcc/platform.h>

#include <gtest/gtest.h>
#include "ajTestCommon.h"

#include <map>
#include <memory>
#include "ConfigDB.h"
#include "LocalTransport.h"
#include "Message.h"
#include "MsgArg.h"
#include "policydb/PolicyDB.h"

static const char* BUS_NAME = "com.unittest.a";

class TestMessage : public ajn::_Message {
  public:
    TestMessage(ajn::BusAttachment& bus)
        : ajn::_Message(bus) { }

    TestMessage(const TestMessage& other)
        : ajn::_Message(other) { }

    QStatus MakeMethodCallMsg(const qcc::String& sender,
                              const qcc::String& destination,
                              const qcc::String& objPath,
                              const qcc::String& iface,
                              const qcc::String& methodName,
                              const ajn::MsgArg* args,
                              size_t numArgs,
                              uint8_t flags = 0u)
    {
        return CallMsg(ajn::MsgArg::Signature(args, numArgs), sender, destination, 0, objPath, iface, methodName,
                       args, numArgs, flags);
    }

    QStatus MakeErrorMsg(const qcc::String& sender, const char* errorName, uint32_t replySerial = 0u)
    {
        return ErrorMsg(sender, errorName, replySerial);
    }
};

class PolicyDBTest : public::testing::Test {
  public:
    PolicyDBTest()
        :
#ifndef ROUTER
        // Manually instantiate a ConfigDB singleton
        // (necessary for Bus initialization, normally done by the Bundled Router).
        configDb(new ajn::ConfigDB("")),
#endif
        policyDb(), bus(new ajn::Bus(BUS_NAME, factories))
    { }

    virtual void SetUp()
    { }

    virtual void TearDown()
    { }

    ajn::BusEndpoint CreateEndpoint(const qcc::String& uniqueName)
    {
        const uint32_t CONCURRENCY = 1u;
        ajn::LocalEndpoint localEndpoint(*bus, CONCURRENCY);
        localEndpoint->SetUniqueName(uniqueName);

        ajn::BusEndpoint busEndpoint = ajn::BusEndpoint::cast(localEndpoint);
        return busEndpoint;
    }

    void RegisterEndpoint(ajn::BusEndpoint& endpoint, const qcc::String& busName)
    {
        policyDb->AddAlias(endpoint->GetUniqueName(), endpoint->GetUniqueName());
        policyDb->AddAlias(busName, endpoint->GetUniqueName());
    }

    std::shared_ptr<ajn::ConfigDB> configDb;
    ajn::PolicyDB policyDb;
    ajn::TransportFactoryContainer factories;
    std::shared_ptr<ajn::Bus> bus;
};

TEST_F(PolicyDBTest, OKToConnect_NoRules_Allows)
{
    const uint32_t UID = 1u, GID = 2u;
    EXPECT_TRUE(policyDb->OKToConnect(UID, GID));
}

TEST_F(PolicyDBTest, OKToConnect_DenyAllUsersRule_Denies)
{
    const uint32_t UID = 1u, GID = 2u;
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["user"] = "*";

    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    EXPECT_FALSE(policyDb->OKToConnect(UID, GID));
}

TEST_F(PolicyDBTest, OKToConnect_DenyAllGroupsRule_Denies)
{
    const uint32_t UID = 1u, GID = 2u;
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["group"] = "*";

    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    EXPECT_FALSE(policyDb->OKToConnect(UID, GID));
}

TEST_F(PolicyDBTest, OKToConnect_DenyRuleAddedLaterThanAllowRule_Denies)
{
    const uint32_t UID = 1u, GID = 2u;
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["user"] = "*";

    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", ruleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    EXPECT_FALSE(policyDb->OKToConnect(UID, GID));
}

TEST_F(PolicyDBTest, OKToConnect_AllowRuleAddedLaterThanDenyRule_Allows)
{
    const uint32_t UID = 1u, GID = 2u;
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["user"] = "*";

    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", ruleAttributes));

    EXPECT_TRUE(policyDb->OKToConnect(UID, GID));
}

// Context-mandatory rules should have higher priority than context-default rules
TEST_F(PolicyDBTest, OKToConnect_MandatoryRuleAllowsDefaultRuleDenies_Allows)
{
    const uint32_t UID = 1u, GID = 2u;
    std::map<qcc::String, qcc::String> mandatoryRuleAttributes;
    mandatoryRuleAttributes["user"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "mandatory", "allow", mandatoryRuleAttributes));

    std::map<qcc::String, qcc::String> defaultRuleAttributes;
    defaultRuleAttributes["user"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", defaultRuleAttributes));

    EXPECT_TRUE(policyDb->OKToConnect(UID, GID));
}

// Context-mandatory rules should have higher priority than context-default rules
TEST_F(PolicyDBTest, OKToConnect_MandatoryRuleDeniesDefaultRuleAllows_Denies)
{
    const uint32_t UID = 1u, GID = 2u;
    std::map<qcc::String, qcc::String> mandatoryRuleAttributes;
    mandatoryRuleAttributes["user"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "mandatory", "deny", mandatoryRuleAttributes));

    std::map<qcc::String, qcc::String> defaultRuleAttributes;
    defaultRuleAttributes["user"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", defaultRuleAttributes));

    EXPECT_FALSE(policyDb->OKToConnect(UID, GID));
}

TEST_F(PolicyDBTest, OKToOwn_NoRules_Allows)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);

    policyDb->Finalize(bus.get());

    EXPECT_TRUE(policyDb->OKToOwn(BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToOwn_DenyAllBusNames_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_FALSE(policyDb->OKToOwn(BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToOwn_DeniedBusNameIsSameAsRequestedBusName_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* POLICY_BUS_NAME = "org.unittest.a";
    const char* REQUESTED_BUS_NAME = "org.unittest.a";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own"] = POLICY_BUS_NAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_FALSE(policyDb->OKToOwn(REQUESTED_BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToOwn_RequestedBusNameAllowedByOneRuleDeniedByLater_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* REQUESTED_BUS_NAME = "org.unittest.a";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own"] = REQUESTED_BUS_NAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", ruleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_FALSE(policyDb->OKToOwn(REQUESTED_BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToOwn_RequestedBusNameDeniedByOneRuleAllowedByLater_Allows)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* REQUESTED_BUS_NAME = "org.unittest.a";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own"] = REQUESTED_BUS_NAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_TRUE(policyDb->OKToOwn(REQUESTED_BUS_NAME, endpoint));
}

// Context-mandatory rules should have higher priority than context-default rules
TEST_F(PolicyDBTest, OKToOwn_RequestedBusNameDeniedByMandatoryRuleAllowedByDefault_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* REQUESTED_BUS_NAME = "org.unittest.a";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own"] = REQUESTED_BUS_NAME;
    ASSERT_TRUE(policyDb->AddRule("context", "mandatory", "deny", ruleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_FALSE(policyDb->OKToOwn(REQUESTED_BUS_NAME, endpoint));
}

// Context-mandatory rules should have higher priority than context-default rules
TEST_F(PolicyDBTest, OKToOwn_RequestedBusNameAllowedByMandatoryRuleDeniedByDefault_Allows)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* REQUESTED_BUS_NAME = "org.unittest.a";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own"] = REQUESTED_BUS_NAME;
    ASSERT_TRUE(policyDb->AddRule("context", "mandatory", "allow", ruleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_TRUE(policyDb->OKToOwn(REQUESTED_BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToOwn_DeniedBusNameIsDifferentFromRequestedBusName_Allows)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* POLICY_BUS_NAME = "org.unittest.a";
    const char* REQUESTED_BUS_NAME = "com.unittest.b";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own"] = POLICY_BUS_NAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_TRUE(policyDb->OKToOwn(REQUESTED_BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToOwn_DeniedBusNameIsPrefixOfRequestedBusName_Allows)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* POLICY_BUS_NAME = "org.unittest.a";
    const char* REQUESTED_BUS_NAME = "org.unittest.ab";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own"] = POLICY_BUS_NAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_TRUE(policyDb->OKToOwn(REQUESTED_BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToOwn_RequestedBusNameIsPrefixOfDeniedBusName_Allows)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* POLICY_BUS_NAME = "org.unittest.abc";
    const char* REQUESTED_BUS_NAME = "org.unittest.ab";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own"] = POLICY_BUS_NAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_TRUE(policyDb->OKToOwn(REQUESTED_BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToOwn_DenyAllBusNamesByPrefix_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own_prefix"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_FALSE(policyDb->OKToOwn(BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToOwn_DeniedByPrefixBusNameIsSameAsRequestedBusName_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* POLICY_BUS_NAME = "org.unittest.a";
    const char* REQUESTED_BUS_NAME = "org.unittest.a";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own_prefix"] = POLICY_BUS_NAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_FALSE(policyDb->OKToOwn(REQUESTED_BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToOwn_DeniedByPrefixBusNameIsPrefixOfRequestedBusName_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* POLICY_BUS_NAME = "org.unittest.a";
    const char* REQUESTED_BUS_NAME = "org.unittest.a.b.c.d";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own_prefix"] = POLICY_BUS_NAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_FALSE(policyDb->OKToOwn(REQUESTED_BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToOwn_RequestedBusNameIsPrefixOfDeniedByPrefixBusName_Allows)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* POLICY_BUS_NAME = "org.unittest.abc";
    const char* REQUESTED_BUS_NAME = "org.unittest.ab";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["own_prefix"] = POLICY_BUS_NAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    EXPECT_TRUE(policyDb->OKToOwn(REQUESTED_BUS_NAME, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_NoRules_Allows)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());
    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyAllSenderBusnames_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["receive_sender"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DeniedSenderBusname_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    const char* DENIED_BUSNAME = "com.denied";
    ruleAttributes["receive_sender"] = DENIED_BUSNAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg(DENIED_BUSNAME, "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyingAndAllowingBusnameRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* DENIED_BUSNAME = "com.denied";
    const char* ALLOWED_BUSNAME = "com.allowed";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    denyingRuleAttributes["receive_sender"] = DENIED_BUSNAME;
    allowingRuleAttributes["receive_sender"] = ALLOWED_BUSNAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg(ALLOWED_BUSNAME, "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg(DENIED_BUSNAME, "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToReceive(allowedNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToReceive(deniedNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyAllReceiveInterfaces_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["receive_interface"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DeniedReceiveInterface_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    std::map<qcc::String, qcc::String> ruleAttributes;
    const char* DENIED_INTERFACE = "com.denied";
    ruleAttributes["receive_interface"] = DENIED_INTERFACE;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           DENIED_INTERFACE, "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyingAndAllowingInterfaceRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* DENIED_INTERFACE = "com.denied";
    const char* ALLOWED_INTERFACE = "com.allowed";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    denyingRuleAttributes["receive_interface"] = DENIED_INTERFACE;
    allowingRuleAttributes["receive_interface"] = ALLOWED_INTERFACE;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           ALLOWED_INTERFACE, "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           DENIED_INTERFACE, "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToReceive(allowedNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToReceive(deniedNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyAllTypes_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["receive_type"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DeniedType_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    std::map<qcc::String, qcc::String> ruleAttributes;
    const char* DENIED_TYPE = "method_call";
    ruleAttributes["receive_type"] = DENIED_TYPE;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyingAndAllowingTypeRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    allowingRuleAttributes["receive_type"] = "error";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    denyingRuleAttributes["receive_type"] = "method_call";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DESTINATION_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg("com.bus", "org.alljoyn.Error.Foo"));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToReceive(allowedNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToReceive(deniedNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyAllMembers_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["receive_member"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DeniedMember_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    std::map<qcc::String, qcc::String> ruleAttributes;
    const char* DENIED_MEMBER = "deniedMethod";
    ruleAttributes["receive_member"] = DENIED_MEMBER;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", DENIED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyingAndAllowingMemberRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* DENIED_MEMBER = "deniedMethod";
    const char* ALLOWED_MEMBER = "allowedMethod";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    denyingRuleAttributes["receive_member"] = DENIED_MEMBER;
    allowingRuleAttributes["receive_member"] = ALLOWED_MEMBER;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));

    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));
    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", ALLOWED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", DENIED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToReceive(allowedNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToReceive(deniedNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyAllPaths_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["receive_path"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DeniedPath_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    const char* DENIED_PATH = "/obj/denied";
    ruleAttributes["receive_path"] = DENIED_PATH;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", DENIED_PATH,
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyingAndAllowingPathRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* DENIED_PATH = "/obj/denied";
    const char* ALLOWED_PATH = "/obj/allowed";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    denyingRuleAttributes["receive_path"] = DENIED_PATH;
    allowingRuleAttributes["receive_path"] = ALLOWED_PATH;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", ALLOWED_PATH,
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", DENIED_PATH,
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToReceive(allowedNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToReceive(deniedNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyAllErrorMsgs_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["receive_error"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg("com.bus", "org.alljoyn.Error.Foo"));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DeniedErrorMsg_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    std::map<qcc::String, qcc::String> ruleAttributes;
    const char* DENIED_ERRORMSG = "org.alljoyn.Error.Denied";
    ruleAttributes["receive_error"] = DENIED_ERRORMSG;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg("com.bus", DENIED_ERRORMSG));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyingAndAllowingErrorMsgRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* DENIED_ERRORMSG = "org.alljoyn.Error.Denied";
    const char* ALLOWED_ERRORMSG = "org.alljoyn.Error.Allowed";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    denyingRuleAttributes["receive_error"] = DENIED_ERRORMSG;
    allowingRuleAttributes["receive_error"] = ALLOWED_ERRORMSG;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg("com.bus", ALLOWED_ERRORMSG));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg("com.bus", DENIED_ERRORMSG));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToReceive(allowedNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToReceive(deniedNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_ComplexRule_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* DENIED_INTERFACE = "com.denied.iface";
    const char* DENIED_PATH = "/obj/denied";
    const char* DENIED_BUSNAME = "com.denied.busname";
    const char* DENIED_TYPE = "method_call";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    denyingRuleAttributes["receive_interface"] = DENIED_INTERFACE;
    denyingRuleAttributes["receive_path"] = DENIED_PATH;
    denyingRuleAttributes["receive_sender"] = DENIED_BUSNAME;
    denyingRuleAttributes["receive_type"] = DENIED_TYPE;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg(DENIED_BUSNAME, "com.destination", DENIED_PATH,
                                           DENIED_INTERFACE, "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr matchingNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg(DENIED_BUSNAME, "com.destination", DENIED_PATH,
                                           "com.different.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr differentInterfaceNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg(DENIED_BUSNAME, "com.destination", "/obj/different",
                                           DENIED_INTERFACE, "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr differentPathNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.different.busname", "com.destination", DENIED_PATH,
                                           DENIED_INTERFACE, "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr differentBusnameNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg(DENIED_BUSNAME, "org.alljoyn.Error.Foo"));
    ajn::NormalizedMsgHdr differentTypeNmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(matchingNmh, endpoint));
    EXPECT_TRUE(policyDb->OKToReceive(differentInterfaceNmh, endpoint));
    EXPECT_TRUE(policyDb->OKToReceive(differentPathNmh, endpoint));
    EXPECT_TRUE(policyDb->OKToReceive(differentBusnameNmh, endpoint));
    EXPECT_TRUE(policyDb->OKToReceive(differentTypeNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_MultipleRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* DENIED_INTERFACE = "com.denied.iface";
    const char* DENIED_PATH = "/obj/denied";
    const char* DENIED_BUSNAME = "com.denied.busname";
    const char* DENIED_TYPE = "error";
    const char* DENIED_MEMBER = "deniedMethod";

    std::map<qcc::String, qcc::String> firstRuleAttributes;
    firstRuleAttributes["receive_interface"] = DENIED_INTERFACE;
    firstRuleAttributes["receive_path"] = DENIED_PATH;
    std::map<qcc::String, qcc::String> secondRuleAttributes;
    secondRuleAttributes["receive_sender"] = DENIED_BUSNAME;
    std::map<qcc::String, qcc::String> thirdRuleAttributes;
    thirdRuleAttributes["receive_type"] = DENIED_TYPE;
    std::map<qcc::String, qcc::String> fourthRuleAttributes;
    fourthRuleAttributes["receive_member"] = DENIED_MEMBER;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", firstRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", secondRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", thirdRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", fourthRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.busname", "com.destination", DENIED_PATH,
                                           DENIED_INTERFACE, "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr firstRuleNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg(DENIED_BUSNAME, "com.destination", "/obj/different",
                                           "com.different.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr secondRuleNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg(DENIED_BUSNAME, "org.alljoyn.Error.Foo"));
    ajn::NormalizedMsgHdr thirdRuleNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.different.busname", "com.destination", DENIED_PATH,
                                           "com.different.interface", DENIED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr fourthRuleNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.different.busname", "com.destination", DENIED_PATH,
                                           "com.different.interface", "differentMethod", NULL, 0));
    ajn::NormalizedMsgHdr notMatchingNmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToReceive(firstRuleNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToReceive(secondRuleNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToReceive(thirdRuleNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToReceive(fourthRuleNmh, endpoint));
    EXPECT_TRUE(policyDb->OKToReceive(notMatchingNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToReceive_DenyingSendRule_Allows)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["send_destination"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DESTINATION_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToReceive(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_NoRules_Allows)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DESTINATION_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DenyAllDestinationBusnames_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["send_destination"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DESTINATION_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DeniedDestinationBusname_Denies)
{
    const char* DENIED_BUSNAME = "com.denied";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["send_destination"] = DENIED_BUSNAME;

    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));
    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DENIED_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DENIED_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_AllowingAndDenyingDestinationBusnameRules_AllowsAccordingly)
{
    const char* DENIED_BUSNAME = "com.denied";
    const char* ALLOWED_BUSNAME = "com.allowed";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    denyingRuleAttributes["send_destination"] = DENIED_BUSNAME;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    allowingRuleAttributes["send_destination"] = ALLOWED_BUSNAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));

    ajn::BusEndpoint deniedEndpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(deniedEndpoint, DENIED_BUSNAME);
    ajn::BusEndpoint allowedEndpoint = CreateEndpoint(":Endpoint.2");
    RegisterEndpoint(allowedEndpoint, ALLOWED_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", ALLOWED_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, allowedEndpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DENIED_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, deniedEndpoint);

    EXPECT_TRUE(policyDb->OKToSend(allowedNmh, allowedEndpoint));
    EXPECT_FALSE(policyDb->OKToSend(deniedNmh, deniedEndpoint));
}

TEST_F(PolicyDBTest, OKToSend_EndpointNoLongerHasDeniedBusName_Allows)
{
    const char* DENIED_BUSNAME = "com.denied";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["send_destination"] = DENIED_BUSNAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DENIED_BUSNAME);
    policyDb->Finalize(bus.get());

    qcc::String uniqueName = endpoint->GetUniqueName();
    policyDb->NameOwnerChanged(DENIED_BUSNAME, &uniqueName, ajn::SessionOpts::ALL_NAMES,
                               nullptr, ajn::SessionOpts::ALL_NAMES);

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DENIED_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_EndpointChangesBusNameToDenied_Denies)
{
    const char* ALLOWED_BUSNAME = "com.allowed";
    const char* DENIED_BUSNAME = "com.denied";
    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["send_destination"] = DENIED_BUSNAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, ALLOWED_BUSNAME);
    policyDb->Finalize(bus.get());

    qcc::String uniqueName = endpoint->GetUniqueName();
    policyDb->NameOwnerChanged(ALLOWED_BUSNAME, &uniqueName, ajn::SessionOpts::ALL_NAMES,
                               nullptr, ajn::SessionOpts::ALL_NAMES);
    policyDb->NameOwnerChanged(DENIED_BUSNAME, nullptr, ajn::SessionOpts::ALL_NAMES,
                               &uniqueName, ajn::SessionOpts::ALL_NAMES);

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DENIED_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_EndpointHasBothDeniedAndAllowedBusnameAllowingRuleLast_Allows)
{
    const char* ALLOWED_BUSNAME = "com.allowed";
    const char* DENIED_BUSNAME = "com.denied";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    denyingRuleAttributes["send_destination"] = DENIED_BUSNAME;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    allowingRuleAttributes["send_destination"] = ALLOWED_BUSNAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, ALLOWED_BUSNAME);
    policyDb->Finalize(bus.get());

    qcc::String uniqueName = endpoint->GetUniqueName();
    policyDb->AddAlias(DENIED_BUSNAME, endpoint->GetUniqueName());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DENIED_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_EndpointHasBothDeniedAndAllowedBusnameDenyingRuleLast_Denies)
{
    const char* ALLOWED_BUSNAME = "com.allowed";
    const char* DENIED_BUSNAME = "com.denied";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    denyingRuleAttributes["send_destination"] = DENIED_BUSNAME;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    allowingRuleAttributes["send_destination"] = ALLOWED_BUSNAME;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, ALLOWED_BUSNAME);
    policyDb->Finalize(bus.get());

    qcc::String uniqueName = endpoint->GetUniqueName();
    policyDb->AddAlias(DENIED_BUSNAME, endpoint->GetUniqueName());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DENIED_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DenyAllInterfaces_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["send_interface"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DESTINATION_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DeniedInterface_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    std::map<qcc::String, qcc::String> ruleAttributes;
    const char* DENIED_INTERFACE = "com.denied";
    ruleAttributes["send_interface"] = DENIED_INTERFACE;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DESTINATION_BUSNAME, "/obj/path",
                                           DENIED_INTERFACE, "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_AllowingAndDenyingInterfaceRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* DENIED_INTERFACE = "com.denied";
    const char* ALLOWED_INTERFACE = "com.allowed";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    denyingRuleAttributes["send_interface"] = DENIED_INTERFACE;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    allowingRuleAttributes["send_interface"] = ALLOWED_INTERFACE;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           ALLOWED_INTERFACE, "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           DENIED_INTERFACE, "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToSend(allowedNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToSend(deniedNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DenyAllTypes_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["send_type"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DESTINATION_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DeniedType_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    const char* DENIED_TYPE = "method_call";
    ruleAttributes["send_type"] = DENIED_TYPE;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DESTINATION_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DenyingAndAllowingTypeRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    allowingRuleAttributes["send_type"] = "error";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    denyingRuleAttributes["send_type"] = "method_call";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", DESTINATION_BUSNAME, "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg("com.bus", "org.alljoyn.Error.Foo"));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToSend(allowedNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToSend(deniedNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DenyAllMembers_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["send_member"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DeniedMember_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    const char* DENIED_MEMBER = "deniedMethod";
    ruleAttributes["send_member"] = DENIED_MEMBER;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", DENIED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DenyingAndAllowingMemberRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* DENIED_MEMBER = "deniedMethod";
    const char* ALLOWED_MEMBER = "allowedMethod";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    denyingRuleAttributes["send_member"] = DENIED_MEMBER;
    allowingRuleAttributes["send_member"] = ALLOWED_MEMBER;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", ALLOWED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", DENIED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToSend(allowedNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToSend(deniedNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DenyAllPaths_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["send_path"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DeniedPath_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    const char* DENIED_PATH = "/obj/denied";
    ruleAttributes["send_path"] = DENIED_PATH;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", DENIED_PATH,
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DenyingAndAllowingPathRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* DENIED_PATH = "/obj/denied";
    const char* ALLOWED_PATH = "/obj/allowed";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    denyingRuleAttributes["send_path"] = DENIED_PATH;
    allowingRuleAttributes["send_path"] = ALLOWED_PATH;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", ALLOWED_PATH,
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", DENIED_PATH,
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToSend(allowedNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToSend(deniedNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DenyAllErrorMsgs_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["send_error"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg("com.bus", "org.alljoyn.Error.Foo"));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DeniedErrorMsg_Denies)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    const char* DENIED_ERRORMSG = "org.alljoyn.Error.Denied";
    ruleAttributes["send_error"] = DENIED_ERRORMSG;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg("com.bus", DENIED_ERRORMSG));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_FALSE(policyDb->OKToSend(nmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_DenyingAndAllowingErrorMsgRules_AllowsAccordingly)
{
    const char* DESTINATION_BUSNAME = "com.destination";
    const char* DENIED_ERRORMSG = "org.alljoyn.Error.Denied";
    const char* ALLOWED_ERRORMSG = "org.alljoyn.Error.Allowed";
    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    std::map<qcc::String, qcc::String> allowingRuleAttributes;
    denyingRuleAttributes["send_error"] = DENIED_ERRORMSG;
    allowingRuleAttributes["send_error"] = ALLOWED_ERRORMSG;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "allow", allowingRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg("com.bus", ALLOWED_ERRORMSG));
    ajn::NormalizedMsgHdr allowedNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg("com.bus", DENIED_ERRORMSG));
    ajn::NormalizedMsgHdr deniedNmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToSend(allowedNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToSend(deniedNmh, endpoint));
}

TEST_F(PolicyDBTest, OKToSend_ComplexRule_AllowsAccordingly)
{
    const char* DENIED_INTERFACE = "com.denied.iface";
    const char* DENIED_PATH = "/obj/denied";
    const char* DENIED_BUSNAME = "com.denied.busname";
    const char* DENIED_MEMBER = "deniedMethod";
    const char* DIFFERENT_BUSNAME = "com.different.busname";

    std::map<qcc::String, qcc::String> denyingRuleAttributes;
    denyingRuleAttributes["send_interface"] = DENIED_INTERFACE;
    denyingRuleAttributes["send_path"] = DENIED_PATH;
    denyingRuleAttributes["send_destination"] = DENIED_BUSNAME;
    denyingRuleAttributes["send_member"] = DENIED_MEMBER;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", denyingRuleAttributes));

    ajn::BusEndpoint deniedEndpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(deniedEndpoint, DENIED_BUSNAME);
    ajn::BusEndpoint differentEndpoint = CreateEndpoint(":Endpoint.2");
    RegisterEndpoint(differentEndpoint, DIFFERENT_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.sender", DENIED_BUSNAME, DENIED_PATH,
                                           DENIED_INTERFACE, DENIED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr matchingNmh(msg, *policyDb, deniedEndpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.sender", DENIED_BUSNAME, DENIED_PATH,
                                           "com.different.iface", DENIED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr differentInterfaceNmh(msg, *policyDb, deniedEndpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.sender", DENIED_BUSNAME, "/obj/different",
                                           DENIED_INTERFACE, DENIED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr differentPathNmh(msg, *policyDb, deniedEndpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.sender", DIFFERENT_BUSNAME, DENIED_PATH,
                                           DENIED_INTERFACE, DENIED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr differentBusnameNmh(msg, *policyDb, differentEndpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.sender", DENIED_BUSNAME, DENIED_PATH,
                                           DENIED_INTERFACE, "differentMember", NULL, 0));
    ajn::NormalizedMsgHdr differentMemberNmh(msg, *policyDb, deniedEndpoint);

    EXPECT_FALSE(policyDb->OKToSend(matchingNmh, deniedEndpoint));
    EXPECT_TRUE(policyDb->OKToSend(differentInterfaceNmh, deniedEndpoint));
    EXPECT_TRUE(policyDb->OKToSend(differentPathNmh, deniedEndpoint));
    EXPECT_TRUE(policyDb->OKToSend(differentBusnameNmh, differentEndpoint));
    EXPECT_TRUE(policyDb->OKToSend(differentMemberNmh, deniedEndpoint));
}

TEST_F(PolicyDBTest, OKToSend_MultipleRules_AllowsAccordingly)
{
    const char* DENIED_INTERFACE = "com.denied.iface";
    const char* DENIED_PATH = "/obj/denied";
    const char* DENIED_BUSNAME = "com.denied.busname";
    const char* DENIED_TYPE = "error";
    const char* DENIED_MEMBER = "deniedMethod";
    const char* DIFFERENT_BUSNAME = "com.different.busname";

    std::map<qcc::String, qcc::String> firstRuleAttributes;
    firstRuleAttributes["send_interface"] = DENIED_INTERFACE;
    firstRuleAttributes["send_path"] = DENIED_PATH;
    std::map<qcc::String, qcc::String> secondRuleAttributes;
    secondRuleAttributes["send_destination"] = DENIED_BUSNAME;
    std::map<qcc::String, qcc::String> thirdRuleAttributes;
    thirdRuleAttributes["send_type"] = DENIED_TYPE;
    std::map<qcc::String, qcc::String> fourthRuleAttributes;
    fourthRuleAttributes["send_member"] = DENIED_MEMBER;
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", firstRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", secondRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", thirdRuleAttributes));
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", fourthRuleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DENIED_BUSNAME);
    ajn::BusEndpoint differentEndpoint = CreateEndpoint(":Endpoint.2");
    RegisterEndpoint(differentEndpoint, DIFFERENT_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.sender", "com.destination", DENIED_PATH,
                                           DENIED_INTERFACE, "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr firstRuleNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.sender", DENIED_BUSNAME, "/obj/different",
                                           "com.different.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr secondRuleNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeErrorMsg("com.sender", "org.alljoyn.Error.Foo"));
    ajn::NormalizedMsgHdr thirdRuleNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.sender", "com.destination", DENIED_PATH,
                                           "com.different.interface", DENIED_MEMBER, NULL, 0));
    ajn::NormalizedMsgHdr fourthRuleNmh(msg, *policyDb, endpoint);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.different.busname", "com.destination", DENIED_PATH,
                                           "com.different.interface", "differentMethod", NULL, 0));
    ajn::NormalizedMsgHdr notMatchingNmh(msg, *policyDb, differentEndpoint);

    EXPECT_FALSE(policyDb->OKToSend(firstRuleNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToSend(secondRuleNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToSend(thirdRuleNmh, endpoint));
    EXPECT_FALSE(policyDb->OKToSend(fourthRuleNmh, endpoint));
    EXPECT_TRUE(policyDb->OKToSend(notMatchingNmh, differentEndpoint));
}

TEST_F(PolicyDBTest, OKToSend_DenyingReceiveRule_Allows)
{
    const char* DESTINATION_BUSNAME = "com.destination";

    std::map<qcc::String, qcc::String> ruleAttributes;
    ruleAttributes["receive_sender"] = "*";
    ASSERT_TRUE(policyDb->AddRule("context", "default", "deny", ruleAttributes));

    ajn::BusEndpoint endpoint = CreateEndpoint(":Endpoint.1");
    RegisterEndpoint(endpoint, DESTINATION_BUSNAME);
    policyDb->Finalize(bus.get());

    TestMessage msg(*bus);
    ASSERT_EQ(ER_OK, msg.MakeMethodCallMsg("com.bus", "com.destination", "/obj/path",
                                           "com.iface", "testMethod", NULL, 0));
    ajn::NormalizedMsgHdr nmh(msg, *policyDb, endpoint);

    EXPECT_TRUE(policyDb->OKToSend(nmh, endpoint));
}
