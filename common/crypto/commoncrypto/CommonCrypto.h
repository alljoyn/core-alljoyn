/**
 * @file CommonCrypto.h
 *
 * This file just provides static initialization for the CommonCrypto APIs.
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
#ifndef _QCC_COMMONCRYPTO_H
#define _QCC_COMMONCRYPTO_H

#include <CommonCrypto/CommonCrypto.h>
#include <CommonCrypto/CommonCryptoError.h>
#include <CommonCrypto/CommonCryptor.h>
#include <CommonCrypto/CommonHMAC.h>
#include <CommonCrypto/CommonRandom.h>
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonKeyDerivation.h>
#include <CommonCrypto/CommonSymmetricKeywrap.h>

namespace qcc {

/**
 * The CommonCrypto cryptor is not thread safe so the wrappers
 * must obtain a mutual exclusion lock before making calls into
 * the library. To obtain the lock declare an instance of this
 * class before calling any CommonCrypto APIs.
 */
class CommonCrypto_ScopedLock {
  public:
    CommonCrypto_ScopedLock();
    ~CommonCrypto_ScopedLock();
};

}

#endif
