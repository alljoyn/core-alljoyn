/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include "ajTestCommon.h"
#include "ServiceSetup.h"

#include "CredentialAccessor.h"

using namespace qcc;
using namespace std;

class LocalAuthListener : public AuthListener {
    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {
        return true;
    }
    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
    }
};

class CredentialAccessorTest : public testing::Test {
  public:

    BusAttachment* g_msgBus;
    LocalAuthListener myListener;

    virtual void SetUp() {
        g_msgBus = new BusAttachment("testservices", true);
        QStatus status = g_msgBus->Start();
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        if (!g_msgBus->IsConnected()) {
            /* Connect to the daemon and wait for the bus to exit */
            status = g_msgBus->Connect(ajn::getConnectArg().c_str());
            ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        }
        g_msgBus->ClearKeyStore();
        status = g_msgBus->EnablePeerSecurity("ALLJOYN_ECDHE_NULL", &myListener, "CredentialAccessorTest");
        ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }

    virtual void TearDown() {
        if (g_msgBus) {
            BusAttachment* deleteMe = g_msgBus;
            g_msgBus = NULL;
            delete deleteMe;
        }
    }

};

TEST_F(CredentialAccessorTest, GetLocalGuid)
{

    CredentialAccessor ca(*g_msgBus);
    qcc::GUID128 localGuid;
    QStatus status = ca.GetGuid(localGuid);
    ASSERT_EQ(ER_OK, status) << " ca.GetGuid failed with actual status: " << QCC_StatusText(status);

    cout << "Local GUID: " << localGuid.ToString().c_str() << endl;
}

TEST_F(CredentialAccessorTest, StoreDSAKey)
{

    CredentialAccessor ca(*g_msgBus);

    qcc::GUID128 localGuid;
    QStatus status = ca.GetGuid(localGuid);
    ASSERT_EQ(ER_OK, status) << " ca.GetGuid failed with actual status: " << QCC_StatusText(status);

    cout << "Local GUID: " << localGuid.ToString().c_str() << endl;

    KeyBlob kb("This is the DSA Key", KeyBlob::GENERIC);

    kb.SetExpiration(100);

    GUID128 dsaGuid;
    status = ca.GetLocalGUID(KeyBlob::DSA_PRIVATE, dsaGuid);
    ASSERT_EQ(ER_OK, status) << " ca.GetLocalGUID failed with actual status: " << QCC_StatusText(status);

    status = ca.StoreKey(dsaGuid, kb);
    ASSERT_EQ(ER_OK, status) << " ca.StoreKey failed with actual status: " << QCC_StatusText(status);

    KeyBlob readBackKb;
    status = ca.GetKey(dsaGuid, readBackKb);
    ASSERT_EQ(ER_OK, status) << " ca.GetKey failed with actual status: " << QCC_StatusText(status);

    ASSERT_TRUE(memcmp(kb.GetData(), readBackKb.GetData(), kb.GetSize()) == 0) << " the read back KB does not match original";
}

TEST_F(CredentialAccessorTest, StoreCustomKey)
{

    CredentialAccessor ca(*g_msgBus);

    KeyBlob kb("This is the peer secret", KeyBlob::GENERIC);

    kb.SetExpiration(100);

    GUID128 peerGuid;

    QStatus status = ca.StoreKey(peerGuid, kb);
    ASSERT_EQ(ER_OK, status) << " ca.StoreKey failed with actual status: " << QCC_StatusText(status);

    const char* blobName = "This is the custom key 1";
    KeyBlob customKb1((const uint8_t*) blobName, strlen(blobName), KeyBlob::SPKI_CERT);

    printf("customKb1 has blob type %d\n", customKb1.GetType());
    GUID128 customGuid1;
    status = ca.AddAssociatedKey(peerGuid, customGuid1, customKb1);

    ASSERT_EQ(ER_OK, status) << " ca.AddAssociatedKey failed with actual status: " << QCC_StatusText(status);

    GUID128 customGuid2;
    qcc::String customName2("Blob for custom key 2");
    KeyBlob customKb2(customName2, KeyBlob::SPKI_CERT);
    status = ca.AddAssociatedKey(peerGuid, customGuid2, customKb2);

    ASSERT_EQ(ER_OK, status) << " ca.AddAssociatedKey failed with actual status: " << QCC_StatusText(status);

    /* now retrieve the list back */
    GUID128* customGuidList = NULL;
    size_t numGuids = 0;
    status = ca.GetKeys(peerGuid, &customGuidList, &numGuids);
    if (status != ER_OK) {
        delete [] customGuidList;
        FAIL() << " ca.GetKeys failed with actual status: " << QCC_StatusText(status);
        return;
    }

    GUID128 twoGuids[2];
    for (size_t cnt = 0; cnt < numGuids; cnt++) {
        twoGuids[cnt] = customGuidList[cnt];
    }
    delete [] customGuidList;
    ASSERT_EQ(2U, numGuids) << " ca.GetKeys expected to return 2 guids";

    for (size_t cnt = 0; cnt < numGuids; cnt++) {
        cout << "Custom GUID: " << twoGuids[cnt].ToString().c_str()  << endl;
        ASSERT_TRUE((twoGuids[cnt] == customGuid1) || (twoGuids[cnt] == customGuid2)) << " custom GUID does not match any of the originals";

        KeyBlob readBackKb;
        status = ca.GetKey(twoGuids[cnt], readBackKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey failed with actual status: " << QCC_StatusText(status);

        ASSERT_TRUE((memcmp(customKb1.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) || (memcmp(customKb2.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0)) << " the read back KB does not match original";
    }

    /* now delete customGuid1 */
    status = ca.DeleteKey(customGuid1);

    ASSERT_EQ(ER_OK, status) << " ca.DeleteKey failed with actual status: " << QCC_StatusText(status);

    /* retrieve the custom list back */
    /* now retrieve the list back */
    numGuids = 0;
    customGuidList = NULL;
    status = ca.GetKeys(peerGuid, &customGuidList, &numGuids);
    if (status != ER_OK) {
        delete [] customGuidList;
        FAIL() << " ca.GetKeys failed with actual status: " << QCC_StatusText(status);
        return;
    }

    if (numGuids != 1) {
        delete [] customGuidList;
        FAIL() << " ca.GetKeys expected to return 1 guid";
        return;
    }

    GUID128 oneCustomGuid = customGuidList[0];
    delete [] customGuidList;
    cout << "Custom GUID: " << oneCustomGuid.ToString().c_str()  << endl;
    ASSERT_TRUE(oneCustomGuid == customGuid2) << " custom GUID does not match any of the originals";

    KeyBlob readBackKb;
    status = ca.GetKey(oneCustomGuid, readBackKb);
    ASSERT_EQ(ER_OK, status) << " ca.GetKey failed with actual status: " << QCC_StatusText(status);

    ASSERT_TRUE(memcmp(customKb2.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) << " the read back KB does not match original";

    /* delete the header key */
    status = ca.DeleteKey(peerGuid);
    ASSERT_EQ(ER_OK, status) << " ca.DeleteKey on header guid failed with actual status: " << QCC_StatusText(status);

    KeyBlob tmpKb;
    status = ca.GetKey(peerGuid, tmpKb);
    ASSERT_NE(ER_OK, status) << " ca.GetKey on peerGuid expected to fail but got actual status: " << QCC_StatusText(status);

    status = ca.GetKey(customGuid2, tmpKb);
    ASSERT_NE(ER_OK, status) << " ca.GetKey on customGuid2 expected to fail but got actual status: " << QCC_StatusText(status);
}


QStatus HeaderKeyExpired_Pre(CredentialAccessor& ca, GUID128& peerGuid, GUID128& customGuid1, GUID128& customGuid2)
{

    QStatus status = ER_FAIL;
    KeyBlob kb("This is the peer secret", KeyBlob::GENERIC);
    kb.SetExpiration(30); /* the minimum is 30 */

    status = ca.StoreKey(peerGuid, kb);
    if (status != ER_OK) {
        return status;
    }
    Timespec expiry;
    kb.GetExpiration(expiry);
    const char* blobName = "This is the custom key 1";
    KeyBlob customKb1((const uint8_t*) blobName, strlen(blobName), KeyBlob::SPKI_CERT);

    status = ca.AddAssociatedKey(peerGuid, customGuid1, customKb1);
    if (status != ER_OK) {
        return status;
    }

    qcc::String customName2("Blob for custom key 2");
    KeyBlob customKb2(customName2, KeyBlob::SPKI_CERT);
    return ca.AddAssociatedKey(peerGuid, customGuid2, customKb2);
}

QStatus MemberKeyExpired_Pre(CredentialAccessor& ca, GUID128& peerGuid, GUID128& customGuid1, GUID128& customGuid2)
{

    KeyBlob kb("This is the peer secret", KeyBlob::GENERIC);
    kb.SetExpiration(60); /* the minimum is 30 */

    QStatus status = ca.StoreKey(peerGuid, kb);
    if (status != ER_OK) {
        return status;
    }
    Timespec expiry;
    kb.GetExpiration(expiry);
    const char* blobName = "This is the custom key 1";
    KeyBlob customKb1((const uint8_t*) blobName, strlen(blobName), KeyBlob::SPKI_CERT);
    customKb1.SetExpiration(30);

    status = ca.AddAssociatedKey(peerGuid, customGuid1, customKb1);
    if (status != ER_OK) {
        return status;
    }
    qcc::String customName2("Blob for custom key 2");
    KeyBlob customKb2(customName2, KeyBlob::SPKI_CERT);
    customKb2.SetExpiration(45);
    return ca.AddAssociatedKey(peerGuid, customGuid2, customKb2);
}

QStatus ComboMemberKeyExpired_Pre(CredentialAccessor& ca, GUID128& peerGuid, GUID128& customGuid1, GUID128& customGuid2, GUID128& customGuid3)
{
    KeyBlob kb("This is the peer secret", KeyBlob::GENERIC);

    kb.SetExpiration(60); /* the minimum is 30 */

    QStatus status = ca.StoreKey(peerGuid, kb);
    if (status != ER_OK) {
        return status;
    }
    Timespec expiry;
    kb.GetExpiration(expiry);
    const char* blobName = "This is the custom key 1";
    KeyBlob customKb1((const uint8_t*) blobName, strlen(blobName), KeyBlob::SPKI_CERT);
    customKb1.SetExpiration(75);
    status = ca.AddAssociatedKey(peerGuid, customGuid1, customKb1);
    if (status != ER_OK) {
        return status;
    }
    qcc::String customName2("Blob for custom key 2");
    KeyBlob customKb2(customName2, KeyBlob::SPKI_CERT);
    customKb2.SetExpiration(31);
    status = ca.AddAssociatedKey(peerGuid, customGuid2, customKb2);
    if (status != ER_OK) {
        return status;
    }

    qcc::String customName3("Blob for custom key 3");
    KeyBlob customKb3(customName3, KeyBlob::SPKI_CERT);
    customKb2.SetExpiration(200);
    return ca.AddAssociatedKey(customGuid2, customGuid3, customKb3);
}

TEST_F(CredentialAccessorTest, KeysExpired)
{

    CredentialAccessor ca(*g_msgBus);

    /* setup header key expired test */
    GUID128 t1_peerGuid;
    GUID128 t1_customGuid1;
    GUID128 t1_customGuid2;
    QStatus status = HeaderKeyExpired_Pre(ca, t1_peerGuid, t1_customGuid1, t1_customGuid2);
    ASSERT_EQ(ER_OK, status) << " HeaderKeyExpired_Pre failed with actual status: " << QCC_StatusText(status);

    /* member key expired test */
    GUID128 t2_peerGuid;
    GUID128 t2_customGuid1;
    GUID128 t2_customGuid2;
    status = MemberKeyExpired_Pre(ca, t2_peerGuid, t2_customGuid1, t2_customGuid2);
    ASSERT_EQ(ER_OK, status) << " MemberKeyExpired_Pre failed with actual status: " << QCC_StatusText(status);

    /* combo member key expired test */
    GUID128 t3_peerGuid;
    GUID128 t3_customGuid1;
    GUID128 t3_customGuid2;
    GUID128 t3_customGuid3;
    status = ComboMemberKeyExpired_Pre(ca, t3_peerGuid, t3_customGuid1, t3_customGuid2, t3_customGuid3);
    ASSERT_EQ(ER_OK, status) << " ComboMemberKeyExpired_Pre failed with actual status: " << QCC_StatusText(status);

    Timespec now;
    GetTimeNow(&now);
    printf("*** Sleep 35 secs since the minimum key expiration time is 30 seconds\n");
    qcc::Sleep(35000);

    KeyBlob kb2("This is the peer secret 2", KeyBlob::GENERIC);
    kb2.SetExpiration(60);
    GUID128 peerGuid2;
    status = ca.StoreKey(peerGuid2, kb2);
    ASSERT_EQ(ER_OK, status) << " ca.StoreKey failed with actual status: " << QCC_StatusText(status);

    /* header key expired test */
    {
        KeyBlob tmpKb;
        status = ca.GetKey(t1_peerGuid, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t1_peerGuid expected to fail but got actual status: " << QCC_StatusText(status);

        status = ca.GetKey(t1_customGuid1, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t1_customGuid1 expected to fail but got actual status: " << QCC_StatusText(status);
        status = ca.GetKey(t1_customGuid2, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t1_customGuid2 expected to fail but got actual status: " << QCC_StatusText(status);
    }

    /* member key expired test */
    {
        KeyBlob tmpKb;
        status = ca.GetKey(t2_peerGuid, tmpKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey on t2_peerGuid failed with actual status: " << QCC_StatusText(status);

        status = ca.GetKey(t2_customGuid1, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t2_customGuid1 expected to fail but got actual status: " << QCC_StatusText(status);
        status = ca.GetKey(t2_customGuid2, tmpKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey on t2_customGuid2 failed with actual status: " << QCC_StatusText(status);
    }

    /* combo memeber key expired test */
    {
        KeyBlob tmpKb;
        status = ca.GetKey(t3_peerGuid, tmpKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey on t3_peerGuid failed with actual status: " << QCC_StatusText(status);

        status = ca.GetKey(t3_customGuid1, tmpKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey on t3_customGuid1 failed with actual status: " << QCC_StatusText(status);
        status = ca.GetKey(t3_customGuid2, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t3_customGuid2 expected to fail but got actual status: " << QCC_StatusText(status);
        status = ca.GetKey(t3_customGuid3, tmpKb);
        ASSERT_NE(ER_OK, status) << " ca.GetKey on t3_customGuid3 expected to fail but got actual status: " << QCC_StatusText(status);
    }
}

TEST_F(CredentialAccessorTest, StoreComplexKeyChain)
{
    CredentialAccessor ca(*g_msgBus);

    KeyBlob kb("This is the peer secret", KeyBlob::GENERIC);

    kb.SetExpiration(100);

    GUID128 peerGuid;

    QStatus status = ca.StoreKey(peerGuid, kb);
    ASSERT_EQ(ER_OK, status) << " ca.StoreKey failed with actual status: " << QCC_StatusText(status);

    const char* blobName = "This is the custom key 1";
    KeyBlob customKb1((const uint8_t*) blobName, strlen(blobName), KeyBlob::SPKI_CERT);

    printf("customKb1 has blob type %d\n", customKb1.GetType());
    GUID128 customGuid1;
    status = ca.AddAssociatedKey(peerGuid, customGuid1, customKb1);

    ASSERT_EQ(ER_OK, status) << " ca.AddAssociatedKey failed with actual status: " << QCC_StatusText(status);

    GUID128 customGuid2;
    qcc::String customName2("Blob for custom key 2");
    KeyBlob customKb2(customName2, KeyBlob::SPKI_CERT);
    status = ca.AddAssociatedKey(peerGuid, customGuid2, customKb2);

    ASSERT_EQ(ER_OK, status) << " ca.AddAssociatedKey failed with actual status: " << QCC_StatusText(status);

    GUID128 customGuid3;
    qcc::String customName3("Blob for custom key 3");
    KeyBlob customKb3(customName3, KeyBlob::SPKI_CERT);
    status = ca.AddAssociatedKey(customGuid2, customGuid3, customKb3);

    ASSERT_EQ(ER_OK, status) << " ca.AddAssociatedKey failed with actual status: " << QCC_StatusText(status);

    /* now retrieve the list back */
    GUID128* customGuidList = NULL;
    size_t numGuids = 0;
    status = ca.GetKeys(peerGuid, &customGuidList, &numGuids);
    if (status != ER_OK) {
        delete [] customGuidList;
        FAIL() << " ca.GetKeys failed with actual status: " << QCC_StatusText(status);
        return;
    }

    ASSERT_EQ(2U, numGuids) << " ca.GetKeys expected to return 2 guids";

    GUID128 twoGuids[2];
    for (size_t cnt = 0; cnt < numGuids; cnt++) {
        twoGuids[cnt] = customGuidList[cnt];
    }
    delete [] customGuidList;

    for (size_t cnt = 0; cnt < numGuids; cnt++) {
        cout << "Custom GUID: " << twoGuids[cnt].ToString().c_str()  << endl;
        ASSERT_TRUE((twoGuids[cnt] == customGuid1) || (twoGuids[cnt] == customGuid2)) << " custom GUID does not match any of the originals";

        KeyBlob readBackKb;
        status = ca.GetKey(twoGuids[cnt], readBackKb);
        ASSERT_EQ(ER_OK, status) << " ca.GetKey failed with actual status: " << QCC_StatusText(status);

        ASSERT_TRUE((memcmp(customKb1.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) || (memcmp(customKb2.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0)) << " the read back KB does not match original";
    }

    /* get the custom keys for customGUID2 */
    numGuids = 0;
    customGuidList = NULL;
    status = ca.GetKeys(customGuid2, &customGuidList, &numGuids);
    if (status != ER_OK) {
        delete [] customGuidList;
        FAIL() << " ca.GetKeys for customGuid2 failed with actual status: " << QCC_StatusText(status);
        return;
    }

    ASSERT_EQ(1U, numGuids) << " ca.GetKeys expected to return 1 guids";

    GUID128 oneCustomGuid = customGuidList[0];
    delete [] customGuidList;
    cout << "Custom GUID: " << oneCustomGuid.ToString().c_str()  << endl;
    ASSERT_TRUE(oneCustomGuid == customGuid3) << " custom GUID does not match any of the originals";

    KeyBlob readBackKb;
    status = ca.GetKey(oneCustomGuid, readBackKb);
    ASSERT_EQ(ER_OK, status) << " ca.GetKey failed with actual status: " << QCC_StatusText(status);

    ASSERT_TRUE(memcmp(customKb3.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) << " the read back KB does not match original";

    /* now delete customGuid1 */
    status = ca.DeleteKey(customGuid1);

    ASSERT_EQ(ER_OK, status) << " ca.DeleteKey failed with actual status: " << QCC_StatusText(status);

    /* retrieve the custom list back */
    /* now retrieve the list back */
    numGuids = 0;
    customGuidList = NULL;
    status = ca.GetKeys(peerGuid, &customGuidList, &numGuids);
    if (status != ER_OK) {
        delete [] customGuidList;
        FAIL() << " ca.GetKeys for customGuid2 failed with actual status: " << QCC_StatusText(status);
        return;
    }

    ASSERT_EQ(1U, numGuids) << " ca.GetKeys expected to return 1 guid";

    oneCustomGuid = customGuidList[0];
    delete [] customGuidList;
    cout << "Custom GUID: " << oneCustomGuid.ToString().c_str()  << endl;
    ASSERT_TRUE(oneCustomGuid == customGuid2) << " custom GUID does not match any of the originals";

    status = ca.GetKey(oneCustomGuid, readBackKb);
    ASSERT_EQ(ER_OK, status) << " ca.GetKey failed with actual status: " << QCC_StatusText(status);

    ASSERT_TRUE(memcmp(customKb2.GetData(), readBackKb.GetData(), readBackKb.GetSize()) == 0) << " the read back KB does not match original";

    /* delete the header key */
    status = ca.DeleteKey(peerGuid);
    ASSERT_EQ(ER_OK, status) << " ca.DeleteKey on header guid failed with actual status: " << QCC_StatusText(status);

    KeyBlob tmpKb;
    status = ca.GetKey(peerGuid, tmpKb);
    ASSERT_NE(ER_OK, status) << " ca.GetKey on peerGuid expected to fail but got actual status: " << QCC_StatusText(status);

    status = ca.GetKey(customGuid3, tmpKb);
    ASSERT_NE(ER_OK, status) << " ca.GetKey on customGuid3 expected to fail but got actual status: " << QCC_StatusText(status);

    status = ca.GetKey(customGuid2, tmpKb);
    ASSERT_NE(ER_OK, status) << " ca.GetKey on customGuid2 expected to fail but got actual status: " << QCC_StatusText(status);
}
