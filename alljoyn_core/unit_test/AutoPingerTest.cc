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
#include <alljoyn/BusAttachment.h>
#include <alljoyn/AutoPinger.h>
#include <gtest/gtest.h>

using namespace ajn;

class AutoPingerTest : public testing::Test {
  public:
    BusAttachment serviceBus;
    AutoPinger autoPinger;

    AutoPingerTest() : serviceBus("AutoPingerTest", false), autoPinger(serviceBus) { };

    virtual void SetUp() {
        QStatus status = ER_OK;
        status = serviceBus.Start();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        ASSERT_FALSE(serviceBus.IsConnected());
        status = serviceBus.Connect();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        ASSERT_TRUE(serviceBus.IsConnected());
    }

    virtual void TearDown() {
        serviceBus.Disconnect();
        serviceBus.Stop();
        serviceBus.Join();
    }

};

#define MAX_RETRIES 100

class TestPingListener : public PingListener {

  private:
    std::set<qcc::String> found;
    qcc::Mutex foundmutex;
    std::set<qcc::String> lost;
    qcc::Mutex lostmutex;

    virtual void DestinationLost(const qcc::String& group, const qcc::String& destination) {

        std::cout << "on lost " << destination  << std::endl;
        lostmutex.Lock();
        lost.insert(destination);
        lostmutex.Unlock();

    }

    virtual void DestinationFound(const qcc::String& group, const qcc::String& destination) {

        std::cout << "on found " << destination  << std::endl;
        foundmutex.Lock();
        found.insert(destination);
        foundmutex.Unlock();

    }

  public:
    TestPingListener() { }

    void WaitUntilFound(const qcc::String& destination) {

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

    void WaitUntilLost(const qcc::String& destination) {

        int retries = 0;
        std::cout << "Wait until " << destination << "is gone " << std::endl;
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

TEST_F(AutoPingerTest, Basic) {

    BusAttachment clientBus("app", false);
    EXPECT_EQ(ER_OK, clientBus.Start());
    EXPECT_EQ(ER_OK, clientBus.Connect());

    TestPingListener tpl;

    autoPinger.AddPingGroup("testgroup", tpl, 1);
    qcc::String uniqueName = clientBus.GetUniqueName();
    EXPECT_EQ(ER_BUS_PING_GROUP_NOT_FOUND, autoPinger.AddDestination("badgroup", uniqueName));
    EXPECT_EQ(ER_OK, autoPinger.AddDestination("testgroup", uniqueName));
    EXPECT_EQ(ER_OK, autoPinger.AddDestination("testgroup", uniqueName));

    tpl.WaitUntilFound(uniqueName);
    clientBus.Disconnect();
    tpl.WaitUntilLost(uniqueName);

    autoPinger.RemoveDestination("badgroup", uniqueName);
    autoPinger.RemoveDestination("testgroup", uniqueName);
    autoPinger.RemoveDestination("testgroup", uniqueName);

    EXPECT_EQ(ER_BUS_PING_GROUP_NOT_FOUND, autoPinger.SetPingInterval("badgroup", 2));
    EXPECT_EQ(ER_OK, autoPinger.SetPingInterval("testgroup", 2)); /* no real test on updated interval */


    autoPinger.Pause();
    autoPinger.Pause();
    autoPinger.Resume();
    autoPinger.Resume();

    clientBus.Connect();

    uniqueName = clientBus.GetUniqueName();
    EXPECT_EQ(ER_OK, autoPinger.AddDestination("testgroup", uniqueName));
    tpl.WaitUntilFound(uniqueName);

    autoPinger.RemovePingGroup("badgroup");
    autoPinger.RemovePingGroup("testgroup");

    clientBus.Disconnect();
    clientBus.Stop();
    clientBus.Join();
}


TEST_F(AutoPingerTest, Multibus) {

    const int G = 2;
    TestPingListener tpl[G];
    qcc::String groupNames[G] = { "evengroup", "oddgroup" };

    for (int i = 0; i < G; ++i) {
        autoPinger.AddPingGroup(groupNames[i], tpl[i], 1);
    }

    // On darwin platform the number 10 causes "Too many open files error". 5 is found to sustain
#ifdef __MACH__
    const int N = 5;
#else
    const int N = 10;
#endif
    std::vector<BusAttachment*> serviceBuses(N);
    std::vector<qcc::String> uniqueNames(N);
    for (size_t i = 0; i < serviceBuses.size(); ++i) {
        serviceBuses[i] = new BusAttachment("test");
        serviceBuses[i]->Start();
        serviceBuses[i]->Connect();
        uniqueNames[i] = serviceBuses[i]->GetUniqueName();
        autoPinger.AddDestination(groupNames[i % G], uniqueNames[i]);
    }

    for (size_t i = 0; i < serviceBuses.size(); ++i) {
        tpl[i % G].WaitUntilFound(uniqueNames[i]);
        serviceBuses[i]->Disconnect();
    }

    for (size_t i = 0; i < serviceBuses.size(); ++i) {
        tpl[i % G].WaitUntilLost(uniqueNames[i]);
    }

    for (size_t i = 0; i < serviceBuses.size(); ++i) {
        serviceBuses[i]->Stop();
        serviceBuses[i]->Join();
        delete serviceBuses[i];
    }

}

