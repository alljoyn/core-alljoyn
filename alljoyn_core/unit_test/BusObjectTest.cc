/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/DBusStd.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>

using namespace ajn;
using namespace qcc;

/*constants*/
static const char* OBJECT_PATH =   "/org/alljoyn/test/BusObjectTest";

class BusObjectTest : public testing::Test {
  public:
    BusObjectTest() : bus("BusObjectTest", false) { }

    class BusObjectTestBusObject : public BusObject {
      public:
        BusObjectTestBusObject(BusAttachment& bus, const char* path)
            : BusObject(path), wasRegistered(false), wasUnregistered(false) { }
        virtual ~BusObjectTestBusObject() { }
        virtual void ObjectRegistered(void) {
            BusObject::ObjectRegistered();
            wasRegistered = true;
        }
        virtual void ObjectUnregistered(void) {
            BusObject::ObjectUnregistered();
            wasUnregistered = true;
        }

        bool wasRegistered, wasUnregistered;
    };

    QStatus status;
    BusAttachment bus;
};

/* ALLJOYN-1292 */
TEST_F(BusObjectTest, DISABLED_ObjectRegisteredUnregistered) {
    BusObjectTestBusObject testObj(bus, OBJECT_PATH);
    status = bus.RegisterBusObject(testObj);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.Start();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Connect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = bus.Disconnect(ajn::getConnectArg().c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Stop();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = bus.Join();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (int i = 0; i < 500; ++i) {
        qcc::Sleep(10);
        if (testObj.wasRegistered && testObj.wasUnregistered) {
            break;
        }
    }
    EXPECT_TRUE(testObj.wasRegistered);
    EXPECT_TRUE(testObj.wasUnregistered);
}
