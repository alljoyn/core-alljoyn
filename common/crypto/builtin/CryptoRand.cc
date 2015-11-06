/**
 * @file
 *
 * Secure random number generator
 *
 * CTR DRBG is implemented using algorithms described in the
 * NIST SP 800-90A standard, which can be found at
 * http://csrc.nist.gov/publications/nistpubs/800-90A/SP800-90A.pdf
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

#include <qcc/Crypto.h>
#include "Crypto.h"
#include <qcc/Util.h>

#include <Status.h>

#include <sys/types.h>

using namespace qcc;

#define QCC_MODULE  "CRYPTO"

class Crypto_DRBG::Context {
  public:
    uint8_t v[Crypto_DRBG::OUTLEN];
    uint8_t k[Crypto_DRBG::KEYLEN];
    uint32_t c;

    Context()
    {
        memset(v, 0, sizeof(v));
        memset(k, 0, sizeof(k));
        c = 0;
    }

    // Clear the sensitive contents of the DRBG state.
    ~Context()
    {
        ClearMemory(v, sizeof(v));
        ClearMemory(k, sizeof(k));
        c = 0;
    }
};

// One instance per application
static Crypto_DRBG* drbgctx = NULL;

static uint32_t PlatformEntropy(uint8_t* data, uint32_t size)
{
    FILE* f = fopen("/dev/urandom", "r");
    if (NULL == f) {
        return 0;
    }
    size = fread(data, sizeof (uint8_t), size, f);
    fclose(f);
    return size;
}

static void Increment(uint8_t* data, size_t size)
{
    while (size--) {
        data[size]++;
        if (data[size]) {
            break;
        }
    }
}

Crypto_DRBG::Crypto_DRBG()
{
    ctx = new Context();
}

Crypto_DRBG::~Crypto_DRBG()
{
    if (ctx) {
        delete ctx;
    }
}

QStatus Crypto_DRBG::Seed(uint8_t* seed, size_t size)
{
    if (SEEDLEN != size) {
        return ER_CRYPTO_ERROR;
    }
    Update(seed);
    ctx->c = 1;
    return ER_OK;
}

QStatus Crypto_DRBG::Generate(uint8_t* rand, size_t size)
{
    uint8_t data[SEEDLEN];
    size_t copy;
    Crypto_AES::Block block;

    if (RESEED_COUNT < ctx->c) {
        /*
         * If counter is at least RESEED_COUNT,
         * attempt seeding. Pass on error, try next call.
         */
        copy = PlatformEntropy(data, sizeof (data));
        if (sizeof (data) == copy) {
            Seed(data, sizeof (data));
        }
    }

    KeyBlob key(ctx->k, KEYLEN, KeyBlob::AES);
    Crypto_AES aes(key, Crypto_AES::ECB_ENCRYPT);
    while (size) {
        Increment(ctx->v, OUTLEN);
        copy = (size < OUTLEN) ? size : OUTLEN;
        aes.Encrypt(ctx->v, OUTLEN, &block, 1);
        memcpy(rand, block.data, copy);
        rand += copy;
        size -= copy;
    }
    memset(data, 0, SEEDLEN);
    Update(data);
    ctx->c++;

    return ER_OK;
}

void Crypto_DRBG::Update(uint8_t* data)
{
    QStatus status;
    size_t i = 0;
    uint8_t tmp[SEEDLEN];
    uint8_t* t = tmp;
    Crypto_AES::Block block;

    KeyBlob key(ctx->k, KEYLEN, KeyBlob::AES);
    Crypto_AES aes(key, Crypto_AES::ECB_ENCRYPT);
    for (i = 0; i < SEEDLEN; i += OUTLEN) {
        Increment(ctx->v, OUTLEN);
        status = aes.Encrypt(ctx->v, OUTLEN, &block, 1);
        if (status != ER_OK) {
            QCC_LogError(status, ("Encryption failed"));
        }
        QCC_ASSERT(ER_OK == status);
        memcpy(t, block.data, OUTLEN);
        t += OUTLEN;
    }

    for (i = 0; i < SEEDLEN; i++) {
        tmp[i] ^= data[i];
    }

    memcpy(ctx->k, tmp, KEYLEN);
    memcpy(ctx->v, tmp + KEYLEN, OUTLEN);
    ClearMemory(tmp, sizeof(tmp));
}

QStatus qcc::Crypto_GetRandomBytes(uint8_t* data, size_t len)
{
    QStatus status = ER_CRYPTO_ERROR;

    if (NULL != data) {
        status = drbgctx->Generate(data, len);
    }

    return status;
}

QStatus Crypto::Init()
{
    uint8_t seed[Crypto_DRBG::SEEDLEN];
    size_t size;

    if (NULL == drbgctx) {
        drbgctx = new Crypto_DRBG;
    }
    /* Initial seeding, fail on error */
    size = PlatformEntropy(seed, sizeof (seed));
    if (sizeof (seed) == size) {
        drbgctx->Seed(seed, sizeof (seed));
    } else {
        QCC_DbgHLPrintf(("Low entropy: %" PRIuSIZET " (requested %" PRIuSIZET ")\n", size, sizeof (seed)));
        return ER_CRYPTO_ERROR;
    }

    ClearMemory(seed, sizeof (seed));
    return ER_OK;
}

void Crypto::Shutdown() {
    delete drbgctx;
    drbgctx = NULL;
}
