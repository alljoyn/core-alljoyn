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

#include <alljoyn/KeyStoreListener.h>
#include <alljoyn/Status.h>

#include "KeyStore.h"

#include <memory>

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

class FileLocker {
  public:
    FileLocker(qcc::String fileName) : fileName(fileName) { }

    ~FileLocker() { }

    QStatus AcquireExclusiveLock() {
        QStatus status = ER_OK;
        FileSource s(fileName);
        if (s.IsValid()) {
            exclusiveLock.reset(new FileSink(fileName, true));
        } else {
            exclusiveLock.reset(new FileSink(fileName, FileSink::PRIVATE));
        }
        if (!exclusiveLock->IsValid()) {
            status = ER_BUS_WRITE_ERROR;
        } else if (!exclusiveLock->Lock(true)) {
            status = ER_OS_ERROR;
        }
        if (status != ER_OK) {
            exclusiveLock.reset();
        }
        return status;
    }

    void ReleaseExclusiveLock() {
        exclusiveLock.reset();
    }

    /* FileSource passed here should be the same as this->fileName */
    QStatus AcquireReadLock(FileSource& s) {
        QStatus status = ER_OK;
        if (exclusiveLock.get() != nullptr) {
            return status;
        }
        if (!s.LockShared(true)) {
            status = ER_OS_ERROR;
        }
        return status;
    }

  private:
    qcc::String fileName;
    std::unique_ptr<FileSink> exclusiveLock;
};

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
        FileSource s(path);
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

    DefaultKeyStoreListener(const qcc::String& application, const char* fname) : fileName(GetDefaultKeyStoreFileName(application.c_str(), fname)), fileLocker(fileName)
    {
        /* This creates an empty KeyStore file if it does not exist yet */
        QStatus status = fileLocker.AcquireExclusiveLock();
        if (status == ER_OK) {
            fileLocker.ReleaseExclusiveLock();
        }
    }

    QStatus AcquireExclusiveLock(const char* file, uint32_t line)
    {
        QStatus status = KeyStoreListener::AcquireExclusiveLock(file, line);
        if (status != ER_OK) {
            QCC_LogError(status, ("KeyStoreListener::AcquireExclusiveLock failed"));
            return status;
        }
        status = fileLocker.AcquireExclusiveLock();
        if (status != ER_OK) {
            QCC_LogError(status, ("fileLocker.AcquireExclusiveLock() failed"));
        }
        return status;
    }

    void ReleaseExclusiveLock(const char* file, uint32_t line)
    {
        fileLocker.ReleaseExclusiveLock();
        KeyStoreListener::ReleaseExclusiveLock(file, line);
    }

    QStatus LoadRequest(KeyStore& keyStore)
    {
        QStatus status;
        /* Try to load the keystore */
        FileSource source = FileSource(fileName);
        if (source.IsValid()) {
            status = fileLocker.AcquireReadLock(source);
            if (status == ER_OK) {
                status = keyStore.Pull(source, fileName);
                if (status == ER_OK) {
                    QCC_DbgHLPrintf(("Read key store from %s", fileName.c_str()));
                }
            } else {
                QCC_LogError(status, ("fileLocker.AcquireReadLock() failed"));
            }
        } else {
            status = ER_BUS_READ_ERROR;
            QCC_LogError(status, ("Failed to read key store %s", fileName.c_str()));
        }
        return status;
    }

    QStatus StoreRequest(KeyStore& keyStore)
    {
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
    FileLocker fileLocker;
};

KeyStoreListener* KeyStoreListenerFactory::CreateInstance(const qcc::String& application, const char* fname)
{
    return new DefaultKeyStoreListener(application, fname);
}

}
