/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/
#include <alljoyn/Status.h>
#include <gtest/gtest.h>
#include <qcc/platform.h>

#include "ajTestCommon.h"
#include "KeyInfoHelper.h"
#include "PermissionPolicyOverwriteUtils.h"
#include "XmlPoliciesConverter.h"
#include "XmlPoliciesConverterTest.h"

using namespace std;
using namespace qcc;
using namespace ajn;

typedef enum {
    ANY_TRUSTED_PEER_INDEX = 0,
    FIRST_WITH_MEMBERSHIP_PEER_INDEX,
    SECOND_WITH_MEMBERSHIP_PEER_INDEX,
    FIRST_FROM_CA_PEER_INDEX,
    SECOND_FROM_CA_PEER_INDEX,
    FIRST_WITH_PUBLIC_KEY_PEER_INDEX,
    SECOND_WITH_PUBLIC_KEY_PEER_INDEX
} TestPeerIndex;

static AJ_PCSTR s_validAllCasesPolicy =
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
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>" FIRST_VALID_PUBLIC_KEY "</publicKey>"
    "<sgID>" FIRST_VALID_GUID "</sgID>"
    "</peer>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>" SECOND_VALID_PUBLIC_KEY "</publicKey>"
    "<sgID>" SECOND_VALID_GUID "</sgID>"
    "</peer>"
    "<peer>"
    "<type>FROM_CERTIFICATE_AUTHORITY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "<peer>"
    "<type>FROM_CERTIFICATE_AUTHORITY</type>"
    "<publicKey>"
    SECOND_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "<peer>"
    "<type>WITH_PUBLIC_KEY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "<peer>"
    "<type>WITH_PUBLIC_KEY</type>"
    "<publicKey>"
    SECOND_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";

struct KeyParams {
  public:
    uint32_t m_index;
    AJ_PCSTR m_keyPem;

    KeyParams(uint32_t _index, AJ_PCSTR _keyPem) :
        m_index(_index),
        m_keyPem(_keyPem)
    { }
};

struct GuidParams {
  public:
    uint32_t m_index;
    AJ_PCSTR m_guid;

    GuidParams(uint32_t _index, AJ_PCSTR _guid) :
        m_index(_index),
        m_guid(_guid)
    { }
};

struct TypeParams {
  public:
    uint32_t m_index;
    PermissionPolicy::Peer::PeerType m_type;

    TypeParams(uint32_t _index, PermissionPolicy::Peer::PeerType _type) :
        m_index(_index),
        m_type(_type)
    { }
};

class XmlPoliciesConverterToXmlDetailedTest : public testing::Test {
  public:
    PermissionPolicy m_validPolicy;
    string m_retrievedPolicyXml;

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllCasesPolicy, m_validPolicy));
    }
};

class XmlPolicyConverterToXmlDetailedFailureTest : public XmlPoliciesConverterToXmlDetailedTest { };

class XmlPolicyConverterToXmlDetailedPassTest : public XmlPoliciesConverterToXmlDetailedTest {
  public:
    PermissionPolicy m_retrievedPolicy;
};

class XmlPolicyConverterToXmlPublicKeyTest : public testing::TestWithParam<KeyParams> {
  public:
    uint32_t m_peerIndex;
    string m_retrievedPolicyXml;
    PermissionPolicy m_validPolicy;
    PermissionPolicy m_retrievedPolicy;
    KeyInfoNISTP256 m_expectedPublicKey;

    XmlPolicyConverterToXmlPublicKeyTest() :
        m_peerIndex(GetParam().m_index)
    { }

    virtual void SetUp() {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllCasesPolicy, m_validPolicy));
        ASSERT_EQ(ER_OK, KeyInfoHelper::PEMToKeyInfoNISTP256(GetParam().m_keyPem, m_expectedPublicKey));
    }
};

class XmlPolicyConverterToXmlGuidTest : public testing::TestWithParam<GuidParams> {
  public:
    uint32_t m_peerIndex;
    string m_retrievedPolicyXml;
    PermissionPolicy m_validPolicy;
    PermissionPolicy m_retrievedPolicy;
    GUID128 m_expectedGuid;

    XmlPolicyConverterToXmlGuidTest() :
        m_peerIndex(GetParam().m_index),
        m_expectedGuid(GetParam().m_guid)
    { }

    virtual void SetUp() {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllCasesPolicy, m_validPolicy));
    }
};

class XmlPolicyConverterToXmlPeerTypeTest : public testing::TestWithParam<TypeParams> {
  public:
    uint32_t m_peerIndex;
    string m_retrievedPolicyXml;
    PermissionPolicy m_validPolicy;
    PermissionPolicy m_retrievedPolicy;
    PermissionPolicy::Peer::PeerType m_expectedType;

    XmlPolicyConverterToXmlPeerTypeTest() :
        m_peerIndex(GetParam().m_index),
        m_expectedType(GetParam().m_type)
    { }

    virtual void SetUp() {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllCasesPolicy, m_validPolicy));
    }
};

class XmlPolicyConverterToXmlPassTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    PermissionPolicy m_policy;
    string m_retrievedPolicyXml;

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(GetParam(), m_policy));
    }
};

class XmlPolicyConverterToXmlCountTest : public testing::TestWithParam<SizeParams> {
  public:
    PermissionPolicy m_validPolicy;
    PermissionPolicy m_retrievedPolicy;
    size_t m_expectedCount;
    string m_retrievedPolicyXml;

    XmlPolicyConverterToXmlCountTest() :
        m_expectedCount(GetParam().m_integer)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(GetParam().m_xml, m_validPolicy));
    }
};

class XmlPolicyConverterToXmlAclsCountTest : public XmlPolicyConverterToXmlCountTest { };

class XmlPolicyConverterToXmlPeersCountTest : public XmlPolicyConverterToXmlCountTest { };

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForInvalidPolicyVersion)
{
    m_validPolicy.SetSpecificationVersion(0);

    EXPECT_EQ(ER_XML_INVALID_POLICY_VERSION, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForZeroAcls)
{
    m_validPolicy.SetAcls(0, nullptr);

    EXPECT_EQ(ER_XML_ACLS_MISSING, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForZeroPeers)
{
    ASSERT_GT(m_validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeers(0, nullptr, m_validPolicy);

    EXPECT_EQ(ER_XML_ACL_PEERS_MISSING, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForAllTypePeerWithOthers)
{
    ASSERT_GT(m_validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerType(ANY_TRUSTED_PEER_INDEX, PermissionPolicy::Peer::PEER_ALL, m_validPolicy);

    EXPECT_EQ(ER_XML_ACL_ALL_TYPE_PEER_WITH_OTHERS, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForAnyTrustedTypePeerTwice)
{
    ASSERT_GT(m_validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerType(FIRST_FROM_CA_PEER_INDEX, PermissionPolicy::Peer::PEER_ANY_TRUSTED, m_validPolicy);
    PolicyOverwriteUtils::ChangePeerPublicKey(FIRST_FROM_CA_PEER_INDEX, nullptr, m_validPolicy);

    EXPECT_EQ(ER_XML_ACL_PEER_NOT_UNIQUE, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForAnyTrustedPeerWithPublicKey)
{
    ASSERT_GT(m_validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(ANY_TRUSTED_PEER_INDEX, FIRST_VALID_PUBLIC_KEY, m_validPolicy);

    EXPECT_EQ(ER_XML_ACL_PEER_PUBLIC_KEY_SET, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForAllTypePeerWithPublicKey)
{
    PermissionPolicy::Peer allPeer = PolicyOverwriteUtils::BuildPeer(PermissionPolicy::Peer::PEER_ALL, FIRST_VALID_PUBLIC_KEY);
    ASSERT_GT(m_validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeers(1, &allPeer, m_validPolicy);

    EXPECT_EQ(ER_XML_ACL_PEER_PUBLIC_KEY_SET, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForWithPublicKeyPeerTypeWithoutPublicKey)
{
    ASSERT_GT(m_validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(FIRST_WITH_PUBLIC_KEY_PEER_INDEX, nullptr, m_validPolicy);

    EXPECT_EQ(ER_XML_INVALID_ACL_PEER_PUBLIC_KEY, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForWithMembershipPeerTypeWithoutPublicKey)
{
    ASSERT_GT(m_validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(FIRST_WITH_MEMBERSHIP_PEER_INDEX, nullptr, m_validPolicy);

    EXPECT_EQ(ER_XML_INVALID_ACL_PEER_PUBLIC_KEY, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForCAPeerTypeWithoutPublicKey)
{
    ASSERT_GT(m_validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(FIRST_FROM_CA_PEER_INDEX, nullptr, m_validPolicy);

    EXPECT_EQ(ER_XML_INVALID_ACL_PEER_PUBLIC_KEY, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForTwoSameWithMembershipPeers)
{
    ASSERT_GT(m_validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(SECOND_WITH_MEMBERSHIP_PEER_INDEX, FIRST_VALID_PUBLIC_KEY, m_validPolicy);
    PolicyOverwriteUtils::ChangePeerSgId(SECOND_WITH_MEMBERSHIP_PEER_INDEX, FIRST_VALID_GUID, m_validPolicy);

    EXPECT_EQ(ER_XML_ACL_PEER_NOT_UNIQUE, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForTwoSameWithPublicKeyPeers)
{
    ASSERT_GT(m_validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(SECOND_WITH_PUBLIC_KEY_PEER_INDEX, FIRST_VALID_PUBLIC_KEY, m_validPolicy);

    EXPECT_EQ(ER_XML_ACL_PEER_NOT_UNIQUE, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForTwoSameCAPeers)
{
    ASSERT_GT(m_validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(SECOND_FROM_CA_PEER_INDEX, FIRST_VALID_PUBLIC_KEY, m_validPolicy);

    EXPECT_EQ(ER_XML_ACL_PEER_NOT_UNIQUE, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSamePolicyAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));

    EXPECT_EQ(m_validPolicy, m_retrievedPolicy);
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSamePolicyXmlAfterTwoConversions)
{
    string secondRetrievedPolicyXml;
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_retrievedPolicy, secondRetrievedPolicyXml));

    EXPECT_EQ(m_retrievedPolicyXml, secondRetrievedPolicyXml);
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetPolicyVersion)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));

    EXPECT_EQ(1U, m_retrievedPolicy.GetSpecificationVersion());
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSerialNumber)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));

    EXPECT_EQ((uint16_t)10, m_retrievedPolicy.GetVersion());
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSomeAcls)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));

    EXPECT_GT(m_retrievedPolicy.GetAclsSize(), 0U);
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSomePeers)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));
    ASSERT_GT(m_retrievedPolicy.GetAclsSize(), 0U);

    EXPECT_GT(m_retrievedPolicy.GetAcls()[0].GetPeersSize(), 0U);
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSomeRules)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));
    ASSERT_GT(m_retrievedPolicy.GetAclsSize(), 0U);

    EXPECT_GT(m_retrievedPolicy.GetAcls()[0].GetRulesSize(), 0U);
}

INSTANTIATE_TEST_CASE_P(XmlPolicyConverterToXmlPublicKeyCorrect,
                        XmlPolicyConverterToXmlPublicKeyTest,
                        ::testing::Values(KeyParams(FIRST_WITH_MEMBERSHIP_PEER_INDEX, FIRST_VALID_PUBLIC_KEY),
                                          KeyParams(SECOND_WITH_MEMBERSHIP_PEER_INDEX, SECOND_VALID_PUBLIC_KEY),
                                          KeyParams(FIRST_WITH_PUBLIC_KEY_PEER_INDEX, FIRST_VALID_PUBLIC_KEY),
                                          KeyParams(SECOND_WITH_PUBLIC_KEY_PEER_INDEX, SECOND_VALID_PUBLIC_KEY),
                                          KeyParams(FIRST_FROM_CA_PEER_INDEX, FIRST_VALID_PUBLIC_KEY),
                                          KeyParams(SECOND_FROM_CA_PEER_INDEX, SECOND_VALID_PUBLIC_KEY)));
TEST_P(XmlPolicyConverterToXmlPublicKeyTest, shouldGetCorrectPublicKey)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));
    ASSERT_GT(m_retrievedPolicy.GetAclsSize(), 0U);
    ASSERT_GT(m_retrievedPolicy.GetAcls()[0].GetPeersSize(), m_peerIndex);

    PermissionPolicy::Peer analyzedPeer = m_retrievedPolicy.GetAcls()[0].GetPeers()[m_peerIndex];

    EXPECT_EQ(m_expectedPublicKey, *(analyzedPeer.GetKeyInfo()));
}

INSTANTIATE_TEST_CASE_P(XmlPolicyConverterToXmlGuidCorrect,
                        XmlPolicyConverterToXmlGuidTest,
                        ::testing::Values(GuidParams(FIRST_WITH_MEMBERSHIP_PEER_INDEX, FIRST_VALID_GUID),
                                          GuidParams(SECOND_WITH_MEMBERSHIP_PEER_INDEX, SECOND_VALID_GUID)));
TEST_P(XmlPolicyConverterToXmlGuidTest, shouldGetCorrectGuid)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));
    ASSERT_GT(m_retrievedPolicy.GetAclsSize(), 0U);
    ASSERT_GT(m_retrievedPolicy.GetAcls()[0].GetPeersSize(), m_peerIndex);

    PermissionPolicy::Peer analyzedPeer = m_retrievedPolicy.GetAcls()[0].GetPeers()[m_peerIndex];

    EXPECT_EQ(m_expectedGuid, analyzedPeer.GetSecurityGroupId());
}

INSTANTIATE_TEST_CASE_P(XmlPolicyConverterToXmlPeerTypeCorrect,
                        XmlPolicyConverterToXmlPeerTypeTest,
                        ::testing::Values(TypeParams(ANY_TRUSTED_PEER_INDEX, PermissionPolicy::Peer::PEER_ANY_TRUSTED),
                                          TypeParams(FIRST_WITH_MEMBERSHIP_PEER_INDEX, PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP),
                                          TypeParams(SECOND_WITH_MEMBERSHIP_PEER_INDEX, PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP),
                                          TypeParams(FIRST_WITH_PUBLIC_KEY_PEER_INDEX, PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY),
                                          TypeParams(SECOND_WITH_PUBLIC_KEY_PEER_INDEX, PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY),
                                          TypeParams(FIRST_FROM_CA_PEER_INDEX, PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY),
                                          TypeParams(SECOND_FROM_CA_PEER_INDEX, PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY)));
TEST_P(XmlPolicyConverterToXmlPeerTypeTest, shouldContainCorrectPeerType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));
    ASSERT_GT(m_retrievedPolicy.GetAclsSize(), 0U);
    ASSERT_GT(m_retrievedPolicy.GetAcls()[0].GetPeersSize(), m_peerIndex);

    PermissionPolicy::Peer analyzedPeer = m_retrievedPolicy.GetAcls()[0].GetPeers()[m_peerIndex];

    EXPECT_EQ(m_expectedType, analyzedPeer.GetType());
}

INSTANTIATE_TEST_CASE_P(XmlPolicyConverterToXmlPass,
                        XmlPolicyConverterToXmlPassTest,
                        ::testing::Values(s_validAllCasesPolicy,
                                          s_validAllTypePeer,
                                          s_validTwoAcls,
                                          s_validAnyTrustedPeer,
                                          s_validAnyTrustedPeerWithOther,
                                          s_validFromCa,
                                          s_validSameKeyCaAndWithPublicKey,
                                          s_validTwoDifferentCa,
                                          s_validTwoDifferentWithPublicKey,
                                          s_validTwoWithMembershipDifferentKeys,
                                          s_validTwoWithMembershipDifferentSgids,
                                          s_validWithMembership,
                                          s_validWithPublicKey,
                                          s_validNoRulesElement));
TEST_P(XmlPolicyConverterToXmlPassTest, shouldPassForValidInput)
{
    EXPECT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_policy, m_retrievedPolicyXml));
}

INSTANTIATE_TEST_CASE_P(XmlPolicyConverterToXmlAclsCount,
                        XmlPolicyConverterToXmlAclsCountTest,
                        ::testing::Values(SizeParams(s_validAllCasesPolicy, 1),
                                          SizeParams(s_validTwoAcls, 2)));
TEST_P(XmlPolicyConverterToXmlAclsCountTest, shouldGetCorrectAclsCount)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));

    EXPECT_EQ(m_expectedCount, m_retrievedPolicy.GetAclsSize());
}

INSTANTIATE_TEST_CASE_P(XmlPolicyConverterToXmlPeersCount,
                        XmlPolicyConverterToXmlPeersCountTest,
                        ::testing::Values(SizeParams(s_validAllCasesPolicy, 7),
                                          SizeParams(s_validAllTypePeer, 1),
                                          SizeParams(s_validTwoAcls, 1),
                                          SizeParams(s_validAnyTrustedPeer, 1),
                                          SizeParams(s_validAnyTrustedPeerWithOther, 2),
                                          SizeParams(s_validFromCa, 1),
                                          SizeParams(s_validSameKeyCaAndWithPublicKey, 2),
                                          SizeParams(s_validTwoDifferentCa, 2),
                                          SizeParams(s_validTwoDifferentWithPublicKey, 2),
                                          SizeParams(s_validTwoWithMembershipDifferentKeys, 2),
                                          SizeParams(s_validTwoWithMembershipDifferentSgids, 2),
                                          SizeParams(s_validWithMembership, 1),
                                          SizeParams(s_validWithPublicKey, 1),
                                          SizeParams(s_validNoRulesElement, 1)));
TEST_P(XmlPolicyConverterToXmlPeersCountTest, shouldGetCorrectPeersCount)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(m_validPolicy, m_retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_retrievedPolicyXml.c_str(), m_retrievedPolicy));
    ASSERT_GT(m_retrievedPolicy.GetAclsSize(), 0U);

    EXPECT_EQ(m_expectedCount, m_retrievedPolicy.GetAcls()[0].GetPeersSize());
}