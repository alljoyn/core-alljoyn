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
#define TEN_MINS 600

static String PrintActionMask(uint8_t actionMask) {
    String result;
    bool addPipeChar = false;
    if (actionMask & PermissionPolicy::Rule::Member::ACTION_PROVIDE) {
        result += "PROVIDE";
        addPipeChar = true;
    }
    if (actionMask & PermissionPolicy::Rule::Member::ACTION_MODIFY) {
        if (addPipeChar) {
            result += " | MODIFY";
        } else {
            result += "MODIFY";
        }
        addPipeChar = true;
    }
    if (actionMask & PermissionPolicy::Rule::Member::ACTION_OBSERVE) {
        if (addPipeChar) {
            result += " | OBSERVE";
        } else {
            result += "OBSERVE";
        }
        addPipeChar = true;
    }
    // Since no other action is found its a DENY rule
    if (addPipeChar == false) {
        result += "DENY";
    }
    return result;
}

class PolicyRules_ApplicationStateListener : public ApplicationStateListener {
  public:
    PolicyRules_ApplicationStateListener() : stateMap() { }

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

class PolicyRulesTestSessionPortListener : public SessionPortListener {
  public:
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
};

class PolicyRulesTestBusObject : public BusObject {
  public:
    PolicyRulesTestBusObject(BusAttachment& bus, const char* path, const char* interfaceName, bool announce = true)
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
            { iface->GetMember("Echo"), static_cast<MessageReceiver::MethodHandler>(&PolicyRulesTestBusObject::Echo) }
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

class SecurityPolicyRulesTest : public testing::Test {
  public:
    SecurityPolicyRulesTest() :
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

        interface = "<node>"
                    "<interface name='" + String(interfaceName) + "'>"
                    "<annotation name='org.alljoyn.Bus.Secure' value='true'/>"
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

        EXPECT_EQ(ER_OK, sapWithManager.Claim(managerKey,
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
            if (appStateListener.isClaimed(peer2Bus.GetUniqueName())) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }

        ASSERT_EQ(PermissionConfigurator::ApplicationState::CLAIMED, appStateListener.stateMap[peer1Bus.GetUniqueName()]);

        //Change the managerBus so it only uses ECDHE_ECDSA
        EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", managerAuthListener));

        PermissionPolicy defaultPolicy;
        EXPECT_EQ(ER_OK, sapWithManager.GetDefaultPolicy(defaultPolicy));

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
        EXPECT_EQ(ER_OK, sapWithManager.InstallMembership(managerMembershipCertificate, 1));

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

    QStatus UpdatePolicyWithValuesFromDefaultPolicy(const PermissionPolicy& defaultPolicy,
                                                    PermissionPolicy& policy,
                                                    bool keepCAentry = true,
                                                    bool keepAdminGroupEntry = false,
                                                    bool keepInstallMembershipEntry = false);

    /*
     * The policy for all of the GetAllProperties tests only differs by what is
     * defined in the members.  This Will build the same policy with only the members
     * changed for all of the GetAllProperties tests.
     */
    void CreatePolicyWithMembersForGetAllProperties(PermissionPolicy& policy, PermissionPolicy::Rule::Member* members, size_t membersSize);
    void UpdatePeer1Manifest(PermissionPolicy::Rule* manifest, size_t manifestSize);
    void UpdatePeer2Manifest(PermissionPolicy::Rule* manifest, size_t manifestSize);

    BusAttachment managerBus;
    BusAttachment peer1Bus;
    BusAttachment peer2Bus;

    SessionPort managerSessionPort;
    SessionPort peer1SessionPort;
    SessionPort peer2SessionPort;

    PolicyRulesTestSessionPortListener managerSessionPortListener;
    PolicyRulesTestSessionPortListener peer1SessionPortListener;
    PolicyRulesTestSessionPortListener peer2SessionPortListener;

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

    PolicyRules_ApplicationStateListener appStateListener;

    //Random GUID used for the SecurityManager
    GUID128 managerGuid;
};

QStatus SecurityPolicyRulesTest::UpdatePolicyWithValuesFromDefaultPolicy(const PermissionPolicy& defaultPolicy,
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
 * The policy for all of the GetAllProperties tests only differs by what is
 * defined in the members.  This Will build the same policy with only the members
 * changed for all of the GetAllProperties tests.
 */
void SecurityPolicyRulesTest::CreatePolicyWithMembersForGetAllProperties(PermissionPolicy& policy, PermissionPolicy::Rule::Member* members, size_t membersSize) {
    policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[2];
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            rules[0].SetMembers(membersSize, members);
            //make sure peer1 can call UpdateIdentity to update the manifest
            rules[1].SetObjPath(org::alljoyn::Bus::Security::ObjectPath);
            rules[1].SetInterfaceName(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[1].SetMembers(1, members);
            }
            acls[0].SetRules(2, rules);
        }
        policy.SetAcls(1, acls);
    }
}

void SecurityPolicyRulesTest::UpdatePeer1Manifest(PermissionPolicy::Rule* manifest, size_t manifestSize) {
    const size_t certChainSize = 1;
    /*************Update Peer1 Manifest *************/
    //peer1 key
    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    uint8_t peer1Digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(managerBus,
                                                               manifest, manifestSize,
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

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChainPeer1, certChainSize, manifest, manifestSize));
}

void SecurityPolicyRulesTest::UpdatePeer2Manifest(PermissionPolicy::Rule* manifest, size_t manifestSize) {
    const size_t certChainSize = 1;
    /*************Update peer2 Manifest *************/
    //peer2 key
    KeyInfoNISTP256 peer2Key;
    PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));

    uint8_t peer2Digest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(managerBus,
                                                               manifest, manifestSize,
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

    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdateIdentity(identityCertChainPeer2, certChainSize, manifest, manifestSize));
}

class MethodRulesTestValue {
  public:
    MethodRulesTestValue(uint8_t mask1, uint8_t mask2, bool makeMethodCall, bool respondToMethodCall) :
        peer1ActionMask(mask1),
        peer2ActionMask(mask2),
        proxyObjAllowedToCallMethod(makeMethodCall),
        busObjAllowedToRespondToMethodCall(respondToMethodCall) { }

    friend::std::ostream& operator<<(::std::ostream& os, const MethodRulesTestValue& val);
    uint8_t peer1ActionMask;
    uint8_t peer2ActionMask;
    bool proxyObjAllowedToCallMethod;
    bool busObjAllowedToRespondToMethodCall;
};

::std::ostream& operator<<(::std::ostream& os, const MethodRulesTestValue& val) {
    os << "peer1Mask = " << PrintActionMask(val.peer1ActionMask).c_str() << "\n";
    os << "peer2Mask = " << PrintActionMask(val.peer2ActionMask).c_str() << "\n";
    if (val.proxyObjAllowedToCallMethod) {
        os << "ProxyBusObject is expected to call Method\n";
    } else {
        os << "ProxyBusObject is NOT expected to call Method\n";
    }
    if (val.busObjAllowedToRespondToMethodCall) {
        os << "BusObject is expected to respond to Method call\n";
    } else {
        os << "BusObject is NOT expected to respond to Method call\n";
    }
    return os;
}

class SecurityPolicyRulesMethodCalls : public SecurityPolicyRulesTest,
    public testing::WithParamInterface<MethodRulesTestValue> {
};

TEST_P(SecurityPolicyRulesMethodCalls, PolicyRules)
{

    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
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
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo", PermissionPolicy::Rule::Member::METHOD_CALL, GetParam().peer1ActionMask);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

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
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               GetParam().peer2ActionMask);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());
    qcc::String p2policyStr = "\n----Peer2 Policy-----\n" + peer2Policy.ToString();
    SCOPED_TRACE(p2policyStr.c_str());

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    QStatus methodCallStatus = proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg);

    if (GetParam().proxyObjAllowedToCallMethod && GetParam().busObjAllowedToRespondToMethodCall) {
        EXPECT_EQ(ER_OK, methodCallStatus);
        char* echoReply;
        replyMsg->GetArg(0)->Get("s", &echoReply);
        EXPECT_STREQ("String that should be Echoed back.", echoReply);
    } else if (GetParam().proxyObjAllowedToCallMethod && !GetParam().busObjAllowedToRespondToMethodCall) {
        EXPECT_EQ(ER_PERMISSION_DENIED, methodCallStatus);
        EXPECT_STREQ("org.alljoyn.Bus.Security.Error.PermissionDenied", replyMsg->GetErrorName());
    } else { //!GetParam().proxyObjAllowedToCallMethod
        EXPECT_EQ(ER_PERMISSION_DENIED, methodCallStatus);
        ASSERT_STREQ("org.alljoyn.Bus.ErStatus", replyMsg->GetErrorName());
        EXPECT_EQ(ER_PERMISSION_DENIED, (QStatus)replyMsg->GetArg(1)->v_uint16) << "\n" << replyMsg->GetArg(0)->ToString().c_str() << "\n" << replyMsg->GetArg(1)->ToString().c_str();
    }

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

INSTANTIATE_TEST_CASE_P(Method, SecurityPolicyRulesMethodCalls,
                        ::testing::Values(
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //0
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 true,   //Proxy object allowed to make method call
                                                 false), //bus object allowed to respond to method call
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //1
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 true,
                                                 true),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //2
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 true,
                                                 false),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //3
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 false,
                                                 false),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //4
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 false,
                                                 true),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //5
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 false,
                                                 false),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //6
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 false,
                                                 false),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //7
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 false,
                                                 true),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //8
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 false,
                                                 false)
                            ));

class SecurityPolicyRulesMethodCallsManifest : public SecurityPolicyRulesTest,
    public testing::WithParamInterface<MethodRulesTestValue> {
};

TEST_P(SecurityPolicyRulesMethodCallsManifest, PolicyRules)
{

    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
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

            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

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
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);


    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy, true, true);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy, true, true);
    }

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());
    qcc::String p2policyStr = "\n----Peer2 Policy-----\n" + peer2Policy.ToString();
    SCOPED_TRACE(p2policyStr.c_str());

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    const size_t manifestSize = 1;
    const size_t certChainSize = 1;
    /*************Update Peer1 Manifest *************/
    //peer1 key
    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Echo", PermissionPolicy::Rule::Member::METHOD_CALL, GetParam().peer1ActionMask);
        peer1Manifest[0].SetObjPath("*");
        peer1Manifest[0].SetInterfaceName(interfaceName);
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
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Echo", PermissionPolicy::Rule::Member::METHOD_CALL, GetParam().peer2ActionMask);
        peer2Manifest[0].SetObjPath("*");
        peer2Manifest[0].SetInterfaceName(interfaceName);
        peer2Manifest[0].SetMembers(1, members);
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
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    QStatus methodCallStatus = proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg);

    if (GetParam().proxyObjAllowedToCallMethod && GetParam().busObjAllowedToRespondToMethodCall) {
        EXPECT_EQ(ER_OK, methodCallStatus);
        char* echoReply;
        replyMsg->GetArg(0)->Get("s", &echoReply);
        EXPECT_STREQ("String that should be Echoed back.", echoReply);
    } else if (GetParam().proxyObjAllowedToCallMethod && !GetParam().busObjAllowedToRespondToMethodCall) {
        EXPECT_EQ(ER_PERMISSION_DENIED, methodCallStatus);
        EXPECT_STREQ("org.alljoyn.Bus.Security.Error.PermissionDenied", replyMsg->GetErrorName());
    } else { //!GetParam().proxyObjAllowedToCallMethod
        EXPECT_EQ(ER_PERMISSION_DENIED, methodCallStatus);
        ASSERT_STREQ("org.alljoyn.Bus.ErStatus", replyMsg->GetErrorName());
        EXPECT_EQ(ER_PERMISSION_DENIED, (QStatus)replyMsg->GetArg(1)->v_uint16) << "\n" << replyMsg->GetArg(0)->ToString().c_str() << "\n" << replyMsg->GetArg(1)->ToString().c_str();
    }

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

INSTANTIATE_TEST_CASE_P(Method, SecurityPolicyRulesMethodCallsManifest,
                        ::testing::Values(
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //0
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 true, //ProxyBusObject can make method call
                                                 false), //BusObject can respond to method call
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //1
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 false,
                                                 false),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //2
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 false,
                                                 false),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //3
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 true,
                                                 true),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //4
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 false,
                                                 true),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //5
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 false,
                                                 true),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //6
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 true,
                                                 false),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //7
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 false,
                                                 false),
                            MethodRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //8
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 false,
                                                 false)
                            ));

class GetPropertyRulesTestValue {
  public:
    GetPropertyRulesTestValue(uint8_t mask1, uint8_t mask2, bool makeGetPropertyCall, bool respondToGetPropertyCall) :
        peer1ActionMask(mask1),
        peer2ActionMask(mask2),
        proxyObjAllowedToCallGetProperty(makeGetPropertyCall),
        busObjAllowedToRespondToGetPropertyCall(respondToGetPropertyCall) { }
    friend ostream& operator<<(ostream& os, const GetPropertyRulesTestValue& val);

    uint8_t peer1ActionMask;
    uint8_t peer2ActionMask;
    bool proxyObjAllowedToCallGetProperty;
    bool busObjAllowedToRespondToGetPropertyCall;
};

::std::ostream& operator<<(::std::ostream& os, const GetPropertyRulesTestValue& val) {
    os << "peer1Mask = " << PrintActionMask(val.peer1ActionMask).c_str() << "\n";
    os << "peer2Mask = " << PrintActionMask(val.peer2ActionMask).c_str() << "\n";
    if (val.proxyObjAllowedToCallGetProperty) {
        os << "ProxyBusObject is expected to call GetProperty\n";
    } else {
        os << "ProxyBusObject is NOT expected to call GetProperty\n";
    }
    if (val.busObjAllowedToRespondToGetPropertyCall) {
        os << "BusObject is expected to respond to GetProperty call\n";
    } else {
        os << "BusObject is NOT expected to respond to GetProperty call\n";
    }
    return os;
}

class SecurityPolicyRulesGetProperty : public SecurityPolicyRulesTest,
    public testing::WithParamInterface<GetPropertyRulesTestValue> {
};

TEST_P(SecurityPolicyRulesGetProperty, PolicyRules)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
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
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Prop1",
                               PermissionPolicy::Rule::Member::PROPERTY,
                               GetParam().peer1ActionMask);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

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
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Prop1",
                               PermissionPolicy::Rule::Member::PROPERTY,
                               GetParam().peer2ActionMask);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());
    qcc::String p2policyStr = "\n----Peer2 Policy-----\n" + peer2Policy.ToString();
    SCOPED_TRACE(p2policyStr.c_str());

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    QStatus getPropertyStatus = proxy.GetProperty(interfaceName, "Prop1", prop1Arg);
    if (GetParam().proxyObjAllowedToCallGetProperty && GetParam().busObjAllowedToRespondToGetPropertyCall) {
        EXPECT_EQ(ER_OK, getPropertyStatus);
        //Verify we got Prop1 prop1Arg should be changed from 513 to 42 (note prop1 defaults to 42 by the constructor)
        int32_t prop1;
        prop1Arg.Get("i", &prop1);
        EXPECT_EQ(42, prop1);
    } else if (GetParam().proxyObjAllowedToCallGetProperty && !GetParam().busObjAllowedToRespondToGetPropertyCall) {
        EXPECT_EQ(ER_PERMISSION_DENIED, getPropertyStatus);
        //Currently no way to find out that the error string is org.alljoyn.Bus.Security.Error.PermissionDenied
    } else { //!GetParam().proxyObjAllowedToCallSetProperty
        // Maybe this should be ER_PERMISSION_DENIED like it is for the SetProperty call
        EXPECT_EQ(ER_PERMISSION_DENIED, getPropertyStatus);
    }

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

INSTANTIATE_TEST_CASE_P(GetProperty, SecurityPolicyRulesGetProperty,
                        ::testing::Values(
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //0
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      true,   //ProxyBusObj Allowed To Call GetProperty;
                                                      false), //BusObj Allowed To Respond To GetProperty Call
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //1
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      true,
                                                      false),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //2
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      true,
                                                      true),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //3
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      false,
                                                      false),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //4
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      false,
                                                      false),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //5
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      false,
                                                      true),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //6
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      false,
                                                      false),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //7
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      false,
                                                      false),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //8
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      false,
                                                      true)
                            ));

class SecurityPolicyRulesGetPropertyManifest : public SecurityPolicyRulesTest,
    public testing::WithParamInterface<GetPropertyRulesTestValue> {
};

TEST_P(SecurityPolicyRulesGetPropertyManifest, PolicyRules)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
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
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Prop1",
                               PermissionPolicy::Rule::Member::PROPERTY,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

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
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Prop1",
                               PermissionPolicy::Rule::Member::PROPERTY,
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy, true, true);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy, true, true);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    const size_t manifestSize = 1;
    const size_t certChainSize = 1;
    /*************Update Peer1 Manifest *************/
    //peer1 key
    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, GetParam().peer1ActionMask);
        peer1Manifest[0].SetObjPath("*");
        peer1Manifest[0].SetInterfaceName(interfaceName);
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
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, GetParam().peer2ActionMask);
        peer2Manifest[0].SetObjPath("*");
        peer2Manifest[0].SetInterfaceName(interfaceName);
        peer2Manifest[0].SetMembers(1, members);
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
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    QStatus getPropertyStatus = proxy.GetProperty(interfaceName, "Prop1", prop1Arg);
    if (GetParam().proxyObjAllowedToCallGetProperty && GetParam().busObjAllowedToRespondToGetPropertyCall) {
        EXPECT_EQ(ER_OK, getPropertyStatus);
        //Verify we got Prop1 prop1Arg should be changed from 513 to 42 (note prop1 defaults to 42 by the constructor)
        int32_t prop1;
        prop1Arg.Get("i", &prop1);
        EXPECT_EQ(42, prop1);
    } else if (GetParam().proxyObjAllowedToCallGetProperty && !GetParam().busObjAllowedToRespondToGetPropertyCall) {
        EXPECT_EQ(ER_PERMISSION_DENIED, getPropertyStatus);
        //Currently no way to find out that the error string is org.alljoyn.Bus.Security.Error.PermissionDenied
    } else { //!GetParam().proxyObjAllowedToCallSetProperty
        // Maybe this should be ER_PERMISSION_DENIED like it is for the SetProperty call
        EXPECT_EQ(ER_PERMISSION_DENIED, getPropertyStatus);
    }

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

INSTANTIATE_TEST_CASE_P(GetProperty, SecurityPolicyRulesGetPropertyManifest,
                        ::testing::Values(
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //0
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      true, //ProxyBusObj Allowed To Call GetProperty;
                                                      false), //BusObj Allowed To Respond To GetProperty Call
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //1
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      false,
                                                      false),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //2
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      false,
                                                      false),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //3
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      true,
                                                      false),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //4
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      false,
                                                      false),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //5
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      false,
                                                      false),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //6
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      true,
                                                      true),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //7
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      false,
                                                      true),
                            GetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //8
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      false,
                                                      true)
                            ));

class SetPropertyRulesTestValue {
  public:
    SetPropertyRulesTestValue(uint8_t mask1, uint8_t mask2, bool makeSetPropertyCall, bool respondToSetPropertyCall) :
        peer1ActionMask(mask1),
        peer2ActionMask(mask2),
        proxyObjAllowedToCallSetProperty(makeSetPropertyCall),
        busObjAllowedToRespondToSetPropertyCall(respondToSetPropertyCall) { }
    friend::std::ostream& operator<<(::std::ostream& os, const SetPropertyRulesTestValue& val);
    uint8_t peer1ActionMask;
    uint8_t peer2ActionMask;
    bool proxyObjAllowedToCallSetProperty;
    bool busObjAllowedToRespondToSetPropertyCall;
};

::std::ostream& operator<<(::std::ostream& os, const SetPropertyRulesTestValue& val) {
    os << "peer1Mask = " << PrintActionMask(val.peer1ActionMask).c_str() << "\n";
    os << "peer2Mask = " << PrintActionMask(val.peer2ActionMask).c_str() << "\n";
    if (val.proxyObjAllowedToCallSetProperty) {
        os << "ProxyBusObject is expected to call SetProperty\n";
    } else {
        os << "ProxyBusObject is NOT expected to call SetProperty\n";
    }
    if (val.busObjAllowedToRespondToSetPropertyCall) {
        os << "BusObject is expected to respond to SetProperty call\n";
    } else {
        os << "BusObject is NOT expected to respond to SetProperty call\n";
    }
    return os;
}

class SecurityPolicyRulesSetProperty : public SecurityPolicyRulesTest,
    public testing::WithParamInterface<SetPropertyRulesTestValue> {
};

TEST_P(SecurityPolicyRulesSetProperty, PolicyRules)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
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
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Prop1",
                               PermissionPolicy::Rule::Member::PROPERTY,
                               GetParam().peer1ActionMask);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

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
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Prop1",
                               PermissionPolicy::Rule::Member::PROPERTY,
                               GetParam().peer2ActionMask);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());
    qcc::String p2policyStr = "\n----Peer2 Policy-----\n" + peer2Policy.ToString();
    SCOPED_TRACE(p2policyStr.c_str());

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    QStatus setPropertyStatus = proxy.SetProperty(interfaceName, "Prop1", prop1Arg);
    if (GetParam().proxyObjAllowedToCallSetProperty && GetParam().busObjAllowedToRespondToSetPropertyCall) {
        EXPECT_EQ(ER_OK, setPropertyStatus);
        //Verify Prop1 is changed.
        EXPECT_EQ(513, peer2BusObject.ReadProp1());
    } else if (GetParam().proxyObjAllowedToCallSetProperty && !GetParam().busObjAllowedToRespondToSetPropertyCall) {
        EXPECT_EQ(ER_PERMISSION_DENIED, setPropertyStatus);
        //Verify Prop1 is unchanged (note prop1 defaults to 42 by the constructor)
        EXPECT_EQ(42, peer2BusObject.ReadProp1());
    } else { //!GetParam().proxyObjAllowedToCallSetProperty
        EXPECT_EQ(ER_PERMISSION_DENIED, setPropertyStatus);
        EXPECT_EQ(42, peer2BusObject.ReadProp1());
    }

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

INSTANTIATE_TEST_CASE_P(SetProperty, SecurityPolicyRulesSetProperty,
                        ::testing::Values(
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //0
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      true, //ProxyBusObj Allowed To Call SetProperty;
                                                      false), //BusObj Allowed To Respond To SetProperty Call
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //1
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      true,
                                                      true),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //2
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      true,
                                                      false),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //3
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      false,
                                                      false),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //4
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      false,
                                                      true),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //5
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      false,
                                                      false),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //6
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      false,
                                                      false),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //7
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      false,
                                                      true),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //8
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      false,
                                                      false)
                            ));

class SecurityPolicyRulesSetPropertyManifest : public SecurityPolicyRulesTest,
    public testing::WithParamInterface<SetPropertyRulesTestValue> {
};

TEST_P(SecurityPolicyRulesSetPropertyManifest, PolicyRules)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
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
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Prop1",
                               PermissionPolicy::Rule::Member::PROPERTY,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

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
            PermissionPolicy::Rule rules[2];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Prop1",
                               PermissionPolicy::Rule::Member::PROPERTY,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            //make sure peer2 can call UpdateIdentity to update the manifest
            rules[1].SetObjPath("*");
            rules[1].SetObjPath(org::alljoyn::Bus::Security::ObjectPath);
            rules[1].SetInterfaceName(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[1].SetMembers(1, members);
            }
            acls[0].SetRules(2, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy, true, true);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy, true, true);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    const size_t manifestSize = 1;
    const size_t certChainSize = 1;
    /*************Update Peer1 Manifest *************/
    //peer1 key
    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, GetParam().peer1ActionMask);
        peer1Manifest[0].SetObjPath("*");
        peer1Manifest[0].SetInterfaceName(interfaceName);
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
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, GetParam().peer2ActionMask);
        peer2Manifest[0].SetObjPath("*");
        peer2Manifest[0].SetInterfaceName(interfaceName);
        peer2Manifest[0].SetMembers(1, members);
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
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    QStatus setPropertyStatus = proxy.SetProperty(interfaceName, "Prop1", prop1Arg);
    if (GetParam().proxyObjAllowedToCallSetProperty && GetParam().busObjAllowedToRespondToSetPropertyCall) {
        EXPECT_EQ(ER_OK, setPropertyStatus);
        //Verify Prop1 is changed.
        EXPECT_EQ(513, peer2BusObject.ReadProp1());
    } else { //!GetParam().proxyObjAllowedToCallSetProperty
        EXPECT_TRUE(ER_PERMISSION_DENIED == setPropertyStatus || ER_BUS_REPLY_IS_ERROR_MESSAGE == setPropertyStatus);
        EXPECT_EQ(42, peer2BusObject.ReadProp1());
    }

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

INSTANTIATE_TEST_CASE_P(SetProperty, SecurityPolicyRulesSetPropertyManifest,
                        ::testing::Values(
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      true,
                                                      false),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      false,
                                                      false),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      false,
                                                      false),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      true,
                                                      true),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      false,
                                                      true),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      false,
                                                      true),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                      true,
                                                      false),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                      false,
                                                      false),
                            SetPropertyRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                      false,
                                                      false)
                            ));

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

class SignalRulesTestValue {
  public:
    SignalRulesTestValue(uint8_t mask1, uint8_t mask2, bool canSendSignal, bool canReceiveSignal) :
        peer1ActionMask(mask1),
        peer2ActionMask(mask2),
        busObjAllowedToSendSignal(canSendSignal),
        allowedToReceiveSignal(canReceiveSignal) { }
    friend::std::ostream& operator<<(::std::ostream& os, const SignalRulesTestValue& val);
    uint8_t peer1ActionMask;
    uint8_t peer2ActionMask;
    bool busObjAllowedToSendSignal;
    bool allowedToReceiveSignal;
};

::std::ostream& operator<<(::std::ostream& os, const SignalRulesTestValue& val) {
    os << "peer1Mask = " << PrintActionMask(val.peer1ActionMask).c_str() << "\n";
    os << "peer2Mask = " << PrintActionMask(val.peer2ActionMask).c_str() << "\n";
    if (val.busObjAllowedToSendSignal) {
        os << "BusObject should be able to emit signals\n";
    } else {
        os << "BusObject should NOT be able to emit signals\n";
    }
    if (val.allowedToReceiveSignal) {
        os << "We are expected to be able to receive signals\n";
    } else {
        os << "We are NOT expected to be able to receive signals\n";
    }
    return os;
}

class SecurityPolicyRulesSignal : public SecurityPolicyRulesTest,
    public testing::WithParamInterface<SignalRulesTestValue> {
};

TEST_P(SecurityPolicyRulesSignal, PolicyRules)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
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
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Chirp",
                               PermissionPolicy::Rule::Member::SIGNAL,
                               GetParam().peer1ActionMask);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

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
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member peer2Prms[1];
                peer2Prms[0].Set("Chirp",
                                 PermissionPolicy::Rule::Member::SIGNAL,
                                 GetParam().peer2ActionMask);
                rules[0].SetMembers(1, peer2Prms);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());
    qcc::String p2policyStr = "\n----Peer2 Policy-----\n" + peer2Policy.ToString();
    SCOPED_TRACE(p2policyStr.c_str());

    /*
     * Create the ProxyBusObject and call the SecureConnection this will make
     * sure any permission keys are exchanged between peers
     */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

    MsgArg arg("s", "Chipr this String out in the signal.");
    // Signals are send and forget.  They will always return ER_OK.
    QStatus status = peer1BusObject.Signal(peer2Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId, *peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), &arg, 1);

    if (GetParam().busObjAllowedToSendSignal) {
        EXPECT_EQ(ER_OK, status);
        //Wait for a maximum of 2 sec for the Chirp Signal.
        for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
            if (chirpSignalReceiver.signalReceivedFlag) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }
        if (GetParam().allowedToReceiveSignal) {
            EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);
        } else {
            EXPECT_FALSE(chirpSignalReceiver.signalReceivedFlag) << "According to the policy rules we should NOT be able to send a signal";
        }

    } else {
        EXPECT_EQ(ER_PERMISSION_DENIED, status);
        EXPECT_FALSE(chirpSignalReceiver.signalReceivedFlag) << "According to the policy rules we should NOT be able to send a signal";
    }

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
}

INSTANTIATE_TEST_CASE_P(Signal, SecurityPolicyRulesSignal,
                        ::testing::Values(
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //0
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 false,      //can send signal
                                                 true),      //can receive signal
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //1
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 false,
                                                 false),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //2
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 false,
                                                 false),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //3
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 false,
                                                 true),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //4
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 false,
                                                 false),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //5
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 false,
                                                 false),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //6
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 true,
                                                 true),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //7
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 true,
                                                 false),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //8
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 true,
                                                 false)
                            ));

class SecurityPolicyRulesSignalManifest : public SecurityPolicyRulesTest,
    public testing::WithParamInterface<SignalRulesTestValue> {
};

TEST_P(SecurityPolicyRulesSignalManifest, PolicyRules)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));

    /* install permissions to send signals */
    //Permission policy that will be installed on peer1
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
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Chirp",
                               PermissionPolicy::Rule::Member::SIGNAL,
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

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
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Chirp",
                               PermissionPolicy::Rule::Member::SIGNAL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy, true, true);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy, true, true);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    /* After having a new policy installed, the target bus
       clears out all of its peer's secret and session keys, so the
       next call will get security violation.  So just make the call and ignore
       the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    const size_t manifestSize = 1;
    const size_t certChainSize = 1;
    /*************Update Peer1 Manifest *************/
    //peer1 key
    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    {
        PermissionPolicy::Rule::Member member[1];
        member[0].Set("Chirp", PermissionPolicy::Rule::Member::SIGNAL, GetParam().peer1ActionMask);
        peer1Manifest[0].SetObjPath("*");
        peer1Manifest[0].SetInterfaceName(interfaceName);
        peer1Manifest[0].SetMembers(1, member);
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
    {
        PermissionPolicy::Rule::Member member[1];
        member[0].Set("Chirp", PermissionPolicy::Rule::Member::SIGNAL, GetParam().peer2ActionMask);
        peer2Manifest[0].SetObjPath("*");
        peer2Manifest[0].SetInterfaceName(interfaceName);
        peer2Manifest[0].SetMembers(1, member);
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
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /*
     * Create the ProxyBusObject and call the SecureConnection this will make
     * sure any permission keys are exchanged between peers
     */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

    MsgArg arg("s", "Chipr this String out in the signal.");
    QStatus status = peer1BusObject.Signal(peer2Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId, *peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), &arg, 1);

    if (GetParam().busObjAllowedToSendSignal) {
        EXPECT_EQ(ER_OK, status);
        //Wait for a maximum of 2 sec for the Chirp Signal.
        for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
            if (chirpSignalReceiver.signalReceivedFlag) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }
        if (GetParam().allowedToReceiveSignal) {
            EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);
        } else {
            EXPECT_FALSE(chirpSignalReceiver.signalReceivedFlag) << "According to the policy rules we should NOT be able to send a signal";
        }
    } else {
        EXPECT_EQ(ER_PERMISSION_DENIED, status);
        EXPECT_FALSE(chirpSignalReceiver.signalReceivedFlag) << "According to the policy rules we should NOT be able to send a signal";
    }

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
}

INSTANTIATE_TEST_CASE_P(Signal, SecurityPolicyRulesSignalManifest,
                        ::testing::Values(
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //0
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 false,         //can send signal
                                                 true),         //can receive signal
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //1
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 false,
                                                 true),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_PROVIDE, //2
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 true,
                                                 true),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //3
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 false,
                                                 false),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //4
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 false,
                                                 false),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_MODIFY, //5
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 true,
                                                 false),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //6
                                                 PermissionPolicy::Rule::Member::ACTION_PROVIDE,
                                                 false,
                                                 false),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //7
                                                 PermissionPolicy::Rule::Member::ACTION_MODIFY,
                                                 false,
                                                 false),
                            SignalRulesTestValue(PermissionPolicy::Rule::Member::ACTION_OBSERVE, //8
                                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE,
                                                 true,
                                                 false)
                            ));

/*
 * Setup following policies and manifests
 *
 * Peer1 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 * Peer1 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 *
 * Expected Result:
 * Both properties Prop1 and Prop2 will be fetched.
 */
TEST_F(SecurityPolicyRulesTest, GetAllProperties_test1_properties_succesfully_sent)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));
    const size_t manifestSize = 1;

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        CreatePolicyWithMembersForGetAllProperties(peer1Policy, members, 1);
    }

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    peer1Manifest[0].SetObjPath("/test");
    peer1Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[0].SetMembers(2, members);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        CreatePolicyWithMembersForGetAllProperties(peer2Policy, members, 2);
    }

    // Peer2 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    peer2Manifest[0].SetObjPath("/test");
    peer2Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        peer2Manifest[0].SetMembers(1, members);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));

    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    UpdatePeer1Manifest(peer1Manifest, manifestSize);
    UpdatePeer2Manifest(peer2Manifest, manifestSize);
    //------------------------------------------------------------------------//
    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg props;
    EXPECT_EQ(ER_OK, proxy.GetAllProperties(interfaceName, props));

    {
        int32_t prop1;
        MsgArg* propArg;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop1", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop1)) << propArg->ToString().c_str();
        EXPECT_EQ(42, prop1);
    }
    {
        int32_t prop2;
        MsgArg* propArg;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop2", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop2)) << propArg->ToString().c_str();
        EXPECT_EQ(17, prop2);
    }

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Setup following policies and manifests
 *
 * Peer1 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 * Peer1 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}
 * Peer2 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 *
 * Expected Result:
 * GetAllProperties will be successfully sent.
 * Only Prop1 will be fetched.
 */
TEST_F(SecurityPolicyRulesTest, GetAllProperties_test2_only_prop1_successfully_fetched)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));
    const size_t manifestSize = 1;

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        CreatePolicyWithMembersForGetAllProperties(peer1Policy, members, 1);
    }

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    peer1Manifest[0].SetObjPath("/test");
    peer1Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[0].SetMembers(1, members);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        CreatePolicyWithMembersForGetAllProperties(peer2Policy, members, 2);
    }

    // Peer2 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    peer2Manifest[0].SetObjPath("/test");
    peer2Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        peer2Manifest[0].SetMembers(1, members);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));

    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    UpdatePeer1Manifest(peer1Manifest, manifestSize);
    UpdatePeer2Manifest(peer2Manifest, manifestSize);
    //------------------------------------------------------------------------//
    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg props;
    EXPECT_EQ(ER_OK, proxy.GetAllProperties(interfaceName, props));

    {
        int32_t prop1;
        MsgArg* propArg;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop1", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop1)) << propArg->ToString().c_str();
        EXPECT_EQ(42, prop1);
    }
    {
        MsgArg* propArg;
        EXPECT_EQ(ER_BUS_ELEMENT_NOT_FOUND, props.GetElement("{sv}", "Prop2", &propArg)) << props.ToString().c_str();
    }

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Setup following policies and manifests
 *
 * Peer1 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 * Peer1 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}
 * Peer2 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 *
 * Expected Result:
 * GetAllProperties will be successfully sent.
 * Only Prop1 will be fetched.
 */
TEST_F(SecurityPolicyRulesTest, GetAllProperties_test3_only_prop1_successfully_fetched)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));
    const size_t manifestSize = 1;

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        CreatePolicyWithMembersForGetAllProperties(peer1Policy, members, 1);
    }

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    peer1Manifest[0].SetObjPath("/test");
    peer1Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[0].SetMembers(2, members);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Prop1",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        CreatePolicyWithMembersForGetAllProperties(peer2Policy, members, 1);
    }

    // Peer2 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    peer2Manifest[0].SetObjPath("/test");
    peer2Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        peer2Manifest[0].SetMembers(1, members);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));

    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    UpdatePeer1Manifest(peer1Manifest, manifestSize);
    UpdatePeer2Manifest(peer2Manifest, manifestSize);
    //------------------------------------------------------------------------//
    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg props;
    EXPECT_EQ(ER_OK, proxy.GetAllProperties(interfaceName, props));

    {
        int32_t prop1;
        MsgArg* propArg;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop1", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop1)) << propArg->ToString().c_str();
        EXPECT_EQ(42, prop1);
    }
    {
        MsgArg* propArg;
        EXPECT_EQ(ER_BUS_ELEMENT_NOT_FOUND, props.GetElement("{sv}", "Prop2", &propArg)) << props.ToString().c_str();
    }

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Setup following policies and manifests
 *
 * Peer1 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 * Peer1 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Method_call, PROVIDE}
 *
 * Expected Result:
 * GetAllProperties won’t be sent. Because, The wildcard rule is for a Method
 * and not for a Property
 */
TEST_F(SecurityPolicyRulesTest, GetAllProperties_test4_no_properties_fetched)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));
    const size_t manifestSize = 1;

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        CreatePolicyWithMembersForGetAllProperties(peer1Policy, members, 1);
    }

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    peer1Manifest[0].SetObjPath("/test");
    peer1Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[0].SetMembers(2, members);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        CreatePolicyWithMembersForGetAllProperties(peer2Policy, members, 2);
    }

    // Peer2 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    peer2Manifest[0].SetObjPath("/test");
    peer2Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*", PermissionPolicy::Rule::Member::METHOD_CALL, PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        peer2Manifest[0].SetMembers(1, members);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));

    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    UpdatePeer1Manifest(peer1Manifest, manifestSize);
    UpdatePeer2Manifest(peer2Manifest, manifestSize);
    //------------------------------------------------------------------------//
    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg props;
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetAllProperties(interfaceName, props));

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Setup following policies and manifests
 *
 * Peer1 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Method_call, PROVIDE}
 * Peer1 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 *
 * Expected Result:
 * GetAllProperties won’t be sent. Because, the wildcard rule is for a Method
 * and not for a Property
 */
TEST_F(SecurityPolicyRulesTest, GetAllProperties_test5_no_properties_fetched)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));
    const size_t manifestSize = 1;

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::METHOD_CALL,
                       PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        CreatePolicyWithMembersForGetAllProperties(peer1Policy, members, 1);
    }

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    peer1Manifest[0].SetObjPath("/test");
    peer1Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[0].SetMembers(2, members);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        CreatePolicyWithMembersForGetAllProperties(peer2Policy, members, 2);
    }

    // Peer2 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    peer2Manifest[0].SetObjPath("/test");
    peer2Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        peer2Manifest[0].SetMembers(1, members);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));

    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    UpdatePeer1Manifest(peer1Manifest, manifestSize);
    UpdatePeer2Manifest(peer2Manifest, manifestSize);
    //------------------------------------------------------------------------//
    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg props;
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetAllProperties(interfaceName, props));

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Setup following policies and manifests
 *
 * Peer1 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 * Peer1 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, OBSERVE}
 * Peer2 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 *
 * Expected Result:
 * GetAllProperties will be successfully sent.
 * Both properties Prop1 and Prop2 will be fetched.
 */
TEST_F(SecurityPolicyRulesTest, GetAllProperties_test6_properties_successfully_fetched)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));
    const size_t manifestSize = 1;

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        CreatePolicyWithMembersForGetAllProperties(peer1Policy, members, 1);
    }

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    peer1Manifest[0].SetObjPath("/test");
    peer1Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[0].SetMembers(2, members);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        CreatePolicyWithMembersForGetAllProperties(peer2Policy, members, 1);
    }

    // Peer2 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    peer2Manifest[0].SetObjPath("/test");
    peer2Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        peer2Manifest[0].SetMembers(1, members);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));

    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    UpdatePeer1Manifest(peer1Manifest, manifestSize);
    UpdatePeer2Manifest(peer2Manifest, manifestSize);
    //------------------------------------------------------------------------//
    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg props;
    EXPECT_EQ(ER_OK, proxy.GetAllProperties(interfaceName, props));

    {
        int32_t prop1;
        MsgArg* propArg;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop1", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop1)) << propArg->ToString().c_str();
        EXPECT_EQ(42, prop1);
    }
    {
        int32_t prop2;
        MsgArg* propArg;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop2", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop2)) << propArg->ToString().c_str();
        EXPECT_EQ(17, prop2);
    }

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Setup following policies and manifests
 *
 * Peer1 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 * Peer1 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, OBSERVE}
 * Peer2 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 *
 * Expected Result:
 * GetAllProperties will be successfully sent.
 * Both properties Prop1 and Prop2 will be fetched
 */
TEST_F(SecurityPolicyRulesTest, GetAllProperties_test7_properties_successfully_fetched)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));
    const size_t manifestSize = 1;

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        CreatePolicyWithMembersForGetAllProperties(peer1Policy, members, 1);
    }

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    peer1Manifest[0].SetObjPath("/test");
    peer1Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[0].SetMembers(1, members);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        CreatePolicyWithMembersForGetAllProperties(peer2Policy, members, 2);
    }

    // Peer2 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    peer2Manifest[0].SetObjPath("/test");
    peer2Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        peer2Manifest[0].SetMembers(1, members);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));

    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    UpdatePeer1Manifest(peer1Manifest, manifestSize);
    UpdatePeer2Manifest(peer2Manifest, manifestSize);
    //------------------------------------------------------------------------//
    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg props;
    EXPECT_EQ(ER_OK, proxy.GetAllProperties(interfaceName, props));

    {
        int32_t prop1;
        MsgArg* propArg;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop1", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop1)) << propArg->ToString().c_str();
        EXPECT_EQ(42, prop1);
    }
    {
        int32_t prop2;
        MsgArg* propArg;
        EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop2", &propArg)) << props.ToString().c_str();
        EXPECT_EQ(ER_OK, propArg->Get("i", &prop2)) << propArg->ToString().c_str();
        EXPECT_EQ(17, prop2);
    }

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Setup following policies and manifests
 *
 * Peer1 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 * Peer1 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, PROVIDE}
 *
 * Expected Result:
 * GetAllProperties won’t be sent. Because, the receiver manifest does not have
 * the wildcard for the property
 */
TEST_F(SecurityPolicyRulesTest, GetAllProperties_test8_no_properties_fetched)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));
    const size_t manifestSize = 1;

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        CreatePolicyWithMembersForGetAllProperties(peer1Policy, members, 1);
    }

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    peer1Manifest[0].SetObjPath("/test");
    peer1Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[0].SetMembers(2, members);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        CreatePolicyWithMembersForGetAllProperties(peer2Policy, members, 2);
    }

    // Peer2 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    peer2Manifest[0].SetObjPath("/test");
    peer2Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        peer2Manifest[0].SetMembers(1, members);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));

    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    UpdatePeer1Manifest(peer1Manifest, manifestSize);
    UpdatePeer2Manifest(peer2Manifest, manifestSize);
    //------------------------------------------------------------------------//
    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg props;
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetAllProperties(interfaceName, props));

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Setup following policies and manifests
 *
 * Peer1 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, PROVIDE}
 * Peer1 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 policy
 * -----------------
 * PeerType: ANY_TRUSTED
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { Prop1, Property, OBSERVE}, {Prop2, Property, OBSERVE}
 * Peer2 manifest:
 * ------------------
 * Rule:
 *   ObjectPath: /test
 *   interfaceName: org.allseen.test.SecurityApplication.rules
 *   Members: { *, Property, PROVIDE}
 *
 * Expected Result:
 * GetAllProperties won’t be sent. Because, the sender local policy does not
 * have the wildcard for Property.
 */
TEST_F(SecurityPolicyRulesTest, GetAllProperties_test9_no_properties_fetched)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));
    const size_t manifestSize = 1;

    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Prop1",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        CreatePolicyWithMembersForGetAllProperties(peer1Policy, members, 1);
    }

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    peer1Manifest[0].SetObjPath("/test");
    peer1Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[0].SetMembers(2, members);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    {
        PermissionPolicy::Rule::Member members[2];
        members[0].Set("Prop1",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        members[1].Set("Prop2",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        CreatePolicyWithMembersForGetAllProperties(peer2Policy, members, 2);
    }

    // Peer2 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    peer2Manifest[0].SetObjPath("/test");
    peer2Manifest[0].SetInterfaceName(interfaceName);
    {
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*", PermissionPolicy::Rule::Member::PROPERTY, PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        peer2Manifest[0].SetMembers(1, members);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str());
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str());

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));

    /*
     * After having a new policy installed, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy retPolicy;
    sapWithPeer1.GetPolicy(retPolicy);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    sapWithPeer2.GetPolicy(retPolicy);

    UpdatePeer1Manifest(peer1Manifest, manifestSize);
    UpdatePeer2Manifest(peer2Manifest, manifestSize);
    //------------------------------------------------------------------------//
    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    MsgArg props;
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetAllProperties(interfaceName, props));

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose
 * Verify that DENY rules that are specific or wild card do not take effect if
 * the ACL has a peer type of ALL.
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * Peer1 and Peer2 set up an ECDHE_NULL based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 rules:
 * ACL: Peer type: ALL
 * Rule 0: Object Path= *, Interface=*, Member name=*, Member Type = NS, Action mask = DENY
 * Rule 1: Object Path=/test, Interface = org.allseen.test.SecurityApplication.rules, Member name = *, Member type = NS, Action Mask = DENY
 * Rule 2: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * Peer2 rules:
 * ACL: Peer type: ALL
 * Rule 0: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by B.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_1)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ALL);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[3];
            //rule 0
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 1
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 2
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[2].SetMembers(1, members);
            }
            acls[0].SetRules(3, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ALL);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            //rule 0
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

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    /* We should be using a ECDHE_NULL based session */
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", managerAuthListener, NULL, false));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", managerAuthListener, NULL, false));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose
 * Verify that DENY rules that are specific or wild card do not take effect if
 * the ACL has a peer type of ALL.
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * Peer1 and Peer2 set up an ECDHE_NULL based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 * Peer1 sends a signal to Peer2.
 *
 * Peer1 rules:
 * ACL: Peer type: ALL
 * Rule 0: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * Peer1 rules:
 * ACL: Peer type: ALL
 * Rule 0: Object Path= *, Interface=*, Member name=*, Member Type = NS, Action mask = DENY
 * Rule 1: Object Path=/test, Interface = org.allseen.test.SecurityApplication.rules, Member name = *, Member type = NS, Action Mask = DENY
 * Rule 2: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 *
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by B.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_2)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    // Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ALL);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            //rule 0
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
        peer1Policy.SetAcls(1, acls);
    }

    //Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ALL);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[3];
            //rule 0
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 1
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 2
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[2].SetMembers(1, members);
            }
            acls[0].SetRules(3, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    /* We should be using a ECDHE_NULL based session */
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", managerAuthListener, NULL, false));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", managerAuthListener, NULL, false));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}


/*
 * Purpose
 * Verify that DENY rules that are specific or wild card do not take effect if
 * the ACL has a peer type of ANY_TRUSTED.
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 0: Object Path= *, Interface=*, Member name=*, Member Type = NS, Action mask = DENY
 * Rule 1: Object Path=/test, Interface = org.allseen.test.SecurityApplication.rules, Member name = *, Member type = NS, Action Mask = DENY
 * Rule 2: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * Peer2 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 0: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_3)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
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
            PermissionPolicy::Rule rules[3];
            //rule 0
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 1
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 2
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[2].SetMembers(1, members);
            }
            acls[0].SetRules(3, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }
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
            //rule 0
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

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose
 * Verify that DENY rules that are specific or wild card do not take effect if
 * the ACL has a peer type of ANY_TRUSTED.
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 0: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * Peer2 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 0: Object Path= *, Interface=*, Member name=*, Member Type = NS, Action mask = DENY
 * Rule 1: Object Path=/test, Interface = org.allseen.test.SecurityApplication.rules, Member name = *, Member type = NS, Action Mask = DENY
 * Rule 2: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 *
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_4)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
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
            //rule 0
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
        peer1Policy.SetAcls(1, acls);
    }

    //Permission policy that will be installed on peer2
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
            PermissionPolicy::Rule rules[3];
            //rule 0
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 1
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 2
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[2].SetMembers(1, members);
            }
            acls[0].SetRules(3, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}


/*
 * Purpose
 * Verify that DENY rules that are specific or wild card do not take effect
 * if the ACL has a peer type of WITH_CA
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * ICC of Peer2: CA1->Peer2
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 rules:
 * ACL: Peer type: WITH_CA Public Key: CA1
 * Rule 0: Object Path= *, Interface=*, Member name=*, Member Type = NS, Action mask = DENY
 * Rule 1: Object Path=/test, Interface = org.allseen.test.SecurityApplication.rules, Member name = *, Member type = NS, Action Mask = DENY
 * Rule 2: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * Peer2 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 0: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, DISABLED_PolicyRules_DENY_5)
{
    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetObjPath("*");
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    PermissionMgmtObj::GenerateManifestDigest(managerBus, manifest, 1, digest, Crypto_SHA256::DIGEST_SIZE);

    uint8_t subjectCN[] = { 1, 2, 3, 4 };
    uint8_t issuerCN[] = { 5, 6, 7, 8 };

    //Create the CA1 cert
    qcc::IdentityCertificate ca1Cert;
    ca1Cert.SetSerial((uint8_t*)"5678", 5);
    ca1Cert.SetIssuerCN(issuerCN, 4);
    ca1Cert.SetSubjectCN(issuerCN, 4);
    CertificateX509::ValidPeriod validityCA;
    validityCA.validFrom = qcc::GetEpochTimestamp() / 1000;
    validityCA.validTo = validityCA.validFrom + TEN_MINS;
    ca1Cert.SetValidity(&validityCA);
    ca1Cert.SetDigest(digest, Crypto_SHA256::DIGEST_SIZE);

    KeyInfoNISTP256 peer1PublicKey;
    PermissionConfigurator& peer1PermissionConfigurator = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.GetSigningPublicKey(peer1PublicKey));

    ca1Cert.SetSubjectPublicKey(peer1PublicKey.GetPublicKey());
    ca1Cert.SetAlias("ca1-cert-alias");
    ca1Cert.SetCA(true);

    //sign the ca1 cert
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.SignCertificate(ca1Cert));

    // Create the peer2Cert
    qcc::IdentityCertificate peer2Cert;
    peer2Cert.SetSerial((uint8_t*)"1234", 5);
    peer2Cert.SetIssuerCN(issuerCN, 4);
    peer2Cert.SetSubjectCN(subjectCN, 4);
    CertificateX509::ValidPeriod validity;
    validity.validFrom = qcc::GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + TEN_MINS;
    peer2Cert.SetValidity(&validity);
    peer2Cert.SetDigest(digest, Crypto_SHA256::DIGEST_SIZE);

    ECCPublicKey peer2PublicKey;
    sapWithPeer2.GetEccPublicKey(peer2PublicKey);

    peer2Cert.SetSubjectPublicKey(&peer2PublicKey);
    peer2Cert.SetAlias("peer2-cert-alias");
    peer2Cert.SetCA(true);

    //sign the leaf cert
    //PermissionConfigurator& peer2PermissionConfigurator = peer2Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.SignCertificate(peer2Cert));

    //We need identityCert chain CA1->Peer2
    const size_t certChainSize = 2;
    IdentityCertificate identityCertChain[certChainSize];
    identityCertChain[0] = peer2Cert;
    identityCertChain[1] = ca1Cert;

    // Call UpdateIdentity to install the cert chain
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdateIdentity(identityCertChain, certChainSize, manifest, manifestSize))
        << "Failed to update Identity cert or manifest ";

    /*
     * After updating the identity, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy dummyPolicy;
    sapWithPeer2.GetPolicy(dummyPolicy);

    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
            peers[0].SetKeyInfo(&peer1PublicKey);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[3];
            //rule 0
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 1
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 2
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[2].SetMembers(1, members);
            }
            acls[0].SetRules(3, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }
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
            //rule 0
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
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}


/*
 * Purpose
 * Verify that DENY rules that are specific or wild card do not
 * take effect if the ACL has a peer type of WITH_CA.
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 0: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * Peer2 rules:
 * ACL: Peer type: WITH_CA Public Key: CA1
 * Rule 0: Object Path= *, Interface=*, Member name=*, Member Type = NS, Action mask = DENY
 * Rule 1: Object Path=/test, Interface = org.allseen.test.SecurityApplication.rules, Member name = *, Member type = NS, Action Mask = DENY
 * Rule 2: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 *
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, DISABLED_PolicyRules_DENY_6)
{
    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    // All Inclusive manifest
    PermissionPolicy::Rule::Member member[1];
    member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED, PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetObjPath("*");
    manifest[0].SetInterfaceName("*");
    manifest[0].SetMembers(1, member);

    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    PermissionMgmtObj::GenerateManifestDigest(managerBus, manifest, 1, digest, Crypto_SHA256::DIGEST_SIZE);

    uint8_t subjectCN[] = { 1, 2, 3, 4 };
    uint8_t issuerCN[] = { 5, 6, 7, 8 };

    //Create the CA1 cert
    qcc::IdentityCertificate ca1Cert;
    ca1Cert.SetSerial((uint8_t*)"5678", 5);
    ca1Cert.SetIssuerCN(issuerCN, 4);
    ca1Cert.SetSubjectCN(issuerCN, 4);
    CertificateX509::ValidPeriod validityCA;
    validityCA.validFrom = 1427404154;
    validityCA.validTo = 1427404154 + 630720000;
    ca1Cert.SetValidity(&validityCA);
    ca1Cert.SetDigest(digest, Crypto_SHA256::DIGEST_SIZE);

    KeyInfoNISTP256 peer1PublicKey;
    PermissionConfigurator& peer1PermissionConfigurator = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.GetSigningPublicKey(peer1PublicKey));

    ca1Cert.SetSubjectPublicKey(peer1PublicKey.GetPublicKey());
    ca1Cert.SetAlias("ca1-cert-alias");
    ca1Cert.SetCA(true);

    //sign the ca1 cert
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.SignCertificate(ca1Cert));

    // Create the peer2Cert
    qcc::IdentityCertificate peer2Cert;
    peer2Cert.SetSerial((uint8_t*)"1234", 5);
    peer2Cert.SetIssuerCN(issuerCN, 4);
    peer2Cert.SetSubjectCN(subjectCN, 4);
    CertificateX509::ValidPeriod validity;
    validity.validFrom = qcc::GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + TEN_MINS;
    peer2Cert.SetValidity(&validity);
    peer2Cert.SetDigest(digest, Crypto_SHA256::DIGEST_SIZE);

    ECCPublicKey peer2PublicKey;
    sapWithPeer2.GetEccPublicKey(peer2PublicKey);

    peer2Cert.SetSubjectPublicKey(&peer2PublicKey);
    peer2Cert.SetAlias("peer2-cert-alias");
    peer2Cert.SetCA(true);

    //sign the leaf cert
    EXPECT_EQ(ER_OK, peer1PermissionConfigurator.SignCertificate(peer2Cert));

    //We need identityCert chain CA1->Peer2
    const size_t certChainSize = 2;
    IdentityCertificate identityCertChain[certChainSize];
    identityCertChain[0] = peer2Cert;
    identityCertChain[1] = ca1Cert;

    // Call UpdateIdentity to install the cert chain
    EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChain, certChainSize, manifest, manifestSize))
        << "Failed to update Identity cert or manifest ";

    /*
     * After updating the identity, the target bus clears out all of
     * its peer's secret and session keys, so the next call will get security
     * violation.  So just make the call and ignore the outcome.
     */
    PermissionPolicy dummyPolicy;
    sapWithPeer2.GetPolicy(dummyPolicy);

    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
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
            //rule 0
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
        peer1Policy.SetAcls(1, acls);
    }

    //Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
            peers[0].SetKeyInfo(&peer1PublicKey);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[3];
            //rule 0
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 1
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 2
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[2].SetMembers(1, members);
            }
            acls[0].SetRules(3, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose
 * Verify that DENY rules that are specific or wild card do not take effect if
 * the ACL has a peer type of WITH_MEMBERSHIP.
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * Peer2 has membership certificate installed.
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 rules:
 * ACL: Peer type: WITH_MEMBERSHIP, SGID: Living Room, Security Group Authority public key
 * Rule 1: Object Path= *, Interface=*, Member name=*, Member Type = NS, Action mask = DENY
 * Rule 2: Object Path=/test, Interface = org.allseen.test.SecurityApplication.rules, Member name = *, Member type = NS, Action Mask = DENY
 * Rule 3: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * Peer2 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 0: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_7)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(managerGuid);
            //Get manager key
            KeyInfoNISTP256 managerKey;
            PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));
            peers[0].SetKeyInfo(&managerKey);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[3];
            //rule 0
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 1
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 2
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[2].SetMembers(1, members);
            }
            acls[0].SetRules(3, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }
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
            //rule 0
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

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose
 * Verify that DENY rules that are specific or wild card do not take effect if
 * the ACL has a peer type of WITH_MEMBERSHIP.
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * Peer2 has membership certificate installed.
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 0: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * Peer2 rules:
 * ACL: Peer type: WITH_MEMBERSHIP, SGID: Living Room, Security Group Authority public key
 * Rule 1: Object Path= *, Interface=*, Member name=*, Member Type = NS, Action mask = DENY
 * Rule 2: Object Path=/test, Interface = org.allseen.test.SecurityApplication.rules, Member name = *, Member type = NS, Action Mask = DENY
 * Rule 3: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_8)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
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
            //rule 0
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
        peer1Policy.SetAcls(1, acls);
    }

    //Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(managerGuid);
            //Get manager key
            KeyInfoNISTP256 managerKey;
            PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));
            peers[0].SetKeyInfo(&managerKey);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[3];
            //rule 0
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 1
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 2
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[2].SetMembers(1, members);
            }
            acls[0].SetRules(3, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose
 * Verify that DENY rules takes effect and precedence over other rules only if
 * it is a wild card under the peer type: WITH_PUBLICKEY DUT is sender.
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 rules:
 * ACL: Peer type: WITH_PUBLICKEY, Public Key of Peer2
 * Rule 1: Object Path= *, Interface=*, Member name=*, Member Type = NS, Action mask = DENY
 * Rule 2: Object Path=/test, Interface = org.allseen.test.SecurityApplication.rules, Member name = *, Member type = NS, Action Mask = DENY
 * Rule 3: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * Peer2 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 0: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * verification:
 * Verify that method call, get/set property calls, signal cannot be sent by
 * sender.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_9)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            // Peer type: WITH_PUBLICKEY, Public Key of Peer2
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            KeyInfoNISTP256 peer2Key;
            PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));
            peers[0].SetKeyInfo(&peer2Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[3];
            //rule 0 Object Path= *, Interface=*, Member name=*, Member Type = NS, Action mask = DENY
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 1 Object Path=/test, Interface = org.allseen.test.SecurityApplication.rules, Member name = *, Member type = NS, Action Mask = DENY
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 2 Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[2].SetMembers(1, members);
            }
            acls[0].SetRules(3, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            // Peer type: ANY_TRUSTED
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            //rule 0 Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
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

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

    arg.Set("s", "Chirp this String out in the signal.");
    // Signals are send and forget.  They will always return ER_OK.
    EXPECT_EQ(ER_PERMISSION_DENIED, peer1BusObject.Signal(peer2Bus.GetUniqueName().c_str(), peer1ToPeer2SessionId, *peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), &arg, 1, 0, 0));

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose
 * Verify that DENY rules takes effect and precedence over other rules only if
 * it is a wild card under the peer type: WITH_PUBLICKEY DUT is receiver.
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 0: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * Peer2 rules:
 * ACL: Peer type: WITH_PUBLICKEY, Public Key of Peer1
 * Rule 1: Object Path= *, Interface=*, Member name=*, Member Type = NS, Action mask = DENY
 * Rule 2: Object Path=/test, Interface = org.allseen.test.SecurityApplication.rules, Member name = *, Member type = NS, Action Mask = DENY
 * Rule 3: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * verification:
 * Verify that method call, get/set property calls, signal can be sent, but are
 * not received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_10)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
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
            //rule 0
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
        peer1Policy.SetAcls(1, acls);
    }

    //Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            //Get manager key
            KeyInfoNISTP256 peer1Key;
            PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));
            peers[0].SetKeyInfo(&peer1Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[3];
            //rule 0
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 1
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 2
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[2].SetMembers(1, members);
            }
            acls[0].SetRules(3, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    EXPECT_STREQ("org.alljoyn.Bus.Security.Error.PermissionDenied", replyMsg->GetErrorName());

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_FALSE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose
 * Verify that DENY rule does not take effect if it is a specific rule under the
 * peer type: WITH_PUBLICKEY
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 rules:
 * ACL: Peer type: WITH_PUBLICKEY, Public Key of peer2
 * Rule1: Object Path: *; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule2: Object Path: /test ; Interface: *; Member Name: *, Action: DENY
 * Rule3: Object Path: * ; Interface: *; Member Name: Echo , Action: DENY
 * Rule4: Object Path: * ; Interface: *; Member Name: Prop1 , Action: DENY
 * Rule5: Object Path: * ; Interface: *; Member Name: Chirp , Action: DENY
 * Rule6: Object Path: /test ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule7: Object Path: /t*  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule8: Object Path: *  ; Interface: org.allseen.test.*; Member Name: *; Action: DENY
 * Rule9: Object Path: /test  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: Chirp, Action: DENY
 * Rule 10: Object Path: *, Interface: *, Member Name: NS; Action: PROVIDE|OBSERVE|MODIFY
 *
 * Peer2 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 1: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 *
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_11)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            //Get manager key
            KeyInfoNISTP256 peer2Key;
            PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));
            peers[0].SetKeyInfo(&peer2Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[10];
            //rule 1
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 2
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 3
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               0 /*DENY*/);
                rules[2].SetMembers(1, members);
            }
            //rule 4
            rules[3].SetObjPath("*");
            rules[3].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Prop1",
                               PermissionPolicy::Rule::Member::PROPERTY,
                               0 /*DENY*/);
                rules[3].SetMembers(1, members);
            }
            //rule 5
            rules[4].SetObjPath("/test");
            rules[4].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Chirp",
                               PermissionPolicy::Rule::Member::SIGNAL,
                               0 /*DENY*/);
                rules[4].SetMembers(1, members);
            }
            //rule 6
            rules[5].SetObjPath("/test");
            rules[5].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[5].SetMembers(1, members);
            }
            //rule 7
            rules[6].SetObjPath("/t*");
            rules[6].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[6].SetMembers(1, members);
            }
            //rule 8
            rules[7].SetObjPath("*");
            rules[7].SetInterfaceName("org.allseen.test.*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[7].SetMembers(1, members);
            }
            //rule 9
            rules[8].SetObjPath("/test");
            rules[8].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Chirp",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[8].SetMembers(1, members);
            }
            //rule 10
            rules[9].SetObjPath("*");
            rules[9].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].SetMemberName("*");
                members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                                         PermissionPolicy::Rule::Member::ACTION_MODIFY |
                                         PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[9].SetMembers(1, members);
            }
            acls[0].SetRules(10, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

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
            //rule 0
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

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose
 * Verify that DENY rule does not take effect if it is a specific rule under the
 * peer type: WITH_PUBLICKEY
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 rules:
 * ACL: Peer type: ANY_TRUSTED
 * Rule 1: Object Path= *, Interface=*, Member Name=*, Member Type= NS, Action Mask = PROVIDE|MODIFY|OBSERVE
 *
 * Peer2 rules:
 * ACL: Peer type: WITH_PUBLICKEY, Public Key of peer1
 * Rule1: Object Path: *; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule2: Object Path: /test ; Interface: *; Member Name: *, Action: DENY
 * Rule3: Object Path: * ; Interface: *; Member Name: Echo , Action: DENY
 * Rule4: Object Path: * ; Interface: *; Member Name: Prop1 , Action: DENY
 * Rule5: Object Path: * ; Interface: *; Member Name: Chirp , Action: DENY
 * Rule6: Object Path: /test ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule7: Object Path: /t*  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule8: Object Path: *  ; Interface: org.allseen.test.*; Member Name: *; Action: DENY
 * Rule9: Object Path: /test  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: Chirp, Action: DENY
 * Rule 10: Object Path: *, Interface: *, Member Name: NS; Action: PROVIDE|OBSERVE|MODIFY
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_12)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
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
            //rule 1
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
        peer1Policy.SetAcls(1, acls);
    }

    //Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            /* ACL: Peer type: WITH_PUBLICKEY, Public Key of peer1 */
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            //Get manager key
            KeyInfoNISTP256 peer1Key;
            PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));
            peers[0].SetKeyInfo(&peer1Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[10];
            //rule 1  Object Path: *; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY

            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[0].SetMembers(1, members);
            }
            //rule 2 Object Path: /test ; Interface: *; Member Name: *, Action: DENY
            rules[1].SetObjPath("/test");
            rules[1].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[1].SetMembers(1, members);
            }
            //rule 3 Object Path: * ; Interface: *; Member Name: Echo , Action: DENY
            rules[2].SetObjPath("*");
            rules[2].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               0 /*DENY*/);
                rules[2].SetMembers(1, members);
            }
            //rule 4 Object Path: * ; Interface: *; Member Name: Prop1 , Action: DENY
            rules[3].SetObjPath("*");
            rules[3].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Prop1",
                               PermissionPolicy::Rule::Member::PROPERTY,
                               0 /*DENY*/);
                rules[3].SetMembers(1, members);
            }
            //rule 5 Object Path: * ; Interface: *; Member Name: Chirp , Action: DENY
            rules[4].SetObjPath("/test");
            rules[4].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Chirp",
                               PermissionPolicy::Rule::Member::SIGNAL,
                               0 /*DENY*/);
                rules[4].SetMembers(1, members);
            }
            //rule 6 Object Path: /test ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
            rules[5].SetObjPath("/test");
            rules[5].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[5].SetMembers(1, members);
            }
            //rule 7 Object Path: /a*  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
            rules[6].SetObjPath("/t*");
            rules[6].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[6].SetMembers(1, members);
            }
            //rule 8 Object Path: *  ; Interface: test.*; Member Name: *; Action: DENY
            rules[7].SetObjPath("*");
            rules[7].SetInterfaceName("org.allseen.test.*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[7].SetMembers(1, members);
            }
            //rule 9 Object Path: /test  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: Chirp, Action: DENY
            rules[8].SetObjPath("/test");
            rules[8].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Chirp",
                               PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               0 /*DENY*/);
                rules[8].SetMembers(1, members);
            }
            //rule 10 Object Path: *, Interface: *, Member Name: NS; Action: PROVIDE|OBSERVE|MODIFY
            rules[9].SetObjPath("*");
            rules[9].SetInterfaceName("*");
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].SetMemberName("*");
                members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                                         PermissionPolicy::Rule::Member::ACTION_MODIFY |
                                         PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[9].SetMembers(1, members);
            }
            acls[0].SetRules(10, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose
 * Verify that specific rules with DENY action mask are ignored in the manifest.
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 *
 * Policy rules allow everything
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 Policy:
 * ACL 1: Peer type: ANY_TRUSTED, Rule: Allow everything
 *
 * Peer1 manifest:
 * Rule1: Object Path: *; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule2: Object Path: /test ; Interface: *; Member Name: *, Action: DENY
 * Rule3: Object Path: * ; Interface: *; Member Name: Echo , Action: DENY
 * Rule4: Object Path: * ; Interface: *; Member Name: Prop1 , Action: DENY
 * Rule5: Object Path: * ; Interface: *; Member Name: Chirp , Action: DENY
 * Rule6: Object Path: /test ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule7: Object Path: /t*  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule8: Object Path: *  ; Interface: org.allseen.test.*; Member Name: *; Action: DENY
 * Rule9: Object Path: /test  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: Chirp, Action: DENY
 * Rule 10: Object Path: *, Interface: *, Member Name: NS; Action: PROVIDE|OBSERVE|MODIFY

 * Peer2 Policy:
 * ACL 1: Peer type: ANY_TRUSTED: Allow everything
 *
 * Peer2 manifest:
 * Rule 1: Allow everything
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_13)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    const size_t manifestSize = 10;
    const size_t certChainSize = 1;
    /*************Update Peer1 Manifest *************/
    //peer1 key
    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    { //Rule1: Object Path: *; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
        peer1Manifest[0].SetObjPath("*");
        peer1Manifest[0].SetInterfaceName(interfaceName);
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer1Manifest[0].SetMembers(1, members);
    }
    { //Rule2: Object Path: /test ; Interface: *; Member Name: *, Action: DENY
        peer1Manifest[1].SetObjPath("/test");
        peer1Manifest[1].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer1Manifest[1].SetMembers(1, members);
    }
    { //Rule3: Object Path: * ; Interface: *; Member Name: Echo , Action: DENY
        peer1Manifest[2].SetObjPath("*");
        peer1Manifest[2].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Echo",
                       PermissionPolicy::Rule::Member::METHOD_CALL,
                       0 /*DENY*/);
        peer1Manifest[2].SetMembers(1, members);
    }
    { //Rule4: Object Path: * ; Interface: *; Member Name: Prop1 , Action: DENY
        peer1Manifest[3].SetObjPath("*");
        peer1Manifest[3].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Prop1",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       0 /*DENY*/);
        peer1Manifest[3].SetMembers(1, members);
    }
    { //Rule5: Object Path: * ; Interface: *; Member Name: Chirp , Action: DENY
        peer1Manifest[4].SetObjPath("*");
        peer1Manifest[4].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Chirp",
                       PermissionPolicy::Rule::Member::SIGNAL,
                       0 /*DENY*/);
        peer1Manifest[4].SetMembers(1, members);
    }
    { //Rule6: Object Path: /test ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
        peer1Manifest[5].SetObjPath("/test");
        peer1Manifest[5].SetInterfaceName(interfaceName);
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer1Manifest[5].SetMembers(1, members);
    }
    { //Rule7: Object Path: /t*  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
        peer1Manifest[6].SetObjPath("/t*");
        peer1Manifest[6].SetInterfaceName(interfaceName);
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer1Manifest[6].SetMembers(1, members);
    }
    { //Rule8: Object Path: *  ; Interface: org.allseen.test.*; Member Name: *; Action: DENY
        peer1Manifest[7].SetObjPath("*");
        peer1Manifest[7].SetInterfaceName("org.allseen.test.*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer1Manifest[7].SetMembers(1, members);
    }
    { //Rule9: Object Path: /test  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: Chirp, Action: DENY
        peer1Manifest[8].SetObjPath("/test");
        peer1Manifest[8].SetInterfaceName(interfaceName);
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Chirp",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer1Manifest[8].SetMembers(1, members);
    }
    { //Rule 10: Object Path: *, Interface: *, Member Name: NS; Action: PROVIDE|OBSERVE|MODIFY
        peer1Manifest[9].SetObjPath("*");
        peer1Manifest[9].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].SetMemberName("*");
        members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                                 PermissionPolicy::Rule::Member::ACTION_MODIFY |
                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[9].SetMembers(1, members);
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

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChainPeer1, certChainSize, peer1Manifest, manifestSize))
        << "Failed to update Identity cert or manifest ";

    //Peer2 already has a manifest installed that allows everything from the SetUp

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}


/*
 * Purpose
 * Verify that specific rules with DENY action mask are ignored in the manifest.
 *
 * Setup
 * Sender and Receiver bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 *
 * Policy rules allow everything
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 Policy:
 * ACL 1: Peer type: ANY_TRUSTED: Allow everything
 *
 * Peer1 manifest:
 * Rule 1: Allow everything
 *
 * Peer2 Policy:
 * ACL 1: Peer type: ANY_TRUSTED, Rule: Allow everything
 *
 * Peer2 manifest:
 * Rule1: Object Path: *; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule2: Object Path: /test ; Interface: *; Member Name: *, Action: DENY
 * Rule3: Object Path: * ; Interface: *; Member Name: Echo , Action: DENY
 * Rule4: Object Path: * ; Interface: *; Member Name: Prop1 , Action: DENY
 * Rule5: Object Path: * ; Interface: *; Member Name: Chirp , Action: DENY
 * Rule6: Object Path: /test ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule7: Object Path: /t*  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
 * Rule8: Object Path: *  ; Interface: org.allseen.test.*; Member Name: *; Action: DENY
 * Rule9: Object Path: /test  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: Chirp, Action: DENY
 * Rule 10: Object Path: *, Interface: *, Member Name: NS; Action: PROVIDE|OBSERVE|MODIFY
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_14)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    //Peer2 already has a manifest installed that allows everything from the SetUp

    const size_t manifestSize = 10;
    const size_t certChainSize = 1;
    /*************Update Peer1 Manifest *************/
    //peer1 key
    KeyInfoNISTP256 peer2Key;
    PermissionConfigurator& pcPeer1 = peer2Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer2Key));

    // Peer1 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    { //Rule1: Object Path: *; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
        peer2Manifest[0].SetObjPath("*");
        peer2Manifest[0].SetInterfaceName(interfaceName);
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer2Manifest[0].SetMembers(1, members);
    }
    { //Rule2: Object Path: /test ; Interface: *; Member Name: *, Action: DENY
        peer2Manifest[1].SetObjPath("/test");
        peer2Manifest[1].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer2Manifest[1].SetMembers(1, members);
    }
    { //Rule3: Object Path: * ; Interface: *; Member Name: Echo , Action: DENY
        peer2Manifest[2].SetObjPath("*");
        peer2Manifest[2].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Echo",
                       PermissionPolicy::Rule::Member::METHOD_CALL,
                       0 /*DENY*/);
        peer2Manifest[2].SetMembers(1, members);
    }
    { //Rule4: Object Path: * ; Interface: *; Member Name: Prop1 , Action: DENY
        peer2Manifest[3].SetObjPath("*");
        peer2Manifest[3].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Prop1",
                       PermissionPolicy::Rule::Member::PROPERTY,
                       0 /*DENY*/);
        peer2Manifest[3].SetMembers(1, members);
    }
    { //Rule5: Object Path: * ; Interface: *; Member Name: Chirp , Action: DENY
        peer2Manifest[4].SetObjPath("*");
        peer2Manifest[4].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Chirp",
                       PermissionPolicy::Rule::Member::SIGNAL,
                       0 /*DENY*/);
        peer2Manifest[4].SetMembers(1, members);
    }
    { //Rule6: Object Path: /test ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
        peer2Manifest[5].SetObjPath("/test");
        peer2Manifest[5].SetInterfaceName(interfaceName);
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer2Manifest[5].SetMembers(1, members);
    }
    { //Rule7: Object Path: /t*  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: *, Action: DENY
        peer2Manifest[6].SetObjPath("/t*");
        peer2Manifest[6].SetInterfaceName(interfaceName);
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer2Manifest[6].SetMembers(1, members);
    }
    { //Rule8: Object Path: *  ; Interface: org.allseen.test.*; Member Name: *; Action: DENY
        peer2Manifest[7].SetObjPath("*");
        peer2Manifest[7].SetInterfaceName("org.allseen.test.*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer2Manifest[7].SetMembers(1, members);
    }
    { //Rule9: Object Path: /test  ; Interface: org.allseen.test.SecurityApplication.rules; Member Name: Chirp, Action: DENY
        peer2Manifest[8].SetObjPath("/test");
        peer2Manifest[8].SetInterfaceName(interfaceName);
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("Chirp",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer2Manifest[8].SetMembers(1, members);
    }
    { //Rule 10: Object Path: *, Interface: *, Member Name: NS; Action: PROVIDE|OBSERVE|MODIFY
        peer2Manifest[9].SetObjPath("*");
        peer2Manifest[9].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].SetMemberName("*");
        members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                                 PermissionPolicy::Rule::Member::ACTION_MODIFY |
                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer2Manifest[9].SetMembers(1, members);
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
                                                                  "Peer1Alias",
                                                                  3600,
                                                                  identityCertChainPeer2[0],
                                                                  peer2Digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdateIdentity(identityCertChainPeer2, certChainSize, peer2Manifest, manifestSize))
        << "Failed to update Identity cert or manifest ";

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/*
 * Purpose
 * Verify that wild card DENY rule does not take effect in the manifest file.
 *
 * Setup
 * Peer2 and Peer2 bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 *
 * Policy rules allow everything
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 Policy:
 * ACL 1: Peer type: ANY_TRUSTED, Rule: Allow everything
 *
 * Peer1 manifest:
 * Sender Manifest:
 * Rule 1: Object Path: *, Interface: *, Member Name: *: Action: DENY
 * Rule 2: Object Path: *, Interface: *, Member Name: NS; Action: PROVIDE|OBSERVE|MODIFY

 * Peer2 Policy:
 * ACL 1: Peer type: ANY_TRUSTED: Allow everything
 *
 * Peer2 manifest:
 * Rule 1: Allow everything
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_15)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    const size_t manifestSize = 2;
    const size_t certChainSize = 1;
    /*************Update Peer1 Manifest *************/
    //peer1 key
    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    // Peer1 manifest
    PermissionPolicy::Rule peer1Manifest[manifestSize];
    { //Rule 1: Object Path: *, Interface: *, Member Name: *: Action: DENY
        peer1Manifest[0].SetObjPath("*");
        peer1Manifest[0].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer1Manifest[0].SetMembers(1, members);
    }
    { //Rule 2: Object Path: *, Interface: *, Member Name: NS; Action: PROVIDE|OBSERVE|MODIFY
        peer1Manifest[1].SetObjPath("*");
        peer1Manifest[1].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].SetMemberName("*");
        members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                                 PermissionPolicy::Rule::Member::ACTION_MODIFY |
                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer1Manifest[1].SetMembers(1, members);
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

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChainPeer1, certChainSize, peer1Manifest, manifestSize))
        << "Failed to update Identity cert or manifest ";

    //Peer2 already has a manifest installed that allows everything from the SetUp

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}


/*
 * Purpose
 * Verify that wild card DENY rule does not take effect in the manifest file.
 *
 * Setup
 * Peer1 and Peer2 bus implement the following :
 * Object Path:  /test
 * Interface Name: "org.allseen.test.SecurityApplication.rules"
 * Member name: Echo (method call)
 * Member name: Prop1  (property, read write)
 * Member name: Chirp (signal)
 *
 * Policy rules allow everything
 * Peer1 and Peer2 set up an ECDHE_ECDSA based session.
 * Peer1 makes a method call to Peer2.
 * Peer1 sends a signal to Peer2.
 * Peer1 makes a get property call on Peer2
 * Peer1 makes a set property call on Peer2
 *
 * Peer1 Policy:
 * ACL 1: Peer type: ANY_TRUSTED: Allow everything
 *
 * Peer1 manifest:
 * Rule 1: Allow everything
 *
 * Peer2 Policy:
 * ACL 1: Peer type: ANY_TRUSTED, Rule: Allow everything
 *
 * Peer2 manifest:
 * Sender Manifest:
 * Rule 1: Object Path: *, Interface: *, Member Name: *: Action: DENY
 * Rule 2: Object Path: *, Interface: *, Member Name: NS; Action: PROVIDE|OBSERVE|MODIFY
 *
 * verification:
 * Verify that method call, get/set property calls are successful.
 * Verify that signal is received by Peer2.
 */
TEST_F(SecurityPolicyRulesTest, PolicyRules_DENY_16)
{
    PolicyRulesTestBusObject peer1BusObject(peer1Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1BusObject));
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    //Peer2 already has a manifest installed that allows everything from the SetUp

    const size_t manifestSize = 2;
    const size_t certChainSize = 1;
    /*************Update Peer1 Manifest *************/
    //peer1 key
    KeyInfoNISTP256 peer2Key;
    PermissionConfigurator& pcPeer1 = peer2Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer2Key));

    // Peer1 manifest
    PermissionPolicy::Rule peer2Manifest[manifestSize];
    { //Rule 1: Object Path: *, Interface: *, Member Name: *: Action: DENY
        peer2Manifest[0].SetObjPath("*");
        peer2Manifest[0].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].Set("*",
                       PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                       0 /*DENY*/);
        peer2Manifest[0].SetMembers(1, members);
    }
    { //Rule 2: Object Path: *, Interface: *, Member Name: NS; Action: PROVIDE|OBSERVE|MODIFY
        peer2Manifest[1].SetObjPath("*");
        peer2Manifest[1].SetInterfaceName("*");
        PermissionPolicy::Rule::Member members[1];
        members[0].SetMemberName("*");
        members[0].SetMemberType(PermissionPolicy::Rule::Member::NOT_SPECIFIED);
        members[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                                 PermissionPolicy::Rule::Member::ACTION_MODIFY |
                                 PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        peer2Manifest[1].SetMembers(1, members);
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
                                                                  "Peer1Alias",
                                                                  3600,
                                                                  identityCertChainPeer2[0],
                                                                  peer2Digest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdateIdentity(identityCertChainPeer2, certChainSize, peer2Manifest, manifestSize))
        << "Failed to update Identity cert or manifest ";

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    // Verify Method call
    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    // Verify Set/Get Property
    MsgArg prop1Arg;
    EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
    EXPECT_EQ(ER_OK, proxy.SetProperty(interfaceName, "Prop1", prop1Arg));
    EXPECT_EQ(513, peer2BusObject.ReadProp1());

    MsgArg prop1ArgOut;
    EXPECT_EQ(ER_OK, proxy.GetProperty(interfaceName, "Prop1", prop1Arg));
    uint32_t prop1;
    prop1Arg.Get("i", &prop1);
    EXPECT_EQ((uint32_t)513, prop1);

    // Send/Receive Signal
    ChirpSignalReceiver chirpSignalReceiver;
    EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver, static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler), peer1Bus.GetInterface(interfaceName)->GetMember("Chirp"), NULL));

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
    EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);

    /* clean up */
    peer1Bus.UnregisterBusObject(peer1BusObject);
    peer2Bus.UnregisterBusObject(peer2BusObject);
}

/**
 * Purpose:
 * Method call is successful between two peers both of whom have a local policy
 * with WITH_PUBLIC_KEY peer type. The public key for this peer type is the
 * public key of each of the peers.
 *
 * Setup:
 * Peer 1 has a local policy WITH_PUBLIC_KEY Peer Type with public key of Peer 2.
 * Peer 2 has a local policy WITH_PUBLIC_KEY Peer Type with public key of Peer 1.
 * Both peers implement rules that allows method call.
 * The secure session is ECDHE_ECDSA based.
 * A makes a method call to B.
 *
 * verification:
 * Peers authentication are sucecssful.
 * Method call is successful.
 */
TEST_F(SecurityPolicyRulesTest, acl_with_public_key_method_call_should_pass)
{

    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            // Peer type: WITH_PUBLICKEY, Public Key of Peer2
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            KeyInfoNISTP256 peer2Key;
            PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));
            peers[0].SetKeyInfo(&peer2Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            // Peer type: WITH_PUBLICKEY, Public key of Peer1
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            //Get manager key
            KeyInfoNISTP256 peer1Key;
            PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));
            peers[0].SetKeyInfo(&peer1Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.SecureConnection(true));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());
    qcc::String p2policyStr = "\n----Peer2 Policy-----\n" + peer2Policy.ToString();
    SCOPED_TRACE(p2policyStr.c_str());

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));
    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}


/**
 * Purpose:
 * Method call is not successful if the public key in the policy of the receiver
 * does not match with that of the remote peer (sender).
 *
 * Setup:
 * Peer 1 has a local policy with WITH_PUBLIC_KEY Peer Type with public key of Peer 2.
 * Peer 2 has a local policy with WITH_PUBLIC_KEY Peer Type with a public key NOT of Peer 1 (managerBus).
 * Both peers implement rules that allows method call.
 * The secure session is ECDHE_ECDSA based.
 * A makes a method call to B.
 *
 * verification:
 * Peers authentication are sucecssful.
 * Method call is sent but cannot be received.
 */
TEST_F(SecurityPolicyRulesTest, acl_with_public_key_recieving_peer_has_incorrect_public_key)
{
    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            // Peer type: WITH_PUBLICKEY, Public Key of Peer2
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            KeyInfoNISTP256 peer2Key;
            PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));
            peers[0].SetKeyInfo(&peer2Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            // Peer type: WITH_PUBLICKEY, Public key of managerBus
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            //Get manager key
            KeyInfoNISTP256 managerKey;
            PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));
            peers[0].SetKeyInfo(&managerKey);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.SecureConnection(true));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());
    qcc::String p2policyStr = "\n----Peer2 Policy-----\n" + peer2Policy.ToString();
    SCOPED_TRACE(p2policyStr.c_str());

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}


/**
 * Purpose:
 * Method call is not successful if the public key in the policy of the sender
 * does not match with that of the remote peer (receiver).
 *
 * Setup:
 * Peer 1 has a local policy with WITH_PUBLIC_KEY Peer Type with a public key NOT of Peer2 (managerBus).
 * Peer 2 has a local policy with WITH_PUBLIC_KEY Peer Type with public key of Peer 1.
 * Both peers implement rules that allows method call.
 * The secure session is ECDHE_ECDSA based.
 * A makes a method call to B.
 *
 * verification:
 * Peers authentication are successful.
 * Method call cannot be sent.
 */
TEST_F(SecurityPolicyRulesTest, acl_with_public_key_sending_peer_has_incorrect_public_key)
{

    PolicyRulesTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    /* install permissions make method calls */
    //Permission policy that will be installed on peer1
    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            // Peer type: WITH_PUBLICKEY, Public Key of managerBus
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            KeyInfoNISTP256 managerKey;
            PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));
            peers[0].SetKeyInfo(&managerKey);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("/test");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer1Policy.SetAcls(1, acls);
    }

    // Permission policy that will be installed on peer2
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[1];
        {
            // Peer type: WITH_PUBLICKEY, Public key of Peer1
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            //Get manager key
            KeyInfoNISTP256 peer1Key;
            PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));
            peers[0].SetKeyInfo(&peer1Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("Echo",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }

    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    {
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, peer1Policy);
    }
    {
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, peer2Policy);
    }

    EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));
    EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
    EXPECT_EQ(ER_OK, sapWithPeer2.SecureConnection(true));

    SessionOpts opts;
    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));
    qcc::String p1policyStr = "\n----Peer1 Policy-----\n" + peer1Policy.ToString();
    SCOPED_TRACE(p1policyStr.c_str());
    qcc::String p2policyStr = "\n----Peer2 Policy-----\n" + peer2Policy.ToString();
    SCOPED_TRACE(p2policyStr.c_str());

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;

    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "Echo", &arg, static_cast<size_t>(1), replyMsg));

    /* clean up */
    peer2Bus.UnregisterBusObject(peer2BusObject);
}
