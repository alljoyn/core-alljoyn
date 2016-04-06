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
#include "XmlPoliciesConverterTest.h"

using namespace std;
using namespace qcc;
using namespace ajn;

static AJ_PCSTR NON_WELL_FORMED_XML = "<abc>";
static AJ_PCSTR EMPTY_POLICY_ELEMENT =
    "<policy>"
    "</policy>";
static AJ_PCSTR MISSING_POLICY_VERSION_ELEMENT =
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
static AJ_PCSTR MISSING_SERIAL_NUMBER_ELEMENT =
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
static AJ_PCSTR MISSING_ACLS_ELEMENT =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "</policy>";
static AJ_PCSTR MISSING_ACL_ELEMENT =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls></acls>"
    "</policy>";
static AJ_PCSTR MISSING_PEERS_ELEMENT =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
static AJ_PCSTR MISSING_RULES_ELEMENT =
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
static AJ_PCSTR MISSING_PEER_ELEMENT =
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
static AJ_PCSTR MISSING_TYPE_ELEMENT =
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
static AJ_PCSTR EMPTY_POLICY_VERSION_ELEMENT =
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
static AJ_PCSTR EMPTY_SERIAL_NUMBER_ELEMENT =
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
static AJ_PCSTR EMPTY_TYPE_ELEMENT =
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
static AJ_PCSTR POLICY_ELEMENTS_INCORRECT_ORDER =
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
static AJ_PCSTR ACL_ELEMENTS_INCORRECT_ORDER =
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
static AJ_PCSTR PEER_ELEMENTS_INCORRECT_ORDER =
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
static AJ_PCSTR INVALID_PUBLIC_KEY =
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
static AJ_PCSTR INVALID_SGID =
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
static AJ_PCSTR POLICY_VERSION_NOT_ONE =
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
static AJ_PCSTR SERIAL_NUMBER_NEGATIVE =
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
static AJ_PCSTR UNKNOWN_PEER_TYPE =
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
static AJ_PCSTR ALL_TYPE_WITH_OTHER =
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
static AJ_PCSTR ANY_TRUSTED_TWICE =
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
static AJ_PCSTR SAME_FROM_CA_TWICE =
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
static AJ_PCSTR SAME_WITH_PUBLIC_KEY_TWICE =
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
static AJ_PCSTR SAME_WITH_MEMBERSHIP_TWICE =
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

static AJ_PCSTR VALID_WHITESPACE_IN_POLICY_VERSION =
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
static AJ_PCSTR VALID_WHITESPACE_IN_SERIAL_NUMBER =
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
static AJ_PCSTR VALID_WHITESPACE_IN_TYPE =
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
static AJ_PCSTR VALID_WHITESPACE_IN_PUBLIC_KEY =
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
static AJ_PCSTR VALID_WHITESPACE_IN_SGID =
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
    PermissionPolicy policy;
    PermissionPolicy::Peer::PeerType retrievedPeerType;
    const qcc::KeyInfoNISTP256* retrievedPeerPublicKey;
    qcc::String retrievedPeerPublicKeyPEM;
    qcc::GUID128 retrievedPeerSgId;

    virtual void SetUp()
    {
        /*
         * Workaround for ASACORE-2703. Initializes algorithm providers for global variable "cngCache".
         */
        Crypto_ECC ecc;
    }

    void RetrievePeerDetails(const PermissionPolicy::Peer& retrievedPeer)
    {
        retrievedPeerType = retrievedPeer.GetType();
        retrievedPeerPublicKey = retrievedPeer.GetKeyInfo();
        if (nullptr != retrievedPeerPublicKey) {
            ASSERT_EQ(ER_OK, qcc::CertificateX509::EncodePublicKeyPEM(retrievedPeerPublicKey->GetPublicKey(), retrievedPeerPublicKeyPEM));
        }
        retrievedPeerSgId = retrievedPeer.GetSecurityGroupId();
    }
};

class XmlPoliciesConverterFromXmlBasicTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    PermissionPolicy policy;
    AJ_PCSTR policyXml;

    XmlPoliciesConverterFromXmlBasicTest() :
        policyXml(GetParam())
    { }
};

class XmlPoliciesConverterFromXmlFailureTest : public XmlPoliciesConverterFromXmlBasicTest { };

class XmlPoliciesConverterFromXmlPassTest : public XmlPoliciesConverterFromXmlBasicTest { };

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldReturnErrorForNonWellFormedXml)
{
    EXPECT_EQ(ER_EOF, XmlPoliciesConverter::FromXml(NON_WELL_FORMED_XML, policy));
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetPolicyVersion)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_ALL_TYPE_PEER, policy));

    EXPECT_EQ(1U, policy.GetSpecificationVersion());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetSerialNumber)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_ALL_TYPE_PEER, policy));

    EXPECT_EQ((uint16_t)10, policy.GetVersion());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetOneAcl)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_ALL_TYPE_PEER, policy));

    EXPECT_EQ((size_t)1, policy.GetAclsSize());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetTwoAcls)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_TWO_ACLS, policy));

    EXPECT_EQ((size_t)2, policy.GetAclsSize());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetOnePeer)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_ALL_TYPE_PEER, policy));
    ASSERT_EQ((size_t)1, policy.GetAclsSize());

    EXPECT_EQ((size_t)1, policy.GetAcls()[0].GetPeersSize());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetTwoPeers)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_TWO_DIFFERENT_CA, policy));
    ASSERT_EQ((size_t)1, policy.GetAclsSize());

    EXPECT_EQ((size_t)2, policy.GetAcls()[0].GetPeersSize());
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetValidPeerForAllType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_ALL_TYPE_PEER, policy));
    ASSERT_EQ((size_t)1, policy.GetAclsSize());
    ASSERT_EQ((size_t)1, policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_ALL, retrievedPeerType);
    EXPECT_EQ(nullptr, retrievedPeerPublicKey);
    EXPECT_EQ(qcc::GUID128(0), retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetValidPeerForAnyTrustedType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_ANY_TRUSTED_PEER, policy));
    ASSERT_EQ((size_t)1, policy.GetAclsSize());
    ASSERT_EQ((size_t)1, policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_ANY_TRUSTED, retrievedPeerType);
    EXPECT_EQ(nullptr, retrievedPeerPublicKey);
    EXPECT_EQ(qcc::GUID128(0), retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetValidPeerForFromCAType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_FROM_CA, policy));
    ASSERT_EQ((size_t)1, policy.GetAclsSize());
    ASSERT_EQ((size_t)1, policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_FROM_CERTIFICATE_AUTHORITY, retrievedPeerType);
    EXPECT_STREQ(FIRST_VALID_PUBLIC_KEY, retrievedPeerPublicKeyPEM.c_str());
    EXPECT_EQ(qcc::GUID128(0), retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetValidPeerForWithPublicKey)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_WITH_PUBLIC_KEY, policy));
    ASSERT_EQ((size_t)1, policy.GetAclsSize());
    ASSERT_EQ((size_t)1, policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_WITH_PUBLIC_KEY, retrievedPeerType);
    EXPECT_STREQ(FIRST_VALID_PUBLIC_KEY, retrievedPeerPublicKeyPEM.c_str());
    EXPECT_EQ(qcc::GUID128(0), retrievedPeerSgId);
}

TEST_F(XmlPoliciesConverterFromXmlDetailedTest, shouldGetValidPeerForWithMembershipType)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_WITH_MEMBERSHIP, policy));
    ASSERT_EQ((size_t)1, policy.GetAclsSize());
    ASSERT_EQ((size_t)1, policy.GetAcls()[0].GetPeersSize());

    RetrievePeerDetails(policy.GetAcls()[0].GetPeers()[0]);

    EXPECT_EQ(PermissionPolicy::Peer::PeerType::PEER_WITH_MEMBERSHIP, retrievedPeerType);
    EXPECT_STREQ(FIRST_VALID_PUBLIC_KEY, retrievedPeerPublicKeyPEM.c_str());
    EXPECT_EQ(qcc::GUID128(FIRST_VALID_GUID), retrievedPeerSgId);
}

INSTANTIATE_TEST_CASE_P(XmlPoliciesConverterInvalidRulesSet,
                        XmlPoliciesConverterFromXmlFailureTest,
                        ::testing::Values(EMPTY_POLICY_ELEMENT,
                                          EMPTY_POLICY_VERSION_ELEMENT,
                                          EMPTY_SERIAL_NUMBER_ELEMENT,
                                          EMPTY_TYPE_ELEMENT,
                                          MISSING_ACLS_ELEMENT,
                                          MISSING_ACL_ELEMENT,
                                          MISSING_PEERS_ELEMENT,
                                          MISSING_PEER_ELEMENT,
                                          MISSING_POLICY_VERSION_ELEMENT,
                                          MISSING_RULES_ELEMENT,
                                          MISSING_SERIAL_NUMBER_ELEMENT,
                                          MISSING_TYPE_ELEMENT,
                                          POLICY_ELEMENTS_INCORRECT_ORDER,
                                          ACL_ELEMENTS_INCORRECT_ORDER,
                                          PEER_ELEMENTS_INCORRECT_ORDER,
                                          INVALID_PUBLIC_KEY,
                                          INVALID_SGID,
                                          POLICY_VERSION_NOT_ONE,
                                          SERIAL_NUMBER_NEGATIVE,
                                          UNKNOWN_PEER_TYPE,
                                          ALL_TYPE_WITH_OTHER,
                                          ANY_TRUSTED_TWICE,
                                          SAME_FROM_CA_TWICE,
                                          SAME_WITH_PUBLIC_KEY_TWICE,
                                          SAME_WITH_MEMBERSHIP_TWICE));
TEST_P(XmlPoliciesConverterFromXmlFailureTest, shouldReturnErrorForInvalidRulesSet)
{
    EXPECT_EQ(ER_XML_MALFORMED, XmlPoliciesConverter::FromXml(policyXml, policy));
}

INSTANTIATE_TEST_CASE_P(XmlPoliciesConverterPass,
                        XmlPoliciesConverterFromXmlPassTest,
                        ::testing::Values(VALID_ALL_TYPE_PEER,
                                          VALID_TWO_ACLS,
                                          VALID_ANY_TRUSTED_PEER,
                                          VALID_ANY_TRUSTED_PEER_WITH_OTHER,
                                          VALID_FROM_CA,
                                          VALID_SAME_KEY_CA_AND_WITH_PUBLIC_KEY,
                                          VALID_TWO_DIFFERENT_CA,
                                          VALID_TWO_DIFFERENT_WITH_PUBLIC_KEY,
                                          VALID_TWO_WITH_MEMBERSHIP_DIFFERENT_KEYS,
                                          VALID_TWO_WITH_MEMBERSHIP_DIFFERENT_SGIDS,
                                          VALID_WHITESPACE_IN_POLICY_VERSION,
                                          VALID_WHITESPACE_IN_PUBLIC_KEY,
                                          VALID_WHITESPACE_IN_SERIAL_NUMBER,
                                          VALID_WHITESPACE_IN_SGID,
                                          VALID_WHITESPACE_IN_TYPE,
                                          VALID_WITH_MEMBERSHIP,
                                          VALID_WITH_PUBLIC_KEY));
TEST_P(XmlPoliciesConverterFromXmlPassTest, shouldPassForValidInput)
{
    EXPECT_EQ(ER_OK, XmlPoliciesConverter::FromXml(policyXml, policy));
}
