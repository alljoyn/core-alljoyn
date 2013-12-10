/**
 * @file
 *
 * Unit test of the name validation checks
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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


#include <BusUtil.h>
#include <stdio.h>

#include <gtest/gtest.h>

using namespace ajn;

TEST(NamesTest, Basic) {
    /*
     * Basic checks - should all pass
     */
    EXPECT_TRUE(IsLegalUniqueName(":1.0"));
    EXPECT_TRUE(IsLegalBusName("th_is.t9h-At"));
    EXPECT_TRUE(IsLegalObjectPath("/This/tha_t/99"));
    EXPECT_TRUE(IsLegalInterfaceName("THIS._that._1__"));
    EXPECT_TRUE(IsLegalMemberName("this2Isa_member"));
}

void PadTo(char* buf, const char* str, size_t len, char pad)
{
    strcpy(buf, str);
    for (size_t i = strlen(buf); i < len; i++) {
        buf[i] = pad;
    }
    buf[len] = 0;
}

TEST(NamesTest, Maximum_length) {
    /*
     * Maximum length checks - should all pass
     */
    char buf[512];

    PadTo(buf, ":1.0.", 255, '0');
    EXPECT_EQ(static_cast<size_t>(255), strlen(buf));
    EXPECT_TRUE(IsLegalUniqueName(buf));

    PadTo(buf, "abc.def.hij.", 255, '-');
    EXPECT_EQ(static_cast<size_t>(255), strlen(buf));
    EXPECT_TRUE(IsLegalBusName(buf));

    PadTo(buf, "abc.def.hij.", 255, '_');
    EXPECT_EQ(static_cast<size_t>(255), strlen(buf));
    EXPECT_TRUE(IsLegalInterfaceName(buf));

    PadTo(buf, "member", 255, '_');
    EXPECT_EQ(static_cast<size_t>(255), strlen(buf));
    EXPECT_TRUE(IsLegalMemberName(buf));

    /*
     * There is no maximum length for object paths
     */
    PadTo(buf, "/object/path/long/", 500, '_');
    EXPECT_EQ(static_cast<size_t>(500), strlen(buf));
    EXPECT_TRUE(IsLegalObjectPath(buf));
}

TEST(NamesTest, Beyond_Maximum_length) {
    /*
     * Beyond maximum length checks - should all fail
     */
    char buf[512];

    PadTo(buf, ":1.0.", 256, '0');
    EXPECT_EQ(static_cast<size_t>(256), strlen(buf));
    EXPECT_FALSE(IsLegalUniqueName(buf));

    PadTo(buf, "abc.def.hij.", 256, '-');
    EXPECT_EQ(static_cast<size_t>(256), strlen(buf));
    EXPECT_FALSE(IsLegalBusName(buf));

    PadTo(buf, "abc.def.hij.", 256, '_');
    EXPECT_EQ(static_cast<size_t>(256), strlen(buf));
    EXPECT_FALSE(IsLegalInterfaceName(buf));

    PadTo(buf, "member", 256, '_');
    EXPECT_EQ(static_cast<size_t>(256), strlen(buf));
    EXPECT_FALSE(IsLegalMemberName(buf));
}

TEST(NamesTest, name_list) {
    const int STR_BUF_SIZE = 128;
    char str[STR_BUF_SIZE];
    snprintf(str, STR_BUF_SIZE, "foo");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_TRUE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, ":foo");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, ":foo.2");
    EXPECT_TRUE(IsLegalUniqueName(str));
    EXPECT_TRUE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "/foo/bar");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_TRUE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "/foo//bar");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "/foo/bar/");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "foo/bar/");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "/");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_TRUE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "foo.bar");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_TRUE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_TRUE(IsLegalInterfaceName(str));
    EXPECT_TRUE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, ".foo.bar");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "foo.bar.");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "foo..bar");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "_._._");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_TRUE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_TRUE(IsLegalInterfaceName(str));
    EXPECT_TRUE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "-.-.-");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_TRUE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "8.8.8");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "999");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, "_999");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_TRUE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, ":1.0");
    EXPECT_TRUE(IsLegalUniqueName(str));
    EXPECT_TRUE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, ":1.0.2.3.4");
    EXPECT_TRUE(IsLegalUniqueName(str));
    EXPECT_TRUE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, ":1.0.2.3..4");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, ":1.0.2.3.4.");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));

    snprintf(str, STR_BUF_SIZE, ":.1.0");
    EXPECT_FALSE(IsLegalUniqueName(str));
    EXPECT_FALSE(IsLegalBusName(str));
    EXPECT_FALSE(IsLegalObjectPath(str));
    EXPECT_FALSE(IsLegalInterfaceName(str));
    EXPECT_FALSE(IsLegalErrorName(str));
    EXPECT_FALSE(IsLegalMemberName(str));
}
