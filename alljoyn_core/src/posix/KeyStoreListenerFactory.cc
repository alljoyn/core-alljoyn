/**
 * @file
 * The KeyStoreListenerFactory implements the default key store listener to stores key blobs.
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
#include <qcc/String.h>
#include <qcc/FileStream.h>
#include <qcc/Util.h>
#include <qcc/LockLevel.h>

#include <alljoyn/KeyStoreListener.h>

#include "KeyStore.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {

class DefaultKeyStoreListener : public KeyStoreListener {

  public:

    DefaultKeyStoreListener(const qcc::String& application, const char* fname) : lockAcquired(false), lock(LOCK_LEVEL_FILE_LOCKER) {
        if (fname) {
            fileName = GetHomeDir() + "/" + fname;
        } else {
            fileName = GetHomeDir() + "/.alljoyn_keystore/" + application;
        }
        /* This also creates an empty KeyStore if it did not exist */
        lockFile = FileSource(fileName);
        if (!FileSource(fileName).IsValid()) {
            QStatus status = ER_OK;
            FileSink sink(fileName, FileSink::PRIVATE);
            if (!sink.IsValid()) {
                status = ER_BUS_WRITE_ERROR;
            } else {
                lockFile = FileSource(fileName);
                if (!lockFile.IsValid()) {
                    status = ER_BUS_READ_ERROR;
                }
            }
            if (status == ER_OK) {
                QCC_LogError(status, ("Initialized empty key store %s", fileName.c_str()));
            } else {
                QCC_LogError(status, ("Cannot initialize key store %s", fileName.c_str()));
            }
        }
    }

    QStatus AcquireExclusiveLock(const char* file, uint32_t line)
    {
        QStatus status = KeyStoreListener::AcquireExclusiveLock(file, line);
        if (status != ER_OK) {
            QCC_LogError(status, ("KeyStoreListener::AcquireExclusiveLock failed"));
            return status;
        }
        if (!lockFile.IsValid() || !lockFile.Lock(true)) {
            lockAcquired = false;
            KeyStoreListener::ReleaseExclusiveLock(file, line);
            status = ER_OS_ERROR;
            QCC_LogError(status, ("DefaultKeyStoreListener::AcquireExclusiveLock failed"));
            return status;
        } else {
            lockAcquired = true;
        }
        return ER_OK;
    }

    void ReleaseExclusiveLock(const char* file, uint32_t line)
    {
        lockFile.Unlock();
        lockAcquired = false;
        KeyStoreListener::ReleaseExclusiveLock(file, line);
    }

    QStatus LoadRequest(KeyStore& keyStore) {
        QStatus status;
        /* Try to load the keystore */
        FileSource source(fileName);
        if (source.IsValid()) {
            /* LoadRequest can be triggered e.g. from KeyStore::Init() and in that case
             * DefaultKeyStoreListener::AcquireExclusiveLock was not called
             */
            QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
            if (!lockAcquired) {
                lockFile.Lock(true);
            }
            lock.Unlock(MUTEX_CONTEXT);
            status = keyStore.Pull(source, fileName);
            if (status == ER_OK) {
                QCC_DbgHLPrintf(("Read key store from %s", fileName.c_str()));
            }
            QCC_VERIFY(ER_OK == lock.Lock(MUTEX_CONTEXT));
            if (!lockAcquired) {
                lockFile.Unlock();
            }
            lock.Unlock(MUTEX_CONTEXT);
        }  else {
            status = ER_BUS_READ_ERROR;
            QCC_LogError(status, ("Failed to read key store %s", fileName.c_str()));
        }
        return status;
    }

    QStatus StoreRequest(KeyStore& keyStore) {
        QStatus status;
        FileSink sink(fileName, FileSink::PRIVATE);
        if (sink.IsValid()) {
            status = keyStore.Push(sink);
            if (status == ER_OK) {
                QCC_DbgHLPrintf(("Wrote key store to %s", fileName.c_str()));
            }
        } else {
            status = ER_BUS_WRITE_ERROR;
            QCC_LogError(status, ("Cannot write key store to %s", fileName.c_str()));
        }
        return status;
    }

  private:

    qcc::String fileName;
    FileSource lockFile;
    bool lockAcquired;
    mutable qcc::Mutex lock;
};

KeyStoreListener* KeyStoreListenerFactory::CreateInstance(const qcc::String& application, const char* fname)
{
    return new DefaultKeyStoreListener(application, fname);
}

}
