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
#include <qcc/KeyInfoECC.h>

using namespace qcc;

TEST(KeyInfoECCTest, constructor) {
    KeyInfoECC keyInfoECC;
    EXPECT_EQ(KeyInfo::FORMAT_ALLJOYN, keyInfoECC.GetFormat());
    EXPECT_EQ((uint8_t)Crypto_ECC::ECC_NIST_P256, keyInfoECC.GetCurve());
    EXPECT_EQ((uint8_t)SigInfo::ALGORITHM_ECDSA_SHA_256, keyInfoECC.GetAlgorithm());
}

TEST(KeyInfoECCTest, export_import) {
    KeyInfoECC keyInfoECC;

    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);

    size_t exportSize = keyInfoECC.GetExportSize();
    // export size is KeyInfo exportSize + byte for the curve type
    EXPECT_EQ(static_cast<KeyInfo>(keyInfoECC).GetExportSize() + 1 * sizeof(uint8_t), exportSize);

    uint8_t* buf = new uint8_t[exportSize];
    EXPECT_EQ(ER_OK, keyInfoECC.Export(buf));

    KeyInfoECC importedKey;
    EXPECT_EQ(ER_OK, importedKey.Import(buf, exportSize));
    EXPECT_EQ(keyInfoECC.GetExportSize(), importedKey.GetExportSize());
    EXPECT_EQ(keyInfoECC.GetFormat(), importedKey.GetFormat());
    EXPECT_TRUE(keyInfoECC == importedKey);
    delete [] buf;
}

//Verify the compiler generated copy/assignment operators work as expected
TEST(KeyInfoECCTest, copy_assign)
{
    KeyInfoECC keyInfoECC;

    uint8_t dummyKeyId[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    keyInfoECC.SetKeyId(dummyKeyId, 10);

    KeyInfoECC copyKeyInfo(keyInfoECC);
    KeyInfoECC assignedKeyInfo = keyInfoECC;

    EXPECT_TRUE(keyInfoECC == copyKeyInfo);
    EXPECT_TRUE(keyInfoECC == assignedKeyInfo);
    EXPECT_TRUE(copyKeyInfo == assignedKeyInfo);
}

TEST(KeyInfoNISTP256Test, constructor) {
    KeyInfoNISTP256 keyInfoNISTP256;
    EXPECT_EQ(KeyInfo::FORMAT_ALLJOYN, keyInfoNISTP256.GetFormat());
    EXPECT_EQ((uint8_t)Crypto_ECC::ECC_NIST_P256, keyInfoNISTP256.GetCurve());
    EXPECT_EQ((uint8_t)SigInfo::ALGORITHM_ECDSA_SHA_256, keyInfoNISTP256.GetAlgorithm());
}


