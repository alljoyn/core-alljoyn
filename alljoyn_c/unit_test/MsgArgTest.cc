/******************************************************************************
 * Copyright (c) 2012-2013, AllSeen Alliance. All rights reserved.
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
#include <alljoyn_c/MsgArg.h>
#include <alljoyn_c/MsgArg.h>
#include <Status.h>
#include <qcc/Thread.h>
#include "ajTestCommon.h"

TEST(MsgArgTest, Basic) {
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
    /* UINT32 */
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

    /* BYTE */
    uint8_t yout;
    /* BOOLEAN */
    bool bout;
    /* INT16 */
    int16_t nout;
    /* UINT16 */
    uint16_t qout;
    /* DOUBLE */
    double dout;
    /* INT32 */
    int32_t iout;
    /* UINT32 */
    uint32_t uout;
    /* INT64 */
    int64_t xout;
    /* UINT64 */
    uint64_t tout;
    /* STRING */
    const char* sout;
    /* OBJECT_PATH */
    const char* oout;
    /* SIGNATURE */
    const char* gout;

    QStatus status = ER_OK;
    alljoyn_msgarg arg = NULL;
    status = alljoyn_msgarg_set(arg, "i", -9999);
    EXPECT_EQ(ER_BAD_ARG_1, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_msgarg_get(arg, "i", &iout);
    EXPECT_EQ(ER_BAD_ARG_1, status) << "  Actual Status: " << QCC_StatusText(status);

    arg = alljoyn_msgarg_create();
    EXPECT_TRUE(arg != NULL);

    status = alljoyn_msgarg_set(arg, "i", -9999);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_msgarg_get(arg, "i", &iout);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(iout, -9999);

    status = alljoyn_msgarg_set(arg, "s", "hello");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    char* str;
    status = alljoyn_msgarg_get(arg, "s", &str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_STREQ("hello", str);

    alljoyn_msgarg_destroy(arg);

    alljoyn_msgarg argList = NULL;
    argList = alljoyn_msgarg_create();
    EXPECT_TRUE(argList != NULL);

    status = alljoyn_msgarg_set(argList, "(ybnqdiuxtsoqg)", y, b, n, q, d, i, u, x, t, s, o, q, g);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_get(argList, "(ybnqdiuxtsoqg)", &yout, &bout, &nout, &qout, &dout, &iout, &uout, &xout, &tout, &sout, &oout, &qout, &gout);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(yout, 0);
    EXPECT_EQ(bout, true);
    EXPECT_EQ(nout, 42);
    EXPECT_EQ(qout, 0xBEBE);
    EXPECT_EQ(iout, -9999);

    EXPECT_EQ(uout, 0x32323232u);
    EXPECT_EQ(xout, -1LL);
    EXPECT_EQ(tout, 0x6464646464646464ULL);
    ASSERT_STREQ(sout, "this is a string");
    ASSERT_STREQ(oout, "/org/foo/bar");
    EXPECT_EQ(qout, 0xBEBE);
    ASSERT_STREQ(gout, "a{is}d(siiux)");

    alljoyn_msgarg_destroy(argList);

    argList = alljoyn_msgarg_create();
    EXPECT_TRUE(argList != NULL);

    /*  Structs */
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_set(argList, "((ydx)(its))", y, d, x, i, t, s);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_get(argList, "((ydx)(its))", &yout, &dout, &xout, &iout, &tout, &sout);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_EQ(yout, 0);
    EXPECT_EQ(xout, -1LL);
    EXPECT_EQ(iout, -9999);
    EXPECT_EQ(tout, 0x6464646464646464ULL);
    EXPECT_STREQ(sout, "this is a string");

    alljoyn_msgarg_destroy(argList);

    argList = alljoyn_msgarg_create();
    EXPECT_TRUE(argList != NULL);

    status = alljoyn_msgarg_set(argList, "((iuiu)(yd)at)", i, u, i, u, y, d, sizeof(at) / sizeof(at[0]), at);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int64_t*p64;
    size_t p64len;
    status = alljoyn_msgarg_get(argList, "((iuiu)(yd)at)", &iout, &uout, &iout, &uout, &yout, &dout, &p64len, &p64);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(iout, -9999);
    EXPECT_EQ(uout, 0x32323232u);
    EXPECT_EQ(yout, 0);
    EXPECT_EQ(p64len, sizeof(at) / sizeof(at[0]));
    for (size_t k = 0; k < p64len; ++k) {
        EXPECT_EQ(at[k], p64[k]) << "index " << k;
    }
    alljoyn_msgarg_destroy(argList);
}

TEST(MsgArgTest, Variants)
{
    /* DOUBLE */
    static double d = 3.14159265L;
    /* STRING */
    static const char* s = "this is a string";

    int32_t i;
    double dt;
    char* str;

    QStatus status = ER_OK;
    alljoyn_msgarg arg = NULL;
    alljoyn_msgarg arg2 = NULL;
    arg = alljoyn_msgarg_create();

    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    arg2 = alljoyn_msgarg_create_and_set("i", 420);
    status = alljoyn_msgarg_set(arg, "v", arg2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_msgarg_get(arg, "u", &i);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg_destroy(arg2);

    arg2 = alljoyn_msgarg_create_and_set("d", &d);
    status = alljoyn_msgarg_set(arg, "v", arg2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_msgarg_get(arg, "i", &i);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_get(arg, "s", &str);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_get(arg, "d", &dt);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg_destroy(arg2);

    arg2 = alljoyn_msgarg_create_and_set("s", s);
    status = alljoyn_msgarg_set(arg, "v", arg2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_msgarg_get(arg, "i", &i);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_get(arg, "s", &str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_STREQ(s, str);
    alljoyn_msgarg_destroy(arg2);
    alljoyn_msgarg_destroy(arg);
}

TEST(MsgArgTest, arrays_of_scalars) {
    QStatus status = ER_OK;
    /* Array of BYTE */
    uint8_t ay[] = { 9, 19, 29, 39, 49 };
    /* Array of QCC_BOOL */
    static QCC_BOOL ab[] = { QCC_FALSE, QCC_FALSE, QCC_TRUE, QCC_TRUE, QCC_TRUE, QCC_FALSE };
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
        alljoyn_msgarg arg = alljoyn_msgarg_create();
        status = alljoyn_msgarg_set(arg, "ay", sizeof(ay) / sizeof(ay[0]), ay);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        uint8_t* pay;
        size_t lay;
        status = alljoyn_msgarg_get(arg, "ay", &lay, &pay);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(ay) / sizeof(ay[0]), lay);
        for (size_t i = 0; i < lay; ++i) {
            EXPECT_EQ(ay[i], pay[i]);
        }
        alljoyn_msgarg_destroy(arg);
    }
    {
        alljoyn_msgarg arg = alljoyn_msgarg_create();
        status = alljoyn_msgarg_set(arg, "ab", sizeof(ab) / sizeof(ab[0]), ab);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        QCC_BOOL* pab;
        size_t lab;
        status = alljoyn_msgarg_get(arg, "ab", &lab, &pab);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(ab) / sizeof(ab[0]), lab);
        for (size_t i = 0; i < lab; ++i) {
            EXPECT_EQ(ab[i], pab[i]) << "i = " << i << " ab[i] = " << ((ab[i]) ? "true" : "false")
                                     << " pab[i] = " << ((pab[i]) ? "true" : "false");
        }
        alljoyn_msgarg_destroy(arg);
    }
    {
        alljoyn_msgarg arg = alljoyn_msgarg_create();
        status = alljoyn_msgarg_set(arg, "an", sizeof(an) / sizeof(an[0]), an);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        int16_t* pan;
        size_t lan;
        status = alljoyn_msgarg_get(arg, "an", &lan, &pan);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(an) / sizeof(an[0]), lan);
        for (size_t i = 0; i < lan; ++i) {
            EXPECT_EQ(an[i], pan[i]);
        }
        alljoyn_msgarg_destroy(arg);
    }
    {
        alljoyn_msgarg arg = alljoyn_msgarg_create();
        status = alljoyn_msgarg_set(arg, "ai", sizeof(ai) / sizeof(ai[0]), ai);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        int32_t* pai;
        size_t lai;
        status = alljoyn_msgarg_get(arg, "ai", &lai, &pai);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(ai) / sizeof(ai[0]), lai);
        for (size_t i = 0; i < lai; ++i) {
            EXPECT_EQ(ai[i], pai[i]);
        }
        alljoyn_msgarg_destroy(arg);
    }
    {
        alljoyn_msgarg arg = alljoyn_msgarg_create();
        status = alljoyn_msgarg_set(arg, "ax", sizeof(ax) / sizeof(ax[0]), ax);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        int64_t* pax;
        size_t lax;
        status = alljoyn_msgarg_get(arg, "ax", &lax, &pax);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(ax) / sizeof(ax[0]), lax);
        for (size_t i = 0; i < lax; ++i) {
            EXPECT_EQ(ax[i], pax[i]);
        }
        alljoyn_msgarg_destroy(arg);
    }
    {
        alljoyn_msgarg arg = alljoyn_msgarg_create();
        status = alljoyn_msgarg_set(arg, "at", sizeof(at) / sizeof(at[0]), at);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        int64_t* pat;
        size_t lat;
        status = alljoyn_msgarg_get(arg, "at", &lat, &pat);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(at) / sizeof(at[0]), lat);
        for (size_t i = 0; i < lat; ++i) {
            EXPECT_EQ(at[i], pat[i]);
        }
        alljoyn_msgarg_destroy(arg);
    }
    {
        alljoyn_msgarg arg = alljoyn_msgarg_create();
        status = alljoyn_msgarg_set(arg, "ad", sizeof(ad) / sizeof(ad[0]), ad);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        double* pad;
        size_t lad;
        status = alljoyn_msgarg_get(arg, "ad", &lad, &pad);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(sizeof(ad) / sizeof(ad[0]), lad);
        for (size_t i = 0; i < lad; ++i) {
            EXPECT_EQ(ad[i], pad[i]);
        }
        alljoyn_msgarg_destroy(arg);
    }
}


TEST(MsgArgTest, arrays_of_nonscalars)
{
    QStatus status = ER_OK;

    /* Array of STRING */
    static const char*as[] = { "one", "two", "three", "four" };
    /* Array of OBJECT_PATH */
    static const char*ao[] = { "/org/one", "/org/two", "/org/three", "/org/four" };
    /* Array of SIGNATURE */
    static const char*ag[] = { "s", "sss", "as", "a(iiiiuu)" };

    alljoyn_msgarg arg;
    arg = alljoyn_msgarg_create();
    ASSERT_TRUE(arg != NULL);

    status = alljoyn_msgarg_set(arg, "as", sizeof(as) / sizeof(as[0]), as);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg pas;
    char*str[4];
    size_t las;
    status = alljoyn_msgarg_get(arg, "as", &las, &pas);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(sizeof(as) / sizeof(as[0]), las);
    ASSERT_TRUE(arg);
    ASSERT_TRUE(pas);
    for (size_t k = 0; k < las; k++) {
        ASSERT_TRUE(alljoyn_msgarg_array_element(pas, k));
        status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(pas, k), "s", &str[k]);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        ASSERT_TRUE(str[k]);
        EXPECT_STREQ(as[k], str[k]);
    }

    status = alljoyn_msgarg_set(arg, "ag", sizeof(ag) / sizeof(ag[0]), ag);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg pag;
    char* str_ag[4];
    size_t lag;
    status = alljoyn_msgarg_get(arg, "ag", &lag, &pag);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(sizeof(ag) / sizeof(ag[0]), lag);
    for (size_t k = 0; k < lag; k++) {
        status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(pag, k), "g", &str_ag[k]);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        ASSERT_TRUE(str_ag[k]);
        EXPECT_STREQ(ag[k], str_ag[k]);
    }

    status = alljoyn_msgarg_set(arg, "ao", sizeof(ao) / sizeof(ao[0]), ao);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    alljoyn_msgarg pao;
    char* str_ao[4];
    size_t lao;
    status = alljoyn_msgarg_get(arg, "ao", &lao, &pao);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(sizeof(ao) / sizeof(ao[0]), lao);
    for (size_t k = 0; k < lao; k++) {
        status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(pao, k), "o", &str_ao[k]);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        ASSERT_TRUE(str_ao[k]);
        EXPECT_STREQ(ao[k], str_ao[k]);
    }
    alljoyn_msgarg_destroy(arg);
}

TEST(MsgArgTest, Dictionary)
{
    QStatus status = ER_OK;
    const char*keys[] = { "red", "green", "blue", "yellow" };
    //size_t numEntries = sizeof(keys) / sizeof(keys[0]);
    alljoyn_msgarg dictEntries = NULL;
    alljoyn_msgarg values = NULL;
    dictEntries = alljoyn_msgarg_array_create(sizeof(keys) / sizeof(keys[0]));
    values = alljoyn_msgarg_array_create(sizeof(keys) / sizeof(keys[0]));
    alljoyn_msgarg_set(alljoyn_msgarg_array_element(values, 0), "s", keys[0]);
    alljoyn_msgarg_set(alljoyn_msgarg_array_element(values, 1), "(ss)", keys[1], "bean");
    alljoyn_msgarg_set(alljoyn_msgarg_array_element(values, 2), "s", keys[2]);
    alljoyn_msgarg_set(alljoyn_msgarg_array_element(values, 3), "(ss)", keys[3], "mellow");

    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(dictEntries, 0), "{iv}", 1, alljoyn_msgarg_array_element(values, 0));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(dictEntries, 1), "{iv}", 1, alljoyn_msgarg_array_element(values, 1));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(dictEntries, 2), "{iv}", 1, alljoyn_msgarg_array_element(values, 2));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(dictEntries, 3), "{iv}", 1, alljoyn_msgarg_array_element(values, 3));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg dict = alljoyn_msgarg_create();
    status = alljoyn_msgarg_set(dict, "a{iv}", sizeof(keys) / sizeof(keys[0]), dictEntries);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg entries;
    size_t num;
    status = alljoyn_msgarg_get(dict, "a{iv}", &num, &entries);
    EXPECT_EQ(num, sizeof(keys) / sizeof(keys[0]));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    for (size_t i = 0; i < num; ++i) {
        char* str1;
        char* str2;
        int32_t key;
        status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(entries, i), "{is}", &key, &str1);
        if (status == ER_BUS_SIGNATURE_MISMATCH) {
            status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(entries, i), "{i(ss)}", &key, &str1, &str2);
            EXPECT_EQ(1, key);
            EXPECT_STREQ(keys[i], str1);
            if (i == 1) {
                EXPECT_STREQ("bean", str2);
            } else if (i == 3) {
                EXPECT_STREQ("mellow", str2);
            }
        } else if (status == ER_OK) {
            EXPECT_EQ(1, key);
            EXPECT_STREQ(keys[i], str1);
        }
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }
    alljoyn_msgarg_destroy(dictEntries);
    alljoyn_msgarg_destroy(values);
    alljoyn_msgarg_destroy(dict);
}

TEST(MsgArgTest, alljoyn_msgarg_array_set_get) {
    QStatus status = ER_OK;
    alljoyn_msgarg arg;
    arg = alljoyn_msgarg_array_create(4);
    size_t numArgs = 4;
    status = alljoyn_msgarg_array_set(arg, &numArgs, "issi", 1, "two", "three", 4);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    int32_t argvalue1;
    char* argvalue2;
    char* argvalue3;
    int32_t argvalue4;
    status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(arg, 0), "i", &argvalue1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(1, argvalue1);
    status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(arg, 1), "s", &argvalue2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("two", argvalue2);
    status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(arg, 2), "s", &argvalue3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_STREQ("three", argvalue3);
    status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(arg, 3), "i", &argvalue4);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(4, argvalue4);

    int32_t out1;
    char* out2;
    char* out3;
    int32_t out4;
    status = alljoyn_msgarg_array_get(arg, 4, "issi", &out1, &out2, &out3, &out4);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(1, out1);
    EXPECT_STREQ("two", out2);
    EXPECT_STREQ("three", out3);
    EXPECT_EQ(4, out4);

    alljoyn_msgarg_destroy(arg);
}

/*
 * the tostring method is one of a few functions that has different behaver in
 * release build vs. debug build.
 * in release build tostring function will always return an empty string.
 * in debug build the tostring function will return an xml representation of the msgarg
 */
TEST(MsgArgTest, tostring) {
    QStatus status = ER_OK;
    alljoyn_msgarg arg;
    arg = alljoyn_msgarg_array_create(4);
    size_t numArgs = 4;
    status = alljoyn_msgarg_array_set(arg, &numArgs, "issi", 1, "two", "three", 4);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ((size_t)4,  numArgs);
#if NDEBUG
    size_t buf;
    char* str;
    buf = alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 0), NULL, 0, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 0), str, buf, 0);
    EXPECT_STREQ("", str);
    free(str);
    buf = alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 1), NULL, 0, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 1), str, buf, 0);
    EXPECT_STREQ("", str);
    free(str);
    buf = alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 2), NULL, 0, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 2), str, buf, 0);
    EXPECT_STREQ("", str);
    free(str);
    buf = alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 3), NULL, 0, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 3), str, buf, 0);
    EXPECT_STREQ("", str);
    free(str);
    buf = alljoyn_msgarg_array_tostring(arg, 4, NULL, 0, 0);
    char* val = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_array_tostring(arg, 4, val, buf, 0);
    EXPECT_STREQ("", val);
    free(val);
#else
    size_t buf;
    char* str;
    buf = alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 0), NULL, 0, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 0), str, buf, 0);
    EXPECT_STREQ("<int32>1</int32>", str);
    free(str);
    buf = alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 1), NULL, 0, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 1), str, buf, 0);
    EXPECT_STREQ("<string>two</string>", str);
    free(str);
    buf = alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 2), NULL, 0, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 2), str, buf, 0);
    EXPECT_STREQ("<string>three</string>", str);
    free(str);
    buf = alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 3), NULL, 0, 0);
    str = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_tostring(alljoyn_msgarg_array_element(arg, 3), str, buf, 0);
    EXPECT_STREQ("<int32>4</int32>", str);
    free(str);
    buf = alljoyn_msgarg_array_tostring(arg, 4, NULL, 0, 0);
    char* val = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_array_tostring(arg, 4, val, buf, 0);
    EXPECT_STREQ("<int32>1</int32>\n<string>two</string>\n<string>three</string>\n<int32>4</int32>\n", val);
    free(val);
#endif
    alljoyn_msgarg_destroy(arg);
}

TEST(MsgArgTest, signature) {
    alljoyn_msgarg arg1 = alljoyn_msgarg_create_and_set("i", 42);
    size_t buf = alljoyn_msgarg_signature(arg1, NULL, 0);
    char* val = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_signature(arg1, val, buf);
    EXPECT_STREQ("i", val);
    free(val);

    QStatus status = ER_OK;
    alljoyn_msgarg arg2;
    arg2 = alljoyn_msgarg_array_create(4);
    size_t numArgs = 4;
    status = alljoyn_msgarg_array_set(arg2, &numArgs, "issi", 1, "two", "three", 4);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    buf = alljoyn_msgarg_array_signature(arg2, 4, NULL, 0);
    val = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_array_signature(arg2, 4, val, buf);
    EXPECT_STREQ("issi", val);
    free(val);

    alljoyn_msgarg_destroy(arg1);
    alljoyn_msgarg_destroy(arg2);
}

TEST(MsgArgTest, hassignature) {
    alljoyn_msgarg arg = alljoyn_msgarg_create_and_set("i", 42);
    EXPECT_TRUE(alljoyn_msgarg_hassignature(arg, "i"));
    alljoyn_msgarg_destroy(arg);
    arg = alljoyn_msgarg_create_and_set("s", "whats 6 times 7");
    EXPECT_FALSE(alljoyn_msgarg_hassignature(arg, "i"));
    alljoyn_msgarg_destroy(arg);
}

TEST(MsgArgTest, equal) {
    alljoyn_msgarg arg1 = alljoyn_msgarg_create_and_set("i", 42);
    alljoyn_msgarg arg2 = alljoyn_msgarg_create_and_set("i", 42);

    // arg1 and arg2 should not have the same memory address.
    EXPECT_NE(arg1, arg2);
    EXPECT_TRUE(alljoyn_msgarg_equal(arg1, arg2));

    alljoyn_msgarg_destroy(arg1);
    alljoyn_msgarg_destroy(arg2);
}

TEST(MsgArgTest, copy) {
    alljoyn_msgarg arg1 = alljoyn_msgarg_create_and_set("s", "meaning of life");
    alljoyn_msgarg arg2 = alljoyn_msgarg_copy(arg1);
    // arg1 and arg2 should not have the same memory address.
    EXPECT_NE(arg1, arg2);
    EXPECT_TRUE(alljoyn_msgarg_equal(arg1, arg2));

    alljoyn_msgarg_destroy(arg1);
    alljoyn_msgarg_destroy(arg2);
}

TEST(MsgArgTest, getdictelement) {
    QStatus status = ER_OK;
    alljoyn_msgarg dictEntries;
    dictEntries = alljoyn_msgarg_array_create(3);

    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(dictEntries, 0), "{s(yus)}", "amy", 21, 151, "somewhere");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(dictEntries, 1), "{s(yus)}", "fred", 29, 212, "anywhere");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = alljoyn_msgarg_set(alljoyn_msgarg_array_element(dictEntries, 2), "{s(yus)}", "john", 33, 190, "nowhere");
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg dict = alljoyn_msgarg_create();
    status = alljoyn_msgarg_set(dict, "a{s(yus)}", 3, dictEntries);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    uint8_t age;
    uint32_t height;
    const char* address;
    status = alljoyn_msgarg_getdictelement(dict, "{s(yus)}", "fred", &age, &height,  &address);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_EQ(29, age);
    EXPECT_TRUE(212 == height);
    EXPECT_STREQ("anywhere", address);

    status = alljoyn_msgarg_getdictelement(dict, "{ss}", "fred", &age, &height,  &address);
    EXPECT_EQ(ER_BUS_SIGNATURE_MISMATCH, status) << "  Actual Status: " << QCC_StatusText(status);

    status = alljoyn_msgarg_getdictelement(dict, "{s(yus)}", "phil", &age, &height,  &address);
    EXPECT_EQ(ER_BUS_ELEMENT_NOT_FOUND, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg arg = alljoyn_msgarg_create_and_set("i", 42);
    status = alljoyn_msgarg_getdictelement(arg, "{s(yus)}", "fred", &age, &height,  &address);
    EXPECT_EQ(ER_BUS_NOT_A_DICTIONARY, status) << "  Actual Status: " << QCC_StatusText(status);

    alljoyn_msgarg_destroy(arg);
    alljoyn_msgarg_destroy(dictEntries);
    alljoyn_msgarg_destroy(dict);
}

TEST(MsgArgTest, clear_and_gettype) {
    alljoyn_msgarg arg = alljoyn_msgarg_create_and_set("i", 42);
    EXPECT_EQ(ALLJOYN_INT32, alljoyn_msgarg_gettype(arg));

    alljoyn_msgarg_clear(arg);
    EXPECT_EQ(ALLJOYN_INVALID, alljoyn_msgarg_gettype(arg));

    alljoyn_msgarg_destroy(arg);
}

TEST(MsgArgTest, stabilize) {
    QStatus status = ER_OK;
    alljoyn_msgarg arg;
    arg = alljoyn_msgarg_create();

    if (true) {
        char str_pointer[16] = "GoodBye";
        status = alljoyn_msgarg_set(arg, "s", str_pointer);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        alljoyn_msgarg_stabilize(arg);
        snprintf(str_pointer, 16, "%s", "stabilize");
    }
    /*
     * Since MsgArg::Stabilize was called on the MsgArg before the string
     * pointed to went out of scope the contents of str_pointer were copied
     * into MsgArg.  Using the Stabilize method creates a copy of anything
     * MsgArg is pointing to when it was called.
     */
    char* out_str;
    status = alljoyn_msgarg_get(arg, "s", &out_str);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_STREQ("GoodBye", out_str);

    alljoyn_msgarg_destroy(arg);
}

TEST(MsgArgTest, null_pointer_test) {
    alljoyn_msgarg arg = NULL;
    alljoyn_msgarg arg2 = NULL;
    alljoyn_msgarg arg_array = NULL;
    QStatus status = ER_OK;

    EXPECT_NO_FATAL_FAILURE(alljoyn_msgarg_destroy(arg));
    EXPECT_NO_FATAL_FAILURE(alljoyn_msgarg_destroy(arg_array));

    EXPECT_NO_FATAL_FAILURE(alljoyn_msgarg_array_element(arg_array, 1));

    EXPECT_NO_FATAL_FAILURE(status = alljoyn_msgarg_set(arg, "i", 42));
    EXPECT_EQ(ER_BAD_ARG_1, status) << "  Actual Status: " << QCC_StatusText(status);

    int32_t i;
    EXPECT_NO_FATAL_FAILURE(status = alljoyn_msgarg_get(arg, "i", &i));
    EXPECT_EQ(ER_BAD_ARG_1, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_TRUE(NULL == alljoyn_msgarg_copy(arg));

    EXPECT_FALSE(alljoyn_msgarg_equal(arg, arg2));

    size_t numArgs = 1;
    EXPECT_NO_FATAL_FAILURE(status = alljoyn_msgarg_array_set(arg_array, &numArgs, "i", 42));
    EXPECT_EQ(ER_BAD_ARG_1, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_NO_FATAL_FAILURE(status = alljoyn_msgarg_array_get(arg_array, 1, "i", &i));
    EXPECT_EQ(ER_BAD_ARG_1, status) << "  Actual Status: " << QCC_StatusText(status);

    EXPECT_EQ((size_t)0, alljoyn_msgarg_tostring(arg, NULL, 0, 0));
    size_t buf = alljoyn_msgarg_signature(arg, NULL, 0);
    char* val = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_signature(arg, val, buf);
    EXPECT_TRUE(0 == buf);
    free(val);

    buf = alljoyn_msgarg_array_signature(arg_array, numArgs, NULL, 0);
    val = (char*)malloc(sizeof(char) * buf);
    alljoyn_msgarg_array_signature(arg_array, numArgs, val, buf);
    EXPECT_TRUE(0 == buf);
    free(val);

    EXPECT_FALSE(alljoyn_msgarg_hassignature(arg, "i"));
    EXPECT_NO_FATAL_FAILURE(status = alljoyn_msgarg_getdictelement(arg, "{ii}", 1, &i));
    EXPECT_EQ(ER_BAD_ARG_1, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_NO_FATAL_FAILURE(alljoyn_msgarg_clear(arg));
    EXPECT_EQ(ALLJOYN_INVALID, alljoyn_msgarg_gettype(arg));
    EXPECT_NO_FATAL_FAILURE(alljoyn_msgarg_stabilize(arg));
}
