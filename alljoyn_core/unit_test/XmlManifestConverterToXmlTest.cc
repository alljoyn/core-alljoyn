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
#include <qcc/Util.h>
#include <string>

#include "XmlManifestConverter.h"
#include "XmlManifestConverterTest.h"

using namespace std;
using namespace qcc;
using namespace ajn;

class XmlManifestConverterToXmlDetailedTest : public testing::Test {
  public:

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(s_validManifest, m_validManifest));
    }

  protected:
    string m_retrievedManifestXml;
    Manifest m_validManifest;
    Manifest m_retrievedManifest;
};

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldFailForManifestWithNoRules)
{
    m_validManifest->SetRules(nullptr, 0);
    EXPECT_EQ(ER_XML_INVALID_RULES_COUNT, XmlManifestConverter::ManifestToXml(m_validManifest, m_retrievedManifestXml));
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldPassForValidManifest)
{
    EXPECT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(m_validManifest, m_retrievedManifestXml));
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldGetSameRulesSizeAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(m_validManifest, m_retrievedManifestXml));
    ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(m_retrievedManifestXml.c_str(), m_retrievedManifest));

    EXPECT_EQ(1U, m_retrievedManifest->GetRules().size());
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldGetSameRulesAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(m_validManifest, m_retrievedManifestXml));
    ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(m_retrievedManifestXml.c_str(), m_retrievedManifest));
    ASSERT_EQ(1U, m_retrievedManifest->GetRules().size());

    EXPECT_EQ(m_validManifest->GetRules()[0], m_retrievedManifest->GetRules()[0]);
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldGetSameThumbprintAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(m_validManifest, m_retrievedManifestXml));
    ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(m_retrievedManifestXml.c_str(), m_retrievedManifest));

    EXPECT_EQ(m_validManifest->GetThumbprint(), m_retrievedManifest->GetThumbprint());
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldGetSameSignatureAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(m_validManifest, m_retrievedManifestXml));
    ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(m_retrievedManifestXml.c_str(), m_retrievedManifest));

    EXPECT_EQ(m_validManifest->GetSignature(), m_retrievedManifest->GetSignature());
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldGetSameXmlAfterTwoConversions)
{
    string secondRetrievedManifestXml;
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(m_validManifest, m_retrievedManifestXml));
    ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(m_retrievedManifestXml.c_str(), m_retrievedManifest));
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(m_retrievedManifest, secondRetrievedManifestXml));

    EXPECT_EQ(m_retrievedManifestXml, secondRetrievedManifestXml);
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldPassForValidManifestArray)
{
    Manifest manifests[10];

    for (size_t i = 0; i < ArraySize(manifests); i++) {
        manifests[i] = m_validManifest;
    }

    std::vector<std::string> xmls;

    EXPECT_EQ(ER_OK, XmlManifestConverter::ManifestsToXmlArray(manifests, ArraySize(manifests), xmls));
    EXPECT_EQ(ArraySize(manifests), xmls.size());
}