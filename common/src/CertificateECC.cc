/**
 * @file CertificateECC.cc
 *
 * Utilites for SPKI-style Certificates
 */
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

#include <qcc/platform.h>
#include <qcc/Crypto.h>
#include <qcc/CertificateECC.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

namespace qcc {

/**
 * The X.509 Version 3
 */
#define X509_VERSION_3  2

/**
 * The X.509 OIDs
 */
const qcc::String OID_SIG_ECDSA_SHA256 = "1.2.840.10045.4.3.2";
const qcc::String OID_KEY_ECC = "1.2.840.10045.2.1";
const qcc::String OID_CRV_PRIME256V1 = "1.2.840.10045.3.1.7";
const qcc::String OID_DN_OU = "2.5.4.11";
const qcc::String OID_DN_CN = "2.5.4.3";
const qcc::String OID_SUB_ALTNAME = "2.5.29.17";
const qcc::String OID_BASIC_CONSTRAINTS = "2.5.29.19";
const qcc::String OID_DIG_SHA256 = "2.16.840.1.101.3.4.2.1";
const qcc::String OID_CUSTOM_CERT_TYPE = "1.3.6.1.4.1.44924.1.1";
const qcc::String OID_CUSTOM_DIGEST = "1.3.6.1.4.1.44924.1.2";

#define CERT_TYPE0_SIGN_LEN Crypto_SHA256::DIGEST_SIZE
#define CERT_TYPE0_RAW_LEN sizeof(uint32_t) + sizeof(ECCPublicKeyOldEncoding) + CERT_TYPE0_SIGN_LEN + sizeof(ECCSignatureOldEncoding)

#define CERT_TYPE1_SIGN_LEN sizeof(uint32_t) + 2 * sizeof(ECCPublicKeyOldEncoding) + sizeof(ValidPeriod) + sizeof (uint8_t) + Crypto_SHA256::DIGEST_SIZE
#define CERT_TYPE1_RAW_LEN CERT_TYPE1_SIGN_LEN + sizeof(ECCSignatureOldEncoding)

#define CERT_TYPE2_SIGN_LEN sizeof(uint32_t) + 2 * sizeof(ECCPublicKeyOldEncoding) + sizeof(ValidPeriod) + sizeof (uint8_t) + GUILD_ID_LEN + Crypto_SHA256::DIGEST_SIZE
#define CERT_TYPE2_RAW_LEN CERT_TYPE2_SIGN_LEN + sizeof(ECCSignatureOldEncoding)

static qcc::String EncodeRawByte(const uint8_t* raw, size_t rawLen, const char* beginToken, const char* endToken)
{
    qcc::String str;
    qcc::String rawStr((const char*) raw, rawLen);
    qcc::String base64;

    QStatus status = Crypto_ASN1::EncodeBase64(rawStr, base64);
    if (status != ER_OK) {
        return "";
    }

    str += beginToken;
    str += base64;
    str += endToken;
    return str;
}

static qcc::String EncodeCertRawByte(const uint8_t* raw, size_t rawLen)
{
    return EncodeRawByte(raw, rawLen, "-----BEGIN CERTIFICATE-----",
                         "-----END CERTIFICATE-----");
}

static QStatus CountNumOfChunksFromEncoded(const String& encoded, const char* beginToken, const char* endToken, size_t* count)
{

    size_t pos;

    *count = 0;
    qcc::String remainder = encoded;
    while (true) {
        pos = remainder.find(beginToken);
        if (pos == qcc::String::npos) {
            /* no more */
            return ER_OK;
        }
        remainder = remainder.substr(pos + strlen(beginToken));
        pos = remainder.find(endToken);
        if (pos == qcc::String::npos) {
            return ER_OK;
        }
        *count += 1;
        remainder = remainder.substr(pos + strlen(endToken));
    }
    return ER_OK;
}

static QStatus RetrieveNumOfChunksFromPEM(const String& encoded, const char* beginToken, const char* endToken, qcc::String chunks[],  size_t count)
{
    size_t pos;

    qcc::String remainder = encoded;
    for (size_t idx = 0; idx < count; idx++) {
        pos = remainder.find(beginToken);
        if (pos == qcc::String::npos) {
            /* no more */
            return ER_OK;
        }
        remainder = remainder.substr(pos + strlen(beginToken));
        pos = remainder.find(endToken);
        if (pos == qcc::String::npos) {
            return ER_OK;
        }
        chunks[idx] = beginToken;
        chunks[idx] += remainder.substr(0, pos);
        chunks[idx] += endToken;
        remainder = remainder.substr(pos + strlen(endToken));
    }
    return ER_OK;
}


static QStatus RetrieveRawFromPEM(const String& pem, const char* beginToken, const char* endToken, String& rawStr)
{
    qcc::String base64;

    size_t pos;

    pos = pem.find(beginToken);
    if (pos == qcc::String::npos) {
        return ER_INVALID_DATA;
    }
    qcc::String remainder = pem.substr(pos + strlen(beginToken));
    pos = remainder.find(endToken);
    if (pos == qcc::String::npos) {
        base64 = remainder;
    } else {
        base64 = remainder.substr(0, pos);
    }
    return Crypto_ASN1::DecodeBase64(base64, rawStr);
}

static QStatus RetrieveRawCertFromPEM(const String& pem, String& rawStr)
{
    return RetrieveRawFromPEM(pem,
                              "-----BEGIN CERTIFICATE-----",
                              "-----END CERTIFICATE-----", rawStr);
}

static QStatus DecodeKeyFromPEM(const String& pem, uint32_t* key, size_t len, const char* beginToken, const char* endToken)
{
    String rawStr;
    QStatus status = RetrieveRawFromPEM(pem, beginToken, endToken, rawStr);
    if (status != ER_OK) {
        return status;
    }
    if (len != rawStr.length()) {
        return ER_INVALID_DATA;
    }
    memcpy(key, rawStr.data(), len);
    return ER_OK;
}

QStatus CertECCUtil_EncodePrivateKey(const uint32_t* privateKey, size_t len, String& encoded)
{
    encoded = EncodeRawByte((uint8_t*) privateKey, len,
                            "-----BEGIN PRIVATE KEY-----", "-----END PRIVATE KEY-----");
    return ER_OK;
}


QStatus CertECCUtil_DecodePrivateKey(const String& encoded, uint32_t* privateKey, size_t len)
{
    return DecodeKeyFromPEM(encoded, privateKey, len,
                            "-----BEGIN PRIVATE KEY-----", "-----END PRIVATE KEY-----");
}

QStatus CertECCUtil_EncodePublicKey(const uint32_t* publicKey, size_t len, String& encoded)
{
    encoded = EncodeRawByte((uint8_t*) publicKey, len,
                            "-----BEGIN PUBLIC KEY-----", "-----END PUBLIC KEY-----");
    return ER_OK;
}

QStatus CertECCUtil_DecodePublicKey(const String& encoded, uint32_t* publicKey, size_t len)
{
    return DecodeKeyFromPEM(encoded, publicKey, len,
                            "-----BEGIN PUBLIC KEY-----", "-----END PUBLIC KEY-----");
}

QStatus CertECCUtil_GetCertCount(const String& encoded, size_t* count)
{
    *count = 0;
    return CountNumOfChunksFromEncoded(encoded,
                                       "-----BEGIN CERTIFICATE-----", "-----END CERTIFICATE-----", count);
}

QStatus CertECCUtil_GetVersionFromEncoded(const uint8_t* encoded, size_t len, uint32_t* certVersion)
{
    if (len < sizeof(uint32_t)) {
        return ER_INVALID_DATA;
    }
    uint32_t* p = (uint32_t*) encoded;
    *certVersion = betoh32(*p);
    return ER_OK;
}

QStatus CertECCUtil_GetVersionFromPEM(const String& pem, uint32_t* certVersion)
{
    qcc::String rawStr;
    QStatus status = RetrieveRawCertFromPEM(pem, rawStr);
    if (status != ER_OK) {
        return status;
    }
    uint8_t* raw = (uint8_t*) rawStr.data();
    size_t rawLen = rawStr.length();

    return CertECCUtil_GetVersionFromEncoded(raw, rawLen, certVersion);
}

QStatus CertECCUtil_GetCertChain(const String& pem, CertificateECC* certChain[], size_t count)
{
    qcc::String* chunks = new  qcc::String[count];

    QStatus status = RetrieveNumOfChunksFromPEM(pem, "-----BEGIN CERTIFICATE-----", "-----END CERTIFICATE-----", chunks, count);
    if (status != ER_OK) {
        delete [] chunks;
        return status;
    }

    for (size_t idx = 0; idx < count; idx++) {
        uint32_t certVersion;
        status = CertECCUtil_GetVersionFromPEM(chunks[idx], &certVersion);
        if (status != ER_OK) {
            break;
        }
        switch (certVersion) {
        case 0:
            certChain[idx] = new CertificateType0();
            break;

        case 1:
            certChain[idx] = new CertificateType1();
            break;

        case 2:
            certChain[idx] = new CertificateType2();
            break;

        default:
            delete [] chunks;
            return ER_INVALID_DATA;
        }
        status = certChain[idx]->LoadPEM(chunks[idx]);
        if (status != ER_OK) {
            break;
        }

    }
    delete [] chunks;
    return status;
}

CertificateType0::CertificateType0(const ECCPublicKey* issuer, const uint8_t* externalDigest) : CertificateECC(0)
{
    SetVersion(0);
    SetIssuer(issuer);
    SetExternalDataDigest(externalDigest);
}

void CertificateType0::SetVersion(uint32_t val)
{
    uint32_t* p = (uint32_t*) &encoded[OFFSET_VERSION];
    *p = htobe32(val);
}

const ECCPublicKey* CertificateType0::GetIssuer()
{
    Crypto_ECC ecc;
    ecc.ReEncode((const ECCPublicKeyOldEncoding*) &encoded[OFFSET_ISSUER], &issuer);
    return &issuer;
}

void CertificateType0::SetIssuer(const ECCPublicKey* issuer)
{
    Crypto_ECC ecc;
    ECCPublicKeyOldEncoding oldenc;
    ecc.ReEncode(issuer, &oldenc);
    memcpy(&encoded[OFFSET_ISSUER], &oldenc, sizeof(ECCPublicKeyOldEncoding));
}

void CertificateType0::SetExternalDataDigest(const uint8_t* externalDataDigest)
{
    memcpy(&encoded[OFFSET_DIGEST], externalDataDigest, Crypto_SHA256::DIGEST_SIZE);
}

const ECCSignature* CertificateType0::GetSig()
{
    Crypto_ECC ecc;
    ecc.ReEncode((const ECCSignatureOldEncoding*) &encoded[OFFSET_SIG], &signature);
    return &signature;
}

void CertificateType0::SetSig(const ECCSignature* sig)
{
    Crypto_ECC ecc;
    ECCSignatureOldEncoding oldenc;
    ecc.ReEncode(sig, &oldenc);
    memcpy(&encoded[OFFSET_SIG], &oldenc, sizeof(ECCSignatureOldEncoding));
}

QStatus CertificateType0::Sign(const ECCPrivateKey* dsaPrivateKey)
{
    Crypto_ECC ecc;
    ECCSignature newenc;
    ecc.SetDSAPrivateKey(dsaPrivateKey);
    QStatus status = ecc.DSASignDigest(GetExternalDataDigest(), Crypto_SHA256::DIGEST_SIZE, &newenc);
    ecc.ReEncode(&newenc, (ECCSignatureOldEncoding*) &encoded[OFFSET_SIG]);
    return status;
}

bool CertificateType0::VerifySignature()
{
    Crypto_ECC ecc;
    ecc.SetDSAPublicKey(GetIssuer());
    return ecc.DSAVerifyDigest(GetExternalDataDigest(), Crypto_SHA256::DIGEST_SIZE, GetSig()) == ER_OK;
}

QStatus CertificateType0::LoadEncoded(const uint8_t* encodedBytes, size_t len)
{
    if (len != GetEncodedLen()) {
        return ER_INVALID_DATA;
    }
    uint32_t*pVersion = (uint32_t*) encodedBytes;
    uint32_t version = betoh32(*pVersion);
    if (version != GetVersion()) {
        return ER_INVALID_DATA;
    }
    memcpy(&encoded, encodedBytes, len);
    return ER_OK;
}

String CertificateType0::GetPEM()
{
    return EncodeCertRawByte(GetEncoded(), GetEncodedLen());
}

QStatus CertificateType0::LoadPEM(const String& pem)
{
    qcc::String rawStr;
    QStatus status = RetrieveRawCertFromPEM(pem, rawStr);
    if (status != ER_OK) {
        return status;
    }
    uint8_t* raw = (uint8_t*) rawStr.data();
    size_t rawLen = rawStr.length();

    return LoadEncoded(raw, rawLen);
}

qcc::String CertificateType0::ToString()
{

    qcc::String str("Certificate:\n");
    str += "version: ";
    str += U32ToString(GetVersion());
    str += "\n";
    str += "issuer: ";
    str += BytesToHexString((const uint8_t*) &encoded[OFFSET_ISSUER], sizeof(ECCPublicKeyOldEncoding));
    str += "\n";
    str += "digest: ";
    str += BytesToHexString((const uint8_t*) GetExternalDataDigest(), Crypto_SHA256::DIGEST_SIZE);
    str += "\n";
    str += "sig: ";
    str += BytesToHexString((const uint8_t*) &encoded[OFFSET_SIG], sizeof(ECCSignatureOldEncoding));
    str += "\n";
    return str;
}

void CertificateType1::SetVersion(uint32_t val)
{
    uint32_t* p = (uint32_t*) &encoded[OFFSET_VERSION];
    *p = htobe32(val);
}

const ECCPublicKey* CertificateType1::GetIssuer()
{
    Crypto_ECC ecc;
    ecc.ReEncode((const ECCPublicKeyOldEncoding*) &encoded[OFFSET_ISSUER], &issuer);
    return &issuer;
}

void CertificateType1::SetIssuer(const ECCPublicKey* issuer)
{
    Crypto_ECC ecc;
    ECCPublicKeyOldEncoding oldenc;
    ecc.ReEncode(issuer, &oldenc);
    memcpy(&encoded[OFFSET_ISSUER], &oldenc, sizeof(ECCPublicKeyOldEncoding));
}

const ECCPublicKey* CertificateType1::GetSubject()
{
    Crypto_ECC ecc;
    ecc.ReEncode((const ECCPublicKeyOldEncoding*) &encoded[OFFSET_SUBJECT], &subject);
    return &subject;
}

void CertificateType1::SetSubject(const ECCPublicKey* issuer)
{
    Crypto_ECC ecc;
    ECCPublicKeyOldEncoding oldenc;
    ecc.ReEncode(issuer, &oldenc);
    memcpy(&encoded[OFFSET_SUBJECT], &oldenc, sizeof(ECCPublicKeyOldEncoding));
}

void CertificateType1::SetValidity(const ValidPeriod* validPeriod)
{
    validity = *validPeriod;
    /* setup encoded buffer */
    uint64_t* p = (uint64_t*) &encoded[OFFSET_VALIDFROM];
    *p = htobe64(validity.validFrom);
    p = (uint64_t*) &encoded[OFFSET_VALIDTO];
    *p = htobe64(validity.validTo);
}

CertificateType1::CertificateType1(const ECCPublicKey* issuer, const ECCPublicKey* subject) : CertificateECC(1)
{
    SetVersion(1);
    SetIssuer(issuer);
    SetSubject(subject);
    SetDelegate(false);
}

void CertificateType1::SetExternalDataDigest(const uint8_t* externalDataDigest)
{
    memcpy(&encoded[OFFSET_DIGEST], externalDataDigest, Crypto_SHA256::DIGEST_SIZE);
}

const ECCSignature* CertificateType1::GetSig()
{
    Crypto_ECC ecc;
    ecc.ReEncode((const ECCSignatureOldEncoding*) &encoded[OFFSET_SIG], &signature);
    return &signature;
}

void CertificateType1::SetSig(const ECCSignature* sig)
{
    Crypto_ECC ecc;
    ECCSignatureOldEncoding oldenc;
    ecc.ReEncode(sig, &oldenc);
    memcpy(&encoded[OFFSET_SIG], &oldenc, sizeof(ECCSignatureOldEncoding));
}

QStatus CertificateType1::Sign(const ECCPrivateKey* dsaPrivateKey)
{
    QStatus status;
    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(dsaPrivateKey);
    ECCSignature newenc;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    Crypto_SHA256 hash;
    hash.Init();
    hash.Update(encoded, OFFSET_SIG);
    hash.GetDigest(digest);
    status = ecc.DSASignDigest(digest, sizeof(digest), &newenc);
    ecc.ReEncode(&newenc, (ECCSignatureOldEncoding*) &encoded[OFFSET_SIG]);
    return status;
}

bool CertificateType1::VerifySignature()
{
    Crypto_ECC ecc;
    ecc.SetDSAPublicKey(GetIssuer());
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    Crypto_SHA256 hash;
    hash.Init();
    hash.Update(encoded, OFFSET_SIG);
    hash.GetDigest(digest);
    return ecc.DSAVerifyDigest(digest, sizeof(digest), GetSig()) == ER_OK;
}

QStatus CertificateType1::LoadEncoded(const uint8_t* encodedBytes, size_t len)
{
    if (len != GetEncodedLen()) {
        return ER_INVALID_DATA;
    }
    uint32_t* pVersion = (uint32_t*) encodedBytes;
    uint32_t version = betoh32(*pVersion);
    if (version != GetVersion()) {
        return ER_INVALID_DATA;
    }
    memcpy(&encoded, encodedBytes, len);
    /* setup the localized validitiy value from the encoded bytes */
    uint64_t* p = (uint64_t*) &encoded[OFFSET_VALIDFROM];
    validity.validFrom = betoh64(*p);
    p = (uint64_t*) &encoded[OFFSET_VALIDTO];
    validity.validTo = betoh64(*p);

    return ER_OK;
}

String CertificateType1::GetPEM()
{
    return EncodeCertRawByte(GetEncoded(), GetEncodedLen());
}

QStatus CertificateType1::LoadPEM(const String& pem)
{
    qcc::String rawStr;
    QStatus status = RetrieveRawCertFromPEM(pem, rawStr);
    if (status != ER_OK) {
        return status;
    }
    uint8_t* raw = (uint8_t*) rawStr.data();
    size_t rawLen = rawStr.length();
    return LoadEncoded(raw, rawLen);
}

qcc::String CertificateType1::ToString()
{
    qcc::String str("Certificate:\n");
    str += "version: ";
    str += U32ToString(GetVersion());
    str += "\n";
    str += "issuer: ";
    str += BytesToHexString((const uint8_t*) &encoded[OFFSET_ISSUER], sizeof(ECCPublicKeyOldEncoding));
    str += "\n";
    str += "subject: ";
    str += BytesToHexString((const uint8_t*) &encoded[OFFSET_SUBJECT], sizeof(ECCPublicKeyOldEncoding));
    str += "\n";
    str += "validity: not-before ";
    str += U64ToString(GetValidity()->validFrom);
    str += " not-after ";
    str += U64ToString(GetValidity()->validTo);
    str += "\n";
    if (IsDelegate()) {
        str += "delegate: true\n";
    } else {
        str += "delegate: false\n";
    }
    str += "digest: ";
    str += BytesToHexString((const uint8_t*) GetExternalDataDigest(), Crypto_SHA256::DIGEST_SIZE);
    str += "\n";
    str += "sig: ";
    str += BytesToHexString((const uint8_t*) &encoded[OFFSET_SIG], sizeof(ECCSignatureOldEncoding));
    str += "\n";
    return str;
}


void CertificateType2::SetVersion(uint32_t val)
{
    uint32_t* p = (uint32_t*) &encoded[OFFSET_VERSION];
    *p = htobe32(val);
}

const ECCPublicKey* CertificateType2::GetIssuer()
{
    Crypto_ECC ecc;
    ecc.ReEncode((const ECCPublicKeyOldEncoding*) &encoded[OFFSET_ISSUER], &issuer);
    return &issuer;
}

void CertificateType2::SetIssuer(const ECCPublicKey* issuer)
{
    Crypto_ECC ecc;
    ECCPublicKeyOldEncoding oldenc;
    ecc.ReEncode(issuer, &oldenc);
    memcpy(&encoded[OFFSET_ISSUER], &oldenc, sizeof(ECCPublicKeyOldEncoding));
}

const ECCPublicKey* CertificateType2::GetSubject()
{
    Crypto_ECC ecc;
    ecc.ReEncode((const ECCPublicKeyOldEncoding*) &encoded[OFFSET_SUBJECT], &subject);
    return &subject;
}

void CertificateType2::SetSubject(const ECCPublicKey* issuer)
{
    Crypto_ECC ecc;
    ECCPublicKeyOldEncoding oldenc;
    ecc.ReEncode(issuer, &oldenc);
    memcpy(&encoded[OFFSET_SUBJECT], &oldenc, sizeof(ECCPublicKeyOldEncoding));
}

void CertificateType2::SetValidity(const ValidPeriod* validPeriod)
{
    validity = *validPeriod;
    /* setup encoded buffer */
    uint64_t* p = (uint64_t*) &encoded[OFFSET_VALIDFROM];
    *p = htobe64(validity.validFrom);
    p = (uint64_t*) &encoded[OFFSET_VALIDTO];
    *p = htobe64(validity.validTo);
}

CertificateType2::CertificateType2(const ECCPublicKey* issuer, const ECCPublicKey* subject) : CertificateECC(2)
{
    SetVersion(2);
    SetIssuer(issuer);
    SetSubject(subject);
    SetDelegate(false);
}

void CertificateType2::SetExternalDataDigest(const uint8_t* externalDataDigest)
{
    memcpy(&encoded[OFFSET_DIGEST], externalDataDigest, Crypto_SHA256::DIGEST_SIZE);
}

const ECCSignature* CertificateType2::GetSig()
{
    Crypto_ECC ecc;
    ecc.ReEncode((const ECCSignatureOldEncoding*) &encoded[OFFSET_SIG], &signature);
    return &signature;
}

void CertificateType2::SetSig(const ECCSignature* sig)
{
    Crypto_ECC ecc;
    ECCSignatureOldEncoding oldenc;
    ecc.ReEncode(sig, &oldenc);
    memcpy(&encoded[OFFSET_SIG], &oldenc, sizeof(ECCSignatureOldEncoding));
}

QStatus CertificateType2::Sign(const ECCPrivateKey* dsaPrivateKey)
{
    QStatus status;
    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(dsaPrivateKey);
    ECCSignature newenc;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    Crypto_SHA256 hash;
    hash.Init();
    hash.Update(encoded, OFFSET_SIG);
    hash.GetDigest(digest);
    status = ecc.DSASignDigest(digest, sizeof(digest), &newenc);
    ecc.ReEncode(&newenc, (ECCSignatureOldEncoding*) &encoded[OFFSET_SIG]);
    return status;
}

bool CertificateType2::VerifySignature()
{
    Crypto_ECC ecc;
    ecc.SetDSAPublicKey(GetIssuer());
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    Crypto_SHA256 hash;
    hash.Init();
    hash.Update(encoded, OFFSET_SIG);
    hash.GetDigest(digest);
    return ecc.DSAVerifyDigest(digest, sizeof(digest), GetSig()) == ER_OK;
}

QStatus CertificateType2::LoadEncoded(const uint8_t* encodedBytes, size_t len)
{
    if (len != GetEncodedLen()) {
        return ER_INVALID_DATA;
    }
    uint32_t* pVersion = (uint32_t*) encodedBytes;
    uint32_t version = betoh32(*pVersion);
    if (version != GetVersion()) {
        return ER_INVALID_DATA;
    }
    memcpy(&encoded, encodedBytes, len);
    /* setup the localized validitiy value from the encoded bytes */
    uint64_t* p = (uint64_t*) &encoded[OFFSET_VALIDFROM];
    validity.validFrom = betoh64(*p);
    p = (uint64_t*) &encoded[OFFSET_VALIDTO];
    validity.validTo = betoh64(*p);

    return ER_OK;
}

String CertificateType2::GetPEM()
{
    return EncodeCertRawByte(GetEncoded(), GetEncodedLen());
}

QStatus CertificateType2::LoadPEM(const String& pem)
{
    qcc::String rawStr;
    QStatus status = RetrieveRawCertFromPEM(pem, rawStr);
    if (status != ER_OK) {
        return status;
    }
    uint8_t* raw = (uint8_t*) rawStr.data();
    size_t rawLen = rawStr.length();
    return LoadEncoded(raw, rawLen);
}

void CertificateType2::SetGuild(const uint8_t* newGuild, size_t guildLen)
{
    size_t len = GUILD_ID_LEN;
    memset(&encoded[OFFSET_GUILD], 0, len);
    if (len > guildLen) {
        len = guildLen;
    }
    memcpy(&encoded[OFFSET_GUILD], newGuild, len);
}

String CertificateType2::ToString()
{
    qcc::String str("Certificate:\n");
    str += "version: ";
    str += U32ToString(GetVersion());
    str += "\n";
    str += "issuer: ";
    str += BytesToHexString((const uint8_t*) &encoded[OFFSET_ISSUER], sizeof(ECCPublicKeyOldEncoding));
    str += "\n";
    str += "subject: ";
    str += BytesToHexString((const uint8_t*) &encoded[OFFSET_SUBJECT], sizeof(ECCPublicKeyOldEncoding));
    str += "\n";
    str += "validity: not-before ";
    str += U64ToString(GetValidity()->validFrom);
    str += " not-after ";
    str += U64ToString(GetValidity()->validTo);
    str += "\n";
    if (IsDelegate()) {
        str += "delegate: true\n";
    } else {
        str += "delegate: false\n";
    }
    str += "guild: ";
    str += BytesToHexString((const uint8_t*) GetGuild(), GUILD_ID_LEN);
    str += "\n";
    str += "digest: ";
    str += BytesToHexString((const uint8_t*) GetExternalDataDigest(), Crypto_SHA256::DIGEST_SIZE);
    str += "\n";
    str += "sig: ";
    str += BytesToHexString((const uint8_t*) &encoded[OFFSET_SIG], sizeof(ECCSignatureOldEncoding));
    str += "\n";
    return str;
}

QStatus CertificateX509::DecodeCertificateName(const qcc::String& dn, CertificateX509::DistinguishedName& name)
{
    QStatus status = ER_OK;
    qcc::String tmp = dn;

    while ((ER_OK == status) && (tmp.size())) {
        qcc::String oid;
        qcc::String str;
        qcc::String rem;
        status = Crypto_ASN1::Decode(tmp, "{(ou)}.", &oid, &str, &rem);
        if (ER_OK != status) {
            return status;
        }
        if (!qcc::GUID128::IsGUID(str)) {
            return ER_FAIL;
        }
        if (OID_DN_OU == oid) {
            name.ou = GUID128(str);
            name.ouExists = true;
        } else if (OID_DN_CN == oid) {
            name.cn = GUID128(str);
            name.cnExists = true;
        }
        tmp = rem;
    }

    return status;
}

QStatus CertificateX509::EncodeCertificateName(qcc::String& dn, CertificateX509::DistinguishedName& name)
{
    QStatus status;
    qcc::String oid;
    qcc::String tmp;

    if (name.ouExists) {
        oid = OID_DN_OU;
        tmp = name.ou.ToString();
        status = Crypto_ASN1::Encode(dn, "{(ou)}", &oid, &tmp);
        if (ER_OK != status) {
            return status;
        }
    }
    if (name.cnExists) {
        oid = OID_DN_CN;
        tmp = name.cn.ToString();
        status = Crypto_ASN1::Encode(dn, "{(ou)}", &oid, &tmp);
        if (ER_OK != status) {
            return status;
        }
    }

    return status;
}

QStatus CertificateX509::DecodeCertificatePub(const qcc::String& pub)
{
    QStatus status = ER_OK;
    qcc::String oid1;
    qcc::String oid2;
    qcc::String key;
    size_t keylen;

    status = Crypto_ASN1::Decode(pub, "(oo)b", &oid1, &oid2, &key, &keylen);
    if (ER_OK != status) {
        return status;
    }
    if (OID_KEY_ECC != oid1) {
        return ER_FAIL;
    }
    if (OID_CRV_PRIME256V1 != oid2) {
        return ER_FAIL;
    }
    if (1 + sizeof (publickey) != key.size()) {
        return ER_FAIL;
    }
    // Uncompressed points only
    if (0x4 != *key.data()) {
        return ER_FAIL;
    }
    memcpy((uint8_t*) &publickey, key.data() + 1, key.size() - 1);

    return status;
}

QStatus CertificateX509::EncodeCertificatePub(qcc::String& pub)
{
    QStatus status = ER_OK;
    qcc::String oid1 = OID_KEY_ECC;
    qcc::String oid2 = OID_CRV_PRIME256V1;

    // Uncompressed points only
    qcc::String key(0x4);
    key += qcc::String((const char*) &publickey, sizeof (publickey));
    status = Crypto_ASN1::Encode(pub, "(oo)b", &oid1, &oid2, &key, 8 * key.size());
    if (ER_OK != status) {
        return status;
    }

    return status;
}

QStatus CertificateX509::DecodeCertificateExt(const qcc::String& ext)
{
    QStatus status = ER_OK;
    qcc::String tmp;

    status = Crypto_ASN1::Decode(ext, "c((.))", 3, &tmp);
    if (ER_OK != status) {
        return status;
    }
    while ((ER_OK == status) && (tmp.size())) {
        qcc::String oid;
        qcc::String str;
        qcc::String rem;
        status = Crypto_ASN1::Decode(tmp, "(ox).", &oid, &str, &rem);
        if (ER_OK != status) {
            return status;
        }
        if (OID_BASIC_CONSTRAINTS == oid) {
            status = Crypto_ASN1::Decode(str, "(z)", &ca);
            if (ER_OK != status) {
                return status;
            }
        } else if (OID_CUSTOM_CERT_TYPE == oid) {
            status = Crypto_ASN1::Decode(str, "(i)", (uint32_t*) &type);
            if (ER_OK != status) {
                return status;
            }
        } else if (OID_SUB_ALTNAME == oid) {
            alias = str;
        } else if (OID_CUSTOM_DIGEST == oid) {
            qcc::String dig;
            oid.clear();
            status = Crypto_ASN1::Decode(str, "(ox)", &oid, &dig);
            if (ER_OK != status) {
                return status;
            }
            if (OID_DIG_SHA256 != oid) {
                return ER_INVALID_DATA;
            }
            digest = dig;
        }
        tmp = rem;
    }

    return status;
}

QStatus CertificateX509::EncodeCertificateExt(qcc::String& ext)
{
    QStatus status = ER_OK;
    qcc::String oid;
    qcc::String opt;
    qcc::String tmp;
    qcc::String tmp1;
    qcc::String raw;

    status = Crypto_ASN1::Encode(tmp, "(z)", ca);
    if (ER_OK != status) {
        return status;
    }
    oid = OID_BASIC_CONSTRAINTS;
    status = Crypto_ASN1::Encode(raw, "(ox)", &oid, &tmp);
    if (ER_OK != status) {
        return status;
    }
    tmp.clear();
    status = Crypto_ASN1::Encode(tmp, "(i)", (uint32_t) type);
    if (ER_OK != status) {
        return status;
    }
    oid = OID_CUSTOM_CERT_TYPE;
    status = Crypto_ASN1::Encode(raw, "(ox)", &oid, &tmp);
    if (ER_OK != status) {
        return status;
    }
    if (alias.size()) {
        oid = OID_SUB_ALTNAME;
        status = Crypto_ASN1::Encode(raw, "(ox)", &oid, &alias);
        if (ER_OK != status) {
            return status;
        }
    }
    if (digest.size()) {
        qcc::String dig;
        oid = OID_DIG_SHA256;
        status = Crypto_ASN1::Encode(dig, "(ox)", &oid, &digest);
        if (ER_OK != status) {
            return status;
        }
        oid = OID_CUSTOM_DIGEST;
        status = Crypto_ASN1::Encode(raw, "(ox)", &oid, &dig);
        if (ER_OK != status) {
            return status;
        }
    }
    status = Crypto_ASN1::Encode(ext, "c((R))", 3, &raw);

    return status;
}

QStatus CertificateX509::DecodeCertificateTBS()
{
    QStatus status = ER_OK;
    uint32_t x509Version;
    qcc::String oid;
    qcc::String iss;
    qcc::String sub;
    qcc::String time1;
    qcc::String time2;
    qcc::String pub;
    qcc::String ext;

    status = Crypto_ASN1::Decode(tbs, "(c(i)l(o)(.)(tt)(.)(.).)",
                                 0, &x509Version, &serial, &oid, &iss, &time1, &time2, &sub, &pub, &ext);
    if (ER_OK != status) {
        return status;
    }
    if (X509_VERSION_3 != x509Version) {
        return ER_FAIL;
    }
    if (OID_SIG_ECDSA_SHA256 != oid) {
        return ER_FAIL;
    }
    status = DecodeCertificateName(iss, issuer);
    if (ER_OK != status) {
        return status;
    }
    //TODO read out the time values
    status = DecodeCertificateName(sub, subject);
    if (ER_OK != status) {
        return status;
    }
    status = DecodeCertificatePub(pub);
    if (ER_OK != status) {
        return status;
    }
    status = DecodeCertificateExt(ext);

    return status;
}

QStatus CertificateX509::EncodeCertificateTBS()
{
    QStatus status = ER_OK;
    uint32_t x509Version = X509_VERSION_3;
    qcc::String oid = OID_SIG_ECDSA_SHA256;
    qcc::String iss;
    qcc::String sub;
    qcc::String time1 = "140101000000Z";
    qcc::String time2 = "150101000000Z";
    qcc::String pub;
    qcc::String ext;

    status = EncodeCertificateName(iss, issuer);
    if (ER_OK != status) {
        return status;
    }
    status = EncodeCertificateName(sub, subject);
    if (ER_OK != status) {
        return status;
    }
    status = EncodeCertificatePub(pub);
    if (ER_OK != status) {
        return status;
    }
    status = EncodeCertificateExt(ext);
    if (ER_OK != status) {
        return status;
    }
    status = Crypto_ASN1::Encode(tbs, "(c(i)l(o)(R)(tt)(R)(R)R)",
                                 0, x509Version, &serial, &oid, &iss, &time1, &time2, &sub, &pub, &ext);

    return status;
}

QStatus CertificateX509::DecodeCertificateSig(const qcc::String& sig)
{
    QStatus status = ER_OK;
    qcc::String r;
    qcc::String s;

    status = Crypto_ASN1::Decode(sig, "(ll)", &r, &s);
    if (ER_OK != status) {
        return status;
    }
    memset(&signature, 0, sizeof (signature));
    if (sizeof (signature.r) < r.size()) {
        return ER_FAIL;
    }
    if (sizeof (signature.s) < s.size()) {
        return ER_FAIL;
    }
    /* need to prepend leading zero bytes if r size smaller than signagure.r size because the ASN.1 encoder strips the leading zero bytes for type l */
    uint8_t* p = signature.r;
    p += (sizeof (signature.r) - r.size());
    memcpy(p, r.data(), r.size());

    /* need to prepend leading zero bytes if s size smaller than signagure.s size because the ASN.1 encoder strips the leading zero bytes for type l */
    p = signature.s;
    p += (sizeof (signature.s) - s.size());
    memcpy(p, s.data(), s.size());

    return status;
}

QStatus CertificateX509::EncodeCertificateSig(qcc::String& sig)
{
    QStatus status = ER_OK;
    qcc::String r((const char*) signature.r, sizeof (signature.r));
    qcc::String s((const char*) signature.s, sizeof (signature.s));

    status = Crypto_ASN1::Encode(sig, "(ll)", &r, &s);
    if (ER_OK != status) {
        return status;
    }

    return status;
}

QStatus CertificateX509::DecodeCertificateDER(const qcc::String& der)
{
    QStatus status;
    qcc::String oid;
    qcc::String sig;
    qcc::String tmp;
    size_t siglen;

    status = Crypto_ASN1::Decode(der, "((.)(o)b)", &tmp, &oid, &sig, &siglen);
    if (ER_OK != status) {
        return status;
    }
    // Put the sequence back on the TBS
    status = Crypto_ASN1::Encode(tbs, "(R)", &tmp);
    if (ER_OK != status) {
        return status;
    }
    if (OID_SIG_ECDSA_SHA256 != oid) {
        return ER_FAIL;
    }
    status = DecodeCertificateTBS();
    if (ER_OK != status) {
        return status;
    }
    status = DecodeCertificateSig(sig);

    return status;
}

QStatus CertificateX509::EncodeCertificateDER(qcc::String& der)
{
    QStatus status;
    qcc::String oid = OID_SIG_ECDSA_SHA256;
    qcc::String sig;

    if (tbs.empty()) {
        return ER_FAIL;
    }
    status = EncodeCertificateSig(sig);
    if (ER_OK != status) {
        return status;
    }
    status = Crypto_ASN1::Encode(der, "(R(o)b)", &tbs, &oid, &sig, 8 * sig.size());

    return status;
}

QStatus CertificateX509::DecodeCertificatePEM(const qcc::String& pem)
{
    QStatus status;
    size_t pos;
    qcc::String rem;
    qcc::String der;
    qcc::String tag1 = "-----BEGIN CERTIFICATE-----";
    qcc::String tag2 = "-----END CERTIFICATE-----";

    pos = pem.find(tag1);
    if (pos == qcc::String::npos) {
        return ER_INVALID_DATA;
    }
    rem = pem.substr(pos + tag1.size());

    pos = rem.find(tag2);
    if (pos == qcc::String::npos) {
        return ER_INVALID_DATA;
    }
    rem = rem.substr(0, pos);

    status = Crypto_ASN1::DecodeBase64(rem, der);
    if (ER_OK != status) {
        return status;
    }
    status = DecodeCertificateDER(der);

    return status;
}

QStatus CertificateX509::EncodeCertificatePEM(qcc::String& der, qcc::String& pem)
{
    QStatus status;
    qcc::String rem;
    qcc::String tag1 = "-----BEGIN CERTIFICATE-----\n";
    qcc::String tag2 = "-----END CERTIFICATE-----";

    status = Crypto_ASN1::EncodeBase64(der, rem);
    if (ER_OK != status) {
        return status;
    }
    pem = tag1 + rem + tag2;

    return status;
}

QStatus CertificateX509::EncodeCertificatePEM(qcc::String& pem)
{
    qcc::String der;
    QStatus status = EncodeCertificateDER(der);
    if (ER_OK != status) {
        return status;
    }
    return EncodeCertificatePEM(der, pem);
}

QStatus CertificateX509::Verify()
{
    return Verify(&publickey);
}

QStatus CertificateX509::Verify(const ECCPublicKey* key)
{
    Crypto_ECC ecc;
    ecc.SetDSAPublicKey(key);
    return ecc.DSAVerify((const uint8_t*) tbs.data(), tbs.size(), &signature);
}

QStatus CertificateX509::Verify(const KeyInfoNISTP256& ta)
{
    Crypto_ECC ecc;
    if (qcc::GUID128::SIZE != ta.GetKeyIdLen()) {
        return ER_FAIL;
    }
    if (0 != memcmp(issuer.cn.GetBytes(), ta.GetKeyId(), qcc::GUID128::SIZE)) {
        return ER_FAIL;
    }
    ecc.SetDSAPublicKey(ta.GetPublicKey());
    return ecc.DSAVerify((const uint8_t*) tbs.data(), tbs.size(), &signature);
}

QStatus CertificateX509::Sign(const ECCPrivateKey* key)
{
    QStatus status;
    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(key);
    status = EncodeCertificateTBS();
    if (ER_OK != status) {
        return status;
    }
    return ecc.DSASign((const uint8_t*) tbs.data(), tbs.size(), &signature);
}

String CertificateX509::ToString()
{
    qcc::String str("Certificate:\n");
    str += "serial:    " + serial + "\n";
    str += "issuer:    " + issuer.cn.ToString() + "\n";
    str += "subject:   " + subject.cn.ToString() + "\n";
    if (subject.ouExists) {
        str += "guild:     " + subject.ou.ToString() + "\n";
    }
    str += "publickey: " + BytesToHexString((const uint8_t*) &publickey, sizeof(publickey)) + "\n";
    str += "ca:        " + BytesToHexString((const uint8_t*) &ca, sizeof(uint8_t)) + "\n";
    if (digest.size()) {
        str += "digest:    " + BytesToHexString((const uint8_t*) digest.c_str(), digest.size()) + "\n";
    }
    if (alias.size()) {
        str += "alias:     " + alias + "\n";
    }
    str += "signature: " + BytesToHexString((const uint8_t*) &signature, sizeof(signature)) + "\n";
    return str;
}

QStatus CertificateX509::GenEncoded()
{
    delete [] encoded;
    encoded = NULL;
    encodedLen = 0;
    String der;
    QStatus status = EncodeCertificateDER(der);
    if (ER_OK != status) {
        return status;
    }
    encodedLen = der.length();
    encoded = new uint8_t[encodedLen];
    if (!encoded) {
        return ER_OUT_OF_MEMORY;
    }
    memcpy(encoded, der.data(), encodedLen);
    return ER_OK;
}

const uint8_t* CertificateX509::GetEncoded()
{
    if (encodedLen == 0) {
        GenEncoded();
    }
    return encoded;
}

size_t CertificateX509::GetEncodedLen()
{
    if (encodedLen == 0) {
        GenEncoded();
    }
    return encodedLen;
}

QStatus CertificateX509::LoadEncoded(const uint8_t* encodedBytes, size_t len)
{
    String der((const char*) encodedBytes, len);
    return DecodeCertificateDER(der);
}

String CertificateX509::GetPEM()
{
    String pem;
    EncodeCertificatePEM(pem);
    return pem;
}

QStatus CertificateX509::LoadPEM(const String& pem)
{
    return DecodeCertificatePEM(pem);
}

QStatus CertificateX509::DecodeCertChainPEM(const String& encoded, CertificateX509* certs, size_t count)
{
    qcc::String* chunks = new  qcc::String[count];

    QStatus status = RetrieveNumOfChunksFromPEM(encoded, "-----BEGIN CERTIFICATE-----", "-----END CERTIFICATE-----", chunks, count);
    if (status != ER_OK) {
        delete [] chunks;
        return status;
    }

    for (size_t idx = 0; idx < count; idx++) {
        status = certs[idx].LoadPEM(chunks[idx]);
        if (ER_OK != status) {
            break;
        }
    }
    delete [] chunks;
    return status;
}

}
