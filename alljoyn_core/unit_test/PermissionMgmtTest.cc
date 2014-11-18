/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#include <alljoyn/AllJoynStd.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <qcc/Log.h>
#include <qcc/KeyInfoECC.h>
#include <qcc/StringUtil.h>
#include <alljoyn/PermissionPolicy.h>
#include "CredentialAccessor.h"

using namespace ajn;
using namespace qcc;

static const char* INTERFACE_NAME = "org.allseen.Security.PermissionMgmt";
static const char* NOTIFY_INTERFACE_NAME = "org.allseen.Security.PermissionMgmt.Notification";
static const char* PERMISSION_MGMT_PATH = "/org/allseen/Security/PermissionMgmt";
static const char* APP_PATH = "/app";
static const char* ONOFF_IFC_NAME = "org.allseenalliance.control.OnOff";
static const char* TV_IFC_NAME = "org.allseenalliance.control.TV";

static const char* KEYX_ECDHE_NULL = "ALLJOYN_ECDHE_NULL";
static const char* KEYX_ECDHE_PSK = "ALLJOYN_ECDHE_PSK";
static const char* KEYX_ECDHE_ECDSA = "ALLJOYN_ECDHE_ECDSA";

static GUID128 membershipGUID1;
static const char* membershipSerial1 = "10001";
static GUID128 membershipGUID2;
static GUID128 membershipGUID3;
static const char* membershipSerial3 = "30003";

static const char adminPrivateKeyPEM[] = {
    "-----BEGIN PRIVATE KEY-----\n"
    "WvfKW/zzFtv9E+8S752Y9JBbvMjVE3+bd7VQxy/r6kU="
    "-----END PRIVATE KEY-----"
};

static const char adminPublicKeyPEM[] = {
    "-----BEGIN PUBLIC KEY-----\n"
    "k/5HsY/cJYTrDutk/06LlyuEldPyqx53B84QlP8thIzl5Fut/+CwW2DDUWD/infS\n"
    "SCVStzLPL8IJDCJIy9u7Lw==\n"
    "-----END PUBLIC KEY-----"
};
static const char clientCertChainType1PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "AAAAAf8thIwHzhCU8qsedyuEldP/TouX6w7rZI/cJYST/kexAAAAAMvbuy8JDCJI\n"
    "Ms8vwkglUrf/infSYMNRYP/gsFvl5FutAAAAAAAAAAD/LYSMB84QlPKrHncrhJXT\n"
    "/06Ll+sO62SP3CWEk/5HsQAAAADL27svCQwiSDLPL8JIJVK3/4p30mDDUWD/4LBb\n"
    "5eRbrQAAAAAAAAAAAAAAAAASgF0AAAAAABKBiQABMa7uTLSqjDggO0t6TAgsxKNt\n"
    "+Zhu/jc3s242BE0drFU12USXXIYQdqps/HrMtqw6q9hrZtaGJS+e9y7mJegAAAAA\n"
    "APpeLT1cHNm3/OupnEcUCmg+jqi4SUEi4WTWSR4OzvCSAAAAAA==\n"
    "-----END CERTIFICATE-----"
};

static const char servicePrivateKeyPEM[] = {
    "-----BEGIN PRIVATE KEY-----\n"
    "r4xFNBM7UQVS40QJUVyuJmQCC3ey4Eduj1evmDncZCc="
    "-----END PRIVATE KEY-----"
};

static const char servicePublicKeyPEM[] = {
    "-----BEGIN PUBLIC KEY-----\n"
    "PULf9zxQIxiuoiu0Aih5C46b7iekwVQyC0fljWaWJYlmzgl5Knd51ilhcoT9h45g"
    "hxgYrj8X2zPcex5b3MZN2w=="
    "-----END PUBLIC KEY-----"
};

static const char serviceCertChainType1PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "AAAAAWaWJYkLR+WNpMFUMo6b7icCKHkLrqIrtDxQIxg9Qt/3AAAAANzGTdvcex5b"
    "PxfbM4cYGK79h45gKWFyhCp3edZmzgl5AAAAAAAAAABmliWJC0fljaTBVDKOm+4n"
    "Aih5C66iK7Q8UCMYPULf9wAAAADcxk3b3HseWz8X2zOHGBiu/YeOYClhcoQqd3nW"
    "Zs4JeQAAAAAAAAAAAAAAAAARcI0AAAAAABFxuQABMa7uTLSqjDggO0t6TAgsxKNt"
    "+Zhu/jc3s242BE0drFkulpB/I/875Iq9JAdGCO6uLJpPHlSJwA8xZPYxzNiQAAAA"
    "APH11DQAtqb0yrr4lry4fptgQ/Ri8ZOVlQkFFEKaD1XPAAAAAA=="
    "-----END CERTIFICATE-----"
};

static const char consumerPrivateKeyPEM[] = {
    "-----BEGIN PRIVATE KEY-----\n"
    "IxdMNqXrmT4eCGC2d1Mh35K9+dfyLNcZn+14C5xExqI="
    "-----END PRIVATE KEY-----"
};

static const char consumerPublicKeyPEM[] = {
    "-----BEGIN PUBLIC KEY-----\n"
    "odJyzleew1K20kzhP31ZfM0GkHlO4sGZ06qfAIZrh7wrCE5Y2WiGpsoY1DLpdUTR\n"
    "Yxa1CGYH++K6ViVn1L9znQ==\n"
    "-----END PUBLIC KEY-----"
};

static QStatus CreateCert(const qcc::String& serial, const qcc::GUID128& issuer, const ECCPrivateKey* issuerPrivateKey, const ECCPublicKey* issuerPubKey, const qcc::GUID128& subject, const ECCPublicKey* subjectPubKey, qcc::String& der)
{
    QStatus status = ER_CRYPTO_ERROR;
    CertificateX509 x509(CertificateX509::GUID_CERTIFICATE);

    x509.SetSerial(serial);
    x509.SetIssuer(issuer);
    x509.SetSubject(subject);
    x509.SetSubjectPublicKey(subjectPubKey);
    status = x509.Sign(issuerPrivateKey);
    if (ER_OK != status) {
        return status;
    }
    return x509.EncodeCertificateDER(der);
}

static QStatus CreateIdentityCert(CredentialAccessor& ca, const qcc::String& serial, const char* issuerPrivateKeyPEM, const char* issuerPublicKeyPEM, bool selfSign, qcc::String& der)
{
    qcc::GUID128 localGUID;
    ca.GetGuid(localGUID);
    qcc::GUID128 userGuid;
    Crypto_ECC userECC;

    ECCPrivateKey issuerPrivateKey;
    CertECCUtil_DecodePrivateKey(issuerPrivateKeyPEM, (uint32_t*) &issuerPrivateKey, sizeof(ECCPrivateKey));
    ECCPublicKey issuerPublicKey;
    CertECCUtil_DecodePublicKey(issuerPublicKeyPEM, (uint32_t*) &issuerPublicKey, sizeof(ECCPublicKey));

    const ECCPublicKey* subjectPublicKey;
    if (selfSign) {
        subjectPublicKey = &issuerPublicKey;
    } else {
        userECC.GenerateDSAKeyPair();
        subjectPublicKey = userECC.GetDSAPublicKey();
    }
    return CreateCert(serial, localGUID, &issuerPrivateKey, &issuerPublicKey, userGuid, subjectPublicKey, der);
}

static QStatus CreateGuildCert(const String& serial, const uint8_t* authDataHash, const qcc::GUID128& issuer, const ECCPrivateKey* issuerPrivateKey, const qcc::GUID128& subject, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, qcc::String& der)
{
    QStatus status = ER_CRYPTO_ERROR;
    MembershipCertificate x509;

    printf("Creating membership certificate\n");
    x509.SetSerial(serial);
    x509.SetIssuer(issuer);
    x509.SetSubject(subject);
    x509.SetSubjectPublicKey(subjectPubKey);
    x509.SetGuild(guild);
    x509.SetDigest(authDataHash, Crypto_SHA256::DIGEST_SIZE);
    printf("Signing certificate\n");
    status = x509.Sign(issuerPrivateKey);
    printf("Sign certificate return status 0x%x\n", status);
    if (ER_OK != status) {
        return status;
    }
    printf("Certificate: %s\n", x509.ToString().c_str());
    status = x509.EncodeCertificateDER(der);
    printf("x509.ExportDER return status 0x%x\n", status);
    return status;
}

static QStatus InterestInSignal(BusAttachment* bus)
{
    const char* notifyConfigMatchRule = "type='signal',interface='" "org.allseen.Security.PermissionMgmt.Notification" "',member='NotifyConfig'";
    return bus->AddMatch(notifyConfigMatchRule);
}

static void MakePEM(qcc::String& der, qcc::String& pem)
{
    qcc::String tag1 = "-----BEGIN CERTIFICATE-----\n";
    qcc::String tag2 = "-----END CERTIFICATE-----";
    Crypto_ASN1::EncodeBase64(der, pem);
    pem = tag1 + pem + tag2;
}

/*
 * This is the local implementation of the an AuthListener.  ECDHEKeyXListener is
 * designed to only handle ECDHE Key Exchange Authentication requests.
 *
 * If any other authMechanism is used other than ECDHE Key Exchange authentication
 * will fail.
 */
class ECDHEKeyXListener : public AuthListener {
  public:
    /* the type of agent */
    typedef enum {
        RUN_AS_ADMIN = 0,
        RUN_AS_SERVICE = 1,
        RUN_AS_CONSUMER = 2
    } AgentType;

    ECDHEKeyXListener(AgentType agentType, BusAttachment& bus) : agentType(agentType), bus(bus)
    {
    }

    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds)
    {
        if (strcmp(authMechanism, KEYX_ECDHE_NULL) == 0) {
            creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
            return true;
        } else if (strcmp(authMechanism, KEYX_ECDHE_PSK) == 0) {
            /*
             * Solicit the Pre shared secret
             */
            /*
             * Based on the pre shared secret id, the application can retrieve
             * the pre shared secret from storage or from the end user.
             * In this example, the pre shared secret is a hard coded string
             */
            String psk("123456");
            creds.SetPassword(psk);
            creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
            return true;
        } else if (strcmp(authMechanism, KEYX_ECDHE_ECDSA) == 0) {
            const char* privateKeyPEM;
            const char* publicKeyPEM;

            if (agentType == RUN_AS_ADMIN) {
                privateKeyPEM = adminPrivateKeyPEM;
                publicKeyPEM = adminPublicKeyPEM;
            } else if (agentType == RUN_AS_SERVICE) {
                privateKeyPEM = servicePrivateKeyPEM;
                publicKeyPEM = servicePublicKeyPEM;
            } else {
                privateKeyPEM = consumerPrivateKeyPEM;
                publicKeyPEM = consumerPublicKeyPEM;
            }

            /*
             * The application may provide the DSA private key and public key in the certificate.
             * AllJoyn stores the keys in the key store for future use.
             * If the application does not provide the private key, AllJoyn will
             * generate the DSA key pair.
             */
            bool providePrivateKey = true;      /* use to toggle the test */
            if (providePrivateKey) {
                if ((credMask & AuthListener::CRED_PRIVATE_KEY) == AuthListener::CRED_PRIVATE_KEY) {
                    String pk(privateKeyPEM, strlen(privateKeyPEM));
                    creds.SetPrivateKey(pk);
                }
                if ((credMask & AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN) {
                    CredentialAccessor ca(bus);
                    qcc::String der;
                    /* make a self sign cert */
                    CreateIdentityCert(ca, "1001", privateKeyPEM, publicKeyPEM, true, der);
                    qcc::String pem;
                    MakePEM(der, pem);
                    creds.SetCertChain(pem);
                }
            }
            creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
            return true;
        }
        return false;
    }

    bool VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds)
    {
        /* only the ECDHE_ECDSA calls for peer credential verification */
        if (strcmp(authMechanism, KEYX_ECDHE_ECDSA) == 0) {
            if (creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
                /*
                 * AllJoyn sends back the certificate chain for the application to verify.
                 * The application has to option to verify the certificate
                 * chain.  If the cert chain is validated and trusted then return true; otherwise, return false.
                 */
                return true;
            }
            return true;
        }
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        qcc::String msg;
        if (agentType == RUN_AS_ADMIN) {
            msg += "Admin: ";
        } else if (agentType == RUN_AS_SERVICE) {
            msg += "Service: ";
        } else {
            msg += "Consumer: ";
        }
        msg += "AuthenticationComplete auth mechanism ";
        msg += authMechanism;
        msg += " failed";
        ASSERT_TRUE(success) << msg;
    }

  private:
    AgentType agentType;
    BusAttachment& bus;
};

class PermissionMgmtTest : public testing::Test, public BusObject {
  public:

    PermissionMgmtTest() : BusObject(APP_PATH),
        adminBus("PermissionMgmtTestAdmin", false),
        adminProxyBus("PermissionMgmtTestAdmin", false),
        serviceBus("PermissionMgmtTestService", false),
        consumerBus("PermissionMgmtTestConsumer", false),
        status(ER_OK),
        authComplete(false),
        serviceKeyListener(NULL),
        adminKeyListener(NULL),
        consumerKeyListener(NULL),
        signalNotifyConfigReceived(false)
    {
    }

    virtual void SetUp()
    {
        status = SetupBus(adminBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = SetupBus(adminProxyBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = SetupBus(serviceBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = SetupBus(consumerBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        const InterfaceDescription* itf = adminBus.GetInterface(NOTIFY_INTERFACE_NAME);
        status = adminBus.RegisterSignalHandler(this,
                                                static_cast<MessageReceiver::SignalHandler>(&PermissionMgmtTest::SignalHandler), itf->GetMember("NotifyConfig"), NULL);
        EXPECT_EQ(ER_OK, status) << "  Failed to register signal handler.  Actual Status: " << QCC_StatusText(status);
        status = InterestInSignal(&adminBus);
        EXPECT_EQ(ER_OK, status) << "  Failed to show interest in session-less signal.  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown()
    {
        status = TeardownBus(adminBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = TeardownBus(adminProxyBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = TeardownBus(serviceBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = TeardownBus(consumerBus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        delete serviceKeyListener;
        delete adminKeyListener;
        delete consumerKeyListener;
    }

    void EnableSecurity(const char* keyExchange, bool clearPrevious)
    {
        if (clearPrevious) {
            adminBus.EnablePeerSecurity(NULL, NULL, NULL, false);
            adminProxyBus.EnablePeerSecurity(NULL, NULL, NULL, false);
            serviceBus.EnablePeerSecurity(NULL, NULL, NULL, false);
            consumerBus.EnablePeerSecurity(NULL, NULL, NULL, false);
        }
        delete adminKeyListener;
        adminKeyListener = new ECDHEKeyXListener(ECDHEKeyXListener::RUN_AS_ADMIN, adminBus);
        adminBus.EnablePeerSecurity(keyExchange, adminKeyListener, NULL, true);
        adminProxyBus.EnablePeerSecurity(keyExchange, adminKeyListener, NULL, true);
        delete serviceKeyListener;
        serviceKeyListener = new ECDHEKeyXListener(ECDHEKeyXListener::RUN_AS_SERVICE, serviceBus);
        serviceBus.EnablePeerSecurity(keyExchange, serviceKeyListener, NULL, false);
        delete consumerKeyListener;
        consumerKeyListener = new ECDHEKeyXListener(ECDHEKeyXListener::RUN_AS_CONSUMER, consumerBus);
        consumerBus.EnablePeerSecurity(keyExchange, consumerKeyListener, NULL, false);
    }

    void CreateOnOffAppInterface(BusAttachment& bus, bool addService)
    {
        /* create/activate alljoyn_interface */
        InterfaceDescription* ifc = NULL;
        QStatus status = bus.CreateInterface(ONOFF_IFC_NAME, ifc, AJ_IFC_SECURITY_REQUIRED);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(ifc != NULL);
        if (ifc != NULL) {
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "On", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "Off", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            ifc->Activate();
        }
        if (!addService) {
            return;  /* done */
        }
        status = AddInterface(*ifc);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        AddMethodHandler(ifc->GetMember("On"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtTest::OnOffOn));
        AddMethodHandler(ifc->GetMember("Off"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtTest::OnOffOff));
    }

    void CreateTVAppInterface(BusAttachment& bus, bool addService)
    {
        /* create/activate alljoyn_interface */
        InterfaceDescription* ifc = NULL;
        QStatus status = bus.CreateInterface(TV_IFC_NAME, ifc, AJ_IFC_SECURITY_REQUIRED);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(ifc != NULL);
        if (ifc != NULL) {
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "Up", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "Down", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "Channel", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            ifc->Activate();
        }
        if (!addService) {
            return;  /* done */
        }
        status = AddInterface(*ifc);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        AddMethodHandler(ifc->GetMember("Up"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtTest::TVUp));
        AddMethodHandler(ifc->GetMember("Down"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtTest::TVDown));
        AddMethodHandler(ifc->GetMember("Channel"), static_cast<MessageReceiver::MethodHandler>(&PermissionMgmtTest::TVChannel));
    }

    void CreateAppInterfaces(BusAttachment& bus, bool addService)
    {
        CreateOnOffAppInterface(bus, addService);
        CreateTVAppInterface(bus, addService);
        if (addService) {
            QStatus status = bus.RegisterBusObject(*this);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        }
    }

    BusAttachment adminBus;
    BusAttachment adminProxyBus;
    BusAttachment serviceBus;
    BusAttachment consumerBus;
    GUID128 serviceGUID;
    GUID128 consumerGUID;
    QStatus status;
    bool authComplete;

    void SignalHandler(const InterfaceDescription::Member* member,
                       const char* sourcePath, Message& msg)
    {
        signalNotifyConfigReceived = true;
        printf("A signal name [%s] is received with msg %s\n", member->name.c_str(), msg->ToString().c_str());
        uint8_t*guid;
        size_t guidLen;
        uint32_t serialNum;
        uint8_t claimableState;
        QStatus status = msg->GetArg(0)->Get("ay", &guidLen, &guid);
        if (ER_OK == status) {
            printf("guid: %s\n", BytesToHexString(guid, guidLen).c_str());
        }
        status = msg->GetArg(1)->Get("y", &claimableState);
        if (ER_OK == status) {
            switch (claimableState) {
            case 0:
                printf("claimableState: %d unclaimable\n", claimableState);
                break;

            case 1:
                printf("claimableState: %d claimable\n", claimableState);
                break;

            case 2:
                printf("claimableState: %d claimed\n", claimableState);
                break;
            }
        }
        status = msg->GetArg(2)->Get("u", &serialNum);
        if (ER_OK == status) {
            printf("serial number: %d\n", serialNum);
        }
    }

    void SetNotifyConfigSignalReceived(bool flag)
    {
        signalNotifyConfigReceived = flag;
    }

    const bool GetNotifyConfigSignalReceived()
    {
        return signalNotifyConfigReceived;
    }

    void OnOffOn(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

    void OnOffOff(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

    void TVUp(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

    void TVDown(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

    void TVChannel(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

  private:
    QStatus SetupBus(BusAttachment& bus)
    {
        QStatus status = bus.Start();
        if (ER_OK != status) {
            return status;
        }
        return bus.Connect(ajn::getConnectArg().c_str());
    }

    QStatus TeardownBus(BusAttachment& bus)
    {
        bus.UnregisterBusObject(*this);
        status = bus.Disconnect(ajn::getConnectArg().c_str());
        if (ER_OK != status) {
            return status;
        }
        status = bus.Stop();
        if (ER_OK != status) {
            return status;
        }
        return bus.Join();
    }

    ECDHEKeyXListener* serviceKeyListener;
    ECDHEKeyXListener* adminKeyListener;
    ECDHEKeyXListener* consumerKeyListener;
    bool signalNotifyConfigReceived;
};

static bool IsPermissionDeniedError(QStatus status, Message& msg)
{
    if (ER_PERMISSION_DENIED == status) {
        return true;
    }
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        if (msg->GetErrorName() == NULL) {
            return false;
        }
        printf("IsPermissionDeniedError error name %s error description %s\n", msg->GetErrorName(), msg->GetErrorDescription().c_str());
        if (strcmp(msg->GetErrorName(), "org.alljoyn.Bus.ER_PERMISSION_DENIED") == 0) {
            return true;
        }
    }
    return false;
}

QStatus ReadClaimResponse(Message& msg, ECCPublicKey* pubKey)
{
    uint8_t keyFormat;
    MsgArg* variantArg;
    QStatus status = msg->GetArg(0)->Get("(yv)", &keyFormat, &variantArg);
    if (ER_OK != status) {
        return status;
    }
    if (keyFormat != KeyInfo::FORMAT_ALLJOYN) {
        return status;
    }
    uint8_t* kid;
    size_t kidLen;
    uint8_t keyUsageType;
    uint8_t keyType;
    MsgArg* keyVariantArg;
    status = variantArg->Get("(ayyyv)", &kidLen, &kid, &keyUsageType, &keyType, &keyVariantArg);
    if (ER_OK != status) {
        return status;
    }
    if ((keyUsageType != KeyInfo::USAGE_SIGNING) && (keyUsageType != KeyInfo::USAGE_ENCRYPTION)) {
        return status;
    }
    if (keyType != KeyInfoECC::KEY_TYPE) {
        return status;
    }
    uint8_t algorithm;
    uint8_t curve;
    MsgArg* curveVariant;
    status = keyVariantArg->Get("(yyv)", &algorithm, &curve, &curveVariant);
    if (ER_OK != status) {
        return status;
    }
    if (curve != Crypto_ECC::ECC_NIST_P256) {
        return status;
    }

    uint8_t* xCoord;
    size_t xLen;
    uint8_t* yCoord;
    size_t yLen;
    status = curveVariant->Get("(ayay)", &xLen, &xCoord, &yLen, &yCoord);
    if (ER_OK != status) {
        return status;
    }
    if ((xLen != ECC_COORDINATE_SZ) || (yLen != ECC_COORDINATE_SZ)) {
        return status;
    }
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetXCoord(xCoord);
    keyInfo.SetYCoord(yCoord);
    memcpy(pubKey, keyInfo.GetPublicKey(), sizeof(ECCPublicKey));
    return ER_OK;
}

QStatus Claim(BusAttachment& bus, ProxyBusObject& remoteObj, const ECCPublicKey* pubKey, ECCPublicKey* claimedPubKey, GUID128* claimedGUID)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg inputs[2];

    KeyInfoNISTP256 keyInfo;
    keyInfo.SetPublicKey(pubKey);
    CredentialAccessor ca(bus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    if (!claimedGUID) {
        claimedGUID = &localGUID;
    }
    inputs[0].Set("(yv)", KeyInfo::FORMAT_ALLJOYN,
                  new MsgArg("(ayyyv)", GUID128::SIZE, localGUID.GetBytes(), KeyInfo::USAGE_SIGNING, KeyInfoECC::KEY_TYPE,
                             new MsgArg("(yyv)", keyInfo.GetAlgorithm(), keyInfo.GetCurve(),
                                        new MsgArg("(ayay)", ECC_COORDINATE_SZ, keyInfo.GetXCoord(), ECC_COORDINATE_SZ, keyInfo.GetYCoord()))));
    inputs[0].SetOwnershipFlags(MsgArg::OwnsArgs, true);

    inputs[1].Set("ay", GUID128::SIZE, claimedGUID->GetBytes());
    uint32_t timeout = 10000; /* Claim is a bit show */

    printf("Claim send keyInfo with GUID %s\n", localGUID.ToString().c_str());
    status = remoteObj.MethodCall(INTERFACE_NAME, "Claim", inputs, 2, reply, timeout);

    if (ER_OK == status) {
        status = ReadClaimResponse(reply, claimedPubKey);
        if (ER_OK != status) {
            return status;
        }
    } else if (IsPermissionDeniedError(status, reply)) {
        status = ER_PERMISSION_DENIED;
    }
    return status;
}

PermissionPolicy* GeneratePolicy(qcc::GUID128& guid)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(74892317);

    /* add the admin section */
    PermissionPolicy::Peer* admins = new PermissionPolicy::Peer[1];
    admins[0].SetType(PermissionPolicy::Peer::PEER_GUID);
    admins[0].SetID(guid.GetBytes(), guid.SIZE);
    policy->SetAdmins(1, admins);

    /* add the provider section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[3];

    /* terms record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(ONOFF_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Off");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    prms[1].SetMemberName("*");
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);
    terms[0].SetRules(1, rules);

    /* terms record 1 GUILD membershipGUID1 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    peers[0].SetID(membershipGUID1.GetBytes(), qcc::GUID128::SIZE);
    terms[1].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("Channel");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(3, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);
    terms[1].SetRules(2, rules);

    /* terms record 2 GUILD membershipGUID2 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    peers[0].SetID(membershipGUID2.GetBytes(), qcc::GUID128::SIZE);
    terms[2].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[2];
    rules[0].SetObjPath("/control/settings");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_DENIED);
    rules[0].SetMembers(1, prms);
    rules[1].SetObjPath("*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    terms[2].SetRules(2, rules);

    policy->SetTerms(3, terms);

    return policy;
}

PermissionPolicy* GenerateMembeshipAuthData()
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(88473);

    /* add the provider section */

    PermissionPolicy::Term* terms = new PermissionPolicy::Term[1];

    /* terms record 0 */
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);

    terms[0].SetRules(1, rules);
    policy->SetTerms(1, terms);

    return policy;
}

QStatus GenerateManifest(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    rules[0].SetInterfaceName(TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    *retRules = rules;
    return ER_OK;
}

QStatus GetManifest(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy::Rule** retRules, size_t* count)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(INTERFACE_NAME, "GetManifest", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
        return status;
    }
    uint8_t type;
    MsgArg* variant;
    status = reply->GetArg(0)->Get("(yv)", &type, &variant);
    if (ER_OK != status) {
        return status;
    }
    return PermissionPolicy::ParseRules(*variant, retRules, count);
}

QStatus InstallPolicy(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy& policy)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg policyArg;

    status = policy.Export(policyArg);
    if (ER_OK != status) {
        return status;
    }
    status = remoteObj.MethodCall(INTERFACE_NAME, "InstallPolicy", &policyArg, 1, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus GetPolicy(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy& policy)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(INTERFACE_NAME, "GetPolicy", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
        return status;
    }
    uint8_t version;
    MsgArg* variant;
    reply->GetArg(0)->Get("(yv)", &version, &variant);
    return policy.Import(version, *variant);
}

QStatus RemovePolicy(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(INTERFACE_NAME, "RemovePolicy", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}


QStatus CreateMembershipCert(const String& serial, const uint8_t* authDataHash, const qcc::GUID128& issuer, const qcc::GUID128& subject, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild, qcc::String& der)
{
    ECCPrivateKey trustAnchorPrivateKey;
    CertECCUtil_DecodePrivateKey(adminPrivateKeyPEM, (uint32_t*) &trustAnchorPrivateKey, sizeof(ECCPrivateKey));
    return CreateGuildCert(serial, authDataHash, issuer, &trustAnchorPrivateKey, subject, subjectPubKey, guild, der);
}

QStatus RetrieveDSAPublicKeyFromKeyStore(BusAttachment& bus, ECCPublicKey* publicKey)
{
    GUID128 guid;
    KeyBlob kb;
    CredentialAccessor ca(bus);
    ca.GetLocalGUID(KeyBlob::DSA_PUBLIC, guid);
    QStatus status = ca.GetKey(guid, kb);
    if (status != ER_OK) {
        return status;
    }
    memcpy(publicKey, kb.GetData(), kb.GetSize());
    return ER_OK;
}

QStatus LoadCertificateBytes(Message& msg, CertificateX509& cert)
{
    uint8_t encoding;
    uint8_t* encoded;
    size_t encodedLen;
    QStatus status = msg->GetArg(0)->Get("(yay)", &encoding, &encodedLen, &encoded);
    if (ER_OK != status) {
        return status;
    }
    status = ER_NOT_IMPLEMENTED;
    if (encoding == Certificate::ENCODING_X509_DER) {
        status = cert.DecodeCertificateDER(String((const char*) encoded, encodedLen));
    } else if (encoding == Certificate::ENCODING_X509_DER_PEM) {
        status = cert.DecodeCertificatePEM(String((const char*) encoded, encodedLen));
    }
    return status;
}

QStatus InstallMembership(const String& serial, BusAttachment& bus, ProxyBusObject& remoteObj, const qcc::GUID128& subjectGUID, const ECCPublicKey* subjectPubKey, const qcc::GUID128& guild)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    CredentialAccessor ca(bus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    if (status != ER_OK) {
        return status;
    }
    PermissionPolicy* membershipAuthData = GenerateMembeshipAuthData();
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    Message tmpMsg(bus);
    DefaultPolicyMarshaller marshaller(tmpMsg);
    membershipAuthData->Digest(marshaller, digest, Crypto_SHA256::DIGEST_SIZE);

    qcc::String der;
    status = CreateMembershipCert(serial, digest, localGUID, subjectGUID, subjectPubKey, guild, der);
    if (status != ER_OK) {
        return status;
    }
    printf("Sending membership cert length %d\n", (int) der.length());
    MsgArg certArgs[1];
    certArgs[0].Set("(yay)", Certificate::ENCODING_X509_DER, der.length(), der.c_str());
    MsgArg arg("a(yay)", 1, certArgs);

    status = remoteObj.MethodCall(INTERFACE_NAME, "InstallMembership", &arg, 1, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }

    if (ER_OK == status) {
        /* installing the auth data */
        MsgArg args[3];
        args[0].Set("s", serial.c_str());
        args[1].Set("ay", GUID128::SIZE, localGUID.GetBytes());
        membershipAuthData->Export(args[2]);
        printf("InstallMembership calls InstallMembershipAuthData with auth data %s\n", membershipAuthData->ToString().c_str());
        status = remoteObj.MethodCall(INTERFACE_NAME, "InstallMembershipAuthData", args, ArraySize(args), reply, 5000);
        if (ER_OK != status) {
            if (IsPermissionDeniedError(status, reply)) {
                status = ER_PERMISSION_DENIED;
            }
        }
    }
    delete membershipAuthData;
    return status;
}


QStatus RemoveMembership(BusAttachment& bus, ProxyBusObject& remoteObj, const qcc::String serialNum, const qcc::GUID128& issuer)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    MsgArg inputs[2];
    inputs[0].Set("s", serialNum.c_str());
    inputs[1].Set("ay", GUID128::SIZE, issuer.GetBytes());

    status = remoteObj.MethodCall(INTERFACE_NAME, "RemoveMembership", inputs, 2, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus InstallIdentity(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::String& der)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg arg("(yay)", Certificate::ENCODING_X509_DER, der.size(), der.data());
    printf("InstallIdentity with der length %d\n", (int) der.size());

    status = remoteObj.MethodCall(INTERFACE_NAME, "InstallIdentity", &arg, 1, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus GetIdentity(BusAttachment& bus, ProxyBusObject& remoteObj, qcc::String& der)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(INTERFACE_NAME, "GetIdentity", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
        return status;
    }
    IdentityCertificate cert;
    status = LoadCertificateBytes(reply, cert);
    if (ER_OK != status) {
        printf("GetIdentity LoadCertificateBytes return status 0x%x\n", status);
        return status;
    }
    status = cert.EncodeCertificateDER(der);
    //return cert.EncodeCertificateDER(der);
    printf("GetIdentity return status 0x%x\n", status);
    return status;
}

QStatus RemoveIdentity(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    status = remoteObj.MethodCall(INTERFACE_NAME, "RemoveIdentity", NULL, 0, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus InstallGuildEquivalence(BusAttachment& bus, ProxyBusObject& remoteObj, const char* pem)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg arg("(yay)", Certificate::ENCODING_X509_DER_PEM, strlen(pem), pem);

    status = remoteObj.MethodCall(INTERFACE_NAME, "InstallGuildEquivalence", &arg, 1, reply, 5000);

    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus ExcerciseOn(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(ONOFF_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    printf("ExcerciseOn calling On\n");
    status = remoteObj.MethodCall(ONOFF_IFC_NAME, "On", NULL, 0, reply, 5000);

    printf("ExcerciseOn calling On return status 0x%x\n", status);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus ExcerciseOff(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(ONOFF_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    printf("ExcerciseOff calling Off\n");
    status = remoteObj.MethodCall(ONOFF_IFC_NAME, "Off", NULL, 0, reply, 5000);

    printf("ExcerciseOff calling Off return status 0x%x\n", status);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus ExcerciseTVUp(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    printf("ExcerciseTVUp calling Up\n");
    status = remoteObj.MethodCall(TV_IFC_NAME, "Up", NULL, 0, reply, 5000);

    printf("ExcerciseTVUp calling Up return status 0x%x\n", status);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus ExcerciseTVDown(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    printf("ExcerciseTVDown calling Down\n");
    status = remoteObj.MethodCall(TV_IFC_NAME, "Down", NULL, 0, reply, 5000);

    printf("ExcerciseTVDown calling Down return status 0x%x\n", status);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus ExcerciseTVChannel(BusAttachment& bus, ProxyBusObject& remoteObj)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(TV_IFC_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);

    printf("ExcerciseTVChannel calling Channel\n");
    status = remoteObj.MethodCall(TV_IFC_NAME, "Channel", NULL, 0, reply, 5000);

    printf("ExcerciseTVChannel calling Channel return status 0x%x\n", status);
    if (ER_OK != status) {
        if (IsPermissionDeniedError(status, reply)) {
            status = ER_PERMISSION_DENIED;
        }
    }
    return status;
}

QStatus JoinPeerSession(BusAttachment& initiator, BusAttachment& responder, SessionId& sessionId)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    QStatus status = ER_FAIL;
    for (int cnt = 0; cnt < 30; cnt++) {
        status = initiator.JoinSession(responder.GetUniqueName().c_str(),
                                       ALLJOYN_SESSIONPORT_PERMISSION_MGMT, NULL, sessionId, opts);
        if (ER_OK == status) {
            return status;
        }
        /* sleep a few seconds since the responder may not yet setup the listener port */
        qcc::Sleep(100);
    }
    return status;
}

/*
 *  Preparation work to clear out the existing key stores.
 */
TEST_F(PermissionMgmtTest, Prep)
{
    EnableSecurity("ALLJOYN_ECDHE_NULL", false);

    SessionId sessionId;
    QStatus status = JoinPeerSession(adminBus, serviceBus, sessionId);
    EXPECT_EQ(ER_OK, status) << "  JoinSession to service failed.  Actual Status: " << QCC_StatusText(status);
    /* retrieve the identity cert just to enable the secured session */
    qcc::String retIdentity;
    ProxyBusObject serviceProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
    status = GetIdentity(adminBus, serviceProxyObject, retIdentity);
    EXPECT_NE(ER_OK, status) << "  GetIdentity did not fail.  Actual Status: " << QCC_StatusText(status);

    status = JoinPeerSession(adminBus, consumerBus, sessionId);
    EXPECT_EQ(ER_OK, status) << "  JoinSession consumer failed.  Actual Status: " << QCC_StatusText(status);
    /* retrieve the identity cert just to enable the secured session */
    ProxyBusObject consumerProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
    status = GetIdentity(adminBus, consumerProxyObject, retIdentity);
    EXPECT_NE(ER_OK, status) << "  GetIdentity did not fail.  Actual Status: " << QCC_StatusText(status);

    /* clear all the key store for the remaining tests */
    adminBus.ClearKeyStore();
    serviceBus.ClearKeyStore();
    consumerBus.ClearKeyStore();
}

/*
 *  Test PermissionMgmt Claim method to the admin
 */
TEST_F(PermissionMgmtTest, ClaimAdmin)
{
    QStatus status = ER_OK;
    EnableSecurity("ALLJOYN_ECDHE_NULL", false);
    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = JoinPeerSession(adminBus, adminProxyBus, sessionId);
    EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
    ProxyBusObject clientProxyObject(adminBus, adminProxyBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
    ECCPublicKey claimedPubKey;
    ECCPublicKey issuerPubKey;
    CertECCUtil_DecodePublicKey(adminPublicKeyPEM, (uint32_t*) &issuerPubKey, sizeof(ECCPublicKey));

    status = Claim(adminBus, clientProxyObject, &issuerPubKey, &claimedPubKey, NULL);
    EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);
}

/*
 *  Test PermissionMgmt Claim method to the service
 */
TEST_F(PermissionMgmtTest, ClaimService)
{
    QStatus status = ER_OK;
    EnableSecurity("ALLJOYN_ECDHE_NULL", false);
    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = JoinPeerSession(adminBus, serviceBus, sessionId);
    EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
    ECCPublicKey claimedPubKey;
    ECCPublicKey issuerPubKey;
    CertECCUtil_DecodePublicKey(adminPublicKeyPEM, (uint32_t*) &issuerPubKey, sizeof(ECCPublicKey));

    SetNotifyConfigSignalReceived(false);
    status = Claim(adminBus, clientProxyObject, &issuerPubKey, &claimedPubKey, &serviceGUID);
    EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

    /* try to claim a second time */
    status = Claim(adminBus, clientProxyObject, &issuerPubKey, &claimedPubKey, &serviceGUID);
    EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

    /* sleep a second to see whether the NotifyConfig signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetNotifyConfigSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
}

/*
 *  Test PermissionMgmt Claim method to the consumer
 */
TEST_F(PermissionMgmtTest, ClaimConsumer)
{
    QStatus status = ER_OK;
    EnableSecurity("ALLJOYN_ECDHE_NULL", false);
    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = JoinPeerSession(adminBus, consumerBus, sessionId);
    EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
    ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, sessionId, false);
    ECCPublicKey claimedPubKey;
    ECCPublicKey issuerPubKey;
    CertECCUtil_DecodePublicKey(adminPublicKeyPEM, (uint32_t*) &issuerPubKey, sizeof(ECCPublicKey));

    SetNotifyConfigSignalReceived(false);
    status = Claim(adminBus, clientProxyObject, &issuerPubKey, &claimedPubKey, &consumerGUID);
    EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

    /* try to claim a second time */
    status = Claim(adminBus, clientProxyObject, &issuerPubKey, &claimedPubKey, &consumerGUID);
    EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

    /* sleep a second to see whether the NotifyConfig signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetNotifyConfigSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
}

/*
 *  Test PermissionMgmt InstallPolicy method
 */
TEST_F(PermissionMgmtTest, InstallPolicy)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    CredentialAccessor ca(adminBus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    EXPECT_EQ(ER_OK, status) << "  ca.GetGuid failed.  Actual Status: " << QCC_StatusText(status);

    SetNotifyConfigSignalReceived(false);
    PermissionPolicy* policy = GeneratePolicy(localGUID);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    status = InstallPolicy(adminBus, clientProxyObject, *policy);
    EXPECT_EQ(ER_OK, status) << "  InstallPolicy failed.  Actual Status: " << QCC_StatusText(status);

    /* retrieve back the policy to compare */
    PermissionPolicy retPolicy;
    status = GetPolicy(adminBus, clientProxyObject, retPolicy);
    EXPECT_EQ(ER_OK, status) << "  GetPolicy failed.  Actual Status: " << QCC_StatusText(status);

    EXPECT_EQ(policy->GetSerialNum(), retPolicy.GetSerialNum()) << " GetPolicy failed. Different serial number.";
    EXPECT_EQ(policy->GetAdminsSize(), retPolicy.GetAdminsSize()) << " GetPolicy failed. Different admin size.";
    EXPECT_EQ(policy->GetTermsSize(), retPolicy.GetTermsSize()) << " GetPolicy failed. Different provider size.";
    printf("InstallPolicy gets back policy %s\n", retPolicy.ToString().c_str());
    delete policy;
    /* sleep a second to see whether the NotifyConfig signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetNotifyConfigSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
}

/*
 *  Test PermissionMgmt InstallIdentity method
 */
TEST_F(PermissionMgmtTest, InstallIdentity)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    CredentialAccessor ca(adminBus);
    qcc::String der;
    status = CreateIdentityCert(ca, "1010101", adminPrivateKeyPEM, adminPublicKeyPEM, false, der);
    EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);

    status = InstallIdentity(adminBus, clientProxyObject, der);
    EXPECT_EQ(ER_OK, status) << "  InstallIdentity failed.  Actual Status: " << QCC_StatusText(status);

    /* retrieve back the identity cert PEM to compare */
    qcc::String retIdentity;
    status = GetIdentity(adminBus, clientProxyObject, retIdentity);
    EXPECT_FALSE(retIdentity.empty()) << "  GetIdentity failed.  Return value is empty";
    EXPECT_STREQ(der.c_str(), retIdentity.c_str()) << "  GetIdentity failed.  Return value does not equal original";

    /* delete the identity */
    status = RemoveIdentity(adminBus, clientProxyObject);
    EXPECT_EQ(ER_OK, status) << "  DeleteIdentity failed.  Actual Status: " << QCC_StatusText(status);

    /* retrieve back the identity cert.  Expect failure */
    status = GetIdentity(adminBus, clientProxyObject, retIdentity);
    EXPECT_NE(ER_OK, status) << "  GetIdentity did not fail.  Actual Status: " << QCC_StatusText(status);

}

/*
 *  Test PermissionMgmt InstallMembership method to a provider
 */
TEST_F(PermissionMgmtTest, InstallMembershipToServiceProvider)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    ECCPublicKey claimedPubKey;
    status = RetrieveDSAPublicKeyFromKeyStore(serviceBus, &claimedPubKey);
    EXPECT_EQ(ER_OK, status) << "  InstallMembership RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    status = InstallMembership(membershipSerial3, adminBus, clientProxyObject, serviceGUID, &claimedPubKey, membershipGUID3);
    EXPECT_EQ(ER_OK, status) << "  InstallMembership cert1 failed.  Actual Status: " << QCC_StatusText(status);
    status = InstallMembership(membershipSerial3, adminBus, clientProxyObject, serviceGUID, &claimedPubKey, membershipGUID3);
    EXPECT_NE(ER_OK, status) << "  InstallMembership cert1 again is supposed to fail.  Actual Status: " << QCC_StatusText(status);

}

/*
 *  Test PermissionMgmt RemoveMembership method from service provider
 */
TEST_F(PermissionMgmtTest, RemoveMembershipFromServiceProvider)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);
    CredentialAccessor ca(adminBus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    EXPECT_EQ(ER_OK, status) << "  ca.GetGuid failed.  Actual Status: " << QCC_StatusText(status);

    status = RemoveMembership(adminBus, clientProxyObject, membershipSerial3, localGUID);
    EXPECT_EQ(ER_OK, status) << "  RemoveMembershipFromServiceProvider failed.  Actual Status: " << QCC_StatusText(status);

    /* removing it again */
    status = RemoveMembership(adminBus, clientProxyObject, membershipSerial3, localGUID);
    EXPECT_NE(ER_OK, status) << "  RemoveMembershipFromServiceProvider succeeded.  Expect it to fail.  Actual Status: " << QCC_StatusText(status);

}

/*
 *  Test PermissionMgmt InstallMembership method to a consumer
 */
TEST_F(PermissionMgmtTest, InstallMembershipToConsumer)
{
    ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    ECCPublicKey claimedPubKey;
    status = RetrieveDSAPublicKeyFromKeyStore(consumerBus, &claimedPubKey);
    EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    status = InstallMembership(membershipSerial1, adminBus, clientProxyObject, consumerGUID, &claimedPubKey, membershipGUID1);
    EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer cert1 failed.  Actual Status: " << QCC_StatusText(status);

}

/*
 *  Test PermissionMgmt InstallGuildEquivalence method
 */
TEST_F(PermissionMgmtTest, InstallGuildEquivalence)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    const char* pem = clientCertChainType1PEM;
    status = InstallGuildEquivalence(adminBus, clientProxyObject, pem);
    EXPECT_EQ(ER_OK, status) << "  InstallGuildEquivalence failed.  Actual Status: " << QCC_StatusText(status);

}

/*
 *  Test access by any user
 */
TEST_F(PermissionMgmtTest, AnyUserCanCallOnAndNotOff)
{

    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);
    ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), APP_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);
    QStatus status = ExcerciseOn(consumerBus, clientProxyObject);
    EXPECT_EQ(ER_OK, status) << "  AccessOnOffByAnyUser ExcerciseOn failed.  Actual Status: " << QCC_StatusText(status);
    status = ExcerciseOff(consumerBus, clientProxyObject);
    EXPECT_NE(ER_OK, status) << "  AccessOffByAnyUser ExcersizeOff did not fail.  Actual Status: " << QCC_StatusText(status);
    printf("AccessOnOffByAnyUser done.");
}

/*
 *  Test access by guild user
 */
TEST_F(PermissionMgmtTest, GuildMemberCanTVUpAndDownAndNotChannel)
{

    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);
    ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), APP_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);
    QStatus status = ExcerciseTVUp(consumerBus, clientProxyObject);
    EXPECT_EQ(ER_OK, status) << "  AccessTVByUserWithGuildMembership ExcerciseTVUp failed.  Actual Status: " << QCC_StatusText(status);
    status = ExcerciseTVDown(consumerBus, clientProxyObject);
    EXPECT_EQ(ER_OK, status) << "  AccessTVByUserWithGuildMembership ExcerciseTVDown failed.  Actual Status: " << QCC_StatusText(status);
    status = ExcerciseTVChannel(consumerBus, clientProxyObject);
    EXPECT_NE(ER_OK, status) << "  AccessTVByUserWithGuildMembership ExcerciseTVChannel did not fail.  Actual Status: " << QCC_StatusText(status);
    printf("AccessTVByUserWithGuildMembership done.");
}

/*
 *  Test manifest
 */
TEST_F(PermissionMgmtTest, SetPermissionManifest)
{
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    PermissionPolicy::Rule* rules = NULL;
    size_t count = 0;
    QStatus status = GenerateManifest(&rules, &count);
    EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest GenerateManifest failed.  Actual Status: " << QCC_StatusText(status);
    status = serviceBus.SetPermissionManifest(rules, count);
    EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest SetPermissionManifest failed.  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    PermissionPolicy::Rule* retrievedRules = NULL;
    size_t retrievedCount = 0;
    status = GetManifest(consumerBus, clientProxyObject, &retrievedRules, &retrievedCount);
    EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest GetManifest failed.  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(count, retrievedCount) << "  SetPermissionManifest GetManifest failed to retrieve the same count.";
    for (size_t cnt = 0; cnt < retrievedCount; cnt++) {
        printf("rules[%d]: %s\n", (int) cnt, retrievedRules[cnt].ToString().c_str());
    }
    delete [] rules;
    delete [] retrievedRules;
}

/*
 *  Test PermissionMgmt RemovePolicy method
 */
TEST_F(PermissionMgmtTest, RemovePolicy)
{
    ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);

    CredentialAccessor ca(adminBus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    EXPECT_EQ(ER_OK, status) << "  ca.GetGuid failed.  Actual Status: " << QCC_StatusText(status);

    printf("ca.GetGuid return guid %s\n", localGUID.ToString().c_str());

    PermissionPolicy* policy = GeneratePolicy(localGUID);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    status = InstallPolicy(adminBus, clientProxyObject, *policy);
    EXPECT_EQ(ER_OK, status) << "  InstallPolicy failed.  Actual Status: " << QCC_StatusText(status);

    /* retrieve back the policy to compare */
    PermissionPolicy retPolicy;
    status = GetPolicy(adminBus, clientProxyObject, retPolicy);
    EXPECT_EQ(ER_OK, status) << "  GetPolicy failed.  Actual Status: " << QCC_StatusText(status);

    /* remove the policy */
    SetNotifyConfigSignalReceived(false);
    status = RemovePolicy(adminBus, clientProxyObject);
    EXPECT_EQ(ER_OK, status) << "  RemovePolicy failed.  Actual Status: " << QCC_StatusText(status);

    /* get policy again.  Expect it to fail */
    status = GetPolicy(adminBus, clientProxyObject, retPolicy);
    EXPECT_NE(ER_OK, status) << "  GetPolicy did not fail.  Actual Status: " << QCC_StatusText(status);
    delete policy;
    /* sleep a second to see whether the NotifyConfig signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetNotifyConfigSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetNotifyConfigSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
}

/*
 * Test PermissionMgmt RemoveMembership method from consumer.
 */
TEST_F(PermissionMgmtTest, RemoveMembershipFromConsumer)
{
    ProxyBusObject clientProxyObject(adminBus, consumerBus.GetUniqueName().c_str(), PERMISSION_MGMT_PATH, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA", false);
    CredentialAccessor ca(adminBus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    EXPECT_EQ(ER_OK, status) << "  ca.GetGuid failed.  Actual Status: " << QCC_StatusText(status);

    status = RemoveMembership(adminBus, clientProxyObject, membershipSerial1, localGUID);
    EXPECT_EQ(ER_OK, status) << "  RemoveMembershipFromConsumer failed.  Actual Status: " << QCC_StatusText(status);

    /* removing it again */
    status = RemoveMembership(adminBus, clientProxyObject, membershipSerial1, localGUID);
    EXPECT_NE(ER_OK, status) << "  RemoveMembershipFromConsumer succeeded.  Expect it to fail.  Actual Status: " << QCC_StatusText(status);

}
