
/**
 * @file
 *
 * This file tests the keystore and keyblob functionality
 */

/******************************************************************************
 *
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

#include <qcc/platform.h>

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
#include "ajTestCommon.h"

using namespace qcc;
using namespace std;
using namespace ajn;

static const char testData[] = "This is the message that we are going to store and then load and verify";



TEST(KeyStoreTest, basic_store_load) {
    QStatus status = ER_OK;
    KeyBlob key;

    /*
     *  Testing basic key encryption/decryption
     */
    /* Store step */
    {
        FileSink sink("keystore_test");

        key.Set((const uint8_t*)testData, sizeof(testData), KeyBlob::GENERIC);
        //printf("Key %d in  %s\n", key.GetType(), BytesToHexString(key.GetData(), key.GetSize()).c_str());

        /*
         * Write the key to a stream
         */
        status = key.Store(sink);
        ASSERT_EQ(ER_OK, status) << " Failed to store key";

        /*
         * Set expiration and write again
         */
        qcc::Timespec<qcc::EpochTime> expires(GetEpochTimestamp() + 1000);
        key.SetExpiration(expires);
        status = key.Store(sink);
        ASSERT_EQ(ER_OK, status) << " Failed to store key with expiration";

        /*
         * Set tag and write again
         */
        key.SetTag("My Favorite Key");
        status = key.Store(sink);
        ASSERT_EQ(ER_OK, status) << " Failed to store key with tag";

        key.Erase();
    }

    /* Load step */
    {
        FileSource source("keystore_test");

        /*
         * Read the key from a stream
         */
        KeyBlob inKey;
        status = inKey.Load(source);
        ASSERT_EQ(ER_OK, status) << " Failed to load key";

        //printf("Key %d out %s\n", inKey.GetType(), BytesToHexString(inKey.GetData(), inKey.GetSize()).c_str());

        /*
         * Read the key with expiration
         */
        status = inKey.Load(source);
        ASSERT_EQ(ER_OK, status) << " Failed to load key with expiration";

        /*
         * Read the key with tag
         */
        status = inKey.Load(source);
        ASSERT_EQ(ER_OK, status) << " Failed to load key with tag";

        ASSERT_STREQ("My Favorite Key", inKey.GetTag().c_str()) << "Tag was incorrect";

        ASSERT_STREQ(testData, (const char*)inKey.GetData()) << "Key data was incorrect";
    }
    DeleteFile("keystore_test");
}

TEST(KeyStoreTest, keystore_store_load_merge) {
    qcc::GUID128 guid1;
    qcc::GUID128 guid2;
    qcc::GUID128 guid3;
    qcc::GUID128 guid4;
    KeyStore::Key idx1(KeyStore::Key::LOCAL, guid1);
    KeyStore::Key idx2(KeyStore::Key::LOCAL, guid2);
    KeyStore::Key idx3(KeyStore::Key::LOCAL, guid3);
    KeyStore::Key idx4(KeyStore::Key::LOCAL, guid4);
    QStatus status = ER_OK;
    KeyBlob key;

    /*
     * Testing key store STORE
     */
    {
        KeyStore keyStore("keystore_test");

        keyStore.Init(NULL, true);
        keyStore.Clear();

        key.Rand(620, KeyBlob::GENERIC);
        keyStore.AddKey(idx1, key);
        key.Rand(620, KeyBlob::GENERIC);
        keyStore.AddKey(idx2, key);

        status = keyStore.Store();
        ASSERT_EQ(ER_OK, status) << " Failed to store keystore";
    }

    /*
     * Testing key store LOAD
     */
    {
        KeyStore keyStore("keystore_test");
        keyStore.Init(NULL, true);

        status = keyStore.GetKey(idx1, key);
        ASSERT_EQ(ER_OK, status) << " Failed to load key1";

        status = keyStore.GetKey(idx2, key);
        ASSERT_EQ(ER_OK, status) << " Failed to load key2";
    }

    /*
     * Testing key store MERGE
     */
    {
        KeyStore keyStore("keystore_test");
        keyStore.Init(NULL, true);

        key.Rand(620, KeyBlob::GENERIC);
        keyStore.AddKey(idx4, key);

        {
            KeyStore keyStore2("keystore_test");
            keyStore2.Init(NULL, true);

            /* Replace a key */
            key.Rand(620, KeyBlob::GENERIC);
            keyStore2.AddKey(idx1, key);

            /* Add a key */
            key.Rand(620, KeyBlob::GENERIC);
            keyStore2.AddKey(idx3, key);

            /* Delete a key */
            keyStore2.DelKey(idx2);

            status = keyStore2.Store();
            ASSERT_EQ(ER_OK, status) << " Failed to store keystore";
        }

        status = keyStore.Reload();
        ASSERT_EQ(ER_OK, status) << " Failed to reload keystore";

        status = keyStore.GetKey(idx1, key);
        ASSERT_EQ(ER_OK, status) << " Failed to load idx1";

        status = keyStore.GetKey(idx2, key);
        ASSERT_EQ(ER_BUS_KEY_UNAVAILABLE, status) << " idx2 was not deleted";

        status = keyStore.GetKey(idx3, key);
        ASSERT_EQ(ER_OK, status) << " Failed to load idx3";

        status = keyStore.GetKey(idx4, key);
        ASSERT_EQ(ER_OK, status) << " Failed to load idx4";

        /* Store merged key store */
        status = keyStore.Store();
        ASSERT_EQ(ER_OK, status) << " Failed to store keystore";
    }
    DeleteFile("keystore_test");
}

