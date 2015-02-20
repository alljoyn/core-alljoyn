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

#ifndef SQLSTORAGE_H_
#define SQLSTORAGE_H_

#include "sqlite3.h"

#include <alljoyn/securitymgr/Storage.h>
#include <alljoyn/securitymgr/ManagedApplicationInfo.h>
#include <alljoyn/securitymgr/GuildInfo.h>
#include <alljoyn/securitymgr/IdentityInfo.h>
#include <alljoyn/securitymgr/cert/X509Certificate.h>

#include <alljoyn/Status.h>
#include <qcc/String.h>
#include <qcc/Certificate.h>
#include <qcc/CertificateECC.h>
#include <qcc/CryptoECC.h>
#include <qcc/Mutex.h>

#include <iostream>
#include "SQLStorageConfig.h"

/**
 * \class SQLStorage
 * \brief A class that is meant to implement the Storage abstract class in order to provide a persistent storage
 *        on a native Linux device.
 **/
#define INITIAL_SERIAL_NUMBER 1

using namespace qcc;
namespace ajn {
namespace securitymgr {
struct Keys {
    const qcc::ECCPublicKey* appECCPublicKey;
    qcc::String* guildID;
};

enum InfoType {
    INFO_GUILD,
    INFO_IDENTITY
};

class SQLStorage :
    public Storage {
  private:

    sqlite3* nativeStorageDB;
    SQLStorageConfig storageConfig;
    mutable qcc::Mutex storageMutex;

    QStatus Init();

    QStatus StoreInfo(InfoType type,
                      const qcc::ECCPublicKey& authority,
                      const GUID128& guid,
                      const qcc::String& name,
                      const qcc::String& desc,
                      bool update);

    QStatus GetInfo(InfoType type,
                    const qcc::ECCPublicKey& auth,
                    const GUID128& guid,
                    qcc::String& name,
                    qcc::String& desc) const;

    QStatus RemoveInfo(InfoType type,
                       const qcc::ECCPublicKey& authority,
                       const GUID128& guid);

    QStatus BindCertForStorage(const qcc::Certificate& certificate,
                               const char* sqlStmtText,
                               sqlite3_stmt** statement);                                                                       //Could be generalized in the future for all cert types

    QStatus StepAndFinalizeSqlStmt(sqlite3_stmt* statement) const;

    int GetBlobSize(const char* table,
                    const char* columnName,
                    const Keys* keys) const; //It calculates the size of any blob in a given table.

    QStatus InitSerialNumber();

    qcc::String GetStoragePath() const;

    QStatus PrepareCertificateQuery(const qcc::X509MemberShipCertificate& cert,
                                    sqlite3_stmt** statement) const;

    QStatus GetCertificateFromRow(sqlite3_stmt** statement,
                                  qcc::X509MemberShipCertificate& cert) const;

    QStatus GetCertificateFromRow(sqlite3_stmt** statement,
                                  qcc::X509CertificateECC& cert,
                                  const qcc::String tableName,
                                  Keys keys) const;

  public:

    SQLStorage(const SQLStorageConfig& _storageConfig) :
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

    QStatus GetCertificates(const qcc::X509MemberShipCertificate& certificate,
                            std::vector<qcc::X509MemberShipCertificate>& certificates) const;

    QStatus GetAssociatedData(const qcc::Certificate& certificate,
                              qcc::String& data) const;

    QStatus StoreGuild(const GuildInfo& guildInfo);

    QStatus RemoveGuild(const GuildInfo& guildInfo);

    QStatus GetGuild(GuildInfo& guildInfo) const;

    QStatus GetGuilds(std::vector<GuildInfo>& guildsInfo) const;

    QStatus StoreIdentity(const IdentityInfo& idInfo);

    QStatus RemoveIdentity(const IdentityInfo& idInfo);

    QStatus GetIdentity(IdentityInfo& idInfo) const;

    QStatus GetIdentities(std::vector<IdentityInfo>& idInfos) const;

    QStatus GetNewSerialNumber(qcc::String& serialNumber) const;

    void Reset();

    virtual ~SQLStorage();
};
}
}
#endif /* SQLSTORAGE_H_ */
