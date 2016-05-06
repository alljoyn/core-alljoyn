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
    EXPECT_EQ(ER_FAIL, XmlManifestConverter::ManifestToXml(m_validManifest, m_retrievedManifestXml));
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