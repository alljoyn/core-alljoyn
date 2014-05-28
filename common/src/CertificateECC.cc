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

#include <sys/types.h>

#include <qcc/CertificateECC.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

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

static qcc::String EncodeRawByte(uint8_t* raw, size_t rawLen, const char* beginToken, const char* endToken)
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

static qcc::String EncodeCertRawByte(uint8_t* raw, size_t rawLen)
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

static QStatus RetrieveNumOfChunksFromEncoded(const String& encoded, const char* beginToken, const char* endToken, qcc::String chunks[],  size_t count)
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


static QStatus RetrieveRawFromEncoded(const String& encoded, const char* beginToken, const char* endToken, String& rawStr)
{
    qcc::String base64;

    size_t pos;

    pos = encoded.find(beginToken);
    if (pos == qcc::String::npos) {
        return ER_INVALID_DATA;
    }
    qcc::String remainder = encoded.substr(pos + strlen(beginToken));
    pos = remainder.find(endToken);
    if (pos == qcc::String::npos) {
        base64 = remainder;
    } else {
        base64 = remainder.substr(0, pos);
    }
    return Crypto_ASN1::DecodeBase64(base64, rawStr);
}

static QStatus RetrieveRawCertFromEncoded(const String& encoded, String& rawStr)
{
    return RetrieveRawFromEncoded(encoded,
                                  "-----BEGIN CERTIFICATE-----",
                                  "-----END CERTIFICATE-----", rawStr);
}

static QStatus DecodeKeyFromEncoded(const String& encoded, uint32_t* key, size_t len, const char* beginToken, const char* endToken)
{
    String rawStr;
    QStatus status = RetrieveRawFromEncoded(encoded, beginToken, endToken, rawStr);
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
    return DecodeKeyFromEncoded(encoded, privateKey, len,
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
    return DecodeKeyFromEncoded(encoded, publicKey, len,
                                "-----BEGIN PUBLIC KEY-----", "-----END PUBLIC KEY-----");
}


QStatus CertECCUtil_GetCertCount(const String& encoded, size_t* count)
{
    *count = 0;
    return CountNumOfChunksFromEncoded(encoded,
                                       "-----BEGIN CERTIFICATE-----", "-----END CERTIFICATE-----", count);
}

QStatus CertECCUtil_GetVersionFromEncoded(const String& encoded, uint32_t* certVersion)
{
    qcc::String rawStr;
    QStatus status = RetrieveRawCertFromEncoded(encoded, rawStr);
    if (status != ER_OK) {
        return status;
    }
    uint8_t* raw = (uint8_t*) rawStr.data();
    size_t rawLen = rawStr.length();
    size_t minRawLen = sizeof(uint32_t);

    if (rawLen < minRawLen) {
        return ER_INVALID_DATA;
    }
    memcpy(certVersion, raw, sizeof(uint32_t));
    return ER_OK;
}

QStatus CertECCUtil_GetCertChain(const String& encoded, CertificateECC* certChain[], size_t count)
{
    qcc::String* chunks = new  qcc::String[count];

    QStatus status = RetrieveNumOfChunksFromEncoded(encoded,
                                                    "-----BEGIN CERTIFICATE-----", "-----END CERTIFICATE-----", chunks, count);
    if (status != ER_OK) {
        delete [] chunks;
        return status;
    }

    for (size_t idx = 0; idx < count; idx++) {
        uint32_t certVersion;
        status = CertECCUtil_GetVersionFromEncoded(chunks[idx], &certVersion);
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
        status = certChain[idx]->LoadEncoded(chunks[idx]);
        if (status != ER_OK) {
            break;
        }

    }
    delete [] chunks;
    return status;
}

void CertificateType0::SetIssuer(const ECCPublicKey* issuer)
{
    memcpy(&signable.issuer, issuer, sizeof(ECCPublicKey));
}

CertificateType0::CertificateType0(const ECCPublicKey* issuer, const uint8_t* externalDigest) : CertificateECC(0)
{
    SetIssuer(issuer);
    SetExternalDataDigest(externalDigest);
}

void CertificateType0::SetExternalDataDigest(const uint8_t* externalDataDigest) {
    memcpy(signable.digest, externalDataDigest, sizeof(signable.digest));
}

void CertificateType0::SetSig(const ECCSignature* sig)
{
    memcpy(&this->sig, sig, sizeof(ECCSignature));
}


QStatus CertificateType0::Sign(const ECCPrivateKey* dsaPrivateKey)
{
    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(dsaPrivateKey);
    return ecc.DSASignDigest(signable.digest, sizeof(signable.digest), &sig);
}

bool CertificateType0::VerifySignature()
{
    Crypto_ECC ecc;
    ecc.SetDSAPublicKey(&signable.issuer);
    return ecc.DSAVerifyDigest(signable.digest, sizeof(signable.digest), &sig) == ER_OK;
}

String CertificateType0::GetEncoded()
{
    qcc::String str;

    /* build the raw buffer */
    uint8_t raw[CERT_TYPE0_RAW_LEN];
    uint8_t* dest = raw;
    uint32_t certVersion = GetVersion();
    memcpy(dest, &certVersion, sizeof(certVersion));
    dest += sizeof(certVersion);
    memcpy(dest, &signable.issuer, sizeof(signable.issuer));
    dest += sizeof(signable.issuer);
    memcpy(dest, signable.digest, sizeof(signable.digest));
    dest += sizeof(signable.digest);
    memcpy(dest, &sig, sizeof(sig));
    dest += sizeof(sig);
    return EncodeCertRawByte(raw, sizeof(raw));
}

QStatus CertificateType0::LoadEncoded(const String& encoded)
{
    qcc::String rawStr;
    QStatus status = RetrieveRawCertFromEncoded(encoded, rawStr);
    if (status != ER_OK) {
        return status;
    }
    uint8_t* raw = (uint8_t*) rawStr.data();
    size_t rawLen = rawStr.length();

    if (rawLen != CERT_TYPE0_RAW_LEN) {
        return ER_INVALID_DATA;
    }
    uint32_t certVersion;
    memcpy(&certVersion, raw, sizeof(certVersion));
    if (certVersion != GetVersion()) {
        return ER_INVALID_DATA;
    }
    raw += sizeof(certVersion);

    memcpy(&signable.issuer, raw, sizeof(signable.issuer));
    raw += sizeof(signable.issuer);
    memcpy(&signable.digest, raw, sizeof(signable.digest));
    raw += sizeof(signable.digest);
    memcpy(&sig, raw, sizeof(sig));

    return ER_OK;
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

void CertificateType1::SetIssuer(const ECCPublicKey* issuer)
{
    memcpy(&signable.issuer, issuer, sizeof(ECCPublicKey));
}

void CertificateType1::SetSubject(const ECCPublicKey* subject)
{
    memcpy(&signable.subject, subject, sizeof(ECCPublicKey));
}

CertificateType1::CertificateType1(const ECCPublicKey* issuer, const ECCPublicKey* subject) : CertificateECC(1)
{
    SetIssuer(issuer);
    SetSubject(subject);
    signable.delegate = false;
}

void CertificateType1::SetExternalDataDigest(const uint8_t* externalDataDigest)
{
    memcpy(signable.digest, externalDataDigest, sizeof(signable.digest));
}

void CertificateType1::SetSig(const ECCSignature* sig)
{
    memcpy(&this->sig, sig, sizeof(ECCSignature));
}

QStatus CertificateType1::GenSignable(uint8_t* digest, size_t len)
{
    if (len != Crypto_SHA256::DIGEST_SIZE) {
        return ER_FAIL;
    }
    Crypto_SHA256 hash;

    hash.Init();
    uint32_t certVersion = GetVersion();
    hash.Update((const uint8_t*) &certVersion, sizeof(certVersion));
    hash.Update((const uint8_t*) &signable.issuer, sizeof(signable.issuer));
    hash.Update((const uint8_t*) &signable.subject, sizeof(signable.subject));
    hash.Update((const uint8_t*) &signable.validity, sizeof(signable.validity));
    hash.Update((const uint8_t*) &signable.delegate, sizeof(signable.delegate));
    hash.Update((const uint8_t*) signable.digest, sizeof(signable.digest));
    hash.GetDigest(digest);
    return ER_OK;
}

QStatus CertificateType1::Sign(const ECCPrivateKey* dsaPrivateKey)
{
    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(dsaPrivateKey);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    GenSignable(digest, sizeof(digest));
    return ecc.DSASignDigest(digest, sizeof(digest), &sig);
}

bool CertificateType1::VerifySignature()
{
    Crypto_ECC ecc;
    ecc.SetDSAPublicKey(&signable.issuer);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    GenSignable(digest, sizeof(digest));
    return ecc.DSAVerifyDigest(digest, sizeof(digest), &sig) == ER_OK;
}

String CertificateType1::GetEncoded()
{
    qcc::String str;

    /* build the raw buffer */
    uint8_t raw[CERT_TYPE1_RAW_LEN];
    uint8_t* dest = raw;
    uint32_t certVersion = GetVersion();
    memcpy(dest, &certVersion, sizeof(certVersion));
    dest += sizeof(certVersion);
    memcpy(dest, &signable.issuer, sizeof(signable.issuer));
    dest += sizeof(signable.issuer);
    memcpy(dest, &signable.subject, sizeof(signable.subject));
    dest += sizeof(signable.subject);
    memcpy(dest, &signable.validity, sizeof(signable.validity));
    dest += sizeof(signable.validity);
    memcpy(dest, &signable.delegate, sizeof(signable.delegate));
    dest += sizeof(signable.delegate);
    memcpy(dest, signable.digest, sizeof(signable.digest));
    dest += sizeof(signable.digest);
    memcpy(dest, &sig, sizeof(sig));
    dest += sizeof(sig);
    return EncodeCertRawByte(raw, sizeof(raw));
}

QStatus CertificateType1::LoadEncoded(const String& encoded)
{
    qcc::String rawStr;
    QStatus status = RetrieveRawCertFromEncoded(encoded, rawStr);
    if (status != ER_OK) {
        return status;
    }
    uint8_t* raw = (uint8_t*) rawStr.data();
    size_t rawLen = rawStr.length();

    if (rawLen != CERT_TYPE1_RAW_LEN) {
        return ER_INVALID_DATA;
    }
    uint32_t certVersion;
    memcpy(&certVersion, raw, sizeof(certVersion));
    if (certVersion != GetVersion()) {
        return ER_INVALID_DATA;
    }
    raw += sizeof(certVersion);

    memcpy(&signable.issuer, raw, sizeof(signable.issuer));
    raw += sizeof(signable.issuer);
    memcpy(&signable.subject, raw, sizeof(signable.subject));
    raw += sizeof(signable.subject);
    memcpy(&signable.validity, raw, sizeof(signable.validity));
    raw += sizeof(signable.validity);
    memcpy(&signable.delegate, raw, sizeof(signable.delegate));
    raw += sizeof(signable.delegate);
    memcpy(&signable.digest, raw, sizeof(signable.digest));
    raw += sizeof(signable.digest);
    memcpy(&sig, raw, sizeof(sig));

    return ER_OK;
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

void CertificateType2::SetIssuer(const ECCPublicKey* issuer)
{
    memcpy(&signable.issuer, issuer, sizeof(ECCPublicKey));
}

void CertificateType2::SetSubject(const ECCPublicKey* subject)
{
    memcpy(&signable.subject, subject, sizeof(ECCPublicKey));
}

void CertificateType2::SetGuild(const uint8_t* newGuild, size_t guildLen)
{
    size_t len = sizeof(signable.guild);
    memset(signable.guild, 0, len);
    if (len > guildLen) {
        len = guildLen;
    }
    memcpy(signable.guild, newGuild, len);
}

CertificateType2::CertificateType2(const ECCPublicKey* issuer, const ECCPublicKey* subject) : CertificateECC(2)
{
    SetIssuer(issuer);
    SetSubject(subject);
    signable.delegate = false;
}

void CertificateType2::SetExternalDataDigest(const uint8_t* externalDataDigest)
{
    memcpy(signable.digest, externalDataDigest, sizeof(signable.digest));
}

void CertificateType2::SetSig(const ECCSignature* sig)
{
    memcpy(&this->sig, sig, sizeof(ECCSignature));
}

QStatus CertificateType2::GenSignable(uint8_t* digest, size_t len)
{
    if (len != Crypto_SHA256::DIGEST_SIZE) {
        return ER_FAIL;
    }
    Crypto_SHA256 hash;

    hash.Init();
    uint32_t certVersion = GetVersion();
    hash.Update((const uint8_t*) &certVersion, sizeof(certVersion));
    hash.Update((const uint8_t*) &signable.issuer, sizeof(signable.issuer));
    hash.Update((const uint8_t*) &signable.subject, sizeof(signable.subject));
    hash.Update((const uint8_t*) &signable.validity, sizeof(signable.validity));
    hash.Update((const uint8_t*) &signable.delegate, sizeof(signable.delegate));
    hash.Update((const uint8_t*) signable.guild, sizeof(signable.guild));
    hash.Update((const uint8_t*) signable.digest, sizeof(signable.digest));
    hash.GetDigest(digest);
    return ER_OK;
}

QStatus CertificateType2::Sign(const ECCPrivateKey* dsaPrivateKey)
{
    Crypto_ECC ecc;
    ecc.SetDSAPrivateKey(dsaPrivateKey);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    GenSignable(digest, sizeof(digest));
    return ecc.DSASignDigest(digest, sizeof(digest), &sig);
}

bool CertificateType2::VerifySignature()
{
    Crypto_ECC ecc;
    ecc.SetDSAPublicKey(&signable.issuer);
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    GenSignable(digest, sizeof(digest));
    return ecc.DSAVerifyDigest(digest, sizeof(digest), &sig) == ER_OK;
}

String CertificateType2::GetEncoded()
{
    qcc::String str;

    /* build the raw buffer */
    uint8_t raw[CERT_TYPE2_RAW_LEN];
    uint8_t* dest = raw;
    uint32_t certVersion = GetVersion();
    memcpy(dest, &certVersion, sizeof(certVersion));
    dest += sizeof(certVersion);
    memcpy(dest, &signable.issuer, sizeof(signable.issuer));
    dest += sizeof(signable.issuer);
    memcpy(dest, &signable.subject, sizeof(signable.subject));
    dest += sizeof(signable.subject);
    memcpy(dest, &signable.validity, sizeof(signable.validity));
    dest += sizeof(signable.validity);
    memcpy(dest, &signable.delegate, sizeof(signable.delegate));
    dest += sizeof(signable.delegate);
    memcpy(dest, signable.guild, sizeof(signable.guild));
    dest += sizeof(signable.guild);
    memcpy(dest, signable.digest, sizeof(signable.digest));
    dest += sizeof(signable.digest);
    memcpy(dest, &sig, sizeof(sig));
    dest += sizeof(sig);
    return EncodeCertRawByte(raw, sizeof(raw));
}

QStatus CertificateType2::LoadEncoded(const String& encoded)
{
    qcc::String rawStr;
    QStatus status = RetrieveRawCertFromEncoded(encoded, rawStr);
    if (status != ER_OK) {
        return status;
    }
    uint8_t* raw = (uint8_t*) rawStr.data();
    size_t rawLen = rawStr.length();

    if (rawLen != CERT_TYPE2_RAW_LEN) {
        return ER_INVALID_DATA;
    }
    uint32_t certVersion;
    memcpy(&certVersion, raw, sizeof(certVersion));
    if (certVersion != GetVersion()) {
        return ER_INVALID_DATA;
    }
    raw += sizeof(certVersion);

    memcpy(&signable.issuer, raw, sizeof(signable.issuer));
    raw += sizeof(signable.issuer);
    memcpy(&signable.subject, raw, sizeof(signable.subject));
    raw += sizeof(signable.subject);
    memcpy(&signable.validity, raw, sizeof(signable.validity));
    raw += sizeof(signable.validity);
    memcpy(&signable.delegate, raw, sizeof(signable.delegate));
    raw += sizeof(signable.delegate);
    memcpy(signable.guild, raw, sizeof(signable.guild));
    raw += sizeof(signable.guild);
    memcpy(signable.digest, raw, sizeof(signable.digest));
    raw += sizeof(signable.digest);
    memcpy(&sig, raw, sizeof(sig));

    return ER_OK;
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
