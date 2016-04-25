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

static AJ_PCSTR VALID_ALL_CASES_MANIFEST_TEMPLATE =
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
        delete [] mutableMembers;
    }
    static void ChangeMemberActionMask(PermissionPolicy::Rule& rule, size_t memberIndex, uint8_t newActionMask)
    {
        PermissionPolicy::Rule::Member* mutableMembers = nullptr;
        GetMembersCopy(rule, &mutableMembers);

        mutableMembers[memberIndex].SetActionMask(newActionMask);
        rule.SetMembers(rule.GetMembersSize(), mutableMembers);
        delete [] mutableMembers;
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
    PermissionPolicy::Rule* validRules;
    size_t rulesCount;
    string retrievedManifestTemplateXml;

    XmlRulesConverterToXmlDetailedTest() :
        validRules(nullptr),
        rulesCount(0)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(VALID_ALL_CASES_MANIFEST_TEMPLATE, rulesVector));
        validRules = rulesVector.data();
        rulesCount = rulesVector.size();
    }

  private:
    std::vector<PermissionPolicy::Rule> rulesVector;
};

class XmlRulesConverterToXmlDetailedFailureTest : public XmlRulesConverterToXmlDetailedTest { };

class XmlRulesConverterToXmlDetailedPassTest : public XmlRulesConverterToXmlDetailedTest {
  public:
    std::vector<PermissionPolicy::Rule> retrievedRules;
};

class XmlRulesConverterToXmlInvalidElementNamesTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    PermissionPolicy::Rule* rulesWithFlaw;
    size_t rulesCount;
    string retrievedManifestTemplateXml;

    XmlRulesConverterToXmlInvalidElementNamesTest() :
        rulesWithFlaw(nullptr),
        rulesCount(0)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(VALID_ALL_CASES_MANIFEST_TEMPLATE, rulesVector));
        rulesWithFlaw = rulesVector.data();
        rulesCount = rulesVector.size();
        FlawRules();
    }

  protected:

    virtual void FlawRules() = 0;

  private:
    std::vector<PermissionPolicy::Rule> rulesVector;
};

class XmlRulesConverterToXmlInvalidObjectPathsTest : public XmlRulesConverterToXmlInvalidElementNamesTest {
  protected:

    virtual void FlawRules()
    {
        rulesWithFlaw[0].SetObjPath(GetParam());
    }
};

class XmlRulesConverterToXmlInvalidInterfaceNamesTest : public XmlRulesConverterToXmlInvalidElementNamesTest {
  protected:

    virtual void FlawRules()
    {
        rulesWithFlaw[0].SetInterfaceName(GetParam());
    }
};

class XmlRulesConverterToXmlInvalidMemberNamesTest : public XmlRulesConverterToXmlInvalidElementNamesTest {
  protected:

    virtual void FlawRules()
    {
        MembersOverwriteUtils::ChangeMemberName(rulesWithFlaw[0], 0, GetParam());
    }
};

class XmlRulesConverterToXmlPassTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    PermissionPolicy::Rule* rules;
    size_t rulesCount;
    string retrievedManifestTemplateXml;

    XmlRulesConverterToXmlPassTest() :
        rules(nullptr),
        rulesCount(0)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(GetParam(), rulesVector));
        rules = rulesVector.data();
        rulesCount = rulesVector.size();
    }

  private:
    std::vector<PermissionPolicy::Rule> rulesVector;
};

class XmlRulesConverterToXmlCountTest : public testing::TestWithParam<SizeParams> {
  public:
    PermissionPolicy::Rule* rules;
    std::vector<PermissionPolicy::Rule> retrievedRules;
    size_t rulesCount;
    size_t expectedCount;
    string retrievedManifestTemplateXml;

    XmlRulesConverterToXmlCountTest() :
        rules(nullptr),
        rulesCount(0),
        expectedCount(GetParam().integer)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(GetParam().rulesXml, rulesVector));
        rules = rulesVector.data();
        rulesCount = rulesVector.size();
    }

  private:
    std::vector<PermissionPolicy::Rule> rulesVector;
};

class XmlRulesConverterToXmlRulesCountTest : public XmlRulesConverterToXmlCountTest { };

class XmlRulesConverterToXmlMembersCountTest : public XmlRulesConverterToXmlCountTest { };

class XmlRulesConverterToXmlMemberNamesTest : public testing::TestWithParam<TwoStringsParams> {
  public:
    PermissionPolicy::Rule* rules;
    std::vector<PermissionPolicy::Rule> retrievedRules;
    size_t rulesCount;
    string retrievedManifestTemplateXml;
    std::vector<std::string> expectedNames;

    XmlRulesConverterToXmlMemberNamesTest() :
        rules(nullptr),
        rulesCount(0),
        expectedNames(GetParam().strings)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(GetParam().rulesXml, rulesVector));
        rules = rulesVector.data();
        rulesCount = rulesVector.size();
    }

  private:
    std::vector<PermissionPolicy::Rule> rulesVector;
};

class XmlRulesConverterToXmlSameInterfaceMemberNamesTest : public XmlRulesConverterToXmlMemberNamesTest { };

class XmlRulesConverterToXmlSeparateInterfaceMemberNamesTest : public XmlRulesConverterToXmlMemberNamesTest { };

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForNonPositiveRulesCount)
{
    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, 0, retrievedManifestTemplateXml));
}

#ifdef REGEX_SUPPORTED

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForMissingNodeName)
{
    validRules[0].SetObjPath("");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForMissingInterfaceName)
{
    validRules[0].SetInterfaceName("");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForMissingMemberName)
{
    MembersOverwriteUtils::ChangeMemberName(validRules[0], METHOD_MEMBER_INDEX, "");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
}

#endif /* REGEX_SUPPORTED */

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForRuleWithZeroMembers)
{
    validRules[0].SetMembers(0, nullptr);

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSameNameInterfacesInSeparateSameNameNodes)
{
    validRules[2].SetObjPath("/Node0");
    validRules[2].SetInterfaceName("org.interface0");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSameNameInterfacesInSameNode)
{
    validRules[1].SetInterfaceName("org.interface0");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSameNameMethods)
{
    MembersOverwriteUtils::ChangeMemberName(validRules[0], METHOD_MEMBER_INDEX, "Method1");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSameNameProperties)
{
    MembersOverwriteUtils::ChangeMemberName(validRules[0], PROPERTY_MEMBER_INDEX, "Property1");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSameNameSignals)
{
    MembersOverwriteUtils::ChangeMemberName(validRules[0], SIGNAL_MEMBER_INDEX, "Signal1");

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForMethodWithObserve)
{
    MembersOverwriteUtils::ChangeMemberActionMask(validRules[0], METHOD_MEMBER_INDEX, PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedFailureTest, shouldReturnErrorForSignalWithModify)
{
    MembersOverwriteUtils::ChangeMemberActionMask(validRules[0], SIGNAL_MEMBER_INDEX, PermissionPolicy::Rule::Member::ACTION_MODIFY);

    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetSameRulesCountAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));

    EXPECT_EQ(rulesCount, retrievedRules.size());
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetSameRulesAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));
    ASSERT_EQ(rulesCount, retrievedRules.size());

    for (size_t index = 0; index < rulesCount; index++) {
        EXPECT_EQ(validRules[index], retrievedRules[index]);
    }
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetSameXmlAfterTwoConversions)
{
    string secondRetrievedManifestTemplateXml;
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(retrievedRules.data(), retrievedRules.size(), secondRetrievedManifestTemplateXml));

    EXPECT_EQ(retrievedManifestTemplateXml, secondRetrievedManifestTemplateXml);
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetValidMethodForValidAllCasesManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));
    ASSERT_EQ((size_t)3, retrievedRules.size());
    ASSERT_EQ((size_t)6, retrievedRules[0].GetMembersSize());

    PermissionPolicy::Rule::Member methodMember = retrievedRules[0].GetMembers()[0];

    EXPECT_STREQ("Method0", methodMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::METHOD_CALL, methodMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY
              | PermissionPolicy::Rule::Member::ACTION_PROVIDE, methodMember.GetActionMask());
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetValidPropertyForValidNeedAllManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));
    ASSERT_EQ((size_t)3, retrievedRules.size());
    ASSERT_EQ((size_t)6, retrievedRules[0].GetMembersSize());

    PermissionPolicy::Rule::Member propertyMember = retrievedRules[0].GetMembers()[1];

    EXPECT_STREQ("Property0", propertyMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::PROPERTY, propertyMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_MODIFY
              | PermissionPolicy::Rule::Member::ACTION_PROVIDE
              | PermissionPolicy::Rule::Member::ACTION_OBSERVE, propertyMember.GetActionMask());
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetValidSignalForValidNeedAllManifestTemplate)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));
    ASSERT_EQ((size_t)3, retrievedRules.size());
    ASSERT_EQ((size_t)6, retrievedRules[0].GetMembersSize());

    PermissionPolicy::Rule::Member signalMember = retrievedRules[0].GetMembers()[2];

    EXPECT_STREQ("Signal0", signalMember.GetMemberName().c_str());
    EXPECT_EQ(PermissionPolicy::Rule::Member::MemberType::SIGNAL, signalMember.GetMemberType());
    EXPECT_EQ(PermissionPolicy::Rule::Member::ACTION_PROVIDE
              | PermissionPolicy::Rule::Member::ACTION_OBSERVE, signalMember.GetActionMask());
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetValidSpecificNodeName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));
    ASSERT_EQ((size_t)3, retrievedRules.size());

    EXPECT_STREQ("/Node0", retrievedRules[0].GetObjPath().c_str());
}

TEST_F(XmlRulesConverterToXmlDetailedPassTest, shouldGetValidSpecificInterfaceName)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(validRules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));
    ASSERT_EQ((size_t)3, retrievedRules.size());

    EXPECT_STREQ("org.interface0", retrievedRules[0].GetInterfaceName().c_str());
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
    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(rulesWithFlaw, rulesCount, retrievedManifestTemplateXml));
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
    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(rulesWithFlaw, rulesCount, retrievedManifestTemplateXml));
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlInvalidNames,
                        XmlRulesConverterToXmlInvalidMemberNamesTest,
                        ::testing::Values("Method**",
                                          "Method!",
                                          "0Method",
                                          "Meth*d"));
TEST_P(XmlRulesConverterToXmlInvalidMemberNamesTest, shouldReturnErrorForInvalidMemberName)
{
    EXPECT_EQ(ER_FAIL, XmlRulesConverter::RulesToXml(rulesWithFlaw, rulesCount, retrievedManifestTemplateXml));
}

#endif /* REGEX_SUPPORTED */

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlPass,
                        XmlRulesConverterToXmlPassTest,
                        ::testing::Values(VALID_ALL_CASES_MANIFEST_TEMPLATE,
                                          VALID_NEED_ALL_MANIFEST_TEMPLATE,
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
TEST_P(XmlRulesConverterToXmlPassTest, shouldPassForValidInput)
{
    EXPECT_EQ(ER_OK, XmlRulesConverter::RulesToXml(rules, rulesCount, retrievedManifestTemplateXml));
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlRulesCount,
                        XmlRulesConverterToXmlRulesCountTest,
                        ::testing::Values(SizeParams(VALID_SAME_NAME_INTERFACES_IN_SEPARATE_NODES, 2),
                                          SizeParams(VALID_NAMELESS_INTERFACES_IN_SEPARATE_NODES, 2),
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
TEST_P(XmlRulesConverterToXmlRulesCountTest, shouldGetCorrectRulesCount)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(rules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));

    EXPECT_EQ(expectedCount, retrievedRules.size());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlSameInerfaceMembersCount,
                        XmlRulesConverterToXmlMembersCountTest,
                        ::testing::Values(SizeParams(VALID_DIFFERENT_NAME_METHODS_IN_ONE_INTERFACE, 2),
                                          SizeParams(VALID_DIFFERENT_NAME_PROPERTIES_IN_ONE_INTERFACE, 2),
                                          SizeParams(VALID_DIFFERENT_NAME_SIGNALS_IN_ONE_INTERFACE, 2),
                                          SizeParams(VALID_NEED_ALL_MANIFEST_TEMPLATE, 3)));
TEST_P(XmlRulesConverterToXmlMembersCountTest, shouldGetCorrectMembersCount)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(rules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));
    ASSERT_EQ((size_t)1, retrievedRules.size());

    EXPECT_EQ(expectedCount, retrievedRules[0].GetMembersSize());
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlSameInerfaceMemberNames,
                        XmlRulesConverterToXmlSameInterfaceMemberNamesTest,
                        ::testing::Values(TwoStringsParams(VALID_DIFFERENT_NAME_METHODS_IN_ONE_INTERFACE, "Method0", "Method1"),
                                          TwoStringsParams(VALID_DIFFERENT_NAME_PROPERTIES_IN_ONE_INTERFACE, "Property0", "Property1"),
                                          TwoStringsParams(VALID_DIFFERENT_NAME_SIGNALS_IN_ONE_INTERFACE, "Signal0", "Signal1")));
TEST_P(XmlRulesConverterToXmlSameInterfaceMemberNamesTest, shouldGetCorrectSameInterfaceMemberNames)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(rules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));
    ASSERT_EQ((size_t)1, retrievedRules.size());
    ASSERT_EQ(expectedNames.size(), retrievedRules[0].GetMembersSize());

    for (size_t index = 0; index < expectedNames.size(); index++) {
        EXPECT_EQ(expectedNames.at(index), retrievedRules[0].GetMembers()[index].GetMemberName().c_str());
    }
}

INSTANTIATE_TEST_CASE_P(XmlRulesConverterToXmlSeparateInerfaceMemberNames,
                        XmlRulesConverterToXmlSeparateInterfaceMemberNamesTest,
                        ::testing::Values(TwoStringsParams(VALID_SAME_NAME_METHODS_IN_SEPARATE_INTERFACES, "Method", "Method"),
                                          TwoStringsParams(VALID_SAME_NAME_PROPERTIES_IN_SEPARATE_INTERFACES, "Property", "Property"),
                                          TwoStringsParams(VALID_SAME_NAME_SIGNALS_IN_SEPARATE_INTERFACES, "Signal", "Signal"),
                                          TwoStringsParams(VALID_NAMELESS_METHODS_IN_SEPARATE_INTERFACES, "*", "*"),
                                          TwoStringsParams(VALID_NAMELESS_PROPERTIES_IN_SEPARATE_INTERFACES, "*", "*"),
                                          TwoStringsParams(VALID_NAMELESS_SIGNALS_IN_SEPARATE_INTERFACES, "*", "*")));
TEST_P(XmlRulesConverterToXmlSeparateInterfaceMemberNamesTest, shouldGetCorrectSeparateInterfacesMemberNames)
{
    ASSERT_EQ(ER_OK, XmlRulesConverter::RulesToXml(rules, rulesCount, retrievedManifestTemplateXml));
    ASSERT_EQ(ER_OK, XmlRulesConverter::XmlToRules(retrievedManifestTemplateXml.c_str(), retrievedRules));
    ASSERT_EQ(expectedNames.size(), retrievedRules.size());

    for (size_t index = 0; index < expectedNames.size(); index++) {
        ASSERT_EQ((size_t)1, retrievedRules[index].GetMembersSize());

        EXPECT_EQ(expectedNames.at(index), retrievedRules[index].GetMembers()[0].GetMemberName().c_str());
    }
}
