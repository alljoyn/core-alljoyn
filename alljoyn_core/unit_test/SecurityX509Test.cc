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
#include "ajTestCommon.h"
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/DBusStd.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/CertificateECC.h>
#include <qcc/time.h>
#include <qcc/Crypto.h>
#include "InMemoryKeyStore.h"

using namespace ajn;
using namespace qcc;

static const char* interface1 = "org.alljoyn.security.interface1";
static const char* object_path = "/security";

class SecureServiceTestObject : public BusObject {

  public:

    SecureServiceTestObject(const char* path, BusAttachment& mBus) :
        BusObject(path),
        msgEncrypted(false),
        bus(mBus)
    {
        QStatus status = ER_OK;
        /* Add interface1 to the BusObject. */
        const InterfaceDescription* Intf1 = mBus.GetInterface(interface1);
        EXPECT_TRUE(Intf1 != NULL);
        if (Intf1 != NULL) {
            AddInterface(*Intf1);

            /* Register the method handlers with the object */
            const MethodEntry methodEntries[] = {
                { Intf1->GetMember("my_ping"), static_cast<MessageReceiver::MethodHandler>(&SecureServiceTestObject::Ping) },
            };
            status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
            EXPECT_EQ(ER_OK, status);
        }
    }

    void Ping(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);
        char* value = NULL;
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = arg->Get("s", &value);
        EXPECT_EQ(ER_OK, status);
        if (msg->IsEncrypted()) {
            msgEncrypted = true;
        }
        status = MethodReply(msg, arg, 1);
        EXPECT_EQ(ER_OK, status) << "Ping: Error sending reply";
    }

    bool msgEncrypted;
    BusAttachment& bus;

};


class SecurityX509Test : public testing::Test {
  public:
    SecurityX509Test() :
        clientbus("SecureClient", false),
        servicebus("SecureService", false),
        status(ER_OK)
    { };

    virtual void SetUp() {
        status = clientbus.Start();
        ASSERT_EQ(ER_OK, status);
        status = clientbus.Connect();
        ASSERT_EQ(ER_OK, status);
        ASSERT_EQ(ER_OK, clientbus.RegisterKeyStoreListener(clientKeyStoreListener));

        status = servicebus.Start();
        ASSERT_EQ(ER_OK, status);
        status = servicebus.Connect();
        ASSERT_EQ(ER_OK, status);
        ASSERT_EQ(ER_OK, servicebus.RegisterKeyStoreListener(serviceKeyStoreListener));

        //Create interface and add a method to the service and client bus attachments.
        InterfaceDescription* Intf1 = NULL;
        status = servicebus.CreateInterface(interface1, Intf1);
        ASSERT_EQ(ER_OK, status);
        ASSERT_TRUE(Intf1 != NULL);
        status = Intf1->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
        ASSERT_EQ(ER_OK, status);
        Intf1->Activate();

        //Register the service bus object
        serviceObject = new SecureServiceTestObject(object_path, servicebus);
        status = servicebus.RegisterBusObject(*serviceObject, false);
        ASSERT_EQ(ER_OK, status);

        InterfaceDescription* Intf2 = NULL;
        status = clientbus.CreateInterface(interface1, Intf2);
        ASSERT_EQ(ER_OK, status);
        ASSERT_TRUE(Intf2 != NULL);
        status = Intf2->AddMethod("my_ping", "s", "s", "inStr,outStr", 0);
        ASSERT_EQ(ER_OK, status);
        Intf2->Activate();

        clientProxyObject = new ProxyBusObject(clientbus, servicebus.GetUniqueName().c_str(), object_path, 0, false);
        status = clientProxyObject->IntrospectRemoteObject();
        ASSERT_EQ(ER_OK, status);
    }

    virtual void TearDown() {

        clientbus.UnregisterKeyStoreListener();
        servicebus.UnregisterKeyStoreListener();
        servicebus.UnregisterBusObject(*serviceObject);

        status = clientbus.Disconnect();
        EXPECT_EQ(ER_OK, status);
        status = servicebus.Disconnect();
        EXPECT_EQ(ER_OK, status);
        status = clientbus.Stop();
        EXPECT_EQ(ER_OK, status);
        status = servicebus.Stop();
        EXPECT_EQ(ER_OK, status);
        status = clientbus.Join();
        EXPECT_EQ(ER_OK, status);
        status = servicebus.Join();
        EXPECT_EQ(ER_OK, status);

        delete clientProxyObject;
        delete serviceObject;
    }

    BusAttachment clientbus;
    BusAttachment servicebus;
    InMemoryKeyStoreListener clientKeyStoreListener;
    InMemoryKeyStoreListener serviceKeyStoreListener;
    SecureServiceTestObject*serviceObject;
    ProxyBusObject*clientProxyObject;
    QStatus status;

};


class ECDSAAuthListener : public AuthListener {


  private:

    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {

        QCC_UNUSED(userId);
        requestCredentialsCalled++;
        EXPECT_FALSE(credMask & AuthListener::CRED_PASSWORD);
        EXPECT_TRUE(credMask & AuthListener::CRED_PRIVATE_KEY);
        EXPECT_TRUE(credMask & AuthListener::CRED_CERT_CHAIN);
        EXPECT_STREQ("ALLJOYN_ECDHE_ECDSA", authMechanism);
        EXPECT_EQ((uint16_t)1, authCount);
        requestCredentialsAuthPeer = authPeer;
        if (!requestCredentialsPrivateKey.empty()) {
            creds.SetPrivateKey(requestCredentialsPrivateKey);
        }
        if (!requestCredentialsX509CertChain.empty()) {
            creds.SetCertChain(requestCredentialsX509CertChain);
        }

        return requestCredentialsReturn;
    }

    bool VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds) {
        verifyCredentialsCalled++;
        verifyCredentialsAuthPeer = authPeer;
        EXPECT_STREQ("ALLJOYN_ECDHE_ECDSA", authMechanism);
        verifyCredentialsX509CertChain = creds.GetCertChain();
        return verifyCredentialsReturn;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        QCC_UNUSED(authPeer);
        authenticationCompleteCalled++;
        EXPECT_STREQ("ALLJOYN_ECDHE_ECDSA", authMechanism);
        authenticationResult = success;
    }

  public:
    uint32_t requestCredentialsCalled;
    uint32_t verifyCredentialsCalled;
    uint32_t authenticationCompleteCalled;
    bool authenticationResult;
    uint32_t securityViolationCalled;
    QStatus securityViolationStatus;
    String requestCredentialsAuthPeer;
    String verifyCredentialsAuthPeer;
    String verifyCredentialsX509CertChain;
    String requestCredentialsPrivateKey;
    String requestCredentialsX509CertChain;
    bool requestCredentialsReturn;
    bool verifyCredentialsReturn;

    ECDSAAuthListener() {
    }


    ECDSAAuthListener(const char*requestCredentialsPrivateKey, const char*requestCredentialsX509CertChain, bool requestCredentialsReturn, bool verifyCredentialsReturn) {
        requestCredentialsCalled = 0;
        verifyCredentialsCalled = 0;
        authenticationCompleteCalled = 0;
        authenticationResult = false;
        securityViolationCalled = 0;
        securityViolationStatus = ER_OK;
        this->requestCredentialsPrivateKey = requestCredentialsPrivateKey;
        this->requestCredentialsX509CertChain = requestCredentialsX509CertChain;
        this->requestCredentialsReturn = requestCredentialsReturn;
        this->verifyCredentialsReturn = verifyCredentialsReturn;
    }

};

static ECDSAAuthListener g_ecdsaAuthListenerForService;
static ECDSAAuthListener g_ecdsaAuthListenerForClient;

/* Client makes a method call.
   Client provides ECDSA key/X509 cert in PEM.
   Service provides ECDSA key/X509 cert in PEM.
   The key/certs are generated using OpenSSL commands.
   Auth should succeed.
 */
static char privateKey_from_openssl_PEM[] = {
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIAqN6AtyOAPxY5k7eFNXAwzkbsGMl4uqvPrYkIj0LNZBoAoGCCqGSM49\n"
    "AwEHoUQDQgAEvnRd4fX9opwgXX4Em2UiCMsBbfaqhB1U5PJCDZacz9HumDEzYdrS\n"
    "MymSxR34lL0GJVgEECvBTvpaHP2bpTIl6g==\n"
    "-----END EC PRIVATE KEY-----"
};

static char cert_from_openssl_PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n"
    "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n"
    "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n"
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAKfmglMgl67L5ALF\n"
    "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n"
    "IizUeK0oI5c=\n"
    "-----END CERTIFICATE-----"
};

TEST_F(SecurityX509Test, Test1) {
    const char*clientecdsaPrivateKeyPEM = privateKey_from_openssl_PEM;
    const char*clientecdsaCertChainX509PEM = cert_from_openssl_PEM;

    const char serviceecdsaPrivateKeyPEM[] = {
        "-----BEGIN EC PRIVATE KEY-----\n"
        "MHcCAQEEIB3ugUBAsT0qhMBw3OePiicJf/le+AT0d0Sn7kJMSn3toAoGCCqGSM49\n"
        "AwEHoUQDQgAEJ63ir6VW/w7DlgeKi1Ylaqomfk00oRiE69q6KKSk/r9JCpnrZY/Z\n"
        "Vcp53/8TiQWbXvt3cz8k1/h08qMmtUMPOg==\n"
        "-----END EC PRIVATE KEY-----"
    };

    const char serviceecdsaCertChainX509PEM[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBkjCCATmgAwIBAgIJAO5pmFr3abYcMAoGCCqGSM49BAMCMD4xETAPBgNVBAsM\n"
        "CFF1YWxjb21tMSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
        "Y2M1NjAeFw0xNTAyMjUwMDQ2MjlaFw0xNjAyMjUwMDQ2MjlaMD4xETAPBgNVBAsM\n"
        "CFF1YWxjb21tMSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
        "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABCet4q+lVv8Ow5YHiotWJWqq\n"
        "Jn5NNKEYhOvauiikpP6/SQqZ62WP2VXKed//E4kFm177d3M/JNf4dPKjJrVDDzqj\n"
        "IDAeMAkGA1UdEwQCMAAwEQYKKwYBBAGC3nwBAQQDAgEBMAoGCCqGSM49BAMCA0cA\n"
        "MEQCIBu1WTN3rDKnJvMKnUT0rFzq6IpWN7X3ZheiHXI4We2XAiACVY7Vd+hrzL8r\n"
        "JbIKcG4ZWLcFx8oQ5x6ghdgvfUMzCA==\n"
        "-----END CERTIFICATE-----"
    };

    //Enable Peer security and reg. auth listener
    ECDSAAuthListener ecdsaAuthListenerForService(serviceecdsaPrivateKeyPEM, serviceecdsaCertChainX509PEM, true, true);
    ECDSAAuthListener ecdsaAuthListenerForClient(clientecdsaPrivateKeyPEM, clientecdsaCertChainX509PEM, true, true);
    g_ecdsaAuthListenerForService = ecdsaAuthListenerForService;
    g_ecdsaAuthListenerForClient = ecdsaAuthListenerForClient;
    status = servicebus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForService, NULL, false);
    EXPECT_EQ(ER_OK, status);
    status = clientbus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForClient, NULL, false);
    EXPECT_EQ(ER_OK, status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject->GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status);
    status = clientProxyObject->MethodCall(*pingMethod, &pingArgs, 1, reply, 5000, ALLJOYN_FLAG_ENCRYPTED);
    ASSERT_EQ(ER_OK, status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject->msgEncrypted);

    //check for AuthListener details on service side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.requestCredentialsCalled);
    EXPECT_STREQ(clientbus.GetUniqueName().c_str(), g_ecdsaAuthListenerForService.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.verifyCredentialsCalled);
    EXPECT_STREQ(clientbus.GetUniqueName().c_str(), g_ecdsaAuthListenerForService.verifyCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.securityViolationCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.authenticationCompleteCalled);
    EXPECT_TRUE(g_ecdsaAuthListenerForService.authenticationResult);
    String der1;
    Crypto_ASN1::DecodeBase64(g_ecdsaAuthListenerForService.verifyCredentialsX509CertChain, der1);
    String der2;
    Crypto_ASN1::DecodeBase64(clientecdsaCertChainX509PEM, der2);
    EXPECT_TRUE(der1 == der2);

    //check for AuthListener details on client side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.requestCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.verifyCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.verifyCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForClient.securityViolationCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.authenticationCompleteCalled);
    EXPECT_TRUE(g_ecdsaAuthListenerForClient.authenticationResult);
    String der3;
    Crypto_ASN1::DecodeBase64(g_ecdsaAuthListenerForClient.verifyCredentialsX509CertChain, der3);
    String der4;
    Crypto_ASN1::DecodeBase64(serviceecdsaCertChainX509PEM, der4);
    EXPECT_TRUE(der3 == der4);

}


/* Client makes a method call.
   Client provides ECDSA key/X509 cert in PEM.
   Service provides ECDSA key/SPKI cert in PEM.
   Auth should fail as SPKI format is not supported anymore.
 */

TEST_F(SecurityX509Test, Test2) {

    const char*clientecdsaPrivateKeyPEM = privateKey_from_openssl_PEM;
    const char*clientecdsaCertChainX509PEM = cert_from_openssl_PEM;

    /* The spki based keys and certificates are copied from 14.12 release which supported SPKI format.
       However, the private key should begin and end in "-----BEGIN EC PRIVATE KEY-----, -----END EC PRIVATE KEY-----" format
       for it to be recongnised by AllJoyn library. */
    const char spkiserviceecdsaPrivateKeyPEM[] = {
        "-----BEGIN EC PRIVATE KEY-----\n"
        "tV/tGPp7kI0pUohc+opH1LBxzk51pZVM/RVKXHGFjAcAAAAA\n"
        "-----END EC PRIVATE KEY-----"
    };

    const char spkiserviceecdsaCertChainType1PEM[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "AAAAAfUQdhMSDuFWahMG/rFmFbKM06BjIA2Scx9GH+ENLAgtAAAAAIbhHnjAyFys\n"
        "6DoN2kKlXVCgtHpFiEYszOYXI88QDvC1AAAAAAAAAAC5dRALLg6Qh1J2pVOzhaTP\n"
        "xI+v/SKMFurIEo2b4S8UZAAAAADICW7LLp1pKlv6Ur9+I2Vipt5dDFnXSBiifTmf\n"
        "irEWxQAAAAAAAAAAAAAAAAABXLAAAAAAAAFd3AABMa7uTLSqjDggO0t6TAgsxKNt\n"
        "+Zhu/jc3s242BE0drPcL4K+FOVJf+tlivskovQ3RfzTQ+zLoBH5ZCzG9ua/dAAAA\n"
        "ACt5bWBzbcaT0mUqwGOVosbMcU7SmhtE7vWNn/ECvpYFAAAAAA==\n"
        "-----END CERTIFICATE-----"
    };

    //Enable Peer security and reg. auth listener
    ECDSAAuthListener ecdsaAuthListenerForService(spkiserviceecdsaPrivateKeyPEM, spkiserviceecdsaCertChainType1PEM, true, true);
    ECDSAAuthListener ecdsaAuthListenerForClient(clientecdsaPrivateKeyPEM, clientecdsaCertChainX509PEM, true, true);
    g_ecdsaAuthListenerForService = ecdsaAuthListenerForService;
    g_ecdsaAuthListenerForClient = ecdsaAuthListenerForClient;
    status = servicebus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForService, NULL, false);
    EXPECT_EQ(ER_OK, status);
    status = clientbus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForClient, NULL, false);
    EXPECT_EQ(ER_OK, status);


    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject->GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status);
    status = clientProxyObject->MethodCall(*pingMethod, &pingArgs, 1, reply, 5000, ALLJOYN_FLAG_ENCRYPTED);
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status);
    EXPECT_STREQ("ER_AUTH_FAIL", reply->GetArg(0)->v_string.str);

    //check for AuthListener details on service side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.requestCredentialsCalled);
    EXPECT_STREQ(clientbus.GetUniqueName().c_str(), g_ecdsaAuthListenerForService.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForService.authenticationResult);

    //check for AuthListener details on client side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.requestCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForClient.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForClient.authenticationResult);

}

/* Client makes a method call.
   Client provides ECDSA key/SPKI cert in PEM.
   Service provides ECDSA key/X509 cert in PEM.
   Auth should fail as SPKI format is not supported anymore.
 */

TEST_F(SecurityX509Test, Test3) {

    const char*serviceecdsaPrivateKeyPEM = privateKey_from_openssl_PEM;
    const char*serviceecdsaCertChainX509PEM = cert_from_openssl_PEM;

    /* The spki based keys and certificates are copied from 14.12 release which supported SPKI format.
       However, the private key should begin and end in "-----BEGIN EC PRIVATE KEY-----, -----END EC PRIVATE KEY-----" format
       for it to be recongnised by AllJoyn library. */
    const char spkiclientecdsaPrivateKeyPEM[] = {
        "-----BEGIN EC PRIVATE KEY-----\n"
        "tV/tGPp7kI0pUohc+opH1LBxzk51pZVM/RVKXHGFjAcAAAAA\n"
        "-----END EC PRIVATE KEY-----"
    };

    const char spkiclientecdsaCertChainType1PEM[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "AAAAAfUQdhMSDuFWahMG/rFmFbKM06BjIA2Scx9GH+ENLAgtAAAAAIbhHnjAyFys\n"
        "6DoN2kKlXVCgtHpFiEYszOYXI88QDvC1AAAAAAAAAAC5dRALLg6Qh1J2pVOzhaTP\n"
        "xI+v/SKMFurIEo2b4S8UZAAAAADICW7LLp1pKlv6Ur9+I2Vipt5dDFnXSBiifTmf\n"
        "irEWxQAAAAAAAAAAAAAAAAABXLAAAAAAAAFd3AABMa7uTLSqjDggO0t6TAgsxKNt\n"
        "+Zhu/jc3s242BE0drPcL4K+FOVJf+tlivskovQ3RfzTQ+zLoBH5ZCzG9ua/dAAAA\n"
        "ACt5bWBzbcaT0mUqwGOVosbMcU7SmhtE7vWNn/ECvpYFAAAAAA==\n"
        "-----END CERTIFICATE-----"
    };


    //Enable Peer security and reg. auth listener
    ECDSAAuthListener ecdsaAuthListenerForService(serviceecdsaPrivateKeyPEM, serviceecdsaCertChainX509PEM, true, true);
    ECDSAAuthListener ecdsaAuthListenerForClient(spkiclientecdsaPrivateKeyPEM, spkiclientecdsaCertChainType1PEM, true, true);
    g_ecdsaAuthListenerForService = ecdsaAuthListenerForService;
    g_ecdsaAuthListenerForClient = ecdsaAuthListenerForClient;
    status = servicebus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForService, NULL, false);
    EXPECT_EQ(ER_OK, status);
    status = clientbus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForClient, NULL, false);
    EXPECT_EQ(ER_OK, status);


    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject->GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status);
    status = clientProxyObject->MethodCall(*pingMethod, &pingArgs, 1, reply, 5000, ALLJOYN_FLAG_ENCRYPTED);
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status);
    EXPECT_STREQ("ER_AUTH_FAIL", reply->GetArg(0)->v_string.str);

    //check for AuthListener details on service side
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.requestCredentialsCalled);
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.authenticationCompleteCalled);

    //check for AuthListener details on client side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.requestCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForClient.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForClient.authenticationResult);

}



/* Client makes a method call.
   Client provides ECDSA key/X509 cert in PEM.
   Service provides RSA key/X509 cert in PEM.
   Auth. should fail as the RSA X509 cert and keys cannot be decoded for ECDHE_ECDSA.
 */

TEST_F(SecurityX509Test, Test4) {

    const char*clientecdsaPrivateKeyPEM = privateKey_from_openssl_PEM;
    const char*clientecdsaCertChainX509PEM = cert_from_openssl_PEM;

    /* The service provides RSA private key/ public certificate. The certifcate is still provided in X509 format.
       The private key/ public certificate for the service side is copied from the test program bbclient.cc.
       However, the private key should begin and end in "-----BEGIN EC PRIVATE KEY-----, -----END EC PRIVATE KEY-----" format
       for it to be recongnised by AllJoyn library. */
    const char servicersaPrivateKeyPEM[] = {
        "-----BEGIN EC PRIVATE KEY-----\n"
        "LSJOp+hEzNDDpIrh2UJ+3CauxWRKvmAoGB3r2hZfGJDrCeawJFqH0iSYEX0n0QEX\n"
        "jfQlV4LHSCoGMiw6uItTof5kHKlbp5aXv4XgQb74nw+2LkftLaTchNs0bW0TiGfQ\n"
        "XIuDNsmnZ5+CiAVYIKzsPeXPT4ZZSAwHsjM7LFmosStnyg4Ep8vko+Qh9TpCdFX8\n"
        "w3tH7qRhfHtpo9yOmp4hV9Mlvx8bf99lXSsFJeD99C5GQV2lAMvpfmM8Vqiq9CQN\n"
        "9OY6VNevKbAgLG4Z43l0SnbXhS+mSzOYLxl8G728C6HYpnn+qICLe9xOIfn2zLjm\n"
        "YaPlQR4MSjHEouObXj1F4MQUS5irZCKgp4oM3G5Ovzt82pqzIW0ZHKvi1sqz/KjB\n"
        "wYAjnEGaJnD9B8lRsgM2iLXkqDmndYuQkQB8fhr+zzcFmqKZ1gLRnGQVXNcSPgjU\n"
        "Y0fmpokQPHH/52u+IgdiKiNYuSYkCfHX1Y3nftHGvWR3OWmw0k7c6+DfDU2fDthv\n"
        "3MUSm4f2quuiWpf+XJuMB11px1TDkTfY85m1aEb5j4clPGELeV+196OECcMm4qOw\n"
        "AYxO0J/1siXcA5o6yAqPwPFYcs/14O16FeXu+yG0RPeeZizrdlv49j6yQR3JLa2E\n"
        "pWiGR6hmnkixzOj43IPJOYXySuFSi7lTMYud4ZH2+KYeK23C2sfQSsKcLZAFATbq\n"
        "DY0TZHA5lbUiOSUF5kgd12maHAMidq9nIrUpJDzafgK9JrnvZr+dVYM6CiPhiuqJ\n"
        "bXvt08wtKt68Ymfcx+l64mwzNLS+OFznEeIjLoaHU4c=\n"
        "-----END EC PRIVATE KEY-----"
    };

    const char servicersaCertChainPEM[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBszCCARwCCQDuCh+BWVBk2DANBgkqhkiG9w0BAQUFADAeMQ0wCwYDVQQKDARN\n"
        "QnVzMQ0wCwYDVQQDDARHcmVnMB4XDTEwMDUxNzE1MTg1N1oXDTExMDUxNzE1MTg1\n"
        "N1owHjENMAsGA1UECgwETUJ1czENMAsGA1UEAwwER3JlZzCBnzANBgkqhkiG9w0B\n"
        "AQEFAAOBjQAwgYkCgYEArSd4r62mdaIRG9xZPDAXfImt8e7GTIyXeM8z49Ie1mrQ\n"
        "h7roHbn931Znzn20QQwFD6pPC7WxStXJVH0iAoYgzzPsXV8kZdbkLGUMPl2GoZY3\n"
        "xDSD+DA3m6krcXcN7dpHv9OlN0D9Trc288GYuFEENpikZvQhMKPDUAEkucQ95Z8C\n"
        "AwEAATANBgkqhkiG9w0BAQUFAAOBgQBkYY6zzf92LRfMtjkKs2am9qvjbqXyDJLS\n"
        "viKmYe1tGmNBUzucDC5w6qpPCTSe23H2qup27///fhUUuJ/ssUnJ+Y77jM/u1O9q\n"
        "PIn+u89hRmqY5GKHnUSZZkbLB/yrcFEchHli3vLo4FOhVVHwpnwLtWSpfBF9fWcA\n"
        "7THIAV79Lg==\n"
        "-----END CERTIFICATE-----"
    };

    //Enable Peer security and reg. auth listener
    ECDSAAuthListener ecdsaAuthListenerForService(servicersaPrivateKeyPEM, servicersaCertChainPEM, true, true);
    ECDSAAuthListener ecdsaAuthListenerForClient(clientecdsaPrivateKeyPEM, clientecdsaCertChainX509PEM, true, true);
    g_ecdsaAuthListenerForService = ecdsaAuthListenerForService;
    g_ecdsaAuthListenerForClient = ecdsaAuthListenerForClient;
    status = servicebus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForService, NULL, false);
    EXPECT_EQ(ER_OK, status);
    status = clientbus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForClient, NULL, false);
    EXPECT_EQ(ER_OK, status);


    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject->GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status);
    status = clientProxyObject->MethodCall(*pingMethod, &pingArgs, 1, reply, 5000, ALLJOYN_FLAG_ENCRYPTED);
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status);
    EXPECT_STREQ("ER_AUTH_FAIL", reply->GetArg(0)->v_string.str);

    //check for AuthListener details on service side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.requestCredentialsCalled);
    EXPECT_STREQ(clientbus.GetUniqueName().c_str(), g_ecdsaAuthListenerForService.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForService.authenticationResult);

    //check for AuthListener details on client side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.requestCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForClient.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForClient.authenticationResult);

}


/* Client makes a method call.
   Client provides RSA key/X509 cert in PEM.
   Service provides ECDSA key/X509 cert in PEM.
   Auth. should fail as the RSA X509 cert and kets cannot be decoded for ECDHE_ECDSA.
 */

TEST_F(SecurityX509Test, Test5) {

    const char*serviceecdsaPrivateKeyPEM = privateKey_from_openssl_PEM;
    const char*serviceecdsaCertChainX509PEM = cert_from_openssl_PEM;

    /* The client provides RSA private key/ public certificate. The certifcate is still provided in X509 format.
          The private key/ public certificate for the client side is copied from the test program bbclient.cc.
          However, the private key should begin and end in "-----BEGIN EC PRIVATE KEY-----, -----END EC PRIVATE KEY-----" format
          for it to be recongnised by AllJoyn library. */
    const char clientrsaPrivateKeyPEM[] = {
        "-----BEGIN EC PRIVATE KEY-----\n"
        "LSJOp+hEzNDDpIrh2UJ+3CauxWRKvmAoGB3r2hZfGJDrCeawJFqH0iSYEX0n0QEX\n"
        "jfQlV4LHSCoGMiw6uItTof5kHKlbp5aXv4XgQb74nw+2LkftLaTchNs0bW0TiGfQ\n"
        "XIuDNsmnZ5+CiAVYIKzsPeXPT4ZZSAwHsjM7LFmosStnyg4Ep8vko+Qh9TpCdFX8\n"
        "w3tH7qRhfHtpo9yOmp4hV9Mlvx8bf99lXSsFJeD99C5GQV2lAMvpfmM8Vqiq9CQN\n"
        "9OY6VNevKbAgLG4Z43l0SnbXhS+mSzOYLxl8G728C6HYpnn+qICLe9xOIfn2zLjm\n"
        "YaPlQR4MSjHEouObXj1F4MQUS5irZCKgp4oM3G5Ovzt82pqzIW0ZHKvi1sqz/KjB\n"
        "wYAjnEGaJnD9B8lRsgM2iLXkqDmndYuQkQB8fhr+zzcFmqKZ1gLRnGQVXNcSPgjU\n"
        "Y0fmpokQPHH/52u+IgdiKiNYuSYkCfHX1Y3nftHGvWR3OWmw0k7c6+DfDU2fDthv\n"
        "3MUSm4f2quuiWpf+XJuMB11px1TDkTfY85m1aEb5j4clPGELeV+196OECcMm4qOw\n"
        "AYxO0J/1siXcA5o6yAqPwPFYcs/14O16FeXu+yG0RPeeZizrdlv49j6yQR3JLa2E\n"
        "pWiGR6hmnkixzOj43IPJOYXySuFSi7lTMYud4ZH2+KYeK23C2sfQSsKcLZAFATbq\n"
        "DY0TZHA5lbUiOSUF5kgd12maHAMidq9nIrUpJDzafgK9JrnvZr+dVYM6CiPhiuqJ\n"
        "bXvt08wtKt68Ymfcx+l64mwzNLS+OFznEeIjLoaHU4c=\n"
        "-----END EC PRIVATE KEY-----"
    };

    const char clientrsaCertChainType1PEM[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBszCCARwCCQDuCh+BWVBk2DANBgkqhkiG9w0BAQUFADAeMQ0wCwYDVQQKDARN\n"
        "QnVzMQ0wCwYDVQQDDARHcmVnMB4XDTEwMDUxNzE1MTg1N1oXDTExMDUxNzE1MTg1\n"
        "N1owHjENMAsGA1UECgwETUJ1czENMAsGA1UEAwwER3JlZzCBnzANBgkqhkiG9w0B\n"
        "AQEFAAOBjQAwgYkCgYEArSd4r62mdaIRG9xZPDAXfImt8e7GTIyXeM8z49Ie1mrQ\n"
        "h7roHbn931Znzn20QQwFD6pPC7WxStXJVH0iAoYgzzPsXV8kZdbkLGUMPl2GoZY3\n"
        "xDSD+DA3m6krcXcN7dpHv9OlN0D9Trc288GYuFEENpikZvQhMKPDUAEkucQ95Z8C\n"
        "AwEAATANBgkqhkiG9w0BAQUFAAOBgQBkYY6zzf92LRfMtjkKs2am9qvjbqXyDJLS\n"
        "viKmYe1tGmNBUzucDC5w6qpPCTSe23H2qup27///fhUUuJ/ssUnJ+Y77jM/u1O9q\n"
        "PIn+u89hRmqY5GKHnUSZZkbLB/yrcFEchHli3vLo4FOhVVHwpnwLtWSpfBF9fWcA\n"
        "7THIAV79Lg==\n"
        "-----END CERTIFICATE-----"
    };

    //Enable Peer security and reg. auth listener
    ECDSAAuthListener ecdsaAuthListenerForService(serviceecdsaPrivateKeyPEM, serviceecdsaCertChainX509PEM, true, true);
    ECDSAAuthListener ecdsaAuthListenerForClient(clientrsaPrivateKeyPEM, clientrsaCertChainType1PEM, true, true);
    g_ecdsaAuthListenerForService = ecdsaAuthListenerForService;
    g_ecdsaAuthListenerForClient = ecdsaAuthListenerForClient;
    status = servicebus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForService, NULL, false);
    EXPECT_EQ(ER_OK, status);
    status = clientbus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForClient, NULL, false);
    EXPECT_EQ(ER_OK, status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject->GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status);
    status = clientProxyObject->MethodCall(*pingMethod, &pingArgs, 1, reply, 5000, ALLJOYN_FLAG_ENCRYPTED);
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status);
    EXPECT_STREQ("ER_AUTH_FAIL", reply->GetArg(0)->v_string.str);

    //check for AuthListener details on service side
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.requestCredentialsCalled);
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.authenticationCompleteCalled);

    //check for AuthListener details on client side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.requestCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForClient.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForClient.authenticationResult);

}


/* Client makes a method call.
   Client provides ECDSA key/X509 cert in PEM.
   Service does not provide private key or certificate.
   Auth. should fail gracefully.
 */

TEST_F(SecurityX509Test, Test6) {

    const char*clientecdsaPrivateKeyPEM = privateKey_from_openssl_PEM;
    const char*clientecdsaCertChainX509PEM = cert_from_openssl_PEM;

    //Enable Peer security and reg. auth listener
    ECDSAAuthListener ecdsaAuthListenerForService("", "", true, true);
    ECDSAAuthListener ecdsaAuthListenerForClient(clientecdsaPrivateKeyPEM, clientecdsaCertChainX509PEM, true, true);
    g_ecdsaAuthListenerForService = ecdsaAuthListenerForService;
    g_ecdsaAuthListenerForClient = ecdsaAuthListenerForClient;
    status = servicebus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForService, NULL, false);
    EXPECT_EQ(ER_OK, status);
    status = clientbus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForClient, NULL, false);
    EXPECT_EQ(ER_OK, status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject->GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status);
    status = clientProxyObject->MethodCall(*pingMethod, &pingArgs, 1, reply, 5000, ALLJOYN_FLAG_ENCRYPTED);
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status);
    EXPECT_STREQ("ER_AUTH_FAIL", reply->GetArg(0)->v_string.str);

    //check for AuthListener details on service side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.requestCredentialsCalled);
    EXPECT_STREQ(clientbus.GetUniqueName().c_str(), g_ecdsaAuthListenerForService.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForService.authenticationResult);

    //check for AuthListener details on client side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.requestCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForClient.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForClient.authenticationResult);

}


/* Client makes a method call.
   Client does not provide private key or certificate.
   Service provides ECDSA key/X509 cert in PEM.
   Auth. should fail gracefully.
 */

TEST_F(SecurityX509Test, Test7) {

    const char*serviceecdsaPrivateKeyPEM = privateKey_from_openssl_PEM;
    const char*serviceecdsaCertChainX509PEM = cert_from_openssl_PEM;

    //Enable Peer security and reg. auth listener
    ECDSAAuthListener ecdsaAuthListenerForService(serviceecdsaPrivateKeyPEM, serviceecdsaCertChainX509PEM, true, true);
    ECDSAAuthListener ecdsaAuthListenerForClient("", "", true, true);
    g_ecdsaAuthListenerForService = ecdsaAuthListenerForService;
    g_ecdsaAuthListenerForClient = ecdsaAuthListenerForClient;
    status = servicebus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForService, NULL, false);
    EXPECT_EQ(ER_OK, status);
    status = clientbus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForClient, NULL, false);
    EXPECT_EQ(ER_OK, status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject->GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status);
    status = clientProxyObject->MethodCall(*pingMethod, &pingArgs, 1, reply, 5000, ALLJOYN_FLAG_ENCRYPTED);
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status);
    EXPECT_STREQ("ER_AUTH_FAIL", reply->GetArg(0)->v_string.str);

    //check for AuthListener details on service side
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.requestCredentialsCalled);
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.authenticationCompleteCalled);

    //check for AuthListener details on client side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.requestCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForClient.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForClient.authenticationResult);

}



/* Client makes a method call.
   Client provides ECDSA key/X509 cert in PEM.
   Service only supplies the private key.
   Auth. should fail gracefully.
 */

TEST_F(SecurityX509Test, Test8) {

    const char*clientecdsaPrivateKeyPEM = privateKey_from_openssl_PEM;
    const char*clientecdsaCertChainX509PEM = cert_from_openssl_PEM;

    const char serviceecdsaPrivateKeyPEM[] = {
        "-----BEGIN EC PRIVATE KEY-----\n"
        "MHcCAQEEIB3ugUBAsT0qhMBw3OePiicJf/le+AT0d0Sn7kJMSn3toAoGCCqGSM49\n"
        "AwEHoUQDQgAEJ63ir6VW/w7DlgeKi1Ylaqomfk00oRiE69q6KKSk/r9JCpnrZY/Z\n"
        "Vcp53/8TiQWbXvt3cz8k1/h08qMmtUMPOg==\n"
        "-----END EC PRIVATE KEY-----"
    };

    //Enable Peer security and reg. auth listener
    ECDSAAuthListener ecdsaAuthListenerForService(serviceecdsaPrivateKeyPEM, "", true, true);
    ECDSAAuthListener ecdsaAuthListenerForClient(clientecdsaPrivateKeyPEM, clientecdsaCertChainX509PEM, true, true);
    g_ecdsaAuthListenerForService = ecdsaAuthListenerForService;
    g_ecdsaAuthListenerForClient = ecdsaAuthListenerForClient;
    status = servicebus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForService, NULL, false);
    EXPECT_EQ(ER_OK, status);
    status = clientbus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForClient, NULL, false);
    EXPECT_EQ(ER_OK, status);


    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject->GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status);
    status = clientProxyObject->MethodCall(*pingMethod, &pingArgs, 1, reply, 5000, ALLJOYN_FLAG_ENCRYPTED);
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status);
    EXPECT_STREQ("ER_AUTH_FAIL", reply->GetArg(0)->v_string.str);

    //check for AuthListener details on service side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.requestCredentialsCalled);
    EXPECT_STREQ(clientbus.GetUniqueName().c_str(), g_ecdsaAuthListenerForService.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForService.authenticationResult);

    //check for AuthListener details on client side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.requestCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForClient.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForClient.authenticationResult);

}


/* Client makes a method call.
   Client only supplies the private key.
   Service provides ECDSA key/X509 cert in PEM.
   Auth. should fail gracefully.
 */

TEST_F(SecurityX509Test, Test9) {

    const char*serviceecdsaPrivateKeyPEM = privateKey_from_openssl_PEM;
    const char*serviceecdsaCertChainX509PEM = cert_from_openssl_PEM;

    const char clientecdsaPrivateKeyPEM[] = {
        "-----BEGIN EC PRIVATE KEY-----\n"
        "MHcCAQEEIB3ugUBAsT0qhMBw3OePiicJf/le+AT0d0Sn7kJMSn3toAoGCCqGSM49\n"
        "AwEHoUQDQgAEJ63ir6VW/w7DlgeKi1Ylaqomfk00oRiE69q6KKSk/r9JCpnrZY/Z\n"
        "Vcp53/8TiQWbXvt3cz8k1/h08qMmtUMPOg==\n"
        "-----END EC PRIVATE KEY-----"
    };

    //Enable Peer security and reg. auth listener
    ECDSAAuthListener ecdsaAuthListenerForService(serviceecdsaPrivateKeyPEM, serviceecdsaCertChainX509PEM, true, true);
    ECDSAAuthListener ecdsaAuthListenerForClient(clientecdsaPrivateKeyPEM, "", true, true);
    g_ecdsaAuthListenerForService = ecdsaAuthListenerForService;
    g_ecdsaAuthListenerForClient = ecdsaAuthListenerForClient;
    status = servicebus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForService, NULL, false);
    EXPECT_EQ(ER_OK, status);
    status = clientbus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForClient, NULL, false);
    EXPECT_EQ(ER_OK, status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject->GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status);
    status = clientProxyObject->MethodCall(*pingMethod, &pingArgs, 1, reply, 5000, ALLJOYN_FLAG_ENCRYPTED);
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status);
    EXPECT_STREQ("ER_AUTH_FAIL", reply->GetArg(0)->v_string.str);

    //check for AuthListener details on service side
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.requestCredentialsCalled);
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.authenticationCompleteCalled);

    //check for AuthListener details on client side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.requestCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForClient.verifyCredentialsCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.authenticationCompleteCalled);
    EXPECT_FALSE(g_ecdsaAuthListenerForClient.authenticationResult);

}

/* Client makes a method call.
   Client generates ECDSA key/X509 cert using AllJoyn APIs.
   Service provides ECDSA key/X509 using OpenSSL.
   Service and Verify certificate details using ALlJoyn APIs
   Auth.succeeds. The various fields in the certificate are validated on both sides.
 */
TEST_F(SecurityX509Test, Test10) {

    const char serviceecdsaPrivateKeyPEM[] = {
        "-----BEGIN EC PRIVATE KEY-----\n"
        "MHcCAQEEIBDHrvvxplWokYRf5EGAh+Igv6xT+DlWmnxfAVbSMWaRoAoGCCqGSM49\n"
        "AwEHoUQDQgAE6kuo/Ys1Dr9YvlAPyvGXpZIIMvnxkX4a+9zoUCW/LpovDLSTreqy\n"
        "Y14WvRcnY1KWI/BnR26fLMp2XI7DHeePFg==\n"
        "-----END EC PRIVATE KEY-----"
    };

    const char serviceecdsaCertChainX509PEM[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBcDCCARagAwIBAgICJw8wCgYIKoZIzj0EAwIwMDEZMBcGA1UECwwQVGVzdE9y\n"
        "Z2FuaXphdGlvbjETMBEGA1UEAwwKVGVzdENvbW1vbjAeFw0xNTAzMTcyMDA2MTla\n"
        "Fw0xNjAzMTYyMDA2MTlaMDAxGTAXBgNVBAsMEFRlc3RPcmdhbml6YXRpb24xEzAR\n"
        "BgNVBAMMClRlc3RDb21tb24wWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATqS6j9\n"
        "izUOv1i+UA/K8Zelkggy+fGRfhr73OhQJb8umi8MtJOt6rJjXha9FydjUpYj8GdH\n"
        "bp8synZcjsMd548WoyAwHjAJBgNVHRMEAjAAMBEGCisGAQQBgt58AQEEAwIBATAK\n"
        "BggqhkjOPQQDAgNIADBFAiEAjhzM1AeMa7fgMAPzKXpj67RuhOQKbsaUt6kABNa9\n"
        "RsYCIAWn78L966fLSHRsoNxilOA10z+CeIve07ZDB8Uy07GX\n"
        "-----END CERTIFICATE-----"
    };

/* The following fields are encoded in the service certificate and will used to verify.*/
    const char serviceecdsaPublicKeyPEM[] = {
        "-----BEGIN PUBLIC KEY-----\n"
        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE6kuo/Ys1Dr9YvlAPyvGXpZIIMvnx\n"
        "kX4a+9zoUCW/LpovDLSTreqyY14WvRcnY1KWI/BnR26fLMp2XI7DHeePFg==\n"
        "-----END PUBLIC KEY-----"
    };

    const char* const service_cert_serial = "270F";
    const char* const service_cert_subject_ou = "TestOrganization";
    const char* const service_cert_issuer_ou =  "TestOrganization";
    const char* const service_cert_subject_cn = "TestCommon";
    const char* const service_cert_issuer_cn = "TestCommon";

    //Client generates ECDSA private key/X509 certificate using AllJoyn APIs
    //Create a dsa key pair.
    Crypto_ECC ecc;
    status = ecc.GenerateDSAKeyPair();
    EXPECT_EQ(ER_OK, status);
    const ECCPublicKey* dsaPublicKey = ecc.GetDSAPublicKey();
    const ECCPrivateKey* dsaPrivateKey = ecc.GetDSAPrivateKey();

    //Create a cert out of the public key. It is a self signed certificate where subject == issuer
    CertificateX509 x509;
    String serial("AllJoyn-serial");
    x509.SetSerial((const uint8_t*)serial.data(), serial.size());
    const char*issuer_cn = "client issuer cn";
    const char*subject_cn = "subject issuer cn";
    x509.SetIssuerCN((uint8_t*)issuer_cn, strlen(issuer_cn) + 1);
    x509.SetSubjectCN((uint8_t*)subject_cn, strlen(subject_cn) + 1);
    const char*issuer_ou = "client organization";
    const char*subject_ou = "subject organization";
    x509.SetIssuerOU((uint8_t*)issuer_ou, strlen(issuer_ou) + 1);
    x509.SetSubjectOU((uint8_t*)subject_ou, strlen(subject_ou) + 1);
    x509.SetSubjectPublicKey(dsaPublicKey);
    x509.SetCA(true);
    CertificateX509::ValidPeriod validity;
    validity.validFrom = 1427404154;
    validity.validTo = 1427404154 + 630720000;
    x509.SetValidity(&validity);

    //Sign the certificate using issuer private key
    status = x509.Sign(dsaPrivateKey);
    EXPECT_EQ(ER_OK, status) << "Failed to sign the certificate";

    //Encode the private key to PEM
    String clientecdsaPrivateKeyPEM;
    status = CertificateX509::EncodePrivateKeyPEM(dsaPrivateKey, clientecdsaPrivateKeyPEM);
    EXPECT_EQ(ER_OK, status) << "Failed to encode the private key to PEM";

    //Encode the certifcate to PEM
    String clientecdsaCertChainX509PEM = x509.GetPEM();

    //Enable Peer security and reg. auth listener
    ECDSAAuthListener ecdsaAuthListenerForService(serviceecdsaPrivateKeyPEM, serviceecdsaCertChainX509PEM, true, true);
    ECDSAAuthListener ecdsaAuthListenerForClient(clientecdsaPrivateKeyPEM.c_str(), clientecdsaCertChainX509PEM.c_str(), true, true);
    g_ecdsaAuthListenerForService = ecdsaAuthListenerForService;
    g_ecdsaAuthListenerForClient = ecdsaAuthListenerForClient;
    status = servicebus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForService, NULL, false);
    EXPECT_EQ(ER_OK, status);
    status = clientbus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForClient, NULL, false);
    EXPECT_EQ(ER_OK, status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject->GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status);
    status = clientProxyObject->MethodCall(*pingMethod, &pingArgs, 1, reply, 5000, ALLJOYN_FLAG_ENCRYPTED);
    ASSERT_EQ(ER_OK, status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject->msgEncrypted);

    //check for AuthListener details on service side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.requestCredentialsCalled);
    EXPECT_STREQ(clientbus.GetUniqueName().c_str(), g_ecdsaAuthListenerForService.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.verifyCredentialsCalled);
    EXPECT_STREQ(clientbus.GetUniqueName().c_str(), g_ecdsaAuthListenerForService.verifyCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.securityViolationCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.authenticationCompleteCalled);
    EXPECT_TRUE(g_ecdsaAuthListenerForService.authenticationResult);
    String der1;
    Crypto_ASN1::DecodeBase64(g_ecdsaAuthListenerForService.verifyCredentialsX509CertChain, der1);
    String der2;
    Crypto_ASN1::DecodeBase64(clientecdsaCertChainX509PEM, der2);
    EXPECT_TRUE(der1 == der2);

    EXPECT_STREQ(g_ecdsaAuthListenerForService.verifyCredentialsX509CertChain.c_str(), clientecdsaCertChainX509PEM.c_str());

    //check for AuthListener details on client side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.requestCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.verifyCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.verifyCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForClient.securityViolationCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.authenticationCompleteCalled);
    EXPECT_TRUE(g_ecdsaAuthListenerForClient.authenticationResult);
    String der3;
    Crypto_ASN1::DecodeBase64(g_ecdsaAuthListenerForClient.verifyCredentialsX509CertChain, der3);
    String der4;
    Crypto_ASN1::DecodeBase64(serviceecdsaCertChainX509PEM, der4);
    EXPECT_TRUE(der3 == der4);



    /* The client side generated the certificate using AllJoyn APIs. The certificate is presented on the service side via Verify credentials callback.
       The certificate will be decoded and the fields will be validated. The following fields will be validated using the CertificateX509 getter APIs.
       Serial, IssuerOU, IssuerCN, SubjectOU, SubjectCN, Validity, IsCA, PublicKey */
    CertificateX509 x509Validate;
    status = x509Validate.DecodeCertificatePEM(g_ecdsaAuthListenerForService.verifyCredentialsX509CertChain);

    const String SerialNo((const char*)x509Validate.GetSerial(), x509Validate.GetSerialLen());
    EXPECT_STREQ(SerialNo.c_str(), "AllJoyn-serial");

    const char* get_issuer_ou = (const char*)x509Validate.GetIssuerOU();
    EXPECT_STREQ(get_issuer_ou, issuer_ou);
    const size_t get_issuer_ou_len = x509Validate.GetIssuerOULength();
    EXPECT_EQ((size_t)get_issuer_ou_len, strlen(issuer_ou) + 1);

    const char* get_subject_ou = (const char*)x509Validate.GetSubjectOU();
    EXPECT_STREQ(get_subject_ou, subject_ou);
    const size_t get_subject_ou_len = x509Validate.GetSubjectOULength();
    EXPECT_EQ((size_t)get_subject_ou_len, strlen(subject_ou) + 1);

    const char* get_subject_cn  = (const char*)x509Validate.GetSubjectCN();
    EXPECT_STREQ(get_subject_cn, subject_cn);
    const size_t get_subject_cn_len = x509Validate.GetSubjectCNLength();
    EXPECT_EQ((size_t)get_subject_cn_len, strlen(subject_cn) + 1);

    const char* get_issuer_cn  = (const char*)x509Validate.GetIssuerCN();
    EXPECT_STREQ(get_issuer_cn, issuer_cn);
    const size_t get_issuer_cn_len = x509Validate.GetIssuerCNLength();
    EXPECT_EQ((size_t)get_issuer_cn_len, strlen(issuer_cn) + 1);

    EXPECT_TRUE(x509Validate.IsCA());

    const CertificateX509::ValidPeriod*get_validity = x509Validate.GetValidity();
    EXPECT_EQ(get_validity->validFrom, validity.validFrom);
    EXPECT_EQ(get_validity->validTo, validity.validTo);

    const ECCPublicKey* get_public_key = x509Validate.GetSubjectPublicKey();
    String get_public_key_PEM;
    status = CertificateX509::EncodePublicKeyPEM(get_public_key, get_public_key_PEM);
    EXPECT_EQ(ER_OK, status) << "Failed to encode the public key to PEM";
    String dsa_public_key_PEM;
    status = CertificateX509::EncodePublicKeyPEM(dsaPublicKey, dsa_public_key_PEM);
    EXPECT_STREQ(get_public_key_PEM.c_str(), dsa_public_key_PEM.c_str());

    /* The service side generated the certificate using OpenSSL. The certificate is presented on the client side via Verify credentials callback.
       The certificate will be decoded and the fields will be validated. The following fields will be validated using the CertificateX509 getter APIs.
       Serial, IssuerOU, IssuerCN, SubjectOU, SubjectCN, Validity, IsCA, PublicKey */
    CertificateX509 x509ValidateOnClient;
    status = x509ValidateOnClient.DecodeCertificatePEM(g_ecdsaAuthListenerForClient.verifyCredentialsX509CertChain);

    const uint8_t* Client_SerialNo = x509ValidateOnClient.GetSerial();
    const size_t Client_SerialNo_Len = x509ValidateOnClient.GetSerialLen();
    String S = BytesToHexString(Client_SerialNo, Client_SerialNo_Len);
    EXPECT_STREQ(S.c_str(), service_cert_serial);

    uint8_t* client_get_issuer_ou_bytes = (uint8_t*)x509ValidateOnClient.GetIssuerOU();
    const size_t client_get_issuer_ou_len = x509ValidateOnClient.GetIssuerOULength();
    EXPECT_EQ((size_t)client_get_issuer_ou_len, strlen(service_cert_issuer_ou));
    for (size_t i = 0; i < client_get_issuer_ou_len; i++) {
        EXPECT_EQ(client_get_issuer_ou_bytes[i], service_cert_issuer_ou[i]);
    }

    uint8_t* client_get_subject_ou_bytes = (uint8_t*)x509ValidateOnClient.GetSubjectOU();
    const size_t client_get_subject_ou_len = x509ValidateOnClient.GetSubjectOULength();
    EXPECT_EQ((size_t)client_get_subject_ou_len, strlen(service_cert_subject_ou));
    for (size_t i = 0; i < client_get_subject_ou_len; i++) {
        EXPECT_EQ(client_get_subject_ou_bytes[i], service_cert_subject_ou[i]);
    }


    uint8_t* client_get_subject_cn_bytes  = (uint8_t*)x509ValidateOnClient.GetSubjectCN();
    const size_t client_get_subject_cn_len = x509ValidateOnClient.GetSubjectCNLength();
    EXPECT_EQ((size_t)client_get_subject_cn_len, strlen(service_cert_subject_cn));
    for (size_t i = 0; i < client_get_subject_cn_len; i++) {
        EXPECT_EQ(client_get_subject_cn_bytes[i], service_cert_subject_cn[i]);
    }

    uint8_t* client_get_issuer_cn_bytes  = (uint8_t*)x509ValidateOnClient.GetIssuerCN();
    const size_t client_get_issuer_cn_len = x509ValidateOnClient.GetIssuerCNLength();
    EXPECT_EQ((size_t)client_get_issuer_cn_len, strlen(service_cert_issuer_cn));
    for (size_t i = 0; i < client_get_issuer_cn_len; i++) {
        EXPECT_EQ(client_get_issuer_cn_bytes[i], service_cert_issuer_cn[i]);
    }

    EXPECT_FALSE(x509ValidateOnClient.IsCA());

    const CertificateX509::ValidPeriod* client_get_validity = x509ValidateOnClient.GetValidity();
    EXPECT_EQ((client_get_validity->validTo - client_get_validity->validFrom), (long unsigned int)365 * 24 * 3600);

    const ECCPublicKey* client_get_public_key = x509ValidateOnClient.GetSubjectPublicKey();
    String client_get_public_key_PEM;
    status = CertificateX509::EncodePublicKeyPEM(client_get_public_key, client_get_public_key_PEM);
    EXPECT_EQ(ER_OK, status) << "Failed to encode the public key to PEM";
    EXPECT_STREQ(client_get_public_key_PEM.c_str(), serviceecdsaPublicKeyPEM);

}

/* Client makes a method call.
   Client provides OpenSSL generated ECDSA key/X509 cert in PEM. This is a cert chain involving 4 certificates i.e CA->A->B->Alice
   Service provides AllJoyn generated ECDSA key/X509 cert in PEM. This is a cert chain involving 3 certificates i.e CA->A->Bob
   Auth should succeed.
   All certificates are valid for from March 2015 to March 2035.
 */

TEST_F(SecurityX509Test, Test11) {

    const char clientecdsaPrivateKeyPEM[] = {
        "-----BEGIN EC PRIVATE KEY-----\n"
        "MHcCAQEEILNNyg8fH46NGlb11l4P83oJ0VjMTG2ndR92hxWZlIgdoAoGCCqGSM49\n"
        "AwEHoUQDQgAEEJy7HV9dVoGPup1tguTH1LKHRuDU443fMw8+In8fYhzDG7bLRdT5\n"
        "GENCvUBlE0UspnCUt9t0iX7RN0rsFIX/fw==\n"
        "-----END EC PRIVATE KEY-----"

    };

    const char clientecdsaCertChainX509PEM[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBWDCB/qADAgECAgEEMAoGCCqGSM49BAMCMDgxGjAYBgNVBAsMEUludGVybWVk\n"
        "aWF0ZS1CLU9VMRowGAYDVQQDDBFJbnRlcm1lZGlhdGUtQi1DTjAeFw0xNTAzMjAy\n"
        "MTE2NTNaFw0zNDA1MTkyMTE2NTNaMCQxEDAOBgNVBAsMB0FsaWNlT1UxEDAOBgNV\n"
        "BAMMB0FsaWNlQ04wWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQQnLsdX11WgY+6\n"
        "nW2C5MfUsodG4NTjjd8zDz4ifx9iHMMbtstF1PkYQ0K9QGUTRSymcJS323SJftE3\n"
        "SuwUhf9/ow0wCzAJBgNVHRMEAjAAMAoGCCqGSM49BAMCA0kAMEYCIQDTaQmitWfF\n"
        "j4WSxnms4OozntYJtb0fUA2duVvADWqsggIhAO7uKWgKOYulSmmCRcyLjCIas8y7\n"
        "5FQUIYSJ99QswfiM\n"
        "-----END CERTIFICATE-----\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBbzCCARWgAwIBAgIBAzAKBggqhkjOPQQDAjA4MRowGAYDVQQLDBFJbnRlcm1l\n"
        "ZGlhdGUtQS1PVTEaMBgGA1UEAwwRSW50ZXJtZWRpYXRlLUEtQ04wHhcNMTUwMzIw\n"
        "MjExNjMxWhcNMzQwODI3MjExNjMxWjA4MRowGAYDVQQLDBFJbnRlcm1lZGlhdGUt\n"
        "Qi1PVTEaMBgGA1UEAwwRSW50ZXJtZWRpYXRlLUItQ04wWTATBgcqhkjOPQIBBggq\n"
        "hkjOPQMBBwNCAAQCwjmrGaD/j8LzDdO6TEcSMZeygeBxX4QvPTjymwdSHCXl6A7e\n"
        "x18TQ7XkJ2VrD/5xiXlkDukqVU5mSx2NIE9boxAwDjAMBgNVHRMEBTADAQH/MAoG\n"
        "CCqGSM49BAMCA0gAMEUCIQCZ1qEiPjkKOESgXZnDdoXtbywk0NVAuJlbryyfmefZ\n"
        "cQIgK1JR//TxiwMlW0OBysw1m8ZLpfa8svYi0cBX4XDnd2Q=\n"
        "-----END CERTIFICATE-----\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBezCCASCgAwIBAgIBAjAKBggqhkjOPQQDAjBDMSAwHgYDVQQLDBdDZXJ0aWZp\n"
        "Y2F0ZUF1dGhvcml0eU9yZzEfMB0GA1UEAwwWQ2VydGlmaWNhdGVBdXRob3JpdHlD\n"
        "TjAeFw0xNTAzMjAyMTE2MTlaFw0zNDEyMDUyMTE2MTlaMDgxGjAYBgNVBAsMEUlu\n"
        "dGVybWVkaWF0ZS1BLU9VMRowGAYDVQQDDBFJbnRlcm1lZGlhdGUtQS1DTjBZMBMG\n"
        "ByqGSM49AgEGCCqGSM49AwEHA0IABCJ9oBo98xdoGe9fidu4pdVfOUs8JTNrCdYb\n"
        "XiLE07BA2FNqBt8tyThV683817QBjDsNu62J+KO0H9sCpuxMj3qjEDAOMAwGA1Ud\n"
        "EwQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAPFrv11SjwFg/gfufrUeIFy/vQV1\n"
        "Yfp4F1b0wqL8GpNSAiEA+dk6wfJNVORbuzT5O8wA1Ds+EkckWANGM5hTcyJVUHg=\n"
        "-----END CERTIFICATE-----\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBhjCCASugAwIBAgIBATAKBggqhkjOPQQDAjBDMSAwHgYDVQQLDBdDZXJ0aWZp\n"
        "Y2F0ZUF1dGhvcml0eU9yZzEfMB0GA1UEAwwWQ2VydGlmaWNhdGVBdXRob3JpdHlD\n"
        "TjAeFw0xNTAzMjAyMTE2MDlaFw0zNTAzMTUyMTE2MDlaMEMxIDAeBgNVBAsMF0Nl\n"
        "cnRpZmljYXRlQXV0aG9yaXR5T3JnMR8wHQYDVQQDDBZDZXJ0aWZpY2F0ZUF1dGhv\n"
        "cml0eUNOMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEXtD5Is0ZmCi+X1bnle4h\n"
        "qtT5E+UUniTBWnfkifVrMhZ6XCNSkMIZkGhpdi0sMrli4LpjE8j2v7JKUquZv9vN\n"
        "YKMQMA4wDAYDVR0TBAUwAwEB/zAKBggqhkjOPQQDAgNJADBGAiEA9Z95rWN9DCaD\n"
        "hrA1Ph/HmxYFMHwteCMVWjb0IHpPlkwCIQCSuhwoSCaZRas+mbHPYBTZ2q2kNemn\n"
        "8cgJuQqjLb017w==\n"
        "-----END CERTIFICATE-----"
    };

    const char CAPrivateKeyPEM[] = {
        "-----BEGIN EC PRIVATE KEY-----\n"
        "MHcCAQEEILZ9M/JyxbAxab3ulQogsZItUmfFVUzPogkMHjD2tzizoAoGCCqGSM49\n"
        "AwEHoUQDQgAEXtD5Is0ZmCi+X1bnle4hqtT5E+UUniTBWnfkifVrMhZ6XCNSkMIZ\n"
        "kGhpdi0sMrli4LpjE8j2v7JKUquZv9vNYA==\n"
        "-----END EC PRIVATE KEY-----"
    };

    const char CACertificatePEM[] = {
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBhjCCASugAwIBAgIBATAKBggqhkjOPQQDAjBDMSAwHgYDVQQLDBdDZXJ0aWZp\n"
        "Y2F0ZUF1dGhvcml0eU9yZzEfMB0GA1UEAwwWQ2VydGlmaWNhdGVBdXRob3JpdHlD\n"
        "TjAeFw0xNTAzMjAyMTE2MDlaFw0zNTAzMTUyMTE2MDlaMEMxIDAeBgNVBAsMF0Nl\n"
        "cnRpZmljYXRlQXV0aG9yaXR5T3JnMR8wHQYDVQQDDBZDZXJ0aWZpY2F0ZUF1dGhv\n"
        "cml0eUNOMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEXtD5Is0ZmCi+X1bnle4h\n"
        "qtT5E+UUniTBWnfkifVrMhZ6XCNSkMIZkGhpdi0sMrli4LpjE8j2v7JKUquZv9vN\n"
        "YKMQMA4wDAYDVR0TBAUwAwEB/zAKBggqhkjOPQQDAgNJADBGAiEA9Z95rWN9DCaD\n"
        "hrA1Ph/HmxYFMHwteCMVWjb0IHpPlkwCIQCSuhwoSCaZRas+mbHPYBTZ2q2kNemn\n"
        "8cgJuQqjLb017w==\n"
        "-----END CERTIFICATE-----"
    };

    //Intermediate A certificate
    //Create a dsa key pair for A.
    Crypto_ECC eccA;
    status = eccA.GenerateDSAKeyPair();
    EXPECT_EQ(ER_OK, status);
    const ECCPublicKey* dsaPublicKeyA = eccA.GetDSAPublicKey();
    const ECCPrivateKey* dsaPrivateKeyA = eccA.GetDSAPrivateKey();

    //Create a cert out of the public key. It is signed by CA
    CertificateX509 x509A;
    const char*serial_a = "AllJoyn-A";
    const char*issuer_ou_a = "CertificateAuthorityOrg";
    const char*subject_ou_a = "Intermediate-A-OU-AJ";
    const char*issuer_cn_a = "CertificateAuthorityCN";
    const char*subject_cn_a = "Intermediate-A-CN-AJ";
    x509A.SetSerial((uint8_t*)serial_a, strlen(serial_a));
    x509A.SetIssuerCN((uint8_t*)issuer_cn_a, strlen(issuer_cn_a));
    x509A.SetSubjectCN((uint8_t*) subject_cn_a, strlen(subject_cn_a));
    x509A.SetIssuerOU((uint8_t*)issuer_ou_a, strlen(issuer_ou_a));
    x509A.SetSubjectOU((uint8_t*)subject_ou_a, strlen(subject_ou_a));
    x509A.SetSubjectPublicKey(dsaPublicKeyA);
    x509A.SetCA(true);
    CertificateX509::ValidPeriod validity;
    //Validity set from March 2015 to March 2035
    validity.validFrom = 1426890572;
    validity.validTo = 1426890572 + 630720000;
    x509A.SetValidity(&validity);

    //Sign the certificate A using CA private key
    //Convert the CA private key in PEM to ECCPrivateKey format.
    ECCPrivateKey CAPrivateKey;
    status = CertificateX509::DecodePrivateKeyPEM(CAPrivateKeyPEM, &CAPrivateKey);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::DecodePrivateKeyPEM failed with actual status: " << QCC_StatusText(status);
    status = x509A.Sign(&CAPrivateKey);
    EXPECT_EQ(ER_OK, status) << "Failed to sign the certificate";


    //Leaf certificate for Bob
    //Create a dsa key pair for Bob.
    Crypto_ECC eccBob;
    status = eccBob.GenerateDSAKeyPair();
    EXPECT_EQ(ER_OK, status);
    const ECCPublicKey* dsaPublicKeyBob = eccBob.GetDSAPublicKey();
    const ECCPrivateKey* dsaPrivateKeyBob = eccBob.GetDSAPrivateKey();

    //Create a cert out of the public key. It is signed by A
    CertificateX509 x509Bob;
    const char*serial_bob = "AllJoyn-Bob";
    const char*issuer_ou_bob = "Intermediate-A-OU-AJ";
    const char*subject_ou_bob = "BobOU-AJ";
    const char*issuer_cn_bob = "Intermediate-A-CN-AJ";
    const char*subject_cn_bob = "BobCN-AJ";
    x509Bob.SetSerial((uint8_t*)serial_bob, strlen(serial_bob));
    x509Bob.SetIssuerCN((uint8_t*)issuer_cn_bob, strlen(issuer_cn_bob));
    x509Bob.SetSubjectCN((uint8_t*) subject_cn_bob, strlen(subject_cn_bob));
    x509Bob.SetIssuerOU((uint8_t*)issuer_ou_bob, strlen(issuer_ou_bob));
    x509Bob.SetSubjectOU((uint8_t*)subject_ou_bob, strlen(subject_ou_bob));
    x509Bob.SetSubjectPublicKey(dsaPublicKeyBob);
    x509Bob.SetCA(false);
    x509Bob.SetValidity(&validity);

    //Sign the Bob certificate using A private key
    status = x509Bob.Sign(dsaPrivateKeyA);
    EXPECT_EQ(ER_OK, status) << "Failed to sign the certificate";

    //Get the cert chain in PEM format.
    //Encode the certifcate to PEM
    String CACertificatePEMStr(CACertificatePEM);
    String serviceecdsaCertChainX509PEM = x509Bob.GetPEM() + "\n" + x509A.GetPEM() + "\n" +  CACertificatePEMStr;

    //Encode the private key to PEM
    String serviceecdsaPrivateKeyPEM;
    status = CertificateX509::EncodePrivateKeyPEM(dsaPrivateKeyBob, serviceecdsaPrivateKeyPEM);
    EXPECT_EQ(ER_OK, status) << "Failed to encode the private key to PEM";

    //Enable Peer security and reg. auth listener
    ECDSAAuthListener ecdsaAuthListenerForService(serviceecdsaPrivateKeyPEM.c_str(), serviceecdsaCertChainX509PEM.c_str(), true, true);
    ECDSAAuthListener ecdsaAuthListenerForClient(clientecdsaPrivateKeyPEM, clientecdsaCertChainX509PEM, true, true);
    g_ecdsaAuthListenerForService = ecdsaAuthListenerForService;
    g_ecdsaAuthListenerForClient = ecdsaAuthListenerForClient;
    status = servicebus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForService, NULL, false);
    EXPECT_EQ(ER_OK, status);
    status = clientbus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &g_ecdsaAuthListenerForClient, NULL, false);
    EXPECT_EQ(ER_OK, status);

    Message reply(clientbus);
    const InterfaceDescription::Member* pingMethod;
    const InterfaceDescription* ifc = clientProxyObject->GetInterface(interface1);
    pingMethod = ifc->GetMember("my_ping");
    MsgArg pingArgs;
    status = pingArgs.Set("s", "Ping String");
    EXPECT_EQ(ER_OK, status);
    status = clientProxyObject->MethodCall(*pingMethod, &pingArgs, 1, reply, 5000, ALLJOYN_FLAG_ENCRYPTED);
    ASSERT_EQ(ER_OK, status);
    EXPECT_STREQ("Ping String", reply->GetArg(0)->v_string.str);
    EXPECT_TRUE(serviceObject->msgEncrypted);

    //check for AuthListener details on service side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.requestCredentialsCalled);
    EXPECT_STREQ(clientbus.GetUniqueName().c_str(), g_ecdsaAuthListenerForService.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.verifyCredentialsCalled);
    EXPECT_STREQ(clientbus.GetUniqueName().c_str(), g_ecdsaAuthListenerForService.verifyCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForService.securityViolationCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForService.authenticationCompleteCalled);
    EXPECT_TRUE(g_ecdsaAuthListenerForService.authenticationResult);
    String der1;
    Crypto_ASN1::DecodeBase64(g_ecdsaAuthListenerForService.verifyCredentialsX509CertChain, der1);
    String der2;
    Crypto_ASN1::DecodeBase64(clientecdsaCertChainX509PEM, der2);
    EXPECT_TRUE(der1 == der2);

    //check for AuthListener details on client side
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.requestCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.requestCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.verifyCredentialsCalled);
    EXPECT_STREQ(servicebus.GetUniqueName().c_str(), g_ecdsaAuthListenerForClient.verifyCredentialsAuthPeer.c_str());
    EXPECT_EQ((uint32_t)0, g_ecdsaAuthListenerForClient.securityViolationCalled);
    EXPECT_EQ((uint32_t)1, g_ecdsaAuthListenerForClient.authenticationCompleteCalled);
    EXPECT_TRUE(g_ecdsaAuthListenerForClient.authenticationResult);
    String der3;
    Crypto_ASN1::DecodeBase64(g_ecdsaAuthListenerForClient.verifyCredentialsX509CertChain, der3);
    String der4;
    Crypto_ASN1::DecodeBase64(serviceecdsaCertChainX509PEM, der4);
    EXPECT_TRUE(der3 == der4);

}



