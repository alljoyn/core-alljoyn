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
#include <qcc/SecureAllocator.h>

#include <alljoyn/KeyStoreListener.h>
#include <alljoyn/Status.h>

#include "KeyStore.h"

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;


namespace ajn {

static qcc::String GetDefaultKeyStoreFileName(const char* application, const char* fname)
{
    qcc::String path = qcc::GetHomeDir();

    if (fname != nullptr) {
        path += "/";
        path += fname;
    } else {
        path += "/.alljoyn_keystore/";
        path += application;
    }

    return path;
}

/* Note: this function is used by test code. */
QStatus DeleteDefaultKeyStoreFile(const qcc::String& application, const char* fname)
{
    QStatus status = ER_OK;
    qcc::String path = GetDefaultKeyStoreFileName(application.c_str(), fname);

    if (qcc::FileExists(path) == ER_OK) {
        FileSink s(path, false, FileSink::PRIVATE);
        s.Lock(true);
        status = qcc::DeleteFile(path);
        if (status != ER_OK) {
            QCC_LogError(status, ("DeleteFile(%s) failed", path.c_str()));
        }
    }

    return status;
}

class DefaultKeyStoreListener : public KeyStoreListener {

  public:

    DefaultKeyStoreListener(const qcc::String& application, const char* fname) : fileName(GetDefaultKeyStoreFileName(application.c_str(), fname)), fileLocker(fileName.c_str())
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
                QCC_LogError(status, ("FileLocker::AcquireWriteLock() failed, status=(%#x) - cannot write file (%s).", status, fileName.c_str()));
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
                status = keyStore.Pull(*source, fileLocker.GetFileName());
                if (status == ER_OK) {
                    QCC_DbgHLPrintf(("Read key store from %s", fileLocker.GetFileName()));
                }
            } else {
                status = ER_OS_ERROR;
            }
        }
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to read key store %s", fileLocker.GetFileName()));
        }
        return status;
    }

    QStatus StoreRequest(KeyStore& keyStore)
    {
        class BufferSink : public Sink {
          public:
            virtual ~BufferSink() { }

            QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent)
            {
                const uint8_t* start = static_cast<const uint8_t*>(buf);
                const uint8_t* end = start + numBytes;
                sbuf.reserve(sbuf.size() + numBytes);
                sbuf.insert(sbuf.end(), start, end);
                numSent = numBytes;
                return ER_OK;
            }

            const std::vector<uint8_t, SecureAllocator<uint8_t> >& GetBuffer() const { return sbuf; }

          private:
            std::vector<uint8_t, SecureAllocator<uint8_t> > sbuf;        /**< storage for byte stream */
        };

        FileLock writeLock;
        QStatus status = fileLocker.GetFileLockForWrite(&writeLock);
        if (status == ER_OK) {
            BufferSink buffer;
            status = keyStore.Push(buffer);
            if (status != ER_OK) {
                QCC_LogError(status, ("StoreRequest error during data buffering"));
                return status;
            }
            size_t pushed = 0;
            status = writeLock.GetSink()->PushBytes(buffer.GetBuffer().data(), buffer.GetBuffer().size(), pushed);
            if (status != ER_OK) {
                QCC_LogError(status, ("StoreRequest error during data saving"));
                return status;
            }
            if (pushed != buffer.GetBuffer().size()) {
                status = ER_BUS_CORRUPT_KEYSTORE;
                QCC_LogError(status, ("StoreRequest failed to save data correctly"));
                return status;
            }
            if (!writeLock.GetSink()->Truncate()) {
                QCC_LogError(ER_WARNING, ("FileSink::Truncate failed"));
            }
            QCC_DbgHLPrintf(("Wrote key store to %s", fileLocker.GetFileName()));
        } else {
            QCC_LogError(status, ("Failed to store request - write lock has not been taken, status=(%#x)", status));
            QCC_ASSERT(!"write lock has not been taken");
        }
        return status;
    }

  private:

    qcc::String fileName;
    FileLocker fileLocker;
};

KeyStoreListener* KeyStoreListenerFactory::CreateInstance(const qcc::String& application, const char* fname)
{
    return new DefaultKeyStoreListener(application, fname);
}

}
