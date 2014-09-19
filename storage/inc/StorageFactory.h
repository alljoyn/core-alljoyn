/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#ifndef STORAGEFACTORY_H_
#define STORAGEFACTORY_H_

/**
 * \class StorageFactory
 * \brief We need a StorageFactory because at run-time we are not sure which class (derived from Storage) we'll be using
 *        Every implementation needs to provided their own implementation of this class.
 *
 */

//#define QCC_MODULE "SEC_MGR"
#include <Storage.h>
#include <StorageConfig.h>

namespace ajn {
namespace securitymgr {
class StorageFactory {
  private:
    StorageFactory() { }

    void operator=(StorageFactory const&);

  public:
    static StorageFactory& GetInstance()
    {
        static StorageFactory sf;
        return sf;
    }

    Storage* GetStorage(const StorageConfig& storageConfig);
};
}
}
#endif
