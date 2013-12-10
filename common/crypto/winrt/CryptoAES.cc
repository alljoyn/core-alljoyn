/**
 * @file CryptoAES.cc
 *
 * Class for AES block encryption/decryption this wraps the WinRT APIs
 */

/******************************************************************************
 * Copyright (c) 2012 AllSeen Alliance. All rights reserved.
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
#include <algorithm>

#include <qcc/platform.h>

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Crypto.h>
#include <qcc/KeyBlob.h>

#include <qcc/winrt/utility.h>

#include <Status.h>

using namespace qcc;
using namespace Windows::Storage::Streams;
using namespace Windows::Security::Cryptography;
using namespace Windows::Security::Cryptography::Core;

#define QCC_MODULE "CRYPTO"

static IBuffer ^ ivec0 = CryptographicBuffer::DecodeFromHexString("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");

static inline void ByteCopy(Platform::WriteOnlyArray<uint8>^ in, size_t offset, void* out, size_t len)
{
    memcpy(out, in->Data + offset, len);
}

namespace qcc {

class Crypto_AES::KeyState {
  public:

    KeyState(CryptographicKey ^ ecbKey, CryptographicKey ^ cbcKey) : ecbKey(ecbKey), cbcKey(cbcKey) { }

    CryptographicKey ^ ecbKey;
    CryptographicKey ^ cbcKey;

    QStatus Encrypt_CTR(const uint8_t* in, uint8_t* out, size_t len, Crypto_AES::Block& ivec, uint32_t& counter);

    QStatus Compute_CCM_AuthField(Crypto_AES::Block& T, uint8_t M, uint8_t L, const KeyBlob& nonce, const uint8_t* mData, size_t mLen, const uint8_t* addData, size_t addLen);

};

QStatus Crypto_AES::KeyState::Encrypt_CTR(const uint8_t* in, uint8_t* out, size_t len, Crypto_AES::Block& ivec, uint32_t& counter)
{
    QStatus status = ER_OK;
    try {
        Platform::ArrayReference<uint8> iv((uint8*)ivec.data, 16);
        while (len) {
            ivec.data[15] = counter;
            ivec.data[14] = counter >> 8;
            IBuffer ^ buf = CryptographicEngine::Encrypt(ecbKey,  CryptographicBuffer::CreateFromByteArray(iv), nullptr);
            Platform::Array<uint8>^ enc;
            CryptographicBuffer::CopyToByteArray(buf, &enc);
            uint8_t* p = enc->Data;
            size_t n = std::min(len, (size_t)16);
            len -= n;
            while (n--) {
                *out++ = *p++ ^ *in++;
            }
            ++counter;
        }
    } catch (...) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to encrypt"));
    }
    return status;
}

QStatus Crypto_AES::KeyState::Compute_CCM_AuthField(Crypto_AES::Block& T, uint8_t M, uint8_t L, const KeyBlob& nonce, const uint8_t* mData, size_t mLen, const uint8_t* addData, size_t addLen)
{
    QStatus status = ER_OK;
    uint8_t flags = ((addLen) ? 0x40 : 0) | (((M - 2) / 2) << 3) | (L - 1);
    size_t inLen;
    uint8_t* p;
    uint8_t* inBuf = NULL;
    size_t bufLen;
    /*
     * Compute the B_0 block. This encodes the flags, the nonce, and the data length.
     */
    Crypto_AES::Block B_0(0);
    B_0.data[0] = flags;
    memset(&B_0.data[1], 0, 15 - L);
    memcpy(&B_0.data[1], nonce.GetData(), nonce.GetSize());
    for (size_t i = 15, l = mLen; l != 0; i--) {
        B_0.data[i] = (uint8_t)(l & 0xFF);
        l >>= 8;
    }
    /*
     * Accumulate data into the input array
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
            initialLen = std::min(addLen, sizeof(A.data) - 2);
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
         * We now know the total length
         */
        bufLen = sizeof(B_0.data) + sizeof(A.data) + ((addLen + 15) & ~0xF) + ((mLen + 15) & ~0xF);
        p = inBuf = new uint8_t[bufLen];

        memcpy(p, &B_0.data, sizeof(B_0.data));
        p += sizeof(B_0.data);
        memcpy(p, &A.data, sizeof(A.data));
        p += sizeof(A.data);
        memcpy(p, addData, addLen);
        p += addLen;
        /* Zero-pad to next block boundary */
        if (addLen & 0xF) {
            size_t pad = 16 - (addLen & 0xF);
            memset(p, 0, pad);
            p += pad;
        }
    } else {
        bufLen = sizeof(B_0.data) + ((mLen + 15) & ~0xF);
        p = inBuf = new uint8_t[bufLen];
        memcpy(p, &B_0.data, sizeof(B_0.data));
        p += sizeof(B_0.data);
    }
    memcpy(p, mData, mLen);
    p += mLen;
    /* Zero-pad to next block boundary */
    if (mLen & 0xF) {
        size_t pad = 16 - (mLen & 0xF);
        memset(p, 0, pad);
    }
    assert(ivec0->Length == 16);
    /*
     * Now compute the CBC-MAC over the entire input buffer. We are only interested
     * in the final block in the output.
     */
    try {
        Platform::ArrayReference<uint8> in((uint8*)inBuf, bufLen);
        IBuffer ^ out = CryptographicEngine::Encrypt(cbcKey, CryptographicBuffer::CreateFromByteArray(in), ivec0);
        Platform::Array<uint8>^ outBytes;
        CryptographicBuffer::CopyToByteArray(out, &outBytes);
        ByteCopy(outBytes, outBytes->Length - 16, T.data, sizeof(T.data));
    } catch (...) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to encrypt"));
    }
    delete [] inBuf;
    return status;
}

/*
 * Cached algoritm providers so we don't need to keep looking these up.
 */
static SymmetricKeyAlgorithmProvider ^ ecbHandle;
static SymmetricKeyAlgorithmProvider ^ cbcHandle;


Crypto_AES::Crypto_AES(const KeyBlob& key, Mode mode) : mode(mode), keyState(NULL)
{
    QStatus status;
    Platform::ArrayReference<uint8> inKey((uint8*)key.GetData(), key.GetSize());
    IBuffer ^ keyBuf = CryptographicBuffer::CreateFromByteArray(inKey);
    CryptographicKey ^ cbcKey;

    if (mode == CCM) {
        if (cbcHandle == nullptr) {
            cbcHandle = SymmetricKeyAlgorithmProvider::OpenAlgorithm("AES_CBC");
            if (cbcHandle == nullptr) {
                status = ER_CRYPTO_ERROR;
                QCC_LogError(status, ("Failed to open AES CBC algorithm provider"));
                return;
            }
        }
        cbcKey = cbcHandle->CreateSymmetricKey(keyBuf);
    }
    if (ecbHandle == nullptr) {
        ecbHandle = SymmetricKeyAlgorithmProvider::OpenAlgorithm("AES_ECB");
        if (ecbHandle == nullptr) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to open AES ECB algorithm provider"));
            return;
        }
    }
    keyState = new KeyState(ecbHandle->CreateSymmetricKey(keyBuf), cbcKey);
}

Crypto_AES::~Crypto_AES()
{
    delete keyState;
}

QStatus Crypto_AES::Encrypt(const Block* in, Block* out, uint32_t numBlocks)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_AES::Encrypt(const void* in, size_t len, Block* out, uint32_t numBlocks)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_AES::Decrypt(const Block* in, Block* out, uint32_t numBlocks)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_AES::Decrypt(const Block* in, uint32_t numBlocks, void* out, size_t len)
{
    return ER_NOT_IMPLEMENTED;
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

QStatus Crypto_AES::Encrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* addData, size_t addLen, uint8_t authLen)
{
    QStatus status = ER_OK;
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
    const uint8_t L = 15 - (uint8_t) std::max(nLen, (size_t)11);
    if (L < LengthOctetsFor(len)) {
        return ER_BAD_ARG_3;
    }
    /*
     * Compute the authentication field T.
     */
    Block T;
    status = keyState->Compute_CCM_AuthField(T, authLen, L, nonce, (uint8_t*)in, len, (uint8_t*)addData, addLen);
    if (status == ER_OK) {
        /*
         * Initialize ivec and other initial args.
         */
        Block U;
        Block ivec(0);
        ivec.data[0] = (L - 1);
        memcpy(&ivec.data[1], nonce.GetData(), nLen);
        unsigned int num = 0;
        status = keyState->Encrypt_CTR(T.data, U.data, 16, ivec, num);
        if (status == ER_OK) {
            status = keyState->Encrypt_CTR((const uint8_t*)in, (uint8_t*)out, len, ivec, num);
        }
        if (status == ER_OK) {
            memcpy((uint8_t*)out + len, U.data, authLen);
            len += authLen;
        }
    }
    return status;
}

QStatus Crypto_AES::Decrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* addData, size_t addLen, uint8_t authLen)
{
    QStatus status = ER_OK;
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
    const uint8_t L = 15 - (uint8_t) std::max(nLen, (size_t)11);
    if (L < LengthOctetsFor(len)) {
        return ER_BAD_ARG_3;
    }
    /*
     * Initialize ivec and other initial args.
     */
    Block ivec(0);
    ivec.data[0] = (L - 1);
    memcpy(&ivec.data[1], nonce.GetData(), nLen);
    uint32_t num = 0;
    /*
     * Decrypt the authentication field
     */
    Block U;
    Block T;
    len = len - authLen;
    memcpy(U.data, (const uint8_t*)in + len, authLen);
    status = keyState->Encrypt_CTR(U.data, T.data, sizeof(T.data), ivec, num);
    if (status == ER_OK) {
        /*
         * Decrypt message.
         */
        status = keyState->Encrypt_CTR((const uint8_t*)in, (uint8_t*)out, len, ivec, num);
    }
    if (status == ER_OK) {
        /*
         * Compute and verify the authentication field T.
         */
        Block F;
        status = keyState->Compute_CCM_AuthField(F, authLen, L, nonce, (uint8_t*)in, len, (uint8_t*)addData, addLen);
        if (memcmp(F.data, T.data, authLen) != 0) {
            /* Clear the decrypted data */
            memset(out, 0, len + authLen);
            len = 0;
            status = ER_AUTH_FAIL;
        }
    }
    return status;
}

}
