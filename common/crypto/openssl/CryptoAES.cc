/**
 * @file CryptoAES.cc
 *
 * Class for AES block encryption/decryption
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

#include <algorithm>
#include <ctype.h>
/* due to a change in gcc6, cmath must be included now */
#if __GNUC__ >= 6
 #include <cmath>
#else
 #include <math.h>
#endif


#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Crypto.h>
#include <qcc/KeyBlob.h>
#include <qcc/Debug.h>

#include <Status.h>
#include "OpenSsl.h"

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

namespace qcc {

#ifdef CCM_TRACE
static void Trace(const char* tag, void* data, size_t len)
{
    qcc::String s = BytesToHexString(static_cast<uint8_t*>(data), len, false, ' ');
    printf("%s %s\n", tag, s.c_str());
}
#else
#define Trace(x, y, z)
#endif

/* definition of CryptoAES_BLOCK_LEN */
const size_t Crypto_AES::BLOCK_LEN;

struct Crypto_AES::KeyState {
    AES_KEY key;
};

Crypto_AES::Crypto_AES(const KeyBlob& key, Mode mode) : mode(mode), keyState(new KeyState())
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    AES_set_encrypt_key((unsigned char*)key.GetData(), key.GetSize() * 8, &keyState->key);

#ifdef QCC_LINUX_OPENSSL_GT_1_1_X
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
#endif
}

Crypto_AES::~Crypto_AES()
{
    delete keyState;
#ifdef QCC_LINUX_OPENSSL_GT_1_1_X
    EVP_cleanup();
    ERR_free_strings();
#endif
}

QStatus Crypto_AES::Encrypt(const Block* in, Block* out, uint32_t numBlocks)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;
    if (nullptr == in) {
        return ER_BAD_ARG_1;
    }
    if (nullptr == out) {
        return ER_BAD_ARG_2;
    }
    /*
     * Check if we are initialized for encryption
     */
    if (mode != ECB_ENCRYPT) {
        return ER_CRYPTO_ERROR;
    }
    while (numBlocks--) {
        AES_encrypt(in->data, out->data, &keyState->key);
        ++in;
        ++out;
    }
    return ER_OK;
}

QStatus Crypto_AES::Encrypt(const void* in, size_t len, Block* out, uint32_t numBlocks)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;
    QStatus status;

    if (nullptr == in) {
        return ER_BAD_ARG_1;
    }

    if (nullptr == out) {
        return ER_BAD_ARG_3;
    }
    /*
     * Check the lengths make sense
     */
    if (numBlocks != NumBlocks(len)) {
        return ER_CRYPTO_ERROR;
    }
    /*
     * Check for a partial final block
     */
    size_t partial = len % Crypto_AES::BLOCK_LEN;
    Block inBlock(static_cast<const uint8_t*>(in), 16);
    if (partial) {
        numBlocks--;
        status = Encrypt(&inBlock, out, numBlocks);
        if (status == ER_OK) {
            Block padBlock;
            memcpy(&padBlock, (static_cast<const uint8_t*>(in) + numBlocks * Crypto_AES::BLOCK_LEN), partial);
            status = Encrypt(&padBlock, out + numBlocks, 1);
        }
    } else {
        status = Encrypt(&inBlock, out, numBlocks);
    }
    return status;
}

static inline uint8_t LengthOctetsFor(size_t len)
{
    if (len <= 0xFFFF) {
        return 2;
    } else if (len <= 0xFFFFFF) {
        return 3;
    } else {
        return 4;
    }
}

#ifndef QCC_LINUX_OPENSSL_GT_1_1_X
static void Compute_CCM_AuthField(AES_KEY* key, Crypto_AES::Block& T, uint8_t M, uint8_t L, const KeyBlob& nonce, const uint8_t* mData, size_t mLen, const uint8_t* aadData, size_t aadLen)
{
    uint8_t flags = ((aadLen) ? 0x40 : 0) | (((M - 2) / 2) << 3) | (L - 1);
    /*
     * Compute the B_0 block. This encodes the flags, the nonce, and the data length.
     */
    Crypto_AES::Block B_0(0);
    B_0.data[0] = flags;
    memset(&B_0.data[1], 0, 15 - L);
    memcpy(&B_0.data[1], nonce.GetData(), min((size_t)15, nonce.GetSize()));
    for (size_t i = 15, l = mLen; l != 0; i--) {
        B_0.data[i] = static_cast<uint8_t>(l & 0xFF);
        l >>= 8;
    }
    /*
     * Initialize CBC-MAC with B_0 initialization vector is 0.
     */
    Crypto_AES::Block ivec(0);
    Trace("CBC IV in: ", B_0.data, sizeof(B_0.data));
    AES_cbc_encrypt(B_0.data, T.data, sizeof(T.data), key, ivec.data, AES_ENCRYPT);
    Trace("CBC IV out:", T.data, sizeof(T.data));
    /*
     * Compute CBC-MAC for the add data.
     */
    if (aadLen) {
        /*
         * This encodes the add data length and the first few octets of the add data
         */
        Crypto_AES::Block A;
        size_t initialLen;
        if (aadLen < ((1 << 16) - (1 << 8))) {
            A.data[0] = static_cast<uint8_t>(aadLen >> 8);
            A.data[1] = static_cast<uint8_t>(aadLen >> 0);
            initialLen = min(aadLen, sizeof(A.data) - 2);
            memcpy(&A.data[2], aadData, initialLen);
            A.Pad(16 - initialLen - 2);
        } else {
            A.data[0] = 0xFF;
            A.data[1] = 0xFE;
            A.data[2] = static_cast<uint8_t>(aadLen >> 24);
            A.data[3] = static_cast<uint8_t>(aadLen >> 16);
            A.data[4] = static_cast<uint8_t>(aadLen >> 8);
            A.data[5] = static_cast<uint8_t>(aadLen >> 0);
            initialLen = sizeof(A.data) - 6;
            memcpy(&A.data[6], aadData, initialLen);
        }
        aadData += initialLen;
        aadLen -= initialLen;
        /*
         * Continue computing the CBC-MAC
         */
        AES_cbc_encrypt(A.data, T.data, sizeof(T.data), key, ivec.data, AES_ENCRYPT);
        Trace("After AES: ", T.data, sizeof(T.data));
        while (aadLen >= Crypto_AES::BLOCK_LEN) {
            AES_cbc_encrypt(aadData, T.data, sizeof(T.data), key, ivec.data, AES_ENCRYPT);
            Trace("After AES: ", T.data, sizeof(T.data));
            aadData += Crypto_AES::BLOCK_LEN;
            aadLen -= Crypto_AES::BLOCK_LEN;
        }
        if (aadLen) {
            memcpy(A.data, aadData, aadLen);
            A.Pad(16 - aadLen);
            AES_cbc_encrypt(A.data, T.data, sizeof(T.data), key, ivec.data, AES_ENCRYPT);
            Trace("After AES: ", T.data, sizeof(T.data));
        }

    }
    /*
     * Continue computing CBC-MAC over the message data.
     */
    if (mLen) {
        while (mLen >= Crypto_AES::BLOCK_LEN) {
            AES_cbc_encrypt(mData, T.data, sizeof(T.data), key, ivec.data, AES_ENCRYPT);
            Trace("After AES: ", T.data, sizeof(T.data));
            mData += Crypto_AES::BLOCK_LEN;
            mLen -= Crypto_AES::BLOCK_LEN;
        }
        if (mLen) {
            Crypto_AES::Block final;
            memcpy(final.data, mData, mLen);
            final.Pad(16 - mLen);
            AES_cbc_encrypt(final.data, T.data, sizeof(T.data), key, ivec.data, AES_ENCRYPT);
            Trace("After AES: ", T.data, sizeof(T.data));
        }
    }
    Trace("CBC-MAC:   ", T.data, M);
}

/*
 * Implementation of AES-CCM (Counter with CBC-MAC) as described in RFC 3610
 */
QStatus Crypto_AES::Encrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* aadData, size_t aadLen, uint8_t authLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;
    /*
     * Check we are initialized for CCM
     */
    if (mode != CCM) {
        return ER_CRYPTO_ERROR;
    }
    size_t nLen = nonce.GetSize();
    if ((nullptr == in) && len) {
        return ER_BAD_ARG_1;
    }
    if ((nullptr == out) && len) {
        return ER_BAD_ARG_2;
    }
    if ((nLen < 4) || (nLen > 14)) {
        return ER_BAD_ARG_4;
    }
    if ((authLen < 4) || (authLen > 16)) {
        return ER_BAD_ARG_7;
    }
    const uint8_t L = 15 - static_cast<uint8_t>(max(nLen, (size_t)11));
    if (L < LengthOctetsFor(len)) {
        return ER_BAD_ARG_3;
    }
    /*
     * Compute the authentication field T.
     */
    Block T;
    Compute_CCM_AuthField(&keyState->key, T, authLen, L, nonce, static_cast<const uint8_t*>(in), len, static_cast<const uint8_t*>(aadData), aadLen);
    /*
     * Initialize ivec and other initial args.
     */
    Block ivec(0);
    ivec.data[0] = (L - 1);
    memcpy(&ivec.data[1], nonce.GetData(), nLen);
    unsigned int num = 0;
    Block ecount_buf(0);
    /*
     * Encrypt the authentication field
     */
    Block U;
    AES_ctr128_encrypt(T.data, U.data, 16, &keyState->key, ivec.data, ecount_buf.data, &num);
    Trace("CTR Start: ", ivec.data, 16);
    AES_ctr128_encrypt(static_cast<const uint8_t*>(in), static_cast<uint8_t*>(out), len, &keyState->key, ivec.data, ecount_buf.data, &num);
    memcpy(static_cast<uint8_t*>(out) + len, U.data, authLen);
    len += authLen;
    return ER_OK;
}


QStatus Crypto_AES::Decrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* aadData, size_t aadLen, uint8_t authLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;
    /*
     * Check we are initialized for CCM
     */
    if (mode != CCM) {
        return ER_CRYPTO_ERROR;
    }
    size_t nLen = nonce.GetSize();
    if (nullptr == in) {
        return ER_BAD_ARG_1;
    }
    if ((0 == len) || (len < authLen)) {
        return ER_BAD_ARG_3;
    }
    if ((nLen < 4) || (nLen > 14)) {
        return ER_BAD_ARG_4;
    }
    if ((authLen < 4) || (authLen > 16)) {
        return ER_BAD_ARG_7;
    }
    const uint8_t L = 15 - static_cast<uint8_t>(max(nLen, (size_t)11));
    if (L < LengthOctetsFor(len)) {
        return ER_BAD_ARG_3;
    }
    /*
     * Initialize ivec and other initial args.
     */
    Block ivec(0);
    ivec.data[0] = (L - 1);
    memcpy(&ivec.data[1], nonce.GetData(), nLen);
    unsigned int num = 0;
    Block ecount_buf(0);
    /*
     * Decrypt the authentication field
     */
    Block U;
    Block T;
    len = len - authLen;
    memcpy(U.data, static_cast<const uint8_t*>(in) + len, authLen);
    AES_ctr128_encrypt(U.data, T.data, sizeof(T.data), &keyState->key, ivec.data, ecount_buf.data, &num);
    /*
     * Decrypt message.
     */
    AES_ctr128_encrypt(static_cast<const uint8_t*>(in), static_cast<uint8_t*>(out), len, &keyState->key, ivec.data, ecount_buf.data, &num);
    /*
     * Compute and verify the authentication field T.
     */
    Block F;
    Compute_CCM_AuthField(&keyState->key, F, authLen, L, nonce, static_cast<uint8_t*>(out), len, static_cast<const uint8_t*>(aadData), aadLen);
    if (Crypto_Compare(F.data, T.data, authLen) == 0) {
        return ER_OK;
    } else {
        /* Clear the decrypted data */
        memset(out, 0, len + authLen);
        len = 0;
        return ER_AUTH_FAIL;
    }
}
//Due to changes in OpenSSL1.1.x the Encrypt_CCM and Decrypt_CCM functions must be implemented differently
#else

#ifndef AES_CCM_IV_MIN_LEN
#define AES_CCM_IV_MIN_LEN 7
#endif
//Get the openssl libcrypto errors and return QStatus
static QStatus CCM_handleErrors(QStatus status)
{
    char* buf = static_cast<char*>(calloc(128, sizeof(char)));
    if (nullptr == buf) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("Failed to allocate memory for Crypto error message."));
        return status;
    }
    ERR_error_string(ERR_get_error(), buf);
    QCC_LogError(status, ("error message from libcrypto: %s", buf));

    free(buf);
    return status;
}

//Set the IV
static QStatus SetIV(const KeyBlob& nonce, Crypto_AES::Block& iv, int& ivLen)
{
    OpenSsl_ScopedLock lock;
    size_t origLen = nonce.GetSize();

    //Just in case this function is called somewhere else without input check
    if ((origLen < 4) || (origLen > 14)) {
        return ER_BAD_ARG_4;
    }

    memcpy(iv.data, nonce.GetData(), origLen);
    ivLen = origLen;

    //Pad IV if necessary. AES_CCM requires 7 bytes minimum
    if (origLen < AES_CCM_IV_MIN_LEN) {
        memcpy(iv.data + origLen, nonce.GetData(), AES_CCM_IV_MIN_LEN - origLen);
        ivLen = AES_CCM_IV_MIN_LEN;
    }
    return ER_OK;
}

QStatus Crypto_AES::Encrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* aadData, size_t aadLen, uint8_t authLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;
    /*
     * Check if we are initialized for CCM
     */
    if (mode != CCM) {
        return ER_CRYPTO_ERROR;
    }
    size_t nLen = nonce.GetSize();
    if ((nullptr == in) && len) {
        return ER_BAD_ARG_1;
    }
    if ((nullptr == out) && len) {
        return ER_BAD_ARG_2;
    }
    if ((nLen < 4) || (nLen > 14)) {
        return ER_BAD_ARG_4;
    }
    if ((authLen < 4) || (authLen > 16)) {
        return ER_BAD_ARG_7;
    }
    const uint8_t L = 15 - static_cast<uint8_t>(max(nLen, (size_t)11));
    if (L < LengthOctetsFor(len)) {
        return ER_BAD_ARG_3;
    }

    Block iv;
    int ivLen;
    QStatus status = SetIV(nonce, iv, ivLen);

    if (ER_OK != status) {
        return status;
    }

    EVP_CIPHER_CTX* ctx;
    int ciphertext_len;
    int plaintext_len = len;
    int encrypt_len;
    Block tag(0);

    /* Create and initialize the context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        return CCM_handleErrors(ER_CRYPTO_CTX_NEW_FAIL);
    }

    /* Initialize the encryption operation. */
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ccm(), nullptr, nullptr, nullptr)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_INIT_FAIL);
    }

    /* Set IV len, minimal & default is 7.*/
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, ivLen, nullptr)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_CTRL_FAIL);
    }

    /* Set tag length, must use nullptr for the buffer here */
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, authLen, nullptr)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_CTRL_FAIL);
    }

    /* Initialize key and IV */
    if (1 != EVP_EncryptInit_ex(ctx, nullptr, nullptr, reinterpret_cast<const uint8_t*>(&keyState->key), iv.data)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_INIT_FAIL);
    }

    /* Provide the total plaintext length */
    if (1 != EVP_EncryptUpdate(ctx, nullptr, &encrypt_len, nullptr, plaintext_len)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_UPDATE_FAIL);
    }

    /* Provide AAD data, if there is any */
    if ((aadLen > 0) && (nullptr != aadData)) {
        if (1 != EVP_EncryptUpdate(ctx, nullptr, &encrypt_len, static_cast<const uint8_t*>(aadData), aadLen)) {
            return CCM_handleErrors(ER_CRYPTO_CTX_UPDATE_FAIL);
        }
    }

    /* Provide the message to be encrypted, and obtain the encrypted output. */
    if (1 != EVP_EncryptUpdate(ctx, static_cast<uint8_t*>(out), &encrypt_len, static_cast<const uint8_t*>(in), plaintext_len)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_UPDATE_FAIL);
    }

    ciphertext_len = encrypt_len;

    /* Finalize the encryption. */
    if (1 != EVP_EncryptFinal_ex(ctx, static_cast<uint8_t*>(out) + encrypt_len, &encrypt_len)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_FINAL_FAIL);
    }
    ciphertext_len += encrypt_len;

    /* Get the tag */
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_GET_TAG, authLen, tag.data)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_CTRL_FAIL);
    }

    /* Clean up CTX */
    EVP_CIPHER_CTX_free(ctx);

    /* Append the tag data to the end of ciphertext(out), and increase the length of total message
     */
    memcpy(static_cast<uint8_t*>(out) + ciphertext_len, tag.data, authLen);
    len = ciphertext_len + authLen;

    return ER_OK;
}

QStatus Crypto_AES::Decrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* aadData, size_t aadLen, uint8_t authLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;
    /*
     * Check we are initialized for CCM
     */

    if (mode != CCM) {
        return ER_CRYPTO_ERROR;
    }
    size_t nLen = nonce.GetSize();
    if (nullptr == in) {
        return ER_BAD_ARG_1;
    }
    if ((0 == len) || (len < authLen)) {
        return ER_BAD_ARG_3;
    }
    if ((nLen < 4) || (nLen > 14)) {
        return ER_BAD_ARG_4;
    }
    if ((authLen < 4) || (authLen > 16)) {
        return ER_BAD_ARG_7;
    }
    const uint8_t L = 15 - static_cast<uint8_t>(max(nLen, (size_t)11));
    if (L < LengthOctetsFor(len)) {
        return ER_BAD_ARG_3;
    }

    Block iv;
    int ivLen;
    QStatus status;

    status = SetIV(nonce, iv, ivLen);

    if (ER_OK != status) {
        return status;
    }

    EVP_CIPHER_CTX* ctx;

    int ciphertext_len = len - authLen;
    int decrypt_len;
    Block tag(0);
    memcpy(tag.data, static_cast<const uint8_t*>(in) + ciphertext_len, authLen);

    /* Create and initialize the context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        return CCM_handleErrors(ER_CRYPTO_CTX_NEW_FAIL);
    }

    /* Initialise the encryption operation. */
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_ccm(), nullptr, nullptr, nullptr)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_INIT_FAIL);
    }

    /* Set IV len, minimal & default is 7.*/
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, ivLen, nullptr)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_CTRL_FAIL);
    }

    /* Set tag and length */
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, authLen, tag.data);

    /* Initialize key and IV */
    if (1 != EVP_DecryptInit_ex(ctx, nullptr, nullptr, reinterpret_cast<const uint8_t*>(&keyState->key), iv.data)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_INIT_FAIL);
    }

    /* Provide the total plaintext length */
    if (1 != EVP_DecryptUpdate(ctx, nullptr, &decrypt_len, nullptr, ciphertext_len)) {
        return CCM_handleErrors(ER_CRYPTO_CTX_UPDATE_FAIL);
    }

    /* Provide AAD data if there is any */
    if ((aadLen > 0) && (nullptr != aadData)) {
        if (1 != EVP_DecryptUpdate(ctx, nullptr, &decrypt_len, static_cast<const uint8_t*>(aadData), aadLen)) {
            return CCM_handleErrors(ER_CRYPTO_CTX_UPDATE_FAIL);
        }
    }

    /* Provide the message to be decrypted, and obtain the decrypted output.
     * EVP_DecryptUpdate can only be called once for this
     */
    if (1 != EVP_DecryptUpdate(ctx, static_cast<uint8_t*>(out), &decrypt_len, static_cast<const uint8_t*>(in), ciphertext_len)) {
        /* usually this fails due to auth data(Tag) mismatch between the input and computed */
        return CCM_handleErrors(ER_AUTH_FAIL);
    }
    len = decrypt_len;

    /* Note for Openssl-1.1.0, EVP_DecryptFinal_ex() aways fails. Although it is not necessary
     * for CCM mode, we should consider calling it for consistency once OpenSSL fixes it
     */

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ER_OK;
}
#endif //QCC_LINUX_OPENSSL_GT_1_1_X

}
