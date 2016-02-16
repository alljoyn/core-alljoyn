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

#include <gtest/gtest.h>

#include <stdio.h>

#include <AJNCa.h>

using namespace std;
using namespace ajn;
using namespace qcc;
using namespace securitymgr;

/** @file AJNCaTests.cc */

namespace secmgr_tests {
/**
 * @test Basic tests for the sample implementation of a CA based on AllJoyn.
 *       -# Initialize an AJNCA instance.
 *       -# Retrieve its public and private key.
 *       -# Create another instance with the same storeName.
 *       -# Check whether its private and public key are the same as the
 *          original AJNCA store.
 *       -# Creating an AJNCA with an empty storeName should fail.
 *       -# Reset the AJNCA store.
 *       -# Resetting an AJNCA twice should fail.
 **/
TEST(AJNCaTest, BasicTest) {
    ECCPublicKey epk;
    ECCPrivateKey eprk;
    {
        AJNCa ca;
        ASSERT_EQ(ER_OK, ca.Init("AJNCaTest"));
        ASSERT_EQ(ER_OK, ca.GetDSAPublicKey(epk));
        ASSERT_EQ(ER_OK, ca.GetDSAPrivateKey(eprk));
        ASSERT_FALSE(epk.empty());
    }
    {
        ECCPublicKey epk2;
        ECCPrivateKey eprk2;
        AJNCa ca;
        ASSERT_EQ(ER_OK, ca.Init("AJNCaTest"));
        ASSERT_EQ(ER_OK, ca.GetDSAPublicKey(epk2));
        ASSERT_EQ(ER_OK, ca.GetDSAPrivateKey(eprk2));
        ASSERT_TRUE(epk == epk2);
        ASSERT_TRUE(eprk == eprk2);
        AJNCa ca2;
        ASSERT_NE(ER_OK, ca2.Init(""));
        ASSERT_EQ(ER_OK, ca.Reset());
    }
    ECCPublicKey epk3;
    ECCPrivateKey eprk3;
    AJNCa ca;
    ASSERT_EQ(ER_OK, ca.Init("AJNCaTest"));
    ASSERT_EQ(ER_OK, ca.GetDSAPublicKey(epk3));
    ASSERT_EQ(ER_OK, ca.GetDSAPrivateKey(eprk3));
    ASSERT_FALSE(epk == epk3);
    ASSERT_FALSE(eprk == eprk3);
    ASSERT_EQ(ER_OK, ca.Reset());
    ASSERT_EQ(ER_FAIL, ca.Reset());
}
}
