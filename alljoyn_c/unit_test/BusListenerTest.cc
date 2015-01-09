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
#include <qcc/Thread.h>
#include <alljoyn_c/BusAttachment.h>

/* flags */
static QCC_BOOL listener_registered_flag = QCC_FALSE;
static QCC_BOOL listener_unregistered_flag = QCC_FALSE;
static QCC_BOOL found_advertised_name_flag = QCC_FALSE;
static QCC_BOOL lost_advertised_name_flag = QCC_FALSE;
static QCC_BOOL name_owner_changed_flag = QCC_FALSE;
static QCC_BOOL bus_stopping_flag = QCC_FALSE;
static QCC_BOOL bus_disconnected_flag = QCC_FALSE;
static QCC_BOOL prop_changed_flag = QCC_FALSE;
static alljoyn_transportmask transport_found = 0;

/* bus listener functions */
static void AJ_CALL listener_registered(const void* context, alljoyn_busattachment bus) {
    listener_registered_flag = QCC_TRUE;
}
static void AJ_CALL listener_unregistered(const void* context) {
    listener_unregistered_flag = QCC_TRUE;
}
static void AJ_CALL found_advertised_name(const void* context, const char* name, alljoyn_transportmask transport, const char* namePrefix) {
    transport_found |= transport;
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

static void AJ_CALL bus_prop_changed(const void* context, const char* prop_name, alljoyn_msgarg prop_value) {
    prop_changed_flag = QCC_TRUE;
}

class BusListenerTest : public testing::Test {
  public:
    virtual void SetUp() {
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
            &bus_prop_changed
        };
        buslistener = alljoyn_buslistener_create(&buslistenerCbs, NULL);
        bus = alljoyn_busattachment_create("BusListenerTest", QCC_FALSE);
        object_name = ajn::genUniqueName(bus);
    }

    virtual void TearDown() {
        EXPECT_NO_FATAL_FAILURE(alljoyn_buslistener_destroy(buslistener));
        EXPECT_NO_FATAL_FAILURE(alljoyn_busattachment_destroy(bus));
    }

    void resetFlags() {
        listener_registered_flag = QCC_FALSE;
        listener_unregistered_flag = QCC_FALSE;
        found_advertised_name_flag = QCC_FALSE;
        lost_advertised_name_flag = QCC_FALSE;
        name_owner_changed_flag = QCC_FALSE;
        bus_stopping_flag = QCC_FALSE;
        bus_disconnected_flag = QCC_FALSE;
        prop_changed_flag = QCC_FALSE;
        transport_found = 0;
    }
    QStatus status;
    alljoyn_busattachment bus;
    alljoyn_buslistener buslistener;
    qcc::String object_name;
};

TEST_F(BusListenerTest, listner_registered_unregistered) {
    alljoyn_busattachment_registerbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);
    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_unregistered_flag);
}

TEST_F(BusListenerTest, bus_stopping_disconnected) {

    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());

    alljoyn_busattachment_registerbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);

    alljoyn_busattachment_disconnect(bus, ajn::getConnectArg().c_str());
    for (size_t i = 0; i < 200; ++i) {
        if (bus_disconnected_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_disconnected_flag);

    alljoyn_busattachment_stop(bus);
    for (size_t i = 0; i < 200; ++i) {
        if (bus_stopping_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_stopping_flag);
    alljoyn_busattachment_join(bus);

    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_unregistered_flag);
}

TEST_F(BusListenerTest, found_lost_advertised_name) {
    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());

    alljoyn_busattachment_registerbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);

    alljoyn_sessionopts opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);

    status = alljoyn_busattachment_findadvertisedname(bus, object_name.c_str());
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_advertisename(bus, object_name.c_str(), alljoyn_sessionopts_get_transports(opts));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (size_t i = 0; i < 200; ++i) {
        if (found_advertised_name_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(found_advertised_name_flag);

    status = alljoyn_busattachment_canceladvertisename(bus, object_name.c_str(), alljoyn_sessionopts_get_transports(opts));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < 200; ++i) {
        if (lost_advertised_name_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(lost_advertised_name_flag);

    alljoyn_busattachment_stop(bus);
    for (size_t i = 0; i < 200; ++i) {
        if (bus_stopping_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_stopping_flag);
    alljoyn_busattachment_join(bus);
    /* the bus will automatically disconnect when it is stopped */
    for (size_t i = 0; i < 200; ++i) {
        if (bus_disconnected_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_disconnected_flag);

    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
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

TEST_F(BusListenerTest, found_name_by_transport) {
    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());

    alljoyn_busattachment_registerbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);

    alljoyn_sessionopts opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);

    status = alljoyn_busattachment_findadvertisednamebytransport(bus, object_name.c_str(), ALLJOYN_TRANSPORT_LOCAL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_advertisename(bus, object_name.c_str(), alljoyn_sessionopts_get_transports(opts));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (size_t i = 0; i < 200; ++i) {
        if (found_advertised_name_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(found_advertised_name_flag);
    EXPECT_EQ(ALLJOYN_TRANSPORT_LOCAL, transport_found);

    status = alljoyn_busattachment_canceladvertisename(bus, object_name.c_str(), alljoyn_sessionopts_get_transports(opts));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_busattachment_cancelfindadvertisednamebytransport(bus, object_name.c_str(), ALLJOYN_TRANSPORT_LOCAL);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    found_advertised_name_flag = QCC_FALSE;
    status = alljoyn_busattachment_advertisename(bus, object_name.c_str(), alljoyn_sessionopts_get_transports(opts));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    for (size_t i = 0; i < 50; ++i) {
        if (found_advertised_name_flag) {
            break;
        }
        qcc::Sleep(5);
    }

    EXPECT_FALSE(found_advertised_name_flag);
    alljoyn_busattachment_stop(bus);
    for (size_t i = 0; i < 200; ++i) {
        if (bus_stopping_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_stopping_flag);
    alljoyn_busattachment_join(bus);
    /* the bus will automatically disconnect when it is stopped */
    for (size_t i = 0; i < 200; ++i) {
        if (bus_disconnected_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_disconnected_flag);

    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
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

TEST_F(BusListenerTest, name_owner_changed) {
    status = alljoyn_busattachment_start(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_busattachment_connect(bus, ajn::getConnectArg().c_str());

    alljoyn_busattachment_registerbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_registered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_registered_flag);

    alljoyn_busattachment_requestname(bus, object_name.c_str(), 0);
    for (size_t i = 0; i < 200; ++i) {
        if (name_owner_changed_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(name_owner_changed_flag);

    alljoyn_busattachment_stop(bus);
    for (size_t i = 0; i < 200; ++i) {
        if (bus_stopping_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_stopping_flag);
    alljoyn_busattachment_join(bus);
    /* the bus will automatically disconnect when it is stopped */
    for (size_t i = 0; i < 200; ++i) {
        if (bus_disconnected_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(bus_disconnected_flag);

    alljoyn_busattachment_unregisterbuslistener(bus, buslistener);
    for (size_t i = 0; i < 200; ++i) {
        if (listener_unregistered_flag) {
            break;
        }
        qcc::Sleep(5);
    }
    EXPECT_TRUE(listener_unregistered_flag);

    status = alljoyn_busattachment_stop(bus);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}
