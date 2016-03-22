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

#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/AuthListener.h>
#include <alljoyn_c/Status.h>

#include <alljoyn_c/AjAPI.h>
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
    QCC_UNUSED(context);
    QCC_UNUSED(previousOwner);
    QCC_UNUSED(newOwner);

    if (strcmp(busName, OBJECT_NAME) == 0) {
        name_owner_changed_flag = QCC_TRUE;
    }
}

/* Exposed methods */
static void AJ_CALL ping_method(alljoyn_busobject bus, const alljoyn_interfacedescription_member* member, alljoyn_message msg)
{
    QCC_UNUSED(member);

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
        EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("AuthListenerTestService"));
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
        EXPECT_EQ(ER_OK, DeleteDefaultKeyStoreFileCTest("AuthListenerTestClient"));
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

    void SetUpAuthClient() {
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

    void SetUpAuthClientAuthFail() {
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

    /* Helper methods to run a test with different authentication mechanisms. */
    void RunAuthSucceedsTest(const char* mechanism, const char* context_string);
    void RunAuthFailsTest(const char* mechanism);
    void RunSecureConnectionTest(const char* mechanism, const char* context_string);

    void RunAsyncAuthSucceedsTest(const char* mechanism, const char* context_string);
    void RunAsyncAuthFailsTest(const char* mechanism);
    void RunAsyncSecureConnectionTest(const char* mechanism, const char* context_string);

    QStatus status;
    alljoyn_busattachment servicebus;
    alljoyn_busattachment clientbus;

    alljoyn_busobject testObj;
    alljoyn_buslistener buslistener;
};

/* AuthListener callback functions*/
static QCC_BOOL AJ_CALL authlistener_requestcredentials_service(const void* context, const char* authMechanism,
                                                                const char* peerName, uint16_t authCount,
                                                                const char* userName, uint16_t credMask,
                                                                alljoyn_credentials credentials) {
    QCC_UNUSED(peerName);
    QCC_UNUSED(authCount);

    if (strcmp("ALLJOYN_SRP_KEYX", authMechanism) == 0) {
        EXPECT_STREQ("context test string", (const char*)context);
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(credentials, "ABCDEFGH");
        }
        requestcredentials_service_flag = QCC_TRUE;
        return QCC_TRUE;
    }

    if (strcmp("ALLJOYN_ECDHE_SPEKE", authMechanism) == 0) {
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(credentials, "ABCDEFGH");
        }
        requestcredentials_service_flag = QCC_TRUE;
        return QCC_TRUE;
    }

    if (strcmp("ALLJOYN_SRP_LOGON", authMechanism) == 0) {
        if (userName == nullptr) {
            return QCC_FALSE;
        } else if (credMask & ALLJOYN_CRED_PASSWORD) {
            EXPECT_STREQ("Mr. Cuddles", userName);
            if (strcmp(userName, "Mr. Cuddles") == 0) {
                alljoyn_credentials_setpassword(credentials, "123456");
            } else {
                printf("authlistener_requestcredentials_service: unknown username");
                return QCC_FALSE;
            }
        } else {
            printf("authlistener_requestcredentials_service: invalid credential type");
            return QCC_FALSE;
        }
        requestcredentials_service_flag = QCC_TRUE;
        return QCC_TRUE;
    }

    printf("authlistener_requestcredentials_service: invalid auth mechanism");
    return QCC_FALSE;

}

static QCC_BOOL AJ_CALL authlistener_requestcredentials_client(const void* context, const char* authMechanism,
                                                               const char* peerName, uint16_t authCount,
                                                               const char* userName, uint16_t credMask,
                                                               alljoyn_credentials credentials) {
    QCC_UNUSED(peerName);
    QCC_UNUSED(authCount);
    QCC_UNUSED(userName);

    if (strcmp("ALLJOYN_SRP_KEYX", authMechanism) == 0) {
        EXPECT_STREQ("context test string", (const char*)context);
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(credentials, "ABCDEFGH");
        }
        requestcredentials_client_flag = QCC_TRUE;
        return QCC_TRUE;
    }

    if (strcmp("ALLJOYN_ECDHE_SPEKE", authMechanism) == 0) {
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(credentials, "ABCDEFGH");
        }
        requestcredentials_client_flag = QCC_TRUE;
        return QCC_TRUE;
    }

    if (strcmp("ALLJOYN_SRP_LOGON", authMechanism) == 0) {
        if (credMask & ALLJOYN_CRED_USER_NAME) {
            alljoyn_credentials_setusername(credentials, "Mr. Cuddles");
        }

        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(credentials, "123456");
        }
        requestcredentials_client_flag = QCC_TRUE;
        return QCC_TRUE;
    }

    printf("authlistener_requestcredentials_client: invalid auth mechanism");
    return QCC_FALSE;
}

static void AJ_CALL authlistener_authenticationcomplete_service(const void* context, const char* authMechanism,
                                                                const char* peerName, QCC_BOOL success) {
    QCC_UNUSED(peerName);

    if (strcmp("ALLJOYN_SRP_KEYX", authMechanism) == 0) {
        EXPECT_STREQ("context test string", (const char*)context);
    }
    EXPECT_TRUE(success);
    authenticationcomplete_service_flag = QCC_TRUE;
}

static void AJ_CALL authlistener_authenticationcomplete_client(const void* context, const char* authMechanism,
                                                               const char* peerName, QCC_BOOL success) {
    QCC_UNUSED(peerName);

    if (strcmp("ALLJOYN_SRP_KEYX", authMechanism) == 0) {
        EXPECT_STREQ("context test string", (const char*)context);
    }
    EXPECT_TRUE(success) << "Client authentication complete callback called with QCC_FALSE, indicating authentication failed";
    authenticationcomplete_client_flag = QCC_TRUE;
}

/* Failing RequestCredentials implementation, to test the case when no password is provided. */
static QCC_BOOL AJ_CALL authlistener_requestcredentials_service_fails(const void* context, const char* authMechanism,
                                                                      const char* peerName, uint16_t authCount,
                                                                      const char* userName, uint16_t credMask,
                                                                      alljoyn_credentials credentials) {
    QCC_UNUSED(context);
    QCC_UNUSED(peerName);
    QCC_UNUSED(authCount);
    QCC_UNUSED(userName);
    QCC_UNUSED(credMask);
    QCC_UNUSED(credentials);

    if ((strcmp("ALLJOYN_SRP_KEYX", authMechanism) == 0) ||
        (strcmp("ALLJOYN_ECDHE_SPEKE", authMechanism) == 0) ||
        (strcmp("ALLJOYN_SRP_LOGON", authMechanism) == 0)) {
        requestcredentials_service_flag = QCC_TRUE;
    }

    return QCC_FALSE;
}

/* Corresponding authenticationcomplete implementation; it EXPECTs authentication to fail. */
static void AJ_CALL authlistener_authenticationcomplete_service_fails(const void* context, const char* authMechanism,
                                                                      const char* peerName, QCC_BOOL success) {
    QCC_UNUSED(peerName);

    if (strcmp("ALLJOYN_SRP_KEYX", authMechanism) == 0) {
        EXPECT_STREQ("context test string", (const char*)context);
    }
    EXPECT_FALSE(success);
    authenticationcomplete_service_flag = QCC_TRUE;
}

static void AJ_CALL authlistener_authenticationcomplete_client_fails(const void* context, const char* authMechanism,
                                                                     const char* peerName, QCC_BOOL success) {
    QCC_UNUSED(peerName);

    if (strcmp("ALLJOYN_SRP_KEYX", authMechanism) == 0) {
        EXPECT_STREQ("context test string", (const char*)context);
    }
    EXPECT_FALSE(success);
    authenticationcomplete_client_flag = QCC_TRUE;
}

static void AJ_CALL authlistener_securityviolation_client(const void* context, QStatus status, const alljoyn_message msg) {
    QCC_UNUSED(context);
    QCC_UNUSED(status);
    QCC_UNUSED(msg);
    securityviolation_client_flag = QCC_TRUE;
}

static void AJ_CALL authlistener_securityviolation_service(const void* context, QStatus status, const alljoyn_message msg) {
    QCC_UNUSED(context);
    QCC_UNUSED(status);
    QCC_UNUSED(msg);

    securityviolation_service_flag = QCC_TRUE;
}

/* Asynchronous versions of RequestCredentials*/
static QStatus AJ_CALL authlistener_requestcredentialsasync_service(const void* context, alljoyn_authlistener listener,
                                                                    const char* authMechanism, const char* peerName,
                                                                    uint16_t authCount, const char* userName,
                                                                    uint16_t credMask, void* authContext) {
    QCC_UNUSED(peerName);
    QCC_UNUSED(authCount);

    QStatus status = ER_FAIL;
    alljoyn_credentials creds = alljoyn_credentials_create();
    if (strcmp("ALLJOYN_SRP_KEYX", authMechanism) == 0) {
        EXPECT_STREQ("context test string", (const char*)context);

        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(creds, "ABCDEFGH");
        }
        status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
        requestcredentials_service_flag = QCC_TRUE;
    } else if (strcmp("ALLJOYN_SRP_LOGON", authMechanism) == 0) {
        if (userName == nullptr) {
            status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_FALSE, creds);
        } else if (credMask & ALLJOYN_CRED_PASSWORD) {
            EXPECT_STREQ("Mr. Cuddles", userName);
            if (strcmp(userName, "Mr. Cuddles") == 0) {
                alljoyn_credentials_setpassword(creds, "123456");
                status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
            } else {
                status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_FALSE, creds);
            }
        } else {
            status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_FALSE, creds);
        }
        requestcredentials_service_flag = QCC_TRUE;
    } else if (strcmp("ALLJOYN_ECDHE_SPEKE", authMechanism) == 0) {
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(creds, "ABCDEFGH");
        }
        status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
        requestcredentials_service_flag = QCC_TRUE;
    }

    alljoyn_credentials_destroy(creds);

    return status;
}

static QStatus AJ_CALL authlistener_requestcredentialsasync_client(const void* context, alljoyn_authlistener listener,
                                                                   const char* authMechanism, const char* peerName,
                                                                   uint16_t authCount, const char* userName,
                                                                   uint16_t credMask, void* authContext) {
    QCC_UNUSED(peerName);
    QCC_UNUSED(authCount);
    QCC_UNUSED(userName);

    QStatus status = ER_FAIL;
    alljoyn_credentials creds = alljoyn_credentials_create();
    if (strcmp("ALLJOYN_SRP_KEYX", authMechanism) == 0) {
        EXPECT_STREQ("context test string", (const char*)context);
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(creds, "ABCDEFGH");
        }
        status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
        requestcredentials_client_flag = QCC_TRUE;
    } else if (strcmp("ALLJOYN_SRP_LOGON", authMechanism) == 0) {
        if (credMask & ALLJOYN_CRED_USER_NAME) {
            alljoyn_credentials_setusername(creds, "Mr. Cuddles");
        }

        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(creds, "123456");
        }
        status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
        requestcredentials_client_flag = QCC_TRUE;
    } else if (strcmp("ALLJOYN_ECDHE_SPEKE", authMechanism) == 0) {
        if (credMask & ALLJOYN_CRED_PASSWORD) {
            alljoyn_credentials_setpassword(creds, "ABCDEFGH");
        }
        status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_TRUE, creds);
        requestcredentials_client_flag = QCC_TRUE;
    }

    alljoyn_credentials_destroy(creds);

    return status;
}

/* Failing RequestCredentials implementation, to test the case when no password is provided. */
static QStatus AJ_CALL authlistener_requestcredentialsasync_service_fails(const void* context, alljoyn_authlistener listener,
                                                                          const char* authMechanism, const char* peerName,
                                                                          uint16_t authCount, const char* userName,
                                                                          uint16_t credMask, void* authContext) {
    QCC_UNUSED(context);
    QCC_UNUSED(peerName);
    QCC_UNUSED(userName);
    QCC_UNUSED(credMask);
    QCC_UNUSED(authCount);

    QStatus status = ER_FAIL;
    if (strcmp("ALLJOYN_SRP_KEYX", authMechanism) == 0    ||
        strcmp("ALLJOYN_ECDHE_SPEKE", authMechanism) == 0 ||
        strcmp("ALLJOYN_SRP_LOGON", authMechanism) == 0) {

        alljoyn_credentials creds = alljoyn_credentials_create();
        QStatus status = alljoyn_authlistener_requestcredentialsresponse(listener, authContext, QCC_FALSE, creds);
        requestcredentials_service_flag = QCC_TRUE;
        alljoyn_credentials_destroy(creds);
        return status;
    }

    return status;
}

/*
 * Tests for successful authentication
 */

void AuthListenerTest::RunAuthSucceedsTest(const char* mechanism, const char* context_string)
{
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistener_callbacks authlistener_cb_service = {
        authlistener_requestcredentials_service,
        NULL, //verifycredentials
        NULL, //securityviolation
        authlistener_authenticationcomplete_service
    };
    alljoyn_authlistener serviceauthlistener = alljoyn_authlistener_create(&authlistener_cb_service, (void*)context_string);

    status = alljoyn_busattachment_enablepeersecurity(servicebus, mechanism, serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistener_callbacks authlistener_cb_client = {
        authlistener_requestcredentials_client,
        NULL, //verifycredentials
        NULL, //securityviolation
        authlistener_authenticationcomplete_client
    };
    alljoyn_authlistener clientauthlistener = alljoyn_authlistener_create(&authlistener_cb_client, (void*)context_string);

    status = alljoyn_busattachment_enablepeersecurity(clientbus, mechanism, clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    SetUpAuthClient();

    EXPECT_TRUE(requestcredentials_service_flag);
    EXPECT_TRUE(authenticationcomplete_service_flag);

    EXPECT_TRUE(requestcredentials_client_flag);
    EXPECT_TRUE(authenticationcomplete_client_flag);

    alljoyn_authlistener_destroy(serviceauthlistener);
    alljoyn_authlistener_destroy(clientauthlistener);
}

TEST_F(AuthListenerTest, AuthSucceeds_SRP_LOGON) {
    RunAuthSucceedsTest("ALLJOYN_SRP_LOGON", NULL);
}

TEST_F(AuthListenerTest, AuthSucceeds_SRP_KEYX) {
    RunAuthSucceedsTest("ALLJOYN_SRP_KEYX", "context test string");
}

TEST_F(AuthListenerTest, AuthSucceeds_SPEKE) {
    RunAuthSucceedsTest("ALLJOYN_ECDHE_SPEKE", NULL);
}

/*
 * Tests for successful authentication, with asynchronous callback.
 */

void AuthListenerTest::RunAsyncAuthSucceedsTest(const char* mechanism, const char* context_string)
{
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistenerasync_callbacks authlistenerasync_cb_service = {
        authlistener_requestcredentialsasync_service,
        NULL, //verifycredentials
        NULL, //securityviolation
        authlistener_authenticationcomplete_service
    };
    alljoyn_authlistener serviceauthlistener = alljoyn_authlistenerasync_create(&authlistenerasync_cb_service, (void*)context_string);

    status = alljoyn_busattachment_enablepeersecurity(servicebus, mechanism, serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistenerasync_callbacks authlistenerasync_cb_client = {
        authlistener_requestcredentialsasync_client,
        NULL, //verifycredentials
        NULL, //securityviolation
        authlistener_authenticationcomplete_client
    };
    alljoyn_authlistener clientauthlistener = alljoyn_authlistenerasync_create(&authlistenerasync_cb_client, (void*)context_string);

    status = alljoyn_busattachment_enablepeersecurity(clientbus, mechanism, clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    SetUpAuthClient();

    EXPECT_TRUE(requestcredentials_service_flag);
    EXPECT_TRUE(authenticationcomplete_service_flag);

    EXPECT_TRUE(requestcredentials_client_flag);
    EXPECT_TRUE(authenticationcomplete_client_flag);

    alljoyn_authlistenerasync_destroy(serviceauthlistener);
    alljoyn_authlistenerasync_destroy(clientauthlistener);
}

TEST_F(AuthListenerTest, AsyncAuthSucceeds_SRP_LOGON) {
    RunAsyncAuthSucceedsTest("ALLJOYN_SRP_LOGON", NULL);
}

TEST_F(AuthListenerTest, AsyncAuthSucceeds_SRP_KEYX) {
    RunAsyncAuthSucceedsTest("ALLJOYN_SRP_KEYX", "context test string");
}

TEST_F(AuthListenerTest, AsyncAuthSucceeds_SPEKE) {
    RunAsyncAuthSucceedsTest("ALLJOYN_ECDHE_SPEKE", NULL);
}

/*
 * Tests for failing authentication. Expect to see an authlistener security violation.
 */

void AuthListenerTest::RunAuthFailsTest(const char* mechanism)
{
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistener_callbacks authlistener_cb_service = {
        authlistener_requestcredentials_service_fails,
        NULL, //verifycredentials
        authlistener_securityviolation_service,
        authlistener_authenticationcomplete_service_fails
    };
    alljoyn_authlistener serviceauthlistener = alljoyn_authlistener_create(&authlistener_cb_service, (void*)"context test string");

    status = alljoyn_busattachment_enablepeersecurity(servicebus, mechanism, serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistener_callbacks authlistener_cb_client = {
        authlistener_requestcredentials_client, /* The client does provide a password, but expects authentication to fail */
        NULL, //verifycredentials
        authlistener_securityviolation_client,
        authlistener_authenticationcomplete_client_fails
    };
    alljoyn_authlistener clientauthlistener = alljoyn_authlistener_create(&authlistener_cb_client, (void*)"context test string");

    status = alljoyn_busattachment_enablepeersecurity(clientbus, mechanism, clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    SetUpAuthClientAuthFail();

    /* Wait up to 2 seconds for the signal to complete. */
    for (int i = 0; i < 200; ++i) {

        if (securityviolation_client_flag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(requestcredentials_service_flag);

    EXPECT_TRUE(authenticationcomplete_client_flag);
    EXPECT_TRUE(securityviolation_client_flag);

    alljoyn_authlistener_destroy(serviceauthlistener);
    alljoyn_authlistener_destroy(clientauthlistener);

}

TEST_F(AuthListenerTest, AuthFails_SRP_KEYX) {
    RunAuthFailsTest("ALLJOYN_SRP_KEYX");
}

TEST_F(AuthListenerTest, AuthFails_SPEKE) {
    RunAuthFailsTest("ALLJOYN_ECDHE_SPEKE");
}

/*
 * Tests for failing authentication, with asynchronous callback. Expect to see an authlistener security violation.
 */

void AuthListenerTest::RunAsyncAuthFailsTest(const char* mechanism)
{
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistenerasync_callbacks authlistenerasync_cb_service = {
        authlistener_requestcredentialsasync_service_fails,
        NULL, //verifycredentials
        authlistener_securityviolation_service,
        authlistener_authenticationcomplete_service_fails
    };
    alljoyn_authlistener serviceauthlistener = alljoyn_authlistenerasync_create(&authlistenerasync_cb_service, (void*)"context test string");

    status = alljoyn_busattachment_enablepeersecurity(servicebus, mechanism, serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistenerasync_callbacks authlistenerasync_cb_client = {
        authlistener_requestcredentialsasync_client, /* The client does provide a password, but expects authentication to fail */
        NULL, //verifycredentials
        authlistener_securityviolation_client,
        authlistener_authenticationcomplete_client_fails
    };
    alljoyn_authlistener clientauthlistener = alljoyn_authlistenerasync_create(&authlistenerasync_cb_client, (void*)"context test string");

    status = alljoyn_busattachment_enablepeersecurity(clientbus, mechanism, clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    SetUpAuthClientAuthFail();

    /* Wait up to 2 seconds for the signal to complete. */
    for (int i = 0; i < 200; ++i) {

        if (securityviolation_client_flag) {
            break;
        }
        qcc::Sleep(10);
    }

    EXPECT_TRUE(requestcredentials_service_flag);

    EXPECT_TRUE(authenticationcomplete_client_flag);
    EXPECT_TRUE(securityviolation_client_flag);

    alljoyn_authlistenerasync_destroy(serviceauthlistener);
    alljoyn_authlistenerasync_destroy(clientauthlistener);

}

TEST_F(AuthListenerTest, AsyncAuthFails_SRP_KEYX) {
    RunAsyncAuthFailsTest("ALLJOYN_SRP_KEYX");
}

TEST_F(AuthListenerTest, AsyncAuthFails_SPEKE) {
    RunAsyncAuthFailsTest("ALLJOYN_ECDHE_SPEKE");
}

/*
 * Tests that authentication is being done when alljoyn_proxybusobject_secureconnection is called.
 */

void AuthListenerTest::RunSecureConnectionTest(const char* mechanism, const char* context_string)
{
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistener_callbacks authlistener_cb_service = {
        authlistener_requestcredentials_service,
        NULL, //verifycredentials
        authlistener_securityviolation_service,
        authlistener_authenticationcomplete_service
    };
    alljoyn_authlistener serviceauthlistener = alljoyn_authlistener_create(&authlistener_cb_service, (void*)context_string);

    status = alljoyn_busattachment_enablepeersecurity(servicebus, mechanism, serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistener_callbacks authlistener_cb_client = {
        authlistener_requestcredentials_client,
        NULL, //verifycredentials
        authlistener_securityviolation_client,
        authlistener_authenticationcomplete_client
    };
    alljoyn_authlistener clientauthlistener = alljoyn_authlistener_create(&authlistener_cb_client, (void*)context_string);

    status = alljoyn_busattachment_enablepeersecurity(clientbus, mechanism, clientauthlistener, NULL, QCC_FALSE);
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
     * The peer-to-peer connection should have been authenticated with the last
     * call to alljoyn_proxybusobject_secureconnection. This call should return
     * ER_OK without calling any of the authlistener functions.
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

TEST_F(AuthListenerTest, secureconnection_SRP_KEYX) {
    RunSecureConnectionTest("ALLJOYN_SRP_KEYX", "context test string");
}

TEST_F(AuthListenerTest, secureconnection_SPEKE) {
    RunSecureConnectionTest("ALLJOYN_ECDHE_SPEKE", NULL);
}

/*
 * Tests that authentication is being done when alljoyn_proxybusobject_secureconnection is called,
 * and the asynchronous authentication callback is used.
 */

void AuthListenerTest::RunAsyncSecureConnectionTest(const char* mechanism, const char* context_string)
{
    ResetAuthFlags();

    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistenerasync_callbacks authlistenerasync_cb_service = {
        authlistener_requestcredentialsasync_service,
        NULL, //verifycredentials
        authlistener_securityviolation_service,
        authlistener_authenticationcomplete_service
    };
    alljoyn_authlistener serviceauthlistener = alljoyn_authlistenerasync_create(&authlistenerasync_cb_service, (void*)context_string);

    status = alljoyn_busattachment_enablepeersecurity(servicebus, mechanism, serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthService();

    /* set up the client */
    alljoyn_authlistenerasync_callbacks authlistenerasync_cb_client = {
        authlistener_requestcredentialsasync_client,
        NULL, //verifycredentials
        authlistener_securityviolation_client,
        authlistener_authenticationcomplete_client
    };
    alljoyn_authlistener clientauthlistener = alljoyn_authlistenerasync_create(&authlistenerasync_cb_client, (void*)context_string);

    status = alljoyn_busattachment_enablepeersecurity(clientbus, mechanism, clientauthlistener, NULL, QCC_FALSE);
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
     * call to alljoyn_proxybusobject_secureconnection. This call should return
     * ER_OK without calling any of the authlistener functions.
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

    alljoyn_authlistenerasync_destroy(serviceauthlistener);
    alljoyn_authlistenerasync_destroy(clientauthlistener);
}

TEST_F(AuthListenerTest, async_secureconnection_SRP_KEYX) {
    RunAsyncSecureConnectionTest("ALLJOYN_SRP_KEYX", "context test string");
}

TEST_F(AuthListenerTest, async_secureconnection_SPEKE) {
    RunAsyncSecureConnectionTest("ALLJOYN_ECDHE_SPEKE", NULL);
}
