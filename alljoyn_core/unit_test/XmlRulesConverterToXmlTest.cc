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
#include <string>

#include "XmlRulesConverter.h"
#include "XmlRulesConverterTest.h"


using namespace std;
using namespace qcc;
using namespace ajn;

#define LONG_INTERFACE_NAME "Org.interface.with.an.extremely.long.name.that.just.wont." \
    "end.because.it.has.to.be.over.two.hundred.fifty.five.characters.long.We.are.in." \
    "the.middle.now.so.I.still.have.to.go.on.for.quite.a.while.and.it.feels.pretty.much." \
    "like.writing.an.essay.at.school.only.this.text.makes.slightly.more.sense.and.more." \
    "than.one.person.might.even.read.it.Thank.you"

#define METHOD_MEMBER_INDEX 0
#define PROPERTY_MEMBER_INDEX 1
#define SIGNAL_MEMBER_INDEX 2

static AJ_PCSTR s_validAllCasesManifestTemplate =
    "<manifest>"
    "<node name = \"/Node0\">"
    "<interface name = \"org.interface0\">"
    "<method name = \"Method0\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "<property name = \"Property0\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</property>"
    "<signal name = \"Signal0\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</signal>"
    "<method name = \"Method1\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "<property name = \"Property1\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</property>"
    "<signal name = \"Signal1\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</signal>"
    "</interface>"
    "<interface name = \"org.interface1\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</property>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "<node name = \"/Node1\">"
    "<interface name = \"org.interface0\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</property>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</manifest>";

class MembersOverwriteUtils {
  public:

    static void ChangeMemberName(PermissionPolicy::Rule& rule, size_t memberIndex, AJ_PCSTR newName)
    {
        PermissionPolicy::Rule::Member* mutableMembers = nullptr;
        GetMembersCopy(rule, &mutableMembers);

        mutableMembers[memberIndex].SetMemberName(newName);
        rule.SetMembers(rule.GetMembersSize(), mutableMembers);

        delete[] mutableMembers;
    }
    static void ChangeMemberActionMask(PermissionPolicy::Rule& rule, size_t memberIndex, uint8_t newActionMask)
    {
        PermissionPolicy::Rule::Member* mutableMembers = nullptr;
        GetMembersCopy(rule, &mutableMembers);

        mutableMembers[memberIndex].SetActionMask(newActionMask);
        rule.SetMembers(rule.GetMembersSize(), mutableMembers);

        delete[] mutableMembers;
    }

  private:

    static void GetMembersCopy(const PermissionPolicy::Rule& rule, PermissionPolicy::Rule::Member** mutableMembers)
    {
        size_t membersSize = rule.GetMembersSize();
        *mutableMembers = new PermissionPolicy::Rule::Member[membersSize];

        for (size_t index = 0; index < membersSize; index++) {
            (*mutableMembers)[index] = rule.GetMembers()[index];
        }
    }
};

class XmlRulesConverterToXmlDetailedTest : public testing::Test {
  public:
    PermissionPolicy::Rule* m_validRules;
    size_t m_rulesCount;
    string m_retrievedManifestTemplateXml;

    XmlRulesConverterToXmlDetailedTest() :
        m_validRules(nullptr),
        m_rulesCount(0)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(s_validAllCasesManifestTemplate, m_rulesVector));
        m_validRules = m_rulesVector.data();
        m_rulesCount = m_rulesVector.size();
    }

  private:
    vector<PermissionPolicy::Rule> m_rulesVector;
};

class XmlRulesConverterToXmlDetailedFailureTest : public XmlRulesConverterToXmlDetailedTest { };

class XmlRulesConverterToXmlDetailedPassTest : public XmlRulesConverterToXmlDetailedTest {
  public:
    vector<PermissionPolicy::Rule> m_retrievedRules;
};

class XmlRulesConverterToXmlInvalidElementNamesTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    PermissionPolicy::Rule* m_rulesWithFlaw;
    size_t m_rulesCount;
    string m_retrievedManifestTemplateXml;

    XmlRulesConverterToXmlInvalidElementNamesTest() :
        m_rulesWithFlaw(nullptr),
        m_rulesCount(0)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(s_validAllCasesManifestTemplate, m_rulesVector));
        m_rulesWithFlaw = m_rulesVector.data();
        m_rulesCount = m_rulesVector.size();
        FlawRules();
    }

  protected:

    virtual void FlawRules() = 0;

  private:
    vector<PermissionPolicy::Rule> m_rulesVector;
};

class XmlRulesConverterToXmlInvalidObjectPathsTest : public XmlRulesConverterToXmlInvalidElementNamesTest {
  protected:

    virtual void FlawRules()
    {
        m_rulesWithFlaw[0].SetObjPath(GetParam());
    }
};

class XmlRulesConverterToXmlInvalidInterfaceNamesTest : public XmlRulesConverterToXmlInvalidElementNamesTest {
  protected:

    virtual void FlawRules()
    {
        m_rulesWithFlaw[0].SetInterfaceName(GetParam());
    }
};

class XmlRulesConverterToXmlInvalidMemberNamesTest : public XmlRulesConverterToXmlInvalidElementNamesTest {
  protected:

    virtual void FlawRules()
    {
        MembersOverwriteUtils::ChangeMemberName(m_rulesWithFlaw[0], 0, GetParam());
    }
};

class XmlRulesConverterToXmlPassTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    PermissionPolicy::Rule* m_rules;
    size_t m_rulesCount;
    string m_retrievedManifestTemplateXml;

    XmlRulesConverterToXmlPassTest() :
        m_rules(nullptr),
        m_rulesCount(0)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(GetParam(), m_rulesVector));
        m_rules = m_rulesVector.data();
        m_rulesCount = m_rulesVector.size();
    }

  private:
    vector<PermissionPolicy::Rule> m_rulesVector;
};

class XmlRulesConverterToXmlCountTest : public testing::TestWithParam<SizeParams> {
  public:
    PermissionPolicy::Rule* m_rules;
    vector<PermissionPolicy::Rule> m_retrievedRules;
    size_t m_rulesCount;
    size_t m_expectedCount;
    string m_retrievedManifestTemplateXml;

    XmlRulesConverterToXmlCountTest() :
        m_rules(nullptr),
        m_rulesCount(0),
        m_expectedCount(GetParam().m_integer)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(GetParam().m_rulesXml, m_rulesVector));
        m_rules = m_rulesVector.data();
        m_rulesCount = m_rulesVector.size();
    }

  private:
    vector<PermissionPolicy::Rule> m_rulesVector;
};

class XmlRulesConverterToXmlRulesCountTest : public XmlRulesConverterToXmlCountTest { };

class XmlRulesConverterToXmlMembersCountTest : public XmlRulesConverterToXmlCountTest { };

class XmlRulesConverterToXmlMemberNamesTest : public testing::TestWithParam<TwoStringsParams> {
  public:
    PermissionPolicy::Rule* m_rules;
    vector<PermissionPolicy::Rule> m_retrievedRules;
    size_t m_rulesCount;
    string m_retrievedManifestTemplateXml;
    vector<string> m_expectedNames;

    XmlRulesConverterToXmlMemberNamesTest() :
        m_rules(nullptr),
        m_rulesCount(0),
        m_expectedNames(GetParam().m_strings)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(GetParam().m_rulesXml, m_rulesVector));
        m_rules = m_rulesVector.data();
        m_rulesCount = m_rulesVector.size();
    }

  private:
    vector<PermissionPolicy::Rule> m_rulesVector;
};

class XmlRulesConverterToXmlSameInterfaceMemberNamesTest : public XmlRulesConverterToXmlMemberNamesTest { };

class XmlRulesConverterToXmlSeparateInterfaceMemberNamesTest : public XmlRulesConverterToXmlMemberNamesTest { };

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForNonPositiveRulesCount)
{
    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, 0, m_retrievedManifestTemplateXml));
}

#ifdef REGEX_SUPPORTED

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForMissingNodeName)
{
    m_validRules[0].SetObjPath("");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForMissingInterfaceName)
{
    m_validRules[0].SetInterfaceName("");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForMissingMemberName)
{
    MembersOverwriteUtils::ChangeMemberName(m_validRules[0], METHOD_MEMBER_INDEX, "");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
}

#endif /* REGEX_SUPPORTED */

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForRuleWithZeroMembers)
{
    m_validRules[0].SetMembers(0, nullptr);

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSameNameInterfacesInSeparateSameNameNodes)
{
    m_validRules[2].SetObjPath("/Node0");
    m_validRules[2].SetInterfaceName("org.interface0");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSameNameInterfacesInSameNode)
{
    m_validRules[1].SetInterfaceName("org.interface0");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSameNameMethods)
{
    MembersOverwriteUtils::ChangeMemberName(m_validRules[0], METHOD_MEMBER_INDEX, "Method1");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSameNameProperties)
{
    MembersOverwriteUtils::ChangeMemberName(m_validRules[0], PROPERTY_MEMBER_INDEX, "Property1");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSameNameSignals)
{
    MembersOverwriteUtils::ChangeMemberName(m_validRules[0], SIGNAL_MEMBER_INDEX, "Signal1");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForMethodWithObserve)
{
    MembersOverwriteUtils::ChangeMemberActionMask(m_validRules[0], METHOD_MEMBER_INDEX, PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSignalWithModify)
{
    MembersOverwriteUtils::ChangeMemberActionMask(m_validRules[0], SIGNAL_MEMBER_INDEX, PermissionPolicy::Rule::Member::ACTION_MODIFY);

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetSameRulesCountAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));

    EXPECT_EQ(m_rulesCount, m_retrievedRules.size());
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetSameRulesAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));
    ASSERT_EQ(m_rulesCount, m_retrievedRules.size());

    for (size_t index = 0; index < m_rulesCount; index++) {
        EXPECT_EQ(m_validRules[index], m_retrievedRules[index]);
    }
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetSameXmlAfterTwoConversions)
{
    string secondRetrievedManifestTemplateXml;
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_retrievedRules.data(), m_retrievedRules.size(), secondRetrievedManifestTemplateXml));

    EXPECT_EQ(m_retrievedManifestTemplateXml, secondRetrievedManifestTemplateXml);
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetValidMethodForValidAllCasesManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));
    ASSERT_EQ((size_t)3, m_retrievedRules.size());
    ASSERT_EQ((size_t)6, m_retrievedRules[0].GetMembersSize());

    PermissionPolicy::Rule::Member methodMember = m_retrievedRules[0].GetMembers()[0];

    EXPECT_STREQ("Method0", methodMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::METHOD_CALL, methodMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY
              | PermissionPolicy::Rule::Member::ACTION_PROVIDE, methodMember.GetActionMask());
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetValidPropertyForValidNeedAllManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));
    ASSERT_EQ((size_t)3, m_retrievedRules.size());
    ASSERT_EQ((size_t)6, m_retrievedRules[0].GetMembersSize());

    PermissionPolicy::Rule::Member propertyMember = m_retrievedRules[0].GetMembers()[1];

    EXPECT_STREQ("Property0", propertyMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::PROPERTY, propertyMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY
              | PermissionPolicy::Rule::Member::ACTION_PROVIDE
              | PermissionPolicy::Rule::Member::ACTION_OBSERVE, propertyMember.GetActionMask());
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetValidSignalForValidNeedAllManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));
    ASSERT_EQ((size_t)3, m_retrievedRules.size());
    ASSERT_EQ((size_t)6, m_retrievedRules[0].GetMembersSize());

    PermissionPolicy::Rule::Member signalMember = m_retrievedRules[0].GetMembers()[2];

    EXPECT_STREQ("Signal0", signalMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::SIGNAL, signalMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_PROVIDE
              | PermissionPolicy::Rule::Member::ACTION_OBSERVE, signalMember.GetActionMask());
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetValidSpecificNodeName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));
    ASSERT_EQ((size_t)3, m_retrievedRules.size());

    EXPECT_STREQ("/Node0", m_retrievedRules[0].GetObjPath().c_str());
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetValidSpecificInterfaceName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_validRules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));
    ASSERT_EQ((size_t)3, m_retrievedRules.size());

    EXPECT_STREQ("org.interface0", m_retrievedRules[0].GetInterfaceName().c_str());
}

#ifdef REGEX_SUPPORTED

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlInvalidObjectPaths,
                        XmlRulesConverterToXmlInvalidObjectPathsTest,
                        ::testing::Values("/Node/",
                                          "/Node//Node",
                                          "Node",
                                          "/Node!",
                                          "/Node*",
                                          "/Node/*/Node",
                                          "/Node**"));
TEST_P(XmlRulesConverterToXmlInvalidObjectPathsTest, shouldReturnErrorForInvalidObjectPath)
{
    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_rulesWithFlaw, m_rulesCount, m_retrievedManifestTemplateXml));
}


INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlInvalidNames,
                        XmlRulesConverterToXmlInvalidInterfaceNamesTest,
                        ::testing::Values("org..interface",
                                          ".org.interface",
                                          "org.interface.",
                                          "org",
                                          LONG_INTERFACE_NAME,
                                          "org.interf@ce",
                                          "org.interface.*.moreInterface",
                                          "org.interface*",
                                          "org.interface.**"));
TEST_P(XmlRulesConverterToXmlInvalidInterfaceNamesTest, shouldReturnErrorForInvalidInterfaceName)
{
    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_rulesWithFlaw, m_rulesCount, m_retrievedManifestTemplateXml));
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlInvalidNames,
                        XmlRulesConverterToXmlInvalidMemberNamesTest,
                        ::testing::Values("Method**",
                                          "Method!",
                                          "0Method",
                                          "Meth*d"));
TEST_P(XmlRulesConverterToXmlInvalidMemberNamesTest, shouldReturnErrorForInvalidMemberName)
{
    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(m_rulesWithFlaw, m_rulesCount, m_retrievedManifestTemplateXml));
}

#endif /* REGEX_SUPPORTED */

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlPass,
                        XmlRulesConverterToXmlPassTest,
                        ::testing::Values(s_validAllCasesManifestTemplate,
                                          s_validNeedAllManifestTemplate,
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
TEST_P(XmlRulesConverterToXmlPassTest, shouldPassForValidInput)
{
    EXPECT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_rules, m_rulesCount, m_retrievedManifestTemplateXml));
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlRulesCount,
                        XmlRulesConverterToXmlRulesCountTest,
                        ::testing::Values(SizeParams(s_validSameNameInterfacesInSeparateNodes, 2),
                                          SizeParams(s_validNamelessInterfacesInSeparateNodes, 2),
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
TEST_P(XmlRulesConverterToXmlRulesCountTest, shouldGetCorrectRulesCount)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_rules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));

    EXPECT_EQ(m_expectedCount, m_retrievedRules.size());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlSameInerfaceMembersCount,
                        XmlRulesConverterToXmlMembersCountTest,
                        ::testing::Values(SizeParams(s_validDifferentNameMethodsInOneInterface, 2),
                                          SizeParams(s_validDifferentNamePropertiesInOneInterface, 2),
                                          SizeParams(s_validDifferentNameSignalsInOneInterface, 2),
                                          SizeParams(s_validNeedAllManifestTemplate, 3)));
TEST_P(XmlRulesConverterToXmlMembersCountTest, shouldGetCorrectMembersCount)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_rules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));
    ASSERT_EQ((size_t)1, m_retrievedRules.size());

    EXPECT_EQ(m_expectedCount, m_retrievedRules[0].GetMembersSize());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlSameInerfaceMemberNames,
                        XmlRulesConverterToXmlSameInterfaceMemberNamesTest,
                        ::testing::Values(TwoStringsParams(s_validDifferentNameMethodsInOneInterface, "Method0", "Method1"),
                                          TwoStringsParams(s_validDifferentNamePropertiesInOneInterface, "Property0", "Property1"),
                                          TwoStringsParams(s_validDifferentNameSignalsInOneInterface, "Signal0", "Signal1")));
TEST_P(XmlRulesConverterToXmlSameInterfaceMemberNamesTest, shouldGetCorrectSameInterfaceMemberNames)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_rules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));
    ASSERT_EQ((size_t)1, m_retrievedRules.size());
    ASSERT_EQ(m_expectedNames.size(), m_retrievedRules[0].GetMembersSize());

    for (size_t index = 0; index < m_expectedNames.size(); index++) {
        EXPECT_EQ(m_expectedNames.at(index), m_retrievedRules[0].GetMembers()[index].GetMemberName().c_str());
    }
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlSeparateInerfaceMemberNames,
                        XmlRulesConverterToXmlSeparateInterfaceMemberNamesTest,
                        ::testing::Values(TwoStringsParams(s_validSameNameMethodsInSeparateInterfaces, "Method", "Method"),
                                          TwoStringsParams(s_validSameNamePropertiesInSeparateInterfaces, "Property", "Property"),
                                          TwoStringsParams(s_validSameNameSignalsInSeparateInterfaces, "Signal", "Signal"),
                                          TwoStringsParams(s_validNamelessMethodsInSeparateInterfaces, "*", "*"),
                                          TwoStringsParams(s_validNamelessPropertiesInSeparateInterfaces, "*", "*"),
                                          TwoStringsParams(s_validNamelessSignalsInSeparateInterfaces, "*", "*")));
TEST_P(XmlRulesConverterToXmlSeparateInterfaceMemberNamesTest, shouldGetCorrectSeparateInterfacesMemberNames)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(m_rules, m_rulesCount, m_retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(m_retrievedManifestTemplateXml.c_str(), m_retrievedRules));
    ASSERT_EQ(m_expectedNames.size(), m_retrievedRules.size());

    for (size_t index = 0; index < m_expectedNames.size(); index++) {
        ASSERT_EQ((size_t)1, m_retrievedRules[index].GetMembersSize());

        EXPECT_EQ(m_expectedNames.at(index), m_retrievedRules[index].GetMembers()[0].GetMemberName().c_str());
    }
}
