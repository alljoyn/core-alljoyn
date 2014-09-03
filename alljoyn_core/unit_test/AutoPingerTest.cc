/******************************************************************************
 * Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <gtest/gtest.h>

using namespace ajn;

class AutoPingerTest : public testing::Test {
  public:
    BusAttachment bus;
    AutoPinger autoPinger;

    AutoPingerTest() : bus("BusAttachmentTest", false), autoPinger(bus) { };

    virtual void SetUp() {
        QStatus status = ER_OK;
        status = bus.Start();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        ASSERT_FALSE(bus.IsConnected());
        status = bus.Connect();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        ASSERT_TRUE(bus.IsConnected());
    }

    virtual void TearDown() {
        bus.Stop();
        bus.Join();
    }

};


class TestPingListener : public PingListener {

  private:
    std::set<qcc::String> found;
    std::mutex foundmutex;
    std::condition_variable cv_found;

    std::set<qcc::String> lost;
    std::mutex lostmutex;
    std::condition_variable cv_lost;

    virtual void DestinationLost(const qcc::String& group, const qcc::String& destination) {

        std::cout << "on lost " << destination  << std::endl;
        std::unique_lock<std::mutex> lk(lostmutex);
        lost.insert(destination);
        cv_lost.notify_one();

    }

    virtual void DestinationFound(const qcc::String& group, const qcc::String& destination) {

        std::cout << "on found " << destination  << std::endl;
        std::unique_lock<std::mutex> lk(foundmutex);
        found.insert(destination);
        cv_found.notify_one();

    }

  public:
    void WaitUntilFound(const qcc::String& destination) {

        std::cout << "Wait until we see " << destination << std::endl;
        std::unique_lock<std::mutex> lk(foundmutex);
        cv_found.wait(lk, [this, destination] { return found.find(destination) != found.end(); });

    }

    void WaitUntilLost(const qcc::String& destination) {

        std::cout << "Wait until " << destination << "is gone " << std::endl;
        std::unique_lock<std::mutex> lk(lostmutex);
        cv_lost.wait(lk, [this, destination] { return lost.find(destination) != lost.end(); });

    }
};

TEST_F(AutoPingerTest, Basic) {

    BusAttachment bus2("app", false);
    EXPECT_EQ(ER_OK, bus2.Start());
    EXPECT_EQ(ER_OK, bus2.Connect());

    TestPingListener tpl;

    autoPinger.AddPingGroup("testgroup", tpl, 1);
    qcc::String uniqueName = bus2.GetUniqueName();
    EXPECT_EQ(ER_FAIL, autoPinger.AddDestination("badgroup", uniqueName));
    EXPECT_EQ(ER_OK, autoPinger.AddDestination("testgroup", uniqueName));
    EXPECT_EQ(ER_OK, autoPinger.AddDestination("testgroup", uniqueName));

    tpl.WaitUntilFound(uniqueName);
    bus2.Disconnect();
    tpl.WaitUntilLost(uniqueName);

    autoPinger.RemoveDestination("badgroup", uniqueName);
    autoPinger.RemoveDestination("testgroup", uniqueName);
    autoPinger.RemoveDestination("testgroup", uniqueName);

    EXPECT_EQ(ER_FAIL, autoPinger.SetPingInterval("badgroup", 2));
    EXPECT_EQ(ER_OK, autoPinger.SetPingInterval("testgroup", 2)); /* no real test on updated interval */


    autoPinger.Pause();
    autoPinger.Pause();
    autoPinger.Resume();
    autoPinger.Resume();

    bus2.Connect();

    uniqueName = bus2.GetUniqueName();
    EXPECT_EQ(ER_OK, autoPinger.AddDestination("testgroup", uniqueName));
    tpl.WaitUntilFound(uniqueName);

    autoPinger.RemovePingGroup("badgroup");
    autoPinger.RemovePingGroup("testgroup");

}


TEST_F(AutoPingerTest, Multibus) {

    const int G = 2;
    TestPingListener tpl[G];
    qcc::String groupNames[G] = { "evengroup", "oddgroup" };

    for (size_t i = 0; i < G; ++i) {
        autoPinger.AddPingGroup(groupNames[i], tpl[i], 1);
    }

    const int N = 10;
    std::vector<std::unique_ptr<BusAttachment> > busses(N);
    std::vector<qcc::String> uniqueNames(N);
    for (size_t i = 0; i < busses.size(); ++i) {
        busses[i] = std::move(std::unique_ptr<BusAttachment>(new BusAttachment("test")));
        busses[i]->Start();
        busses[i]->Connect();
        uniqueNames[i] = busses[i]->GetUniqueName();
        autoPinger.AddDestination(groupNames[i % G], uniqueNames[i]);
    }

    for (size_t i = 0; i < busses.size(); ++i) {
        tpl[i % G].WaitUntilFound(uniqueNames[i]);
        busses[i]->Disconnect();
    }

    for (size_t i = 0; i < busses.size(); ++i) {
        tpl[i % G].WaitUntilLost(uniqueNames[i]);
    }

}

