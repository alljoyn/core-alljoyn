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

using namespace ajn::securitymgr;

/** @file ManifestUtilTests.cc */

namespace secmgr_tests {
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

    void GetManifest(Manifest& mf)
    {
        PermissionPolicy::Rule rules[1];
        rules[0].SetInterfaceName("org.allseenalliance.control.TV");
        PermissionPolicy::Rule::Member prms[3];
        prms[0].SetMemberName("Up");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[1].SetMemberName("Down");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[2].SetMemberName("Channel");
        prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        rules[0].SetMembers(3, prms);

        mf.SetFromRules(rules, 1);
    }

    void GetPermutedManifest(Manifest& mf)
    {
        PermissionPolicy::Rule rules[1];
        rules[0].SetInterfaceName("org.allseenalliance.control.TV");
        PermissionPolicy::Rule::Member prms[3];
        prms[0].SetMemberName("Channel");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        prms[1].SetMemberName("Down");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[2].SetMemberName("Up");
        prms[2].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(3, prms);

        mf.SetFromRules(rules, 1);
    }

    void GetSplitManifest(Manifest& mf)
    {
        PermissionPolicy::Rule rules[2];
        rules[0].SetInterfaceName("org.allseenalliance.control.TV");
        PermissionPolicy::Rule::Member prms[2];
        prms[0].SetMemberName("Up");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[1].SetMemberName("Down");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(2, prms);

        rules[1].SetInterfaceName("org.allseenalliance.control.TV");
        PermissionPolicy::Rule::Member tprms[1];
        tprms[0].SetMemberName("Channel");
        tprms[0].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        tprms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        rules[1].SetMembers(1, tprms);

        mf.SetFromRules(rules, 2);
    }

    void GetExtendedManifest(Manifest& mf)
    {
        PermissionPolicy::Rule rules[2];

        rules[0].SetInterfaceName("org.allseenalliance.control.TV");
        PermissionPolicy::Rule::Member prms[3];
        prms[0].SetMemberName("Up");
        prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[1].SetMemberName("Down");
        prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
        prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        prms[2].SetMemberName("Channel");
        prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
        prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE |
                              PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[0].SetMembers(3, prms);

        rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
        PermissionPolicy::Rule::Member mprms[1];
        mprms[0].SetMemberName("*");
        mprms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
        rules[1].SetMembers(1, mprms);

        mf.SetFromRules(rules, 2);
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

/**
 * @test Verify the construction of valid Manifest objects using the Manifest
 *       class. Also verify the provided operators and the digest matching.
 *       -# Create an empty manifest object and make sure that its rules and
 *          byte-array are empty. Both sizes of rules and the byte-array
 *          should be zero.
 *       -# Create a manifest from some two generated rules (manifestFromRules)
 *          and verify that it has those exact generated rules. Also, make sure
 *          that the corresponding byte-array is not empty.
 *       -# Repeat the previous step for a manifest created form the previous
 *          byte-array (manifestFromByteArray). Also, verify that the
 *          byte-array of manifestFromRules matches that of manifestFromByteArray.
 *       -# Get digests of both manifestFromByteArray and manifestFromRules and
 *          make sure they match.
 *       -# Create a copy (copyManifest) form manifestFromByteArray using the
 *          copy constructor and make sure == and != operators against
 *          manifestFromByteArray and manifestFromRules hold.
 *       -# Create a manifestAssignee using the assignment operator form
 *          manifestFromByteArray and make sure == and != operators against
 *          manifestFromByteArray and manifestFromRules hold.
 *       -# Get the digests from  manifestAssignee, manifestFromByteArray and
 *          copyManifest and make sure they are all identical.
 **/
TEST_F(ManifestUtilTests, ManifestConstruction) {
    Manifest emptyManifest;
    uint8_t* byteArray = nullptr;
    size_t sizeOfByteArray;
    PermissionPolicy::Rule* rules = nullptr;
    PermissionPolicy::Rule* otherRules = nullptr;
    size_t count = 0;

    ASSERT_EQ(ER_OK, Util::Init(ba));

    ASSERT_EQ(ER_END_OF_DATA, emptyManifest.GetByteArray(&byteArray, &sizeOfByteArray));
    ASSERT_EQ(ER_END_OF_DATA, emptyManifest.GetRules(&rules, &count));
    ASSERT_TRUE(byteArray == nullptr);
    ASSERT_EQ((size_t)0, sizeOfByteArray);
    ASSERT_EQ((size_t)0, count);
    ASSERT_TRUE(nullptr == rules);

    // Test construction by rules
    EXPECT_EQ(ER_OK, GenerateManifest(&otherRules, &count));
    Manifest manifestFromRules(otherRules, count);
    EXPECT_EQ(ER_OK, manifestFromRules.GetByteArray(&byteArray, &sizeOfByteArray));
    EXPECT_EQ(ER_OK, manifestFromRules.GetRules(&rules, &count));
    EXPECT_TRUE(byteArray != nullptr);
    EXPECT_NE((size_t)0, sizeOfByteArray);

    EXPECT_EQ((size_t)2, count);
    EXPECT_EQ((size_t)2, rules->GetMembersSize());
    EXPECT_EQ((size_t)2, otherRules->GetMembersSize());
    EXPECT_TRUE(*otherRules == *rules);
    EXPECT_TRUE(otherRules != rules);

    // Test construction by byteString
    Manifest manifestFromByteArray(byteArray, sizeOfByteArray);
    uint8_t* byteArray2 = nullptr;
    size_t sizeOfByteArray2;

    EXPECT_EQ(ER_OK, manifestFromByteArray.GetByteArray(&byteArray2, &sizeOfByteArray2));
    EXPECT_EQ(ER_OK, manifestFromByteArray.GetRules(&rules, &count));
    EXPECT_TRUE(byteArray2 != nullptr);
    EXPECT_NE((size_t)0, sizeOfByteArray2);

    EXPECT_EQ((size_t)2, count);
    EXPECT_EQ((size_t)2, rules->GetMembersSize());

    EXPECT_TRUE(*otherRules == *rules);
    EXPECT_FALSE(otherRules == rules);
    EXPECT_TRUE(memcmp(byteArray, byteArray2, sizeOfByteArray2) == 0);

    delete[]byteArray2;
    byteArray2 = nullptr;
    delete[]byteArray;
    byteArray = nullptr;

    uint8_t* digestMfromRules = new uint8_t[Crypto_SHA256::DIGEST_SIZE];
    uint8_t* digestMFromByteStr = new uint8_t[Crypto_SHA256::DIGEST_SIZE];

    EXPECT_EQ(ER_OK, manifestFromByteArray.GetDigest(digestMFromByteStr));
    EXPECT_EQ(ER_OK, manifestFromRules.GetDigest(digestMfromRules));

    EXPECT_EQ(0, memcmp(digestMfromRules, digestMFromByteStr, Crypto_SHA256::DIGEST_SIZE));

    // Test copy constructor and comparison
    Manifest copyManifest(manifestFromByteArray);
    EXPECT_TRUE(copyManifest == manifestFromByteArray);
    EXPECT_FALSE(copyManifest != manifestFromByteArray);
    EXPECT_TRUE(copyManifest == manifestFromRules);
    EXPECT_FALSE(copyManifest != manifestFromRules);

    //Test assignment operator
    Manifest manifestAssignee;
    manifestAssignee = manifestFromByteArray;
    EXPECT_TRUE(manifestAssignee == manifestFromByteArray);
    EXPECT_TRUE(manifestAssignee == manifestFromRules);
    EXPECT_TRUE(manifestAssignee != emptyManifest);

    // Test GetDigest after copy and assignment
    uint8_t* digest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];
    uint8_t* otherDigest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];

    EXPECT_EQ(ER_OK, copyManifest.GetDigest(digest));
    EXPECT_EQ(ER_OK, manifestFromByteArray.GetDigest(otherDigest));

    cout << "Digest is \n";
    PrintDigest(digest);

    cout << "otherDigest is \n";
    PrintDigest(otherDigest);

    EXPECT_EQ(0, memcmp(digest, otherDigest, Crypto_SHA256::DIGEST_SIZE));

    uint8_t* assigneeDigest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];
    // Manifest assigneeManifest = copyManifest;
    EXPECT_EQ(ER_OK, manifestAssignee.GetDigest(assigneeDigest));
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

/**
 * @test Verify the PermissionPolicy's digest consistency after
 *       copy or assignment operations. Also verify the functionalities
 *       provided by the static public Util class.
 *       -# Using a DefaultPolicyMarshaller create a PermissionPolicy
 *          (permPolicy) and get its digest.
 *       -# Successfully create a copy PermissionPolicy (permPolicyCopy) and
 *          get its digest.
 *       -# Compare digests from permPolicyCopy and permPolicy and make sure
 *          they match.
 *       -# Create a PermissionPolicy (permPolicyAssignee) using the assignment
 *          operator from permPolicy
 *          and make sure its digest matches those from permPolicyCopy and
 *          permPolicy.
 *       -# In a scoped block, initialize the Util class and get the byte-array
 *          of permPolicy successfully.
 *          Using that byteArray invoke GetPolicy on Util to create a
 *          policyFromImport successfully. Finalize the Util successfully.
 *       -# Finally, make sure the digest of policyFromImport matches that of
 *          permPolicy.
 **/
TEST_F(ManifestUtilTests, ExtendedPermissionPolicyDigest) {
    PermissionPolicy::Rule* rules;
    size_t size;

    EXPECT_EQ(ER_OK, GenerateManifest(&rules, &size));

    PermissionPolicy permPolicy;
    PermissionPolicy::Acl* acl = new PermissionPolicy::Acl[1];
    acl[0].SetRules(size, rules);
    permPolicy.SetAcls(1, acl);
    delete[] acl;
    acl = nullptr;

    uint8_t* originalDigest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];

    Message msg(*ba);
    DefaultPolicyMarshaller marshaller(msg);

    EXPECT_EQ(ER_OK, permPolicy.Digest(marshaller, originalDigest, Crypto_SHA256::DIGEST_SIZE));

    PermissionPolicy permPolicyCopy(permPolicy);

    uint8_t* copyDigest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];

    EXPECT_EQ(ER_OK, permPolicyCopy.Digest(marshaller, copyDigest, Crypto_SHA256::DIGEST_SIZE));

    EXPECT_EQ(0, memcmp(copyDigest, originalDigest, Crypto_SHA256::DIGEST_SIZE));

    PermissionPolicy permPolicyAssignee = permPolicy;

    uint8_t* assigneeDigest = new uint8_t[Crypto_SHA256::DIGEST_SIZE];

    EXPECT_EQ(ER_OK, permPolicyAssignee.Digest(marshaller, assigneeDigest, Crypto_SHA256::DIGEST_SIZE));

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

/**
 * @test Verify that the Manifest class methods can handle illegal arguments.
 *       -# Try to construct a manifest from nullptr rules and/or a size <= 0
 *          and make sure then it is equal to a manifest created with the
 *          default constructor.
 *       -# Try to construct a manifest from nullptr byte-array and/or a
 *          size <= 0 and make sure then it is equal to a manifest created
 *          with the default constructor.
 *       -# Call all getter functions on a manifest created with the default
 *          constructor with nullptr inputs and make sure this fails (!= ER_OK).
 *       -# Call all setter functions on a manifest created with the default
 *          constructor with nullptr first argument and size <= 0 and make sure
 *          all will fail (!= ER_OK).
 *       -# Call all setter functions on a manifest created with the default
 *          constructor with valid first argument and size <= 0 and make sure
 *          all will fail (!= ER_OK).
 **/
TEST_F(ManifestUtilTests, ManifestIllegalArgs) {
    ASSERT_EQ(ER_OK, Util::Init(ba));

    PermissionPolicy::Rule* rules = nullptr;
    size_t count;
    Manifest defaultManifest;

    Manifest* m = nullptr;
    m = new Manifest(rules, 0);
    ASSERT_TRUE(*m == defaultManifest);
    delete m;
    m = nullptr;

    uint8_t* nullByteArray = nullptr;
    m = new Manifest(nullByteArray, 0);
    ASSERT_TRUE(*m == defaultManifest);
    delete m;
    m = nullptr;

    ASSERT_NE(ER_OK, defaultManifest.GetByteArray(nullptr, nullptr));
    ASSERT_NE(ER_OK, defaultManifest.GetRules(nullptr, nullptr));
    ASSERT_NE(ER_OK, defaultManifest.GetDigest(nullptr));
    ASSERT_NE(ER_OK, defaultManifest.SetFromByteArray(nullptr, 0));
    ASSERT_NE(ER_OK, defaultManifest.SetFromRules(nullptr, 0));

    /*
     * \CAUTION: Calling SetFromRules and SetFromByteArray
     *           with a valid first argument and a negative
     *           second argument (size of type size_t)
     *            will most probably cause bad_alloc.
     */
    rules = nullptr;
    ASSERT_EQ(ER_OK, GenerateManifest(&rules, &count));
    ASSERT_NE(ER_OK, defaultManifest.SetFromRules(rules, 0));

    Manifest manifestFromRules(rules, count);
    uint8_t* byteArray = nullptr;
    size_t sizeOfByteArray;
    EXPECT_EQ(ER_OK, manifestFromRules.GetByteArray(&byteArray, &sizeOfByteArray));

    EXPECT_NE(ER_OK, defaultManifest.SetFromByteArray(byteArray, 0));

    delete[]rules;
    delete[]byteArray;

    ASSERT_EQ(ER_OK, Util::Fini());
}

/**
 * @test Verify that the Util class methods can handle illegal arguments.
 *       -# Init Util with a nullptr argument and make sure it fails.
 *       -# Call all methods of Util with valid arguments and make sure
 *          they all fail (!= ER_OK).
 *       -# Init Util with a valid busattachment and make sure it succeeds.
 *       -# Call GetDefaultMarshaller with nullptr argument and make sure
 *          it fails (!= ER_OK).
 *       -# Call GetPolicyByteArray with nullptr byte-array and size and
 *          make sure this fails (!= ER_OK).
 *       -# Call GetPolicy with nullptr byte-array and/or size <= 0 and
 *          make sure this fails (!= OR_OK).
 *       -# Call Fini on Util and make sure it succeeds (== ER_OK).
 **/
TEST_F(ManifestUtilTests, UtilIllegalArgs) {
    ASSERT_NE(ER_OK, Util::Init(nullptr));
    Message* msg = nullptr;
    ASSERT_EQ(nullptr, Util::GetDefaultMarshaller(&msg));
    PermissionPolicy pp;

    ASSERT_EQ(ER_OK, Util::Init(ba)); // Needed to create a valid test policy/manifest
    PermissionPolicy::Rule* rules = nullptr;
    size_t count;
    ASSERT_EQ(ER_OK, GenerateManifest(&rules, &count));
    Manifest manifestFromRules(rules, count);
    uint8_t* byteArray = nullptr;
    size_t sizeOfByteArray;
    EXPECT_EQ(ER_OK, manifestFromRules.GetByteArray(&byteArray, &sizeOfByteArray));
    EXPECT_EQ(ER_OK, Util::Fini());

    ASSERT_NE(ER_OK, Util::Init(nullptr));
    EXPECT_NE(ER_OK, Util::GetPolicy(byteArray, sizeOfByteArray, pp));
    delete[] byteArray;
    byteArray = nullptr;
    sizeOfByteArray = 0;
    ASSERT_NE(ER_OK, Util::GetPolicyByteArray(pp, &byteArray, &sizeOfByteArray));
    ASSERT_EQ((size_t)0, sizeOfByteArray);
    ASSERT_EQ(nullptr, byteArray);

    ASSERT_EQ(ER_OK, Util::Init(ba));
    EXPECT_EQ(ER_OK, Util::GetPolicyByteArray(pp, &byteArray, &sizeOfByteArray)); //empty pp should be fine
    EXPECT_NE((size_t)0, sizeOfByteArray);
    EXPECT_NE(nullptr, byteArray);
    delete[] byteArray;
    byteArray = nullptr;
    sizeOfByteArray = 0;

    ASSERT_EQ(nullptr, Util::GetDefaultMarshaller(nullptr));
    ASSERT_NE(ER_OK, Util::GetPolicyByteArray(pp, nullptr, nullptr));
    ASSERT_NE(ER_OK, Util::GetPolicy(nullptr, 0, pp));

    ASSERT_EQ(ER_OK, Util::Fini());
}

/**
 * @test Verify the difference between two manifest is computed correctly.
 *       -# Create two manifests: one basic manifest, and one extending the basic manifest by
 *          adding another interface and by extending the action mask on a specific member.
 *       -# Compute the difference between the extended manifest and the basic manifest, and
 *          check whether the outcome is as expected.
 *       -# Compute the difference between the basic manifest and the extend manifest, and
 *          make sure it is empty.
 */
TEST_F(ManifestUtilTests, Difference) {
    ASSERT_EQ(ER_OK, Util::Init(ba));

    Manifest manifest;
    GetManifest(manifest);
    Manifest permutedManifest;
    GetPermutedManifest(permutedManifest);
    Manifest splitManifest;
    GetSplitManifest(splitManifest);
    Manifest extendedManifest;
    GetExtendedManifest(extendedManifest);

    // compare two identical manifests
    Manifest differenceMM;
    manifest.Difference(manifest, differenceMM);
    ASSERT_EQ((size_t)0, differenceMM.GetRulesSize());

    Manifest differencePP;
    permutedManifest.Difference(permutedManifest, differencePP);
    ASSERT_EQ((size_t)0, differencePP.GetRulesSize());

    Manifest differenceSS;
    splitManifest.Difference(splitManifest, differenceSS);
    ASSERT_EQ((size_t)0, differenceSS.GetRulesSize());

    Manifest differenceEE;
    extendedManifest.Difference(extendedManifest, differenceEE);
    ASSERT_EQ((size_t)0, differenceEE.GetRulesSize());

    // compare permuted manifest
    Manifest differenceMP;
    manifest.Difference(permutedManifest, differenceMP);
    ASSERT_EQ((size_t)0, differenceMP.GetRulesSize());

    Manifest differencePM;
    permutedManifest.Difference(manifest, differencePM);
    ASSERT_EQ((size_t)0, differencePM.GetRulesSize());

    // compare split manifest
    Manifest differenceSM;
    splitManifest.Difference(manifest, differenceSM);
    ASSERT_EQ((size_t)0, differenceSM.GetRulesSize());

    Manifest differenceMS;
    manifest.Difference(splitManifest, differenceMS);
    ASSERT_EQ((size_t)0, differenceMS.GetRulesSize());

    // compare split with permuted manifest
    Manifest differencePS;
    permutedManifest.Difference(splitManifest, differencePS);
    ASSERT_EQ((size_t)0, differencePS.GetRulesSize());

    Manifest differenceSP;
    splitManifest.Difference(permutedManifest, differenceSP);
    ASSERT_EQ((size_t)0, differencePS.GetRulesSize());

    // compare extended manifest
    Manifest differenceEM;
    extendedManifest.Difference(manifest, differenceEM);
    ASSERT_EQ((size_t)2, differenceEM.GetRulesSize());

    Manifest differenceES;
    extendedManifest.Difference(splitManifest, differenceES);
    ASSERT_EQ((size_t)2, differenceES.GetRulesSize());

    Manifest differenceEP;
    extendedManifest.Difference(permutedManifest, differenceEP);
    ASSERT_EQ((size_t)2, differenceEP.GetRulesSize());

    Manifest differenceME;
    manifest.Difference(extendedManifest, differenceME);
    ASSERT_EQ((size_t)0, differenceME.GetRulesSize());

    Manifest differenceSE;
    splitManifest.Difference(extendedManifest, differenceSE);
    ASSERT_EQ((size_t)0, differenceSE.GetRulesSize());

    Manifest differencePE;
    permutedManifest.Difference(permutedManifest, differencePE);
    ASSERT_EQ((size_t)0, differencePE.GetRulesSize());

    ASSERT_EQ(ER_OK, Util::Fini());
}
}
