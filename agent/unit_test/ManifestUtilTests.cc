/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#include <alljoyn/securitymgr/Util.h>

#include "TestUtil.h"

namespace secmgrcoretest_unit_nominaltests {
using namespace secmgrcoretest_unit_testutil;
using namespace ajn::securitymgr;

class ManifestUtilTests :
    public BasicTest {
  private:

  protected:

  public:
    ManifestUtilTests()
    {
    }

    QStatus GenerateManifest(PermissionPolicy::Rule** retRules,
                             size_t* count)
    {
        *count = 2;
        PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
        rules[0].SetInterfaceName("org.allseenalliance.control.TV");
        PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
        prms[0].SetMemberName("Up");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[1].SetMemberName("Down");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(2, prms);

        rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
        prms = new PermissionPolicy::Rule::Member[1];
        prms[0].SetMemberName("*");
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[1].SetMembers(1, prms);

        *retRules = rules;
        return ER_OK;
    }
};

static void PrintDigest(const uint8_t* const buf)
{
    for (size_t i = 0; i < Crypto_SHA256::DIGEST_SIZE; i++) {
        if (i > 0) {
            printf(":");
        }
        printf("%02X", buf[i]);
    }
    printf("\n");
}

TEST_F(ManifestUtilTests, ManifestConstruction) {
    Manifest emptyManifest;
    uint8_t* byteString = nullptr;
    size_t sizeOfByteArray;
    PermissionPolicy::Rule* rules = nullptr;
    PermissionPolicy::Rule* otherRules = nullptr;
    size_t count = 0;

    ASSERT_EQ(ER_OK, Util::Init(ba));

    ASSERT_EQ(ER_END_OF_DATA, emptyManifest.GetByteArray(&byteString, &sizeOfByteArray));
    ASSERT_EQ(ER_END_OF_DATA, emptyManifest.GetRules(&rules, &count));
    ASSERT_TRUE(byteString == nullptr);
    ASSERT_EQ((size_t)0, sizeOfByteArray);
    ASSERT_EQ((size_t)0, count);
    ASSERT_TRUE(nullptr == rules);

    // Test construction by rules
    EXPECT_EQ(ER_OK, GenerateManifest(&otherRules, &count));
    Manifest manifestFromRules(otherRules, count);
    EXPECT_EQ(ER_OK, manifestFromRules.GetByteArray(&byteString, &sizeOfByteArray));
    EXPECT_EQ(ER_OK, manifestFromRules.GetRules(&rules, &count));
    EXPECT_TRUE(byteString != nullptr);
    EXPECT_NE((size_t)0, sizeOfByteArray);

    EXPECT_EQ((size_t)2, count);
    EXPECT_EQ((size_t)2, rules->GetMembersSize());
    EXPECT_EQ((size_t)2, otherRules->GetMembersSize());
    EXPECT_TRUE(*otherRules == *rules);
    EXPECT_TRUE(otherRules != rules);

    // Test construction by byteString
    Manifest manifestFromByteString(byteString, sizeOfByteArray);
    uint8_t* byteString2 = nullptr;
    size_t sizeOfByteArray2;

    EXPECT_EQ(ER_OK, manifestFromByteString.GetByteArray(&byteString2, &sizeOfByteArray2));
    EXPECT_EQ(ER_OK, manifestFromByteString.GetRules(&rules, &count));
    EXPECT_TRUE(byteString2 != nullptr);
    EXPECT_NE((size_t)0, sizeOfByteArray2);

    EXPECT_EQ((size_t)2, count);
    EXPECT_EQ((size_t)2, rules->GetMembersSize());

    EXPECT_TRUE(*otherRules == *rules);
    EXPECT_FALSE(otherRules == rules);
    EXPECT_TRUE(memcmp(byteString, byteString2, sizeOfByteArray2) == 0);

    delete[]byteString2;
    byteString2 = nullptr;
    delete[]byteString;
    byteString = nullptr;

    uint8_t* digestMfromRules = new uint8_t[Crypto_SHA256::DIGEST_SIZE];
    uint8_t* digestMFromByteStr = new uint8_t[Crypto_SHA256::DIGEST_SIZE];

    EXPECT_EQ(ER_OK, manifestFromByteString.GetDigest(digestMFromByteStr));
    EXPECT_EQ(ER_OK, manifestFromRules.GetDigest(digestMfromRules));

    EXPECT_EQ(0, memcmp(digestMfromRules, digestMFromByteStr, Crypto_SHA256::DIGEST_SIZE));

    // Test copy constructor and comparison
    Manifest copyManifest(manifestFromByteString);
    EXPECT_TRUE(copyManifest == manifestFromByteString);
    EXPECT_FALSE(copyManifest != manifestFromByteString);
    EXPECT_TRUE(copyManifest == manifestFromRules);
    EXPECT_FALSE(copyManifest != manifestFromRules);

    //Test assignment operator
    Manifest manifestAssignee;
    manifestAssignee = manifestFromByteString;
    EXPECT_TRUE(manifestAssignee == manifestFromByteString);
    EXPECT_TRUE(manifestAssignee == manifestFromRules);
    EXPECT_TRUE(manifestAssignee != emptyManifest);

    // Test GetDigest after copy and assignment
    uint8_t* digest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];
    uint8_t* otherDigest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];

    EXPECT_EQ(ER_OK, copyManifest.GetDigest(digest));
    EXPECT_EQ(ER_OK, manifestFromByteString.GetDigest(otherDigest));

    cout << "Digest is \n";
    PrintDigest(digest);

    cout << "otherDigest is \n";
    PrintDigest(otherDigest);

    EXPECT_EQ(0, memcmp(digest, otherDigest, Crypto_SHA256::DIGEST_SIZE));

    uint8_t* assigneeDigest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];
    Manifest assigneeManifest = copyManifest;
    EXPECT_EQ(ER_OK, assigneeManifest.GetDigest(assigneeDigest));
    cout << "assigneeDigest is \n";
    PrintDigest(assigneeDigest);
    EXPECT_EQ(0, memcmp(assigneeDigest, otherDigest, Crypto_SHA256::DIGEST_SIZE));

    delete[]otherRules;
    delete[]rules;
    delete digest;
    delete otherDigest;
    delete assigneeDigest;
    delete digestMfromRules;
    delete digestMFromByteStr;
    ASSERT_EQ(ER_OK, Util::Fini());
}

TEST_F(ManifestUtilTests, PermissionPolicyDigestExtendedTest) {
    PermissionPolicy::Rule* rules;
    size_t size;

    EXPECT_EQ(ER_OK, GenerateManifest(&rules, &size));

    PermissionPolicy permPolicy;
    PermissionPolicy::Acl* acl = new PermissionPolicy::Acl[1];
    acl[0].SetRules(size, rules);
    permPolicy.SetAcls(1, acl);

    uint8_t* originalDigest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];

    Message msg(*ba);
    DefaultPolicyMarshaller marshaller(msg);

    permPolicy.Digest(marshaller, originalDigest, Crypto_SHA256::DIGEST_SIZE);

    PermissionPolicy permPolicyCopy(permPolicy);

    uint8_t* copyDigest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];

    permPolicyCopy.Digest(marshaller, copyDigest, Crypto_SHA256::DIGEST_SIZE);

    EXPECT_EQ(0, memcmp(copyDigest, originalDigest, Crypto_SHA256::DIGEST_SIZE));

    PermissionPolicy permPolicyAssignee = permPolicy;

    uint8_t* assigneeDigest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];

    permPolicyAssignee.Digest(marshaller, assigneeDigest, Crypto_SHA256::DIGEST_SIZE);

    EXPECT_EQ(0, memcmp(assigneeDigest, originalDigest, Crypto_SHA256::DIGEST_SIZE));

    PermissionPolicy policyFromImport;
    {
        EXPECT_EQ(ER_OK, Util::Init(ba));
        uint8_t* byteArrayExp;
        size_t size;
        EXPECT_EQ(ER_OK, Util::GetPolicyByteArray(permPolicy, &byteArrayExp, &size));
        EXPECT_EQ(ER_OK, Util::GetPolicy(byteArrayExp, size, policyFromImport));
        EXPECT_EQ(ER_OK, Util::Fini());
        delete[]byteArrayExp;
    }

    uint8_t* policyFromImportDigest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, policyFromImport.Digest(marshaller, policyFromImportDigest, Crypto_SHA256::DIGEST_SIZE));

    EXPECT_EQ(0, memcmp(policyFromImportDigest, originalDigest, Crypto_SHA256::DIGEST_SIZE));

    delete copyDigest;
    delete assigneeDigest;
    delete originalDigest;
    delete policyFromImportDigest;
}
}
