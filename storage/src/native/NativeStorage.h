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

#ifndef NATIVESTORAGE_H_
#define NATIVESTORAGE_H_

#include <sqlite3.h>

#include <Storage.h>
#include <StorageConfig.h>
#include <AppGuildInfo.h>
#include <alljoyn/Status.h>
#include <qcc/String.h>
#include <qcc/Certificate.h>
#include <qcc/CertificateECC.h>
#include <qcc/CryptoECC.h>

#include <iostream>

#define QCC_MODULE "SEC_MGR"

/**
 * \class NativeStorage
 * \brief A class that is meant to implement the Storage abstract class in order to provide a persistent storage
 *        on a native Linux device.
 **/
#define INITIAL_SERIAL_NUMBER 1

namespace ajn {
namespace securitymgr {
struct Keys {
    const qcc::ECCPublicKey* appECCPublicKey;
    qcc::String* guildID;
};

class NativeStorage :
    public Storage {
  private:

    sqlite3* nativeStorageDB;
    StorageConfig storageConfig;

    QStatus Init();

    QStatus BindCertForStorage(const qcc::Certificate& certificate,
                               const char* sqlStmtText,
                               sqlite3_stmt** statement);                                                                       //Could be generalized in the future for all cert types

    QStatus StepAndFinalizeSqlStmt(sqlite3_stmt* statement) const;

    int GetBlobSize(const char* table,
                    const char* columnName,
                    const Keys* keys) const; //It calculates the size of any blob in a given table.

    QStatus InitSerialNumber();

    qcc::String GetStoragePath() const;

  public:

    NativeStorage(const StorageConfig& _storageConfig) :
        storageConfig(_storageConfig)
    {
        status = Init();
    }

    QStatus StoreApplication(const ManagedApplicationInfo& managedApplicationInfo,
                             const bool update = false);

    QStatus RemoveApplication(const ManagedApplicationInfo& managedApplicationInfo);

    QStatus GetManagedApplications(std::vector<ManagedApplicationInfo>& managedApplications) const;

    QStatus GetManagedApplication(ManagedApplicationInfo& managedApplicationInfo) const;

    QStatus StoreCertificate(const qcc::Certificate& certificate,
                             bool update = false);

    QStatus StoreAssociatedData(const qcc::Certificate& certificate,
                                const qcc::String& data,
                                bool update = false);

    QStatus RemoveCertificate(qcc::Certificate& certificate);

    QStatus RemoveAssociatedData(const qcc::Certificate& certificate);

    QStatus GetCertificate(qcc::Certificate& certificate);

    QStatus GetAssociatedData(const qcc::Certificate& certificate,
                              qcc::String& data) const;

    QStatus StoreGuild(const GuildInfo& guildInfo,
                       const bool update = false);

    QStatus RemoveGuild(const GUID128& guildId);

    QStatus GetGuild(GuildInfo& guildInfo) const;

    QStatus GetManagedGuilds(std::vector<GuildInfo>& guildsInfo) const;

    QStatus GetNewSerialNumber(qcc::String& serialNumber) const;

    void Reset();

    virtual ~NativeStorage();
};
}
}
#endif /* NATIVESTORAGE_H_ */
