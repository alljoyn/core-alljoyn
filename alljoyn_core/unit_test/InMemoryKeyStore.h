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
#ifndef _INMEMORYKEYSTORE_H
#define _INMEMORYKEYSTORE_H

#include <qcc/platform.h>
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
        qcc::GUID128 guid;
        pwd = guid.ToString();
    }

    InMemoryKeyStoreListener(qcc::String& source, qcc::String& pwd) : KeyStoreListener(), pwd(pwd)
    {
        CopySink(source);
    }

    QStatus LoadRequest(KeyStore& keyStore) {
        QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
        memset(hist, 0, sizeof(hist));
        hist[0] = keyStore.keys->size();
        hist[1] = keyStore.persistentKeys->size();
        qcc::StringSource source(sink.GetString());
        hist[2] = keyStore.keys->size();
        hist[3] = keyStore.persistentKeys->size();
        QStatus status = keyStore.Pull(source, pwd);
        hist[4] = keyStore.keys->size();
        hist[5] = keyStore.persistentKeys->size();
        QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
        return status;
    }

    QStatus StoreRequest(KeyStore& keyStore) {
        qcc::StringSink newSink;
        QStatus status = keyStore.Push(newSink);
        if (ER_OK != status) {
            return status;
        }
        QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
        sink.Clear();
        status = CopySink(newSink.GetString());
        QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
        return status;
    }

    /**
     * Copy constructor
     */
    InMemoryKeyStoreListener(const InMemoryKeyStoreListener& other)
    {
        *this = other;
    }

    /**
     * Assignment operator
     */
    InMemoryKeyStoreListener& operator=(const InMemoryKeyStoreListener& other)
    {
        if (this != &other) {
            pwd = other.pwd;
            CopySink(other.sink);
        }
        return *this;
    }
#if 0
    QStatus AcquireExclusiveLock(const char* file, uint32_t line)
    {
        QCC_VERIFY(ER_OK == lock.Lock(file, line));
        return ER_OK;
    }

    void ReleaseExclusiveLock(const char* file, uint32_t line)
    {
        QCC_VERIFY(ER_OK == lock.Unlock(file, line));
    }
#endif
  private:

    QStatus CopySink(qcc::String& other)
    {
        size_t numSent = 0;
        QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
        QStatus status = sink.PushBytes(other.data(), other.length(), numSent);
        QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
        return status;
    }

    QStatus CopySink(const qcc::StringSink& other)
    {
        QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
        QStatus status = CopySink(((qcc::StringSink&) other).GetString());
        QCC_VERIFY(ER_OK == lock.Unlock(MUTEX_CONTEXT));
        return status;
    }

    qcc::Mutex lock;
    qcc::StringSink sink;
    qcc::String pwd;
    size_t hist[11];
    
};

}

#endif // _INMEMORYKEYSTORE_H
