/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#include "Common.h"
#include <qcc/Util.h>

using namespace ajn::securitymgr;
using namespace std;

namespace secmgrcoretest_unit_testutil {
TestApplicationListener::TestApplicationListener(sem_t& _sem,
                                                 sem_t& _lock) :
    sem(_sem), lock(_lock)
{
}

void TestApplicationListener::OnApplicationStateChange(const ApplicationInfo* old,
                                                       const ApplicationInfo* updated)
{
#if 1
    cout << "  Application updated:" << endl;
    cout << "  ====================" << endl;
    cout << "  Application name :" << updated->appName << endl;
    cout << "  Hostname            :" << updated->deviceName << endl;
    cout << "  Busname            :" << updated->busName << endl;
    cout << "  - claim state     :" << ToString(old->claimState) << " --> " <<
    ToString(updated->claimState) <<
    endl;
    cout << "  - running state     :" << ToString(old->runningState) << " --> " <<
    ToString(updated->runningState) <<
    endl << "> " << flush;
#endif
    sem_wait(&lock);
    _lastAppInfo = *updated;
    sem_post(&sem);
    sem_post(&lock);
}

BasicTest::BasicTest()
{
    secMgr = NULL;
    tal = NULL;
    ba = NULL;
}

static int counter;

void BasicTest::SetUp()
{
    const char* storage_path = NULL;
    if ((storage_path = getenv("STORAGE_PATH")) == NULL) {
        storage_path = "/tmp/secmgr.db";
    }
    qcc::String path = storage_path;

    stringstream tmp;
    tmp << (++counter);
    path += tmp.str().c_str();
    remove(path.c_str());

    // clean up any lingering stub keystore
    qcc::String fname = GetHomeDir();
    fname.append("/.alljoyn_keystore/stub.ks");
    remove(fname.c_str());

    ajn::securitymgr::SecurityManagerFactory& secFac = ajn::securitymgr::SecurityManagerFactory::GetInstance();
    ba = new BusAttachment("test", true);
    ASSERT_TRUE(ba != NULL);
    ASSERT_EQ(ER_OK, ba->Start());
    ASSERT_EQ(ER_OK, ba->Connect());

    sc.settings["STORAGE_PATH"] = path;
    ASSERT_EQ(sc.settings.at("STORAGE_PATH").compare(path.c_str()), 0);
    secMgr = secFac.GetSecurityManager(sc, smc, NULL, ba);
    ASSERT_TRUE(secMgr != NULL);

    sem_init(&sem, 0, 0);
    sem_init(&lock, 0, 1);

    tal = new TestApplicationListener(sem, lock);
    secMgr->RegisterApplicationListener(tal);
}

void BasicTest::UpdateLastAppInfo()
{
    sem_wait(&lock);
    lastAppInfo = tal->_lastAppInfo;
    sem_post(&lock);
}

bool BasicTest::WaitForState(ajn::PermissionConfigurator::ClaimableState newState,
                             ajn::securitymgr::ApplicationRunningState newRunningState)
{
    printf("\nWaitForState: waiting for event(s) ...\n");
    //Prior to entering this function, the test should have taken an action which leads to one or more events.
    //These events are handled in a separate thread.
    timespec timeout;
    do {
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 5; //Wait for a maximum of 5 seconds ...
        if (sem_timedwait(&sem, &timeout) == 0) { //if events are in the queue, we will return immediately
            UpdateLastAppInfo(); //update latest value.
            printf("WaitForState: Checking event ... ");
            if (lastAppInfo.claimState == newState && newRunningState == lastAppInfo.runningState &&
                lastAppInfo.appName.size() != 0) {
                printf("ok\n");
                return true;
            }
        } else {
            printf("timeout- failing test\n");
            break;
        }
        printf("not ok, waiting for next event\n");
    } while (true);
    printf("WaitForState failed.\n");
    printf("\tClaimableState: expected = %s, got %s\n", ToString(newState), ToString(lastAppInfo.claimState));
    printf("\tRunningState: expected = %s, got %s\n", ToString(newRunningState), ToString(lastAppInfo.runningState));
    printf("\tApplicationName = '%s' (size = %lu)\n", lastAppInfo.appName.c_str(), lastAppInfo.appName.size());
    return false;
}

void BasicTest::TearDown()
{
    if (tal) {
        secMgr->UnregisterApplicationListener(tal);
        delete tal;
        tal = NULL;
    }

    sem_destroy(&sem);

    if (ba) {
        ba->Disconnect();
        ba->Stop();
        ba->Join();
    }
    delete secMgr;
    delete ba;
}

void BasicTest::SetSmcStub()
{
    smc.pmNotificationIfn = "org.allseen.Security.PermissionMgmt.Stub.Notification";
    smc.pmIfn = "org.allseen.Security.PermissionMgmt.Stub";
    smc.pmObjectPath = "/security/PermissionMgmt";
}
}
