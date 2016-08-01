/******************************************************************************
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
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

#include "XmlManifestConverter.h"
#include "XmlManifestConverterTest.h"

using namespace std;
using namespace qcc;
using namespace ajn;

static AJ_PCSTR NON_WELL_FORMED_XML = "<abc>";
static AJ_PCSTR EMPTY_MANIFEST_ELEMENT =
    "<manifest>"
    "</manifest>";
static AJ_PCSTR MISSING_VERSION_ELEMENT =
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
static AJ_PCSTR MISSING_RULES_ELEMENT =
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
static AJ_PCSTR MISSING_THUMBPRINT_ELEMENT =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";
static AJ_PCSTR MISSING_SIGNATURE_ELEMENT =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "</manifest>";
static AJ_PCSTR MISSING_VERSION_CONTENT =
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
static AJ_PCSTR INVALID_VERSION_NUMBER =
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
static AJ_PCSTR MISSING_THUMBPRINT_CONTENT =
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
static AJ_PCSTR MISSING_THUMBPRINT_OID =
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
static AJ_PCSTR MISSING_THUMBPRINT_VALUE =
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
static AJ_PCSTR INVALID_THUMBPRINT_OID =
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
static AJ_PCSTR INVALID_THUMBPRINT_VALUE_NOT_BASE64 =
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
static AJ_PCSTR INVALID_THUMBPRINT_VALUE_NOT_BINARY =
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
static AJ_PCSTR MISSING_SIGNATURE_CONTENT =
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
static AJ_PCSTR MISSING_SIGNATURE_OID =
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
static AJ_PCSTR MISSING_SIGNATURE_VALUE =
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
static AJ_PCSTR INVALID_SIGNATURE_OID =
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
static AJ_PCSTR INVALID_SIGNATURE_VALUE_NOT_BASE64 =
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
static AJ_PCSTR INVALID_SIGNATURE_VALUE_NOT_BINARY =
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
        ASSERT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(VALID_MANIFEST, retrievedManifest));
    }

  protected:
    Manifest retrievedManifest;
};

class XmlManifestConverterToManifestInvalidXmlTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    XmlManifestConverterToManifestInvalidXmlTest() :
        manifestXml(GetParam())
    { }

  protected:
    Manifest retrievedManifest;
    AJ_PCSTR manifestXml;
};

TEST(XmlManifestConverterToManifestTest, shouldReturnErrorForNonWellFormedXml)
{
    Manifest retrievedManifest;
    EXPECT_EQ(ER_EOF, XmlManifestConverter::XmlToManifest(NON_WELL_FORMED_XML, retrievedManifest));
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldPassForValidInput)
{
    Manifest someManifest;
    EXPECT_EQ(ER_OK, XmlManifestConverter::XmlToManifest(VALID_MANIFEST, someManifest));
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveCorrectVersion)
{
    EXPECT_EQ(1U, retrievedManifest->GetVersion());
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveCorrectRulesSize)
{
    EXPECT_EQ(1U, retrievedManifest->GetRules().size());
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveCorrectThumbprintOid)
{
    EXPECT_STREQ(OID_DIG_SHA256.c_str(), retrievedManifest->GetThumbprintAlgorithmOid().c_str());
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveSetThumbprint)
{
    EXPECT_GT(retrievedManifest->GetThumbprint().size(), 0U);
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveCorrectSignatureOid)
{
    EXPECT_STREQ(OID_SIG_ECDSA_SHA256.c_str(), retrievedManifest->GetSignatureAlgorithmOid().c_str());
}

TEST_F(XmlManifestConverterToManifestDetailedTest, shouldHaveSetSignature)
{
    EXPECT_GT(retrievedManifest->GetSignature().size(), 0U);
}

INSTANTIATE_TEST_CASE_P(XmlManifestConverterToRulesInvalidXml,
                        XmlManifestConverterToManifestInvalidXmlTest,
                        ::testing::Values(EMPTY_MANIFEST_ELEMENT,
                                          MISSING_VERSION_ELEMENT,
                                          MISSING_RULES_ELEMENT,
                                          MISSING_THUMBPRINT_ELEMENT,
                                          MISSING_SIGNATURE_ELEMENT,
                                          MISSING_VERSION_CONTENT,
                                          INVALID_VERSION_NUMBER,
                                          MISSING_THUMBPRINT_CONTENT,
                                          MISSING_THUMBPRINT_OID,
                                          MISSING_THUMBPRINT_VALUE,
                                          INVALID_THUMBPRINT_OID,
                                          INVALID_THUMBPRINT_VALUE_NOT_BASE64,
                                          INVALID_THUMBPRINT_VALUE_NOT_BINARY,
                                          MISSING_SIGNATURE_CONTENT,
                                          MISSING_SIGNATURE_OID,
                                          MISSING_SIGNATURE_VALUE,
                                          INVALID_SIGNATURE_OID,
                                          INVALID_SIGNATURE_VALUE_NOT_BASE64,
                                          INVALID_SIGNATURE_VALUE_NOT_BINARY));
TEST_P(XmlManifestConverterToManifestInvalidXmlTest, shouldReturnErrorForInvalidManifestXml)
{
    EXPECT_EQ(ER_XML_MALFORMED, XmlManifestConverter::XmlToManifest(manifestXml, retrievedManifest));
}