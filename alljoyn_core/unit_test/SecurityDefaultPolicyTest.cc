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

class DefaultPolicy_ApplicationStateListener : public ApplicationStateListener {
  public:
    DefaultPolicy_ApplicationStateListener() : stateMap() { }

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

class DefaultPolicyTestSessionPortListener : public SessionPortListener {
  public:
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
};

class DefaultRulesTestBusObject : public BusObject {
  public:
    DefaultRulesTestBusObject(BusAttachment& bus, const char* path, const char* interfaceName, bool announce = true)
        : BusObject(path), isAnnounced(announce), prop1(42), prop2(17) {
        const InterfaceDescription* iface = bus.GetInterface(interfaceName);
        EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for " << interfaceName;
        if (iface == NULL) {
            printf("The interfaceDescription pointer for %s was NULL when it should not have been.\n", interfaceName);
            return;
        }

        if (isAnnounced) {
            AddInterface(*iface, ANNOUNCED);
        } else {
            AddInterface(*iface, UNANNOUNCED);
        }

        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { iface->GetMember("Echo"), static_cast<MessageReceiver::MethodHandler>(&DefaultRulesTestBusObject::Echo) }
        };
        EXPECT_EQ(ER_OK, AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0])));
    }

    void Echo(const InterfaceDescription::Member* member, Message& msg) {
        QCC_UNUSED(member);
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = MethodReply(msg, arg, 1);
        EXPECT_EQ(ER_OK, status) << "Echo: Error sending reply";
    }

    QStatus Get(const char* ifcName, const char* propName, MsgArg& val)
    {
        QCC_UNUSED(ifcName);
        QStatus status = ER_OK;
        if (0 == strcmp("Prop1", propName)) {
            val.Set("i", prop1);
        } else if (0 == strcmp("Prop2", propName)) {
            val.Set("i", prop2);
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;

    }

    QStatus Set(const char* ifcName, const char* propName, MsgArg& val)
    {
        QCC_UNUSED(ifcName);
        QStatus status = ER_OK;
        if ((0 == strcmp("Prop1", propName)) && (val.typeId == ALLJOYN_INT32)) {
            val.Get("i", &prop1);
        } else if ((0 == strcmp("Prop2", propName)) && (val.typeId == ALLJOYN_INT32)) {
            val.Get("i", &prop2);
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;
    }
    int32_t ReadProp1() {
        return prop1;
    }
  private:
    bool isAnnounced;
    int32_t prop1;
    int32_t prop2;
};

class ChirpSignalReceiver : public MessageReceiver {
  public:
    ChirpSignalReceiver() : signalReceivedFlag(false) { }
    void ChirpSignalHandler(const InterfaceDescription::Member* member,
                            const char* sourcePath, Message& msg) {
        QCC_UNUSED(member);
        QCC_UNUSED(sourcePath);
        QCC_UNUSED(msg);
        signalReceivedFlag = true;
    }
    bool signalReceivedFlag;
};

class SecurityDefaultPolicyTest : public testing::Test {
  public:
    SecurityDefaultPolicyTest() :
        managerBus("SecurityPolicyRulesManager"),
        peer1Bus("SecurityPolicyRulesPeer1"),
        peer2Bus("SecurityPolicyRulesPeer2"),
        managerSessionPort(42),
        peer1SessionPort(42),
        peer2SessionPort(42),
        managerToManagerSessionId(0),
        managerToPeer1SessionId(0),
        managerToPeer2SessionId(0),
        interfaceName("org.allseen.test.SecurityApplication.rules"),
        managerAuthListener(NULL),
        peer1AuthListener(NULL),
        peer2AuthListener(NULL),
        appStateListener()
    {
    }

    virtual void SetUp() {
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

        EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", managerAuthListener));
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", peer1AuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", peer2AuthListener));

        // We are not marking the interface as a secure interface. Some of the
        // tests don't use security. So we use Object based security for any
        // test that security is required.
        interface = "<node>"
                    "<interface name='" + String(interfaceName) + "'>"
                    "  <method name='Echo'>"
                    "    <arg name='shout' type='s' direction='in'/>"
                    "    <arg name='reply' type='s' direction='out'/>"
                    "  </method>"
                    "  <signal name='Chirp'>"
                    "    <arg name='tweet' type='s'/>"
                    "  </signal>"
                    "  <property name='Prop1' type='i' access='readwrite'/>"
                    "  <property name='Prop2' type='i' access='readwrite'/>"
                    "</interface>"
                    "</node>";

        EXPECT_EQ(ER_OK, managerBus.CreateInterfacesFromXml(interface.c_str()));
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
        EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationStateManager);

        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        PermissionConfigurator::ApplicationState applicationStatePeer1;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetApplicationState(applicationStatePeer1));
        EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer1);

        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        PermissionConfigurator::ApplicationState applicationStatePeer2;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetApplicationState(applicationStatePeer2));
        EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationStatePeer2);

        managerBus.RegisterApplicationStateListener(appStateListener);
        managerBus.AddApplicationStateRule();

        // All Inclusive manifest
        const size_t manifestSize = 1;
        PermissionPolicy::Rule manifest[manifestSize];
        manifest[0].SetObjPath("*");
        manifest[0].SetInterfaceName("*");
        {
            PermissionPolicy::Rule::Member members[1];
            members[0].Set("*",
                           PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                           PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                           PermissionPolicy::Rule::Member::ACTION_MODIFY |
                           PermissionPolicy::Rule::Member::ACTION_OBSERVE);
            manifest[0].SetMembers(1, members);
        }
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
        sapWithManager.GetEccPublicKey(managerPublicKey);
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
        EXPECT_EQ(ER_OK, sapWithPeer2.Claim(managerKey,
                                            managerGuid,
                                            managerKey,
                                            identityCertChainPeer2, certChainSize,
                                            manifest, manifestSize));

        for (int msec = 0; msec < 10000; msec += WAIT_MSECS) {
            if (appStateListener.isClaimed(peer1Bus.GetUniqueName())) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }

        ASSERT_EQ(PermissionConfigurator::ApplicationState::CLAIMED, appStateListener.stateMap[peer1Bus.GetUniqueName()]);

        EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", managerAuthListener));
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", managerAuthListener));
        EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", managerAuthListener));
    }

    virtual void TearDown() {
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

    void InstallMemberShipOnManager() {
        //Get manager key
        KeyInfoNISTP256 managerKey;
        PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

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
        SecurityApplicationProxy sapWithManagerBus(managerBus, managerBus.GetUniqueName().c_str());
        EXPECT_EQ(ER_OK, sapWithManagerBus.InstallMembership(managerMembershipCertificate, 1));
    }

    void InstallMemberShipOnPeer1() {
        //Create peer1 key
        KeyInfoNISTP256 peer1Key;
        PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

        String membershipSerial = "1";
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
        SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1MembershipCertificate, 1));
    }

    void InstallMemberShipOnPeer2() {
        //Create peer2 key
        KeyInfoNISTP256 peer2Key;
        PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
        EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));

        String membershipSerial = "1";
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
        SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
        EXPECT_EQ(ER_OK, sapWithPeer2.InstallMembership(peer2MembershipCertificate, 1));
    }

    /*
     * Creates a PermissionPolicy that allows everything.
     * @policy[out] the policy to set
     * @version[in] the version number for the policy
     */
    void GeneratePermissivePolicy(PermissionPolicy& policy, uint32_t version) {
        policy.SetVersion(version);
        {
            PermissionPolicy::Acl acls[1];
            {
                PermissionPolicy::Peer peers[1];
                peers[0].SetType(PermissionPolicy::Peer::PEER_ALL);
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
            policy.SetAcls(1, acls);
        }
    }

    QStatus UpdatePolicyWithValuesFromDefaultPolicy(const PermissionPolicy& defaultPolicy,
                                                    PermissionPolicy& policy,
                                                    bool keepCAentry = true,
                                                    bool keepAdminGroupEntry = false,
                                                    bool keepInstallMembershipEntry = false) {
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
    BusAttachment managerBus;
    BusAttachment peer1Bus;
    BusAttachment peer2Bus;

    SessionPort managerSessionPort;
    SessionPort peer1SessionPort;
    SessionPort peer2SessionPort;

    DefaultPolicyTestSessionPortListener managerSessionPortListener;
    DefaultPolicyTestSessionPortListener peer1SessionPortListener;
    DefaultPolicyTestSessionPortListener peer2SessionPortListener;

    SessionId managerToManagerSessionId;
    SessionId managerToPeer1SessionId;
    SessionId managerToPeer2SessionId;

    InMemoryKeyStoreListener managerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;

    String interface;
    const char* interfaceName;
    DefaultECDHEAuthListener* managerAuthListener;
    DefaultECDHEAuthListener* peer1AuthListener;
    DefaultECDHEAuthListener* peer2AuthListener;

    DefaultPolicy_ApplicationStateListener appStateListener;

    //Random GUID used for the SecurityManager
    GUID128 managerGuid;
};

/*
 * Purpose:
 * On the app's default policy, ASG member can send and receive messages
 * securely with the claimed app.
 *
 * app bus  implements the following message types: method call, signal,
 * property 1, property 2.
 * ASG bus implements the following message types: method call, signal, property
 * 1, property 2.
 *
 * app. bus is claimed by the ASGA.
 * ASG bus has a MC signed by ASGA.
 * app. bus has default policy.
 * ASG bus has a policy that allow everything.
 *
 * ASG bus and app. bus have enabled ECDHE_ECDSA auth. mechanism.
 * Both peers have a default manifest that allows everything.
 *
 * 1. App. bus makes a method call, get property call, set property call, getall
 *    properties call on the ASG bus.
 * 2. App. bus sends a signal to to the ASG bus.
 * 3. ASG bus makes a method call, get property call, set property call, getall
 *    properties call on the app. bus.
 * 4. ASG bus sends a signal to the app. bus.
 * 5. ASG bus calls Reset on the app. bus.
 *
 * Verification:
 * 1. Method call, get property, set property, getall properties are successful.
 * 2. The signal is received by the ASG bus.
 * 3. Method call, get property, set property, getall properties are successful.
 * 4. The signal is received by the app. bus.
 * 5. Verify that Reset method call was successful.
 *
 * In this test managerBus == ASGA
 *              peer1Bus == ASA bus
 *              peer2Bus == app. bus
 */
TEST_F(SecurityDefaultPolicyTest, DefaultPolicy_ECDSA_everything_passes)
{
    InstallMemberShipOnManager();
    InstallMemberShipOnPeer1();

    DefaultRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject, true));
    DefaultRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject, true));

    /* install all permissive permission policy for Peer1*/
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    GeneratePermissivePolicy(peer1Policy, 1);
    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    {
        PermissionPolicy defaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(defaultPolicy));
        EXPECT_EQ(ER_OK, UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer1Policy));
    }
    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    ProxyBusObject proxy[2];
    proxy[0] = ProxyBusObject(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    proxy[1] = ProxyBusObject(peer2Bus, peer1Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    for (int i = 0; i < 2; ++i) {
        EXPECT_EQ(ER_OK, proxy[i].ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy[i].ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

        // Verify Method call
        MsgArg arg("s", "String that should be Echoed back.");
        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_OK, proxy[i].MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg))
            << "Peer" << i + 1 << " failed make MethodCall call " << replyMsg->GetErrorDescription().c_str();
        char* echoReply;
        replyMsg->GetArg(0)->Get("s", &echoReply);
        EXPECT_STREQ("String that should be Echoed back.", echoReply);

        // Verify Set/Get Property and GetAll Properties
        MsgArg prop1Arg;
        EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
        EXPECT_EQ(ER_OK, proxy[i].SetProperty(interfaceName, "Prop1", prop1Arg)) << "Peer" << i + 1 << " failed SetProperty call";
        EXPECT_EQ(513, peer2BusObject.ReadProp1());

        MsgArg prop1ArgOut;
        EXPECT_EQ(ER_OK, proxy[i].GetProperty(interfaceName, "Prop1", prop1Arg)) << "Peer" << i + 1 << " failed GetProperty call";;
        int32_t prop1;
        prop1Arg.Get("i", &prop1);
        EXPECT_EQ(513, prop1);

        MsgArg props;
        EXPECT_EQ(ER_OK, proxy[i].GetAllProperties(interfaceName, props)) << "Peer" << i + 1 << " failed GetAllProperties call";;
        MsgArg* propArg;
        int32_t prop2;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop1", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop1)) << propArg->ToString().c_str();
        EXPECT_EQ(513, prop1);

        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop2", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop2)) << propArg->ToString().c_str();
        EXPECT_EQ(17, prop2);
    }

    // Peer1 can Send Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

    MsgArg arg;
    arg.Set("s", "Chirp this String out in the signal.");
    // Signals are send and forget.  They will always return ER_OK.
    EXPECT_EQ(ER_OK, peer1BusObject.Signal(peer2Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId, *peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), &arg, 1, 0, 0));

    //Wait for a maximum of 2 sec for the Chirp Signal.
    for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
        if (chirpSignalReceiver.signalReceivedFlag) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag) << "Peer2 failed to receive the Signal from Peer1";
    peer2Bus.UnregisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL);


    // Peer2 can Send Signal
    EXPECT_EQ(ER_OK, peer1Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

    // Signals are send and forget.  They will always return ER_OK.
    EXPECT_EQ(ER_OK, peer2BusObject.Signal(peer1Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId, *peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"), &arg, 1, 0, 0));

    //Wait for a maximum of 2 sec for the Chirp Signal.
    for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
        if (chirpSignalReceiver.signalReceivedFlag) {
            break;
        }
        qcc::Sleep(WAIT_MSECS);
    }
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag) << "Peer1 failed to receive the Signal from Peer2";
    peer1Bus.UnregisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL);

    SecurityApplicationProxy sapPeer1toPeer2(peer1Bus, peer2Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId);
    EXPECT_EQ(ER_OK, sapPeer1toPeer2.Reset());

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose:
 * ASGA cannot access the app. bus if it does not have a membership certificate
 * belonging to the ASG. (Membership certificate is must even if the bus is the ASGA.)
 *
 * Setup:
 * app bus  implements the following message types: method call, signal, property 1, property 2.
 * ASGA bus implements the following message types: method call, signal, property 1, property 2.
 *
 * app. bus is claimed by the ASGA.
 * ASGA does not have a  MC belonging to the ASG.
 * app. bus has default policy.
 *
 * ASG bus and app. bus have enabled ECDHE_ECDSA auth. mechanism.
 *
 * 1. ASGA bus makes a method call, get property call, set property call, getall properties call on the app. bus.
 * 2. ASGA bus sends a signal to the app. bus.
 * 3. ASGA bus calls Reset on the app. bus.
 *
 * Verification:
 * 1. Method call, get property, set property, getall properties are not received by the app. bus.
 * 2. The signal is not received by the app. bus.
 * 3. Reset method call should fail.
 *
 * In this test managerBus == ASGA
 *              peer1Bus == app. bus
 */
TEST_F(SecurityDefaultPolicyTest, DefaultPolicy_manager_must_have_certificate_to_interact_with_peers)
{
    DefaultRulesTestBusObject managerBusObject(managerBus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, managerBus.RegisterBusObject(managerBusObject, true));
    DefaultRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject, true));

    SessionOpts opts;
    SessionId managerToPeer1SessionId;
    EXPECT_EQ(ER_OK, managerBus.JoinSession(peer1Bus.GetUniqueName().c_str(), peer1SessionPort, NULL, managerToPeer1SessionId, opts));

    ProxyBusObject managerToPeer1Proxy = ProxyBusObject(managerBus, peer1Bus.GetUniqueName().c_str(), "/test", managerToPeer1SessionId, true);
    {
        EXPECT_EQ(ER_OK, managerToPeer1Proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(managerToPeer1Proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

        // Verify Method call
        MsgArg arg("s", "String that should be Echoed back.");
        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_PERMISSION_DENIED, managerToPeer1Proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
        EXPECT_STREQ("org.alljoyn.Bus.Security.Error.PermissionDenied", replyMsg->GetErrorName());

        // Verify Set/Get Property and GetAll Properties
        MsgArg prop1Arg;
        EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
        EXPECT_EQ(ER_PERMISSION_DENIED, managerToPeer1Proxy.SetProperty(interfaceName, "Prop1", prop1Arg)) << "Peer failed SetProperty call";

        MsgArg prop1ArgOut;
        EXPECT_EQ(ER_PERMISSION_DENIED, managerToPeer1Proxy.GetProperty(interfaceName, "Prop1", prop1Arg)) << "Peer failed GetProperty call";;

        MsgArg props;
        EXPECT_EQ(ER_PERMISSION_DENIED, managerToPeer1Proxy.GetAllProperties(interfaceName, props)) << "Peer failed GetAllProperties call";;
    }
    {
        // manager can Send Signal peer1 will not get signal it is blocked by default policy.
        ChirpSignalReceiver chirpSignalReceiver;
        EXPECT_EQ(ER_OK, peer1Bus.RegisterSignalHandler(&chirpSignalReceiver,
                                                        static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                                        managerBus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

        MsgArg arg;
        arg.Set("s", "Chirp this String out in the signal.");
        // Signals are send and forget.  They will always return ER_OK.
        EXPECT_EQ(ER_OK, managerBusObject.Signal(peer1Bus.GetUniqueName().c_str(),
                                                 managerToPeer1SessionId,
                                                 *managerBus.GetInterface(interfaceName)->GetMember("Chirp"),
                                                 &arg, 1, 0, 0));

        //Wait for a maximum of 2 sec for the Chirp Signal.
        for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
            if (chirpSignalReceiver.signalReceivedFlag) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }
        EXPECT_FALSE(chirpSignalReceiver.signalReceivedFlag) << "managerBus failed to receive the Signal from Peer1";
        EXPECT_EQ(ER_OK, peer1Bus.UnregisterSignalHandler(&chirpSignalReceiver,
                                                          static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                                          peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));
    }
    {
        // Peer1 can Send Signal manager will not get signal it is blocked
        // by default policy.
        ChirpSignalReceiver chirpSignalReceiver;
        EXPECT_EQ(ER_OK, managerBus.RegisterSignalHandler(&chirpSignalReceiver,
                                                          static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                                          peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

        MsgArg arg;
        arg.Set("s", "Chirp this String out in the signal.");
        EXPECT_EQ(ER_OK, peer1BusObject.Signal(managerBus.GetUniqueName().c_str(),
                                               managerToPeer1SessionId,
                                               *peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                               &arg, 1, 0, 0));

        //Wait for a maximum of 2 sec for the Chirp Signal.
        for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
            if (chirpSignalReceiver.signalReceivedFlag) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }
        EXPECT_FALSE(chirpSignalReceiver.signalReceivedFlag) << "managerBus recieved a signal it when permissioins should have stopped signal.";
        EXPECT_EQ(ER_OK, managerBus.UnregisterSignalHandler(&chirpSignalReceiver,
                                                            static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                                            peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));
    }
    {
        SecurityApplicationProxy sapManagertoPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
        EXPECT_EQ(ER_PERMISSION_DENIED, sapManagertoPeer1.Reset());
    }
    /* clean up */
    managerBus.UnregisterBusObject(managerBusObject);
    peer1Bus.UnregisterBusObject(peer1BusObject);
}

/*
 * Purpose:
 * Only Trusted peers are allowed to interact with the application under default
 * policy.
 *
 * app bus  implements the following message types: method call, signal,
 * property 1, property 2.
 * ASG bus implements the following message types: method call, signal,
 * property 1, property 2.
 *
 * app. bus is claimed by the ASGA.
 * ASG bus has a MC signed by ASGA.
 * app. bus has default policy.
 * ASG bus has a policy that allows everything.
 *
 * ASG bus and app. bus have enabled ECDHE_NULL auth. mechanism.
 * Both peers have a default manifest that allows everything.
 *
 * 1. App. bus makes a method call, get property call, set property call, getall
 *    properties call on the ASG bus.
 * 2. App. bus sends a signal to to the ASG bus.
 * 3. ASG bus makes a method call, get property call, set property call, getall
 *    properties call on the app. bus.
 * 4. ASG bus sends a signal to the app. bus.
 * 5. ASG bus calls Reset on the app. bus.
 *
 * Verification:
 * The messages cannot be sent or received successfully by the app. bus.
 *
 * In this test managerBus == ASGA
 *              peer1Bus == ASA bus
 *              peer2Bus == app. bus
 */
TEST_F(SecurityDefaultPolicyTest, DefaultPolicy_ECDHE_NULL_everything_fails)
{
    InstallMemberShipOnManager();
    InstallMemberShipOnPeer1();

    DefaultRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject, true));
    DefaultRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject, true));

    /* install all permissive permission policy for Peer1*/
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    GeneratePermissivePolicy(peer1Policy, 1);

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    {
        PermissionPolicy defaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(defaultPolicy));
        EXPECT_EQ(ER_OK, UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer1Policy));
    }
    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));

    // Stitch the auth mechanism to ECDHE_NULL
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer1AuthListener));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", peer2AuthListener));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    //1 . App. bus makes a method call, get property call, set property call,
    //    getall properties call on the ASG bus.
    // verify: The messages cannot be sent or received successfully by the app. bus.
    {
        ProxyBusObject proxy(peer2Bus, peer1Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

        // Verify Method call
        MsgArg arg("s", "String that should be Echoed back.");
        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));

        // Verify Set/Get Property and GetAll Properties
        MsgArg prop1Arg;
        EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
        EXPECT_EQ(42, peer2BusObject.ReadProp1());

        MsgArg prop1ArgOut;
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));

        MsgArg props;
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetAllProperties(interfaceName, props)) << "Peer2 failed GetAllProperties call";
        EXPECT_EQ((size_t)0, props.v_array.GetNumElements());
    }
    //2. App. bus sends a signal to to the ASG bus.
    // verify: The signal cannot be received successfully by the ASG bus.
    {
        // Peer2 can Send Signal
        ChirpSignalReceiver chirpSignalReceiver;

        MsgArg arg("s", "Chirp this String out in the signal.");
        EXPECT_EQ(ER_OK, peer1Bus.RegisterSignalHandler(&chirpSignalReceiver,
                                                        static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                                        peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                                        NULL));

        QStatus status = peer2BusObject.Signal(peer1Bus.GetUniqueName().c_str(),
                                               peer1ToPeer2SessionId,
                                               *peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                               &arg, 1);
        EXPECT_EQ(ER_PERMISSION_DENIED, status);

        // If we get ER_OK back from the signal we want to know if the signal
        // was actually sent or is it just a failure to properly report the
        // proper status.
        if (ER_OK == status) {
            //Wait for a maximum of 2 sec for the Chirp Signal.
            for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
                if (chirpSignalReceiver.signalReceivedFlag) {
                    break;
                }
                qcc::Sleep(WAIT_MSECS);
            }
            EXPECT_FALSE(chirpSignalReceiver.signalReceivedFlag) << "Peer1 failed to receive the Signal from Peer2";
        }
        peer1Bus.UnregisterSignalHandler(&chirpSignalReceiver,
                                         static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                         peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                         NULL);
    }

    //3. ASG bus makes a method call, get property call, set property call, getall
    //   properties call on the app. bus.
    // verify: The messages cannot be sent or received successfully by the app. bus.
    {
        ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

        // Verify Method call
        MsgArg arg("s", "String that should be Echoed back.");
        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
        EXPECT_STREQ("org.alljoyn.Bus.Security.Error.PermissionDenied", replyMsg->GetErrorName());

        // Verify Set/Get Property and GetAll Properties
        MsgArg prop1Arg;
        EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
        EXPECT_EQ(42, peer2BusObject.ReadProp1());

        MsgArg prop1ArgOut;
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));

        MsgArg props;
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetAllProperties(interfaceName, props)) << "Peer2 failed GetAllProperties call";;
        EXPECT_EQ((size_t)0, props.v_array.GetNumElements());
    }
    // 4. ASG bus sends a signal to the app. bus.
    // verify: The signal cannot be received successfully by the app. bus.
    {
        // Peer1 can Send Signal
        ChirpSignalReceiver chirpSignalReceiver;
        EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver,
                                                        static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                                        peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                                        NULL));

        MsgArg arg("s", "Chirp this String out in the signal.");
        // Signals are send and forget.  They will always return ER_OK.
        EXPECT_EQ(ER_OK, peer1BusObject.Signal(peer2Bus.GetUniqueName().c_str(),
                                               peer1ToPeer2SessionId,
                                               *peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                               &arg, 1, 0, 0));

        //Wait for a maximum of 2 sec for the Chirp Signal.
        for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
            if (chirpSignalReceiver.signalReceivedFlag) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }
        EXPECT_FALSE(chirpSignalReceiver.signalReceivedFlag) << "Peer2 failed to receive the Signal from Peer1";
        peer2Bus.UnregisterSignalHandler(&chirpSignalReceiver,
                                         static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                         peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                         NULL);
    }
    // 5. ASG bus calls Reset on the app. bus.
    // verify: The Reset cannot be sent or received successfully on the app. bus.
    {
        SecurityApplicationProxy sapPeer1toPeer2(peer1Bus, peer2Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId);
        EXPECT_EQ(ER_PERMISSION_DENIED, sapPeer1toPeer2.Reset());
    }

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose:
 * On the app's default policy, a non ASG member can only receive messages sent
 * by the app. bus. The non ASG member cannot send messages to the app. bus.
 *
 * app. bus  implements the following message types: method call, signal,
 * property 1, property 2.
 * Peer A implements the following message types: method call, signal,
 * property 1, property 2.
 *
 * app. bus is claimed by the ASGA.
 * Peer A does not belong to ASG i.e it does not have a MC from ASG.
 * Peer A has a policy that enables method calls, signals, and properties.
 * app. bus has default policy.
 *
 * Peer A bus and app. bus have enabled ECDHE_ECDSA auth. mechanism.
 *
 * 1. App. bus makes a method call, get property call, set property call,
 *    getall properties call on Peer A.
 * 2. App. bus sends a signal to to Peer A.
 * 3. Peer A makes a method call, get property call, set property call, getall
 *    properties call on the app. bus.
 * 4. Peer A sends a signal to the app. bus.
 * 5. Peer A calls Reset on the app. bus
 *
 * Verification:
 * 1. Method call, get property, set property, getall properties are successful.
 * 2. The signal received by the ASG bus.
 * 3. Method call, get property, set property, getall properties are not
 *    received by the app. bus.
 * 4. The signal is not received by the app. bus.
 * 5. Reset method call should fail.
 *
 * In this test managerBus == ASGA
 *              peer1Bus == Peer A
 *              peer2Bus == app. bus
 */
TEST_F(SecurityDefaultPolicyTest, DefaultPolicy_MemberShipCertificate_not_installed)
{
    InstallMemberShipOnManager();

    /*
     * Peer1 is not expected to be a member of the security manager security
     * group. We need it to be a member of the security group to install the
     * permission policy that is expected to be installed so we install the
     * membership on peer1 then we Remove the membership.
     */
    DefaultRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject, true));
    DefaultRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject, true));

    /* install all permissive permission policy for Peer1*/
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    GeneratePermissivePolicy(peer1Policy, 1);

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    {
        PermissionPolicy defaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(defaultPolicy));
        EXPECT_EQ(ER_OK, UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer1Policy));
    }
    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    /* After having a new policy installed, the target bus
       clears out all of its peer's secret and session keys, so the
       next call will get security violation.  So just make the call and ignore
       the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    // 1. App. bus (Peer2) makes a method call, get property call, set property call,
    //   getall properties call on Peer A (Peer1).
    // verify: Method call, get property, set property, getall properties are successful.
    {
        ProxyBusObject proxy(peer2Bus, peer1Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

        // Verify Method call
        MsgArg arg("s", "String that should be Echoed back.");
        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg))
            << "Peer2 failed make MethodCall call " << replyMsg->GetErrorDescription().c_str();
        char* echoReply;
        replyMsg->GetArg(0)->Get("s", &echoReply);
        EXPECT_STREQ("String that should be Echoed back.", echoReply);

        // Verify Set/Get Property and GetAll Properties
        MsgArg prop1Arg;
        EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
        EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg)) << "Peer2 failed SetProperty call";
        EXPECT_EQ(513, peer1BusObject.ReadProp1());

        MsgArg prop1ArgOut;
        EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg)) << "Peer2 failed GetProperty call";;
        int32_t prop1;
        ASSERT_EQ(ER_OK, prop1Arg.Get("i", &prop1));
        EXPECT_EQ(513, prop1);

        MsgArg props;
        EXPECT_EQ(ER_OK, proxy.GetAllProperties(interfaceName, props)) << "Peer2 failed GetAllProperties call";;
        MsgArg* propArg;
        int32_t prop2;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop1", &propArg)) << props.ToString().c_str();
        ASSERT_EQ(ER_OK, propArg->Get("i", &prop1)) << propArg->ToString().c_str();
        EXPECT_EQ(513, prop1);

        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop2", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop2)) << propArg->ToString().c_str();
        EXPECT_EQ(17, prop2);
    }
    //2. App. bus (Peer2) sends a signal to to Peer A.
    // verify: The signal received by the ASG bus. (Peer1)
    {
        // Peer2 can Send Signal
        ChirpSignalReceiver chirpSignalReceiver;

        MsgArg arg("s", "Chirp this String out in the signal.");
        EXPECT_EQ(ER_OK, peer1Bus.RegisterSignalHandler(&chirpSignalReceiver,
                                                        static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                                        peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                                        NULL));

        // Signals are send and forget.  They will always return ER_OK.
        EXPECT_EQ(ER_OK, peer2BusObject.Signal(peer1Bus.GetUniqueName().c_str(),
                                               peer1ToPeer2SessionId,
                                               *peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                               &arg, 1, 0, 0));

        //Wait for a maximum of 2 sec for the Chirp Signal.
        for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
            if (chirpSignalReceiver.signalReceivedFlag) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }
        EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag) << "Peer1 failed to receive the Signal from Peer2";
        peer1Bus.UnregisterSignalHandler(&chirpSignalReceiver,
                                         static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                         peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                         NULL);
    }

    //3. Peer A (Peer1) makes a method call, get property call, set property call, getall
    //  properties call on the app. bus. (Peer2)
    // verify: Method call, get property, set property, getall properties are not
    //         received by the app. bus. (Peer2)
    {
        ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

        // Verify Method call
        MsgArg arg("s", "String that should be Echoed back.");
        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
        EXPECT_STREQ("org.alljoyn.Bus.Security.Error.PermissionDenied", replyMsg->GetErrorName());

        // Verify Set/Get Property and GetAll Properties
        MsgArg prop1Arg;
        EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
        EXPECT_EQ(42, peer2BusObject.ReadProp1());

        MsgArg prop1ArgOut;
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));

        MsgArg props;
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetAllProperties(interfaceName, props));
        EXPECT_EQ((size_t)0, props.v_array.GetNumElements());
    }
    // 4. Peer A (Peer1) sends a signal to the app. bus (Peer2).
    // verify: The signal is not received by the app. bus (Peer2).
    {
        // Peer1 can Send Signal
        ChirpSignalReceiver chirpSignalReceiver;
        EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver,
                                                        static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                                        peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                                        NULL));

        MsgArg arg("s", "Chirp this String out in the signal.");
        // Signals are send and forget.  They will always return ER_OK.
        EXPECT_EQ(ER_OK, peer1BusObject.Signal(peer2Bus.GetUniqueName().c_str(),
                                               peer1ToPeer2SessionId,
                                               *peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                               &arg, 1, 0, 0));

        //Wait for a maximum of 2 sec for the Chirp Signal.
        for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
            if (chirpSignalReceiver.signalReceivedFlag) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }
        EXPECT_FALSE(chirpSignalReceiver.signalReceivedFlag) << "Peer2 failed to receive the Signal from Peer1";
        peer2Bus.UnregisterSignalHandler(&chirpSignalReceiver,
                                         static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                         peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                         NULL);
    }
    // 5. Peer A (Peer1) calls Reset on the app. bus (Peer2)
    // verify: Reset method call should fail.
    {
        SecurityApplicationProxy sapPeer1toPeer2(peer1Bus, peer2Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId);
        EXPECT_EQ(ER_PERMISSION_DENIED, sapPeer1toPeer2.Reset());
    }

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose:
 * Any application can send and receive messages unsecurely.
 *
 * app. bus  implements the following message types: method call, signal,
 * property 1, property 2.
 * Peer A implements the following message types: method call, signal,
 * property 1, property 2.
 *
 * app. bus is claimed by the ASGA.
 * Peer A does not belong to ASG i.e it does not have a MC from ASG.
 * app. bus has default policy.
 *
 * Peer A bus and app. bus have enabled ECDHE_ECDSA auth. mechanism.
 *
 * 1. App. bus makes an unsecure method call, get property call, set property call,
 *    getall properties call on Peer A.
 * 2. App. bus sends an unsecure signal to to Peer A.
 * 3. Peer A makes an unsecure method call, get property call, set property call, getall
 *    properties call on the app. bus.
 * 4. Peer A sends an unsecure signal to the app. bus.
 *
 * Verification:
 * 1. Method call, get property, set property, getall properties are successful.
 * 2. The signal received by the ASG bus.
 * 3. Method call, get property, set property, getall properties are successful.
 * 4. The signal is received by the app. bus.
 *
 * In this test managerBus == ASGA
 *              peer1Bus == Peer A
 *              peer2Bus == app. bus
 */
TEST_F(SecurityDefaultPolicyTest, DefaultPolicy_unsecure_method_signal_properties_succeed)
{
    InstallMemberShipOnManager();

    // Both Peer1 and Peer2 have unsecure BusObjects that should succeed even
    // when using Security2.0
    DefaultRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject, false));
    DefaultRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject, false));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    // 1. App. bus (Peer2) makes a method call, get property call, set property call,
    //   getall properties call on Peer A (Peer1).
    // verify:  Method call, get property, set property, getall properties are successful.
    {
        ProxyBusObject proxy(peer2Bus, peer1Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, false);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

        // Verify Method call
        MsgArg arg("s", "String that should be Echoed back.");
        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg))
            << "Peer2 failed make MethodCall call " << replyMsg->GetErrorDescription().c_str();
        char* echoReply;
        replyMsg->GetArg(0)->Get("s", &echoReply);
        EXPECT_STREQ("String that should be Echoed back.", echoReply);

        // Verify Set/Get Property and GetAll Properties
        MsgArg prop1Arg;
        EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
        EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg)) << "Peer2 failed SetProperty call";
        EXPECT_EQ(513, peer1BusObject.ReadProp1());

        MsgArg prop1ArgOut;
        EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg)) << "Peer2 failed GetProperty call";;
        int32_t prop1;
        ASSERT_EQ(ER_OK, prop1Arg.Get("i", &prop1));
        EXPECT_EQ(513, prop1);

        MsgArg props;
        EXPECT_EQ(ER_OK, proxy.GetAllProperties(interfaceName, props)) << "Peer2 failed GetAllProperties call";;
        MsgArg* propArg;
        int32_t prop2;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop1", &propArg)) << props.ToString().c_str();
        ASSERT_EQ(ER_OK, propArg->Get("i", &prop1)) << propArg->ToString().c_str();
        EXPECT_EQ(513, prop1);

        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop2", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop2)) << propArg->ToString().c_str();
        EXPECT_EQ(17, prop2);
    }
    //2. App. bus (Peer2) sends a signal to to Peer A.
    // verify: The signal received by the ASG bus. (Peer1)
    {
        // Peer2 can Send Signal
        ChirpSignalReceiver chirpSignalReceiver;

        MsgArg arg("s", "Chirp this String out in the signal.");
        EXPECT_EQ(ER_OK, peer1Bus.RegisterSignalHandler(&chirpSignalReceiver,
                                                        static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                                        peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                                        NULL));

        // Signals are send and forget.  They will always return ER_OK.
        EXPECT_EQ(ER_OK, peer2BusObject.Signal(peer1Bus.GetUniqueName().c_str(),
                                               peer1ToPeer2SessionId,
                                               *peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                               &arg, 1, 0, 0));

        //Wait for a maximum of 2 sec for the Chirp Signal.
        for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
            if (chirpSignalReceiver.signalReceivedFlag) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }
        EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag) << "Peer1 failed to receive the Signal from Peer2";
        peer1Bus.UnregisterSignalHandler(&chirpSignalReceiver,
                                         static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                         peer2Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                         NULL);
    }

    // 3. Peer A (Peer1) makes an unsecure method call, get property call, set property call, getall
    //    properties call on the app. bus (Peer2).
    // verify:  Method call, get property, set property, getall properties are successful.
    {
        ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, false);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

        // Verify Method call
        MsgArg arg("s", "String that should be Echoed back.");
        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg))
            << "Peer2 failed make MethodCall call " << replyMsg->GetErrorDescription().c_str();
        char* echoReply;
        replyMsg->GetArg(0)->Get("s", &echoReply);
        EXPECT_STREQ("String that should be Echoed back.", echoReply);

        // Verify Set/Get Property and GetAll Properties
        MsgArg prop1Arg;
        EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
        EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg)) << "Peer2 failed SetProperty call";
        EXPECT_EQ(513, peer1BusObject.ReadProp1());

        MsgArg prop1ArgOut;
        EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg)) << "Peer2 failed GetProperty call";;
        int32_t prop1;
        ASSERT_EQ(ER_OK, prop1Arg.Get("i", &prop1));
        EXPECT_EQ(513, prop1);

        MsgArg props;
        EXPECT_EQ(ER_OK, proxy.GetAllProperties(interfaceName, props)) << "Peer2 failed GetAllProperties call";;
        MsgArg* propArg;
        int32_t prop2;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop1", &propArg)) << props.ToString().c_str();
        ASSERT_EQ(ER_OK, propArg->Get("i", &prop1)) << propArg->ToString().c_str();
        EXPECT_EQ(513, prop1);

        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop2", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop2)) << propArg->ToString().c_str();
        EXPECT_EQ(17, prop2);
    }
    // 4. Peer A (Peer1 )sends an unsecure signal to the app. bus.
    // verify: The signal is received by the app. bus. (Peer2)
    {
        // Peer1 can Send Signal
        ChirpSignalReceiver chirpSignalReceiver;

        MsgArg arg("s", "Chirp this String out in the signal.");
        EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver,
                                                        static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                                        peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                                        NULL));

        // Signals are send and forget.  They will always return ER_OK.
        EXPECT_EQ(ER_OK, peer1BusObject.Signal(peer2Bus.GetUniqueName().c_str(),
                                               peer1ToPeer2SessionId,
                                               *peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                               &arg, 1, 0, 0));

        //Wait for a maximum of 2 sec for the Chirp Signal.
        for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
            if (chirpSignalReceiver.signalReceivedFlag) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }
        EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag) << "Peer1 failed to receive the Signal from Peer2";
        peer1Bus.UnregisterSignalHandler(&chirpSignalReceiver,
                                         static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                         peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"),
                                         NULL);
    }

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}
/*
 * Purpose:
 * After Claiming, application bus can self install membership certificates on itself.
 *
 * Setup:
 * app. bus is claimed by the ASGA.
 * app. bus has default policy.
 *
 * app. bus calls InstallMembership on itself.
 * ASGA bus calls get property ("MembershipSummaries")
 *
 * Verification:
 * Verify that InstallMembership is successful.
 * Verify that when ASGA bus calls get property ("MembershipSummaries"), it
 * returns the same membership certificate details as the one installed above.
 *      ASGA =     managerBus
 *      app. bus = Peer1
 */
TEST_F(SecurityDefaultPolicyTest, DefaultPolicy_self_install_membership_certificates)
{
    InstallMemberShipOnManager();

    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    String membershipSerial = "1";
    qcc::GUID128 peer1Guid;

    qcc::MembershipCertificate peer1MembershipCertificate[1];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                    managerBus,
                                                                    peer1Bus.GetUniqueName(),
                                                                    peer1Key.GetPublicKey(),
                                                                    peer1Guid,
                                                                    false,
                                                                    3600,
                                                                    peer1MembershipCertificate[0]
                                                                    ));
    SecurityApplicationProxy sapPeer1WithSelf(peer1Bus, peer1Bus.GetUniqueName().c_str());

    // app. bus calls InstallMembership on itself.
    // verify: Verify that InstallMembership is successful.
    EXPECT_EQ(ER_OK, sapPeer1WithSelf.InstallMembership(peer1MembershipCertificate, 1));

    SecurityApplicationProxy sapManagerWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);

    /* retrieve the membership summaries */
    MsgArg arg;
    EXPECT_EQ(ER_OK, sapManagerWithPeer1.GetMembershipSummaries(arg)) << "GetMembershipSummaries failed.";
    size_t count = arg.v_array.GetNumElements();

    ASSERT_GT(count, (size_t) 0) << "No membership cert found.";

    KeyInfoNISTP256* keyInfos = new KeyInfoNISTP256[count];
    String* serials = new String[count];
    EXPECT_EQ(ER_OK, SecurityApplicationProxy::MsgArgToCertificateIds(arg, serials, keyInfos, count)) << " MsgArgToCertificateIds failed.";

    KeyInfoNISTP256 managerKey;
    PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

    EXPECT_EQ(*managerKey.GetPublicKey(), *keyInfos[0].GetPublicKey());
    EXPECT_EQ(membershipSerial, serials[0]);

    delete [] keyInfos;
    delete [] serials;
}

/*
 * Purpose:
 * The default policies are overridden when a new policy is installed.
 *
 * Setup:
 * app. bus is claimed by the ASGA.
 * ASG bus has a MC signed by ASGA.
 * app. bus has default policy.
 *
 * ASG bus installs the following policy on the app. bus:
 * ACL: Peer type: ANY_TRUSTED; Rule: Allow method call "Ping"
 *
 * ASG bus and app. bus have enabled ECDHE_ECDSA auth. mechanism.
 * Both peers have a default manifest that allows everything.
 *
 * 1. ASG bus calls Reset on the app. bus
 *
 * Verification:
 * Verify that Reset method call fails. (There is no rule that explicitly allows Reset.)
 *      ASGA =     managerBus
 *      app. bus = Peer1
 */
TEST_F(SecurityDefaultPolicyTest, default_policy_overridden_when_a_new_policy_installed)
{
    InstallMemberShipOnManager();

    PermissionPolicy policy;
    policy.SetVersion(1);
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
                members[0].Set("Ping",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        policy.SetAcls(1, acls);
    }
    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);

    PermissionPolicy defaultPolicy;
    EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(defaultPolicy));
    EXPECT_EQ(ER_OK, UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, policy));

    EXPECT_NE(policy, defaultPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));

    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
    PermissionPolicy peer2Policy;
    GeneratePermissivePolicy(peer2Policy, 1);
    {
        PermissionPolicy defaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(defaultPolicy));
        EXPECT_EQ(ER_OK, UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer2Policy));
    }
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer2Bus.JoinSession(peer1Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    SecurityApplicationProxy sapPeer2withPeer1(peer2Bus, peer1Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId);
    EXPECT_EQ(ER_PERMISSION_DENIED, sapPeer2withPeer1.Reset());
}

/*
 * Purpose:
 * Application manifest can deny secure management operations.
 *
 * Setup:
 * app. bus (Peer2) is claimed by the ASGA.
 * ASG bus (Peer1) has a MC signed by ASGA.
 * app. bus (Peer2) has default policy.
 *
 * ASG bus(Peer1) and app. bus (Peer2) have enabled ECDHE_ECDSA auth. mechanism.
 *
 * app. bus (Peer2) manifest has the following rules:
 * Allow everything
 * Deny 'Reset' method call
 *
 * ASG bus (Peer1) manifest has the following rules:
 * Allow everything
 * Deny 'UpdateIdentity' method call
 *
 * 1. ASG bus (Peer1) calls Reset on the app. bus
 * 2. ASG bus (Peer1) calls UpdateIdentity on the app. bus.
 *
 * Verification:
 * 1. Verify that Reset call cannot be sent by the ASG bus (Peer1).
 * 2. Verify that UpdateIdentity call cannot be received by the app. bus (Peer2).
 *      ASGA =     managerBus
 *      ASG bus = Peer1
 *      app. Bus = Peer2
 */
TEST_F(SecurityDefaultPolicyTest, manifest_can_deny_secure_management_operations)
{
    InstallMemberShipOnManager();
    InstallMemberShipOnPeer1();

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
    PermissionPolicy peer1Policy;
    GeneratePermissivePolicy(peer1Policy, 1);
    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy, true, true);
    }
    sapWithPeer1.UpdatePolicy(peer1Policy);

    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);

    const size_t manifestSize = 2;
    const size_t certChainSize = 1;
    /*************Update Peer1 Manifest *************/
    //peer1 key
    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    peer1Manifest[0].SetObjPath("*");
    peer1Manifest[0].SetInterfaceName("*");
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                       PermissionPolicy::Rule::Member::ACTION_MODIFY |
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[0].SetMembers(1, members);
    }
    peer1Manifest[1].SetInterfaceName(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName);
    peer1Manifest[1].SetObjPath(org::alljoyn::Bus::Security::ObjectPath);
    {
        PermissionPolicy::Rule::Member members[1];
        // This will block the UpdateIdentity method from being called.
        members[0].Set("UpdateIdentity",
                       PermissionPolicy::Rule::Member::METHOD_CALL,
                       0);
        peer1Manifest[0].SetMembers(1, members);
    }

    uint8_t peer1Digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(managerBus,
                                                               peer1Manifest, manifestSize,
                                                               peer1Digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    //Create peer1 identityCert
    IdentityCertificate identityCertChainPeer1[certChainSize];

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                  "1",
                                                                  managerGuid.ToString(),
                                                                  peer1Key.GetPublicKey(),
                                                                  "Peer1Alias",
                                                                  3600,
                                                                  identityCertChainPeer1[0],
                                                                  peer1Digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChainPeer1, certChainSize, peer1Manifest, manifestSize));

    /*************Update peer2 Manifest *************/
    //peer2 key
    KeyInfoNISTP256 peer2Key;
    PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));

    // Peer1 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    peer2Manifest[0].SetObjPath("*");
    peer2Manifest[0].SetInterfaceName("*");
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                       PermissionPolicy::Rule::Member::ACTION_MODIFY |
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[0].SetMembers(1, members);
    }
    peer2Manifest[1].SetInterfaceName(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName);
    peer2Manifest[1].SetObjPath(org::alljoyn::Bus::Security::ObjectPath);
    {
        PermissionPolicy::Rule::Member members[1];
        // This will block the Reset method from being called.
        members[0].Set("Reset",
                       PermissionPolicy::Rule::Member::METHOD_CALL,
                       0);
        peer1Manifest[0].SetMembers(1, members);
    }

    uint8_t peer2Digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(managerBus,
                                                               peer2Manifest, manifestSize,
                                                               peer2Digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    //Create peer2 identityCert
    IdentityCertificate identityCertChainPeer2[certChainSize];

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                  "1",
                                                                  managerGuid.ToString(),
                                                                  peer2Key.GetPublicKey(),
                                                                  "Peer2Alias",
                                                                  3600,
                                                                  identityCertChainPeer2[0],
                                                                  peer2Digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    EXPECT_EQ(ER_OK, sapWithPeer2.UpdateIdentity(identityCertChainPeer2, certChainSize, peer2Manifest, manifestSize));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer1SessionPort, NULL, peer1ToPeer2SessionId, opts));
    SecurityApplicationProxy sapPeer1withPeer2(peer1Bus, peer2Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId);
    EXPECT_EQ(ER_PERMISSION_DENIED, sapPeer1withPeer2.Reset());

    EXPECT_EQ(ER_PERMISSION_DENIED, sapPeer1withPeer2.UpdateIdentity(identityCertChainPeer1, certChainSize, peer1Manifest, manifestSize));
}

/*
 * Purpose:
 * Before Claiming, an application should not be able to self install membership
 * certificates on itself.
 *
 * Setup:
 * app. bus is not claimed.
 * app. bus calls InstallMembership on itself.
 *
 * Verification:
 * Verify that InstallMembership fails as the app. bus is not yet claimed.
 */
TEST(SecurityDefaultPolicy2Test, DefaultPolicy_self_install_membership_certificates_fails_before_claim)
{
    BusAttachment unclaimedBus("SecurityTestUnclamedBus");
    EXPECT_EQ(ER_OK, unclaimedBus.Start());
    EXPECT_EQ(ER_OK, unclaimedBus.Connect());

    // Register in memory keystore listeners
    InMemoryKeyStoreListener keyStoreListener;
    DefaultECDHEAuthListener authListener;
    EXPECT_EQ(ER_OK, unclaimedBus.RegisterKeyStoreListener(keyStoreListener));
    EXPECT_EQ(ER_OK, unclaimedBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &authListener));

    KeyInfoNISTP256 key;
    PermissionConfigurator& pc = unclaimedBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pc.GetSigningPublicKey(key));

    String serial = "1";
    qcc::GUID128 guid;

    qcc::MembershipCertificate membershipCertificate[1];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(serial,
                                                                    unclaimedBus,
                                                                    unclaimedBus.GetUniqueName(),
                                                                    key.GetPublicKey(),
                                                                    guid,
                                                                    false,
                                                                    3600,
                                                                    membershipCertificate[0]
                                                                    ));
    SecurityApplicationProxy sapWithSelf(unclaimedBus, unclaimedBus.GetUniqueName().c_str());

    // app. bus is not claimed.
    // app. bus calls InstallMembership on itself.
    // verify: Verify that InstallMembership fails as the app. bus is not yet claimed.
    EXPECT_EQ(ER_PERMISSION_DENIED, sapWithSelf.InstallMembership(membershipCertificate, 1));
}
