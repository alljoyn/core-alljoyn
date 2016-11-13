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
#include <gtest/gtest.h>
#include <alljoyn/AuthListener.h>
#include <alljoyn/BusAttachment.h>

using namespace ajn;
using namespace qcc;

class TestSessionPortListener : public SessionPortListener {
  public:
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
};

// Scope this object to just this file to avoid compiler bug described in ASACORE-3467
namespace {
class TestBusObject : public BusObject {
  public:
    TestBusObject(BusAttachment& bus, const char* path, const char* interfaceName, bool announce = true)
        : BusObject(path), isAnnounced(announce) {
        const InterfaceDescription* iface = bus.GetInterface(interfaceName);
        EXPECT_TRUE(iface != nullptr) << "nullptr InterfaceDescription* for " << interfaceName;
        if (iface == nullptr) {
            printf("The interfaceDescription pointer for %s was nullptr when it should not have been.\n", interfaceName);
            return;
        }

        if (isAnnounced) {
            AddInterface(*iface, ANNOUNCED);
        } else {
            AddInterface(*iface, UNANNOUNCED);
        }

        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { iface->GetMember("ping"), static_cast<MessageReceiver::MethodHandler>(&TestBusObject::Ping) },
        };
        EXPECT_EQ(ER_OK, AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0])));
    }

    void Ping(const InterfaceDescription::Member* member, Message& msg) {
        QCC_UNUSED(member);
        QStatus status = MethodReply(msg);
        EXPECT_EQ(ER_OK, status) << "Error sending reply";
    }

  private:
    bool isAnnounced;
};
} // anonymous namespace

TEST(SameInterfaceTest, peer1_and_peer2_call_each_others_methods) {
    const std::string interfaceName =
        "org.allseen.test.SecurityApplication.membershipPropagation";
    const std::string interfaceXml =
        "<node>"
        "<interface name='" + String(interfaceName) + "'>"
        "  <method name='ping'>"
        "  </method>"
        "</interface>"
        "</node>";

    BusAttachment peer1BusNoSec("Peer1");
    peer1BusNoSec.Start();
    peer1BusNoSec.Connect();
    SessionPort p1Port(42);
    SessionOpts p1Opts;
    TestSessionPortListener p1Listener;
    peer1BusNoSec.BindSessionPort(p1Port, p1Opts, p1Listener);

    BusAttachment peer2BusNoSec("Peer2");
    peer2BusNoSec.Start();
    peer2BusNoSec.Connect();
    SessionPort p2Port(42);
    SessionOpts p2Opts;
    TestSessionPortListener p2Listener;
    peer2BusNoSec.BindSessionPort(p2Port, p2Opts, p2Listener);

    EXPECT_EQ(ER_OK, peer1BusNoSec.CreateInterfacesFromXml(interfaceXml.c_str()));
    TestBusObject peer1BusObject(peer1BusNoSec, "/test", interfaceName.c_str());
    EXPECT_EQ(ER_OK, peer1BusNoSec.RegisterBusObject(peer1BusObject));

    EXPECT_EQ(ER_OK, peer2BusNoSec.CreateInterfacesFromXml(interfaceXml.c_str()));
    TestBusObject peer2BusObject(peer2BusNoSec, "/test", interfaceName.c_str());
    EXPECT_EQ(ER_OK, peer2BusNoSec.RegisterBusObject(peer2BusObject));

    /* Make Peer1 join a session hosted by Peer2. */
    SessionId peer1ToPeer2SessionId;
    SessionOpts opts;
    EXPECT_EQ(ER_OK, peer1BusNoSec.JoinSession(peer2BusNoSec.GetUniqueName().c_str(), p2Port, nullptr, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call Peer2's "ping" method. */
    ProxyBusObject peer1ToPeer2Proxy(peer1BusNoSec, peer2BusNoSec.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, false);
    EXPECT_EQ(ER_OK, peer1ToPeer2Proxy.ParseXml(interfaceXml.c_str()));
    EXPECT_TRUE(peer1ToPeer2Proxy.ImplementsInterface(interfaceName.c_str()));

    Message peer1ToPeer2ReplyMsg(peer1BusNoSec);
    EXPECT_EQ(ER_OK, peer1ToPeer2Proxy.MethodCall(interfaceName.c_str(), "ping", nullptr, static_cast<size_t>(0), peer1ToPeer2ReplyMsg));

    /* Make Peer2 join a session hosted by Peer1. */
    SessionId peer2ToPeer1SessionId;
    EXPECT_EQ(ER_OK, peer2BusNoSec.JoinSession(peer1BusNoSec.GetUniqueName().c_str(), p1Port, nullptr, peer2ToPeer1SessionId, opts));

    /* Create the ProxyBusObject and call Peer1's "ping" method. */
    ProxyBusObject peer2ToPeer1Proxy(peer2BusNoSec, peer1BusNoSec.GetUniqueName().c_str(), "/test", peer2ToPeer1SessionId, false);
    EXPECT_EQ(ER_OK, peer2ToPeer1Proxy.ParseXml(interfaceXml.c_str()));
    EXPECT_TRUE(peer2ToPeer1Proxy.ImplementsInterface(interfaceName.c_str()));

    Message peer2ToPeer1ReplyMsg(peer2BusNoSec);
    EXPECT_EQ(ER_OK, peer2ToPeer1Proxy.MethodCall(interfaceName.c_str(), "ping", nullptr, static_cast<size_t>(0), peer2ToPeer1ReplyMsg));
}
