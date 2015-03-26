/**
 * @file
 * In-memory keystore implementation
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

#include <qcc/platform_cpp.h>
#include <qcc/GUID.h>
#include <qcc/StringSink.h>
#include <qcc/StringSource.h>
#include <qcc/StringUtil.h>

#include <alljoyn/KeyStoreListener.h>
#include <alljoyn/Status.h>
#include "KeyStore.h"

namespace ajn {

class InMemoryKeyStoreListener : public KeyStoreListener {

  public:

    InMemoryKeyStoreListener() : KeyStoreListener()
    {
    }

    QStatus LoadRequest(KeyStore& keyStore) {
        lock.Lock(MUTEX_CONTEXT);
        qcc::StringSource source(sink.GetString());
        QStatus status = keyStore.Pull(source, pwd.ToString());
        lock.Unlock(MUTEX_CONTEXT);
        return status;
    }

    QStatus StoreRequest(KeyStore& keyStore) {
        qcc::StringSink newSink;
        QStatus status = keyStore.Push(newSink);
        if (ER_OK != status) {
            return status;
        }
        lock.Lock(MUTEX_CONTEXT);
        sink.Clear();
        size_t numSent = 0;
        status = sink.PushBytes(newSink.GetString().data(), newSink.GetString().length(), numSent);
        lock.Unlock(MUTEX_CONTEXT);
        return status;
    }

  private:
    /**
     * Assignment operator is private
     */
    InMemoryKeyStoreListener& operator=(const InMemoryKeyStoreListener& other);

    /**
     * Copy constructor is private
     */
    InMemoryKeyStoreListener(const InMemoryKeyStoreListener& other);

    qcc::Mutex lock;
    qcc::StringSink sink;
    qcc::GUID128 pwd;
};

}
