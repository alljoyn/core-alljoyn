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
#include <ClaimedApplicationInfo.h>
#include <alljoyn/Status.h>
#include <qcc/String.h>
#include <qcc/Certificate.h>
#include <qcc/CertificateECC.h>
#include <qcc/CryptoECC.h>
#include <StorageConfig.h>

#include <iostream>

#define QCC_MODULE "SEC_MGR"

/**
 * \class NativeStorage
 * \brief A class that is meant to implement the Storage abstract class in order to provide a persistent storage
 *        on a native Linux device.
 **/

namespace ajn {
namespace securitymgr {
class NativeStorage :
    public Storage {
  private:

    sqlite3* nativeStorageDB;
    StorageConfig storageConfig;

    QStatus Init();

    CertificateType GetCertificateType(const qcc::Certificate& certificate);

    QStatus BindIdentityCertForStorage(const qcc::Certificate& certificate,
                                       const char* sqlStmtText,
                                       sqlite3_stmt** statement);                                                               //Could be generalized in the future for all cert types

    QStatus ExecuteGenericSqlStmt(sqlite3_stmt* statement); //Cannot be passed as const

    qcc::String GetStoragePath() const;

  public:

    NativeStorage(const StorageConfig& _storageConfig) :
        storageConfig(_storageConfig)
    {
        status = Init();
    }

    QStatus TrackClaimedApplication(const ClaimedApplicationInfo& claimedApplicationInfo);

    QStatus UnTrackClaimedApplication(const ClaimedApplicationInfo& claimedApplicationInfo);

    QStatus GetClaimedApplications(std::vector<ClaimedApplicationInfo>* claimedApplications);

    QStatus ClaimedApplication(const qcc::ECCPublicKey& appECCPublicKey,
                               bool* claimed);

    QStatus StoreCertificate(const qcc::ECCPublicKey& appECCPublicKey,
                             const qcc::Certificate& certificate);

    QStatus RemoveCertificate(const qcc::ECCPublicKey& appECCPublicKey,
                              const CertificateType certificateType);

    qcc::Certificate GetCertificate(const qcc::ECCPublicKey& appECCPublicKey,
                                    const CertificateType certificateType);

    void Reset();

    virtual ~NativeStorage();

  protected:
};
}
}
#endif /* NATIVESTORAGE_H_ */
