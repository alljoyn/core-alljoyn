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

#ifndef X509CERTIFICATEGENERATOR_H_
#define X509CERTIFICATEGENERATOR_H_

#include <qcc/String.h>
#include <qcc/Crypto.h>
#include <qcc/CryptoECC.h>

#include <qcc/Debug.h>
#include <qcc/X509Certificate.h>

#define QCC_MODULE "SEC_MGR"

#define X509_VERTIFICATE_VERSION_V3 2

#define OID_X509_BASIC_CONSTRAINTS "2.5.29.19"
#define OID_X509_COMMON_NAME "2.5.4.3"
#define OID_X509_OUNIT_NAME "2.5.4.11"
#define OID_X509_SUBJECT_ALT_NAME "2.5.29.17"

#define OID_X509_CUSTOM_AJN_DIGEST "1.2.3.4.5.6.7.8.1"
#define OID_X509_CUSTOM_AJN_CERT_TYPE "1.2.3.4.5.6.7.8.2"

#define OID_EC_PUBLIC_KEY "1.2.840.10045.2.1"
#define OID_ECC_NIST_P256_V1 "1.2.840.10045.3.1.7"
#define OID_ECDSA_WITH_SHA256 "1.2.840.10045.4.3.2"

#define OID_SHA_256 "2.16.840.1.101.3.4.2.1"

namespace ajn {
namespace securitymgr {
class X509CertificateGenerator {
  private:
    qcc::String name;
    qcc::Crypto_ECC* keys;

    static QStatus Decoding(qcc::String der);

    QStatus GetPemEncodedX509Certificate(qcc::String extensions,
                                         qcc::String subjectGUID,
                                         qcc::X509CertificateECC& certificate);

  public:
    X509CertificateGenerator(qcc::String issuerCommonName,
                             qcc::Crypto_ECC* ca);
    ~X509CertificateGenerator();

    QStatus GetIdentityCertificate(qcc::IdentityCertificate& certificate);

    QStatus GenerateMembershipCertificate(qcc::MemberShipCertificate& certificate);

    /** public scope to facilitate testing. */
    static qcc::String GetConstraints(bool ca,
                                      qcc::CertificateType type);

    /** public scope to facilitate testing. */
    static qcc::String ToASN1TimeString(uint64_t time);
};
}
}

#undef QCC_MODULE

#endif /* X509CERTIFICATEGENERATOR_H_ */
