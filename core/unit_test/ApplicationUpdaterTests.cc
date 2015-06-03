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

#include <gtest/gtest.h>
#include "TestUtil.h"

#include <qcc/GUID.h>
#include <qcc/String.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/Status.h>

#include <alljoyn/securitymgr/PolicyGenerator.h>

using namespace secmgrcoretest_unit_testutil;

class ApplicationUpdaterTests :
    public ClaimedTest {
  public:
    ApplicationUpdaterTests()
    {
        qcc::GUID128 guildGUID(0xab);
        qcc::String guildName = "Test";
        qcc::String guildDesc = "This is a test guild";

        guildInfo.guid = guildGUID;
        guildInfo.name = guildName;
        guildInfo.desc = guildDesc;

        policyGuilds.push_back(guildGUID);
    }

  public:
    GuildInfo guildInfo;
    PermissionPolicy policy;
    vector<GUID128> policyGuilds;
};

/**
 * \test Reset an offline application and check its claimable state when it
 *       comes back online.
 *       -# Stop remote application.
 *       -# Reset the application using the security manager.
 *       -# Restart the remote application.
 *       -# Check whether the remote application is CLAIMABLE.
 **/

TEST_F(ApplicationUpdaterTests, Reset) {
    // stop the stub
    delete stub;
    stub = NULL;

    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));

    // reset the stub
    ASSERT_EQ(ER_OK, secMgr->Reset(lastAppInfo));
    ApplicationInfo checkUpdatesPendingInfo;
    checkUpdatesPendingInfo.publicKey = lastAppInfo.publicKey;
    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_TRUE(checkUpdatesPendingInfo.updatesPending); // The app was off-line so the reset was not done remotely

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING, false));

    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_FALSE(checkUpdatesPendingInfo.updatesPending);
}

/**
 * \test Install a membership certificate for an offline application and bring
 *       it back online.
 *       -# Stop remote application.
 *       -# Install a membership certificate using the security manager.
 *       -# Restart the remote application.
 **/
TEST_F(ApplicationUpdaterTests, InstallMembership) {
    // stop the stub
    delete stub;
    stub = NULL;
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));

    // change security configuration
    ASSERT_EQ(ER_OK, secMgr->StoreGuild(guildInfo));

    ASSERT_EQ(ER_OK, secMgr->InstallMembership(lastAppInfo, guildInfo));
    ApplicationInfo checkUpdatesPendingInfo;
    checkUpdatesPendingInfo.publicKey = lastAppInfo.publicKey;
    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_TRUE(checkUpdatesPendingInfo.updatesPending); // The app was off-line so the membership cert was not installed remotely

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING, false));

    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_FALSE(checkUpdatesPendingInfo.updatesPending);
}

/**
 * \test Update a policy for an offline application and bring it back online.
 *       -# Stop remote application.
 *       -# Install a policy using the security manager.
 *       -# Restart the remote application.
 **/
TEST_F(ApplicationUpdaterTests, UpdatePolicy) {
    // stop the stub
    delete stub;
    stub = NULL;
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));

    // change security configuration
    ASSERT_EQ(ER_OK, secMgr->StoreGuild(guildInfo));
    vector<GuildInfo> guilds;
    guilds.push_back(guildInfo);
    PolicyGenerator::DefaultPolicy(guilds, policy);
    ASSERT_EQ(ER_OK, secMgr->UpdatePolicy(lastAppInfo, policy));

    ApplicationInfo checkUpdatesPendingInfo;
    checkUpdatesPendingInfo.publicKey = lastAppInfo.publicKey;
    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_TRUE(checkUpdatesPendingInfo.updatesPending); // The app was off-line so the policy was not installed remotely

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING, false));

    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_FALSE(checkUpdatesPendingInfo.updatesPending);
}

/**
 * \test Install an identity certificate for an offline application and bring
 *       it back online.
 *       -# Stop remote application.
 *       -# Install an identity certificate using the security manager.
 *       -# Restart the remote application.
 **/
// TODO : Investigate AS-1488 to re-enable the test
TEST_F(ApplicationUpdaterTests, DISABLED_InstallIdentity) {
    // stop the stub
    delete stub;
    stub = NULL;
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));

    // change security configuration
    IdentityInfo identityInfo2;
    identityInfo2.name = "Updated test name";
    ASSERT_EQ(ER_OK, secMgr->StoreIdentity(identityInfo2));
    ASSERT_EQ(ER_OK, secMgr->UpdateIdentity(lastAppInfo, identityInfo2));

    ApplicationInfo checkUpdatesPendingInfo;
    checkUpdatesPendingInfo.publicKey = lastAppInfo.publicKey;
    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_TRUE(checkUpdatesPendingInfo.updatesPending); // The app was off-line so the identity was not installed remotely

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING, false));

    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_FALSE(checkUpdatesPendingInfo.updatesPending);
}

/**
 * \test Change the complete security configuration of an offline application
 *       and bring it back online.
 *       -# Stop remote application.
 *       -# Install a membership certificate using the security manager.
 *       -# Install a policy using the security manager.
 *       -# Install an identity certificate using the security manager.
 *       -# Restart the remote application.
 *       -# Wait for the updates to complete.
 *       -# Stop the remote application again.
 *       -# Reset the remote application using the security manager.
 *       -# Restart the remote application.
 *       -# Check whether the remote application is CLAIMABLE.
 **/
TEST_F(ApplicationUpdaterTests, UpdateAll) {
    // stop the stub
    delete stub;
    stub = NULL;
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));

    // change security configuration
    ASSERT_EQ(ER_OK, secMgr->StoreGuild(guildInfo));
    ASSERT_EQ(ER_OK, secMgr->InstallMembership(lastAppInfo, guildInfo));
    vector<GuildInfo> guilds;
    guilds.push_back(guildInfo);
    PolicyGenerator::DefaultPolicy(guilds, policy);
    ASSERT_EQ(ER_OK, secMgr->UpdatePolicy(lastAppInfo, policy));
    IdentityInfo identityInfo2;
    identityInfo2.name = "Updated test name";
    ASSERT_EQ(ER_OK, secMgr->StoreIdentity(identityInfo2));
    ASSERT_EQ(ER_OK, secMgr->UpdateIdentity(lastAppInfo, identityInfo2));

    ApplicationInfo checkUpdatesPendingInfo;
    checkUpdatesPendingInfo.publicKey = lastAppInfo.publicKey;
    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_TRUE(checkUpdatesPendingInfo.updatesPending); // The app was off-line so the update was not done remotely

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_RUNNING, false));

    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_FALSE(checkUpdatesPendingInfo.updatesPending);

    // stop the stub
    delete stub;
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMED, ajn::securitymgr::STATE_NOT_RUNNING));

    // reset the stub
    ASSERT_EQ(ER_OK, secMgr->Reset(lastAppInfo));

    // restart the stub
    stub = new Stub(tcl, true);
    ASSERT_TRUE(WaitForState(ajn::PermissionConfigurator::STATE_CLAIMABLE, ajn::securitymgr::STATE_RUNNING, false));

    ASSERT_EQ(ER_OK, secMgr->GetApplication(checkUpdatesPendingInfo));
    ASSERT_FALSE(checkUpdatesPendingInfo.updatesPending);
}
