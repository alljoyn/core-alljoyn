/**
 * @file
 * The CustomKeyStoreListener is a sample implementation of KeyStoreListener interface.
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
#include <qcc/String.h>
#include <qcc/Util.h>
#include <qcc/FileStream.h>

#include <alljoyn/KeyStoreListener.h>
#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace qcc;
using namespace ajn;

static String FixKeyStoreFilePath(const char* fname)
{
    String path = GetHomeDir();
    path += fname;
    return path;
}

class CustomKeyStoreListener : public KeyStoreListener {

  public:

    CustomKeyStoreListener(const char* fname) : fileName(FixKeyStoreFilePath(fname)), fileLocker(fileName.c_str())
    {
        FileLock readLock;
        /* 'readLock' is released when goes out of scope. */
        QStatus status = fileLocker.GetFileLockForRead(&readLock);
        if (status == ER_EOF) {
            /* The file does not exist. Create one here by simply acquiring Write lock. */
            status = fileLocker.AcquireWriteLock();
            if (status == ER_OK) {
                fileLocker.ReleaseWriteLock();
            } else {
                QCC_LogError(status, ("FileLocker::AcquireWriteLock() failed, cannot write file (%s)", fileName.c_str()));
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
        status = fileLocker.AcquireWriteLock();
        if (status != ER_OK) {
            QCC_LogError(status, ("fileLocker.AcquireWriteLock() failed"));
        }
        return status;
    }

    void ReleaseExclusiveLock(const char* file, uint32_t line)
    {
        QCC_ASSERT(fileLocker.HasWriteLock());
        fileLocker.ReleaseWriteLock();
        KeyStoreListener::ReleaseExclusiveLock(file, line);
    }

    QStatus LoadRequest(KeyStore& keyStore)
    {
        FileLock readLock;
        QStatus status = fileLocker.GetFileLockForRead(&readLock);
        if (status == ER_OK) {
            FileSource* source = readLock.GetSource();
            QCC_ASSERT(source != nullptr);
            if (source != nullptr) {
                int64_t size = 0;
                status = source->GetSize(size);
                if (status == ER_OK) {
                    String keysToLoad;
                    if ((size > 0) && (static_cast<size_t>(size) < SIZE_MAX)) {
                        char* buffer = new char[size];
                        size_t pulled = 0;
                        status = source->PullBytes(buffer, size, pulled);
                        QCC_ASSERT(static_cast<size_t>(size) == pulled);
                        keysToLoad = String(buffer, pulled);
                        delete[] buffer;
                    } else if (static_cast<size_t>(size) >= SIZE_MAX) {
                        QCC_LogError(ER_READ_ERROR, ("Too big key store file %s, loading empty key store", fileLocker.GetFileName()));
                    }
                    status = PutKeys(keyStore, keysToLoad, fileLocker.GetFileName());
                    if (status != ER_OK) {
                        QCC_LogError(status, ("PutKeys failed"));
                    }
                } else {
                    QCC_LogError(status, ("Failed to get the size of %s\n", fileLocker.GetFileName()));
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("Internal error has occurred, cannot get a handle to %s\n", fileLocker.GetFileName()));
            }
        } else {
            QCC_LogError(status, ("Failed to access read lock on file %s\n", fileLocker.GetFileName()));
        }
        if (status == ER_OK) {
            QCC_DbgHLPrintf(("LoadRequest from %s done", fileLocker.GetFileName()));
        }
        return status;
    }

    QStatus StoreRequest(KeyStore& keyStore)
    {
        FileLock writeLock;
        QStatus status = fileLocker.GetFileLockForWrite(&writeLock);
        if (status == ER_OK) {
            String sink;
            status = GetKeys(keyStore, sink);
            if (status != ER_OK) {
                QCC_LogError(status, ("GetKeys failed"));
                return status;
            }
            size_t pushed = 0;
            status = writeLock.GetSink()->PushBytes(sink.data(), sink.size(), pushed);
            if (status != ER_OK) {
                QCC_LogError(status, ("StoreRequest error during data saving"));
                return status;
            }
            if (pushed != sink.size()) {
                status = ER_BUS_CORRUPT_KEYSTORE;
                QCC_LogError(status, ("StoreRequest failed to save data correctly"));
                return status;
            }
            if (!writeLock.GetSink()->Truncate()) {
                QCC_LogError(status, ("FileSink::Truncate failed"));
            }
        } else {
            QCC_LogError(status, ("Failed to store request - write lock has not been taken"));
            QCC_ASSERT(!"write lock has not been taken");
        }
        if (status == ER_OK) {
            QCC_DbgHLPrintf(("StoreRequest to %s done", fileLocker.GetFileName()));
        }
        return status;
    }

  private:

    String fileName;
    FileLocker fileLocker;
};

KeyStoreListener* CreateKeyStoreListenerInstance(const char* fname)
{
    return new CustomKeyStoreListener(fname);
}
