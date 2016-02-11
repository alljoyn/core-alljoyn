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
