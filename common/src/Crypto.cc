/**
 * @file Crypto.cc
 *
 * Implementation for methods from Crypto.h
 */

/******************************************************************************
 * Copyright (c) 2009-2013, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>

#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

namespace qcc {

QStatus Crypto_PseudorandomFunctionCCM(const KeyBlob& secret, const char* label, const qcc::String& seed, uint8_t* out, size_t outLen)
{
    QStatus status = ER_OK;
    uint8_t counter[4] = { 0, 0, 0, 0 };
    String in((char*)secret.GetData(), secret.GetSize());

    if (!label) {
        return ER_BAD_ARG_2;
    }
    if (!out) {
        return ER_BAD_ARG_4;
    }
    /*
     * Concatenate the seed with the the secret and label
     */
    in += seed;
    in += label;
    /*
     * Construct AES key from the first 16 bytes of the input.
     */
    Crypto_AES aes(KeyBlob((uint8_t*)in.data(), 16, KeyBlob::AES), Crypto_AES::CCM);

    while (outLen) {
        uint8_t mac[16];
        size_t mlen = 0;
        size_t len =  (std::min)((size_t)16, outLen);
        KeyBlob nonce(counter, sizeof(counter), KeyBlob::GENERIC);
        status = aes.Encrypt_CCM(NULL, mac, mlen, nonce, in.data() + 16, in.size() - 16, 16);
        memcpy(out, mac, len);
        outLen -= len;
        out += len;
        ++counter[0];
    }
    in.secure_clear();
    return status;
}

QStatus Crypto_PseudorandomFunction(const KeyBlob& secret, const char* label, const qcc::String& seed, uint8_t* out, size_t outLen)
{
    if (!label) {
        return ER_BAD_ARG_2;
    }
    if (!out) {
        return ER_BAD_ARG_4;
    }
    Crypto_SHA256 hash;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    size_t len = 0;

    while (outLen) {
        /*
         * Initialize SHA256 in HMAC mode with the secret
         */
        hash.Init(secret.GetData(), secret.GetSize());
        /*
         * If this is not the first iteration hash in the digest from the previous iteration.
         */
        if (len) {
            hash.Update(digest, sizeof(digest));
        }
        hash.Update((const uint8_t*)label, strlen(label));
        hash.Update((const uint8_t*)seed.data(), seed.size());
        hash.GetDigest(digest);
        len =  (std::min)(sizeof(digest), outLen);
        memcpy(out, digest, len);
        outLen -= len;
        out += len;
    }
    return ER_OK;
}

}
