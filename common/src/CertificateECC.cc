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
#include <qcc/CertificateECC.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

namespace qcc {

#define CERT_TYPE0_SIGN_LEN Crypto_SHA256::DIGEST_SIZE
#define CERT_TYPE0_RAW_LEN sizeof(uint32_t) + sizeof(ECCPublicKey) + CERT_TYPE0_SIGN_LEN + sizeof(ECCSignature)

#define CERT_TYPE1_SIGN_LEN sizeof(uint32_t) + 2 * sizeof(ECCPublicKey) + sizeof(ValidPeriod) + sizeof (uint8_t) + Crypto_SHA256::DIGEST_SIZE
#define CERT_TYPE1_RAW_LEN CERT_TYPE1_SIGN_LEN + sizeof(ECCSignature)

#define CERT_TYPE2_SIGN_LEN sizeof(uint32_t) + 2 * sizeof(ECCPublicKey) + sizeof(ValidPeriod) + sizeof (uint8_t) + GUILD_ID_LEN + Crypto_SHA256::DIGEST_SIZE
#define CERT_TYPE2_RAW_LEN CERT_TYPE2_SIGN_LEN + sizeof(ECCSignature)

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

void CertificateType0::SetIssuer(const ECCPublicKey* issuer)
{
    memcpy(&encoded[OFFSET_ISSUER], issuer, sizeof(ECCPublicKey));
}

void CertificateType0::SetExternalDataDigest(const uint8_t* externalDataDigest)
{
    memcpy(&encoded[OFFSET_DIGEST], externalDataDigest, Crypto_SHA256::DIGEST_SIZE);
}

void CertificateType0::SetSig(const ECCSignature* sig)
{
    memcpy(&encoded[OFFSET_SIG], sig, sizeof(ECCSignature));
}


QStatus CertificateType0::Sign(const ECCPrivateKey* dsaPrivateKey)
{
    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(dsaPrivateKey);
    return ecc.DSASignDigest(GetExternalDataDigest(), Crypto_SHA256::DIGEST_SIZE, (ECCSignature*) &encoded[OFFSET_SIG]);
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
    str += BytesToHexString((const uint8_t*) GetIssuer(), sizeof(ECCPublicKey));
    str += "\n";
    str += "digest: ";
    str += BytesToHexString((const uint8_t*) GetExternalDataDigest(), Crypto_SHA256::DIGEST_SIZE);
    str += "\n";
    str += "sig: ";
    str += BytesToHexString((const uint8_t*) GetSig(), sizeof(ECCSignature));
    str += "\n";
    return str;
}

void CertificateType1::SetVersion(uint32_t val)
{
    uint32_t* p = (uint32_t*) &encoded[OFFSET_VERSION];
    *p = htobe32(val);
}

void CertificateType1::SetIssuer(const ECCPublicKey* issuer)
{
    memcpy(&encoded[OFFSET_ISSUER], issuer, sizeof(ECCPublicKey));
}

void CertificateType1::SetSubject(const ECCPublicKey* subject)
{
    memcpy(&encoded[OFFSET_SUBJECT], subject, sizeof(ECCPublicKey));
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

void CertificateType1::SetSig(const ECCSignature* sig)
{
    memcpy(&encoded[OFFSET_SIG], sig, sizeof(ECCSignature));
}

QStatus CertificateType1::Sign(const ECCPrivateKey* dsaPrivateKey)
{
    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(dsaPrivateKey);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    Crypto_SHA256 hash;
    hash.Init();
    hash.Update(encoded, OFFSET_SIG);
    hash.GetDigest(digest);
    return ecc.DSASignDigest(digest, sizeof(digest), (ECCSignature*) &encoded[OFFSET_SIG]);
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
    str += BytesToHexString((const uint8_t*) GetIssuer(), sizeof(ECCPublicKey));
    str += "\n";
    str += "subject: ";
    str +=  BytesToHexString((const uint8_t*) GetSubject(), sizeof(ECCPublicKey));
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
    str += BytesToHexString((const uint8_t*) GetSig(), sizeof(ECCSignature));
    str += "\n";
    return str;
}


void CertificateType2::SetVersion(uint32_t val)
{
    uint32_t* p = (uint32_t*) &encoded[OFFSET_VERSION];
    *p = htobe32(val);
}

void CertificateType2::SetIssuer(const ECCPublicKey* issuer)
{
    memcpy(&encoded[OFFSET_ISSUER], issuer, sizeof(ECCPublicKey));
}

void CertificateType2::SetSubject(const ECCPublicKey* subject)
{
    memcpy(&encoded[OFFSET_SUBJECT], subject, sizeof(ECCPublicKey));
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

void CertificateType2::SetSig(const ECCSignature* sig)
{
    memcpy(&encoded[OFFSET_SIG], sig, sizeof(ECCSignature));
}

QStatus CertificateType2::Sign(const ECCPrivateKey* dsaPrivateKey)
{
    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(dsaPrivateKey);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    Crypto_SHA256 hash;
    hash.Init();
    hash.Update(encoded, OFFSET_SIG);
    hash.GetDigest(digest);
    return ecc.DSASignDigest(digest, sizeof(digest), (ECCSignature*) &encoded[OFFSET_SIG]);
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
    str += BytesToHexString((const uint8_t*) GetIssuer(), sizeof(ECCPublicKey));
    str += "\n";
    str += "subject: ";
    str +=  BytesToHexString((const uint8_t*) GetSubject(), sizeof(ECCPublicKey));
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
    str += BytesToHexString((const uint8_t*) GetSig(), sizeof(ECCSignature));
    str += "\n";
    return str;
}

}
