/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#include "SQLStorageConfig.h"
#include "AJNCaStorage.h"

using namespace std;
using namespace ajn;
using namespace qcc;
using namespace securitymgr;

class AJNCaStorageTest :
    public::testing::Test {
  public:
    AJNCaStorageTest() : ca(nullptr), sql(nullptr) { }

    void SetUp()
    {
    }

    void TearDown()
    {
        if (sql != nullptr) {
            sql->Reset();
        }
        if (ca != nullptr) {
            ca->Reset();
        }
    }

    shared_ptr<AJNCaStorage> ca;
    shared_ptr<SQLStorage> sql;
};
/**
 * A Basic test for AJNCaStorage class.
 */
TEST_F(AJNCaStorageTest, BasicTest) {
    const char* storeName = "AJNCaStorageTestCA";
    SQLStorageConfig storageConfig;
    storageConfig.settings[STORAGE_FILEPATH_KEY] = "AJNCaStorageTestDB";
    sql = shared_ptr<SQLStorage>(new SQLStorage(storageConfig));
    ASSERT_EQ(ER_OK, sql->GetStatus());
    ca = shared_ptr<AJNCaStorage>(new AJNCaStorage());
    ASSERT_EQ(ER_OK, ca->Init(storeName, sql));
    KeyInfoNISTP256 key;
    ASSERT_EQ(ER_OK, ca->GetCaPublicKeyInfo(key));
    MembershipCertificate cert;
    Application app;
    app.keyInfo = key;
    GroupInfo group;
    group.authority = key;
    group.guid = GUID128(0xbc);
    ASSERT_EQ(ER_OK, ca->GenerateMembershipCertificate(app, group, cert));
    cout << cert.ToString() << endl;
    String der;
    ASSERT_EQ(ER_OK, cert.EncodeCertificateDER(der));
    MembershipCertificate cert2;
    ASSERT_EQ(ER_OK, cert2.DecodeCertificateDER(der));
    String der2;
    ASSERT_EQ(ER_OK, cert2.EncodeCertificateDER(der2));
    ASSERT_EQ(der.size(), der2.size());
    ASSERT_EQ(ER_OK, cert2.Verify(key));

    ASSERT_NE((size_t)0, cert.GetEncodedLen());
    ASSERT_FALSE(nullptr == cert.GetEncoded());
    MembershipCertificate cert3;
    ASSERT_EQ(ER_OK, cert3.LoadEncoded(cert.GetEncoded(), cert.GetEncodedLen()));
    ASSERT_EQ(ER_OK, cert3.Verify(key));
    ASSERT_EQ(cert.GetEncodedLen(), cert3.GetEncodedLen());
    ASSERT_EQ(0, memcmp(cert.GetEncoded(), cert3.GetEncoded(), cert3.GetEncodedLen()));

    vector<MembershipCertificate> local;
    local.push_back(cert3);
    vector<MembershipCertificate>::iterator localIt;
    for (localIt = local.begin(); localIt != local.end(); ++localIt) {
        ASSERT_EQ(ER_OK, localIt->Verify(key));
        ASSERT_EQ(cert.GetEncodedLen(), localIt->GetEncodedLen());
        ASSERT_EQ(0, memcmp(cert.GetEncoded(), localIt->GetEncoded(), localIt->GetEncodedLen()));
    }
    MembershipCertificate cert4;
    ASSERT_EQ(ER_OK, cert4.LoadEncoded(cert.GetEncoded(), cert.GetEncodedLen()));
    ASSERT_EQ(ER_OK, cert4.Verify(key));
    cert4.SetGuild(cert.GetGuild());
    ASSERT_EQ(cert.GetEncodedLen(), cert4.GetEncodedLen());
    ASSERT_EQ(0, memcmp(cert.GetEncoded(), cert4.GetEncoded(), cert4.GetEncodedLen()));

    AJNCa realCa;
    ASSERT_EQ(ER_OK, realCa.Init(storeName));
    MembershipCertificate cert5;
    ASSERT_EQ(ER_OK, cert5.LoadEncoded(cert.GetEncoded(), cert.GetEncodedLen()));
    ASSERT_EQ(ER_OK, cert5.Verify(key));
    ASSERT_EQ(ER_OK, realCa.SignCertificate(cert5));
    ASSERT_EQ(ER_OK, cert5.Verify(key));
    ASSERT_EQ(ER_OK, cert5.VerifyValidity());
    MembershipCertificate cert6;
    String der5;
    ASSERT_EQ(ER_OK, cert5.EncodeCertificateDER(der5));
    // ASSERT_EQ(ER_OK, cert6.DecodeCertificateDER(der5));
    // ASSERT_EQ(ER_OK, cert6.Verify(key));
}
