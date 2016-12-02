/**
 * @file
 *
 * This file tests atomic memory changes.
 */

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

#include <qcc/platform.h>
#include <qcc/atomic.h>

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