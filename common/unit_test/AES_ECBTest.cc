/**
 * @file
 *
 * This file tests AES-ECB mode.
 * The known answer tests are taken from http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf
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
#include <qcc/KeyBlob.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include <Status.h>

#include <gtest/gtest.h>

using namespace qcc;
using namespace std;

typedef struct {
    const char* key;     /* AES key */
    const char* input;   /* Input text */
    const char* output;  /* Encrypted output for verification */
} TEST_CASE;

static TEST_CASE testVector[] = {
    {
        "000102030405060708090a0b0c0d0e0f",
        "00112233445566778899aabbccddeeff",
        "69c4e0d86a7b0430d8cdb78070b4c55a"
    }
};

TEST(AES_ECBTest, AES_ECB_Test_Vector) {
    QStatus status = ER_OK;
    for (size_t i = 0; i < ArraySize(testVector); i++) {
        uint8_t key[16];
        uint8_t msg[16];

        HexStringToBytes(testVector[i].key, key, sizeof(key));
        HexStringToBytes(testVector[i].input, msg, sizeof(msg));

        KeyBlob kb(key, sizeof(key), KeyBlob::AES);
        Crypto_AES aes(kb, Crypto_AES::ECB_ENCRYPT);

        Crypto_AES::Block* out = new Crypto_AES::Block;
        status = aes.Encrypt(msg, sizeof(msg), out, 1);
        EXPECT_EQ(ER_OK, status) << "  Encryption error " << QCC_StatusText(status)
                                 << " for test #" << (i + 1);

        String output = BytesToHexString(out->data, 16, true);
        EXPECT_STREQ(testVector[i].output, output.c_str()) << "Encrypt verification failure for test #"
                                                           << (i + 1) << output.c_str();
        delete out;
    }
}
