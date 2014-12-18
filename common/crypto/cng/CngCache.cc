/**
 * @file
 *
 * This file implements CngCache object for managing lifetime of global CNG handles.
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/Crypto.h>

#include <qcc/CngCache.h>

#define QCC_MODULE "CRYPTO"

namespace qcc {

CngCache::CngCache() : ccmHandle(NULL), ecbHandle(NULL), rsaHandle(NULL)
{
    assert(sizeof(algHandles) == (sizeof(BCRYPT_ALG_HANDLE) * ALGORITHM_COUNT * 2));
    memset(&algHandles, 0, sizeof(algHandles));
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
    if (ntStatus < 0) {
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
    CloseAlgorithmProvider(&rsaHandle);
}

/**
 * The one and only CNG cache instance.
 */
uint64_t cngCacheDummy[sizeof(CngCache) / 8];

CngCache& cngCache = (CngCache &)cngCacheDummy;

static int cngCacheCounter = 0;
bool CngCacheInit::cleanedup = false;
CngCacheInit::CngCacheInit()
{
    if (cngCacheCounter++ == 0) {
        //placement new
        new (&cngCache)CngCache();
    }
}

CngCacheInit::~CngCacheInit()
{
    if (--cngCacheCounter == 0 && !cleanedup) {
        //placement delete
        cngCache.~CngCache();
        cleanedup = true;
    }

}

void CngCacheInit::Cleanup()
{
    if (!cleanedup) {
        //placement delete
        cngCache.~CngCache();
        cleanedup = true;
    }
}
}         // qcc

