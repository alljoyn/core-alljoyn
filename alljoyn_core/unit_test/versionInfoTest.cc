/******************************************************************************
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/version.h>
#include <gtest/gtest.h>
#include <qcc/String.h>
#include <ctype.h>

TEST(VersionInfoTest, VersionInfo) {
    /*
     * version is expected to be string 'v#.#.#' where # represents a
     * number of unknown length This test code is most likely more complex than
     * the code use to generate the string but it should handle any value
     * returned
     */
    size_t pos1 = 1;
    size_t pos2 = 0;
    qcc::String ver = ajn::GetVersion();
    EXPECT_EQ('v', ver[0]);
    pos2 = ver.find_first_of(".");
    /*
     * unusual use of the unary operator '+' makes gcc compiler see qcc::String::npos as a rvalue
     * this prevents an 'undefined reference' compiler error when building with gcc.
     */
    EXPECT_NE(+qcc::String::npos, pos2);
    qcc::String architectureLevel = ver.substr(pos1, pos2 - pos1);
    for (unsigned int i = 0; i < architectureLevel.length(); i++) {
        EXPECT_TRUE(isdigit(architectureLevel[i]))
        << "architectureLevel version expected to be a number : "
        << architectureLevel.c_str();
    }
    pos1 = pos2 + 1;
    pos2 = ver.find_first_of(".", pos1);
    EXPECT_NE(+qcc::String::npos, pos2);

    qcc::String apiLevel = ver.substr(pos1, pos2 - pos1);
    for (unsigned int i = 0; i < apiLevel.length(); i++) {
        EXPECT_TRUE(isdigit(apiLevel[i]))
        << "apiLevel version expected to be a number : "
        << apiLevel.c_str();
    }
    qcc::String release = ver.substr(pos2 + 1);
    for (unsigned int i = 0; i < release.length(); i++) {
        EXPECT_TRUE(isdigit(release[i]))
        << "Release version expected to be a number : "
        << release.c_str();
    }
}
