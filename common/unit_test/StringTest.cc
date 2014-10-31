/******************************************************************************
 *
 * Copyright (c) 2011,2014 AllSeen Alliance. All rights reserved.
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
#include <qcc/String.h>

TEST(StringTest, constructor) {
    const char* testStr = "abcdefgdijk";

    qcc::String s(testStr);
    ASSERT_STREQ(testStr, s.c_str());
    ASSERT_EQ(::strlen(testStr), s.size());
}

TEST(StringTest, find_first_of) {
    const char* testStr = "abcdefgdijk";
    qcc::String s(testStr);

    /* Test find_first_of */
    ASSERT_EQ(static_cast<size_t>(3), s.find_first_of('d'));
    ASSERT_EQ(static_cast<size_t>(3), s.find_first_of('d', 3));
    ASSERT_EQ(static_cast<size_t>(3), s.find_first_of("owed", 3));
    ASSERT_EQ(+qcc::String::npos, s.find_first_of('d', 8));
}

TEST(StringTest, find_last_of) {
    const char* testStr = "abcdefgdijk";
    qcc::String s(testStr);

    /* Test find_last_of */
    ASSERT_EQ(static_cast<size_t>(7), s.find_last_of('d'));
    ASSERT_EQ(static_cast<size_t>(3), s.find_last_of('d', 7));
    /*
     * unusual use of the unary operator '+' makes gcc compiler see qcc::String::npos as a rvalue
     * this prevents an 'undefined reference' compiler error when building with gcc.
     */
    ASSERT_EQ(+qcc::String::npos, s.find_last_of('d', 2));
}

TEST(StringTest, find_first_not_of) {
    const char* testStr = "abcdefgdijk";
    qcc::String s(testStr);

    /* Test find_*_not_of */
    qcc::String ss = "xyxyxyx" + s + "xy";
    ASSERT_EQ(static_cast<size_t>(7), ss.find_first_not_of("xy"));
    ASSERT_EQ(static_cast<size_t>(17), ss.find_last_not_of("xy"));
}

TEST(StringTest, empty) {
    const char* testStr = "abcdefgdijk";
    qcc::String s(testStr);

    /* Test empty */
    ASSERT_FALSE(s.empty());
    s.clear();
    ASSERT_TRUE(s.empty());
    ASSERT_EQ(static_cast<size_t>(0), s.size());
}

TEST(StringTest, operator_equals) {
    qcc::String s;
    /* Test operator= */
    s = "123456";
    ASSERT_STREQ("123456", s.c_str());
}

TEST(StringTest, copyConstructor) {
    /* test copy constructor */
    qcc::String s2 = "abcdefg";
    qcc::String t2 = s2;
    ASSERT_EQ(s2.c_str(), t2.c_str());
    ASSERT_TRUE(t2 == "abcdefg");
}

TEST(StringTest, append) {
    /* Test append */
    qcc::String pre = "abcd";
    qcc::String post = "efgh";
    pre.append(post);
    ASSERT_STREQ("abcdefgh", pre.c_str());
    ASSERT_EQ(::strlen("abcdefgh"), pre.size());
    ASSERT_STREQ("efgh", post.c_str());
    ASSERT_EQ(::strlen("efgh"), post.size());

    pre.append("ijklm", 4);
    ASSERT_EQ(::strlen("abcdefghijkl"), pre.size());
    ASSERT_STREQ("abcdefghijkl", pre.c_str());
}

TEST(StringTest, erase) {
    qcc::String pre("abcdefghijkl");
    /* Test erase */
    pre.erase(4, 2);
    ASSERT_STREQ("abcdghijkl", pre.c_str());

    /*
     * Test erasing past the end of the string. It should stop at the string
     * size.
     */
    pre.erase(pre.size() - 1, 100);
    ASSERT_STREQ("abcdghijk", pre.c_str());

    /*
     * Test erasing after the end of the string. It should be a no-op and should
     * not trigger any crash.
     */
    pre.erase(pre.size(), 2);
    ASSERT_STREQ("abcdghijk", pre.c_str());

    pre.erase(pre.size() + 1, 100);
    ASSERT_STREQ("abcdghijk", pre.c_str());
}

TEST(StringTest, resize) {
    qcc::String pre("abcdefghijk");
    ASSERT_EQ(static_cast<size_t>(11), pre.size());
    /* Test resize */
    pre.resize(4, 'x');
    ASSERT_EQ(static_cast<size_t>(4), pre.size());
    ASSERT_STREQ("abcd", pre.c_str());

    pre.resize(8, 'x');
    ASSERT_EQ(static_cast<size_t>(8), pre.size());
    ASSERT_STREQ("abcdxxxx", pre.c_str());
}

TEST(StringTest, reserve) {
    qcc::String pre("abcdxxxx");

    /* Test reserve */
    pre.reserve(100);
    const char* preAppend = pre.c_str();
    pre.append("y", 92);
    ASSERT_STREQ(preAppend, pre.c_str());
}

TEST(StringTest, insert) {
    /* Test insert */
    qcc::String s5 = "abcdijkl";
    s5.insert(4, "efgh");
    ASSERT_STREQ("abcdefghijkl", s5.c_str());
}

TEST(StringTest, logicOperators) {
    /* Test operator== and operator!= */
    qcc::String s5 = "abcdefghijkl";
    qcc::String s6 = "abcdefghijkl";
    ASSERT_TRUE(s5 == s6);
    ASSERT_FALSE(s5 != s6);

    /* Test operator< */
    ASSERT_FALSE(s5 < s6);
    ASSERT_FALSE(s6 < s5);
    s6.append('m');
    ASSERT_TRUE(s5 < s6);
    ASSERT_FALSE(s6 < s5);
}

TEST(StringTest, threeParamConstructor) {
    /* Test String(size_t, char, size_t) */
    qcc::String s3(8, 's', 8);
    ASSERT_STREQ("ssssssss", s3.c_str());
    ASSERT_EQ(::strlen("ssssssss"), s3.size());
}

TEST(StringTest, arrayOperator1) {
    /* test copy constructor and char& operator[] */
    qcc::String s2 = "abcdefg";
    qcc::String t2 = s2;
    t2[1] = 'B';
    ASSERT_STREQ("abcdefg", s2.c_str());
    ASSERT_STREQ("aBcdefg", t2.c_str());
}

TEST(StringTest, arrayOperator2) {
    /* Test const char& operator[] */
    const char* testChars = "abcdefgh";
    qcc::String s7 = testChars;
    const char* orig = s7.c_str();
    ASSERT_EQ(::strlen(testChars), s7.size());
    for (size_t i = 0; i < s7.size(); ++i) {
        char c = s7[i];
        ASSERT_EQ(c, testChars[i]);
    }
    ASSERT_STREQ(orig, s7.c_str());
}

TEST(StringTest, arrayOperator3) {
    const char* testStr = "abcdefgdijk";
    qcc::String s = testStr;
    ASSERT_EQ('a', s[0]);
    ASSERT_EQ('\0', s[11]);
}
TEST(StringTest, iterators) {
    const char* testChars = "abcdefgh";

    /* Test iterators */
    qcc::String s4(strlen(testChars), 'x');
    qcc::String::iterator it = s4.begin();
    for (size_t i = 0; i < s4.size(); ++i) {
        *it++ = testChars[i];
    }
    qcc::String::const_iterator cit = s4.begin();
    ASSERT_EQ(strlen(testChars), s4.size());
    size_t i = 0;
    while (cit != s4.end()) {
        ASSERT_EQ(*cit++, testChars[i++]);
    }
    ASSERT_EQ(strlen(testChars), i);
}

TEST(StringTest, substr) {
    const char* testStr = "abcdefgdijk";
    qcc::String s(testStr);

    /* Test substr */
    qcc::String s2 = s.substr(0, 4) + "1234";
    ASSERT_TRUE(s2 == "abcd1234");
    ASSERT_TRUE(s2.substr(4, 1) == "1");
    ASSERT_TRUE(s2.substr(1000, 1) == "");
    ASSERT_TRUE(s2.substr(0, 0) == "");
    ASSERT_EQ(0, s.compare(1, 2, s2, 1, 2));
}

TEST(StringTest, plusEqualsOperator) {
    /* Test += */
    qcc::String s = "";
    for (size_t i = 0; i < 1000; ++i) {
        s += "foo";
        ASSERT_EQ(3 * (i + 1), s.size());
    }

    /* Test resize */
    s.erase(3, s.size() - 6);
    ASSERT_EQ(static_cast<size_t>(6), s.size());
    ASSERT_TRUE(s == "foofoo");
    s.resize(s.size() + 3, 'x');
    ASSERT_TRUE(s == "foofooxxx");
}

/* ASACORE-1058 */
TEST(StringTest, assignDoesNotAppend) {
    const char* before = "012345678901234567890123456789012345";
    const char* after = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdef";
    qcc::String t(before);
    t.assign(after);
    ASSERT_STREQ(after, t.c_str());
}
