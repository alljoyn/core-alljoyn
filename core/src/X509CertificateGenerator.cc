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

#include <alljoyn/Status.h>
#include <CredentialAccessor.h>
#include <qcc/String.h>
#include <qcc/Debug.h>
#include <ctime>

#include <X509CertificateGenerator.h>

#define QCC_MODULE "SEC_MGR"

//using namespace ajn;
using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
X509CertificateGenerator::X509CertificateGenerator(qcc::String issuerCommonName,
                                                   ajn::BusAttachment* ba) :
    name(issuerCommonName), keys(new Crypto_ECC())
{
    CredentialAccessor ca(*ba);
    ECCPublicKey pubKey;
    ca.GetDSAPublicKey(pubKey);
    keys->SetDSAPublicKey(&pubKey);
    ECCPrivateKey privKey;
    ca.GetDSAPrivateKey(privKey);
    keys->SetDSAPrivateKey(&privKey);
}

X509CertificateGenerator::~X509CertificateGenerator()
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

qcc::String X509CertificateGenerator::ToASN1TimeString(uint64_t time)
{
    time_t timer = time;
    struct tm* date = localtime(&timer);
    char string[65];

    /* Dirty hack to work around timezones.*/
#if defined(QCC_OS_GROUP_WINDOWS)
    timer -= _timezone;
#else
    timer -= date->tm_gmtoff;
#endif
    date = localtime(&timer);

    /**
     * CAs conforming to this profile MUST always encode certificate
     * validity dates through the year 2049 as UTCTime (YYMMDDHHMMSSZ); certificate validity
     * dates in 2050 or later MUST be encoded as GeneralizedTime (YYYYMMDDHHMMSSZ).
     */
    const char* pattern = (date->tm_year < 150) ? //years since 1900
                          "%y%m%d%H%M%SZ" : "%Y%m%d%H%M%SZ";
    size_t result = strftime(string, 64, pattern, date);

    qcc::String asn;
    qcc::String timeString = qcc::String(string, result);
    Crypto_ASN1::Encode(asn, (date->tm_year < 150 ? "t" : "T"), &timeString);

    return asn;
}

QStatus X509CertificateGenerator::GetDerEncodedX509Certificate(qcc::String extensions,
                                                               qcc::String subjectDnASN1,
                                                               X509CertificateECC& inputCertificate)
{
    QStatus status = ER_OK;
    qcc::String OID_CommonName = OID_X509_COMMON_NAME;
    qcc::String OIDidecPublicKey = OID_EC_PUBLIC_KEY;
    qcc::String OIDprime256v1 = OID_ECC_NIST_P256_V1;
    qcc::String OID_signatureAlgorithm = OID_ECDSA_WITH_SHA256;
    qcc::String serialNumber = inputCertificate.GetSerialNumber();
    const qcc::Certificate::ValidPeriod* period = inputCertificate.GetValidity();

    qcc::String beginTime = ToASN1TimeString(period->validFrom);
    qcc::String endTime = ToASN1TimeString(period->validTo);
    const qcc::ECCPublicKey* subjectPublicKey = inputCertificate.GetSubject();

    qcc::String pubkey = L'\4';
    pubkey += String((char*)subjectPublicKey->x, qcc::ECC_COORDINATE_SZ);
    pubkey += String((char*)subjectPublicKey->y, qcc::ECC_COORDINATE_SZ);

    qcc::String tbsCertificate = ""; //The certificate to be signed.
    status = Crypto_ASN1::Encode(tbsCertificate,
                                 "(c(i)l(o)({(ou)})(RR)R((oo)b)c(R))",
                                 (uint32_t)0,
                                 (uint32_t)X509_VERTIFICATE_VERSION_V3,
                                 // i
                                 &serialNumber,
                                 // l
                                 &OID_signatureAlgorithm,
                                 // (o)
                                 &OID_CommonName,
                                 &name,
                                 // ({(ou)})
                                 &beginTime,
                                 &endTime,
                                 // (RR)
                                 &subjectDnASN1,
                                 // R
                                 &OIDidecPublicKey,
                                 &OIDprime256v1,
                                 &pubkey,
                                 pubkey.size() * 8, //The keys must be written as a BIT string. The size must be bit length of the key.
                                 // ((oo)b)
                                 //&IssuerUniqueID, // --> not needed
                                 //&SubjectUniqueID, // b
                                 (uint32_t)3,
                                 &extensions);     // R

    /* Encode Certificate */
    if (status == ER_OK) {
        qcc::ECCSignatureOldEncoding signatureOld;
        qcc::ECCSignature signature;
        Crypto_ECC ecc;
        ecc.ReEncode(&signatureOld, &signature);
        keys->DSASign((uint8_t*)tbsCertificate.data(), (uint16_t)tbsCertificate.size(), &signature);
        /* Certificate parameter */
        qcc::String SignatureValue;
        qcc::String r((const char*)signature.r, sizeof(signature.r));
        qcc::String s((const char*)signature.s, sizeof(signature.s));

        status = Crypto_ASN1::Encode(SignatureValue, "(ll)", &r, &s);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to encode signature"));
            return status;
        }
        qcc::String derCert;
        status = Crypto_ASN1::Encode(derCert, "(R(o)b)",
                                     &tbsCertificate, // R
                                     &OID_signatureAlgorithm, // (o)
                                     &SignatureValue, 8 * SignatureValue.size()); // b
        if (status  == ER_OK) {
            inputCertificate.LoadDER(derCert);
        }
    }
    return status;
}

qcc::String X509CertificateGenerator::GetConstraints(bool ca, qcc::CertificateType type)
{
    qcc::String OID_BasicConstraints = OID_X509_BASIC_CONSTRAINTS;

    /* encode basic constraints = CA = false. */
    qcc::String basicConstraints;
    if (ca) {
        Crypto_ASN1::Encode(basicConstraints, "(zi)", (uint32_t)1,  (uint32_t)0); //pathlength = 0
    } else {
        Crypto_ASN1::Encode(basicConstraints, "(z)", (uint32_t)0);
    }
    qcc::String typeOID = OID_X509_CUSTOM_AJN_CERT_TYPE;
    qcc::String typeString;
    Crypto_ASN1::Encode(typeString, "(i)", (uint32_t)type);

    qcc::String extension = "";
    Crypto_ASN1::Encode(extension,
                        "(ox)",
                        //Don't send type yet is not supported right now.
                        //"(ox)(ox)",
                        &OID_BasicConstraints, // o
                        &basicConstraints
                        //&typeOID, // o
                        //&typeString // x
                        );
    return extension;
}

QStatus X509CertificateGenerator::GenerateMembershipCertificate(qcc::X509MemberShipCertificate& certificate)
{
    qcc::String basicConstraints = GetConstraints(certificate.IsDelegate(), MEMBERSHIP_CERTIFICATE);
    qcc::String digest;
    qcc::String sha256OID = OID_SHA_256;
    qcc::String hash = certificate.GetDataDigest();
    Crypto_ASN1::Encode(digest, "(ox)", &sha256OID, &hash);
    qcc::String extensions = "";
    qcc::String digestOID = OID_X509_CUSTOM_AJN_DIGEST;

    Crypto_ASN1::Encode(extensions, "(R(ox))", &basicConstraints, &digestOID, &digest);

    qcc::String OID_CommonName = OID_X509_COMMON_NAME;
    qcc::String OID_Org_Unit = OID_X509_OUNIT_NAME;
    qcc::String subjectDn;
    qcc::String appID = certificate.GetApplicationID().ToString();
    qcc::String guildID = certificate.GetGuildId();
    Crypto_ASN1::Encode(subjectDn, "({(ou)}{(ou)})", &OID_Org_Unit, &guildID, &OID_CommonName, &appID);

    return GetDerEncodedX509Certificate(extensions, subjectDn, certificate);
}

QStatus X509CertificateGenerator::GetIdentityCertificate(qcc::X509IdentityCertificate& idCertificate)
{
    qcc::String OID_CommonName = OID_X509_COMMON_NAME;
    qcc::String basicConstraints = GetConstraints(false, IDENTITY_CERTIFICATE);
    qcc::String digest;
    qcc::String sha256OID = OID_SHA_256;
    qcc::String subjectAltNameOID = OID_X509_SUBJECT_ALT_NAME;
    qcc::String hash = idCertificate.GetDataDigest();
    qcc::String subjectAltName;
    qcc::String OID_Org_Unit = OID_X509_OUNIT_NAME;
    qcc::String idID = idCertificate.GetAlias().ToString();
    Crypto_ASN1::Encode(subjectAltName, "(c(({(ou)})))", (uint32_t)4, &OID_CommonName, &idCertificate.GetName());
    Crypto_ASN1::Encode(digest, "(ox)", &sha256OID, &hash);
    qcc::String extensions = "";
    qcc::String digestOID = OID_X509_CUSTOM_AJN_DIGEST;

    Crypto_ASN1::Encode(extensions,
                        "(R(ox)(ox))",
                        &basicConstraints,
                        &digestOID,
                        &digest,
                        &subjectAltNameOID,
                        &subjectAltName);

    qcc::String subjectDn;
    Crypto_ASN1::Encode(subjectDn,
                        "({(ou)}{(ou)})",
                        &OID_Org_Unit,
                        &idID,
                        &OID_CommonName,
                        &idCertificate.GetApplicationID().ToString());

    return GetDerEncodedX509Certificate(extensions,
                                        subjectDn,
                                        idCertificate);
}
}
}

#undef QCC_MODULE
