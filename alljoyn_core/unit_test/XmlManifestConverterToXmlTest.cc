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

#include "XmlManifestConverter.h"
#include "XmlManifestConverterTest.h"

using namespace std;
using namespace qcc;
using namespace ajn;

class XmlManifestConverterToXmlDetailedTest : public testing::Test {
  public:

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(VALID_MANIFEST, validManifest));
    }

  protected:
    AJ_PSTR retrievedManifestXml;
    Manifest validManifest;
    Manifest retrievedManifest;
};

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldFailForManifestWithNoRules)
{
    validManifest->SetRules(nullptr, 0);
    EXPECT_EQ(ER_FAIL, XmlManifestConverter::ManifestToXml(validManifest, &retrievedManifestXml));
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldPassForValidManifest)
{
    EXPECT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(validManifest, &retrievedManifestXml));
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldGetSameRulesSizeAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(validManifest, &retrievedManifestXml));
    ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(retrievedManifestXml, retrievedManifest));

    EXPECT_EQ(1U, retrievedManifest->GetRules().size());
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldGetSameRulesAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(validManifest, &retrievedManifestXml));
    ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(retrievedManifestXml, retrievedManifest));
    ASSERT_EQ(1U, retrievedManifest->GetRules().size());

    EXPECT_EQ(validManifest->GetRules()[0], retrievedManifest->GetRules()[0]);
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldGetSameThumbprintAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(validManifest, &retrievedManifestXml));
    ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(retrievedManifestXml, retrievedManifest));

    EXPECT_EQ(validManifest->GetThumbprint(), retrievedManifest->GetThumbprint());
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldGetSameSignatureAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(validManifest, &retrievedManifestXml));
    ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(retrievedManifestXml, retrievedManifest));

    EXPECT_EQ(validManifest->GetSignature(), retrievedManifest->GetSignature());
}

TEST_F(XmlManifestConverterToXmlDetailedTest, shouldGetSameXmlAfterTwoConversions)
{
    AJ_PSTR secondRetrievedManifestXml = nullptr;
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(validManifest, &retrievedManifestXml));
    ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(retrievedManifestXml, retrievedManifest));
    ASSERT_EQ(ER_OK, XmlManifestConverter::ManifestToXml(retrievedManifest, &secondRetrievedManifestXml));

    EXPECT_STREQ(retrievedManifestXml, secondRetrievedManifestXml);
}