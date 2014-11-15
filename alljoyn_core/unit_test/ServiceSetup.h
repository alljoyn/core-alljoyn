/*******************************************************************************
 * Copyright (c) 2011,2014 AllSeen Alliance. All rights reserved.
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
#ifndef _TEST_ServiceSetup_H
#define _TEST_ServiceSetup_H

#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <vector>

#include <alljoyn/DBusStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/version.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/BusObject.h>

#include <qcc/platform.h>
#include <qcc/Environ.h>
#include <qcc/Util.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;
using namespace ajn;

/* Service Bus Listener */
class MyBusListener : public BusListener, public SessionPortListener {

  public:
    MyBusListener() : BusListener() { }

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts);

};

/* Auth Listener */
class MyAuthListener : public AuthListener {
    QStatus RequestPwd(const qcc::String& authMechanism, uint8_t minLen, qcc::String& pwd);
};

/* Old LocalTestObject class */
class ServiceObject : public BusObject {
  public:
    ServiceObject(BusAttachment& bus, const char*path);
    ~ServiceObject();

    // Service Object related API
    void ObjectRegistered();
    QStatus AddInterfaceToObject(const InterfaceDescription* intf);
    void PopulateSignalMembers();
    void NameAcquiredCB(Message& msg, void* context);
    void RequestName(const char* name);
    QStatus InstallMethodHandlers();
    void Ping(const InterfaceDescription::Member* member, Message& msg);
    void Sing(const InterfaceDescription::Member* member, Message& msg);
    void ParamTest(const InterfaceDescription::Member* member, Message& msg);
    QStatus EmitTestSignal(String newName);
    QStatus Get(const char*ifcName, const char*propName, MsgArg& val);
    QStatus Set(const char*ifcName, const char*propName, MsgArg& val);
    bool getobjectRegistered();
    void setobjectRegistered(bool value);

    const char* getAlljoynInterfaceName() const;
    const char* getServiceInterfaceName() const;
    const char* getAlljoynWellKnownName() const;
    const char* getServiceWellKnownName() const;
    const char* getAlljoynObjectPath() const;
    const char* getServiceObjectPath() const;
    const char* getAlljoynValuesInterfaceName() const;
    const char* getServiceValuesInterfaceName() const;

    const char* getAlljoynDummyInterfaceName1() const;
    const char* getServiceDummyInterfaceName1() const;
    const char* getAlljoynDummyInterfaceName2() const;
    const char* getServiceDummyInterfaceName2() const;
    const char* getAlljoynDummyInterfaceName3() const;
    const char* getServiceDummyInterfaceName3() const;

    const char* getAlljoynValuesDummyInterfaceName1() const;
    const char* getServiceValuesDummyInterfaceName1() const;
    const char* getAlljoynValuesDummyInterfaceName2() const;
    const char* getServiceValuesDummyInterfaceName2() const;
    const char* getAlljoynValuesDummyInterfaceName3() const;
    const char* getServiceValuesDummyInterfaceName3() const;

  private:

    qcc::String prop_str_val;
    qcc::String prop_ro_str;
    int32_t prop_int_val;

    qcc::String prop_signal;
    const InterfaceDescription::Member* my_signal_member;
    const InterfaceDescription::Member* my_signal_member_2;
    bool objectRegistered;
    BusAttachment& myBus;
    qcc::String alljoynWellKnownName;
    qcc::String serviceWellKnownName;
};

#endif
