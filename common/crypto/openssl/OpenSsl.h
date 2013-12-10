/**
 * @file
 *
 * This file just provides static initialization for the OpenSSL libraries.
 */

/******************************************************************************
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
#ifndef _QCC_OPENSSL_H
#define _QCC_OPENSSL_H

#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

namespace qcc {

/**
 * OpenSSL may not be thread safe so the wrappers must obtain a mutual
 * exclusion lock before making calls into the crypto library. To
 * obtain the lock declare an instance of this class before calling
 * any OpenSSL crypto library APIs.
 *
 * This is a no-op when OpenSSL is compiled with multi-threaded
 * support.
 */
class OpenSsl_ScopedLock {
  public:
    OpenSsl_ScopedLock();
    ~OpenSsl_ScopedLock();
};

static class OpenSslInitializer {
  public:
    OpenSslInitializer();
    ~OpenSslInitializer();
} openSslInitializer;

}

#endif
