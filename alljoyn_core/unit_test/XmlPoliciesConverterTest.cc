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

#include "ajTestCommon.h"
#include "XmlPoliciesConverter.h"

using namespace std;
using namespace qcc;
using namespace ajn;

#define BASIC_VALID_RULES \
    "<rules>" \
    "<node>" \
    "<interface>" \
    "<method>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" \
    "</method>" \
    "</interface>" \
    "</node>" \
    "</rules>"

#define FIRST_VALID_PUBLIC_KEY \
    "-----BEGIN PUBLIC KEY-----\n" \
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEaGDzY4qFMpBqZcex+c66GdvMo/Q9\n" \
    "H5DHr/+//RCDkuUorPCEHoJS5+UTkvn7XJNqp0gzGj+v55hrz0p/V9NHQw==\n" \
    "-----END PUBLIC KEY-----" \

#define SECOND_VALID_PUBLIC_KEY \
    "-----BEGIN PUBLIC KEY-----\n" \
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAETHDIEthNo/28eKnN4cBtapQ6n3qA\n" \
    "IJkXmOuHapWDvx6MUqLesiqrWe2zwY+tLPdMB+ud16kWp34Na/ZMHfxi3A==\n" \
    "-----END PUBLIC KEY-----"

#define FIRST_VALID_GUID "16fc57377fced83516fc57377fced835"
#define SECOND_VALID_GUID "f5175474e932917fa3d9b5fd665f65de"

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
static AJ_PCSTR s_missingRulesElement =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
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
    "<type>ALL</type>"
    "</peer>"
    "<peer>"
    "<type>ANY_TRUSTED</type>"
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
    "<type>WITH_MEMBERSHIP</type>"
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

static AJ_PCSTR s_validAllTypePeer =
    "<policy>"
    "<policyVersion>1</policyVersion>"
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
static AJ_PCSTR s_validTwoAcls =
    "<policy>"
    "<policyVersion>1</policyVersion>"
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
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ANY_TRUSTED</type>"
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
static AJ_PCSTR s_validAnyTrustedPeer =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ANY_TRUSTED</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validFromCa =
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
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validWithMembership =
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
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validWithPublicKey =
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
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validAnyTrustedPeerWithOther =
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
static AJ_PCSTR s_validTwoDifferentCa =
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
    SECOND_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validTwoWithMembershipDifferentKeys =
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
    SECOND_VALID_PUBLIC_KEY
    "</publicKey>"
    "<sgID>" FIRST_VALID_GUID "</sgID>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validTwoWithMembershipDifferentSgids =
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
    "<sgID>" SECOND_VALID_GUID "</sgID>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validTwoDifferentWithPublicKey =
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
    SECOND_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR s_validSameKeyCaAndWithPublicKey =
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

class XmlPoliciesConverterDetailedTest : public testing::Test {
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

class XmlPoliciesConverterBasicTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    PermissionPolicy m_policy;
    AJ_PCSTR m_policyXml;

    XmlPoliciesConverterBasicTest() :
        m_policyXml(GetParam())
    { }
};

class XmlPoliciesConverterFailureTest : public XmlPoliciesConverterBasicTest { };

class XmlPoliciesConverterPassTest : public XmlPoliciesConverterBasicTest { };

TEST_F(XmlPoliciesConverterDetailedTest, shouldReturnErrorForNonWellFormedXml)
{
    EXPECT_EQ(ER_EOF, XmlPoliciesConverter::FromXml(s_nonWellFormedXml, m_policy));
}

TEST_F(XmlPoliciesConverterDetailedTest, shouldGetPolicyVersion)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllTypePeer, m_policy));

    EXPECT_EQ(1U, m_policy.GetSpecificationVersion());
}

TEST_F(XmlPoliciesConverterDetailedTest, shouldGetSerialNumber)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllTypePeer, m_policy));

    EXPECT_EQ((uint16_t)10, m_policy.GetVersion());
}

TEST_F(XmlPoliciesConverterDetailedTest, shouldGetOneAcl)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllTypePeer, m_policy));

    EXPECT_EQ((size_t)1, m_policy.GetAclsSize());
}

TEST_F(XmlPoliciesConverterDetailedTest, shouldGetTwoAcls)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validTwoAcls, m_policy));

    EXPECT_EQ((size_t)2, m_policy.GetAclsSize());
}

TEST_F(XmlPoliciesConverterDetailedTest, shouldGetOnePeer)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllTypePeer, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());

    EXPECT_EQ((size_t)1, m_policy.GetAcls()[0].GetPeersSize());
}

TEST_F(XmlPoliciesConverterDetailedTest, shouldGetTwoPeers)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validTwoDifferentCa, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());

    EXPECT_EQ((size_t)2, m_policy.GetAcls()[0].GetPeersSize());
}

TEST_F(XmlPoliciesConverterDetailedTest, shouldGetValidPeerForAllType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAllTypePeer, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());
    ASSERT_EQ((size_t)1, m_policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(m_policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_ALL, m_retrievedPeerType);
    EXPECT_EQ(nullptr, m_retrievedPeerPublicKey);
    EXPECT_EQ(qcc::GUID128(0), m_retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterDetailedTest, shouldGetValidPeerForAnyTrustedType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validAnyTrustedPeer, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());
    ASSERT_EQ((size_t)1, m_policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(m_policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_ANY_TRUSTED, m_retrievedPeerType);
    EXPECT_EQ(nullptr, m_retrievedPeerPublicKey);
    EXPECT_EQ(qcc::GUID128(0), m_retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterDetailedTest, shouldGetValidPeerForFromCAType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validFromCa, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());
    ASSERT_EQ((size_t)1, m_policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(m_policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_FROM_CERTIFICATE_AUTHORITY, m_retrievedPeerType);
    EXPECT_STREQ(FIRST_VALID_PUBLIC_KEY, m_retrievedPeerPublicKeyPEM.c_str());
    EXPECT_EQ(qcc::GUID128(0), m_retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterDetailedTest, shouldGetValidPeerForWithPublicKey)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(s_validWithPublicKey, m_policy));
    ASSERT_EQ((size_t)1, m_policy.GetAclsSize());
    ASSERT_EQ((size_t)1, m_policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(m_policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_WITH_PUBLIC_KEY, m_retrievedPeerType);
    EXPECT_STREQ(FIRST_VALID_PUBLIC_KEY, m_retrievedPeerPublicKeyPEM.c_str());
    EXPECT_EQ(qcc::GUID128(0), m_retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterDetailedTest, shouldGetValidPeerForWithMembershipType)
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
                        XmlPoliciesConverterFailureTest,
                        ::testing::Values(s_emptyPolicyElement,
                                          s_emptyPolicyVersionElement,
                                          s_emptySerialNumberElement,
                                          s_emptyTypeElement,
                                          s_missingAclsElement,
                                          s_missingAclElement,
                                          s_missingPeersElement,
                                          s_missingPeerElement,
                                          s_missingPolicyVersionElement,
                                          s_missingRulesElement,
                                          s_missingSerialNumberElement,
                                          s_missingTypeElement,
                                          s_policyElementsIncorrectOrder,
                                          s_aclElementsIncorrectOrder,
                                          s_peerElementsIncorrectOrder,
                                          s_invalidPublicKey,
                                          s_invalidSgId,
                                          s_policyVersionNotOne,
                                          s_serialNumberNegative,
                                          s_unknownPeerType,
                                          s_allTypeWithOther,
                                          s_anyTrustedTwice,
                                          s_sameFromCaTwice,
                                          s_sameWithPublicKeyTwice,
                                          s_sameWithMembershipTwice));
TEST_P(XmlPoliciesConverterFailureTest, shouldReturnErrorForInvalidRulesSet)
{
    EXPECT_EQ(ER_XML_MALFORMED, XmlPoliciesConverter::FromXml(m_policyXml, m_policy));
}

INSTANTIATE_TEST_CASE_P(XmlPoliciesConverterPass,
                        XmlPoliciesConverterPassTest,
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
                                          s_validWhitespaceInSgid,
                                          s_validWhitespaceInType,
                                          s_validWithMembership,
                                          s_validWithPublicKey));
TEST_P(XmlPoliciesConverterPassTest, shouldPassForValidInput)
{
    EXPECT_EQ(ER_OK, XmlPoliciesConverter::FromXml(m_policyXml, m_policy));
}
