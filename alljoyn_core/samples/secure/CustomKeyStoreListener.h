/**
 * @file
 * The CustomtKeyStoreListener is a sample implementation of KeyStoreListener interface.
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
#ifndef _CUSTOM_KEYSTORELISTENER_H
#define _CUSTOM_KEYSTORELISTENER_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Util.h>
#include <qcc/FileStream.h>

#include <alljoyn/KeyStoreListener.h>
#include <alljoyn/Status.h>

using namespace qcc;
using namespace ajn;

static qcc::String FixKeyStoreFilePath(const char* fname)
{
    qcc::String path = qcc::GetHomeDir();
    path += fname;
    return path;
}

class CustomtKeyStoreListener : public KeyStoreListener {

  public:

    CustomtKeyStoreListener(const char* fname) : fileName(FixKeyStoreFilePath(fname)), fileLocker(fileName.c_str())
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
                source->GetSize(size);
                if (size == 0) {
                    status = PutKeys(keyStoreContext, "", fileLocker.GetFileName());
                } else {
                    char buffer[size];
                    size_t pulled = 0;
                    status = source->PullBytes(buffer, size, pulled);
                    QCC_ASSERT(static_cast<size_t>(size) == pulled);
                    status = PutKeys(keyStoreContext, String(buffer, pulled), fileLocker.GetFileName());
                }
                if (status == ER_OK) {
                    printf("Read key store from %s\n", fileLocker.GetFileName());
                } else {
                    printf("PutKeys failed, status=%s\n", QCC_StatusText(status));
                }
            } else {
                status = ER_OS_ERROR;
            }
        }
        if (status != ER_OK) {
            printf("Failed to read key store %s\n", fileLocker.GetFileName());
        }
        return status;
    }

    QStatus StoreRequest(KeyStoreContext keyStoreContext)
    {
        FileLock writeLock;
        QStatus status = fileLocker.GetFileLockForWrite(&writeLock);
        if (status == ER_OK) {
            String ssink;
            status = GetKeys(keyStoreContext, ssink);
            if (status != ER_OK) {
                printf("GetKeys failed, status=%s", QCC_StatusText(status));
                return status;
            }
            size_t pushed = 0;
            status = writeLock.GetSink()->PushBytes(ssink.data(), ssink.size(), pushed);
            if (status != ER_OK) {
                printf("StoreRequest error during data saving, status=%s\n", QCC_StatusText(status));
                return status;
            }
            if (pushed != ssink.size()) {
                status = ER_BUS_CORRUPT_KEYSTORE;
                printf("StoreRequest failed to save data correctly, status=%s\n", QCC_StatusText(status));
                return status;
            }
            if (!writeLock.GetSink()->Truncate()) {
                printf("FileSink::Truncate failed, status=%s\n", QCC_StatusText(status));
            }
            printf("Wrote key store to %s\n", fileLocker.GetFileName());
        } else {
            printf("Failed to store request - write lock has not been taken, status=%s\n", QCC_StatusText(status));
            QCC_ASSERT(!"write lock has not been taken");
        }
        return status;
    }

  private:

    qcc::String fileName;
    FileLocker fileLocker;
};

#endif