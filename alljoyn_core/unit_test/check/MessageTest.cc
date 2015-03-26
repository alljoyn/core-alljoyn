/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
#include <qcc/platform_cpp.h>

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
