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

using namespace ajn::securitymgr;

/** @file UIStorageTests.cc */

namespace secmgr_tests {
class StorageListenerReset :
    public StorageListener {
  public:
    StorageListenerReset() : storageReset(false) { }

    void OnPendingChanges(vector<Application>& apps) { QCC_UNUSED(apps); }

    void OnPendingChangesCompleted(vector<Application>& apps) { QCC_UNUSED(apps); }

    void OnStorageReset()
    {
        storageReset = true;
    }

    bool WaitForStorageReset()
    {
        QStatus status;
        lock.Lock();
        while (!storageReset) {
            status = sem.TimedWait(lock, 10000);
            if (ER_OK != status) {
                printf("Timeout- failing test - %i\n", status);
                lock.Unlock();
                return false;
            }
        }
        lock.Unlock();
        return true;
    }

  public:
    bool storageReset;

  private:
    qcc::Mutex lock;
    qcc::Condition sem;
};

class UIStorageTests :
    public SecurityAgentTest {
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
    OnlineApplication app;
    ASSERT_EQ(ER_OK, GetPublicKey(testApp, app));

    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMABLE));

    IdentityInfo idInfo;
    idInfo.guid = GUID128();
    idInfo.name = "TestIdentity";
    ASSERT_EQ(storage->StoreIdentity(idInfo), ER_OK);

    ApplicationMetaData appMetaData;
    ASSERT_EQ(ER_END_OF_DATA, storage->SetAppMetaData(app, appMetaData));
    ASSERT_EQ(ER_END_OF_DATA, storage->GetAppMetaData(app, appMetaData));

    ASSERT_EQ(ER_OK, secMgr->Claim(app, idInfo));
    ASSERT_TRUE(WaitForState(app, PermissionConfigurator::CLAIMED));
    ASSERT_TRUE(CheckIdentity(app, idInfo, aa.lastManifest));

    string userDefinedName = "User-defined test name";
    string deviceName = "Device test name";
    string appName = "Application test name";

    appMetaData.userDefinedName  = userDefinedName;
    appMetaData.deviceName  = deviceName;
    appMetaData.appName  = appName;

    ASSERT_EQ(ER_OK, storage->SetAppMetaData(app, appMetaData));

    OnlineApplication newapp;
    newapp.busName = newapp.busName;
    ASSERT_EQ(ER_END_OF_DATA, secMgr->GetApplication(newapp));
    newapp.keyInfo = app.keyInfo;
    ASSERT_EQ(ER_OK, secMgr->GetApplication(newapp));
    Application mAppInfo;
    mAppInfo.keyInfo = app.keyInfo;
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

/**
 * @test Ensure that resetting the database will trigger OnStorageReset.
 *       -# Register a storage listener.
 *       -# Reset the storage and make sure that OnStorageReset was called.
 *       -# Unregister the listener.
 **/
TEST_F(UIStorageTests, StorageReset) {
    StorageListenerReset listener;
    GetAgentCAStorage()->RegisterStorageListener(&listener);
    storage->Reset();
    EXPECT_TRUE(listener.WaitForStorageReset());
    GetAgentCAStorage()->UnRegisterStorageListener(&listener);
}
}
