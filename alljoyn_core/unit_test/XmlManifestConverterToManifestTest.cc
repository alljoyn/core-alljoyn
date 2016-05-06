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

static AJ_PCSTR s_nonWellFormedXml = "<abc>";
static AJ_PCSTR s_emptyManifestElement =
    "<manifest>"
    "</manifest>";
static AJ_PCSTR s_missingVersionElement =
    "<manifest>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_missingRulesElement =
    "<manifest>"
    "<version>1</version>"
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_missingThumbprintElement =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_missingSignatureElement =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "</manifest>";
static AJ_PCSTR s_missingVersionContent =
    "<manifest>"
    "<version></version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_invalidVersionNumber =
    "<manifest>"
    "<version>0</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_missingThumbprintContent =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_missingThumbprintOid =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_missingThumbprintValue =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_invalidThumbprintOid =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_invalidThumbprintValueNotBase64 =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>ABB.</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_invalidThumbprintValueNotBinary =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>ZHVwYXRhaw==</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_missingSignatureContent =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_missingSignatureOid =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_missingSignatureValue =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_invalidSignatureOid =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_invalidSignatureValueNotBase64 =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>ABB.</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR s_invalidSignatureValueNotBinary =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>ZHVwYXRhaw==</value>"
    "</signature>"
    "</manifest>";

class XmlManifestConverterToManifestDetailedTest : public testing::Test {
  public:

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(s_validManifest, m_retrievedManifest));
    }

  protected:
    Manifest m_retrievedManifest;
};

class XmlManifestConverterToManifestInvalidXmlTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    XmlManifestConverterToManifestInvalidXmlTest() :
        m_manifestXml(GetParam())
    { }

  protected:
    Manifest m_retrievedManifest;
    AJ_PCSTR m_manifestXml;
};

TEST(XmlManifestConverterToManifestTest, shouldReturnErrorForNonWellFormedXml)
{
    Manifest retrievedManifest;
    EXPECT_EQ(ER_EOF, XmlManifestConverter::XmlToManifest(s_nonWellFormedXml, retrievedManifest));
}

TEST(XmlManifestConverterToManifestTest, shouldPassForValidInput)
{
    Manifest someManifest;
    EXPECT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(s_validManifest, someManifest));
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveCorrectVersion)
{
    EXPECT_EQ(1U, m_retrievedManifest->GetVersion());
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveCorrectRulesSize)
{
    EXPECT_EQ(1U, m_retrievedManifest->GetRules().size());
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveCorrectThumbprintOid)
{
    EXPECT_STREQ(OID_DIG_SHA256.c_str(), m_retrievedManifest->GetThumbprintAlgorithmOid().c_str());
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveSetThumbprint)
{
    EXPECT_LT(0U, m_retrievedManifest->GetThumbprint().size());
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveCorrectSignatureOid)
{
    EXPECT_STREQ(OID_SIG_ECDSA_SHA256.c_str(), m_retrievedManifest->GetSignatureAlgorithmOid().c_str());
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveSetSignature)
{
    EXPECT_LT(0U, m_retrievedManifest->GetSignature().size());
}

INSTANTIATE_TEST_CASE_P(XmlManifestConverterToRulesInvalidXml,
                        XmlManifestConverterToManifestInvalidXmlTest,
                        ::testing::Values(s_emptyManifestElement,
                                          s_missingVersionElement,
                                          s_missingRulesElement,
                                          s_missingThumbprintElement,
                                          s_missingSignatureElement,
                                          s_missingVersionContent,
                                          s_invalidVersionNumber,
                                          s_missingThumbprintContent,
                                          s_missingThumbprintOid,
                                          s_missingThumbprintValue,
                                          s_invalidThumbprintOid,
                                          s_invalidThumbprintValueNotBase64,
                                          s_invalidThumbprintValueNotBinary,
                                          s_missingSignatureContent,
                                          s_missingSignatureOid,
                                          s_missingSignatureValue,
                                          s_invalidSignatureOid,
                                          s_invalidSignatureValueNotBase64,
                                          s_invalidSignatureValueNotBinary));
TEST_P(XmlManifestConverterToManifestInvalidXmlTest, shouldReturnErrorForInvalidManifestXml)
{
    EXPECT_EQ(ER_XML_MALFORMED, XmlManifestConverter::XmlToManifest(m_manifestXml, m_retrievedManifest));
}