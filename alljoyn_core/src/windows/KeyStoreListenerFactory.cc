/**
 * @file
 * The KeyStoreListenerFactory implements the default key store listener to stores key blobs.  This is place holder implementation.  It should be replaced with
 * a windows specific implementation.
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
#include <qcc/StringSource.h>
#include <qcc/StringSink.h>
#include <qcc/Util.h>
#include <qcc/SecureAllocator.h>

#include <alljoyn/KeyStoreListener.h>

#include "KeyStore.h"

#include <alljoyn/Status.h>
#include <dpapi.h>

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {

class DefaultKeyStoreListener : public KeyStoreListener {
  public:
    DefaultKeyStoreListener(const qcc::String& application, const char* fname);
    virtual ~DefaultKeyStoreListener();

    virtual QStatus AcquireExclusiveLock(const char* file, uint32_t line);
    virtual void ReleaseExclusiveLock(const char* file, uint32_t line);

    virtual QStatus LoadRequest(KeyStore& keyStore);
    virtual QStatus StoreRequest(KeyStore& keyStore);

  private:
    FileLocker m_fileLocker;
};

static qcc::String GetDefaultKeyStoreFileName(const char* application, const char* fname)
{
    qcc::String path = qcc::GetHomeDir() + "/.alljoyn_secure_keystore/";

    if (fname != nullptr) {
        path += fname;
    } else {
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
        status = qcc::DeleteFile(path);
        if (status != ER_OK) {
            QCC_LogError(status, ("DeleteFile(%s) failed", path.c_str()));
        }
    }

    return status;
}

DefaultKeyStoreListener::DefaultKeyStoreListener(const qcc::String& application, const char* fname) :
    m_fileLocker(GetDefaultKeyStoreFileName(application.c_str(), fname).c_str())
{
    /* Ensure that the shared keystore file exists. */
    FileLock readLock;
    /* 'readLock' is released when goes out of scope. */
    QStatus status = m_fileLocker.GetFileLockForRead(&readLock);
    if (status == ER_EOF) {
        /* The file does not exist. Create one here by simply acquiring Write lock. */
        status = m_fileLocker.AcquireWriteLock();
        if (status == ER_OK) {
            m_fileLocker.ReleaseWriteLock();
        } else {
            qcc::String fileName = GetDefaultKeyStoreFileName(application.c_str(), fname);
            QCC_LogError(status, ("FileLocker::AcquireWriteLock() failed, status=(%#x) - cannot write file (%s).",
                                  status, GetDefaultKeyStoreFileName(application.c_str(), fname).c_str()));
        }
    }
}

DefaultKeyStoreListener::~DefaultKeyStoreListener()
{
}

/**
 * The exclusive lock implemented by this function is machine-scoped.
 *  - For Windows, this means process_1 can lock process_2 keystore (if shared the same file). This
 *     allows the keystore to be shared safely across multiple processes.
 *  - Non-Windows change is tracked by ASACORE-2587.
 */
QStatus DefaultKeyStoreListener::AcquireExclusiveLock(const char* file, uint32_t line)
{
    QStatus status = KeyStoreListener::AcquireExclusiveLock(file, line);
    if (status != ER_OK) {
        QCC_LogError(status, ("KeyStoreListener::AcquireWriteLock failed, status=(%#x)", status));
        return status;
    }

    status = m_fileLocker.AcquireWriteLock();
    if (status != ER_OK) {
        KeyStoreListener::ReleaseExclusiveLock(file, line);
        QCC_LogError(status, ("DefaultKeyStoreListener::AcquireWriteLock failed, status=(%#x)", status));
        return status;
    }
    return ER_OK;
}

void DefaultKeyStoreListener::ReleaseExclusiveLock(const char* file, uint32_t line)
{
    QCC_ASSERT(m_fileLocker.HasWriteLock());
    m_fileLocker.ReleaseWriteLock();
    KeyStoreListener::ReleaseExclusiveLock(file, line);
}

QStatus DefaultKeyStoreListener::LoadRequest(KeyStore& keyStore)
{
    DATA_BLOB dataIn = { };
    DATA_BLOB dataOut = { };
    FileLock readLock;
    /* 'readLock' is released when goes out of scope. */
    QStatus status = m_fileLocker.GetFileLockForRead(&readLock);
    if (status != ER_OK) {
        QCC_LogError(status, ("FileLocker::GetFileLockForRead() failed, status=(%#x)", status));
        goto Exit;
    }

    /* Try to load the keystore... */
    FileSource* source = readLock.GetSource();
    QCC_ASSERT(source != nullptr);

    int64_t fileSize = 0LL;
    status = source->GetSize(fileSize);
    if (status != ER_OK) {
        QCC_LogError(status, ("GetSize() failed, status=(%#x)", status));
        goto Exit;
    }

    /**
     * The key store should never be more than 4 GB, check here to prevent an overflow when assigning
     * the filesize to a 32 bit DWORD.
     */
    if (fileSize >= (int64_t)UINT_MAX) {
        status = ER_BUS_CORRUPT_KEYSTORE;
        QCC_LogError(status, ("Keystore fileSize is too large, status=(%#x)", status));
        goto Exit;
    }

    if (fileSize == 0LL) {
        /* Load the empty keystore */
        keyStore.LoadFromEmptyFile(m_fileLocker.GetFileName());
    } else {
        dataIn.cbData = fileSize;
        dataIn.pbData = new uint8_t[dataIn.cbData];

        size_t pulled = 0;
        status = source->PullBytes(dataIn.pbData, dataIn.cbData, pulled);
        if (status != ER_OK) {
            QCC_LogError(status, ("FileSource::PullBytes() failed, status=(%#x)", status));
            goto Exit;
        }

        if ((int64_t)pulled != fileSize) {
            status = ER_BUS_CORRUPT_KEYSTORE;
            QCC_LogError(status, ("Pulled byte offset went beyond the file size."));
            goto Exit;
        }

        /**
         * CryptUnprotectData will return an invalid argument error if called with a 0 byte buffer. This happens after
         * AllJoyn creates the KeyStore file, but hasn't yet written any keys to the store. In this case, just skip
         * the CryptUnprotectData step and pass the empty buffer into the KeyStore.
         */
        if (!CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut)) {
            status = ER_BUS_CORRUPT_KEYSTORE;
            QCC_LogError(status, ("CryptUnprotectData reading keystore failed error=(%#x) status=(%#x)", ::GetLastError(), status));
            goto Exit;
        }

        StringSource bufferSource(dataOut.pbData, dataOut.cbData);
        status = keyStore.Pull(bufferSource, m_fileLocker.GetFileName());
        if (status != ER_OK) {
            QCC_LogError(status, ("KeyStore::Pull failed, status=(%#x)", status));
            goto Exit;
        }
    }

    QCC_DbgHLPrintf(("Read key store from %s, size %lld", m_fileLocker.GetFileName(), fileSize));

Exit:
    delete[] dataIn.pbData;
    if (dataOut.pbData != nullptr) {
        ClearMemory(dataOut.pbData, dataOut.cbData);
        LocalFree(dataOut.pbData);
    }
    return status;
}

QStatus DefaultKeyStoreListener::StoreRequest(KeyStore& keyStore)
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
        std::vector<uint8_t, SecureAllocator<uint8_t> > sbuf;    /**< storage for byte stream */
    };

    BufferSink buffer;
    DATA_BLOB dataIn = { };
    DATA_BLOB dataOut = { };
    QStatus status = ER_OK;

    /* 'writeLock' is released when goes out of scope. */
    FileLock writeLock;
    status = m_fileLocker.GetFileLockForWrite(&writeLock);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to store request - write lock has not been taken, status=(%#x)", status));
        QCC_ASSERT(!"write lock has not been taken");
        goto Exit;
    }

    status = keyStore.Push(buffer);
    if (status != ER_OK) {
        goto Exit;
    }

    size_t pushed = 0;
    dataIn.pbData = (BYTE*)buffer.GetBuffer().data();
    dataIn.cbData = buffer.GetBuffer().size();
    if (!CryptProtectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut)) {
        status = ER_BUS_CORRUPT_KEYSTORE;
        QCC_LogError(status, ("CryptProtectData writing keystore failed error=(%#x) status=(%#x)", ::GetLastError(), status));
        goto Exit;
    }

    status = writeLock.GetSink()->PushBytes(dataOut.pbData, dataOut.cbData, pushed);
    if (status != ER_OK) {
        QCC_LogError(status, ("FileSink::PushBytes failed status=(%#x)", status));
        goto Exit;
    }

    if (pushed != dataOut.cbData) {
        status = ER_BUS_CORRUPT_KEYSTORE;
        QCC_LogError(status, ("Bytes pushed is different than dataOut.cbData"));
        goto Exit;
    }

    if (!writeLock.GetSink()->Truncate()) {
        QCC_LogError(ER_WARNING, ("FileSink::Truncate failed"));
    }

    QCC_DbgHLPrintf(("Wrote key store to %s", m_fileLocker.GetFileName()));

Exit:
    LocalFree(dataOut.pbData);
    return status;
}

KeyStoreListener* KeyStoreListenerFactory::CreateInstance(const qcc::String& application, const char* fname)
{
    return new DefaultKeyStoreListener(application, fname);
}

} /* namespace ajn */
