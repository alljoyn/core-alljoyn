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
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/DBusStd.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>

using namespace ajn;
using namespace qcc;


class BusObjectTestBusObject : public BusObject {
  public:
    BusObjectTestBusObject(BusAttachment& bus, const char* path)
        : BusObject(path), bus(bus), wasRegistered(false), wasUnregistered(false) {

    }

    virtual ~BusObjectTestBusObject() { }

    void ObjectRegistered(void) {
        wasRegistered = true;
    }
    void ObjectUnregistered(void) {
        wasUnregistered = true;
    }

    QStatus SendSignal(SessionId sessionid = 0) {
        const InterfaceDescription::Member*  signal_member = bus.GetInterface("org.test")->GetMember("my_signal");
        assert(signal_member != NULL);
        MsgArg arg("s", "Signal");
        QStatus status = Signal(NULL, sessionid, *signal_member, &arg, 1, 0, 0);
        return status;
    }

    void Pasta(const InterfaceDescription::Member* member, Message& msg)
    {
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = MethodReply(msg, arg, 1);
        EXPECT_EQ(ER_OK, status) << "Pasta: Error sending reply,  Actual Status: " << QCC_StatusText(status);
    }

    BusAttachment& bus;
    bool wasRegistered, wasUnregistered;
};

class BusObjectTestSignalReceiver : public MessageReceiver {

  public:

    BusObjectTestSignalReceiver() {
        signalReceived = 0;
    }

    void SignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg) {
        signalReceived++;
    }

    unsigned int signalReceived;
};

