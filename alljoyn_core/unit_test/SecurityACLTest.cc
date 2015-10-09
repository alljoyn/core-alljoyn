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

class AclTestSessionPortListener : public SessionPortListener {
  public:
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
};


static void CreatePermissivePolicy(PermissionPolicy& policy, uint32_t version) {
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

static QStatus UpdatePolicyWithValuesFromDefaultPolicy(const PermissionPolicy& defaultPolicy,
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

class ACLTestBusObject : public BusObject {
  public:
    ACLTestBusObject(BusAttachment& bus, const char* path, const char* interfaceName, bool announce = true)
        : BusObject(path), isAnnounced(announce) {
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
            { iface->GetMember("ping"), static_cast<MessageReceiver::MethodHandler>(&ACLTestBusObject::Ping) },
            { iface->GetMember("bing"), static_cast<MessageReceiver::MethodHandler>(&ACLTestBusObject::Bing) },
            { iface->GetMember("king"), static_cast<MessageReceiver::MethodHandler>(&ACLTestBusObject::King) },
            { iface->GetMember("sing"), static_cast<MessageReceiver::MethodHandler>(&ACLTestBusObject::Sing) }
        };
        EXPECT_EQ(ER_OK, AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0])));
    }

    void Ping(const InterfaceDescription::Member* member, Message& msg) {
        QCC_UNUSED(member);
        QStatus status = MethodReply(msg);
        EXPECT_EQ(ER_OK, status) << "Error sending reply";
    }

    void Bing(const InterfaceDescription::Member* member, Message& msg) {
        QCC_UNUSED(member);
        QStatus status = MethodReply(msg);
        EXPECT_EQ(ER_OK, status) << "Error sending reply";
    }

    void King(const InterfaceDescription::Member* member, Message& msg) {
        QCC_UNUSED(member);
        QStatus status = MethodReply(msg);
        EXPECT_EQ(ER_OK, status) << "Error sending reply";
    }

    void Sing(const InterfaceDescription::Member* member, Message& msg) {
        QCC_UNUSED(member);
        QStatus status = MethodReply(msg);
        EXPECT_EQ(ER_OK, status) << "Error sending reply";
    }

  private:
    bool isAnnounced;
};


/*
 * Purpose:
 * Test with multiple ACLs and different peer types.
 *
 * Setup:
 * peer3  is claimed by ASGA
 * peer3's IC is signed by the CA.
 *
 * Peer1 has a Membership certificate issued by Living room SGA.
 * Peer2 has the public key 2.
 *
 * peer3 has the following ACLs.
 * peer3 implements four methods: ping, bing, king, sing
 * ACL1:  Peer type: ALL; MODIFY for method call ping
 * ACL2:  Peer type: ANY_TRUSTED;  MODIFY for method call bing
 * ACL3:  Peer type: WITH_PUBLIC_KEY, public key B; MODIFY for method call king
 * ACL4: Peer type:  WITH_MEMBERSHIP, SGID: Living room, authority: Living room SGA; MODIFY for method call sing
 *
 * Peer1 and peer3 have ECDSA, NULL auth mechanisms supported. Peer1's IC is signed by CA'.
 * Peer1 makes a method call ping. It should be successful. Auth mechanism should be ECDHE_NULL.
 * Peer1 makes a method call bing. Ensure that it cannot be received by app. bus
 * Peer1 makes a method call king. Ensure that it cannot be received by app. bus
 * Peer1 makes a method call sing. Ensure that it cannot be received by app. bus
 *
 * Peer1 and peer3 have ECDSA, NULL auth mechanisms supported. Peer1's IC is signed by CA.
 * Peer1 makes a method call ping. It should be successful. Auth mechanism should be ECDHE_ECDSA.
 * Peer1 makes a method call bing. It should be successful. Auth mechanism should be ECDHE_ECDSA.
 * Peer1 makes a method call king. Ensure that it cannot be received by peer3
 * Peer1 makes a method call sing.  It should be successful. Auth mechanism should be ECDHE_ECDSA.
 *
 * Peer1 and peer3 have ECDSA, NULL auth mechanisms supported. Peer A's IC is signed by ASGA.
 * Peer1 makes a method call ping. It should be successful. Auth mechanism should be ECDHE_ECDSA.
 * Peer1 makes a method call bing. It should be successful. Auth mechanism should be ECDHE_ECDSA.
 * Peer1 makes a method call king. Ensure that it cannot be received by peer3
 * Peer1 makes a method call sing. It should be successful. Auth mechanism should be ECDHE_ECDSA.
 *
 * Peer1 and peer3 have ECDSA, NULL auth mechanisms supported. Peer1's IC is signed by Living room SGID
 * Peer1 makes a method call ping. It should be successful. Auth mechanism should be ECDHE_ECDSA.
 * Peer1 makes a method call bing It should be successful. Auth mechanism should be ECDHE_ECDSA.
 * Peer1 makes a method call king. Ensure that it cannot be received by app. bus
 * Peer1 makes a method call sing. It should be successful. Auth mechanism should be ECDHE_ECDSA.
 *
 * Peer2 and peer3 have ECDSA, NULL auth mechanisms supported. Peer2's IC is signed by Living room SGID
 * Peer2 makes a method call ping. It should be successful. Auth mechanism should be ECDHE_ECDSA.
 * Peer2 makes a method call bing It should be successful. Auth mechanism should be ECDHE_ECDSA.
 * Peer2 makes a method call king. It should be successful. Auth mechanism should be ECDHE_ECDSA.
 * Peer2 makes a method call sing. Ensure that it cannot be received by peer3.
 *
 * Verification:
 * Verification mentioned in the set up.
 *
 */
TEST(SecurityACLTest, multiple_acls_and_different_peer_types) {
    BusAttachment managerBus("SecurityACLManager");
    BusAttachment peer1Bus("SecurityACLPeer1");
    BusAttachment peer2Bus("SecurityACLPeer2");
    BusAttachment peer3Bus("SecurityACLPeer3");

    BusAttachment busUsedAsCA("busUsedAsCA");
    BusAttachment busUsedAsLivingRoom("busUsedAsLivingRoom");

    EXPECT_EQ(ER_OK, managerBus.Start());
    EXPECT_EQ(ER_OK, managerBus.Connect());
    EXPECT_EQ(ER_OK, peer1Bus.Start());
    EXPECT_EQ(ER_OK, peer1Bus.Connect());
    EXPECT_EQ(ER_OK, peer2Bus.Start());
    EXPECT_EQ(ER_OK, peer2Bus.Connect());
    EXPECT_EQ(ER_OK, peer3Bus.Start());
    EXPECT_EQ(ER_OK, peer3Bus.Connect());

    EXPECT_EQ(ER_OK, busUsedAsCA.Start());
    EXPECT_EQ(ER_OK, busUsedAsCA.Connect());
    EXPECT_EQ(ER_OK, busUsedAsLivingRoom.Start());
    EXPECT_EQ(ER_OK, busUsedAsLivingRoom.Connect());

    InMemoryKeyStoreListener managerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;
    InMemoryKeyStoreListener peer3KeyStoreListener;
    InMemoryKeyStoreListener caKeyStoreListener;
    InMemoryKeyStoreListener livingRoomKeyStoreListener;

    EXPECT_EQ(ER_OK, managerBus.RegisterKeyStoreListener(managerKeyStoreListener));
    EXPECT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
    EXPECT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));
    EXPECT_EQ(ER_OK, peer3Bus.RegisterKeyStoreListener(peer3KeyStoreListener));
    EXPECT_EQ(ER_OK, busUsedAsCA.RegisterKeyStoreListener(caKeyStoreListener));
    EXPECT_EQ(ER_OK, busUsedAsLivingRoom.RegisterKeyStoreListener(livingRoomKeyStoreListener));

    DefaultECDHEAuthListener managerAuthListener;
    DefaultECDHEAuthListener peer1AuthListener;
    DefaultECDHEAuthListener peer2AuthListener;
    DefaultECDHEAuthListener peer3AuthListener;
    DefaultECDHEAuthListener caAuthListener;
    DefaultECDHEAuthListener livingRoomAuthListener;

    EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
    EXPECT_EQ(ER_OK, peer3Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer3AuthListener));

    EXPECT_EQ(ER_OK, busUsedAsCA.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &caAuthListener));
    EXPECT_EQ(ER_OK, busUsedAsLivingRoom.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &livingRoomAuthListener));


    const char* interfaceName = "org.allseen.test.SecurityApplication.acl";
    String interface = "<node>"
                       "<interface name='" + String(interfaceName) + "'>"
                       "<annotation name='org.alljoyn.Bus.Secure' value='true'/>"
                       "  <method name='ping'>"
                       "  </method>"
                       "  <method name='bing'>"
                       "  </method>"
                       "  <method name='king'>"
                       "  </method>"
                       "  <method name='sing'>"
                       "  </method>"
                       "</interface>"
                       "</node>";

    EXPECT_EQ(ER_OK, peer1Bus.CreateInterfacesFromXml(interface.c_str()));
    EXPECT_EQ(ER_OK, peer2Bus.CreateInterfacesFromXml(interface.c_str()));
    EXPECT_EQ(ER_OK, peer3Bus.CreateInterfacesFromXml(interface.c_str()));

    ACLTestBusObject peer3BusObject(peer3Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer3Bus.RegisterBusObject(peer3BusObject));

    SessionPort managerSessionPort(42);
    SessionPort peer1SessionPort(42);
    SessionPort peer2SessionPort(42);
    SessionPort peer3SessionPort(42);

    SessionOpts opts;

    AclTestSessionPortListener managerSessionPortListener;
    AclTestSessionPortListener peer1SessionPortListener;
    AclTestSessionPortListener peer2SessionPortListener;
    AclTestSessionPortListener peer3SessionPortListener;

    EXPECT_EQ(ER_OK, managerBus.BindSessionPort(managerSessionPort, opts, managerSessionPortListener));
    EXPECT_EQ(ER_OK, peer1Bus.BindSessionPort(peer1SessionPort, opts, peer1SessionPortListener));
    EXPECT_EQ(ER_OK, peer2Bus.BindSessionPort(peer2SessionPort, opts, peer2SessionPortListener));
    EXPECT_EQ(ER_OK, peer3Bus.BindSessionPort(peer3SessionPort, opts, peer3SessionPortListener));

    SessionId managerToManagerSessionId;
    SessionId managerToPeer1SessionId;
    SessionId managerToPeer2SessionId;
    SessionId managerToPeer3SessionId;

    EXPECT_EQ(ER_OK, managerBus.JoinSession(managerBus.GetUniqueName().c_str(), managerSessionPort, NULL, managerToManagerSessionId, opts));
    EXPECT_EQ(ER_OK, managerBus.JoinSession(peer1Bus.GetUniqueName().c_str(), peer1SessionPort, NULL, managerToPeer1SessionId, opts));
    EXPECT_EQ(ER_OK, managerBus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, managerToPeer2SessionId, opts));
    EXPECT_EQ(ER_OK, managerBus.JoinSession(peer3Bus.GetUniqueName().c_str(), peer3SessionPort, NULL, managerToPeer3SessionId, opts));

    SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);
    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);
    SecurityApplicationProxy sapWithPeer3(managerBus, peer3Bus.GetUniqueName().c_str(), managerToPeer3SessionId);

    //Get public keys
    KeyInfoNISTP256 managerKey;
    PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    KeyInfoNISTP256 peer2Key;
    PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));

    KeyInfoNISTP256 peer3Key;
    PermissionConfigurator& pcPeer3 = peer3Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer3.GetSigningPublicKey(peer3Key));

    KeyInfoNISTP256 caKey;
    PermissionConfigurator& pcCA = busUsedAsCA.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcCA.GetSigningPublicKey(caKey));

    KeyInfoNISTP256 livingRoomKey;
    PermissionConfigurator& pcLivingRoom = busUsedAsLivingRoom.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcLivingRoom.GetSigningPublicKey(livingRoomKey));

    // All Inclusive manifest
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetObjPath("*");
    manifest[0].SetInterfaceName("*");
    {
        PermissionPolicy::Rule::Member member[1];
        member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                      PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                      PermissionPolicy::Rule::Member::ACTION_MODIFY |
                      PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        manifest[0].SetMembers(1, member);
    }

    uint8_t managerDigest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(managerBus,
                                                               manifest, manifestSize,
                                                               managerDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    uint8_t caDigest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(busUsedAsCA,
                                                               manifest, manifestSize,
                                                               caDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    uint8_t livingRoomDigest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(busUsedAsLivingRoom,
                                                               manifest, manifestSize,
                                                               livingRoomDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    GUID128 managerGuid;
    PermissionMgmtTestHelper::GetGUID(managerBus, managerGuid);
    GUID128 peer1Guid;
    PermissionMgmtTestHelper::GetGUID(peer1Bus, peer1Guid);
    GUID128 peer2Guid;
    PermissionMgmtTestHelper::GetGUID(peer2Bus, peer2Guid);
    GUID128 peer3Guid;
    PermissionMgmtTestHelper::GetGUID(peer3Bus, peer3Guid);
    GUID128 caGuid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsCA, caGuid);
    GUID128 livingRoomGuid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsLivingRoom, livingRoomGuid);

    //Create identityCerts
    const size_t certChainSize = 1;
    IdentityCertificate identityCertChainMaster[certChainSize];
    IdentityCertificate identityCertChainPeer1[certChainSize];
    IdentityCertificate identityCertChainPeer2[certChainSize];
    IdentityCertificate identityCertChainPeer3[certChainSize];

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  managerKey.GetPublicKey(),
                                                                  "ManagerAlias",
                                                                  3600,
                                                                  identityCertChainMaster[0],
                                                                  caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                  "0",
                                                                  peer1Guid.ToString(),
                                                                  peer1Key.GetPublicKey(),
                                                                  "Peer1Alias",
                                                                  3600,
                                                                  identityCertChainPeer1[0],
                                                                  caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                  "0",
                                                                  peer2Guid.ToString(),
                                                                  peer2Key.GetPublicKey(),
                                                                  "Peer2Alias",
                                                                  3600,
                                                                  identityCertChainPeer2[0],
                                                                  caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                  "0",
                                                                  peer3Guid.ToString(),
                                                                  peer3Key.GetPublicKey(),
                                                                  "Peer3Alias",
                                                                  3600,
                                                                  identityCertChainPeer3[0],
                                                                  caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    managerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    EXPECT_EQ(ER_OK, sapWithManager.Claim(caKey,
                                          managerGuid,
                                          managerKey,
                                          identityCertChainMaster, certChainSize,
                                          manifest, manifestSize));

    peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(caKey,
                                        managerGuid,
                                        managerKey,
                                        identityCertChainPeer1, certChainSize,
                                        manifest, manifestSize));

    peer2Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    EXPECT_EQ(ER_OK, sapWithPeer2.Claim(caKey,
                                        managerGuid,
                                        managerKey,
                                        identityCertChainPeer2, certChainSize,
                                        manifest, manifestSize));

    peer3Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    EXPECT_EQ(ER_OK, sapWithPeer3.Claim(caKey,
                                        managerGuid,
                                        managerKey,
                                        identityCertChainPeer3, certChainSize,
                                        manifest, manifestSize));

    EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
    EXPECT_EQ(ER_OK, peer3Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer3AuthListener));

    qcc::String membershipSerial = "1";
    qcc::MembershipCertificate managerMembershipCertificate[2];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-1",
                                                                    busUsedAsCA,
                                                                    managerGuid.ToString(),
                                                                    managerKey.GetPublicKey(),
                                                                    managerGuid,
                                                                    true,
                                                                    3600,
                                                                    managerMembershipCertificate[1]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-0",
                                                                    managerBus,
                                                                    managerGuid.ToString(),
                                                                    managerKey.GetPublicKey(),
                                                                    managerGuid,
                                                                    false,
                                                                    3600,
                                                                    managerMembershipCertificate[0]
                                                                    ));
    EXPECT_EQ(ER_OK, sapWithManager.InstallMembership(managerMembershipCertificate, 2));

    /**
     * Need to install a membership cert chain with busUsedAsCA in the mix
     * in order for the membership to exchanged to the other peer.
     * Make sure the subject name used in the guid in order to pass the issuer
     * DN check.
     * The leaf is signed by busUsedAsLivingRoomCA.
     * The root is signed by busUsedAsCA.
     */

    qcc::MembershipCertificate peer1MembershipCertificate[2];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("2-1",
                                                                    busUsedAsCA,
                                                                    livingRoomGuid.ToString(),
                                                                    livingRoomKey.GetPublicKey(),
                                                                    livingRoomGuid,
                                                                    true,
                                                                    3600,
                                                                    peer1MembershipCertificate[1]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("2-0",
                                                                    busUsedAsLivingRoom,
                                                                    peer1Guid.ToString(),
                                                                    peer1Key.GetPublicKey(),
                                                                    livingRoomGuid,
                                                                    false,
                                                                    3600,
                                                                    peer1MembershipCertificate[0]
                                                                    ));
    EXPECT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1MembershipCertificate, 2));

    qcc::MembershipCertificate peer2MembershipCertificate[1];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                    busUsedAsCA,
                                                                    peer2Bus.GetUniqueName(),
                                                                    peer2Key.GetPublicKey(),
                                                                    managerGuid,
                                                                    false,
                                                                    3600,
                                                                    peer2MembershipCertificate[0]
                                                                    ));
    EXPECT_EQ(ER_OK, sapWithPeer2.InstallMembership(peer2MembershipCertificate, 1));

    qcc::MembershipCertificate peer3MembershipCertificate[1];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert(membershipSerial,
                                                                    busUsedAsCA,
                                                                    peer3Bus.GetUniqueName(),
                                                                    peer3Key.GetPublicKey(),
                                                                    managerGuid,
                                                                    false,
                                                                    3600,
                                                                    peer3MembershipCertificate[0]
                                                                    ));
    EXPECT_EQ(ER_OK, sapWithPeer3.InstallMembership(peer3MembershipCertificate, 1));

    //Install permissive policies on Peer1 and Peer2
    {
        PermissionPolicy peer1Policy;
        CreatePermissivePolicy(peer1Policy, 1);
        PermissionPolicy defaultPolicy;
        sapWithPeer1.GetDefaultPolicy(defaultPolicy);
        UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer1Policy, true, true, true);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
        EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));
    }
    {
        PermissionPolicy peer2Policy;
        CreatePermissivePolicy(peer2Policy, 1);
        PermissionPolicy defaultPolicy;
        sapWithPeer2.GetDefaultPolicy(defaultPolicy);
        UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer2Policy, true, true, true);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
        EXPECT_EQ(ER_OK, sapWithPeer2.SecureConnection(true));
    }

    //Permission policy that will be installed on peer3
    // This is the Peers policy we are testing the others are permissive this is
    // restrictive.
    PermissionPolicy peer3Policy;
    peer3Policy.SetVersion(1);
    {
        PermissionPolicy::Acl acls[4];
        //ACL0:  Peer type: ALL; MODIFY for method call ping
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ALL);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[2];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("ping",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            rules[1].SetObjPath("*");
            rules[1].SetInterfaceName(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY |
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE |
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[1].SetMembers(1, members);
            }
            acls[0].SetRules(2, rules);
        }
        //ACL1:  Peer type: ANY_TRUSTED;  MODIFY for method call bing
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
            acls[1].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("bing",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[1].SetRules(1, rules);
        }
        //ACL2: Peer type: WITH_PUBLIC_KEY, public keyPeer2; MODIFY for method call king
        {
            // Peer type: WITH_PUBLICKEY, Public Key of Peer2
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
            KeyInfoNISTP256 peer2Key;
            PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
            EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));
            peers[0].SetKeyInfo(&peer2Key);
            acls[2].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("king",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[2].SetRules(1, rules);
        }
        //ACL3: Peer type:  WITH_MEMBERSHIP, SGID: Living room, authority: Living room SGA; MODIFY for method call sing
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(livingRoomGuid);
            peers[0].SetKeyInfo(&livingRoomKey);
            acls[3].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("sing",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[3].SetRules(1, rules);
        }
        peer3Policy.SetAcls(4, acls);
    }
    {
        PermissionPolicy defaultPolicy;
        sapWithPeer3.GetDefaultPolicy(defaultPolicy);
        UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer3Policy, true, true, true);
        EXPECT_EQ(ER_OK, sapWithPeer3.UpdatePolicy(peer3Policy));
        EXPECT_EQ(ER_OK, sapWithPeer3.SecureConnection(true));
    }


    SessionId peer1ToPeer3SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer3Bus.GetUniqueName().c_str(), peer3SessionPort, NULL, peer1ToPeer3SessionId, opts));
    SessionId peer2ToPeer3SessionId;
    EXPECT_EQ(ER_OK, peer2Bus.JoinSession(peer3Bus.GetUniqueName().c_str(), peer3SessionPort, NULL, peer2ToPeer3SessionId, opts));

    /*
     * Peer1 and app. bus have ECDSA, NULL auth mechanisms supported. Peer1's IC is signed by CA'.
     * Peer1 makes a method call ping. It should be successful. Auth mechanism should be ECDHE_NULL.
     * Peer1 makes a method call bing. Ensure that it cannot be received by peer3
     * Peer1 makes a method call king. Ensure that it cannot be received by peer3
     * Peer1 makes a method call sing. Ensure that it cannot be received by peer3
     */
    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer3Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &peer3AuthListener));

        /* Create the ProxyBusObject and call the Echo method on the interface */
        ProxyBusObject proxy(peer1Bus, peer3Bus.GetUniqueName().c_str(), "/test", peer1ToPeer3SessionId, true);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "ping", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "bing", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "king", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "sing", NULL, static_cast<size_t>(0), replyMsg));
    }

    /*
     * Peer1 and app. bus have ECDSA, NULL auth mechanisms supported. Peer1's IC is signed by CA.
     * Peer1 makes a method call ping. It should be successful. Auth mechanism should be ECDHE_ECDSA.
     * Peer1 makes a method call bing. It should be successful. Auth mechanism should be ECDHE_ECDSA.
     * Peer1 makes a method call king. Ensure that it cannot be received by peer3
     * Peer1 makes a method call sing.  It should be successful. Auth mechanism should be ECDHE_ECDSA.
     */
    {
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer3Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer3AuthListener));

        /* Create the ProxyBusObject and call the Echo method on the interface */
        ProxyBusObject proxy(peer1Bus, peer3Bus.GetUniqueName().c_str(), "/test", peer1ToPeer3SessionId, true);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "ping", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "bing", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "king", NULL, static_cast<size_t>(0), replyMsg));

        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "sing", NULL, static_cast<size_t>(0), replyMsg));
    }

    /*
     * Peer1 and peer3 have ECDSA, NULL auth mechanisms supported. Peer1's IC is signed by ASGA.
     * Peer1 makes a method call ping. It should be successful. Auth mechanism should be ECDHE_ECDSA.
     * Peer1 makes a method call bing. It should be successful. Auth mechanism should be ECDHE_ECDSA.
     * Peer1 makes a method call king. Ensure that it cannot be received by peer3
     * Peer1 makes a method call sing. It should be successful. Auth mechanism should be ECDHE_ECDSA.
     */
    {
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(managerBus,
                                                                      "4",
                                                                      peer1Guid.ToString(),
                                                                      peer1Key.GetPublicKey(),
                                                                      "Peer1Alias",
                                                                      3600,
                                                                      identityCertChainPeer1[0],
                                                                      managerDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";
        /* peer1 was set to use ECDHE_NULL in the previous test, so now it
           needs to be enabled with ECDSA */
        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChainPeer1, 1, manifest, manifestSize));

        EXPECT_EQ(ER_OK, peer3Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer3AuthListener));

        /* Create the ProxyBusObject and call the Echo method on the interface */
        ProxyBusObject proxy(peer1Bus, peer3Bus.GetUniqueName().c_str(), "/test", peer1ToPeer3SessionId, true);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "ping", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "bing", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "king", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "sing", NULL, static_cast<size_t>(0), replyMsg));
    }

    /*
     * Peer1 and app. bus have ECDSA, NULL auth mechanisms supported. Peer1's IC is signed by Living room SGID
     * Peer1 makes a method call ping. It should be successful. Auth mechanism should be ECDHE_ECDSA.
     * Peer1 makes a method call bing It should be successful. Auth mechanism should be ECDHE_ECDSA.
     * Peer1 makes a method call king. Ensure that it cannot be received by peer3
     * Peer1 makes a method call sing. It should be successful. Auth mechanism should be ECDHE_ECDSA.
     */
    {
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsLivingRoom,
                                                                      "5",
                                                                      peer1Guid.ToString(),
                                                                      peer1Key.GetPublicKey(),
                                                                      "Peer1Alias",
                                                                      3600,
                                                                      identityCertChainPeer1[0],
                                                                      livingRoomDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

        /* reestablish connection */
        EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdateIdentity(identityCertChainPeer1, 1, manifest, manifestSize));

        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer3Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer3AuthListener));

        /* Create the ProxyBusObject and call the Echo method on the interface */
        ProxyBusObject proxy(peer1Bus, peer3Bus.GetUniqueName().c_str(), "/test", peer1ToPeer3SessionId, true);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "ping", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "bing", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "king", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "sing", NULL, static_cast<size_t>(0), replyMsg));
    }

    /*
     * Peer2 and app. bus have ECDSA, NULL auth mechanisms supported. Peer2's IC is signed by Living room SGID
     * Peer2 makes a method call ping. It should be successful. Auth mechanism should be ECDHE_ECDSA.
     * Peer2 makes a method call bing It should be successful. Auth mechanism should be ECDHE_ECDSA.
     * Peer2 makes a method call king. It should be successful. Auth mechanism should be ECDHE_ECDSA.
     * Peer2 makes a method call sing. Ensure that it cannot be received by peer2.
     */
    {
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsLivingRoom,
                                                                      "6",
                                                                      peer2Guid.ToString(),
                                                                      peer2Key.GetPublicKey(),
                                                                      "Peer2Alias",
                                                                      3600,
                                                                      identityCertChainPeer2[0],
                                                                      livingRoomDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdateIdentity(identityCertChainPeer2, 1, manifest, manifestSize));

        EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        EXPECT_EQ(ER_OK, peer3Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer3AuthListener));

        /* Create the ProxyBusObject and call the Echo method on the interface */
        ProxyBusObject proxy(peer2Bus, peer3Bus.GetUniqueName().c_str(), "/test", peer2ToPeer3SessionId, true);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
        EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

        Message replyMsg(peer2Bus);
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "ping", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "bing", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "king", NULL, static_cast<size_t>(0), replyMsg));
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall(interfaceName, "sing", NULL, static_cast<size_t>(0), replyMsg));
    }
}

/**
 * Purpose:
 * Verify that in case of multiple membership certificates signed by different entities,
 * the message is still authorized.
 *
 * Setup:
 * Peer2 is claimed by ASGA
 * Peer2's IC is signed by CA
 *
 * Peer1 Bus is claimed by ASGA
 * Peer1's IC is signed by CA
 *
 * Peer2 has the following ACLs.
 * WITH_MEMBERSHIP: alpha SGID; rule is MODIFY on ping
 * WITH_MEMBERSHIP: beta SGID; rule is MODIFY on king
 * WITH_MEMBERSHIP: gamma SGID; rule is MODIFY on sing
 *
 * Peer1 has the following ACLs.
 * WITH_MEMBERSHIP: phi SGID; rule is to PROVIDE on ping
 * WITH_MEMBERSHIP: si SGID; rule is to PROVIDE on king
 * WITH_MEMBERSHIP: omega SGID; rule is to PROVIDE on sing
 * WITH_MEMBERSHIP: tau SGID; rule is to DENY all
 *
 * Both buses implement methods: ping, king, sing
 *
 * List of Membership certs installed on Peer1 bus:
 * alpha->Intermediate cert1->Peer1
 * beta->Intermediate cert1->Peer1
 * gamma->Peer1

 * List of Membership certs installed on Peer2:
 * phi->Intermediate cert2->Peer2
 * omega->Intermediate cert2->Peer2
 * psi->Peer2
 *
 * Peer1 calls method ping on Peer2.
 * Peer1 calls method king on Peer2.
 * Peer1 calls method sing on Peer2.
 *
 * verification:
 * All method calls should be successful
 */
TEST(SecurityACLTest, acl_verify_peers_using_different_membershipchain_can_make_successful_method_calls) {
    BusAttachment managerBus("SecurityACLManager");
    BusAttachment peer1Bus("SecurityACLPeer1");
    BusAttachment peer2Bus("SecurityACLPeer2");

    BusAttachment busUsedAsCA("busUsedAsCA");

    BusAttachment busUsedAsAlpha("busUsedAsAlpha");
    BusAttachment busUsedAsBeta("busUsedAsBeta");
    BusAttachment busUsedAsGamma("busUsedAsGamma");

    BusAttachment busUsedAsPhi("busUsedAsPhi");
    BusAttachment busUsedAsSi("busUsedAsSi");
    BusAttachment busUsedAsOmega("busUsedAsOmega");
    BusAttachment busUsedAsTau("busUsedAsTau");

    BusAttachment busUsedAsInt1("busUsedAsInt1");
    BusAttachment busUsedAsInt2("busUsedAsInt2");


    EXPECT_EQ(ER_OK, managerBus.Start());
    EXPECT_EQ(ER_OK, managerBus.Connect());
    EXPECT_EQ(ER_OK, peer1Bus.Start());
    EXPECT_EQ(ER_OK, peer1Bus.Connect());
    EXPECT_EQ(ER_OK, peer2Bus.Start());
    EXPECT_EQ(ER_OK, peer2Bus.Connect());

    EXPECT_EQ(ER_OK, busUsedAsCA.Start());
    EXPECT_EQ(ER_OK, busUsedAsCA.Connect());

    EXPECT_EQ(ER_OK, busUsedAsAlpha.Start());
    EXPECT_EQ(ER_OK, busUsedAsAlpha.Connect());
    EXPECT_EQ(ER_OK, busUsedAsBeta.Start());
    EXPECT_EQ(ER_OK, busUsedAsBeta.Connect());
    EXPECT_EQ(ER_OK, busUsedAsGamma.Start());
    EXPECT_EQ(ER_OK, busUsedAsGamma.Connect());

    EXPECT_EQ(ER_OK, busUsedAsPhi.Start());
    EXPECT_EQ(ER_OK, busUsedAsPhi.Connect());
    EXPECT_EQ(ER_OK, busUsedAsSi.Start());
    EXPECT_EQ(ER_OK, busUsedAsSi.Connect());
    EXPECT_EQ(ER_OK, busUsedAsOmega.Start());
    EXPECT_EQ(ER_OK, busUsedAsOmega.Connect());
    EXPECT_EQ(ER_OK, busUsedAsTau.Start());
    EXPECT_EQ(ER_OK, busUsedAsTau.Connect());

    EXPECT_EQ(ER_OK, busUsedAsInt1.Start());
    EXPECT_EQ(ER_OK, busUsedAsInt1.Connect());

    EXPECT_EQ(ER_OK, busUsedAsInt2.Start());
    EXPECT_EQ(ER_OK, busUsedAsInt2.Connect());



    InMemoryKeyStoreListener managerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;
    InMemoryKeyStoreListener caKeyStoreListener;

    InMemoryKeyStoreListener alphaKeyStoreListener;
    InMemoryKeyStoreListener betaKeyStoreListener;
    InMemoryKeyStoreListener gammaKeyStoreListener;

    InMemoryKeyStoreListener phiKeyStoreListener;
    InMemoryKeyStoreListener siKeyStoreListener;
    InMemoryKeyStoreListener omegaKeyStoreListener;
    InMemoryKeyStoreListener tauKeyStoreListener;

    InMemoryKeyStoreListener int1KeyStoreListener;
    InMemoryKeyStoreListener int2KeyStoreListener;


    EXPECT_EQ(ER_OK, managerBus.RegisterKeyStoreListener(managerKeyStoreListener));
    EXPECT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
    EXPECT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));
    EXPECT_EQ(ER_OK, busUsedAsCA.RegisterKeyStoreListener(caKeyStoreListener));

    EXPECT_EQ(ER_OK, busUsedAsAlpha.RegisterKeyStoreListener(alphaKeyStoreListener));
    EXPECT_EQ(ER_OK, busUsedAsBeta.RegisterKeyStoreListener(betaKeyStoreListener));
    EXPECT_EQ(ER_OK, busUsedAsGamma.RegisterKeyStoreListener(gammaKeyStoreListener));

    EXPECT_EQ(ER_OK, busUsedAsPhi.RegisterKeyStoreListener(phiKeyStoreListener));
    EXPECT_EQ(ER_OK, busUsedAsSi.RegisterKeyStoreListener(siKeyStoreListener));
    EXPECT_EQ(ER_OK, busUsedAsOmega.RegisterKeyStoreListener(omegaKeyStoreListener));
    EXPECT_EQ(ER_OK, busUsedAsTau.RegisterKeyStoreListener(tauKeyStoreListener));

    EXPECT_EQ(ER_OK, busUsedAsInt1.RegisterKeyStoreListener(int1KeyStoreListener));
    EXPECT_EQ(ER_OK, busUsedAsInt2.RegisterKeyStoreListener(int2KeyStoreListener));


    DefaultECDHEAuthListener managerAuthListener;
    DefaultECDHEAuthListener peer1AuthListener;
    DefaultECDHEAuthListener peer2AuthListener;
    DefaultECDHEAuthListener caAuthListener;

    DefaultECDHEAuthListener alphaAuthListener;
    DefaultECDHEAuthListener betaAuthListener;
    DefaultECDHEAuthListener gammaAuthListener;

    DefaultECDHEAuthListener phiAuthListener;
    DefaultECDHEAuthListener siAuthListener;
    DefaultECDHEAuthListener omegaAuthListener;
    DefaultECDHEAuthListener tauAuthListener;

    DefaultECDHEAuthListener int1AuthListener;
    DefaultECDHEAuthListener int2AuthListener;


    EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));
    EXPECT_EQ(ER_OK, busUsedAsAlpha.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &alphaAuthListener));
    EXPECT_EQ(ER_OK, busUsedAsBeta.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &betaAuthListener));
    EXPECT_EQ(ER_OK, busUsedAsGamma.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &gammaAuthListener));

    EXPECT_EQ(ER_OK, busUsedAsPhi.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &phiAuthListener));
    EXPECT_EQ(ER_OK, busUsedAsSi.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &siAuthListener));
    EXPECT_EQ(ER_OK, busUsedAsOmega.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &omegaAuthListener));
    EXPECT_EQ(ER_OK, busUsedAsTau.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &tauAuthListener));

    EXPECT_EQ(ER_OK, busUsedAsCA.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &caAuthListener));

    EXPECT_EQ(ER_OK, busUsedAsInt1.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &int1AuthListener));
    EXPECT_EQ(ER_OK, busUsedAsInt2.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &int2AuthListener));


    const char* interfaceName = "org.allseen.test.SecurityApplication.acl";
    String interface = "<node>"
                       "<interface name='" + String(interfaceName) + "'>"
                       "<annotation name='org.alljoyn.Bus.Secure' value='true'/>"
                       "  <method name='ping'>"
                       "  </method>"
                       "  <method name='bing'>"
                       "  </method>"
                       "  <method name='king'>"
                       "  </method>"
                       "  <method name='sing'>"
                       "  </method>"
                       "</interface>"
                       "</node>";

    EXPECT_EQ(ER_OK, peer1Bus.CreateInterfacesFromXml(interface.c_str()));
    EXPECT_EQ(ER_OK, peer2Bus.CreateInterfacesFromXml(interface.c_str()));

    ACLTestBusObject peer2BusObject(peer2Bus, "/test", interfaceName);
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    SessionPort managerSessionPort(42);
    SessionPort peer1SessionPort(42);
    SessionPort peer2SessionPort(42);

    SessionOpts opts;

    AclTestSessionPortListener managerSessionPortListener;
    AclTestSessionPortListener peer1SessionPortListener;
    AclTestSessionPortListener peer2SessionPortListener;

    EXPECT_EQ(ER_OK, managerBus.BindSessionPort(managerSessionPort, opts, managerSessionPortListener));
    EXPECT_EQ(ER_OK, peer1Bus.BindSessionPort(peer1SessionPort, opts, peer1SessionPortListener));
    EXPECT_EQ(ER_OK, peer2Bus.BindSessionPort(peer2SessionPort, opts, peer2SessionPortListener));

    SessionId managerToManagerSessionId;
    SessionId managerToPeer1SessionId;
    SessionId managerToPeer2SessionId;

    EXPECT_EQ(ER_OK, managerBus.JoinSession(managerBus.GetUniqueName().c_str(), managerSessionPort, NULL, managerToManagerSessionId, opts));
    EXPECT_EQ(ER_OK, managerBus.JoinSession(peer1Bus.GetUniqueName().c_str(), peer1SessionPort, NULL, managerToPeer1SessionId, opts));
    EXPECT_EQ(ER_OK, managerBus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, managerToPeer2SessionId, opts));

    SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);
    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    //Get public keys
    KeyInfoNISTP256 managerKey;
    PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

    KeyInfoNISTP256 peer1Key;
    PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

    KeyInfoNISTP256 peer2Key;
    PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));

    KeyInfoNISTP256 caKey;
    PermissionConfigurator& pcCA = busUsedAsCA.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcCA.GetSigningPublicKey(caKey));

    KeyInfoNISTP256 alphaKey;
    PermissionConfigurator& pcAlpha = busUsedAsAlpha.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcAlpha.GetSigningPublicKey(alphaKey));

    KeyInfoNISTP256 betaKey;
    PermissionConfigurator& pcBeta = busUsedAsBeta.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcBeta.GetSigningPublicKey(betaKey));

    KeyInfoNISTP256 gammaKey;
    PermissionConfigurator& pcGamma = busUsedAsGamma.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcGamma.GetSigningPublicKey(gammaKey));

    KeyInfoNISTP256 phiKey;
    PermissionConfigurator& pcPhi = busUsedAsPhi.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcPhi.GetSigningPublicKey(phiKey));

    KeyInfoNISTP256 siKey;
    PermissionConfigurator& pcSi = busUsedAsSi.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcSi.GetSigningPublicKey(siKey));

    KeyInfoNISTP256 omegaKey;
    PermissionConfigurator& pcOmega = busUsedAsOmega.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcOmega.GetSigningPublicKey(omegaKey));

    KeyInfoNISTP256 tauKey;
    PermissionConfigurator& pcTau = busUsedAsTau.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcTau.GetSigningPublicKey(tauKey));

    KeyInfoNISTP256 int1Key;
    PermissionConfigurator& pcInt1 = busUsedAsInt1.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcInt1.GetSigningPublicKey(int1Key));

    KeyInfoNISTP256 int2Key;
    PermissionConfigurator& pcInt2 = busUsedAsInt2.GetPermissionConfigurator();
    EXPECT_EQ(ER_OK, pcInt2.GetSigningPublicKey(int2Key));


    // All Inclusive manifest
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetObjPath("*");
    manifest[0].SetInterfaceName("*");
    {
        PermissionPolicy::Rule::Member member[1];
        member[0].Set("*", PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                      PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                      PermissionPolicy::Rule::Member::ACTION_MODIFY |
                      PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        manifest[0].SetMembers(1, member);
    }

    uint8_t managerDigest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(managerBus,
                                                               manifest, manifestSize,
                                                               managerDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    uint8_t caDigest[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(busUsedAsCA,
                                                               manifest, manifestSize,
                                                               caDigest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";

    GUID128 managerGuid;
    PermissionMgmtTestHelper::GetGUID(managerBus, managerGuid);
    GUID128 peer1Guid;
    PermissionMgmtTestHelper::GetGUID(peer1Bus, peer1Guid);
    GUID128 peer2Guid;
    PermissionMgmtTestHelper::GetGUID(peer2Bus, peer2Guid);
    GUID128 caGuid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsCA, caGuid);
    GUID128 alphaGuid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsAlpha, alphaGuid);
    GUID128 betaGuid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsBeta, betaGuid);
    GUID128 gammaGuid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsGamma, gammaGuid);
    GUID128 phiGuid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsPhi, phiGuid);
    GUID128 siGuid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsSi, siGuid);
    GUID128 omegaGuid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsOmega, omegaGuid);
    GUID128 tauGuid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsTau, tauGuid);
    GUID128 int1Guid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsInt1, int1Guid);
    GUID128 int2Guid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsInt2, int2Guid);

    //Create identityCerts
    const size_t certChainSize = 1;
    IdentityCertificate identityCertChainMaster[certChainSize];
    IdentityCertificate identityCertChainPeer1[certChainSize];
    IdentityCertificate identityCertChainPeer2[certChainSize];

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  managerKey.GetPublicKey(),
                                                                  "ManagerAlias",
                                                                  3600,
                                                                  identityCertChainMaster[0],
                                                                  caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                  "0",
                                                                  peer1Guid.ToString(),
                                                                  peer1Key.GetPublicKey(),
                                                                  "Peer1Alias",
                                                                  3600,
                                                                  identityCertChainPeer1[0],
                                                                  caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                  "0",
                                                                  peer2Guid.ToString(),
                                                                  peer2Key.GetPublicKey(),
                                                                  "Peer2Alias",
                                                                  3600,
                                                                  identityCertChainPeer2[0],
                                                                  caDigest, Crypto_SHA256::DIGEST_SIZE)) << "Failed to create identity certificate.";

    managerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    EXPECT_EQ(ER_OK, sapWithManager.Claim(caKey,
                                          managerGuid,
                                          managerKey,
                                          identityCertChainMaster, certChainSize,
                                          manifest, manifestSize));

    peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    EXPECT_EQ(ER_OK, sapWithPeer1.Claim(caKey,
                                        managerGuid,
                                        managerKey,
                                        identityCertChainPeer1, certChainSize,
                                        manifest, manifestSize));

    peer2Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    EXPECT_EQ(ER_OK, sapWithPeer2.Claim(caKey,
                                        managerGuid,
                                        managerKey,
                                        identityCertChainPeer2, certChainSize,
                                        manifest, manifestSize));

    EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

    qcc::MembershipCertificate managerMembershipCertificate[2];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-1",
                                                                    busUsedAsCA,
                                                                    managerGuid.ToString(),
                                                                    managerKey.GetPublicKey(),
                                                                    managerGuid,
                                                                    true,
                                                                    3600,
                                                                    managerMembershipCertificate[1]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-0",
                                                                    managerBus,
                                                                    managerGuid.ToString(),
                                                                    managerKey.GetPublicKey(),
                                                                    managerGuid,
                                                                    false,
                                                                    3600,
                                                                    managerMembershipCertificate[0]
                                                                    ));
    EXPECT_EQ(ER_OK, sapWithManager.InstallMembership(managerMembershipCertificate, 2));
    EXPECT_EQ(ER_OK, sapWithManager.SecureConnection());

    // List of Membership on Peer1
    // alpha->Int1->Peer1
    // beta->Int1->Peer1
    // gamma->Peer1
    qcc::MembershipCertificate peer1MembershipCertificate[3];

    // alpha->Int1->Peer1
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-2",
                                                                    busUsedAsCA,
                                                                    alphaGuid.ToString(),
                                                                    alphaKey.GetPublicKey(),
                                                                    alphaGuid,
                                                                    true,
                                                                    3600,
                                                                    peer1MembershipCertificate[2]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-1",
                                                                    busUsedAsAlpha,
                                                                    int1Guid.ToString(),
                                                                    int1Key.GetPublicKey(),
                                                                    alphaGuid,
                                                                    true,
                                                                    3600,
                                                                    peer1MembershipCertificate[1]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-0",
                                                                    busUsedAsInt1,
                                                                    peer1Guid.ToString(),
                                                                    peer1Key.GetPublicKey(),
                                                                    alphaGuid,
                                                                    false,
                                                                    3600,
                                                                    peer1MembershipCertificate[0]
                                                                    ));

    EXPECT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1MembershipCertificate, 3));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection());

    // beta->Int1->Peer1
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("1-2",
                                                                    busUsedAsCA,
                                                                    betaGuid.ToString(),
                                                                    betaKey.GetPublicKey(),
                                                                    betaGuid,
                                                                    true,
                                                                    3600,
                                                                    peer1MembershipCertificate[2]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("1-1",
                                                                    busUsedAsBeta,
                                                                    int1Guid.ToString(),
                                                                    int1Key.GetPublicKey(),
                                                                    betaGuid,
                                                                    true,
                                                                    3600,
                                                                    peer1MembershipCertificate[1]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("1-0",
                                                                    busUsedAsInt1,
                                                                    peer1Guid.ToString(),
                                                                    peer1Key.GetPublicKey(),
                                                                    betaGuid,
                                                                    false,
                                                                    3600,
                                                                    peer1MembershipCertificate[0]
                                                                    ));

    EXPECT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1MembershipCertificate, 3));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection());

    // gamma->Peer1
    qcc::MembershipCertificate peer1GammaMembershipCertificate[2];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("2-1",
                                                                    busUsedAsCA,
                                                                    gammaGuid.ToString(),
                                                                    gammaKey.GetPublicKey(),
                                                                    gammaGuid,
                                                                    true,
                                                                    3600,
                                                                    peer1GammaMembershipCertificate[1]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("2-0",
                                                                    busUsedAsGamma,
                                                                    peer1Guid.ToString(),
                                                                    peer1Key.GetPublicKey(),
                                                                    gammaGuid,
                                                                    false,
                                                                    3600,
                                                                    peer1GammaMembershipCertificate[0]
                                                                    ));

    EXPECT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1GammaMembershipCertificate, 2));
    EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection());

    // List of memberships on Peer2 (App Bus)
    // phi->Int2->Peer2
    // omega->Int2->Peer2
    // si->Peer2

    qcc::MembershipCertificate peer2MembershipCertificate[3];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-2",
                                                                    busUsedAsCA,
                                                                    phiGuid.ToString(),
                                                                    phiKey.GetPublicKey(),
                                                                    phiGuid,
                                                                    true,
                                                                    3600,
                                                                    peer2MembershipCertificate[2]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-1",
                                                                    busUsedAsPhi,
                                                                    int2Guid.ToString(),
                                                                    int2Key.GetPublicKey(),
                                                                    phiGuid,
                                                                    true,
                                                                    3600,
                                                                    peer2MembershipCertificate[1]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-0",
                                                                    busUsedAsInt2,
                                                                    peer2Guid.ToString(),
                                                                    peer2Key.GetPublicKey(),
                                                                    phiGuid,
                                                                    false,
                                                                    3600,
                                                                    peer2MembershipCertificate[0]
                                                                    ));

    EXPECT_EQ(ER_OK, sapWithPeer2.InstallMembership(peer2MembershipCertificate, 3));
    EXPECT_EQ(ER_OK, sapWithPeer2.SecureConnection());

    // omega->Int2->Peer2
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("1-2",
                                                                    busUsedAsCA,
                                                                    omegaGuid.ToString(),
                                                                    omegaKey.GetPublicKey(),
                                                                    omegaGuid,
                                                                    true,
                                                                    3600,
                                                                    peer2MembershipCertificate[2]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("1-1",
                                                                    busUsedAsOmega,
                                                                    int2Guid.ToString(),
                                                                    int2Key.GetPublicKey(),
                                                                    omegaGuid,
                                                                    true,
                                                                    3600,
                                                                    peer2MembershipCertificate[1]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("1-0",
                                                                    busUsedAsInt2,
                                                                    peer2Guid.ToString(),
                                                                    peer2Key.GetPublicKey(),
                                                                    omegaGuid,
                                                                    false,
                                                                    3600,
                                                                    peer2MembershipCertificate[0]
                                                                    ));

    EXPECT_EQ(ER_OK, sapWithPeer2.InstallMembership(peer2MembershipCertificate, 3));
    EXPECT_EQ(ER_OK, sapWithPeer2.SecureConnection());

    // si->Peer2
    qcc::MembershipCertificate peer2SiMembershipCertificate[2];
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("2-1",
                                                                    busUsedAsCA,
                                                                    siGuid.ToString(),
                                                                    siKey.GetPublicKey(),
                                                                    siGuid,
                                                                    true,
                                                                    3600,
                                                                    peer2SiMembershipCertificate[1]
                                                                    ));
    EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("2-0",
                                                                    busUsedAsSi,
                                                                    peer2Guid.ToString(),
                                                                    peer2Key.GetPublicKey(),
                                                                    siGuid,
                                                                    false,
                                                                    3600,
                                                                    peer2SiMembershipCertificate[0]
                                                                    ));

    EXPECT_EQ(ER_OK, sapWithPeer2.InstallMembership(peer2SiMembershipCertificate, 2));
    EXPECT_EQ(ER_OK, sapWithPeer2.SecureConnection());

    /*
       Peer2 has the following ACLs
       WITH_MEMBERSHIP: alpha SGID; rule is MODIFY on ping
       WITH_MEMBERSHIP: beta SGID; rule is MODIFY on king
       WITH_MEMBERSHIP: gamma SGID; rule is MODIFY on sing
     */

    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(2);
    {
        PermissionPolicy::Acl acls[3];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(alphaGuid);
            peers[0].SetKeyInfo(&alphaKey);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("ping",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(betaGuid);
            peers[0].SetKeyInfo(&betaKey);
            acls[1].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("king",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[1].SetRules(1, rules);
        }
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(gammaGuid);
            peers[0].SetKeyInfo(&gammaKey);
            acls[2].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("sing",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[2].SetRules(1, rules);
        }
        peer2Policy.SetAcls(3, acls);
    }
    {
        PermissionPolicy defaultPolicy;
        sapWithPeer2.GetDefaultPolicy(defaultPolicy);
        UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer2Policy, true, true, true);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
        EXPECT_EQ(ER_OK, sapWithPeer2.SecureConnection(true));
    }

/*
   Peer1 has the following ACLs.
   WITH_MEMBERSHIP: phi SGID; rule is to PROVIDE on ping
   WITH_MEMBERSHIP: si SGID; rule is to PROVIDE on king
   WITH_MEMBERSHIP: omega SGID; rule is to PROVIDE on sing
   WITH_MEMBERSHIP: tau SGID; rule is to DENY all
 */

    PermissionPolicy peer1Policy;
    peer1Policy.SetVersion(2);
    {
        PermissionPolicy::Acl acls[4];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(phiGuid);
            peers[0].SetKeyInfo(&phiKey);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("ping",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(siGuid);
            peers[0].SetKeyInfo(&siKey);
            acls[1].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("king",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[1].SetRules(1, rules);
        }
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(omegaGuid);
            peers[0].SetKeyInfo(&omegaKey);
            acls[2].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("sing",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE);
                rules[0].SetMembers(1, members);
            }
            acls[2].SetRules(1, rules);
        }
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(tauGuid);
            peers[0].SetKeyInfo(&tauKey);
            acls[3].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("*",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_OBSERVE);
                rules[0].SetMembers(1, members);
            }
            acls[3].SetRules(1, rules);
        }
        peer1Policy.SetAcls(4, acls);
    }
    {
        PermissionPolicy defaultPolicy;
        sapWithPeer1.GetDefaultPolicy(defaultPolicy);
        UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer1Policy, true, true, true);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(peer1Policy));
        EXPECT_EQ(ER_OK, sapWithPeer1.SecureConnection(true));
    }

    SessionId peer1ToPeer2SessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the Echo method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface(interfaceName)) << interface.c_str() << "\n" << interfaceName;
    EXPECT_EQ(ER_OK, proxy.SecureConnection(true));

    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "ping", NULL, static_cast<size_t>(0), replyMsg));
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "king", NULL, static_cast<size_t>(0), replyMsg));
    EXPECT_EQ(ER_OK, proxy.MethodCall(interfaceName, "sing", NULL, static_cast<size_t>(0), replyMsg));
}
