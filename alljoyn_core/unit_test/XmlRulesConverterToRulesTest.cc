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
static AJ_PCSTR s_emptyRulesElement =
    "<rules>"
    "</rules>";
static AJ_PCSTR s_emptyNodeElement =
    "<rules>"
    "<node>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_emptyInterfaceElement =
    "<rules>"
    "<node>"
    "<interface>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_emptyMemberElement =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_annotationElementUnderRules =
    "<rules>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</rules>";
static AJ_PCSTR s_annotationElementUnderNode =
    "<rules>"
    "<node>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_annotationElementUnderInterface =
    "<rules>"
    "<node>"
    "<interface>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_bothNodeAndAnnotationElements =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</rules>";
static AJ_PCSTR s_bothInterfaceAndAnnotationElements =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "<annotation name = \"org.alljoyn.Bus.Action\" />"
    "</node>"
    "</rules>";
static AJ_PCSTR s_bothMemberAndAnnotationElements =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_annotationWithMissingValue =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_annotationWithInvalidName =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Invalid.Name\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_repeatedSameAnnotation =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_sameNameNodes =
    "<rules>"
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
    "</rules>";
static AJ_PCSTR s_sameNamelessNodes =
    "<rules>"
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
    "</rules>";
static AJ_PCSTR s_sameNameInterfaces =
    "<rules>"
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
    "</rules>";
static AJ_PCSTR s_sameNamelessInterfaces =
    "<rules>"
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
    "</rules>";
static AJ_PCSTR s_sameNameMethods =
    "<rules>"
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
    "</rules>";
static AJ_PCSTR s_sameNameProperties =
    "<rules>"
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
    "</rules>";
static AJ_PCSTR s_sameNameSignals =
    "<rules>"
    "<node>"
    "<interface>"
    "<signal name = \"Signal\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</signal>"
    "<signal name = \"Signal\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_sameNamelessMethods =
    "<rules>"
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
    "</rules>";
static AJ_PCSTR s_sameNamelessProperties =
    "<rules>"
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
    "</rules>";
static AJ_PCSTR s_sameNamelessSignals =
    "<rules>"
    "<node>"
    "<interface>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</signal>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_methodWithObserve =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_signalWithModify =
    "<rules>"
    "<node>"
    "<interface>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_methodWithDoubleDeny =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_methodWithDenyAndOther =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_signalWithDoubleDeny =
    "<rules>"
    "<node>"
    "<interface>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_signalWithDenyAndOther =
    "<rules>"
    "<node>"
    "<interface>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_propertyWithDoubleDeny =
    "<rules>"
    "<node>"
    "<interface>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "</property>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_propertyWithDenyAndOther =
    "<rules>"
    "<node>"
    "<interface>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</property>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_anyWithDoubleDeny =
    "<rules>"
    "<node>"
    "<interface>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_anyWithDenyAndOther =
    "<rules>"
    "<node>"
    "<interface>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</rules>";

#ifdef REGEX_SUPPORTED
static AJ_PCSTR s_nodeNameWithoutSlash =
    "<rules>"
    "<node name = \"Node\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_nodeNameWithSpecialCharacter =
    "<rules>"
    "<node name = \"/Node!\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_nodeNameEndingWithSlash =
    "<rules>"
    "<node name = \"/Node/\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_nodeNameDoubleWildcard =
    "<rules>"
    "<node name = \"/Node/**\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_nodeNameWildcardInMiddle =
    "<rules>"
    "<node name = \"/Node/*/MoreNode\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_nodeNameMultipleSlash =
    "<rules>"
    "<node name = \"/Node//MoreNode\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_interfaceNameJustOneElement =
    "<rules>"
    "<node>"
    "<interface name = \"org\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_interfaceNameElementStartingWithDigit =
    "<rules>"
    "<node>"
    "<interface name = \"org.1interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_interfaceNameDoubleDot =
    "<rules>"
    "<node>"
    "<interface name = \"org..interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_interfaceNameEndingWithDot =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface.\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_interfaceNameSpecialCharacter =
    "<rules>"
    "<node>"
    "<interface name = \"org.interf@ce\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_interfaceNameOver_255_CHARACTERS =
    "<rules>"
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
    "</rules>";
static AJ_PCSTR s_interfaceNameDoubleWildcard =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface**\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_interfaceNameWildcardInMiddle =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface.*.someMoreInterfaceName\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_memberNameWildcardInMiddle =
    "<rules>"
    "<node>"
    "<interface>"
    "<method name = \"some*MethodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_memberNameStartingWithDigit =
    "<rules>"
    "<node>"
    "<interface>"
    "<method name = \"0someMethodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_memberNameSpecialCharacter =
    "<rules>"
    "<node>"
    "<interface>"
    "<method name = \"some.MethodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
static AJ_PCSTR s_memberNameDoubleWildcard =
    "<rules>"
    "<node>"
    "<interface>"
    "<method name = \"someMethodName**\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
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

class XmlRulesConverterToRulesInvalidRulesTest : public testing::TestWithParam<StatusParams> {
  public:
    vector<PermissionPolicy::Rule> m_rules;
    AJ_PCSTR m_rulesXml;
    QStatus m_expectedStatus;

    XmlRulesConverterToRulesInvalidRulesTest() :
        m_rulesXml(GetParam().m_xml),
        m_expectedStatus(GetParam().m_status)
    {
    }
};

class XmlRulesConverterToRulesInvalidNamesTest : public XmlRulesConverterToRulesInvalidRulesTest { };

class XmlRulesConverterToRulesPassTest : public XmlRulesConverterToRulesBasicTest { };

class XmlRulesConverterToRulesCountTest : public testing::TestWithParam<SizeParams> {
  public:
    vector<PermissionPolicy::Rule> m_rules;
    size_t m_expectedCount;
    AJ_PCSTR m_rulesXml;

    XmlRulesConverterToRulesCountTest() :
        m_expectedCount(GetParam().m_integer),
        m_rulesXml(GetParam().m_xml)
    { }
};

class XmlRulesConverterToRulesRulesCountTest : public XmlRulesConverterToRulesCountTest { };

class XmlRulesConverterToRulesMembersCountTest : public XmlRulesConverterToRulesCountTest { };

class XmlRulesConverterToRulesMemberNamesTest : public testing::TestWithParam<TwoStringsParams> {
  public:
    AJ_PCSTR m_rulesXml;
    vector<string> m_expectedNames;
    vector<PermissionPolicy::Rule> m_rules;

    XmlRulesConverterToRulesMemberNamesTest() :
        m_rulesXml(GetParam().m_rulesXml),
        m_expectedNames(GetParam().m_strings)
    { }
};

class XmlRulesConverterToRulesSameInterfaceMemberNamesTest : public XmlRulesConverterToRulesMemberNamesTest { };

class XmlRulesConverterToRulesSeparateInterfaceMemberNamesTest : public XmlRulesConverterToRulesMemberNamesTest { };

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldReturnErrorForNonWellFormedXml)
{
    EXPECT_EQ(ER_EOF, XmlRulesConverter::GetInstance()->XmlToRules(s_nonWellFormedXml, m_rules));
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldSetManifestPolicyRuleType)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(s_validNeedAllRulesXml, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());

    EXPECT_EQ(PermissionPolicy::Rule::MANIFEST_POLICY_RULE, m_rules[0].GetRuleType());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidMethodForValidNeedAllRules)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(s_validNeedAllRulesXml, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());
    ASSERT_EQ((size_t)4, m_rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member methodMember = m_rules[0].GetMembers()[0];

    EXPECT_STREQ("*", methodMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::METHOD_CALL, methodMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY
              | PermissionPolicy::Rule::Member::ACTION_PROVIDE, methodMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidPropertyForValidNeedAllRules)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(s_validNeedAllRulesXml, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());
    ASSERT_EQ((size_t)4, m_rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member propertyMember = m_rules[0].GetMembers()[1];

    EXPECT_STREQ("*", propertyMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::PROPERTY, propertyMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY
              | PermissionPolicy::Rule::Member::ACTION_PROVIDE
              | PermissionPolicy::Rule::Member::ACTION_OBSERVE, propertyMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidSignalForValidNeedAllRules)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(s_validNeedAllRulesXml, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());
    ASSERT_EQ((size_t)4, m_rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member signalMember = m_rules[0].GetMembers()[2];

    EXPECT_STREQ("*", signalMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::SIGNAL, signalMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_PROVIDE
              | PermissionPolicy::Rule::Member::ACTION_OBSERVE, signalMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidAnyMemberForValidNeedAllRules)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(s_validNeedAllRulesXml, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());
    ASSERT_EQ((size_t)4, m_rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member anyMember = m_rules[0].GetMembers()[3];

    EXPECT_STREQ("*", anyMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::NOT_SPECIFIED, anyMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY
              | PermissionPolicy::Rule::Member::ACTION_PROVIDE
              | PermissionPolicy::Rule::Member::ACTION_OBSERVE, anyMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidMethodForDenyAction)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(s_validMethodWithDeny, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());
    ASSERT_EQ((size_t)1, m_rules[0].GetMembersSize());

    PermissionPolicy::Rule::Member methodMember = m_rules[0].GetMembers()[0];

    EXPECT_EQ(0, methodMember.GetActionMask());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidNamelessNodeName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(s_validNeedAllRulesXml, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());

    EXPECT_STREQ("*", m_rules[0].GetObjPath().c_str());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidSpecificNodeName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(s_validNodeWithName, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());

    EXPECT_STREQ("/Node", m_rules[0].GetObjPath().c_str());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidNamelessInterfaceName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(s_validNeedAllRulesXml, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());

    EXPECT_STREQ("*", m_rules[0].GetInterfaceName().c_str());
}

TEST_F(XmlRulesConverterToRulesDetailedTest, shouldGetValidSpecificInterfaceName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(s_validInterfaceWithName, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());

    EXPECT_STREQ("org.Interface", m_rules[0].GetInterfaceName().c_str());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesInvalidRulesSet,
                        XmlRulesConverterToRulesInvalidRulesTest,
                        ::testing::Values(StatusParams(s_emptyRulesElement, ER_INVALID_XML_ELEMENT_CHILDREN_COUNT),
                                          StatusParams(s_emptyNodeElement, ER_INVALID_XML_ELEMENT_CHILDREN_COUNT),
                                          StatusParams(s_emptyInterfaceElement, ER_INVALID_XML_ELEMENT_CHILDREN_COUNT),
                                          StatusParams(s_emptyMemberElement, ER_INVALID_XML_ELEMENT_CHILDREN_COUNT),
                                          StatusParams(s_annotationElementUnderRules, ER_INVALID_XML_ELEMENT_NAME),
                                          StatusParams(s_annotationElementUnderNode, ER_INVALID_ANNOTATIONS_COUNT),
                                          StatusParams(s_annotationElementUnderInterface, ER_INVALID_ANNOTATIONS_COUNT),
                                          StatusParams(s_bothNodeAndAnnotationElements, ER_INVALID_XML_ELEMENT_NAME),
                                          StatusParams(s_bothInterfaceAndAnnotationElements, ER_INVALID_ANNOTATIONS_COUNT),
                                          StatusParams(s_bothMemberAndAnnotationElements, ER_INVALID_ANNOTATIONS_COUNT),
                                          StatusParams(s_annotationWithMissingValue, ER_INVALID_MEMBER_ACTION),
                                          StatusParams(s_annotationWithInvalidName, ER_INVALID_XML_ATTRIBUTE_VALUE),
                                          StatusParams(s_repeatedSameAnnotation, ER_ANNOTATION_NOT_UNIQUE),
                                          StatusParams(s_methodWithObserve, ER_INVALID_MEMBER_ACTION),
                                          StatusParams(s_signalWithModify, ER_INVALID_MEMBER_ACTION),
                                          StatusParams(s_sameNameNodes, ER_OBJECT_PATH_NOT_UNIQUE),
                                          StatusParams(s_sameNamelessNodes, ER_OBJECT_PATH_NOT_UNIQUE),
                                          StatusParams(s_sameNameInterfaces, ER_INTERFACE_NAME_NOT_UNIQUE),
                                          StatusParams(s_sameNamelessInterfaces, ER_INTERFACE_NAME_NOT_UNIQUE),
                                          StatusParams(s_sameNameMethods, ER_MEMBER_NAME_NOT_UNIQUE),
                                          StatusParams(s_sameNameProperties, ER_MEMBER_NAME_NOT_UNIQUE),
                                          StatusParams(s_sameNameSignals, ER_MEMBER_NAME_NOT_UNIQUE),
                                          StatusParams(s_sameNamelessMethods, ER_MEMBER_NAME_NOT_UNIQUE),
                                          StatusParams(s_sameNamelessProperties, ER_MEMBER_NAME_NOT_UNIQUE),
                                          StatusParams(s_sameNamelessSignals, ER_MEMBER_NAME_NOT_UNIQUE),
                                          StatusParams(s_methodWithDoubleDeny, ER_ANNOTATION_NOT_UNIQUE),
                                          StatusParams(s_methodWithDenyAndOther, ER_MEMBER_DENY_ACTION_WITH_OTHER),
                                          StatusParams(s_signalWithDoubleDeny, ER_ANNOTATION_NOT_UNIQUE),
                                          StatusParams(s_signalWithDenyAndOther, ER_MEMBER_DENY_ACTION_WITH_OTHER),
                                          StatusParams(s_propertyWithDoubleDeny, ER_ANNOTATION_NOT_UNIQUE),
                                          StatusParams(s_propertyWithDenyAndOther, ER_MEMBER_DENY_ACTION_WITH_OTHER),
                                          StatusParams(s_anyWithDoubleDeny, ER_ANNOTATION_NOT_UNIQUE),
                                          StatusParams(s_anyWithDenyAndOther, ER_MEMBER_DENY_ACTION_WITH_OTHER),
                                          StatusParams(s_needAllManifestTemplateWithNodeSecurityLevelAnnotation, ER_INVALID_XML_ELEMENT_NAME),
                                          StatusParams(s_needAllManifestTemplateWithInterfaceSecurityLevelAnnotation, ER_INVALID_XML_ELEMENT_NAME)));
TEST_P(XmlRulesConverterToRulesInvalidRulesTest, shouldReturnErrorForInvalidRulesSet)
{
    EXPECT_EQ(m_expectedStatus, XmlRulesConverter::GetInstance()->XmlToRules(m_rulesXml, m_rules));
}

#ifdef REGEX_SUPPORTED

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesInvalidNames,
                        XmlRulesConverterToRulesInvalidNamesTest,
                        ::testing::Values(StatusParams(s_nodeNameEndingWithSlash, ER_INVALID_OBJECT_PATH),
                                          StatusParams(s_nodeNameMultipleSlash, ER_INVALID_OBJECT_PATH),
                                          StatusParams(s_nodeNameWithoutSlash, ER_INVALID_OBJECT_PATH),
                                          StatusParams(s_nodeNameWithSpecialCharacter, ER_INVALID_OBJECT_PATH),
                                          StatusParams(s_nodeNameWildcardInMiddle, ER_INVALID_OBJECT_PATH),
                                          StatusParams(s_nodeNameDoubleWildcard, ER_INVALID_OBJECT_PATH),
                                          StatusParams(s_interfaceNameDoubleDot, ER_INVALID_INTERFACE_NAME),
                                          StatusParams(s_interfaceNameElementStartingWithDigit, ER_INVALID_INTERFACE_NAME),
                                          StatusParams(s_interfaceNameEndingWithDot, ER_INVALID_INTERFACE_NAME),
                                          StatusParams(s_interfaceNameJustOneElement, ER_INVALID_INTERFACE_NAME),
                                          StatusParams(s_interfaceNameOver_255_CHARACTERS, ER_INVALID_INTERFACE_NAME),
                                          StatusParams(s_interfaceNameSpecialCharacter, ER_INVALID_INTERFACE_NAME),
                                          StatusParams(s_interfaceNameWildcardInMiddle, ER_INVALID_INTERFACE_NAME),
                                          StatusParams(s_interfaceNameDoubleWildcard, ER_INVALID_INTERFACE_NAME),
                                          StatusParams(s_memberNameDoubleWildcard, ER_INVALID_MEMBER_NAME),
                                          StatusParams(s_memberNameSpecialCharacter, ER_INVALID_MEMBER_NAME),
                                          StatusParams(s_memberNameStartingWithDigit, ER_INVALID_MEMBER_NAME),
                                          StatusParams(s_memberNameWildcardInMiddle, ER_INVALID_MEMBER_NAME)));
TEST_P(XmlRulesConverterToRulesInvalidNamesTest, shouldReturnErrorForInvalidNames)
{
    EXPECT_EQ(m_expectedStatus, XmlRulesConverter::GetInstance()->XmlToRules(m_rulesXml, m_rules));
}

#endif /* REGEX_SUPPORTED */

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesPass,
                        XmlRulesConverterToRulesPassTest,
                        ::testing::Values(s_validNeedAllRulesXml,
                                          s_validSameNameInterfacesInSeparateNodes,
                                          s_validNamelessInterfacesInSeparateNodes,
                                          s_validDifferentNameInterfacesInOneNode,
                                          s_validDifferentNameMethodsInOneInterface,
                                          s_validSameNameMethodsInSeparateInterfaces,
                                          s_validNamelessMethodsInSeparateInterfaces,
                                          s_validDifferentNamePropertiesInOneInterface,
                                          s_validSameNamePropertiesInSeparateInterfaces,
                                          s_validNamelessPropertiesInSeparateInterfaces,
                                          s_validDifferentNameSignalsInOneInterface,
                                          s_validSameNameSignalsInSeparateInterfaces,
                                          s_validNamelessSignalsInSeparateInterfaces,
                                          s_validDifferentNameAnyMembersInOneInterface,
                                          s_validSameNameAnyMembersInSeparateInterfaces,
                                          s_validNamelessAnyMembersInSeparateInterfaces,
                                          s_validInterfaceWithName,
                                          s_validInterfaceNameWithWildcardNotAfterDot,
                                          s_validInterfaceWithDigit,
                                          s_validInterfaceWithUnderscore,
                                          s_validInterfaceWithWildcard,
                                          s_validMemberWithDigit,
                                          s_validMemberWithName,
                                          s_validNodeWithDigit,
                                          s_validNodeWithName,
                                          s_validNodeWithWildcard,
                                          s_validNodeNameWithWildcardNotAfterSlash,
                                          s_validNodeWithUnderscore,
                                          s_validNodeWildcardOnly,
                                          s_validMemberWithUnderscore,
                                          s_validMemberWithWildcard,
                                          s_validMethodWithDeny));
TEST_P(XmlRulesConverterToRulesPassTest, shouldPassForValidInput)
{
    EXPECT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(m_rulesXml, m_rules));
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesRulesCount,
                        XmlRulesConverterToRulesRulesCountTest,
                        ::testing::Values(SizeParams(s_validSameNameInterfacesInSeparateNodes, 2),
                                          SizeParams(s_validNamelessInterfacesInSeparateNodes, 2),
                                          SizeParams(s_validDifferentNameInterfacesInOneNode, 2),
                                          SizeParams(s_validNeedAllRulesXml, 1),
                                          SizeParams(s_validDifferentNameMethodsInOneInterface, 1),
                                          SizeParams(s_validDifferentNamePropertiesInOneInterface, 1),
                                          SizeParams(s_validDifferentNameSignalsInOneInterface, 1),
                                          SizeParams(s_validDifferentNameAnyMembersInOneInterface, 1),
                                          SizeParams(s_validSameNamePropertiesInSeparateInterfaces, 2),
                                          SizeParams(s_validSameNameSignalsInSeparateInterfaces, 2),
                                          SizeParams(s_validNamelessMethodsInSeparateInterfaces, 2),
                                          SizeParams(s_validSameNameAnyMembersInSeparateInterfaces, 2),
                                          SizeParams(s_validNamelessSignalsInSeparateInterfaces, 2),
                                          SizeParams(s_validNamelessSignalsInSeparateInterfaces, 2),
                                          SizeParams(s_validNamelessAnyMembersInSeparateInterfaces, 2)));
TEST_P(XmlRulesConverterToRulesRulesCountTest, shouldGetCorrectRulesCount)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(m_rulesXml, m_rules));

    EXPECT_EQ(m_expectedCount, m_rules.size());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesSameInerfaceMembersCount,
                        XmlRulesConverterToRulesMembersCountTest,
                        ::testing::Values(SizeParams(s_validDifferentNameMethodsInOneInterface, 2),
                                          SizeParams(s_validDifferentNamePropertiesInOneInterface, 2),
                                          SizeParams(s_validDifferentNameSignalsInOneInterface, 2),
                                          SizeParams(s_validDifferentNameAnyMembersInOneInterface, 2),
                                          SizeParams(s_validNeedAllRulesXml, 4)));
TEST_P(XmlRulesConverterToRulesMembersCountTest, shouldGetCorrectMembersCount)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(m_rulesXml, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());

    EXPECT_EQ(m_expectedCount, m_rules[0].GetMembersSize());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesSameInerfaceMemberNames,
                        XmlRulesConverterToRulesSameInterfaceMemberNamesTest,
                        ::testing::Values(TwoStringsParams(s_validDifferentNameMethodsInOneInterface, "Method0", "Method1"),
                                          TwoStringsParams(s_validDifferentNamePropertiesInOneInterface, "Property0", "Property1"),
                                          TwoStringsParams(s_validDifferentNameSignalsInOneInterface, "Signal0", "Signal1"),
                                          TwoStringsParams(s_validDifferentNameAnyMembersInOneInterface, "Any0", "Any1")));
TEST_P(XmlRulesConverterToRulesSameInterfaceMemberNamesTest, shouldGetCorrectSameInterfaceMemberNames)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(m_rulesXml, m_rules));
    ASSERT_EQ((size_t)1, m_rules.size());
    ASSERT_EQ(m_expectedNames.size(), m_rules[0].GetMembersSize());

    for (size_t index = 0; index < m_expectedNames.size(); index++) {
        EXPECT_EQ(m_expectedNames[index], m_rules[0].GetMembers()[index].GetMemberName().c_str());
    }
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToRulesSeparateInerfaceMemberNames,
                        XmlRulesConverterToRulesSeparateInterfaceMemberNamesTest,
                        ::testing::Values(TwoStringsParams(s_validSameNameMethodsInSeparateInterfaces, "Method", "Method"),
                                          TwoStringsParams(s_validSameNamePropertiesInSeparateInterfaces, "Property", "Property"),
                                          TwoStringsParams(s_validSameNameSignalsInSeparateInterfaces, "Signal", "Signal"),
                                          TwoStringsParams(s_validSameNameAnyMembersInSeparateInterfaces, "Any", "Any"),
                                          TwoStringsParams(s_validNamelessPropertiesInSeparateInterfaces, "*", "*"),
                                          TwoStringsParams(s_validNamelessSignalsInSeparateInterfaces, "*", "*"),
                                          TwoStringsParams(s_validNamelessSignalsInSeparateInterfaces, "*", "*"),
                                          TwoStringsParams(s_validNamelessAnyMembersInSeparateInterfaces, "*", "*")));
TEST_P(XmlRulesConverterToRulesSeparateInterfaceMemberNamesTest, shouldGetCorrectSeparateInterfacesMemberNames)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::GetInstance()->XmlToRules(m_rulesXml, m_rules));
    ASSERT_EQ(m_expectedNames.size(), m_rules.size());

    for (size_t index = 0; index < m_expectedNames.size(); index++) {
        ASSERT_EQ((size_t)1, m_rules[index].GetMembersSize());

        EXPECT_EQ(m_expectedNames[index], m_rules[index].GetMembers()[0].GetMemberName().c_str());
    }
}
