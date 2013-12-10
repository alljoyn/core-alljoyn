/**
 * @file
 *
 * This file tests AllJoyn header compression.
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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

#include <qcc/Debug.h>
#include <qcc/Pipe.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Message.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>

/* Private files included for unit testing */
#include <RemoteEndpoint.h>


using namespace qcc;
using namespace std;
using namespace ajn;


class MyMessage : public _Message {
  public:

    MyMessage(BusAttachment& bus) : _Message(bus) { }


    QStatus MethodCall(const char* destination,
                       const char* objPath,
                       const char* interface,
                       const char* methodName,
                       uint8_t flags = 0)
    {
        flags |= ALLJOYN_FLAG_COMPRESSED;
        return CallMsg("", destination, 0, objPath, interface, methodName, NULL, 0, flags);
    }

    QStatus Signal(const char* destination,
                   const char* objPath,
                   const char* interface,
                   const char* signalName,
                   uint16_t ttl,
                   uint32_t sessionId = 0)
    {
        return SignalMsg("", destination, sessionId, objPath, interface, signalName, NULL, 0, ALLJOYN_FLAG_COMPRESSED, ttl);
    }
    QStatus Read(RemoteEndpoint& ep, const qcc::String& endpointName, bool pedantic = true)
    {
        return _Message::Read(ep, pedantic);
    }

    QStatus Unmarshal(RemoteEndpoint& ep, const qcc::String& endpointName, bool pedantic = true)
    {
        return _Message::Unmarshal(ep, pedantic);
    }

    QStatus Deliver(RemoteEndpoint& ep)
    {
        return _Message::Deliver(ep);
    }

};


int main(int argc, char** argv)
{
    QStatus status;
    uint32_t tok1;
    uint32_t tok2;
    BusAttachment bus("compression");
    MyMessage msg(bus);
    Pipe* stream = new Pipe();
    static const bool falsiness = false;
    RemoteEndpoint ep(bus, falsiness, String::Empty, stream);

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    bus.Start();

    status = msg.MethodCall(":1.99", "/foo/bar", "foo.bar", "test");
    if (status != ER_OK) {
        printf("Error %s\n", QCC_StatusText(status));
    }
    tok1 = msg.GetCompressionToken();

    status = msg.MethodCall(":1.99", "/foo/bar", "foo.bar", "test");
    if (status != ER_OK) {
        printf("Error %s\n", QCC_StatusText(status));
    }
    tok2 = msg.GetCompressionToken();

    /* Expect same message to have same token */
    if (tok1 != tok2) {
        printf("\nFAILED 1\n");
        delete stream;
        return -1;
    }

    status = msg.MethodCall(":1.98", "/foo/bar", "foo.bar", "test");
    if (status != ER_OK) {
        printf("Error %s\n", QCC_StatusText(status));
    }
    tok2 = msg.GetCompressionToken();

    /* Expect different messages to have different token */
    if (tok1 == tok2) {
        printf("\nFAILED 2\n");
        delete stream;
        return -1;
    }

    status = msg.Signal(":1.99", "/foo/bar/gorn", "foo.bar", "test", 0);
    if (status != ER_OK) {
        printf("Error %s\n", QCC_StatusText(status));
    }
    tok1 = msg.GetCompressionToken();

    status = msg.Signal(":1.99", "/foo/bar/gorn", "foo.bar", "test", 1000);
    if (status != ER_OK) {
        printf("Error %s\n", QCC_StatusText(status));
        delete stream;
        return -1;
    }
    tok2 = msg.GetCompressionToken();

    /* Expect messages with and without TTL to have different token */
    if (tok1 == tok2) {
        printf("\nFAILED 3\n");
        delete stream;
        return -1;
    }

    /* Expect messages with different TTLs to have different token */
    status = msg.Signal(":1.99", "/foo/bar/gorn", "foo.bar", "test", 9999);
    if (status != ER_OK) {
        printf("Error %s\n", QCC_StatusText(status));
        delete stream;
        return -1;
    }
    tok1 = msg.GetCompressionToken();

    if (tok1 == tok2) {
        printf("\nFAILED 4\n");
        delete stream;
        return -1;
    }

    /* Expect messages with same TTL but different timestamps to have same token */
    status = msg.Signal(":1.1234", "/foo/bar/again", "boo.far", "test", 1700);
    if (status != ER_OK) {
        printf("Error %s\n", QCC_StatusText(status));
        delete stream;
        return -1;
    }
    tok1 = msg.GetCompressionToken();

    qcc::Sleep(5);

    status = msg.Signal(":1.1234", "/foo/bar/again", "boo.far", "test", 1700);
    if (status != ER_OK) {
        printf("Error %s\n", QCC_StatusText(status));
        delete stream;
        return -1;
    }
    tok2 = msg.GetCompressionToken();

    if (tok1 != tok2) {
        printf("\nFAILED 5\n");
        delete stream;
        return -1;
    }

    status = msg.Signal(":1.99", "/foo/bar/gorn", "foo.bar", "test", 0, 1234);
    if (status != ER_OK) {
        printf("Error %s\n", QCC_StatusText(status));
    }
    tok1 = msg.GetCompressionToken();

    status = msg.Signal(":1.99", "/foo/bar/gorn", "foo.bar", "test", 0, 5678);
    if (status != ER_OK) {
        printf("Error %s\n", QCC_StatusText(status));
        delete stream;
        return -1;
    }
    tok2 = msg.GetCompressionToken();

    /* Expect messages with different session id's to have different token */
    if (tok1 == tok2) {
        printf("\nFAILED 3\n");
        delete stream;
        return -1;
    }

    /* No do a real marshal unmarshal round trip */
    for (int i = 0; i < 20; ++i) {
        SessionId sess = 1000 + i % 3;
        qcc::String sig = "test" + qcc::U32ToString(i);
        status = msg.Signal(":1.1234", "/fun/games", "boo.far", sig.c_str(), 1900, sess);
        if (status != ER_OK) {
            printf("Error %s\n", QCC_StatusText(status));
            delete stream;
            return -1;
        }
        status = msg.Deliver(ep);
        if (status != ER_OK) {
            printf("Error %s\n", QCC_StatusText(status));
            delete stream;
            return -1;
        }
    }

    for (int i = 0; i < 20; ++i) {
        SessionId sess = 1000 + i % 3;
        qcc::String sig = "test" + qcc::U32ToString(i);
        MyMessage msg2(bus);
        status = msg2.Read(ep, ":88.88");
        if (status != ER_OK) {
            printf("Error %s\n", QCC_StatusText(status));
            delete stream;
            return -1;
        }

        status = msg2.Unmarshal(ep, ":88.88");
        if (status != ER_OK) {
            printf("Error %s\n", QCC_StatusText(status));
            delete stream;
            return -1;
        }
        if (msg2.GetSessionId() != sess) {
            printf("\nFAILED 6.%d\n", i);
            delete stream;
            return -1;
        }
        if (sig != msg2.GetMemberName()) {
            printf("\nFAILED 6.%d\n", i);
            delete stream;
            return -1;
        }
    }

    printf("PASSED\n");
    delete stream;
    return 0;
}
