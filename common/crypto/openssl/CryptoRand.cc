/**
 * @file
 *
 * Platform-specific secure random number generator
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

#include <qcc/platform.h>

#include <qcc/Crypto.h>

#include <Status.h>
#include "OpenSsl.h"

#define QCC_MODULE  "CRYPTO"

QStatus qcc::Crypto_GetRandomBytes(uint8_t* data, size_t len)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    QStatus status = ER_OK;
    BIGNUM* rand = BN_new();
    if (!rand) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("Failed to allocate memory for EVP PKEY"));
        return status;
    }
    if (BN_rand(rand, len * 8, -1, 0)) {
        BN_bn2bin(rand, data);
    } else {
        status = ER_CRYPTO_ERROR;
    }
    BN_free(rand);
    return status;
}