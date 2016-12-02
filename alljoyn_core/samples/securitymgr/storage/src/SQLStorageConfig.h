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

#ifndef ALLJOYN_SECMGR_STORAGE_SQLSTORAGECONFIG_H_
#define ALLJOYN_SECMGR_STORAGE_SQLSTORAGECONFIG_H_

#include <map>
#include <string>

#define DEFAULT_STORAGE_FILENAME "secmgrstorage.db"
#define STORAGE_FILEPATH_KEY "STORAGE_PATH"

using namespace std;

namespace ajn {
namespace securitymgr {
struct SQLStorageConfig {
    map<string, string> settings;
};
}
}
#endif /* ALLJOYN_SECMGR_STORAGE_SQLSTORAGECONFIG_H_ */