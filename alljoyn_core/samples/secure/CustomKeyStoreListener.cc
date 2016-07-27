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
#include <qcc/Debug.h>

#include <alljoyn/KeyStoreListener.h>
#include <alljoyn/Status.h>

#include "ExclusiveFile.h"

#if defined(QCC_OS_ANDROID)
#include <stdlib.h>
#endif

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace qcc;
using namespace ajn;

static String FixKeyStoreFilePath(const char* fname)
{
    String path;
#if !defined(QCC_OS_GROUP_WINDOWS)
    char* val = getenv("HOME");
    if (val) {
        path = val;
    }
#else
    String key = "LOCALAPPDATA";
    DWORD len = GetEnvironmentVariableA(key.c_str(), nullptr, 0);
    if (len == 0) {
        key = "USERPROFILE";
        len = GetEnvironmentVariableA(key.c_str(), nullptr, 0);
    }
    if (len != 0) {
        char* val = new char[len];
        DWORD readLen = GetEnvironmentVariableA(key.c_str(), val, len);
        if (readLen != 0) {
            path = val;
        }
        delete[] val;
    }
#endif
    path += fname;
    return path;
}

class CustomKeyStoreListener : public KeyStoreListener {

  public:

    CustomKeyStoreListener(const char* fname) : fileName(FixKeyStoreFilePath(fname)), exclusiveFile(fileName.c_str())
    {
    }

    QStatus AcquireExclusiveLock(const char* file, uint32_t line)
    {
        QStatus status = KeyStoreListener::AcquireExclusiveLock(file, line);
        if (status != ER_OK) {
            QCC_LogError(status, ("KeyStoreListener::AcquireExclusiveLock failed"));
            return status;
        }
        status = exclusiveFile.AcquireExclusiveLock();
        if (status != ER_OK) {
            QCC_LogError(status, ("exclusiveFile.AcquireExclusiveLock() failed"));
        }
        return status;
    }

    void ReleaseExclusiveLock(const char* file, uint32_t line)
    {
        QCC_ASSERT(exclusiveFile.HasExclusiveLock());
        exclusiveFile.ReleaseExclusiveLock();
        KeyStoreListener::ReleaseExclusiveLock(file, line);
    }

    QStatus LoadRequest(KeyStore& keyStore)
    {
        int64_t size = 0;
        QStatus status = exclusiveFile.GetSize(size);
        if (status == ER_OK) {
            String keysToLoad;
            if ((size > 0) && (static_cast<size_t>(size) < SIZE_MAX)) {
                char* buffer = new char[size];
                size_t pulled = 0;
                status = exclusiveFile.Read(buffer, size, pulled);
                QCC_ASSERT(static_cast<size_t>(size) == pulled);
                keysToLoad = String(buffer, pulled);
                delete[] buffer;
            } else if (static_cast<size_t>(size) >= SIZE_MAX) {
                QCC_LogError(ER_READ_ERROR, ("Too big key store file %s, loading empty key store", fileName.c_str()));
            }
            status = PutKeys(keyStore, keysToLoad, fileName.c_str());
            if (status != ER_OK) {
                QCC_LogError(status, ("PutKeys failed"));
            }
        } else {
            QCC_LogError(status, ("Failed to get the size of %s\n", fileName.c_str()));
        }
        if (status == ER_OK) {
            QCC_DbgHLPrintf(("LoadRequest from %s done", fileName.c_str()));
        }
        return status;
    }

    QStatus StoreRequest(KeyStore& keyStore)
    {
        String sink;
        QStatus status = GetKeys(keyStore, sink);
        if (status != ER_OK) {
            QCC_LogError(status, ("GetKeys failed"));
            return status;
        }
        size_t pushed = 0;
        status = exclusiveFile.Write(sink.data(), sink.size(), pushed);
        if (status != ER_OK) {
            QCC_LogError(status, ("StoreRequest error during data saving"));
            return status;
        }
        if (pushed != sink.size()) {
            status = ER_BUS_CORRUPT_KEYSTORE;
            QCC_LogError(status, ("StoreRequest failed to save data correctly"));
            return status;
        }
        if (!exclusiveFile.Truncate()) {
            QCC_LogError(status, ("FileSink::Truncate failed"));
        }
        if (status == ER_OK) {
            QCC_DbgHLPrintf(("StoreRequest to %s done", fileName.c_str()));
        }
        return status;
    }

  private:

    String fileName;
    ExclusiveFile exclusiveFile;
};

KeyStoreListener* CreateKeyStoreListenerInstance(const char* fname)
{
    return new CustomKeyStoreListener(fname);
}
