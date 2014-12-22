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
#include <qcc/platform.h>

#include <alljoyn/BusAttachment.h>

/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "../ajTestCommon.h"

using namespace std;
using namespace qcc;
using namespace ajn;

/* ASACORE-880 */
TEST(BusAttachmentTest, RegisterTwoBusObjectsWithSamePathFails)
{
    BusAttachment bus(NULL);
    BusObject busObj0("/parent/child");
    BusObject busObj1("/parent/child");
    EXPECT_EQ(ER_OK, bus.RegisterBusObject(busObj0));
    EXPECT_NE(ER_OK, bus.RegisterBusObject(busObj1));
}

/* ASACORE-880 */
TEST(BusAttachmentTest, RegisterChildThenParentBusObjectSucceeds)
{
    BusAttachment bus(NULL);
    BusObject child("/parent/child");
    BusObject parent("/parent");
    EXPECT_EQ(ER_OK, bus.RegisterBusObject(child));
    EXPECT_EQ(ER_OK, bus.RegisterBusObject(parent));
}
