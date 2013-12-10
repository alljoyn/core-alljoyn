/**
 * @file CryptoRSA.cc
 *
 * Class for RSA public/private key encryption - this wraps the Windows CNG APIs
 */

/******************************************************************************
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/Util.h>
#include <qcc/StringUtil.h>
#include <Status.h>
#include <qcc/BigNum.h>

//#include <bcrypt.h>  // for blob definitions
typedef struct _BCRYPT_KEY_DATA_BLOB_HEADER {
    ULONG dwMagic;
    ULONG dwVersion;
    ULONG cbKeyData;
} BCRYPT_KEY_DATA_BLOB_HEADER, *PBCRYPT_KEY_DATA_BLOB_HEADER;

typedef struct _BCRYPT_RSAKEY_BLOB {
    ULONG Magic;
    ULONG BitLength;
    ULONG cbPublicExp;
    ULONG cbModulus;
    ULONG cbPrime1;
    ULONG cbPrime2;
} BCRYPT_RSAKEY_BLOB;

#define BCRYPT_RSAPUBLIC_MAGIC      0x31415352  // RSA1
#define BCRYPT_RSAPRIVATE_MAGIC     0x32415352  // RSA2

using namespace std;
using namespace qcc;

using namespace Windows::Storage::Streams;
using namespace Windows::Security::Cryptography;
using namespace Windows::Security::Cryptography::Core;

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

// OIDs defined in the Windows header files that are not defined for Metro.
static const qcc::String OID_PKCS1           = "1.2.840.113549.1.1";
static const qcc::String OID_RSA_RSA         = "1.2.840.113549.1.1.1";
static const qcc::String OID_RSA_SHA1RSA     = "1.2.840.113549.1.1.5";

class Crypto_RSA::CertContext {
  public:

    CertContext(CryptographicKey ^ keyPair) : keyPair(keyPair) {
        asymAlgProv = AsymmetricKeyAlgorithmProvider::OpenAlgorithm(AsymmetricAlgorithmNames::RsaPkcs1);
        asymAlgProvSigning = AsymmetricKeyAlgorithmProvider::OpenAlgorithm(AsymmetricAlgorithmNames::RsaSignPkcs1Sha1);
    }

    CryptographicKey ^ keyPair;
    AsymmetricKeyAlgorithmProvider ^ asymAlgProv;
    AsymmetricKeyAlgorithmProvider ^ asymAlgProvSigning;
    CryptographicKey ^ keyPairSigning;

    KeyDerivationAlgorithmProvider ^ keyDerivationProvider;
    qcc::String derCertificate; /// TODO: make sure to clear and delete this.
};

bool Crypto_RSA::RSA_Init()
{
    if (certContext) {
        delete certContext;
        certContext = NULL;
    }

    certContext = new CertContext(NULL);

    certContext->keyDerivationProvider = KeyDerivationAlgorithmProvider::OpenAlgorithm(KeyDerivationAlgorithmNames::Pbkdf2Sha1);

    // Open the algorithm provider for the specified asymmetric algorithm.
    certContext->asymAlgProv = AsymmetricKeyAlgorithmProvider::OpenAlgorithm(AsymmetricAlgorithmNames::RsaPkcs1);

    // This key should be related to the other key pair
    certContext->asymAlgProvSigning = AsymmetricKeyAlgorithmProvider::OpenAlgorithm(AsymmetricAlgorithmNames::RsaSignPkcs1Sha1);

    return true;
}



void Crypto_RSA::Generate(uint32_t modLen)
{
    if (!RSA_Init()) {
        QCC_LogError(ER_CRYPTO_ERROR, ("Failed initialize the context object"));
    }


    // Create an asymmetric key pair.
    certContext->keyPair = certContext->asymAlgProv->CreateKeyPair(modLen);

    IBuffer ^ keyPairBuffer = certContext->keyPair->Export();
    certContext->keyPairSigning = certContext->asymAlgProvSigning->ImportKeyPair(keyPairBuffer);
}

static inline bool UnpackLen(uint8_t*& p, uint8_t* end, size_t& l)
{
    l = *p++;
    if (l & 0x80) {
        size_t n = l & 0x7F;
        l = 0;
        while (n--) {
            l = (l << 8) + *p++;
        }
    }
    return (p + l) <= end;
}

static qcc::String UnpackOID(uint8_t* p, size_t len)
{
    qcc::String oid;

    oid += U32ToString(*p / 40) + '.';
    oid += U32ToString(*p % 40);

    uint32_t v = 0;
    while (--len) {
        v = (v << 7) + (*(++p) % 128);
        if (!(*p & 0x80)) {
            oid += '.' + U32ToString(v);
            v = 0;
        }
    }
    return oid;
}

void FormatTime(qcc::String& timeNow, qcc::String& timeOneYearLater)
{
    SYSTEMTIME stNow;
    SYSTEMTIME stOneYear;

    FILETIME ft;
    GetSystemTime(&stNow);
    SystemTimeToFileTime(&stNow, &ft);

    // convert to a 64 bit number to add big numbers correctly
    ULARGE_INTEGER ui64Now;
    ui64Now.LowPart = ft.dwLowDateTime;
    ui64Now.HighPart = ft.dwHighDateTime;

    // add one year to the current time
    const uint64_t ui64OneYearInHundredNanoSeconds = ((uint64_t) 10000000 * 60 * 60 * 24  * 365);
    ui64Now.QuadPart += ui64OneYearInHundredNanoSeconds;

    // put the result back into the filetime structure
    ft.dwHighDateTime = ui64Now.HighPart;
    ft.dwLowDateTime = ui64Now.LowPart;

    FileTimeToSystemTime(&ft, &stOneYear); // convert next year back to system time

    char buf[13];
    snprintf(buf, sizeof(buf), "%02d%02d%02d%02d%02d%02dZ",
             stNow.wYear % 100,
             stNow.wMonth,
             stNow.wDay,
             stNow.wHour,
             stNow.wMinute,
             stNow.wSecond);

    timeNow.assign(buf, sizeof(buf));

    snprintf(buf, sizeof(buf), "%02d%02d%02d%02d%02d%02dZ",
             stOneYear.wYear % 100,
             stOneYear.wMonth,
             stOneYear.wDay,
             stOneYear.wHour,
             stOneYear.wMinute,
             stOneYear.wSecond);
    timeOneYearLater.assign(buf, sizeof(buf));
}


QStatus Crypto_RSA::MakeSelfCertificate(const qcc::String& commonName, const qcc::String& app)
{
    QStatus status = ER_CRYPTO_ERROR;

    if (!RSA_Init()) {
        return ER_CRYPTO_ERROR;
    }

    if (GetSize() > 0) {
        Generate(GetSize() * 8);
    } else {
        Generate(512);
    }

    if (!certContext->keyPair) {
        return ER_CRYPTO_ERROR;
    }

    // generate a unique serial number for each cert.
    uint8_t serial[8];
    IBuffer ^ serialBuffer = CryptographicBuffer::GenerateRandom(sizeof(serial));
    Platform::Array<uint8> ^ serialArray;
    CryptographicBuffer::CopyToByteArray(serialBuffer, &serialArray);
    qcc::String SerialNumber((char*)serialArray->Data, serialArray->Length);

    // get the time now and one year from now
    qcc::String timeNow;
    qcc::String timeOneYearLater;
    FormatTime(timeNow, timeOneYearLater);

    // format the public key for output
    IBuffer ^ X509PublicKey = certContext->keyPair->ExportPublicKey(CryptographicPublicKeyBlobType::X509SubjectPublicKeyInfo);
    Platform::Array<uint8>^ X509PublicKeyArray;
    CryptographicBuffer::CopyToByteArray(X509PublicKey, &X509PublicKeyArray);
    qcc::String PublicKey((char*)X509PublicKeyArray->Data, X509PublicKeyArray->Length); // this is already in ASN.1 format

    qcc::String Certificate;
    status = Crypto_ASN1::Encode(Certificate,
                                 "(l(on)({(ou)}{(ou)})(tt)({(ou)}{(ou)})R)",
                                 &SerialNumber,
                                 &OID_RSA_SHA1RSA,
                                 &OID_CN, &commonName, &OID_ORG, &app,
                                 &timeNow, &timeOneYearLater,
                                 &OID_CN, &commonName, &OID_ORG, &app,
                                 &PublicKey);

    if (status != ER_OK) {
        QCC_LogError(status, ("Failed encode the certificate string"));
        return status;
    }

    // Now sign the certificate data
    Platform::ArrayReference<uint8> CertificateArray((uint8*)Certificate.data(), Certificate.size());
    IBuffer ^ CertificateBuffer = CryptographicBuffer::CreateFromByteArray(CertificateArray);

    IBuffer ^ SignatureBuffer = CryptographicEngine::Sign(certContext->keyPairSigning, CertificateBuffer);
    Platform::Array<uint8>^ SignatureArray;
    CryptographicBuffer::CopyToByteArray(SignatureBuffer, &SignatureArray);
    qcc::String Signature((char*)SignatureArray->Data, SignatureArray->Length);

    // concat the certificate, algorithm, and signature.
    qcc::String SelfSignedCertificate;
    status = Crypto_ASN1::Encode(SelfSignedCertificate,
                                 "(R(on)b)",
                                 &Certificate,
                                 &OID_RSA_SHA1RSA,
                                 &Signature,
                                 SignatureArray->Length * 8);  //bytes to bits.
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed encode the certificate and its signature"));
        return status;
    }

    certContext->derCertificate = SelfSignedCertificate;

    return status;
}

QStatus Crypto_RSA::ImportPEM(const qcc::String& pem)
{
    if (!RSA_Init()) {
        return ER_CRYPTO_ERROR;
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

    // now we reverse the operations in ExportPrivateKey
    uint32_t VersionNumber;
    qcc::String SerialNumber;
    qcc::String SignatureAlgorithm;
    qcc::String Signature_OID_RSA_RSA;

    qcc::String PublicKey;
    size_t PublicKeySize = 0;

    qcc::String SignatureOID;
    qcc::String KeyBits;
    size_t KeyBitsSize;

    qcc::String TBSCertificateInner;
    qcc::String TBSCertificateOptional;

    // find the optional version number of the cert.
    status = Crypto_ASN1::Decode(der,
                                 "(([i].)(on)b)",
                                 &VersionNumber,
                                 &TBSCertificateInner,
                                 &SignatureOID,
                                 &KeyBits,
                                 &KeyBitsSize);
    // There might not be a version number, so try again without one.
    if (status != ER_OK) {
        status = Crypto_ASN1::Decode(der,
                                     "((.)(on)b)",
                                     &TBSCertificateInner,
                                     &SignatureOID,
                                     &KeyBits,
                                     &KeyBitsSize);
    }

    // parse the inner data to retrieve the public key.
    if (status == ER_OK) {
        status = Crypto_ASN1::Decode(TBSCertificateInner,
                                     "l(on)(*)(*)(*)((on)b).", // skip the issuer, validity, and subject
                                     &SerialNumber,
                                     &SignatureAlgorithm,
                                     &Signature_OID_RSA_RSA,
                                     &PublicKey,
                                     &PublicKeySize,
                                     &TBSCertificateOptional); // optional TBSCertificate [1, 2, 3], the issuerUniqueID, subjectUniqueID, and extensions
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("Failed decode DER into members"));
        return status;
    }
    // format the public key for output
    Platform::Array<uint8> ^ X509PublicKeyArray = ref new Platform::Array<uint8>((uint8*)PublicKey.data(), PublicKey.size());
    IBuffer ^ X509PublicKey = CryptographicBuffer::CreateFromByteArray(X509PublicKeyArray);

    certContext->keyPair = certContext->asymAlgProv->ImportPublicKey(X509PublicKey, CryptographicPublicKeyBlobType::Pkcs1RsaPublicKey);
    certContext->keyPairSigning = certContext->asymAlgProvSigning->ImportPublicKey(X509PublicKey, CryptographicPublicKeyBlobType::Pkcs1RsaPublicKey);

    // successfully imported, so update the stashed cert.
    certContext->derCertificate = der;

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
    CryptographicKey ^ DerivePBKDF2(const qcc::String & prfAlg, const qcc::String & cipher, const qcc::String & passphrase, qcc::String & salt, uint32_t iter)
    {
        if (!Init(cipher)) {
            return nullptr;
        }

        CryptographicKey ^ derivedSymKey;
        KeyDerivationAlgorithmProvider ^ keyDerivationProvider;

        if (prfAlg == OID_HMAC_SHA1) {
            keyDerivationProvider = KeyDerivationAlgorithmProvider::OpenAlgorithm(KeyDerivationAlgorithmNames::Pbkdf2Sha1);
        } else {
            QCC_LogError(ER_CRYPTO_ERROR, ("PRF %s not supported", prfAlg.c_str()));
            return nullptr;
        }
        if (keyDerivationProvider) {
            Platform::ArrayReference<uint8> inPassPhraseBuf((uint8*)passphrase.data(), passphrase.size());
            IBuffer ^ passphraseBuffer = CryptographicBuffer::CreateFromByteArray(inPassPhraseBuf);
            Platform::ArrayReference<uint8> inSaltBuf((uint8*)salt.data(), salt.size());
            IBuffer ^ saltBuffer = CryptographicBuffer::CreateFromByteArray(inSaltBuf);

            KeyDerivationParameters ^ pbkdf2Params = KeyDerivationParameters::BuildForPbkdf2(saltBuffer, iter);

            // create a key based on original key and derivation parmaters
            CryptographicKey ^ keyOriginal = keyDerivationProvider->CreateKey(passphraseBuffer);
            IBuffer ^ keyMaterial = CryptographicEngine::DeriveKeyMaterial(keyOriginal, pbkdf2Params, keyLen);


            //SymmetricKeyAlgorithmProvider ^ symKeyProv = SymmetricKeyAlgorithmProvider::OpenAlgorithm(algId);
            derivedSymKey = algProvider->CreateSymmetricKey(keyMaterial);

        }
        return derivedSymKey;
    }

    CryptographicKey ^ DerivePBKDF1(const qcc::String cipher, const qcc::String & passphrase, qcc::String & ivec, uint32_t iter)
    {
        if (!Init(cipher)) {
            return nullptr;
        }
        if (ivec.size() != 8) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Initialization vector has wrong length expected 8"));
            return nullptr;
        }
        if (keyLen > Crypto_MD5::DIGEST_SIZE) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Cannot generate key of length %d", keyLen));
            return nullptr;
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

        Platform::ArrayReference<uint8> keyMaterialRef(digest, keyLen);
        IBuffer ^ keyMaterial = CryptographicBuffer::CreateFromByteArray(keyMaterialRef);

        CryptographicKey ^ derivedSymKey = algProvider->CreateSymmetricKey(keyMaterial);

        free(kbh);
        return derivedSymKey;
    }

    CryptographicKey ^ DeriveLegacy(const qcc::String cipher, const qcc::String & passphrase, const qcc::String & ivec)
    {
        if (!Init(cipher)) {
            return nullptr;
        }
        if (ivec.size() != blockLen) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Initialization vector has wrong length expected %d", blockLen));
            return nullptr;
        }
        // Allocate and initialize a key blob big enough to compose 2 digests in-place
        ULONG kbhLen = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 2 * Crypto_MD5::DIGEST_SIZE;
        BCRYPT_KEY_DATA_BLOB_HEADER* kbh = (BCRYPT_KEY_DATA_BLOB_HEADER*)malloc(kbhLen);
        memset(kbh, 0, kbhLen);

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

        Platform::ArrayReference<uint8> keyMaterialRef(digest, keyLen);
        IBuffer ^ keyMaterial = CryptographicBuffer::CreateFromByteArray(keyMaterialRef);

        CryptographicKey ^ derivedSymKey = algProvider->CreateSymmetricKey(keyMaterial);

        free(kbh);

        return derivedSymKey;
    }

    ~PBKD() {
    }

  private:

    bool Init(const qcc::String& cipher)
    {
        Platform::String ^ algId;

        if ((cipher == OID_AES_CBC) || (cipher == "AES-128-CBC")) {
            algId = SymmetricAlgorithmNames::AesCbcPkcs7;
            keyLen = 16;
            blockLen = 16;
        } else if ((cipher == OID_DES_ED3_CBC) || (cipher ==  "DES-EDE3-CBC")) {
            algId = SymmetricAlgorithmNames::TripleDesCbcPkcs7;
            keyLen = 24;
            blockLen = 8;
        } else if (cipher == "DES-CBC") {
            algId = SymmetricAlgorithmNames::DesCbcPkcs7;
            keyLen = 8;
            blockLen = 8;
        } else {
            QCC_LogError(ER_CRYPTO_ERROR, ("Cipher %s not supported", cipher.c_str()));
            return false;
        }
        try {
            algProvider = SymmetricKeyAlgorithmProvider::OpenAlgorithm(algId);
        } catch (...) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to open algorithm provider"));
        }
        return true;
    }

    DWORD blockLen;
    DWORD keyLen;
    CryptographicKey ^ keyObjSym;
    SymmetricKeyAlgorithmProvider ^ algProvider;
};

//
// take in: kdey, ivec, blob
// return a private, asymetric key.
//
static QStatus DecryptPriv(CryptographicKey ^ kdKey, qcc::String& ivec, const uint8_t* blob, size_t blobLen, CryptographicKey ^& privKey, bool legacy)
{
    QStatus status = ER_AUTH_FAIL;
    qcc::String pk;
    uint32_t version;

    qcc::String n;  // modulus
    qcc::String e;  // Public exponent
    qcc::String d;  // Private exponent
    qcc::String p;  // prime1
    qcc::String q;  // prime2
    qcc::String exp1;  // d mod (p-1)
    qcc::String exp2;  // d mod (q-1
    qcc::String coef;  // prime2
    qcc::String PKCS8PrivateKey;
    qcc::String PrivateKey;

    AsymmetricKeyAlgorithmProvider ^ objAlgProv;

    // Decrypt the blob

    Platform::ArrayReference<uint8> blobRef((uint8*)blob, blobLen);
    IBuffer ^ blobBuf = CryptographicBuffer::CreateFromByteArray(blobRef);

    Platform::ArrayReference<uint8> ivecRef((uint8*)ivec.data(), ivec.size());
    IBuffer ^ ivecBuf = CryptographicBuffer::CreateFromByteArray(ivecRef);
    IBuffer ^ decryptedBlob;
    Platform::Array<uint8>^ decryptedArray;
    uint8_t* buf = nullptr;
    size_t bufLength = 0;
    try {
        if (kdKey) {
            decryptedBlob = CryptographicEngine::Decrypt(kdKey,  blobBuf, ivecBuf);

            CryptographicBuffer::CopyToByteArray(decryptedBlob, &decryptedArray);
            bufLength = decryptedArray->Length;
            buf = new uint8_t[bufLength];
            memcpy(buf, decryptedArray->Data, bufLength);
        } else {
            bufLength = blobLen;
            buf = new uint8_t[bufLength];
            memcpy(buf, blob, bufLength);
            decryptedBlob = blobBuf;
        }
        PrivateKey.assign((char*)buf, bufLength);


        // Check if the key is legacy or pkcs#8 encapsulated
        // See RFC 3447 for documentation on this formatting
        status = Crypto_ASN1::Decode(buf, bufLength, "(illllllll)", &version, &n, &e, &d, &p, &q, &exp1, &exp2, &coef);
        if (status == ER_OK) {
            legacy = true;
            status = Crypto_ASN1::Encode(PKCS8PrivateKey,
                                         "(i(on)R)",
                                         version,
                                         &OID_RSA_RSA,
                                         &PrivateKey);
            // copy the key into a new buffer
            Platform::ArrayReference<uint8> privateKeyArray((uint8*)PKCS8PrivateKey.data(), PKCS8PrivateKey.size());
        } else {
            // This might be a PKCS#8 encoded key, try to decode it.
            qcc::String oid;
            status = Crypto_ASN1::Decode(buf, bufLength, "(i(on)x*)", &version, &oid, &pk); // this ignores the rest of the key info.
            if (status == ER_OK) {
                if (oid != OID_RSA_RSA) {
                    QCC_LogError(status, ("Key was not an RSA private key"));
                    goto ExitDecryptPriv;
                }
                status = Crypto_ASN1::Decode(pk, "(ill?ll*)", &version, &n, &e, NULL, &p, &q);
            }
        }
    } catch (Platform::COMException ^ e) {
        QCC_LogError(status, ("COMException %S", e->Message->Data()));
        status = ER_AUTH_FAIL;
    }

    // Up to this point all failures are considered to be authentication failures
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to decode private key"));
        status = ER_AUTH_FAIL;
        goto ExitDecryptPriv;
    }

    // pk now holds the DER version of the keypair.

    // re-import the keys from the decrypted blob above.
    // Open the algorithm provider for the specified asymmetric algorithm.
    objAlgProv = AsymmetricKeyAlgorithmProvider::OpenAlgorithm(AsymmetricAlgorithmNames::RsaPkcs1);

    // the legacy keys were not this type:
    if (!legacy) {
        privKey = objAlgProv->ImportKeyPair(decryptedBlob, CryptographicPrivateKeyBlobType::Pkcs8RawPrivateKeyInfo);
    } else {
        privKey = objAlgProv->ImportKeyPair(decryptedBlob, CryptographicPrivateKeyBlobType::Pkcs1RsaPrivateKey);
    }


    pk.secure_clear();

    // Success
    status = ER_OK;

ExitDecryptPriv:

    // Clear out any plaintext key info
    if (buf) {
        memset(buf, 0, bufLength);
        delete [] buf;
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

    CryptographicKey ^ kdKey;
    if (!RSA_Init()) {
        return status;
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
        // stash the still encrypted DER certificate info for exporting later.
        certContext->derCertificate = der;

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

    CryptographicKey ^ keyForContext;

    try {
        if (kdKey) {
            status = DecryptPriv(kdKey, ivec, (const uint8_t*)pk.data(), pk.size(), keyForContext, legacy);
        } else if (passphrase.size() == 0) {
            // A cert with an empty passphras gets exported as legacy unencrypted cert.
            status = DecryptPriv(kdKey, ivec, (const uint8_t*)pk.data(), pk.size(), keyForContext, false);

        } else {
            status = ER_CRYPTO_ERROR;
        }
    } catch (Platform::COMException ^ e) {
        QCC_LogError(status, ("COMException %S", e->Message->Data()));
        status = ER_AUTH_FAIL;
    }

    if (status == ER_OK) {
        certContext->keyPair = keyForContext;
        certContext->keyPairSigning = certContext->asymAlgProvSigning->ImportKeyPair(certContext->keyPair->Export());
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

    uint8_t rand[SALT_LEN + IVEC_LEN] = { };
    uint8_t* iv = rand + SALT_LEN;

    // populate the salt and iv with random data.
    IBuffer ^ randBuffer = CryptographicBuffer::GenerateRandom(sizeof(rand));
    Platform::Array<uint8> ^ randArray;
    CryptographicBuffer::CopyToByteArray(randBuffer, &randArray);
    memcpy(rand, randArray->Data, sizeof(rand));

    qcc::String ivec((char*)iv, IVEC_LEN);
    qcc::String salt((char*)rand, SALT_LEN);


    PBKD pbkd;
    CryptographicKey ^ kdKey;


    IBuffer ^ Pkcs8Blob = certContext->keyPair->Export(CryptographicPrivateKeyBlobType::Pkcs8RawPrivateKeyInfo);
    Platform::Array<uint8>^ resArray;
    CryptographicBuffer::CopyToByteArray(Pkcs8Blob, &resArray);
    // Private key components are already encoded in PKCS#8 order
    qcc::String pk((char*)resArray->Data, resArray->Length);

    qcc::String pem;

    if (passphrase.size() == 0) {
        // Convert to base 64 and wrap with PEM header and trailer
        status = Crypto_ASN1::EncodeBase64(pk, pem);
        pem.insert(0, "-----BEGIN RSA PRIVATE KEY-----\n");
        pem.append("-----END RSA PRIVATE KEY-----\n");

    } else {
        kdKey = pbkd.DerivePBKDF2(OID_HMAC_SHA1, OID_AES_CBC, passphrase, salt, iter);

        // Encrypt the private key information

        // Note that iv is modified by this call which is why we copied it into a string ealier
        Platform::ArrayReference<uint8> ivecArray((uint8*)iv, IVEC_LEN);
        IBuffer ^ ivecBuffer = CryptographicBuffer::CreateFromByteArray(ivecArray);

        IBuffer ^ resultBuffer = CryptographicEngine::Encrypt(kdKey, Pkcs8Blob, ivecBuffer);

        Platform::Array<uint8>^ encryptedArray;
        CryptographicBuffer::CopyToByteArray(resultBuffer, &encryptedArray);
        uint8_t* buf = new uint8_t[encryptedArray->Length];

        memcpy(buf, encryptedArray->Data, encryptedArray->Length);

        // Private key components are already encoded in PKCS#8 order
        qcc::String pkInfo;
        pkInfo.assign((char*)buf, encryptedArray->Length);

        // ANS.1 encode the entire PKCS8 structure
        qcc::String der;
        status = Crypto_ASN1::Encode(der, "((o((o(xi))(ox)))x)", &OID_PBES2, &OID_PKDF2, &salt, iter, &OID_AES_CBC, &ivec, &pkInfo);

        // Convert to base 64 and wrap with PEM header and trailer
        status = Crypto_ASN1::EncodeBase64(der, pem);
        pem.insert(0, "-----BEGIN ENCRYPTED PRIVATE KEY-----\n");
        pem.append("-----END ENCRYPTED PRIVATE KEY-----\n");

        delete [] buf;
    }
    // All done
    keyBlob.Set((uint8_t*)pem.data(), pem.size(), KeyBlob::PRIVATE);

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
    if (certContext && (certContext->derCertificate.size() > 0)) {
        return Crypto_ASN1::ToString((uint8_t*)certContext->derCertificate.data(), certContext->derCertificate.size());
    } else {
        return "";
    }
}

QStatus Crypto_RSA::ExportPEM(qcc::String& pem)
{
    QStatus status = ER_OK;
    if (certContext) {



        if (certContext->derCertificate.size() > 0) {
            pem = "-----BEGIN CERTIFICATE-----\n";
            status = Crypto_ASN1::EncodeBase64(certContext->derCertificate, pem);
            pem.append("-----END CERTIFICATE-----\n");
        } else {
            // copied from makeselfsignedcert

            // generate a unique serial number for each cert.
            uint8_t serial[8] = { 0x01 };
            qcc::String SerialNumber((char*)serial, sizeof(serial));

            // get the time now and one year from now
            qcc::String timeNow;
            qcc::String timeOneYearLater;
            FormatTime(timeNow, timeOneYearLater);

            // format the public key for output
            IBuffer ^ X509PublicKey = certContext->keyPair->ExportPublicKey(CryptographicPublicKeyBlobType::X509SubjectPublicKeyInfo);
            Platform::Array<uint8>^ X509PublicKeyArray;
            CryptographicBuffer::CopyToByteArray(X509PublicKey, &X509PublicKeyArray);
            qcc::String PublicKey((char*)X509PublicKeyArray->Data, X509PublicKeyArray->Length); // this is already in ASN.1 format

            qcc::String emptyCommonName("AllJoynON");
            qcc::String emptyApp("AllJoyn CN");
            qcc::String Certificate;
            status = Crypto_ASN1::Encode(Certificate,
                                         "(l(on)({(ou)}{(ou)})(tt)({(ou)}{(ou)})R)",
                                         &SerialNumber,
                                         &OID_RSA_SHA1RSA,
                                         &OID_CN, &emptyCommonName, &OID_ORG, &emptyApp,
                                         &timeNow, &timeOneYearLater,
                                         &OID_CN, &emptyCommonName, &OID_ORG, &emptyApp,
                                         &PublicKey);

            if (status != ER_OK) {
                QCC_LogError(status, ("Failed encode the certificate string"));
                return status;
            }

            // Now sign the certificate data
            Platform::ArrayReference<uint8> CertificateArray((uint8*)Certificate.data(), Certificate.size());
            IBuffer ^ CertificateBuffer = CryptographicBuffer::CreateFromByteArray(CertificateArray);

            IBuffer ^ SignatureBuffer = CryptographicEngine::Sign(certContext->keyPairSigning, CertificateBuffer);
            Platform::Array<uint8>^ SignatureArray;
            CryptographicBuffer::CopyToByteArray(SignatureBuffer, &SignatureArray);
            qcc::String Signature((char*)SignatureArray->Data, SignatureArray->Length);

            // concat the certificate, algorithm, and signature.
            qcc::String SelfSignedCertificate;
            status = Crypto_ASN1::Encode(SelfSignedCertificate,
                                         "(R(on)b)",
                                         &Certificate,
                                         &OID_RSA_SHA1RSA,
                                         &Signature,
                                         SignatureArray->Length * 8);  //bytes to bits.
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed encode the certificate and its signature"));
                return status;
            }

            certContext->derCertificate = SelfSignedCertificate;

            pem = "-----BEGIN CERTIFICATE-----\n";
            status = Crypto_ASN1::EncodeBase64(certContext->derCertificate, pem);
            pem.append("-----END CERTIFICATE-----\n");




        }
    } else {
        QCC_LogError(status, ("No cert to export"));
    }
    return status;
}

size_t Crypto_RSA::GetSize()
{
    if (!size && certContext->keyPair) {
        size = certContext->keyPair->KeySize / 8;
    }
    return size;
}

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
    if (!certContext) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (!certContext->keyPairSigning) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }

    // Just encrypt the hash value instead of CryptographicEngine::Sign, which adds a digest envelope around hash.
    // This is to match how our other platforms work currently.

    // retrieve the private key as a BCrypt blob from the current context
    IBuffer ^ privateAsBCrypt = certContext->keyPairSigning->Export(CryptographicPrivateKeyBlobType::BCryptPrivateKey);
    Platform::Array<uint8> ^ BCryptArray;
    CryptographicBuffer::CopyToByteArray(privateAsBCrypt, &BCryptArray);
    size_t signatureLength = certContext->keyPairSigning->KeySize / 8;
    uint8_t* buf = new uint8_t[BCryptArray->Length];
    memcpy(buf, BCryptArray->Data, BCryptArray->Length);

    BCRYPT_RSAKEY_BLOB* pBlob = (BCRYPT_RSAKEY_BLOB*)buf;

    //check that this is the expected type of key
    if (pBlob->Magic != BCRYPT_RSAPRIVATE_MAGIC) {
        delete [] buf;
        return ER_AUTH_FAIL;
    }

    // retrieve the members of the blob and make the BigNums
    BigNum publicExponent;
    BigNum p;
    BigNum q;
    BigNum n;
    BigNum phi;
    BigNum privateExponent;
    BigNum bnSignature;

    // get the primes from the private key info.
    publicExponent.set_bytes(buf + sizeof(BCRYPT_RSAKEY_BLOB), pBlob->cbPublicExp);
    p.set_bytes(buf + sizeof(BCRYPT_RSAKEY_BLOB) + pBlob->cbPublicExp + pBlob->cbModulus, pBlob->cbPrime1);
    q.set_bytes(buf + sizeof(BCRYPT_RSAKEY_BLOB) + pBlob->cbPublicExp + pBlob->cbModulus + pBlob->cbPrime1, pBlob->cbPrime2);
    n = p * q;
    phi = (p - 1) * (q - 1);
    privateExponent = publicExponent.mod_inv(phi);

    // Pad the message according to PKCS#1 1.5, EMSA-PKCS1-v1_5-ENCODE
    uint8_t* padBuffer = new uint8_t[signatureLength];
    memset(padBuffer, 0xFF, signatureLength);
    padBuffer[0] = 0;
    padBuffer[1] = 1;
    padBuffer[(signatureLength - digLen) - 1] = 0;
    memcpy(padBuffer + (signatureLength - digLen), digest, digLen);

    bnSignature.set_bytes(padBuffer, signatureLength);

    // encrypt the buffer using the private key, and store into parameter signature.
    BigNum encryptedSignature = bnSignature.mod_exp(privateExponent, n);
    encryptedSignature.get_bytes(signature, signatureLength, false);

    sigLen = encryptedSignature.byte_len();

    delete [] buf;
    delete [] padBuffer;

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
    if (!certContext) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (!certContext->keyPair) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }

    // retrieve the public key as a BCrypt blob from the current context
    IBuffer ^ publicAsBCrypt = certContext->keyPairSigning->ExportPublicKey(CryptographicPublicKeyBlobType::BCryptPublicKey);
    Platform::Array<uint8> ^ BCryptArray;
    CryptographicBuffer::CopyToByteArray(publicAsBCrypt, &BCryptArray);
    size_t signatureLength = certContext->keyPairSigning->KeySize / 8;
    uint8_t* buf = new uint8_t[BCryptArray->Length];
    memcpy(buf, BCryptArray->Data, BCryptArray->Length);

    BCRYPT_RSAKEY_BLOB* pBlob = (BCRYPT_RSAKEY_BLOB*)buf;

    //check that this is the expected type of key
    if (pBlob->Magic != BCRYPT_RSAPUBLIC_MAGIC) {
        delete [] buf;
        return ER_AUTH_FAIL;
    }

    // retrieve the members of the blob and make the BigNums
    BigNum modulus;
    BigNum publicExponent;
    BigNum bnSignature;

    modulus.set_bytes(buf + sizeof(BCRYPT_RSAKEY_BLOB) + pBlob->cbPublicExp, pBlob->cbModulus);
    publicExponent.set_bytes(buf + sizeof(BCRYPT_RSAKEY_BLOB), pBlob->cbPublicExp);

    bnSignature.set_bytes(signature, signatureLength);

    // Decrypt the buffer using the public key.
    BigNum decryptedSignature = bnSignature.mod_exp(publicExponent, modulus);

    // convert the decypted data into a useful buffer.
    uint8_t* decryptedbuff2 = new uint8_t[signatureLength];
    decryptedSignature.get_bytes(decryptedbuff2, signatureLength, false);

    // pDigest will point to the digest inside the decrypted buffer
    uint8_t* pDigest = decryptedbuff2 + signatureLength - (digLen + 1);

    // if the supplied digest and the decrypted digest differ, return an error.
    if (0 != memcmp(digest, pDigest, digLen)) {
        status = ER_AUTH_FAIL;
    }

    delete [] buf;
    delete [] decryptedbuff2;

    return status;
}

QStatus Crypto_RSA::PublicEncrypt(const uint8_t* inData, size_t inLen, uint8_t* outData, size_t& outLen)
{
    if (!certContext) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (!certContext->keyPair) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (inLen > MaxDigestSize()) {
        return ER_CRYPTO_TRUNCATED;
    }
    if (outLen < GetSize()) {
        return ER_BUFFER_TOO_SMALL;
    }

    Platform::ArrayReference<uint8> inArray((uint8*)inData, inLen);
    IBuffer ^ inBuffer = CryptographicBuffer::CreateFromByteArray(inArray);

    IBuffer ^ encBuffer = CryptographicEngine::Encrypt(certContext->keyPair, inBuffer, nullptr);

    Platform::Array<uint8>^ encryptedArray;
    CryptographicBuffer::CopyToByteArray(encBuffer, &encryptedArray);

    memcpy(outData, encryptedArray->Data, encryptedArray->Length);

    outLen = encryptedArray->Length;
    return ER_OK;
}

QStatus Crypto_RSA::PrivateDecrypt(const uint8_t* inData, size_t inLen, uint8_t* outData, size_t& outLen)
{
    if (!certContext) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (!certContext->keyPair) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (inLen > GetSize()) {
        return ER_CRYPTO_TRUNCATED;
    }
    if (outLen < MaxDigestSize()) {
        return ER_BUFFER_TOO_SMALL;
    }

    Platform::ArrayReference<uint8> inArray((uint8*)inData, inLen);
    IBuffer ^ inBuffer = CryptographicBuffer::CreateFromByteArray(inArray);

    IBuffer ^ decBuffer = CryptographicEngine::Decrypt(certContext->keyPair, inBuffer, nullptr);

    Platform::Array<uint8>^ decryptedArray;
    CryptographicBuffer::CopyToByteArray(decBuffer, &decryptedArray);

    memcpy(outData, decryptedArray->Data, decryptedArray->Length);
    outLen = decryptedArray->Length;

    return ER_OK;
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
    delete certContext;
}
