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
#include <alljoyn/securitymgr/CertificateUtil.h>
#include <qcc/CertificateECC.h>

using namespace ajn;
using namespace ajn::securitymgr;

/** @file ClaimContextTests.cc */

namespace secmgr_tests {
class TestClaimContext :
    public ClaimContext {
  public:
    TestClaimContext(const OnlineApplication& _app,
                     const Manifest& _mnf,
                     const PermissionConfigurator::ClaimCapabilities _capabilities,
                     const PermissionConfigurator::ClaimCapabilityAdditionalInfo _capInfo) :
        ClaimContext(_app,
                     _mnf,
                     _capabilities,
                     _capInfo)
    {
    }

    TestClaimContext(const TestClaimContext& other, const PermissionConfigurator::ClaimCapabilities _capabilities) :
        ClaimContext(other.GetApplication(), other.GetManifest(), _capabilities, other.GetClaimCapabilityInfo())
    {
    }

    QStatus SetPreSharedKey(const uint8_t* psk,
                            size_t pskSize)
    {
        QCC_UNUSED(psk);
        QCC_UNUSED(pskSize);
        return ER_OK;
    }
};

class ClaimContextTests :
    public BasicTest {
  private:

  protected:

  public:
    ClaimContextTests()
    {
    }
};

/**
 * @test Verify the construction of a ClaimContext and its getters.
 *       -# Create an ClaimContext object
 *       -# Call the Get functions and check the return values
 **/
TEST_F(ClaimContextTests, BasicConstructor) {
    OnlineApplication app;
    Manifest mnf;
    PermissionConfigurator::ClaimCapabilities caps = 0x1234;
    PermissionConfigurator::ClaimCapabilityAdditionalInfo info = 0x4321;

    TestClaimContext ctx(app, mnf, caps, info);

    ASSERT_EQ(app, ctx.GetApplication());
    ASSERT_EQ(mnf, ctx.GetManifest());
    ASSERT_EQ(caps, ctx.GetClaimCapabilities());
    ASSERT_EQ(info, ctx.GetClaimCapabilityInfo());

    ASSERT_FALSE(ctx.IsManifestApproved());
    ASSERT_EQ(ClaimContext::CLAIM_TYPE_NOT_SET, ctx.GetClaimType());
}

/**
 * @test Verify the ApproveManifest function is working as expected.
 *       -# Create an ClaimContext object
 *       -# Call the ApproveManifest function with different values and validate the result
 **/
TEST_F(ClaimContextTests, ApproveManifest) {
    OnlineApplication app;
    Manifest mnf;
    PermissionConfigurator::ClaimCapabilities caps = 0x1234;
    PermissionConfigurator::ClaimCapabilityAdditionalInfo info = 0x4321;

    TestClaimContext ctx(app, mnf, caps, info);

    ASSERT_FALSE(ctx.IsManifestApproved());
    ctx.ApproveManifest();
    ASSERT_TRUE(ctx.IsManifestApproved());
    ctx.ApproveManifest(false);
    ASSERT_FALSE(ctx.IsManifestApproved());
    ctx.ApproveManifest(true);
    ASSERT_TRUE(ctx.IsManifestApproved());
    ctx.ApproveManifest(false);
    ASSERT_FALSE(ctx.IsManifestApproved());
}

/**
 * @test Verify the SetClaimType function is working as expected.
 *       -# Create an ClaimContext object
 *       -# Call the SetClaimType function with different values and validate the results
 **/
TEST_F(ClaimContextTests, SetClaimType) {
    OnlineApplication app;
    Manifest mnf;
    PermissionConfigurator::ClaimCapabilities caps = 0x1238;
    PermissionConfigurator::ClaimCapabilityAdditionalInfo info = 0x4321;

    TestClaimContext ctx(app, mnf, caps, info);

    ASSERT_EQ(ClaimContext::CLAIM_TYPE_NOT_SET, ctx.GetClaimType());
    ASSERT_EQ(ER_BAD_ARG_1, ctx.SetClaimType(0x08));
    ASSERT_EQ(ER_BAD_ARG_1, ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_ECDSA));
    ASSERT_EQ(ER_BAD_ARG_1, ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_PSK));
    ASSERT_EQ(ER_BAD_ARG_1, ctx.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_NULL));
    ASSERT_EQ(ClaimContext::CLAIM_TYPE_NOT_SET, ctx.GetClaimType());

    TestClaimContext ctx2(ctx, PermissionConfigurator::CAPABLE_ECDHE_PSK | PermissionConfigurator::CAPABLE_ECDHE_NULL);
    ASSERT_EQ(ER_OK, ctx2.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_PSK));
    ASSERT_EQ(PermissionConfigurator::CAPABLE_ECDHE_PSK, ctx2.GetClaimType());
    ASSERT_EQ(ER_OK, ctx2.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_NULL));
    ASSERT_EQ(PermissionConfigurator::CAPABLE_ECDHE_NULL, ctx2.GetClaimType());
    ASSERT_EQ(ER_BAD_ARG_1, ctx2.SetClaimType(0x08));
    ASSERT_EQ(ER_BAD_ARG_1, ctx2.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_ECDSA));
    ASSERT_EQ(PermissionConfigurator::CAPABLE_ECDHE_NULL, ctx2.GetClaimType());
    ASSERT_EQ(ER_BAD_ARG_1,
              ctx2.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_PSK &
                                PermissionConfigurator::CAPABLE_ECDHE_NULL));
    ASSERT_EQ(PermissionConfigurator::CAPABLE_ECDHE_NULL, ctx2.GetClaimType());
    ASSERT_EQ(ER_OK, ctx2.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_PSK));
    ASSERT_EQ(PermissionConfigurator::CAPABLE_ECDHE_PSK, ctx2.GetClaimType());

    TestClaimContext ctx3(ctx, PermissionConfigurator::CAPABLE_ECDHE_ECDSA);
    ASSERT_EQ(ER_BAD_ARG_1, ctx3.SetClaimType(0x08));
    ASSERT_EQ(ER_BAD_ARG_1, ctx3.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_PSK));
    ASSERT_EQ(ER_BAD_ARG_1, ctx3.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_NULL));
    ASSERT_EQ(ClaimContext::CLAIM_TYPE_NOT_SET, ctx3.GetClaimType());
    ASSERT_EQ(ER_OK, ctx3.SetClaimType(PermissionConfigurator::CAPABLE_ECDHE_ECDSA));
    ASSERT_EQ(PermissionConfigurator::CAPABLE_ECDHE_ECDSA, ctx3.GetClaimType());
}
}
