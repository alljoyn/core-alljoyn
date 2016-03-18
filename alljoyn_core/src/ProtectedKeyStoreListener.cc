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
    : listener(kslistener), lock(qcc::LOCK_LEVEL_PROTECTEDKEYSTORELISTENER_LOCK), refCount(0), refCountSelf(1)
{
}

ProtectedKeyStoreListener::~ProtectedKeyStoreListener()
{
    QCC_ASSERT(refCountSelf == 0);
    QCC_ASSERT(refCount == 0);
}

void ProtectedKeyStoreListener::AddRef()
{
#ifdef NDEBUG
    (void)qcc::IncrementAndFetch(&refCountSelf);
#else
    int32_t count = qcc::IncrementAndFetch(&refCountSelf);
    QCC_ASSERT(count > 1);
#endif
}

void ProtectedKeyStoreListener::DelRef()
{
    int32_t count = qcc::DecrementAndFetch(&refCountSelf);
    QCC_ASSERT(count >= 0);
    if (count == 0) {
        delete this;
    }
}

void ProtectedKeyStoreListener::ClearListener()
{
    AddRef();
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    listener = nullptr;
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    DelRef();
}

QStatus ProtectedKeyStoreListener::AcquireExclusiveLock(const char* file, uint32_t line)
{
    AddRef();
    QStatus status = ER_FAIL;
    QCC_VERIFY(ER_OK == lock.Lock(file, line));
    KeyStoreListener* keyStoreListener = this->listener;
    ++refCount;
    QCC_ASSERT(refCount > 0);
    QCC_VERIFY(ER_OK == lock.Unlock(file, line));
    if (keyStoreListener) {
        status = keyStoreListener->AcquireExclusiveLock(file, line);
    }
    QCC_VERIFY(ER_OK == lock.Lock(file, line));
    --refCount;
    QCC_ASSERT(refCount >= 0);
    QCC_VERIFY(ER_OK == lock.Unlock(file, line));
    DelRef();
    return status;
}
void ProtectedKeyStoreListener::ReleaseExclusiveLock(const char* file, uint32_t line)
{
    AddRef();
    QCC_VERIFY(ER_OK == lock.Lock(file, line));
    KeyStoreListener* keyStoreListener = this->listener;
    ++refCount;
    QCC_ASSERT(refCount > 0);
    QCC_VERIFY(ER_OK == lock.Unlock(file, line));
    if (keyStoreListener) {
        keyStoreListener->ReleaseExclusiveLock(file, line);
    }
    QCC_VERIFY(ER_OK == lock.Lock(file, line));
    --refCount;
    QCC_ASSERT(refCount >= 0);
    QCC_VERIFY(ER_OK == lock.Unlock(file, line));
    DelRef();
}

QStatus ProtectedKeyStoreListener::LoadRequest(KeyStore& keyStore)
{
    AddRef();
    QStatus status = ER_FAIL;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    KeyStoreListener* keyStoreListener = this->listener;
    ++refCount;
    QCC_ASSERT(refCount > 0);
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    if (keyStoreListener) {
        status = keyStoreListener->LoadRequest(keyStore);
    }
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    --refCount;
    QCC_ASSERT(refCount >= 0);
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    DelRef();
    return status;
}

QStatus ProtectedKeyStoreListener::StoreRequest(KeyStore& keyStore)
{
    AddRef();
    QStatus status = ER_FAIL;
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    KeyStoreListener* keyStoreListener = this->listener;
    ++refCount;
    QCC_ASSERT(refCount > 0);
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    if (keyStoreListener) {
        status = keyStoreListener->StoreRequest(keyStore);
    }
    QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
    --refCount;
    QCC_ASSERT(refCount >= 0);
    QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
    DelRef();
    return status;
}

} /* namespace ajn */
