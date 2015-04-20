/**
 * @file
 *
 * This file tests SHA1, HMAC-SHA1, SHA256, HMAC-SHA256.
 * The known answer tests are taken from RFC 4634.
 */

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

#include <qcc/platform.h>

#include <qcc/Crypto.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include <Status.h>

#include <gtest/gtest.h>

using namespace qcc;
using namespace std;

typedef struct {
    const char* key;     /* input key (for hmac) - hex string */
    const char* msg;     /* input message - ascii */
    const char* dig;     /* output digest - hex string */
} TEST_CASE;

static TEST_CASE sha1test[] = {
    {
        "",
        "abc",
        "A9993E364706816ABA3E25717850C26C9CD0D89D"
    },
    {
        "",
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
        "84983E441C3BD26EBAAE4AA1F95129E5E54670F1"
    },
    {
        "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
        "Hi There",
        "B617318655057264E28BC0B6FB378C8EF146BE00"
    },
    {
        "4a656665",
        "what do ya want for nothing?",
        "EFFCDF6AE5EB2FA2D27416D5F184DF9C259A7C79"
    },
    {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "Test Using Larger Than Block-Size Key - "
        "Hash Key First",
        "AA4AE5E15272D00E95705637CE8A3B55ED402112"
    },
    {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "Test Using Larger Than Block-Size Key an"
        "d Larger Than One Block-Size Data",
        "E8E99D0F45237D786D6BBAA7965C7808BBFF1A91"
    },
};

static TEST_CASE sha256test[] = {
    {
        "",
        "abc",
        "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD"
    },
    {
        "",
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
        "248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1"
    },
    {
        "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
        "Hi There",
        "B0344C61D8DB38535CA8AFCEAF0BF12B881DC200C9833DA726E9376C2E32CFF7"
    },
    {
        "4a656665",
        "what do ya want for nothing?",
        "5BDCC146BF60754E6A042426089575C75A003F089D2739839DEC58B964EC3843"
    },
    {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaa",
        "Test Using Larger Than Block-Size Key - Hash Key First",
        "60E431591EE0B67F0D8A26AACBF5B77F8E0BC6213728C5140546040F0EE37F54"
    },
    {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaa",
        "This is a test using a larger than block-size key and a larger t"
        "han block-size data. The key needs to be hashed before being use"
        "d by the HMAC algorithm.",
        "9B09FFA71B942FCB27635FBCD5B0E944BFDC63644F0713938A7F51535C3A35E2"
    },
};

TEST(SHA1_Test, SHA1_Test_Vector) {
    QStatus status = ER_OK;
    Crypto_SHA1 hash;
    const char* key;
    const char* msg;
    const char* dig;
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
    uint8_t byt[4096];
    size_t len;
    for (size_t i = 0; i < ArraySize(sha1test); i++) {
        key = sha1test[i].key;
        msg = sha1test[i].msg;
        dig = sha1test[i].dig;
        if (strlen(key)) {
            len = HexStringToBytes(key, byt, sizeof (byt));
            status = hash.Init(byt, len);
        } else {
            status = hash.Init(NULL, 0);
        }
        EXPECT_EQ(ER_OK, status) << "  Init error " << QCC_StatusText(status);
        status = hash.Update((const uint8_t*) msg, strlen(msg));
        EXPECT_EQ(ER_OK, status) << "  Update error " << QCC_StatusText(status);
        status = hash.GetDigest(digest);
        EXPECT_EQ(ER_OK, status) << "  Digest error " << QCC_StatusText(status);
        String hex = BytesToHexString(digest, sizeof (digest), false);
        EXPECT_STREQ(dig, hex.c_str());
    }
}

TEST(SHA256_Test, SHA256_Test_Vector) {
    QStatus status = ER_OK;
    Crypto_SHA256 hash;
    const char* key;
    const char* msg;
    const char* dig;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    uint8_t byt[1024];
    size_t len;
    for (size_t i = 0; i < ArraySize(sha256test); i++) {
        key = sha256test[i].key;
        msg = sha256test[i].msg;
        dig = sha256test[i].dig;
        if (strlen(key)) {
            len = HexStringToBytes(key, byt, sizeof (byt));
            status = hash.Init(byt, len);
        } else {
            status = hash.Init(NULL, 0);
        }
        EXPECT_EQ(ER_OK, status) << "  Init error " << QCC_StatusText(status);
        status = hash.Update((const uint8_t*) msg, strlen(msg));
        EXPECT_EQ(ER_OK, status) << "  Update error " << QCC_StatusText(status);
        status = hash.GetDigest(digest);
        EXPECT_EQ(ER_OK, status) << "  Digest error " << QCC_StatusText(status);
        String hex = BytesToHexString(digest, sizeof (digest), false);
        EXPECT_STREQ(dig, hex.c_str());
    }
}
