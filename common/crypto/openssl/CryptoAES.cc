/**
 * @file CryptoAES.cc
 *
 * Class for AES block encryption/decryption
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

#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Crypto.h>
#include <qcc/KeyBlob.h>

#include <Status.h>
#include "OpenSsl.h"

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

namespace qcc {

#ifdef CCM_TRACE
static void Trace(const char* tag, void* data, size_t len)
{
    qcc::String s = BytesToHexString((uint8_t*)data, len, false, ' ');
    printf("%s %s\n", tag, s.c_str());
}
#else
#define Trace(x, y, z)
#endif

struct Crypto_AES::KeyState {
    AES_KEY key;
};

Crypto_AES::Crypto_AES(const KeyBlob& key, Mode mode) : mode(mode), keyState(new KeyState())
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if ((mode == ECB_ENCRYPT) || (mode == CCM)) {
        AES_set_encrypt_key((unsigned char*)key.GetData(), key.GetSize() * 8, &keyState->key);
    } else {
        AES_set_decrypt_key((unsigned char*)key.GetData(), key.GetSize() * 8, &keyState->key);
    }
}

Crypto_AES::~Crypto_AES()
{
    delete keyState;
}

QStatus Crypto_AES::Encrypt(const Block* in, Block* out, uint32_t numBlocks)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;
    if (!in || !out) {
        return in ? ER_BAD_ARG_1 : ER_BAD_ARG_2;
    }
    /*
     * Check we are initialized for encryption
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

    if (!in || !out) {
        return in ? ER_BAD_ARG_1 : ER_BAD_ARG_2;
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
    size_t partial = len % sizeof(Block);
    if (partial) {
        numBlocks--;
        status = Encrypt((Block*)in, out, numBlocks);
        if (status == ER_OK) {
            Block padBlock;
            memcpy(&padBlock, ((const uint8_t*)in) + numBlocks * sizeof(Block), partial);
            status = Encrypt(&padBlock, out + numBlocks, 1);
        }
    } else {
        status = Encrypt((const Block*)in, out, numBlocks);
    }
    return status;
}

QStatus Crypto_AES::Decrypt(const Block* in, Block* out, uint32_t numBlocks)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;
    if (!in || !out) {
        return in ? ER_BAD_ARG_1 : ER_BAD_ARG_2;
    }
    /*
     * Check we are initialized for decryption
     */
    if (mode != ECB_DECRYPT) {
        return ER_CRYPTO_ERROR;
    }
    while (numBlocks--) {
        AES_decrypt(in->data, out->data, &keyState->key);
        ++in;
        ++out;
    }
    return ER_OK;
}

QStatus Crypto_AES::Decrypt(const Block* in, uint32_t numBlocks, void* out, size_t len)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;
    QStatus status;

    if (!in || !out) {
        return in ? ER_BAD_ARG_1 : ER_BAD_ARG_2;
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
    size_t partial = len % sizeof(Block);
    if (partial) {
        numBlocks--;
        status = Decrypt(in, (Block*)out, numBlocks);
        if (status == ER_OK) {
            Block padBlock;
            status = Decrypt(in + numBlocks, &padBlock, 1);
            memcpy(((uint8_t*)out) + sizeof(Block) * numBlocks, &padBlock, partial);
        }
    } else {
        status = Decrypt(in, (Block*)out, numBlocks);
    }
    return status;
}


static void Compute_CCM_AuthField(AES_KEY* key, Crypto_AES::Block& T, uint8_t M, uint8_t L, const KeyBlob& nonce, const uint8_t* mData, size_t mLen, const uint8_t* addData, size_t addLen)
{
    uint8_t flags = ((addLen) ? 0x40 : 0) | (((M - 2) / 2) << 3) | (L - 1);
    /*
     * Compute the B_0 block. This encodes the flags, the nonce, and the data length.
     */
    Crypto_AES::Block B_0(0);
    B_0.data[0] = flags;
    memset(&B_0.data[1], 0, 15 - L);
    memcpy(&B_0.data[1], nonce.GetData(), min((size_t)15, nonce.GetSize()));
    for (size_t i = 15, l = mLen; l != 0; i--) {
        B_0.data[i] = (uint8_t)(l & 0xFF);
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
    if (addLen) {
        /*
         * This encodes the add data length and the first few octets of the add data
         */
        Crypto_AES::Block A;
        size_t initialLen;
        if (addLen < ((1 << 16) - (1 << 8))) {
            A.data[0] = (uint8_t)(addLen >> 8);
            A.data[1] = (uint8_t)(addLen >> 0);
            initialLen = min(addLen, sizeof(A.data) - 2);
            memcpy(&A.data[2], addData, initialLen);
            A.Pad(16 - initialLen - 2);
        } else {
            A.data[0] = 0xFF;
            A.data[1] = 0xFE;
            A.data[2] = (uint8_t)(addLen >> 24);
            A.data[3] = (uint8_t)(addLen >> 16);
            A.data[4] = (uint8_t)(addLen >> 8);
            A.data[5] = (uint8_t)(addLen >> 0);
            initialLen = sizeof(A.data) - 6;
            memcpy(&A.data[6], addData, initialLen);
        }
        addData += initialLen;
        addLen -= initialLen;
        /*
         * Continue computing the CBC-MAC
         */
        AES_cbc_encrypt(A.data, T.data, sizeof(T.data), key, ivec.data, AES_ENCRYPT);
        Trace("After AES: ", T.data, sizeof(T.data));
        while (addLen >= sizeof(Crypto_AES::Block)) {
            AES_cbc_encrypt(addData, T.data, sizeof(T.data), key, ivec.data, AES_ENCRYPT);
            Trace("After AES: ", T.data, sizeof(T.data));
            addData += sizeof(Crypto_AES::Block);
            addLen -= sizeof(Crypto_AES::Block);
        }
        if (addLen) {
            memcpy(A.data, addData, addLen);
            A.Pad(16 - addLen);
            AES_cbc_encrypt(A.data, T.data, sizeof(T.data), key, ivec.data, AES_ENCRYPT);
            Trace("After AES: ", T.data, sizeof(T.data));
        }

    }
    /*
     * Continue computing CBC-MAC over the message data.
     */
    if (mLen) {
        while (mLen >= sizeof(Crypto_AES::Block)) {
            AES_cbc_encrypt(mData, T.data, sizeof(T.data), key, ivec.data, AES_ENCRYPT);
            Trace("After AES: ", T.data, sizeof(T.data));
            mData += sizeof(Crypto_AES::Block);
            mLen -= sizeof(Crypto_AES::Block);
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

/*
 * Implementation of AES-CCM (Counter with CBC-MAC) as described in RFC 3610
 */
QStatus Crypto_AES::Encrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* addData, size_t addLen, uint8_t authLen)
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
    if (!in && len) {
        return ER_BAD_ARG_1;
    }
    if (!out && len) {
        return ER_BAD_ARG_2;
    }
    if (nLen < 4 || nLen > 14) {
        return ER_BAD_ARG_4;
    }
    if ((authLen < 4) || (authLen > 16)) {
        return ER_BAD_ARG_8;
    }
    const uint8_t L = 15 - (uint8_t)max(nLen, (size_t)11);
    if (L < LengthOctetsFor(len)) {
        return ER_BAD_ARG_3;
    }
    /*
     * Compute the authentication field T.
     */
    Block T;
    Compute_CCM_AuthField(&keyState->key, T, authLen, L, nonce, (uint8_t*)in, len, (uint8_t*)addData, addLen);
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
    AES_ctr128_encrypt((const uint8_t*)in, (uint8_t*)out, len, &keyState->key, ivec.data, ecount_buf.data, &num);
    memcpy((uint8_t*)out + len, U.data, authLen);
    len += authLen;
    return ER_OK;
}


QStatus Crypto_AES::Decrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* addData, size_t addLen, uint8_t authLen)
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
    if (!in) {
        return ER_BAD_ARG_1;
    }
    if (!len || (len < authLen)) {
        return ER_BAD_ARG_3;
    }
    if (nLen < 4 || nLen > 14) {
        return ER_BAD_ARG_4;
    }
    if ((authLen < 4) || (authLen > 16)) {
        return ER_BAD_ARG_8;
    }
    const uint8_t L = 15 - (uint8_t)max(nLen, (size_t)11);
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
    memcpy(U.data, (const uint8_t*)in + len, authLen);
    AES_ctr128_encrypt(U.data, T.data, sizeof(T.data), &keyState->key, ivec.data, ecount_buf.data, &num);
    /*
     * Decrypt message.
     */
    AES_ctr128_encrypt((const uint8_t*)in, (uint8_t*)out, len, &keyState->key, ivec.data, ecount_buf.data, &num);
    /*
     * Compute and verify the authentication field T.
     */
    Block F;
    Compute_CCM_AuthField(&keyState->key, F, authLen, L, nonce, (uint8_t*)out, len, (uint8_t*)addData, addLen);
    if (memcmp(F.data, T.data, authLen) == 0) {
        return ER_OK;
    } else {
        /* Clear the decrypted data */
        memset(out, 0, len + authLen);
        len = 0;
        return ER_AUTH_FAIL;
    }
}


}
