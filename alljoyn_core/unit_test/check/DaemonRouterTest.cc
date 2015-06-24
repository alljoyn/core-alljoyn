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
#include <qcc/platform.h>

#include <set>
#include <list>
#include <vector>

#include <qcc/StringUtil.h>
#include "AllJoynObj.h"
#include "SessionlessObj.h"
#include "DaemonRouter.h"
#include "ConfigDB.h"

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "../ajTestCommon.h"

#if GTEST_HAS_COMBINE

/********************************************************************************
 * Bring labels into the global namespace for convenience.
 ********************************************************************************/
using namespace std;
using namespace qcc;
using namespace ajn;

using testing::TestWithParam;
using testing::Values;
using testing::ValuesIn;
using testing::Bool;
using testing::Combine;

/********************************************************************************
 * Test globals
 ********************************************************************************/
static const char* TEST_SIGNATURE = "";
static const char* TEST_OBJPATH = "/";
static const char* TEST_IFACE = "org.allseen.DaemonRouterTest";
static const String TEST_MEMBER = "DaemonRouterTest";
static const String TEST_MEMBER_SENDER_DENIED = "SenderDenied";
static const String TEST_MEMBER_RECEIVER_DENIED = "ReceiverDenied";
static const String TEST_ERROR = "org.allseen.DaemonRouterTest.Error";
static const String TEST_ERROR_SENDER_DENIED = "org.allseen.DaemonRouterTest.Error.SenderDenied";
static const String TEST_ERROR_RECEIVER_DENIED = "org.allseen.DaemonRouterTest.Error.ReceiverDenied";
static const SessionId TEST_SESSION_ID = 5;

static const char* CONFIG_STR =
    "<busconfig>"
    "  <policy context=\"mandatory\">"
    "    <allow send_error = \"org.allseen.DaemonRouterTest.Error\" send_type=\"error\"/>"
    "    <deny send_error = \"org.allseen.DaemonRouterTest.Error.SenderDenied\" send_type=\"error\"/>"
    "    <deny receive_error = \"org.allseen.DaemonRouterTest.Error.ReceiverDenied\" receive_type=\"error\"/>"
    "    <allow send_member = \"DaemonRouterTest\" send_type=\"method_call\"/>"
    "    <deny send_member = \"SenderDenied\" send_type=\"method_call\"/>"
    "    <deny receive_member = \"ReceiverDenied\" receive_type=\"method_call\"/>"
    "    <allow send_member = \"DaemonRouterTest\" send_type=\"signal\"/>"
    "    <deny send_member = \"SenderDenied\" send_type=\"signal\"/>"
    "    <deny receive_member = \"ReceiverDenied\" receive_type=\"signal\"/>"
    "  </policy>"
    "</busconfig>";

/********************************************************************************
 * Test Stub Classes
 ********************************************************************************/
/*
 * This is called by the PushMessage() method of all TestEndpoints.  It
 * primarily checks if the message that was received by the endpoint is in fact
 * expected.  It also checks for certain kinds of bugs that are possible in the
 * test code itself.  If there is a test code bug then it will return
 * ER_INVALID_DATA as an indication to the real test case function.
 */
static QStatus TestPushMessage(Message& msg, const BusEndpoint& ep, bool slsRoute = false, bool slsPush = false);


namespace {
/*
 * Enumeration of messages flags used in this test.  This allows for
 * pretty printing the flag names rather than a number in gtest
 * output.
 */
enum TestMessageFlags {
    MF_NONE = 0,
    MF_SESSIONLESS = ALLJOYN_FLAG_SESSIONLESS,
    MF_NO_REPLY_EXPECTED = ALLJOYN_FLAG_NO_REPLY_EXPECTED,
    MF_GLOBAL_BROADCAST = ALLJOYN_FLAG_GLOBAL_BROADCAST
};


/*
 * ostream formatter for TestMessageFlags.
 */
static ostream& operator<<(ostream& os, const TestMessageFlags& flag)
{
    // Extra WS courtesy of uncrustify
    switch (flag) {
    case MF_NONE:              return os << "NONE";

    case MF_SESSIONLESS:       return os << "SESSIONLESS";

    case MF_NO_REPLY_EXPECTED: return os << "NO_REPLY_EXPECTED";

    case MF_GLOBAL_BROADCAST:  return os << "GLOBAL_BROADCAST";
    }
    return os;
}

/*
 * Test case flags for different signal delivery tests.
 */
enum TestSignalFlags {
    SF_NONE,
    SF_SLS_ONLY,
    SF_SELF_JOIN
};

/*
 * ostream formatter for TestSignalFlags.
 */
static ostream& operator<<(ostream& os, const TestSignalFlags& flag)
{
    // Extra WS courtesy of uncrustify
    switch (flag) {
    case SF_NONE:      return os << "NONE";

    case SF_SLS_ONLY:  return os << "SLS_ONLY";

    case SF_SELF_JOIN: return os << "SELF_JOIN";
    }
    return os;
}

/*
 * Class that contains information about endpoints.  This serves as both a base
 * class for the derived test endpoints as well as one of the test case
 * parameters about the sender endpoint.
 */
class _TestEndpointInfo {
  public:
    _TestEndpointInfo() : name(""), type(ENDPOINT_TYPE_INVALID) { }
    _TestEndpointInfo(String& name, EndpointType type, SessionId id, bool allow, bool slsMatchRule) :
        name(name),
        type(type),
        id(id),
        allow(allow),
        slsMatchRule(slsMatchRule)
    { }
    _TestEndpointInfo(const _TestEndpointInfo& other) :
        name(other.name),
        type(other.type),
        id(other.id),
        allow(other.allow),
        slsMatchRule(other.slsMatchRule)
    { }
    virtual ~_TestEndpointInfo() { }
    bool operator==(const _TestEndpointInfo& other)
    {
        return ((type == other.type) &&
                (id == other.id) &&
                (allow == other.allow) &&
                (slsMatchRule == other.slsMatchRule) &&
                (name == other.name));
    }
    String name;
    EndpointType type;
    SessionId id;
    bool allow;
    bool slsMatchRule;
};
typedef ManagedObj<_TestEndpointInfo> TestEndpointInfo;


/*
 * This is a specialized version of Message that contains additional information
 * for testing.  This includes the expected message recipients as well as the
 * original message type in the event that DaemonRouter::PushMessage()
 * automagically replies with its own error message.
 */
class _TestMessage : public _Message {
  public:
    _TestMessage(BusAttachment& bus, String memberName, String errorName,
                 AllJoynMessageType type, String sender, String dest, SessionId id, uint8_t flags) :
        _Message(bus), type(type)
    {
        if (type == MESSAGE_SIGNAL) {
            const char* sDest = dest.empty() ? NULL : dest.c_str();
            SignalMsg(TEST_SIGNATURE, sender, sDest, id, TEST_OBJPATH, TEST_IFACE, memberName, NULL, 0, flags, 0);
        } else {
            bool isMethodCall = (type == MESSAGE_METHOD_CALL);
            String& cSender = isMethodCall ? sender : dest;
            String& cDest = isMethodCall ? dest : sender;
            CallMsg(TEST_SIGNATURE, cSender, cDest, id, TEST_OBJPATH, TEST_IFACE, memberName, NULL, 0, flags);
            Message call = Message::wrap(this);
            if (type == MESSAGE_METHOD_RET) {
                ReplyMsg(call, NULL, 0);
            } else if (type == MESSAGE_ERROR) {
                ErrorMsg(call, errorName.c_str(), "Test Error Message");
            }
        }
    }

    void AddNormalRx(BusEndpoint ep) { normalRx.insert(ep); }
    bool RemoveNormalRx(BusEndpoint ep) { return normalRx.erase(ep) > 0; }
    size_t NormalRxCount() const { return normalRx.size(); }
    set<BusEndpoint>& GetNormalRxSet() { return normalRx; }

    void AddErrorRx(BusEndpoint ep) { errorRx.insert(ep); }
    bool RemoveErrorRx(BusEndpoint ep) { return errorRx.erase(ep) > 0; }
    size_t ErrorRxCount() const { return errorRx.size(); }
    set<BusEndpoint>& GetErrorRxSet() { return errorRx; }

    void AddSlsRxRoute(BusEndpoint ep) { slsRxRoute.insert(ep); }
    bool RemoveSlsRxRoute(BusEndpoint ep) { return slsRxRoute.erase(ep) > 0; }
    size_t SlsRxRouteCount() const { return slsRxRoute.size(); }
    set<BusEndpoint>& GetSlsRxRouteSet() { return slsRxRoute; }

    void AddSlsRxPush(BusEndpoint ep) { slsRxPush.insert(ep); }
    bool RemoveSlsRxPush(BusEndpoint ep) { return slsRxPush.erase(ep) > 0; }
    size_t SlsRxPushCount() const { return slsRxPush.size(); }
    set<BusEndpoint>& GetSlsRxPushSet() { return slsRxPush; }

    AllJoynMessageType GetOrigType() const { return type; }

  private:
    set<BusEndpoint> normalRx;
    set<BusEndpoint> errorRx;
    set<BusEndpoint> slsRxRoute;
    set<BusEndpoint> slsRxPush;
    AllJoynMessageType type;
};
typedef ManagedObj<_TestMessage> TestMessage;

/*
 * Test override of BusEndpoint.  This is primarily used for Null endpoints.
 */
class _TestEndpoint : public _BusEndpoint, public _TestEndpointInfo {
  public:
    _TestEndpoint() { }
    _TestEndpoint(const TestEndpointInfo& epInfo) :
        _BusEndpoint(epInfo->type),
        _TestEndpointInfo(*epInfo.unwrap())
    { }
    virtual ~_TestEndpoint() { }
    QStatus PushMessage(Message& msg) { return TestPushMessage(msg, BusEndpoint::wrap(this)); }
    const String& GetUniqueName() const { return name; }
    bool AllowRemoteMessages() { return allow; }
};
typedef ManagedObj<_TestEndpoint> TestEndpoint;

/*
 * Test override of LocalEndpoint.
 */
class _TestLocalEndpoint : public _LocalEndpoint, public _TestEndpointInfo {
  public:
    _TestLocalEndpoint() { }
    _TestLocalEndpoint(BusAttachment& bus, String uniqueName) :
        _LocalEndpoint(bus, 1),
        _TestEndpointInfo(uniqueName, ENDPOINT_TYPE_LOCAL, 0, true, false)
    {
        _BusEndpoint::endpointType = ENDPOINT_TYPE_LOCAL;
        _BusEndpoint::isValid = true;
    }
    virtual ~_TestLocalEndpoint() { }
    QStatus PushMessage(Message& msg) { return TestPushMessage(msg, BusEndpoint::wrap(this)); }
    const String& GetUniqueName() const { return name; }
};
typedef ManagedObj<_TestLocalEndpoint> TestLocalEndpoint;

/*
 * Test override of RemoteEndpoint.  This is used for Remote endpoints and
 * Bus2Bus endpoints.
 */
class _TestRemoteEndpoint : public _RemoteEndpoint, public _TestEndpointInfo {
  public:
    _TestRemoteEndpoint() { }
    _TestRemoteEndpoint(const TestEndpointInfo& epInfo) :
        _RemoteEndpoint(),
        _TestEndpointInfo(*epInfo.unwrap())
    {
        _BusEndpoint::endpointType = type;
        _BusEndpoint::isValid = true;
    }
    virtual ~_TestRemoteEndpoint() { }
    QStatus PushMessage(Message& msg) { return TestPushMessage(msg, BusEndpoint::wrap(this)); }
    const String& GetUniqueName() const { return name; }
    const String& GetRemoteName() const { return remoteName; }
    void SetRemoteName(const String& name) { remoteName = name; }
    bool AllowRemoteMessages() { return allow; }
    uint32_t GetSessionId() const { return (uint32_t)id; }
  private:
    String remoteName;
};
typedef ManagedObj<_TestRemoteEndpoint> TestRemoteEndpoint;

/*
 * Test override of VirtualEndpoint.
 */
class _TestVirtualEndpoint : public _VirtualEndpoint, public _TestEndpointInfo {
  public:
    _TestVirtualEndpoint() { }
    _TestVirtualEndpoint(RemoteEndpoint& remoteEp, const TestEndpointInfo& epInfo) :
        _VirtualEndpoint(epInfo->name, remoteEp),
        _TestEndpointInfo(*epInfo.unwrap()),
        b2bEp(TestRemoteEndpoint::cast(remoteEp))
    { }
    virtual ~_TestVirtualEndpoint() { }
    QStatus PushMessage(Message& msg) { return TestPushMessage(msg, BusEndpoint::wrap(this)); }
    QStatus PushMessage(Message& msg, SessionId id) { QCC_UNUSED(id); return TestPushMessage(msg, BusEndpoint::wrap(this)); }
    const String& GetUniqueName() const { return name; }
    bool AllowRemoteMessages() { return allow; }
    QStatus AddSessionRef(SessionId sessionId, RemoteEndpoint& b2bEp) { QCC_UNUSED(sessionId); QCC_UNUSED(b2bEp); return ER_OK; }
    QStatus AddSessionRef(SessionId sessionId, SessionOpts* opts, RemoteEndpoint& b2bEp) { QCC_UNUSED(sessionId); QCC_UNUSED(opts); QCC_UNUSED(b2bEp); return ER_OK; }
    void RemoveSessionRef(SessionId sessionId) { QCC_UNUSED(sessionId); }
    TestRemoteEndpoint& GetTestRemoteEndpoint() { return b2bEp; }
  private:
    TestRemoteEndpoint b2bEp;
};
typedef ManagedObj<_TestVirtualEndpoint> TestVirtualEndpoint;

/*
 * Test override of AllJoynObj.
 */
class TestAllJoynObj : public AllJoynObj {
  public:
    TestAllJoynObj(Bus& bus, DaemonRouter& router) : AllJoynObj(bus, NULL, router) { }
    virtual ~TestAllJoynObj()  { }
    virtual QStatus AddBusToBusEndpoint(RemoteEndpoint& endpoint) { QCC_UNUSED(endpoint); return ER_OK; }
    virtual void RemoveBusToBusEndpoint(RemoteEndpoint& endpoint) { QCC_UNUSED(endpoint); }
};

/*
 * Test override of SessionlessObj.
 */
class TestSessionlessObj : public SessionlessObj {
  public:
    TestSessionlessObj(Bus& bus, DaemonRouter& router) :
        SessionlessObj(bus, NULL, router),
        routed(false),
        pushed(false)
    { }
    virtual ~TestSessionlessObj()  { }
    virtual void AddRule(const qcc::String& epName, Rule& rule) { QCC_UNUSED(epName); QCC_UNUSED(rule); }
    virtual void RemoveRule(const qcc::String& epName, Rule& rule) { QCC_UNUSED(epName); QCC_UNUSED(rule); }
    virtual QStatus PushMessage(Message& msg) { MsgDeliveryHelper(msg, true); pushed = true; return ER_OK; }
    virtual void RouteSessionlessMessage(uint32_t sid, Message& msg)
    {
        QCC_UNUSED(sid);
        MsgDeliveryHelper(msg, false);
        routed = true;
    }

    bool WasRouted() const { return routed; }
    bool WasPushed() const { return pushed; }
  private:
    bool routed;
    bool pushed;
    void MsgDeliveryHelper(Message& msg, bool push);
};

}

/********************************************************************************
 * Utility functions.
 ********************************************************************************/

/*
 * Utility function to get the TestEndpointInfo from a BusEndpoint that is
 * really a derived test endpoint.
 */
static TestEndpointInfo GetTestEndpointInfo(BusEndpoint ep)
{
    // WS violations courtesy of uncrustify
    switch (ep->GetEndpointType()) {
    case ENDPOINT_TYPE_LOCAL: {
            TestLocalEndpoint lep = TestLocalEndpoint::cast(ep);
            return TestEndpointInfo::cast(lep);
        }

    case ENDPOINT_TYPE_VIRTUAL: {
            TestVirtualEndpoint vep = TestVirtualEndpoint::cast(ep);
            return TestEndpointInfo::cast(vep);
        }

    case ENDPOINT_TYPE_BUS2BUS:
    case ENDPOINT_TYPE_REMOTE: {
            TestRemoteEndpoint rep = TestRemoteEndpoint::cast(ep);
            return TestEndpointInfo::cast(rep);
        }

    default: {
            TestEndpoint tep = TestEndpoint::cast(ep);
            return TestEndpointInfo::cast(tep);
        }
    }
}

/*
 * ostream formatter for TestEndpointInfo.
 */
static ostream& operator<<(ostream& os, const TestEndpointInfo& epInfo)
{
    if (epInfo->type == ENDPOINT_TYPE_INVALID) {
        os << "<empty>";
    } else {
        os << "ep " << epInfo->name
           << " (id=" << std::left << std::setw(2) << epInfo->id
           << "  allow=" << (epInfo->allow ? "T" : "F")
           << "  slsMatch=" << (epInfo->slsMatchRule ? "T" : "F")
           << ")";
    }
    return os;
}

/*
 * ostream formatter for a set of BusEndpoints.
 */
static std::ostream& operator<<(std::ostream& os, const set<BusEndpoint>& epSet)
{
    for (set<BusEndpoint>::iterator it = epSet.begin(); it != epSet.end(); ++it) {
        const BusEndpoint& ep = *it;
        TestEndpointInfo epInfo = GetTestEndpointInfo(ep);
        os << "\n    " << epInfo;
    }
    return os;
}


/********************************************************************************
 * Test class code
 ********************************************************************************/
void TestSessionlessObj::MsgDeliveryHelper(Message& msg, bool push)
{
    TestMessage tMsg = TestMessage::cast(msg);
    String destName = tMsg->GetDestination();
    // copy the set since TestPushMessage() will modify the one in tMsg.
    set<BusEndpoint> rEps = push ? tMsg->GetSlsRxPushSet() : tMsg->GetSlsRxRouteSet();
    if (destName.empty()) {
        for (set<BusEndpoint>::iterator it = rEps.begin(); it != rEps.end(); ++it) {
            const BusEndpoint& ep = *it;
            TestPushMessage(msg, ep, !push, push);
        }
    } else {
        for (set<BusEndpoint>::iterator it = rEps.begin(); it != rEps.end(); ++it) {
            BusEndpoint ep = *it;
            if (ep->GetUniqueName() == destName) {
                TestPushMessage(msg, ep, !push, push);
                break;
            }
        }
    }
}


/********************************************************************************
 * The Google test code (finally :) )
 ********************************************************************************/
typedef TestWithParam<tuple<TestEndpointInfo,
                            TestEndpointInfo,
                            AllJoynMessageType,
                            SessionId,
                            TestMessageFlags,
                            String,
                            TestSignalFlags> > TestParamTuple;

/*
 * Google Test harness class
 */
class DaemonRouterTest : public TestParamTuple {
  public:
    DaemonRouterTest() :
        TestParamTuple(),
        matchRule1("type='signal'"),
        matchRule2("type='signal',sessionless='true'")
    {
    }

    static void SetUpTestCase()
    {
        configDb = new ConfigDB(CONFIG_STR);
        configDb->LoadConfig();
        bus = new BusAttachment("DaemonRouterTest");
        bus->Start();
    }

    static void TearDownTestCase()
    {
        bus->Stop();
        bus->Join();
        delete bus;
        bus = NULL;
        delete configDb;
        configDb = NULL;
    }

    ~DaemonRouterTest() { }

    static String ToDot1(const String& name)
    {
        return name.substr(0, name.find_last_of('.')) + ".1";
    }

    virtual void SetUp()
    {
        String name;

        senderInfo =   get<0>(GetParam());
        destInfo =     get<1>(GetParam());
        msgType =      get<2>(GetParam());
        sessionId =    get<3>(GetParam());
        msgFlagParam = get<4>(GetParam());
        name =         get<5>(GetParam());
        signalFlag =   get<6>(GetParam());

        if (msgType == MESSAGE_ERROR) {
            testMember = TEST_MEMBER;
            errorName = name;
        } else {
            testMember = name;
        }

        router = new DaemonRouter();

        alljoynObj = new TestAllJoynObj(*reinterpret_cast<Bus*>(bus), *router);
        sessionlessObj = new TestSessionlessObj(*reinterpret_cast<Bus*>(bus), *router);

        router->SetAllJoynObj(static_cast<AllJoynObj*>(alljoynObj));
        router->SetSessionlessObj(static_cast<SessionlessObj*>(sessionlessObj));
        ruleTable = &router->GetRuleTable();

        for (list<TestEndpointInfo>::const_iterator it = epInfoList.begin(); it != epInfoList.end(); ++it) {
            const TestEndpointInfo& epInfo = *it;
            const BusEndpoint& bep = GenEndpoint(epInfo, signalFlag == SF_SLS_ONLY);
            epList.push_back(bep);
            if (senderInfo == epInfo) {
                senderEp = bep;
            }
        }

        for (list<BusEndpoint>::iterator sit = epList.begin(); sit != epList.end(); ++sit) {
            BusEndpoint& sep = *sit;
            SessionId id = GetTestEndpointInfo(sep)->id;
            if (id != 0) {
                // Start from 'sit' to avoid duplicate entries in the sessionCastSet.
                for (list<BusEndpoint>::iterator dit = sit; dit != epList.end(); ++dit) {
                    BusEndpoint& dep = *dit;
                    if ((GetTestEndpointInfo(dep)->id == id) &&
                        ((sep != dep) || (signalFlag == SF_SELF_JOIN))) {
                        RemoteEndpoint srcB2b;
                        RemoteEndpoint destB2b;
                        bool useSrcB2b = false;
                        if (sep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
                            srcB2b = RemoteEndpoint::cast(TestVirtualEndpoint::cast(sep)->GetTestRemoteEndpoint());
                            useSrcB2b = true;
                        }
                        if (dep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL) {
                            destB2b = RemoteEndpoint::cast(TestVirtualEndpoint::cast(dep)->GetTestRemoteEndpoint());
                            BusEndpoint bep = BusEndpoint::cast(destB2b);
                            router->RegisterEndpoint(bep);
                        }
                        ASSERT_EQ(ER_OK, router->AddSessionRoute(id, sep, useSrcB2b ? &srcB2b : NULL, dep, destB2b));
                    }
                }
            }
        }
    }

    virtual void TearDown()
    {
#ifdef ENABLE_POLICYDB
        // Clear out names cached by PolicyDB to prevent assert fails.
        PolicyDB policyDb = ConfigDB::GetConfigDB()->GetPolicyDB();
        for (list<TestEndpointInfo>::const_iterator it = epInfoList.begin(); it != epInfoList.end(); ++it) {
            const TestEndpointInfo& epInfo = *it;
            policyDb->NameOwnerChanged(epInfo->name,
                                       &epInfo->name, SessionOpts::ALL_NAMES,
                                       NULL, SessionOpts::ALL_NAMES);
            if (epInfo->name.compare(1, 7, "virtual") == 0) {
                String name1 = ToDot1(epInfo->name);
                policyDb->NameOwnerChanged(name1,
                                           &name1, SessionOpts::ALL_NAMES,
                                           NULL, SessionOpts::ALL_NAMES);
            }
        }
#endif

        ruleTable = NULL;
        delete sessionlessObj;
        sessionlessObj = NULL;
        delete alljoynObj;
        alljoynObj = NULL;
        delete router;
        router = NULL;
    }

    BusEndpoint GenEndpoint(const TestEndpointInfo& epInfo, bool onlySlsMatchRules)
    {
        BusEndpoint bep;
        // WS violations courtesy of uncrustify
        switch (epInfo->type) {
        case ENDPOINT_TYPE_LOCAL: {
                TestLocalEndpoint ep(*bus, epInfo->name);
                bep = BusEndpoint::cast(ep);
                localEp = LocalEndpoint::cast(ep);
                break;
            }

        case ENDPOINT_TYPE_VIRTUAL: {
                String b2bname = GenUniqueName(ENDPOINT_TYPE_BUS2BUS, epInfo->id, epInfo->allow, epInfo->slsMatchRule);
                TestEndpointInfo dot1Info(epInfo, true);
                dot1Info->name = ToDot1(epInfo->name);
                String b2bName = String(":bus2bus_") + epInfo->name.substr(9, epInfo->name.find_last_of('.') - 9);
                map<StringMapKey, TestRemoteEndpoint>::iterator it = b2bEps.find(b2bName);
                assert(it != b2bEps.end());
                TestRemoteEndpoint& trep = it->second;
                trep->SetRemoteName(dot1Info->name);
                RemoteEndpoint rep = RemoteEndpoint::cast(trep);
                TestVirtualEndpoint dot1(rep, dot1Info);
                bep = BusEndpoint::cast(dot1);
                router->RegisterEndpoint(bep);
                TestVirtualEndpoint ep(rep, epInfo);
                bep = BusEndpoint::cast(ep);
                break;
            }

        case ENDPOINT_TYPE_BUS2BUS: {
                TestRemoteEndpoint ep(epInfo);
                b2bEps[epInfo->name.substr(0, epInfo->name.find_last_of('.'))] = ep;
                bep = BusEndpoint::cast(ep);
                break;
            }

        case ENDPOINT_TYPE_REMOTE: {
                TestRemoteEndpoint ep(epInfo);
                bep = BusEndpoint::cast(ep);
                break;
            }

        default: {
                TestEndpoint ep(epInfo);
                bep = BusEndpoint::cast(ep);
                break;
            }
        }

        router->RegisterEndpoint(bep);
        if (epInfo->slsMatchRule) {
            ruleTable->AddRule(bep, matchRule2);
        } else if (!onlySlsMatchRules) {
            ruleTable->AddRule(bep, matchRule1);
        }
        return bep;
    }

    static String GenUniqueName(EndpointType type, SessionId id, bool allow, bool slsMatchRule)
    {
        static const char* testUniqueBaseNames[] = {
            ":invalid_",
            ":null____",
            ":local___",
            ":remote__",
            ":bus2bus_",
            ":virtual_"
        };
        static uint32_t counter = 0;
        String name = testUniqueBaseNames[(size_t) type];
        name += ((id != 0) ? "s" : "_");
        name += (allow ? "a" : "_");
        name += (slsMatchRule ? "m." : "_.");
        name += U32ToString(++counter);
        return name;
    }

    static void GenEndpointInfo(EndpointType type, SessionId id, bool allow, bool slsMatchRule)
    {
        String name = GenUniqueName(type, id, allow, slsMatchRule);
        TestEndpointInfo epInfo(name, type, id, allow, slsMatchRule);
        epInfoList.push_back(epInfo);
        if (!slsMatchRule) {
            directEpInfoList.push_back(epInfo);
        }
        if (slsMatchRule) {
            slsEpInfoList.push_back(epInfo);
        }
    }

    static void GenEndpointInfoList()
    {
        static const EndpointType types[] = {
            ENDPOINT_TYPE_NULL,
            ENDPOINT_TYPE_REMOTE,
            ENDPOINT_TYPE_BUS2BUS,
            ENDPOINT_TYPE_VIRTUAL
        };
        static const SessionId ids[] = { 0, TEST_SESSION_ID };

        GenEndpointInfo(ENDPOINT_TYPE_LOCAL, 0, true, false);

        for (size_t t = 0; t < ArraySize(types); ++t) {
            for (size_t i = 0; i < ArraySize(ids); ++i) {
                for (uint8_t f = 0; f < 0x4; ++f) {
                    GenEndpointInfo(types[t], ids[i], (bool)(f & 0x2), (bool)(f & 0x1));
                }
            }
        }
    }

    static list<TestEndpointInfo>& GetEpInfoList()
    {
        if (epInfoList.empty()) {
            GenEndpointInfoList();
        }
        return epInfoList;
    }

    static list<TestEndpointInfo>& GetDirectEpInfoList()
    {
        if (epInfoList.empty()) {
            GenEndpointInfoList();
        }
        return directEpInfoList;
    }

    // Test params
    TestEndpointInfo senderInfo;
    TestEndpointInfo destInfo;
    String destName;
    AllJoynMessageType msgType;
    SessionId sessionId;
    TestMessageFlags msgFlagParam;
    String testMember;
    String errorName;
    TestSignalFlags signalFlag;
    BusEndpoint senderEp;

    // Test support
    DaemonRouter* router;
    RuleTable* ruleTable;
    Rule matchRule1;
    Rule matchRule2;
    list<BusEndpoint> epList;
    map<StringMapKey, TestRemoteEndpoint> b2bEps;

    static ConfigDB* configDb;
    static BusAttachment* bus;
    static TestAllJoynObj* alljoynObj;
    static TestSessionlessObj* sessionlessObj;
    static LocalEndpoint localEp;
    static list<TestEndpointInfo> epInfoList;
    static list<TestEndpointInfo> directEpInfoList;
    static list<TestEndpointInfo> slsEpInfoList;
    static TestEndpointInfo emptyDestInfo;
};

ConfigDB* DaemonRouterTest::configDb = NULL;
BusAttachment* DaemonRouterTest::bus = NULL;
TestAllJoynObj* DaemonRouterTest::alljoynObj = NULL;
TestSessionlessObj* DaemonRouterTest::sessionlessObj = NULL;
LocalEndpoint DaemonRouterTest::localEp;
list<TestEndpointInfo> DaemonRouterTest::epInfoList;
list<TestEndpointInfo> DaemonRouterTest::directEpInfoList;
list<TestEndpointInfo> DaemonRouterTest::slsEpInfoList;

TestEndpointInfo DaemonRouterTest::emptyDestInfo;

static QStatus TestPushMessage(Message& msg, const BusEndpoint& ep, bool slsRoute, bool slsPush)
{
    TestEndpointInfo epInfo = GetTestEndpointInfo(ep);
    TestMessage testMsg = TestMessage::cast(msg);
    const bool msgIsRouterError = (testMsg->GetType() == MESSAGE_ERROR) && (testMsg->GetOrigType() != MESSAGE_ERROR);
    const bool msgIsMethodCall =  (testMsg->GetType() == MESSAGE_METHOD_CALL);
    const bool msgIsSignal =      (testMsg->GetType() == MESSAGE_SIGNAL);
    const bool msgIsInvalid =     (testMsg->GetType() == MESSAGE_INVALID);
    const bool replyIsExpected =  msgIsMethodCall && ((msg->GetFlags() & ALLJOYN_FLAG_NO_REPLY_EXPECTED) == 0);
    const bool msgIsSessionless = (msg->GetFlags() & ALLJOYN_FLAG_SESSIONLESS);

    if (msgIsInvalid) {
        EXPECT_FALSE(msgIsInvalid) << "Test bug: received message type " << testMsg->GetType()
                                   << "; " << testMsg->GetOrigType() << " was expected";
        return ER_INVALID_DATA;
    }

    if ((testMsg->GetOrigType() == MESSAGE_SIGNAL) && (!msgIsSignal)) {
        EXPECT_EQ(testMsg->GetOrigType(), testMsg->GetType()) << "Test bug: received message type " << testMsg->GetType() << "; SIGNAL was expected";
        return ER_INVALID_DATA;
    }
    if (msgIsSignal || msgIsMethodCall) {
        EXPECT_STREQ(TEST_IFACE, msg->GetInterface()) << "Test bug: received interface not used by test - MSG:\n" << msg->ToString();
        if ((strcmp(msg->GetInterface(), TEST_IFACE) != 0)) {
            // Return bogus status to let test function to know to abort.
            return ER_INVALID_DATA;
        }
    }

    if (slsRoute) {
        bool expectedSessionless = testMsg->RemoveSlsRxRoute(ep);
        EXPECT_TRUE(expectedSessionless) << (msgIsSessionless ? "Sessionless " : "Normal ")
                                         << testMsg->GetType()
                                         << (replyIsExpected ? " (reply expected)" : "")
                                         << " erroneously received by " << epInfo
                                         << " via SessionlessObj::RouteSessionlessMessage()";
    } else if (slsPush) {
        bool expectedSessionless = testMsg->RemoveSlsRxPush(ep);
        EXPECT_TRUE(expectedSessionless) << (msgIsSessionless ? "Sessionless " : "Normal ")
                                         << testMsg->GetType()
                                         << (replyIsExpected ? " (reply expected)" : "")
                                         << " erroneously received by " << epInfo
                                         << " via SessionlessObj:PushMessage()";
    } else if (msgIsRouterError) {
        bool expectedError = testMsg->RemoveErrorRx(ep);
        EXPECT_TRUE(expectedError) << (msgIsSessionless ? "Sessionless " : "Normal ")
                                   << testMsg->GetType()
                                   << (replyIsExpected ? " (reply expected)" : "")
                                   << " erroneously received by " << epInfo
                                   << " via router error creation";
    } else {
        bool expectedNormal = testMsg->RemoveNormalRx(ep);
        EXPECT_TRUE(expectedNormal) << (msgIsSessionless ? "Sessionless " : "Normal ")
                                    << testMsg->GetType()
                                    << (replyIsExpected ? " (reply expected)" : "")
                                    << " erroneously received by " << epInfo
                                    << " via normal delivery";
    }
    return ER_OK;
}


/*
 * This is the test case function that Google Test calls for each combination of
 * the test parameters.  It first figures out what the expected behavior is for
 * each set of parameters before calling DaemonRouter::PushMessage().  This
 * includes filling out 4 sets of endpoints: one for the endpoints that are
 * expected to receive the message, one for the set of endpoints that are
 * expected to receive a locally generated error from within
 * DaemonRouter::PushMessage() itself, one for the set of endpoints that are
 * expected to receive the message via
 * SessionlessObj::RouteSessionlessMessage(), and one for the set of endpoints
 * that are expected to receive the message via SessionlessObj::PushMessage().
 *
 * Once DaemonRouter::PushMessage() returns, this function verifies that all the
 * expected message recipients did in fact receive the message.  It also checks
 * that certain combinations of sender, destination and other parameters return
 * the expected status code.
 */
TEST_P(DaemonRouterTest, PushMessage)
{
    BusEndpoint destEp;
    uint8_t flags = (uint8_t)msgFlagParam;
    String destName = destInfo->name;
    TestMessage testMsg(*bus, testMember, errorName, msgType, senderInfo->name, destName, sessionId, flags);

    ASSERT_EQ(msgType, testMsg->GetType()) << "Test bug: Failure to create correct message type";

    ASSERT_TRUE(senderEp->IsValid()) << "Should never happen.  Please fix bug in test code for invalid sender: "
                                     << senderInfo;

    // Decompose conditionals into simply named variables for easy (re)use.
    const bool onlySls = (signalFlag == SF_SLS_ONLY);
    const bool selfJoin = (signalFlag == SF_SELF_JOIN);

    const bool msgIsUnicast = (destInfo->type != ENDPOINT_TYPE_INVALID);  // Invalid dest EP type == broadcast/sessioncast

    const bool msgIsBroadcast = !msgIsUnicast && (sessionId == 0);
    const bool msgIsSessioncast = !msgIsUnicast && !msgIsBroadcast;
    const bool msgIsSessioncastable = msgIsSessioncast && (senderInfo->id == sessionId);  // Sender can sessioncast

    const bool msgIsMethodCall = (msgType == MESSAGE_METHOD_CALL);
    const bool msgIsError = (msgType == MESSAGE_ERROR);
    const bool replyIsExpected = msgIsMethodCall && ((flags & ALLJOYN_FLAG_NO_REPLY_EXPECTED) == 0);

    const bool msgIsSessionless = ((flags & ALLJOYN_FLAG_SESSIONLESS) != 0);
    const bool msgIsGlobalBroadcast = ((flags & ALLJOYN_FLAG_GLOBAL_BROADCAST) != 0);

    const bool senderIsB2b = (senderInfo->type == ENDPOINT_TYPE_BUS2BUS);
    const bool senderIsVirtual = (senderInfo->type == ENDPOINT_TYPE_VIRTUAL);
    const bool senderAllowsRemote = senderEp->AllowRemoteMessages();
    const bool senderIsRemote = (senderIsB2b || senderIsVirtual);
    const bool senderIsLocal = !senderIsRemote;

    const bool senderDenied = msgIsError ? (errorName == TEST_ERROR_SENDER_DENIED) : (testMember == TEST_MEMBER_SENDER_DENIED);
    const bool receiverDenied = msgIsError ? (errorName == TEST_ERROR_RECEIVER_DENIED) : (testMember == TEST_MEMBER_RECEIVER_DENIED);

    bool destAllowsRemote = false;

    bool policyError = false;
    bool noRoute = false;

    // Figure out how DaemonRouter::PushMessage will deal with each EP in the system.
    for (list<BusEndpoint>::iterator it = epList.begin(); it != epList.end(); ++it) {
        BusEndpoint& ep = *it;
        TestEndpointInfo epInfo = GetTestEndpointInfo(ep);
        const bool epIsDest = msgIsUnicast && (ep->GetUniqueName() == destName);
        const bool epIsB2b = (ep->GetEndpointType() == ENDPOINT_TYPE_BUS2BUS);
        const bool epIsVirtual = (ep->GetEndpointType() == ENDPOINT_TYPE_VIRTUAL);
        const bool epAllowsRemote = ep->AllowRemoteMessages();
        const bool epIsRemote = (epIsB2b || epIsVirtual);
        const bool epIsLocal = !epIsRemote;
        const bool epIsInSession = (msgIsSessioncast && (epInfo->id == sessionId));
        const bool localDelivery = (senderIsLocal && epIsLocal);

        bool willRxNorm = false;        // DaemonRouter directly delivers msg
        bool willRxSlsRoute = false;    // DaemonRouter delivers msg via SessionlessObj::RouteSessionlessMessage()
        bool willRxSlsPush = false;     // DaemonRouter delivers msg via SessionlessObj::PushMessage()

        if (epIsDest) {
            destEp = ep;
            destAllowsRemote = epAllowsRemote;
        }

        /*
         * Normal expectation is that policy rules apply to all messages.
         */
        if (!senderDenied && !receiverDenied) {

            /*
             * One would expect that DaemonRouter would route and deliver all
             * SLS msgs.  However, only some SLS msgs are delivered via 1 of 2
             * different functions in SessionlessObj depending on certain
             * criteria.  The rest are delivered directly to endpoints rather
             * than the sessionless mechanism.
             */
            willRxSlsRoute = msgIsSessionless && senderIsB2b && (epIsDest || !msgIsUnicast);
            willRxSlsPush = !willRxSlsRoute && msgIsSessionless && msgIsBroadcast && epInfo->slsMatchRule;

            if (!willRxSlsRoute && !willRxSlsPush) {
                if (epIsDest) {
                    /*
                     * Normal expectation is that msgs will be delivered when
                     * both sender and dest are directly connected to the router
                     * node.
                     */
                    willRxNorm = willRxNorm || localDelivery;

                    /*
                     * Normal expectation is that msgs will be delivered when
                     * the dest allows remote msgs.
                     */
                    willRxNorm = willRxNorm || destAllowsRemote;

                    /*
                     * PushMessage() optimization - method calls to virtual dest
                     * EPs gets blocked if sender does not allow remote msgs and
                     * a reply is expected.
                     */
                    willRxNorm = willRxNorm && !(msgIsMethodCall &&
                                                 !senderAllowsRemote &&
                                                 replyIsExpected &&
                                                 !localDelivery);

                } else if (msgIsBroadcast) {
                    if (!epInfo->slsMatchRule && !onlySls) {
                        /*
                         * Normal expectation is that broacast msgs will be
                         * delivered when both sender and dest are directly
                         * connected to the router node.
                         */
                        willRxNorm = willRxNorm || localDelivery;

                        /*
                         * Normal expectation is that broadcast msgs will be
                         * delivered when the dest allows remote msgs
                         */
                        willRxNorm = willRxNorm || epAllowsRemote;
                    }

                    /*
                     * PushMessage() bug - B2B dest may get global broadcast
                     * msgs twice if sessionless flag is set.  ASACORE-1615 will
                     * address this.
                     */
                    willRxNorm = willRxNorm || (epIsB2b && msgIsGlobalBroadcast && (senderEp != ep) && epAllowsRemote);

                } else if (msgIsSessioncastable && epIsInSession) {
                    /*
                     * Normal expectation is that sessioncast msgs will be
                     * delivered when both sender and dest are directly
                     * connected to the router node and the sender is not the
                     * destination.
                     */
                    willRxNorm = willRxNorm || (localDelivery && (senderEp != ep));

                    /*
                     * Normal expectation is that sessioncast msgs will be
                     * delivered when both sender and dest are directly
                     * connected to the router node, the sender is the
                     * destination, and the sender self-joined its session.
                     */
                    willRxNorm = willRxNorm || (localDelivery && (senderEp == ep) && selfJoin);

                    /*
                     * Normal expectation is that sessioncast msgs will be
                     * delivered when either the sender or dest are not directly
                     * connected to the router node, the dest allows remote
                     * messages, and the sender is not the destination.
                     */
                    willRxNorm = willRxNorm || (!localDelivery && epAllowsRemote && (senderEp != ep));

                    /*
                     * PushMessage() bug - all session members get messages
                     * regardless of AllowRemote, provided the sender is not the
                     * dest or the sender self-joined.  ASACORE-1609 will
                     * address this.
                     */
                    willRxNorm = willRxNorm || ((senderEp != ep) || selfJoin);
                }
            }
        }

        if (willRxNorm) {
            testMsg->AddNormalRx(ep);
        } else if (willRxSlsRoute) {
            testMsg->AddSlsRxRoute(ep);
        } else if (willRxSlsPush) {
            testMsg->AddSlsRxPush(ep);
        } else if (replyIsExpected && !senderIsB2b && epIsDest && (!senderIsVirtual || senderAllowsRemote)) {
            testMsg->AddErrorRx(senderEp);
        }

        if (epIsDest) {
            ASSERT_TRUE(ep->IsValid()) << "Should never happen.  The " << ep << "  is INVALID";
        }
    }

    /*
     * Expect ER_BUS_NO_ROUTE if the message will not be delivered to anyone.
     */
    noRoute = ((testMsg->NormalRxCount() == 0) &&
               (testMsg->SlsRxRouteCount() == 0) &&
               (testMsg->SlsRxPushCount() == 0));

    /*
     * Expect ER_BUS_POLICY_VIOLATION if the message will not be delivered to
     * anyone because of a policy rule violation.  (This overrides
     * ER_BUS_NO_ROUTE.)
     */
    policyError = (senderDenied || receiverDenied);

    Message msg = Message::cast(testMsg);
    QStatus pushMessageStatus = router->PushMessage(msg, senderEp);

    ASSERT_NE(ER_INVALID_DATA, pushMessageStatus) << "Should never happen.  Please fix bug in test code for invalid msg sent: " << msg->ToString();

    QStatus expectedStatus = ER_OK;
    if (policyError) {
        expectedStatus = ER_BUS_POLICY_VIOLATION;
    } else if (noRoute) {
        expectedStatus = ER_BUS_NO_ROUTE;
    }
    EXPECT_EQ(expectedStatus, pushMessageStatus) << (msgIsSessionless ? "Sessionless " : "Normal ")
                                                 << testMsg->GetOrigType()
                                                 << (replyIsExpected ? " (reply expected)" : "")
                                                 << " " << testMember
                                                 << " from " << senderInfo
                                                 << " to " << destInfo
                                                 << " over session ID " << sessionId;
    EXPECT_EQ((size_t)0, testMsg->NormalRxCount()) << (msgIsSessionless ? "Sessionless " : "Normal ")
                                                   << testMsg->GetOrigType()
                                                   << (replyIsExpected ? " (reply expected)" : "")
                                                   << " " << testMember
                                                   << " from " << senderInfo
                                                   << " to " << destInfo
                                                   << " over session ID " << sessionId
                                                   << " not delivered to all recipients directly: "
                                                   << testMsg->GetNormalRxSet();
    EXPECT_EQ((size_t)0, testMsg->SlsRxRouteCount()) << (msgIsSessionless ? "Sessionless " : "Normal ")
                                                     << testMsg->GetOrigType()
                                                     << (replyIsExpected ? " (reply expected)" : "")
                                                     << " " << testMember
                                                     << " from " << senderInfo
                                                     << " to " << destInfo
                                                     << " over session ID " << sessionId
                                                     << " not delivered via SessionlessObj::RouteSessionlessMessage(): "
                                                     << testMsg->GetSlsRxRouteSet();
    EXPECT_EQ((size_t)0, testMsg->SlsRxPushCount()) << (msgIsSessionless ? "Sessionless " : "Normal ")
                                                    << testMsg->GetOrigType()
                                                    << (replyIsExpected ? " (reply expected)" : "")
                                                    << " " << testMember
                                                    << " from " << senderInfo
                                                    << " to " << destInfo
                                                    << " over session ID " << sessionId
                                                    << " not delivered via SessionlessObj::PushMessage(): "
                                                    << testMsg->GetSlsRxPushSet();
    EXPECT_EQ((size_t)0, testMsg->ErrorRxCount()) << "ERROR from " << destInfo
                                                  << " to " << senderInfo
                                                  << " over session ID " << sessionId
                                                  << " not delivered to all error recipients: "
                                                  << testMsg->GetErrorRxSet();
}


#ifdef ENABLE_POLICYDB
#define PolicyDBMemberParams() Values(TEST_MEMBER, TEST_MEMBER_SENDER_DENIED, TEST_MEMBER_RECEIVER_DENIED)
#define PolicyDBErrorParams() Values(TEST_ERROR, TEST_ERROR_SENDER_DENIED, TEST_ERROR_RECEIVER_DENIED)
#else
#define PolicyDBMemberParams() Values(TEST_MEMBER)
#define PolicyDBErrorParams() Values(TEST_ERROR)
#endif

/*
 * Generate the test cases where Signals are sent to specific destinations.
 * Below are the parameters that feed into the generation of test cases:
 *
 * Source:                  set of direct msg test endpoints
 * Destination:             set of direct msg test endpoints
 * Message Type:            SIGNAL
 * Session ID:              0 (special session id all endpoints are implicitly
 *                          part of) testSessionId (test session ID certain
 *                          endpoints are part of)
 * Message Flags:           <none>, SessionLess, GlobalBroadcast
 * Interface Member Name:   Used for testing policy rules.
 */
INSTANTIATE_TEST_CASE_P(SendSignalsDirect,
                        DaemonRouterTest,
                        Combine(ValuesIn(DaemonRouterTest::GetDirectEpInfoList()),
                                ValuesIn(DaemonRouterTest::GetDirectEpInfoList()),
                                Values(MESSAGE_SIGNAL),
                                Values(0, TEST_SESSION_ID),
                                Values(MF_NONE, MF_SESSIONLESS, MF_GLOBAL_BROADCAST),
                                PolicyDBMemberParams(),
                                Values(SF_NONE)));

/*
 * Generate the test cases where Signals are broadcast or sessioncast.  Below
 * are the parameters that feed into the generation of test cases:
 *
 * Source:                  set of all test endpoints
 * Destination:             "" (empty destination indicating broadcast or
 *                          sessioncast)
 * Message Type:            SIGNAL
 * Session ID:              0 (special session id all endpoints are implicitly
 *                          part of) testSessionId (test session ID certain
 *                          endpoints are part of)
 * Message Flags:           <none>, SessionLess, GlobalBroadcast
 * Interface Member Name:   Used for testing policy rules
 */
INSTANTIATE_TEST_CASE_P(SendSignalsCast,
                        DaemonRouterTest,
                        Combine(ValuesIn(DaemonRouterTest::GetEpInfoList()),
                                Values(DaemonRouterTest::emptyDestInfo),
                                Values(MESSAGE_SIGNAL),
                                Values(0, TEST_SESSION_ID),
                                Values(MF_NONE, MF_SESSIONLESS, MF_GLOBAL_BROADCAST),
                                PolicyDBMemberParams(),
                                Values(SF_NONE, SF_SLS_ONLY, SF_SELF_JOIN)));

/*
 * Generate the test cases where Method Calls are sent to specific
 * destinations.  Below are the parameters that feed into the
 * generation of test cases:
 *
 * Source:                  set of direct msg test endpoints
 * Destination:             set of direct msg test endpoints
 * Message Type:            METHOD_CALL
 * Session ID:              0 (special session id all endpoints are implicitly
 *                          part of) testSessionId (test session ID certain
 *                          endpoints are part of)
 * Message Flags:           <none>, SessionLess, GlobalBroadcast
 * Interface Member Name:   Used for testing policy rules
 */
INSTANTIATE_TEST_CASE_P(SendMethodCalls,
                        DaemonRouterTest,
                        Combine(ValuesIn(DaemonRouterTest::GetDirectEpInfoList()),
                                ValuesIn(DaemonRouterTest::GetDirectEpInfoList()),
                                Values(MESSAGE_METHOD_CALL),
                                Values(0, TEST_SESSION_ID),
                                Values(MF_NONE, MF_NO_REPLY_EXPECTED),
                                PolicyDBMemberParams(),
                                Values(SF_NONE)));

/*
 * Generate the test cases where Method Replies are sent to specific
 * destinations.  Below are the parameters that feed into the generation of test
 * cases:
 *
 * Source:                  set of direct msg test endpoints
 * Destination:             set of direct msg test endpoints
 * Message Type:            METHOD_RET
 * Session ID:              0 (special session id all endpoints are implicitly
 *                          part of) testSessionId (test session ID certain
 *                          endpoints are part of)
 * Message Flags:           <none>, SessionLess, GlobalBroadcast
 * Interface Member Name:   Only the normal test member name since it is nigh
 *                          impossible to create reasonable policy rules for
 *                          method return messages
 */
INSTANTIATE_TEST_CASE_P(SendMethodReplies,
                        DaemonRouterTest,
                        Combine(ValuesIn(DaemonRouterTest::GetDirectEpInfoList()),
                                ValuesIn(DaemonRouterTest::GetDirectEpInfoList()),
                                Values(MESSAGE_METHOD_RET),
                                Values(0, TEST_SESSION_ID),
                                Values(MF_NONE),
                                Values(TEST_MEMBER),
                                Values(SF_NONE)));

/*
 * Generate the test cases where Errors are sent to specific destinations.
 * Below are the parameters that feed into the generation of test cases:
 *
 * Source:                  set of direct msg test endpoints
 * Destination:             set of direct msg test endpoints
 * Message Type:            ERROR
 * Session ID:              0 (special session id all endpoints are implicitly
 *                          part of) testSessionId (test session ID certain
 *                          endpoints are part of)
 * Message Flags:           <none>, SessionLess, GlobalBroadcast
 * Error Names:             Used for testing policy rules
 */
INSTANTIATE_TEST_CASE_P(SendErrors,
                        DaemonRouterTest,
                        Combine(ValuesIn(DaemonRouterTest::GetDirectEpInfoList()),
                                ValuesIn(DaemonRouterTest::GetDirectEpInfoList()),
                                Values(MESSAGE_ERROR),
                                Values(0, TEST_SESSION_ID),
                                Values(MF_NONE),
                                PolicyDBErrorParams(),
                                Values(SF_NONE)));


#else
TEST(DummyTest, CombineIsNotSupportedOnThisPlatform)
{
}
#endif
