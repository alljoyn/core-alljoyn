/**
 * @file
 * Implementation of a service object.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>

#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <vector>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Environ.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>

#define SUCCESS 1

using namespace std;
using namespace qcc;
using namespace ajn;


class ServiceTestObject : public BusObject {
  public:

    ServiceTestObject(BusAttachment& bus, const char*);
    void NameAcquiredSignalHandler(const InterfaceDescription::Member*member, const char*, Message& msg);
    void ObjectRegistered(void);
    void ByteArrayTest(const InterfaceDescription::Member* member, Message& msg);
    void DoubleArrayTest(const InterfaceDescription::Member* member, Message& msg);
    void Ping(const InterfaceDescription::Member* member, Message& msg);
    void Sing(const InterfaceDescription::Member* member, Message& msg);
    void King(const InterfaceDescription::Member* member, Message& msg);
    QStatus Get(const char*ifcName, const char*propName, MsgArg& val);
    QStatus Set(const char*ifcName, const char*propName, MsgArg& val);
    int getOutput() { return output1; }
    int setOutput(int value) {  output1 = value; return output1; }
    void RegisterForNameAcquiredSignals();
    void PopulateSignalMembers(const char*);
    QStatus InstallMethodHandlers(const char*);
    QStatus AddInterfaceToObject(const InterfaceDescription*);

    const InterfaceDescription::Member* my_signal_member;
    qcc::String prop_str_val;
    qcc::String prop_ro_str;
    int32_t prop_int_val;
    int output1;
    BusAttachment& myBus;
};
