/******************************************************************************
 * Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/BusAttachment.h>
#include <qcc/Thread.h>

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.test.SessionTest";
static qcc::String objectName;
static const char* OBJECT_PATH = "/org/alljoyn/test/SessionTest";
static const alljoyn_sessionport SESSION_PORT = 42;

static QCC_BOOL joinsessionhandler_flag;
static QCC_BOOL foundadvertisedname_flag;
static QCC_BOOL sessionjoined_flag;
static alljoyn_sessionid joinsessionid;
static alljoyn_sessionid joinsessionid_alt;

/* AcceptSessionJoiner callback */
static QCC_BOOL AJ_CALL accept_session_joiner(const void* context, alljoyn_sessionport sessionPort,
                                              const char* joiner,  const alljoyn_sessionopts opts)
{
    QCC_BOOL ret = QCC_FALSE;
    if (sessionPort == SESSION_PORT) {
        ret = QCC_TRUE;
    }
    return ret;
}

static void AJ_CALL session_joined(const void* context, alljoyn_sessionport sessionPort, alljoyn_sessionid id, const char* joiner)
{
    //printf("session_joined\n");
    EXPECT_EQ(SESSION_PORT, sessionPort);
    joinsessionid = id;
    sessionjoined_flag = true;
}

static void AJ_CALL joinsessionhandler(QStatus status, alljoyn_sessionid sessionId, const alljoyn_sessionopts opts, void* context)
{
    EXPECT_STREQ("A test string to send as the context void*", (char*)context);
    joinsessionid_alt = sessionId;
    joinsessionhandler_flag = QCC_TRUE;
}


/* FoundAdvertisedName callback */
void AJ_CALL found_advertised_name(const void* context, const char* name, alljoyn_transportmask transport, const char* namePrefix)
{
    //printf("FoundAdvertisedName(name=%s, prefix=%s)\n", name, namePrefix);
    EXPECT_STREQ(objectName.c_str(), name);
    if (0 == strcmp(name, objectName.c_str())) {
        foundadvertisedname_flag = QCC_TRUE;
    }
}
/* Exposed methods */
static void AJ_CALL ping_method(alljoyn_busobject bus, const alljoyn_interfacedescription_member* member, alljoyn_message msg)
{
    alljoyn_msgarg outArg = alljoyn_msgarg_create();
    outArg = alljoyn_message_getarg(msg, 0);
    const char* str;
    alljoyn_msgarg_get(outArg, "s", &str);
    QStatus status = alljoyn_busobject_methodreply_args(bus, msg, outArg, 1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

class SessionTest : public testing::Test {
  public:
    virtual void SetUp() {
        bus = alljoyn_busattachment_create("SessionTest", false);
        status = alljoyn_busattachment_start(bus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        alljoyn_busattachment_stop(bus);
        alljoyn_busattachment_join(bus);
        EXPECT_NO_FATAL_FAILURE(alljoyn_busattachment_destroy(bus));
    }

    void SetUpSessionTestService()
    {
        /* create/start/connect alljoyn_busattachment */
        servicebus = alljoyn_busattachment_create("SessionTestservice", false);
        objectName = ajn::genUniqueName(servicebus);
        status = alljoyn_busattachment_start(servicebus);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = alljoyn_busattachment_connect(servicebus, ajn::getConnectArg().c_str());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        alljoyn_interfacedescription testIntf = NULL;
        status = alljoyn_busattachment_createinterface(servicebus, INTERFACE_NAME, &testIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(testIntf != NULL);
        if (testIntf != NULL) {
            status = alljoyn_interfacedescription_addmethod(testIntf, "ping", "s", "s", "in,out", 0, 0);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            alljoyn_interfacedescription_activate(testIntf);

            //    /* register bus listener */
            //    alljoyn_buslistener_callbacks buslistenerCbs = {
            //        NULL, /* bus listener registered CB */
            //        NULL, /* bus listener unregistered CB */
            //        NULL, /* found advertised name CB */
            //        NULL, /* lost advertised name CB */
            //        NULL, /* name owner changed CB */
            //        NULL, /* Bus Stopping CB */
            //        NULL, /* Bus Disconnected CB */
            //        NULL  /* Property Changed CB */
            //    };
            //    buslistener = alljoyn_buslistener_create(&buslistenerCbs, NULL);
            //    alljoyn_busattachment_registerbuslistener(servicebus, buslistener);

            /* Set up bus object */
            alljoyn_busobject_callbacks busObjCbs = {
                NULL,     /* Get alljoyn property CB */
                NULL,     /* Set alljoyn property CB */
                NULL,     /* BusObject Registered CB */
                NULL      /* BusObject Unregistered CB */
            };
            testObj = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);

            status = alljoyn_busobject_addinterface(testObj, testIntf);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

            /* register method handlers */
            alljoyn_interfacedescription_member ping_member;
            EXPECT_TRUE(alljoyn_interfacedescription_getmember(testIntf, "ping", &ping_member));

            status = alljoyn_busobject_addmethodhandler(testObj, ping_member, &ping_method, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

            status = alljoyn_busattachment_registerbusobject(servicebus, testObj);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

            /* request name */
            uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
            status = alljoyn_busattachment_requestname(servicebus, objectName.c_str(), flags);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

            /* Create session port listener */
            alljoyn_sessionportlistener_callbacks spl_cbs = {
                &accept_session_joiner,     /* accept session joiner CB */
                &session_joined     /* session joined CB */
            };
            sessionPortListener = alljoyn_sessionportlistener_create(&spl_cbs, NULL);

            /* Bind SessionPort */
            alljoyn_sessionopts opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
            alljoyn_sessionport sp = SESSION_PORT;
            status = alljoyn_busattachment_bindsessionport(servicebus, &sp, opts, sessionPortListener);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            /* Advertise Name */
            status = alljoyn_busattachment_advertisename(servicebus, objectName.c_str(), alljoyn_sessionopts_get_transports(opts));
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            alljoyn_sessionopts_destroy(opts);
        }
    }

    void TearDownSessionTestService()
    {
        alljoyn_busattachment_stop(servicebus);
        alljoyn_busattachment_join(servicebus);
        alljoyn_busattachment_destroy(servicebus);
        alljoyn_sessionportlistener_destroy(sessionPortListener);
        alljoyn_busobject_destroy(testObj);
    }

    QStatus status;
    alljoyn_busattachment bus;
    alljoyn_busobject testObj;

    alljoyn_busattachment servicebus;
    alljoyn_buslistener buslistener;
    alljoyn_sessionportlistener sessionPortListener;
};


TEST_F(SessionTest, joinsession) {
    SetUpSessionTestService();

    /* Create a bus listener */
    alljoyn_buslistener_callbacks callbacks = {
        NULL,     /* listener registered CB */
        NULL,     /* listener unregistered CB */
        &found_advertised_name,     /* found advertised name CB */
        NULL,     /* lost advertised name CB */
        NULL,     /* name owner changed CB */
        NULL,     /* bus stopping CB*/
        NULL     /* bus disconnected CB */
    };

    buslistener = alljoyn_buslistener_create(&callbacks, NULL);
    alljoyn_busattachment_registerbuslistener(bus, buslistener);

    foundadvertisedname_flag = QCC_FALSE;
    sessionjoined_flag = QCC_FALSE;
    /* Begin discover of the well-known name */
    status = alljoyn_busattachment_findadvertisedname(bus, objectName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 1000; ++i) {
        if (foundadvertisedname_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(foundadvertisedname_flag);

    /* We found a remote bus that is advertising basic service's  well-known name so connect to it */
    alljoyn_sessionopts opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
    alljoyn_sessionid sid;
    status = alljoyn_busattachment_joinsession(bus, objectName.c_str(), SESSION_PORT, NULL, &sid, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 1000; ++i) {
        if (sessionjoined_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(sessionjoined_flag);
    EXPECT_EQ(sid, joinsessionid);

    alljoyn_sessionopts_destroy(opts);
    //joincompleted_flag = QCC_TRUE;

    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    alljoyn_buslistener_destroy(buslistener);
    TearDownSessionTestService();
}

TEST_F(SessionTest, joinsessionasync) {
    SetUpSessionTestService();

    /* Create a bus listener */
    alljoyn_buslistener_callbacks callbacks = {
        NULL,     /* listener registered CB */
        NULL,     /* listener unregistered CB */
        &found_advertised_name,     /* found advertised name CB */
        NULL,     /* lost advertised name CB */
        NULL,     /* name owner changed CB */
        NULL,     /* bus stopping CB*/
        NULL     /* bus disconnected CB */
    };

    buslistener = alljoyn_buslistener_create(&callbacks, NULL);
    alljoyn_busattachment_registerbuslistener(bus, buslistener);

    foundadvertisedname_flag = QCC_FALSE;
    sessionjoined_flag = QCC_FALSE;
    joinsessionhandler_flag = QCC_FALSE;

    joinsessionid = 0;
    joinsessionid_alt = 0;
    /* Begin discover of the well-known name */
    status = alljoyn_busattachment_findadvertisedname(bus, objectName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 1000; ++i) {
        if (foundadvertisedname_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(foundadvertisedname_flag);

    /* We found a remote bus that is advertising basic service's  well-known name so connect to it */
    alljoyn_sessionopts opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);

    char dave[64] = "A test string to send as the context void*";
    status = alljoyn_busattachment_joinsessionasync(bus, objectName.c_str(), SESSION_PORT, NULL, opts, &joinsessionhandler, (void*)dave);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 1000; ++i) {
        // we can not break till both session flags have been set or we have a
        // race condition that one of the two values have not been set.
        if (sessionjoined_flag && joinsessionhandler_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(sessionjoined_flag);
    EXPECT_TRUE(joinsessionhandler_flag);
    EXPECT_EQ(joinsessionid_alt, joinsessionid);

    alljoyn_sessionopts_destroy(opts);

    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    alljoyn_buslistener_destroy(buslistener);
    TearDownSessionTestService();
}

TEST_F(SessionTest, setLinkTimeout)
{
    SetUpSessionTestService();

    /* Create a bus listener */
    alljoyn_buslistener_callbacks callbacks = {
        NULL,         /* listener registered CB */
        NULL,         /* listener unregistered CB */
        &found_advertised_name,         /* found advertised name CB */
        NULL,         /* lost advertised name CB */
        NULL,         /* name owner changed CB */
        NULL,         /* bus stopping CB*/
        NULL         /* bus disconnected CB */
    };

    buslistener = alljoyn_buslistener_create(&callbacks, NULL);
    alljoyn_busattachment_registerbuslistener(bus, buslistener);

    foundadvertisedname_flag = QCC_FALSE;
    sessionjoined_flag = QCC_FALSE;
    /* Begin discover of the well-known name */
    status = alljoyn_busattachment_findadvertisedname(bus, objectName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        if (foundadvertisedname_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(foundadvertisedname_flag);

    /* We found a remote bus that is advertising basic service's  well-known name so connect to it */
    alljoyn_sessionopts opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES,
                                                          QCC_FALSE, ALLJOYN_PROXIMITY_ANY,
                                                          ALLJOYN_TRANSPORT_ANY);
    alljoyn_sessionid sid;
    status = alljoyn_busattachment_joinsession(bus, objectName.c_str(), SESSION_PORT, NULL, &sid, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        if (sessionjoined_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_EQ(sid, joinsessionid);
    EXPECT_TRUE(sessionjoined_flag);
    // setting the linkTimeout to 2 min.  This value is high enough that the
    // it should not be changed by the underlying transport.
    uint32_t linkTimeout = 120;
    status = alljoyn_busattachment_setlinktimeout(bus, sid, &linkTimeout);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(120u, linkTimeout);

    alljoyn_sessionopts_destroy(opts);

    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    alljoyn_buslistener_destroy(buslistener);
    TearDownSessionTestService();
}

QCC_BOOL setlinktimeout_flag = QCC_FALSE;
void AJ_CALL alljoyn_busattachment_setlinktimeoutcb(QStatus status, uint32_t timeout, void* context)
{
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(120u, timeout);
    EXPECT_STREQ("String passed as context.", (const char*)context);
    setlinktimeout_flag = QCC_TRUE;
}

TEST_F(SessionTest, setLinkTimeoutasync)
{
    SetUpSessionTestService();

    /* Create a bus listener */
    alljoyn_buslistener_callbacks callbacks = {
        NULL,         /* listener registered CB */
        NULL,         /* listener unregistered CB */
        &found_advertised_name,         /* found advertised name CB */
        NULL,         /* lost advertised name CB */
        NULL,         /* name owner changed CB */
        NULL,         /* bus stopping CB*/
        NULL         /* bus disconnected CB */
    };

    buslistener = alljoyn_buslistener_create(&callbacks, NULL);
    alljoyn_busattachment_registerbuslistener(bus, buslistener);

    foundadvertisedname_flag = QCC_FALSE;
    sessionjoined_flag = QCC_FALSE;
    /* Begin discover of the well-known name */
    status = alljoyn_busattachment_findadvertisedname(bus, objectName.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        if (foundadvertisedname_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(foundadvertisedname_flag);

    /* We found a remote bus that is advertising basic service's  well-known name so connect to it */
    alljoyn_sessionopts opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES,
                                                          QCC_FALSE, ALLJOYN_PROXIMITY_ANY,
                                                          ALLJOYN_TRANSPORT_ANY);
    alljoyn_sessionid sid;
    status = alljoyn_busattachment_joinsession(bus, objectName.c_str(), SESSION_PORT, NULL, &sid, opts);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        if (sessionjoined_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_EQ(sid, joinsessionid);
    EXPECT_TRUE(sessionjoined_flag);
    // setting the linkTimeout to 2 min.  This value is high enough that the
    // it should not be changed by the underlying transport.
    setlinktimeout_flag = QCC_FALSE;
    uint32_t linkTimeout = 120;
    status = alljoyn_busattachment_setlinktimeoutasync(bus, sid, linkTimeout,
                                                       alljoyn_busattachment_setlinktimeoutcb,
                                                       (void*)"String passed as context.");
    for (size_t i = 0; i < 200; ++i) {
        if (setlinktimeout_flag) {
            break;
        }
        qcc::Sleep(5);
    }

    EXPECT_TRUE(setlinktimeout_flag);
    alljoyn_sessionopts_destroy(opts);

    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    alljoyn_buslistener_destroy(buslistener);
    TearDownSessionTestService();
}
