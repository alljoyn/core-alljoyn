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

#include <alljoyn/Status.h>
#include <qcc/String.h>
#include <qcc/Crypto.h>
#include <qcc/CryptoECC.h>
#include <qcc/Debug.h>

#include <CertificateGenerator.h>

#define QCC_MODULE "SEC_MGR"

//using namespace ajn;
using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
CertificateGenerator::CertificateGenerator(qcc::String issuerCommonName,
                                           qcc::Crypto_ECC* ca) :
    name(issuerCommonName), keys(ca)
{
}

CertificateGenerator::~CertificateGenerator()
{
    delete keys;
}

/*
 * ASN.1 of the X.509 Certificate
 *
 * Useful Doc
 * rfc5280
 * https://www.cs.auckland.ac.nz/~pgut001/pubs/x509guide.txt
 * http://luca.ntop.org/Teaching/Appunti/asn1.html
 */
/*
   --- ALJOYN NOTATION CERTIFICATE ---
   (R(on)b)
   Certificate  ::=  SEQUENCE  {
   tbsCertificate         TBSCertificate,
   signatureAlgorithm     SEQUENCE { ecdsa-with-sha256 },
   signatureValue         BIT STRING
   }

   ??? ALLJOYN NOTATION TBSCERTIFICATE ???
   (il(on)({(ou)})(tt)({(ou)})(00b)bbR)
   TBSCertificate  ::=  SEQUENCE  {
   version                v3(2),
   serialNumber           INTEGER,
   signature              SEQUENCE { ecdsa-with-sha256 },
   issuer                 Name,
   validity               Validity,
   subject                Name,
   subjectPublicKeyInfo   SEQUENCE { id-ecPublicKey, prime256v1, BIT STRING },
   issuerUniqueID         IMPLICIT UniqueIdentifier OPTIONAL,
   subjectUniqueID        IMPLICIT UniqueIdentifier OPTIONAL,
   extensions             EXPLICIT Extensions OPTIONAL
   }

   --- ALLJOYN NOTATION EXTENSIONS ---
   ((x)x(ox)(ox))
   Extensions  ::=  SEQUENCE  {
   AuthorityKeyIdentifier SEQUENCE { OCTET STRING },
   SubjectKeyIdentifier   OCTET STRING,
   IssuerAltName          SEQUENCE { OID, OCTET STRING },
   SubjectAltName         SEQUENCE { OID, OCTET STRING }
   }

   --- ALLJOYN NOTATION VALIDITY ---
   (tt)
   Validity  ::=  SEQUENCE {
   notBefore		TIME,
   notAfter		TIME
   }

   --- ALLJOYN NOTATION UNIQUEIDENTIFIER ---
   b
   UniqueIdentifier  ::=  BIT STRING

   ??? ALLJOYN NOTATION NAME ???
   ({(ou)})
   Name ::= CHOICE { -- only one possibility for now --
   rdnSequence  RDNSequence }

   RDNSequence ::= SEQUENCE OF RelativeDistinguishedName

   RelativeDistinguishedName ::=
   SET SIZE (1..MAX) OF AttributeTypeAndValue

   AttributeTypeAndValue ::= SEQUENCE {
   type     AttributeType,
   value    AttributeValue }

   AttributeType ::= OBJECT IDENTIFIER

   AttributeValue ::= ANY -- DEFINED BY AttributeType

 */
/* Encode an x509 certificate to der asn1 */
QStatus CertificateGenerator::GetPemEncodedCertificate(qcc::String extensions,
                                                       qcc::String subjectGUID,
                                                       qcc::String subjectPublicKeyInfo,
                                                       qcc::String& certificate)
{
    QStatus status = ER_OK;
    uint32_t version = 2; //TODO: replace by define X509 version
    qcc::String OID_CommonName = "2.5.4.3"; //TODO: replace by defines
    qcc::String OIDidecPublicKey = "1.2.840.10045.2.1";
    qcc::String OIDprime256v1 = "1.2.840.10045.3.1.7";
    qcc::String OID_signatureAlgorithm = "1.2.840.10045.4.3.2";
    qcc::String serialNumber = "1234567890"; //TODO refactor this !!!
    qcc::String beginTime = "140912120000.000Z";  //TODO refactor this !!!
    qcc::String endTime = "150912120000.000Z";  //TODO refactor this !!!

    qcc::String tbsCertificate; //The certificate to be signed.
    status = Crypto_ASN1::Encode(tbsCertificate,
                                 "(c(i)l(o)({(op)})(tt)({(op)})((oo)b)c(R))",
                                 (uint32_t)0,
                                 version,
                                 // i
                                 &serialNumber,
                                 // l
                                 &OID_signatureAlgorithm,
                                 // (o)
                                 &OID_CommonName,
                                 &name,
                                 // ({(op)})
                                 &beginTime,
                                 &endTime,
                                 // (tt)
                                 &OID_CommonName,
                                 &subjectGUID,
                                 // ({(op)})
                                 &OIDidecPublicKey,
                                 &OIDprime256v1,
                                 &subjectPublicKeyInfo,
                                 subjectPublicKeyInfo.size() * 8,
                                 // ((oo)b)
                                 //&IssuerUniqueID, // --> not needed
                                 //&SubjectUniqueID, // b
                                 (uint32_t)3,
                                 &extensions);     // R

    /* Encode Certificate */
    if (status == ER_OK) {
        qcc::ECCSignature signature;
        keys->DSASign((uint8_t*)tbsCertificate.data(), (uint16_t)tbsCertificate.size(), &signature);
        /* Certificate parameter */
        qcc::String SignatureValue = qcc::String((char*)signature.data, sizeof(signature.data));
        qcc::String derCert;
        status = Crypto_ASN1::Encode(derCert, "(R(o)b)",
                                     &tbsCertificate, // R
                                     &OID_signatureAlgorithm, // (o)
                                     &SignatureValue, 8 * sizeof(signature.data)); // b
        if (status  == ER_OK) {
            return qcc::Crypto_ASN1::EncodeBase64(derCert, certificate);
        }
    }
    return status;
}

QStatus CertificateGenerator::GetIdentityCertificate(qcc::String subjGUID,
                                                     qcc::String pubKey,
                                                     qcc::String& certificate)
{
    qcc::String OID_BasicConstraints = "2.5.29.19"; //TODO replace by define

    /* encode basic constraints = CA = false. */
    qcc::String basicConstraints;
    QStatus status = Crypto_ASN1::Encode(basicConstraints, "(z)", (uint32_t)0);
    if (status != ER_OK) {
        return status;
    }

    /* Encode extensions part */
    qcc::String extensions = "";
    status = Crypto_ASN1::Encode(extensions,
                                 "((ox))",
                                 &OID_BasicConstraints,    // o
                                 &basicConstraints         // x
                                 );
    if (status != ER_OK) {
        return status;
    }

    certificate += "-----BEGIN CERTIFICATE-----\n";
    status = GetPemEncodedCertificate(extensions,
                                      subjGUID,
                                      pubKey,
                                      certificate);
    certificate += "-----END CERTIFICATE-----\n";
    return status;
}
#if 0
//leftover from prototype
QStatus CertificateGenerator::Decoding(qcc::String der)
{
    qcc::String TBSCertificate;
    qcc::String SignatureOID;
    qcc::String KeyBits;
    size_t KeyBitsSize;

    /* Decode the certificate */
    QStatus status = Crypto_ASN1::Decode(der, "((.)(on)b)", &TBSCertificate,     // (.)
                                         &SignatureOID, // (on)
                                         &KeyBits, &KeyBitsSize); // b

    uint32_t Version;
    qcc::String SerialNumber;
    qcc::String Signature;
    qcc::String IssuerOID;
    qcc::String Issuer;
    qcc::String timeNow;
    qcc::String timeOneYearLater;
    qcc::String SubjectOID;
    qcc::String Subject;
    qcc::String OIDidecPublicKey;
    qcc::String OIDprime256v1;
    qcc::String SubjectPublicKeyInfo;
    size_t SubjectPublicKeyInfoSize;
    qcc::String IssuerUniqueID;
    size_t IssuerUniqueIDSize;
    qcc::String SubjectUniqueID;
    size_t SubjectUniqueIDSize;
    qcc::String Extensions;

    /* Decode the TBSCertificate */
    if (status == ER_OK) {
        status = Crypto_ASN1::Decode(TBSCertificate,
                                     "(il(on)({(ou)})(tt)({(ou)})(oob)bb.)",
                                     &Version, // i
                                     &SerialNumber, // l
                                     &Signature, // (on)
                                     &IssuerOID, &Issuer, // ({(ou)})
                                     &timeNow, &timeOneYearLater, // (tt)
                                     &SubjectOID, &Subject, // ({(ou)})
                                     &OIDidecPublicKey, &OIDprime256v1, // (oo
                                     &SubjectPublicKeyInfo, &SubjectPublicKeyInfoSize, // b)
                                     &IssuerUniqueID, &IssuerUniqueIDSize, // b
                                     &SubjectUniqueID, &SubjectUniqueIDSize, // b
                                     &Extensions); // .
    }

    qcc::String AuthorityKeyIdentifier;
    qcc::String SubjectKeyIdentifier;
    qcc::String OID_IssuerAltName;
    qcc::String IssuerAltName;
    qcc::String OID_SubjectAltName;
    qcc::String SubjectAltName;

    /* Decode the extensions */
    if (status == ER_OK) {
        status = Crypto_ASN1::Decode(Extensions,
                                     "((x)x(ox)(ox))",
                                     &AuthorityKeyIdentifier, // (x)
                                     &SubjectKeyIdentifier, // x
                                     &OID_IssuerAltName, &IssuerAltName, // (ox)
                                     &OID_SubjectAltName, &SubjectAltName); // (ox)
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("Failed decode DER into members"));
        return status;
    }
    return status;
}
#endif
}
}

#undef QCC_MODULE
