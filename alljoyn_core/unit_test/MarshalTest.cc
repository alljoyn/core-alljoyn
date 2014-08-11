/******************************************************************************
 * Copyright (c) 2011-2014, AllSeen Alliance. All rights reserved.
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
#include <ctype.h>
#include <qcc/platform.h>
#include <queue>
#include <algorithm>

#include <qcc/Util.h>
#include <qcc/Pipe.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Message.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>

/* Private files included for unit testing */
#include <PeerState.h>
#include <SignatureUtils.h>
#include <RemoteEndpoint.h>

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>

using namespace ajn;
using namespace qcc;
using namespace std;


class TestPipe : public qcc::Pipe {
  public:
    TestPipe() : qcc::Pipe() { }

    QStatus PullBytesAndFds(void* buf, size_t reqBytes, size_t& actualBytes, SocketFd* fdList, size_t& numFds, uint32_t timeout = Event::WAIT_FOREVER)
    {
        QStatus status = ER_OK;
        numFds = std::min(numFds, fds.size());
        for (size_t i = 0; i < numFds; ++i) {
            *fdList++ = fds.front();
            fds.pop();
        }
        if (status == ER_OK) {
            status = PullBytes(buf, reqBytes, actualBytes);
        }
        return status;
    }

    QStatus PushBytesAndFds(const void* buf, size_t numBytes, size_t& numSent, SocketFd* fdList, size_t numFds, uint32_t pid = -1)
    {
        QStatus status = ER_OK;
        while (numFds--) {
            qcc::SocketFd sock;
            status = qcc::SocketDup(*fdList++, sock);
            if (status != ER_OK) {
                break;
            }
            fds.push(sock);
        }
        if (status == ER_OK) {
            status = PushBytes(buf, numBytes, numSent);
        }
        return status;
    }

    /** Destructor */
    virtual ~TestPipe() { }

  private:

    /* OOB file descriptors */
    std::queue<qcc::SocketFd> fds;

};


class MyMessage : public _Message {
  public:

    MyMessage(BusAttachment& Bus) : _Message(Bus) { };

    QStatus MethodCall(const char* destination,
                       const char* objPath,
                       const char* iface,
                       const char* methodName,
                       const MsgArg* argList,
                       size_t numArgs,
                       uint8_t flags = 0)
    {
        qcc::String sig = MsgArg::Signature(argList, numArgs);
        return CallMsg(sig, destination, 0, objPath, iface, methodName, argList, numArgs, flags);
    }

    QStatus Signal(const char* destination,
                   const char* objPath,
                   const char* iface,
                   const char* signalName,
                   const MsgArg* argList,
                   size_t numArgs)
    {
        qcc::String sig = MsgArg::Signature(argList, numArgs);
        return SignalMsg(sig, destination, 0, objPath, iface, signalName, argList, numArgs, 0, 0);
    }

    QStatus UnmarshalBody() { return UnmarshalArgs("*"); }

    QStatus Read(RemoteEndpoint& ep, const qcc::String& endpointName, bool pedantic = true)
    {
        QStatus test = _Message::Read(ep, pedantic);
        return test;
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



TEST(MarshalTest, ComplexSignatures) {
    QStatus status = ER_OK;

    const char* goodOnes[] = {
        "aiaaiaaaiaaaaiaaaaaaiaaaaaaaaaaaaaaaaaaaaaaaaaaaaai",
        "sigaa{s(vvvvvs(iia(ii)))}a(a(a(a(a(a(a(a(a(a(a(a(a(hii)))))))))))))(((a(((ai))))))",
        "(ybnqiuxtdsogai(i)va{ii})((((((((((ii))))))))))aaa(a(iai))si",
        "a{i(((((((((a((((((i)))))))))))))))}",
        "((ii)(xx)(ss)(y)(dhd)(nnn)(b)(h)(b)(b)a(o))",
        "a{ya{ba{na{qa{ia{ua{xa{ta{da{sa{oa{ga(ybnqiuxtsaogv)}}}}}}}}}}}}"
    };

    for (size_t i = 0; i < ArraySize(goodOnes); i++) {
        const char*sig = goodOnes[i];
        while (*sig) {
            const char* start = sig;
            status = SignatureUtils::ParseCompleteType(sig);
            ASSERT_EQ(status, ER_OK) <<  "Actual Status: " << QCC_StatusText(status) <<  "\nOriginal Signature:  " << goodOnes[i] <<
            "\nIncomplete type \"" << qcc::String(start, sig - start).c_str() << "\"";
        }
        /* If the string is successfully parsed, then its length must be 0. */
        ASSERT_EQ(static_cast<size_t>(0), ::strlen(sig));
    }

}

TEST(MarshalTest, InvalidSignatures) {

    QStatus status = ER_OK;

    const char* badIncompleteTypes[] = {
        "(((s)",
        "aaaaaaaa",
        "((iii)a)",
        "}ss}",
        "(ss}",
        "a(ss}",
        "a{ss)",
        "a{sss}",
        "a{(s)s}",
        "AI",
        "S",
        "X",
        "aX",
        "(WW)",
    };

    for (size_t i = 0; i < ArraySize(badIncompleteTypes); i++) {
        const char* sig = badIncompleteTypes[i];
        status = SignatureUtils::ParseCompleteType(sig);
        ASSERT_NE(ER_OK, status) << "Actual Status: " << QCC_StatusText(status) <<  "\nPassed parsing for OriginalSignature:  \"" << badIncompleteTypes[i] << "\"" <<
        ", this was expected to fail.";
    }

}

TEST(MarshalTest, SmallestAndBiggestSignatures) {

    /* Populate a 257 byte char array with i's */
    char sig[257];
    for (size_t i = 0; i < 257; ++i) {
        sig[i] = 'i';
    }

    sig[256] = 0;
    bool result = SignatureUtils::IsValidSignature(sig);
    ASSERT_FALSE(result) << "\nThis signature length is 256 and should be an invalid one. But it got parsed as valid. ";


    sig[255] = 0;
    result = SignatureUtils::IsValidSignature(sig);
    ASSERT_TRUE(result) << "\nThis signature length is 255 and should have been a valid one.";

    sig[0] = 0;
    result = SignatureUtils::IsValidSignature(sig);
    ASSERT_TRUE(result) << "\nThis signature length is 0 and should have been a valid one.";

}

TEST(MarshalTest, NestedArraysAndStructs) {

    /* Good array */
    const char aaaGood[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaai";
    bool result = SignatureUtils::IsValidSignature(aaaGood);
    ASSERT_TRUE(result) << "\nThe array : \"" << aaaGood << "\" is a valid signature and should be parsed correctly.";

    /* Bad array */
    const char aaaaBad[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaai";
    result = SignatureUtils::IsValidSignature(aaaaBad);
    ASSERT_FALSE(result) << "\nThe array : \"" << aaaaBad << "\" is an invalid signature and should have thrown error.";

    /* Good structure */
    const char sssGood[] = "((((((((((((((((((((((((((((((((ii))))))))))))))))))))))))))))))))";
    result = SignatureUtils::IsValidSignature(sssGood);
    ASSERT_TRUE(result) << "\nThe struct : \"" << sssGood << "\" is a valid signature and should be parsed correctly.";

    /* Bad structure */
    const char ssssBad[] = "(((((((((((((((((((((((((((((((((ii)))))))))))))))))))))))))))))))))";
    result = SignatureUtils::IsValidSignature(ssssBad);
    ASSERT_FALSE(result) << "\nThe struct : \"" << ssssBad << "\" is an invalid signature and should have thrown error.";

    /* Good signature */
    const char soGood[] = "((((((((((((((((((((((((((((((((iaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaai))))))))))))))))))))))))))))))))";
    result = SignatureUtils::IsValidSignature(soGood);
    ASSERT_TRUE(result) << "\nThe struct : \"" << soGood << "\" is a valid signature and should be parsed correctly.";

    /* Not so good signature */
    const char notSoGood[] = "a((((((((((((((((((((((((((((((((iaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaai))))))))))))))))))))))))))))))))";
    result = SignatureUtils::IsValidSignature(notSoGood);
    ASSERT_FALSE(result) << "\nThe struct : \"" << notSoGood << "\" is an invalid signature and should have thrown error.";

}

TEST(MarshalTest, TestMsgUnpack) {
    QStatus status = ER_OK;

    BusAttachment* bus = new BusAttachment("TestMsgUnPack", false);
    bus->Start();

    TestPipe stream;
    MyMessage msg(*bus);
    MsgArg args[4];
    size_t numArgs = ArraySize(args);
    double d = 0.9;

    TestPipe* pStream = &stream;
    static const bool falsiness = false;
    RemoteEndpoint ep(*bus, falsiness, String::Empty, pStream);
    ep->GetFeatures().handlePassing = true;

    MsgArg::Set(args, numArgs, "usyd", 4, "hello", 8, d);
    status = msg.MethodCall("a.b.c", "/foo/bar", "foo.bar", "test", args, numArgs);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = msg.Deliver(ep);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = msg.Read(ep, ":88.88");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = msg.Unmarshal(ep, ":88.88");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = msg.UnmarshalBody();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    uint32_t i;
    const char* s;
    uint8_t y;
    status = msg.GetArgs("usyd", &i, &s, &y, &d);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    if ((i != 4) || (strcmp(s, "hello") != 0) || (y != 8) || (d != 0.9)) {
        status = ER_FAIL;
    }

    ASSERT_EQ(status, ER_OK) << "Actual Status: " << QCC_StatusText(status) << ". TestMsgUnpack failed.";
    delete bus;
}

/*--------------------------FUZZING TEST CODE---------------------------------*/
static bool fuzzing = false;
static bool nobig = false;
static bool bigArray = false;
static bool quiet = false;
static qcc::String errString;
static BusAttachment* fuzzingBus;


/* BYTE */
static uint8_t y = 0;
/* BOOLEAN */
static bool b = true;
/* INT16 */
static int16_t n = 42;
/* UINT16 */
static uint16_t q = 0xBEBE;
/* DOUBLE */
static double d = 3.14159265L;
/* INT32 */
static int32_t i = -9999;
/* INT32 */
static uint32_t u = 0x32323232;
/* INT64 */
static int64_t x = -1LL;
/* UINT64 */
static uint64_t t = 0x6464646464646464ULL;
/* STRING */
static const char* s = "this is a string";
/* OBJECT_PATH */
static const char* o = "/org/foo/bar";
/* SIGNATURE */
static const char* g = "a{is}d(siiux)";

/* Array of BYTE */
static uint8_t ay[] = { 9, 19, 29, 39, 49 };
/* Array of INT16 */
static int16_t an[] = { -9, -99, 999, 9999 };
/* Array of INT32 */
static int32_t ai[] = { -8, -88, 888, 8888 };
/* Array of bool */
static bool ab[] = { true, false, true, true, true, false, true };
/* Array of INT64 */
static int64_t ax[] = { -8, -88, 888, 8888 };
/* Array of UINT64 */
static uint64_t at[] = { 8, 88, 888, 8888 };
/* Array of DOUBLE */
static double ad[] = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0 };
/* Array of STRING */
static const char* as[] = { "one", "two", "three", "four" };
/* Array of OBJECT_PATH */
static const char* ao[] = { "/org/one", "/org/two", "/org/three", "/org/four" };
/* Array of SIGNATURE */
static const char* ag[] = { "s", "sss", "as", "a(iiiiuu)" };

/* Arrays of more than 8 elements */
/* Array of BYTE */
static uint8_t aly[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 30, 46 };
/* Array of INT16 */
static int16_t aln[] = { -9, -99, 999, 9999, -1, -6, 10, -2, 13, 20, 29 };
/* Array of INT32 */
static int32_t ali[] = { -8, -88, 888, 8888, -8, -88, 888, 8888, -8, -88, 888, 8888, -8, -88, 888, 8888, -8, -88, 888, 8888, -8, -88, 888, 8888, -8, -88, 888, 8888 };
/* Array of bool */
static bool alb[] = { true, false, true, true, true, false, true, true, false, true, true, true, false, true };
/* Array of INT64 */
static int64_t alx[] = { -8, -88, 888, 8888, -8, -88, 888, 8888, -8, -88, 888, 8888 };
/* Array of UINT64 */
static uint64_t alt[] = { 8, 88, 888, 8888, 8, 88, 888, 8888, 8, 88, 888, 8888 };
/* Array of DOUBLE */
static double ald[] = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0, 0.001, 0.01, 0.1, 1.0, 10.0, 100.0 };
/* Array of STRING */
static const char* als[] = { "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve" };
/* Array of OBJECT_PATH */
static const char* alo[] = { "/org/one", "/org/two", "/org/three", "/org/four", "/org/five", "/org/six", "/org/seven", "/org/eight", "/org/nine", "/org/ten", "/org/eleven", "/org/twelve", "/org/thirteen" };
/* Array of SIGNATURE */
static const char* alg[] = { "s", "sss", "as", "a(iiiiuu)", "s", "sss", "as", "a(iiiiuu)", "s", "sss", "as", "a(iiiiuu)" };

void Randfuzzing(void* buf, size_t len, uint8_t percent)
{
    uint8_t* p = (uint8_t*)buf;
    if (percent > 100) {
        percent = 100;
    }
    while (len--) {
        if (percent > ((100 * qcc::Rand8()) / 256)) {
            *p = qcc::Rand8();
        }
        ++p;
    }
}

typedef struct {
    char endian;           ///< The endian-ness of this message
    uint8_t msgType;       ///< Indicates if the message is method call, signal, etc.
    uint8_t flags;         ///< Flag bits
    uint8_t majorVersion;  ///< Major version of this message
    uint32_t bodyLen;      ///< Length of the body data
    uint32_t serialNum;    ///< serial of this message
    uint32_t headerLen;    ///< Length of the header fields
} MsgHeader;

static void Fuzz(TestPipe& stream)
{
    uint8_t* fuzzBuf;
    size_t size = stream.AvailBytes();
    size_t offset;

    fuzzBuf = new uint8_t[size];
    stream.PullBytes(fuzzBuf, size, size);
    MsgHeader* hdr = (MsgHeader*)(fuzzBuf);

    uint8_t test = qcc::Rand8() % 16;

    switch (test) {
    case 0:
        /*
         * Protect fixed header from fuzzing
         */
        offset = sizeof(MsgHeader);
        Randfuzzing(fuzzBuf + offset, size - offset, 5);
        break;

    case 1:
        /*
         * Protect entire header from fuzzing
         */
        offset = sizeof(MsgHeader) + hdr->headerLen;
        Randfuzzing(fuzzBuf + offset, size - offset, 5);
        break;

    case 2:
        /*
         * Toggle endianess
         */
        hdr->endian = (hdr->endian == ALLJOYN_BIG_ENDIAN) ? ALLJOYN_LITTLE_ENDIAN : ALLJOYN_BIG_ENDIAN;
        break;

    case 3:
        /*
         * Toggle flag bits
         */
        {
            uint8_t bit = (1 << qcc::Rand8() % 8);
            hdr->flags ^= bit;
        }
        break;

    case 4:
        /*
         * Mess with header len a little
         */
        hdr->headerLen += (qcc::Rand8() % 8) - 4;
        break;

    case 5:
        /*
         * Randomly set header len
         */
        hdr->headerLen = qcc::Rand16() - 0x7FFF;
        break;

    case 6:
        /*
         * Mess with body len a little
         */
        hdr->bodyLen += (qcc::Rand8() % 8) - 4;
        break;

    case 7:
        /*
         * Randomly set body len
         */
        hdr->headerLen = qcc::Rand16() - 0x7FFF;
        break;

    case 8:
        /*
         * Change message type (includes invalid types)
         */
        hdr->msgType = qcc::Rand8() % 6;
        break;

    default:
        /*
         * Fuzz the entire message
         */
        Randfuzzing(fuzzBuf, size, 1 + (qcc::Rand8() % 10));
        break;
    }
    stream.PushBytes(fuzzBuf, size, size);
    delete [] fuzzBuf;
    /*
     * Sometimes append garbage
     */
    if (qcc::Rand8() > 2) {
        size_t len = qcc::Rand8();
        while (len--) {
            uint8_t b = qcc::Rand8();
            stream.PushBytes(&b, 1, size);
        }
    }
}

static qcc::String StripWS(const qcc::String& str)
{
    qcc::String out;
    for (size_t i = 0; i < str.size(); i++) {
        if (!isspace(str[i])) {
            out.push_back(str[i]);
        }
    }
    return out;
}

static qcc::SocketFd MakeHandle()
{
    qcc::SocketFd sock;
    QStatus status = qcc::Socket(QCC_AF_INET, QCC_SOCK_STREAM, sock);
    if (status != ER_OK) {
        EXPECT_EQ(status, ER_OK) << "Actual Status: " << QCC_StatusText(status) << " Failed to create socket";
        return qcc::INVALID_SOCKET_FD;
    } else {
        return sock;
    }
}

/*
 * Marshal and unmarshal an arg list
 */
static QStatus TestMarshal(const MsgArg* argList, size_t numArgs, const char* exception = NULL)
{
    QStatus status;
    TestPipe stream;
    MyMessage msg(*fuzzingBus);
    errString.clear();

    TestPipe* pStream = &stream;
    static const bool falsiness = false;
    RemoteEndpoint ep(*fuzzingBus, falsiness, String::Empty, pStream);
    ep->GetFeatures().handlePassing = true;

    if (numArgs == 0) {
        if (!quiet) {
            printf("Empty arg.v_struct.Elements, arg.v_struct.numElements\n");
        } else {      errString += "Empty arg.v_struct.Elements, arg.v_struct.numElements\n"; }
        return ER_FAIL;
    }

    if (!quiet) {
        printf("++++++++++++++++++++++++++++++++++++++++++++\n");
    } else {  errString += "\n++++++++++++++++++++++++++++++++++++++++++++\n"; }
    qcc::String inargList = MsgArg::ToString(argList, numArgs);
    qcc::String inSig = MsgArg::Signature(argList, numArgs);
    if (!quiet) {
        printf("ArgList:\n%s", inargList.c_str());
    } else if (!bigArray) {
        errString += "ArgList:\n" + static_cast<qcc::String>(inargList.c_str());
    }

    status = msg.MethodCall("desti.nation", "/foo/bar", "foo.bar", "test", argList, numArgs);
    if (!quiet) {
        printf("MethodCall status:%s\n", QCC_StatusText(status));
    } else {  errString += "MethodCall status: " + static_cast<qcc::String>(QCC_StatusText(status)) + "\n"; }
    if (status != ER_OK) {
        return status;
    }
    status = msg.Deliver(ep);
    if (status != ER_OK) {
        return status;
    }

    if (fuzzing) {
        Fuzz(stream);
    }

    status = msg.Read(ep, ":88.88");
    if (status != ER_OK) {
        if (!quiet) {
            printf("Message::Read status:%s\n", QCC_StatusText(status));
        } else {      errString += "Message::Read status : " + static_cast<qcc::String>(QCC_StatusText(status)) + "\n"; }
        return status;
    }

    status = msg.Unmarshal(ep, ":88.88");
    if (status != ER_OK) {
        if (!quiet) {
            printf("Message::Unmarshal status:%s\n", QCC_StatusText(status));
        } else {      errString += "Message::Unmarshal status : " + static_cast<qcc::String>(QCC_StatusText(status)) + "\n"; }
        return status;
    }
    status = msg.UnmarshalBody();
    if (status != ER_OK) {
        if (!quiet) {
            printf("Message::UnmarshalArgs status:%s\n", QCC_StatusText(status));
        } else {      errString += "Message::UnmarshalArgs status : " + static_cast<qcc::String>(QCC_StatusText(status)) + "\n"; }
        return status;
    }
    msg.GetArgs(numArgs, argList);
    qcc::String outargList = MsgArg::ToString(argList, numArgs);
    qcc::String outSig = MsgArg::Signature(argList, numArgs);
    if (!quiet) {
        printf("--------------------------------------------\n");
    } else {  errString += "--------------------------------------------\n"; }
    if (inargList == outargList) {
        if (!quiet) {
            printf("outargList == inargList\n");
        } else {      errString += "outargList == inargList\n"; }
    } else if (exception && (StripWS(outargList) == StripWS(exception))) {
        if (!quiet) {
            printf("outargList == exception\n%s\n", exception);
        } else {      errString += "outargList == exception\n" + static_cast<qcc::String>(exception) + "\n"; }
    } else if (exception && (strcmp(exception, "*") == 0) && (inSig == outSig)) {
        if (!quiet) {
            printf("Unmarshal: hand compare:\n%s\n%s\n", inargList.c_str(), outargList.c_str());
        } else {      errString += "Unmarshal: hand compare:\n" + static_cast<qcc::String>(inargList.c_str()) +
                                   "\n" + static_cast<qcc::String>(outargList.c_str()) + "\n"; }
    } else {
        if (!quiet) {
            printf("FAILED\n");
        } else {      errString += "FAILED\n"; }
        if (!quiet) {
            printf("Unmarshal: %u argList\n%s\n", static_cast<uint32_t>(numArgs), outargList.c_str());
        } else {      errString += "Unmarshal: argList\n" + static_cast<qcc::String>(outargList.c_str()) + "\n"; }
        status = ER_FAIL;
    }
    return status;
}

/*
 * When running the Fuzzing test any of the following errors could be returned
 * and are expected.
 */
bool foundExpectedFuzzingStatus(QStatus status) {
    return (ER_OK == status ||
            ER_FAIL == status ||
            ER_TIMEOUT == status ||
            ER_BUS_BAD_VALUE == status ||
            ER_BUS_BAD_LENGTH == status ||
            ER_BUS_BAD_HEADER_FIELD == status ||
            ER_BUS_BAD_HEADER_LEN == status ||
            ER_BUS_BAD_BODY_LEN == status ||
            ER_BUS_BAD_SIGNATURE == status ||
            ER_BUS_BAD_VALUE_TYPE == status ||
            ER_BUS_HANDLES_MISMATCH == status ||
            ER_BUS_INVALID_HEADER_SERIAL == status ||
            ER_BUS_MEMBER_MISSING == status ||
            ER_BUS_NO_SUCH_HANDLE == status ||
            ER_BUS_PATH_MISSING == status ||
            ER_BUS_REPLY_SERIAL_MISSING == status ||
            ER_BUS_ERROR_NAME_MISSING == status ||
            ER_BUS_NOT_AUTHORIZED == status ||
            ER_BUS_NOT_NUL_TERMINATED == status ||
            ER_BUS_MISSING_COMPRESSION_TOKEN == status
            );
}
QStatus MarshalTests()
{
    QStatus status = ER_OK;

    /*
     * Test cases using MsgArg constructors
     */
    if (fuzzing || (status == ER_OK)) {
        errString.clear();
        MsgArg arg("i", 1);
        status = TestMarshal(&arg, 1);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg arg("s", "hello");
        status = TestMarshal(&arg, 1);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg hello("s", "hello");
        MsgArg goodbye("s", "goodbyte");
        MsgArg argList[] = { hello, hello, hello, goodbye };
        status = TestMarshal(argList, ArraySize(argList));
        if (!fuzzing) {
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
        } else {
            EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
        }

        /* Zero length string */
        MsgArg arg("s", "");
        status = TestMarshal(&arg, 1);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Dynamic construction of an array of integers
     */
    if (fuzzing || (status == ER_OK)) {
        static const char* result = "<array type=\"int32\">0 1 2 3 4 5 6 7 8 9</array>";
        MsgArg arg(ALLJOYN_ARRAY);
        size_t numElements = 10;
        MsgArg* elements = new MsgArg[numElements];
        for (size_t i = 0; i < numElements; i++) {
            elements[i].typeId = ALLJOYN_INT32;
            elements[i].v_int32 = i;
        }
        status = arg.v_array.SetElements("i", numElements, elements);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1, result);
        }
        arg.SetOwnershipFlags(MsgArg::OwnsArgs);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Dynamic construction of a dictionary
     */
    if (fuzzing || (status == ER_OK)) {
        const char* keys[] = { "red", "green", "blue" };
        const uint32_t values[] = { 21, 45, 245 };
        MsgArg dict(ALLJOYN_ARRAY);
        size_t numEntries = ArraySize(keys);
        MsgArg* entries = new MsgArg[ArraySize(keys)];
        for (size_t i = 0; i < numEntries; i++) {
            entries[i].typeId = ALLJOYN_DICT_ENTRY;
            entries[i].v_dictEntry.key = new MsgArg("s", keys[i]);
            entries[i].v_dictEntry.val = new MsgArg("v", new MsgArg("u", values[i]));
        }
        status = dict.v_array.SetElements("{sv}", numEntries, entries);
        if (status == ER_OK) {
            status = TestMarshal(&dict, 1);
        }
        dict.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Dynamic construction of an array of dictionaries
     */
    if (fuzzing || (status == ER_OK)) {
        const char* keys[] = { "yellow", "cyan", "magenta" };
        const uint32_t values[] = { 29, 63, 12 };
        MsgArg arry(ALLJOYN_ARRAY);

        size_t numDicts = 1;
        MsgArg* dicts = new MsgArg[numDicts];

        for (size_t d = 0; d < numDicts; d++) {
            size_t numEntries = ArraySize(keys);
            MsgArg* entries = new MsgArg[numEntries];
            for (size_t e = 0; e < numEntries; e++) {
                entries[e].typeId = ALLJOYN_DICT_ENTRY;
                entries[e].v_dictEntry.key = new MsgArg("s", keys[e]);
                entries[e].v_dictEntry.val = new MsgArg("v", new MsgArg("u", values[e]));
            }
            dicts[d].typeId = ALLJOYN_ARRAY;
            status = dicts[d].v_array.SetElements("{sv}", numEntries, entries);
            if (status != ER_OK) {
                delete [] entries;
                break;
            }
        }
        if (status == ER_OK) {
            status = arry.v_array.SetElements("a{sv}", numDicts, dicts);
        }
        if (status == ER_OK) {
            arry.SetOwnershipFlags(MsgArg::OwnsArgs, true);
            status = TestMarshal(&arry, 1);
        } else {
            delete [] dicts;
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Test cases using the varargList constructor. Note some of these test cases use the trick of wrapping an argument list
     * in a struct so multiple MsgArgs can be initialized in one call to Set().
     */
    /*
     * Simple types
     */
    if (fuzzing || (status == ER_OK)) {
        MsgArg argList;
        status = argList.Set("(ybnqdiuxtsoqg)", y, b, n, q, d, i, u, x, t, s, o, q, g);
        if (status == ER_OK) {
            status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Arrays
     */
    if (fuzzing || (status == ER_OK)) {
        MsgArg argList;
        status = argList.Set("(aias)", ArraySize(ai), ai, ArraySize(as), as);
        if (status == ER_OK) {
            status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg arg;
        status = arg.Set("ad", ArraySize(ad), ad);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg argList;
        status = argList.Set("(ayad)", ArraySize(ay), ay, ArraySize(ad), ad);
        if (status == ER_OK) {
            status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg argList;
        status = argList.Set("(anax)", ArraySize(an), an, ArraySize(ax), ax);
        if (status == ER_OK) {
            status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg argList;
        status = argList.Set("(agao)", ArraySize(ag), ag, ArraySize(ao), ao);
        if (status == ER_OK) {
            status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg var("s", "hello");
        MsgArg struc;
        struc.Set("(yv)", 128, &var);
        MsgArg arg;
        status = arg.Set("a(yv)", (size_t)1, &struc);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Arrays of arrays
     */
    if (fuzzing || (status == ER_OK)) {
        MsgArg inner[2];
        inner[0].Set("ai", ArraySize(ai), ai, ArraySize(ai) - 2, ai);
        inner[1].Set("ai", ArraySize(ai) - 2, ai);
        MsgArg arg;
        status = arg.Set("aai", ArraySize(inner), inner);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        const char* ay1 = "foo";
        const char* ay2 = "bar";
        MsgArg inner[2];
        inner[0].Set("ay", strlen(ay1), ay1);
        inner[1].Set("ay", strlen(ay2), ay2);
        MsgArg arg;
        status = arg.Set("aay", ArraySize(inner), inner);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        static const char* result =
            "<array type_sig=\"ay\">"
            "  <array type=\"byte\">102 111 111</array>"
            "  <array type=\"byte\">98 97 114</array>"
            "  <array type=\"byte\">103 111 114 110</array>"
            "  <array type=\"byte\">121 111 119 122 101 114</array>"
            "  <array type=\"byte\">98 105 110 103 111</array>"
            "</array>";

        const char* l[] = { "foo", "bar", "gorn", "yowzer", "bingo" };
        MsgArg* outer = new MsgArg[ArraySize(l)];
        for (size_t i = 0; i < ArraySize(l); i++) {
            MsgArg* inner = new MsgArg[strlen(l[i])];
            for (size_t j = 0; j < strlen(l[i]); j++) {
                inner[j].typeId = ALLJOYN_BYTE;
                inner[j].v_byte = l[i][j];
            }
            outer[i].typeId = ALLJOYN_ARRAY;
            outer[i].v_array.SetElements("y", strlen(l[i]), inner);
        }
        MsgArg arg;
        status = arg.Set("aay", ArraySize(l), outer);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1, result);
        }
        arg.SetOwnershipFlags(MsgArg::OwnsArgs, true);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        const char* l[] = { "aristole", "plato", "socrates" };
        MsgArg* ayay = new MsgArg[ArraySize(l)];
        for (size_t i = 0; i < ArraySize(l); i++) {
            ayay[i].Set("ay", strlen(l[i]), l[i]);
        }
        MsgArg arg;
        status = arg.Set("aay", ArraySize(l), ayay);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
        delete [] ayay;
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        static const char* result =
            "<array type_sig=\"as\">"
            "  <string>apple</string>"
            "  <string>orange</string>"
            "  <string>pear</string>"
            "  <string>grape</string>"
            "</array>";
        MsgArg strings[] = { MsgArg("s", "apple"), MsgArg("s", "orange"), MsgArg("s", "pear"), MsgArg("s", "grape") };
        MsgArg arg;
        status = arg.Set("a*", ArraySize(strings), strings);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1, result);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Zero-length arrays of scalars
     */
    if (fuzzing || (status == ER_OK)) {
        MsgArg argList;
        status = argList.Set("(aiayadax)", (size_t)0, ai, (size_t)0, ay, (size_t)0, ad, (size_t)0, ax);
        if (status == ER_OK) {
            status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Zero-length arrays
     */
    if (fuzzing || (status == ER_OK)) {
        MsgArg arg;
        status = arg.Set("a(ssiv)", (size_t)0, NULL);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg empty("a(ii)", (size_t)0, NULL);
        MsgArg var("v", &empty);
        MsgArg arg;
        status = arg.Set("av", 1, &var);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg arg;
        status = arg.Set("a{yy}", (size_t)0, NULL);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg arg;
        status = arg.Set("a{yy}", (size_t)0, NULL);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Empty strings
     */
    if (fuzzing || (status == ER_OK)) {
        MsgArg args[2];
        args[0].Set("s", NULL);
        args[1].Set("g", NULL);
        if (status == ER_OK) {
            status = TestMarshal(args, 2);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }

    /*
     * Directly set array arg fields.
     */
    if (fuzzing || (status == ER_OK)) {
        MsgArg arg;
        uint8_t data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        arg.typeId = ALLJOYN_BYTE_ARRAY;
        arg.v_scalarArray.numElements = 10;
        arg.v_scalarArray.v_byte = data;
        status = TestMarshal(&arg, 1);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg arg;
        arg.typeId = ALLJOYN_DOUBLE_ARRAY;
        arg.v_scalarArray.numElements = ArraySize(ad);
        arg.v_scalarArray.v_double = ad;
        status = TestMarshal(&arg, 1);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg arg;
        arg.typeId = ALLJOYN_BOOLEAN_ARRAY;
        arg.v_scalarArray.numElements = ArraySize(ab);
        arg.v_scalarArray.v_bool = ab;
        status = TestMarshal(&arg, 1);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Structs
     */
    if (fuzzing || (status == ER_OK)) {
        MsgArg args[2];
        status = args[0].Set("s", "hello");
        status = args[1].Set("(qqq)", q, q, q);
        if (status == ER_OK) {
            status = TestMarshal(args, 2);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg argList;
        status = argList.Set("((ydx)(its))", y, d, x, i, t, s);
        if (status == ER_OK) {
            status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg arg;
        status = arg.Set("((iuiu)(yd)atab)", i, u, i, u, y, d, ArraySize(at), at, ArraySize(ab), ab);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }

    if (fuzzing || (status == ER_OK)) {
        MsgArg hello("s", "hello");
        MsgArg world("(si)", "world", 999);
        MsgArg arg("(**)", &hello, &world);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }

    if (fuzzing || (status == ER_OK)) {
        MsgArg argList;
        status = argList.Set("as", ArraySize(als), als);
        MsgArg arg;
        status = arg.Set("*", &argList);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Dictionary
     */
    if (fuzzing || (status == ER_OK)) {
        struct {
            uint32_t num;
            const char* ord;
            bool even;
        } table[] = { { 1, "first", true }, { 2, "second", false }, { 3, "third", true } };
        MsgArg dict[ArraySize(table)];
        for (size_t i = 0; i < ArraySize(table); i++) {
            dict[i].Set("{s(ib)}", table[i].ord, table[i].num, table[i].even);
        }
        MsgArg arg;
        status = arg.Set("a{s(ib)}", (size_t)3, dict);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        const char* str[] = { "first", "second", "third" };
        MsgArg dict[ArraySize(str)];
        for (size_t i = 0; i < ArraySize(dict); i++) {
            MsgArg* var = new MsgArg("v", new MsgArg("u", i));
            dict[i].Set("{sv}", str[i], var);
            dict[i].SetOwnershipFlags(MsgArg::OwnsArgs, true);
        }
        MsgArg dicts[2];
        dicts[0].Set("a{sv}", (size_t)3, dict);
        dicts[1].Set("a{sv}", (size_t)2, dict);
        MsgArg arg;
        status = arg.Set("aa{sv}", ArraySize(dicts), dicts);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Variants
     */
    if (fuzzing || (status == ER_OK)) {
        MsgArg val("u", 3);
        MsgArg arg;
        status = arg.Set("v", &val);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg arry;
        arry.Set("ai", ArraySize(ai), ai);
        MsgArg arg;
        status = arg.Set("v", &arry);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg dub;
        dub.Set("d", d);
        MsgArg struc;
        struc.Set("(ybv)", y, b, &dub);
        MsgArg arg;
        status = arg.Set("v", &struc);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Extreme test case
     */
    if (fuzzing || (status == ER_OK)) {
        struct {
            size_t num;
            const char* nom;
        } beasts[] = { { 1, "dog" }, { 2, "cat" }, { 3, "pig" }, { 4, "rat" }, { 5, "cow" } };
        MsgArg dict[ArraySize(beasts)];
        for (size_t i = 0; i < ArraySize(beasts); i++) {
            dict[i].Set("{is}", beasts[i].num, beasts[i].nom);
        }
        MsgArg beastArray;
        beastArray.Set("a{is}", ArraySize(dict), dict);

        MsgArg arg;
        status = arg.Set("(tidbsy(n(no)ai)gvasd)", t, 1, d, true, "hello world", 0xFF, 2, 3, "/path", ArraySize(ai), ai, "signatu",
                         &beastArray,
                         ArraySize(as), as, d);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * inner arrays
     */
    if (fuzzing || (status == ER_OK)) {
        const char* breeds[] = { "labrador", "poodle", "mutt" };
        MsgArg dogs;
        dogs.Set("(sas)", "dogs", ArraySize(breeds), breeds);
        MsgArg arg;
        status = arg.Set("a(sas)", (size_t)1, &dogs);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        MsgArg dogs;
        dogs.Set("(sas)", "no dogs here", NULL);
        MsgArg arg;
        status = arg.Set("a(sas)", (size_t)1, &dogs);
        if (status == ER_OK) {
            status = TestMarshal(&arg, 1);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Handles
     */
    if (fuzzing || (status == ER_OK)) {
        qcc::SocketFd handle = MakeHandle();
        MsgArg arg("h", handle);
        status = TestMarshal(&arg, 1, "*");
        qcc::Close(handle);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        qcc::SocketFd h1 = MakeHandle();
        qcc::SocketFd h2 = MakeHandle();
        qcc::SocketFd h3 = MakeHandle();
        MsgArg args[3];
        size_t numArgs = ArraySize(args);
        MsgArg::Set(args, numArgs, "hhh", h1, h2, h3);
        status = TestMarshal(args, ArraySize(args), "*");
        qcc::Close(h1);
        qcc::Close(h2);
        qcc::Close(h3);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        qcc::SocketFd h1 = MakeHandle();
        qcc::SocketFd h2 = MakeHandle();
        qcc::SocketFd h3 = MakeHandle();
        MsgArg arg("(shshsh)", "first handle", h1, "second handle", h2, "third handle", h3);
        status = TestMarshal(&arg, 1, "*");
        qcc::Close(h1);
        qcc::Close(h2);
        qcc::Close(h3);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        qcc::SocketFd h[8];
        MsgArg handles[8];
        for (size_t i = 0; i < ArraySize(handles); ++i) {
            h[i] = MakeHandle();
            handles[i].Set("h", h[i]);
        }
        MsgArg arg("ah", ArraySize(handles), handles);
        status = TestMarshal(&arg, 1, "*");
        for (size_t i = 0; i < ArraySize(handles); ++i) {
            qcc::Close(h[i]);
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    if (fuzzing || (status == ER_OK)) {
        qcc::SocketFd handle = MakeHandle();
        MsgArg h("h", handle);
        MsgArg arg("(ivi)", 999, &h, 666);
        status = TestMarshal(&arg, 1, "*");
        qcc::Close(handle);
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    /*
     * Maximum array size 2^17 - last test case because it takes so long
     */
    if (status == ER_OK) {
        if (!nobig) {
            /* Force quiet so we don't print 128MBytes of output data */
            bool wasQuiet = quiet;
            quiet = true;
            bigArray = true;
            const size_t max_array_size = ALLJOYN_MAX_ARRAY_LEN;
            uint8_t* big = new uint8_t[max_array_size];
            MsgArg arg;
            status = arg.Set("ay", max_array_size, big);
            if (status == ER_OK) {
                status = TestMarshal(&arg, 1);
            }
            delete [] big;
            quiet = wasQuiet;
            bigArray = false;
        }
    }
    if (!fuzzing) {
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    } else {
        EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    }
    return status;
}

TEST(MarshalTest, noFuzzing) {
    fuzzingBus = new BusAttachment("TestMsgUnPack", false);
    fuzzingBus->Start();

    fuzzing = false;
    nobig = true;
    quiet = true;
    QStatus status = MarshalTests();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    fuzzingBus->Stop();
    fuzzingBus->Join();
    delete fuzzingBus;
    fuzzingBus = NULL;
}
TEST(MarshalTest, fuzzing) {
    printf("The Fuzzing tests are meant to run thousands of times.\n");
    printf("To run this test multiple times add commandline option:\n");
    printf("\t--gtest_filter=MarshalTest.fuzzing --gtest_repeat=[COUNT]\n");
    printf("\nNOTE: This Test Produces a large amount of errors.\n\n");
    fuzzingBus = new BusAttachment("TestMsgUnPack", false);
    fuzzingBus->Start();
    fuzzing = true;
    nobig = true;
    quiet = true;
    QStatus status;
    status = MarshalTests();
    EXPECT_TRUE(foundExpectedFuzzingStatus(status)) << "Actual Status: " << QCC_StatusText(status) << errString.c_str();
    fuzzingBus->Stop();
    fuzzingBus->Join();
    delete fuzzingBus;
    fuzzingBus = NULL;

}
TEST(MarshalTest, TestBigArrays) {
    /* Marshal unmarshal arrays bigger than 8 elements */
    fuzzingBus = new BusAttachment("TestMsgUnPack", false);
    fuzzingBus->Start();
    fuzzing = false;
    nobig = true;
    quiet = true;

    MsgArg argList;
    QStatus status = argList.Set("(ayad)", ArraySize(aly), aly, ArraySize(ald), ald);
    if (status == ER_OK) {
        status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
    }
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();

    status = argList.Set("(anax)", ArraySize(aln), aln, ArraySize(alx), alx);
    if (status == ER_OK) {
        status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
    }
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();

    status = argList.Set("(aias)", ArraySize(ali), ali, ArraySize(als), als);
    if (status == ER_OK) {
        status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
    }
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();

    MsgArg inner[2];
    inner[0].Set("ai", ArraySize(ali), ali, ArraySize(ali) - 2, ali);
    inner[1].Set("ai", ArraySize(ali) - 2, ali);
    MsgArg arg;
    status = arg.Set("aai", ArraySize(inner), inner);
    if (status == ER_OK) {
        status = TestMarshal(&arg, 1);
    }
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    MsgArg arry;
    arry.Set("ai", ArraySize(ali), ali);

    status = arg.Set("v", &arry);
    if (status == ER_OK) {
        status = TestMarshal(&arg, 1);
    }
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();

    status = arg.Set("((iuiu)(yd)atab)", i, u, i, u, y, d, ArraySize(alt), alt, ArraySize(alb), alb);
    if (status == ER_OK) {
        status = TestMarshal(&arg, 1);
    }
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();

    status = arg.Set("ad", ArraySize(ald), ald);
    if (status == ER_OK) {
        status = TestMarshal(&arg, 1);
    }
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();

    status = argList.Set("(aias)", ArraySize(ali), ali, ArraySize(als), als);
    if (status == ER_OK) {
        status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
    }
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();

    status = argList.Set("(agao)", ArraySize(alg), alg, ArraySize(alo), alo);
    if (status == ER_OK) {
        status = TestMarshal(argList.v_struct.members, argList.v_struct.numMembers);
    }
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();

    MsgArg var1("s", "hello1");
    MsgArg var2("s", "hello2");
    MsgArg var3("s", "hello3");
    MsgArg var4("s", "hello4");
    MsgArg var5("s", "hello5");
    MsgArg var6("s", "hello6");
    MsgArg var7("s", "hello7");
    MsgArg var8("s", "hello8");
    MsgArg var9("s", "hello9");
    MsgArg var10("s", "hello10");


    MsgArg struc[10];
    struc[0].Set("(yv)", 1, &var1);
    struc[1].Set("(yv)", 2, &var2);
    struc[2].Set("(yv)", 3, &var3);
    struc[3].Set("(yv)", 4, &var4);
    struc[4].Set("(yv)", 5, &var5);
    struc[5].Set("(yv)", 6, &var6);
    struc[6].Set("(yv)", 7, &var7);
    struc[7].Set("(yv)", 8, &var8);
    struc[8].Set("(yv)", 9, &var9);
    struc[9].Set("(yv)", 10, &var10);

    status = arg.Set("a(yv)", (size_t)10, &struc[0]);
    if (status == ER_OK) {
        status = TestMarshal(&arg, 1);
    }
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();
    const char* breeds[] = { "labrador", "poodle", "mutt", "pomeranian", "porcelaine", "pug", "talbot", "german-shepard", "chihuahua" };
    MsgArg dogs[10];
    dogs[0].Set("(sas)", "dogs0", ArraySize(breeds), breeds);
    dogs[1].Set("(sas)", "dogs1", ArraySize(breeds), breeds);
    dogs[2].Set("(sas)", "dogs2", ArraySize(breeds), breeds);
    dogs[3].Set("(sas)", "dogs3", ArraySize(breeds), breeds);
    dogs[4].Set("(sas)", "dogs4", ArraySize(breeds), breeds);
    dogs[5].Set("(sas)", "dogs5", ArraySize(breeds), breeds);
    dogs[6].Set("(sas)", "dogs6", ArraySize(breeds), breeds);
    dogs[7].Set("(sas)", "dogs7", ArraySize(breeds), breeds);
    dogs[8].Set("(sas)", "dogs8", ArraySize(breeds), breeds);
    dogs[9].Set("(sas)", "dogs9", ArraySize(breeds), breeds);

    status = arg.Set("a(sas)", (size_t)10, &dogs);
    if (status == ER_OK) {
        status = TestMarshal(&arg, 1);
    }

    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << errString.c_str();

    fuzzingBus->Stop();
    fuzzingBus->Join();
    delete fuzzingBus;
    fuzzingBus = NULL;

}
/*--------------------------FUZZING TEST CODE---------------------------------*/



