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

#ifndef ALLJOYN_SECMGR_STORAGE_SQLSTORAGE_H_
#define ALLJOYN_SECMGR_STORAGE_SQLSTORAGE_H_

#if defined (QCC_OS_GROUP_WINDOWS)
#include "sqlite3.h"
#else
#include <sqlite3.h>
#endif

#include <iostream>
#include <string>
#include <vector>

#include <qcc/CertificateECC.h>
#include <qcc/CryptoECC.h>
#include <qcc/Mutex.h>

#include <alljoyn/Status.h>

#include <alljoyn/securitymgr/Application.h>
#include <alljoyn/securitymgr/GroupInfo.h>
#include <alljoyn/securitymgr/IdentityInfo.h>
#include <alljoyn/securitymgr/Manifest.h>
#include <alljoyn/securitymgr/storage/ApplicationMetaData.h>
#include "SQLStorageConfig.h"

/**
 * @brief A class that is meant to implement the Storage abstract class in order to provide a persistent storage
 *        on a native Linux device.
 **/
#define INITIAL_SERIAL_NUMBER 1

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
/**
 * A vector of MembershipCertificates to emulate a chain of MembershipCertificates.
 * */
typedef vector<MembershipCertificate> MembershipCertificateChain;

enum InfoType {
    INFO_GROUP,
    INFO_IDENTITY
};

class SQLStorage {
  private:

    QStatus status;
    sqlite3* nativeStorageDB;
    SQLStorageConfig storageConfig;
    mutable Mutex storageMutex;

    QStatus Init();

    static QStatus ExportKeyInfo(const KeyInfoNISTP256& keyInfo,
                                 uint8_t** byteArray,
                                 size_t& byteArraySize);

    QStatus StoreInfo(InfoType type,
                      const KeyInfoNISTP256& authority,
                      const GUID128& guid,
                      const string& name,
                      const string& desc,
                      bool update);

    QStatus GetInfo(InfoType type,
                    const KeyInfoNISTP256& auth,
                    const GUID128& guid,
                    string& name,
                    string& desc) const;

    QStatus RemoveInfo(InfoType type,
                       const KeyInfoNISTP256& authority,
                       const GUID128& guid,
                       vector<Application>& appsToSync);

    QStatus BindCertForStorage(const Application& app,
                               CertificateX509& certificate,
                               const char* sqlStmtText,
                               sqlite3_stmt** statement);

    QStatus StepAndFinalizeSqlStmt(sqlite3_stmt* statement) const;

    QStatus InitSerialNumber();

    string GetStoragePath() const;

    QStatus PrepareMembershipCertificateQuery(const Application& app,
                                              const MembershipCertificate& certificate,
                                              sqlite3_stmt** statement) const;

    QStatus GetPolicyOrManifest(const Application& app,
                                const char* type,
                                uint8_t** byteArray,
                                size_t* size) const;

    QStatus StorePolicyOrManifest(const Application& app,
                                  const uint8_t* byteArray,
                                  const size_t size,
                                  const char* type);

    QStatus GetApplicationsPerGuid(const InfoType type,
                                   const GUID128& guid,
                                   vector<Application>& apps);

  public:

    SQLStorage(const SQLStorageConfig& _storageConfig) :
        status(ER_OK), nativeStorageDB(nullptr), storageConfig(_storageConfig)
    {
        status = Init();
    }

    QStatus GetStatus() const
    {
        return status;
    }

    QStatus StoreApplication(const Application& app,
                             const bool update = false,
                             const bool updatePolicy = false);

    QStatus SetAppMetaData(const Application& app,
                           const ApplicationMetaData& appMetaData);

    QStatus GetAppMetaData(const Application& app,
                           ApplicationMetaData& appMetaData) const;

    QStatus RemoveApplication(const Application& app);

    QStatus GetManagedApplications(vector<Application>& apps) const;

    QStatus GetManagedApplication(Application& app) const;

    QStatus GetManifest(const Application& app,
                        Manifest& manifest) const;

    QStatus GetPolicy(const Application& app,
                      PermissionPolicy& policy) const;

    QStatus StoreManifest(const Application& app,
                          const Manifest& manifest);

    QStatus StorePolicy(const Application& app,
                        const PermissionPolicy& policy);

    QStatus RemovePolicy(const Application& app);

    QStatus StoreCertificate(const Application& app,
                             CertificateX509& certificate,
                             bool update = false);

    QStatus RemoveCertificate(const Application& app,
                              CertificateX509& certificate);

    QStatus GetCertificate(const Application& app,
                           CertificateX509& certificate);

    QStatus GetMembershipCertificates(const Application& app,
                                      const MembershipCertificate& certificate,
                                      MembershipCertificateChain& certificates) const;

    QStatus StoreGroup(const GroupInfo& groupInfo);

    QStatus RemoveGroup(const GroupInfo& groupInfo,
                        vector<Application>& appsToSync);

    QStatus GetGroup(GroupInfo& groupInfo) const;

    QStatus GetGroups(vector<GroupInfo>& groupsInfo) const;

    QStatus StoreIdentity(const IdentityInfo& idInfo);

    QStatus RemoveIdentity(const IdentityInfo& idInfo,
                           vector<Application>& appsToSync);

    QStatus GetIdentity(IdentityInfo& idInfo) const;

    QStatus GetIdentities(vector<IdentityInfo>& idInfos) const;

    QStatus GetNewSerialNumber(CertificateX509& cert) const;

    void Reset();

    virtual ~SQLStorage();
};
}
}
#endif /* ALLJOYN_SECMGR_STORAGE_SQLSTORAGE_H_ */
