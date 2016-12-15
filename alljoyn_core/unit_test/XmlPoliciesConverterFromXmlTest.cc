/******************************************************************************
 *  *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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

#include "ajTestCommon.h"
#include "XmlPoliciesConverter.h"
#include "XmlPoliciesConverterTest.h"

using namespace std;
using namespace qcc;
using namespace ajn;

static AJ_PCSTR s_nonWellFormedXml = "<abc>";
static AJ_PCSTR s_emptyPolicyElement =
    "<policy>"
    "</policy>";
static AJ_PCSTR s_missingPolicyVersionElement =
    "<policy>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_missingSerialNumberElement =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_missingAclsElement =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "</policy>";
static AJ_PCSTR s_missingAclElement =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls></acls>"
    "</policy>";
static AJ_PCSTR s_missingPeersElement =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_missingPeerElement =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers></peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_missingTypeElement =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer></peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_emptyPolicyVersionElement =
    "<policy>"
    "<policyVersion></policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_emptySerialNumberElement =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber></serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_emptyTypeElement =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type></type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_policyElementsIncorrectOrder =
    "<policy>"
    "<serialNumber>10</serialNumber>"
    "<policyVersion>1</policyVersion>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_aclElementsIncorrectOrder =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    BASIC_VALID_RULES
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_peerElementsIncorrectOrder =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "<type>WITH_PUBLIC_KEY</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_invalidPublicKey =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>InvalidPublicKey</publicKey>"
    "<sgID>" FIRST_VALID_GUID "</sgID>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_invalidSgId =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "<sgID>InvalidsgID</sgID>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_policyVersionNotOne =
    "<policy>"
    "<policyVersion>100</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_policyVersionNotNumeric =
    "<policy>"
    "<policyVersion>value</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_serialNumberNegative =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>-1</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_serialNumberNotNumeric =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>value</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_unknownPeerType =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>UNKNOWN_TYPE</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_allTypeWithOther =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ANY_TRUSTED</type>"
    "</peer>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_anyTrustedTwice =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ANY_TRUSTED</type>"
    "</peer>"
    "<peer>"
    "<type>ANY_TRUSTED</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_sameFromCaTwice =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>FROM_CERTIFICATE_AUTHORITY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "<peer>"
    "<type>FROM_CERTIFICATE_AUTHORITY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_sameWithPublicKeyTwice =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_PUBLIC_KEY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "<peer>"
    "<type>WITH_PUBLIC_KEY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_sameWithMembershipTwice =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "<sgID>" FIRST_VALID_GUID "</sgID>"
    "</peer>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "<sgID>" FIRST_VALID_GUID "</sgID>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";

static AJ_PCSTR s_validWhitespaceInPolicyVersion =
    "<policy>"
    "<policyVersion> 1 </policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validSerialNumberEqualToZero =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>0</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validWhitespaceInSerialNumber =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber> 1 </serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validWhitespaceInType =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type> ALL </type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validWhitespaceInPublicKey =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_PUBLIC_KEY</type>"
    "<publicKey>"
    " " FIRST_VALID_PUBLIC_KEY " "
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validWhitespaceInSgid =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "<sgID> " FIRST_VALID_GUID " </sgID>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";

class XmlPoliciesConverterFromXmlDetailedTest : public testing::Test {
  public:
    PermissionPolicy m_policy;
    PermissionPolicy::Peer::PeerType m_retrievedPeerType;
    const qcc::KeyInfoNISTP256* m_retrievedPeerPublicKey;
    qcc::String m_retrievedPeerPublicKeyPEM;
    qcc::GUID128 m_retrievedPeerSgId;

    void RetrievePeerDetails(const PermissionPolicy::Peer& retrievedPeer)
    {
        m_retrievedPeerType = retrievedPeer.GetType();
        m_retrievedPeerPublicKey = retrievedPeer.GetKeyInfo();
        if (nullptr != m_retrievedPeerPublicKey) {
            ASSERT_EQ(ER_OK, qcc::CertificateX509::EncodePublicKeyPEM(m_retrievedPeerPublicKey->GetPublicKey(), m_retrievedPeerPublicKeyPEM));
        }
        m_retrievedPeerSgId = retrievedPeer.GetSecurityGroupId();
    }
};

class XmlPoliciesConverterFromXmlBasicTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    PermissionPolicy m_policy;
    AJ_PCSTR m_policyXml;

    XmlPoliciesConverterFromXmlBasicTest() :
        m_policyXml(GetParam())
    { }
};

class XmlPoliciesConverterFromXmlFailureTest : public testing::TestWithParam<StatusParams> {
  public:
    PermissionPolicy m_policy;
    AJ_PCSTR m_policyXml;
    QStatus m_expectedStatus;

    XmlPoliciesConverterFromXmlFailureTest() :
        m_policyXml(GetParam().m_xml),
        m_expectedStatus(GetParam().m_status)
    {
    }
};

class XmlPoliciesConverterFromXmlPassTest : public XmlPoliciesConverterFromXmlBasicTest { };

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldReturnErrorForNonWellFormedXml)
{
    EXPECT_EQ(ER_EOF, XmlPoliciesConverter::FromXml(s_nonWellFormedXml, m_policy));
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetPolicyVersion)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllTypePeer, m_policy));

    EXPECT_EQ(1U, m_policy.GetSpecificationVersion());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetSerialNumber)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllTypePeer, m_policy));

    EXPECT_EQ((uint16_t)10, m_policy.GetVersion());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetOneAcl)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllTypePeer, m_policy));

    EXPECT_EQ((size_t)1, m_policy.GetAclsSize());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetTwoAcls)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validTwoAcls, m_policy));

    EXPECT_EQ((size_t)2, m_policy.GetAclsSize());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetOnePeer)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllTypePeer, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());

    EXPECT_EQ((size_t)1, m_policy.GetAcls()[0].GetPeersSize());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetTwoPeers)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validTwoDifferentCa, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());

    EXPECT_EQ((size_t)2, m_policy.GetAcls()[0].GetPeersSize());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetValidPeerForAllType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllTypePeer, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());
    ASSERT_EQ((size_t)1, m_policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(m_policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_ALL, m_retrievedPeerType);
    EXPECT_EQ(nullptr, m_retrievedPeerPublicKey);
    EXPECT_EQ(qcc::GUID128(0), m_retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetValidPeerForAnyTrustedType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAnyTrustedPeer, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());
    ASSERT_EQ((size_t)1, m_policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(m_policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_ANY_TRUSTED, m_retrievedPeerType);
    EXPECT_EQ(nullptr, m_retrievedPeerPublicKey);
    EXPECT_EQ(qcc::GUID128(0), m_retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetValidPeerForFromCAType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validFromCa, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());
    ASSERT_EQ((size_t)1, m_policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(m_policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_FROM_CERTIFICATE_AUTHORITY, m_retrievedPeerType);
    EXPECT_STREQ(FIRST_VALID_PUBLIC_KEY, m_retrievedPeerPublicKeyPEM.c_str());
    EXPECT_EQ(qcc::GUID128(0), m_retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetValidPeerForWithPublicKey)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validWithPublicKey, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());
    ASSERT_EQ((size_t)1, m_policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(m_policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_WITH_PUBLIC_KEY, m_retrievedPeerType);
    EXPECT_STREQ(FIRST_VALID_PUBLIC_KEY, m_retrievedPeerPublicKeyPEM.c_str());
    EXPECT_EQ(qcc::GUID128(0), m_retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetValidPeerForWithMembershipType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validWithMembership, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());
    ASSERT_EQ((size_t)1, m_policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(m_policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_WITH_MEMBERSHIP, m_retrievedPeerType);
    EXPECT_STREQ(FIRST_VALID_PUBLIC_KEY, m_retrievedPeerPublicKeyPEM.c_str());
    EXPECT_EQ(qcc::GUID128(FIRST_VALID_GUID), m_retrievedPeerSgId);
}

INSTANTIATE_TEST_CASE_P(XmlPoliciesConverterInvalidRulesSet,
                        XmlPoliciesConverterFromXmlFailureTest,
                        ::testing::Values(StatusParams(s_emptyPolicyElement, ER_XML_INVALID_ELEMENT_CHILDREN_COUNT),
                                          StatusParams(s_emptyPolicyVersionElement, ER_XML_INVALID_POLICY_VERSION),
                                          StatusParams(s_emptySerialNumberElement, ER_XML_INVALID_POLICY_SERIAL_NUMBER),
                                          StatusParams(s_emptyTypeElement, ER_XML_INVALID_ACL_PEER_TYPE),
                                          StatusParams(s_missingAclsElement, ER_XML_INVALID_ELEMENT_CHILDREN_COUNT),
                                          StatusParams(s_missingAclElement, ER_XML_INVALID_ELEMENT_CHILDREN_COUNT),
                                          StatusParams(s_missingPeersElement, ER_XML_INVALID_ELEMENT_NAME),
                                          StatusParams(s_missingPeerElement, ER_XML_INVALID_ELEMENT_CHILDREN_COUNT),
                                          StatusParams(s_missingPolicyVersionElement, ER_XML_INVALID_ELEMENT_CHILDREN_COUNT),
                                          StatusParams(s_missingSerialNumberElement, ER_XML_INVALID_ELEMENT_CHILDREN_COUNT),
                                          StatusParams(s_missingTypeElement, ER_XML_INVALID_ELEMENT_CHILDREN_COUNT),
                                          StatusParams(s_policyElementsIncorrectOrder, ER_XML_INVALID_ELEMENT_NAME),
                                          StatusParams(s_aclElementsIncorrectOrder, ER_XML_INVALID_ELEMENT_NAME),
                                          StatusParams(s_peerElementsIncorrectOrder, ER_XML_INVALID_ELEMENT_NAME),
                                          StatusParams(s_invalidPublicKey, ER_XML_INVALID_ACL_PEER_PUBLIC_KEY),
                                          StatusParams(s_invalidSgId, ER_INVALID_GUID),
                                          StatusParams(s_policyVersionNotOne, ER_XML_INVALID_POLICY_VERSION),
                                          StatusParams(s_policyVersionNotNumeric, ER_XML_INVALID_POLICY_VERSION),
                                          StatusParams(s_serialNumberNegative, ER_XML_INVALID_POLICY_SERIAL_NUMBER),
                                          StatusParams(s_serialNumberNotNumeric, ER_XML_INVALID_POLICY_SERIAL_NUMBER),
                                          StatusParams(s_unknownPeerType, ER_XML_INVALID_ACL_PEER_TYPE),
                                          StatusParams(s_allTypeWithOther, ER_XML_ACL_ALL_TYPE_PEER_WITH_OTHERS),
                                          StatusParams(s_anyTrustedTwice, ER_XML_ACL_PEER_NOT_UNIQUE),
                                          StatusParams(s_sameFromCaTwice, ER_XML_ACL_PEER_NOT_UNIQUE),
                                          StatusParams(s_sameWithPublicKeyTwice, ER_XML_ACL_PEER_NOT_UNIQUE),
                                          StatusParams(s_sameWithMembershipTwice, ER_XML_ACL_PEER_NOT_UNIQUE)));
TEST_P(XmlPoliciesConverterFromXmlFailureTest, shouldReturnErrorForInvalidRulesSet)
{
    EXPECT_EQ(m_expectedStatus, XmlPoliciesConverter::FromXml(m_policyXml, m_policy));
}

INSTANTIATE_TEST_CASE_P(XmlPoliciesConverterPass,
                        XmlPoliciesConverterFromXmlPassTest,
                        ::testing::Values(s_validAllTypePeer,
                                          s_validTwoAcls,
                                          s_validAnyTrustedPeer,
                                          s_validAnyTrustedPeerWithOther,
                                          s_validFromCa,
                                          s_validSameKeyCaAndWithPublicKey,
                                          s_validTwoDifferentCa,
                                          s_validTwoDifferentWithPublicKey,
                                          s_validTwoWithMembershipDifferentKeys,
                                          s_validTwoWithMembershipDifferentSgids,
                                          s_validWhitespaceInPolicyVersion,
                                          s_validWhitespaceInPublicKey,
                                          s_validWhitespaceInSerialNumber,
                                          s_validSerialNumberEqualToZero,
                                          s_validWhitespaceInSgid,
                                          s_validWhitespaceInType,
                                          s_validWithMembership,
                                          s_validWithPublicKey,
                                          s_validNoRulesElement));
TEST_P(XmlPoliciesConverterFromXmlPassTest, shouldPassForValidInput)
{
    EXPECT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_policyXml, m_policy));
}