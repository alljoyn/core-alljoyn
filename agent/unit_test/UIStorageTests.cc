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

using namespace ajn::securitymgr;

/** @file UIStorageTests.cc */

namespace secmgr_tests {
class UIStorageTests :
    public SecurityAgentTest {
  private:

  protected:

  public:
    UIStorageTests()
    {
    }
};

/**
 * @test Set the user defined name of an application and check whether it can
 *       be retrieved.
 *       -# Claim the remote application.
 *       -# Set some meta data.
 *       -# Retrieve the application from the security agent.
 *       -# Check whether the retrieved meta data matches the data that was set.
 **/
TEST_F(UIStorageTests, SetMetaData) {
    TestApplication testApp;
    ASSERT_EQ(ER_OK, testApp.Start());

    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMABLE, true));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    ApplicationMetaData appMetaData;
    ASSERT_EQ(ER_END_OF_DATA, storage->SetAppMetaData(lastAppInfo, appMetaData));
    ASSERT_EQ(ER_END_OF_DATA, storage->GetAppMetaData(lastAppInfo, appMetaData));

    ASSERT_EQ(ER_OK, secMgr->Claim(lastAppInfo, idInfo));
    ASSERT_TRUE(WaitForState(PermissionConfigurator::CLAIMED, true));
    ASSERT_TRUE(CheckIdentity(idInfo, aa.lastManifest));

    string userDefinedName = "User-defined test name";
    string deviceName = "Device test name";
    string appName = "Application test name";

    appMetaData.userDefinedName  = userDefinedName;
    appMetaData.deviceName  = deviceName;
    appMetaData.appName  = appName;

    ASSERT_EQ(ER_OK, storage->SetAppMetaData(lastAppInfo, appMetaData));

    OnlineApplication app;
    app.busName = lastAppInfo.busName;
    ASSERT_EQ(ER_END_OF_DATA, secMgr->GetApplication(app));
    app.keyInfo = lastAppInfo.keyInfo;
    ASSERT_EQ(ER_OK, secMgr->GetApplication(app));
    Application mAppInfo;
    mAppInfo.keyInfo = lastAppInfo.keyInfo;
    ASSERT_EQ(ER_OK, storage->GetManagedApplication(mAppInfo));

    appMetaData.userDefinedName = "";
    appMetaData.deviceName = "";
    appMetaData.appName = "";

    ASSERT_EQ(ER_OK, storage->GetAppMetaData(mAppInfo, appMetaData));
    ASSERT_TRUE(userDefinedName == appMetaData.userDefinedName);
    ASSERT_TRUE(deviceName == appMetaData.deviceName);
    ASSERT_TRUE(appName == appMetaData.appName);

    ApplicationMetaData emptyAppMetaData;
    ASSERT_EQ(ER_OK, storage->SetAppMetaData(mAppInfo, emptyAppMetaData));
    ASSERT_EQ(ER_OK, storage->GetAppMetaData(mAppInfo, appMetaData));
    ASSERT_TRUE(appMetaData == emptyAppMetaData);
}
}
