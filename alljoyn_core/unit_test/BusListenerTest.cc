/******************************************************************************
 * Copyright (c) 2012, 2014, AllSeen Alliance. All rights reserved.
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
#include "ajTestCommon.h"

#include <qcc/Thread.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>

using namespace ajn;

/*flags*/
static bool listener_registered_flag = false;
static bool listener_unregistered_flag = false;
static bool found_advertised_name_flag = false;
static bool lost_advertised_name_flag = false;
static bool name_owner_changed_flag = false;
static bool bus_stopping_flag = false;
static bool bus_disconnected_flag = false;

class TestBusListener : public BusListener {
  public:
    virtual void ListenerRegistered(BusAttachment* bus) {
        listener_registered_flag = true;
    }
    virtual void ListenerUnregistered() {
        listener_unregistered_flag = true;
    }
    virtual void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix) {
        found_advertised_name_flag = true;
    }
    virtual void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix) {
        lost_advertised_name_flag = true;
    }
    virtual void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner) {
        name_owner_changed_flag = true;
    }
    virtual void BusStopping() {
        bus_stopping_flag = true;
    }
    virtual void BusDisconnected() {
        bus_disconnected_flag = true;
    }
};

class BusListenerTest : public testing::Test {
  public:
    BusListenerTest() : bus("BusListenerTest", false) {
    }

    virtual void SetUp() {
        objectName = genUniqueName(bus);
        resetFlags();
    }

    virtual void TearDown() {

    }

    void resetFlags() {
        listener_registered_flag = false;
        listener_unregistered_flag = false;
        found_advertised_name_flag = false;
        lost_advertised_name_flag = false;
        name_owner_changed_flag = false;
        bus_stopping_flag = false;
        bus_disconnected_flag = false;
    }

    QStatus status;
    TestBusListener buslistener;
    BusAttachment bus;
    qcc::String objectName;
};

TEST_F(BusListenerTest, listener_registered_unregistered) {
    bus.RegisterBusListener(buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);
    bus.UnregisterBusListener(buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_unregistered_flag);
}

TEST_F(BusListenerTest, bus_unregister_listener_when_busAttachment_destroyed) {
    BusAttachment* busattachment = new BusAttachment("BusListenerTestInternal", false);
    busattachment->RegisterBusListener(buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);

    status = busattachment->Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = busattachment->Connect(ajn::getConnectArg().c_str());

    busattachment->Stop();
    for (size_t i = 0; i < 200; ++i) {
        if (bus_stopping_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_stopping_flag);
    busattachment->Join();

    /* the bus will automatically disconnect when it is stopped */
    for (size_t i = 0; i < 200; ++i) {
        if (bus_disconnected_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_disconnected_flag);

    /*
     * We do not expect the ListenerUnregistered callback to be called when
     * the BusAttachment is stopped.  We only expect it to be unregistered
     * when UnregisterBusListener is called or when the BusAttachment's
     * destructor has been called.
     */
    EXPECT_FALSE(listener_unregistered_flag);
    delete busattachment;

    /*
     * BusAttachments destructor should have been called at the end of the
     * scope so we should have the listener_unregistered_flag set.
     */
    EXPECT_TRUE(listener_unregistered_flag);
}
/* ALLJOYN-1308 */
TEST_F(BusListenerTest, bus_stopping_disconnected) {
    bus.RegisterBusListener(buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);

    status = bus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Connect(ajn::getConnectArg().c_str());

    bus.Disconnect(ajn::getConnectArg().c_str());
    for (size_t i = 0; i < 200; ++i) {
        if (bus_disconnected_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    /*
     * Expect the bus_disconnected_flag to be set when BusAttachment.Disconnect
     * is called.
     */
    EXPECT_TRUE(bus_disconnected_flag);

    bus.Stop();
    for (size_t i = 0; i < 200; ++i) {
        if (bus_stopping_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_disconnected_flag);
    EXPECT_TRUE(bus_stopping_flag);
    bus.Join();

    bus.UnregisterBusListener(buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_unregistered_flag);
}

TEST_F(BusListenerTest, found_lost_advertised_name) {
    status = bus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Connect(ajn::getConnectArg().c_str());

    bus.RegisterBusListener(buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    status = bus.FindAdvertisedName(objectName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.AdvertiseName(objectName.c_str(), opts.transports);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (size_t i = 0; i < 200; ++i) {
        if (found_advertised_name_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(found_advertised_name_flag);

    status = bus.CancelAdvertiseName(objectName.c_str(), opts.transports);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        if (lost_advertised_name_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(lost_advertised_name_flag);

    bus.Stop();
    for (size_t i = 0; i < 200; ++i) {
        if (bus_stopping_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_stopping_flag);
    bus.Join();
    /* the bus will automatically disconnect when it is stopped */
    for (size_t i = 0; i < 200; ++i) {
        if (bus_disconnected_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_disconnected_flag);

    bus.UnregisterBusListener(buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_unregistered_flag);

    status = bus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST_F(BusListenerTest, name_owner_changed) {
    status = bus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Connect(ajn::getConnectArg().c_str());

    bus.RegisterBusListener(buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);

    bus.RequestName(objectName.c_str(), 0);
    for (size_t i = 0; i < 200; ++i) {
        if (name_owner_changed_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(name_owner_changed_flag);

    bus.Stop();
    for (size_t i = 0; i < 200; ++i) {
        if (bus_stopping_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_stopping_flag);
    bus.Join();
    /* the bus will automatically disconnect when it is stopped */
    for (size_t i = 0; i < 200; ++i) {
        if (bus_disconnected_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_disconnected_flag);

    bus.UnregisterBusListener(buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_unregistered_flag);

    status = bus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

