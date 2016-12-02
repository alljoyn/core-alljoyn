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
#include <qcc/platform.h>

#include <alljoyn/Message.h>

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "../ajTestCommon.h"

using namespace std;
using namespace qcc;
using namespace ajn;

class _TestMessage : public _Message {
  public:
    _TestMessage(BusAttachment& bus, const HeaderFields& hdrFields) : _Message(bus, hdrFields) { }
};
typedef ManagedObj<_TestMessage> TestMessage;

/* ASACORE-1111 */
TEST(MessageTest, GetNullHeaderFieldsReturnsEmptyString)
{
    BusAttachment bus(NULL);
    HeaderFields hdrFields;
    hdrFields.field[ALLJOYN_HDR_FIELD_SIGNATURE].Set("g", NULL);
    hdrFields.field[ALLJOYN_HDR_FIELD_PATH].Set("o", NULL);
    hdrFields.field[ALLJOYN_HDR_FIELD_INTERFACE].Set("s", NULL);
    hdrFields.field[ALLJOYN_HDR_FIELD_MEMBER].Set("s", NULL);
    hdrFields.field[ALLJOYN_HDR_FIELD_SENDER].Set("s", NULL);
    hdrFields.field[ALLJOYN_HDR_FIELD_DESTINATION].Set("s", NULL);
    TestMessage msg(bus, hdrFields);

    EXPECT_STREQ("", msg->GetSignature());
    EXPECT_STREQ("", msg->GetObjectPath());
    EXPECT_STREQ("", msg->GetInterface());
    EXPECT_STREQ("", msg->GetMemberName());
    EXPECT_STREQ("", msg->GetSender());
    EXPECT_STREQ("", msg->GetDestination());
}