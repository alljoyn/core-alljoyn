
/**
 * @file
 *
 * This file tests the keystore and ketyblob functionality
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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

using namespace qcc;
using namespace std;
using namespace ajn;

static const char testData[] = "This is the message that we are going to encrypt and then decrypt and verify";

int main(int argc, char** argv)
{
    qcc::GUID128 guid1;
    qcc::GUID128 guid2;
    qcc::GUID128 guid3;
    qcc::GUID128 guid4;
    QStatus status = ER_OK;
    KeyBlob key;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    Crypto_AES::Block* encrypted = new Crypto_AES::Block[Crypto_AES::NumBlocks(sizeof(testData))];

    printf("Testing basic key encryption/decryption\n");

    /* Encryption step */
    {
        FileSink sink("keystore_test");

        /*
         * Generate a random key
         */
        key.Rand(Crypto_AES::AES128_SIZE, KeyBlob::AES);
        printf("Key %d in  %s\n", key.GetType(), BytesToHexString(key.GetData(), key.GetSize()).c_str());
        /*
         * Encrypt our test string
         */
        Crypto_AES aes(key, Crypto_AES::ECB_ENCRYPT);
        status = aes.Encrypt(testData, sizeof(testData), encrypted, Crypto_AES::NumBlocks(sizeof(testData)));
        if (status != ER_OK) {
            printf("Encrypt failed\n");
            goto ErrorExit;
        }
        /*
         * Write the key to a stream
         */
        status = key.Store(sink);
        if (status != ER_OK) {
            printf("Failed to store key\n");
            goto ErrorExit;
        }
        /*
         * Set expiration and write again
         */
        qcc::Timespec expires(1000, qcc::TIME_RELATIVE);
        key.SetExpiration(expires);
        status = key.Store(sink);
        if (status != ER_OK) {
            printf("Failed to store key with expiration\n");
            goto ErrorExit;
        }
        /*
         * Set tag and write again
         */
        key.SetTag("My Favorite Key");
        status = key.Store(sink);
        if (status != ER_OK) {
            printf("Failed to store key with tag\n");
            goto ErrorExit;
        }

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
        if (status != ER_OK) {
            printf("Failed to load key\n");
            goto ErrorExit;
        }
        printf("Key %d out %s\n", inKey.GetType(), BytesToHexString(inKey.GetData(), inKey.GetSize()).c_str());
        /*
         * Decrypt and verify the test string
         */
        {
            char* out = new char[sizeof(testData)];
            Crypto_AES aes(inKey, Crypto_AES::ECB_DECRYPT);
            status = aes.Decrypt(encrypted, Crypto_AES::NumBlocks(sizeof(testData)), out, sizeof(testData));
            if (status != ER_OK) {
                printf("Encrypt failed\n");
                goto ErrorExit;
            }
            if (strcmp(out, testData) != 0) {
                printf("Encryt/decrypt of test data failed\n");
                goto ErrorExit;
            }
        }
        /*
         * Read the key with expiration
         */
        status = inKey.Load(source);
        if (status != ER_OK) {
            printf("Failed to load key with expiration\n");
            goto ErrorExit;
        }
        /*
         * Read the key with tag
         */
        status = inKey.Load(source);
        if (status != ER_OK) {
            printf("Failed to load key with tag\n");
            goto ErrorExit;
        }
        if (inKey.GetTag() != "My Favorite Key") {
            printf("Tag was incorrect\n");
            goto ErrorExit;
        }

        DeleteFile("keystore_test");
    }

    printf("Testing key store STORE\n");

    {
        KeyStore keyStore("keystore_test");

        keyStore.Init(NULL, true);
        keyStore.Clear();

        key.Rand(Crypto_AES::AES128_SIZE, KeyBlob::AES);
        keyStore.AddKey(guid1, key);
        key.Rand(620, KeyBlob::GENERIC);
        keyStore.AddKey(guid2, key);

        status = keyStore.Store();
        if (status != ER_OK) {
            printf("Failed to store keystore\n");
            goto ErrorExit;
        }
    }

    printf("Testing key store LOAD\n");

    {
        KeyStore keyStore("keystore_test");
        keyStore.Init(NULL, true);

        status = keyStore.GetKey(guid1, key);
        if (status != ER_OK) {
            printf("Failed to load guid1\n");
            goto ErrorExit;
        }
        status = keyStore.GetKey(guid2, key);
        if (status != ER_OK) {
            printf("Failed to load guid2\n");
            goto ErrorExit;
        }
    }

    printf("Testing key store MERGE\n");
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
            if (status != ER_OK) {
                printf("Failed to store keystore %s\n", QCC_StatusText(status));
                goto ErrorExit;
            }
        }

        status = keyStore.Reload();
        if (status != ER_OK) {
            printf("Failed to reload keystore %s\n", QCC_StatusText(status));
            goto ErrorExit;
        }

        status = keyStore.GetKey(guid1, key);
        if (status != ER_OK) {
            printf("Failed to load guid1\n");
            goto ErrorExit;
        }
        status = keyStore.GetKey(guid2, key);
        if (status == ER_OK) {
            printf("guid2 was not deleted\n");
            goto ErrorExit;
        }
        status = keyStore.GetKey(guid3, key);
        if (status != ER_OK) {
            printf("Failed to load guid3\n");
            goto ErrorExit;
        }
        status = keyStore.GetKey(guid4, key);
        if (status != ER_OK) {
            printf("Failed to load guid4\n");
            goto ErrorExit;
        }

        /* Store merged key store */
        status = keyStore.Store();
        if (status != ER_OK) {
            printf("Failed to store keystore\n");
            goto ErrorExit;
        }

    }

    printf("keystore unit test PASSED\n");
    return 0;

ErrorExit:

    printf("keystore unit test FAILED %s\n", QCC_StatusText(status));

    return -1;
}
