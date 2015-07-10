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

#include "TestUtil.h"

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
 *          ones passed on during creation.
 *       -# Verify that membershipCert is not a CA.
 *       -# Verify that membershipCert has a non-nullptr encoding.
 **/
TEST_F(CertificateUtilTests, DISABLED_ToMembershipCertificate) {
}

/**
 * @test Verify the creation on identity certificate.
 *       -# Create an Application with a valid full KeyInfoNISTP256 keyinfo.
 *       -# Create a valid GroupInfo groupInfo with a valid guid.
 *       -# Declare a validityPeriod with a valid value.
 *       -# Use ToIdentityCertificate and make sure it succeeds
 *          and returns an identityCert.
 *       -# Verify that the identityCert fields are matching the
 *          ones passed on during creation.
 *       -# Verify that identityCert is not a CA.
 *       -# Verify that identityCert has a digest.
 *       -# Verify that identityCert has a non-nullptr encoding.
 **/
TEST_F(CertificateUtilTests, DISABLED_ToIdentityCertificate) {
}

/**
 * @test Verify the creation on membership and identity certificates
 *       fails if wrong validity is provided.
 *       -# Create an Application with a valid full KeyInfoNISTP256 keyinfo.
 *       -# Create a valid GroupInfo groupInfo with a valid guid.
 *       -# Declare a validityPeriod with a wrong logical value, (e.g.,
 *          the validTo is set to a time before the validFrom.)
 *       -# Use ToMembershipCertificate and make sure it fails (!= ER_OK).
 *       -# Use ToIdentityCertificate and make sure it fails (!= ER_OK).
 **/
TEST_F(CertificateUtilTests, DISABLED_FailedToMembershipAndIdentityCertificate) {
}
}
