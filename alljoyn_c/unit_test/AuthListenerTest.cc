/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/AuthListener.h>
#include <alljoyn_c/Status.h>

#include <qcc/platform.h>
#include <qcc/Thread.h>

static const char* INTERFACE_NAME = "org.alljoyn.test.c.authlistener";
static const char* OBJECT_NAME = "org.alljoyn.test.c.authlistener";
static const char* OBJECT_PATH = "/org/alljoyn/test";

static QCC_BOOL name_owner_changed_flag = QCC_FALSE;

static QCC_BOOL requestcredentials_service_flag = QCC_FALSE;
static QCC_BOOL authenticationcomplete_service_flag = QCC_FALSE;
static QCC_BOOL verifycredentials_service_flag = QCC_FALSE;
static QCC_BOOL securityviolation_service_flag = QCC_FALSE;

static QCC_BOOL requestcredentials_client_flag = QCC_FALSE;
static QCC_BOOL authenticationcomplete_client_flag = QCC_FALSE;
static QCC_BOOL verifycredentials_client_flag = QCC_FALSE;
static QCC_BOOL securityviolation_client_flag = QCC_TRUE;

/* NameOwnerChanged callback */
static void AJ_CALL name_owner_changed(const void* context, const char* busName, const char* previousOwner, const char* newOwner)
{
    if (strcmp(busName, OBJECT_NAME) == 0) {
        name_owner_changed_flag = QCC_TRUE;
    }
}

/* Exposed methods */
static void AJ_CALL ping_method(alljoyn_busobject bus, const alljoyn_interfacedescription_member* member, alljoyn_message msg)
{
    alljoyn_msgarg outArg = alljoyn_msgarg_create();
    alljoyn_msgarg inArg = alljoyn_message_getarg(msg, 0);
    const char* str;
    alljoyn_msgarg_get(inArg, "s", &str);
    alljoyn_msgarg_set(outArg, "s", str);
    QStatus status = alljoyn_busobject_methodreply_args(bus, msg, outArg, 1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg_destroy(outArg);
}

class AuthListenerTest : public testing::Test {
  public:
    virtual void SetUp() {
        /* initilize test fixture values to known start value */
        status = ER_FAIL;
        servicebus = NULL;
        clientbus = NULL;

        testObj = NULL;
        buslistener = NULL;

        /* set up the service bus */
        servicebus = alljoyn_busattachment_create("AuthListenerTestService", false);
        status = alljoyn_busattachment_start(servicebus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_connect(servicebus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_interfacedescription service_intf = NULL;
        status = alljoyn_busattachment_createinterface_secure(servicebus, INTERFACE_NAME, &service_intf, AJ_IFC_SECURITY_REQUIRED);
        ASSERT_TRUE(service_intf != NULL);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_interfacedescription_addmember(service_intf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        alljoyn_interfacedescription_activate(service_intf);

        clientbus = alljoyn_busattachment_create("AuthListenerTestClient", false);
        status = alljoyn_busattachment_start(clientbus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_connect(clientbus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        alljoyn_busattachment_stop(servicebus);
        alljoyn_busattachment_join(servicebus);
        alljoyn_busattachment_stop(clientbus);
        alljoyn_busattachment_join(clientbus);
        alljoyn_busattachment_destroy(servicebus);
        alljoyn_busattachment_destroy(clientbus);
        alljoyn_buslistener_destroy(buslistener);
        alljoyn_busobject_destroy(testObj);
    }

    void SetUpAuthService() {
        /* register bus listener */
        alljoyn_buslistener_callbacks buslistenerCbs = {
            NULL,
            NULL,
            NULL,
            NULL,
            &name_owner_changed,
            NULL,
            NULL,
            NULL
        };
        buslistener = alljoyn_buslistener_create(&buslistenerCbs, NULL);
        alljoyn_busattachment_registerbuslistener(servicebus, buslistener);

        /* Set up bus object */
        alljoyn_busobject_callbacks busObjCbs = {
            NULL,
            NULL,
            NULL,
            NULL
        };
        testObj = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);
        const alljoyn_interfacedescription exampleIntf = alljoyn_busattachment_getinterface(servicebus, INTERFACE_NAME);
        ASSERT_TRUE(exampleIntf);

        status = alljoyn_busobject_addinterface(testObj, exampleIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* register method handlers */
        alljoyn_interfacedescription_member ping_member;
        QCC_BOOL foundMember = alljoyn_interfacedescription_getmember(exampleIntf, "ping", &ping_member);
        EXPECT_TRUE(foundMember);

        /* add methodhandlers */
        alljoyn_busobject_methodentry methodEntries[] = {
            { &ping_member, ping_method },
        };
        status = alljoyn_busobject_addmethodhandlers(testObj, methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status = alljoyn_busattachment_registerbusobject(servicebus, testObj);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        name_owner_changed_flag = QCC_FALSE;

        /* request name */
        uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
        status = alljoyn_busattachment_requestname(servicebus, OBJECT_NAME, flags);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        for (size_t i = 0; i < 200; ++i) {
            if (name_owner_changed_flag) {
                break;
            }
            qcc::Sleep(5);
        }
        EXPECT_TRUE(name_owner_changed_flag);
    }

    void SetupAuthClient() {
        alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(clientbus, OBJECT_NAME, OBJECT_PATH, 0);
        EXPECT_TRUE(proxyObj);
        status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_message reply = alljoyn_message_create(clientbus);
        alljoyn_msgarg input = alljoyn_msgarg_create_and_set("s", "AllJoyn");
        status = alljoyn_proxybusobject_methodcall(proxyObj, INTERFACE_NAME, "ping", input, 1, reply, ALLJOYN_MESSAGE_DEFAULT_TIMEOUT, 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        const char* str;
        alljoyn_msgarg_get(alljoyn_message_getarg(reply, 0), "s", &str);
        EXPECT_STREQ("AllJoyn", str);

        alljoyn_message_destroy(reply);
        alljoyn_msgarg_destroy(input);
        alljoyn_proxybusobject_destroy(proxyObj);

    }

    void SetupAuthClientAuthFail() {
        alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(clientbus, OBJECT_NAME, OBJECT_PATH, 0);
        EXPECT_TRUE(proxyObj);
        status = alljoyn_proxybusobject_introspectremoteobject(proxyObj);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_message reply = alljoyn_message_create(clientbus);
        alljoyn_msgarg input = alljoyn_msgarg_create_and_set("s", "AllJoyn");
        status = alljoyn_proxybusobject_methodcall(proxyObj, INTERFACE_NAME, "ping", input, 1, reply, 200, 0);
        EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_message_destroy(reply);
        alljoyn_msgarg_destroy(input);
        alljoyn_proxybusobject_destroy(proxyObj);
    }

    void ResetAuthFlags() {
        requestcredentials_service_flag = QCC_FALSE;
        authenticationcomplete_service_flag = QCC_FALSE;
        verifycredentials_service_flag = QCC_FALSE;
        securityviolation_service_flag = QCC_FALSE;

        requestcredentials_client_flag = QCC_FALSE;
        authenticationcomplete_client_flag = QCC_FALSE;
        verifycredentials_client_flag = QCC_FALSE;
        securityviolation_client_flag = QCC_FALSE;
    }
    QStatus status;
    alljoyn_busattachment servicebus;
    alljoyn_busattachment clientbus;

    alljoyn_busobject testObj;
    alljoyn_buslistener buslistener;
};

/* AuthListener callback functions*/
static QCC_BOOL AJ_CALL authlistener_requestcredentials_service_srp_keyx(const void* context, const char* authMechanism,
                                                                         const char* peerName, uint16_t authCount,
                                                                         const char* userName, uint16_t credMask,
                                                                         alljoyn_credentials credentials) {
    EXPECT_STREQ("context test string", (const char*)context);
    EXPECT_STREQ("ALLJOYN_SRP_KEYX", authMechanism);
    if (credMask & ALLJOYN_CRED_PASSWORD) {
        alljoyn_credentials_setpassword(credentials, "ABCDEFGH");
    }
    requestcredentials_service_flag = QCC_TRUE;
    return QCC_TRUE;
}

static void AJ_CALL alljoyn_authlistener_authenticationcomplete_service_srp_keyx(const void* context, const char* authMechanism,
                                                                                 const char* peerName, QCC_BOOL success) {
    EXPECT_STREQ("context test string", (const char*)context);
    EXPECT_TRUE(success);
    authenticationcomplete_service_flag = QCC_TRUE;
}


static QCC_BOOL AJ_CALL authlistener_requestcredentials_client_srp_keyx(const void* context, const char* authMechanism,
                                                                        const char* peerName, uint16_t authCount,
                                                                        const char* userName, uint16_t credMask,
                                                                        alljoyn_credentials credentials) {
    EXPECT_STREQ("context test string", (const char*)context);
    EXPECT_STREQ("ALLJOYN_SRP_KEYX", authMechanism);
    if (credMask & ALLJOYN_CRED_PASSWORD) {
        alljoyn_credentials_setpassword(credentials, "ABCDEFGH");
    }
    requestcredentials_client_flag = QCC_TRUE;
    return QCC_TRUE;
}

static void AJ_CALL alljoyn_authlistener_authenticationcomplete_client_srp_keyx(const void* context, const char* authMechanism,
                                                                                const char* peerName, QCC_BOOL success) {
    EXPECT_STREQ("context test string", (const char*)context);
    EXPECT_TRUE(success);
    authenticationcomplete_client_flag = QCC_TRUE;
}

TEST_F(AuthListenerTest, srp_keyx) {
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistener_callbacks authlistener_cb_service = {
        authlistener_requestcredentials_service_srp_keyx, //requestcredentials
        NULL, //verifycredentials
        NULL, //securityviolation
        alljoyn_authlistener_authenticationcomplete_service_srp_keyx //authenticationcomplete
    };

    alljoyn_authlistener serviceauthlistener = alljoyn_authlistener_create(&authlistener_cb_service, (void*)"context test string");

    status = alljoyn_busattachment_enablepeersecurity(servicebus, "ALLJOYN_SRP_KEYX", serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistener_callbacks authlistener_cb_client = {
        authlistener_requestcredentials_client_srp_keyx, //requestcredentials
        NULL, //verifycredentials
        NULL, //securityviolation
        alljoyn_authlistener_authenticationcomplete_client_srp_keyx //authenticationcomplete
    };

    alljoyn_authlistener clientauthlistener = alljoyn_authlistener_create(&authlistener_cb_client, (void*)"context test string");

    status = alljoyn_busattachment_enablepeersecurity(clientbus, "ALLJOYN_SRP_KEYX", clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    SetupAuthClient();

    EXPECT_TRUE(requestcredentials_service_flag);
    EXPECT_TRUE(authenticationcomplete_service_flag);

    EXPECT_TRUE(requestcredentials_client_flag);
    EXPECT_TRUE(authenticationcomplete_client_flag);

    alljoyn_authlistener_destroy(serviceauthlistener);
    alljoyn_authlistener_destroy(clientauthlistener);
}

/* AuthListener callback functions*/
static QCC_BOOL AJ_CALL authlistener_requestcredentials_service_srp_logon(const void* context, const char* authMechanism,
                                                                          const char* peerName, uint16_t authCount,
                                                                          const char* userName, uint16_t credMask,
                                                                          alljoyn_credentials credentials) {
    EXPECT_STREQ("ALLJOYN_SRP_LOGON", authMechanism);
    if (!userName) {
        return QCC_FALSE;
    } else if (credMask & ALLJOYN_CRED_PASSWORD) {
        EXPECT_STREQ("Mr. Cuddles", userName);
        if (strcmp(userName, "Mr. Cuddles") == 0) {
            alljoyn_credentials_setpassword(credentials, "123456");
        } else {
            return QCC_FALSE;
        }
    } else {
        return QCC_FALSE;
    }
    requestcredentials_service_flag = QCC_TRUE;
    return QCC_TRUE;
}

static void AJ_CALL alljoyn_authlistener_authenticationcomplete_service_srp_logon(const void* context, const char* authMechanism,
                                                                                  const char* peerName, QCC_BOOL success) {
    EXPECT_TRUE(success);
    authenticationcomplete_service_flag = QCC_TRUE;
}


static QCC_BOOL AJ_CALL authlistener_requestcredentials_client_srp_logon(const void* context, const char* authMechanism,
                                                                         const char* peerName, uint16_t authCount,
                                                                         const char* userName, uint16_t credMask,
                                                                         alljoyn_credentials credentials) {
    EXPECT_STREQ("ALLJOYN_SRP_LOGON", authMechanism);
    if (credMask & ALLJOYN_CRED_USER_NAME) {
        alljoyn_credentials_setusername(credentials, "Mr. Cuddles");
    }

    if (credMask & ALLJOYN_CRED_PASSWORD) {
        alljoyn_credentials_setpassword(credentials, "123456");
    }
    requestcredentials_client_flag = QCC_TRUE;
    return QCC_TRUE;
}

static void AJ_CALL alljoyn_authlistener_authenticationcomplete_client_srp_logon(const void* context, const char* authMechanism,
                                                                                 const char* peerName, QCC_BOOL success) {
    EXPECT_TRUE(success);
    authenticationcomplete_client_flag = QCC_TRUE;
}

TEST_F(AuthListenerTest, srp_logon) {
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistener_callbacks authlistener_cb_service = {
        authlistener_requestcredentials_service_srp_logon, //requestcredentials
        NULL, //verifycredentials
        NULL, //securityviolation
        alljoyn_authlistener_authenticationcomplete_service_srp_logon //authenticationcomplete
    };

    alljoyn_authlistener serviceauthlistener = alljoyn_authlistener_create(&authlistener_cb_service, NULL);

    status = alljoyn_busattachment_enablepeersecurity(servicebus, "ALLJOYN_SRP_LOGON", serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistener_callbacks authlistener_cb_client = {
        authlistener_requestcredentials_client_srp_logon, //requestcredentials
        NULL, //verifycredentials
        NULL, //securityviolation
        alljoyn_authlistener_authenticationcomplete_client_srp_logon //authenticationcomplete
    };

    alljoyn_authlistener clientauthlistener = alljoyn_authlistener_create(&authlistener_cb_client, NULL);

    status = alljoyn_busattachment_enablepeersecurity(clientbus, "ALLJOYN_SRP_LOGON", clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    SetupAuthClient();

    EXPECT_TRUE(requestcredentials_service_flag);
    EXPECT_TRUE(authenticationcomplete_service_flag);

    EXPECT_TRUE(requestcredentials_client_flag);
    EXPECT_TRUE(authenticationcomplete_client_flag);

    alljoyn_authlistener_destroy(serviceauthlistener);
    alljoyn_authlistener_destroy(clientauthlistener);
}


/* AuthListener callback functions*/
static QCC_BOOL AJ_CALL authlistener_requestcredentials_service_pin_keyx(const void* context, const char* authMechanism,
                                                                         const char* peerName, uint16_t authCount,
                                                                         const char* userName, uint16_t credMask,
                                                                         alljoyn_credentials credentials) {
    EXPECT_STREQ("ALLJOYN_PIN_KEYX", authMechanism);
    if (credMask & ALLJOYN_CRED_PASSWORD) {
        alljoyn_credentials_setpassword(credentials, "FEE_FI_FO_FUM");
    }
    requestcredentials_service_flag = QCC_TRUE;
    return QCC_TRUE;
}

static void AJ_CALL alljoyn_authlistener_authenticationcomplete_service_pin_keyx(const void* context, const char* authMechanism,
                                                                                 const char* peerName, QCC_BOOL success) {
    EXPECT_TRUE(success);
    authenticationcomplete_service_flag = QCC_TRUE;
}


static QCC_BOOL AJ_CALL authlistener_requestcredentials_client_pin_keyx(const void* context, const char* authMechanism,
                                                                        const char* peerName, uint16_t authCount,
                                                                        const char* userName, uint16_t credMask,
                                                                        alljoyn_credentials credentials) {
    EXPECT_STREQ("ALLJOYN_PIN_KEYX", authMechanism);
    if (credMask & ALLJOYN_CRED_PASSWORD) {
        alljoyn_credentials_setpassword(credentials, "FEE_FI_FO_FUM");
    }
    requestcredentials_client_flag = QCC_TRUE;
    return QCC_TRUE;
}

static void AJ_CALL alljoyn_authlistener_authenticationcomplete_client_pin_keyx(const void* context, const char* authMechanism,
                                                                                const char* peerName, QCC_BOOL success) {
    EXPECT_TRUE(success);
    authenticationcomplete_client_flag = QCC_TRUE;
}

TEST_F(AuthListenerTest, pin_keyx) {
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistener_callbacks authlistener_cb_service = {
        authlistener_requestcredentials_service_pin_keyx, //requestcredentials
        NULL, //verifycredentials
        NULL, //securityviolation
        alljoyn_authlistener_authenticationcomplete_service_pin_keyx //authenticationcomplete
    };

    alljoyn_authlistener serviceauthlistener = alljoyn_authlistener_create(&authlistener_cb_service, NULL);

    status = alljoyn_busattachment_enablepeersecurity(servicebus, "ALLJOYN_PIN_KEYX", serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistener_callbacks authlistener_cb_client = {
        authlistener_requestcredentials_client_pin_keyx, //requestcredentials
        NULL, //verifycredentials
        NULL, //securityviolation
        alljoyn_authlistener_authenticationcomplete_client_pin_keyx //authenticationcomplete
    };

    alljoyn_authlistener clientauthlistener = alljoyn_authlistener_create(&authlistener_cb_client, NULL);

    status = alljoyn_busattachment_enablepeersecurity(clientbus, "ALLJOYN_PIN_KEYX", clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    SetupAuthClient();

    EXPECT_TRUE(requestcredentials_service_flag);
    EXPECT_TRUE(authenticationcomplete_service_flag);

    EXPECT_TRUE(requestcredentials_client_flag);
    EXPECT_TRUE(authenticationcomplete_client_flag);

    alljoyn_authlistener_destroy(serviceauthlistener);
    alljoyn_authlistener_destroy(clientauthlistener);
}


static const char service_x509certChain[] = {
    // User certificate
    "-----BEGIN CERTIFICATE-----\n"
    "MIICxzCCAjCgAwIBAgIJALZkSW0TWinQMA0GCSqGSIb3DQEBBQUAME8xCzAJBgNV\n"
    "BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMQ0wCwYDVQQKEwRRdUlDMQ0wCwYD\n"
    "VQQLEwRNQnVzMQ0wCwYDVQQDEwRHcmVnMB4XDTEwMDgyNTIzMTYwNVoXDTExMDgy\n"
    "NTIzMTYwNVowfzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAO\n"
    "BgNVBAcTB1NlYXR0bGUxIzAhBgNVBAoTGlF1YWxjb21tIElubm92YXRpb24gQ2Vu\n"
    "dGVyMREwDwYDVQQLEwhNQnVzIGRldjERMA8GA1UEAxMIU2VhIEtpbmcwgZ8wDQYJ\n"
    "KoZIhvcNAQEBBQADgY0AMIGJAoGBALz+YZcH0DZn91sjOA5vaTwjQVBnbR9ZRpCA\n"
    "kGD2am0F91juEPFvj/PAlvVLPd5nwGKSPiycN3l3ECxNerTrwIG2XxzBWantFn5n\n"
    "7dDzlRm3aerFr78EJmcCiImwgqsuhUT4eo5/jn457vANO9B5k/1ddc6zJ67Jvuh6\n"
    "0p4YAW4NAgMBAAGjezB5MAkGA1UdEwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5T\n"
    "U0wgR2VuZXJhdGVkIENlcnRpZmljYXRlMB0GA1UdDgQWBBTXau+rH64d658efvkF\n"
    "jkaEZJ+5BTAfBgNVHSMEGDAWgBTu5FqZL5ShsNq4KJjOo8IPZ70MBTANBgkqhkiG\n"
    "9w0BAQUFAAOBgQBNBt7+/IaqGUSOpYAgHun87c86J+R38P2dmOm+wk8CNvKExdzx\n"
    "Hp08aA51d5YtGrkDJdKXfC+Ly0CuE2SCiMU4RbK9Pc2H/MRQdmn7ZOygisrJNgRK\n"
    "Gerh1OQGuc1/USAFpfD2rd+xqndp1WZz7iJh+ezF44VMUlo2fTKjYr5jMQ==\n"
    "-----END CERTIFICATE-----\n"
    // Root certificate
    "-----BEGIN CERTIFICATE-----\n"
    "MIICzjCCAjegAwIBAgIJALZkSW0TWinPMA0GCSqGSIb3DQEBBQUAME8xCzAJBgNV\n"
    "BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMQ0wCwYDVQQKEwRRdUlDMQ0wCwYD\n"
    "VQQLEwRNQnVzMQ0wCwYDVQQDEwRHcmVnMB4XDTEwMDgyNTIzMTQwNloXDTEzMDgy\n"
    "NDIzMTQwNlowTzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xDTAL\n"
    "BgNVBAoTBFF1SUMxDTALBgNVBAsTBE1CdXMxDTALBgNVBAMTBEdyZWcwgZ8wDQYJ\n"
    "KoZIhvcNAQEBBQADgY0AMIGJAoGBANc1GTPfvD347zk1NlZbDhTf5txn3AcSG//I\n"
    "gdgdZOY7ubXkNMGEVBMyZDXe7K36MEmj5hfXRiqfZwpZjjzJeJBoPJvXkETzatjX\n"
    "vs4d5k1m0UjzANXp01T7EK1ZdIP7AjLg4QMk+uj8y7x3nElmSpNvPf3tBe3JUe6t\n"
    "Io22NI/VAgMBAAGjgbEwga4wHQYDVR0OBBYEFO7kWpkvlKGw2rgomM6jwg9nvQwF\n"
    "MH8GA1UdIwR4MHaAFO7kWpkvlKGw2rgomM6jwg9nvQwFoVOkUTBPMQswCQYDVQQG\n"
    "EwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjENMAsGA1UEChMEUXVJQzENMAsGA1UE\n"
    "CxMETUJ1czENMAsGA1UEAxMER3JlZ4IJALZkSW0TWinPMAwGA1UdEwQFMAMBAf8w\n"
    "DQYJKoZIhvcNAQEFBQADgYEAg3pDFX0270jUTf8mFJHJ1P+CeultB+w4EMByTBfA\n"
    "ZPNOKzFeoZiGe2AcMg41VXvaKJA0rNH+5z8zvVAY98x1lLKsJ4fb4aIFGQ46UZ35\n"
    "DMrqZYmULjjSXWMxiphVRf1svKGU4WHR+VSvtUNLXzQyvg2yUb6PKDPUQwGi9kDx\n"
    "tCI=\n"
    "-----END CERTIFICATE-----\n"
};

static const char service_privKey[] = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: DES-EDE3-CBC,86B9DBED35AEBAB3\n"
    "\n"
    "f28sibgVCkDz3VNoC/MzazG2tFj+KGf6xm9LQki/GsxpMhJsEEvT9dUluT1T4Ypr\n"
    "NjG+nBleLcfdHxOl5XHnusn8r/JVaQQGVSnDaeP/27KiirtB472p+8Wc2wfXexRz\n"
    "uSUv0DJT+Fb52zYGiGzwgaOinQEBskeO9AwRyG34sFKqyyapyJtSZDjh+wUAIMZb\n"
    "wKifvl1KHSCbXEhjDVlxBw4Rt7I36uKzTY5oax2L6W6gzxfHuOtzfVelAaM46j+n\n"
    "KANZgx6KGW2DKk27aad2HEZUYeDwznpwU5Duw9b0DeMTkez6CuayiZHb5qEod+0m\n"
    "pCCMwpqxFCJ/vg1VJjmxM7wpCQTc5z5cjX8saV5jMUJXp09NuoU/v8TvhOcXOE1T\n"
    "ENukIWYBT1HC9MJArroLwl+fMezKCu+F/JC3M0RfI0dlQqS4UWH+Uv+Ujqa2yr9y\n"
    "20zYS52Z4kyq2WnqwBk1//PLBl/bH/awWXPUI2yMnIILbuCisRYLyK52Ge/rS51P\n"
    "vUgUCZ7uoEJGTX6EGh0yQhp+5jGYVdHHZB840AyxzBQx7pW4MtTwqkw1NZuQcdSN\n"
    "IU9y/PferHhMKZeGfVRVEkAOcjeXOqvSi6NKDvYn7osCkvj9h7K388o37VMPSacR\n"
    "jDwDTT0HH/UcM+5v/74NgE/OebaK3YfxBVyMmBzi0WVFXgxHJir4xpj9c20YQVw9\n"
    "hE3kYepW8gGz/JPQmRszwLQpwQNEP60CgQveqtH7tZVXzDkElvSyveOdjJf1lw4B\n"
    "uCz54678UNNeIe7YB4yV1dMVhhcoitn7G/+jC9Qk3FTnuP+Ws5c/0g==\n"
    "-----END RSA PRIVATE KEY-----"
};

static const char client_x509cert[] = {
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

static const char client_privKey[] = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: AES-128-CBC,0AE4BAB94CEAA7829273DD861B067DBA\n"
    "\n"
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
    "-----END RSA PRIVATE KEY-----"
};

/* AuthListener callback functions*/
static QCC_BOOL AJ_CALL authlistener_requestcredentials_service_rsa_keyx(const void* context, const char* authMechanism,
                                                                         const char* peerName, uint16_t authCount,
                                                                         const char* userName, uint16_t credMask,
                                                                         alljoyn_credentials credentials) {
    EXPECT_STREQ("ALLJOYN_RSA_KEYX", authMechanism);
    if (credMask & ALLJOYN_CRED_CERT_CHAIN) {
        alljoyn_credentials_setcertchain(credentials, service_x509certChain);
    }
    if (credMask & ALLJOYN_CRED_PRIVATE_KEY) {
        alljoyn_credentials_setprivatekey(credentials, service_privKey);
    }
    if (credMask & ALLJOYN_CRED_PASSWORD) {
        alljoyn_credentials_setpassword(credentials, "123456");
    }
    requestcredentials_service_flag = QCC_TRUE;
    return QCC_TRUE;
}

static QCC_BOOL AJ_CALL authlistener_verifycredentials_service_rsa_keyx(const void* context, const char* authMechanism,
                                                                        const char* peerName, const alljoyn_credentials credentials) {
    // TODO add code that actually verifies the alljoyn_credentials.
    verifycredentials_service_flag = QCC_TRUE;
    return QCC_TRUE;
}

static void AJ_CALL alljoyn_authlistener_authenticationcomplete_service_rsa_keyx(const void* context, const char* authMechanism,
                                                                                 const char* peerName, QCC_BOOL success) {
    EXPECT_TRUE(success);
    authenticationcomplete_service_flag = QCC_TRUE;
}


static QCC_BOOL AJ_CALL authlistener_requestcredentials_client_rsa_keyx(const void* context, const char* authMechanism,
                                                                        const char* peerName, uint16_t authCount,
                                                                        const char* userName, uint16_t credMask,
                                                                        alljoyn_credentials credentials) {
    EXPECT_STREQ("ALLJOYN_RSA_KEYX", authMechanism);
    if (credMask & ALLJOYN_CRED_CERT_CHAIN) {
        alljoyn_credentials_setcertchain(credentials, client_x509cert);
    }
    if (credMask & ALLJOYN_CRED_PRIVATE_KEY) {
        alljoyn_credentials_setprivatekey(credentials, client_privKey);
    }
    if (credMask & ALLJOYN_CRED_PASSWORD) {
        alljoyn_credentials_setpassword(credentials, "123456");
    }
    requestcredentials_client_flag = QCC_TRUE;
    return QCC_TRUE;
}

static QCC_BOOL AJ_CALL authlistener_verifycredentials_client_rsa_keyx(const void* context, const char* authMechanism,
                                                                       const char* peerName, const alljoyn_credentials credentials) {
    // TODO add code that actually verifies the alljoyn_credentials.
    verifycredentials_client_flag = QCC_TRUE;
    return QCC_TRUE;
}

static void AJ_CALL alljoyn_authlistener_authenticationcomplete_client_rsa_keyx(const void* context, const char* authMechanism,
                                                                                const char* peerName, QCC_BOOL success) {
    EXPECT_TRUE(success);
    authenticationcomplete_client_flag = QCC_TRUE;
}

TEST_F(AuthListenerTest, rsa_keyx) {
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistener_callbacks authlistener_cb_service = {
        authlistener_requestcredentials_service_rsa_keyx, //requestcredentials
        authlistener_verifycredentials_service_rsa_keyx, //verifycredentials
        NULL, //securityviolation
        alljoyn_authlistener_authenticationcomplete_service_rsa_keyx //authenticationcomplete
    };

    alljoyn_authlistener serviceauthlistener = alljoyn_authlistener_create(&authlistener_cb_service, NULL);

    status = alljoyn_busattachment_enablepeersecurity(servicebus, "ALLJOYN_RSA_KEYX", serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistener_callbacks authlistener_cb_client = {
        authlistener_requestcredentials_client_rsa_keyx, //requestcredentials
        authlistener_verifycredentials_client_rsa_keyx, //verifycredentials
        NULL, //securityviolation
        alljoyn_authlistener_authenticationcomplete_client_rsa_keyx //authenticationcomplete
    };

    alljoyn_authlistener clientauthlistener = alljoyn_authlistener_create(&authlistener_cb_client, NULL);

    status = alljoyn_busattachment_enablepeersecurity(clientbus, "ALLJOYN_RSA_KEYX", clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    SetupAuthClient();

    EXPECT_TRUE(requestcredentials_service_flag);
    EXPECT_TRUE(authenticationcomplete_service_flag);
    EXPECT_TRUE(verifycredentials_service_flag);

    EXPECT_TRUE(requestcredentials_client_flag);
    EXPECT_TRUE(authenticationcomplete_client_flag);
    EXPECT_TRUE(verifycredentials_client_flag);

    alljoyn_authlistener_destroy(serviceauthlistener);
    alljoyn_authlistener_destroy(clientauthlistener);
}


/* AuthListener callback functions*/
static QCC_BOOL AJ_CALL authlistener_requestcredentials_service_srp_keyx2(const void* context, const char* authMechanism,
                                                                          const char* peerName, uint16_t authCount,
                                                                          const char* userName, uint16_t credMask,
                                                                          alljoyn_credentials credentials) {
    EXPECT_STREQ("ALLJOYN_SRP_KEYX", authMechanism);
    requestcredentials_service_flag = QCC_TRUE;
    return QCC_FALSE;
}

static void AJ_CALL alljoyn_authlistener_authenticationcomplete_service_srp_keyx2(const void* context, const char* authMechanism,
                                                                                  const char* peerName, QCC_BOOL success) {
    EXPECT_FALSE(success);
    authenticationcomplete_service_flag = QCC_TRUE;
}


static QCC_BOOL AJ_CALL authlistener_requestcredentials_client_srp_keyx2(const void* context, const char* authMechanism,
                                                                         const char* peerName, uint16_t authCount,
                                                                         const char* userName, uint16_t credMask,
                                                                         alljoyn_credentials credentials) {
    EXPECT_STREQ("ALLJOYN_SRP_KEYX", authMechanism);
    requestcredentials_client_flag = QCC_TRUE;
    return QCC_FALSE;
}

static void AJ_CALL authlistener_securityviolation_client_srp_keyx2(const void* context, QStatus status, const alljoyn_message msg) {
    securityviolation_client_flag = QCC_TRUE;
}

/*
 * Run the SRP Key Exchange test again except this time fail the authentication
 * we expect to see an authlistener secutity violation.
 */
static void AJ_CALL alljoyn_authlistener_authenticationcomplete_client_srp_keyx2(const void* context, const char* authMechanism,
                                                                                 const char* peerName, QCC_BOOL success) {
    EXPECT_FALSE(success);
    authenticationcomplete_client_flag = QCC_TRUE;
}

TEST_F(AuthListenerTest, srp_keyx2) {
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistener_callbacks authlistener_cb_service = {
        authlistener_requestcredentials_service_srp_keyx2, //requestcredentials
        NULL, //verifycredentials
        NULL, //securityviolation
        alljoyn_authlistener_authenticationcomplete_service_srp_keyx2 //authenticationcomplete
    };

    alljoyn_authlistener serviceauthlistener = alljoyn_authlistener_create(&authlistener_cb_service, NULL);

    status = alljoyn_busattachment_enablepeersecurity(servicebus, "ALLJOYN_SRP_KEYX", serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistener_callbacks authlistener_cb_client = {
        authlistener_requestcredentials_client_srp_keyx2, //requestcredentials
        NULL, //verifycredentials
        authlistener_securityviolation_client_srp_keyx2, //securityviolation
        alljoyn_authlistener_authenticationcomplete_client_srp_keyx2 //authenticationcomplete
    };

    alljoyn_authlistener clientauthlistener = alljoyn_authlistener_create(&authlistener_cb_client, NULL);

    status = alljoyn_busattachment_enablepeersecurity(clientbus, "ALLJOYN_SRP_KEYX", clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    SetupAuthClientAuthFail();

    //Wait upto 2 seconds for the signal to complete.
    for (int i = 0; i < 200; ++i) {

        if (securityviolation_client_flag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(requestcredentials_service_flag);
    EXPECT_TRUE(authenticationcomplete_service_flag);

    EXPECT_TRUE(authenticationcomplete_client_flag);
    EXPECT_TRUE(securityviolation_client_flag);

    alljoyn_authlistener_destroy(serviceauthlistener);
    alljoyn_authlistener_destroy(clientauthlistener);
}

/*
 * This test re-uses the authlisteners used for the srp_keyx unit test.
 * It is unimportant to know what authlistener is used just that authentication
 * is being done when alljoyn_proxybusobject_secureconnection is called.
 */
TEST_F(AuthListenerTest, secureconnection) {
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistener_callbacks authlistener_cb_service = {
        authlistener_requestcredentials_service_srp_keyx, //requestcredentials
        NULL, //verifycredentials
        NULL, //securityviolation
        alljoyn_authlistener_authenticationcomplete_service_srp_keyx //authenticationcomplete
    };

    alljoyn_authlistener serviceauthlistener = alljoyn_authlistener_create(&authlistener_cb_service, (void*)"context test string");

    status = alljoyn_busattachment_enablepeersecurity(servicebus, "ALLJOYN_SRP_KEYX", serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistener_callbacks authlistener_cb_client = {
        authlistener_requestcredentials_client_srp_keyx, //requestcredentials
        NULL, //verifycredentials
        NULL, //securityviolation
        alljoyn_authlistener_authenticationcomplete_client_srp_keyx //authenticationcomplete
    };

    alljoyn_authlistener clientauthlistener = alljoyn_authlistener_create(&authlistener_cb_client, (void*)"context test string");

    status = alljoyn_busattachment_enablepeersecurity(clientbus, "ALLJOYN_SRP_KEYX", clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create(clientbus, OBJECT_NAME, OBJECT_PATH, 0);
    ASSERT_TRUE(proxyObj);

    status = alljoyn_proxybusobject_secureconnection(proxyObj, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(requestcredentials_service_flag);
    EXPECT_TRUE(authenticationcomplete_service_flag);

    EXPECT_TRUE(requestcredentials_client_flag);
    EXPECT_TRUE(authenticationcomplete_client_flag);

    ResetAuthFlags();
    /*
     * the peer-to-peer connection should have been authenticated with the last
     * call to alljoyn_proxybusobject_secureconnection this call should return
     * ER_OK with out calling any of the authlistener functions.
     */
    status = alljoyn_proxybusobject_secureconnection(proxyObj, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_FALSE(requestcredentials_service_flag);
    EXPECT_FALSE(authenticationcomplete_service_flag);

    EXPECT_FALSE(requestcredentials_client_flag);
    EXPECT_FALSE(authenticationcomplete_client_flag);

    ResetAuthFlags();

    /*
     * Although the peer-to-peer connection has already been authenticated we
     * are forcing re-authentication so we expect the authlistener functions to
     * be called again.
     */
    status = alljoyn_proxybusobject_secureconnection(proxyObj, QCC_TRUE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(requestcredentials_service_flag);
    EXPECT_TRUE(authenticationcomplete_service_flag);

    EXPECT_TRUE(requestcredentials_client_flag);
    EXPECT_TRUE(authenticationcomplete_client_flag);

    alljoyn_proxybusobject_destroy(proxyObj);

    alljoyn_authlistener_destroy(serviceauthlistener);
    alljoyn_authlistener_destroy(clientauthlistener);
}
