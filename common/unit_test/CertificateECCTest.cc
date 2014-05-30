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
#include <qcc/CertificateECC.h>
#include <qcc/GUID.h>

using namespace qcc;
using namespace std;


class CertificateECCTest : public testing::Test {
  public:
    Crypto_ECC ecc;
    virtual void SetUp() {
        QStatus status;

        status = ecc.GenerateDSAKeyPair();
        ASSERT_EQ(ER_OK, status) << " ecc.GenerateDSAKeyPair() failed with actual status: " << QCC_StatusText(status);

        status = ecc.GenerateDHKeyPair();
        ASSERT_EQ(ER_OK, status) << " ecc.GenerateDHKeyPair() failed with actual status: " << QCC_StatusText(status);
    }

  private:


};

static QStatus GenerateCertificateType1(bool selfSign, bool regenKeys, uint32_t expiredInSecs, CertificateType1& cert, const char* msg,
                                        ECCPrivateKey* dsaPrivateKey, ECCPublicKey* dsaPublicKey, ECCPrivateKey* subjectPrivateKey, ECCPublicKey* subjectPublicKey)
{
    Crypto_ECC ecc;

    if (regenKeys) {
        ecc.GenerateDSAKeyPair();
        memcpy(dsaPrivateKey, ecc.GetDSAPrivateKey(), sizeof(ECCPrivateKey));
        memcpy(dsaPublicKey, ecc.GetDSAPublicKey(), sizeof(ECCPublicKey));
        ecc.GenerateDSAKeyPair();
        memcpy(subjectPrivateKey, ecc.GetDSAPrivateKey(), sizeof(ECCPrivateKey));
        memcpy(subjectPublicKey, ecc.GetDSAPublicKey(), sizeof(ECCPublicKey));
    }
    cert.SetIssuer(dsaPublicKey);
    if (selfSign) {
        cert.SetSubject(cert.GetIssuer());
    } else {
        cert.SetSubject(subjectPublicKey);
    }

    Timespec now;
    GetTimeNow(&now);
    Certificate::ValidPeriod valid;
    valid.validFrom = now.seconds;
    valid.validTo = valid.validFrom + expiredInSecs;


    cert.SetValidity(&valid);
    cert.SetDelegate(false);

    String externalData(msg);
    Crypto_SHA256 digestUtil;
    digestUtil.Init();
    digestUtil.Update(externalData);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    digestUtil.GetDigest(digest);

    cert.SetExternalDataDigest(digest);

    return cert.Sign(dsaPrivateKey);
}

static QStatus GenerateCertificateType1(CertificateType1& cert, const char* msg)
{
    ECCPrivateKey pk;
    ECCPublicKey pubk;
    ECCPrivateKey subjectpk;
    ECCPublicKey subjectk;
    /* not self signed and expired in 1 hour */
    return GenerateCertificateType1(false, true, 3600, cert, msg, &pk, &pubk, &subjectpk, &subjectk);
}

static QStatus GenerateCertificateType2(bool selfSign, bool regenKeys, uint32_t expiredInSecs, CertificateType2& cert, const char* msg,
                                        ECCPrivateKey* dsaPrivateKey, ECCPublicKey* dsaPublicKey, ECCPrivateKey* subjectPrivateKey, ECCPublicKey* subjectPublicKey)
{
    Crypto_ECC ecc;

    if (regenKeys) {
        ecc.GenerateDSAKeyPair();
        memcpy(dsaPrivateKey, ecc.GetDSAPrivateKey(), sizeof(ECCPrivateKey));
        memcpy(dsaPublicKey, ecc.GetDSAPublicKey(), sizeof(ECCPublicKey));
        ecc.GenerateDSAKeyPair();
        memcpy(subjectPrivateKey, ecc.GetDSAPrivateKey(), sizeof(ECCPrivateKey));
        memcpy(subjectPublicKey, ecc.GetDSAPublicKey(), sizeof(ECCPublicKey));
    }
    cert.SetIssuer(dsaPublicKey);
    if (selfSign) {
        cert.SetSubject(cert.GetIssuer());
    } else {
        cert.SetSubject(subjectPublicKey);
    }

    Timespec now;
    GetTimeNow(&now);
    Certificate::ValidPeriod valid;
    valid.validFrom = now.seconds;
    valid.validTo = valid.validFrom + expiredInSecs;


    cert.SetValidity(&valid);
    cert.SetDelegate(false);
    GUID128 aGuid;
    cert.SetGuild(aGuid.GetBytes(), GUID128::SIZE);

    String externalData(msg);
    Crypto_SHA256 digestUtil;
    digestUtil.Init();
    digestUtil.Update(externalData);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    digestUtil.GetDigest(digest);

    cert.SetExternalDataDigest(digest);

    return cert.Sign(dsaPrivateKey);
}

static QStatus GenerateCertificateType2(CertificateType2& cert, const char* msg)
{
    ECCPrivateKey pk;
    ECCPublicKey pubk;
    ECCPrivateKey subjectpk;
    ECCPublicKey subjectk;
    /* not self signed and expired in 1 hour */
    return GenerateCertificateType2(false, true, 3600, cert, msg, &pk, &pubk, &subjectpk, &subjectk);
}

TEST_F(CertificateECCTest, CertificateType1SignatureVerifies)
{
    CertificateType1 cert1(ecc.GetDSAPublicKey(), ecc.GetDHPublicKey());

    ASSERT_EQ(memcmp(ecc.GetDSAPublicKey(), cert1.GetIssuer(), sizeof(ECCPublicKey)), 0) << " cert1's issuer not equal to original";

    ASSERT_EQ(memcmp(ecc.GetDHPublicKey(), cert1.GetSubject(), sizeof(ECCPublicKey)), 0) << " cert1's subject not equal to original";

    Timespec now;
    GetTimeNow(&now);
    Certificate::ValidPeriod valid;
    valid.validFrom = now.seconds;
    valid.validTo = valid.validFrom + 3600;   /* one hour from now */

    cert1.SetValidity(&valid);
    ASSERT_EQ(cert1.GetValidity()->validFrom, valid.validFrom) << " cert1's validity.validFrom not equal to original";
    ASSERT_EQ(cert1.GetValidity()->validTo, valid.validTo) << " cert1's validity.validTo not equal to original";


    cert1.SetDelegate(true);
    EXPECT_TRUE(cert1.IsDelegate());

    String externalData("This is a test from the emergency network");
    Crypto_SHA256 digestUtil;
    digestUtil.Init();
    digestUtil.Update(externalData);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    digestUtil.GetDigest(digest);

    cert1.SetExternalDataDigest(digest);

    ASSERT_EQ(memcmp(cert1.GetExternalDataDigest(), digest, sizeof(digest)), 0) << " cert1's digest not equal to original";

    QStatus status = cert1.Sign(ecc.GetDSAPrivateKey());
    ASSERT_EQ(ER_OK, status) << " cert1.Sign() failed with actual status: " << QCC_StatusText(status);

    EXPECT_TRUE(cert1.VerifySignature());

    std::cout << cert1.ToString().c_str() << endl;
}

TEST_F(CertificateECCTest, CertificateType1SignatureNotVerified)
{

    CertificateType1 cert1(ecc.GetDSAPublicKey(), ecc.GetDHPublicKey());

    ASSERT_EQ(memcmp(ecc.GetDSAPublicKey(), cert1.GetIssuer(), sizeof(ECCPublicKey)), 0) << " cert1's issuer not equal to original";

    ASSERT_EQ(memcmp(ecc.GetDHPublicKey(), cert1.GetSubject(), sizeof(ECCPublicKey)), 0) << " cert1's subject not equal to original";

    Timespec now;
    GetTimeNow(&now);
    Certificate::ValidPeriod valid;
    valid.validFrom = now.seconds;
    valid.validTo = valid.validFrom + 3600;   /* one hour from now */

    cert1.SetValidity(&valid);
    ASSERT_EQ(cert1.GetValidity()->validFrom, valid.validFrom) << " cert1's validity.validFrom not equal to original";
    ASSERT_EQ(cert1.GetValidity()->validTo, valid.validTo) << " cert1's validity.validTo not equal to original";


    cert1.SetDelegate(true);
    EXPECT_TRUE(cert1.IsDelegate());

    String externalData("The quick brown fox jumps over the lazy dog");
    Crypto_SHA256 digestUtil;
    digestUtil.Init();
    digestUtil.Update(externalData);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    digestUtil.GetDigest(digest);

    cert1.SetExternalDataDigest(digest);

    ASSERT_EQ(memcmp(cert1.GetExternalDataDigest(), digest, sizeof(digest)), 0) << " cert1's digest not equal to original";

    ECCSignature garbage;
    cert1.SetSig(&garbage);

    EXPECT_TRUE(!cert1.VerifySignature());

    std::cout << cert1.ToString().c_str() << endl;
}

TEST_F(CertificateECCTest, CertificateType2SignatureVerifies)
{
    CertificateType2 cert2(ecc.GetDSAPublicKey(), ecc.GetDHPublicKey());

    ASSERT_EQ(memcmp(ecc.GetDSAPublicKey(), cert2.GetIssuer(), sizeof(ECCPublicKey)), 0) << " cert2's issuer not equal to original";

    ASSERT_EQ(memcmp(ecc.GetDHPublicKey(), cert2.GetSubject(), sizeof(ECCPublicKey)), 0) << " cert2's subject not equal to original";

    Timespec now;
    GetTimeNow(&now);
    Certificate::ValidPeriod valid;
    valid.validFrom = now.seconds;
    valid.validTo = valid.validFrom + 3600;   /* one hour from now */

    cert2.SetValidity(&valid);
    ASSERT_EQ(cert2.GetValidity()->validFrom, valid.validFrom) << " cert2's validity.validFrom not equal to original";
    ASSERT_EQ(cert2.GetValidity()->validTo, valid.validTo) << " cert2's validity.validTo not equal to original";

    EXPECT_FALSE(cert2.IsDelegate());

    GUID128 aGuid;
    cert2.SetGuild(aGuid.GetBytes(), GUID128::SIZE);
    ASSERT_EQ(memcmp(cert2.GetGuild(), aGuid.GetBytes(), GUID128::SIZE), 0) << " cert2's guild not equal to original";

    String externalData("This is a test from the emergency network");
    Crypto_SHA256 digestUtil;
    digestUtil.Init();
    digestUtil.Update(externalData);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    digestUtil.GetDigest(digest);

    cert2.SetExternalDataDigest(digest);

    ASSERT_EQ(memcmp(cert2.GetExternalDataDigest(), digest, sizeof(digest)), 0) << " cert2's digest not equal to original";

    QStatus status = cert2.Sign(ecc.GetDSAPrivateKey());
    ASSERT_EQ(ER_OK, status) << " cert2.Sign() failed with actual status: " << QCC_StatusText(status);

    EXPECT_TRUE(cert2.VerifySignature());

    std::cout << cert2.ToString().c_str() << endl;
}

TEST_F(CertificateECCTest, CertificateType2FailsSignatureVerification)
{
    CertificateType2 cert2(ecc.GetDSAPublicKey(), ecc.GetDHPublicKey());

    ASSERT_EQ(memcmp(ecc.GetDSAPublicKey(), cert2.GetIssuer(), sizeof(ECCPublicKey)), 0) << " cert2's issuer not equal to original";

    ASSERT_EQ(memcmp(ecc.GetDHPublicKey(), cert2.GetSubject(), sizeof(ECCPublicKey)), 0) << " cert2's subject not equal to original";

    Timespec now;
    GetTimeNow(&now);
    Certificate::ValidPeriod valid;
    valid.validFrom = now.seconds;
    valid.validTo = valid.validFrom + 3600;   /* one hour from now */

    cert2.SetValidity(&valid);
    ASSERT_EQ(cert2.GetValidity()->validFrom, valid.validFrom) << " cert2's validity.validFrom not equal to original";
    ASSERT_EQ(cert2.GetValidity()->validTo, valid.validTo) << " cert2's validity.validTo not equal to original";


    cert2.SetDelegate(true);
    EXPECT_TRUE(cert2.IsDelegate());

    GUID128 aGuid;
    cert2.SetGuild(aGuid.GetBytes(), GUID128::SIZE);
    ASSERT_EQ(memcmp(cert2.GetGuild(), aGuid.GetBytes(), GUID128::SIZE), 0) << " cert2's guild not equal to original";
    String externalData("The quick brown fox jumps over the lazy dog");
    Crypto_SHA256 digestUtil;
    digestUtil.Init();
    digestUtil.Update(externalData);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    digestUtil.GetDigest(digest);

    cert2.SetExternalDataDigest(digest);

    ASSERT_EQ(memcmp(cert2.GetExternalDataDigest(), digest, sizeof(digest)), 0) << " cert1's digest not equal to original";

    ECCSignature garbage;
    cert2.SetSig(&garbage);

    EXPECT_TRUE(!cert2.VerifySignature());

    std::cout << cert2.ToString().c_str() << endl;
}

TEST_F(CertificateECCTest, LoadCertificateType1)
{
    QStatus status;

    CertificateType1 cert1(ecc.GetDSAPublicKey(), ecc.GetDHPublicKey());

    Timespec now;
    GetTimeNow(&now);
    Certificate::ValidPeriod valid;
    valid.validFrom = now.seconds;
    valid.validTo = valid.validFrom + 3600;   /* one hour from now */

    cert1.SetValidity(&valid);
    cert1.SetDelegate(true);

    String externalData("This is a test from generate encoded cert");
    Crypto_SHA256 digestUtil;
    digestUtil.Init();
    digestUtil.Update(externalData);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    digestUtil.GetDigest(digest);

    cert1.SetExternalDataDigest(digest);

    status = cert1.Sign(ecc.GetDSAPrivateKey());

    const uint8_t* encoded = cert1.GetEncoded();
    CertificateType1 cert2;

    status = cert2.LoadEncoded(encoded, cert1.GetEncodedLen());
    ASSERT_EQ(ER_OK, status) << " CertificateType1::LoadEncoded failed with actual status: " << QCC_StatusText(status);

    ASSERT_EQ(memcmp(cert2.GetIssuer(), cert1.GetIssuer(), sizeof(ECCPublicKey)), 0) << " new cert's issuer not equal to original";
    ASSERT_EQ(memcmp(cert2.GetSubject(), cert1.GetSubject(), sizeof(ECCPublicKey)), 0) << " new cert's subject not equal to original";
    ASSERT_EQ(cert2.GetValidity()->validFrom, cert1.GetValidity()->validFrom) << " new cert's validity.validFrom not equal to original";
    ASSERT_EQ(cert2.GetValidity()->validTo, cert1.GetValidity()->validTo) << " new cert's validity.validTo not equal to original";
    EXPECT_TRUE(cert2.IsDelegate());
    ASSERT_EQ(memcmp(cert2.GetExternalDataDigest(), cert1.GetExternalDataDigest(), sizeof(digest)), 0) << " new cert's digest not equal to original";

    EXPECT_TRUE(cert2.VerifySignature());
    ASSERT_EQ(memcmp(cert2.GetSig(), cert1.GetSig(), sizeof(ECCSignature)), 0) << " new cert's signature not equal to original";

    std::cout << "Original cert: " << cert1.ToString().c_str() << endl;
    std::cout << "New cert loaded from encoded string: " << cert2.ToString().c_str() << endl;
}

TEST_F(CertificateECCTest, LoadCertificateType1PEM)
{
    QStatus status;

    CertificateType1 cert1(ecc.GetDSAPublicKey(), ecc.GetDHPublicKey());

    Timespec now;
    GetTimeNow(&now);
    Certificate::ValidPeriod valid;
    valid.validFrom = now.seconds;
    valid.validTo = valid.validFrom + 3600;   /* one hour from now */

    cert1.SetValidity(&valid);
    cert1.SetDelegate(true);

    String externalData("This is a test from generate encoded cert");
    Crypto_SHA256 digestUtil;
    digestUtil.Init();
    digestUtil.Update(externalData);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    digestUtil.GetDigest(digest);

    cert1.SetExternalDataDigest(digest);

    status = cert1.Sign(ecc.GetDSAPrivateKey());

    String pem = cert1.GetPEM();
    CertificateType1 cert2;

    status = cert2.LoadPEM(pem);
    ASSERT_EQ(ER_OK, status) << " CertificateType1::LoadPEM failed with actual status: " << QCC_StatusText(status);

    ASSERT_EQ(memcmp(cert2.GetIssuer(), cert1.GetIssuer(), sizeof(ECCPublicKey)), 0) << " new cert's issuer not equal to original";
    ASSERT_EQ(memcmp(cert2.GetSubject(), cert1.GetSubject(), sizeof(ECCPublicKey)), 0) << " new cert's subject not equal to original";
    ASSERT_EQ(cert2.GetValidity()->validFrom, cert1.GetValidity()->validFrom) << " new cert's validity.validFrom not equal to original";
    ASSERT_EQ(cert2.GetValidity()->validTo, cert1.GetValidity()->validTo) << " new cert's validity.validTo not equal to original";
    EXPECT_TRUE(cert2.IsDelegate());
    ASSERT_EQ(memcmp(cert2.GetExternalDataDigest(), cert1.GetExternalDataDigest(), sizeof(digest)), 0) << " new cert's digest not equal to original";

    EXPECT_TRUE(cert2.VerifySignature());
    ASSERT_EQ(memcmp(cert2.GetSig(), cert1.GetSig(), sizeof(ECCSignature)), 0) << " new cert's signature not equal to original";

    std::cout << "Original cert: " << cert1.ToString().c_str() << endl;
    std::cout << "New cert loaded from encoded string: " << cert2.ToString().c_str() << endl;
}

TEST_F(CertificateECCTest, LoadCertificateType2)
{
    QStatus status;
    CertificateType2 cert1(ecc.GetDSAPublicKey(), ecc.GetDHPublicKey());

    Timespec now;
    GetTimeNow(&now);
    Certificate::ValidPeriod valid;
    valid.validFrom = now.seconds;
    valid.validTo = valid.validFrom + 3600;   /* one hour from now */

    cert1.SetValidity(&valid);
    cert1.SetDelegate(true);
    GUID128 aGuid;
    cert1.SetGuild(aGuid.GetBytes(), GUID128::SIZE);

    String externalData("This is a test from generate encoded cert");
    Crypto_SHA256 digestUtil;
    digestUtil.Init();
    digestUtil.Update(externalData);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    digestUtil.GetDigest(digest);

    cert1.SetExternalDataDigest(digest);

    status = cert1.Sign(ecc.GetDSAPrivateKey());

    const uint8_t* encoded = cert1.GetEncoded();
    CertificateType2 cert2;

    status = cert2.LoadEncoded(encoded, cert1.GetEncodedLen());
    ASSERT_EQ(ER_OK, status) << " CertificateType2::LoadEncoded failed with actual status: " << QCC_StatusText(status);

    ASSERT_EQ(memcmp(cert2.GetIssuer(), cert1.GetIssuer(), sizeof(ECCPublicKey)), 0) << " new cert's issuer not equal to original";
    ASSERT_EQ(memcmp(cert2.GetSubject(), cert1.GetSubject(), sizeof(ECCPublicKey)), 0) << " new cert's subject not equal to original";
    ASSERT_EQ(cert2.GetValidity()->validFrom, cert1.GetValidity()->validFrom) << " new cert's validity.validFrom not equal to original";
    ASSERT_EQ(cert2.GetValidity()->validTo, cert1.GetValidity()->validTo) << " new cert's validity.validTo not equal to original";
    EXPECT_TRUE(cert2.IsDelegate());
    ASSERT_EQ(memcmp(cert2.GetGuild(), cert1.GetGuild(), GUID128::SIZE), 0) << " new cert's guild not equal to original";
    ASSERT_EQ(memcmp(cert2.GetExternalDataDigest(), cert1.GetExternalDataDigest(), sizeof(digest)), 0) << " new cert's digest not equal to original";

    EXPECT_TRUE(cert2.VerifySignature());
    ASSERT_EQ(memcmp(cert2.GetSig(), cert1.GetSig(), sizeof(ECCSignature)), 0) << " new cert's signature not equal to original";

    std::cout << "Original cert: " << cert1.ToString().c_str() << endl;
    std::cout << "New cert loaded from encoded string: " << cert2.ToString().c_str() << endl;
}

TEST_F(CertificateECCTest, LoadCertificateType2PEM)
{
    QStatus status;
    CertificateType2 cert1(ecc.GetDSAPublicKey(), ecc.GetDHPublicKey());

    Timespec now;
    GetTimeNow(&now);
    Certificate::ValidPeriod valid;
    valid.validFrom = now.seconds;
    valid.validTo = valid.validFrom + 3600;   /* one hour from now */

    cert1.SetValidity(&valid);
    cert1.SetDelegate(true);
    GUID128 aGuid;
    cert1.SetGuild(aGuid.GetBytes(), GUID128::SIZE);

    String externalData("This is a test from generate encoded cert");
    Crypto_SHA256 digestUtil;
    digestUtil.Init();
    digestUtil.Update(externalData);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    digestUtil.GetDigest(digest);

    cert1.SetExternalDataDigest(digest);

    status = cert1.Sign(ecc.GetDSAPrivateKey());

    String pem = cert1.GetPEM();
    CertificateType2 cert2;

    status = cert2.LoadPEM(pem);
    ASSERT_EQ(ER_OK, status) << " CertificateType2::LoadPEM failed with actual status: " << QCC_StatusText(status);

    ASSERT_EQ(memcmp(cert2.GetIssuer(), cert1.GetIssuer(), sizeof(ECCPublicKey)), 0) << " new cert's issuer not equal to original";
    ASSERT_EQ(memcmp(cert2.GetSubject(), cert1.GetSubject(), sizeof(ECCPublicKey)), 0) << " new cert's subject not equal to original";
    ASSERT_EQ(cert2.GetValidity()->validFrom, cert1.GetValidity()->validFrom) << " new cert's validity.validFrom not equal to original";
    ASSERT_EQ(cert2.GetValidity()->validTo, cert1.GetValidity()->validTo) << " new cert's validity.validTo not equal to original";
    EXPECT_TRUE(cert2.IsDelegate());
    ASSERT_EQ(memcmp(cert2.GetGuild(), cert1.GetGuild(), GUID128::SIZE), 0) << " new cert's guild not equal to original";
    ASSERT_EQ(memcmp(cert2.GetExternalDataDigest(), cert1.GetExternalDataDigest(), sizeof(digest)), 0) << " new cert's digest not equal to original";

    EXPECT_TRUE(cert2.VerifySignature());
    ASSERT_EQ(memcmp(cert2.GetSig(), cert1.GetSig(), sizeof(ECCSignature)), 0) << " new cert's signature not equal to original";

    std::cout << "Original cert: " << cert1.ToString().c_str() << endl;
    std::cout << "New cert loaded from encoded string: " << cert2.ToString().c_str() << endl;
}

TEST_F(CertificateECCTest, EncodePrivateKey)
{
    QStatus status;

    qcc::String encoded;
    status = CertECCUtil_EncodePrivateKey((uint32_t*) ecc.GetDSAPrivateKey(), sizeof(ECCPrivateKey), encoded);
    ASSERT_EQ(ER_OK, status) << " CertECCUtil_EncodePrivateKey failed with actual status: " << QCC_StatusText(status);

    printf("The encoded private key PEM %s\n", encoded.c_str());

    ECCPrivateKey pk;
    status = CertECCUtil_DecodePrivateKey(encoded, (uint32_t*) &pk, sizeof(pk));
    ASSERT_EQ(ER_OK, status) << " CertECCUtil_DecodePrivateKey failed with actual status: " << QCC_StatusText(status);

    printf("Original private key %s\n", BytesToHexString((uint8_t*) ecc.GetDSAPrivateKey(), sizeof(ECCPrivateKey)).c_str());
    printf("Decoded private key %s\n", BytesToHexString((uint8_t*) &pk, sizeof(pk)).c_str());
    ASSERT_EQ(memcmp(ecc.GetDSAPrivateKey(), &pk, sizeof(ECCPrivateKey)), 0) << " decoded private key not equal to original";

    /* test buffer len */
    status = CertECCUtil_DecodePrivateKey(encoded, (uint32_t*) &pk, 3);
    ASSERT_NE(ER_OK, status) << " CertECCUtil_DecodePrivateKey succeeded when expected to fail.  The actual actual status: " << QCC_StatusText(status);

}

TEST_F(CertificateECCTest, CompareWithWrongPEM)
{
    QStatus status;

    qcc::String pkEncoded;
    status = CertECCUtil_EncodePrivateKey((uint32_t*) ecc.GetDSAPrivateKey(), sizeof(ECCPrivateKey), pkEncoded);
    ASSERT_EQ(ER_OK, status) << " CertECCUtil_EncodePrivateKey failed with actual status: " << QCC_StatusText(status);

    printf("The encoded private key PEM %s\n", pkEncoded.c_str());

    CertificateType1 cert1(ecc.GetDSAPublicKey(), ecc.GetDHPublicKey());

    Timespec now;
    GetTimeNow(&now);
    Certificate::ValidPeriod valid;
    valid.validFrom = now.seconds;
    valid.validTo = valid.validFrom + 3600;   /* one hour from now */

    cert1.SetValidity(&valid);
    cert1.SetDelegate(true);

    String externalData("This is a test from generate encoded cert");
    Crypto_SHA256 digestUtil;
    digestUtil.Init();
    digestUtil.Update(externalData);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    digestUtil.GetDigest(digest);

    cert1.SetExternalDataDigest(digest);

    status = cert1.Sign(ecc.GetDSAPrivateKey());

    String certPEM = cert1.GetPEM();

    /* test decode with the wrong PEM */
    CertificateType1 cert2;
    status = cert2.LoadPEM(pkEncoded);
    ASSERT_NE(ER_OK, status) << " cert2.LoadEncoded succeeded when expected to fail.  The actual actual status: " << QCC_StatusText(status);

    ECCPrivateKey pk;
    status = CertECCUtil_DecodePrivateKey(certPEM, (uint32_t* ) &pk, sizeof(pk));
    ASSERT_NE(ER_OK, status) << " CertECCUtil_DecodePrivateKey succeeded when expected to fail.  The actual actual status: " << QCC_StatusText(status);
}

TEST_F(CertificateECCTest, EncodePublicKey)
{
    QStatus status;

    qcc::String encoded;
    status = CertECCUtil_EncodePublicKey((uint32_t*) ecc.GetDSAPublicKey(), sizeof(ECCPublicKey), encoded);
    ASSERT_EQ(ER_OK, status) << " CertECCUtil_EncodePublicKey failed with actual status: " << QCC_StatusText(status);

    printf("The encoded public key PEM %s\n", encoded.c_str());

    ECCPublicKey pk;
    memset(&pk, 0, sizeof(ECCPublicKey));
    status = CertECCUtil_DecodePublicKey(encoded, (uint32_t*) &pk, sizeof(pk));
    ASSERT_EQ(ER_OK, status) << " CertECCUtil_DecodePublicKey failed with actual status: " << QCC_StatusText(status);

    printf("Original public key %s\n", BytesToHexString((uint8_t*) ecc.GetDSAPublicKey(), sizeof(ECCPublicKey)).c_str());
    printf("Decoded public key %s\n", BytesToHexString((uint8_t*) &pk, sizeof(pk)).c_str());
    ASSERT_EQ(memcmp(ecc.GetDSAPublicKey(), &pk, sizeof(ECCPublicKey)), 0) << " decoded private key not equal to original";

    /* test buffer len */
    status = CertECCUtil_DecodePublicKey(encoded, (uint32_t*) &pk, 3);
    ASSERT_NE(ER_OK, status) << " CertECCUtil_DecodePublicKey succeeded when expected to fail.  The actual actual status: " << QCC_StatusText(status);

}



TEST_F(CertificateECCTest, GenerateKeyPairs)
{
    CertificateType1 cert;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;
    uint32_t expiresInSecsFromNow = 300; /* this key expires in 5 minutes */
    QStatus status = GenerateCertificateType1(true, true, expiresInSecsFromNow, cert, "Sample ECDSA KeyPair", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey);
    ASSERT_EQ(ER_OK, status) << " GenereateCertificateType1 failed with actual status: " << QCC_StatusText(status);

    String encodedPK;
    status = CertECCUtil_EncodePrivateKey((uint32_t*) &dsaPrivateKey, sizeof(ECCPrivateKey), encodedPK);
    ASSERT_EQ(ER_OK, status) << " CertECCUtil_EncodePrivateKey failed with actual status: " << QCC_StatusText(status);

    std::cout << "The encoded private key PEM:" << endl << encodedPK.c_str() << endl;

    String pem = cert.GetPEM();
    std::cout << "The encoded cert PEM:" << endl << pem.c_str() << endl;

    std::cout << "The cert: " << endl << cert.ToString().c_str() << endl;

}

TEST_F(CertificateECCTest, GenerateCertChain)
{
    CertificateType1 cert1;
    CertificateType2 cert2;
    QStatus status = GenerateCertificateType1(cert1, "SUCCESS_GetLeafCert 1");;
    ASSERT_EQ(ER_OK, status) << " GenereateCertificateType1 failed with actual status: " << QCC_StatusText(status);
    status = GenerateCertificateType2(cert2, "SUCCESS_GetLeafCert 2");;
    ASSERT_EQ(ER_OK, status) << " GenereateCertificateType2 failed with actual status: " << QCC_StatusText(status);

    String pem = cert1.GetPEM();
    pem += "\n";
    pem += cert2.GetPEM();

    size_t count = 0;
    status = CertECCUtil_GetCertCount(pem, &count);
    ASSERT_EQ(ER_OK, status) << " CertECCUtil_GetCertCount failed with actual status: " << QCC_StatusText(status);
    ASSERT_EQ(2U, count) << " CertECCUtil_GetCertCount failed to count certs: ";

    cout << "Calling CertECCUtil_GetCertChain" << endl;
    CertificateECC** certChain = new CertificateECC * [count];
    status = CertECCUtil_GetCertChain(pem, certChain, count);
    if (status == ER_OK) {
        std::cout << "The cert chain:" << endl;
        for (size_t idx = 0; idx < count; idx++) {
            std::cout << certChain[idx]->ToString().c_str() << endl;
            delete certChain[idx];
        }
    }
    delete [] certChain;
    ASSERT_EQ(ER_OK, status) << " CertECCUtil_GetCertChain failed with actual status: " << QCC_StatusText(status);
}

/**
 * Use the encoded texts to put in the bbservice and bbclient files
 */
TEST_F(CertificateECCTest, GenCertForBBservice)
{
    CertificateType1 cert1;
    CertificateType2 cert2;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;
    uint32_t expiresInSecsFromNow = 300; /* this key expires in 5 minutes.  Feel free to change it to fit your needs */

    QStatus status = GenerateCertificateType1(false, true, expiresInSecsFromNow, cert1, "Sample Certificate Type 1", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey);
    ASSERT_EQ(ER_OK, status) << " GenereateCertificateType1 failed with actual status: " << QCC_StatusText(status);

    String encodedPK;
    status = CertECCUtil_EncodePrivateKey((uint32_t*) &subjectPrivateKey, sizeof(ECCPrivateKey), encodedPK);
    ASSERT_EQ(ER_OK, status) << " CertECCUtil_EncodePrivateKey failed with actual status: " << QCC_StatusText(status);

    std::cout << "The encoded private key PEM:" << endl << encodedPK.c_str() << endl;

    EXPECT_TRUE(cert1.VerifySignature());
    String pem1 = cert1.GetPEM();
    std::cout << "The encoded cert PEM for cert1:" << endl << pem1.c_str() << endl;

    std::cout << "The cert1: " << endl << cert1.ToString().c_str() << endl;


    /* build cert2 of type2 with the same subject and signed by the same issuer as cert1*/

    status = GenerateCertificateType2(false, false, expiresInSecsFromNow, cert2, "Sample Certificate Type 2", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey);
    ASSERT_EQ(ER_OK, status) << " GenereateCertificateType2 failed with actual status: " << QCC_StatusText(status);
    EXPECT_TRUE(cert2.VerifySignature());

    String pem2 = cert2.GetPEM();
    std::cout << "The encoded cert PEM for cert2:" << endl << pem2.c_str() << endl;

    std::cout << "The cert2: " << endl << cert2.ToString().c_str() << endl;

}
