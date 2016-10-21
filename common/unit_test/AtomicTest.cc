/**
 * @file
 *
 * This file tests atomic memory changes.
 */

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
#include <qcc/atomic.h>
#include <qcc/Thread.h>

#include <gtest/gtest.h>

using namespace qcc;

TEST(AtomicTest, CompareAndExchange)
{
    /* Test the case where the two values being compared are equal */
    volatile int32_t destination = 0xABCD1234;
    int32_t expectedValue = destination;
    int32_t newValue = 7;
    ASSERT_TRUE(CompareAndExchange(&destination, expectedValue, newValue));
    ASSERT_EQ(destination, newValue);

    /* Test the case where the two values being compared are not equal */
    destination = 14;
    expectedValue = destination + 1;
    newValue = 0;
    ASSERT_FALSE(CompareAndExchange(&destination, expectedValue, newValue));
    ASSERT_EQ(destination, 14);
}

ThreadReturn my_thread_func_int32(void*pArg)
{
    atomic_int32*pVal = static_cast<atomic_int32*>(pArg);
    (*pVal)++;
    return nullptr;
}

TEST(AtomicTest, Atomic_int32)
{
    atomic_int32 a(10);
    ASSERT_EQ(int32_t(a), 10);

    qcc::Thread t("dummy", my_thread_func_int32);
    t.Start(&a);
    t.Join();
    ASSERT_EQ(int32_t(a), 11);

    int32_t oldA = int32_t(a);
    ASSERT_EQ(a++, oldA);
    ASSERT_EQ(a--, oldA + 1);

    ASSERT_EQ(int32_t(++a), oldA + 1);
    ASSERT_EQ(int32_t(--a), oldA);
}

ThreadReturn my_thread_func_uint32(void*pArg)
{
    atomic_uint32*pVal = static_cast<atomic_uint32*>(pArg);
    (*pVal)++;
    return nullptr;
}

TEST(AtomicTest, Atomic_uint32)
{
    atomic_uint32 a(10);
    ASSERT_EQ(uint32_t(a), 10U);

    qcc::Thread t("dummy", my_thread_func_uint32);
    t.Start(&a);
    t.Join();
    ASSERT_EQ(uint32_t(a), 11U);

    uint32_t oldA = uint32_t(a);
    ASSERT_EQ(a++, oldA);
    ASSERT_EQ(a--, oldA + 1);

    ASSERT_EQ(uint32_t(++a), oldA + 1);
    ASSERT_EQ(uint32_t(--a), oldA);
}

ThreadReturn my_thread_func_bool(void*pArg)
{
    atomic_bool*pVal = static_cast<atomic_bool*>(pArg);
    *pVal = true;
    return nullptr;
}

TEST(AtomicTest, atomic_bool)
{
    atomic_bool a(false);
    ASSERT_EQ(bool(a), false);

    qcc::Thread t("dummy", my_thread_func_bool);
    t.Start(&a);
    t.Join();
    ASSERT_EQ(bool(a), true);

    const atomic_bool b(true);
    ASSERT_EQ(bool(b), true);
}
