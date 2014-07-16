#ifndef _ALLJOYN_PROTECTED_KEYSTORE_LISTENER_H
#define _ALLJOYN_PROTECTED_KEYSTORE_LISTENER_H
/**
 * @file
 * This file defines a wrapper class for ajn::KeyStoreListener that protects against asynchronous
 * deregistration of the listener instance.
 */

/******************************************************************************
 * Copyright (c) 2012, 2014, AllSeen Alliance. All rights reserved.
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
#ifndef __cplusplus
#error Only include ProtectedKeyStoreListener.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/Thread.h>

#include <alljoyn/KeyStoreListener.h>

#include <alljoyn/Status.h>

namespace ajn {


/**
 * This class adds a level of indirection to an AuthListener so the actual AuthListener can
 * asynchronously be set or removed safely. If the
 */
class ProtectedKeyStoreListener : public KeyStoreListener {
  public:

    ProtectedKeyStoreListener(KeyStoreListener* kslistener) : listener(kslistener), refCount(0) { }
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~ProtectedKeyStoreListener() {
        lock.Lock(MUTEX_CONTEXT);
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
            lock.Lock(MUTEX_CONTEXT);
        }
        lock.Unlock(MUTEX_CONTEXT);
    }

    /**
     * Simply wraps the call of the same name to the inner KeyStoreListener
     */
    QStatus LoadRequest(KeyStore& keyStore)
    {
        QStatus status = ER_FAIL;
        lock.Lock(MUTEX_CONTEXT);
        KeyStoreListener* listener = this->listener;
        ++refCount;
        lock.Unlock(MUTEX_CONTEXT);
        if (listener) {
            status = listener->LoadRequest(keyStore);
        }
        lock.Lock(MUTEX_CONTEXT);
        --refCount;
        lock.Unlock(MUTEX_CONTEXT);
        return status;
    }

    /**
     * Simply wraps the call of the same name to the inner KeyStoreListener
     */
    QStatus StoreRequest(KeyStore& keyStore)
    {
        QStatus status = ER_FAIL;
        lock.Lock(MUTEX_CONTEXT);
        KeyStoreListener* listener = this->listener;
        ++refCount;
        lock.Unlock(MUTEX_CONTEXT);
        if (listener) {
            status = listener->StoreRequest(keyStore);
        }
        lock.Lock(MUTEX_CONTEXT);
        --refCount;
        lock.Unlock(MUTEX_CONTEXT);
        return status;
    }

  private:

    /*
     * The inner listener that is being protected.
     */
    KeyStoreListener* listener;

    /*
     * Reference count so we know when the inner listener is no longer in use.
     */
    qcc::Mutex lock;
    int32_t refCount;
};
}
#endif
