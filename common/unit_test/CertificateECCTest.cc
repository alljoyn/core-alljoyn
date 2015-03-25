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
#include <qcc/Thread.h>
#include <qcc/Crypto.h>
#include <qcc/CertificateECC.h>
#include <qcc/CertificateHelper.h>
#include <qcc/GUID.h>
#include <qcc/StringUtil.h>

using namespace qcc;
using namespace std;


#define AUTH_VERIFIER_LEN  Crypto_SHA256::DIGEST_SIZE

/* The key and certificate are generated using openssl */
static const char eccPrivateKeyPEM[] = {
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEICkeoQeosiS380hFJYo9zL1ziyTbea1mYqqqgHvGKZ6qoAoGCCqGSM49\n"
    "AwEHoUQDQgAE9jiMexU/7Z55ZQQU67Rn/MpXzAkYx5m6nQt2lWWUvWXYbOOLUBx0\n"
    "Tdw/Gy3Ia1WmLSY5ecyw1CUtHsZxjhrlcg==\n"
    "-----END EC PRIVATE KEY-----"
};

static const char eccSelfSignCertX509PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBsDCCAVagAwIBAgIJAJVJ9/7bbQcWMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDZkODVjMjkyMjYxM2IzNmUyZWVlZjUyNzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1\n"
    "YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQyY2M1NjAeFw0xNTAyMjYxODAzNDlaFw0x\n"
    "NjAyMjYxODAzNDlaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABPY4jHsVP+2eeWUEFOu0Z/zK\n"
    "V8wJGMeZup0LdpVllL1l2Gzji1AcdE3cPxstyGtVpi0mOXnMsNQlLR7GcY4a5XKj\n"
    "DTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSAAwRQIhAKrCirrUWNNAO2gFiNTl\n"
    "/ncnbELhDiDq/N43LIpfAfX8AiAKX7h/9nXEerJlthl5gUOa4xV6UjqbZLM6+KH/\n"
    "Hk/Yvw==\n"
    "-----END CERTIFICATE-----"
};
static const char eccCertChainX509PEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBtDCCAVmgAwIBAgIJAMlyFqk69v+OMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjVaFw0x\n"
    "NjAyMjYyMTUxMjVaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABL50XeH1/aKcIF1+BJtlIgjL\n"
    "AW32qoQdVOTyQg2WnM/R7pgxM2Ha0jMpksUd+JS9BiVYBBArwU76Whz9m6UyJeqj\n"
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAKfmglMgl67L5ALF\n"
    "Z63haubkItTMACY1k4ROC2q7cnVmAiEArvAmcVInOq/U5C1y2XrvJQnAdwSl/Ogr\n"
    "IizUeK0oI5c=\n"
    "-----END CERTIFICATE-----"
    "\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBszCCAVmgAwIBAgIJAILNujb37gH2MAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMjYyMTUxMjNaFw0x\n"
    "NjAyMjYyMTUxMjNaMFYxKTAnBgNVBAsMIDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBm\n"
    "NzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5\n"
    "ZGQwMjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGEkAUATvOE4uYmt/10vkTcU\n"
    "SA0C+YqHQ+fjzRASOHWIXBvpPiKgHcINtNFQsyX92L2tMT2Kn53zu+3S6UAwy6yj\n"
    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSAAwRQIgKit5yeq1uxTvdFmW\n"
    "LDeoxerqC1VqBrmyEvbp4oJfamsCIQDvMTmulW/Br/gY7GOP9H/4/BIEoR7UeAYS\n"
    "4xLyu+7OEA==\n"
    "-----END CERTIFICATE-----"
};

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

static QStatus CreateCert(const qcc::String& serial, const qcc::GUID128& issuer, const ECCPrivateKey* issuerPrivateKey, const ECCPublicKey* issuerPubKey, const qcc::GUID128& subject, const ECCPublicKey* subjectPubKey, uint32_t expiredInSeconds, CertificateX509& x509)
{
    QCC_UNUSED(issuerPubKey);

    QStatus status = ER_CRYPTO_ERROR;

    x509.SetSerial(serial);
    x509.SetIssuerCN(issuer.GetBytes(), issuer.SIZE);
    x509.SetSubjectCN(subject.GetBytes(), subject.SIZE);
    x509.SetSubjectPublicKey(subjectPubKey);
    x509.SetCA(true);
    CertificateX509::ValidPeriod validity;
    validity.validFrom = qcc::GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + expiredInSeconds;
    x509.SetValidity(&validity);
    status = x509.Sign(issuerPrivateKey);
    return status;
}

static QStatus CreateIdentityCert(qcc::GUID128& issuer, const qcc::String& serial, ECCPrivateKey* dsaPrivateKey, ECCPublicKey* dsaPublicKey, ECCPrivateKey* subjectPrivateKey, ECCPublicKey* subjectPublicKey, bool selfSign, uint32_t expiredInSeconds, CertificateX509& x509)
{
    Crypto_ECC ecc;
    ecc.GenerateDSAKeyPair();
    *dsaPrivateKey = *ecc.GetDSAPrivateKey();
    *dsaPublicKey = *ecc.GetDSAPublicKey();
    if (!selfSign) {
        ecc.GenerateDSAKeyPair();
    }
    *subjectPrivateKey = *ecc.GetDSAPrivateKey();
    *subjectPublicKey = *ecc.GetDSAPublicKey();
    qcc::GUID128 userGuid;
    return CreateCert(serial, issuer, dsaPrivateKey, dsaPublicKey, userGuid, subjectPublicKey, expiredInSeconds, x509);
}

TEST_F(CertificateECCTest, EncodePrivateKey)
{
    QStatus status;

    qcc::String encoded;
    status = CertificateX509::EncodePrivateKeyPEM((uint8_t*) ecc.GetDSAPrivateKey(), sizeof(ECCPrivateKey), encoded);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::EncodePrivateKeyPEM failed with actual status: " << QCC_StatusText(status);

    printf("The encoded private key PEM %s\n", encoded.c_str());

    ECCPrivateKey pk;
    status = CertificateX509::DecodePrivateKeyPEM(encoded, (uint8_t*) &pk, sizeof(pk));
    ASSERT_EQ(ER_OK, status) << " CertificateX509::DecodePrivateKeyPEM failed with actual status: " << QCC_StatusText(status);

    printf("Original private key %s\n", BytesToHexString((uint8_t*) ecc.GetDSAPrivateKey(), sizeof(ECCPrivateKey)).c_str());
    printf("Decoded private key %s\n", BytesToHexString((uint8_t*) &pk, sizeof(pk)).c_str());
    ASSERT_EQ(pk, *ecc.GetDSAPrivateKey()) << " decoded private key not equal to original";

    /* test buffer len */
    status = CertificateX509::DecodePrivateKeyPEM(encoded, (uint8_t*) &pk, 3);
    ASSERT_NE(ER_OK, status) << " CertificateX509::DecodePrivateKeyPEM succeeded when expected to fail.  The actual actual status: " << QCC_StatusText(status);

}

TEST_F(CertificateECCTest, DecodePrivateKey)
{
    QStatus status;

    qcc::String encoded(eccPrivateKeyPEM);

    ECCPrivateKey pk;
    status = CertificateX509::DecodePrivateKeyPEM(encoded, (uint8_t*) &pk, sizeof(pk));
    ASSERT_EQ(ER_OK, status) << " CertificateX509::DecodePrivateKeyPEM failed with actual status: " << QCC_StatusText(status);
    printf("Decoded private key %s\n", BytesToHexString((uint8_t*) &pk, sizeof(pk)).c_str());
}

TEST_F(CertificateECCTest, EncodePublicKey)
{
    QStatus status;

    qcc::String encoded;
    status = CertificateX509::EncodePublicKeyPEM((uint8_t*) ecc.GetDSAPublicKey(), sizeof(ECCPublicKey), encoded);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::EncodePublicKeyPEM failed with actual status: " << QCC_StatusText(status);

    printf("The encoded public key PEM %s\n", encoded.c_str());

    ECCPublicKey pk;
    status = CertificateX509::DecodePublicKeyPEM(encoded, (uint8_t*) &pk, sizeof(pk));
    ASSERT_EQ(ER_OK, status) << " CertificateX509::DecodePublicKeyPEM failed with actual status: " << QCC_StatusText(status);

    printf("Original public key %s\n", BytesToHexString((uint8_t*) ecc.GetDSAPublicKey(), sizeof(ECCPublicKey)).c_str());
    printf("Decoded public key %s\n", BytesToHexString((uint8_t*) &pk, sizeof(pk)).c_str());
    ASSERT_EQ(pk, *ecc.GetDSAPublicKey()) << " decoded private key not equal to original";

    /* test buffer len */
    status = CertificateX509::DecodePublicKeyPEM(encoded, (uint8_t*) &pk, 3);
    ASSERT_NE(ER_OK, status) << " CertificateX509::DecodePublicKeyPEM succeeded when expected to fail.  The actual actual status: " << QCC_StatusText(status);

}

/**
 * Use the encoded texts to put in the bbservice and bbclient files
 */
TEST_F(CertificateECCTest, GenSelfSignECCX509CertForBBservice)
{
    qcc::GUID128 issuer;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;
    CertificateX509 x509;

    /* cert expires in one year */
    QStatus status = CreateIdentityCert(issuer, "1010101", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey, true, 365 * 24 * 3600, x509);
    ASSERT_EQ(ER_OK, status) << " CreateIdentityCert failed with actual status: " << QCC_StatusText(status);

    String encodedPK;
    status = CertificateX509::EncodePrivateKeyPEM((uint8_t*) &subjectPrivateKey, sizeof(ECCPrivateKey), encodedPK);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::EncodePrivateKeyPEM failed with actual status: " << QCC_StatusText(status);

    std::cout << "The encoded subject private key PEM:" << endl << encodedPK.c_str() << endl;

    status = CertificateX509::EncodePublicKeyPEM((uint8_t*) &subjectPublicKey, sizeof(ECCPublicKey), encodedPK);
    ASSERT_EQ(ER_OK, status) << " CertificateX509::EncodePublicKeyPEM failed with actual status: " << QCC_StatusText(status);

    std::cout << "The encoded subject public key PEM:" << endl << encodedPK.c_str() << endl;

    status = x509.Verify(&dsaPublicKey);
    ASSERT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);
    String pem = x509.GetPEM();
    std::cout << "The encoded cert PEM for ECC X.509 cert:" << endl << pem.c_str() << endl;

    std::cout << "The ECC X.509 cert: " << endl << x509.ToString().c_str() << endl;
}


/**
 * Test expiry date for X509 cert
 */
TEST_F(CertificateECCTest, ExpiredX509Cert)
{
    qcc::GUID128 issuer;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;
    CertificateX509 x509;

    /* cert expires in two seconds */
    QStatus status = CreateIdentityCert(issuer, "1010101", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey, true, 2, x509);
    ASSERT_EQ(ER_OK, status) << " CreateIdentityCert failed with actual status: " << QCC_StatusText(status);

    /* verify that the cert is not yet expired */
    status = x509.Verify(&dsaPublicKey);
    ASSERT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);

    /* sleep for 3 seconds to wait for the cert expires */
    qcc::Sleep(3000);
    status = x509.Verify(&dsaPublicKey);
    ASSERT_NE(ER_OK, status) << " verify cert did not fail with actual status: " << QCC_StatusText(status);
}

/**
 * Generate certificate with expiry date past the year 2050
 */
TEST_F(CertificateECCTest, X509CertExpiresBeyond2050)
{
    qcc::GUID128 issuer;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
    ECCPrivateKey subjectPrivateKey;
    ECCPublicKey subjectPublicKey;
    CertificateX509 cert;

    /* cert expires in about 36 years */
    uint32_t expiredInSecs = 36 * 365 * 24 * 60 * 60;
    QStatus status = CreateIdentityCert(issuer, "1010101", &dsaPrivateKey, &dsaPublicKey, &subjectPrivateKey, &subjectPublicKey, true, expiredInSecs, cert);
    ASSERT_EQ(ER_OK, status) << " CreateIdentityCert failed with actual status: " << QCC_StatusText(status);
    status = cert.Verify(&dsaPublicKey);
    ASSERT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);
}

/**
 * Verify a X509 self signed certificate generated by external tool.
 */
TEST_F(CertificateECCTest, VerifyX509SelfSignExternalCert)
{
    CertificateX509 cert;
    String pem(eccSelfSignCertX509PEM);
    QStatus status = cert.LoadPEM(pem);
    ASSERT_EQ(ER_OK, status) << " load external cert PEM failed with actual status: " << QCC_StatusText(status);
    status = cert.Verify();
    ASSERT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);
}

/**
 * Verify a X509 self signed certificate generated by external tool.
 * Add check that it does not verify with a different public key.
 *
 */
TEST_F(CertificateECCTest, VerifyX509SelfSignCertPlusDoNotVerifyWithOtherKey)
{
    CertificateX509 cert;
    String pem(eccSelfSignCertX509PEM);
    QStatus status = cert.LoadPEM(pem);
    ASSERT_EQ(ER_OK, status) << " load external cert PEM failed with actual status: " << QCC_StatusText(status);
    status = cert.Verify();
    ASSERT_EQ(ER_OK, status) << " verify cert failed with actual status: " << QCC_StatusText(status);
    /* now verify with a different public key.  It is expected to fail */
    status = cert.Verify(ecc.GetDSAPublicKey());
    ASSERT_NE(ER_OK, status) << " verify cert did not fail";
}

/**
 * Verify a X509 self signed certificate generated by external tool.
 */
TEST_F(CertificateECCTest, VerifyX509ExternalCertChain)
{
    String pem(eccCertChainX509PEM);
    /* count how many certs in the chain */
    size_t count = 0;
    QStatus status = CertificateHelper::GetCertCount(pem, &count);
    ASSERT_EQ(ER_OK, status) << " count the number of certs in the chain failed with actual status: " << QCC_StatusText(status);
    ASSERT_EQ((size_t) 2, count) << " expecting two certs in the cert chain";

    CertificateX509 certs[2];
    status = CertificateX509::DecodeCertChainPEM(pem, certs, count);
    ASSERT_EQ(ER_OK, status) << " decode the cert chain failed with actual status: " << QCC_StatusText(status);
    for (size_t cnt = 0; cnt < count; cnt++) {
        printf("certs[%d]: %s\n", (int) cnt, certs[cnt].ToString().c_str());
    }
    status = certs[0].Verify(certs[1].GetSubjectPublicKey());
    ASSERT_EQ(ER_OK, status) << " verify leaf cert failed with actual status: " << QCC_StatusText(status);
}

