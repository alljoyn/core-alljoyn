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

/** @file AJNCaStorageTests.cc */

namespace secmgr_tests {
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
 * @test Basic tests for the sample implementation of a CAStorage based on
 *       AllJoyn.
 *       -# Initialize an AJNCAStorage.
 *       -# Retrieve the public key of the CA.
 *       -# Generate a membership certificate.
 **/
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
    qcc::String der_;
    ASSERT_EQ(ER_OK, cert.EncodeCertificateDER(der_));
    MembershipCertificate cert3;
    ASSERT_EQ(ER_OK, cert3.DecodeCertificateDER(der_));
    ASSERT_EQ(ER_OK, cert3.Verify(key));
    qcc::String der2_;
    ASSERT_EQ(ER_OK, cert.EncodeCertificateDER(der2_));
    ASSERT_EQ(der, der2_);

    MembershipCertificate cert4;
    ASSERT_EQ(ER_OK, cert4.DecodeCertificateDER(der));
    ASSERT_EQ(ER_OK, cert4.Verify(key));
    cert4.SetGuild(cert.GetGuild());
    qcc::String der4;
    ASSERT_EQ(ER_OK, cert.EncodeCertificateDER(der4));
    ASSERT_EQ(der, der4);

    AJNCa realCa;
    ASSERT_EQ(ER_OK, realCa.Init(storeName));
    MembershipCertificate cert5;
    ASSERT_EQ(ER_OK, cert5.DecodeCertificateDER(der));
    ASSERT_EQ(ER_OK, cert5.Verify(key));
    ASSERT_EQ(ER_OK, realCa.SignCertificate(cert5));
    ASSERT_EQ(ER_OK, cert5.Verify(key));
    ASSERT_EQ(ER_OK, cert5.VerifyValidity());
    MembershipCertificate cert6;
    String der5;
    ASSERT_EQ(ER_OK, cert5.EncodeCertificateDER(der5));
    ASSERT_EQ(ER_OK, cert6.DecodeCertificateDER(der5));
    ASSERT_EQ(ER_OK, cert6.Verify(key));
}
}
