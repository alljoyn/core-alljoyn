/**
 * @file
 * This file implements the wrapper class for ajn::KeyStoreListener that protects against asynchronous
 * unregistration of the listener instance.
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

#include <qcc/Debug.h>
#include <qcc/LockLevel.h>
#include <qcc/String.h>
#include <qcc/Thread.h>

#include "ProtectedKeyStoreListener.h"

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace qcc;

namespace ajn {

ProtectedKeyStoreListener::ProtectedKeyStoreListener(KeyStoreListener* kslistener)
    : listener(kslistener), lock(qcc::LOCK_LEVEL_PROTECTEDKEYSTORELISTENER_LOCK), refCount(0)
{
}

ProtectedKeyStoreListener::~ProtectedKeyStoreListener()
{
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    /*
     * Clear the current listener to prevent any more calls to this listener.
     */
    this->listener = NULL;
    /*
     * Poll and sleep until the current listener is no longer in use.
     */
    while (refCount) {
        lock.Unlock(MUTEX_CONTEXT);
        qcc::Sleep(10);
        QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    }
    lock.Unlock(MUTEX_CONTEXT);
}

QStatus ProtectedKeyStoreListener::AcquireExclusiveLock()
{
    QStatus status = ER_FAIL;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    KeyStoreListener* keyStoreListener = this->listener;
    ++refCount;
    QCC_ASSERT(refCount > 0);
    lock.Unlock(MUTEX_CONTEXT);
    if (keyStoreListener) {
        status = keyStoreListener->AcquireExclusiveLock();
    }
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    --refCount;
    QCC_ASSERT(refCount >= 0);
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}
void ProtectedKeyStoreListener::ReleaseExclusiveLock()
{
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    KeyStoreListener* keyStoreListener = this->listener;
    ++refCount;
    QCC_ASSERT(refCount > 0);
    lock.Unlock(MUTEX_CONTEXT);
    if (keyStoreListener) {
        keyStoreListener->ReleaseExclusiveLock();
    }
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    --refCount;
    QCC_ASSERT(refCount >= 0);
    lock.Unlock(MUTEX_CONTEXT);
}

QStatus ProtectedKeyStoreListener::LoadRequest(KeyStore& keyStore)
{
    QStatus status = ER_FAIL;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    KeyStoreListener* keyStoreListener = this->listener;
    ++refCount;
    QCC_ASSERT(refCount > 0);
    lock.Unlock(MUTEX_CONTEXT);
    if (keyStoreListener) {
        status = keyStoreListener->LoadRequest(keyStore);
    }
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    --refCount;
    QCC_ASSERT(refCount >= 0);
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus ProtectedKeyStoreListener::StoreRequest(KeyStore& keyStore)
{
    QStatus status = ER_FAIL;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    KeyStoreListener* keyStoreListener = this->listener;
    ++refCount;
    QCC_ASSERT(refCount > 0);
    lock.Unlock(MUTEX_CONTEXT);
    if (keyStoreListener) {
        status = keyStoreListener->StoreRequest(keyStore);
    }
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    --refCount;
    QCC_ASSERT(refCount >= 0);
    lock.Unlock(MUTEX_CONTEXT);
    return status;
}

} /* namespace ajn */
