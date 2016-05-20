/**
 * @file CryptoRand.cc
 *
 * Platform-specific secure random number generator
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

#include <Status.h>

#include <CommonCrypto/CommonCrypto.h>
#include <CommonCrypto/CommonRandom.h>

#define QCC_MODULE  "CRYPTO"

QStatus qcc::Crypto_GetRandomBytes(uint8_t* data, size_t len)
{
    QStatus status = ER_OK;
    CCRNGStatus err = CCRandomGenerateBytes(data, len);
    if (err != kCCSuccess) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("CCRandomGenerateBytes failed, code = %d", err));
        return status;
    }
    return status;
}
