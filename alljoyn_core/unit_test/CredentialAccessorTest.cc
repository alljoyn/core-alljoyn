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

#include <iostream>
#include <qcc/time.h>
#include <qcc/GUID.h>
#include <qcc/KeyBlob.h>
#include <qcc/Thread.h>
#include <qcc/Crypto.h>

#include "ajTestCommon.h"
#include "ServiceSetup.h"

#include "CredentialAccessor.h"
#include "InMemoryKeyStore.h"

using namespace qcc;
using namespace std;

class LocalAuthListener : public AuthListener {
    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {
        QCC_UNUSED(authMechanism);
        QCC_UNUSED(authPeer);
        QCC_UNUSED(authCount);
        QCC_UNUSED(userId);
        QCC_UNUSED(credMask);
        QCC_UNUSED(creds);
        return true;
    }
    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        QCC_UNUSED(authMechanism);
        QCC_UNUSED(authPeer);
        QCC_UNUSED(success);
    }
};

class CredentialAccessorTest : public testing::Test {
  public:

    CredentialAccessorTest() : testing::Test(), ksListener(NULL)
    {
    }
    CredentialAccessorTest(qcc::String keyStoreSource, qcc::String keyStorePwd) : testing::Test(), ksListener(NULL), keyStoreSource(keyStoreSource), keyStorePwd(keyStorePwd)
    {
    }

    BusAttachment* g_msgBus;
    LocalAuthListener myListener;

    virtual void SetUp() {
        g_msgBus = new BusAttachment("testservices", true);
        QStatus status = g_msgBus->Start();
        ASSERT_EQ(ER_OK, status);
        if (!g_msgBus->IsConnected()) {
            /* Connect to the daemon and wait for the bus to exit */
            status = g_msgBus->Connect(ajn::getConnectArg().c_str());
            ASSERT_EQ(ER_OK, status);
        }
        g_msgBus->ClearKeyStore();
        if (keyStoreSource.empty()) {
            ksListener = new InMemoryKeyStoreListener();
        } else {
            qcc::String decodedSource;
            EXPECT_EQ(ER_OK, Crypto_ASN1::DecodeBase64(keyStoreSource, decodedSource)) << " Error base64 decoding of the key store source string";
            qcc::String decodedPwd;
            EXPECT_EQ(ER_OK, Crypto_ASN1::DecodeBase64(keyStorePwd, decodedPwd)) << " Error base64 decoding of the key store source password";
            ksListener = new InMemoryKeyStoreListener(decodedSource, decodedPwd);
        }
        EXPECT_EQ(ER_OK, g_msgBus->RegisterKeyStoreListener(*ksListener)) << " Fail to register key store listener";
        status = g_msgBus->EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &myListener, "CredentialAccessorTest");
        ASSERT_EQ(ER_OK, status);
    }

    virtual void TearDown() {
        if (g_msgBus) {
            BusAttachment* deleteMe = g_msgBus;
            g_msgBus = NULL;
            delete deleteMe;
        }
    }

    virtual ~CredentialAccessorTest()
    {
        delete ksListener;
    }

  private:
    KeyStoreListener* ksListener;
    qcc::String keyStoreSource;
    qcc::String keyStorePwd;
};

TEST_F(CredentialAccessorTest, StoreCustomKey)
{

    CredentialAccessor ca(*g_msgBus);

    KeyBlob kb("This is the peer secret", KeyBlob::GENERIC);

    kb.SetExpiration(100);

    GUID128 peerGuid;
    KeyStore::Key peerKey(KeyStore::Key::REMOTE, peerGuid);

    QStatus status = ca.StoreKey(peerKey, kb);
    ASSERT_EQ(ER_OK, status) << " ca.StoreKey failed";

    const char* blobName = "This is the custom key 1";
    KeyBlob customKb1((const uint8_t*) blobName, strlen(blobName), KeyBlob::GENERIC);

    printf("customKb1 has blob type %d\n", customKb1.GetType());
    GUID128 customGuid1;
    KeyStore::Key customKey1(KeyStore::Key::REMOTE, customGuid1);
    status = ca.AddAssociatedKey(peerKey, customKey1, customKb1);

    ASSERT_EQ(ER_OK, status) << " ca.AddAssociatedKey failed";

    GUID128 customGuid2;
    KeyStore::Key customKey2(KeyStore::Key::REMOTE, customGuid2);
    qcc::String customName2("Blob for custom key 2");
    KeyBlob customKb2(customName2, KeyBlob::GENERIC);
    status = ca.AddAssociatedKey(peerKey, customKey2, customKb2);

    ASSERT_EQ(ER_OK, status) << " ca.AddAssociatedKey failed";

    /* now retrieve the list back */
    KeyStore::Key* customKeyList = NULL;
    size_t numKeys = 0;
    status = ca.GetKeys(peerKey, &customKeyList, &numKeys);
    if (status != ER_OK) {
        delete [] customKeyList;
        FAIL() << " ca.GetKeys failed";
        return;
    }

    KeyStore::Key twoKeys[2];
    for (size_t cnt = 0; cnt < numKeys; cnt++) {
        twoKeys[cnt] = customKeyList[cnt];
    }
    delete [] customKeyList;
    ASSERT_EQ(2U, numKeys) << " ca.GetKeys expected to return 2 keys";

    for (size_t cnt = 0; cnt < numKeys; cnt++) {
        cout << "Custom key: " << twoKeys[cnt].ToString().c_str()  << endl;
        ASSERT_TRUE((twoKeys[cnt] == customKey1) || (twoKeys[cnt] == customKey2)) << " custom key does not match any of the originals";

        KeyBlob readBackKb;
        status = ca.GetKey(twoKeys[cnt], readBackKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey failed";

        ASSERT_TRUE((memcmp(customKb1.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) || (memcmp(customKb2.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0)) << " the read back KB does not match original";
    }

    /* now delete customGuid1 */
    status = ca.DeleteKey(customKey1);

    ASSERT_EQ(ER_OK, status) << " ca.DeleteKey failed";

    /* retrieve the custom list back */
    /* now retrieve the list back */
    numKeys = 0;
    customKeyList = NULL;
    status = ca.GetKeys(peerKey, &customKeyList, &numKeys);
    if (status != ER_OK) {
        delete [] customKeyList;
        FAIL() << " ca.GetKeys failed";
        return;
    }

    if (numKeys != 1) {
        delete [] customKeyList;
        FAIL() << " ca.GetKeys expected to return 1 key";
        return;
    }

    KeyStore::Key oneCustomKey = customKeyList[0];
    delete [] customKeyList;
    cout << "Custom key: " << oneCustomKey.ToString().c_str()  << endl;
    ASSERT_TRUE(oneCustomKey == customKey2) << " custom key does not match any of the originals";

    KeyBlob readBackKb;
    status = ca.GetKey(oneCustomKey, readBackKb);
    ASSERT_EQ(ER_OK, status) << " ca.GetKey failed";

    ASSERT_TRUE(memcmp(customKb2.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) << " the read back KB does not match original";

    /* delete the header key */
    status = ca.DeleteKey(peerKey);
    ASSERT_EQ(ER_OK, status) << " ca.DeleteKey on header key failed";

    KeyBlob tmpKb;
    status = ca.GetKey(peerKey, tmpKb);
    ASSERT_NE(ER_OK, status) << " ca.GetKey on peerKey expected to fail";

    status = ca.GetKey(customKey2, tmpKb);
    ASSERT_NE(ER_OK, status) << " ca.GetKey on customKey2 expected to fail";
}


QStatus HeaderKeyExpired_Pre(CredentialAccessor& ca, KeyStore::Key& peerKey, KeyStore::Key& customKey1, KeyStore::Key& customKey2)
{

    QStatus status = ER_FAIL;
    KeyBlob kb("This is the peer secret", KeyBlob::GENERIC);
    kb.SetExpiration(30); /* the minimum is 30 */

    status = ca.StoreKey(peerKey, kb);
    if (status != ER_OK) {
        return status;
    }
    Timespec expiry;
    kb.GetExpiration(expiry);
    const char* blobName = "This is the custom key 1";
    KeyBlob customKb1((const uint8_t*) blobName, strlen(blobName), KeyBlob::GENERIC);

    status = ca.AddAssociatedKey(peerKey, customKey1, customKb1);
    if (status != ER_OK) {
        return status;
    }

    qcc::String customName2("Blob for custom key 2");
    KeyBlob customKb2(customName2, KeyBlob::GENERIC);
    return ca.AddAssociatedKey(peerKey, customKey2, customKb2);
}

QStatus MemberKeyExpired_Pre(CredentialAccessor& ca, KeyStore::Key& peerKey, KeyStore::Key& customKey1, KeyStore::Key& customKey2)
{

    KeyBlob kb("This is the peer secret", KeyBlob::GENERIC);
    kb.SetExpiration(60); /* the minimum is 30 */

    QStatus status = ca.StoreKey(peerKey, kb);
    if (status != ER_OK) {
        return status;
    }
    Timespec expiry;
    kb.GetExpiration(expiry);
    const char* blobName = "This is the custom key 1";
    KeyBlob customKb1((const uint8_t*) blobName, strlen(blobName), KeyBlob::GENERIC);
    customKb1.SetExpiration(30);

    status = ca.AddAssociatedKey(peerKey, customKey1, customKb1);
    if (status != ER_OK) {
        return status;
    }
    qcc::String customName2("Blob for custom key 2");
    KeyBlob customKb2(customName2, KeyBlob::GENERIC);
    customKb2.SetExpiration(45);
    return ca.AddAssociatedKey(peerKey, customKey2, customKb2);
}

QStatus ComboMemberKeyExpired_Pre(CredentialAccessor& ca, KeyStore::Key& peerKey, KeyStore::Key& customKey1, KeyStore::Key& customKey2, KeyStore::Key& customKey3)
{
    KeyBlob kb("This is the peer secret", KeyBlob::GENERIC);

    kb.SetExpiration(60); /* the minimum is 30 */

    QStatus status = ca.StoreKey(peerKey, kb);
    if (status != ER_OK) {
        return status;
    }
    Timespec expiry;
    kb.GetExpiration(expiry);
    const char* blobName = "This is the custom key 1";
    KeyBlob customKb1((const uint8_t*) blobName, strlen(blobName), KeyBlob::GENERIC);
    customKb1.SetExpiration(75);
    status = ca.AddAssociatedKey(peerKey, customKey1, customKb1);
    if (status != ER_OK) {
        return status;
    }
    qcc::String customName2("Blob for custom key 2");
    KeyBlob customKb2(customName2, KeyBlob::GENERIC);
    customKb2.SetExpiration(31);
    status = ca.AddAssociatedKey(peerKey, customKey2, customKb2);
    if (status != ER_OK) {
        return status;
    }

    qcc::String customName3("Blob for custom key 3");
    KeyBlob customKb3(customName3, KeyBlob::GENERIC);
    customKb2.SetExpiration(200);
    return ca.AddAssociatedKey(customKey2, customKey3, customKb3);
}

TEST_F(CredentialAccessorTest, KeysExpired)
{

    CredentialAccessor ca(*g_msgBus);

    /* setup header key expired test */
    GUID128 t1_peerGuid;
    GUID128 t1_customGuid1;
    GUID128 t1_customGuid2;
    KeyStore::Key t1PeerKey(KeyStore::Key::REMOTE, t1_peerGuid);
    KeyStore::Key t1CustomKey1(KeyStore::Key::REMOTE, t1_customGuid1);
    KeyStore::Key t1CustomKey2(KeyStore::Key::REMOTE, t1_customGuid2);
    QStatus status = HeaderKeyExpired_Pre(ca, t1PeerKey, t1CustomKey1, t1CustomKey2);
    ASSERT_EQ(ER_OK, status) << " HeaderKeyExpired_Pre failed";

    /* member key expired test */
    GUID128 t2_peerGuid;
    GUID128 t2_customGuid1;
    GUID128 t2_customGuid2;
    KeyStore::Key t2PeerKey(KeyStore::Key::REMOTE, t2_peerGuid);
    KeyStore::Key t2CustomKey1(KeyStore::Key::REMOTE, t2_customGuid1);
    KeyStore::Key t2CustomKey2(KeyStore::Key::REMOTE, t2_customGuid2);
    status = MemberKeyExpired_Pre(ca, t2PeerKey, t2CustomKey1, t2CustomKey2);
    ASSERT_EQ(ER_OK, status) << " MemberKeyExpired_Pre failed";

    /* combo member key expired test */
    GUID128 t3_peerGuid;
    GUID128 t3_customGuid1;
    GUID128 t3_customGuid2;
    GUID128 t3_customGuid3;
    KeyStore::Key t3PeerKey(KeyStore::Key::REMOTE, t3_peerGuid);
    KeyStore::Key t3CustomKey1(KeyStore::Key::REMOTE, t3_customGuid1);
    KeyStore::Key t3CustomKey2(KeyStore::Key::REMOTE, t3_customGuid2);
    KeyStore::Key t3CustomKey3(KeyStore::Key::REMOTE, t3_customGuid3);
    status = ComboMemberKeyExpired_Pre(ca, t3PeerKey, t3CustomKey1, t3CustomKey2, t3CustomKey3);
    ASSERT_EQ(ER_OK, status) << " ComboMemberKeyExpired_Pre failed";

    Timespec now;
    GetTimeNow(&now);
    printf("*** Sleep 35 secs since the minimum key expiration time is 30 seconds\n");
    qcc::Sleep(35000);

    KeyBlob kb2("This is the peer secret 2", KeyBlob::GENERIC);
    kb2.SetExpiration(60);
    GUID128 peerGuid2;
    KeyStore::Key peerKey2(KeyStore::Key::REMOTE, peerGuid2);
    status = ca.StoreKey(peerKey2, kb2);
    ASSERT_EQ(ER_OK, status) << " ca.StoreKey failed";

    /* header key expired test */
    {
        KeyBlob tmpKb;
        status = ca.GetKey(t1PeerKey, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t1_peerGuid expected to fail";

        status = ca.GetKey(t1CustomKey1, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t1_customGuid1 expected to fail";
        status = ca.GetKey(t1CustomKey2, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t1_customGuid2 expected to fail";
    }

    /* member key expired test */
    {
        KeyBlob tmpKb;
        status = ca.GetKey(t2PeerKey, tmpKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey on t2_peerGuid failed";

        status = ca.GetKey(t2CustomKey1, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t2_customGuid1 expected to fail";
        status = ca.GetKey(t2CustomKey2, tmpKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey on t2_customGuid2 failed";
    }

    /* combo memeber key expired test */
    {
        KeyBlob tmpKb;
        status = ca.GetKey(t3PeerKey, tmpKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey on t3_peerGuid failed";

        status = ca.GetKey(t3CustomKey1, tmpKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey on t3_customGuid1 failed";
        status = ca.GetKey(t3CustomKey2, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t3_customGuid2 expected to fail";
        status = ca.GetKey(t3CustomKey3, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t3_customGuid3 expected to fail";
    }
}

TEST_F(CredentialAccessorTest, StoreComplexKeyChain)
{
    CredentialAccessor ca(*g_msgBus);

    KeyBlob kb("This is the peer secret", KeyBlob::GENERIC);

    kb.SetExpiration(100);

    GUID128 localGuid;
    KeyStore::Key localKey(KeyStore::Key::LOCAL, localGuid);

    QStatus status = ca.StoreKey(localKey, kb);
    ASSERT_EQ(ER_OK, status) << " ca.StoreKey failed";

    const char* blobName = "This is the custom key 1";
    KeyBlob customKb1((const uint8_t*) blobName, strlen(blobName), KeyBlob::GENERIC);

    printf("customKb1 has blob type %d\n", customKb1.GetType());
    GUID128 customGuid1;
    KeyStore::Key customKey1(KeyStore::Key::LOCAL, customGuid1);
    status = ca.AddAssociatedKey(localKey, customKey1, customKb1);

    ASSERT_EQ(ER_OK, status) << " ca.AddAssociatedKey failed";

    GUID128 customGuid2;
    qcc::String customName2("Blob for custom key 2");
    KeyBlob customKb2(customName2, KeyBlob::GENERIC);
    KeyStore::Key customKey2(KeyStore::Key::LOCAL, customGuid2);
    status = ca.AddAssociatedKey(localKey, customKey2, customKb2);

    ASSERT_EQ(ER_OK, status) << " ca.AddAssociatedKey failed";

    GUID128 customGuid3;
    qcc::String customName3("Blob for custom key 3");
    KeyBlob customKb3(customName3, KeyBlob::GENERIC);
    KeyStore::Key customKey3(KeyStore::Key::LOCAL, customGuid3);
    status = ca.AddAssociatedKey(customKey2, customKey3, customKb3);

    ASSERT_EQ(ER_OK, status) << " ca.AddAssociatedKey failed";

    /* now retrieve the list back */
    KeyStore::Key* customKeyList = NULL;
    size_t numKeys = 0;
    status = ca.GetKeys(localKey, &customKeyList, &numKeys);
    if (status != ER_OK) {
        delete [] customKeyList;
        FAIL() << " ca.GetKeys failed";
        return;
    }

    ASSERT_EQ(2U, numKeys) << " ca.GetKeys expected to return 2 entries";

    KeyStore::Key twoKeys[2];
    for (size_t cnt = 0; cnt < numKeys; cnt++) {
        twoKeys[cnt] = customKeyList[cnt];
    }
    delete [] customKeyList;

    for (size_t cnt = 0; cnt < numKeys; cnt++) {
        cout << "Custom GUID: " << twoKeys[cnt].ToString().c_str()  << endl;
        ASSERT_TRUE((twoKeys[cnt] == customKey1) || (twoKeys[cnt] == customKey2)) << " custom GUID does not match any of the originals";

        KeyBlob readBackKb;
        status = ca.GetKey(twoKeys[cnt], readBackKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey failed";

        ASSERT_TRUE((memcmp(customKb1.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) || (memcmp(customKb2.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0)) << " the read back KB does not match original";
    }

    /* get the custom keys for customGUID2 */
    numKeys = 0;
    customKeyList = NULL;
    status = ca.GetKeys(customKey2, &customKeyList, &numKeys);
    if (status != ER_OK) {
        delete [] customKeyList;
        FAIL() << " ca.GetKeys for customGuid2 failed";
        return;
    }

    ASSERT_EQ(1U, numKeys) << " ca.GetKeys expected to return 1 guids";

    KeyStore::Key oneCustomKey = customKeyList[0];
    delete [] customKeyList;
    cout << "Custom Key: " << oneCustomKey.ToString().c_str()  << endl;
    ASSERT_TRUE(oneCustomKey == customKey3) << " custom key does not match any of the originals";

    KeyBlob readBackKb;
    status = ca.GetKey(oneCustomKey, readBackKb);
    ASSERT_EQ(ER_OK, status) << " ca.GetKey failed";

    ASSERT_TRUE(memcmp(customKb3.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) << " the read back KB does not match original";

    /* now delete customGuid1 */
    status = ca.DeleteKey(customKey1);

    ASSERT_EQ(ER_OK, status) << " ca.DeleteKey failed";

    /* retrieve the custom list back */
    /* now retrieve the list back */
    numKeys = 0;
    customKeyList = NULL;
    status = ca.GetKeys(localKey, &customKeyList, &numKeys);
    if (status != ER_OK) {
        delete [] customKeyList;
        FAIL() << " ca.GetKeys for localKey failed";
        return;
    }

    ASSERT_EQ(1U, numKeys) << " ca.GetKeys expected to return 1 key";

    oneCustomKey = customKeyList[0];
    delete [] customKeyList;
    cout << "Custom GUID: " << oneCustomKey.ToString().c_str()  << endl;
    ASSERT_TRUE(oneCustomKey == customKey2) << " custom GUID does not match any of the originals";

    status = ca.GetKey(oneCustomKey, readBackKb);
    ASSERT_EQ(ER_OK, status) << " ca.GetKey failed";

    ASSERT_TRUE(memcmp(customKb2.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) << " the read back KB does not match original";

    /* delete the header key */
    status = ca.DeleteKey(localKey);
    ASSERT_EQ(ER_OK, status) << " ca.DeleteKey on header guid failed";

    KeyBlob tmpKb;
    status = ca.GetKey(localKey, tmpKb);
    ASSERT_NE(ER_OK, status) << " ca.GetKey on localKey expected to fail";

    status = ca.GetKey(customKey3, tmpKb);
    ASSERT_NE(ER_OK, status) << " ca.GetKey on customKey3 expected to fail";

    status = ca.GetKey(customKey2, tmpKb);
    ASSERT_NE(ER_OK, status) << " ca.GetKey on customKey2 expected to fail";
}

/**
 * Test the keystore using two keys with the same GUID -- one remote and one
 * local.  Make sure none of the keys overwrites each other.
 */
TEST_F(CredentialAccessorTest, TwoKeysWithTheSameGUID)
{
    CredentialAccessor ca(*g_msgBus);

    KeyBlob localKb("Local", KeyBlob::GENERIC);
    KeyBlob remoteKb("Remote", KeyBlob::GENERIC);

    localKb.SetExpiration(1000);
    remoteKb.SetExpiration(1000);

    GUID128 guid;
    KeyStore::Key localKey(KeyStore::Key::LOCAL, guid);
    KeyStore::Key remoteKey(KeyStore::Key::REMOTE, guid);

    ASSERT_EQ(ER_OK, ca.StoreKey(localKey, localKb)) << " ca.StoreKey local key failed";
    ASSERT_EQ(ER_OK, ca.StoreKey(remoteKey, remoteKb)) << " ca.StoreKey remote key failed";

    KeyBlob readBackKb;
    /* read back the local key blob */
    ASSERT_EQ(ER_OK, ca.GetKey(localKey, readBackKb)) << " ca.GetKey local key failed";
    ASSERT_TRUE(memcmp(localKb.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) << " the read back KB does not match original local KB";

    /* read back the remote key blob */
    ASSERT_EQ(ER_OK, ca.GetKey(remoteKey, readBackKb)) << " ca.GetKey remote key failed";
    ASSERT_TRUE(memcmp(remoteKb.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) << " the read back KB does not match original remote KB";

    /* now delete remote guid */
    ASSERT_EQ(ER_OK, ca.DeleteKey(remoteKey)) << " ca.DeleteKey remote key failed";
    ASSERT_EQ(ER_OK, ca.GetKey(localKey, readBackKb)) << " ca.GetKey local key failed";
    ASSERT_NE(ER_OK, ca.GetKey(remoteKey, readBackKb)) << " ca.GetKey remote key is extected to fail";
    /* now delete local key */
    ASSERT_EQ(ER_OK, ca.DeleteKey(localKey)) << " ca.DeleteKey local key failed";
    ASSERT_NE(ER_OK, ca.GetKey(localKey, readBackKb)) << " ca.GetKey local key is expected to fail";

}

#if defined(QCC_OS_GROUP_POSIX)

/** a keystore compatibility test using old key store generated in a linux
 * application with keystore version 0x103.
 */

static const char Rev0X103KeyStore[] = {
    "AwEjAAAACEtbgok0cG9FkAZiTHILwWwBAAAAAAAA4jYH7mABwaANNQGjS70p"
    "JlYX7kbxeF02f/K0iTrVcQGe343Yz60DjEo9cG8Gam5jngLGS3k4+etaHjEo"
    "C/Q8GX8eQTtk7hhewBt7U4zHzhQnlIST0y8ZXMYgDaCvwv6LY8vriQLSs0Qo"
    "SqrXmZPxwOyUCaL4QwNZjKuSLSoyobvizzLc7/SQTMpICbY7VkCrRlALyZx+"
    "6hjUHWgSYbxpod65fZrIf3BE6YUMg1uH10lWJo5AzLEINUcRdJKCFwnbArH8"
    "3Q0IoF4/1Wx5EMjK6SSBlHdppV7yFRlRSctx+tBPHv0xXCVOnyRNuoebIWTp"
    "kfaOjXVjwlSdVHaymkLN+vRp43Vfm3zqwcICwhVAdukv/ubr/PJBAVNEu9EJ"
    "cPHY1bVUcQUdDcUfPMFs4zYhSEdat0yghfhMyiIjTNWRFSpIU/Bn9BpgWXVn"
    "7vSFQOmd2b+2gO271nSBT3QXdFdLRK0S3rznBBBoveJlyA=="
};
static const char Rev0X103KeyStorePwd[] = {
    "L3VzcjIvcGhpbG4vQ3JlZGVudGlhbEFjY2Vzc29yVGVzdA=="
};

class CompatibleCredentialAccessorTest : public CredentialAccessorTest {
  public:

    CompatibleCredentialAccessorTest() : CredentialAccessorTest(Rev0X103KeyStore, Rev0X103KeyStorePwd)
    {
    }
};

/**
 * Test the keystore loaded with an older keystore format where the key is
 * just the GUID.
 */
TEST_F(CompatibleCredentialAccessorTest, LoadOldFormat)
{
    CredentialAccessor ca(*g_msgBus);

    KeyBlob localKb("Local", KeyBlob::GENERIC);

    localKb.SetExpiration(1000);

    GUID128 guid;
    KeyStore::Key localKey(KeyStore::Key::LOCAL, guid);

    ASSERT_EQ(ER_OK, ca.StoreKey(localKey, localKb)) << " ca.StoreKey local key failed";

    KeyBlob readBackKb;
    /* read back the local key blob */
    ASSERT_EQ(ER_OK, ca.GetKey(localKey, readBackKb)) << " ca.GetKey local key failed";
    ASSERT_TRUE(memcmp(localKb.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) << " the read back KB does not match original local KB";

}

#endif
