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
#include <qcc/Util.h>
#include <stdlib.h>

using namespace ajn::securitymgr;
using namespace std;

namespace secmgrcoretest_unit_testutil {
TestApplicationListener::TestApplicationListener(qcc::Condition& _sem,
                                                 qcc::Mutex& _lock) :
    event(false), sem(_sem), lock(_lock)
{
}

void TestApplicationListener::OnApplicationStateChange(const ApplicationInfo* old,
                                                       const ApplicationInfo* updated)
{
    const ApplicationInfo* info = updated ? updated : old;
    ApplicationListener::PrintStateChangeEvent(old, updated);
    lock.Lock();
    _lastAppInfo = *info;
    event = true;
    sem.Broadcast();
    lock.Unlock();
}

BasicTest::BasicTest()
{
    secMgr = NULL;
    tal = NULL;
    ba = NULL;
}

void BasicTest::SetUp()
{
    const char* storage_path = NULL;
    if ((storage_path = getenv("STORAGE_PATH")) == NULL) {
        storage_path = "/tmp/secmgr.db";
        ASSERT_EQ(0, setenv("STORAGE_PATH", storage_path, 1));
    }

    remove(storage_path);
    // clean up any lingering stub keystore
    qcc::String fname = GetHomeDir();
    fname.append("/.alljoyn_keystore/stub.ks");
    remove(fname.c_str());

    SecurityManagerFactory& secFac = SecurityManagerFactory::GetInstance();
    SQLStorageFactory& storageFac = SQLStorageFactory::GetInstance();

    ba = new BusAttachment("test", true);
    ASSERT_TRUE(ba != NULL);
    ASSERT_EQ(ER_OK, ba->Start());
    ASSERT_EQ(ER_OK, ba->Connect());

    storage = storageFac.GetStorage();
    secMgr = secFac.GetSecurityManager(storage, ba);
    ASSERT_TRUE(secMgr != NULL);

    secMgr->SetManifestListener(&aa);

    tal = new TestApplicationListener(sem, lock);
    secMgr->RegisterApplicationListener(tal);
}

void BasicTest::UpdateLastAppInfo()
{
    lock.Lock();
    lastAppInfo = tal->_lastAppInfo;
    lock.Unlock();
}

bool BasicTest::WaitForState(ajn::PermissionConfigurator::ClaimableState newState,
                             ajn::securitymgr::ApplicationRunningState newRunningState)
{
    lock.Lock();
    printf("\nWaitForState: waiting for event(s) ...\n");
    //Prior to entering this function, the test should have taken an action which leads to one or more events.
    //These events are handled in a separate thread.
    do {
        if (tal->event) { //if event is in the queue, we will return immediately
            tal->event = false;
            UpdateLastAppInfo(); //update latest value.
            printf("WaitForState: Checking event ... ");
            if (lastAppInfo.claimState == newState && newRunningState == lastAppInfo.runningState &&
                lastAppInfo.appName.size() != 0) {
                printf("ok\n");
                lock.Unlock();
                return true;
            }
        }
        QStatus status = sem.TimedWait(lock, 5000);
        if (ER_OK != status) {
            printf("timeout- failing test - %i\n", status);
            break;
        }
        assert(tal->event); // asume TimedWait returns != ER_OK in case of timeout
        printf("not ok, waiting for next event\n");
    } while (true);
    printf("WaitForState failed.\n");
    printf("\tClaimableState: expected = %s, got %s\n", ToString(newState),
           ToString(lastAppInfo.claimState));
    printf("\tRunningState: expected = %s, got %s\n", ToString(
               newRunningState), ToString(lastAppInfo.runningState));
    printf("\tApplicationName = '%s' (size = %lu)\n", lastAppInfo.appName.c_str(), lastAppInfo.appName.size());
    lock.Unlock();
    return false;
}

void BasicTest::TearDown()
{
    if (tal) {
        secMgr->UnregisterApplicationListener(tal);
        delete tal;
        tal = NULL;
    }
    delete secMgr;

    if (ba) {
        ba->Disconnect();
        ba->Stop();
        ba->Join();
    }
    delete ba;
    storage->Reset();
    delete storage;
    storage = NULL;
    ba = NULL;
    secMgr = NULL;
    tal = NULL;
}
}
