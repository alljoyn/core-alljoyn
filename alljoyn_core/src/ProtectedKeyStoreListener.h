#ifndef _ALLJOYN_PROTECTED_KEYSTORE_LISTENER_H
#define _ALLJOYN_PROTECTED_KEYSTORE_LISTENER_H
/**
 * @file
 * This file defines a wrapper class for ajn::KeyStoreListener that protects against asynchronous
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
#ifndef __cplusplus
#error Only include ProtectedKeyStoreListener.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/Mutex.h>
#include <alljoyn/KeyStoreListener.h>
#include <alljoyn/Status.h>

namespace ajn {

class KeyStore;

/**
 * This class adds a level of indirection to an AuthListener so the actual AuthListener can
 * asynchronously be set or removed safely.
 */
class ProtectedKeyStoreListener {

  public:

    ProtectedKeyStoreListener(KeyStoreListener* kslistener);

    /**
     * Increment reference count of this class.
     */
    void AddRef();

    /**
     * Decrement reference count of this class. The class is deleted when the count goes to zero.
     */
    void DelRef();

    /**
     * Clear the current listener to prevent any more calls to this listener.
     */
    void ClearListener();

    /**
     * Request to acquire exclusive lock of the keystore - used during data commit.
     *
     * NOTE: Best practice is to call `AcquireExclusiveLock(MUTEX_CONTEXT)`
     *
     * @see MUTEX_CONTEXT
     *
     * @param file the name of the file this lock was called from
     * @param line the line number of the file this lock was called from
     *
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus AcquireExclusiveLock(const char* file, uint32_t line);

    /**
     * Release exclusive lock of the keystore - for completing data commit.
     *
     * NOTE: Best practice is to call `ReleaseExclusiveLock(MUTEX_CONTEXT)`
     *
     * @see MUTEX_CONTEXT
     *
     * @param file the name of the file this lock was called from
     * @param line the line number of the file this lock was called from
     */
    void ReleaseExclusiveLock(const char* file, uint32_t line);

    /**
     * Simply wraps the call of the same name to the inner KeyStoreListener
     */
    QStatus LoadRequest(KeyStore& keyStore);

    /**
     * Simply wraps the call of the same name to the inner KeyStoreListener
     *
     * @return
     *      - #ER_OK if successful
     *      - An error status otherwise
     */
    QStatus StoreRequest(KeyStore& keyStore);

  private: ~ProtectedKeyStoreListener();

    /*
     * The inner listener that is being protected.
     */
    KeyStoreListener* listener;

    /*
     * Reference count so we know when the inner listener is no longer in use.
     */
    qcc::Mutex lock;
    int32_t refCount;                /**< The reference count of the child object. */
    volatile int32_t refCountSelf;   /**< The reference count of this class. The class is deleted when this goes to zero. */
};
}
#endif
