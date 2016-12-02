/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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

#include "ajTestCommon.h"

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
        QCC_ASSERT(signal_member != NULL);
        MsgArg arg("s", "Signal");
        QStatus status = Signal(NULL, sessionid, *signal_member, &arg, 1, 0, 0);
        return status;
    }

    void Pasta(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = MethodReply(msg, arg, 1);
        EXPECT_EQ(ER_OK, status) << "Pasta: Error sending reply";
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
        QCC_UNUSED(member);
        QCC_UNUSED(sourcePath);
        QCC_UNUSED(msg);
        signalReceived++;
    }

    unsigned int signalReceived;
};
