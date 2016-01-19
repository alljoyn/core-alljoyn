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

    DefaultKeyStoreListener(const qcc::String& application, const char* fname) {
        if (fname) {
            fileName = GetHomeDir() + "/.alljoyn_secure_keystore/" + fname;
        } else {
            fileName = GetHomeDir() + "/.alljoyn_secure_keystore/" + application;
        }
    }

    QStatus LoadRequest(KeyStore& keyStore) {
        QStatus status;
        /* Try to load the keystore */
        {
            FileSource source(fileName);
            DATA_BLOB dataIn = { 0 };
            DATA_BLOB dataOut = { 0 };

            if (source.IsValid()) {
                source.Lock(true);

                int64_t fileSize = 0;
                size_t pulled = 0;
                status = source.GetSize(fileSize);

                /**
                 * The key store should never be more than 4 GB, check here to prevent an overflow when assigning
                 * the filesize to a 32 bit DWORD.
                 */
                if ((status == ER_OK) && (fileSize >= (int64_t)UINT_MAX)) {
                    status = ER_BUS_CORRUPT_KEYSTORE;
                }

                if (status == ER_OK) {
                    dataIn.cbData = fileSize;
                    dataIn.pbData = new uint8_t[dataIn.cbData];
                    if (dataIn.pbData == NULL) {
                        status = ER_OUT_OF_MEMORY;
                    }
                }

                if (status == ER_OK) {
                    status = source.PullBytes(dataIn.pbData, dataIn.cbData, pulled);
                }

                if ((status == ER_OK) && ((int64_t)pulled != fileSize)) {
                    status = ER_BUS_CORRUPT_KEYSTORE;
                }

                /**
                 * CryptUnprotectData will return an invalid argument error if called with a 0 byte buffer. This happens after
                 * AllJoyn creates the KeyStore file, but hasn't yet written any keys to the store. In this case, just skip
                 * the CryptUnprotectData step and pass an empty buffer into the KeyStore.
                 */
                if (status == ER_OK) {
                    if (fileSize > 0) {
                        if (CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut)) {
                            StringSource bufferSource(dataOut.pbData, dataOut.cbData);
                            status = keyStore.Pull(bufferSource, fileName);
                        } else {
                            status = ER_BUS_CORRUPT_KEYSTORE;
                            QCC_LogError(status, ("CryptUnprotectData reading keystore failed error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
                        }
                    } else {
                        String empty;
                        StringSource bufferSource(empty);
                        status = keyStore.Pull(bufferSource, fileName);
                    }
                }

                if (status == ER_OK) {
                    QCC_DbgHLPrintf(("Read key store from %s", fileName.c_str()));
                }
                source.Unlock();

                delete[] dataIn.pbData;

                if (dataOut.pbData != NULL) {
                    ClearMemory(dataOut.pbData, dataOut.cbData);
                    LocalFree(dataOut.pbData);
                }

                return status;
            }
        }
        /* Create an empty keystore */
        {
            FileSink sink(fileName, FileSink::PRIVATE);
            if (!sink.IsValid()) {
                status = ER_BUS_WRITE_ERROR;
                QCC_LogError(status, ("Cannot initialize key store %s", fileName.c_str()));
                return status;
            }
        }
        /* Load the empty keystore */
        {
            FileSource source(fileName);
            if (source.IsValid()) {
                source.Lock(true);
                status = keyStore.Pull(source, fileName);
                if (status == ER_OK) {
                    QCC_DbgHLPrintf(("Initialized key store %s", fileName.c_str()));
                } else {
                    QCC_LogError(status, ("Failed to initialize key store %s", fileName.c_str()));
                }
                source.Unlock();
            } else {
                status = ER_BUS_READ_ERROR;
            }
            return status;
        }
    }

    QStatus StoreRequest(KeyStore& keyStore) {

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
            void SecureClear() { sbuf.clear(); sbuf.shrink_to_fit(); }

          private:
            std::vector<uint8_t, SecureAllocator<uint8_t> > sbuf;    /**< storage for byte stream */
        };


        QStatus status;
        FileSink sink(fileName, FileSink::PRIVATE);
        BufferSink buffer;
        DATA_BLOB dataIn = { 0 };
        DATA_BLOB dataOut = { 0 };
        size_t pushed = 0;

        if (sink.IsValid()) {
            sink.Lock(true);
            status = keyStore.Push(buffer);

            if (status == ER_OK) {
                dataIn.pbData = (BYTE*)buffer.GetBuffer().data();
                dataIn.cbData = buffer.GetBuffer().size();
                if (!CryptProtectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut)) {
                    status = ER_BUS_CORRUPT_KEYSTORE;
                    QCC_LogError(status, ("CryptProtectData writing keystore failed error=(0x%08X) status=(0x%08X)", ::GetLastError(), status));
                }
            }

            if (status == ER_OK) {
                status = sink.PushBytes(dataOut.pbData, dataOut.cbData, pushed);
            }

            if (status == ER_OK && pushed != dataOut.cbData) {
                status = ER_BUS_CORRUPT_KEYSTORE;
            }

            if (status == ER_OK) {
                QCC_DbgHLPrintf(("Wrote key store to %s", fileName.c_str()));
            }
            sink.Unlock();
        } else {
            status = ER_BUS_WRITE_ERROR;
            QCC_LogError(status, ("Cannot write key store to %s", fileName.c_str()));
        }

        buffer.SecureClear();
        LocalFree(dataOut.pbData);

        return status;
    }

  private:

    qcc::String fileName;

};

KeyStoreListener* KeyStoreListenerFactory::CreateInstance(const qcc::String& application, const char* fname)
{
    return new DefaultKeyStoreListener(application, fname);
}

}
