/******************************************************************************
 *
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
