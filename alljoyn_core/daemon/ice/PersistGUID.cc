/**
 * @file
 * This file implements functions to read or write the Persistent GUID from
 * the file PersistentGUID in the System Home Directory
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
#include <qcc/FileStream.h>
#include <qcc/Debug.h>
#include <qcc/Util.h>

#include "PersistGUID.h"

#define QCC_MODULE "PERSIST_GUID"

using namespace std;
using namespace qcc;

namespace ajn {

/* Retrieve the Persistent GUID from the file PersistentGUID in the System Home Directory */
QStatus GetPersistentGUID(qcc::GUID128& guid)
{
    QStatus status = ER_OK;

    QCC_DbgPrintf(("GetPersistentGUID()"));

    /* Get the path to the system home directory */
    qcc::String homeDir = qcc::GetHomeDir();

    QCC_DbgPrintf(("GetPersistentGUID(): homeDir = %s", homeDir.c_str()));

    if (homeDir == String("/")) {
        status = ER_FAIL;
        QCC_LogError(status, ("GetPersistentGUID(): Unable to retrieve system home directory path"));
        return status;
    }

    qcc::String filePath = homeDir + GUIDFileName;

    /* Check if the PersistentGUID file exists in the system */
    FileSource source(filePath.c_str());
    if (!source.IsValid()) {
        status = ER_FAIL;
        QCC_LogError(status, ("GetPersistentGUID(): Failed to open %s", filePath.c_str()));
        return status;
    }

    /* Retrieve the GUID from the file */
    uint8_t guidBuf[qcc::GUID128::SIZE];
    size_t pulled;

    source.Lock(true);
    status = source.PullBytes(guidBuf, qcc::GUID128::SIZE, pulled);
    source.Unlock();

    if (status != ER_OK) {
        QCC_LogError(status, ("GetPersistentGUID(): Unable to read the GUID from %s", filePath.c_str()));
        return status;
    }

    guid.SetBytes(guidBuf);

    QCC_DbgPrintf(("GetPersistentGUID(): Successfully retrieved the GUID %s", guid.ToString().c_str()));

    return status;
}

/* Set the Persistent GUID in the file PersistentGUID in the System Home Directory */
QStatus SetPersistentGUID(qcc::GUID128 guid)
{
    QStatus status = ER_OK;

    QCC_DbgPrintf(("SetPersistentGUID()"));

    /* Get the path to the system home directory */
    qcc::String homeDir = qcc::GetHomeDir();

    QCC_DbgPrintf(("GetPersistentGUID(): homeDir = %s", homeDir.c_str()));

    if (homeDir == String("/")) {
        status = ER_FAIL;
        QCC_LogError(status, ("GetPersistentGUID(): Unable to retrieve system home directory path"));
        return status;
    }

    qcc::String filePath = homeDir + GUIDFileName;

    /* Access the PersistentGUID file in the system */
    FileSink sink(filePath.c_str(), FileSink::PRIVATE);
    if (!sink.IsValid()) {
        status = ER_FAIL;
        QCC_LogError(status, ("SetPersistentGUID(): Failed to open %s", filePath.c_str()));
        return status;
    }

    size_t pushed;
    sink.Lock(true);
    status = sink.PushBytes(guid.GetBytes(), qcc::GUID128::SIZE, pushed);
    sink.Unlock();

    if (status != ER_OK) {
        QCC_LogError(status, ("SetPersistentGUID(): Unable to write the GUID to %s", filePath.c_str()));
        return status;
    }

    QCC_DbgPrintf(("SetPersistentGUID(): Successfully stored the GUID %s", guid.ToString().c_str()));

    return status;
}

}
