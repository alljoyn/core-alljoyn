/******************************************************************************
 *
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
#include <iostream>

#include <qcc/Util.h>

TEST(CrtTest, snprintf1)
{
    char smallBuffer[35];
    const char fillCharacter = '*';
    memset(smallBuffer, fillCharacter, ArraySize(smallBuffer));

    int outputLength = snprintf(smallBuffer, ArraySize(smallBuffer), "large %p output %#x doesn't fit", nullptr, 0xABCD1234);

#if defined(QCC_OS_GROUP_WINDOWS) && ((_MSC_VER <= 1800) || defined(DO_SNPRINTF_MAPPING))
    /* Value returned by AllJoyn's implementation of snprintf when the Windows CRT doesn't provide one */
    ASSERT_GT(0, outputLength);
#else
    ASSERT_LT((int)ArraySize(smallBuffer), outputLength);
#endif

    ASSERT_EQ('\0', smallBuffer[ArraySize(smallBuffer) - 1]);
    std::cout << "smallBuffer = " << smallBuffer << std::endl;

    for (size_t characterIndex = 0; characterIndex < (ArraySize(smallBuffer) - 1); characterIndex++) {
        EXPECT_NE('\0', smallBuffer[characterIndex]);
        EXPECT_NE(fillCharacter, smallBuffer[characterIndex]);
    }
}