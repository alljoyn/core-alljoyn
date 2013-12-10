/******************************************************************************
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
#include <alljoyn/Status.h>
/* Header files included for Google Test Framework */
#include <gtest/gtest.h>

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
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(i, -9999);

    status = arg.Set("s", "hello");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char*str;
    status = arg.Get("s", &str);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_STREQ("hello", str);

    MsgArg argList;
    status = argList.Set("(ybnqdiuxtsoqg)", y, b, n, q, d, i, u, x, t, s, o, q, g);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = argList.Get("(ybnqdiuxtsoqg)", &y, &b, &n, &q, &d, &i, &u, &x, &t, &s, &o, &q, &g);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = argList.Set("((ydx)(its))", y, d, x, i, t, s);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = argList.Get("((ydx)(its))", &y, &d, &x, &i, &t, &s);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(0, y);
    EXPECT_EQ(-1LL, x);
    EXPECT_EQ(-9999, i);
    EXPECT_EQ(0x6464646464646464ULL, t);
    EXPECT_STREQ("this is a string", s);

    status = arg.Set("((iuiu)(yd)at)", i, u, i, u, y, d, sizeof(at) / sizeof(at[0]), at);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int64_t*p64;
    size_t p64len;
    status = arg.Get("((iuiu)(yd)at)", &i, &u, &i, &u, &y, &d, &p64len, &p64);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Get("u", &i);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status) << "  Actual Status: " << QCC_StatusText(status);

    status = arg.Set("v", new MsgArg("d", &d));
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Get("i", &i);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Get("s", &str);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Get("d", &dt);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = arg.Set("v", new MsgArg("s", s));
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Get("i", &i);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Get("s", &str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    uint8_t*pay;
    size_t lay;
    status = arg.Get("ay", &lay, &pay);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(19, pay[1]);

    status = arg.Set("an", sizeof(an) / sizeof(an[0]), an);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int16_t*pan;
    size_t lan;
    status = arg.Get("an", &lan, &pan);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(-99, pan[1]);


    status = arg.Set("ai", sizeof(ai) / sizeof(ai[0]), ai);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int32_t*pai;
    size_t lai;
    status = arg.Get("ai", &lai, &pai);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(-88, pai[1]);


    status = arg.Set("ax", sizeof(ax) / sizeof(ax[0]), ax);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int64_t*pax;
    size_t lax;
    status = arg.Get("ax", &lax, &pax);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(-88, pax[1]);


    arg.Set("ad", sizeof(ad) / sizeof(ad[0]), ad);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    double*pad;
    size_t lad;
    status = arg.Get("ad", &lad, &pad);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(0.01, pad[1]);

    arg.Set("at", sizeof(at) / sizeof(at[0]), at);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    uint64_t*pat;
    size_t lat;
    status = arg.Get("at", &lat, &pat);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        uint8_t* pay;
        size_t lay;
        status = arg.Get("ay", &lay, &pay);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(ay) / sizeof(ay[0]), lay);
        for (size_t i = 0; i < lay; ++i) {
            EXPECT_EQ(ay[i], pay[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("an", sizeof(an) / sizeof(an[0]), an);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        int16_t* pan;
        size_t lan;
        status = arg.Get("an", &lan, &pan);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(an) / sizeof(an[0]), lan);
        for (size_t i = 0; i < lan; ++i) {
            EXPECT_EQ(an[i], pan[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("ai", sizeof(ai) / sizeof(ai[0]), ai);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        int32_t* pai;
        size_t lai;
        status = arg.Get("ai", &lai, &pai);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(ai) / sizeof(ai[0]), lai);
        for (size_t i = 0; i < lai; ++i) {
            EXPECT_EQ(ai[i], pai[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("ax", sizeof(ax) / sizeof(ax[0]), ax);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        int64_t* pax;
        size_t lax;
        status = arg.Get("ax", &lax, &pax);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(ax) / sizeof(ax[0]), lax);
        for (size_t i = 0; i < lax; ++i) {
            EXPECT_EQ(ax[i], pax[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("at", sizeof(at) / sizeof(at[0]), at);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        int64_t* pat;
        size_t lat;
        status = arg.Get("at", &lat, &pat);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(at) / sizeof(at[0]), lat);
        for (size_t i = 0; i < lat; ++i) {
            EXPECT_EQ(at[i], pat[i]);
        }
    }
    {
        MsgArg arg;
        status = arg.Set("ad", sizeof(ad) / sizeof(ad[0]), ad);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        double* pad;
        size_t lad;
        status = arg.Get("ad", &lad, &pad);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
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
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    MsgArg*pas;
    char*str[4];
    size_t las;
    status = arg.Get("as", &las, &pas);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (int k = 0; k < 4; k++) {
        pas[k].Get("s", &str[k]);
    }
    ASSERT_STREQ(str[1], "two");

    arg.Set("ag", sizeof(ag) / sizeof(ag[0]), ag);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    MsgArg*pag;
    char*str_ag[4];
    size_t lag;
    status = arg.Get("ag", &lag, &pag);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (int k = 0; k < 4; k++) {
        pag[k].Get("g", &str_ag[k]);
    }
    ASSERT_STREQ(str_ag[3], "a(iiiiuu)");


    arg.Set("ao", sizeof(ao) / sizeof(ao[0]), ao);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    MsgArg*pao;
    char*str_ao[4];
    size_t lao;
    status = arg.Get("ao", &lao, &pao);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (int k = 0; k < 4; k++) {
        pao[k].Get("o", &str_ao[k]);
    }
    ASSERT_STREQ(str_ao[3], "/org/four");


}

TEST(MsgArgTest, Dictionary)
{
    const char*keys[] = { "red", "green", "blue", "yellow" };
    MsgArg dict(ALLJOYN_ARRAY);
    size_t numEntries = sizeof(keys) / sizeof(keys[0]);
    MsgArg*dictEntries = new MsgArg[sizeof(keys) / sizeof(keys[0])];

    dictEntries[0].Set("{iv}", 1, new MsgArg("s", keys[0]));
    dictEntries[1].Set("{iv}", 1, new MsgArg("(ss)", keys[1], "bean"));
    dictEntries[2].Set("{iv}", 1, new MsgArg("s", keys[2]));
    dictEntries[3].Set("{iv}", 1, new MsgArg("(ss)", keys[3], "mellow"));

    QStatus status = dict.v_array.SetElements("{iv}", numEntries, dictEntries);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    MsgArg*entries;
    size_t num;
    status = dict.Get("a{iv}", &num, &entries);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < num; ++i) {
        char*str1;
        char*str2;
        uint32_t key;
        status = entries[i].Get("{is}", &key, &str1);
        if (status == ER_BUS_SIGNATURE_MISMATCH) {
            status = entries[i].Get("{i(ss)}", &key, &str1, &str2);
        }
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }
}

TEST(MsgArgTest, InvalidValues)
{
    QStatus status = ER_FAIL;
    MsgArg arg;
    status = arg.Set("o", "FailString"); //must be object path
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Set("o", "org/alljoyn/test"); //must start with '/' character
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Set("o", "/org/alljoyn//test"); // can not have repeated '/' characters
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Set("o", "/org/alljoyn/test/"); // can not end in '/' character
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Set("o", "/org/alljoyn/te*st"); // must be alpha numeric characters or '_'
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Set("o", "/"); // The only path allowed to end in '/' is the root path
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);;
    arg.Clear();
    status = arg.Set("o", "/org/alljoyn/test");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);


    status = arg.Set("g", "FailString"); //not a signature
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Set("g", "aaa"); //not arrays must end in a complete signature
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Set("g", "(sii"); //structs must end in a ')'
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Set("g", "sii)"); //structs must start in '('
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
    status = arg.Set("g", "a{si)"); //dictionaries must end in '}'
    ASSERT_EQ(ER_BUS_BAD_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
}
