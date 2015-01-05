/******************************************************************************
 * Copyright (c) 2011, 2014-2015, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/MsgArg.h>
#include <alljoyn/Message.h>
#include <alljoyn/Status.h>
/* Header files included for Google Test Framework */
#include <gtest/gtest.h>
#include "ajTestCommon.h"

using namespace ajn;
using namespace std;

TEST(MsgArgTest, Basic) {
    QStatus status = ER_OK;

    /* BYTE */
    static uint8_t y = 0;
    /* BOOLEAN */
    static bool b = true;
    /* INT16 */
    static int16_t n = 42;
    /* UINT16 */
    static uint16_t q = 0xBEBE;
    /* DOUBLE */
    static double d = 3.14159265L;
    /* INT32 */
    static int32_t i = -9999;
    /* UINT32 */
    static uint32_t u = 0x32323232;
    /* INT64 */
    static int64_t x = -1LL;
    /* UINT64 */
    static uint64_t t = 0x6464646464646464ULL;
    /* STRING */
    static const char*s = "this is a string";
    /* OBJECT_PATH */
    static const char*o = "/org/foo/bar";
    /* SIGNATURE */
    static const char*g = "a{is}d(siiux)";
    /* Array of UINT64 */
    static int64_t at[] = { -8, -88, 888, 8888 };

    MsgArg arg("i", -9999);
    status = arg.Get("i", &i);
    ASSERT_EQ(ER_OK, status);
    ASSERT_EQ(i, -9999);

    status = arg.Set("s", "hello");
    ASSERT_EQ(ER_OK, status);
    char*str;
    status = arg.Get("s", &str);
    ASSERT_EQ(ER_OK, status);
    ASSERT_STREQ("hello", str);

    MsgArg argList;
    status = argList.Set("(ybnqdiuxtsoqg)", y, b, n, q, d, i, u, x, t, s, o, q, g);
    ASSERT_EQ(ER_OK, status);
    status = argList.Get("(ybnqdiuxtsoqg)", &y, &b, &n, &q, &d, &i, &u, &x, &t, &s, &o, &q, &g);
    ASSERT_EQ(ER_OK, status);
    ASSERT_EQ(0, y);
    ASSERT_EQ(true, b);
    ASSERT_EQ(42, n);
    ASSERT_EQ(0xBEBE, q);
    ASSERT_EQ(-9999, i);

    ASSERT_EQ(static_cast<uint32_t>(0x32323232), u);
    ASSERT_EQ(-1LL, x);
    ASSERT_EQ(0x6464646464646464ULL, t);
    ASSERT_STREQ("this is a string", s);
    ASSERT_STREQ("/org/foo/bar", o);
    ASSERT_EQ(0xBEBE, q);
    ASSERT_STREQ("a{is}d(siiux)", g);

    /*  Structs */
    ASSERT_EQ(ER_OK, status);
    status = argList.Set("((ydx)(its))", y, d, x, i, t, s);
    ASSERT_EQ(ER_OK, status);
    status = argList.Get("((ydx)(its))", &y, &d, &x, &i, &t, &s);
    ASSERT_EQ(ER_OK, status);
    EXPECT_EQ(0, y);
    EXPECT_EQ(-1LL, x);
    EXPECT_EQ(-9999, i);
    EXPECT_EQ(0x6464646464646464ULL, t);
    EXPECT_STREQ("this is a string", s);

    status = arg.Set("((iuiu)(yd)at)", i, u, i, u, y, d, sizeof(at) / sizeof(at[0]), at);
    ASSERT_EQ(ER_OK, status);
    int64_t*p64;
    size_t p64len;
    status = arg.Get("((iuiu)(yd)at)", &i, &u, &i, &u, &y, &d, &p64len, &p64);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(-9999, i);
    EXPECT_EQ(0x32323232u, u);
    EXPECT_EQ(0, y);
    EXPECT_EQ(p64len, sizeof(at) / sizeof(at[0]));
    for (size_t k = 0; k < p64len; ++k) {
        EXPECT_EQ(at[k], p64[k]) << "index " << k;
    }
}


TEST(MsgArgTest, Variants)
{
    /* DOUBLE */
    static double d = 3.14159265L;
    /* STRING */
    static const char*s = "this is a string";

    int32_t i;
    double dt;
    char*str;

    QStatus status = ER_OK;
    MsgArg arg;

    status = arg.Set("v", new MsgArg("i", 420));
    EXPECT_EQ(ER_OK, status);
    status = arg.Get("u", &i);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status);
    arg.SetOwnershipFlags(MsgArg::OwnsArgs);

    status = arg.Set("v", new MsgArg("d", &d));
    ASSERT_EQ(ER_OK, status);
    status = arg.Get("i", &i);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status);
    status = arg.Get("s", &str);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status);
    status = arg.Get("d", &dt);
    EXPECT_EQ(ER_OK, status);
    arg.SetOwnershipFlags(MsgArg::OwnsArgs);

    status = arg.Set("v", new MsgArg("s", s));
    ASSERT_EQ(ER_OK, status);
    status = arg.Get("i", &i);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status);
    status = arg.Get("s", &str);
    EXPECT_EQ(ER_OK, status);
    arg.SetOwnershipFlags(MsgArg::OwnsArgs);
}

TEST(MsgArgTest, Scalars)
{
    QStatus status = ER_OK;
    /* Array of BYTE */
    static uint8_t ay[] = { 9, 19, 29, 39, 49 };
    /* Array of INT16 */
    static int16_t an[] = { -9, -99, 999, 9999 };
    /* Array of INT32 */
    static int32_t ai[] = { -8, -88, 888, 8888 };
    /* Array of INT64 */
    static int64_t ax[] = { -8, -88, 888, 8888 };
    /* Array of UINT64 */
    static uint64_t at[] = { 98, 988, 9888, 98888 };
    /* Array of DOUBLE */
    static double ad[] = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0 };


    /*
     * Arrays of scalars
     */
    MsgArg arg;
    status = arg.Set("ay", sizeof(ay) / sizeof(ay[0]), ay);
    ASSERT_EQ(ER_OK, status);
    uint8_t*pay;
    size_t lay;
    status = arg.Get("ay", &lay, &pay);
    ASSERT_EQ(ER_OK, status);
    ASSERT_EQ(19, pay[1]);

    status = arg.Set("an", sizeof(an) / sizeof(an[0]), an);
    ASSERT_EQ(ER_OK, status);
    int16_t*pan;
    size_t lan;
    status = arg.Get("an", &lan, &pan);
    ASSERT_EQ(ER_OK, status);
    ASSERT_EQ(-99, pan[1]);


    status = arg.Set("ai", sizeof(ai) / sizeof(ai[0]), ai);
    ASSERT_EQ(ER_OK, status);
    int32_t*pai;
    size_t lai;
    status = arg.Get("ai", &lai, &pai);
    ASSERT_EQ(ER_OK, status);
    ASSERT_EQ(-88, pai[1]);


    status = arg.Set("ax", sizeof(ax) / sizeof(ax[0]), ax);
    ASSERT_EQ(ER_OK, status);
    int64_t*pax;
    size_t lax;
    status = arg.Get("ax", &lax, &pax);
    ASSERT_EQ(ER_OK, status);
    ASSERT_EQ(-88, pax[1]);


    arg.Set("ad", sizeof(ad) / sizeof(ad[0]), ad);
    ASSERT_EQ(ER_OK, status);
    double*pad;
    size_t lad;
    status = arg.Get("ad", &lad, &pad);
    ASSERT_EQ(ER_OK, status);
    ASSERT_EQ(0.01, pad[1]);

    arg.Set("at", sizeof(at) / sizeof(at[0]), at);
    ASSERT_EQ(ER_OK, status);
    uint64_t*pat;
    size_t lat;
    status = arg.Get("at", &lat, &pat);
    ASSERT_EQ(ER_OK, status);
    ASSERT_EQ(static_cast<uint64_t>(988), pat[1]);


}

TEST(MsgArgTest, arrays_of_scalars) {
    QStatus status = ER_OK;
    /* Array of BYTE */
    uint8_t ay[] = { 9, 19, 29, 39, 49 };
    /* Array of INT16 */
    static int16_t an[] = { -9, -99, 999, 9999 };
    /* Array of INT32 */
    static int32_t ai[] = { -8, -88, 888, 8888 };
    /* Array of INT64 */
    static int64_t ax[] = { -8, -88, 888, 8888 };
    /* Array of UINT64 */
    static int64_t at[] = { -8, -88, 888, 8888 };
    /* Array of DOUBLE */
    static double ad[] = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0 };

    /*
     * Arrays of scalars
     */
    {
        MsgArg arg;
        status = arg.Set("ay", sizeof(ay) / sizeof(ay[0]), ay);
        EXPECT_EQ(ER_OK, status);
        uint8_t* pay;
        size_t lay;
        status = arg.Get("ay", &lay, &pay);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(ay) / sizeof(ay[0]), lay);
        for (size_t i = 0; i < lay; ++i) {
            EXPECT_EQ(ay[i], pay[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("an", sizeof(an) / sizeof(an[0]), an);
        EXPECT_EQ(ER_OK, status);
        int16_t* pan;
        size_t lan;
        status = arg.Get("an", &lan, &pan);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(an) / sizeof(an[0]), lan);
        for (size_t i = 0; i < lan; ++i) {
            EXPECT_EQ(an[i], pan[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("ai", sizeof(ai) / sizeof(ai[0]), ai);
        EXPECT_EQ(ER_OK, status);
        int32_t* pai;
        size_t lai;
        status = arg.Get("ai", &lai, &pai);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(ai) / sizeof(ai[0]), lai);
        for (size_t i = 0; i < lai; ++i) {
            EXPECT_EQ(ai[i], pai[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("ax", sizeof(ax) / sizeof(ax[0]), ax);
        EXPECT_EQ(ER_OK, status);
        int64_t* pax;
        size_t lax;
        status = arg.Get("ax", &lax, &pax);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(ax) / sizeof(ax[0]), lax);
        for (size_t i = 0; i < lax; ++i) {
            EXPECT_EQ(ax[i], pax[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("at", sizeof(at) / sizeof(at[0]), at);
        EXPECT_EQ(ER_OK, status);
        int64_t* pat;
        size_t lat;
        status = arg.Get("at", &lat, &pat);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(at) / sizeof(at[0]), lat);
        for (size_t i = 0; i < lat; ++i) {
            EXPECT_EQ(at[i], pat[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("ad", sizeof(ad) / sizeof(ad[0]), ad);
        EXPECT_EQ(ER_OK, status);
        double* pad;
        size_t lad;
        status = arg.Get("ad", &lad, &pad);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(ad) / sizeof(ad[0]), lad);
        for (size_t i = 0; i < lad; ++i) {
            EXPECT_EQ(ad[i], pad[i]);
        }
    }
}

TEST(MsgArgTest, DiffStrings)
{
    QStatus status = ER_OK;
    MsgArg arg;
    /* Array of STRING */
    static const char*as[] = { "one", "two", "three", "four" };
    /* Array of OBJECT_PATH */
    static const char*ao[] = { "/org/one", "/org/two", "/org/three", "/org/four" };
    /* Array of SIGNATURE */
    static const char*ag[] = { "s", "sss", "as", "a(iiiiuu)" };

    arg.Set("as", sizeof(as) / sizeof(as[0]), as);
    ASSERT_EQ(ER_OK, status);
    MsgArg*pas;
    char*str[4];
    size_t las;
    status = arg.Get("as", &las, &pas);
    ASSERT_EQ(ER_OK, status);
    for (int k = 0; k < 4; k++) {
        pas[k].Get("s", &str[k]);
    }
    ASSERT_STREQ(str[1], "two");

    arg.Set("ag", sizeof(ag) / sizeof(ag[0]), ag);
    ASSERT_EQ(ER_OK, status);
    MsgArg*pag;
    char*str_ag[4];
    size_t lag;
    status = arg.Get("ag", &lag, &pag);
    ASSERT_EQ(ER_OK, status);
    for (int k = 0; k < 4; k++) {
        pag[k].Get("g", &str_ag[k]);
    }
    ASSERT_STREQ(str_ag[3], "a(iiiiuu)");


    arg.Set("ao", sizeof(ao) / sizeof(ao[0]), ao);
    ASSERT_EQ(ER_OK, status);
    MsgArg*pao;
    char*str_ao[4];
    size_t lao;
    status = arg.Get("ao", &lao, &pao);
    ASSERT_EQ(ER_OK, status);
    for (int k = 0; k < 4; k++) {
        pao[k].Get("o", &str_ao[k]);
    }
    ASSERT_STREQ(str_ao[3], "/org/four");


}

TEST(MsgArgTest, Dictionary)
{
    const char* keys[] = { "red", "green", "blue", "yellow" };
    MsgArg dict(ALLJOYN_ARRAY);
    size_t numEntries = sizeof(keys) / sizeof(keys[0]);
    MsgArg* dictEntries = new MsgArg[sizeof(keys) / sizeof(keys[0])];

    dictEntries[0].Set("{iv}", 1, new MsgArg("s", keys[0]));
    dictEntries[1].Set("{iv}", 1, new MsgArg("(ss)", keys[1], "bean"));
    dictEntries[2].Set("{iv}", 1, new MsgArg("s", keys[2]));
    dictEntries[3].Set("{iv}", 1, new MsgArg("(ss)", keys[3], "mellow"));

    QStatus status = dict.v_array.SetElements("{iv}", numEntries, dictEntries);
    ASSERT_EQ(ER_OK, status);

    dict.SetOwnershipFlags(MsgArg::OwnsArgs, true);

    MsgArg* entries;
    size_t num;
    status = dict.Get("a{iv}", &num, &entries);
    ASSERT_EQ(ER_OK, status);
    for (size_t i = 0; i < num; ++i) {
        char*str1;
        char*str2;
        uint32_t key;
        status = entries[i].Get("{is}", &key, &str1);
        if (status == ER_BUS_SIGNATURE_MISMATCH) {
            status = entries[i].Get("{i(ss)}", &key, &str1, &str2);
        }
        ASSERT_EQ(ER_OK, status);
    }
}

TEST(MsgArgTest, InvalidValues)
{
    QStatus status = ER_FAIL;
    MsgArg arg;
    status = arg.Set("o", "FailString"); //must be object path
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status);
    status = arg.Set("o", "org/alljoyn/test"); //must start with '/' character
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status);
    status = arg.Set("o", "/org/alljoyn//test"); // can not have repeated '/' characters
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status);
    status = arg.Set("o", "/org/alljoyn/test/"); // can not end in '/' character
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status);
    status = arg.Set("o", "/org/alljoyn/te*st"); // must be alpha numeric characters or '_'
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status);
    status = arg.Set("o", "/"); // The only path allowed to end in '/' is the root path
    ASSERT_EQ(ER_OK, status);;
    arg.Clear();
    status = arg.Set("o", "/org/alljoyn/test");
    ASSERT_EQ(ER_OK, status);


    status = arg.Set("g", "FailString"); //not a signature
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status);
    status = arg.Set("g", "aaa"); //not arrays must end in a complete signature
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status);
    status = arg.Set("g", "(sii"); //structs must end in a ')'
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status);
    status = arg.Set("g", "sii)"); //structs must start in '('
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status);
    status = arg.Set("g", "a{si)"); //dictionaries must end in '}'
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status);
}

TEST(MsgArgTest, StructContainingEmptyDict)
{
    QStatus status = ER_FAIL;
    MsgArg arg;

    static uint32_t u = 0x32323232;
    static const char* s = "this is a string";

    size_t numEntries = 0;
    MsgArg* dictEntries = NULL;

    status = arg.Set("(usa{sv})", u, s, numEntries, dictEntries);
    EXPECT_EQ(ER_OK, status);

    uint32_t uOut = 0;
    const char* sOut = NULL;

    //values given that are wrong and must be changed by the arg.Get call
    //if the values are not changed then the test should report failure.
    size_t numEntriesOut = 1000;
    MsgArg dummyArg;
    MsgArg* dictEntriesOut = &dummyArg;

    status = arg.Get("(usa{sv})", &uOut, &sOut, &numEntriesOut, &dictEntriesOut);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(u, uOut);
    EXPECT_STREQ(s, sOut);
    EXPECT_EQ(numEntries, numEntriesOut);
    EXPECT_TRUE(dictEntriesOut == NULL);
}

/*
 * In this we have version a and b of all of the variables used to create the
 * MsgArgs the reason is that we want to make sure no bug exists by comparing
 * pointers not actual values.
 */
TEST(MsgArgTest, Comparison) {
    QStatus status;
    MsgArg a;
    MsgArg b;

    //BYTE
    status = a.Set("y", 2);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("y", 2);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);

    //BOOLEAN
    status = a.Set("b", true);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("b", true);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);

    //INT16
    status = a.Set("n", -255);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("n", -255);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);

    //UINT16
    status = a.Set("q", 42);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("q", 42);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);

    //INT32
    status = a.Set("i", -1984);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("i", -1984);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);

    //UINT32
    status = a.Set("u", 1814);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("u", 1814);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);

    //INT64
    //Due to implicit casting on 32-bit systems we must explicitly cast
    //these values to a int64
    status = a.Set("x", static_cast<int64_t>(-29875));
    EXPECT_EQ(ER_OK, status);
    status = b.Set("x", static_cast<int64_t>(-29875));
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b) << "INT64 ERROR:\n" << a.ToString().c_str() << "\n-----\n" << a.ToString().c_str();

    //UINT64
    //Due to implicit casting on 32-bit systems we must explicitly cast
    //these values to a uint64
    status = a.Set("t", static_cast<uint64_t>(98746541));
    EXPECT_EQ(ER_OK, status);
    status = b.Set("t", static_cast<uint64_t>(98746541));
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b) << "UINT64 ERROR:\n" << a.ToString().c_str() << "\n-----\n" << a.ToString().c_str();

    //DOUBLE
    status = a.Set("d", 3.14);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("d", 3.14);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);

    //STRING
    status = a.Set("s", "AllJoyn Love");
    EXPECT_EQ(ER_OK, status);
    status = b.Set("s", "AllJoyn Love");
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);

    //OBJECT_PATH
    status = a.Set("o", "/for/bar");
    EXPECT_EQ(ER_OK, status);
    status = b.Set("o", "/for/bar");
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);

    //signature
    status = a.Set("g", "a{is}d(siiux)");
    EXPECT_EQ(ER_OK, status);
    status = b.Set("g", "a{is}d(siiux)");
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);

    //ARRAY
    /* Array of BYTE */
    uint8_t ay_a[] = { 9, 19, 29, 39, 49 };
    uint8_t ay_b[] = { 9, 19, 29, 39, 49 };
    status = a.Set("ay", sizeof(ay_a) / sizeof(ay_a[0]), ay_a);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("ay", sizeof(ay_b) / sizeof(ay_b[0]), ay_b);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);
    /* Array of INT16 */
    static int16_t an_a[] = { -9, -99, 999, 9999 };
    static int16_t an_b[] = { -9, -99, 999, 9999 };
    status = a.Set("an", sizeof(an_a) / sizeof(an_a[0]), an_a);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("an", sizeof(an_b) / sizeof(an_b[0]), an_b);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);
    /* Array of INT32 */
    static int32_t ai_a[] = { -8, -88, 888, 8888 };
    static int32_t ai_b[] = { -8, -88, 888, 8888 };
    status = a.Set("ai", sizeof(ai_a) / sizeof(ai_a[0]), ai_a);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("ai", sizeof(ai_b) / sizeof(ai_b[0]), ai_b);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);
    /* Array of INT64 */
    static int64_t ax_a[] = { -8, -88, 888, 8888 };
    static int64_t ax_b[] = { -8, -88, 888, 8888 };
    status = a.Set("ax", sizeof(ax_a) / sizeof(ax_a[0]), ax_a);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("ax", sizeof(ax_b) / sizeof(ax_b[0]), ax_b);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);
    /* Array of UINT64 */
    static int64_t at_a[] = { -8, -88, 888, 8888 };
    static int64_t at_b[] = { -8, -88, 888, 8888 };
    status = a.Set("at", sizeof(at_a) / sizeof(at_a[0]), at_a);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("at", sizeof(at_b) / sizeof(at_b[0]), at_b);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);
    /* Array of DOUBLE */
    static double ad_a[] = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0 };
    static double ad_b[] = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0 };
    status = a.Set("ad", sizeof(ad_a) / sizeof(ad_a[0]), ad_a);
    EXPECT_EQ(ER_OK, status);
    status = b.Set("ad", sizeof(ad_b) / sizeof(ad_b[0]), ad_b);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);
    /* Array of STRING */
    static const char*as_a[] = { "one", "two", "three", "four" };
    static const char*as_b[] = { "one", "two", "three", "four" };
    a.Set("as", sizeof(as_a) / sizeof(as_a[0]), as_a);
    EXPECT_EQ(ER_OK, status);
    b.Set("as", sizeof(as_b) / sizeof(as_b[0]), as_b);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);
    /* Array of OBJECT_PATH */
    static const char*ao_a[] = { "/org/one", "/org/two", "/org/three", "/org/four" };
    static const char*ao_b[] = { "/org/one", "/org/two", "/org/three", "/org/four" };
    a.Set("as", sizeof(ao_a) / sizeof(ao_a[0]), ao_a);
    EXPECT_EQ(ER_OK, status);
    b.Set("as", sizeof(ao_b) / sizeof(ao_b[0]), ao_b);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);
    /* Array of SIGNATURE */
    static const char*ag_a[] = { "s", "sss", "as", "a(iiiiuu)" };
    static const char*ag_b[] = { "s", "sss", "as", "a(iiiiuu)" };
    a.Set("as", sizeof(ag_a) / sizeof(ag_a[0]), ag_a);
    EXPECT_EQ(ER_OK, status);
    b.Set("as", sizeof(ag_b) / sizeof(ag_b[0]), ag_b);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b);

    //STRUCT
    a.Set("(nuds)", 12, 42, 3.14, "AllJoyn");
    EXPECT_EQ(ER_OK, status);
    b.Set("(nuds)", 12, 42, 3.14, "AllJoyn");
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b) << "STRUCT ERROR:\n" << a.ToString().c_str() << "\n-----\n" << a.ToString().c_str();

    //VARIANT
    status = a.Set("v", new MsgArg("s", "AllSeen"));
    EXPECT_EQ(ER_OK, status);
    a.SetOwnershipFlags(MsgArg::OwnsArgs);
    status = b.Set("v", new MsgArg("s", "AllSeen"));
    b.SetOwnershipFlags(MsgArg::OwnsArgs);
    EXPECT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b) << "VARIANT ERROR:\n" << a.ToString().c_str() << "\n-----\n" << a.ToString().c_str();

    //DICT
    const char* keys_a[] = { "red", "green", "blue", "yellow" };
    size_t numEntries_a = sizeof(keys_a) / sizeof(keys_a[0]);
    MsgArg* dictEntries_a = new MsgArg[sizeof(keys_a) / sizeof(keys_a[0])];

    dictEntries_a[0].Set("{iv}", 1, new MsgArg("s", keys_a[0]));
    dictEntries_a[1].Set("{iv}", 1, new MsgArg("(ss)", keys_a[1], "bean"));
    dictEntries_a[2].Set("{iv}", 1, new MsgArg("s", keys_a[2]));
    dictEntries_a[3].Set("{iv}", 1, new MsgArg("(ss)", keys_a[3], "mellow"));

    status = a.Set("a{iv}", numEntries_a, dictEntries_a);
    ASSERT_EQ(ER_OK, status);

    a.SetOwnershipFlags(MsgArg::OwnsArgs, true);

    const char* keys_b[] = { "red", "green", "blue", "yellow" };
    size_t numEntries_b = sizeof(keys_b) / sizeof(keys_b[0]);
    MsgArg* dictEntries_b = new MsgArg[sizeof(keys_b) / sizeof(keys_b[0])];

    dictEntries_b[0].Set("{iv}", 1, new MsgArg("s", keys_b[0]));
    dictEntries_b[1].Set("{iv}", 1, new MsgArg("(ss)", keys_b[1], "bean"));
    dictEntries_b[2].Set("{iv}", 1, new MsgArg("s", keys_b[2]));
    dictEntries_b[3].Set("{iv}", 1, new MsgArg("(ss)", keys_b[3], "mellow"));

    status = b.Set("a{iv}", numEntries_b, dictEntries_b);
    ASSERT_EQ(ER_OK, status);
    EXPECT_TRUE(a == b) << "DICT ERROR:\n" << a.ToString().c_str() << "\n-----\n" << a.ToString().c_str();

    b.SetOwnershipFlags(MsgArg::OwnsArgs, true);
}

// this test has been added mostly for memory verification tools to check that
// memory is released as it should be.  We also expect the code to not crash
TEST(MsgArgTest, SetOwnershipFlags_scalar_arrays) {
    QStatus status = ER_OK;
    /* Array of BYTE */
    const uint16_t AY_SIZE = 9;
    uint8_t* ay = new uint8_t[AY_SIZE];
    //{ 9, 19, 29, 39, 49 }
    for (size_t i = 0; i < AY_SIZE; ++i) {
        ay[i] = (10 * i) + 9;
    }
    /* Array of INT16 */
    int16_t* an = new int16_t[4];
    an[0] = -9;
    an[1] = -99;
    an[2] = 999;
    an[3] = 9999;
    /* Array of INT32 */
    int32_t* ai = new int32_t[4];
    //ai = { -8, -88, 888, 8888 };
    ai[0] = -8;
    ai[1] = -88;
    ai[2] = 888;
    ai[3] = 8888;
    /* Array of INT64 */
    int64_t* ax = new int64_t[4];
    //ax = { -8, -88, 888, 8888 };
    ax[0] = -8;
    ax[1] = -88;
    ax[2] = 888;
    ax[3] = 8888;
    /* Array of UINT64 */
    uint64_t* at = new uint64_t[4];
    //at = { 98, 988, 9888, 98888 };
    at[0] = 98;
    at[1] = 988;
    at[2] = 9888;
    at[3] = 98888;
    /* Array of DOUBLE */
    double* ad = new double[6];
    //ad = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0 };
    ad[0] = 0.001;
    ad[1] = 0.01;
    ad[2] = 0.1;
    ad[3] = 1.0;
    ad[4] = 10.0;
    ad[5] = 100.0;
    /*
     * Arrays of scalars
     */
    {
        MsgArg arg;

        status = arg.Set("ay", sizeof(ay) / sizeof(ay[0]), ay);
        arg.SetOwnershipFlags(MsgArg::OwnsData);
        EXPECT_EQ(ER_OK, status);
        uint8_t* pay;
        size_t lay;
        status = arg.Get("ay", &lay, &pay);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(ay) / sizeof(ay[0]), lay);
        for (size_t i = 0; i < lay; ++i) {
            EXPECT_EQ(ay[i], pay[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("an", sizeof(an) / sizeof(an[0]), an);
        arg.SetOwnershipFlags(MsgArg::OwnsData);
        EXPECT_EQ(ER_OK, status);
        int16_t* pan;
        size_t lan;
        status = arg.Get("an", &lan, &pan);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(an) / sizeof(an[0]), lan);
        for (size_t i = 0; i < lan; ++i) {
            EXPECT_EQ(an[i], pan[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("ai", sizeof(ai) / sizeof(ai[0]), ai);
        arg.SetOwnershipFlags(MsgArg::OwnsData);
        EXPECT_EQ(ER_OK, status);
        int32_t* pai;
        size_t lai;
        status = arg.Get("ai", &lai, &pai);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(ai) / sizeof(ai[0]), lai);
        for (size_t i = 0; i < lai; ++i) {
            EXPECT_EQ(ai[i], pai[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("ax", sizeof(ax) / sizeof(ax[0]), ax);
        arg.SetOwnershipFlags(MsgArg::OwnsData);
        EXPECT_EQ(ER_OK, status);
        int64_t* pax;
        size_t lax;
        status = arg.Get("ax", &lax, &pax);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(ax) / sizeof(ax[0]), lax);
        for (size_t i = 0; i < lax; ++i) {
            EXPECT_EQ(ax[i], pax[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("at", sizeof(at) / sizeof(at[0]), at);
        arg.SetOwnershipFlags(MsgArg::OwnsData);
        EXPECT_EQ(ER_OK, status);
        uint64_t* pat;
        size_t lat;
        status = arg.Get("at", &lat, &pat);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(at) / sizeof(at[0]), lat);
        for (size_t i = 0; i < lat; ++i) {
            EXPECT_EQ(at[i], pat[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("ad", sizeof(ad) / sizeof(ad[0]), ad);
        arg.SetOwnershipFlags(MsgArg::OwnsData);
        EXPECT_EQ(ER_OK, status);
        double* pad;
        size_t lad;
        status = arg.Get("ad", &lad, &pad);
        EXPECT_EQ(ER_OK, status);
        EXPECT_EQ(sizeof(ad) / sizeof(ad[0]), lad);
        for (size_t i = 0; i < lad; ++i) {
            EXPECT_EQ(ad[i], pad[i]);
        }
    }
}

// this test has been added mostly for memory verification tools to check that
// memory is released as it should be.  We also expect the code to not crash
TEST(MsgArgTest, SetOwnershipFlags_struct) {
    MsgArg arg;
    qcc::String str1 = "hello";
    const size_t SIZE = 4;
    qcc::String astr1[SIZE];
    astr1[0] = "the";
    astr1[1] = "sea";
    astr1[2] = "is";
    astr1[3] = "amazing";
    arg.Set("(sas)", str1.c_str(), SIZE, astr1);
    arg.SetOwnershipFlags(MsgArg::OwnsData | MsgArg::OwnsArgs);
}

TEST(MsgArgTest, emptyArrays_byte) {
    QStatus status;
    //byte or uint8_t
    MsgArg arg;
    status = arg.Set("ay", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    uint8_t* py;
    size_t lpy;
    status = arg.Get("ay", &lpy, &py);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpy);
    EXPECT_TRUE(py == NULL);
}

TEST(MsgArgTest, emptyArrays_bool) {
    QStatus status;
    //bool
    MsgArg arg;
    status = arg.Set("ab", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    bool* pb;
    size_t lpb;
    status = arg.Get("ab", &lpb, &pb);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpb);
    EXPECT_TRUE(pb == NULL);
}
TEST(MsgArgTest, emptyArrays_int16) {
    QStatus status;
    //int16_t
    MsgArg arg;
    status = arg.Set("an", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    int16_t* pn;
    size_t lpn;
    status = arg.Get("an", &lpn, &pn);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpn);
    EXPECT_TRUE(pn == NULL);
}
TEST(MsgArgTest, emptyArrays_uint16) {
    QStatus status;
    //uint16_t
    MsgArg arg;
    status = arg.Set("aq", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    uint16_t* pq;
    size_t lpq;
    status = arg.Get("aq", &lpq, &pq);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpq);
    EXPECT_TRUE(pq == NULL);
}
TEST(MsgArgTest, emptyArrays_double) {
    QStatus status;
    //double
    MsgArg arg;
    status = arg.Set("ad", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    double* pd;
    size_t lpd;
    status = arg.Get("ad", &lpd, &pd);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpd);
    EXPECT_TRUE(pd == NULL);
}
TEST(MsgArgTest, emptyArrays_int32) {
    QStatus status;
    //int32_t
    MsgArg arg;
    status = arg.Set("ai", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    int32_t* pi;
    size_t lpi;
    status = arg.Get("ai", &lpi, &pi);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpi);
    EXPECT_TRUE(pi == NULL);
}
TEST(MsgArgTest, emptyArrays_uint32) {
    QStatus status;
    //uint32_t
    MsgArg arg;
    status = arg.Set("au", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    uint32_t* pu;
    size_t lpu;
    status = arg.Get("au", &lpu, &pu);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpu);
    EXPECT_TRUE(pu == NULL);
}
TEST(MsgArgTest, emptyArrays_int64) {
    QStatus status;
    //int64_t
    MsgArg arg;
    status = arg.Set("ax", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    int64_t* px;
    size_t lpx;
    status = arg.Get("ax", &lpx, &px);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpx);
    EXPECT_TRUE(px == NULL);
}
TEST(MsgArgTest, emptyArrays_uint64) {
    QStatus status;
    //uint64_t
    MsgArg arg;
    status = arg.Set("at", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    uint64_t* pt;
    size_t lpt;
    status = arg.Get("at", &lpt, &pt);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpt);
    EXPECT_TRUE(pt == NULL);
}
TEST(MsgArgTest, emptyArrays_string) {
    QStatus status;
    //String
    MsgArg arg;
    status = arg.Set("as", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    MsgArg* ps;
    size_t lps;
    status = arg.Get("as", &lps, &ps);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lps);
    EXPECT_TRUE(ps == NULL);
}
TEST(MsgArgTest, emptyArrays_objectpath) {
    QStatus status;
    //object path
    MsgArg arg;
    status = arg.Set("ao", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    MsgArg* po;
    size_t lpo;
    status = arg.Get("ao", &lpo, &po);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpo);
    EXPECT_TRUE(po == NULL);
}
TEST(MsgArgTest, emptyArrays_signature) {
    QStatus status;
    //Signature
    MsgArg arg;
    status = arg.Set("ag", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    MsgArg* pg;
    size_t lpg;
    status = arg.Get("ag", &lpg, &pg);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpg);
    EXPECT_TRUE(pg == NULL);
}
TEST(MsgArgTest, emptyArrays_struct) {
    QStatus status;
    //Struct
    MsgArg arg;
    status = arg.Set("a(sis)", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    MsgArg* psis;
    size_t lpsis;
    status = arg.Get("a(sis)", &lpsis, &psis);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpsis);
    EXPECT_TRUE(psis == NULL);
}
TEST(MsgArgTest, emptyArrays_variant) {
    QStatus status;
    //variant
    MsgArg arg;
    status = arg.Set("av", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    MsgArg* pv;
    size_t lpv;
    status = arg.Get("av", &lpv, &pv);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpv);
    EXPECT_TRUE(pv == NULL);
}
TEST(MsgArgTest, emptyArrays_dictionary) {
    QStatus status;
    //Dictionary
    MsgArg arg;
    status = arg.Set("a{sv}", 0, NULL);
    EXPECT_EQ(ER_OK, status);
    MsgArg* pdict;
    size_t lpdict;
    status = arg.Get("a{sv}", &lpdict, &pdict);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(0u, lpdict);
    EXPECT_TRUE(pdict == NULL);
}

static uint8_t max_length_ay[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_byte) {
    QStatus status;
    MsgArg arg;
    // stdint.h is not avalible for all platforms so we create local variables
    const uint8_t uint8_max = 255;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_ay[i] = i % uint8_max;
    }
    status = arg.Set("ay", ALLJOYN_MAX_ARRAY_LEN, max_length_ay);
    EXPECT_EQ(ER_OK, status);
    uint8_t* py;
    size_t lpy;
    status = arg.Get("ay", &lpy, &py);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpy);
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        ASSERT_EQ(i % uint8_max, py[i]);
    }
}

static bool max_length_ab[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_bool) {
    QStatus status;
    MsgArg arg;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_ab[i] = (i % 2) ? true : false;
    }
    status = arg.Set("ab", ALLJOYN_MAX_ARRAY_LEN, max_length_ab);
    EXPECT_EQ(ER_OK, status);
    bool* pb;
    size_t lpb;
    status = arg.Get("ab", &lpb, &pb);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpb);
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; i += 2) {
        ASSERT_FALSE(pb[i]);
        ASSERT_TRUE(pb[i + 1]);
    }
}

static int16_t max_length_an[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_int16) {
    QStatus status;
    MsgArg arg;
    // stdint.h is not avalible for all platforms so we create local variables
    const int16_t int16_max = 32767;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_an[i] = i % int16_max;
    }
    status = arg.Set("an", ALLJOYN_MAX_ARRAY_LEN, max_length_an);
    EXPECT_EQ(ER_OK, status);
    int16_t* pan;
    size_t lpan;
    status = arg.Get("an", &lpan, &pan);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpan);
    for (size_t i = 0; i < lpan; ++i) {
        ASSERT_EQ(static_cast<int16_t>(i % int16_max), pan[i]);
    }
}

static uint16_t max_length_aq[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_uint16) {
    QStatus status;
    MsgArg arg;
    // stdint.h is not avalible for all platforms so we create local variables
    const uint16_t uint16_max = 65535;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_aq[i] = i % uint16_max;
    }
    status = arg.Set("an", ALLJOYN_MAX_ARRAY_LEN, max_length_aq);
    EXPECT_EQ(ER_OK, status);
    uint16_t* paq;
    size_t lpaq;
    status = arg.Get("an", &lpaq, &paq);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpaq);
    for (size_t i = 0; i < lpaq; ++i) {
        ASSERT_EQ(i % uint16_max, paq[i]);
    }
}

static double max_length_ad[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_double) {
    QStatus status;
    MsgArg arg;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_ad[i] = static_cast<double>(i);
    }
    status = arg.Set("ad", ALLJOYN_MAX_ARRAY_LEN, max_length_ad);
    EXPECT_EQ(ER_OK, status);
    double* pad;
    size_t lpad;
    status = arg.Get("ad", &lpad, &pad);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpad);
    for (size_t i = 0; i < lpad; ++i) {
        ASSERT_EQ(max_length_ad[i], pad[i]);
    }
}

static int32_t max_length_ai[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_int32) {
    QStatus status;
    MsgArg arg;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_ai[i] = i;
    }
    status = arg.Set("ai", ALLJOYN_MAX_ARRAY_LEN, max_length_ai);
    EXPECT_EQ(ER_OK, status);
    int32_t* pai;
    size_t lpai;
    status = arg.Get("ai", &lpai, &pai);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpai);
    for (size_t i = 0; i < lpai; ++i) {
        ASSERT_EQ(max_length_ai[i], pai[i]);
    }
}

static uint32_t max_length_au[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_uint32) {
    QStatus status;
    MsgArg arg;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_au[i] = i;
    }
    status = arg.Set("au", ALLJOYN_MAX_ARRAY_LEN, max_length_au);
    EXPECT_EQ(ER_OK, status);
    uint32_t* pau;
    size_t lpau;
    status = arg.Get("au", &lpau, &pau);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpau);
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        ASSERT_EQ(max_length_au[i], pau[i]);
    }
}

static int64_t max_length_ax[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_int64) {
    QStatus status;
    MsgArg arg;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_ax[i] = i;
    }
    status = arg.Set("ax", ALLJOYN_MAX_ARRAY_LEN, max_length_ax);
    EXPECT_EQ(ER_OK, status);
    int64_t* pax;
    size_t lpax;
    status = arg.Get("ax", &lpax, &pax);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpax);
    for (size_t i = 0; i < lpax; ++i) {
        ASSERT_EQ(max_length_ax[i], pax[i]);
    }
}

static uint64_t max_length_at[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_uint64) {
    QStatus status;
    MsgArg arg;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_at[i] = i;
    }
    status = arg.Set("at", ALLJOYN_MAX_ARRAY_LEN, max_length_at);
    EXPECT_EQ(ER_OK, status);
    uint64_t* pat;
    size_t lpat;
    status = arg.Get("at", &lpat, &pat);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpat);
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        ASSERT_EQ(max_length_at[i], pat[i]);
    }
}

const char* max_length_as[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_string) {
    QStatus status;
    MsgArg arg;
    const char* in_string = "a";
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_as[i] = in_string;
    }
    status = arg.Set("as", ALLJOYN_MAX_ARRAY_LEN, max_length_as);
    EXPECT_EQ(ER_OK, status);
    MsgArg* pas;
    size_t lpas;
    status = arg.Get("as", &lpas, &pas);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpas);
    const char* s;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        status = pas[i].Get("s", &s);
        EXPECT_EQ(ER_OK, status);
        ASSERT_STREQ(in_string, s);
    }
}

static const char* max_length_ao[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_objectpath) {
    QStatus status;
    MsgArg arg;
    const char* in_string = "/a";
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_ao[i] = in_string;
    }
    status = arg.Set("ao", ALLJOYN_MAX_ARRAY_LEN, max_length_ao);
    EXPECT_EQ(ER_OK, status);
    MsgArg* pao;
    size_t lpao;
    status = arg.Get("ao", &lpao, &pao);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpao);
    const char* o;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        status = pao[i].Get("o", &o);
        EXPECT_EQ(ER_OK, status);
        ASSERT_STREQ(in_string, o);
    }
}

static const char* max_length_ag[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_signature) {
    QStatus status;
    MsgArg arg;
    const char* in_string = "sis";
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        max_length_ag[i] = in_string;
    }
    status = arg.Set("ag", ALLJOYN_MAX_ARRAY_LEN, max_length_ag);
    EXPECT_EQ(ER_OK, status);
    MsgArg* pag;
    size_t lpag;
    status = arg.Get("ag", &lpag, &pag);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpag);
    const char* g;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        status = pag[i].Get("g", &g);
        EXPECT_EQ(ER_OK, status);
        ASSERT_STREQ(in_string, g);
    }
}

static MsgArg max_length_av[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_variant) {
    QStatus status;
    MsgArg arg;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        status = max_length_av[i].Set("v", new MsgArg("i", i));
        EXPECT_EQ(ER_OK, status);
        max_length_av[i].SetOwnershipFlags(MsgArg::OwnsArgs);
    }
    status = arg.Set("av", ALLJOYN_MAX_ARRAY_LEN, max_length_av);
    EXPECT_EQ(ER_OK, status);
    MsgArg* pav;
    size_t lpav;
    status = arg.Get("av", &lpav, &pav);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpav);
    int32_t iValue;
    for (size_t i = 0; i < lpav; ++i) {
        status = pav[i].Get("i", &iValue);
        EXPECT_EQ(ER_OK, status);
        ASSERT_EQ(static_cast<int32_t>(i), iValue);
    }
}

static MsgArg max_length_adictEntry[ALLJOYN_MAX_ARRAY_LEN];
TEST(MsgArgTest, MaximumLengthArrays_dictionary) {
    QStatus status;
    MsgArg arg;
    for (size_t i = 0; i < ALLJOYN_MAX_ARRAY_LEN; ++i) {
        status = max_length_adictEntry[i].Set("{xx}", i, i);
        EXPECT_EQ(ER_OK, status);
    }
    status = arg.Set("a{xx}", ALLJOYN_MAX_ARRAY_LEN, max_length_adictEntry);
    EXPECT_EQ(ER_OK, status);
    MsgArg* padict;
    size_t lpadict;
    status = arg.Get("a{xx}", &lpadict, &padict);
    EXPECT_EQ(ER_OK, status);
    EXPECT_EQ(ALLJOYN_MAX_ARRAY_LEN, lpadict);
    int64_t xKey;
    int64_t xValue;
    for (size_t i = 0; i < lpadict; ++i) {
        padict[i].Get("{xx}", &xKey, &xValue);
        ASSERT_EQ(static_cast<int64_t>(i), xKey);
        ASSERT_EQ(static_cast<int64_t>(i), xValue);
    }
}

static uint8_t* over_max_length_ay[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_byte) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("ay", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_ay);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static bool* over_max_length_ab[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_bool) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("ab", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_ab);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static int16_t* over_max_length_an[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_int16) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("an", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_an);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static uint16_t* over_max_length_aq[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_uint16) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("aq", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_aq);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static double* over_max_length_ad[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_double) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("ad", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_ad);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static int32_t* over_max_length_ai[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_int32) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("ai", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_ai);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static uint32_t* over_max_length_au[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_uint32) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("au", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_au);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static int64_t* over_max_length_ax[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_int64) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("ax", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_ax);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static uint64_t* over_max_length_at[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_uint64) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("at", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_at);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static const char* over_max_length_as[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_string) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("as", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_as);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static const char* over_max_length_ao[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_objectpath) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("ao", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_ao);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static const char* over_max_length_ag[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_signature) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("ag", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_ag);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static MsgArg* over_max_length_astruct[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_struct) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("a(sis)", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_astruct);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static MsgArg* over_max_length_av[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_variant) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("av", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_av);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}

static MsgArg* over_max_length_adict[ALLJOYN_MAX_ARRAY_LEN + 1];
TEST(MsgArgTest, MaximumLengthArraysPlusOne_dictionary) {
    QStatus status;
    MsgArg arg;
    status = arg.Set("a{ss}", ALLJOYN_MAX_ARRAY_LEN + 1, over_max_length_adict);
    EXPECT_EQ(ER_BUS_BAD_VALUE, status);
}
