/**
 * @file CertificateECC.cc
 *
 * Utilites for X.509 ECC Certificates
 */
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

#include <qcc/platform.h>
#include <qcc/Crypto.h>
#include <qcc/CertificateECC.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>
#include <qcc/time.h>

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
const qcc::String OID_BASIC_CONSTRAINTS = "2.5.29.19";
const qcc::String OID_DIG_SHA256 = "2.16.840.1.101.3.4.2.1";
const qcc::String OID_SUB_ALTNAME = "2.5.29.17";
const qcc::String OID_EKU = "2.5.29.37";
const qcc::String OID_CUSTOM_EKU_IDENTITY = "1.3.6.1.4.1.44924.1.1";
const qcc::String OID_CUSTOM_EKU_MEMBERSHIP = "1.3.6.1.4.1.44924.1.5";
const qcc::String OID_CUSTOM_DIGEST = "1.3.6.1.4.1.44924.1.2";
const qcc::String OID_CUSTOM_SECURITY_GROUP_ID = "1.3.6.1.4.1.44924.1.3";
const qcc::String OID_CUSTOM_IDENTITY_ALIAS = "1.3.6.1.4.1.44924.1.4";
const qcc::String OID_AUTHORITY_KEY_IDENTIFIER = "2.5.29.35";


static const char* EC_PRIVATE_KEY_PEM_BEGIN_TAG = "-----BEGIN EC PRIVATE KEY-----";
static const char* EC_PRIVATE_KEY_PEM_END_TAG = "-----END EC PRIVATE KEY-----";
static const char* PUBLIC_KEY_PEM_BEGIN_TAG = "-----BEGIN PUBLIC KEY-----";
static const char* PUBLIC_KEY_PEM_END_TAG = "-----END PUBLIC KEY-----";
static const char* CERTIFICATE_PEM_BEGIN_TAG = "-----BEGIN CERTIFICATE-----";
static const char* CERTIFICATE_PEM_END_TAG = "-----END CERTIFICATE-----";

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

static QStatus StripTags(String& pem, const char* beg, const char* end)
{
    size_t pos;

    pos = pem.find(beg);
    if (pos == qcc::String::npos) {
        return ER_INVALID_DATA;
    }
    pem = pem.erase(0, strlen(beg));
    pos = pem.find(end);
    if (pos != qcc::String::npos) {
        pem = pem.erase(pos);
    }

    return ER_OK;
}

QStatus AJ_CALL CertificateX509::EncodePrivateKeyPEM(const ECCPrivateKey* privateKey, String& encoded)
{
    QStatus status;
    qcc::String beg = EC_PRIVATE_KEY_PEM_BEGIN_TAG;
    qcc::String end = EC_PRIVATE_KEY_PEM_END_TAG;
    qcc::String der;
    qcc::String prv;
    qcc::String oid = OID_CRV_PRIME256V1;
    qcc::String pem;

    size_t privateKeyLength = privateKey->GetSize();
    uint8_t* privateKeyBytes = new uint8_t[privateKeyLength];

    status = privateKey->Export(privateKeyBytes, &privateKeyLength);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error exporting private key"));
        goto Exit;
    }

    prv.assign((const char*)privateKeyBytes, privateKeyLength);
    status = Crypto_ASN1::Encode(der, "(ixc(o))", 1, &prv, 0, &oid);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error encoding private key in PEM format"));
        goto Exit;
    }
    status = Crypto_ASN1::EncodeBase64(der, pem);
    if (ER_OK != status) {
        goto Exit;
    }
    encoded = beg + "\n" + pem + end;

Exit:

    if (NULL != privateKeyBytes) {
        qcc::ClearMemory(privateKeyBytes, privateKeyLength);
        delete[] privateKeyBytes;
    }

    return status;
}

QStatus AJ_CALL CertificateX509::DecodePrivateKeyPEM(const String& encoded, ECCPrivateKey* privateKey)
{
    QStatus status;
    qcc::String pem = encoded;

    status = StripTags(pem, EC_PRIVATE_KEY_PEM_BEGIN_TAG, EC_PRIVATE_KEY_PEM_END_TAG);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error decoding private key from PEM. Only support tag -----BEGIN EC PRIVATE KEY-----, tag -----END EC PRIVATE KEY-----, and key"));
        return status;
    }
    qcc::String der;
    status = Crypto_ASN1::DecodeBase64(pem, der);
    if (ER_OK != status) {
        return status;
    }
    uint32_t ver;
    qcc::String prv;
    qcc::String oid;
    qcc::String rem;
    bool hasOID = true;
    /* the OID and public key fields are optional */
    status = Crypto_ASN1::Decode(der, "(ixc(o).)", &ver, &prv, 0, &oid, &rem);
    if (ER_OK != status) {
        status = Crypto_ASN1::Decode(der, "(ixc(o))", &ver, &prv, 0, &oid);
        if (ER_OK != status) {
            status = Crypto_ASN1::Decode(der, "(ixc)", &ver, &prv, 0);
            hasOID = false;
        }
    }
    if (ER_OK != status) {
        return status;
    }
    if (1 != ver) {
        return ER_FAIL;
    }
    if (hasOID) {
        /* check the oid */
        if (OID_CRV_PRIME256V1 != oid) {
            return ER_FAIL;
        }
    }

    return privateKey->Import((const uint8_t*) prv.data(), prv.size());
}

QStatus AJ_CALL CertificateX509::EncodePublicKeyPEM(const ECCPublicKey* publicKey, String& encoded)
{
    QStatus status;
    qcc::String beg = PUBLIC_KEY_PEM_BEGIN_TAG;
    qcc::String end = PUBLIC_KEY_PEM_END_TAG;
    qcc::String der;
    qcc::String oid1 = OID_KEY_ECC;
    qcc::String oid2 = OID_CRV_PRIME256V1;
    qcc::String pem;

    size_t publicKeySize = publicKey->GetSize();
    uint8_t* publicKeyBytes = new uint8_t[publicKeySize];

    status = publicKey->Export(publicKeyBytes, &publicKeySize);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to export public key bytes"));
        delete[] publicKeyBytes;
        return status;
    }

    // Uncompressed points only
    qcc::String key(0x4);
    key += qcc::String((const char*)publicKeyBytes, publicKeySize);
    delete[] publicKeyBytes;
    status = Crypto_ASN1::Encode(der, "((oo)b)", &oid1, &oid2, &key, 8 * key.size());
    if (ER_OK != status) {
        return status;
    }
    status = Crypto_ASN1::EncodeBase64(der, pem);
    if (ER_OK != status) {
        return status;
    }
    encoded = beg + "\n" + pem + end;

    return ER_OK;
}

QStatus AJ_CALL CertificateX509::DecodePublicKeyPEM(const String& encoded, ECCPublicKey* publicKey)
{
    QStatus status;
    qcc::String pem = encoded;

    status = StripTags(pem, PUBLIC_KEY_PEM_BEGIN_TAG, PUBLIC_KEY_PEM_END_TAG);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error decoding private key from PEM. Only support tag -----BEGIN PUBLIC KEY-----, tag -----END PUBLIC KEY-----, and key"));
        return status;
    }
    qcc::String der;
    status = Crypto_ASN1::DecodeBase64(pem, der);
    if (ER_OK != status) {
        return status;
    }

    qcc::String oid1;
    qcc::String oid2;
    qcc::String key;
    size_t keylen;
    status = Crypto_ASN1::Decode(der, "((oo)b)", &oid1, &oid2, &key, &keylen);
    if (ER_OK != status) {
        return status;
    }
    if (OID_KEY_ECC != oid1) {
        QCC_LogError(ER_FAIL, ("First OID did not match ECC oid"));
        return ER_FAIL;
    }
    if (OID_CRV_PRIME256V1 != oid2) {
        QCC_LogError(ER_FAIL, ("Second OID did not match P-256 OID"));
        return ER_FAIL;
    }
    // Uncompressed points only
    if (0x4 != *key.data()) {
        QCC_LogError(ER_FAIL, ("Key data is not in uncompressed format; other formats are not supported"));
        return ER_FAIL;
    }

    return publicKey->Import((const uint8_t*)(key.data() + 1), key.size() - 1);
}

/* To be deprecated versions of {Encode|Decode}{Private|Public}KeyPEM */
QStatus AJ_CALL CertificateX509::EncodePrivateKeyPEM(const uint8_t* privateKey, size_t len, String& encoded)
{
    ECCPrivateKey keyObject;

    QStatus status = keyObject.Import(privateKey, len);
    if (ER_OK != status) {
        return status;
    }
    return CertificateX509::EncodePrivateKeyPEM(&keyObject, encoded);
}

QStatus AJ_CALL CertificateX509::DecodePrivateKeyPEM(const String& encoded, uint8_t* privateKey, size_t len)
{
    ECCPrivateKey keyObject;

    QStatus status = CertificateX509::DecodePrivateKeyPEM(encoded, &keyObject);
    if (ER_OK != status) {
        return status;
    }

    return keyObject.Export(privateKey, &len);
}

QStatus AJ_CALL EncodePublicKeyPEM(const uint8_t* publicKey, size_t len, String& encoded)
{
    ECCPublicKey keyObject;

    QStatus status = keyObject.Import(publicKey, len);
    if (ER_OK != status) {
        return status;
    }

    return CertificateX509::EncodePublicKeyPEM(&keyObject, encoded);
}

QStatus AJ_CALL DecodePublicKeyPEM(const String& encoded, uint8_t* publicKey, size_t len)
{
    ECCPublicKey keyObject;

    QStatus status = CertificateX509::DecodePublicKeyPEM(encoded, &keyObject);
    if (ER_OK != status) {
        return status;
    }

    return keyObject.Export(publicKey, &len);
}
/* End of to-be-deprecated functions */



QStatus CertificateX509::DecodeCertificateName(const qcc::String& dn, CertificateX509::DistinguishedName& name)
{
    QStatus status = ER_OK;
    qcc::String tmp = dn;

    while ((ER_OK == status) && (tmp.size())) {
        qcc::String oid;
        qcc::String str;
        qcc::String rem;
        status = Crypto_ASN1::Decode(tmp, "{(o.)}.", &oid, &str, &rem);
        if (ER_OK != status) {
            QCC_LogError(status, ("Error decoding distinguished name"));
            return status;
        }
        if (OID_DN_OU == oid) {
            qcc::String val;
            status = Crypto_ASN1::Decode(str, "u", &val);
            if (ER_OK != status) {
                QCC_LogError(status, ("Error decoding OU field of the distinguished name"));
                return status;
            }
            name.SetOU((const uint8_t*) val.data(), val.length());
        } else if (OID_DN_CN == oid) {
            qcc::String val;
            status = Crypto_ASN1::Decode(str, "u", &val);
            if (ER_OK != status) {
                QCC_LogError(status, ("Error decoding CN field of the distinguished name"));
                return status;
            }
            name.SetCN((const uint8_t*) val.data(), val.length());
        }
        /* do not parse the other fields of the distinguished name */
        tmp = rem;
    }

    return status;
}

QStatus CertificateX509::EncodeCertificateName(qcc::String& dn, const CertificateX509::DistinguishedName& name) const
{
    qcc::String ouOID;
    qcc::String cnOID;
    qcc::String ou;
    qcc::String cn;
    if (name.ouLen > 0) {
        ouOID = OID_DN_OU;
        ou.assign((const char*) name.ou, name.ouLen);
    }
    if (name.cnLen > 0) {
        cnOID = OID_DN_CN;
        cn.assign((const char*) name.cn, name.cnLen);
    }
    if ((name.ouLen > 0) && (name.cnLen > 0)) {
        return Crypto_ASN1::Encode(dn, "{(ou)}{(ou)}", &ouOID, &ou, &cnOID, &cn);
    } else if (name.ouLen > 0) {
        return Crypto_ASN1::Encode(dn, "{(ou)}", &ouOID, &ou);
    } else if (name.cnLen > 0) {
        return Crypto_ASN1::Encode(dn, "{(ou)}", &cnOID, &cn);
    }
    return ER_OK;
}

static QStatus DecodeTime(uint64_t& epoch, const qcc::String& t)
{
    struct tm tm;

    /* Parse the string into the tm struct.  Can't use strptime since it not
        available in some platforms like Android or Windows */
    if (0xD == t.size()) {
        /* the time format is "%y%m%d%H%M%SZ".  Sample input: 150205230725Z */
        if (sscanf(t.c_str(), "%2d%2d%2d%2d%2d%2dZ", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) != 6) {
            return ER_FAIL;
        }
        if ((tm.tm_year >= 0) && (tm.tm_year <= 68)) {
            tm.tm_year += 100;    /* tm_year holds  Year - 1900 */
        }
    } else if (0xF == t.size()) {
        /* the time format is "%Y%m%d%H%M%SZ".  Sample input: 20150205230725Z*/
        if (sscanf(t.c_str(), "%4d%2d%2d%2d%2d%2dZ", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) != 6) {
            return ER_FAIL;
        }
        tm.tm_year -= 1900;    /* tm_year hold Year - 1900 */
    } else {
        return ER_FAIL;
    }
    tm.tm_mon--;  /* month's range is [0-11] */
    tm.tm_isdst = 0;

    /* save the tm_hour value since mktime can modify that value if daylight
     *  savings time is in effect
     */
    int originalTmHour = tm.tm_hour;

    /* Compute the GMT time from struct tm.
        Can't use timegm since it is not available in some platforms like Android and Windows */

    int64_t localTime = ConvertStructureToTime(&tm);
    if (localTime < 0) {
        return ER_FAIL;
    }
    struct tm* gtm = ConvertTimeToStructure(&localTime);
    if (!gtm) {
        return ER_FAIL;
    }
    /* figure the time zone offset */
    int32_t tzDiff = gtm->tm_hour - originalTmHour;
    /* some time zones are at 30 minute or 45 minute boundary */
    int32_t minuteDiff = gtm->tm_min - tm.tm_min;
    if (tzDiff < -12) {
        tzDiff += 24;
    } else if (tzDiff > 12) {
        tzDiff = 24 - tzDiff;
    }
    epoch = localTime - (tzDiff * 3600) - (minuteDiff * 60);
    return ER_OK;
}

QStatus CertificateX509::DecodeCertificateTime(const qcc::String& time)
{
    QStatus status;
    qcc::String tmp;
    qcc::String time1;
    qcc::String time2;

    tmp = time;
    status = Crypto_ASN1::Decode(tmp, "t.", &time1, &time2);
    if (ER_OK != status) {
        status = Crypto_ASN1::Decode(tmp, "T.", &time1, &time2);
    }
    if (ER_OK != status) {
        return status;
    }
    tmp = time2;
    status = Crypto_ASN1::Decode(tmp, "t", &time2);
    if (ER_OK != status) {
        status = Crypto_ASN1::Decode(tmp, "T", &time2);
    }
    if (ER_OK != status) {
        return status;
    }

    if ((0xD != time1.size()) && (0xF != time1.size())) {
        return ER_FAIL;
    }
    if ((0xD != time2.size()) && (0xF != time2.size())) {
        return ER_FAIL;
    }

    char buf1[16];
    char buf2[16];
    memset(buf1, 0, 16);
    memset(buf2, 0, 16);
    memcpy(buf1, time1.data(), time1.size());
    memcpy(buf2, time2.data(), time2.size());
    status = DecodeTime(validity.validFrom, time1);
    if (ER_OK != status) {
        return status;
    }
    status = DecodeTime(validity.validTo, time2);
    if (ER_OK != status) {
        return status;
    }

    return ER_OK;
}

static QStatus EncodeTime(uint64_t epoch, qcc::String& t)
{
    struct tm* ptm = ConvertTimeToStructure((int64_t*)&epoch);
    if (!ptm) {
        return ER_FAIL;
    }
    /**
     * RFC5280 section 4.1.2.5 specifies that
     *      validity date through the year 2049 as UTC time YYMMDDHHMMSSZ
     *      validity date in the year 2050 or later as UTC time YYYYMMDDHHMMSSZ
     * The value 150 means 2050 - 1900 where tm_year is based.
     */
    const char* format = (ptm->tm_year < 150) ? "%y%m%d%H%M%SZ" : "%Y%m%d%H%M%SZ";
    char buf[16];
    size_t ret;
    ret = FormatTime(buf, sizeof(buf), format, ptm);
    if (!ret) {
        return ER_FAIL;
    }
    t = qcc::String(buf, ret);
    return ER_OK;
}

QStatus CertificateX509::EncodeCertificateTime(qcc::String& time) const
{
    QStatus status;
    qcc::String time1;
    qcc::String time2;
    status = EncodeTime(validity.validFrom, time1);
    if (ER_OK != status) {
        return status;
    }
    status = EncodeTime(validity.validTo, time2);
    if (ER_OK != status) {
        return status;
    }
    qcc::String fmt;
    fmt += 0xD == time1.size() ? "t" : "T";
    fmt += 0xD == time2.size() ? "t" : "T";
    status = Crypto_ASN1::Encode(time, fmt.c_str(), &time1, &time2);
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
    if (1 + publickey.GetSize() != key.size()) {
        return ER_FAIL;
    }
    // Uncompressed points only
    if (0x4 != *key.data()) {
        return ER_FAIL;
    }
    status = publickey.Import((const uint8_t*)(key.data() + 1), key.size() - 1);

    return status;
}

QStatus CertificateX509::EncodeCertificatePub(qcc::String& pub) const
{
    QStatus status = ER_OK;
    qcc::String oid1 = OID_KEY_ECC;
    qcc::String oid2 = OID_CRV_PRIME256V1;

    size_t publicKeySize = publickey.GetSize();
    uint8_t* publicKeyBytes = new uint8_t[publicKeySize];

    status = publickey.Export(publicKeyBytes, &publicKeySize);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to export public key bytes"));
        delete[] publicKeyBytes;
        return status;
    }

    // Uncompressed points only
    qcc::String key(0x4);
    key += qcc::String((const char*) publicKeyBytes, publicKeySize);
    status = Crypto_ASN1::Encode(pub, "(oo)b", &oid1, &oid2, &key, 8 * key.size());

    delete[] publicKeyBytes;
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
        qcc::String critical;
        qcc::String rem;
        status = Crypto_ASN1::Decode(tmp, "(ozx).", &oid, &critical, &str, &rem);
        if (ER_OK != status) {
            /* the critical boolean flag is not present */
            status = Crypto_ASN1::Decode(tmp, "(ox).", &oid, &str, &rem);
            if (ER_OK != status) {
                return status;
            }
        }
        if (OID_BASIC_CONSTRAINTS == oid) {
            qcc::String opt;
            status = Crypto_ASN1::Decode(str, "(.)", &opt);
            if (ER_OK != status) {
                status = ER_OK;  /* The sequence can be empty since CA is false by default */
            } else if (opt.size()) {
                /* do not parse the path len field */
                status = Crypto_ASN1::Decode(opt, "z*", &ca);
                if (ER_OK != status) {
                    return status;
                }
            }
        } else if (OID_EKU == oid) {
            bool identityEKUPresent = false;
            bool membershipEKUPresent = false;

            status = Crypto_ASN1::Decode(str, "(.)", &tmp);
            if (ER_OK != status) {
                return status;
            }
            /* There has to be at least one EKU listed per the RFC. If tmp is empty,
             * the cert is malformed. */
            if (0 == tmp.size()) {
                return ER_FAIL;
            }
            while ((ER_OK == status) && (tmp.size() > 0)) {
                qcc::String eku;
                qcc::String rem;
                status = Crypto_ASN1::Decode(tmp, "o.", &eku, &rem);
                if (ER_OK == status) {
                    if (OID_CUSTOM_EKU_IDENTITY == eku) {
                        if (identityEKUPresent) {
                            QCC_DbgPrintf(("Identity EKU seen multiple times"));
                        }
                        identityEKUPresent = true;
                    } else if (OID_CUSTOM_EKU_MEMBERSHIP == eku) {
                        if (membershipEKUPresent) {
                            QCC_DbgPrintf(("Membership EKU seen multiple times"));
                        }
                        membershipEKUPresent = true;
                    } else {
                        /* An unrelated EKU. Unexpected but not fatal. Log and move on. */
                        QCC_DbgPrintf(("Not fatal: Saw unexpected EKU %s", eku.c_str()));
                    }

                    tmp = rem;
                }
            }

            if (ER_OK != status) {
                QCC_LogError(status, ("Failed while decoding EKU extension: %s", QCC_StatusText(status)));
                return status;
            }

            if (identityEKUPresent && membershipEKUPresent) {
                type = UNRESTRICTED_CERTIFICATE;
            } else if (identityEKUPresent) {
                type = IDENTITY_CERTIFICATE;
            } else if (membershipEKUPresent) {
                type = MEMBERSHIP_CERTIFICATE;
            } else {
                /* EKUs are present but none belonging to known AllJoyn EKUs, and so certificate is not valid
                 * for any AllJoyn purpose. */
                type = INVALID_CERTIFICATE;
            }
        } else if (OID_SUB_ALTNAME == oid) {
            qcc::String otherNameOid;
            qcc::String otherName;
            /* only interested in the otherName field [0] */
            if (ER_OK == Crypto_ASN1::Decode(str, "(c(o.))", 0, &otherNameOid, &otherName)) {
                if ((OID_CUSTOM_SECURITY_GROUP_ID == otherNameOid) || (OID_CUSTOM_IDENTITY_ALIAS == otherNameOid)) {
                    status = Crypto_ASN1::Decode(otherName, "c(x)", 0, &subjectAltName);
                    if (ER_OK != status) {
                        return status;
                    }
                }
            }
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
        } else if (OID_AUTHORITY_KEY_IDENTIFIER == oid) {
            status = Crypto_ASN1::Decode(str, "(c(.))", 0, &aki);
            if (ER_OK != status) {
                return status;
            }
        }
        tmp = rem;
    }
    return status;
}

QStatus CertificateX509::EncodeCertificateExt(qcc::String& ext) const
{
    QStatus status = ER_OK;
    qcc::String oid;
    qcc::String opt;
    qcc::String tmp;
    qcc::String tmp1;
    qcc::String raw;

    if (ca) {
        status = Crypto_ASN1::Encode(tmp, "(z)", ca);
    } else {
        status = Crypto_ASN1::Encode(tmp, "()");
    }
    if (ER_OK != status) {
        return status;
    }
    oid = OID_BASIC_CONSTRAINTS;
    status = Crypto_ASN1::Encode(raw, "(ox)", &oid, &tmp);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error encoding certificate basic constraint"));
        return status;
    }

    tmp.clear();
    switch (type) {
    case UNRESTRICTED_CERTIFICATE:
        status = Crypto_ASN1::Encode(tmp, "(oo)", &OID_CUSTOM_EKU_IDENTITY, &OID_CUSTOM_EKU_MEMBERSHIP);
        if (ER_OK != status) {
            return status;
        }
        break;

    case IDENTITY_CERTIFICATE:
        status = Crypto_ASN1::Encode(tmp, "(o)", &OID_CUSTOM_EKU_IDENTITY);
        if (ER_OK != status) {
            return status;
        }
        break;

    case MEMBERSHIP_CERTIFICATE:
        status = Crypto_ASN1::Encode(tmp, "(o)", &OID_CUSTOM_EKU_MEMBERSHIP);
        if (ER_OK != status) {
            return status;
        }
        break;

    default: /* Shouldn't ever happen. */
        assert(false);
        QCC_LogError(ER_FAIL, ("Encountered unknown type %u", type));
        return ER_FAIL;
    }

    oid = OID_EKU;
    status = Crypto_ASN1::Encode(raw, "(ox)", &oid, &tmp);
    if (ER_OK != status) {
        return status;
    }

    if (subjectAltName.size()) {
        qcc::String octet;
        if ((type == IDENTITY_CERTIFICATE) || (type == MEMBERSHIP_CERTIFICATE)) {
            qcc::String otherNameOid;
            if (type == IDENTITY_CERTIFICATE) {
                otherNameOid = OID_CUSTOM_IDENTITY_ALIAS;
            } else {
                otherNameOid = OID_CUSTOM_SECURITY_GROUP_ID;
            }
            status = Crypto_ASN1::Encode(octet, "(c(oc(x)))", 0, &otherNameOid, 0, &subjectAltName);
            if (ER_OK != status) {
                return status;
            }
        } else {
            octet = subjectAltName;
        }
        oid = OID_SUB_ALTNAME;
        status = Crypto_ASN1::Encode(raw, "(ox)", &oid, &octet);
        if (ER_OK != status) {
            return status;
        }
    }
    if (aki.size()) {
        String tmp;
        status = Crypto_ASN1::Encode(tmp, "c(R)", 0, &aki);
        if (ER_OK != status) {
            return status;
        }
        String akiOctet;
        status = Crypto_ASN1::Encode(akiOctet, "(R)", &tmp);
        if (ER_OK != status) {
            return status;
        }
        oid = OID_AUTHORITY_KEY_IDENTIFIER;
        status = Crypto_ASN1::Encode(raw, "(ox)", &oid, &akiOctet);
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
    qcc::String time;
    qcc::String pub;
    qcc::String ext;
    qcc::String serialStr;

    status = Crypto_ASN1::Decode(tbs, "(c(i)l(o)(.)(.)(.)(.).)",
                                 0, &x509Version, &serialStr, &oid, &iss, &time, &sub, &pub, &ext);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error decoding certificate"));
        return status;
    }
    if (X509_VERSION_3 != x509Version) {
        QCC_LogError(status, ("Certificate not X.509v3"));
        return ER_FAIL;
    }
    this->SetSerial((const uint8_t*)serialStr.data(), serialStr.size());
    if (OID_SIG_ECDSA_SHA256 != oid) {
        QCC_LogError(status, ("Certificate signature must be SHA-256"));
        return ER_FAIL;
    }
    status = DecodeCertificateName(iss, issuer);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error decoding certificate issuer"));
        return status;
    }
    status = DecodeCertificateTime(time);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error decoding certificate validity period"));
        return status;
    }
    status = DecodeCertificateName(sub, subject);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error decoding certificate subject"));
        return status;
    }
    status = DecodeCertificatePub(pub);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error decoding certificate subject public key"));
        return status;
    }
    status = DecodeCertificateExt(ext);
    if (ER_OK != status) {
        QCC_LogError(status, ("Error decoding certificate extensions"));
    }

    return status;
}

QStatus CertificateX509::EncodeCertificateTBS()
{
    QStatus status = ER_OK;
    uint32_t x509Version = X509_VERSION_3;
    qcc::String oid = OID_SIG_ECDSA_SHA256;
    qcc::String iss;
    qcc::String sub;
    qcc::String time;
    qcc::String pub;
    qcc::String ext;
    qcc::String serialStr;

    status = EncodeCertificateName(iss, issuer);
    if (ER_OK != status) {
        return status;
    }
    status = EncodeCertificateTime(time);
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
    serialStr.assign((const char*)serial, serialLen);
    tbs.clear(tbs.size()); //empty the tbs string.
    status = Crypto_ASN1::Encode(tbs, "(c(i)l(o)(R)(R)(R)(R)R)",
                                 0, x509Version, &serialStr, &oid, &iss, &time, &sub, &pub, &ext);

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

QStatus CertificateX509::EncodeCertificateSig(qcc::String& sig) const
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
    tbs.clear(tbs.size());
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
    if (ER_OK != status) {
        QCC_LogError(status, ("Error decoding certificate signature"));
    }

    return status;
}

QStatus CertificateX509::EncodeCertificateDER(qcc::String& der) const
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
    qcc::String tag1 = CERTIFICATE_PEM_BEGIN_TAG;
    qcc::String tag2 = CERTIFICATE_PEM_END_TAG;

    pos = pem.find(tag1);
    if (pos == qcc::String::npos) {
        QCC_LogError(ER_INVALID_DATA, ("Error decoding certificate data from PEM. Only support tag -----BEGIN CERTIFICATE-----, tag -----END CERTIFICATE-----, and data"));
        return ER_INVALID_DATA;
    }
    rem = pem.substr(pos + tag1.size());

    pos = rem.find(tag2);
    if (pos == qcc::String::npos) {
        QCC_LogError(ER_INVALID_DATA, ("Error decoding certificate data from PEM. Only support tag -----BEGIN CERTIFICATE-----, tag -----END CERTIFICATE-----, and data"));
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

QStatus AJ_CALL CertificateX509::EncodeCertificatePEM(qcc::String& der, qcc::String& pem)
{
    QStatus status;
    qcc::String rem;
    qcc::String tag1 = CERTIFICATE_PEM_BEGIN_TAG;
    qcc::String tag2 = CERTIFICATE_PEM_END_TAG;

    status = Crypto_ASN1::EncodeBase64(der, rem);
    if (ER_OK != status) {
        return status;
    }
    pem = tag1 + "\n" + rem + tag2;

    return status;
}

QStatus CertificateX509::EncodeCertificatePEM(qcc::String& pem) const
{
    qcc::String der;
    QStatus status = EncodeCertificateDER(der);
    if (ER_OK != status) {
        return status;
    }
    return EncodeCertificatePEM(der, pem);
}

QStatus CertificateX509::VerifyValidity() const
{
    uint64_t currentTime = GetEpochTimestamp() / 1000;

    if ((validity.validFrom > currentTime) || (validity.validTo < currentTime)) {
        return ER_FAIL;
    }

    return ER_OK;
}

QStatus CertificateX509::Verify() const
{
    return Verify(&publickey);
}

QStatus CertificateX509::Verify(const ECCPublicKey* key) const
{
    if (key->empty()) {
        return ER_FAIL;
    }
    Crypto_ECC ecc;
    ecc.SetDSAPublicKey(key);
    return ecc.DSAVerify((const uint8_t*) tbs.data(), tbs.size(), &signature);
}

QStatus CertificateX509::Verify(const KeyInfoNISTP256& ta) const
{
    QStatus status;
    status = VerifyValidity();
    if (ER_OK != status) {
        QCC_DbgPrintf(("Invalid validity period"));
        return status;
    }
    return Verify(ta.GetPublicKey());
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

QStatus AJ_CALL CertificateX509::GenerateAuthorityKeyId(const ECCPublicKey* issuerPubKey, String& authorityKeyId)
{
    Crypto_SHA256 sha;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    sha.Init();
    sha.Update((const uint8_t*) issuerPubKey, sizeof(ECCPublicKey));
    sha.GetDigest(digest);
    uint8_t keyId[AUTHORITY_KEY_ID_SZ];
    /* the format of the authority key identifier is 64 bits:
     * first 4 bits is 0100
     * the next 60 bits is the least significant 60 bits of the digest
     */
    memcpy(keyId, &digest[Crypto_SHA256::DIGEST_SIZE - AUTHORITY_KEY_ID_SZ], AUTHORITY_KEY_ID_SZ);
    keyId[0] = 0x40 | (keyId[0] & 0xF);
    authorityKeyId.assign((const char*) keyId, sizeof(keyId));
    return ER_OK;
}

QStatus CertificateX509::GenerateAuthorityKeyId(const ECCPublicKey* issuerPubKey)
{
    return GenerateAuthorityKeyId(issuerPubKey, aki);
}

QStatus CertificateX509::SignAndGenerateAuthorityKeyId(const ECCPrivateKey* privateKey, const ECCPublicKey* publicKey)
{
    QStatus status = GenerateAuthorityKeyId(publicKey);
    if (ER_OK != status) {
        return status;
    }
    return Sign(privateKey);
}

String CertificateX509::ToString() const
{
    qcc::String str("Certificate:\n");
    /* Printing out the serial number as a string can produce control characters that cause output issues
     * on Windows. Sometimes the rest of the output will not appear. Instead, we only print it as a hex string. */
    str += "serial:     0x" + BytesToHexString(serial, serialLen) + "\n";
    if ((GetIssuerOULength() > 0) || (GetIssuerCNLength() > 0)) {
        str += "issuer: ";
        bool addComma = false;
        if (GetIssuerOULength() > 0) {
            str += "OU= " + qcc::String((const char*) GetIssuerOU(), GetIssuerOULength()) +
                   " (0x" + BytesToHexString(GetIssuerOU(), GetIssuerOULength()) + ")";
            addComma = true;
        }
        if (GetIssuerCNLength() > 0) {
            if (addComma) {
                str += ", ";
            }
            str += "CN= " + qcc::String((const char*) GetIssuerCN(), GetIssuerCNLength()) +
                   " (0x" + BytesToHexString(GetIssuerCN(), GetIssuerCNLength()) + ")";
        }
        str += "\n";
    }

    if ((GetSubjectOULength() > 0) || (GetSubjectCNLength() > 0)) {
        str += "subject: ";
        bool addComma = false;
        if (GetSubjectOULength() > 0) {
            str += "OU= " + qcc::String((const char*) GetSubjectOU(), GetSubjectOULength()) +
                   " (0x" + BytesToHexString(GetSubjectOU(), GetSubjectOULength()) + ")";
            addComma = true;
        }
        if (GetSubjectCNLength() > 0) {
            if (addComma) {
                str += ", ";
            }
            str += "CN= " + qcc::String((const char*) GetSubjectCN(), GetSubjectCNLength()) +
                   " (0x" + BytesToHexString(GetSubjectCN(), GetSubjectCNLength()) + ")";
        }
        str += "\n";
    }
    str += "publickey: " + publickey.ToString() + "\n";
    str += "ca:        " + BytesToHexString((const uint8_t*) &ca, sizeof(uint8_t)) + "\n";
    str += "validity: not-before ";
    str += U64ToString(GetValidity()->validFrom);
    qcc::String tmpTime;
    EncodeTime(GetValidity()->validFrom, tmpTime);
    str += " (" + tmpTime + ") ";
    str += " not-after ";
    str += U64ToString(GetValidity()->validTo);
    EncodeTime(GetValidity()->validTo, tmpTime);
    str += " (" + tmpTime + ") ";
    str += "\n";
    if (digest.size()) {
        str += "digest:    " + BytesToHexString((const uint8_t*) digest.data(), digest.size()) + "\n";
    }
    if (subjectAltName.size()) {
        if (GetType() == IDENTITY_CERTIFICATE) {
            str += "alias:     " + subjectAltName;
        } else if (GetType() == MEMBERSHIP_CERTIFICATE) {
            str += "security group Id:     0x" + BytesToHexString((const uint8_t*) subjectAltName.data(), subjectAltName.size());
        } else {
            str += "subject alt name:     " + subjectAltName;
        }
        str += "\n";
    }
    if (aki.size()) {
        str += "aki:     " + BytesToHexString((const uint8_t*) aki.data(), aki.size()) + "\n";
    }
    str += "signature: " + BytesToHexString((const uint8_t*) &signature, sizeof(signature)) + "\n";
    return str;
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

QStatus AJ_CALL CertificateX509::DecodeCertChainPEM(const String& encoded, CertificateX509* certs, size_t count)
{
    qcc::String* chunks = new  qcc::String[count];

    QStatus status = RetrieveNumOfChunksFromPEM(encoded, CERTIFICATE_PEM_BEGIN_TAG, CERTIFICATE_PEM_END_TAG, chunks, count);
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

bool CertificateX509::IsIssuerOf(const CertificateX509& issuedCertificate) const
{
    return ((this->IsDNEqual(issuedCertificate.GetIssuerCN(), issuedCertificate.GetIssuerCNLength(),
                             issuedCertificate.GetIssuerOU(), issuedCertificate.GetIssuerOULength())) &&
            (ER_OK == issuedCertificate.Verify(this->GetSubjectPublicKey())));
}

bool CertificateX509::IsDNEqual(const CertificateX509& other) const
{
    return (this->IsDNEqual(other.GetSubjectCN(), other.GetSubjectCNLength(), other.GetSubjectOU(), other.GetSubjectOULength()));
}

bool CertificateX509::IsDNEqual(const uint8_t* cn, const size_t cnLength, const uint8_t* ou, const size_t ouLength) const
{
    /* Check lengths are equal. */
    if ((this->GetSubjectCNLength() != cnLength) || (this->GetSubjectOULength() != ouLength)) {
        return false;
    }

    /* For CN and OU, if either is NULL, they must be equal (both NULL). */
    if ((NULL == this->GetSubjectCN()) || (NULL == cn)) {
        if (this->GetSubjectCN() != cn) {
            return false;
        }
    }

    if ((NULL == this->GetSubjectOU()) || (NULL == ou)) {
        if (this->GetSubjectOU() != ou) {
            return false;
        }
    }

    /* At this point either both are NULL or neither is, for each pair of CNs and OUs. */
    if ((NULL != this->GetSubjectCN()) &&
        (0 != memcmp(this->GetSubjectCN(), cn, cnLength))) {
        return false;
    }

    if ((NULL != this->GetSubjectOU()) &&
        (0 != memcmp(this->GetSubjectOU(), ou, ouLength))) {
        return false;
    }

    return true;
}

bool CertificateX509::IsSubjectPublicKeyEqual(const ECCPublicKey* publicKey) const
{
    return ((*(this->GetSubjectPublicKey())) == (*publicKey));
}

bool CertificateX509::ValidateCertificateTypeInCertChain(const CertificateX509* certChain, size_t count)
{
    if ((count == 0) || !certChain) {
        return false;
    }
    /* leaf cert must have a type */
    if (certChain[0].GetType() == CertificateX509::UNRESTRICTED_CERTIFICATE) {
        return false;
    }
    if (count == 1) {
        return true;
    }

    /**
     * Any signing cert in the chain must have the same type as the end entity certificate
     * or be unrestricted in order to sign the next cert in the chain.
     */
    for (size_t cnt = 1; cnt < count; cnt++) {
        if ((certChain[cnt].GetType() != CertificateX509::UNRESTRICTED_CERTIFICATE) &&
            (certChain[cnt].GetType() != certChain[0].GetType())) {
            return false;
        }
    }
    return true;
}

}
