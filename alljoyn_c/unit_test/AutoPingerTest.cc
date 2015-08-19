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
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/AutoPinger.h>
#include <qcc/Thread.h>
#include <gtest/gtest.h>

#include "ajTestCommon.h"

namespace test_unit_autopingerC {

using namespace ajn;

class AutoPingerTest : public testing::Test {
  public:
    alljoyn_busattachment serviceBus;
    alljoyn_autopinger autoPinger;

    AutoPingerTest() :
        serviceBus(alljoyn_busattachment_create("AutoPingerTest", false)),
        autoPinger(alljoyn_autopinger_create(serviceBus))
    {
    }

    virtual void SetUp()
    {
        QStatus status = ER_OK;
        status = alljoyn_busattachment_start(serviceBus);
        ASSERT_EQ(ER_OK, status);
        ASSERT_FALSE(alljoyn_busattachment_isconnected(serviceBus));
        status = alljoyn_busattachment_connect(serviceBus, NULL);
        ASSERT_EQ(ER_OK, status);
        ASSERT_TRUE(alljoyn_busattachment_isconnected(serviceBus));
    }

    virtual void TearDown()
    {
        alljoyn_busattachment_disconnect(serviceBus, NULL);
        alljoyn_busattachment_stop(serviceBus);
        alljoyn_busattachment_join(serviceBus);
        alljoyn_autopinger_destroy(autoPinger);
        alljoyn_busattachment_destroy(serviceBus);
    }

};

#define MAX_RETRIES 1000

class TestPingListener {

  private:
    std::set<qcc::String> found;
    qcc::Mutex foundmutex;
    std::set<qcc::String> lost;
    qcc::Mutex lostmutex;

  public:

    TestPingListener()
    {
    }

    void DestinationLost(const qcc::String& group, const qcc::String& destination)
    {
        QCC_UNUSED(group);

        std::cout << "on lost " << destination << std::endl;
        lostmutex.Lock();
        lost.insert(destination);
        lostmutex.Unlock();

    }

    void DestinationFound(const qcc::String& group, const qcc::String& destination)
    {
        QCC_UNUSED(group);

        std::cout << "on found " << destination << std::endl;
        foundmutex.Lock();
        found.insert(destination);
        foundmutex.Unlock();

    }

    void WaitUntilFound(const qcc::String& destination)
    {

        int retries = 0;
        std::cout << "Wait until we see " << destination << std::endl;
        foundmutex.Lock();
        while (retries < MAX_RETRIES && found.find(destination) == found.end()) {
            foundmutex.Unlock();
            qcc::Sleep(10);
            foundmutex.Lock();
        }
        foundmutex.Unlock();
        EXPECT_NE(MAX_RETRIES, retries);

    }

    void WaitUntilLost(const qcc::String& destination)
    {

        int retries = 0;
        std::cout << "Wait until " << destination << " is gone " << std::endl;
        lostmutex.Lock();
        while (retries < MAX_RETRIES && lost.find(destination) == lost.end()) {
            lostmutex.Unlock();
            qcc::Sleep(10);
            lostmutex.Lock();
            ++retries;
        }
        lostmutex.Unlock();
        EXPECT_NE(MAX_RETRIES, retries);

    }
};

static void destination_found(const void* context, const char* group, const char* destination)
{
    ((TestPingListener*) context)->DestinationFound(group, destination);
}
static void destination_lost(const void* context, const char* group, const char* destination)
{
    ((TestPingListener*) context)->DestinationLost(group, destination);
}

static alljoyn_pinglistener_callback listener_cbs =
{ destination_found, destination_lost };

TEST_F(AutoPingerTest, Basic)
{
    alljoyn_busattachment clientBus;
    clientBus = alljoyn_busattachment_create("app", false);
    EXPECT_EQ(ER_OK, alljoyn_busattachment_start(clientBus));
    EXPECT_EQ(ER_OK, alljoyn_busattachment_connect(clientBus, NULL));

    TestPingListener tpl;
    alljoyn_pinglistener pl = alljoyn_pinglistener_create(&listener_cbs, &tpl);

    alljoyn_autopinger_addpinggroup(autoPinger, "testgroup", pl, 1);
    qcc::String uniqueName = alljoyn_busattachment_getuniquename(clientBus);
    EXPECT_EQ(ER_BUS_PING_GROUP_NOT_FOUND,
              alljoyn_autopinger_adddestination(autoPinger, "badgroup", uniqueName.c_str()));
    EXPECT_EQ(ER_OK, alljoyn_autopinger_adddestination(autoPinger, "testgroup",
                                                       uniqueName.c_str()));
    EXPECT_EQ(ER_OK, alljoyn_autopinger_adddestination(autoPinger, "testgroup",
                                                       uniqueName.c_str()));

    tpl.WaitUntilFound(uniqueName);
    alljoyn_busattachment_disconnect(clientBus, NULL);
    tpl.WaitUntilLost(uniqueName);

    EXPECT_EQ(ER_FAIL, alljoyn_autopinger_removedestination(autoPinger, "badgroup",
                                                            uniqueName.c_str()));
    EXPECT_EQ(ER_OK, alljoyn_autopinger_removedestination(autoPinger, "testgroup",
                                                          uniqueName.c_str()));
    EXPECT_EQ(ER_OK, alljoyn_autopinger_removedestination(autoPinger, "testgroup",
                                                          uniqueName.c_str()));

    EXPECT_EQ(ER_BUS_PING_GROUP_NOT_FOUND, alljoyn_autopinger_setpinginterval(
                  autoPinger, "badgroup", 2));
    /* no real test on updated interval */
    EXPECT_EQ(ER_OK, alljoyn_autopinger_setpinginterval(autoPinger, "testgroup", 2));

    alljoyn_autopinger_pause(autoPinger);
    alljoyn_autopinger_pause(autoPinger);
    alljoyn_autopinger_resume(autoPinger);
    alljoyn_autopinger_resume(autoPinger);

    alljoyn_busattachment_connect(clientBus, NULL);

    uniqueName = alljoyn_busattachment_getuniquename(clientBus);
    EXPECT_EQ(ER_OK, alljoyn_autopinger_adddestination(autoPinger, "testgroup",
                                                       uniqueName.c_str()));
    tpl.WaitUntilFound(uniqueName);

    alljoyn_autopinger_removepinggroup(autoPinger, "badgroup");
    alljoyn_autopinger_removepinggroup(autoPinger, "testgroup");

    alljoyn_pinglistener_destroy(pl);

    alljoyn_busattachment_disconnect(clientBus, NULL);
    alljoyn_busattachment_stop(clientBus);
    alljoyn_busattachment_join(clientBus);
    alljoyn_busattachment_destroy(clientBus);
}

TEST_F(AutoPingerTest, Multibus)
{
    const size_t G = 2;
    TestPingListener tpl[G];
    qcc::String groupNames[G] = {   "evengroup", "oddgroup" };
    alljoyn_pinglistener pl[G];

    for (size_t i = 0; i < G; ++i) {
        pl[i] = alljoyn_pinglistener_create(&listener_cbs, &tpl[i]);
        alljoyn_autopinger_addpinggroup(autoPinger, groupNames[i].c_str(), pl[i], 1);
    }

#ifdef __MACH__
    // On darwin platform the number 10 causes "Too many open files error". 5 is found to sustain
    const int N = 5;
#else
    const int N = 10;
#endif
    std::vector<alljoyn_busattachment> serviceBuses(N);
    std::vector<qcc::String> uniqueNames(N);
    for (size_t i = 0; i < serviceBuses.size(); ++i) {
        serviceBuses[i] = alljoyn_busattachment_create("test", false);
        alljoyn_busattachment_start(serviceBuses[i]);
        alljoyn_busattachment_connect(serviceBuses[i], NULL);
        uniqueNames[i] = alljoyn_busattachment_getuniquename(serviceBuses[i]);

        alljoyn_autopinger_adddestination(autoPinger, groupNames[i % G].c_str(),
                                          uniqueNames[i].c_str());
    }

    for (size_t i = 0; i < serviceBuses.size(); ++i) {
        tpl[i % G].WaitUntilFound(uniqueNames[i]);
        alljoyn_busattachment_disconnect(serviceBuses[i], NULL);
    }

    for (size_t i = 0; i < serviceBuses.size(); ++i) {
        tpl[i % G].WaitUntilLost(uniqueNames[i]);
    }

    for (size_t i = 0; i < serviceBuses.size(); ++i) {
        alljoyn_busattachment_stop(serviceBuses[i]);
        alljoyn_busattachment_join(serviceBuses[i]);
        alljoyn_busattachment_destroy(serviceBuses[i]);
    }

    for (size_t i = 0; i < G; ++i) {
        alljoyn_autopinger_removepinggroup(autoPinger, groupNames[i].c_str());
        alljoyn_pinglistener_destroy(pl[i]);
    }

}
}
