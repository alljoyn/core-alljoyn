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

#include "TestUtil.h"
#include <alljoyn/securitymgr/CertificateUtil.h>
#include <qcc/CertificateECC.h>

using namespace ajn;
using namespace ajn::securitymgr;

/** @file CertificateUtilTests.cc */

namespace secmgr_tests {
class CertificateUtilTests :
    public BasicTest {
  private:

  protected:

  public:
    CertificateUtilTests()
    {
    }
};

/**
 * @test Verify the creation on membership certificate.
 *       -# Create an Application with a valid full KeyInfoNISTP256 keyinfo.
 *       -# Create a valid GroupInfo groupInfo with a valid guid.
 *       -# Declare a validityPeriod with a valid value.
 *       -# Use ToMembershipCertificate and make sure it succeeds
 *          and returns a membershipCert.
 *       -# Verify that the membershipCert fields are matching the
 *          ones passed on during creation and that it is not a CA.
 *       -# Verify that membershipCert has a nullptr encoding.
 **/
TEST_F(CertificateUtilTests, ToMembershipCertificate) {
    Application app;
    Crypto_ECC ecc;
    ASSERT_EQ(ER_OK, ecc.GenerateDSAKeyPair());
    app.keyInfo.SetPublicKey(ecc.GetDSAPublicKey());

    GroupInfo groupInfo;
    MembershipCertificate membership;
    uint64_t validityPeriod = 3600 * 24 * 10 * 365;
    ASSERT_EQ(ER_OK, CertificateUtil::ToMembershipCertificate(app, groupInfo, validityPeriod, membership));

    ASSERT_EQ(groupInfo.guid, membership.GetGuild());
    ASSERT_EQ(*(app.keyInfo.GetPublicKey()), *(membership.GetSubjectPublicKey()));
    ASSERT_FALSE(membership.IsCA());
    ASSERT_FALSE(nullptr == membership.GetSubjectCN());
    ASSERT_NE((size_t)0, membership.GetSubjectCNLength());

    const qcc::CertificateX509::ValidPeriod* validity = membership.GetValidity();
    ASSERT_EQ(validity->validTo, (validity->validFrom) + validityPeriod + 3600); // 3600 for drift correction

    uint64_t currentTime = GetEpochTimestamp() / 1000;
    ASSERT_TRUE(validity->validTo >= currentTime);
    ASSERT_TRUE(validity->validFrom <= currentTime);
    qcc::String der;
    ASSERT_EQ(ER_FAIL, membership.EncodeCertificateDER(der));
}

/**
 * @test Verify the creation on identity certificate.
 *       -# Create an Application with a valid full KeyInfoNISTP256 keyinfo.
 *       -# Create a valid IdentityInfo identityInfo with a valid guid.
 *       -# Declare a validityPeriod with a valid value.
 *       -# Use ToIdentityCertificate and make sure it succeeds
 *          and returns an identityCert.
 *       -# Verify that the identityCert fields are matching the
 *          ones passed on during creation.
 *       -# Verify that identityCert is not a CA.
 *       -# Verify that identityCert has no digest.
 *       -# Verify that identityCert has a nullptr encoding.
 **/
TEST_F(CertificateUtilTests, ToIdentityCertificate) {
    Application app;
    Crypto_ECC ecc;
    ASSERT_EQ(ER_OK, ecc.GenerateDSAKeyPair());
    app.keyInfo.SetPublicKey(ecc.GetDSAPublicKey());

    IdentityInfo identityInfo;
    IdentityCertificate identity;
    identityInfo.name = "My Identity";
    uint64_t validityPeriod = 3600 * 24 * 10 * 365;
    ASSERT_EQ(ER_OK, CertificateUtil::ToIdentityCertificate(app, identityInfo, validityPeriod, identity));

    ASSERT_EQ(identityInfo.guid.ToString(), identity.GetAlias());
    ASSERT_EQ(*(app.keyInfo.GetPublicKey()), *(identity.GetSubjectPublicKey()));
    ASSERT_EQ(identityInfo.name, string((const char*)identity.GetSubjectOU(), identity.GetSubjectOULength()));
    ASSERT_EQ(identityInfo.name.size(), identity.GetSubjectOULength());
    ASSERT_FALSE(identity.IsCA());
    ASSERT_FALSE(nullptr == identity.GetSubjectCN());
    ASSERT_NE((size_t)0, identity.GetSubjectCNLength());

    const qcc::CertificateX509::ValidPeriod* validity = identity.GetValidity();
    ASSERT_EQ(validity->validTo, (validity->validFrom) + validityPeriod + 3600); // 3600 for drift correction
    uint64_t currentTime = GetEpochTimestamp() / 1000;
    ASSERT_TRUE(validity->validTo >= currentTime);
    ASSERT_TRUE(validity->validFrom <= currentTime);

    ASSERT_EQ(size_t(0), identity.GetDigestSize());
    qcc::String der;
    ASSERT_EQ(ER_FAIL, identity.EncodeCertificateDER(der));
}

/**
 * @test Verify the creation on membership and identity certificates
 *       fails if wrong validity is provided.
 *       -# Create an Application.
 *       -# Create a GroupInfo groupInfo and IdentityInfo identityInfo.
 *       -# Use ToMembershipCertificate with zero validity period and make sure it fails (!= ER_OK).
 *       -# Use ToIdentityCertificate zero validity period and make sure it fails (!= ER_OK).
 **/
TEST_F(CertificateUtilTests, FailedToMembershipAndIdentityCertificate) {
    Application app;

    IdentityInfo identityInfo;
    IdentityCertificate identity;

    GroupInfo groupInfo;
    MembershipCertificate membership;

    ASSERT_EQ(ER_FAIL, CertificateUtil::ToIdentityCertificate(app, identityInfo, 0, identity));
    ASSERT_EQ(ER_FAIL, CertificateUtil::ToMembershipCertificate(app, groupInfo, 0, membership));
}
}
