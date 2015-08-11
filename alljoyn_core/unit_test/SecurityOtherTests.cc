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

/**
 * This is a collection of misc. test cases that did not fit into another
 * category but are still related to security2.0 feature.
 * most of the tests are related to backward compatibility.
 */

class SecurityOtherTestSessionPortListener : public SessionPortListener {
  public:
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
};

class SecurityOtherTestBusObject : public BusObject {
  public:
    SecurityOtherTestBusObject(BusAttachment& bus, const char* path, const char* interfaceName, bool announce = true)
        : BusObject(path), isAnnounced(announce), prop1(42), prop2(17) {
        const InterfaceDescription* iface = bus.GetInterface(interfaceName);
        EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for " << interfaceName;
        if (iface == NULL) {
            return;
        }

        if (isAnnounced) {
            AddInterface(*iface, ANNOUNCED);
        } else {
            AddInterface(*iface, UNANNOUNCED);
        }

        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { iface->GetMember("Echo"), static_cast<MessageReceiver::MethodHandler>(&SecurityOtherTestBusObject::Echo) }
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

static const char ecdsaPrivateKeyPEM[] = {
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MDECAQEEIICSqj3zTadctmGnwyC/SXLioO39pB1MlCbNEX04hjeioAoGCCqGSM49\n"
    "AwEH\n"
    "-----END EC PRIVATE KEY-----"
};

static const char ecdsaCertChainX509PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBWjCCAQGgAwIBAgIHMTAxMDEwMTAKBggqhkjOPQQDAjArMSkwJwYDVQQDDCAw\n"
    "ZTE5YWZhNzlhMjliMjMwNDcyMGJkNGY2ZDVlMWIxOTAeFw0xNTAyMjYyMTU1MjVa\n"
    "Fw0xNjAyMjYyMTU1MjVaMCsxKTAnBgNVBAMMIDZhYWM5MjQwNDNjYjc5NmQ2ZGIy\n"
    "NmRlYmRkMGM5OWJkMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEP/HbYga30Afm\n"
    "0fB6g7KaB5Vr5CDyEkgmlif/PTsgwM2KKCMiAfcfto0+L1N0kvyAUgff6sLtTHU3\n"
    "IdHzyBmKP6MQMA4wDAYDVR0TBAUwAwEB/zAKBggqhkjOPQQDAgNHADBEAiAZmNVA\n"
    "m/H5EtJl/O9x0P4zt/UdrqiPg+gA+wm0yRY6KgIgetWANAE2otcrsj3ARZTY/aTI\n"
    "0GOQizWlQm8mpKaQ3uE=\n"
    "-----END CERTIFICATE-----"
};

class SecurityOtherECDHE_ECDSAAuthListener : public AuthListener {
  public:
    SecurityOtherECDHE_ECDSAAuthListener() :
        requestCredentialsCalled(false),
        verifyCredentialsCalled(false),
        authenticationSuccessfull(false),
        securityViolationCalled(false)
    {
    }

    QStatus RequestCredentialsAsync(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, void* context)
    {
        QCC_UNUSED(authPeer);
        QCC_UNUSED(authCount);
        QCC_UNUSED(userId);
        QCC_UNUSED(credMask);
        requestCredentialsCalled = true;
        Credentials creds;
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
            if ((credMask& AuthListener::CRED_PRIVATE_KEY) == AuthListener::CRED_PRIVATE_KEY) {
                String pk(ecdsaPrivateKeyPEM, strlen(ecdsaPrivateKeyPEM));
                creds.SetPrivateKey(pk);
            }
            if ((credMask& AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN) {
                String cert(ecdsaCertChainX509PEM, strlen(ecdsaCertChainX509PEM));
                creds.SetCertChain(cert);
            }
            return RequestCredentialsResponse(context, true, creds);
        }
        return RequestCredentialsResponse(context, false, creds);
    }
    QStatus VerifyCredentialsAsync(const char* authMechanism, const char* authPeer, const Credentials& creds, void* context) {
        QCC_UNUSED(authMechanism);
        QCC_UNUSED(authPeer);
        QCC_UNUSED(creds);
        verifyCredentialsCalled = true;
        if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
            if (creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
                return VerifyCredentialsResponse(context, true);
            }
        }
        return VerifyCredentialsResponse(context, false);
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        QCC_UNUSED(authMechanism);
        QCC_UNUSED(authPeer);
        QCC_UNUSED(success);
        if (success) {
            authenticationSuccessfull = true;
        }
    }

    void SecurityViolation(QStatus status, const Message& msg) {
        QCC_UNUSED(status);
        QCC_UNUSED(msg);
        securityViolationCalled = true;
    }
    bool requestCredentialsCalled;
    bool verifyCredentialsCalled;
    bool authenticationSuccessfull;
    bool securityViolationCalled;

};

/*
 * Purpose:
 * Two peers are not claimed can make method call over a ECDHE_ECDSA session.
 *
 * Setup:
 * Two peers who are not claimed make method call over an ECDHE_ECDSA session.
 *
 * Verification:
 * Verify that method call is successful. Verify that application provides
 * credentials via RequestCredentials callback.
 *
 * This test helps verify backward compatibility since it is using ECDHE_ECDSA
 * over using Security1.0.
 */
// Currently this is seeing the same issue as already reported for ASACORE-2287
// re-enable and Delete this comment once issue is fixed.
TEST(SecurityOtherTest, methodCallOverECDHE_ECDSASession) {
    BusAttachment peer1Bus("SecurityOtherPeer1", true);
    BusAttachment peer2Bus("SecurityOtherPeer2", true);

    EXPECT_EQ(ER_OK, peer1Bus.Start());
    EXPECT_EQ(ER_OK, peer1Bus.Connect());
    EXPECT_EQ(ER_OK, peer2Bus.Start());
    EXPECT_EQ(ER_OK, peer2Bus.Connect());

    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;

    // Register in memory keystore listeners
    EXPECT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
    EXPECT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));

    SecurityOtherECDHE_ECDSAAuthListener peer1AuthListener;
    SecurityOtherECDHE_ECDSAAuthListener peer2AuthListener;

    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

    qcc::String interface = "<node>"
                            "<interface name='org.allseen.test.security.other'>"
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

    SecurityOtherTestBusObject peer2BusObject(peer2Bus, "/test", "org.allseen.test.security.other");
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2BusObject, true));

    SessionOpts opts;
    SessionPort sessionPort = 42;
    SecurityOtherTestSessionPortListener sessionPortListener;
    EXPECT_EQ(ER_OK, peer2Bus.BindSessionPort(sessionPort, opts, sessionPortListener));

    uint32_t sessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), sessionPort, NULL, sessionId, opts));

    ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test", sessionId, true);
    EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
    EXPECT_TRUE(proxy.ImplementsInterface("org.allseen.test.security.other")) << interface.c_str() << "\n";

    MsgArg arg("s", "String that should be Echoed back.");
    Message replyMsg(peer1Bus);
    EXPECT_EQ(ER_OK, proxy.MethodCall("org.allseen.test.security.other", "Echo", &arg, static_cast<size_t>(1), replyMsg));

    char* echoReply;
    replyMsg->GetArg(0)->Get("s", &echoReply);
    EXPECT_STREQ("String that should be Echoed back.", echoReply);

    EXPECT_TRUE(peer1AuthListener.requestCredentialsCalled);
    EXPECT_TRUE(peer1AuthListener.verifyCredentialsCalled);
    EXPECT_TRUE(peer1AuthListener.authenticationSuccessfull);
    EXPECT_FALSE(peer1AuthListener.securityViolationCalled);

    EXPECT_TRUE(peer2AuthListener.requestCredentialsCalled);
    EXPECT_TRUE(peer2AuthListener.verifyCredentialsCalled);
    EXPECT_TRUE(peer2AuthListener.authenticationSuccessfull);
    EXPECT_FALSE(peer2AuthListener.securityViolationCalled);
}

/*
 * Purpose:
 * Unsecure messages are not checked against rules. Test for Method calls,
 * get property calls, set property calls, sessionless signals.
 *
 * Setup:
 * Install policies on two peers that denies everything.
 *
 * Peer 1 makes a method call, get property, set property, get all properties call  to app. bus.
 * Peer 1 sends a signal to app. bus.
 * Peer 1 sends a store-and-forward signal to the app. bus
 *
 * Verification:
 * Verify that method call, get property, set property, get all properties call
 * to app. bus. are successful.
 * Verify that the signal is received by the app. bus.
 * Verify that the store-and-forward signal is received by the app. bus.
 */
TEST(SecurityOtherTest, unsecure_messages_not_blocked_by_policies_rules) {
    BusAttachment managerBus("SecurityOtherManager", true);
    BusAttachment peer1Bus("SecurityOtherPeer1", true);
    BusAttachment peer2Bus("SecurityOtherPeer2", true);

    EXPECT_EQ(ER_OK, managerBus.Start());
    EXPECT_EQ(ER_OK, managerBus.Connect());
    EXPECT_EQ(ER_OK, peer1Bus.Start());
    EXPECT_EQ(ER_OK, peer1Bus.Connect());
    EXPECT_EQ(ER_OK, peer2Bus.Start());
    EXPECT_EQ(ER_OK, peer2Bus.Connect());

    InMemoryKeyStoreListener managerKeyStoreListener;
    InMemoryKeyStoreListener peer1KeyStoreListener;
    InMemoryKeyStoreListener peer2KeyStoreListener;

    // Register in memory keystore listeners
    EXPECT_EQ(ER_OK, managerBus.RegisterKeyStoreListener(managerKeyStoreListener));
    EXPECT_EQ(ER_OK, peer1Bus.RegisterKeyStoreListener(peer1KeyStoreListener));
    EXPECT_EQ(ER_OK, peer2Bus.RegisterKeyStoreListener(peer2KeyStoreListener));

    DefaultECDHEAuthListener managerAuthListener;
    DefaultECDHEAuthListener peer1AuthListener;
    DefaultECDHEAuthListener peer2AuthListener;

    EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

    qcc::String interface = "<node>"
                            "<interface name='org.allseen.test.security.other.secure'>"
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
                            "<interface name='org.allseen.test.security.other.insecure'>"
                            "<annotation name='org.alljoyn.Bus.Secure' value='false'/>"
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

    SecurityOtherTestBusObject peer1SecureBusObject(peer1Bus, "/test/secure", "org.allseen.test.security.other.secure");
    SecurityOtherTestBusObject peer1InsecureBusObject(peer1Bus, "/test/insecure", "org.allseen.test.security.other.insecure");
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1SecureBusObject, true));
    EXPECT_EQ(ER_OK, peer1Bus.RegisterBusObject(peer1InsecureBusObject));

    SecurityOtherTestBusObject peer2SecureBusObject(peer2Bus, "/test/secure", "org.allseen.test.security.other.secure");
    SecurityOtherTestBusObject peer2InsecureBusObject(peer2Bus, "/test/insecure", "org.allseen.test.security.other.insecure");
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2SecureBusObject, true));
    EXPECT_EQ(ER_OK, peer2Bus.RegisterBusObject(peer2InsecureBusObject));

    SessionOpts opts1;
    SessionId managerToManagerSessionId;
    SessionPort managerSessionPort = 42;
    SecurityOtherTestSessionPortListener managerSessionPortListener;
    EXPECT_EQ(ER_OK, managerBus.BindSessionPort(managerSessionPort, opts1, managerSessionPortListener));

    SessionOpts opts2;
    SessionId managerToPeer1SessionId;
    SessionPort peer1SessionPort = 42;
    SecurityOtherTestSessionPortListener peer1SessionPortListener;
    EXPECT_EQ(ER_OK, peer1Bus.BindSessionPort(peer1SessionPort, opts2, peer1SessionPortListener));

    SessionOpts opts3;
    SessionId managerToPeer2SessionId;
    SessionPort peer2SessionPort = 42;
    SecurityOtherTestSessionPortListener peer2SessionPortListener;
    EXPECT_EQ(ER_OK, peer2Bus.BindSessionPort(peer2SessionPort, opts3, peer2SessionPortListener));

    EXPECT_EQ(ER_OK, managerBus.JoinSession(managerBus.GetUniqueName().c_str(), managerSessionPort, NULL, managerToManagerSessionId, opts1));
    EXPECT_EQ(ER_OK, managerBus.JoinSession(peer1Bus.GetUniqueName().c_str(), peer1SessionPort, NULL, managerToPeer1SessionId, opts2));
    EXPECT_EQ(ER_OK, managerBus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, managerToPeer2SessionId, opts3));

    //-----------------------Claim each bus Attachments------------------
    SecurityApplicationProxy sapWithManager(managerBus, managerBus.GetUniqueName().c_str(), managerToManagerSessionId);
    SecurityApplicationProxy sapWithPeer1(managerBus, peer1Bus.GetUniqueName().c_str(), managerToPeer1SessionId);
    SecurityApplicationProxy sapWithPeer2(managerBus, peer2Bus.GetUniqueName().c_str(), managerToPeer2SessionId);


    // All Inclusive manifest
    const size_t manifestSize = 1;
    PermissionPolicy::Rule manifest[manifestSize];
    manifest[0].SetObjPath("*");
    manifest[0].SetInterfaceName("*");
    {
        PermissionPolicy::Rule::Member member[1];
        member[0].Set("*",
                      PermissionPolicy::Rule::Member::NOT_SPECIFIED,
                      PermissionPolicy::Rule::Member::ACTION_PROVIDE |
                      PermissionPolicy::Rule::Member::ACTION_MODIFY |
                      PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        manifest[0].SetMembers(1, member);
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

    //------------ Claim self(managerBus), Peer1, and Peer2 --------
    //Random GUID used for the SecurityManager
    GUID128 managerGuid;

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


    ECCPublicKey managerPublicKey;
    sapWithManager.GetEccPublicKey(managerPublicKey);
    ASSERT_EQ(*managerKey.GetPublicKey(), managerPublicKey);

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

    EXPECT_EQ(ER_OK, managerBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &managerAuthListener));
    EXPECT_EQ(ER_OK, peer1Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer1AuthListener));
    EXPECT_EQ(ER_OK, peer2Bus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &peer2AuthListener));

    //--------- InstallMembership certificates on self, peer1, and peer2

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
    EXPECT_EQ(ER_OK, sapWithPeer2.InstallMembership(peer2MembershipCertificate, 1));

    //----------------Install Policy that denies everything--------------
    // To deny everything we just install a policy that is identical to the
    // default policy but removes the AdminGroup entry and the InstallMembership
    // Entry.  This is a policy that will deny eveything.
    {
        PermissionPolicy policy;
        policy.SetVersion(1);
        PermissionPolicy peer1DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer1.GetDefaultPolicy(peer1DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer1DefaultPolicy, policy);
        EXPECT_EQ(ER_OK, sapWithPeer1.UpdatePolicy(policy));
    }
    {
        PermissionPolicy policy;
        policy.SetVersion(1);
        PermissionPolicy peer2DefaultPolicy;
        EXPECT_EQ(ER_OK, sapWithPeer2.GetDefaultPolicy(peer2DefaultPolicy));
        UpdatePolicyWithValuesFromDefaultPolicy(peer2DefaultPolicy, policy);
        EXPECT_EQ(ER_OK, sapWithPeer2.UpdatePolicy(policy));
    }

    uint32_t sessionId;
    EXPECT_EQ(ER_OK, peer1Bus.JoinSession(peer2Bus.GetUniqueName().c_str(), peer2SessionPort, NULL, sessionId, opts2));
    {
        // secure methods should fail
        // Try to make method call, get/set properties and send a signal the
        // policy should deny all interaction with the secure interface.  This
        // is done to verify we have a policy that denies everything.
        ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test/secure", sessionId, true);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface("org.allseen.test.security.other.secure")) << interface.c_str() << "\n";
        MsgArg arg("s", "String that should be Echoed back.");
        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.MethodCall("org.allseen.test.security.other.secure", "Echo", &arg, static_cast<size_t>(1), replyMsg));

        MsgArg prop1Arg;
        EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.SetProperty("org.allseen.test.security.other.secure", "Prop1", prop1Arg));

        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetProperty("org.allseen.test.security.other.secure", "Prop1", prop1Arg));

        MsgArg props;
        EXPECT_EQ(ER_PERMISSION_DENIED, proxy.GetAllProperties("org.allseen.test.security.other.secure", props));

        MsgArg signalArg("s", "Chirp this string out in the signal.");
        // Signals are send and forget.  They will always return ER_OK.
        EXPECT_EQ(ER_PERMISSION_DENIED, peer1SecureBusObject.Signal(peer2Bus.GetUniqueName().c_str(),
                                                                    sessionId,
                                                                    *peer1Bus.GetInterface("org.allseen.test.security.other.secure")->GetMember("Chirp"),
                                                                    &signalArg, 1));
    }
    {
        // insecure methods should pass
        // Permission policies should no effect insecure interfaces.
        ProxyBusObject proxy(peer1Bus, peer2Bus.GetUniqueName().c_str(), "/test/insecure", sessionId);
        EXPECT_EQ(ER_OK, proxy.ParseXml(interface.c_str()));
        EXPECT_TRUE(proxy.ImplementsInterface("org.allseen.test.security.other.insecure")) << interface.c_str() << "\n";
        MsgArg arg("s", "String that should be Echoed back.");
        Message replyMsg(peer1Bus);
        EXPECT_EQ(ER_OK, proxy.MethodCall("org.allseen.test.security.other.insecure", "Echo", &arg, static_cast<size_t>(1), replyMsg));

        char* echoReply;
        replyMsg->GetArg(0)->Get("s", &echoReply);
        EXPECT_STREQ("String that should be Echoed back.", echoReply);

        MsgArg prop1Arg;
        EXPECT_EQ(ER_OK, prop1Arg.Set("i", 513));
        EXPECT_EQ(ER_OK, proxy.SetProperty("org.allseen.test.security.other.insecure", "Prop1", prop1Arg));

        EXPECT_EQ(513, peer2InsecureBusObject.ReadProp1());

        EXPECT_EQ(ER_OK, proxy.GetProperty("org.allseen.test.security.other.insecure", "Prop1", prop1Arg));

        int32_t prop1;
        prop1Arg.Get("i", &prop1);
        EXPECT_EQ(513, prop1);

        MsgArg props;
        EXPECT_EQ(ER_OK, proxy.GetAllProperties("org.allseen.test.security.other.insecure", props));
        {
            int32_t prop1;
            MsgArg* propArg;
            EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop1", &propArg)) << props.ToString().c_str();
            EXPECT_EQ(ER_OK, propArg->Get("i", &prop1)) << propArg->ToString().c_str();
            EXPECT_EQ(513, prop1);
        }
        {
            int32_t prop2;
            MsgArg* propArg;
            EXPECT_EQ(ER_OK, props.GetElement("{sv}", "Prop2", &propArg)) << props.ToString().c_str();
            EXPECT_EQ(ER_OK, propArg->Get("i", &prop2)) << propArg->ToString().c_str();
            EXPECT_EQ(17, prop2);
        }

        ChirpSignalReceiver chirpSignalReceiver;
        EXPECT_EQ(ER_OK, peer2Bus.RegisterSignalHandler(&chirpSignalReceiver,
                                                        static_cast<MessageReceiver::SignalHandler>(&ChirpSignalReceiver::ChirpSignalHandler),
                                                        peer1Bus.GetInterface("org.allseen.test.security.other.insecure")->GetMember("Chirp"),
                                                        NULL));

        MsgArg signalArg("s", "Chirp this string out in the signal.");
        // Signals are send and forget.  They will always return ER_OK.
        EXPECT_EQ(ER_OK, peer1InsecureBusObject.Signal(peer2Bus.GetUniqueName().c_str(),
                                                       sessionId,
                                                       *peer1Bus.GetInterface("org.allseen.test.security.other.insecure")->GetMember("Chirp"),
                                                       &signalArg, 1));

        //Wait for a maximum of 2 sec for the Chirp Signal.
        for (int msec = 0; msec < 2000; msec += WAIT_MSECS) {
            if (chirpSignalReceiver.signalReceivedFlag) {
                break;
            }
            qcc::Sleep(WAIT_MSECS);
        }

        EXPECT_TRUE(chirpSignalReceiver.signalReceivedFlag);
    }
}
