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

static QCC_BOOL requestcredentials_service_flag = QCC_FALSE;
static QCC_BOOL authenticationcomplete_service_flag = QCC_FALSE;

static QCC_BOOL requestcredentials_client_flag = QCC_FALSE;
static QCC_BOOL authenticationcomplete_client_flag = QCC_FALSE;

class ObjectSecurityTest : public testing::Test {
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

    void SetUpAuthServiceWithSecureBusObject() {
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

        // the bus object has been registered as a secure busobject
        status = alljoyn_busattachment_registerbusobject_secure(servicebus, testObj);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        EXPECT_TRUE(alljoyn_busobject_issecure(testObj));
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

    void SetupAuthClientWitSecureProxyBusObject() {
        // make a secure proxybusobject
        alljoyn_proxybusobject proxyObj = alljoyn_proxybusobject_create_secure(clientbus, OBJECT_NAME, OBJECT_PATH, 0);
        EXPECT_TRUE(proxyObj);
        EXPECT_TRUE(alljoyn_proxybusobject_issecure(proxyObj));
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

    void ResetAuthFlags() {
        requestcredentials_service_flag = QCC_FALSE;
        authenticationcomplete_service_flag = QCC_FALSE;

        requestcredentials_client_flag = QCC_FALSE;
        authenticationcomplete_client_flag = QCC_FALSE;
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
    EXPECT_STREQ("ALLJOYN_SRP_KEYX", authMechanism);
    if (credMask & ALLJOYN_CRED_PASSWORD) {
        alljoyn_credentials_setpassword(credentials, "ABCDEFGH");
    }
    requestcredentials_service_flag = QCC_TRUE;
    return QCC_TRUE;
}

static void AJ_CALL alljoyn_authlistener_authenticationcomplete_service_srp_keyx(const void* context, const char* authMechanism,
                                                                                 const char* peerName, QCC_BOOL success) {
    EXPECT_TRUE(success);
    authenticationcomplete_service_flag = QCC_TRUE;
}


static QCC_BOOL AJ_CALL authlistener_requestcredentials_client_srp_keyx(const void* context, const char* authMechanism,
                                                                        const char* peerName, uint16_t authCount,
                                                                        const char* userName, uint16_t credMask,
                                                                        alljoyn_credentials credentials) {
    EXPECT_STREQ("ALLJOYN_SRP_KEYX", authMechanism);
    if (credMask & ALLJOYN_CRED_PASSWORD) {
        alljoyn_credentials_setpassword(credentials, "ABCDEFGH");
    }
    requestcredentials_client_flag = QCC_TRUE;
    return QCC_TRUE;
}

static void AJ_CALL alljoyn_authlistener_authenticationcomplete_client_srp_keyx(const void* context, const char* authMechanism,
                                                                                const char* peerName, QCC_BOOL success) {
    EXPECT_TRUE(success);
    authenticationcomplete_client_flag = QCC_TRUE;
}


TEST_F(ObjectSecurityTest, insecure_interface_sercure_object) {
    //create an interface that is not marked with any security
    alljoyn_interfacedescription service_intf;
    status = alljoyn_busattachment_createinterface(servicebus, INTERFACE_NAME, &service_intf);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(service_intf != NULL);
    status = alljoyn_interfacedescription_addmember(service_intf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(service_intf);

    //make sure all the auth flags are in a known state
    ResetAuthFlags();

    /*
     * if an interface is created without security is should default to an
     * inherit security policy.
     */
    EXPECT_EQ(AJ_IFC_SECURITY_INHERIT, alljoyn_interfacedescription_getsecuritypolicy(service_intf));

    /*
     * clear the busattachment keystore to make sure authentication is performed
     * if needed.
     */
    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistener_callbacks authlistener_cb_service = {
        authlistener_requestcredentials_service_srp_keyx,     //requestcredentials
        NULL,     //verifycredentials
        NULL,     //securityviolation
        alljoyn_authlistener_authenticationcomplete_service_srp_keyx     //authenticationcomplete
    };

    alljoyn_authlistener serviceauthlistener = alljoyn_authlistener_create(&authlistener_cb_service, NULL);

    status = alljoyn_busattachment_enablepeersecurity(servicebus, "ALLJOYN_SRP_KEYX", serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthServiceWithSecureBusObject();

    /* set up the client */
    alljoyn_authlistener_callbacks authlistener_cb_client = {
        authlistener_requestcredentials_client_srp_keyx,     //requestcredentials
        NULL,     //verifycredentials
        NULL,     //securityviolation
        alljoyn_authlistener_authenticationcomplete_client_srp_keyx     //authenticationcomplete
    };

    alljoyn_authlistener clientauthlistener = alljoyn_authlistener_create(&authlistener_cb_client, NULL);

    status = alljoyn_busattachment_enablepeersecurity(clientbus, "ALLJOYN_SRP_KEYX", clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    SetupAuthClientWitSecureProxyBusObject();

    EXPECT_TRUE(requestcredentials_service_flag);
    EXPECT_TRUE(authenticationcomplete_service_flag);

    EXPECT_TRUE(requestcredentials_client_flag);
    EXPECT_TRUE(authenticationcomplete_client_flag);

    alljoyn_authlistener_destroy(serviceauthlistener);
    alljoyn_authlistener_destroy(clientauthlistener);
}

TEST_F(ObjectSecurityTest, interface_security_off_sercure_object) {
    //create an interface that is not marked with any security
    alljoyn_interfacedescription service_intf;
    status = alljoyn_busattachment_createinterface_secure(servicebus, INTERFACE_NAME, &service_intf, AJ_IFC_SECURITY_OFF);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(service_intf != NULL);
    status = alljoyn_interfacedescription_addmember(service_intf, ALLJOYN_MESSAGE_METHOD_CALL, "ping", "s", "s", "in,out", 0);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_interfacedescription_activate(service_intf);

    //make sure all the auth flags are in a known state
    ResetAuthFlags();

    /*
     * if an interface is created with off security policy.
     */
    EXPECT_EQ(AJ_IFC_SECURITY_OFF, alljoyn_interfacedescription_getsecuritypolicy(service_intf));

    /*
     * clear the busattachment keystore to make sure authentication is performed
     * if needed.
     */
    alljoyn_busattachment_clearkeystore(clientbus);

    /* set up the service */
    alljoyn_authlistener_callbacks authlistener_cb_service = {
        authlistener_requestcredentials_service_srp_keyx,     //requestcredentials
        NULL,     //verifycredentials
        NULL,     //securityviolation
        alljoyn_authlistener_authenticationcomplete_service_srp_keyx     //authenticationcomplete
    };

    alljoyn_authlistener serviceauthlistener = alljoyn_authlistener_create(&authlistener_cb_service, NULL);

    status = alljoyn_busattachment_enablepeersecurity(servicebus, "ALLJOYN_SRP_KEYX", serviceauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(servicebus);

    SetUpAuthServiceWithSecureBusObject();

    /* set up the client */
    alljoyn_authlistener_callbacks authlistener_cb_client = {
        authlistener_requestcredentials_client_srp_keyx,     //requestcredentials
        NULL,     //verifycredentials
        NULL,     //securityviolation
        alljoyn_authlistener_authenticationcomplete_client_srp_keyx     //authenticationcomplete
    };

    alljoyn_authlistener clientauthlistener = alljoyn_authlistener_create(&authlistener_cb_client, NULL);

    status = alljoyn_busattachment_enablepeersecurity(clientbus, "ALLJOYN_SRP_KEYX", clientauthlistener, NULL, QCC_FALSE);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    /* Clear the Keystore between runs */
    alljoyn_busattachment_clearkeystore(clientbus);

    SetupAuthClientWitSecureProxyBusObject();

    EXPECT_FALSE(requestcredentials_service_flag);
    EXPECT_FALSE(authenticationcomplete_service_flag);

    EXPECT_FALSE(requestcredentials_client_flag);
    EXPECT_FALSE(authenticationcomplete_client_flag);

    alljoyn_authlistener_destroy(serviceauthlistener);
    alljoyn_authlistener_destroy(clientauthlistener);
}
