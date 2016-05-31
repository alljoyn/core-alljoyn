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
                printf("FileLocker::AcquireWriteLock() failed, status=%s - cannot write file (%s).\n", QCC_StatusText(status), fileName.c_str());
            }
        }
    }

    QStatus AcquireExclusiveLock(const char* file, uint32_t line)
    {
        QStatus status = KeyStoreListener::AcquireExclusiveLock(file, line);
        if (status != ER_OK) {
            printf("KeyStoreListener::AcquireExclusiveLock failed, status=%s\n", QCC_StatusText(status));
            return status;
        }
        status = fileLocker.AcquireWriteLock();
        if (status != ER_OK) {
            printf("fileLocker.AcquireWriteLock() failed, status=%s\n", QCC_StatusText(status));
        }
        return status;
    }

    void ReleaseExclusiveLock(const char* file, uint32_t line)
    {
        QCC_ASSERT(fileLocker.HasWriteLock());
        fileLocker.ReleaseWriteLock();
        KeyStoreListener::ReleaseExclusiveLock(file, line);
    }

    QStatus LoadRequest(KeyStoreContext keyStoreContext)
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
                        printf("[%s] Too big kye store file %s, loading empty key store\n", QCC_StatusText(ER_WARNING), fileLocker.GetFileName());
                    }
                    status = PutKeys(keyStoreContext, keysToLoad, fileLocker.GetFileName());
                    if (status != ER_OK) {
                        printf("[%s] PutKeys failed\n", QCC_StatusText(status));
                    }
                } else {
                    printf("[%s] Failed to get the size of %s\n", QCC_StatusText(status), fileLocker.GetFileName());
                }
            } else {
                status = ER_FAIL;
                printf("[%s] Internal error has occurred, cannot get a handle to %s\n", QCC_StatusText(status), fileLocker.GetFileName());
            }
        } else {
            printf("[%s] Failed to access read lock on file %s\n", QCC_StatusText(status), fileLocker.GetFileName());
        }
        if (status == ER_OK) {
            printf("[%s] LoadRequest done\n", QCC_StatusText(status));
        }
        return status;
    }

    QStatus StoreRequest(KeyStoreContext keyStoreContext)
    {
        FileLock writeLock;
        QStatus status = fileLocker.GetFileLockForWrite(&writeLock);
        if (status == ER_OK) {
            String sink;
            status = GetKeys(keyStoreContext, sink);
            if (status != ER_OK) {
                printf("[%s] GetKeys failed", QCC_StatusText(status));
                return status;
            }
            size_t pushed = 0;
            status = writeLock.GetSink()->PushBytes(sink.data(), sink.size(), pushed);
            if (status != ER_OK) {
                printf("[%s] StoreRequest error during data saving\n", QCC_StatusText(status));
                return status;
            }
            if (pushed != sink.size()) {
                status = ER_BUS_CORRUPT_KEYSTORE;
                printf("[%s] StoreRequest failed to save data correctly\n", QCC_StatusText(status));
                return status;
            }
            if (!writeLock.GetSink()->Truncate()) {
                printf("[%s] FileSink::Truncate failed\n", QCC_StatusText(status));
            }
        } else {
            printf("[%s] Failed to store request - write lock has not been taken\n", QCC_StatusText(status));
            QCC_ASSERT(!"write lock has not been taken");
        }
        if (status == ER_OK) {
            printf("[%s] StoreRequest done\n", QCC_StatusText(status));
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
