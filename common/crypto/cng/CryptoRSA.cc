/**
 * @file CryptoRSA.cc
 *
 * Class for RSA public/private key encryption - this wraps the Windows CNG APIs
 */

/******************************************************************************
 * Copyright (c) 2011, 2014, AllSeen Alliance. All rights reserved.
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

#include <assert.h>
#include <windows.h>
#include <bcrypt.h>
#include <capi.h>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/Util.h>
#include <qcc/StringUtil.h>

#include <Status.h>

#include <qcc/CngCache.h>


using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

#ifndef STATUS_INVALID_SIGNATURE
#define STATUS_INVALID_SIGNATURE ((NTSTATUS)0xC000A000L)
#endif

// Some OIDs not defined in Windows header files
static const qcc::String OID_PBES2           = "1.2.840.113549.1.5.13";
static const qcc::String OID_PKDF2           = "1.2.840.113549.1.5.12";
static const qcc::String OID_PBE_MD5_DES_CBC = "1.2.840.113549.1.5.3";
static const qcc::String OID_HMAC_SHA1       = "1.2.840.113549.2.7";
static const qcc::String OID_AES_CBC         = "2.16.840.1.101.3.4.1.2";
static const qcc::String OID_DES_ED3_CBC     = "1.2.840.113549.3.7";
static const qcc::String OID_CN              = "2.5.4.3";
static const qcc::String OID_ORG             = "2.5.4.10";


bool Crypto_RSA::RSA_Init()
{
    if (!cngCache.rsaHandle) {
        if (BCryptOpenAlgorithmProvider(&cngCache.rsaHandle, BCRYPT_RSA_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0) < 0) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to open RSA algorithm provider"));
            return false;
        }
    }
    return true;
}

void Crypto_RSA::Generate(uint32_t modLen)
{
    if (RSA_Init()) {
        if (BCryptGenerateKeyPair(cngCache.rsaHandle, (BCRYPT_KEY_HANDLE*)&key, modLen, 0) < 0) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to generate RSA key pair"));
            return;
        }
        if (BCryptFinalizeKeyPair((BCRYPT_KEY_HANDLE)key, 0) < 0) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to finalize RSA key pair"));
            return;
        }
    }
}

static NCRYPT_KEY_HANDLE Bkey2Nkey(BCRYPT_KEY_HANDLE bkey)
{
    NCRYPT_KEY_HANDLE nkey = 0;
    DWORD len;
    BCRYPT_RSAKEY_BLOB* blob = NULL;
    // Dry run to get length
    NTSTATUS ntStatus = BCryptExportKey(bkey, NULL, BCRYPT_RSAPRIVATE_BLOB, NULL, 0, &len, 0);
    // Export BKey to the blob
    if (ntStatus >= 0) {
        blob = (BCRYPT_RSAKEY_BLOB*)malloc(len);
        if (blob) {
            ntStatus = BCryptExportKey(bkey, NULL, BCRYPT_RSAPRIVATE_BLOB, (PUCHAR)blob, len, &len, 0);
        } else {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed allocate memory for blob"));
            ntStatus = STATUS_NO_MEMORY;
        }
    }
    // Now import the blob as an NKEY
    if (ntStatus >= 0) {
        NCRYPT_PROV_HANDLE provHandle;
        ntStatus = NCryptOpenStorageProvider(&provHandle, MS_KEY_STORAGE_PROVIDER, 0);
        if (ntStatus >= 0) {
            ntStatus = NCryptImportKey(provHandle, NULL, BCRYPT_RSAPRIVATE_BLOB, NULL, &nkey, (PUCHAR)blob, len, 0);
            NCryptFreeObject(provHandle);
        } else {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to open storage provider NTSTATUS=%x", ntStatus));
        }
    }
    if (ntStatus < 0) {
        QCC_LogError(ER_CRYPTO_ERROR, ("Failed to get NKey from BKey NTSTATUS=%x", ntStatus));
    }
    if (blob) {
        memset(blob, 0, len);
        free(blob);
    }
    return nkey;
}

QStatus Crypto_RSA::MakeSelfCertificate(const qcc::String& commonName, const qcc::String& app)
{
    QStatus status = ER_OK;

    // Avoid handle leaks by clearing stale key and cert handles
    if (key) {
        BCryptDestroyKey((BCRYPT_KEY_HANDLE)key);
        key = NULL;
    }
    if (cert) {
        CertFreeCertificateContext((CERT_CONTEXT*)cert);
        cert = NULL;
    }
    // Generate a key pair
    Generate(512);
    if (!key) {
        return ER_CRYPTO_ERROR;
    }
    // Need an NCRYPT key handle to create the cert
    NCRYPT_KEY_HANDLE nkey = Bkey2Nkey((BCRYPT_KEY_HANDLE)key);
    if (nkey) {
        qcc::String names;
        // ASN.1 encode the name strings
        Crypto_ASN1::Encode(names, "({(op)}{(op)})", &OID_CN, &commonName, &OID_ORG, &app);
        CERT_NAME_BLOB namesBlob = { names.size(), (PBYTE)names.data() };
        // Initialize the provider info struct.
        CRYPT_KEY_PROV_INFO provInfo;
        memset(&provInfo, 0, sizeof(provInfo));
        provInfo.pwszContainerName = L"AllJoyn";
        provInfo.dwProvType = PROV_RSA_FULL;
        provInfo.dwFlags = NCRYPT_SILENT_FLAG;
        provInfo.dwKeySpec = AT_SIGNATURE;
        // Now create the cert
        CERT_CONTEXT* ctx = (CERT_CONTEXT*)CertCreateSelfSignCertificate(nkey, &namesBlob, 0, &provInfo, NULL, NULL, NULL, NULL);
        if (ctx) {
            cert = (void*)ctx;
        } else {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to creat self signed certificate NTSTATUS=%x", GetLastError()));
        }
        NCryptFreeObject(nkey);
    }
    return status;
}

QStatus Crypto_RSA::ImportPEM(const qcc::String& pem)
{
    if (!RSA_Init()) {
        return ER_CRYPTO_ERROR;
    }
    // Avoid handle leaks by clearing stale key and cert handles
    if (key) {
        BCryptDestroyKey((BCRYPT_KEY_HANDLE)key);
        key = NULL;
    }
    if (cert) {
        CertFreeCertificateContext((CERT_CONTEXT*)cert);
        cert = NULL;
    }
    // Convert the PEM encoded X509 cert to DER binary
    size_t beginCert = pem.find("-----BEGIN CERTIFICATE-----");
    size_t endCert = pem.find("-----END CERTIFICATE-----");
    if ((beginCert != 0) || (endCert == String::npos)) {
        return ER_CRYPTO_ERROR;
    }
    qcc::String der;
    QStatus status = Crypto_ASN1::DecodeBase64(pem.substr(27, endCert - 27), der);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed convert BASE64 string"));
        return status;
    }
    // Extract the public key info from the certificate
    CERT_CONTEXT* ctx = (CERT_CONTEXT*)CertCreateContext(CERT_STORE_CERTIFICATE_CONTEXT, X509_ASN_ENCODING, (PBYTE)der.data(), der.size(), 0, NULL);
    if (ctx) {
        cert = (void*)ctx;
        CERT_INFO* info = ctx->pCertInfo;
        CERT_PUBLIC_KEY_INFO* keyInfo = &info->SubjectPublicKeyInfo;
        if (!CryptImportPublicKeyInfoEx2(X509_ASN_ENCODING, keyInfo, 0, NULL, (BCRYPT_KEY_HANDLE*)&key)) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to acquire key NTSTATUS=%x", GetLastError()));
        }
    } else {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to import cert NTSTATUS=%x", GetLastError()));
    }
    return status;
}

// Looks at the first line of str.  If it begins with tag then the remainder is copied to rest, and
// the line is removed from str.  Returns true if the tag is matched and the line is removed.
static bool GetLine(const char* tag, qcc::String& str, qcc::String& rest)
{
    size_t start = str.find(tag);
    if (start == 0) {
        size_t end = str.find_first_of("\r\n");
        size_t l = strlen(tag);
        rest = str.substr(l, end - l);
        str.erase(0, end + 1);
        if (!str.empty() && str[0] == '\n') {
            str.erase(0, 1);
        }
        return true;
    }
    return false;
}

// Wrapper class for various key derivation functions
class PBKD {
  public:
    PBKD() : keyObj(NULL), algHandle(0) { }

    BCRYPT_KEY_HANDLE DerivePBKDF2(const qcc::String& prfAlg, const qcc::String& cipher, const qcc::String& passphrase, qcc::String& salt, uint32_t iter)
    {
        if (!Init(cipher)) {
            return 0;
        }

        BCRYPT_ALG_HANDLE prf = NULL;
        BCRYPT_KEY_HANDLE key = NULL;

        if (prfAlg == OID_HMAC_SHA1) {
            if (BCryptOpenAlgorithmProvider(&prf, BCRYPT_SHA1_ALGORITHM, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0) {
                QCC_LogError(ER_CRYPTO_ERROR, ("Failed to open algorithm provider"));
                return 0;
            }
        } else {
            QCC_LogError(ER_CRYPTO_ERROR, ("PRF %s not supported", prfAlg.c_str()));
            return 0;
        }
        if (prf) {
            ULONG kbhLen = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + keyLen;
            BCRYPT_KEY_DATA_BLOB_HEADER* kbh = (BCRYPT_KEY_DATA_BLOB_HEADER*)malloc(kbhLen);
            if (!kbh) {
                QCC_LogError(ER_CRYPTO_ERROR, ("Failed to allocate memory for key blob header"));
                BCryptCloseAlgorithmProvider(prf, 0);
                return 0;
            }
            if (BCryptDeriveKeyPBKDF2(prf, (PUCHAR)passphrase.data(), passphrase.size(), (PUCHAR)salt.data(), salt.size(), iter, (uint8_t*)(kbh + 1), keyLen, 0) >= 0) {
                key = GenKey(kbh, kbhLen);
            }
            BCryptCloseAlgorithmProvider(prf, 0);
            free(kbh);
        }
        return key;
    }

    BCRYPT_KEY_HANDLE DerivePBKDF1(const qcc::String cipher, const qcc::String& passphrase, qcc::String& ivec, uint32_t iter)
    {
        if (!Init(cipher)) {
            return 0;
        }
        if (ivec.size() != 8) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Initialization vector has wrong length expected 8"));
            return 0;
        }
        if (keyLen > Crypto_MD5::DIGEST_SIZE) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Cannot generate key of length %d", keyLen));
            return 0;
        }
        // Allocate and initialize a key blob big enough to compose the digest in-place
        ULONG kbhLen = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + Crypto_MD5::DIGEST_SIZE;
        BCRYPT_KEY_DATA_BLOB_HEADER* kbh = (BCRYPT_KEY_DATA_BLOB_HEADER*)malloc(kbhLen);

        Crypto_MD5 md;
        uint8_t* digest = (uint8_t*)(kbh + 1);
        // Derive the key
        md.Init();
        md.Update(passphrase);
        md.Update(ivec);
        md.GetDigest(digest);
        for (uint32_t i = 1; i < iter; ++i) {
            md.Init();
            md.Update(digest, Crypto_MD5::DIGEST_SIZE);
            md.GetDigest(digest);
        }
        // IV is the 2nd half of the digest
        ivec.clear();
        ivec.append((char*)(digest + 8), 8);

        BCRYPT_KEY_HANDLE key = GenKey(kbh, kbhLen);
        free(kbh);
        return key;
    }

    BCRYPT_KEY_HANDLE DeriveLegacy(const qcc::String cipher, const qcc::String& passphrase, const qcc::String& ivec)
    {
        if (!Init(cipher)) {
            return 0;
        }
        if (ivec.size() != blockLen) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Initialization vector has wrong length expected %d", blockLen));
            return 0;
        }
        // Allocate and initialize a key blob big enough to compose 2 digests in-place
        ULONG kbhLen = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 2 * Crypto_MD5::DIGEST_SIZE;
        BCRYPT_KEY_DATA_BLOB_HEADER* kbh = (BCRYPT_KEY_DATA_BLOB_HEADER*)malloc(kbhLen);

        Crypto_MD5 md;
        uint8_t* digest = (uint8_t*)(kbh + 1);
        md.Init();
        md.Update(passphrase);
        md.Update((uint8_t*)ivec.data(), 8);
        md.GetDigest(digest);
        if (keyLen > Crypto_MD5::DIGEST_SIZE) {
            md.Init();
            md.Update(digest,  Crypto_MD5::DIGEST_SIZE);
            md.Update(passphrase);
            md.Update((uint8_t*)ivec.data(), 8);
            md.GetDigest(digest + Crypto_MD5::DIGEST_SIZE);
        }
        BCRYPT_KEY_HANDLE key = GenKey(kbh, kbhLen);
        free(kbh);
        return key;
    }

    ~PBKD() {
        if (algHandle) {
            BCryptCloseAlgorithmProvider(algHandle, 0);
        }
        delete [] keyObj;
    }

  private:

    bool Init(const qcc::String& cipher)
    {
        LPCWSTR algId;

        if ((cipher == OID_AES_CBC) || (cipher == "AES-128-CBC")) {
            algId = BCRYPT_AES_ALGORITHM;
            keyLen = 16;
            blockLen = 16;
        } else if ((cipher == OID_DES_ED3_CBC) || (cipher ==  "DES-EDE3-CBC")) {
            algId = BCRYPT_3DES_ALGORITHM;
            keyLen = 24;
            blockLen = 8;
        } else if (cipher == "DES-CBC") {
            algId = BCRYPT_DES_ALGORITHM;
            keyLen = 8;
            blockLen = 8;
        } else {
            QCC_LogError(ER_CRYPTO_ERROR, ("Cipher %s not supported", cipher.c_str()));
            return false;
        }
        if (BCryptOpenAlgorithmProvider(&algHandle, algId, MS_PRIMITIVE_PROVIDER, 0) < 0) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to open algorithm provider"));
            return 0;
        }
        // Enable CBC mode
        if (BCryptSetProperty(algHandle, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, wcslen(BCRYPT_CHAIN_MODE_CBC) + 1, 0) < 0) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to enable CBC mode on encryption algorithm provider"));
            return false;
        }
        return true;
    }

    BCRYPT_KEY_HANDLE GenKey(BCRYPT_KEY_DATA_BLOB_HEADER* kbh, DWORD kbhLen)
    {
        kbh->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
        kbh->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
        kbh->cbKeyData = keyLen;
        // Import the key
        DWORD keyObjLen;
        ULONG got;
        NTSTATUS ntStatus = BCryptGetProperty(algHandle, BCRYPT_OBJECT_LENGTH, (PBYTE)&keyObjLen, sizeof(DWORD), &got, 0);
        if (ntStatus < 0) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to get object length property NTSTATUS=%x", ntStatus));
            memset(kbh, 0, kbhLen);
            return NULL;
        }
        keyObj = new uint8_t[keyObjLen];
        BCRYPT_KEY_HANDLE key = 0;
        ntStatus = BCryptImportKey(algHandle, NULL, BCRYPT_KEY_DATA_BLOB, &key, keyObj, keyObjLen, (PUCHAR)kbh, kbhLen, 0);
        if (ntStatus < 0) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to import key NTSTATUS=%x", ntStatus));
        }
        memset(kbh, 0, kbhLen);
        return key;
    }

    DWORD blockLen;
    DWORD keyLen;
    uint8_t* keyObj;
    BCRYPT_ALG_HANDLE algHandle;

    /**
     * Copy constructor
     *
     * @param src PBKD to be copied.
     */
    PBKD(const PBKD& src) {
        /* private copy constructor to prevent copying */
    }
    /**
     * Assignment operator
     *
     * @param src source PBKD
     *
     * @return copy of PBKD
     */
    PBKD& operator=(const PBKD& src) {
        return *this;
    }
};

static QStatus DecryptPriv(BCRYPT_KEY_HANDLE kdKey, qcc::String& ivec, const uint8_t* blob, size_t blobLen, BCRYPT_KEY_HANDLE& privKey, bool legacy)
{
    QStatus status = ER_AUTH_FAIL;
    NTSTATUS ntStatus;
    BCRYPT_RSAKEY_BLOB* pkBlob = NULL;
    qcc::String pk;
    uint32_t version;

    qcc::String n;  // modulus
    qcc::String e;  // exponent
    qcc::String p;  // prime1
    qcc::String q;  // prime2

    // Decrypt the blob
    DWORD len = blobLen + 8;
    uint8_t* buf = new uint8_t[len];
    ntStatus = BCryptDecrypt(kdKey, (PUCHAR)blob, blobLen, NULL, (PUCHAR)ivec.data(), ivec.size(), (PUCHAR)buf, len, &len, BCRYPT_BLOCK_PADDING);
    if (ntStatus < 0) {
        QCC_LogError(status, ("Failed to decrypt private key NTSTATUS=%x", ntStatus));
        goto ExitDecryptPriv;
    }
    // Check if the key is legacy or pkcs#8 encapsulated
    if (legacy) {
        // See RFC 3447 for documentation on this formatting
        status = Crypto_ASN1::Decode(buf, len, "(ill?ll*)", &version, &n, &e, NULL, &p, &q);
    } else {
        qcc::String oid;
        status = Crypto_ASN1::Decode(buf, len, "(i(on)x)", &version, &oid, &pk);
        if (status == ER_OK) {
            if (oid != szOID_RSA_RSA) {
                QCC_LogError(status, ("Key was not an RSA private key"));
                goto ExitDecryptPriv;
            }
            status = Crypto_ASN1::Decode(pk, "(ill?ll*)", &version, &n, &e, NULL, &p, &q);
        }
    }
    // Up to this point all failures are considered to be authentication failures
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to decode private key"));
        status = ER_AUTH_FAIL;
        goto ExitDecryptPriv;
    }
    pkBlob = (BCRYPT_RSAKEY_BLOB*)malloc(sizeof(BCRYPT_RSAKEY_BLOB) + len);
    if (!pkBlob) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("Failed to allocate memory for pkBlob"));
        goto ExitDecryptPriv;
    }
    pkBlob->Magic = BCRYPT_RSAPRIVATE_MAGIC;
    pkBlob->BitLength = n.size() * 8;
    pkBlob->cbPublicExp = e.size();
    pkBlob->cbModulus = n.size();
    pkBlob->cbPrime1 = p.size();
    pkBlob->cbPrime2 = q.size();
    // Get components into contiguous memory as required by BCryptImportKeyPair(). Note that the
    // exponent and modulus are swapped versus the PKS#1 order.
    pk.reserve(len);
    pk = e + n + p + q;
    // Clear out secret stuff we no longer need
    p.secure_clear();
    q.secure_clear();
    // Append contiguous key matter to the end of the blob
    memcpy((uint8_t*)(pkBlob + 1), pk.data(), pk.size());
    len = sizeof(BCRYPT_RSAKEY_BLOB) + pk.size();
    pk.secure_clear();
    // Now import the key
    ntStatus = BCryptImportKeyPair(cngCache.rsaHandle, NULL, BCRYPT_PRIVATE_KEY_BLOB, &privKey, (PUCHAR)pkBlob, len, 0);
    memset(pkBlob, 0, len);
    if (ntStatus < 0) {
        QCC_LogError(status, ("Failed to import RSA blob NTSTATUS=%x", ntStatus));
        goto ExitDecryptPriv;
    }
    // Success
    status = ER_OK;

ExitDecryptPriv:

    // Clear out any plaintext key info
    memset(buf, 0, blobLen);
    delete [] buf;
    if (pkBlob) {
        free(pkBlob);;
    }
    return status;
}

QStatus Crypto_RSA::ImportPKCS8(const qcc::String& pkcs8, const qcc::String& passphrase)
{
    QStatus status = ER_CRYPTO_ERROR;
    qcc::String ivec;
    qcc::String str = pkcs8;
    qcc::String pk;
    PBKD pbkd;
    bool legacy = false;
    BCRYPT_KEY_HANDLE kdKey = NULL; // key decryption key

    if (!RSA_Init()) {
        return status;
    }
    // Avoid handle leaks by clearing stale key and cert handles
    if (key) {
        BCryptDestroyKey((BCRYPT_KEY_HANDLE)key);
        key = NULL;
    }
    if (cert) {
        CertFreeCertificateContext((CERT_CONTEXT*)cert);
        cert = NULL;
    }
    // Check for SSLeay legacy style encoding
    qcc::String unused;
    if (GetLine("-----BEGIN RSA PRIVATE KEY-----", str, unused)) {
        qcc::String type;
        qcc::String alg;
        qcc::String seed;
        if (GetLine("Proc-Type:", str, type) && (type.find("ENCRYPTED") != String::npos)) {
            if (!GetLine("DEK-Info: ", str, alg) || alg.empty()) {
                return status;
            }
            size_t pos = alg.find(",");
            if (pos != String::npos) {
                seed = alg.substr(pos + 1);
                alg.erase(pos);
            }
        }
        size_t endOfKey = str.find("-----END RSA PRIVATE KEY-----");
        if (endOfKey == String::npos) {
            return status;
        }
        status = Crypto_ASN1::DecodeBase64(str.erase(endOfKey - 1), pk);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed convert BASE64 string"));
            return status;
        }
        ivec = HexStringToByteString(seed);
        kdKey = pbkd.DeriveLegacy(alg, passphrase, ivec);
        legacy = true;
    } else if (GetLine("-----BEGIN ENCRYPTED PRIVATE KEY-----", str, unused)) {
        size_t endOfKey = str.find("-----END ENCRYPTED PRIVATE KEY-----");
        if (endOfKey == String::npos) {
            return status;
        }
        qcc::String der;
        status = Crypto_ASN1::DecodeBase64(str.erase(endOfKey - 1), der);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed convert BASE64 string"));
            return status;
        }
        // Find out what we are decoding
        qcc::String oid;
        qcc::String args;
        status = Crypto_ASN1::Decode(der, "((o?)x)", &oid, &args, &pk);
        if (status != ER_OK) {
            return status;
        }
        uint32_t iter;
        if (oid == OID_PBES2) {
            qcc::String prfOid;
            qcc::String algOid;
            qcc::String salt;
            status = Crypto_ASN1::Decode(args, "((o(xi/o))(ox))", &oid, &salt, &iter, &prfOid, &algOid, &ivec);
            if (status == ER_OK) {
                if (prfOid.empty()) {
                    prfOid = OID_HMAC_SHA1; // The default for PBKDFK2
                }
                kdKey = pbkd.DerivePBKDF2(prfOid, algOid, passphrase, salt, iter);
            }
        } else if (oid == OID_PBE_MD5_DES_CBC) {
            status = Crypto_ASN1::Decode(args, "(xi)", &ivec, &iter);
            if (status == ER_OK) {
                kdKey = pbkd.DerivePBKDF1("DES-CBC", passphrase, ivec, iter);
            }
        }
    } else {
        QCC_LogError(status, ("Unsupported PEM encoding\n%s", pkcs8.c_str()));
    }
    if (kdKey) {
        status = DecryptPriv(kdKey, ivec, (const uint8_t*)pk.data(), pk.size(), (BCRYPT_KEY_HANDLE)key, legacy);
        BCryptDestroyKey(kdKey);
    } else {
        status = ER_CRYPTO_ERROR;
    }
    return status;
}

QStatus Crypto_RSA::ImportPKCS8(const qcc::String& pkcs8, PassphraseListener* listener)
{
    QStatus status;
    if (listener) {
        qcc::String passphrase;
        if (listener->GetPassphrase(passphrase, false)) {
            status = ImportPKCS8(pkcs8, passphrase);
        } else {
            status = ER_AUTH_USER_REJECT;
        }
        passphrase.secure_clear();
    } else {
        status = ER_BAD_ARG_2;
    }
    return status;
}

QStatus Crypto_RSA::ExportPrivateKey(qcc::KeyBlob& keyBlob, const qcc::String& passphrase)
{
    static const size_t SALT_LEN = 8;
    static const size_t IVEC_LEN = 16;
    static const uint32_t iter = 2048;
    QStatus status = ER_OK;

    uint8_t rand[SALT_LEN + IVEC_LEN];
    uint8_t* iv = rand + SALT_LEN;
    Crypto_GetRandomBytes(rand, sizeof(rand));

    qcc::String ivec((char*)iv, IVEC_LEN);
    qcc::String salt((char*)rand, SALT_LEN);
    PBKD pbkd;
    BCRYPT_KEY_HANDLE kdKey = pbkd.DerivePBKDF2(OID_HMAC_SHA1, OID_AES_CBC, passphrase, salt, iter);

    BCRYPT_RSAKEY_BLOB* pkBlob = NULL;
    DWORD len = 0;

    // Dry run to get length
    NTSTATUS ntStatus = BCryptExportKey((BCRYPT_KEY_HANDLE)key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, NULL, 0, &len, 0);
    if (ntStatus >= 0) {
        pkBlob = (BCRYPT_RSAKEY_BLOB*)malloc(len);
        ntStatus = BCryptExportKey((BCRYPT_KEY_HANDLE)key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, (PUCHAR)pkBlob, len, &len, 0);
    }
    if (ntStatus >= 0) {
        char* ptr = (char*)(pkBlob + 1);
        qcc::String e(ptr, pkBlob->cbPublicExp); // exponent
        ptr += pkBlob->cbPublicExp;
        qcc::String n(ptr, pkBlob->cbModulus);   // modulus
        ptr += pkBlob->cbModulus;
        qcc::String p(ptr, pkBlob->cbPrime1);    // prime1
        ptr += pkBlob->cbPrime1;
        qcc::String q(ptr, pkBlob->cbPrime2);    // prime2
        ptr += pkBlob->cbPrime2;
        qcc::String e1(ptr, pkBlob->cbPrime1);   // exponent1
        ptr += pkBlob->cbPrime1;
        qcc::String e2(ptr, pkBlob->cbPrime2);   // exponent2
        ptr += pkBlob->cbPrime2;
        qcc::String c(ptr, pkBlob->cbPrime1);    // coefficient
        ptr += pkBlob->cbPrime1;
        qcc::String d(ptr, pkBlob->cbModulus);   // private exponent

        // We are done with the exported blob - clear it before freeing it.
        memset(pkBlob, 0, len);
        free(pkBlob);

        // Encode the private key components in PKCS#8 order
        qcc::String pk;
        status = Crypto_ASN1::Encode(pk, "(illllllll)", 0, &n, &e, &d, &p, &q, &e1, &e2, &c);

        // Clear out secret stuff we no longer need
        p.secure_clear();
        q.secure_clear();
        e1.secure_clear();
        e2.secure_clear();
        c.secure_clear();
        d.secure_clear();

        // Encode public key algorithm information
        qcc::String oid = szOID_RSA_RSA;
        qcc::String pkInfo;
        status = Crypto_ASN1::Encode(pkInfo, "(i(on)x)", 0, &oid, &pk);
        pk.secure_clear();

        // Encrypt the private key information
        DWORD outLen = pkInfo.size() + 16; // allow for expansion due to block padding
        uint8_t* buf = new uint8_t[outLen];
        // Note that iv is modified by this call which is why we copied it into a string ealier
        ntStatus = BCryptEncrypt(kdKey, (PBYTE)pkInfo.data(), pkInfo.size(), NULL, iv, IVEC_LEN, buf, outLen, &outLen, BCRYPT_BLOCK_PADDING);
        pkInfo.secure_clear();
        if (ntStatus < 0) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to encrypt private key NTSTATUS=%x", ntStatus));
        } else {
            pkInfo.assign((char*)buf, outLen);
            // ANS.1 encode the entire PKCS8 structure
            qcc::String der;
            status = Crypto_ASN1::Encode(der, "((o((o(xi))(ox)))x)", &OID_PBES2, &OID_PKDF2, &salt, iter, &OID_AES_CBC, &ivec, &pkInfo);
            // Convert to base 64 and wrap with PEM header and trailer
            qcc::String pem;
            status = Crypto_ASN1::EncodeBase64(der, pem);
            pem.insert(0, "-----BEGIN ENCRYPTED PRIVATE KEY-----\n");
            pem.append("-----END ENCRYPTED PRIVATE KEY-----\n");
            // All done
            keyBlob.Set((uint8_t*)pem.data(), pem.size(), KeyBlob::PRIVATE);
        }
        delete [] buf;
    } else {
        status = ER_CRYPTO_ERROR;
    }
    return status;
}

QStatus Crypto_RSA::ExportPrivateKey(qcc::KeyBlob& keyBlob, PassphraseListener* listener)
{
    QStatus status;
    if (listener) {
        qcc::String passphrase;
        if (listener->GetPassphrase(passphrase, true)) {
            status = ExportPrivateKey(keyBlob, passphrase);
        } else {
            status = ER_AUTH_USER_REJECT;
        }
        passphrase.secure_clear();
    } else {
        status = ER_BAD_ARG_2;
    }
    return status;
}

QStatus Crypto_RSA::ImportPrivateKey(const qcc::KeyBlob& keyBlob, const qcc::String& passphrase)
{
    if (keyBlob.GetType() != KeyBlob::PRIVATE) {
        return ER_BAD_ARG_1;
    }
    qcc::String pkcs8((char*)keyBlob.GetData(), keyBlob.GetSize());
    return ImportPKCS8(pkcs8, passphrase);
}

QStatus Crypto_RSA::ImportPrivateKey(const qcc::KeyBlob& keyBlob, PassphraseListener* listener)
{
    QStatus status;
    if (listener) {
        qcc::String passphrase;
        if (listener->GetPassphrase(passphrase, false)) {
            status = ImportPrivateKey(keyBlob, passphrase);
        } else {
            status = ER_AUTH_USER_REJECT;
        }
        passphrase.secure_clear();
    } else {
        status = ER_BAD_ARG_2;
    }
    return status;
}

qcc::String Crypto_RSA::CertToString()
{
    if (cert) {
        CERT_CONTEXT* ctx = (CERT_CONTEXT*)cert;
        return Crypto_ASN1::ToString(ctx->pbCertEncoded, ctx->cbCertEncoded);
    } else {
        return "";
    }
}

QStatus Crypto_RSA::ExportPEM(qcc::String& pem)
{
    QStatus status = ER_CRYPTO_ERROR;
    if (cert) {
        CERT_CONTEXT* ctx = (CERT_CONTEXT*)cert;
        qcc::String der((char*)ctx->pbCertEncoded, ctx->cbCertEncoded);
        pem = "-----BEGIN CERTIFICATE-----\n";
        status = Crypto_ASN1::EncodeBase64(der, pem);
        pem.append("-----END CERTIFICATE-----\n");
    } else {
        QCC_LogError(status, ("No cert to export"));
    }
    return status;
}

size_t Crypto_RSA::GetSize()
{
    if (!size && key) {
        DWORD got;
        DWORD len;
        NTSTATUS ntStatus = BCryptGetProperty((BCRYPT_KEY_HANDLE)key, BCRYPT_KEY_STRENGTH, (PBYTE)&len, sizeof(len), &got, 0);
        assert(ntStatus >= 0);
        if (ntStatus < 0) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to get key strength property"));
            len = -1;
        }
        size = len;
    }
    return size;
}

static const BCRYPT_PKCS1_PADDING_INFO PadInfo = { NULL };

QStatus Crypto_RSA::SignDigest(const uint8_t* digest, size_t digLen, uint8_t* signature, size_t& sigLen)
{
    QStatus status = ER_OK;

    if (!digest) {
        return ER_BAD_ARG_1;
    }
    if (digLen > MaxDigestSize()) {
        return ER_CRYPTO_TRUNCATED;
    }
    if (!signature) {
        return ER_BAD_ARG_3;
    }
    if (!key) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }

    DWORD len;
    // Dry run to check the signature length
    NTSTATUS ntStatus = BCryptSignHash((BCRYPT_KEY_HANDLE)key, (void*)&PadInfo, (PUCHAR)digest, digLen, NULL, 0, &len, BCRYPT_PAD_PKCS1);
    if (ntStatus < 0) {
        QCC_LogError(status, ("Failed to get signature length %x", ntStatus));
        return ER_CRYPTO_ERROR;
    }
    if (sigLen < len) {
        return ER_BUFFER_TOO_SMALL;
    }
    // Do the signing
    ntStatus = BCryptSignHash((BCRYPT_KEY_HANDLE)key, (void*)&PadInfo, (PUCHAR)digest, digLen, (PUCHAR)signature, sigLen, &len, BCRYPT_PAD_PKCS1);
    if (ntStatus < 0) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to sign hash %x", ntStatus));
        sigLen = 0;
    } else {
        sigLen = len;
    }
    return status;
}

QStatus Crypto_RSA::VerifyDigest(const uint8_t* digest, size_t digLen, const uint8_t* signature, size_t sigLen)
{
    QStatus status = ER_OK;

    if (!digest) {
        return ER_BAD_ARG_1;
    }
    if (digLen > MaxDigestSize()) {
        return ER_CRYPTO_TRUNCATED;
    }
    if (!signature) {
        return ER_BAD_ARG_3;
    }
    if (!key) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    NTSTATUS ntStatus = BCryptVerifySignature((BCRYPT_KEY_HANDLE)key, (void*)&PadInfo, (PUCHAR)digest, digLen, (PUCHAR)signature, sigLen, BCRYPT_PAD_PKCS1);
    if (ntStatus < 0) {
        if (ntStatus == STATUS_INVALID_SIGNATURE) {
            status = ER_AUTH_FAIL;
        } else {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to verify signature %x", ntStatus));
        }
    }
    return status;
}

QStatus Crypto_RSA::PublicEncrypt(const uint8_t* inData, size_t inLen, uint8_t* outData, size_t& outLen)
{
    if (!key) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (inLen > MaxDigestSize()) {
        return ER_CRYPTO_TRUNCATED;
    }
    if (outLen < GetSize()) {
        return ER_BUFFER_TOO_SMALL;
    }
    ULONG clen;
    if (BCryptEncrypt((BCRYPT_KEY_HANDLE)key, (PUCHAR)inData, inLen, NULL, NULL, 0, (PUCHAR)outData, outLen, &clen, BCRYPT_PAD_PKCS1) < 0) {
        return ER_CRYPTO_ERROR;
    } else {
        outLen = clen;
        return ER_OK;
    }
}

QStatus Crypto_RSA::PrivateDecrypt(const uint8_t* inData, size_t inLen, uint8_t* outData, size_t& outLen)
{
    if (!key) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (inLen > GetSize()) {
        return ER_CRYPTO_TRUNCATED;
    }
    if (outLen < MaxDigestSize()) {
        return ER_BUFFER_TOO_SMALL;
    }
    ULONG clen;
    if (BCryptDecrypt((BCRYPT_KEY_HANDLE)key, (PUCHAR)inData, inLen, NULL, NULL, 0, (PUCHAR)outData, outLen, &clen, BCRYPT_PAD_PKCS1) < 0) {
        return ER_CRYPTO_ERROR;
    } else {
        outLen = clen;
        return ER_OK;
    }
}

QStatus Crypto_RSA::Sign(const uint8_t* data, size_t len, uint8_t* signature, size_t& sigLen)
{
    Crypto_SHA1 sha1;
    sha1.Init();
    sha1.Update(data, len);
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
    sha1.GetDigest(digest);
    return SignDigest(digest, Crypto_SHA1::DIGEST_SIZE, signature, sigLen);
}

QStatus Crypto_RSA::Verify(const uint8_t* data, size_t len, const uint8_t* signature, size_t sigLen)
{
    Crypto_SHA1 sha1;
    sha1.Init();
    sha1.Update(data, len);
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
    sha1.GetDigest(digest);
    return VerifyDigest(digest, Crypto_SHA1::DIGEST_SIZE, signature, sigLen);
}

Crypto_RSA::Crypto_RSA() : size(0), cert(NULL), key(NULL), certContext(NULL)
{
}

Crypto_RSA::~Crypto_RSA()
{
    if (key) {
        BCryptDestroyKey((BCRYPT_KEY_HANDLE)key);
    }
    if (cert) {
        CertFreeCertificateContext((CERT_CONTEXT*)cert);
    }
}
