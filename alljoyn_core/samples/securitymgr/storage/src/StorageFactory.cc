/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include <qcc/Environ.h>
#include <qcc/Debug.h>

#include <alljoyn/securitymgr/storage/StorageFactory.h>

#include "SQLStorage.h"
#include "SQLStorageConfig.h"
#include "AJNCaStorage.h"
#include "UIStorageImpl.h"

#define QCC_MODULE "SECMGR_STORAGE"

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
static void GetHomePath(string& homePath)
{
#if !defined(QCC_OS_GROUP_WINDOWS)
    homePath = Environ::GetAppEnviron()->Find("HOME").c_str();
#else
    // Same path is returned by qcc::GetHomeDir() too.
    homePath = Environ::GetAppEnviron()->Find("LOCALAPPDATA").c_str();
    if (homePath.empty()) {
        homePath = Environ::GetAppEnviron()->Find("USERPROFILE").c_str();
    }
#endif
}

static void GetStorageFilePath(string& storageFilePath)
{
    storageFilePath = Environ::GetAppEnviron()->Find(STORAGE_FILEPATH_KEY).c_str();
}

static SQLStorage* GetSQLStorage()
{
    SQLStorage* storage;
    SQLStorageConfig storageConfig;
    string homePath;
    GetHomePath(homePath);
    string storageFilePath;
    GetStorageFilePath(storageFilePath);

    if (!storageFilePath.empty()) {
        storageConfig.settings[STORAGE_FILEPATH_KEY] = storageFilePath;
    } else if (homePath.empty()) {
        return nullptr;
    } else {
        storageConfig.settings[STORAGE_FILEPATH_KEY] = homePath + "/" + DEFAULT_STORAGE_FILENAME;
    }

    QCC_DbgPrintf(("Storage will be placed in (%s)", storageConfig.settings[STORAGE_FILEPATH_KEY].c_str()));

    storage = new SQLStorage(storageConfig);
    if (!storage) {
        return nullptr;
    }

    if (storage->GetStatus() == ER_OK) {
        return storage;
    }

    delete storage;
    storage = nullptr;
    return nullptr;
}

QStatus StorageFactory::GetStorage(const string caName, shared_ptr<UIStorage>& storage)
{
    SQLStorage* s = GetSQLStorage();
    if (s == nullptr) {
        return ER_FAIL;
    }
    shared_ptr<SQLStorage> localStorage(s);
    shared_ptr<AJNCaStorage> ca = make_shared<AJNCaStorage>();
    if (ca != nullptr) {
        QStatus status = ca->Init(caName, localStorage);
        if (status != ER_OK) {
            return status;
        }
        storage.reset();
        shared_ptr<UIStorageImpl> wr = shared_ptr<UIStorageImpl>(new UIStorageImpl(ca, localStorage));
        storage = wr;
        shared_ptr<StorageListenerHandler> sh = wr;
        ca->SetStorageListenerHandler(sh);
    }
    return ER_OK;
}
}
}
#undef QCC_MODULE