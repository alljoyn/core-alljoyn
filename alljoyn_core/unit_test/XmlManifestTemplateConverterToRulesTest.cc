/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#include <alljoyn/Status.h>
#include <gtest/gtest.h>
#include <qcc/platform.h>

#include "XmlManifestTemplateConverter.h"
#include "XmlRulesConverterTest.h"


using namespace std;
using namespace qcc;
using namespace ajn;

/**
 * The XmlManifestTemplateConverter extends the XmlRulesConverter and differs only
 * in the validation of node and interface level annotations. Since unit tests for the
 * XmlRulesConverter have covered cases that are same for both converters, these tests
 * focus only on the annotations validation.
 */

struct SecurityLevelParams {
  public:
    AJ_PCSTR m_manifestTemplateXml;
    PermissionPolicy::Rule::SecurityLevel m_securityLevel;

    SecurityLevelParams(AJ_PCSTR manifestTemplateXml, PermissionPolicy::Rule::SecurityLevel securityLevel) :
        m_manifestTemplateXml(manifestTemplateXml),
        m_securityLevel(securityLevel)
    { }
};

static AJ_PCSTR s_validDefaultSecurityLevelAnnotation =
    "<manifest>"
    "<node>"
    "<interface>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_validNodeSecurityLevelAnnotationNonPrivileged =
    "<manifest>"
    "<node>"
    SECURITY_LEVEL_ANNOTATION(NON_PRIVILEGED_SECURITY_LEVEL)
    "<interface>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_validNodeSecurityLevelAnnotationUnauthorized =
    "<manifest>"
    "<node>"
    SECURITY_LEVEL_ANNOTATION(UNAUTHENTICATED_SECURITY_LEVEL)
    "<interface>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_validInterfaceSecurityLevelAnnotationNonPrivileged =
    "<manifest>"
    "<node>"
    "<interface>"
    SECURITY_LEVEL_ANNOTATION(NON_PRIVILEGED_SECURITY_LEVEL)
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_validInterfaceSecurityLevelAnnotationUnauthorized =
    "<manifest>"
    "<node>"
    "<interface>"
    SECURITY_LEVEL_ANNOTATION(UNAUTHENTICATED_SECURITY_LEVEL)
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_validNodeSecurityLevelAnnotationOverrideToPrivileged =
    "<manifest>"
    "<node>"
    SECURITY_LEVEL_ANNOTATION(NON_PRIVILEGED_SECURITY_LEVEL)
    "<interface>"
    SECURITY_LEVEL_ANNOTATION(PRIVILEGED_SECURITY_LEVEL)
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_validNodeSecurityLevelAnnotationOverrideToNonPrivileged =
    "<manifest>"
    "<node>"
    SECURITY_LEVEL_ANNOTATION(PRIVILEGED_SECURITY_LEVEL)
    "<interface>"
    SECURITY_LEVEL_ANNOTATION(NON_PRIVILEGED_SECURITY_LEVEL)
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_validNodeSecurityLevelAnnotationOverrideToUnauthorized =
    "<manifest>"
    "<node>"
    SECURITY_LEVEL_ANNOTATION(NON_PRIVILEGED_SECURITY_LEVEL)
    "<interface>"
    SECURITY_LEVEL_ANNOTATION(UNAUTHENTICATED_SECURITY_LEVEL)
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";


static AJ_PCSTR s_doubleSecurityLevelAnnotationInNode =
    "<manifest>"
    "<node>"
    SECURITY_LEVEL_ANNOTATION(NON_PRIVILEGED_SECURITY_LEVEL)
    SECURITY_LEVEL_ANNOTATION(UNAUTHENTICATED_SECURITY_LEVEL)
    "<interface>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_doubleSecurityLevelAnnotationInInterface =
    "<manifest>"
    "<node>"
    "<interface>"
    SECURITY_LEVEL_ANNOTATION(NON_PRIVILEGED_SECURITY_LEVEL)
    SECURITY_LEVEL_ANNOTATION(UNAUTHENTICATED_SECURITY_LEVEL)
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_securityLevelAnnotationInMember =
    "<manifest>"
    "<node>"
    "<interface>"
    "<any>"
    SECURITY_LEVEL_ANNOTATION(UNAUTHENTICATED_SECURITY_LEVEL)
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_invalidSecurityLevelAnnotationValueInNode =
    "<manifest>"
    "<node>"
    SECURITY_LEVEL_ANNOTATION("InvalidValue")
    "<interface>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_invalidSecurityLevelAnnotationValueInInterface =
    "<manifest>"
    "<node>"
    "<interface>"
    SECURITY_LEVEL_ANNOTATION("InvalidValue")
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

class XmlManifestTemplateConverterToRulesInvalidSecurityLevelAnnotationTest : public testing::TestWithParam<StatusParams>{
  public:
    AJ_PCSTR m_manifestTemplateXml;
    vector<PermissionPolicy::Rule> m_rules;
    QStatus m_expectedStatus;

    XmlManifestTemplateConverterToRulesInvalidSecurityLevelAnnotationTest() :
        m_manifestTemplateXml(GetParam().m_xml),
        m_expectedStatus(GetParam().m_status)
    { }
};

class XmlManifestTemplateConverterToRulesSecurityLevelAnnotationTest : public testing::TestWithParam<SecurityLevelParams> {
  public:
    PermissionPolicy::Rule::SecurityLevel m_expectedSecurityLevel;
    PermissionPolicy::Rule::SecurityLevel m_actualSecurityLevel;

    XmlManifestTemplateConverterToRulesSecurityLevelAnnotationTest() :
        m_expectedSecurityLevel(GetParam().m_securityLevel)
    { }

    virtual void SetUp()
    {
        vector<PermissionPolicy::Rule> rules;
        ASSERT_EQ(ER_OK, XmlManifestTemplateConverter::GetInstance()->XmlToRules(GetParam().m_manifestTemplateXml, rules));
        ASSERT_EQ((size_t)1, rules.size());

        m_actualSecurityLevel = rules[0].GetRecommendedSecurityLevel();
    }
};

TEST(XmlManifestTemplateConverterTest, shouldSetManifestTemplateRuleType)
{
    vector<PermissionPolicy::Rule> rules;
    ASSERT_EQ(ER_OK, XmlManifestTemplateConverter::GetInstance()->XmlToRules(s_validDefaultSecurityLevelAnnotation, rules));
    ASSERT_EQ((size_t)1, rules.size());

    EXPECT_EQ(PermissionPolicy::Rule::MANIFEST_TEMPLATE_RULE, rules[0].GetRuleType());
}

INSTANTIATE_TEST_CASE_P(XmlManifestTemplateConverterToRulesSecurityLevelAnnotations,
                        XmlManifestTemplateConverterToRulesSecurityLevelAnnotationTest,
                        ::testing::Values(SecurityLevelParams(s_validDefaultSecurityLevelAnnotation, PermissionPolicy::Rule::SecurityLevel::PRIVILEGED),
                                          SecurityLevelParams(s_validNodeSecurityLevelAnnotationNonPrivileged, PermissionPolicy::Rule::SecurityLevel::NON_PRIVILEGED),
                                          SecurityLevelParams(s_validNodeSecurityLevelAnnotationUnauthorized, PermissionPolicy::Rule::SecurityLevel::UNAUTHENTICATED),
                                          SecurityLevelParams(s_validInterfaceSecurityLevelAnnotationNonPrivileged, PermissionPolicy::Rule::SecurityLevel::NON_PRIVILEGED),
                                          SecurityLevelParams(s_validInterfaceSecurityLevelAnnotationUnauthorized, PermissionPolicy::Rule::SecurityLevel::UNAUTHENTICATED),
                                          SecurityLevelParams(s_validNodeSecurityLevelAnnotationOverrideToPrivileged, PermissionPolicy::Rule::SecurityLevel::PRIVILEGED),
                                          SecurityLevelParams(s_validNodeSecurityLevelAnnotationOverrideToNonPrivileged, PermissionPolicy::Rule::SecurityLevel::NON_PRIVILEGED),
                                          SecurityLevelParams(s_validNodeSecurityLevelAnnotationOverrideToUnauthorized, PermissionPolicy::Rule::SecurityLevel::UNAUTHENTICATED)));
TEST_P(XmlManifestTemplateConverterToRulesSecurityLevelAnnotationTest, shouldSetNodeSecurityLevelAnnotationToExpectedValue)
{
    EXPECT_EQ(m_expectedSecurityLevel, m_actualSecurityLevel);
}

INSTANTIATE_TEST_CASE_P(XmlManifestTemplateConverterToRulesInvalidSecurityLevelAnnotations,
                        XmlManifestTemplateConverterToRulesInvalidSecurityLevelAnnotationTest,
                        ::testing::Values(StatusParams(s_doubleSecurityLevelAnnotationInNode, ER_XML_INVALID_ANNOTATIONS_COUNT),
                                          StatusParams(s_doubleSecurityLevelAnnotationInInterface, ER_XML_INVALID_ANNOTATIONS_COUNT),
                                          StatusParams(s_securityLevelAnnotationInMember, ER_XML_INVALID_ATTRIBUTE_VALUE),
                                          StatusParams(s_invalidSecurityLevelAnnotationValueInNode, ER_XML_INVALID_SECURITY_LEVEL_ANNOTATION_VALUE),
                                          StatusParams(s_invalidSecurityLevelAnnotationValueInInterface, ER_XML_INVALID_SECURITY_LEVEL_ANNOTATION_VALUE)));
TEST_P(XmlManifestTemplateConverterToRulesInvalidSecurityLevelAnnotationTest, shouldReturnErrorForInvalidSecurityLevelAnnotations)
{
    EXPECT_EQ(m_expectedStatus, XmlManifestTemplateConverter::GetInstance()->XmlToRules(m_manifestTemplateXml, m_rules));
}