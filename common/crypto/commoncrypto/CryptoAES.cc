/**
 * @file CryptoAES.cc
 *
 * Class for AES block encryption/decryption
 */

/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <qcc/platform.h>

#include <algorithm>
#include <ctype.h>
/* due to a change in gcc6, cmath must be included now */
#if defined(__GNUC__) && (__GNUC__ >= 6)
 #include <cmath>
#else
 #include <math.h>
#endif

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Crypto.h>
#include <qcc/KeyBlob.h>

#include <Status.h>

#include <CommonCrypto/CommonCrypto.h>

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

/* declaration of CryptoAES_BLOCK_LEN */
extern const size_t Crypto_AES::BLOCK_LEN;

struct Crypto_AES::KeyState {
    KeyBlob* key;
};

Crypto_AES::Crypto_AES(const KeyBlob& key, Mode mode) : mode(mode), keyState(new KeyState())
{
    keyState->key = new KeyBlob(key);
}

Crypto_AES::~Crypto_AES()
{
    delete keyState->key;
    delete keyState;
}

QStatus Crypto_AES::Encrypt(const Block* in, Block* out, uint32_t numBlocks)
{
    if (in == nullptr) {
        return ER_BAD_ARG_1;
    }
    if (out == nullptr) {
        return ER_BAD_ARG_2;
    }
    /*
     * Check if we are initialized for encryption
     */
    if (mode != ECB_ENCRYPT) {
        return ER_CRYPTO_ERROR;
    }
    size_t dataMoved;
    while (numBlocks--) {
        CCCryptorStatus status;
        status = CCCrypt(kCCEncrypt, kCCAlgorithmAES128, kCCOptionECBMode,
                         keyState->key->GetData(), keyState->key->GetSize(), nullptr,
                         in->data, sizeof(in->data), out->data, sizeof(out->data), &dataMoved);
        if (status != kCCSuccess) {
            QCC_LogError(ER_CRYPTO_ERROR, ("CCCrypt failed, code = %d", status));
            return ER_CRYPTO_ERROR;
        }
        ++in;
        ++out;
    }
    return ER_OK;
}

QStatus Crypto_AES::Encrypt(const void* in, size_t len, Block* out, uint32_t numBlocks)
{
    if (in == nullptr) {
        return ER_BAD_ARG_1;
    }
    if (out == nullptr) {
        return ER_BAD_ARG_3;
    }
    /*
     * Check if the lengths make sense
     */
    if (numBlocks != NumBlocks(len)) {
        return ER_CRYPTO_ERROR;
    }

    QStatus status;
    /*
     * Check for a partial final block
     */
    size_t partial = len %  Crypto_AES::BLOCK_LEN;
    Block inBlock(in, 16);
    if (partial) {
        --numBlocks;
        status = Encrypt(&inBlock, out, numBlocks);
        if (status == ER_OK) {
            Block padBlock;
            memcpy(padBlock.data, static_cast<const uint8_t*>(in) + (numBlocks * Crypto_AES::BLOCK_LEN), partial);
            status = Encrypt(&padBlock, out + numBlocks, 1);
        }
    } else {
        status = Encrypt(&inBlock, out, numBlocks);
    }
    return status;
}

/*
 * This helper function calls CCCryptorCreateWithMode and handles errors
 */
static QStatus CryptorCreateWithMode(CCOperation op, CCMode mode, CCAlgorithm alg, CCPadding padding, const void* iv, const void* key, size_t keyLength, const void* tweak, size_t tweakLength, int numRounds, CCModeOptions options, CCCryptorRef* cryptorRef)
{
    QStatus status = ER_OK;

    CCCryptorStatus cryptorStatus = CCCryptorCreateWithMode(op, mode, alg, padding, iv, key, keyLength, tweak, tweakLength, numRounds, options, cryptorRef);
    if (cryptorStatus != kCCSuccess) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("CCCryptorCreateWithMode failed, code = %d", cryptorStatus));
    }

    return status;
}

/*
 * This helper function calls CCCryptorUpdate and handles errors, releases the cryptor if an error occurs
 */
static QStatus CryptorUpdate(CCCryptorRef& cryptor, const void* dataIn, size_t dataInLength, void* dataOut, size_t dataOutAvailable, size_t* dataOutMoved)
{
    QStatus status = ER_OK;

    CCCryptorStatus cryptorStatus = CCCryptorUpdate(cryptor, dataIn, dataInLength, dataOut, dataOutAvailable, dataOutMoved);
    if (cryptorStatus != kCCSuccess) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("CCCryptorUpdate failed, code = %d", cryptorStatus));
        CCCryptorRelease(cryptor);
        cryptor = nullptr;
    }

    return status;
}

/*
 * This helper function calls CCCryptorFinal and handles errors, releases the cryptor if an error occurs
 */
static QStatus CryptorFinal(CCCryptorRef& cryptor, void* dataOut, size_t dataOutAvailable, size_t* dataOutMoved)
{
    QStatus status = ER_OK;

    CCCryptorStatus cryptorStatus = CCCryptorFinal(cryptor, dataOut, dataOutAvailable, dataOutMoved);
    if (cryptorStatus != kCCSuccess) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("CCCryptorFinal failed, code = %d", cryptorStatus));
        CCCryptorRelease(cryptor);
        cryptor = nullptr;
    }

    return status;
}

static QStatus Compute_CCM_AuthField(KeyBlob* key, Crypto_AES::Block& T, uint8_t M, uint8_t L,
                                     const KeyBlob& nonce, const uint8_t* mData, size_t mLen,
                                     const uint8_t* addData, size_t addLen)
{
    uint8_t flags = ((addLen) ? 0x40 : 0) | (((M - 2) / 2) << 3) | (L - 1);
    /*
     * Compute the B_0 block. This encodes the flags, the nonce, and the data length.
     */
    Crypto_AES::Block B_0(0);
    B_0.data[0] = flags;
    memset(&B_0.data[1], 0, 15 - L);
    memcpy(&B_0.data[1], nonce.GetData(), min(size_t(15), nonce.GetSize()));
    for (size_t i = 15, l = mLen; l != 0; i--) {
        B_0.data[i] = static_cast<uint8_t>(l & 0xFF);
        l >>= 8;
    }
    /*
     * Initialize CBC-MAC with B_0 initialization vector of 0s.
     */
    Crypto_AES::Block ivec(0);
    /*
     * Set up the cryptor
     * CryptorUpdate and CryptorFinal release the cryptor if an error occurs
     */
    size_t dataMoved;
    CCCryptorRef cryptor;
    QStatus status = CryptorCreateWithMode(kCCEncrypt, kCCModeCBC, kCCAlgorithmAES128, ccNoPadding,
                                           ivec.data,  key->GetData(), key->GetSize(), nullptr, 0, 0, 0, &cryptor);
    if (status != ER_OK) {
        return status;
    }

    Trace("CBC IV in: ", B_0.data, sizeof(B_0.data));
    status = CryptorUpdate(cryptor, B_0.data, kCCBlockSizeAES128, T.data, kCCBlockSizeAES128, &dataMoved);
    if (status != ER_OK) {
        return status;
    }
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
            A.data[0] = static_cast<uint8_t>(addLen >> 8);
            A.data[1] = static_cast<uint8_t>(addLen >> 0);
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
        status = CryptorUpdate(cryptor, A.data, kCCBlockSizeAES128, T.data, kCCBlockSizeAES128, &dataMoved);
        if (status != ER_OK) {
            return status;
        }
        Trace("After AES: ", T.data, sizeof(T.data));
        while (addLen >= Crypto_AES::BLOCK_LEN) {
            status = CryptorUpdate(cryptor, addData, kCCBlockSizeAES128, T.data, kCCBlockSizeAES128, &dataMoved);
            if (status != ER_OK) {
                return status;
            }
            Trace("After AES: ", T.data, sizeof(T.data));
            addData += kCCBlockSizeAES128;
            addLen -= kCCBlockSizeAES128;
        }
        if (addLen) {
            memcpy(A.data, addData, addLen);
            A.Pad(16 - addLen);
            status = CryptorUpdate(cryptor, A.data, kCCBlockSizeAES128, T.data, kCCBlockSizeAES128, &dataMoved);
            if (status != ER_OK) {
                return status;
            }
            Trace("After AES: ", T.data, sizeof(T.data));
        }
    }
    /*
     * Continue computing CBC-MAC over the message data.
     */
    if (mLen) {
        while (mLen >= Crypto_AES::BLOCK_LEN) {
            status = CryptorUpdate(cryptor, mData, kCCBlockSizeAES128, T.data, kCCBlockSizeAES128, &dataMoved);
            if (status != ER_OK) {
                return status;
            }
            Trace("After AES: ", T.data, sizeof(T.data));
            mData += kCCBlockSizeAES128;
            mLen -= kCCBlockSizeAES128;
        }
        if (mLen) {
            Crypto_AES::Block final;
            memcpy(final.data, mData, mLen);
            final.Pad(16 - mLen);
            status = CryptorUpdate(cryptor, final.data, kCCBlockSizeAES128, T.data, kCCBlockSizeAES128, &dataMoved);
            if (status != ER_OK) {
                return status;
            }
            Trace("After AES: ", T.data, sizeof(T.data));
        }
    }
    status = CryptorFinal(cryptor, T.data, kCCBlockSizeAES128, &dataMoved);
    if (status != ER_OK) {
        return status;
    }
    CCCryptorRelease(cryptor);
    Trace("CBC-MAC:   ", T.data, M);

    return ER_OK;
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
     * Check if we are initialized for CCM
     */
    if (mode != CCM) {
        return ER_CRYPTO_ERROR;
    }
    if (len != 0) {
        if (in == nullptr) {
            return ER_BAD_ARG_1;
        }
        if (out == nullptr) {
            return ER_BAD_ARG_2;
        }
    }

    size_t nLen = nonce.GetSize();
    if (nLen < 4 || nLen > 14) {
        return ER_BAD_ARG_4;
    }
    if ((authLen < 4) || (authLen > 16)) {
        return ER_BAD_ARG_7;
    }
    const uint8_t L = 15 - static_cast<uint8_t>(max(nLen, size_t(11)));
    if (L < LengthOctetsFor(len)) {
        return ER_BAD_ARG_3;
    }
    /*
     * Compute the authentication field T.
     */
    Block T;
    QStatus status = Compute_CCM_AuthField(keyState->key, T, authLen, L, nonce, static_cast<const uint8_t*>(in), len, static_cast<const uint8_t*>(addData), addLen);
    if (status != ER_OK) {
        return status;
    }
    /*
     * Initialize ivec and other initial args.
     */
    Block ivec(0);
    ivec.data[0] = (L - 1);
    memcpy(&ivec.data[1], nonce.GetData(), nLen);
    /*
     * Set up the cryptor
     * CryptorUpdate and CryptorFinal release the cryptor if an error occurs
     */
    CCCryptorRef cryptor;
    size_t dataMoved;
    status = CryptorCreateWithMode(kCCEncrypt, kCCModeCTR, kCCAlgorithmAES128, ccNoPadding,
                                   ivec.data,  keyState->key->GetData(), keyState->key->GetSize(),
                                   nullptr, 0, 0, kCCModeOptionCTR_BE, &cryptor);
    if (status != ER_OK) {
        return status;
    }
    /*
     * Encrypt the authentication field
     */
    Block U;
    status = CryptorUpdate(cryptor, T.data, kCCBlockSizeAES128, U.data, kCCBlockSizeAES128, &dataMoved);
    if (status != ER_OK) {
        return status;
    }
    Trace("CTR Start: ", ivec.data, 16);

    status = CryptorUpdate(cryptor, in, len, out, len, &dataMoved);
    if (status != ER_OK) {
        return status;
    }

    status = CryptorFinal(cryptor, out, len, &dataMoved);
    if (status != ER_OK) {
        return status;
    }
    CCCryptorRelease(cryptor);

    memcpy((uint8_t*)out + len, U.data, authLen);
    len += authLen;
    return ER_OK;
}

QStatus Crypto_AES::Decrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* addData, size_t addLen, uint8_t authLen)
{
    /*
     * Check if we are initialized for CCM
     */
    if (mode != CCM) {
        return ER_CRYPTO_ERROR;
    }
    size_t nLen = nonce.GetSize();
    if (in == nullptr) {
        return ER_BAD_ARG_1;
    }
    if ((len == 0) || (len < authLen)) {
        return ER_BAD_ARG_3;
    }
    if (nLen < 4 || nLen > 14) {
        return ER_BAD_ARG_4;
    }
    if ((authLen < 4) || (authLen > 16)) {
        return ER_BAD_ARG_7;
    }
    const uint8_t L = 15 - (uint8_t)max(nLen, size_t(11));
    if (L < LengthOctetsFor(len)) {
        return ER_BAD_ARG_3;
    }
    /*
     * Initialize ivec.
     */
    Block ivec(0);
    ivec.data[0] = (L - 1);
    memcpy(&ivec.data[1], nonce.GetData(), nLen);
    /*
     * Set up the cryptor
     * CryptorUpdate and CryptorFinal release the cryptor if an error occurs
     */
    size_t dataMoved;
    CCCryptorRef cryptor;
    QStatus status = CryptorCreateWithMode(kCCEncrypt, kCCModeCTR, kCCAlgorithmAES128, ccNoPadding,
                                           ivec.data,  keyState->key->GetData(), keyState->key->GetSize(),
                                           nullptr, 0, 0, kCCModeOptionCTR_BE, &cryptor);
    if (status != ER_OK) {
        return status;
    }
    /*
     * Decrypt the authentication field
     */
    Block U;
    Block T;
    len = len - authLen;
    memcpy(U.data, static_cast<const uint8_t*>(in) + len, authLen);
    status = CryptorUpdate(cryptor, U.data, sizeof(T.data), T.data, sizeof(T.data), &dataMoved);
    if (status != ER_OK) {
        return status;
    }
    /*
     * Decrypt message.
     */
    status = CryptorUpdate(cryptor, in, len, out, len, &dataMoved);
    if (status != ER_OK) {
        return status;
    }

    status = CryptorFinal(cryptor, out, len, &dataMoved);
    if (status != ER_OK) {
        return status;
    }
    CCCryptorRelease(cryptor);
    /*
     * Compute and verify the authentication field T.
     */
    Block F;
    status = Compute_CCM_AuthField(keyState->key, F, authLen, L, nonce, static_cast<uint8_t*>(out), len, static_cast<const uint8_t*>(addData), addLen);
    if (status != ER_OK) {
        return status;
    }

    if (Crypto_Compare(F.data, T.data, authLen) != 0) {
        /* Clear the decrypted data */
        memset(out, 0, len + authLen);
        len = 0;
        return ER_CRYPTO_ERROR;
    }
    return ER_OK;
}
}
