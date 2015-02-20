/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include "gtest/gtest.h"

#include <alljoyn/securitymgr/PolicyGenerator.h>
#include <alljoyn/PermissionPolicy.h>

/**
 * Several nominal tests for policies.
 */
namespace secmgrcoretest_unit_nominaltests {
using namespace secmgrcoretest_unit_testutil;

using namespace ajn::securitymgr;
using namespace std;
using namespace qcc;

class PolicyTest :
    public ClaimedTest {
  public:
    PolicyTest()
    {
    }
};

/**
 * \test The test should verify that after a device is claimed:
 *       -# A policy can be installed on it.
 *       -# A policy can be retrieved from it.
 * */
TEST_F(PolicyTest, DISABLED_SuccessfulPolicy) {
    GUID128 guildGUID1("B509480EE75397473B5A000B82A7E37E");
    GUID128 guildGUID2("0A716F627F53F91E62835CF3F6C7CD87");

    PermissionPolicy policy1;
    vector<GuildInfo> policy1Guilds;
    GuildInfo guild1;
    guild1.guid = guildGUID1;
    policy1Guilds.push_back(guild1);
    ASSERT_EQ(ER_OK, PolicyGenerator::DefaultPolicy(policy1Guilds, policy1));

    PermissionPolicy policy2;
    vector<GuildInfo> policy2Guilds;
    GuildInfo guild2;
    guild2.guid = guildGUID2;
    policy2Guilds.push_back(guild1);
    policy2Guilds.push_back(guild2);
    ASSERT_EQ(ER_OK, PolicyGenerator::DefaultPolicy(policy2Guilds, policy2));

    // PermissionPolicy policyRemote;
    PermissionPolicy policyLocal;
    ASSERT_EQ(ER_END_OF_DATA, secMgr->GetPolicy(lastAppInfo, policyLocal));
    ASSERT_EQ(ER_OK, secMgr->UpdatePolicy(lastAppInfo, policy1));
    ASSERT_EQ(ER_OK, secMgr->GetPolicy(lastAppInfo, policyLocal));
    ASSERT_EQ((size_t)1, policyLocal.GetTermsSize());

    ASSERT_EQ(ER_OK, secMgr->UpdatePolicy(lastAppInfo, policy2));
    ASSERT_EQ(ER_OK, secMgr->GetPolicy(lastAppInfo, policyLocal));
    ASSERT_EQ((size_t)2, policyLocal.GetTermsSize());

    ASSERT_EQ(ER_OK, secMgr->UpdatePolicy(lastAppInfo, policy1));
    ASSERT_EQ(ER_OK, secMgr->GetPolicy(lastAppInfo, policyLocal));
    ASSERT_EQ((size_t)1, policyLocal.GetTermsSize());
}

TEST_F(PolicyTest, DISABLED_InvalidArgsPolicy)
{
    GUID128 guildGUID1("B509480EE75397473B5A000B82A7E37E");

    PermissionPolicy policy1;
    vector<GuildInfo> policy1Guilds;
    GuildInfo guild1;
    guild1.guid = guildGUID1;
    policy1Guilds.push_back(guild1);
    ASSERT_EQ(ER_OK, PolicyGenerator::DefaultPolicy(policy1Guilds, policy1));

    ApplicationInfo invalid = lastAppInfo;
    qcc::Crypto_ECC ecc;
    ecc.GenerateDSAKeyPair();
    invalid.publicKey = *ecc.GetDSAPublicKey();
    ASSERT_NE(ER_OK, secMgr->UpdatePolicy(invalid, policy1));

    destroy();
    PermissionPolicy policy;

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));

    ASSERT_EQ(ER_OK, secMgr->UpdatePolicy(lastAppInfo, policy1));
}
}
//namespace
