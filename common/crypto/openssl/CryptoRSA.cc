/**
 * @file CryptoRSA.cc
 *
 * Class for RSA public/private key encryption
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

#include <assert.h>
#include <ctype.h>

#include <qcc/Debug.h>
#include <qcc/Crypto.h>

#include <Status.h>
#include "OpenSsl.h"

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

#define EXPIRE_DAYS(days)   (long)(60 * 60 * 24 * (days))


/*
 * Default passphrases listener class for implementing APIs that pass in the passphrase string.
 */
class DefaultPassphraseListener : public Crypto_RSA::PassphraseListener {
  public:

    DefaultPassphraseListener(const qcc::String& passphrase) : passphrase(passphrase) { }

    bool GetPassphrase(qcc::String& passphrase, bool toWrite) {
        passphrase = this->passphrase;
        return true;
    }

    ~DefaultPassphraseListener() {
        /* Zero out the passphrase */
        for (size_t i = 0; i < passphrase.size(); ++i) {
            passphrase[i] = 0;
        }
    }

  private:

    qcc::String passphrase;
};

class PassphraseContext {
  public:
    PassphraseContext(Crypto_RSA::PassphraseListener* listener) : listener(listener), status(ER_CRYPTO_ERROR) { }
    Crypto_RSA::PassphraseListener* listener;
    QStatus status;
};

/*
 * Passphrase callback function called by openssl when a passphase is required.
 */
static int passphrase_cb(char* buf, int size, int rwflag, void* u)
{
    qcc::String passphrase;
    PassphraseContext* context = static_cast<PassphraseContext*>(u);

    if (context->listener->GetPassphrase(passphrase, static_cast<bool>(rwflag))) {
        if ((size_t)size > passphrase.size()) {
            size = (int)passphrase.size();
        }
        memcpy(buf, passphrase.data(), size);
        /* Zero out the passphrase now we have copied it */
        for (size_t i = 0; i < passphrase.size(); ++i) {
            passphrase[i] = 0;
        }
        context->status = ER_AUTH_FAIL;
    } else {
        context->status = ER_AUTH_USER_REJECT;
        size = 0;
    }
    if (size == 0) {
        *buf = 0;
    }
    return size;
}

bool Crypto_RSA::RSA_Init()
{
    return true;
}

void Crypto_RSA::Generate(uint32_t keyLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    BIGNUM* bn = BN_new();
    key = RSA_new();
    if (bn && key) {
        if (!BN_set_word(bn, 65537) || !RSA_generate_key_ex((RSA*)key, keyLen, bn, NULL)) {
            RSA_free((RSA*)key);
            key = NULL;
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to generate RSA key"));
        }
    }
    BN_free(bn);
}

QStatus Crypto_RSA::MakeSelfCertificate(const qcc::String& commonName, const qcc::String& app)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    QStatus status = ER_OK;
    int serial = 0;
    X509* x509 = X509_new();

    /*
     * Free old cert if there was one.
     */
    if (cert) {
        X509_free((X509*)cert);
        cert = NULL;
    }

    X509_set_version(x509, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(x509), serial);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), EXPIRE_DAYS(365));

    EVP_PKEY* evpk = EVP_PKEY_new();
    if (!evpk) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("Failed to allocate memory for EVP PKEY"));
        return status;
    }
    Generate(512);
    EVP_PKEY_set1_RSA(evpk, (RSA*)key);
    X509_set_pubkey(x509, evpk);

    struct X509_name_st* name = X509_get_subject_name(x509);
    /*
     * Common name.
     */
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)commonName.c_str(), commonName.size(), -1, 0);
    /*
     * Org (application in this case)
     */
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (const unsigned char*)app.c_str(), app.size(), -1, 0);
    /*
     * This is a self-signed x509 so the issuer is the same as the subject
     */
    X509_set_issuer_name(x509, name);
    /*
     * Sign the certificate
     */
    if (X509_sign(x509, evpk, EVP_sha1())) {
        cert = x509;
        QCC_DbgHLPrintf(("MakeSelfCertificate()\n%s", CertToString().c_str()));
    } else {
        X509_free(x509);
        status = ER_CRYPTO_ERROR;
    }
    EVP_PKEY_free(evpk);
    return status;
}

QStatus Crypto_RSA::ImportPEM(const qcc::String& pem)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

#ifndef NDEBUG
    ERR_load_crypto_strings();
#endif
    QStatus status = ER_CRYPTO_ERROR;
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_write(bio, pem.c_str(), pem.size());
    X509* x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (x509) {
        EVP_PKEY* evpk = X509_get_pubkey(x509);
        if (evpk) {
            cert = x509;
            key = EVP_PKEY_get1_RSA(evpk);
            EVP_PKEY_free(evpk);
            status = ER_OK;
        } else {
            X509_free(x509);
        }
    }
    return status;
}

QStatus Crypto_RSA::ImportPKCS8(const qcc::String& pkcs8, const qcc::String& passphrase)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (passphrase.empty()) {
        return ImportPKCS8(pkcs8, NULL);
    } else {
        DefaultPassphraseListener listener(passphrase);
        return ImportPKCS8(pkcs8, &listener);
    }
}

QStatus Crypto_RSA::ImportPKCS8(const qcc::String& pkcs8, PassphraseListener* listener)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (key) {
        RSA_free((RSA*)key);
        key = NULL;
    }
    QStatus status = ER_CRYPTO_ERROR;
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_write(bio, pkcs8.c_str(), pkcs8.size());
    /* Load all ciphers and digests */
    OpenSSL_add_all_algorithms();
    if (listener) {
        PassphraseContext context(listener);
        key = PEM_read_bio_RSAPrivateKey(bio, NULL, passphrase_cb, (void*)&context);
        if (key) {
            status = ER_OK;
        } else {
            status = context.status;
        }
    } else {
        key = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
        if (key) {
            status = ER_OK;
        }
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("PEM_read_bio_RSAPrivateKey() failed %s", ERR_error_string(ERR_get_error(), NULL)));
    }
    BIO_free(bio);
    return status;
}

QStatus Crypto_RSA::ImportPrivateKey(const qcc::KeyBlob& keyBlob, const qcc::String& passphrase)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (keyBlob.GetType() == KeyBlob::PRIVATE) {
        qcc::String pkcs8((const char*)keyBlob.GetData(), keyBlob.GetSize());
        return ImportPKCS8(pkcs8, passphrase);
    } else {
        return ER_CRYPTO_ERROR;
    }
}

QStatus Crypto_RSA::ImportPrivateKey(const qcc::KeyBlob& keyBlob, PassphraseListener* listener)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (keyBlob.GetType() == KeyBlob::PRIVATE) {
        qcc::String pkcs8((const char*)keyBlob.GetData(), keyBlob.GetSize());
        return ImportPKCS8(pkcs8, listener);
    } else {
        return ER_CRYPTO_ERROR;
    }
}

QStatus Crypto_RSA::ExportPrivateKey(qcc::KeyBlob& keyBlob, const qcc::String& passphrase)
{
    if (passphrase.empty()) {
        return ExportPrivateKey(keyBlob, NULL);
    } else {
        DefaultPassphraseListener listener(passphrase);
        return ExportPrivateKey(keyBlob, &listener);
    }
}

QStatus Crypto_RSA::ExportPrivateKey(qcc::KeyBlob& keyBlob, PassphraseListener* listener)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (!key) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    QStatus status = ER_CRYPTO_ERROR;
    BIO* bio = BIO_new(BIO_s_mem());
    EVP_PKEY* evpk = EVP_PKEY_new();
    if (!evpk) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("Failed to allocate memory for EVP PKEY"));
        return status;
    }

    EVP_PKEY_set1_RSA(evpk, (RSA*)key);
    /* Load all ciphers and digests */
    OpenSSL_add_all_algorithms();
    if (listener) {
        PassphraseContext context(listener);
        if (PEM_write_bio_PKCS8PrivateKey(bio, evpk, EVP_aes_128_cbc(), NULL, 0, passphrase_cb, (void*)&context)) {
            status = ER_OK;
        } else {
            status = context.status;
        }
    } else {
        if (PEM_write_bio_PKCS8PrivateKey(bio, evpk, NULL, NULL, 0, NULL, NULL)) {
            status = ER_OK;
        }
    }
    if (status == ER_OK) {
        int32_t len = BIO_pending(bio);
        uint8_t* s = new uint8_t[len];
        if (BIO_read(bio, s, len) == len) {
            keyBlob.Set(s, len, KeyBlob::PRIVATE);
        }
        delete [] s;
    } else {
        QCC_LogError(status, ("PEM_write_bio_PKCS8PrivateKey() failed %s", ERR_error_string(0, NULL)));
    }
    /* Cleanup */
    EVP_PKEY_free(evpk);
    BIO_free(bio);
    return status;
}

qcc::String Crypto_RSA::CertToString()
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    BIO* bio = BIO_new(BIO_s_mem());
    qcc::String str;
    if (cert && X509_print(bio, (X509*)cert)) {
        int32_t len = BIO_pending(bio);
        char* s = new char[len + 1];
        if (BIO_read(bio, s, len) == len) {
            s[len] = 0;
            str = s;
        }
        delete [] s;
    }
    BIO_free(bio);
    return str;
}

QStatus Crypto_RSA::ExportPEM(qcc::String& pem)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    QStatus status = ER_CRYPTO_ERROR;
    BIO* bio = BIO_new(BIO_s_mem());
    if (cert) {
        if (PEM_write_bio_X509(bio, (X509*)cert)) {
            status = ER_OK;
        }
    }
    if (status == ER_OK) {
        int32_t len = BIO_pending(bio);
        char* s = new char[len + 1];
        if (BIO_read(bio, s, len) == len) {
            s[len] = 0;
            pem = s;
        }
        delete [] s;
    } else {
        QCC_LogError(status, ("PEM_write_bio_X509() failed %s", ERR_error_string(0, NULL)));
    }
    BIO_free(bio);
    return status;
}

size_t Crypto_RSA::GetSize()
{
    if (!size) {
        size = key ? (size_t)RSA_size((RSA*)key) : 0;
    }
    return size;
}

QStatus Crypto_RSA::Sign(const uint8_t* data, size_t len, uint8_t* signature, size_t& sigLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (!data) {
        return ER_BAD_ARG_1;
    }
    if (!signature) {
        return ER_BAD_ARG_3;
    }
    if (!key) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (sigLen < GetSize()) {
        return ER_BUFFER_TOO_SMALL;
    }
    sigLen = GetSize();
    unsigned int l = (unsigned int)len;
    if (RSA_sign(NID_sha1, data, len, signature, &l, (RSA*)key)) {
        len = l;
        return ER_OK;
    } else {
        QStatus status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("RSA_sign() failed %s", ERR_error_string(0, NULL)));
        return status;
    }
}

QStatus Crypto_RSA::Verify(const uint8_t* data, size_t len, const uint8_t* signature, size_t sigLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (!data) {
        return ER_BAD_ARG_1;
    }
    if (!signature) {
        return ER_BAD_ARG_3;
    }
    if (!key) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (sigLen < GetSize()) {
        return ER_BUFFER_TOO_SMALL;
    }
    sigLen = GetSize();
    if (RSA_verify(NID_sha1, data, (unsigned int)len, (unsigned char*)signature, sigLen, (RSA*)key)) {
        return ER_OK;
    } else {
        QStatus status = ER_AUTH_FAIL;
        QCC_LogError(status, ("RSA_verify() failed %s", ERR_error_string(0, NULL)));
        return status;
    }
}

QStatus Crypto_RSA::PublicEncrypt(const uint8_t* inData, size_t inLen, uint8_t* outData, size_t& outLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (!key) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (outLen < GetSize()) {
        return ER_BUFFER_TOO_SMALL;
    }
    int32_t num = RSA_public_encrypt((int)inLen, inData, outData, (RSA*)key, RSA_PKCS1_PADDING);

    if (num < 0) {
        return ER_CRYPTO_ERROR;
    }
    outLen = num;
    return ER_OK;
}

QStatus Crypto_RSA::PrivateDecrypt(const uint8_t* inData, size_t inLen, uint8_t* outData, size_t& outLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (!key) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (inLen != GetSize()) {
        return ER_CRYPTO_TRUNCATED;
    }
    if (outLen < MaxDigestSize()) {
        return ER_BUFFER_TOO_SMALL;
    }
    int32_t num = RSA_private_decrypt((int)inLen, inData, outData, (RSA*)key, RSA_PKCS1_PADDING);

    if (num < 0) {
        return ER_CRYPTO_ERROR;
    }
    outLen = num;
    return ER_OK;
}

QStatus Crypto_RSA::SignDigest(const uint8_t* digest, size_t digLen, uint8_t* signature, size_t& sigLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (!key) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (digLen > MaxDigestSize()) {
        return ER_CRYPTO_TRUNCATED;
    }
    if (sigLen < GetSize()) {
        return ER_BUFFER_TOO_SMALL;
    }
    int32_t num = RSA_private_encrypt((int)digLen, digest, signature, (RSA*)key, RSA_PKCS1_PADDING);
    if (num < 0) {
        return ER_CRYPTO_ERROR;
    }
    sigLen = num;
    return ER_OK;
}

QStatus Crypto_RSA::VerifyDigest(const uint8_t* digest, size_t digLen, const uint8_t* signature, size_t sigLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    QStatus status = ER_OK;
    if (!key) {
        return ER_CRYPTO_KEY_UNUSABLE;
    }
    if (digLen > MaxDigestSize()) {
        return ER_AUTH_FAIL;
    }
    uint8_t* plaintext = new uint8_t[MaxDigestSize()];
    int32_t num = RSA_public_decrypt((int)sigLen, signature, plaintext, (RSA*)key, RSA_PKCS1_PADDING);
    if (((size_t)num != digLen) || (memcmp(plaintext, digest, num) != 0)) {
        status = (num < 0) ? ER_CRYPTO_ERROR : ER_AUTH_FAIL;
    }
    delete [] plaintext;
    return status;
}

Crypto_RSA::Crypto_RSA() : size(0), cert(NULL), key(NULL), certContext(NULL)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

#ifndef NDEBUG
    ERR_load_crypto_strings();
#endif
}

Crypto_RSA::~Crypto_RSA()
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (key) {
        RSA_free((RSA*)key);
    }
    if (cert) {
        X509_free((X509*)cert);
    }
}
