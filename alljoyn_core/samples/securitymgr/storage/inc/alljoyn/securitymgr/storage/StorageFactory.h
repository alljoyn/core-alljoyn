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

#ifndef ALLJOYN_SECMGR_STORAGE_STORAGEFACTORY_H_
#define ALLJOYN_SECMGR_STORAGE_STORAGEFACTORY_H_

#include <memory>
#include <string>

#include <alljoyn/securitymgr/storage/UIStorage.h>
#include <alljoyn/securitymgr/AgentCAStorage.h>

using namespace std;

namespace ajn {
namespace securitymgr {
/**
 * @brief The StorageFactory is introduced because at run-time
 *        we are not sure which class (derived from Storage) we'll be using.
 *        Every implementation needs to provide their own implementation of this class.
 */
class StorageFactory {
  private:
    StorageFactory() { }

    StorageFactory(StorageFactory const&);
    StorageFactory& operator=(StorageFactory const&);

  public:
    /**
     * @brief Get a singleton instance of the storage factory.
     *
     * @return StorageFactory reference to the singleton storage factory.
     */
    static StorageFactory& GetInstance()
    {
        static StorageFactory sf;
        return sf;
    }

    QStatus GetStorage(const string caName,
                       shared_ptr<UIStorage>& storage);
};
}
}
#endif /* ALLJOYN_SECMGR_STORAGE_STORAGEFACTORY_H_ */