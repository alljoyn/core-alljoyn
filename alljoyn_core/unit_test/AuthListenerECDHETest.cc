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

#include <qcc/platform.h>
#include <gtest/gtest.h>
#include <qcc/GUID.h>
#include <qcc/StringUtil.h>

#include <alljoyn/KeyStoreListener.h>
#include <alljoyn/Status.h>
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <qcc/Log.h>
#include "KeyStore.h"
#include "InMemoryKeyStore.h"

#if defined(QCC_OS_GROUP_WINDOWS)
/*
 * This pragma prevents Microsoft compiler warning C4407: cast between different pointer to member representations, compiler may generate incorrect code.
 * This is equivalent to /vmg compiler option but without the burden of forcing that everywhere.
 *
 * Specifically the warning is caused by AuthListenerECDHETest class using multiple inheritance and implementing a member function (MessageReceiver::MethodHandler)
 * that is declared in the base class.
 */
#pragma pointers_to_members(full_generality, virtual_inheritance)
#endif

using namespace ajn;
using namespace qcc;

static const char* ONOFF_IFC_NAME = "org.allseenalliance.control.OnOff";

class AuthListenerECDHETest : public BusObject, public testing::Test {
  public:

    class ECDHEKeyXListener : public AuthListener {
      public:
        ECDHEKeyXListener(bool server) :
            sendKeys(true),
            sendExpiry(true),
            sendPrivateKey(true),
            sendCertChain(true),
            sendEmptyCertChain(false),
            sendInvalidChain(false),
            sendInvalidChainNoCA(false),
            failVerifyCertChain(false),
            authComplete(false),
            pskName("<anonymous>"),
            psk("123456"),
            server(server)
        {
        }

        bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds)
        {
            if (strcmp(authMechanism, "ALLJOYN_ECDHE_NULL") == 0) {
                if (!sendKeys) {
                    return false;
                }
                if (sendExpiry) {
                    creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
                }
                return true;
            } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_PSK") == 0) {
                /*
                 * Solicit the Pre shared secret
                 * Based on the pre shared secret id, the application can retrieve
                 * the pre shared secret from storage or from the end user.
                 * In this example, the pre shared secret is a hard coded string
                 */
                if (!sendKeys) {
                    return false;
                }
                if ((credMask& AuthListener::CRED_USER_NAME) == AuthListener::CRED_USER_NAME) {
                    if (pskName != creds.GetUserName()) {
                        return false;
                    }
                }
                if (pskName != "<anonymous>") {
                    creds.SetUserName(pskName);
                }
                creds.SetPassword(psk);
                if (sendExpiry) {
                    creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
                }
                return true;
            } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
                /* the server key and certificate are generated the unit test common/unit_test/CertificateECCTest::GenSelfSignECCX509CertForBBservice */
                static const char serverPrivateKeyPEM[] = {
                    "-----BEGIN EC PRIVATE KEY-----\n"
                    "MDECAQEEIICSqj3zTadctmGnwyC/SXLioO39pB1MlCbNEX04hjeioAoGCCqGSM49\n"
                    "AwEH\n"
                    "-----END EC PRIVATE KEY-----"
                };

                static const char serverCertChainX509PEM[] = {
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
                /* the client key and certificate are generated using openssl */

                static const char clientPrivateKeyPEM[] = {
                    "-----BEGIN EC PRIVATE KEY-----\n"
                    "MHcCAQEEIAqN6AtyOAPxY5k7eFNXAwzkbsGMl4uqvPrYkIj0LNZBoAoGCCqGSM49\n"
                    "AwEHoUQDQgAEvnRd4fX9opwgXX4Em2UiCMsBbfaqhB1U5PJCDZacz9HumDEzYdrS\n"
                    "MymSxR34lL0GJVgEECvBTvpaHP2bpTIl6g==\n"
                    "-----END EC PRIVATE KEY-----"
                };

                static const char clientCertChainX509PEM[] = {
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
                    "\n"
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIBszCCAVmgAwIBAgIJAILNujb37gH2MAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
                    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
                    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjNaFw0x\n"
                    "NjAyMjYyMTUxMjNaMFYxKTAnBgNVBAsMIDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBm\n"
                    "NzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5\n"
                    "ZGQwMjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGEkAUATvOE4uYmt/10vkTcU\n"
                    "SA0C+YqHQ+fjzRASOHWIXBvpPiKgHcINtNFQsyX92L2tMT2Kn53zu+3S6UAwy6yj\n"
                    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSAAwRQIgKit5yeq1uxTvdFmW\n"
                    "LDeoxerqC1VqBrmyEvbp4oJfamsCIQDvMTmulW/Br/gY7GOP9H/4/BIEoR7UeAYS\n"
                    "4xLyu+7OEA==\n"
                    "-----END CERTIFICATE-----"
                };

                static const char pkWithInvalidChain[] = {
                    "-----BEGIN EC PRIVATE KEY-----\n"
                    "MHcCAQEEIJxWUY1L8fnEMZlo6uFoGxBm/uIOZV6rpOoXXg5Tv01EoAoGCCqGSM49\n"
                    "AwEHoUQDQgAE4bEJQIGst7py9SpK1R//hhPsm7BVHLuHptbxdhudE7bM9kI7y3Uh\n"
                    "XLBHSSxEW7soqXqtJcKFrOzWPRTlF3bFjA==\n"
                    "-----END EC PRIVATE KEY-----"
                };
                static const char invalidChainCert2HasCA[] = {
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIBsTCCAVagAwIBAgIJAIgT+FrlL1qVMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
                    "IDEyMEFGQTU2Q0MwNjZFRkM5QUNCOEVBRTcyNjgxRDI5MSkwJwYDVQQDDCAxMjBB\n"
                    "RkE1NkNDMDY2RUZDOUFDQjhFQUU3MjY4MUQyOTAeFw0xNTAzMDQxNzA2MThaFw0x\n"
                    "NjAzMDMxNzA2MThaMFYxKTAnBgNVBAsMIDEyMEFGQTU2Q0MwNjZFRkM5QUNCOEVB\n"
                    "RTcyNjgxRDI5MSkwJwYDVQQDDCAxMjBBRkE1NkNDMDY2RUZDOUFDQjhFQUU3MjY4\n"
                    "MUQyOTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOGxCUCBrLe6cvUqStUf/4YT\n"
                    "7JuwVRy7h6bW8XYbnRO2zPZCO8t1IVywR0ksRFu7KKl6rSXChazs1j0U5Rd2xYyj\n"
                    "DTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSQAwRgIhAKKDBhCn9ZqYVtJDU6Tw\n"
                    "xwq8BXPzoDrQySHXtDvVpB5pAiEApGStoG974xGLfsIGeMWpUoiOTNe3FIYNmsEW\n"
                    "v6praiU=\n"
                    "-----END CERTIFICATE-----"
                    "\n"
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIBszCCAVmgAwIBAgIJALDTHYnf6i6VMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
                    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
                    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAzMDQxNzA2MjBaFw0x\n"
                    "NjAzMDMxNzA2MjBaMFYxKTAnBgNVBAsMIDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBm\n"
                    "NzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5\n"
                    "ZGQwMjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABANqoKVY9sET8YCj4gjTeYql\n"
                    "GXwLEK4I2aI0SxHZVNj+SQdGltEpnPRHO4jd/tGMnNpwGx0O6acOLrLGH/RIc3Cj\n"
                    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSAAwRQIgPY25+ozlDxgXVJ6T\n"
                    "Uh/vcIUonFt3pqqKtIe99Sc8AdMCIQC8VrFHBFp38e6UkY+Azuikrqi8tXDz8cr3\n"
                    "noKTwIxMpw==\n"
                    "-----END CERTIFICATE-----"
                };
                static const char invalidChainCert2HasNoCA[] = {
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIBsTCCAVagAwIBAgIJAIgT+FrlL1qVMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
                    "IDEyMEFGQTU2Q0MwNjZFRkM5QUNCOEVBRTcyNjgxRDI5MSkwJwYDVQQDDCAxMjBB\n"
                    "RkE1NkNDMDY2RUZDOUFDQjhFQUU3MjY4MUQyOTAeFw0xNTAzMDQxNzA2MThaFw0x\n"
                    "NjAzMDMxNzA2MThaMFYxKTAnBgNVBAsMIDEyMEFGQTU2Q0MwNjZFRkM5QUNCOEVB\n"
                    "RTcyNjgxRDI5MSkwJwYDVQQDDCAxMjBBRkE1NkNDMDY2RUZDOUFDQjhFQUU3MjY4\n"
                    "MUQyOTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOGxCUCBrLe6cvUqStUf/4YT\n"
                    "7JuwVRy7h6bW8XYbnRO2zPZCO8t1IVywR0ksRFu7KKl6rSXChazs1j0U5Rd2xYyj\n"
                    "DTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSQAwRgIhAKKDBhCn9ZqYVtJDU6Tw\n"
                    "xwq8BXPzoDrQySHXtDvVpB5pAiEApGStoG974xGLfsIGeMWpUoiOTNe3FIYNmsEW\n"
                    "v6praiU=\n"
                    "-----END CERTIFICATE-----"
                    "\n"
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIBsDCCAVagAwIBAgIJAP0No5ho6xiVMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
                    "IDZkODVjMjkyMjYxM2IzNmUyZWVlZjUyNzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1\n"
                    "YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQyY2M1NjAeFw0xNTAzMDQxNzA2MjFaFw0x\n"
                    "NjAzMDMxNzA2MjFaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
                    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
                    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABA5Xe+uAlcKzCNfFylIEnggx\n"
                    "F6Gq9tmtLY9mxdyOvTsYwpYuirZAQ2wA+wKBPP7zh7+a3plbedd9GDZ8gow8KCmj\n"
                    "DTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSAAwRQIhAIOU2n6o8QXXbbJVEQe+\n"
                    "n5VkU6DybD3lnsjXSH+1PQVZAiBPCpi8p5xwlBUcFZI1EMPHoLi9XHZtchiJHEo/\n"
                    "OkxLog==\n"
                    "-----END CERTIFICATE-----"
                };

                if (!sendKeys) {
                    return false;
                }
                if (sendPrivateKey && ((credMask& AuthListener::CRED_PRIVATE_KEY) == AuthListener::CRED_PRIVATE_KEY)) {
                    if (sendInvalidChain || sendInvalidChainNoCA) {
                        String pk(pkWithInvalidChain);
                        creds.SetPrivateKey(pk);
                    } else if (server) {
                        String pk(serverPrivateKeyPEM);
                        creds.SetPrivateKey(pk);
                    } else {
                        String pk(clientPrivateKeyPEM);
                        creds.SetPrivateKey(pk);
                    }
                }
                if (sendCertChain && ((credMask& AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN)) {
                    if (sendEmptyCertChain) {
                        String cert;
                        creds.SetCertChain(cert);
                    } else if (sendInvalidChain) {
                        String cert(invalidChainCert2HasCA);
                        creds.SetCertChain(cert);
                    } else if (sendInvalidChainNoCA) {
                        String cert(invalidChainCert2HasNoCA);
                        creds.SetCertChain(cert);
                    } else if (server) {
                        String cert(serverCertChainX509PEM);
                        creds.SetCertChain(cert);
                    } else {
                        String cert(clientCertChainX509PEM);
                        creds.SetCertChain(cert);
                    }
                }
                if (sendExpiry) {
                    creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
                }
                return true;

            }
            return false;
        }

        bool VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds)
        {
            /* only the ECDHE_ECDSA calls for peer credential verification */
            if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
                if (failVerifyCertChain) {
                    return false;
                }
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
            authComplete = success;
            chosenMechanism = authMechanism;
        }

        bool sendKeys;
        bool sendExpiry;
        bool sendPrivateKey;
        bool sendCertChain;
        bool sendEmptyCertChain;
        bool sendInvalidChain;
        bool sendInvalidChainNoCA;
        bool failVerifyCertChain;
        bool authComplete;
        qcc::String pskName;
        qcc::String psk;
        qcc::String chosenMechanism;
      private:
        bool server;
    };

    AuthListenerECDHETest() : BusObject("/AuthListenerECDHETest"),
        clientBus("AuthListenerECDHETestClient", false),
        serverBus("AuthListenerECDHETestServer", false),
        clientAuthListener(false),
        serverAuthListener(true)
    {
    }

    void SetUp()
    {
        EXPECT_EQ(ER_OK, clientBus.Start());
        EXPECT_EQ(ER_OK, clientBus.Connect());
        EXPECT_EQ(ER_OK, clientBus.RegisterKeyStoreListener(clientKeyStoreListener));
        CreateOnOffAppInterface(clientBus, false);
        EXPECT_EQ(ER_OK, serverBus.Start());
        EXPECT_EQ(ER_OK, serverBus.Connect());
        EXPECT_EQ(ER_OK, serverBus.RegisterKeyStoreListener(serverKeyStoreListener));
        CreateOnOffAppInterface(serverBus, true);
    }

    void TearDown()
    {
        clientBus.UnregisterKeyStoreListener();
        clientBus.UnregisterBusObject(*this);
        EXPECT_EQ(ER_OK, clientBus.Disconnect());
        EXPECT_EQ(ER_OK, clientBus.Stop());
        EXPECT_EQ(ER_OK, clientBus.Join());
        serverBus.UnregisterKeyStoreListener();
        serverBus.UnregisterBusObject(*this);
        EXPECT_EQ(ER_OK, serverBus.Disconnect());
        EXPECT_EQ(ER_OK, serverBus.Stop());
        EXPECT_EQ(ER_OK, serverBus.Join());
    }

    QStatus EnableSecurity(bool server, const char* keyExchange)
    {
        if (server) {
            return serverBus.EnablePeerSecurity(keyExchange, &serverAuthListener, NULL, false);
        } else {
            return clientBus.EnablePeerSecurity(keyExchange, &clientAuthListener, NULL, false);
        }
    }

    void CreateOnOffAppInterface(BusAttachment& bus, bool addService)
    {
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
        AddMethodHandler(ifc->GetMember("On"), static_cast<MessageReceiver::MethodHandler>(&AuthListenerECDHETest::OnOffOn));
        AddMethodHandler(ifc->GetMember("Off"), static_cast<MessageReceiver::MethodHandler>(&AuthListenerECDHETest::OnOffOff));
        status = bus.RegisterBusObject(*this);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    BusAttachment clientBus;
    BusAttachment serverBus;
    ECDHEKeyXListener clientAuthListener;
    ECDHEKeyXListener serverAuthListener;

    void OnOffOn(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }
    void OnOffOff(const InterfaceDescription::Member* member, Message& msg)
    {
        MethodReply(msg, ER_OK);
    }

    QStatus ExcerciseOn()
    {
        ProxyBusObject proxyObj(clientBus, serverBus.GetUniqueName().c_str(), GetPath(), 0, false);
        const InterfaceDescription* itf = clientBus.GetInterface(ONOFF_IFC_NAME);
        proxyObj.AddInterface(*itf);
        Message reply(clientBus);

        return proxyObj.MethodCall(ONOFF_IFC_NAME, "On", NULL, 0, reply, 5000);
    }

  private:
    InMemoryKeyStoreListener clientKeyStoreListener;
    InMemoryKeyStoreListener serverKeyStoreListener;
};

TEST_F(AuthListenerECDHETest, ECDHE_NULL_Success)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL"));
    QCC_SetDebugLevel("AUTH_KEY_EXCHANGER", 3);
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_Fail)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL"));
    clientAuthListener.sendKeys = false;
    serverAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_Success_DoNotSendExpiry)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL"));
    clientAuthListener.sendExpiry = false;
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Success_SendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Success_DoNotSendExpiry)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.sendExpiry = false;
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Fail_DoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.sendKeys = false;
    serverAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Fail_ServerDoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    serverAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Fail_ClientDoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Fail_DifferentPSK)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.psk = "123456789";
    serverAuthListener.psk = "nada";
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Success_PSKName)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.pskName = "abc";
    serverAuthListener.pskName = "abc";
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Fail_DifferentPSKName)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.pskName = "abc";
    serverAuthListener.pskName = "dfe";
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Success)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Success_DoNotSendExpiry)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendExpiry = false;
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_DoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendKeys = false;
    serverAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ServerDoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    serverAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ClientDoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_DoNotSendPrivateKey)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendPrivateKey = false;
    serverAuthListener.sendPrivateKey = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ServerDoNotSendPrivateKey)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    serverAuthListener.sendPrivateKey = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ClientDoNotSendPrivateKey)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendPrivateKey = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_DoNotSendCertChain)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendCertChain = false;
    serverAuthListener.sendCertChain = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ServerDoNotSendCertChain)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    serverAuthListener.sendCertChain = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ClientDoNotSendCertChain)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendCertChain = false;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_SendEmptyCertChain)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendEmptyCertChain = true;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_SendInvalidCertChainWithCA)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendInvalidChain = true;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_SendInvalidCertChainWithNoCA)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendInvalidChainNoCA = true;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_FailVerification)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.failVerifyCertChain = true;
    serverAuthListener.failVerifyCertChain = true;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ServerFailVerification)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    serverAuthListener.failVerifyCertChain = true;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ClientFailVerification)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.failVerifyCertChain = true;
    EXPECT_NE(ER_OK, ExcerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_ECDSA_SuccessChosenByServer)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_ECDSA_SuccessChosenByClient)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_ECDSA_SuccessChosenByServer)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_ECDSA_SuccessChosenByClient)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_SuccessChosenByServer)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_SuccessChosenByClient)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_ECDSA_SuccessChosenByServer)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_ECDSA_SuccessChosenByClient)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_ECDSA_AcceptableDowngradeByServer)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, ExcerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
}

