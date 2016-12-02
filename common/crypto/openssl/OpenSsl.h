/**
 * @file
 *
 * This file just provides static initialization for the OpenSSL libraries.
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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

}

#endif