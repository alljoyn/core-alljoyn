/**
 * @file CryptoHash.cc
 *
 * Implementation for methods from CryptoHash.h
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

#include <ctype.h>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/Util.h>

#include <Status.h>
#include "CommonCrypto.h"

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

namespace qcc {

class Crypto_Hash::Context {
  public:

    Context(bool MAC, Algorithm algorithm) : MAC(MAC), algorithm(algorithm) { }

    /// Union of context storage for HMAC or MD.
    union {
        union {
            CC_SHA1_CTX sha1;       ///< Storage for the SHA1 MD context.
            CC_SHA256_CTX sha256;   ///< Storage for the SHA256 MD context.
        } md;
        CCHmacContext hmac;         ///< Storage for the HMAC context.
        uint8_t pad[512];           ///< Ensure we have enough storage
    };
    bool MAC;
    Algorithm algorithm;

};

QStatus Crypto_Hash::Init(Algorithm alg, const uint8_t* hmacKey, size_t keyLen)
{
    /*
     * Protect the open ssl APIs.
     */
    CommonCrypto_ScopedLock lock;

    QStatus status = ER_OK;

    if (ctx) {
        delete ctx;
        ctx = NULL;
        initialized = false;
    }

    MAC = hmacKey != NULL;

    if (MAC && (keyLen == 0)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("HMAC key length cannot be zero"));
        delete ctx;
        ctx = NULL;
        return status;
    }

    /*
     * Select the hash algorithm.
     * No memory allocation/initialization is done; OK to return.
     */
    switch (alg) {
    case qcc::Crypto_Hash::SHA1:
        ctx = new Crypto_Hash::Context(MAC, alg);
        if (MAC) {
            CCHmacInit(&ctx->hmac, kCCHmacAlgSHA1, hmacKey, keyLen);
        } else {
            CC_SHA1_Init(&ctx->md.sha1);
        }
        initialized = true;
        break;

    case qcc::Crypto_Hash::SHA256:
        ctx = new Crypto_Hash::Context(MAC, alg);
        if (MAC) {
            CCHmacInit(&ctx->hmac, kCCHmacAlgSHA256, hmacKey, keyLen);
        } else {
            CC_SHA256_Init(&ctx->md.sha256);
        }
        initialized = true;
        break;

    default:
        status = ER_BAD_ARG_1;
        QCC_LogError(status, ("Unsupported hash algorithm %d", alg));
        return status;
    }

    return status;
}

Crypto_Hash::~Crypto_Hash(void)
{
    /*
     * Protect the open ssl APIs.
     */
    CommonCrypto_ScopedLock lock;

    if (ctx) {
        delete ctx;
    }
}

QStatus Crypto_Hash::Update(const uint8_t* buf, size_t bufSize)
{
    /*
     * Protect the open ssl APIs.
     */
    CommonCrypto_ScopedLock lock;

    QStatus status = ER_OK;

    if (!buf) {
        return ER_BAD_ARG_1;
    }
    if (initialized) {
        if (MAC) {
            CCHmacUpdate(&ctx->hmac, buf, bufSize);
        } else if (ctx->algorithm == qcc::Crypto_Hash::SHA1)   {
            CC_SHA1_Update(&ctx->md.sha1, buf, bufSize);
        } else if (ctx->algorithm == qcc::Crypto_Hash::SHA256)   {
            CC_SHA256_Update(&ctx->md.sha256, buf, bufSize);
        } else   {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Updating hash digest"));
        }
    } else {
        status = ER_CRYPTO_HASH_UNINITIALIZED;
        QCC_LogError(status, ("Hash function not initialized"));
    }
    return status;
}

QStatus Crypto_Hash::Update(const qcc::String& str)
{
    return Update((const uint8_t*)str.data(), str.size());
}

QStatus Crypto_Hash::Update(const vector<uint8_t, SecureAllocator<uint8_t> >& d)
{
    return Update(d.data(), d.size());
}

QStatus Crypto_Hash::GetDigest(uint8_t* digest, bool keepAlive)
{
    /*
     * Protect the open ssl APIs.
     */
    CommonCrypto_ScopedLock lock;

    QStatus status = ER_OK;

    if (!digest) {
        return ER_BAD_ARG_1;
    }
    if (initialized) {
        if (MAC) {
            /* keep alive is not allowed for HMAC */
            if (keepAlive) {
                status = ER_CRYPTO_ERROR;
                QCC_LogError(status, ("Keep alive is not allowed for HMAC"));
                keepAlive = false;
            }
            CCHmacFinal(&ctx->hmac, digest);
            initialized = false;
        } else {
            Context* keep = NULL;
            /* To keep the hash alive we need to copy the context before calling EVP_DigestFinal */
            if (keepAlive) {
                keep = new Context(false, ctx->algorithm);
                memcpy(&keep->md, &ctx->md, sizeof(ctx->md));
            }
            if (ctx->algorithm == qcc::Crypto_Hash::SHA1) {
                CC_SHA1_Final(digest, &ctx->md.sha1);
            } else if (ctx->algorithm == qcc::Crypto_Hash::SHA256)   {
                CC_SHA256_Final(digest, &ctx->md.sha256);
            } else   {
                status = ER_CRYPTO_ERROR;
                QCC_LogError(status, ("Finalizing hash digest"));
            }
            if (keep) {
                delete ctx;
                ctx = keep;
            } else {
                initialized = false;
            }
        }
    } else {
        status = ER_CRYPTO_HASH_UNINITIALIZED;
        QCC_LogError(status, ("Hash function not initialized"));
    }
    return status;
}

}
