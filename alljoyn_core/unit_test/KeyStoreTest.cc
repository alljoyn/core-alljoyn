
/**
 * @file
 *
 * This file tests the keystore and keyblob functionality
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2010-2011, 2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/FileStream.h>
#include <qcc/KeyBlob.h>
#include <qcc/Pipe.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>
#include <qcc/GUID.h>
#include <qcc/time.h>

#include <alljoyn/version.h>
#include "KeyStore.h"

#include <alljoyn/Status.h>

#include <gtest/gtest.h>

using namespace qcc;
using namespace std;
using namespace ajn;

static const char testData[] = "This is the message that we are going to encrypt and then decrypt and verify";



TEST(KeyStoreTest, basic_encryption_decryption) {
    QStatus status = ER_OK;
    KeyBlob key;

    Crypto_AES::Block* encrypted = new Crypto_AES::Block[Crypto_AES::NumBlocks(sizeof(testData))];
    char* out = NULL;
    /*
     *  Testing basic key encryption/decryption
     */
    /* Encryption step */
    {
        FileSink sink("keystore_test");

        /*
         * Generate a random key
         */
        key.Rand(Crypto_AES::AES128_SIZE, KeyBlob::AES);
        //printf("Key %d in  %s\n", key.GetType(), BytesToHexString(key.GetData(), key.GetSize()).c_str());
        /*
         * Encrypt our test string
         */
        Crypto_AES aes(key, Crypto_AES::ECB_ENCRYPT);
        status = aes.Encrypt(testData, sizeof(testData), encrypted, Crypto_AES::NumBlocks(sizeof(testData)));
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Encrypt failed";

        /*
         * Write the key to a stream
         */
        status = key.Store(sink);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to store key";

        /*
         * Set expiration and write again
         */
        qcc::Timespec expires(1000, qcc::TIME_RELATIVE);
        key.SetExpiration(expires);
        status = key.Store(sink);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to store key with expiration";

        /*
         * Set tag and write again
         */
        key.SetTag("My Favorite Key");
        status = key.Store(sink);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to store key with tag";

        key.Erase();
    }

    /* Decryption step */
    {
        FileSource source("keystore_test");

        /*
         * Read the key from a stream
         */
        KeyBlob inKey;
        status = inKey.Load(source);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to load key";

        //printf("Key %d out %s\n", inKey.GetType(), BytesToHexString(inKey.GetData(), inKey.GetSize()).c_str());
        /*
         * Decrypt and verify the test string
         */
        {
            out = new char[sizeof(testData)];
            Crypto_AES aes(inKey, Crypto_AES::ECB_DECRYPT);
            status = aes.Decrypt(encrypted, Crypto_AES::NumBlocks(sizeof(testData)), out, sizeof(testData));
            ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Encrypt failed";

            ASSERT_STREQ(testData, out) << "Encryt/decrypt of test data failed";
            delete [] out;
            out = NULL;
        }
        /*
         * Read the key with expiration
         */
        status = inKey.Load(source);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to load key with expiration";

        /*
         * Read the key with tag
         */
        status = inKey.Load(source);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to load key with tag";

        ASSERT_STREQ("My Favorite Key", inKey.GetTag().c_str()) << "Tag was incorrect";
    }
    DeleteFile("keystore_test");
    delete [] encrypted;
}

TEST(KeyStoreTest, keystore_store_load_merge) {
    qcc::GUID128 guid1;
    qcc::GUID128 guid2;
    qcc::GUID128 guid3;
    qcc::GUID128 guid4;
    QStatus status = ER_OK;
    KeyBlob key;

    /*
     * Testing key store STORE
     */
    {
        KeyStore keyStore("keystore_test");

        keyStore.Init(NULL, true);
        keyStore.Clear();

        key.Rand(Crypto_AES::AES128_SIZE, KeyBlob::AES);
        keyStore.AddKey(guid1, key);
        key.Rand(620, KeyBlob::GENERIC);
        keyStore.AddKey(guid2, key);

        status = keyStore.Store();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to store keystore";
    }

    /*
     * Testing key store LOAD
     */
    {
        KeyStore keyStore("keystore_test");
        keyStore.Init(NULL, true);

        status = keyStore.GetKey(guid1, key);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to load guid1";

        status = keyStore.GetKey(guid2, key);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to load guid2";
    }

    /*
     * Testing key store MERGE
     */
    {
        KeyStore keyStore("keystore_test");
        keyStore.Init(NULL, true);

        key.Rand(Crypto_AES::AES128_SIZE, KeyBlob::AES);
        keyStore.AddKey(guid4, key);

        {
            KeyStore keyStore("keystore_test");
            keyStore.Init(NULL, true);

            /* Replace a key */
            key.Rand(Crypto_AES::AES128_SIZE, KeyBlob::AES);
            keyStore.AddKey(guid1, key);

            /* Add a key */
            key.Rand(Crypto_AES::AES128_SIZE, KeyBlob::AES);
            keyStore.AddKey(guid3, key);

            /* Delete a key */
            keyStore.DelKey(guid2);

            status = keyStore.Store();
            ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to store keystore";
        }

        status = keyStore.Reload();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to reload keystore";

        status = keyStore.GetKey(guid1, key);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to load guid1";

        status = keyStore.GetKey(guid2, key);
        ASSERT_EQ(ER_BUS_KEY_UNAVAILABLE, status) << "  Actual Status: " << QCC_StatusText(status) << " guid2 was not deleted";

        status = keyStore.GetKey(guid3, key);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to load guid3";

        status = keyStore.GetKey(guid4, key);
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to load guid4";

        /* Store merged key store */
        status = keyStore.Store();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status) << " Failed to store keystore";
    }
    DeleteFile("keystore_test");
}

