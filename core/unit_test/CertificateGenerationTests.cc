/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include "gtest/gtest.h"

#include <qcc/CryptoECC.h>
#include "X509CertificateGenerator.h"

#define ECDHE_KEYX "ALLJOYN_ECDHE_ECDSA"

#if 0
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
/**
 * Several X509 certificate generation tests.
 */
namespace secmgrcoretest_unit_certificatetests {
class CertificateGenerationTests :
    public::testing::Test {
  public:

    CertificateGenerationTests() :
        ecc(NULL), ba(NULL)
    {
    }

    virtual void SetUp()
    {
        ecc = new qcc::Crypto_ECC();
        ASSERT_TRUE(ecc != NULL);
        ASSERT_EQ(ecc->GenerateDSAKeyPair(), ER_OK);
        ba = new ajn::BusAttachment("testCert", true);
        ba->Start();
        ba->Connect();
        ba->EnablePeerSecurity(ECDHE_KEYX, NULL, "test_path", true);
    }

    virtual void TearDown()
    {
        ba->Disconnect();
        ba->Stop();
        ba->Join();
        delete ba;
        delete ecc;
    }

    qcc::Crypto_ECC* ecc;
    ajn::BusAttachment* ba;
};

TEST_F(CertificateGenerationTests, BasicGeneration) {
    qcc::String commonName = "MyIssuerCommonName";
    ajn::securitymgr::X509CertificateGenerator* gen = new ajn::securitymgr::X509CertificateGenerator(commonName, ba);
    ASSERT_TRUE(gen != NULL);
    ecc->GenerateDHKeyPair();

    qcc::X509IdentityCertificate certificate;
    certificate.SetApplicationID(qcc::String("myID"));
    certificate.SetDataDigest(qcc::String("129837890478923ABCDEF"));
    certificate.SetAlias(qcc::String("MyAlias"));
    certificate.SetIssuerName(commonName);
    certificate.SetSerialNumber(qcc::String("9876543210"));
    certificate.SetSubject(ecc->GetDHPublicKey());
    qcc::Certificate::ValidPeriod period;
    period.validFrom = 1412050000;
    period.validTo = 2524608000;
    certificate.SetValidity(&period);

    QStatus status = gen->GetIdentityCertificate(certificate);

    qcc::String pemCertifcate = certificate.GetDER();
#if 0
    int fd = open("log", O_RDWR | O_TRUNC | O_CREAT);
    write(fd, pemCertifcate.data(), pemCertifcate.size());
    write(1, pemCertifcate.data(), pemCertifcate.size());
    close(fd);
#endif
    ASSERT_TRUE((status == ER_OK));
}

TEST_F(CertificateGenerationTests, MembershipGeneration) {
    qcc::String commonName = "MyIssuerCommonName";
    ajn::securitymgr::X509CertificateGenerator* gen = new ajn::securitymgr::X509CertificateGenerator(commonName, ba);
    ASSERT_TRUE(gen != NULL);

    qcc::X509MemberShipCertificate certificate;
    certificate.SetApplicationID(qcc::String("myID"));
    certificate.SetDataDigest(qcc::String("129837890478923ABCDEF"));
    certificate.SetDelegate(true);
    certificate.SetGuildId(qcc::String("MyGuild"));
    certificate.SetIssuerName(commonName);
    certificate.SetSerialNumber(qcc::String("9876543210"));
    qcc::Certificate::ValidPeriod period;
    period.validFrom = 1412050000;
    period.validTo = 2524608000;
    certificate.SetValidity(&period);
    ecc->GenerateDHKeyPair();
    certificate.SetSubject(ecc->GetDHPublicKey());

    QStatus status = gen->GenerateMembershipCertificate(certificate);
#if 0
    int fd = open("membership.log", O_RDWR | O_TRUNC | O_CREAT);
    qcc::String pem = certificate.GetDER();
    write(fd, pem.data(), pem.size());
    close(fd);
#endif
    ASSERT_TRUE((status == ER_OK));
}

TEST_F(CertificateGenerationTests, GetBasicConstraintsTest) {
    qcc::String result = ajn::securitymgr::X509CertificateGenerator::GetConstraints(
        true,
        qcc::IDENTITY_CERTIFICATE);
    qcc::String oid;
    qcc::String octests;
    qcc::String typeOID;
    qcc::String typeString;
    //uint32_t type;

    qcc::String caTrue = "\u0030\u0006\u0001\u0001";
    caTrue.append(-1);
    caTrue.append((char)2); //pathlen is set to 0
    caTrue.append((char)1);
    caTrue.append((char)0);
    qcc::String caFalse = "\u0030\u0003\u0001\u0001";
    caFalse.append((char)0);
    //qcc::Crypto_ASN1::Decode(result, "(ox)(ox)", &oid, &octests, &typeOID, &typeString);
    qcc::Crypto_ASN1::Decode(result, "(ox)", &oid, &octests);
    ASSERT_EQ(oid, qcc::String(OID_X509_BASIC_CONSTRAINTS));
    ASSERT_EQ(octests, caTrue);
    //ASSERT_EQ(typeOID, qcc::String(OID_X509_CUSTOM_AJN_CERT_TYPE));
    ///ASSERT_EQ(qcc::Crypto_ASN1::Decode(typeString, "(i)", &type), ER_OK);
    //ASSERT_EQ(type, (uint32_t)1);

    result = "";
    oid = "";
    octests = "";
    typeOID = "";
    typeString = "";
    result = ajn::securitymgr::X509CertificateGenerator::GetConstraints(false,
                                                                        qcc::MEMBERSHIP_CERTIFICATE);
    //qcc::Crypto_ASN1::Decode(result, "(ox)(ox)", &oid, &octests, &typeOID, &typeString);
    qcc::Crypto_ASN1::Decode(result, "(ox)(ox)", &oid, &octests);
    ASSERT_EQ(oid, qcc::String(OID_X509_BASIC_CONSTRAINTS));
    ASSERT_EQ(octests, caFalse);
    //ASSERT_EQ(typeOID, qcc::String(OID_X509_CUSTOM_AJN_CERT_TYPE));
    //ASSERT_EQ(qcc::Crypto_ASN1::Decode(typeString, "(i)", &type), ER_OK);
    //ASSERT_EQ(type, (uint32_t)2);
}

TEST_F(CertificateGenerationTests, ToTimeStringTest) {
    ASSERT_EQ(qcc::String("\u0017\u000D140930040640Z"),
              ajn::securitymgr::X509CertificateGenerator::ToASN1TimeString(1412050000));                                               //Sep 30 04:06:40 UTC 2014
    ASSERT_EQ(qcc::String("\u0017\u000D491202092640Z"),
              ajn::securitymgr::X509CertificateGenerator::ToASN1TimeString(2522050000));                                               //Dec  2 09:26:40 UTC 2049
    ASSERT_EQ(qcc::String("\u0017\u000D491231235959Z"),
              ajn::securitymgr::X509CertificateGenerator::ToASN1TimeString(2524607999));                                               //Dec 31 23:59:59 UTC 2049
    ASSERT_EQ(qcc::String("\u0018\u000F20500101000000Z"),
              ajn::securitymgr::X509CertificateGenerator::ToASN1TimeString(2524608000));                                             //Jan  1 00:00:00 UTC 2050
    ASSERT_EQ(qcc::String("\u0018\u000F20620904150800Z"),
              ajn::securitymgr::X509CertificateGenerator::ToASN1TimeString(2924608080));                                             //Sep  4 15:08:00 UTC 2062
}
} //namespace
