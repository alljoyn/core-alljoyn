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
#include <qcc/Thread.h>
#include <alljoyn_c/BusAttachment.h>

/*constants*/
//static const char* INTERFACE_NAME = "org.alljoyn.test.BusListenerMainThreadTest";
static const char* OBJECT_NAME = "org.alljoyn.test.BusListenerMainThreadTest";
//static const char* OBJECT_PATH = "/org/alljoyn/test/BusListenerMainThreadTest";

/* flags */
static QCC_BOOL listener_registered_flag = QCC_FALSE;
static QCC_BOOL listener_unregistered_flag = QCC_FALSE;
static QCC_BOOL found_advertised_name_flag = QCC_FALSE;
static QCC_BOOL lost_advertised_name_flag = QCC_FALSE;
static QCC_BOOL name_owner_changed_flag = QCC_FALSE;
static QCC_BOOL bus_stopping_flag = QCC_FALSE;
static QCC_BOOL bus_disconnected_flag = QCC_FALSE;

/* bus listener functions */
static void AJ_CALL listener_registered(const void* context, alljoyn_busattachment bus) {
    listener_registered_flag = QCC_TRUE;
}
static void AJ_CALL listener_unregistered(const void* context) {
    listener_unregistered_flag = QCC_TRUE;
}
static void AJ_CALL found_advertised_name(const void* context, const char* name, alljoyn_transportmask transport, const char* namePrefix) {
    found_advertised_name_flag = QCC_TRUE;
}
static void AJ_CALL lost_advertised_name(const void* context, const char* name, alljoyn_transportmask transport, const char* namePrefix) {
    lost_advertised_name_flag = QCC_TRUE;
}
static void AJ_CALL name_owner_changed(const void* context, const char* busName, const char* previousOwner, const char* newOwner) {
    name_owner_changed_flag = QCC_TRUE;
}
static void AJ_CALL bus_stopping(const void* context) {
    bus_stopping_flag = QCC_TRUE;
}
static void AJ_CALL bus_disconnected(const void* context) {
    bus_disconnected_flag = QCC_TRUE;
}

class BusListenerMainThreadTest : public testing::Test {
  public:
    virtual void SetUp() {
        alljoyn_unity_set_deferred_callback_mainthread_only(QCC_TRUE);
        resetFlags();
        /* register bus listener */
        alljoyn_buslistener_callbacks buslistenerCbs = {
            &listener_registered,
            &listener_unregistered,
            &found_advertised_name,
            &lost_advertised_name,
            &name_owner_changed,
            &bus_stopping,
            &bus_disconnected,
            NULL
        };
        buslistener = alljoyn_buslistener_create(&buslistenerCbs, NULL);
        bus = alljoyn_busattachment_create("BusListenerMainThreadTest", QCC_FALSE);
    }

    virtual void TearDown() {
        EXPECT_NO_FATAL_FAILURE(alljoyn_buslistener_destroy(buslistener));
        EXPECT_NO_FATAL_FAILURE(alljoyn_busattachment_destroy(bus));
        alljoyn_unity_set_deferred_callback_mainthread_only(QCC_FALSE);
        //make sure the queued processes are cleared.
        alljoyn_unity_deferred_callbacks_process();

    }

    void resetFlags() {
        listener_registered_flag = QCC_FALSE;
        listener_unregistered_flag = QCC_FALSE;
        found_advertised_name_flag = QCC_FALSE;
        lost_advertised_name_flag = QCC_FALSE;
        name_owner_changed_flag = QCC_FALSE;
        bus_stopping_flag = QCC_FALSE;
        bus_disconnected_flag = QCC_FALSE;
    }
    QStatus status;
    alljoyn_busattachment bus;
    alljoyn_buslistener buslistener;
};

TEST_F(BusListenerMainThreadTest, listner_registered_unregistered) {
    alljoyn_busattachment_registerbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);
    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (listener_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_unregistered_flag);
}

//ALLJOYN-1738
TEST_F(BusListenerMainThreadTest, DISABLED_bus_stopping_disconnected) {

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());

    alljoyn_busattachment_registerbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);

    alljoyn_busattachment_disconnect(bus, ajn::getConnectArg().c_str());
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (bus_disconnected_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_disconnected_flag);

    alljoyn_busattachment_stop(bus);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (bus_stopping_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_stopping_flag);
    alljoyn_busattachment_join(bus);

    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (listener_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_unregistered_flag);
}

// ALLJOYN-1738
TEST_F(BusListenerMainThreadTest, DISABLED_found_lost_advertised_name) {
    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());

    alljoyn_busattachment_registerbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);

    alljoyn_sessionopts opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);

    status = alljoyn_busattachment_findadvertisedname(bus, OBJECT_NAME);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_advertisename(bus, OBJECT_NAME, alljoyn_sessionopts_get_transports(opts));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (found_advertised_name_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(found_advertised_name_flag);

    status = alljoyn_busattachment_canceladvertisename(bus, OBJECT_NAME, alljoyn_sessionopts_get_transports(opts));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (lost_advertised_name_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(lost_advertised_name_flag);

    alljoyn_busattachment_stop(bus);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (bus_stopping_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_stopping_flag);
    alljoyn_busattachment_join(bus);
    /* the bus will automatically disconnect when it is stopped */
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (bus_disconnected_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_disconnected_flag);

    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (listener_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_unregistered_flag);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_sessionopts_destroy(opts);
}

// ALLJOYN-1738
TEST_F(BusListenerMainThreadTest, DISABLED_name_owner_changed) {
    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());

    alljoyn_busattachment_registerbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);

    alljoyn_busattachment_requestname(bus, OBJECT_NAME, 0);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (name_owner_changed_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(name_owner_changed_flag);

    alljoyn_busattachment_stop(bus);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (bus_stopping_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_stopping_flag);
    alljoyn_busattachment_join(bus);
    /* the bus will automatically disconnect when it is stopped */
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (bus_disconnected_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_disconnected_flag);

    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        alljoyn_unity_deferred_callbacks_process();
        if (listener_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_unregistered_flag);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
