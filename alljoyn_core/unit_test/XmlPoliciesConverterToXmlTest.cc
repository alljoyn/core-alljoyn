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
#include "KeyInfoHelper.h"
#include "XmlPoliciesConverter.h"
#include "XmlPoliciesConverterTest.h"

using namespace std;
using namespace qcc;
using namespace ajn;

#define ANY_TRUSTED_PEER_INDEX 0
#define FIRST_WITH_MEMBERSHIP_PEER_INDEX 1
#define SECOND_WITH_MEMBERSHIP_PEER_INDEX 2
#define FIRST_FROM_CA_PEER_INDEX 3
#define SECOND_FROM_CA_PEER_INDEX 4
#define FIRST_WITH_PUBLIC_KEY_PEER_INDEX 5
#define SECOND_WITH_PUBLIC_KEY_PEER_INDEX 6

static AJ_PCSTR VALID_ALL_CASES_POLICY =
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

class PolicyOverwriteUtils {
  public:

    static void ChangeRules(size_t rulesCount, PermissionPolicy::Rule* rules, PermissionPolicy& policy)
    {
        PermissionPolicy::Acl* mutableAcl = new PermissionPolicy::Acl();
        mutableAcl->SetRules(rulesCount, rules);

        policy.SetAcls(1U, mutableAcl);
    }

    static void ChangePeers(size_t peersCount, PermissionPolicy::Peer* peers, PermissionPolicy& policy)
    {
        PermissionPolicy::Acl* mutableAcl = new PermissionPolicy::Acl();
        mutableAcl->SetPeers(peersCount, peers);

        policy.SetAcls(1U, mutableAcl);
    }

    static void ChangePeerType(size_t peerIndex, PermissionPolicy::Peer::PeerType peerType, PermissionPolicy& policy)
    {
        PermissionPolicy::Acl* mutableAcls = nullptr;
        PermissionPolicy::Peer* mutablePeers = nullptr;

        GetAclsCopy(policy, &mutableAcls);
        GetPeersCopy(mutableAcls[0], &mutablePeers);

        mutablePeers[peerIndex].SetType(peerType);
        mutableAcls[0].SetPeers(mutableAcls[0].GetPeersSize(), mutablePeers);

        policy.SetAcls(1, mutableAcls);
    }

    static void ChangePeerPublicKey(size_t peerIndex, AJ_PCSTR publicKeyPem, PermissionPolicy& policy)
    {
        PermissionPolicy::Acl* mutableAcls = nullptr;
        PermissionPolicy::Peer* mutablePeers = nullptr;

        GetAclsCopy(policy, &mutableAcls);
        GetPeersCopy(mutableAcls[0], &mutablePeers);

        SetPeerPublicKey(publicKeyPem, mutablePeers[peerIndex]);
        mutableAcls[0].SetPeers(mutableAcls[0].GetPeersSize(), mutablePeers);

        policy.SetAcls(1, mutableAcls);
    }

    static void ChangePeerSgId(size_t peerIndex, AJ_PCSTR sgIdHex, PermissionPolicy& policy)
    {
        PermissionPolicy::Acl* mutableAcls = nullptr;
        PermissionPolicy::Peer* mutablePeers = nullptr;

        GetAclsCopy(policy, &mutableAcls);
        GetPeersCopy(mutableAcls[0], &mutablePeers);

        mutablePeers[peerIndex].SetSecurityGroupId(GUID128(sgIdHex));
        mutableAcls[0].SetPeers(mutableAcls[0].GetPeersSize(), mutablePeers);

        policy.SetAcls(1, mutableAcls);
    }

    static PermissionPolicy::Peer BuildPeer(PermissionPolicy::Peer::PeerType peerType, AJ_PCSTR publicKeyPem = nullptr, AJ_PCSTR sgIdHex = nullptr)
    {
        PermissionPolicy::Peer result;

        result.SetType(peerType);
        SetPeerPublicKey(publicKeyPem, result);

        if (nullptr != sgIdHex) {
            result.SetSecurityGroupId(GUID128(sgIdHex));
        }

        return result;
    }

  private:

    static void GetAclsCopy(const PermissionPolicy& policy, PermissionPolicy::Acl** mutableAcls)
    {
        size_t aclsSize = policy.GetAclsSize();
        *mutableAcls = new PermissionPolicy::Acl[aclsSize];

        for (size_t index = 0; index < aclsSize; index++) {
            (*mutableAcls)[index] = policy.GetAcls()[index];
        }
    }

    static void GetPeersCopy(const PermissionPolicy::Acl& acl, PermissionPolicy::Peer** mutablePeers)
    {
        size_t peersSize = acl.GetPeersSize();
        *mutablePeers = new PermissionPolicy::Peer[peersSize];

        for (size_t index = 0; index < peersSize; index++) {
            (*mutablePeers)[index] = acl.GetPeers()[index];
        }
    }

    static void SetPeerPublicKey(AJ_PCSTR publicKeyPem, PermissionPolicy::Peer& peer)
    {
        KeyInfoNISTP256* publicKey = nullptr;

        if (nullptr != publicKeyPem) {
            publicKey = new KeyInfoNISTP256();
            ASSERT_EQ(ER_OK, KeyInfoHelper::PEMToKeyInfoNISTP256(publicKeyPem, *publicKey));
        }

        peer.SetKeyInfo(publicKey);
    }
};

struct KeyParams {
  public:
    uint32_t index;
    AJ_PCSTR keyPem;

    KeyParams(uint32_t _index, AJ_PCSTR _keyPem) :
        index(_index),
        keyPem(_keyPem)
    { }
};

struct GuidParams {
  public:
    uint32_t index;
    AJ_PCSTR guid;

    GuidParams(uint32_t _index, AJ_PCSTR _guid) :
        index(_index),
        guid(_guid)
    { }
};

struct TypeParams {
  public:
    uint32_t index;
    PermissionPolicy::Peer::PeerType type;

    TypeParams(uint32_t _index, PermissionPolicy::Peer::PeerType _type) :
        index(_index),
        type(_type)
    { }
};

class XmlPoliciesConverterToXmlDetailedTest : public testing::Test {
  public:
    PermissionPolicy validPolicy;
    AJ_PSTR retrievedPolicyXml;

    XmlPoliciesConverterToXmlDetailedTest() :
        retrievedPolicyXml(nullptr)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_ALL_CASES_POLICY, validPolicy));
    }

    virtual void TearDown()
    {
        delete[] retrievedPolicyXml;
    }
};

class XmlPolicyConverterToXmlDetailedFailureTest : public XmlPoliciesConverterToXmlDetailedTest { };

class XmlPolicyConverterToXmlDetailedPassTest : public XmlPoliciesConverterToXmlDetailedTest {
  public:
    PermissionPolicy retrievedPolicy;
};

class XmlPolicyConverterToXmlPublicKeyTest : public testing::TestWithParam<KeyParams> {
  public:
    uint32_t peerIndex;
    AJ_PSTR retrievedPolicyXml;
    PermissionPolicy validPolicy;
    PermissionPolicy retrievedPolicy;
    KeyInfoNISTP256 expectedPublicKey;

    XmlPolicyConverterToXmlPublicKeyTest() :
        peerIndex(GetParam().index),
        retrievedPolicyXml(nullptr)
    { }

    virtual void SetUp() {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_ALL_CASES_POLICY, validPolicy));
        ASSERT_EQ(ER_OK, KeyInfoHelper::PEMToKeyInfoNISTP256(GetParam().keyPem, expectedPublicKey));
    }

    virtual void TearDown()
    {
        delete[] retrievedPolicyXml;
    }
};

class XmlPolicyConverterToXmlGuidTest : public testing::TestWithParam<GuidParams> {
  public:
    uint32_t peerIndex;
    AJ_PSTR retrievedPolicyXml;
    PermissionPolicy validPolicy;
    PermissionPolicy retrievedPolicy;
    GUID128 expectedGuid;

    XmlPolicyConverterToXmlGuidTest() :
        peerIndex(GetParam().index),
        retrievedPolicyXml(nullptr),
        expectedGuid(GetParam().guid)
    { }

    virtual void SetUp() {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_ALL_CASES_POLICY, validPolicy));
    }

    virtual void TearDown()
    {
        delete[] retrievedPolicyXml;
    }
};

class XmlPolicyConverterToXmlPeerTypeTest : public testing::TestWithParam<TypeParams> {
  public:
    uint32_t peerIndex;
    AJ_PSTR retrievedPolicyXml;
    PermissionPolicy validPolicy;
    PermissionPolicy retrievedPolicy;
    PermissionPolicy::Peer::PeerType expectedType;

    XmlPolicyConverterToXmlPeerTypeTest() :
        peerIndex(GetParam().index),
        retrievedPolicyXml(nullptr),
        expectedType(GetParam().type)
    { }

    virtual void SetUp() {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(VALID_ALL_CASES_POLICY, validPolicy));
    }

    virtual void TearDown()
    {
        delete[] retrievedPolicyXml;
    }
};

class XmlPolicyConverterToXmlPassTest : public testing::TestWithParam<AJ_PCSTR> {
  public:
    PermissionPolicy policy;
    AJ_PSTR retrievedPolicyXml;

    XmlPolicyConverterToXmlPassTest() :
        retrievedPolicyXml(nullptr)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(GetParam(), policy));
    }

    virtual void TearDown()
    {
        delete[] retrievedPolicyXml;
    }
};

class XmlPolicyConverterToXmlCountTest : public testing::TestWithParam<SizeParams> {
  public:
    PermissionPolicy validPolicy;
    PermissionPolicy retrievedPolicy;
    size_t expectedCount;
    AJ_PSTR retrievedPolicyXml;

    XmlPolicyConverterToXmlCountTest() :
        expectedCount(GetParam().integer),
        retrievedPolicyXml(nullptr)
    { }

    virtual void SetUp()
    {
        ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(GetParam().xml, validPolicy));
    }

    virtual void TearDown()
    {
        delete[] retrievedPolicyXml;
    }
};

class XmlPolicyConverterToXmlAclsCountTest : public XmlPolicyConverterToXmlCountTest { };

class XmlPolicyConverterToXmlPeersCountTest : public XmlPolicyConverterToXmlCountTest { };

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForInvalidPolicyVersion)
{
    validPolicy.SetSpecificationVersion(0);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForZeroAcls)
{
    validPolicy.SetAcls(0, nullptr);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForZeroPeers)
{
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeers(0, nullptr, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForZeroRules)
{
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangeRules(0, nullptr, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForAllTypePeerWithOthers)
{
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerType(ANY_TRUSTED_PEER_INDEX, PermissionPolicy::Peer::PEER_ALL, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForAnyTrustedTypePeerTwice)
{
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerType(FIRST_FROM_CA_PEER_INDEX, PermissionPolicy::Peer::PEER_ANY_TRUSTED, validPolicy);
    PolicyOverwriteUtils::ChangePeerPublicKey(FIRST_FROM_CA_PEER_INDEX, nullptr, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForAnyTrustedPeerWithPublicKey)
{
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(ANY_TRUSTED_PEER_INDEX, FIRST_VALID_PUBLIC_KEY, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForAllTypePeerWithPublicKey)
{
    PermissionPolicy::Peer allPeer = PolicyOverwriteUtils::BuildPeer(PermissionPolicy::Peer::PEER_ALL, FIRST_VALID_PUBLIC_KEY);
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeers(1, &allPeer, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForWithPublicKeyPeerTypeWithoutPublicKey)
{
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(FIRST_WITH_PUBLIC_KEY_PEER_INDEX, nullptr, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForWithMembershipPeerTypeWithoutPublicKey)
{
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(FIRST_WITH_MEMBERSHIP_PEER_INDEX, nullptr, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForCAPeerTypeWithoutPublicKey)
{
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(FIRST_FROM_CA_PEER_INDEX, nullptr, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForTwoSameWithMembershipPeers)
{
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(SECOND_WITH_MEMBERSHIP_PEER_INDEX, FIRST_VALID_PUBLIC_KEY, validPolicy);
    PolicyOverwriteUtils::ChangePeerSgId(SECOND_WITH_MEMBERSHIP_PEER_INDEX, FIRST_VALID_GUID, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForTwoSameWithPublicKeyPeers)
{
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(SECOND_WITH_PUBLIC_KEY_PEER_INDEX, FIRST_VALID_PUBLIC_KEY, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedFailureTest, shouldReturnErrorForTwoSameCAPeers)
{
    ASSERT_GT(validPolicy.GetAclsSize(), 0U);
    PolicyOverwriteUtils::ChangePeerPublicKey(SECOND_FROM_CA_PEER_INDEX, FIRST_VALID_PUBLIC_KEY, validPolicy);

    EXPECT_EQ(ER_FAIL, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSamePolicyAfterTwoConversions)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));

    EXPECT_EQ(validPolicy, retrievedPolicy);
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSamePolicyXmlAfterTwoConversions)
{
    AJ_PSTR secondRetrievedPolicyXml = nullptr;
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(retrievedPolicy, &secondRetrievedPolicyXml));

    EXPECT_STREQ(retrievedPolicyXml, secondRetrievedPolicyXml);
    delete[] secondRetrievedPolicyXml;
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetPolicyVersion)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));

    EXPECT_EQ(1U, retrievedPolicy.GetSpecificationVersion());
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSerialNumber)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));

    EXPECT_EQ((uint16_t)10, retrievedPolicy.GetVersion());
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSomeAcls)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));

    EXPECT_GT(retrievedPolicy.GetAclsSize(), 0U);
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSomePeers)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));
    ASSERT_GT(retrievedPolicy.GetAclsSize(), 0U);

    EXPECT_GT(retrievedPolicy.GetAcls()[0].GetPeersSize(), 0U);
}

TEST_F(XmlPolicyConverterToXmlDetailedPassTest, shouldGetSomeRules)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));
    ASSERT_GT(retrievedPolicy.GetAclsSize(), 0U);

    EXPECT_GT(retrievedPolicy.GetAcls()[0].GetRulesSize(), 0U);
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
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));
    ASSERT_GT(retrievedPolicy.GetAclsSize(), 0U);
    ASSERT_GT(retrievedPolicy.GetAcls()[0].GetPeersSize(), peerIndex);

    PermissionPolicy::Peer analyzedPeer = retrievedPolicy.GetAcls()[0].GetPeers()[peerIndex];

    EXPECT_EQ(expectedPublicKey, *(analyzedPeer.GetKeyInfo()));
}

INSTANTIATE_TEST_CASE_P(XmlPolicyConverterToXmlGuidCorrect,
                        XmlPolicyConverterToXmlGuidTest,
                        ::testing::Values(GuidParams(FIRST_WITH_MEMBERSHIP_PEER_INDEX, FIRST_VALID_GUID),
                                          GuidParams(SECOND_WITH_MEMBERSHIP_PEER_INDEX, SECOND_VALID_GUID)));
TEST_P(XmlPolicyConverterToXmlGuidTest, shouldGetCorrectGuid)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));
    ASSERT_GT(retrievedPolicy.GetAclsSize(), 0U);
    ASSERT_GT(retrievedPolicy.GetAcls()[0].GetPeersSize(), peerIndex);

    PermissionPolicy::Peer analyzedPeer = retrievedPolicy.GetAcls()[0].GetPeers()[peerIndex];

    EXPECT_EQ(expectedGuid, analyzedPeer.GetSecurityGroupId());
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
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));
    ASSERT_GT(retrievedPolicy.GetAclsSize(), 0U);
    ASSERT_GT(retrievedPolicy.GetAcls()[0].GetPeersSize(), peerIndex);

    PermissionPolicy::Peer analyzedPeer = retrievedPolicy.GetAcls()[0].GetPeers()[peerIndex];

    EXPECT_EQ(expectedType, analyzedPeer.GetType());
}

INSTANTIATE_TEST_CASE_P(XmlPolicyConverterToXmlPass,
                        XmlPolicyConverterToXmlPassTest,
                        ::testing::Values(VALID_ALL_CASES_POLICY,
                                          VALID_ALL_TYPE_PEER,
                                          VALID_TWO_ACLS,
                                          VALID_ANY_TRUSTED_PEER,
                                          VALID_ANY_TRUSTED_PEER_WITH_OTHER,
                                          VALID_FROM_CA,
                                          VALID_SAME_KEY_CA_AND_WITH_PUBLIC_KEY,
                                          VALID_TWO_DIFFERENT_CA,
                                          VALID_TWO_DIFFERENT_WITH_PUBLIC_KEY,
                                          VALID_TWO_WITH_MEMBERSHIP_DIFFERENT_KEYS,
                                          VALID_TWO_WITH_MEMBERSHIP_DIFFERENT_SGIDS,
                                          VALID_WITH_MEMBERSHIP,
                                          VALID_WITH_PUBLIC_KEY));
TEST_P(XmlPolicyConverterToXmlPassTest, shouldPassForValidInput)
{
    EXPECT_EQ(ER_OK, XmlPoliciesConverter::ToXml(policy, &retrievedPolicyXml));
}

INSTANTIATE_TEST_CASE_P(XmlPolicyConverterToXmlAclsCount,
                        XmlPolicyConverterToXmlAclsCountTest,
                        ::testing::Values(SizeParams(VALID_ALL_CASES_POLICY, 1),
                                          SizeParams(VALID_TWO_ACLS, 2)));
TEST_P(XmlPolicyConverterToXmlAclsCountTest, shouldGetCorrectAclsCount)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));

    EXPECT_EQ(expectedCount, retrievedPolicy.GetAclsSize());
}

INSTANTIATE_TEST_CASE_P(XmlPolicyConverterToXmlPeersCount,
                        XmlPolicyConverterToXmlPeersCountTest,
                        ::testing::Values(SizeParams(VALID_ALL_CASES_POLICY, 7),
                                          SizeParams(VALID_ALL_TYPE_PEER, 1),
                                          SizeParams(VALID_TWO_ACLS, 1),
                                          SizeParams(VALID_ANY_TRUSTED_PEER, 1),
                                          SizeParams(VALID_ANY_TRUSTED_PEER_WITH_OTHER, 2),
                                          SizeParams(VALID_FROM_CA, 1),
                                          SizeParams(VALID_SAME_KEY_CA_AND_WITH_PUBLIC_KEY, 2),
                                          SizeParams(VALID_TWO_DIFFERENT_CA, 2),
                                          SizeParams(VALID_TWO_DIFFERENT_WITH_PUBLIC_KEY, 2),
                                          SizeParams(VALID_TWO_WITH_MEMBERSHIP_DIFFERENT_KEYS, 2),
                                          SizeParams(VALID_TWO_WITH_MEMBERSHIP_DIFFERENT_SGIDS, 2),
                                          SizeParams(VALID_WITH_MEMBERSHIP, 1),
                                          SizeParams(VALID_WITH_PUBLIC_KEY, 1)));
TEST_P(XmlPolicyConverterToXmlPeersCountTest, shouldGetCorrectPeersCount)
{
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::ToXml(validPolicy, &retrievedPolicyXml));
    ASSERT_EQ(ER_OK, XmlPoliciesConverter::FromXml(retrievedPolicyXml, retrievedPolicy));
    ASSERT_GT(retrievedPolicy.GetAclsSize(), 0);

    EXPECT_EQ(expectedCount, retrievedPolicy.GetAcls()[0].GetPeersSize());
}
