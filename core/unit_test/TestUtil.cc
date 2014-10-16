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

using namespace ajn::securitymgr;
using namespace std;

namespace secmgrcoretest_unit_testutil {
TestApplicationListener::TestApplicationListener(sem_t& _sem) :
    sem(_sem)
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

    _lastAppInfo = *updated;
    sem_post(&sem);
}

BasicTest::BasicTest()
{
    secMgr = NULL;
}

static int counter;

void BasicTest::SetUp()
{
    const char* storage_path = NULL;
    if ((storage_path = getenv("STORAGE_PATH")) == NULL) {
        storage_path = "/tmp/secmgr.db";
    }
    qcc::String path = storage_path;
    path += std::to_string(counter++).c_str();
    remove(path.c_str());
    ajn::securitymgr::SecurityManagerFactory& secFac = ajn::securitymgr::SecurityManagerFactory::GetInstance();
    ba = new BusAttachment("test", true);
    ASSERT_TRUE(ba != NULL);
    ASSERT_EQ(ER_OK, ba->Start());
    ASSERT_EQ(ER_OK, ba->Connect());

    sc.settings["STORAGE_PATH"] = path;
    ASSERT_EQ(sc.settings.at("STORAGE_PATH").compare(path.c_str()), 0);
    secMgr = secFac.GetSecurityManager("hello", "world", sc, NULL, ba);
    ASSERT_TRUE(secMgr != NULL);
}

void BasicTest::TearDown()
{
    if (ba) {
        ba->Disconnect();
        ba->Stop();
        ba->Join();
    }
    delete secMgr;
    delete ba;
}
}
