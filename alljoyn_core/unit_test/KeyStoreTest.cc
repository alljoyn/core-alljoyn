
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

#include <vector>
#include <qcc/Debug.h>
#include <qcc/FileStream.h>
#include <qcc/KeyBlob.h>
#include <qcc/Pipe.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>
#include <qcc/GUID.h>
#include <qcc/time.h>
#include <qcc/Thread.h>

#include <alljoyn/version.h>
#include "KeyStore.h"

#include <alljoyn/Status.h>
#include "InMemoryKeyStore.h"

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
        qcc::Timespec expires(1000, qcc::TIME_RELATIVE);
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

class KeyStoreThread : public Thread {
  public:
    KeyStoreThread(String name, KeyStore& keyStore, vector<KeyStore::Key> workList, vector<KeyStore::Key> deleteList) : Thread(name), keyStore(keyStore), workList(workList), deleteList(deleteList)
    {
    }
    ~KeyStoreThread()
    {
        workList.clear();
        deleteList.clear();
    }
  protected:
    ThreadReturn STDCALL Run(void* arg) {
        QCC_UNUSED(arg);
        KeyBlob kb;
        kb.Set((const uint8_t*)testData, sizeof(testData), KeyBlob::GENERIC);
        size_t cnt = 0;
        for (vector<KeyStore::Key>::iterator it = workList.begin(); it != workList.end(); it++, cnt++) {
            EXPECT_EQ(ER_OK, keyStore.AddKey(*it, kb));
            /* random store call */
            if (cnt % 19 == 0) {
                EXPECT_EQ(ER_OK, keyStore.Store());
            }
        }
        for (vector<KeyStore::Key>::iterator it = deleteList.begin(); it != deleteList.end(); it++) {
            EXPECT_EQ(ER_OK, keyStore.DelKey(*it));
        }
        EXPECT_EQ(ER_OK, keyStore.Store());
        return static_cast<ThreadReturn>(0);
    }
  private:
    KeyStore& keyStore;
    vector<KeyStore::Key> workList;
    vector<KeyStore::Key> deleteList;
};

static bool VerifyExistence(KeyStore& keyStore, vector<KeyStore::Key>& workList, vector<KeyStore::Key>& deleteList, size_t& existenceCount, size_t& deletedCount)
{
    existenceCount = 0;
    deletedCount = 0;
    bool passed = true;
    for (vector<KeyStore::Key>::iterator it = workList.begin(); it != workList.end(); it++) {
        QStatus expectedStatus = ER_OK;
        for (vector<KeyStore::Key>::iterator delIt = deleteList.begin(); delIt != deleteList.end(); delIt++) {
            if (*it == *delIt) {
                expectedStatus = ER_BUS_KEY_UNAVAILABLE;
                break;
            }
        }
        KeyBlob kb;
        QStatus status = keyStore.GetKey((*it), kb);
        if (status != expectedStatus) {
            passed = false;
        } else if (expectedStatus == ER_BUS_KEY_UNAVAILABLE) {
            deletedCount++;
        } else {
            existenceCount++;
        }
    }
    return passed;
}

/**
 * Test two threads using one keystore instance.
 * @param useSharedKeyStore keyStore is marked shared or private
 */
static void TestConcurrentKeyStoreAccess(bool useSharedKeyStore)
{
    InMemoryKeyStoreListener keyStoreListener;
    KeyStore keyStore("shared_keystore");
    keyStore.SetListener(keyStoreListener);
    keyStore.Init(NULL, useSharedKeyStore);

    vector<KeyStore::Key> workList1;
    vector<KeyStore::Key> deleteList1;
    for (int cnt = 0; cnt < 100; cnt++) {
        qcc::GUID128 guid;
        KeyStore::Key key(KeyStore::Key::LOCAL, guid);
        workList1.push_back(key);
        if (cnt % 13 == 0) {
            deleteList1.push_back(key);
        }
    }
    vector<KeyStore::Key> workList2;
    vector<KeyStore::Key> deleteList2;
    for (int cnt = 0; cnt < 158; cnt++) {
        qcc::GUID128 guid;
        KeyStore::Key key(KeyStore::Key::LOCAL, guid);
        workList2.push_back(key);
        if (cnt % 37 == 0) {
            deleteList2.push_back(key);
        }
    }
    KeyStoreThread thread1("thread1", keyStore, workList1, deleteList1);
    KeyStoreThread thread2("thread2", keyStore, workList2, deleteList2);
    thread1.Start();
    thread2.Start();
    thread1.Join();
    thread2.Join();

    keyStore.Reload();

    size_t existenceCount = 0;
    size_t deletedCount = 0;
    /* check to make sure the expected keys are in the keystore */
    EXPECT_TRUE(VerifyExistence(keyStore, workList1, deleteList1, existenceCount, deletedCount));
    EXPECT_EQ(existenceCount, (workList1.size() - deleteList1.size()));
    EXPECT_EQ(deletedCount, deleteList1.size());

    existenceCount = 0;
    deletedCount = 0;
    EXPECT_TRUE(VerifyExistence(keyStore, workList2, deleteList2, existenceCount, deletedCount));
    EXPECT_EQ(existenceCount, (workList2.size() - deleteList2.size()));
    EXPECT_EQ(deletedCount, deleteList2.size());
}

TEST(KeyStoreTest, concurrent_access_shared_keystore)
{
    TestConcurrentKeyStoreAccess(true);
}

TEST(KeyStoreTest, concurrent_access_private_keystore)
{
    TestConcurrentKeyStoreAccess(false);
}


