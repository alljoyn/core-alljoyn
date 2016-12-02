/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#include <gtest/gtest.h>

#include <alljoyn_c/Session.h>
#include <alljoyn_c/TransportMask.h>

TEST(SessionOptsTest, accessor_functions) {
    alljoyn_sessionopts opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES,
                                                          QCC_FALSE,
                                                          ALLJOYN_PROXIMITY_ANY,
                                                          ALLJOYN_TRANSPORT_ANY);
    EXPECT_EQ(ALLJOYN_TRAFFIC_TYPE_MESSAGES, alljoyn_sessionopts_get_traffic(opts));
    EXPECT_EQ(QCC_FALSE, alljoyn_sessionopts_get_multipoint(opts));
    EXPECT_EQ(ALLJOYN_PROXIMITY_ANY, alljoyn_sessionopts_get_proximity(opts));
    EXPECT_EQ(ALLJOYN_TRANSPORT_ANY, alljoyn_sessionopts_get_transports(opts));

    alljoyn_sessionopts_set_traffic(opts, ALLJOYN_TRAFFIC_TYPE_RAW_RELIABLE);
    EXPECT_EQ(ALLJOYN_TRAFFIC_TYPE_RAW_RELIABLE, alljoyn_sessionopts_get_traffic(opts));

    alljoyn_sessionopts_set_multipoint(opts, QCC_TRUE);
    EXPECT_EQ(QCC_TRUE, alljoyn_sessionopts_get_multipoint(opts));

    alljoyn_sessionopts_set_proximity(opts, ALLJOYN_PROXIMITY_PHYSICAL);
    EXPECT_EQ(ALLJOYN_PROXIMITY_PHYSICAL, alljoyn_sessionopts_get_proximity(opts));

    alljoyn_sessionopts_set_transports(opts, ALLJOYN_TRANSPORT_TCP);
    EXPECT_EQ(ALLJOYN_TRANSPORT_TCP, alljoyn_sessionopts_get_transports(opts));

    alljoyn_sessionopts_destroy(opts);
}