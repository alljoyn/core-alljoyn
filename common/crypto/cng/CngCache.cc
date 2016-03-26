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

#include <windows.h>
#include <bcrypt.h>
#include <capi.h>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Util.h>
#include <qcc/CryptoECC.h>
#include <qcc/CngCache.h>
#include <qcc/LockLevel.h>
#include "Crypto.h"

#define QCC_MODULE "CRYPTO"

namespace qcc {

static bool s_initializationDone = false;

CngCache::CngCache() : ccmHandle(NULL), ecbHandle(NULL)
{
    QCC_ASSERT(!s_initializationDone);
    m_mutex = new Mutex(LOCK_LEVEL_CNGCACHELOCK);
    QCC_ASSERT(sizeof(algHandles) == (sizeof(BCRYPT_ALG_HANDLE) * ALGORITHM_COUNT * 2));
    memset(&algHandles, 0, sizeof(algHandles));
    QCC_ASSERT(sizeof(ecdsaHandles) == (sizeof(BCRYPT_ALG_HANDLE) * ECDSA_ALGORITHM_COUNT));
    memset(&ecdsaHandles, 0, sizeof(ecdsaHandles));
    QCC_ASSERT(sizeof(ecdhHandles) == (sizeof(BCRYPT_ALG_HANDLE) * ECDH_ALGORITHM_COUNT));
    memset(&ecdhHandles, 0, sizeof(ecdhHandles));
    QCC_ASSERT(!s_initializationDone);
    s_initializationDone = true;
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
    QCC_ASSERT(s_initializationDone);
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
    delete m_mutex;
    QCC_ASSERT(s_initializationDone);
    s_initializationDone = false;
}

/*
 * For each of the Open* methods, always check the handle is still NULL after acquiring the lock
 * to make sure another thread didn't get in there in the meantime. If that does happen, though,
 * do nothing but return ER_OK so the caller can proceed.
 */

QStatus CngCache::OpenCcmHandle()
{
    QCC_ASSERT(s_initializationDone);
    QStatus status = ER_OK;
    QCC_VERIFY(ER_OK == m_mutex->Lock(MUTEX_CONTEXT));
    if (nullptr == cngCache.ccmHandle) {
        NTSTATUS ntStatus = BCryptOpenAlgorithmProvider(&cngCache.ccmHandle, BCRYPT_AES_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to open AES algorithm provider, ntStatus=%X", ntStatus));
        }
    }
    QCC_VERIFY(ER_OK == m_mutex->Unlock(MUTEX_CONTEXT));

    return status;
}

QStatus CngCache::OpenEcbHandle()
{
    QCC_ASSERT(s_initializationDone);
    QStatus status = ER_OK;
    QCC_VERIFY(ER_OK == m_mutex->Lock(MUTEX_CONTEXT));
    if (nullptr == cngCache.ecbHandle) {
        NTSTATUS ntStatus = BCryptOpenAlgorithmProvider(&cngCache.ecbHandle, BCRYPT_AES_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to open AES algorithm provider, ntStatus=%X", ntStatus));
        }
    }
    QCC_VERIFY(ER_OK == m_mutex->Unlock(MUTEX_CONTEXT));

    return status;
}

QStatus CngCache::OpenHashHandle(Crypto_Hash::Algorithm algorithm, bool usingMac)
{
    QCC_ASSERT(s_initializationDone);
    LPCWSTR algId;

    switch (algorithm) {
    case qcc::Crypto_Hash::SHA1:
        algId = BCRYPT_SHA1_ALGORITHM;
        break;

    case qcc::Crypto_Hash::SHA256:
        algId = BCRYPT_SHA256_ALGORITHM;
        break;

    default:
        return ER_BAD_ARG_1;
    }

    QStatus status = ER_OK;
    QCC_VERIFY(ER_OK == m_mutex->Lock(MUTEX_CONTEXT));
    if (nullptr == cngCache.algHandles[algorithm][usingMac]) {
        NTSTATUS ntStatus = BCryptOpenAlgorithmProvider(&cngCache.algHandles[algorithm][usingMac], algId, MS_PRIMITIVE_PROVIDER, usingMac ? BCRYPT_ALG_HANDLE_HMAC_FLAG : 0);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to open hash algorithm provider, ntstatus=%X", ntStatus));
        }
    }
    QCC_VERIFY(ER_OK == m_mutex->Unlock(MUTEX_CONTEXT));

    return status;

}

QStatus CngCache::OpenEcdsaHandle(uint8_t curveType)
{
    QCC_ASSERT(s_initializationDone);
    QStatus status = ER_OK;
    LPCWSTR ecdsaAlgId = nullptr;

    switch (curveType) {
    case Crypto_ECC::ECC_NIST_P256:
        ecdsaAlgId = BCRYPT_ECDSA_P256_ALGORITHM;
        break;

    default:
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        QCC_LogError(status, ("Unrecognized curve type %d", curveType));
        return status;
    }

    QCC_VERIFY(ER_OK == m_mutex->Lock(MUTEX_CONTEXT));
    if (nullptr == cngCache.ecdsaHandles[curveType]) {
        NTSTATUS ntStatus = BCryptOpenAlgorithmProvider(&cngCache.ecdsaHandles[curveType], ecdsaAlgId, NULL, 0);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to open ECDSA algorithm provider, ntStatus=%X", ntStatus));
        }
    }
    QCC_VERIFY(ER_OK == m_mutex->Unlock(MUTEX_CONTEXT));

    return status;
}

QStatus CngCache::OpenEcdhHandle(uint8_t curveType)
{
    QCC_ASSERT(s_initializationDone);
    QStatus status = ER_OK;
    LPCWSTR ecdhAlgId = nullptr;

    switch (curveType) {
    case Crypto_ECC::ECC_NIST_P256:
        ecdhAlgId = BCRYPT_ECDH_P256_ALGORITHM;
        break;

    default:
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        QCC_LogError(status, ("Unrecognized curve type %d", curveType));
        return status;
    }

    QCC_VERIFY(ER_OK == m_mutex->Lock(MUTEX_CONTEXT));
    if (nullptr == cngCache.ecdhHandles[curveType]) {
        NTSTATUS ntStatus = BCryptOpenAlgorithmProvider(&cngCache.ecdhHandles[curveType], ecdhAlgId, NULL, 0);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to open ECDH algorithm provider, ntStatus=%X", ntStatus));
        }
    }
    QCC_VERIFY(ER_OK == m_mutex->Unlock(MUTEX_CONTEXT));

    return status;
}

/**
 * The one and only CNG cache instance.
 */
static uint64_t _cngCache[RequiredArrayLength(sizeof(CngCache), uint64_t)];
static volatile bool initialized = false;

CngCache& cngCache = (CngCache&)_cngCache;

QStatus Crypto::Init()
{
    if (!initialized) {
        new (&cngCache)CngCache();
        initialized = true;
    }

    return ER_OK;
}

void Crypto::Shutdown()
{
    if (initialized) {
        cngCache.~CngCache();
        initialized = false;
    }
}

}         // qcc

