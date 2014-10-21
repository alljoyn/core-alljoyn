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
#include <alljoyn/PermissionPolicy.h>
#include "CredentialAccessor.h"

using namespace ajn;
using namespace qcc;

static const char* INTERFACE_NAME = "org.allseen.Security.PermissionMgmt";
static const char* NOTIFY_INTERFACE_NAME = "org.allseen.Security.PermissionMgmt.Notification";
static const char* object_path = "/org/allseen/Security/PermissionMgmt";

static const char* KEYX_ECDHE_NULL = "ALLJOYN_ECDHE_NULL";
static const char* KEYX_ECDHE_PSK = "ALLJOYN_ECDHE_PSK";
static const char* KEYX_ECDHE_ECDSA = "ALLJOYN_ECDHE_ECDSA";


static const char clientPrivateKeyPEM[] = {
    "-----BEGIN PRIVATE KEY-----\n"
    "CkzgQdvZSOQMmqOnddsw0BRneCNZhioNMyUoJwec9rMAAAAA"
    "-----END PRIVATE KEY-----"
};
static const char clientCertChainType1PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "AAAAAZ1LKGlnpVVtV4Sa1TULsxGJR9C53Uq5AH3fxqxJjNdYAAAAAAobbdvBKaw9\n"
    "eHox7o9fNbN5usuZw8XkSPSmipikYCPJAAAAAAAAAABiToQ8L3KZLwSCetlNJwfd\n"
    "bbxbo2x/uooeYwmvXbH2uwAAAABFQGcdlcsvhdRxgI4SVziI4hbg2d2xAMI47qVB\n"
    "ZZsqJAAAAAAAAAAAAAAAAAABYGEAAAAAAAFhjQABMa7uTLSqjDggO0t6TAgsxKNt\n"
    "+Zhu/jc3s242BE0drNFJAiGa/u6AX5qdR+7RFxVuqm251vKPgWjfwN2AesHrAAAA\n"
    "ANsNwJl8Z1v5jbqo077qdQIT6aM1jc+pKXdgNMk6loqFAAAAAA==\n"
    "-----END CERTIFICATE-----"
};
static const char clientCertChainType2PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "AAAAAp1LKGlnpVVtV4Sa1TULsxGJR9C53Uq5AH3fxqxJjNdYAAAAAAobbdvBKaw9\n"
    "eHox7o9fNbN5usuZw8XkSPSmipikYCPJAAAAAAAAAABiToQ8L3KZLwSCetlNJwfd\n"
    "bbxbo2x/uooeYwmvXbH2uwAAAABFQGcdlcsvhdRxgI4SVziI4hbg2d2xAMI47qVB\n"
    "ZZsqJAAAAAAAAAAAAAAAAAABYGEAAAAAAAFhjQCJ9dkuY0Z6jjx+a8azIQh4UF0h\n"
    "8plX3uAhOlF2vT2jfxe5U06zaWSXcs9kBEQvfOeMM4sUtoXPArUA+TNahfOS9Bbf\n"
    "0Hh08SvDJSDgM2OetQAAAAAYUr2pw2kb90fWblBWVKnrddtrI5Zs8BYx/EodpMrS\n"
    "twAAAAA=\n"
    "-----END CERTIFICATE-----"
};
static const char servicePrivateKeyPEM[] = {
    "-----BEGIN PRIVATE KEY-----\n"
    "tV/tGPp7kI0pUohc+opH1LBxzk51pZVM/RVKXHGFjAcAAAAA\n"
    "-----END PRIVATE KEY-----"
};
static const char serviceCertChainType1PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "AAAAAfUQdhMSDuFWahMG/rFmFbKM06BjIA2Scx9GH+ENLAgtAAAAAIbhHnjAyFys\n"
    "6DoN2kKlXVCgtHpFiEYszOYXI88QDvC1AAAAAAAAAAC5dRALLg6Qh1J2pVOzhaTP\n"
    "xI+v/SKMFurIEo2b4S8UZAAAAADICW7LLp1pKlv6Ur9+I2Vipt5dDFnXSBiifTmf\n"
    "irEWxQAAAAAAAAAAAAAAAAABXLAAAAAAAAFd3AABMa7uTLSqjDggO0t6TAgsxKNt\n"
    "+Zhu/jc3s242BE0drPcL4K+FOVJf+tlivskovQ3RfzTQ+zLoBH5ZCzG9ua/dAAAA\n"
    "ACt5bWBzbcaT0mUqwGOVosbMcU7SmhtE7vWNn/ECvpYFAAAAAA==\n"
    "-----END CERTIFICATE-----"
};
static const char serviceCertChainType2PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "AAAAAvUQdhMSDuFWahMG/rFmFbKM06BjIA2Scx9GH+ENLAgtAAAAAIbhHnjAyFys\n"
    "6DoN2kKlXVCgtHpFiEYszOYXI88QDvC1AAAAAAAAAAC5dRALLg6Qh1J2pVOzhaTP\n"
    "xI+v/SKMFurIEo2b4S8UZAAAAADICW7LLp1pKlv6Ur9+I2Vipt5dDFnXSBiifTmf\n"
    "irEWxQAAAAAAAAAAAAAAAAABXLAAAAAAAAFd3ABjeWi1/GbBcdnK0yJvL4X/UF0h\n"
    "8plX3uAhOlF2vT2jfxe5U06zaWSXcs9kBEQvfOc+WvKloM7m5NFJNSd3qFFGUhfj\n"
    "xx/0CCRJlk/jeIWmzQAAAAB8bexqa95eHEKTqdc8+qKFKggZZXlpaj9af/MFocIP\n"
    "NQAAAAA=\n"
    "-----END CERTIFICATE-----"
};

static QStatus InterestInSignal(BusAttachment* bus)
{
    const char* notifyConfigMatchRule = "type='signal',interface='" "org.allseen.Security.PermissionMgmt.Notification" "',member='NotifyConfig'";
    return bus->AddMatch(notifyConfigMatchRule);
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
    ECDHEKeyXListener(bool isService) : isService(isService)
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
            const char* certChainType1PEM;
            const char* certChainType2PEM;

            if (isService) {
                privateKeyPEM = servicePrivateKeyPEM;
                certChainType1PEM = serviceCertChainType1PEM;
                certChainType2PEM = serviceCertChainType2PEM;
            } else {
                privateKeyPEM = clientPrivateKeyPEM;
                certChainType1PEM = clientCertChainType1PEM;
                certChainType2PEM = clientCertChainType2PEM;
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
                    bool useType1 = false;  /* use to toggle which cert to send */
                    if (useType1) {
                        String cert(certChainType1PEM, strlen(certChainType1PEM));
                        creds.SetCertChain(cert);
                    } else {
                        String cert(certChainType2PEM, strlen(certChainType2PEM));
                        creds.SetCertChain(cert);
                    }
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
            }
            return true;
        }
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
    }

  private:
    bool isService;
};

class PermissionMgmtTest : public testing::Test, public BusObject {
  public:
    PermissionMgmtTest() : BusObject("/test/PermissionMgmt"),
        clientBus("PermissionMgmtTestClient", false),
        serviceBus("PermissionMgmtTestService", false),
        status(ER_OK),
        authComplete(false),
        certChain(NULL),
        certChainLen(0), signalReceived(false)
    {
    }

    virtual void SetUp() {
        status = clientBus.Start();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = clientBus.Connect(ajn::getConnectArg().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus.Start();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus.Connect(ajn::getConnectArg().c_str());
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        const InterfaceDescription* itf = clientBus.GetInterface(NOTIFY_INTERFACE_NAME);
        status = clientBus.RegisterSignalHandler(this,
                                                 static_cast<MessageReceiver::SignalHandler>(&PermissionMgmtTest::SignalHandler), itf->GetMember("NotifyConfig"), NULL);
        EXPECT_EQ(ER_OK, status) << "  Failed to register signal handler.  Actual Status: " << QCC_StatusText(status);
        status = InterestInSignal(&clientBus);
        EXPECT_EQ(ER_OK, status) << "  Failed to show interest in session-less signal.  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {

        status = clientBus.Disconnect(ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus.Disconnect(ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = clientBus.Stop();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus.Stop();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = clientBus.Join();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = serviceBus.Join();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        delete serviceKeyListener;
        delete clientKeyListener;
        if ((certChainLen > 0) && certChain) {
            for (size_t cnt = 0; cnt < certChainLen; cnt++) {
                delete certChain[cnt];
            }
            delete [] certChain;
        }
    }

    void EnableSecurity(const char* keyExchange)
    {
        clientKeyListener = new ECDHEKeyXListener(false);
        clientBus.EnablePeerSecurity(keyExchange, clientKeyListener, NULL, false);
        serviceKeyListener = new ECDHEKeyXListener(true);
        serviceBus.EnablePeerSecurity(keyExchange, serviceKeyListener, NULL, false);
    }

    BusAttachment clientBus;
    BusAttachment serviceBus;
    QStatus status;
    bool authComplete;

    QStatus ParseCertChainPEM(String& encodedCertChain)
    {
        size_t count = 0;
        QStatus status = CertECCUtil_GetCertCount(encodedCertChain, &count);
        if (status != ER_OK) {
            return status;
        }
        certChainLen = count;
        certChain = new CertificateECC *[count];
        status = CertECCUtil_GetCertChain(encodedCertChain, certChain, certChainLen);
        if (status != ER_OK) {
            delete [] certChain;
            certChain = NULL;
            certChainLen = 0;
        }
        return status;
    }

    void SignalHandler(const InterfaceDescription::Member* member,
                       const char* sourcePath, Message& msg)
    {
        signalReceived = true;
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
        CredentialAccessor ca(clientBus);
        qcc::GUID128 localGUID;
        status = ca.GetGuid(localGUID);
        printf("local guid: %s\n", localGUID.ToString().c_str());
    }

    void SetSignalReceived(bool flag)
    {
        signalReceived = flag;
    }

    const bool GetSignalReceived()
    {
        return signalReceived;
    }

    CertificateECC** certChain;
  private:
    ECDHEKeyXListener* serviceKeyListener;
    ECDHEKeyXListener* clientKeyListener;
    size_t certChainLen;
    bool signalReceived;
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
    if ((xLen != KeyInfoECC::ECC_COORDINATE_SZ) || (yLen != KeyInfoECC::ECC_COORDINATE_SZ)) {
        return status;
    }
    KeyInfoNISTP256 keyInfo;
    keyInfo.SetXCoord(xCoord);
    keyInfo.SetYCoord(yCoord);
    keyInfo.Export(pubKey);
    return ER_OK;
}

QStatus Claim(BusAttachment* bus, ProxyBusObject* remoteObj, const ECCPublicKey* pubKey)
{
    QStatus status;
    const InterfaceDescription* itf = bus->GetInterface(INTERFACE_NAME);
    remoteObj->AddInterface(*itf);
    Message reply(*bus);
    MsgArg inputs[2];

    KeyInfoNISTP256 keyInfo;
    keyInfo.Import(pubKey);
    CredentialAccessor ca(*bus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    inputs[0].Set("(yv)", KeyInfo::FORMAT_ALLJOYN,
                  new MsgArg("(ayyyv)", GUID128::SIZE, localGUID.GetBytes(), KeyInfo::USAGE_SIGNING, KeyInfoECC::KEY_TYPE,
                             new MsgArg("(yyv)", keyInfo.GetAlgorithm(), keyInfo.GetCurve(),
                                        new MsgArg("(ayay)", KeyInfoECC::ECC_COORDINATE_SZ, keyInfo.GetXCoord(), KeyInfoECC::ECC_COORDINATE_SZ, keyInfo.GetYCoord()))));
    inputs[0].SetOwnershipFlags(MsgArg::OwnsArgs, true);
    GUID128 claimGUID;
    inputs[1].Set("ay", GUID128::SIZE, claimGUID.GetBytes());
    status = remoteObj->MethodCall(INTERFACE_NAME, "Claim", inputs, 2, reply, 5000);

    if (ER_OK == status) {
        ECCPublicKey claimedPubKey;
        status = ReadClaimResponse(reply, &claimedPubKey);
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

    /* terms record 0 */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY);
    terms[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("org.allseenalliance.control.OnOff");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    rules[0].SetMembers(1, prms);
    terms[0].SetRules(1, rules);

    /* terms record 1 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    qcc::GUID128 guildGUID("5668dd0e9bf2d7f8b73998a430834f5a");
    peers[0].SetID(guildGUID.GetBytes(), guildGUID.SIZE);
    const char* guildAuthority = "guildPubKey";
    peers[0].SetGuildAuthority((const uint8_t*) guildAuthority, strlen(guildAuthority));
    terms[1].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName("org.allseenalliance.control.TV");
    prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[2].SetMemberName("Channel");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    rules[0].SetMembers(3, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    rules[1].SetMembers(1, prms);
    terms[1].SetRules(2, rules);

    /* terms record 2 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_GUILD);
    qcc::GUID128 guildGUID2("317dc2429a69499df9d3eba302fbb3b2");
    peers[0].SetID(guildGUID2.GetBytes(), guildGUID.SIZE);
    const char* guild2Authority = "guild2PubKey";
    peers[0].SetGuildAuthority((const uint8_t*) guild2Authority, strlen(guild2Authority));
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
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    rules[1].SetMembers(1, prms);

    terms[2].SetRules(2, rules);

    policy->SetTerms(3, terms);

    return policy;
}

QStatus InstallPolicy(BusAttachment& bus, ProxyBusObject& remoteObj, PermissionPolicy& policy)
{
    QStatus status;
    const InterfaceDescription* itf = bus.GetInterface(INTERFACE_NAME);
    remoteObj.AddInterface(*itf);
    Message reply(bus);
    MsgArg policyArg;

    status = PermissionPolicy::GeneratePolicyArgs(policyArg, policy);
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
    return PermissionPolicy::BuildPolicyFromArgs(version, *variant, policy);
}

/*
 *  Test PermissionMgmt Claim method
 */
TEST_F(PermissionMgmtTest, Claim)
{
    QStatus status = ER_OK;

    EnableSecurity("ALLJOYN_ECDHE_NULL");
    clientBus.ClearKeyStore();
    serviceBus.ClearKeyStore();
    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = clientBus.JoinSession(serviceBus.GetUniqueName().c_str(),
                                   ALLJOYN_SESSIONPORT_PERMISSION_MGMT, NULL, sessionId, opts);
    EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
    String certChainPEM(clientCertChainType1PEM);
    status = ParseCertChainPEM(certChainPEM);
    EXPECT_EQ(ER_OK, status) << "  Claim failed.  Parsing cert chain PEM failed.  Actual Status: " << QCC_StatusText(status);
    ProxyBusObject clientProxyObject(clientBus, serviceBus.GetUniqueName().c_str(), object_path, sessionId, false);
    status = Claim(&clientBus, &clientProxyObject, certChain[0]->GetSubject());
    EXPECT_EQ(ER_OK, status) << "  Claim failed.  Actual Status: " << QCC_StatusText(status);

    /* try to claim a second time */
    status = Claim(&clientBus, &clientProxyObject, certChain[0]->GetSubject());
    EXPECT_EQ(ER_PERMISSION_DENIED, status) << "  Claim is not supposed to succeed.  Actual Status: " << QCC_StatusText(status);

    /* sleep a few seconds to see whether the signal is received */
    qcc::Sleep(1000);
    EXPECT_TRUE(GetSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
}

/*
 *  Test PermissionMgmt InstallPolicy method
 */
TEST_F(PermissionMgmtTest, InstallPolicy)
{
    ProxyBusObject clientProxyObject(clientBus, serviceBus.GetUniqueName().c_str(), object_path, 0, false);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA");

    CredentialAccessor ca(clientBus);
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    EXPECT_EQ(ER_OK, status) << "  ca.GetGuid failed.  Actual Status: " << QCC_StatusText(status);

    SetSignalReceived(false);
    PermissionPolicy* policy = GeneratePolicy(localGUID);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    status = InstallPolicy(clientBus, clientProxyObject, *policy);
    EXPECT_EQ(ER_OK, status) << "  InstallPolicy failed.  Actual Status: " << QCC_StatusText(status);

    /* retrieve back the policy to compare */
    PermissionPolicy retPolicy;
    status = GetPolicy(clientBus, clientProxyObject, retPolicy);
    EXPECT_EQ(ER_OK, status) << "  GetPolicy failed.  Actual Status: " << QCC_StatusText(status);

    EXPECT_EQ(policy->GetSerialNum(), retPolicy.GetSerialNum()) << " GetPolicy failed. Different serial number.";
    EXPECT_EQ(policy->GetAdminsSize(), retPolicy.GetAdminsSize()) << " GetPolicy failed. Different admin size.";
    EXPECT_EQ(policy->GetTermsSize(), retPolicy.GetTermsSize()) << " GetPolicy failed. Different provider size.";
    delete policy;
    /* sleep a few seconds to see whether the signal is received */
    qcc::Sleep(1000);
    EXPECT_TRUE(GetSignalReceived()) << " Fail to receive expected NotifyConfig signal.";
}

