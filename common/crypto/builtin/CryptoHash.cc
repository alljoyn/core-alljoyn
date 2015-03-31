/**
 * @file Crypto.cc
 *
 * Implementation for methods from Crypto.h
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

#include <assert.h>
#include <ctype.h>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/Util.h>

#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

namespace qcc {

/* External SHA code is only used in this file.
 * Some of the function names are known to conflict with popular open source
 * libraries, so it is best to not pollute the global namespace by including
 * the code within this namespace block.
 *
 * __cplusplus is manipulated to skip 'extern "C"' in the included files.
 */
#undef __cplusplus
#include "sha1.c"
#include "hmac_sha1.c"
#include "sha2.c"
/* Note that __cplusplus cannot be used to detect the supported version of
 * C++ in the remainder of this file.
 */
#define __cplusplus

class Crypto_Hash::Context {
  public:

    Context(Algorithm alg) : algorithm(alg) { }
    Context(const Context& orig) : algorithm(orig.algorithm) {
        memcpy(&sha1, &orig.sha1, sizeof(HMAC_SHA1_CTX));
        memcpy(&sha256, &orig.sha256, sizeof(SHA256_CTX));
    }

    SHA256_CTX sha256;
    uint8_t opad[SHA256_BLOCK_LENGTH];
    union {
        SHA_CTX md;
        HMAC_SHA1_CTX mac;
    } sha1;

    Algorithm algorithm;

  private:
    Context& operator=(const Context& orig);
};

QStatus Crypto_Hash::Init(Algorithm alg, const uint8_t* hmacKey, size_t keyLen)
{
    QStatus status = ER_OK;
    int i;

    if (ctx) {
        delete ctx;
        ctx = NULL;
        initialized = false;
    }

    if (alg == qcc::Crypto_Hash::MD5) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("MD5 not supported"));
        return status;
    }

    MAC = hmacKey != NULL;

    if (MAC && (keyLen == 0)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("HMAC key length cannot be zero"));
        return status;
    }

    ctx = new Crypto_Hash::Context(alg);

    /* No default case - force compile-time error if additional hash types are added */
    switch (alg) {
    case qcc::Crypto_Hash::SHA1:
        if (MAC) {
            HMAC_SHA1_Init(&ctx->sha1.mac);
            HMAC_SHA1_UpdateKey(&ctx->sha1.mac, (uint8_t*) hmacKey, keyLen);
            HMAC_SHA1_EndKey(&ctx->sha1.mac);
            HMAC_SHA1_StartMessage(&ctx->sha1.mac);
        } else {
            SHA1_Init(&ctx->sha1.md);
        }
        break;

    case qcc::Crypto_Hash::SHA256:
        if (MAC) {
            uint8_t ipad[SHA256_BLOCK_LENGTH];
            memset(ipad, 0, SHA256_BLOCK_LENGTH);
            memset(ctx->opad, 0, SHA256_BLOCK_LENGTH);

            if (keyLen > SHA256_BLOCK_LENGTH) {
                uint8_t keyDigest[SHA256_DIGEST_LENGTH];
                SHA256_Init(&ctx->sha256);
                SHA256_Update(&ctx->sha256, hmacKey, keyLen);
                SHA256_Final(keyDigest, &ctx->sha256);
                keyLen = SHA256_DIGEST_LENGTH;
                memcpy(ipad, keyDigest, SHA256_DIGEST_LENGTH);
                memcpy(ctx->opad, keyDigest, SHA256_DIGEST_LENGTH);
            } else {
                memcpy(ipad, hmacKey, keyLen);
                memcpy(ctx->opad, hmacKey, keyLen);
            }

            /* Prepare inner and outer pads */
            for (i = 0; i < SHA256_BLOCK_LENGTH; i++) {
                ipad[i] ^= 0x36;
                ctx->opad[i] ^= 0x5C;
            }

            SHA256_Init(&ctx->sha256);
            SHA256_Update(&ctx->sha256, ipad, SHA256_BLOCK_LENGTH);
            ClearMemory(ipad, SHA256_BLOCK_LENGTH);
        } else {
            SHA256_Init(&ctx->sha256);
        }
        break;

    default:
        return ER_CRYPTO_ERROR;
    }

    if (status == ER_OK) {
        initialized = true;
    } else {
        delete ctx;
        ctx = NULL;
    }

    return status;
}

Crypto_Hash::~Crypto_Hash(void)
{
    if (ctx) {
        /* Zero out keys and other state that are stored in the hash context. */
        ClearMemory(&ctx, sizeof(ctx));
        delete ctx;
    }
}

QStatus Crypto_Hash::Update(const uint8_t* buf, size_t bufSize)
{
    QStatus status = ER_OK;

    if (!initialized) {
        QCC_LogError(status, ("Hash function not initialized"));
        return ER_CRYPTO_HASH_UNINITIALIZED;
    }

    if (!buf) {
        return ER_BAD_ARG_1;
    }

    switch (ctx->algorithm) {
    case qcc::Crypto_Hash::SHA1:
        if (MAC) {
            HMAC_SHA1_UpdateMessage(&ctx->sha1.mac, (uint8_t*)buf, bufSize);
        } else {
            SHA1_Update(&ctx->sha1.md, (uint8_t*)buf, bufSize);
        }
        break;

    case qcc::Crypto_Hash::SHA256:
        SHA256_Update(&ctx->sha256, (uint8_t*)buf, bufSize);
        break;

    default:
        status = ER_CRYPTO_ERROR;
        break;
    }

    return ER_OK;
}

QStatus Crypto_Hash::Update(const qcc::String& str)
{
    return Update((const uint8_t*)str.data(), str.size());
}

QStatus Crypto_Hash::GetDigest(uint8_t* digest, bool keepAlive)
{
    QStatus status = ER_OK;

    if (!initialized) {
        QCC_LogError(status, ("Hash function not initialized"));
        return ER_CRYPTO_HASH_UNINITIALIZED;
    }

    if (!digest) {
        return ER_BAD_ARG_1;
    }

    if (MAC && keepAlive) {
        /* keep alive is not allowed for HMAC */
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Keep alive is not allowed for HMAC"));
        return status;
    }

    switch (ctx->algorithm) {
    case qcc::Crypto_Hash::SHA1:
        if (MAC) {
            HMAC_SHA1_EndMessage(digest, &ctx->sha1.mac);
            HMAC_SHA1_Done(&ctx->sha1.mac);
            initialized = false;
        } else {
            if (keepAlive) {
                SHA_CTX save;
                memcpy(&save, &ctx->sha1.md, sizeof(SHA_CTX));
                SHA1_Final(digest, &ctx->sha1.md);
                memcpy(&ctx->sha1.md, &save, sizeof(SHA_CTX));
            } else {
                SHA1_Final(digest, &ctx->sha1.md);
                initialized = false;
            }
        }
        break;

    case qcc::Crypto_Hash::SHA256:
        if (MAC) {
            // Get inner hash
            SHA256_Final(digest, &ctx->sha256);
            // Get outer hash (we can reuse the context)
            SHA256_Init(&ctx->sha256);
            SHA256_Update(&ctx->sha256, ctx->opad, SHA256_BLOCK_LENGTH);
            SHA256_Update(&ctx->sha256, digest, SHA256_DIGEST_LENGTH);
            SHA256_Final(digest, &ctx->sha256);
            ClearMemory(ctx->opad, SHA256_BLOCK_LENGTH);
            initialized = false;
        } else {
            if (keepAlive) {
                SHA256_CTX save;
                memcpy(&save, &ctx->sha256, sizeof(SHA256_CTX));
                SHA256_Final(digest, &ctx->sha256);
                memcpy(&ctx->sha256, &save, sizeof(SHA256_CTX));
            } else {
                SHA256_Final(digest, &ctx->sha256);
                initialized = false;
            }
        }
        break;

    default:
        status = ER_CRYPTO_ERROR;
        break;
    }

    return status;
}

}
