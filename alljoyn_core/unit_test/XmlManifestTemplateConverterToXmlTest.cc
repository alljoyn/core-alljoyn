/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
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
#include <string>

#include "XmlManifestTemplateConverter.h"


using namespace std;
using namespace qcc;
using namespace ajn;

static AJ_PCSTR s_validBasicManifestTemplate =
    "<manifest>"
    "<node>"
    "<interface>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

class XmlManifestTemplateConverterToXmlSecurityLevelAnnoataionTest : public testing::TestWithParam<PermissionPolicy::Rule::SecurityLevel> {
  public:
    PermissionPolicy::Rule::SecurityLevel m_expectedSecurityLevel;
    PermissionPolicy::Rule* m_rules;
    size_t m_rulesCount;
    string m_retrievedRulesXml;
    vector<PermissionPolicy::Rule> m_extractedRules;

    XmlManifestTemplateConverterToXmlSecurityLevelAnnoataionTest() :
        m_expectedSecurityLevel(GetParam()),
        m_rules(nullptr),
        m_rulesCount(0)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlManifestTemplateConverter::GetInstance()->XmlToRules(s_validBasicManifestTemplate, m_rulesVector));
        ASSERT_EQ((size_t)1, m_rulesVector.size());
        m_rules = m_rulesVector.data();
        m_rulesCount = m_rulesVector.size();
    }

  private:
    vector<PermissionPolicy::Rule> m_rulesVector;
};

INSTANTIATE_TEST_CASE_P(XmlManifestTemplateConverterToXmlSecurityLevelAnnoataions,
                        XmlManifestTemplateConverterToXmlSecurityLevelAnnoataionTest,
                        ::testing::Values(PermissionPolicy::Rule::SecurityLevel::PRIVILEGED,
                                          PermissionPolicy::Rule::SecurityLevel::NON_PRIVILEGED,
                                          PermissionPolicy::Rule::SecurityLevel::UNAUTHENTICATED));
TEST_P(XmlManifestTemplateConverterToXmlSecurityLevelAnnoataionTest, shouldSetProperSecurityLevelAfterTwoConversions)
{
    m_rules[0].SetRecommendedSecurityLevel(m_expectedSecurityLevel);
    ASSERT_EQ(ER_OK, XmlManifestTemplateConverter::GetInstance()->RulesToXml(m_rules, m_rulesCount, m_retrievedRulesXml));
    ASSERT_EQ(ER_OK, XmlManifestTemplateConverter::GetInstance()->XmlToRules(m_retrievedRulesXml.c_str(), m_extractedRules));
    ASSERT_EQ((size_t)1, m_extractedRules.size());

    EXPECT_EQ(m_expectedSecurityLevel, m_extractedRules[0].GetRecommendedSecurityLevel());
}