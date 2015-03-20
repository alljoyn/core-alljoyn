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

#include "AllJoynObj.h"
#include "ConfigDB.h"
#include "SessionInternal.h"

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "../ajTestCommon.h"

using namespace std;
using namespace qcc;
using namespace ajn;

/*
 * ASACORE-489
 */

class TestTransport : public Transport {
  public:
    TestTransport(BusAttachment& bus, TransportMask mask, const char* name) : tried(false), bus(bus), mask(mask), name(name) { }
    virtual QStatus Start() { return ER_OK; }
    virtual QStatus Stop() { return ER_OK; }
    virtual QStatus Join() { return ER_OK; }
    virtual bool IsRunning() { return true; }
    virtual TransportMask GetTransportMask() const { return mask; }
    virtual const char* GetTransportName() const { return name.c_str(); }
    virtual QStatus NormalizeTransportSpec(const char* inSpec, String& outSpec, map<String, String>& argMap) const { return ER_OK; }
    virtual bool IsBusToBus() const { return true; }
    bool tried;
  protected:
    BusAttachment& bus;
    TransportMask mask;
    String name;
};

class ConnectFailTransport : public TestTransport {
  public:
    ConnectFailTransport(BusAttachment& bus, TransportMask mask, const char* name) : TestTransport(bus, mask, name) { }
    virtual QStatus Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newEp) {
        tried = true;
        return ER_FAIL;
    }
};

class ConnectPassTransport : public TestTransport {
  public:
    ConnectPassTransport(BusAttachment& bus, TransportMask mask, const char* name) : TestTransport(bus, mask, name) { }
    virtual QStatus Connect(const char* connectSpec, const SessionOpts& opts, BusEndpoint& newEp) {
        tried = true;
        bool incoming = false;
        Stream* stream = NULL;
        RemoteEndpoint ep(bus, incoming, connectSpec, stream);
        newEp = BusEndpoint::cast(ep);
        return ER_OK;
    }
};

class _TestVirtualEndpoint : public _VirtualEndpoint {
  public:
    _TestVirtualEndpoint(const String& uniqueName, RemoteEndpoint& b2bEp) : _VirtualEndpoint(uniqueName, b2bEp) { }
    virtual bool CanUseRoute(const RemoteEndpoint& b2bEndpoint) const { return true; }
};
typedef ManagedObj<_TestVirtualEndpoint> TestVirtualEndpoint;

class _JoinSessionMethodCall : public _Message {
  public:
    _JoinSessionMethodCall(BusAttachment& bus, const char* joiner, SessionId id, const char* host, SessionPort port, SessionOpts opts) : _Message(bus) {
        const char* signature = "sqa{sv}";
        MsgArg args[3];
        EXPECT_EQ(ER_OK, args[0].Set("s", host));
        EXPECT_EQ(ER_OK, args[1].Set("q", port));
        SetSessionOpts(opts, args[2]);
        EXPECT_EQ(ER_OK, CallMsg(signature, joiner, org::alljoyn::Bus::WellKnownName, id,
                                 org::alljoyn::Bus::ObjectPath, org::alljoyn::Bus::InterfaceName, "JoinSession",
                                 args, 3,
                                 0));
        PeerStateTable peerStateTable;
        EXPECT_EQ(ER_OK, UnmarshalArgs(&peerStateTable, signature));
    }
};
typedef ManagedObj<_JoinSessionMethodCall> JoinSessionMethodCall;

class TestAllJoynObj : public AllJoynObj {
  public:
    TestAllJoynObj(Bus& bus)
        : AllJoynObj(bus, NULL, reinterpret_cast<DaemonRouter&>(bus.GetInternal().GetRouter())),
        bus(bus), replyCode(0), triedTransports(TRANSPORT_NONE), connectedTransport(TRANSPORT_NONE) {
    }
    virtual ~TestAllJoynObj() {
        for (vector<TestTransport*>::iterator it = transportList.begin(); it != transportList.end(); ++it) {
            delete *it;
        }
    }
    void AddTransport(TestTransport* transport) {
        transportList.push_back(transport);
    }
    void AddTransportAndAdvertisement(TestTransport* transport) {
        AddTransport(transport);
        vector<String> names;
        names.push_back(":host.3");
        FoundNames(transport->GetTransportName(), "GUID", transport->GetTransportMask(), &names, 120);
    }
    void RunJoin(SessionOpts opts = SessionOpts()) {
        SessionId id = 0;
        SessionPort port = 80;
        JoinSessionMethodCall msg(bus, ":joiner.3", id, ":host.3", port, opts);

        bool isJoin = true;
        TestJoinSessionThread joinSessionThread(*this, Message::cast(msg), isJoin);
        joinSessionThread.RunJoin();
    }
    virtual Transport* GetTransport(const String& transportSpec) {
        for (vector<TestTransport*>::iterator it = transportList.begin(); it != transportList.end(); ++it) {
            if (transportSpec.compare(0, 3, (*it)->GetTransportName()) == 0) {
                return (*it);
            }
        }
        return NULL;
    }
    virtual BusEndpoint FindEndpoint(const String& busName) {
        if (busName == ":joiner.3") {
            bool incoming = true;
            Stream* stream = NULL;
            RemoteEndpoint joinerEp(bus, incoming, "", stream);
            return BusEndpoint::cast(joinerEp);
        } else {
            return BusEndpoint();
        }
    }
    virtual bool FindEndpoint(const String& busName, VirtualEndpoint& endpoint) {
        bool incoming = false;
        const char* connectSpec = "";
        Stream* stream = NULL;
        RemoteEndpoint b2bEp(bus, incoming, connectSpec, stream);
        TestVirtualEndpoint ep(busName, b2bEp);
        endpoint = VirtualEndpoint::cast(ep);
        return true;
    }
    virtual QStatus SendAttachSession(SessionPort sessionPort, const char* src, const char* sessionHost, const char* dest,
                                      RemoteEndpoint& b2bEp, const char* remoteControllerName, SessionId outgoingSessionId,
                                      const char* busAddr, const SessionOpts& optsIn, uint32_t& replyCode, SessionId& sessionId,
                                      SessionOpts& optsOut, MsgArg& members) {
        optsOut.transports = optsIn.transports;
        return ER_OK;
    }
    virtual QStatus AddSessionRoute(SessionId id, BusEndpoint& srcEp, RemoteEndpoint* srcB2bEp, BusEndpoint& destEp,
                                    RemoteEndpoint& destB2bEp) {
        return ER_OK;
    }

    class TestJoinSessionThread : public JoinSessionThread {
      public:
        TestJoinSessionThread(TestAllJoynObj& ajObj, const Message& msg, bool isJoin)
            : JoinSessionThread(ajObj, msg, isJoin), ajObj(ajObj) { }
        virtual QStatus Reply(uint32_t replyCode, SessionId id, SessionOpts optsOut) {
            ajObj.replyCode = replyCode;
            ajObj.connectedTransport = optsOut.transports;
            for (vector<TestTransport*>::iterator it = ajObj.transportList.begin(); it != ajObj.transportList.end(); ++it) {
                if ((*it)->tried) {
                    ajObj.triedTransports |= (*it)->GetTransportMask();
                }
            }
            return ER_OK;
        }
        TestAllJoynObj& ajObj;
    };
    Bus& bus;
    uint32_t replyCode;
    TransportMask triedTransports;
    TransportMask connectedTransport;
    vector<TestTransport*> transportList;
};

TEST(AllJoynObjTest, JoinSessionToUnadvertisedNameFails)
{
    ConfigDB configDb("");
    configDb.LoadConfig();

    TransportFactoryContainer factories;
    Bus bus("AllJoynObjTest", factories);

    // Set up two available transports, neither advertising
    TestAllJoynObj ajObj(bus);
    ajObj.AddTransport(new ConnectFailTransport(bus, TRANSPORT_UDP, "udp"));

    // Call JoinSession
    ajObj.RunJoin();

    // Verify that join failed
    EXPECT_NE((uint32_t)ALLJOYN_JOINSESSION_REPLY_SUCCESS, ajObj.replyCode);
    EXPECT_EQ(TRANSPORT_NONE, ajObj.triedTransports);
    EXPECT_EQ(TRANSPORT_NONE, ajObj.connectedTransport);
}

TEST(AllJoynObjTest, JoinSessionSkipsUnpermittedAvailableTransports)
{
    ConfigDB configDb("");
    configDb.LoadConfig();

    TransportFactoryContainer factories;
    Bus bus("AllJoynObjTest", factories);

    // Set up two transports
    TestAllJoynObj ajObj(bus);
    ajObj.AddTransportAndAdvertisement(new ConnectPassTransport(bus, TRANSPORT_UDP, "udp"));
    ajObj.AddTransportAndAdvertisement(new ConnectPassTransport(bus, TRANSPORT_TCP, "tcp"));

    // Call JoinSession to only TRANSPORT_TCP
    SessionOpts opts;
    opts.transports = TRANSPORT_TCP;
    ajObj.RunJoin(opts);

    // Verify that join succeeded to TRANSPORT_TCP and that TRANSPORT_UDP was not tried
    EXPECT_EQ((uint32_t)ALLJOYN_JOINSESSION_REPLY_SUCCESS, ajObj.replyCode);
    EXPECT_EQ(TRANSPORT_TCP, ajObj.triedTransports);
    EXPECT_EQ(TRANSPORT_TCP, ajObj.connectedTransport);
}

TEST(AllJoynObjTest, JoinSessionTriesAllAvailableTransportsPass)
{
    ConfigDB configDb("");
    configDb.LoadConfig();

    TransportFactoryContainer factories;
    Bus bus("AllJoynObjTest", factories);

    // Set up two transports, one that fails on connect and one that succeeds
    TestAllJoynObj ajObj(bus);
    ajObj.AddTransportAndAdvertisement(new ConnectFailTransport(bus, TRANSPORT_UDP, "udp"));
    ajObj.AddTransportAndAdvertisement(new ConnectPassTransport(bus, TRANSPORT_TCP, "tcp"));

    // Call JoinSession
    ajObj.RunJoin();

    // Verify that join succeeded to second transport
    EXPECT_EQ((uint32_t)ALLJOYN_JOINSESSION_REPLY_SUCCESS, ajObj.replyCode);
    EXPECT_EQ(TRANSPORT_UDP | TRANSPORT_TCP, ajObj.triedTransports);
    EXPECT_EQ(TRANSPORT_TCP, ajObj.connectedTransport);
}

TEST(AllJoynObjTest, JoinSessionTriesAllAvailableTransportsFail)
{
    ConfigDB configDb("");
    configDb.LoadConfig();

    TransportFactoryContainer factories;
    Bus bus("AllJoynObjTest", factories);

    // Set up two transports, both fail
    TestAllJoynObj ajObj(bus);
    ajObj.AddTransportAndAdvertisement(new ConnectFailTransport(bus, TRANSPORT_UDP, "udp"));
    ajObj.AddTransportAndAdvertisement(new ConnectFailTransport(bus, TRANSPORT_TCP, "tcp"));

    // Call JoinSession
    ajObj.RunJoin();

    // Verify that join failed to both transports
    EXPECT_NE((uint32_t)ALLJOYN_JOINSESSION_REPLY_SUCCESS, ajObj.replyCode);
    EXPECT_EQ(TRANSPORT_UDP | TRANSPORT_TCP, ajObj.triedTransports);
    EXPECT_EQ(TRANSPORT_NONE, ajObj.connectedTransport);
}

class TestAllJoynObjBadSessionOpts : public TestAllJoynObj {
  public:
    TestAllJoynObjBadSessionOpts(Bus& bus) : TestAllJoynObj(bus) {
        AddTransportAndAdvertisement(new ConnectPassTransport(bus, TRANSPORT_UDP, "udp"));
        AddTransportAndAdvertisement(new ConnectPassTransport(bus, TRANSPORT_TCP, "tcp"));
    }
    virtual QStatus SendAttachSession(SessionPort sessionPort, const char* src, const char* sessionHost, const char* dest,
                                      RemoteEndpoint& b2bEp, const char* remoteControllerName, SessionId outgoingSessionId,
                                      const char* busAddr, const SessionOpts& optsIn, uint32_t& replyCode, SessionId& sessionId,
                                      SessionOpts& optsOut, MsgArg& members) {
        if (optsIn.transports == TRANSPORT_UDP) {
            replyCode = ALLJOYN_JOINSESSION_REPLY_BAD_SESSION_OPTS;
        } else if (optsIn.transports == TRANSPORT_TCP) {
            replyCode = ALLJOYN_JOINSESSION_REPLY_SUCCESS;
        }
        optsOut.transports = optsIn.transports;
        return ER_OK;
    }
};

TEST(AllJoynObjTest, JoinSessionTriesAllAvailableTransportsAfterAttachSessionFails)
{
    ConfigDB configDb("");
    configDb.LoadConfig();

    TransportFactoryContainer factories;
    Bus bus("AllJoynObjTest", factories);

    // Set up AllJoynObj that reports BAD_SESSION_OPTS reply code for first transport
    TestAllJoynObjBadSessionOpts ajObj(bus);

    // Call JoinSession
    ajObj.RunJoin();

    // Verify that join succeeded to second transport
    EXPECT_EQ((uint32_t)ALLJOYN_JOINSESSION_REPLY_SUCCESS, ajObj.replyCode);
    EXPECT_EQ(TRANSPORT_UDP | TRANSPORT_TCP, ajObj.triedTransports);
    EXPECT_EQ(TRANSPORT_TCP, ajObj.connectedTransport);
}
