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

    QStatus LoadRequest() {
        lock.Lock(MUTEX_CONTEXT);
        //qcc::StringSource source(sink.GetString());
        //QStatus status = keyStore.Pull(source, pwd);
        QStatus status = PutKeys(sink.GetString(), pwd);
        lock.Unlock(MUTEX_CONTEXT);
        return status;
    }

    QStatus StoreRequest() {
        qcc::StringSink newSink;
        //QStatus status = keyStore.Push(newSink);
        qcc::String s;
        QStatus status = GetKeys(s);
        if (ER_OK == status) {
            size_t pushed = 0;
            status = newSink.PushBytes(s.data(), s.size(), pushed);
            QCC_ASSERT(s.size() == pushed);
            if (ER_OK == status) {
                lock.Lock(MUTEX_CONTEXT);
                sink.Clear();
                status = CopySink(newSink.GetString());
                lock.Unlock(MUTEX_CONTEXT);
            }
        }
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

  private:

    QStatus CopySink(qcc::String& other)
    {
        size_t numSent = 0;
        return sink.PushBytes(other.data(), other.length(), numSent);
    }

    QStatus CopySink(const qcc::StringSink& other)
    {
        return CopySink(((qcc::StringSink&) other).GetString());
    }

    qcc::Mutex lock;
    qcc::StringSink sink;
    qcc::String pwd;
};

}

#endif // _INMEMORYKEYSTORE_H
