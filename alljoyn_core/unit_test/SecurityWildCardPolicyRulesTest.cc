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
#include <alljoyn/SecurityApplicationProxy.h>
#include <iostream>
#include <map>

#include "InMemoryKeyStore.h"
#include "PermissionMgmtObj.h"
#include "PermissionMgmtTest.h"

using namespace ajn;
using namespace qcc;
using namespace std;
/*
 * The unit test use many busy wait loops.  The busy wait loops were chosen
 * over thread sleeps because of the ease of understanding the busy wait loops.
 * Also busy wait loops do not require any platform specific threading code.
 */
#define WAIT_MSECS 5

class WildCardPolicyRules_ApplicationStateListener : public ApplicationStateListener {
  public:
    WildCardPolicyRules_ApplicationStateListener() : stateMap() { }

    virtual void State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state) {
        QCC_UNUSED(publicKeyInfo);
        stateMap[busName] = state;
    }

    bool isClaimed(const String& busName) {
        if (stateMap.count(busName) > 0) {
            if (stateMap.find(busName)->second == PermissionConfigurator::ApplicationState::CLAIMED) {
                return true;
            }
        }
        return false;
    }
    map<String, PermissionConfigurator::ApplicationState> stateMap;
};

class WildCardPolicyRulesTestSessionPortListener : public SessionPortListener {
  public:
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
};

// String constants associated with the interfaces used for tests
namespace test {
static const char argentinaObjPath[] = "/test/argentina";
static const char arabicObjPath[] = "/test/arabic";
namespace calcium {
static const char InterfaceName[] = "test.calcium"; // Interface name
namespace method {
static const char march[] = "march"; //march method name
}
namespace signal {
static const char make[] = "make"; //make signal name
}
namespace property {
static const char mayonise[] = "mayonise"; //mayonise property name
}
}
namespace california {
static const char InterfaceName[] = "test.california"; // Interface name
namespace method {
static const char metal[] = "metal"; //metal method name
}
namespace signal {
static const char mess[] = "mess"; //mess signal name
}
namespace property {
static const char meal[] = "meal"; //meal property name
}
}
namespace camera {
static const char InterfaceName[] = "test.camera"; // Interface name
namespace method {
static const char mob[] = "mob"; //mob method name
}
namespace signal {
static const char money[] = "money"; //money signal name
}
namespace property {
static const char motel[] = "motel"; //motel property name
}
}
namespace cashew {
static const char InterfaceName[] = "test.cashew"; // Interface name
namespace method {
static const char mint[] = "mint"; //mint method name
}
namespace signal {
static const char mits[] = "mits"; //mits signal name
}
namespace property {
static const char mini[] = "mini"; //mini property name
}
}
}

class ArgentinaTestBusObject : public BusObject {
  public:
    ArgentinaTestBusObject(BusAttachment& bus, const char* path, bool announce = true)
        : BusObject(path), isAnnounced(announce), mayonise(42), meal(17) {
        const InterfaceDescription* calciumIface = bus.GetInterface(test::calcium::InterfaceName);
        EXPECT_TRUE(calciumIface != NULL) << "NULL InterfaceDescription* for " << test::calcium::InterfaceName;
        if (calciumIface == NULL) {
            printf("The interfaceDescription pointer for %s was NULL when it should not have been.\n", test::calcium::InterfaceName);
            return;
        }

        if (isAnnounced) {
            AddInterface(*calciumIface, ANNOUNCED);
        } else {
            AddInterface(*calciumIface, UNANNOUNCED);
        }

        /* Register the method handlers with the object */
        const MethodEntry calciumMethodEntries[] = {
            { calciumIface->GetMember(test::calcium::method::march), static_cast<MessageReceiver::MethodHandler>(&ArgentinaTestBusObject::March) }
        };
        EXPECT_EQ(ER_OK, AddMethodHandlers(calciumMethodEntries, sizeof(calciumMethodEntries) / sizeof(calciumMethodEntries[0])));

        const InterfaceDescription* californiaIface = bus.GetInterface(test::california::InterfaceName);
        EXPECT_TRUE(californiaIface != NULL) << "NULL InterfaceDescription* for " << test::california::InterfaceName;
        if (californiaIface == NULL) {
            printf("The interfaceDescription pointer for %s was NULL when it should not have been.\n", test::california::InterfaceName);
            return;
        }

        if (isAnnounced) {
            AddInterface(*californiaIface, ANNOUNCED);
        } else {
            AddInterface(*californiaIface, UNANNOUNCED);
        }

        /* Register the method handlers with the object */
        const MethodEntry californiaMethodEntries[] = {
            { californiaIface->GetMember(test::california::method::metal), static_cast<MessageReceiver::MethodHandler>(&ArgentinaTestBusObject::Metal) }
        };
        EXPECT_EQ(ER_OK, AddMethodHandlers(californiaMethodEntries, sizeof(californiaMethodEntries) / sizeof(californiaMethodEntries[0])));
    }

    void March(const InterfaceDescription::Member* member, Message& msg) {
        QCC_UNUSED(member);
        MsgArg* arg = NULL;
        QStatus status = MethodReply(msg, arg, 0);
        EXPECT_EQ(ER_OK, status) << "Echo: Error sending reply";
    }

    void Metal(const InterfaceDescription::Member* member, Message& msg) {
        QCC_UNUSED(member);
        MsgArg* arg = NULL;
        QStatus status = MethodReply(msg, arg, 0);
        EXPECT_EQ(ER_OK, status) << "Echo: Error sending reply";
    }

    QStatus Get(const char* ifcName, const char* propName, MsgArg& val)
    {
        QCC_UNUSED(ifcName);
        QStatus status = ER_OK;
        if (0 == strcmp(test::calcium::property::mayonise, propName)) {
            val.Set("i", mayonise);
        } else if (0 == strcmp(test::california::property::meal, propName)) {
            val.Set("i", meal);
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;

    }

    QStatus Set(const char* ifcName, const char* propName, MsgArg& val)
    {
        QCC_UNUSED(ifcName);
        QStatus status = ER_OK;
        if ((0 == strcmp(test::calcium::property::mayonise, propName)) && (val.typeId == ALLJOYN_INT32)) {
            val.Get("i", &mayonise);
        } else if ((0 == strcmp(test::california::property::meal, propName)) && (val.typeId == ALLJOYN_INT32)) {
            val.Get("i", &meal);
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;
    }
    int32_t ReadMayoniseProp() {
        return mayonise;
    }
    int32_t ReadMealProp() {
        return meal;
    }
  private:
    bool isAnnounced;
    int32_t mayonise;
    int32_t meal;
};

class SecurityWildCardSignalReceiver : public MessageReceiver {
  public:
    SecurityWildCardSignalReceiver() : signalReceivedFlag(false) { }
    void SignalHandler(const InterfaceDescription::Member* member,
                       const char* sourcePath, Message& msg) {
        QCC_UNUSED(member);
        QCC_UNUSED(sourcePath);
        QCC_UNUSED(msg);
        signalReceivedFlag = true;
    }
    bool signalReceivedFlag;
};

class ArabicTestBusObject : public BusObject {
  public:
    ArabicTestBusObject(BusAttachment& bus, const char* path, bool announce = true)
        : BusObject(path), isAnnounced(announce), motel(42), mini(17) {
        const InterfaceDescription* cameraIface = bus.GetInterface(test::camera::InterfaceName);
        EXPECT_TRUE(cameraIface != NULL) << "NULL InterfaceDescription* for " << test::camera::InterfaceName;
        if (cameraIface == NULL) {
            printf("The interfaceDescription pointer for %s was NULL when it should not have been.\n", test::camera::InterfaceName);
            return;
        }

        if (isAnnounced) {
            AddInterface(*cameraIface, ANNOUNCED);
        } else {
            AddInterface(*cameraIface, UNANNOUNCED);
        }

        /* Register the method handlers with the object */
        const MethodEntry cameraMethodEntries[] = {
            { cameraIface->GetMember(test::camera::method::mob), static_cast<MessageReceiver::MethodHandler>(&ArabicTestBusObject::Mob) }
        };
        EXPECT_EQ(ER_OK, AddMethodHandlers(cameraMethodEntries, sizeof(cameraMethodEntries) / sizeof(cameraMethodEntries[0])));

        const InterfaceDescription* cashewIface = bus.GetInterface(test::cashew::InterfaceName);
        EXPECT_TRUE(cashewIface != NULL) << "NULL InterfaceDescription* for " << test::cashew::InterfaceName;
        if (cashewIface == NULL) {
            printf("The interfaceDescription pointer for %s was NULL when it should not have been.\n", test::cashew::InterfaceName);
            return;
        }

        if (isAnnounced) {
            AddInterface(*cashewIface, ANNOUNCED);
        } else {
            AddInterface(*cashewIface, UNANNOUNCED);
        }

        /* Register the method handlers with the object */
        const MethodEntry cashewMethodEntries[] = {
            { cashewIface->GetMember(test::cashew::method::mint), static_cast<MessageReceiver::MethodHandler>(&ArabicTestBusObject::Mint) }
        };
        EXPECT_EQ(ER_OK, AddMethodHandlers(cashewMethodEntries, sizeof(cashewMethodEntries) / sizeof(cashewMethodEntries[0])));
    }

    void Mob(const InterfaceDescription::Member* member, Message& msg) {
        QCC_UNUSED(member);
        MsgArg* arg = NULL;
        QStatus status = MethodReply(msg, arg, 0);
        EXPECT_EQ(ER_OK, status) << "Echo: Error sending reply";
    }

    void Mint(const InterfaceDescription::Member* member, Message& msg) {
        QCC_UNUSED(member);
        MsgArg* arg = NULL;
        QStatus status = MethodReply(msg, arg, 0);
        EXPECT_EQ(ER_OK, status) << "Echo: Error sending reply";
    }

    QStatus Get(const char* ifcName, const char* propName, MsgArg& val)
    {
        QCC_UNUSED(ifcName);
        QStatus status = ER_OK;
        if (0 == strcmp(test::camera::property::motel, propName)) {
            val.Set("i", motel);
        } else if (0 == strcmp(test::cashew::property::mini, propName)) {
            val.Set("i", mini);
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;

    }

    QStatus Set(const char* ifcName, const char* propName, MsgArg& val)
    {
        QCC_UNUSED(ifcName);
        QStatus status = ER_OK;
        if ((0 == strcmp(test::camera::property::motel, propName)) && (val.typeId == ALLJOYN_INT32)) {
            val.Get("i", &motel);
        } else if ((0 == strcmp(test::cashew::property::mini, propName)) && (val.typeId == ALLJOYN_INT32)) {
            val.Get("i", &mini);
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;
    }
    int32_t ReadMayoniseProp() {
        return motel;
    }
    int32_t ReadMealProp() {
        return mini;
    }
  private:
    bool isAnnounced;
    int32_t motel;
    int32_t mini;
};

static void GetAppPublicKey(BusAttachment& bus, ECCPublicKey& publicKey)
{
    KeyInfoNISTP256 keyInfo;
    bus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
    publicKey = *keyInfo.GetPublicKey();
}

class SecurityWildCardPolicyRulesTest : public testing::Test {
  public:
    SecurityWildCardPolicyRulesTest() :
        managerBus("SecurityPolicyRulesManager"),
        peer1Bus("SecurityPolicyRulesPeer1"),
        peer2Bus("SecurityPolicyRulesPeer2"),
        managerSessionPort(42),
        peer1SessionPort(42),
        peer2SessionPort(42),
        managerToManagerSessionId(0),
        managerToPeer1SessionId(0),
        managerToPeer2SessionId(0),
        calciumInterfaceName("test.calcium"),
        californiaInterfaceName("test.california"),
        cameraInterfaceName("test.camera"),
        cashewInterfaceName("test.cashew"),
        managerAuthListener(NULL),
        peer1AuthListener(NULL),
        peer2AuthListener(NULL),
        appStateListener()
    {
    }

    virtual void SetUp();

    virtual void TearDown();
    QStatus UpdatePolicyWithValuesFromDefaultPolicy(const PermissionPolicy& defaultPolicy,
                                                    PermissionPolicy& policy,
                                                    bool keepCAentry = true,
                                                    bool keepAdminGroupEntry = false,
                                                    bool keepInstallMembershipEntry = false);

    BusAttachment managerBus;
    BusAttachment peer1Bus;
    BusAttachment peer2Bus;

    SessionPort managerSessionPort;
    SessionPort peer1SessionPort;
    SessionPort peer2SessionPort;

    WildCardPolicyRulesTestSessionPortListener managerSessionPortListener;
    WildCardPolicyRulesTestSessionPortListener peer1SessionPortListener;
    WildCardPolicyRulesTestSessionPortListener peer2SessionPortListener;

    SessionId managerToManagerSessionId;
    SessionId managerToPeer1SessionId;
    SessionId managerToPeer2SessionId;

    InMemoryKeyStoreListener managerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;

    String interface;
    const char* calciumInterfaceName;
    const char* californiaInterfaceName;
    const char* cameraInterfaceName;
    const char* cashewInterfaceName;
    DefaultECDHEAuthListener* managerAuthListener;
    DefaultECDHEAuthListener* peer1AuthListener;
    DefaultECDHEAuthListener* peer2AuthListener;

    WildCardPolicyRules_ApplicationStateListener appStateListener;

    //Random GUID used for the SecurityManager
    GUID128 managerGuid;
};

void SecurityWildCardPolicyRulesTest::SetUp()
{
    EXPECT_EQ(ER_OK, managerBus.Start());
    EXPECT_EQ(ER_OK, managerBus.Connect());
    EXPECT_EQ(ER_OK, peer1Bus.Start());
    EXPECT_EQ(ER_OK, peer1Bus.Connect());
    EXPECT_EQ(ER_OK, peer2Bus.Start());
    EXPECT_EQ(ER_OK, peer2Bus.Connect());

    // Register in memory keystore listeners
    EXPECT_EQ(ER_OK, managerBus.RegisterKeyStoreListener(managerKeyStoreListener));
    EXPECT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
    EXPECT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));

    managerAuthListener = new DefaultECDHEAuthListener();
    peer1AuthListener = new DefaultECDHEAuthListener();
    peer2AuthListener = new DefaultECDHEAuthListener();

    EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", managerAuthListener, NULL, true));
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", peer1AuthListener));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", peer2AuthListener));

    interface = "<node name='/test'>"
                "  <node name='/argentina'>"
                "  <interface name='test.calcium'>"
                "  <annotation name='org.alljoyn.Bus.Secure' value='true'/>"
                "    <method name='march'></method>"
                "    <signal name='make'></signal>"
                "    <property name='mayonise' type='i' access='readwrite'/>"
                "  </interface>"
                "  <interface name='test.california'>"
                "  <annotation name='org.alljoyn.Bus.Secure' value='true'/>"
                "    <method name='metal'></method>"
                "    <signal name='mess'></signal>"
                "    <property name='meal' type='i' access='readwrite'/>"
                "  </interface>"
                "  </node>"
                "  <node name='/arabic'>"
                "  <interface name='test.camera'>"
                "  <annotation name='org.alljoyn.Bus.Secure' value='true'/>"
                "    <method name='mob'></method>"
                "    <signal name='money'></signal>"
                "    <property name='motel' type='i' access='readwrite'/>"
                "  </interface>"
                "  <interface name='test.cashew'>"
                "  <annotation name='org.alljoyn.Bus.Secure' value='true'/>"
                "    <method name='mint'></method>"
                "    <signal name='mits'></signal>"
                "    <property name='mini' type='i' access='readwrite'/>"
                "  </interface>"
                "  </node>"
                "</node>";

    EXPECT_EQ(ER_OK, peer1Bus.CreateInterfacesFromXml(interface.c_str()));
    EXPECT_EQ(ER_OK, peer2Bus.CreateInterfacesFromXml(interface.c_str()));

    SessionOpts opts1;
    EXPECT_EQ(ER_OK, managerBus.BindSessionPort(managerSessionPort, opts1, managerSessionPortListener));

    SessionOpts opts2;
    EXPECT_EQ(ER_OK, peer1Bus.BindSessionPort(peer1SessionPort, opts2, peer1SessionPortListener));

    SessionOpts opts3;
    EXPECT_EQ(ER_OK, peer2Bus.BindSessionPort(peer2SessionPort, opts3, peer2SessionPortListener));

    EXPECT_EQ(ER_OK, managerBus.JoinSession(managerBus.GetUniqueName().c_str(), managerSessionPort, NULL, managerToManagerSessionId, opts1));
    EXPECT_EQ(ER_OK, managerBus.JoinSession(peer1Bus.GetUniqueName().c_str(), peer1SessionPort, NULL, managerToPeer1SessionId, opts2));
    EXPECT_EQ(ER_OK, managerBus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, managerToPeer2SessionId, opts3));

    SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);
    PermissionConfigurator::ApplicationState applicationStateManager;
    EXPECT_EQ(ER_OK, sapWithManager.GetApplicationState(applicationStateManager));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStateManager);

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    PermissionConfigurator::ApplicationState applicationStatePeer1;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStatePeer1);

    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
    PermissionConfigurator::ApplicationState applicationStatePeer2;
    EXPECT_EQ(ER_OK, sapWithPeer2.GetApplicationState(applicationStatePeer2));
    EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationStatePeer2);

    managerBus.RegisterApplicationStateListener(appStateListener);
    managerBus.AddApplicationStateRule();

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetObjPath("*");
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    //Get manager key
    KeyInfoNISTP256 managerKey;
    PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

    //Create peer1 key
    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    //Create peer2 key
    KeyInfoNISTP256 peer2Key;
    PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(managerBus,
                                                               manifest, manifestSize,
                                                               digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    //Create identityCert
    const size_t certChainSize = 1;
    IdentityCertificate identityCertChainMaster[certChainSize];

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  managerKey.GetPublicKey(),
                                                                  "ManagerAlias",
                                                                  3600,
                                                                  identityCertChainMaster[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    SecurityApplicationProxy sapWithManagerBus(managerBus, managerBus.GetUniqueName().c_str());
    /* set claimable */
    managerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    EXPECT_EQ(ER_OK, sapWithManagerBus.Claim(managerKey,
                                             managerGuid,
                                             managerKey,
                                             identityCertChainMaster, certChainSize,
                                             manifest, manifestSize));

    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.isClaimed(managerBus.GetUniqueName())) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    ECCPublicKey managerPublicKey;
    GetAppPublicKey(managerBus, managerPublicKey);
    ASSERT_EQ(*managerKey.GetPublicKey(), managerPublicKey);

    ASSERT_EQ(PermissionConfigurator::ApplicationState::CLAIMED, appStateListener.stateMap[managerBus.GetUniqueName()]);

    //Create peer1 identityCert
    IdentityCertificate identityCertChainPeer1[certChainSize];


    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  peer1Key.GetPublicKey(),
                                                                  "Peer1Alias",
                                                                  3600,
                                                                  identityCertChainPeer1[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    //Manager claims Peers
    /* set claimable */
    peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(managerKey,
                                        managerGuid,
                                        managerKey,
                                        identityCertChainPeer1, certChainSize,
                                        manifest, manifestSize));

    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.isClaimed(peer1Bus.GetUniqueName())) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    ASSERT_EQ(PermissionConfigurator::ApplicationState::CLAIMED, appStateListener.stateMap[peer1Bus.GetUniqueName()]);

    //Create peer2 identityCert
    IdentityCertificate identityCertChainPeer2[certChainSize];


    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  peer2Key.GetPublicKey(),
                                                                  "Peer2Alias",
                                                                  3600,
                                                                  identityCertChainPeer2[0],
                                                                  digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";
    /* set claimable */
    peer2Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    EXPECT_EQ(ER_OK, sapWithPeer2.Claim(managerKey,
                                        managerGuid,
                                        managerKey,
                                        identityCertChainPeer2, certChainSize,
                                        manifest, manifestSize));

    for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
        if (appStateListener.isClaimed(peer2Bus.GetUniqueName())) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }

    ASSERT_EQ(PermissionConfigurator::ApplicationState::CLAIMED, appStateListener.stateMap[peer2Bus.GetUniqueName()]);

    //Change the managerBus so it only uses ECDHE_ECDSA
    EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", managerAuthListener));

    String membershipSerial = "1";
    qcc::MembershipCertificate managerMembershipCertificate[1];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                    managerBus,
                                                                    managerBus.GetUniqueName(),
                                                                    managerKey.GetPublicKey(),
                                                                    managerGuid,
                                                                    false,
                                                                    3600,
                                                                    managerMembershipCertificate[0]
                                                                    ));
    EXPECT_EQ(ER_OK, sapWithManagerBus.InstallMembership(managerMembershipCertificate, 1));

    qcc::MembershipCertificate peer1MembershipCertificate[1];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                    managerBus,
                                                                    peer1Bus.GetUniqueName(),
                                                                    peer1Key.GetPublicKey(),
                                                                    managerGuid,
                                                                    false,
                                                                    3600,
                                                                    peer1MembershipCertificate[0]
                                                                    ));
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", managerAuthListener, NULL, false));
    EXPECT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1MembershipCertificate, 1));

    qcc::MembershipCertificate peer2MembershipCertificate[1];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                    managerBus,
                                                                    peer2Bus.GetUniqueName(),
                                                                    peer2Key.GetPublicKey(),
                                                                    managerGuid,
                                                                    false,
                                                                    3600,
                                                                    peer2MembershipCertificate[0]
                                                                    ));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", managerAuthListener, NULL, false));
    EXPECT_EQ(ER_OK, sapWithPeer2.InstallMembership(peer2MembershipCertificate, 1));

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.SecureConnection(true));
}

void SecurityWildCardPolicyRulesTest::TearDown()
{
    managerBus.Stop();
    managerBus.Join();

    peer1Bus.Stop();
    peer1Bus.Join();

    peer2Bus.Stop();
    peer2Bus.Join();

    delete managerAuthListener;
    delete peer1AuthListener;
    delete peer2AuthListener;
}

QStatus SecurityWildCardPolicyRulesTest::UpdatePolicyWithValuesFromDefaultPolicy(const PermissionPolicy& defaultPolicy,
                                                                                 PermissionPolicy& policy,
                                                                                 bool keepCAentry,
                                                                                 bool keepAdminGroupEntry,
                                                                                 bool keepInstallMembershipEntry) {

    size_t count = policy.GetAclsSize();
    if (keepCAentry) {
        ++count;
    }
    if (keepAdminGroupEntry) {
        ++count;
    }
    if (keepInstallMembershipEntry) {
        ++count;
    }

    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[count];
    size_t idx = 0;
    for (size_t cnt = 0; cnt < defaultPolicy.GetAclsSize(); ++cnt) {
        if (defaultPolicy.GetAcls()[cnt].GetPeersSize() > 0) {
            if (defaultPolicy.GetAcls()[cnt].GetPeers()[0].GetType() == PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY) {
                if (keepCAentry) {
                    acls[idx++] = defaultPolicy.GetAcls()[cnt];
                }
            } else if (defaultPolicy.GetAcls()[cnt].GetPeers()[0].GetType() == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) {
                if (keepAdminGroupEntry) {
                    acls[idx++] = defaultPolicy.GetAcls()[cnt];
                }
            } else if (defaultPolicy.GetAcls()[cnt].GetPeers()[0].GetType() == PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY) {
                if (keepInstallMembershipEntry) {
                    acls[idx++] = defaultPolicy.GetAcls()[cnt];
                }
            }
        }

    }
    for (size_t cnt = 0; cnt < policy.GetAclsSize(); ++cnt) {
        assert(idx <= count);
        acls[idx++] = policy.GetAcls()[cnt];
    }
    policy.SetAcls(count, acls);
    delete [] acls;
    return ER_OK;
}

/*
 * Purpose:
 * Verify that wild cards can be used in the Object Path
 * Set-Up:
 * Sender local policy rules
 * Rule 1:  Object path: /test/arg*  Interface: *  Member Name: * Member Type: (NS)  Action: PROVIDE
 *
 * Sender makes method call "march".
 * Sender makes method call "mint".
 *
 * Verification:
 * Verify that "march" method call is successful.
 * Verify that "mint" method call is not sent by the sender.
 */
TEST_F(SecurityWildCardPolicyRulesTest, Wildcard_object_path) {
    ArgentinaTestBusObject peer1ArgentinaBusObject(peer1Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArgentinaBusObject));
    ArabicTestBusObject peer1ArabicBusObject(peer1Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArabicBusObject));

    ArgentinaTestBusObject peer2ArgentinaBusObject(peer2Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArgentinaBusObject));
    ArabicTestBusObject peer2ArabicBusObject(peer2Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArabicBusObject));

    // Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("/test/arg*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].SetMemberName("*");
                members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());

    /* Create the ProxyBusObject and call the march method on the test.calcium interface */
    ProxyBusObject argentinaProxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), test::argentinaObjPath, peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, argentinaProxy.IntrospectRemoteObject());

    EXPECT_TRUE(argentinaProxy.ImplementsInterface(test::calcium::InterfaceName)) << test::calcium::InterfaceName;
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, argentinaProxy.MethodCall(test::calcium::InterfaceName, test::calcium::method::march, NULL, 0, replyMsg));

    /* Create the ProxyBusObject and call the march method on the test.calcium interface */
    ProxyBusObject arabicProxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), test::arabicObjPath, peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, arabicProxy.IntrospectRemoteObject());

    EXPECT_TRUE(arabicProxy.ImplementsInterface(test::cashew::InterfaceName)) << test::cashew::InterfaceName;
    EXPECT_EQ(ER_PERMISSION_DENIED, arabicProxy.MethodCall(test::cashew::InterfaceName, test::cashew::method::mint, NULL, 0, replyMsg));
}

/*
 * Purpose:
 * Verify that wild cards can be used in the Interface name
 *
 * Set-Up:
 * Sender local policy rules
 * Rule 1:  Object path: *  Interface: test.cal*  Member Name: * Member Type: (NS)  Action: PROVIDE
 *
 * Sender makes method call "march".
 * Sender makes get property call "meal".
 * Sender makes a method call  "mob".
 * Sender makes a get property call "mini".
 *
 * Verification:
 * Verify that "march" method call is successful.
 * Verify that "meal" get property call is successful.
 * Verify that "mob" method call is not sent by the sender.
 * Verify that "mini" get property call is not sent by the sender.
 */
TEST_F(SecurityWildCardPolicyRulesTest, Wildcard_interface_names) {
    ArgentinaTestBusObject peer1ArgentinaBusObject(peer1Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArgentinaBusObject));
    ArabicTestBusObject peer1ArabicBusObject(peer1Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArabicBusObject));

    ArgentinaTestBusObject peer2ArgentinaBusObject(peer2Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArgentinaBusObject));
    ArabicTestBusObject peer2ArabicBusObject(peer2Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArabicBusObject));

    // Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("test.cal*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].SetMemberName("*");
                members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());

    /* Create the ProxyBusObject and call the march method on the test.calcium interface */
    ProxyBusObject argentinaProxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), test::argentinaObjPath, peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, argentinaProxy.IntrospectRemoteObject());

    EXPECT_TRUE(argentinaProxy.ImplementsInterface(test::calcium::InterfaceName)) << test::calcium::InterfaceName;
    EXPECT_TRUE(argentinaProxy.ImplementsInterface(test::california::InterfaceName)) << test::california::InterfaceName;
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, argentinaProxy.MethodCall(test::calcium::InterfaceName, test::calcium::method::march, NULL, 0, replyMsg));

    MsgArg mealArg;
    EXPECT_EQ(ER_OK, argentinaProxy.GetProperty(test::california::InterfaceName, test::california::property::meal, mealArg));

    /* Create the ProxyBusObject and call the march method on the test.calcium interface */
    ProxyBusObject arabicProxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), test::arabicObjPath, peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, arabicProxy.IntrospectRemoteObject());

    EXPECT_TRUE(arabicProxy.ImplementsInterface(test::camera::InterfaceName)) << test::camera::InterfaceName;
    EXPECT_EQ(ER_PERMISSION_DENIED, arabicProxy.MethodCall(test::camera::InterfaceName, test::camera::method::mob, NULL, 0, replyMsg));

    MsgArg miniArg;
    EXPECT_EQ(ER_PERMISSION_DENIED, argentinaProxy.GetProperty(test::cashew::InterfaceName, test::cashew::property::mini, miniArg));
}

/*
 * Purpose:
 * Verify that wild cards can be used in member names
 *
 * Set-Up:
 * Sender local policy rules
 * Rule 1: Object Path: *  Interface: *  Member Name: mi*  Member Type: (NS)  Action: PROVIDE
 *
 * Sender makes a method call "metal".
 * Sender makes a method call  "mint".
 *
 * Verification:
 * Verify that "mint" method call is successful.
 * Verify that "metal" method call is not sent by the sender.
 */
TEST_F(SecurityWildCardPolicyRulesTest, Wildcard_member_names) {
    ArgentinaTestBusObject peer1ArgentinaBusObject(peer1Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArgentinaBusObject));
    ArabicTestBusObject peer1ArabicBusObject(peer1Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArabicBusObject));

    ArgentinaTestBusObject peer2ArgentinaBusObject(peer2Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArgentinaBusObject));
    ArabicTestBusObject peer2ArabicBusObject(peer2Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArabicBusObject));

    // Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].SetMemberName("mi*");
                members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());

    /* Create the ProxyBusObject and call the march method on the test.calcium interface */
    ProxyBusObject argentinaProxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), test::argentinaObjPath, peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, argentinaProxy.IntrospectRemoteObject());

    EXPECT_TRUE(argentinaProxy.ImplementsInterface(test::california::InterfaceName)) << test::california::InterfaceName;
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_PERMISSION_DENIED, argentinaProxy.MethodCall(test::california::InterfaceName, test::california::method::metal, NULL, 0, replyMsg));
    ASSERT_STREQ("org.alljoyn.Bus.ErStatus", replyMsg->GetErrorName());
    EXPECT_EQ(ER_PERMISSION_DENIED, (QStatus)replyMsg->GetArg(1)->v_uint16)
        << "\n" << replyMsg->GetArg(0)->ToString().c_str()
        << "\n" << replyMsg->GetArg(1)->ToString().c_str();

    /* Create the ProxyBusObject and call the mint method on the test.cashew interface */
    ProxyBusObject arabicProxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), test::arabicObjPath, peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, arabicProxy.IntrospectRemoteObject());

    EXPECT_TRUE(arabicProxy.ImplementsInterface(test::cashew::InterfaceName)) << test::cashew::InterfaceName;
    EXPECT_EQ(ER_OK, arabicProxy.MethodCall(test::cashew::InterfaceName, test::cashew::method::mint, NULL, 0, replyMsg));
}

/*
 * Purpose:
 * Verify that Message type is matched properly in the rule.
 *
 * Set-Up:
 * Sender local policy rules
 * Rule 1: Object Path: *  Interface: *  Member Name: *  Member Type: Method  Action: PROVIDE|OBSERVE
 *
 * Sender sends a signal "money".
 * Sender Sender makes a get property call "motel".
 * Sender Sender makes a method call  "mob".
 *
 * Verification:
 * Verify that "money" signal is not sent by the sender.
 * Verify that "motel" get property call is not sent by the sender.
 * Verify that "mob" method call is successful.
 */
TEST_F(SecurityWildCardPolicyRulesTest, Wildcard_message_type_matched_properly_in_rule) {
    ArgentinaTestBusObject peer1ArgentinaBusObject(peer1Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArgentinaBusObject));
    ArabicTestBusObject peer1ArabicBusObject(peer1Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArabicBusObject));

    ArgentinaTestBusObject peer2ArgentinaBusObject(peer2Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArgentinaBusObject));
    ArabicTestBusObject peer2ArabicBusObject(peer2Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArabicBusObject));

    // Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].SetMemberName("*");
                members[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
                members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                                         PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());

    /* Create the ProxyBusObject and call the mob method on the test.camera interface */
    ProxyBusObject arabicProxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), test::arabicObjPath, peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, arabicProxy.IntrospectRemoteObject());

    Message replyMsg(peer1Bus);
    EXPECT_TRUE(arabicProxy.ImplementsInterface(test::camera::InterfaceName)) << test::camera::InterfaceName;
    EXPECT_EQ(ER_OK, arabicProxy.MethodCall(test::camera::InterfaceName, test::camera::method::mob, NULL, 0, replyMsg));

    MsgArg motelArg;
    EXPECT_EQ(ER_PERMISSION_DENIED, arabicProxy.GetProperty(test::camera::InterfaceName, test::camera::property::motel, motelArg));

    SecurityWildCardSignalReceiver moneySignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&moneySignalReceiver,
                                                    static_cast<MessageReceiver::SignalHandler>(&SecurityWildCardSignalReceiver::SignalHandler),
                                                    peer1Bus.GetInterface(test::camera::InterfaceName)->GetMember(test::camera::signal::money), NULL));

    const InterfaceDescription::Member* moneyMember = peer1Bus.GetInterface(test::camera::InterfaceName)->GetMember(test::camera::signal::money);
    EXPECT_EQ(ER_PERMISSION_DENIED, peer1ArabicBusObject.Signal(peer2Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId, *moneyMember, NULL, 0));
}

/*
 * Purpose:
 * Verify that if the action mask is not specified, then the rule is explicitly
 * DENIED. The other rules are not even considered.
 *
 * Set-Up:
 * Sender local policy rules
 * Peer type: WITH_PUBIC_KEY KeyInfo: Peer2 Key
 * Rule 1: Object Path: *  Interface: *  Member Name:  *;   Member Type; NS;  Action: Not Specified
 * Rule 2: Object Path: *  Interface: *  Member Name:  *;   Member Type; NS;  Action: PROVIDE|OBSERVE
 *
 * Sender sends a signal "mess". It should not be sent.
 * Sender makes a get property call "meal". It should not be sent.
 * Sender makes a method call  "metal". It should not be sent.
 *
 * Verification:
 * Verify that "mess" signal is not sent by the sender.
 * Verify that "meal" get property call is not sent by the sender.
 * Verify that "metal" method call is not sent by the sender.
 */
TEST_F(SecurityWildCardPolicyRulesTest, unspecified_action_mask_is_explicitly_DENIED) {
    ArgentinaTestBusObject peer1ArgentinaBusObject(peer1Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArgentinaBusObject));
    ArabicTestBusObject peer1ArabicBusObject(peer1Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArabicBusObject));

    ArgentinaTestBusObject peer2ArgentinaBusObject(peer2Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArgentinaBusObject));
    ArabicTestBusObject peer2ArabicBusObject(peer2Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArabicBusObject));

    // Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            KeyInfoNISTP256 peer2Key;
            PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));
            peers[0].SetKeyInfo(&peer2Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[2];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].SetMemberName("*");
                rules[0].SetMembers(1, members);
            }
            rules[1].SetObjPath("*");
            rules[1].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].SetMemberName("*");
                members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                                         PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[1].SetMembers(1, members);
            }
            acls[0].SetRules(2, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());

    /* Create the ProxyBusObject and call the mob method on the test.camera interface */
    ProxyBusObject argentinaProxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), test::argentinaObjPath, peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, argentinaProxy.IntrospectRemoteObject());

    Message replyMsg(peer1Bus);
    EXPECT_TRUE(argentinaProxy.ImplementsInterface(test::california::InterfaceName)) << test::california::InterfaceName;
    EXPECT_EQ(ER_PERMISSION_DENIED, argentinaProxy.MethodCall(test::california::InterfaceName, test::california::method::metal, NULL, 0, replyMsg));
    ASSERT_STREQ("org.alljoyn.Bus.ErStatus", replyMsg->GetErrorName());
    EXPECT_EQ(ER_PERMISSION_DENIED, (QStatus)replyMsg->GetArg(1)->v_uint16)
        << "\n" << replyMsg->GetArg(0)->ToString().c_str()
        << "\n" << replyMsg->GetArg(1)->ToString().c_str();

    MsgArg motelArg;
    EXPECT_EQ(ER_PERMISSION_DENIED, argentinaProxy.GetProperty(test::california::InterfaceName, test::california::property::meal, motelArg));

    SecurityWildCardSignalReceiver messSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&messSignalReceiver,
                                                    static_cast<MessageReceiver::SignalHandler>(&SecurityWildCardSignalReceiver::SignalHandler),
                                                    peer1Bus.GetInterface(test::california::InterfaceName)->GetMember(test::california::signal::mess), NULL));

    const InterfaceDescription::Member* messMember = peer1Bus.GetInterface(test::california::InterfaceName)->GetMember(test::california::signal::mess);
    EXPECT_EQ(ER_PERMISSION_DENIED, peer1ArgentinaBusObject.Signal(peer2Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId, *messMember, NULL, 0));
}

/*
 * Purpose:
 * If the Object Path is not specified, then the rule is not considered as a match
 *
 * Set-Up:
 * Sender local policy rules
 * Rule 1: Object Path: NS  Interface: *  Member Name:  *;   Member Type; NS;  Action: PROVIDE
 *
 * Sender sends a method call "march".
 *
 * Verification:
 * Verify that "march" method call is not sent by the sender.
 */
TEST_F(SecurityWildCardPolicyRulesTest, object_path_not_specified_rule_not_considered_as_match) {
    ArgentinaTestBusObject peer1ArgentinaBusObject(peer1Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArgentinaBusObject));
    ArabicTestBusObject peer1ArabicBusObject(peer1Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArabicBusObject));

    ArgentinaTestBusObject peer2ArgentinaBusObject(peer2Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArgentinaBusObject));
    ArabicTestBusObject peer2ArabicBusObject(peer2Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArabicBusObject));

    // Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetInterfaceName("*");
            rules[0].SetObjPath("");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].SetMemberName("*");
                members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());

    /* Create the ProxyBusObject and call the march method on the test.calcium interface */
    ProxyBusObject argentinaProxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), test::argentinaObjPath, peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, argentinaProxy.IntrospectRemoteObject());

    EXPECT_TRUE(argentinaProxy.ImplementsInterface(test::calcium::InterfaceName)) << test::calcium::InterfaceName;
    Message replyMsg(peer1Bus);
    ASSERT_EQ(ER_PERMISSION_DENIED, argentinaProxy.MethodCall(test::calcium::InterfaceName, test::calcium::method::march, NULL, 0, replyMsg));
    ASSERT_STREQ(org::alljoyn::Bus::ErrorName, replyMsg->GetErrorName());
    EXPECT_EQ(ER_PERMISSION_DENIED, static_cast<QStatus>(replyMsg->GetArg(1)->v_uint16));
}

/*
 * Purpose:
 * If the Interface name is not specified, then the rule is not considered as a match.
 *
 * Set-Up:
 * Sender local policy rules
 * Rule 1: Object Path: *  Interface: NS  Member Name:  *;   Member Type; NS;  Action: PROVIDE
 *
 * Sender sends a method call "march".
 *
 * Verification:
 * Verify that "march" method call is not sent by the sender.
 */
TEST_F(SecurityWildCardPolicyRulesTest, interface_name_not_specified_rule_not_considered_as_match) {
    ArgentinaTestBusObject peer1ArgentinaBusObject(peer1Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArgentinaBusObject));
    ArabicTestBusObject peer1ArabicBusObject(peer1Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArabicBusObject));

    ArgentinaTestBusObject peer2ArgentinaBusObject(peer2Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArgentinaBusObject));
    ArabicTestBusObject peer2ArabicBusObject(peer2Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArabicBusObject));

    // Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].SetMemberName("*");
                members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());

    /* Create the ProxyBusObject and call the march method on the test.calcium interface */
    ProxyBusObject argentinaProxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), test::argentinaObjPath, peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, argentinaProxy.IntrospectRemoteObject());

    EXPECT_TRUE(argentinaProxy.ImplementsInterface(test::calcium::InterfaceName)) << test::calcium::InterfaceName;
    Message replyMsg(peer1Bus);
    ASSERT_EQ(ER_PERMISSION_DENIED, argentinaProxy.MethodCall(test::calcium::InterfaceName, test::calcium::method::march, NULL, 0, replyMsg));
    ASSERT_STREQ(org::alljoyn::Bus::ErrorName, replyMsg->GetErrorName());
    EXPECT_EQ(ER_PERMISSION_DENIED, static_cast<QStatus>(replyMsg->GetArg(1)->v_uint16));
}

/*
 * Purpose:
 * Verify that empty string is not considered as a match.
 *
 * Set-Up:
 * Sender local policy rules
 * Rule 1: Object Path: *  Interface: *  Member Name:  "" (Empty String);   Member Type:  NS;  Action: PROVIDE
 *
 * Sender sends a method call "march".
 *
 * Verification:
 * Verify that "march" method call is not sent by the sender.
 */
TEST_F(SecurityWildCardPolicyRulesTest, empty_string_not_considered_as_match) {
    ArgentinaTestBusObject peer1ArgentinaBusObject(peer1Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArgentinaBusObject));
    ArabicTestBusObject peer1ArabicBusObject(peer1Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1ArabicBusObject));

    ArgentinaTestBusObject peer2ArgentinaBusObject(peer2Bus, test::argentinaObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArgentinaBusObject));
    ArabicTestBusObject peer2ArabicBusObject(peer2Bus, test::arabicObjPath);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2ArabicBusObject));

    // Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].SetMemberName("");
                members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());

    /* Create the ProxyBusObject and call the march method on the test.calcium interface */
    ProxyBusObject argentinaProxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), test::argentinaObjPath, peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, argentinaProxy.IntrospectRemoteObject());

    EXPECT_TRUE(argentinaProxy.ImplementsInterface(test::calcium::InterfaceName)) << test::calcium::InterfaceName;
    Message replyMsg(peer1Bus);
    ASSERT_EQ(ER_PERMISSION_DENIED, argentinaProxy.MethodCall(test::calcium::InterfaceName, test::calcium::method::march, NULL, 0, replyMsg));
    ASSERT_STREQ(org::alljoyn::Bus::ErrorName, replyMsg->GetErrorName());
    EXPECT_EQ(ER_PERMISSION_DENIED, static_cast<QStatus>(replyMsg->GetArg(1)->v_uint16));
}
