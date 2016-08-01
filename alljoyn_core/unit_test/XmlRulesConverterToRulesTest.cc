/******************************************************************************
 *  *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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
#include <alljoyn/Status.h>
#include <gtest/gtest.h>
#include <qcc/platform.h>

#include "XmlRulesConverter.h"
#include "XmlRulesConverterTest.h"


using namespace std;
using namespace qcc;
using namespace ajn;

static AJ_PCSTR NON_WELL_FORMED_XML = "<abc>";
static AJ_PCSTR EMPTY_MANIFEST_ELEMENT =
    "<manifest>"
    "</manifest>";
static AJ_PCSTR EMPTY_NODE_ELEMENT =
    "<manifest>"
    "<node>"
    "</node>"
    "</manifest>";
static AJ_PCSTR EMPTY_INTERFACE_ELEMENT =
    "<manifest>"
    "<node>"
    "<interface>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR EMPTY_MEMBER_ELEMENT =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR ANNOTATION_ELEMENT_UNDER_MANIFEST =
    "<manifest>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</manifest>";
static AJ_PCSTR ANNOTATION_ELEMENT_UNDER_NODE =
    "<manifest>"
    "<node>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</node>"
    "</manifest>";
static AJ_PCSTR ANNOTATION_ELEMENT_UNDER_INTERFACE =
    "<manifest>"
    "<node>"
    "<interface>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR BOTH_NODE_AND_ANNOTATION_ELEMENTS =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</manifest>";
static AJ_PCSTR BOTH_INTERFACE_AND_ANNOTATION_ELEMENTS =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "<annotation name = \"org.alljoyn.Bus.Action\" />"
    "</node>"
    "</manifest>";
static AJ_PCSTR BOTH_MEMBER_AND_ANNOTATION_ELEMENTS =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR ANNOTATION_WITH_MISSING_VALUE =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR ANNOTATION_WITH_INVALID_NAME =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Invalid.Name\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR REPEATED_SAME_ANNOTATION =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SAME_NAME_NODES =
    "<manifest>"
    "<node name = \"/Node\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "<node name = \"/Node\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SAME_NAMELESS_NODES =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SAME_NAME_INTERFACES =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "<interface name = \"org.interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SAME_NAMELESS_INTERFACES =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SAME_NAME_METHODS =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"Method\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "<method name = \"Method\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SAME_NAME_PROPERTIES =
    "<manifest>"
    "<node>"
    "<interface>"
    "<property name = \"Property\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</property>"
    "<property name = \"Property\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</property>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SAME_NAME_SIGNALS =
    "<manifest>"
    "<node>"
    "<interface>"
    "<signal name = \"Signal\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</signal>"
    "<signal name = \"Signal\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SAME_NAMELESS_METHODS =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SAME_NAMELESS_PROPERTIES =
    "<manifest>"
    "<node>"
    "<interface>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</property>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</property>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SAME_NAMELESS_SIGNALS =
    "<manifest>"
    "<node>"
    "<interface>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</signal>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR METHOD_WITH_OBSERVE =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SIGNAL_WITH_MODIFY =
    "<manifest>"
    "<node>"
    "<interface>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR METHOD_WITH_DOUBLE_DENY =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR METHOD_WITH_DENY_AND_OTHER =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SIGNAL_WITH_DOUBLE_DENY =
    "<manifest>"
    "<node>"
    "<interface>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR SIGNAL_WITH_DENY_AND_OTHER =
    "<manifest>"
    "<node>"
    "<interface>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR PROPERTY_WITH_DOUBLE_DENY =
    "<manifest>"
    "<node>"
    "<interface>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "</property>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR PROPERTY_WITH_DENY_AND_OTHER =
    "<manifest>"
    "<node>"
    "<interface>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</property>"
    "</interface>"
    "</node>"
    "</manifest>";

#ifdef REGEX_SUPPORTED
static AJ_PCSTR NODE_NAME_WITHOUT_SLASH =
    "<manifest>"
    "<node name = \"Node\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR NODE_NAME_WITH_SPECIAL_CHARACTER =
    "<manifest>"
    "<node name = \"/Node!\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR NODE_NAME_ENDING_WITH_SLASH =
    "<manifest>"
    "<node name = \"/Node/\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR NODE_NAME_WILDCARD_NOT_AFTER_SLASH =
    "<manifest>"
    "<node name = \"/Node*\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR NODE_NAME_DOUBLE_WILDCARD =
    "<manifest>"
    "<node name = \"/Node/**\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR NODE_NAME_WILDCARD_IN_MIDDLE =
    "<manifest>"
    "<node name = \"/Node/*/MoreNode\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR NODE_NAME_MULTIPLE_SLASH =
    "<manifest>"
    "<node name = \"/Node//MoreNode\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR INTERFACE_NAME_JUST_ONE_ELEMENT =
    "<manifest>"
    "<node>"
    "<interface name = \"org\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR INTERFACE_NAME_ELEMENT_STARTING_WITH_DIGIT =
    "<manifest>"
    "<node>"
    "<interface name = \"org.1interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR INTERFACE_NAME_DOUBLE_DOT =
    "<manifest>"
    "<node>"
    "<interface name = \"org..interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR INTERFACE_NAME_ENDING_WITH_DOT =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface.\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR INTERFACE_NAME_SPECIAL_CHARACTER =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interf@ce\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR INTERFACE_NAME_OVER_255_CHARACTERS =
    "<manifest>"
    "<node>"
    "<interface name = \"Org.interface.with.an.extremely.long.name.that.just.wont."
    "end.because.it.has.to.be.over.two.hundred.fifty_five.characters.long.Were.in."
    "the.middle.now.so.I.have.to.go.on.and.on.and.on.and.it.feels.pretty.much.like."
    "writing.an.essey.at.school.only.this.text.makes.slightly.more.sense.and.more"
    ".than.one.person.might.even.read.it.Thank.you\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR INTERFACE_NAME_WILDCARD_NOT_AFTER_DOT =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface*\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR INTERFACE_NAME_DOUBLE_WILDCARD =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface**\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR INTERFACE_NAME_WILDCARD_IN_MIDDLE =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface.*.someMoreInterfaceName\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR MEMBER_NAME_WILDCARD_IN_MIDDLE =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"some*MethodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR MEMBER_NAME_STARTING_WITH_DIGIT =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"0someMethodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR MEMBER_NAME_SPECIAL_CHARACTER =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"some.MethodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR MEMBER_NAME_DOUBLE_WILDCARD =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"someMethodName**\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
#endif /* REGEX_SUPPORTED */

class XmlRulesConverterToRulesDetailedTest : public testing::Test {
  public:
    std::vector<PermissionPolicy::Rule> rules;
};

class XmlRulesConverterToRulesBasicTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    std::vector<PermissionPolicy::Rule> rules;
    AJ_PCSTR rulesXml;

    XmlRulesConverterToRulesBasicTest() :
        rulesXml(GetParam())
    { }
};

class XmlRulesConverterToRulesInvalidRulesTest : public XmlRulesConverterToRulesBasicTest { };

class XmlRulesConverterToRulesInvalidNamesTest : public XmlRulesConverterToRulesBasicTest { };

class XmlRulesConverterToRulesPassTest : public XmlRulesConverterToRulesBasicTest { };

class XmlRulesConverterToRulesCountTest : public testing::TestWithParam<SizeParams> {
  public:
    std::vector<PermissionPolicy::Rule> rules;
    size_t expectedCount;
    AJ_PCSTR rulesXml;

    XmlRulesConverterToRulesCountTest() :
        expectedCount(GetParam().integer),
        rulesXml(GetParam().rulesXml)
    { }
};

class XmlRulesConverterToRulesRulesCountTest : public XmlRulesConverterToRulesCountTest { };

class XmlRulesConverterToRulesMembersCountTest : public XmlRulesConverterToRulesCountTest { };

class XmlRulesConverterToRulesMemberNamesTest : public testing::TestWithParam<TwoStringsParams> {
  public:
    std::vector<PermissionPolicy::Rule> rules;
    AJ_PCSTR rulesXml;
    std::vector<std::string> expectedNames;

    XmlRulesConverterToRulesMemberNamesTest() :
        rulesXml(GetParam().rulesXml),
        expectedNames(GetParam().strings)
    { }
};

class XmlRulesConverterToRulesSameInterfaceMemberNamesTest : public XmlRulesConverterToRulesMemberNamesTest { };

class XmlRulesConverterToRulesSeparateInterfaceMemberNamesTest : public XmlRulesConverterToRulesMemberNamesTest { };

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldReturnErrorForNonWellFormedXml)
{
    EXPECT_EQ(ER_EOF, XmlRulesConverter::XmlToRules(NON_WELL_FORMED_XML, rules));
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidMethodForValidNeedAllManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(VALID_NEED_ALL_MANIFEST_TEMPLATE, rules));
    ASSERT_EQ((size_t)1, rules.size());
    ASSERT_EQ((size_t)3, rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member methodMember = rules[0].GetMembers()[0];

    EXPECT_STREQ("*", methodMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::METHOD_CALL, methodMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY
              | PermissionPolicy::Rule::Member::ACTION_PROVIDE, methodMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidPropertyForValidNeedAllManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(VALID_NEED_ALL_MANIFEST_TEMPLATE, rules));
    ASSERT_EQ((size_t)1, rules.size());
    ASSERT_EQ((size_t)3, rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member propertyMember = rules[0].GetMembers()[1];

    EXPECT_STREQ("*", propertyMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::PROPERTY, propertyMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY
              | PermissionPolicy::Rule::Member::ACTION_PROVIDE
              | PermissionPolicy::Rule::Member::ACTION_OBSERVE, propertyMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidSignalForValidNeedAllManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(VALID_NEED_ALL_MANIFEST_TEMPLATE, rules));
    ASSERT_EQ((size_t)1, rules.size());
    ASSERT_EQ((size_t)3, rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member signalMember = rules[0].GetMembers()[2];

    EXPECT_STREQ("*", signalMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::SIGNAL, signalMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_PROVIDE
              | PermissionPolicy::Rule::Member::ACTION_OBSERVE, signalMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidMethodForDenyAction)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(VALID_METHOD_WITH_DENY, rules));
    ASSERT_EQ((size_t)1, rules.size());
    ASSERT_EQ((size_t)1, rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member methodMember = rules[0].GetMembers()[0];

    EXPECT_EQ(0, methodMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidNamelessNodeName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(VALID_NEED_ALL_MANIFEST_TEMPLATE, rules));
    ASSERT_EQ((size_t)1, rules.size());

    EXPECT_STREQ("*", rules[0].GetObjPath().c_str());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidSpecificNodeName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(VALID_NODE_WITH_NAME, rules));
    ASSERT_EQ((size_t)1, rules.size());

    EXPECT_STREQ("/Node", rules[0].GetObjPath().c_str());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidNamelessInterfaceName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(VALID_NEED_ALL_MANIFEST_TEMPLATE, rules));
    ASSERT_EQ((size_t)1, rules.size());

    EXPECT_STREQ("*", rules[0].GetInterfaceName().c_str());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidSpecificInterfaceName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(VALID_INTERFACE_WITH_NAME, rules));
    ASSERT_EQ((size_t)1, rules.size());

    EXPECT_STREQ("org.Interface", rules[0].GetInterfaceName().c_str());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesInvalidRulesSet,
                        XmlRulesConverterToRulesInvalidRulesTest,
                        ::testing::Values(EMPTY_MANIFEST_ELEMENT,
                                          EMPTY_NODE_ELEMENT,
                                          EMPTY_INTERFACE_ELEMENT,
                                          EMPTY_MEMBER_ELEMENT,
                                          ANNOTATION_ELEMENT_UNDER_MANIFEST,
                                          ANNOTATION_ELEMENT_UNDER_NODE,
                                          ANNOTATION_ELEMENT_UNDER_INTERFACE,
                                          BOTH_NODE_AND_ANNOTATION_ELEMENTS,
                                          BOTH_INTERFACE_AND_ANNOTATION_ELEMENTS,
                                          BOTH_MEMBER_AND_ANNOTATION_ELEMENTS,
                                          ANNOTATION_WITH_MISSING_VALUE,
                                          ANNOTATION_WITH_INVALID_NAME,
                                          REPEATED_SAME_ANNOTATION,
                                          METHOD_WITH_OBSERVE,
                                          SIGNAL_WITH_MODIFY,
                                          SAME_NAME_NODES,
                                          SAME_NAMELESS_NODES,
                                          SAME_NAME_INTERFACES,
                                          SAME_NAMELESS_INTERFACES,
                                          SAME_NAME_METHODS,
                                          SAME_NAME_PROPERTIES,
                                          SAME_NAME_SIGNALS,
                                          SAME_NAMELESS_METHODS,
                                          SAME_NAMELESS_PROPERTIES,
                                          SAME_NAMELESS_SIGNALS,
                                          METHOD_WITH_DOUBLE_DENY,
                                          METHOD_WITH_DENY_AND_OTHER,
                                          SIGNAL_WITH_DOUBLE_DENY,
                                          SIGNAL_WITH_DENY_AND_OTHER,
                                          PROPERTY_WITH_DOUBLE_DENY,
                                          PROPERTY_WITH_DENY_AND_OTHER));
TEST_P(XmlRulesConverterToRulesInvalidRulesTest, shouldReturnErrorForInvalidRulesSet)
{
    EXPECT_EQ(ER_XML_MALFORMED, XmlRulesConverter::XmlToRules(rulesXml, rules));
}

#ifdef REGEX_SUPPORTED

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesInvalidNames,
                        XmlRulesConverterToRulesInvalidNamesTest,
                        ::testing::Values(NODE_NAME_ENDING_WITH_SLASH,
                                          NODE_NAME_MULTIPLE_SLASH,
                                          NODE_NAME_WITHOUT_SLASH,
                                          NODE_NAME_WITH_SPECIAL_CHARACTER,
                                          NODE_NAME_WILDCARD_NOT_AFTER_SLASH,
                                          NODE_NAME_WILDCARD_IN_MIDDLE,
                                          NODE_NAME_DOUBLE_WILDCARD,
                                          INTERFACE_NAME_DOUBLE_DOT,
                                          INTERFACE_NAME_ELEMENT_STARTING_WITH_DIGIT,
                                          INTERFACE_NAME_ENDING_WITH_DOT,
                                          INTERFACE_NAME_JUST_ONE_ELEMENT,
                                          INTERFACE_NAME_OVER_255_CHARACTERS,
                                          INTERFACE_NAME_SPECIAL_CHARACTER,
                                          INTERFACE_NAME_WILDCARD_IN_MIDDLE,
                                          INTERFACE_NAME_WILDCARD_NOT_AFTER_DOT,
                                          INTERFACE_NAME_DOUBLE_WILDCARD,
                                          MEMBER_NAME_DOUBLE_WILDCARD,
                                          MEMBER_NAME_SPECIAL_CHARACTER,
                                          MEMBER_NAME_STARTING_WITH_DIGIT,
                                          MEMBER_NAME_WILDCARD_IN_MIDDLE));
TEST_P(XmlRulesConverterToRulesInvalidNamesTest, shouldReturnErrorForInvalidNames)
{
    EXPECT_EQ(ER_XML_MALFORMED, XmlRulesConverter::XmlToRules(rulesXml, rules));
}

#endif /* REGEX_SUPPORTED */

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesPass,
                        XmlRulesConverterToRulesPassTest,
                        ::testing::Values(VALID_NEED_ALL_MANIFEST_TEMPLATE,
                                          VALID_SAME_NAME_INTERFACES_IN_SEPARATE_NODES,
                                          VALID_NAMELESS_INTERFACES_IN_SEPARATE_NODES,
                                          VALID_DIFFERENT_NAME_INTERFACES_IN_ONE_NODE,
                                          VALID_DIFFERENT_NAME_METHODS_IN_ONE_INTERFACE,
                                          VALID_SAME_NAME_METHODS_IN_SEPARATE_INTERFACES,
                                          VALID_NAMELESS_METHODS_IN_SEPARATE_INTERFACES,
                                          VALID_DIFFERENT_NAME_PROPERTIES_IN_ONE_INTERFACE,
                                          VALID_SAME_NAME_PROPERTIES_IN_SEPARATE_INTERFACES,
                                          VALID_NAMELESS_PROPERTIES_IN_SEPARATE_INTERFACES,
                                          VALID_NODE_WILDCARD_ONLY,
                                          VALID_NODE_WITH_DIGIT,
                                          VALID_NODE_WITH_NAME,
                                          VALID_NODE_WITH_UNDERSCORE,
                                          VALID_NODE_WITH_WILDCARD,
                                          VALID_INTERFACE_WITH_NAME,
                                          VALID_INTERFACE_WITH_DIGIT,
                                          VALID_INTERFACE_WITH_UNDERSCORE,
                                          VALID_INTERFACE_WITH_WILDCARD,
                                          VALID_MEMBER_WITH_DIGIT,
                                          VALID_MEMBER_WITH_NAME,
                                          VALID_MEMBER_WITH_UNDERSCORE,
                                          VALID_MEMBER_WITH_WILDCARD,
                                          VALID_METHOD_WITH_DENY));
TEST_P(XmlRulesConverterToRulesPassTest, shouldPassForValidInput)
{
    EXPECT_EQ(ER_OK, XmlRulesConverter::XmlToRules(rulesXml, rules));
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesRulesCount,
                        XmlRulesConverterToRulesRulesCountTest,
                        ::testing::Values(SizeParams(VALID_SAME_NAME_INTERFACES_IN_SEPARATE_NODES, 2),
                                          SizeParams(VALID_NAMELESS_INTERFACES_IN_SEPARATE_NODES, 2),
                                          SizeParams(VALID_DIFFERENT_NAME_INTERFACES_IN_ONE_NODE, 2),
                                          SizeParams(VALID_NEED_ALL_MANIFEST_TEMPLATE, 1),
                                          SizeParams(VALID_DIFFERENT_NAME_METHODS_IN_ONE_INTERFACE, 1),
                                          SizeParams(VALID_DIFFERENT_NAME_PROPERTIES_IN_ONE_INTERFACE, 1),
                                          SizeParams(VALID_DIFFERENT_NAME_SIGNALS_IN_ONE_INTERFACE, 1),
                                          SizeParams(VALID_SAME_NAME_METHODS_IN_SEPARATE_INTERFACES, 2),
                                          SizeParams(VALID_SAME_NAME_PROPERTIES_IN_SEPARATE_INTERFACES, 2),
                                          SizeParams(VALID_SAME_NAME_SIGNALS_IN_SEPARATE_INTERFACES, 2),
                                          SizeParams(VALID_NAMELESS_METHODS_IN_SEPARATE_INTERFACES, 2),
                                          SizeParams(VALID_NAMELESS_PROPERTIES_IN_SEPARATE_INTERFACES, 2),
                                          SizeParams(VALID_NAMELESS_SIGNALS_IN_SEPARATE_INTERFACES, 2)));
TEST_P(XmlRulesConverterToRulesRulesCountTest, shouldGetCorrectRulesCount)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(rulesXml, rules));

    EXPECT_EQ(expectedCount, rules.size());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesSameInerfaceMembersCount,
                        XmlRulesConverterToRulesMembersCountTest,
                        ::testing::Values(SizeParams(VALID_DIFFERENT_NAME_METHODS_IN_ONE_INTERFACE, 2),
                                          SizeParams(VALID_DIFFERENT_NAME_PROPERTIES_IN_ONE_INTERFACE, 2),
                                          SizeParams(VALID_DIFFERENT_NAME_SIGNALS_IN_ONE_INTERFACE, 2),
                                          SizeParams(VALID_NEED_ALL_MANIFEST_TEMPLATE, 3)));
TEST_P(XmlRulesConverterToRulesMembersCountTest, shouldGetCorrectMembersCount)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(rulesXml, rules));
    ASSERT_EQ((size_t)1, rules.size());

    EXPECT_EQ(expectedCount, rules[0].GetMembersSize());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesSameInerfaceMemberNames,
                        XmlRulesConverterToRulesSameInterfaceMemberNamesTest,
                        ::testing::Values(TwoStringsParams(VALID_DIFFERENT_NAME_METHODS_IN_ONE_INTERFACE, "Method0", "Method1"),
                                          TwoStringsParams(VALID_DIFFERENT_NAME_PROPERTIES_IN_ONE_INTERFACE, "Property0", "Property1"),
                                          TwoStringsParams(VALID_DIFFERENT_NAME_SIGNALS_IN_ONE_INTERFACE, "Signal0", "Signal1")));
TEST_P(XmlRulesConverterToRulesSameInterfaceMemberNamesTest, shouldGetCorrectSameInterfaceMemberNames)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(rulesXml, rules));
    ASSERT_EQ((size_t)1, rules.size());
    ASSERT_EQ(expectedNames.size(), rules[0].GetMembersSize());

    for (size_t index = 0; index < expectedNames.size(); index++) {
        EXPECT_EQ(expectedNames[index], rules[0].GetMembers()[index].GetMemberName().c_str());
    }
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesSeparateInerfaceMemberNames,
                        XmlRulesConverterToRulesSeparateInterfaceMemberNamesTest,
                        ::testing::Values(TwoStringsParams(VALID_SAME_NAME_METHODS_IN_SEPARATE_INTERFACES, "Method", "Method"),
                                          TwoStringsParams(VALID_SAME_NAME_PROPERTIES_IN_SEPARATE_INTERFACES, "Property", "Property"),
                                          TwoStringsParams(VALID_SAME_NAME_SIGNALS_IN_SEPARATE_INTERFACES, "Signal", "Signal"),
                                          TwoStringsParams(VALID_NAMELESS_METHODS_IN_SEPARATE_INTERFACES, "*", "*"),
                                          TwoStringsParams(VALID_NAMELESS_PROPERTIES_IN_SEPARATE_INTERFACES, "*", "*"),
                                          TwoStringsParams(VALID_NAMELESS_SIGNALS_IN_SEPARATE_INTERFACES, "*", "*")));
TEST_P(XmlRulesConverterToRulesSeparateInterfaceMemberNamesTest, shouldGetCorrectSeparateInterfacesMemberNames)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(rulesXml, rules));
    ASSERT_EQ(expectedNames.size(), rules.size());

    for (size_t index = 0; index < expectedNames.size(); index++) {
        ASSERT_EQ((size_t)1, rules[index].GetMembersSize());

        EXPECT_EQ(expectedNames[index], rules[index].GetMembers()[0].GetMemberName().c_str());
    }
}