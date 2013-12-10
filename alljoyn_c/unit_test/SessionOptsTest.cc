/******************************************************************************
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

    alljoyn_sessionopts_set_transports(opts, ALLJOYN_TRANSPORT_LAN);
    EXPECT_EQ(ALLJOYN_TRANSPORT_LAN, alljoyn_sessionopts_get_transports(opts));

    alljoyn_sessionopts_destroy(opts);
}
