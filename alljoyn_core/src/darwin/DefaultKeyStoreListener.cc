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

#include "KeyStore.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {

DefaultKeyStoreListener::DefaultKeyStoreListener(const qcc::String& application, const char* fname)
{
    m_fileName = CalculateStoreFilePath(application, fname);
}

QStatus DefaultKeyStoreListener::DeleteKeyStoreFile(const qcc::String& application, const char* fname)
{
    QStatus status = ER_OK;
    qcc::String path = CalculateStoreFilePath(application, fname);

    if (qcc::FileExists(path) == ER_OK) {
        status = qcc::DeleteFile(path);
        if (status != ER_OK) {
            QCC_LogError(status, ("DeleteFile(%s) failed", path.c_str()));
        }
    }

    return status;
}

qcc::String DefaultKeyStoreListener::CalculateStoreFilePath(const qcc::String& application, const char* fname)
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

QStatus DefaultKeyStoreListener::LoadRequest(KeyStore& keyStore)
{
    QStatus status;
    /* Try to load the keystore */
    {
        FileSource source(m_fileName);
        if (source.IsValid()) {
            source.Lock(true);
            status = keyStore.Pull(source, m_fileName);
            if (status == ER_OK) {
                QCC_DbgHLPrintf(("Read key store from %s", m_fileName.c_str()));
            }
            source.Unlock();
            return status;
        }
    }
    /* Create an empty keystore */
    {
        FileSink sink(m_fileName, FileSink::PRIVATE);
        if (!sink.IsValid()) {
            status = ER_BUS_WRITE_ERROR;
            QCC_LogError(status, ("Cannot initialize key store %s", m_fileName.c_str()));
            return status;
        }
    }
    /* Load the empty keystore */
    {
        FileSource source(m_fileName);
        if (source.IsValid()) {
            source.Lock(true);
            status = keyStore.Pull(source, m_fileName);
            if (status == ER_OK) {
                QCC_DbgHLPrintf(("Initialized key store %s", m_fileName.c_str()));
            } else {
                QCC_LogError(status, ("Failed to initialize key store %s", m_fileName.c_str()));
            }
            source.Unlock();
        } else {
            status = ER_BUS_READ_ERROR;
        }
        return status;
    }
}

QStatus DefaultKeyStoreListener::StoreRequest(KeyStore& keyStore)
{
    QStatus status;
    FileSink sink(m_fileName, FileSink::PRIVATE);
    if (sink.IsValid()) {
        sink.Lock(true);
        status = keyStore.Push(sink);
        if (status == ER_OK) {
            QCC_DbgHLPrintf(("Wrote key store to %s", m_fileName.c_str()));
        }
        sink.Unlock();
    } else {
        status = ER_BUS_WRITE_ERROR;
        QCC_LogError(status, ("Cannot write key store to %s", m_fileName.c_str()));
    }
    return status;
}

}
