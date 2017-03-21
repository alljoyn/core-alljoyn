/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/
#include <gtest/gtest.h>
#include <alljoyn/AuthListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/SecurityApplicationProxy.h>
#include <qcc/Util.h>
#include <iostream>
#include <map>

#include "InMemoryKeyStore.h"
#include "PermissionMgmtObj.h"
#include "PermissionMgmtTest.h"

using namespace ajn;
using namespace qcc;
using namespace std;

class TestSessionPortListener : public SessionPortListener {
  public:
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
};

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
        QCC_ASSERT(idx <= count);
        acls[idx++] = policy.GetAcls()[cnt];
    }

    policy.SetAcls(count, acls);
    delete [] acls;
    return ER_OK;
}

// Scope this object to just this file to avoid One Definition Rule violation as described in ASACORE-3467
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

class MultipleTrustAnchorsPropagationTest : public testing::Test {
  public:
    MultipleTrustAnchorsPropagationTest() :
        managerBus("SecurityManager"),
        peer1Bus("Peer1"),
        peer2Bus("Peer2"),
        busUsedAsCA1("busUsedAsCA1"),
        busUsedAsCA2("busUsedAsCA2"),
        managerSessionPort(42),
        peer1SessionPort(42),
        peer2SessionPort(42)
    {
    }

    void startBusAttachments() {
        ASSERT_EQ(ER_OK, managerBus.Start());
        ASSERT_EQ(ER_OK, managerBus.Connect());
        ASSERT_EQ(ER_OK, peer1Bus.Start());
        ASSERT_EQ(ER_OK, peer1Bus.Connect());
        ASSERT_EQ(ER_OK, peer2Bus.Start());
        ASSERT_EQ(ER_OK, peer2Bus.Connect());

        ASSERT_EQ(ER_OK, busUsedAsCA1.Start());
        ASSERT_EQ(ER_OK, busUsedAsCA1.Connect());
        ASSERT_EQ(ER_OK, busUsedAsCA2.Start());
        ASSERT_EQ(ER_OK, busUsedAsCA2.Connect());
    }

    void registerKeystoreListeners() {
        ASSERT_EQ(ER_OK, managerBus.RegisterKeyStoreListener(managerKeyStoreListener));
        ASSERT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
        ASSERT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));
        ASSERT_EQ(ER_OK, busUsedAsCA1.RegisterKeyStoreListener(ca1KeyStoreListener));
        ASSERT_EQ(ER_OK, busUsedAsCA2.RegisterKeyStoreListener(ca2KeyStoreListener));
    }

    void enableSecurity() {
        ASSERT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
        ASSERT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
        ASSERT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

        ASSERT_EQ(ER_OK, busUsedAsCA1.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &ca1AuthListener));
        ASSERT_EQ(ER_OK, busUsedAsCA2.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &ca2AuthListener));
    }

    void startSessions() {
        SessionOpts opts;
        ASSERT_EQ(ER_OK, managerBus.BindSessionPort(managerSessionPort, opts, managerSessionPortListener));
        ASSERT_EQ(ER_OK, peer1Bus.BindSessionPort(peer1SessionPort, opts, peer1SessionPortListener));
        ASSERT_EQ(ER_OK, peer2Bus.BindSessionPort(peer2SessionPort, opts, peer2SessionPortListener));

        ASSERT_EQ(ER_OK, managerBus.JoinSession(managerBus.GetUniqueName().c_str(), managerSessionPort, nullptr, managerToManagerSessionId, opts));
        ASSERT_EQ(ER_OK, managerBus.JoinSession(peer1Bus.GetUniqueName().c_str(), peer1SessionPort, nullptr, managerToPeer1SessionId, opts));
        ASSERT_EQ(ER_OK, managerBus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, nullptr, managerToPeer2SessionId, opts));
    }

    void getPublicKeys() {
        PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
        ASSERT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

        PermissionConfigurator& pcPeer1 = peer1Bus.GetPermissionConfigurator();
        ASSERT_EQ(ER_OK, pcPeer1.GetSigningPublicKey(peer1Key));

        PermissionConfigurator& pcPeer2 = peer2Bus.GetPermissionConfigurator();
        ASSERT_EQ(ER_OK, pcPeer2.GetSigningPublicKey(peer2Key));

        PermissionConfigurator& pcCA1 = busUsedAsCA1.GetPermissionConfigurator();
        ASSERT_EQ(ER_OK, pcCA1.GetSigningPublicKey(ca1Key));

        PermissionConfigurator& pcCA2 = busUsedAsCA2.GetPermissionConfigurator();
        ASSERT_EQ(ER_OK, pcCA2.GetSigningPublicKey(ca2Key));
    }

    void getGUIDs() {
        PermissionMgmtTestHelper::GetGUID(managerBus, managerGuid);
        PermissionMgmtTestHelper::GetGUID(peer1Bus, peer1Guid);
        PermissionMgmtTestHelper::GetGUID(peer2Bus, peer2Guid);
        PermissionMgmtTestHelper::GetGUID(busUsedAsCA1, ca1Guid);
        PermissionMgmtTestHelper::GetGUID(busUsedAsCA2, ca2Guid);
    }

    void unbindSessionPorts() {
        EXPECT_EQ(ER_OK, managerBus.UnbindSessionPort(managerSessionPort));
        EXPECT_EQ(ER_OK, peer1Bus.UnbindSessionPort(peer1SessionPort));
        EXPECT_EQ(ER_OK, peer2Bus.UnbindSessionPort(peer2SessionPort));
    }

    void stopBusAttachments() {
        EXPECT_EQ(ER_OK, managerBus.Stop());
        EXPECT_EQ(ER_OK, managerBus.Join());
        EXPECT_EQ(ER_OK, peer1Bus.Stop());
        EXPECT_EQ(ER_OK, peer1Bus.Join());
        EXPECT_EQ(ER_OK, peer2Bus.Stop());
        EXPECT_EQ(ER_OK, peer2Bus.Join());

        EXPECT_EQ(ER_OK, busUsedAsCA1.Stop());
        EXPECT_EQ(ER_OK, busUsedAsCA1.Join());
        EXPECT_EQ(ER_OK, busUsedAsCA2.Stop());
        EXPECT_EQ(ER_OK, busUsedAsCA2.Join());
    }

    virtual void SetUp() {
        startBusAttachments();
        registerKeystoreListeners();
        enableSecurity();
        startSessions();
        getPublicKeys();
        getGUIDs();
    }

    virtual void TearDown() {
        unbindSessionPorts();
        stopBusAttachments();
    }

    static const std::string s_interfaceName;
    static const std::string s_interfaceXml;

    BusAttachment managerBus;
    BusAttachment peer1Bus;
    BusAttachment peer2Bus;
    BusAttachment busUsedAsCA1;
    BusAttachment busUsedAsCA2;

    InMemoryKeyStoreListener managerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;
    InMemoryKeyStoreListener ca1KeyStoreListener;
    InMemoryKeyStoreListener ca2KeyStoreListener;

    DefaultECDHEAuthListener managerAuthListener;
    DefaultECDHEAuthListener peer1AuthListener;
    DefaultECDHEAuthListener peer2AuthListener;
    DefaultECDHEAuthListener ca1AuthListener;
    DefaultECDHEAuthListener ca2AuthListener;

    TestSessionPortListener managerSessionPortListener;
    TestSessionPortListener peer1SessionPortListener;
    TestSessionPortListener peer2SessionPortListener;

    SessionId managerToManagerSessionId;
    SessionId managerToPeer1SessionId;
    SessionId managerToPeer2SessionId;

    String managerBusUniqueName;
    String peer1BusUniqueName;
    String peer2BusUniqueName;

    SessionPort managerSessionPort;
    SessionPort peer1SessionPort;
    SessionPort peer2SessionPort;

    KeyInfoNISTP256 managerKey;
    KeyInfoNISTP256 peer1Key;
    KeyInfoNISTP256 peer2Key;
    KeyInfoNISTP256 ca1Key;
    KeyInfoNISTP256 ca2Key;

    GUID128 managerGuid;
    GUID128 peer1Guid;
    GUID128 peer2Guid;
    GUID128 ca1Guid;
    GUID128 ca2Guid;
    GUID128 livingRoomGuid;
};

const std::string MultipleTrustAnchorsPropagationTest::s_interfaceName =
    "org.allseen.test.SecurityApplication.membershipPropagation";
const std::string MultipleTrustAnchorsPropagationTest::s_interfaceXml =
    "<node>"
    "<interface name='" + String(s_interfaceName) + "'>"
    "<annotation name='org.alljoyn.Bus.Secure' value='true'/>"
    "  <method name='ping'>"
    "  </method>"
    "</interface>"
    "</node>";


/*
 * Purpose:
 * - Verify if an admin is trusted by a peer whose identity certificate is signed by a trust anchor
 *   different from the one which issued the admin's certificates,
 * - Verify if a group membership certificate is obtained and trusted by a peer whose identity
 *   certificate is signed by a trust anchor different from the one which issued the membership certificate.
 *
 * Setup:
 * 1.
 * - Peer1 and Peer2 are claimed by Manager,
 * - Peer1's and Peer2's identity certificates are signed by CA1,
 * - Manager is an admin (member of the ASG),
 * - CA2 is the ASGA,
 * - Manager's identity certificate and ASG certificate are signed by CA2.
 * 2.
 * - Manager is claimed with CA1 as certificate authority.
 * 3.
 * - Peer2 has method "ping",
 * - Method "ping" can be called only by members of group livingRoom, per Peer2's permission policy.
 * 4.
 * - Peer1 has a livingRoom membership certificate installed by Manager and signed by the ASGA.
 * 5.
 * - Peer1 tries to call method "ping" of Peer2.
 *
 * Verification:
 * - The call should succeed.
 */
TEST_F(MultipleTrustAnchorsPropagationTest, Peer_With_Membership_Admin_claimed_with_CA1_as_CA) {
    SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);
    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    ASSERT_EQ(ER_OK, peer2Bus.CreateInterfacesFromXml(s_interfaceXml.c_str()));
    TestBusObject peer2BusObject(peer2Bus, "/test", s_interfaceName.c_str());
    ASSERT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    /* Create identity certs. */
    const size_t certChainSize = 2;
    IdentityCertificate identityCertChainMasterCA1[certChainSize];
    IdentityCertificate identityCertChainMasterCA2[certChainSize];
    IdentityCertificate identityCertChainPeer1[certChainSize];
    IdentityCertificate identityCertChainPeer2[certChainSize];

    /* Create CA certs. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA1,
                                                                  "0",
                                                                  ca1Guid.ToString(),
                                                                  ca1Key.GetPublicKey(),
                                                                  "CertificateAuthority",
                                                                  3600,
                                                                  identityCertChainMasterCA1[1])) << "Failed to create CA1 cert";
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA2,
                                                                  "0",
                                                                  ca2Guid.ToString(),
                                                                  ca2Key.GetPublicKey(),
                                                                  "CertificateAuthority2",
                                                                  3600,
                                                                  identityCertChainMasterCA2[1])) << "Failed to create CA1 cert";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SetCAFlagOnCert(busUsedAsCA1, identityCertChainMasterCA1[1])) << "Failed to set CA flag on CA1's cert";
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SetCAFlagOnCert(busUsedAsCA2, identityCertChainMasterCA2[1])) << "Failed to set CA flag on CA2's cert";

    /* Manager's identity certificate to be signed by CA2. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA2,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  managerKey.GetPublicKey(),
                                                                  "ManagerAlias",
                                                                  3600,
                                                                  identityCertChainMasterCA2[0])) << "Failed to create Manager identity certificate.";

    /* Both peer identity certificates to be signed by CA1. */
    identityCertChainPeer1[1] = identityCertChainMasterCA1[1];
    identityCertChainPeer2[1] = identityCertChainMasterCA1[1];

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA1,
                                                                  "0",
                                                                  peer1Guid.ToString(),
                                                                  peer1Key.GetPublicKey(),
                                                                  "Peer1Alias",
                                                                  3600,
                                                                  identityCertChainPeer1[0])) << "Failed to create Peer1 identity certificate.";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA1,
                                                                  "0",
                                                                  peer2Guid.ToString(),
                                                                  peer2Key.GetPublicKey(),
                                                                  "Peer2Alias",
                                                                  3600,
                                                                  identityCertChainPeer2[0])) << "Failed to create Peer2 identity certificate.";

    managerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(busUsedAsCA2, identityCertChainMasterCA2[0], manifests[0]));
    /*
     * Manager's identity certificate is signed by CA2.
     * Add CA1's key as Manager's authority so that it recognizes
     * peer certificates which are issued by CA1 (see also next test).
     */
    ASSERT_EQ(ER_OK, sapWithManager.Claim(ca1Key,
                                          managerGuid,
                                          ca2Key,
                                          identityCertChainMasterCA2, certChainSize,
                                          manifests, ArraySize(manifests)));

    peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(busUsedAsCA1, identityCertChainPeer1[0], manifests[0]));
    ASSERT_EQ(ER_OK, sapWithPeer1.Claim(ca1Key,
                                        managerGuid,
                                        ca2Key,
                                        identityCertChainPeer1, certChainSize,
                                        manifests, ArraySize(manifests)));

    peer2Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(busUsedAsCA1, identityCertChainPeer2[0], manifests[0]));
    ASSERT_EQ(ER_OK, sapWithPeer2.Claim(ca1Key,
                                        managerGuid,
                                        ca2Key,
                                        identityCertChainPeer2, certChainSize,
                                        manifests, ArraySize(manifests)));

    ASSERT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
    ASSERT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
    ASSERT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

    qcc::MembershipCertificate managerMembershipCertificate[1];

    /* Manager's ASG membership certificate to be signed by CA2 which is the ASGA. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-1",
                                                                    busUsedAsCA2,
                                                                    managerGuid.ToString(),
                                                                    managerKey.GetPublicKey(),
                                                                    managerGuid,
                                                                    false,
                                                                    3600,
                                                                    managerMembershipCertificate[0]
                                                                    ));

    ASSERT_EQ(ER_OK, sapWithManager.InstallMembership(managerMembershipCertificate, 1));
    ASSERT_EQ(ER_OK, sapWithManager.SecureConnection());

    qcc::MembershipCertificate peer1MembershipCertificate[1];
    /* Peer1 Living Room SG certificate to be signed by CA2 which is the ASGA. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("1-1",
                                                                    busUsedAsCA2,
                                                                    peer1Guid.ToString(),
                                                                    peer1Key.GetPublicKey(),
                                                                    livingRoomGuid,
                                                                    false,
                                                                    3600,
                                                                    peer1MembershipCertificate[0]
                                                                    ));

    ASSERT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1MembershipCertificate, 1));
    ASSERT_EQ(ER_OK, sapWithPeer1.SecureConnection());

    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(2);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(livingRoomGuid);
            peers[0].SetKeyInfo(&ca2Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(s_interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("ping",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }
    {
        PermissionPolicy defaultPolicy;
        sapWithPeer2.GetDefaultPolicy(defaultPolicy);
        UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer2Policy, true, true, true);
        ASSERT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
        ASSERT_EQ(ER_OK, sapWithPeer2.SecureConnection(true));
    }

    SessionId peer1ToPeer2SessionId;
    SessionOpts opts;
    ASSERT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, nullptr, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    ASSERT_EQ(ER_OK, proxy.ParseXml(s_interfaceXml.c_str()));
    ASSERT_TRUE(proxy.ImplementsInterface(s_interfaceName.c_str()));
    ASSERT_EQ(ER_OK, proxy.SecureConnection(true));

    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(s_interfaceName.c_str(), "ping", nullptr, static_cast<size_t>(0), replyMsg));
}

/*
 * Purpose:
 * - Verify if an admin is trusted by a peer whose identity certificate is signed by a trust anchor
 *   different from the one which issued the admin's certificates,
 * - Verify if a group membership certificate is obtained and trusted by a peer whose identity
 *   certificate is signed by a trust anchor different from the one which issued the membership certificate,
 * - Verify if adding a FROM_CERTIFICATE_AUTHORITY clause to an Admin's security policy makes the Admin
 *   trust peers whose identity certificates have been issued by CAs previously unknown to the Admin
 *   and specified in the clause.
 *
 * Setup:
 * 1.
 * - Peer1 and Peer2 are claimed by Manager,
 * - Peer1's and Peer2's identity certificates are signed by CA1,
 * - Manager is an admin (member of the ASG),
 * - CA2 is the ASGA,
 * - Manager's identity certificate and ASG certificate are signed by CA2,
 * 2.
 * - Manager is claimed with CA2 as certificate authority,
 * - A FROM_CERTIFICATE_AUTHORITY clause with CA1 is added to Manager's policy.
 * 3.
 * - Peer2 has method "ping",
 * - Method "ping" can be called only by members of group livingRoom, per Peer2's permission policy.
 * 4.
 * - Peer1 has a livingRoom membership certificate installed by Manager and signed by ASGA.
 * 5.
 * - Peer1 tries to call method "ping" of Peer2.
 *
 * Verification:
 * - The call should succeed.
 */
TEST_F(MultipleTrustAnchorsPropagationTest, Peer_With_Membership_CA1_added_to_Admin_in_policy) {
    SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);
    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    ASSERT_EQ(ER_OK, peer2Bus.CreateInterfacesFromXml(s_interfaceXml.c_str()));
    TestBusObject peer2BusObject(peer2Bus, "/test", s_interfaceName.c_str());
    ASSERT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    /* Create identity certs. */
    const size_t certChainSize = 2;
    IdentityCertificate identityCertChainMasterCA1[certChainSize];
    IdentityCertificate identityCertChainMasterCA2[certChainSize];
    IdentityCertificate identityCertChainPeer1[certChainSize];
    IdentityCertificate identityCertChainPeer2[certChainSize];

    /* Create CA certs. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA1,
                                                                  "0",
                                                                  ca1Guid.ToString(),
                                                                  ca1Key.GetPublicKey(),
                                                                  "CertificateAuthority",
                                                                  3600,
                                                                  identityCertChainMasterCA1[1])) << "Failed to create CA1 cert";
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA2,
                                                                  "0",
                                                                  ca2Guid.ToString(),
                                                                  ca2Key.GetPublicKey(),
                                                                  "CertificateAuthority2",
                                                                  3600,
                                                                  identityCertChainMasterCA2[1])) << "Failed to create CA1 cert";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SetCAFlagOnCert(busUsedAsCA1, identityCertChainMasterCA1[1])) << "Failed to set CA flag on CA1's cert";
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SetCAFlagOnCert(busUsedAsCA2, identityCertChainMasterCA2[1])) << "Failed to set CA flag on CA2's cert";

    /* Manager's identity certificate to be signed by CA2. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA2,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  managerKey.GetPublicKey(),
                                                                  "ManagerAlias",
                                                                  3600,
                                                                  identityCertChainMasterCA2[0])) << "Failed to create Manager identity certificate.";

    /* Both peer identity certificates to be signed by CA1. */
    identityCertChainPeer1[1] = identityCertChainMasterCA1[1];
    identityCertChainPeer2[1] = identityCertChainMasterCA1[1];

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA1,
                                                                  "0",
                                                                  peer1Guid.ToString(),
                                                                  peer1Key.GetPublicKey(),
                                                                  "Peer1Alias",
                                                                  3600,
                                                                  identityCertChainPeer1[0])) << "Failed to create Peer1 identity certificate.";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA1,
                                                                  "0",
                                                                  peer2Guid.ToString(),
                                                                  peer2Key.GetPublicKey(),
                                                                  "Peer2Alias",
                                                                  3600,
                                                                  identityCertChainPeer2[0])) << "Failed to create Peer2 identity certificate.";

    managerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(busUsedAsCA2, identityCertChainMasterCA2[0], manifests[0]));
    /*
     * Manager's identity certificate is signed by CA2.
     * Manager's certificate authority is set to CA2.
     * At this point, Manager does not have CA1 set as its trust anchor
     * and will not trust it.
     */
    ASSERT_EQ(ER_OK, sapWithManager.Claim(ca2Key,
                                          managerGuid,
                                          ca2Key,
                                          identityCertChainMasterCA2, certChainSize,
                                          manifests, ArraySize(manifests)));

    peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(busUsedAsCA1, identityCertChainPeer1[0], manifests[0]));
    ASSERT_EQ(ER_OK, sapWithPeer1.Claim(ca1Key,
                                        managerGuid,
                                        ca2Key,
                                        identityCertChainPeer1, certChainSize,
                                        manifests, ArraySize(manifests)));

    peer2Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(busUsedAsCA1, identityCertChainPeer2[0], manifests[0]));
    ASSERT_EQ(ER_OK, sapWithPeer2.Claim(ca1Key,
                                        managerGuid,
                                        ca2Key,
                                        identityCertChainPeer2, certChainSize,
                                        manifests, ArraySize(manifests)));

    ASSERT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
    ASSERT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
    ASSERT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

    qcc::MembershipCertificate managerMembershipCertificate[1];

    /* Manager's ASG membership certificate to be signed by CA2 which is the ASGA. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-1",
                                                                    busUsedAsCA2,
                                                                    managerGuid.ToString(),
                                                                    managerKey.GetPublicKey(),
                                                                    managerGuid,
                                                                    false,
                                                                    3600,
                                                                    managerMembershipCertificate[0]
                                                                    ));

    ASSERT_EQ(ER_OK, sapWithManager.InstallMembership(managerMembershipCertificate, 1));
    ASSERT_EQ(ER_OK, sapWithManager.SecureConnection());

    /*
     * Add a FROM_CERTIFICATE_AUTHORITY clause with CA1 key
     * to Manager's policy so that peers with identity certs
     * signed by CA1 are authorized by Manager.
     */
    PermissionPolicy managerPolicy;
    managerPolicy.SetVersion(2);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
            peers[0].SetKeyInfo(&ca1Key);
            acls[0].SetPeers(1, peers);
        }
        managerPolicy.SetAcls(1, acls);
    }
    {
        PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
        PermissionPolicy defaultPolicy;
        pcManager.GetDefaultPolicy(defaultPolicy);
        UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, managerPolicy, true, true, true);
        ASSERT_EQ(ER_OK, pcManager.UpdatePolicy(managerPolicy));
        ASSERT_EQ(ER_OK, sapWithManager.SecureConnection(true));
    }

    qcc::MembershipCertificate peer1MembershipCertificate[1];
    /* Peer1 Living Room SG certificate to be signed by CA2 which is the ASGA. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("1-1",
                                                                    busUsedAsCA2,
                                                                    peer1Guid.ToString(),
                                                                    peer1Key.GetPublicKey(),
                                                                    livingRoomGuid,
                                                                    false,
                                                                    3600,
                                                                    peer1MembershipCertificate[0]
                                                                    ));

    ASSERT_EQ(ER_OK, sapWithPeer1.InstallMembership(peer1MembershipCertificate, 1));
    ASSERT_EQ(ER_OK, sapWithPeer1.SecureConnection());

    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(2);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
            peers[0].SetSecurityGroupId(livingRoomGuid);
            peers[0].SetKeyInfo(&ca2Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(s_interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("ping",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }
    {
        PermissionPolicy defaultPolicy;
        sapWithPeer2.GetDefaultPolicy(defaultPolicy);
        UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer2Policy, true, true, true);
        ASSERT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
        ASSERT_EQ(ER_OK, sapWithPeer2.SecureConnection(true));
    }

    SessionId peer1ToPeer2SessionId;
    SessionOpts opts;
    ASSERT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, nullptr, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call the method on the interface */
    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    ASSERT_EQ(ER_OK, proxy.ParseXml(s_interfaceXml.c_str()));
    ASSERT_TRUE(proxy.ImplementsInterface(s_interfaceName.c_str()));
    ASSERT_EQ(ER_OK, proxy.SecureConnection(true));

    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall(s_interfaceName.c_str(), "ping", nullptr, static_cast<size_t>(0), replyMsg));
}

/*
 * Purpose:
 * - Verify if an admin is trusted by a peer whose identity certificate is signed by a trust anchor
 *   different from the one which issued the admin's certificates,
 * - Verify if adding a FROM_CERTIFICATE_AUTHORITY clause to a peer's security policy makes the peer
 *   trust peers whose identity certificates have been issued by CAs previously unknown to the peer
 *   and specified in the clause.
 *
 * Setup:
 * 1.
 * - Peer1 and Peer2 are claimed by Manager,
 * - Peer1's identity certificate is signed by CA1,
 * - Peer2's identity certificate is signed by CA2,
 * - Manager is an admin (member of the ASG),
 * - CA2 is the ASGA,
 * - Manager's identity certificate and ASG certificate are signed by CA2.
 * 2.
 * - Manager is claimed with CA1 as certificate authority.
 * 3.
 * - Peer2 has method "ping",
 * - On Peer2, method "ping" can be called only by peers whose identity is verified by CA1
 *   (a FROM_CERTIFICATE_AUTHORITY clause is added to Peer2's policy).
 * 4.
 * - Peer1 tries to call method "ping" of Peer2.
 *
 * Verification:
 * - The call should succeed.
 */
TEST_F(MultipleTrustAnchorsPropagationTest, Peer_From_Certificate_Authority) {
    SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);
    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);

    ASSERT_EQ(ER_OK, peer2Bus.CreateInterfacesFromXml(s_interfaceXml.c_str()));
    TestBusObject peer2BusObject(peer2Bus, "/test", s_interfaceName.c_str());
    ASSERT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject));

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    /* Create identity certs. */
    const size_t certChainSize = 2;
    IdentityCertificate identityCertChainMasterCA1[certChainSize];
    IdentityCertificate identityCertChainMasterCA2[certChainSize];
    IdentityCertificate identityCertChainPeer1[certChainSize];
    IdentityCertificate identityCertChainPeer2[certChainSize];

    /* Create CA certs. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA1,
                                                                  "0",
                                                                  ca1Guid.ToString(),
                                                                  ca1Key.GetPublicKey(),
                                                                  "CertificateAuthority",
                                                                  3600,
                                                                  identityCertChainMasterCA1[1])) << "Failed to create CA1 cert";
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA2,
                                                                  "0",
                                                                  ca2Guid.ToString(),
                                                                  ca2Key.GetPublicKey(),
                                                                  "CertificateAuthority2",
                                                                  3600,
                                                                  identityCertChainMasterCA2[1])) << "Failed to create CA1 cert";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SetCAFlagOnCert(busUsedAsCA1, identityCertChainMasterCA1[1])) << "Failed to set CA flag on CA1's cert";
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SetCAFlagOnCert(busUsedAsCA2, identityCertChainMasterCA2[1])) << "Failed to set CA flag on CA2's cert";

    /* Manager's identity certificate to be signed by CA2. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA2,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  managerKey.GetPublicKey(),
                                                                  "ManagerAlias",
                                                                  3600,
                                                                  identityCertChainMasterCA2[0])) << "Failed to create Manager identity certificate.";

    /* Peer1 identity certificate to be signed by CA1. */
    identityCertChainPeer1[1] = identityCertChainMasterCA1[1];
    /* Peer2 identity certificate to be signed by CA2. */
    identityCertChainPeer2[1] = identityCertChainMasterCA2[1];

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA1,
                                                                  "0",
                                                                  peer1Guid.ToString(),
                                                                  peer1Key.GetPublicKey(),
                                                                  "Peer1Alias",
                                                                  3600,
                                                                  identityCertChainPeer1[0])) << "Failed to create Peer1 identity certificate.";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA2,
                                                                  "0",
                                                                  peer2Guid.ToString(),
                                                                  peer2Key.GetPublicKey(),
                                                                  "Peer2Alias",
                                                                  3600,
                                                                  identityCertChainPeer2[0])) << "Failed to create Peer2 identity certificate.";

    managerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(busUsedAsCA2, identityCertChainMasterCA2[0], manifests[0]));
    /*
     * Manager's identity certificate is signed by CA2.
     * Add CA1's key as Manager's authority so that it recognizes
     * peer certificates which are issued by CA1.
     */
    ASSERT_EQ(ER_OK, sapWithManager.Claim(ca1Key,
                                          managerGuid,
                                          ca2Key,
                                          identityCertChainMasterCA2, certChainSize,
                                          manifests, ArraySize(manifests)));

    peer1Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(busUsedAsCA1, identityCertChainPeer1[0], manifests[0]));
    ASSERT_EQ(ER_OK, sapWithPeer1.Claim(ca1Key,
                                        managerGuid,
                                        ca2Key,
                                        identityCertChainPeer1, certChainSize,
                                        manifests, ArraySize(manifests)));

    peer2Bus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(busUsedAsCA2, identityCertChainPeer2[0], manifests[0]));
    ASSERT_EQ(ER_OK, sapWithPeer2.Claim(ca2Key,
                                        managerGuid,
                                        ca2Key,
                                        identityCertChainPeer2, certChainSize,
                                        manifests, ArraySize(manifests)));

    ASSERT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
    ASSERT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
    ASSERT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

    qcc::MembershipCertificate managerMembershipCertificate[1];

    /* Manager's ASG membership certificate to be signed by CA2 which is the ASGA. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-1",
                                                                    busUsedAsCA2,
                                                                    managerGuid.ToString(),
                                                                    managerKey.GetPublicKey(),
                                                                    managerGuid,
                                                                    false,
                                                                    3600,
                                                                    managerMembershipCertificate[0]
                                                                    ));

    ASSERT_EQ(ER_OK, sapWithManager.InstallMembership(managerMembershipCertificate, 1));
    ASSERT_EQ(ER_OK, sapWithManager.SecureConnection());

    /* Add a FROM_CERTIFICATE_AUTHORITY clause with CA1 key
     * to Peer2's policy.
     */
    PermissionPolicy peer2Policy;
    peer2Policy.SetVersion(2);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
            peers[0].SetKeyInfo(&ca1Key);
            acls[0].SetPeers(1, peers);
        }
        {
            PermissionPolicy::Rule rules[1];
            rules[0].SetObjPath("*");
            rules[0].SetInterfaceName(s_interfaceName);
            {
                PermissionPolicy::Rule::Member members[1];
                members[0].Set("ping",
                               PermissionPolicy::Rule::Member::METHOD_CALL,
                               PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                               PermissionPolicy::Rule::Member::ACTION_MODIFY);
                rules[0].SetMembers(1, members);
            }
            acls[0].SetRules(1, rules);
        }
        peer2Policy.SetAcls(1, acls);
    }
    {
        PermissionPolicy defaultPolicy;
        sapWithPeer2.GetDefaultPolicy(defaultPolicy);
        UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, peer2Policy, true, true, true);
        ASSERT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(peer2Policy));
        ASSERT_EQ(ER_OK, sapWithPeer2.SecureConnection(true));
    }

    /* Make Peer1 join a session hosted by Peer2. */
    SessionId peer1ToPeer2SessionId;
    SessionOpts opts;
    ASSERT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, nullptr, peer1ToPeer2SessionId, opts));

    /* Create the ProxyBusObject and call Peer2's "ping" method. */
    ProxyBusObject peer1ToPeer2Proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", peer1ToPeer2SessionId, true);
    ASSERT_EQ(ER_OK, peer1ToPeer2Proxy.ParseXml(s_interfaceXml.c_str()));
    ASSERT_TRUE(peer1ToPeer2Proxy.ImplementsInterface(s_interfaceName.c_str()));
    ASSERT_EQ(ER_OK, peer1ToPeer2Proxy.SecureConnection(true));

    Message peer1ToPeer2ReplyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, peer1ToPeer2Proxy.MethodCall(s_interfaceName.c_str(), "ping", nullptr, static_cast<size_t>(0), peer1ToPeer2ReplyMsg));
}

/*
 * Failing test case discovered during work on ASACORE-3142.
 * Currently disabled, to be investigated under
 * ASACORE-3451 Security 2.0: ASGA cannot update its own policy remotely.
 */
TEST(MembershipPropagationTest, DISABLED_manager_updates_own_policy_via_remote_call) {
    BusAttachment managerBus("SecurityACLManager");

    BusAttachment busUsedAsCA("busUsedAsCA");

    ASSERT_EQ(ER_OK, managerBus.Start());
    ASSERT_EQ(ER_OK, managerBus.Connect());

    ASSERT_EQ(ER_OK, busUsedAsCA.Start());
    ASSERT_EQ(ER_OK, busUsedAsCA.Connect());

    InMemoryKeyStoreListener managerKeyStoreListener;
    InMemoryKeyStoreListener caKeyStoreListener;

    ASSERT_EQ(ER_OK, managerBus.RegisterKeyStoreListener(managerKeyStoreListener));
    ASSERT_EQ(ER_OK, busUsedAsCA.RegisterKeyStoreListener(caKeyStoreListener));

    DefaultECDHEAuthListener managerAuthListener;
    DefaultECDHEAuthListener caAuthListener;

    ASSERT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &managerAuthListener));

    ASSERT_EQ(ER_OK, busUsedAsCA.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &caAuthListener));

    SessionPort managerSessionPort(42);
    SessionOpts opts;
    TestSessionPortListener managerSessionPortListener;
    ASSERT_EQ(ER_OK, managerBus.BindSessionPort(managerSessionPort, opts, managerSessionPortListener));

    SessionId managerToManagerSessionId;
    ASSERT_EQ(ER_OK, managerBus.JoinSession(managerBus.GetUniqueName().c_str(), managerSessionPort, NULL, managerToManagerSessionId, opts));

    SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);

    /* Get public keys. */
    KeyInfoNISTP256 managerKey;
    PermissionConfigurator& pcManager = managerBus.GetPermissionConfigurator();
    ASSERT_EQ(ER_OK, pcManager.GetSigningPublicKey(managerKey));

    KeyInfoNISTP256 caKey;
    PermissionConfigurator& pcCA = busUsedAsCA.GetPermissionConfigurator();
    ASSERT_EQ(ER_OK, pcCA.GetSigningPublicKey(caKey));

    Manifest manifests[1];
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateAllInclusiveManifest(manifests[0]));

    GUID128 managerGuid;
    PermissionMgmtTestHelper::GetGUID(managerBus, managerGuid);
    GUID128 caGuid;
    PermissionMgmtTestHelper::GetGUID(busUsedAsCA, caGuid);

    /* Create identity certs. */
    const size_t certChainSize = 2;
    IdentityCertificate identityCertChainMasterCA[certChainSize];

    /* Create CA certs. */
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                  "0",
                                                                  caGuid.ToString(),
                                                                  caKey.GetPublicKey(),
                                                                  "CertificateAuthority",
                                                                  3600,
                                                                  identityCertChainMasterCA[1])) << "Failed to create CA cert";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SetCAFlagOnCert(busUsedAsCA, identityCertChainMasterCA[1])) << "Failed to set CA flag on CA's cert";

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateIdentityCert(busUsedAsCA,
                                                                  "0",
                                                                  managerGuid.ToString(),
                                                                  managerKey.GetPublicKey(),
                                                                  "ManagerAlias",
                                                                  3600,
                                                                  identityCertChainMasterCA[0])) << "Failed to create identity certificate.";

    managerBus.GetPermissionConfigurator().SetApplicationState(PermissionConfigurator::CLAIMABLE);
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::SignManifest(busUsedAsCA, identityCertChainMasterCA[0], manifests[0]));
    ASSERT_EQ(ER_OK, sapWithManager.Claim(caKey,
                                          managerGuid,
                                          managerKey,
                                          identityCertChainMasterCA, certChainSize,
                                          manifests, ArraySize(manifests)));

    ASSERT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &managerAuthListener));

    qcc::MembershipCertificate managerMembershipCertificate[2];

    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-1",
                                                                    busUsedAsCA,
                                                                    managerGuid.ToString(),
                                                                    managerKey.GetPublicKey(),
                                                                    managerGuid,
                                                                    true,
                                                                    3600,
                                                                    managerMembershipCertificate[1]
                                                                    ));
    ASSERT_EQ(ER_OK, PermissionMgmtTestHelper::CreateMembershipCert("0-0",
                                                                    managerBus,
                                                                    managerGuid.ToString(),
                                                                    managerKey.GetPublicKey(),
                                                                    managerGuid,
                                                                    false,
                                                                    3600,
                                                                    managerMembershipCertificate[0]
                                                                    ));

    ASSERT_EQ(ER_OK, sapWithManager.InstallMembership(managerMembershipCertificate, 2));
    ASSERT_EQ(ER_OK, sapWithManager.SecureConnection());

    /*
     * Add a PEER_FROM_CERTIFICATE_AUTHORITY clause with CA1 key
     * to the manager's policy via a remote call.
     */
    PermissionPolicy managerPolicy;
    managerPolicy.SetVersion(2);
    {
        PermissionPolicy::Acl acls[1];
        {
            PermissionPolicy::Peer peers[1];
            peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
            peers[0].SetKeyInfo(&caKey);
            acls[0].SetPeers(1, peers);
        }
        managerPolicy.SetAcls(1, acls);
    }
    {
        PermissionPolicy defaultPolicy;
        sapWithManager.GetDefaultPolicy(defaultPolicy);
        UpdatePolicyWithValuesFromDefaultPolicy(defaultPolicy, managerPolicy, true, true, true);
        EXPECT_EQ(ER_OK, sapWithManager.UpdatePolicy(managerPolicy)); // This currently fails
        EXPECT_EQ(ER_OK, sapWithManager.SecureConnection(true));
    }

    EXPECT_EQ(ER_OK, managerBus.UnbindSessionPort(managerSessionPort));
    EXPECT_EQ(ER_OK, managerBus.Stop());
    EXPECT_EQ(ER_OK, managerBus.Join());
}
