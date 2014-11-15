/*******************************************************************************
 * Copyright (c) 2011, 2014, AllSeen Alliance. All rights reserved.
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
#ifndef _TEST_CLIENTSETUP_H
#define _TEST_CLIENTSETUP_H


#include <qcc/String.h>
#include <qcc/Util.h>
#include <qcc/Environ.h>
#include <qcc/Debug.h>




#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/version.h>


#include <alljoyn/Status.h>

#include <stdio.h>
#include <vector>

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;
using namespace ajn;

class ClientSetup : public MessageReceiver {
  public:
    ClientSetup(const char* default_bus_addr, const char* wellKnownName);

    BusAttachment* getClientMsgBus();
    qcc::String getClientArgs();

    QStatus MethodCall(int noOfCalls, int type);
    QStatus AsyncMethodCall(int noOfCalls, int type);
    void AsyncCallReplyHandler(Message& msg, void* context);
    QStatus SignalHandler(int noOfCalls, int type);
    void MySignalHandler(const InterfaceDescription::Member*member,
                         const char* sourcePath,
                         Message& msg);

    void MySignalHandler2(const InterfaceDescription::Member*member,
                          const char* sourcePath,
                          Message& msg);
    int getSignalFlag();
    void setSignalFlag(int flag);

    const char* getClientInterfaceName() const;
    const char* getClientDummyInterfaceName1() const;
    const char* getClientDummyInterfaceName2() const;
    const char* getClientDummyInterfaceName3() const;
    const char* getClientObjectPath() const;

    const char* getClientValuesInterfaceName() const;
    const char* getClientValuesDummyInterfaceName1() const;
    const char* getClientValuesDummyInterfaceName2() const;
    const char* getClientValuesDummyInterfaceName3() const;
  private:
    Event waitEvent;
    int g_Signal_flag;
    BusAttachment clientMsgBus;
    qcc::String clientArgs;
    qcc::String wellKnownName;
};

#endif
