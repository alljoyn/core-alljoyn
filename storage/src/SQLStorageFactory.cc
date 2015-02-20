/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/securitymgr/sqlstorage/SQLStorageFactory.h>
#include <SQLStorage.h>
#include <qcc/Environ.h>
#include <qcc/Debug.h>
#include "SQLStorageConfig.h"

#ifndef QCC_MODULE
#define QCC_MODULE "SEC_MGR"
#endif

#define HOME_KEY "HOME"

namespace ajn {
namespace securitymgr {
Storage* SQLStorageFactory::GetStorage()
{
    Storage* storage;
    SQLStorageConfig storageConfig;
    qcc::String homePath = qcc::Environ::GetAppEnviron()->Find(HOME_KEY);
    qcc::String storageFilePath = qcc::Environ::GetAppEnviron()->Find(STORAGE_FILEPATH_KEY);

    if (!storageFilePath.empty()) {
        storageConfig.settings[STORAGE_FILEPATH_KEY] = storageFilePath;
    } else if (homePath.empty()) {
        return NULL;
    } else {
        storageConfig.settings[STORAGE_FILEPATH_KEY] = homePath + "/" + DEFAULT_STORAGE_FILENAME;
    }

    QCC_DbgPrintf(("Storage will be placed in (%s)", storageConfig.settings[STORAGE_FILEPATH_KEY].c_str()));

    if (!(storage = new SQLStorage(storageConfig))) {
        return NULL;
    }

    if (storage->GetStatus() == ER_OK) {
        return storage;
    }

    delete storage;
    return NULL;
}
}
}
#undef QCC_MODULE
