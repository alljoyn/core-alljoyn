/**
 * @file
 *
 * This file implements CngCache object for managing lifetime of global CNG handles.
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
#include <windows.h>
#include <bcrypt.h>
#include <capi.h>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Util.h>
#include <qcc/Crypto.h>
#include <qcc/CngCache.h>
#include "Crypto.h"

#define QCC_MODULE "CRYPTO"

namespace qcc {

CngCache::CngCache() : ccmHandle(NULL), ecbHandle(NULL)
{
    assert(sizeof(algHandles) == (sizeof(BCRYPT_ALG_HANDLE) * ALGORITHM_COUNT * 2));
    memset(&algHandles, 0, sizeof(algHandles));
    assert(sizeof(ecdsaHandles) == (sizeof(BCRYPT_ALG_HANDLE) * ECDSA_ALGORITHM_COUNT));
    memset(&ecdsaHandles, 0, sizeof(ecdsaHandles));
    assert(sizeof(ecdhHandles) == (sizeof(BCRYPT_ALG_HANDLE) * ECDH_ALGORITHM_COUNT));
    memset(&ecdhHandles, 0, sizeof(ecdhHandles));
}

CngCache::~CngCache()
{
    Cleanup();
}

/**
 * Helper function for closing the algorithm provider handle.
 *
 * @param handle  The pointer of the handle to close. The memory pointed to will
 *                be set to NULL before the function returns.
 */
static void CloseAlgorithmProvider(BCRYPT_ALG_HANDLE* handle)
{
    if (!handle || !(*handle)) {
        return;
    }
    NTSTATUS ntStatus = BCryptCloseAlgorithmProvider(*handle, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        QCC_LogError(ER_OS_ERROR, ("BCryptCloseAlgorithmProvider failed NTSTATUS=0x%x", ntStatus));
    }
    *handle = NULL;
}

void CngCache::Cleanup()
{
    for (int i = 0; i < ALGORITHM_COUNT; ++i) {
        CloseAlgorithmProvider(&algHandles[i][0]);
        CloseAlgorithmProvider(&algHandles[i][1]);
    }
    CloseAlgorithmProvider(&ccmHandle);
    CloseAlgorithmProvider(&ecbHandle);
    for (int i = 0; i < ECDSA_ALGORITHM_COUNT; ++i) {
        CloseAlgorithmProvider(&ecdsaHandles[i]);
    }
    for (int i = 0; i < ECDH_ALGORITHM_COUNT; ++i) {
        CloseAlgorithmProvider(&ecdhHandles[i]);
    }
}

/**
 * The one and only CNG cache instance.
 */
static uint64_t _cngCache[RequiredArrayLength(sizeof(CngCache), uint64_t)];
static bool initialized = false;

CngCache& cngCache = (CngCache&)_cngCache;

void Crypto::Init()
{
    if (!initialized) {
        new (&cngCache)CngCache();
        initialized = true;
    }
}

void Crypto::Shutdown()
{
    if (!initialized) {
        cngCache.~CngCache();
        initialized = false;
    }
}

}         // qcc

