/**
 * @file
 *
 * OS specific utility functions
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

#include <windows.h>
#include <bcrypt.h>

#include <qcc/Crypto.h>
#include <qcc/Util.h>

#define QCC_MODULE  "CRYPTO"

QStatus qcc::Crypto_GetRandomBytes(uint8_t* data, size_t len)
{
    if (BCRYPT_SUCCESS(BCryptGenRandom(NULL, data, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG))) {
        return ER_OK;
    } else {
        return ER_FAIL;
    }
}