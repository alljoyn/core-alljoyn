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

#ifndef ALLJOYN_SECMGR_STORAGE_APPLICATIONMETADATA_H_
#define ALLJOYN_SECMGR_STORAGE_APPLICATIONMETADATA_H_

using namespace std;

/*
 * @brief ApplicationMetaData should include extra information that could be fetched from About.
 */
struct ApplicationMetaData {
    string userDefinedName;
    string deviceName;
    string appName;

    bool operator==(const ApplicationMetaData& rhs) const
    {
        if (userDefinedName != rhs.userDefinedName) {
            return false;
        }
        if (deviceName != rhs.deviceName) {
            return false;
        }
        if (appName != rhs.appName) {
            return false;
        }

        return true;
    }
};

#endif /* ALLJOYN_SECMGR_STORAGE_APPLICATIONMETADATA_H_ */