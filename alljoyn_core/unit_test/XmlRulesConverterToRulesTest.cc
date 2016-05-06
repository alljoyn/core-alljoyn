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
#include <alljoyn/Status.h>
#include <gtest/gtest.h>
#include <qcc/platform.h>

#include "XmlRulesConverter.h"
#include "XmlRulesConverterTest.h"


using namespace std;
using namespace qcc;
using namespace ajn;

static AJ_PCSTR s_nonWellFormedXml = "<abc>";
static AJ_PCSTR s_emptyManifestElement =
    "<manifest>"
    "</manifest>";
static AJ_PCSTR s_emptyNodeElement =
    "<manifest>"
    "<node>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_emptyInterfaceElement =
    "<manifest>"
    "<node>"
    "<interface>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_emptyMemberElement =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_annotationElementUnderManifest =
    "<manifest>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</manifest>";
static AJ_PCSTR s_annotationElementUnderNode =
    "<manifest>"
    "<node>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_annotationElementUnderInterface =
    "<manifest>"
    "<node>"
    "<interface>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_bothNodeAndAnnotationElements =
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
static AJ_PCSTR s_bothInterfaceAndAnnotationElements =
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
static AJ_PCSTR s_bothMemberAndAnnotationElements =
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
static AJ_PCSTR s_annotationWithMissingValue =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_annotationWithInvalidName =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Invalid.Name\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_repeatedSameAnnotation =
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
static AJ_PCSTR s_sameNameNodes =
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
static AJ_PCSTR s_sameNamelessNodes =
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
static AJ_PCSTR s_sameNameInterfaces =
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
static AJ_PCSTR s_sameNamelessInterfaces =
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
static AJ_PCSTR s_sameNameMethods =
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
static AJ_PCSTR s_sameNameProperties =
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
static AJ_PCSTR s_sameNameSignals =
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
static AJ_PCSTR s_sameNamelessMethods =
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
static AJ_PCSTR s_sameNamelessProperties =
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
static AJ_PCSTR s_sameNamelessSignals =
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
static AJ_PCSTR s_methodWithObserve =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_signalWithModify =
    "<manifest>"
    "<node>"
    "<interface>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_methodWithDoubleDeny =
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
static AJ_PCSTR s_methodWithDenyAndOther =
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
static AJ_PCSTR s_signalWithDoubleDeny =
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
static AJ_PCSTR s_signalWithDenyAndOther =
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
static AJ_PCSTR s_propertyWithDoubleDeny =
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
static AJ_PCSTR s_propertyWithDenyAndOther =
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
static AJ_PCSTR s_nodeNameWithoutSlash =
    "<manifest>"
    "<node name = \"Node\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_nodeNameWithSpecialCharacter =
    "<manifest>"
    "<node name = \"/Node!\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_nodeNameEndingWithSlash =
    "<manifest>"
    "<node name = \"/Node/\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_nodeNameWildcardNotAfterSlash =
    "<manifest>"
    "<node name = \"/Node*\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_nodeNameDoubleWildcard =
    "<manifest>"
    "<node name = \"/Node/**\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_nodeNameWildcardInMiddle =
    "<manifest>"
    "<node name = \"/Node/*/MoreNode\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_nodeNameMultipleSlash =
    "<manifest>"
    "<node name = \"/Node//MoreNode\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_interfaceNameJustOneElement =
    "<manifest>"
    "<node>"
    "<interface name = \"org\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_interfaceNameElementStartingWithDigit =
    "<manifest>"
    "<node>"
    "<interface name = \"org.1interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_interfaceNameDoubleDot =
    "<manifest>"
    "<node>"
    "<interface name = \"org..interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_interfaceNameEndingWithDot =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface.\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_interfaceNameSpecialCharacter =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interf@ce\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_interfaceNameOver_255_CHARACTERS =
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
static AJ_PCSTR s_interfaceNameWildcardNotAfterDot =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface*\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_interfaceNameDoubleWildcard =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface**\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_interfaceNameWildcardInMiddle =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface.*.someMoreInterfaceName\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_memberNameWildcardInMiddle =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"some*MethodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_memberNameStartingWithDigit =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"0someMethodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_memberNameSpecialCharacter =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"some.MethodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_memberNameDoubleWildcard =
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
    vector<PermissionPolicy::Rule> m_rules;
};

class XmlRulesConverterToRulesBasicTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    vector<PermissionPolicy::Rule> m_rules;
    AJ_PCSTR m_rulesXml;

    XmlRulesConverterToRulesBasicTest() :
        m_rulesXml(GetParam())
    { }
};

class XmlRulesConverterToRulesInvalidRulesTest : public XmlRulesConverterToRulesBasicTest { };

class XmlRulesConverterToRulesInvalidNamesTest : public XmlRulesConverterToRulesBasicTest { };

class XmlRulesConverterToRulesPassTest : public XmlRulesConverterToRulesBasicTest { };

class XmlRulesConverterToRulesCountTest : public testing::TestWithParam<SizeParams> {
  public:
    vector<PermissionPolicy::Rule> m_rules;
    size_t m_expectedCount;
    AJ_PCSTR m_rulesXml;

    XmlRulesConverterToRulesCountTest() :
        m_expectedCount(GetParam().m_integer),
        m_rulesXml(GetParam().m_rulesXml)
    { }
};

class XmlRulesConverterToRulesRulesCountTest : public XmlRulesConverterToRulesCountTest { };

class XmlRulesConverterToRulesMembersCountTest : public XmlRulesConverterToRulesCountTest { };

class XmlRulesConverterToRulesMemberNamesTest : public testing::TestWithParam<TwoStringsParams> {
  public:
    AJ_PCSTR m_rulesXml;
    vector<string> m_expectedNames;
    vector<PermissionPolicy::Rule> rules;

    XmlRulesConverterToRulesMemberNamesTest() :
        m_rulesXml(GetParam().m_rulesXml),
        m_expectedNames(GetParam().m_strings)
    { }
};

class XmlRulesConverterToRulesSameInterfaceMemberNamesTest : public XmlRulesConverterToRulesMemberNamesTest { };

class XmlRulesConverterToRulesSeparateInterfaceMemberNamesTest : public XmlRulesConverterToRulesMemberNamesTest { };

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldReturnErrorForNonWellFormedXml)
{
    EXPECT_EQ(ER_EOF, XmlRulesConverter::XmlToRules(s_nonWellFormedXml, m_rules));
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidMethodForValidNeedAllManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(s_validNeedAllManifestTemplate, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());
    ASSERT_EQ((size_t)3, m_rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member methodMember = m_rules[0].GetMembers()[0];

    EXPECT_STREQ("*", methodMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::METHOD_CALL, methodMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY
              | PermissionPolicy::Rule::Member::ACTION_PROVIDE, methodMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidPropertyForValidNeedAllManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(s_validNeedAllManifestTemplate, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());
    ASSERT_EQ((size_t)3, m_rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member propertyMember = m_rules[0].GetMembers()[1];

    EXPECT_STREQ("*", propertyMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::PROPERTY, propertyMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY
              | PermissionPolicy::Rule::Member::ACTION_PROVIDE
              | PermissionPolicy::Rule::Member::ACTION_OBSERVE, propertyMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidSignalForValidNeedAllManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(s_validNeedAllManifestTemplate, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());
    ASSERT_EQ((size_t)3, m_rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member signalMember = m_rules[0].GetMembers()[2];

    EXPECT_STREQ("*", signalMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::SIGNAL, signalMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_PROVIDE
              | PermissionPolicy::Rule::Member::ACTION_OBSERVE, signalMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidMethodForDenyAction)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(s_validMethodWithDeny, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());
    ASSERT_EQ((size_t)1, m_rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member methodMember = m_rules[0].GetMembers()[0];

    EXPECT_EQ(0, methodMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidNamelessNodeName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(s_validNeedAllManifestTemplate, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());

    EXPECT_STREQ("*", m_rules[0].GetObjPath().c_str());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidSpecificNodeName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(s_validNodeWithName, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());

    EXPECT_STREQ("/Node", m_rules[0].GetObjPath().c_str());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidNamelessInterfaceName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(s_validNeedAllManifestTemplate, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());

    EXPECT_STREQ("*", m_rules[0].GetInterfaceName().c_str());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidSpecificInterfaceName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(s_validInterfaceWithName, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());

    EXPECT_STREQ("org.Interface", m_rules[0].GetInterfaceName().c_str());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesInvalidRulesSet,
                        XmlRulesConverterToRulesInvalidRulesTest,
                        ::testing::Values(s_emptyManifestElement,
                                          s_emptyNodeElement,
                                          s_emptyInterfaceElement,
                                          s_emptyMemberElement,
                                          s_annotationElementUnderManifest,
                                          s_annotationElementUnderNode,
                                          s_annotationElementUnderInterface,
                                          s_bothNodeAndAnnotationElements,
                                          s_bothInterfaceAndAnnotationElements,
                                          s_bothMemberAndAnnotationElements,
                                          s_annotationWithMissingValue,
                                          s_annotationWithInvalidName,
                                          s_repeatedSameAnnotation,
                                          s_methodWithObserve,
                                          s_signalWithModify,
                                          s_sameNameNodes,
                                          s_sameNamelessNodes,
                                          s_sameNameInterfaces,
                                          s_sameNamelessInterfaces,
                                          s_sameNameMethods,
                                          s_sameNameProperties,
                                          s_sameNameSignals,
                                          s_sameNamelessMethods,
                                          s_sameNamelessProperties,
                                          s_sameNamelessSignals,
                                          s_methodWithDoubleDeny,
                                          s_methodWithDenyAndOther,
                                          s_signalWithDoubleDeny,
                                          s_signalWithDenyAndOther,
                                          s_propertyWithDoubleDeny,
                                          s_propertyWithDenyAndOther));
TEST_P(XmlRulesConverterToRulesInvalidRulesTest, shouldReturnErrorForInvalidRulesSet)
{
    EXPECT_EQ(ER_XML_MALFORMED, XmlRulesConverter::XmlToRules(m_rulesXml, m_rules));
}

#ifdef REGEX_SUPPORTED

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesInvalidNames,
                        XmlRulesConverterToRulesInvalidNamesTest,
                        ::testing::Values(s_nodeNameEndingWithSlash,
                                          s_nodeNameMultipleSlash,
                                          s_nodeNameWithoutSlash,
                                          s_nodeNameWithSpecialCharacter,
                                          s_nodeNameWildcardNotAfterSlash,
                                          s_nodeNameWildcardInMiddle,
                                          s_nodeNameDoubleWildcard,
                                          s_interfaceNameDoubleDot,
                                          s_interfaceNameElementStartingWithDigit,
                                          s_interfaceNameEndingWithDot,
                                          s_interfaceNameJustOneElement,
                                          s_interfaceNameOver_255_CHARACTERS,
                                          s_interfaceNameSpecialCharacter,
                                          s_interfaceNameWildcardInMiddle,
                                          s_interfaceNameWildcardNotAfterDot,
                                          s_interfaceNameDoubleWildcard,
                                          s_memberNameDoubleWildcard,
                                          s_memberNameSpecialCharacter,
                                          s_memberNameStartingWithDigit,
                                          s_memberNameWildcardInMiddle));
TEST_P(XmlRulesConverterToRulesInvalidNamesTest, shouldReturnErrorForInvalidNames)
{
    EXPECT_EQ(ER_XML_MALFORMED, XmlRulesConverter::XmlToRules(m_rulesXml, m_rules));
}

#endif /* REGEX_SUPPORTED */

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesPass,
                        XmlRulesConverterToRulesPassTest,
                        ::testing::Values(s_validNeedAllManifestTemplate,
                                          s_validSameNameInterfacesInSeparateNodes,
                                          s_validNamelessInterfacesInSeparateNodes,
                                          s_validDifferentNameInterfacesInOneNode,
                                          s_validDifferentNameMethodsInOneInterface,
                                          s_validSameNameMethodsInSeparateInterfaces,
                                          s_validNamelessMethodsInSeparateInterfaces,
                                          s_validDifferentNamePropertiesInOneInterface,
                                          s_validSameNamePropertiesInSeparateInterfaces,
                                          s_validNamelessPropertiesInSeparateInterfaces,
                                          s_validNodeWildcardOnly,
                                          s_validNodeWithDigit,
                                          s_validNodeWithName,
                                          s_validNodeWithUnderscore,
                                          s_validNodeWithWildcard,
                                          s_validInterfaceWithName,
                                          s_validInterfaceWithDigit,
                                          s_validInterfaceWithUnderscore,
                                          s_validInterfaceWithWildcard,
                                          s_validMemberWithDigit,
                                          s_validMemberWithName,
                                          s_validMemberWithUnderscore,
                                          s_validMemberWithWildcard,
                                          s_validMethodWithDeny));
TEST_P(XmlRulesConverterToRulesPassTest, shouldPassForValidInput)
{
    EXPECT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_rulesXml, m_rules));
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesRulesCount,
                        XmlRulesConverterToRulesRulesCountTest,
                        ::testing::Values(SizeParams(s_validSameNameInterfacesInSeparateNodes, 2),
                                          SizeParams(s_validNamelessInterfacesInSeparateNodes, 2),
                                          SizeParams(s_validDifferentNameInterfacesInOneNode, 2),
                                          SizeParams(s_validNeedAllManifestTemplate, 1),
                                          SizeParams(s_validDifferentNameMethodsInOneInterface, 1),
                                          SizeParams(s_validDifferentNamePropertiesInOneInterface, 1),
                                          SizeParams(s_validDifferentNameSignalsInOneInterface, 1),
                                          SizeParams(s_validSameNameMethodsInSeparateInterfaces, 2),
                                          SizeParams(s_validSameNamePropertiesInSeparateInterfaces, 2),
                                          SizeParams(s_validSameNameSignalsInSeparateInterfaces, 2),
                                          SizeParams(s_validNamelessMethodsInSeparateInterfaces, 2),
                                          SizeParams(s_validNamelessPropertiesInSeparateInterfaces, 2),
                                          SizeParams(s_validNamelessSignalsInSeparateInterfaces, 2)));
TEST_P(XmlRulesConverterToRulesRulesCountTest, shouldGetCorrectRulesCount)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_rulesXml, m_rules));

    EXPECT_EQ(m_expectedCount, m_rules.size());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesSameInerfaceMembersCount,
                        XmlRulesConverterToRulesMembersCountTest,
                        ::testing::Values(SizeParams(s_validDifferentNameMethodsInOneInterface, 2),
                                          SizeParams(s_validDifferentNamePropertiesInOneInterface, 2),
                                          SizeParams(s_validDifferentNameSignalsInOneInterface, 2),
                                          SizeParams(s_validNeedAllManifestTemplate, 3)));
TEST_P(XmlRulesConverterToRulesMembersCountTest, shouldGetCorrectMembersCount)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_rulesXml, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());

    EXPECT_EQ(m_expectedCount, m_rules[0].GetMembersSize());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesSameInerfaceMemberNames,
                        XmlRulesConverterToRulesSameInterfaceMemberNamesTest,
                        ::testing::Values(TwoStringsParams(s_validDifferentNameMethodsInOneInterface, "Method0", "Method1"),
                                          TwoStringsParams(s_validDifferentNamePropertiesInOneInterface, "Property0", "Property1"),
                                          TwoStringsParams(s_validDifferentNameSignalsInOneInterface, "Signal0", "Signal1")));
TEST_P(XmlRulesConverterToRulesSameInterfaceMemberNamesTest, shouldGetCorrectSameInterfaceMemberNames)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_rulesXml, rules));
    ASSERT_EQ((size_t)1, rules.size());
    ASSERT_EQ(m_expectedNames.size(), rules[0].GetMembersSize());

    for (size_t index = 0; index < m_expectedNames.size(); index++) {
        EXPECT_EQ(m_expectedNames[index], rules[0].GetMembers()[index].GetMemberName().c_str());
    }
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesSeparateInerfaceMemberNames,
                        XmlRulesConverterToRulesSeparateInterfaceMemberNamesTest,
                        ::testing::Values(TwoStringsParams(s_validSameNameMethodsInSeparateInterfaces, "Method", "Method"),
                                          TwoStringsParams(s_validSameNamePropertiesInSeparateInterfaces, "Property", "Property"),
                                          TwoStringsParams(s_validSameNameSignalsInSeparateInterfaces, "Signal", "Signal"),
                                          TwoStringsParams(s_validNamelessMethodsInSeparateInterfaces, "*", "*"),
                                          TwoStringsParams(s_validNamelessPropertiesInSeparateInterfaces, "*", "*"),
                                          TwoStringsParams(s_validNamelessSignalsInSeparateInterfaces, "*", "*")));
TEST_P(XmlRulesConverterToRulesSeparateInterfaceMemberNamesTest, shouldGetCorrectSeparateInterfacesMemberNames)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_rulesXml, rules));
    ASSERT_EQ(m_expectedNames.size(), rules.size());

    for (size_t index = 0; index < m_expectedNames.size(); index++) {
        ASSERT_EQ((size_t)1, rules[index].GetMembersSize());

        EXPECT_EQ(m_expectedNames[index], rules[index].GetMembers()[0].GetMemberName().c_str());
    }
}
