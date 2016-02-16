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

#ifndef ALLJOYN_SECMGR_STORAGE_AJNCASTORAGE_H_
#define ALLJOYN_SECMGR_STORAGE_AJNCASTORAGE_H_

#include <string>
#include <vector>
#include <memory>
#include <map>

#include <alljoyn/Status.h>

#include <alljoyn/securitymgr/AgentCAStorage.h>

#include "SQLStorage.h"
#include "AJNCa.h"

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
class StorageListenerHandler {
  public:
    virtual void RegisterStorageListener(StorageListener* listener) = 0;

    virtual void UnRegisterStorageListener(StorageListener* listener) = 0;

    virtual QStatus UpdatesCompleted(Application& app,
                                     uint64_t& updateID) = 0;

    virtual QStatus StartUpdates(Application& app,
                                 uint64_t& updateID) = 0;

    virtual QStatus ApplicationClaimed(Application& app,
                                       IdentityCertificate& cert,
                                       Manifest& mnf) = 0;

    virtual ~StorageListenerHandler() { }
};

class AJNCaStorage :
    public AgentCAStorage {
  public:
    AJNCaStorage() : ca(nullptr), sql(nullptr), handler(nullptr)
    {
    };

    ~AJNCaStorage()
    {
    }

    QStatus Init(const string storeName,
                 shared_ptr<SQLStorage>& sql);

    void Reset()
    {
        ca->Reset();
    }

    virtual QStatus GetManagedApplication(Application& app) const;

    virtual QStatus RegisterAgent(const KeyInfoNISTP256& agentKey,
                                  const Manifest& manifest,
                                  GroupInfo& adminGroup,
                                  IdentityCertificateChain& identityCertificates,
                                  vector<MembershipCertificateChain>& adminGroupMemberships);

    virtual QStatus FinishApplicationClaiming(const Application& app,
                                              QStatus status);

    virtual QStatus StartUpdates(Application& app,
                                 uint64_t& updateID);

    virtual QStatus UpdatesCompleted(Application& app,
                                     uint64_t& updateID);

    virtual QStatus StartApplicationClaiming(const Application& app,
                                             const IdentityInfo& idInfo,
                                             const Manifest& mf,
                                             GroupInfo& adminGroup,
                                             IdentityCertificateChain& idCert);

    virtual QStatus GetCaPublicKeyInfo(KeyInfoNISTP256& CAKeyInfo) const;

    virtual QStatus GetMembershipCertificates(const Application& app,
                                              vector<MembershipCertificateChain>& membershipCertificates) const;

    virtual QStatus GetIdentityCertificatesAndManifest(const Application& app,
                                                       IdentityCertificateChain& identityCertificates,
                                                       Manifest& mf) const;

    virtual void RegisterStorageListener(StorageListener* listener)
    {
        handler->RegisterStorageListener(listener);
    }

    virtual void UnRegisterStorageListener(StorageListener* listener)
    {
        handler->UnRegisterStorageListener(listener);
    }

    virtual void SetStorageListenerHandler(shared_ptr<StorageListenerHandler>& _handler)
    {
        handler = _handler;
    }

    QStatus GenerateMembershipCertificate(const Application& app,
                                          const GroupInfo& groupInfo,
                                          MembershipCertificate& memberShip);

    QStatus GenerateIdentityCertificate(const Application& app,
                                        const IdentityInfo& idInfo,
                                        const Manifest& mf,
                                        IdentityCertificate& idCertificate);

    virtual QStatus GetPolicy(const Application& app,
                              PermissionPolicy& policy) const;

    QStatus GetAdminGroup(GroupInfo& adminGroup) const;

  private:
    QStatus SignCertifcate(CertificateX509& certificate) const;

    unique_ptr<AJNCa> ca;
    shared_ptr<SQLStorage> sql;
    shared_ptr<StorageListenerHandler> handler;
    Mutex pendingLock;

    struct CachedData {
        IdentityCertificate cert;
        Manifest mnf;
    };

    map<Application, CachedData> claimPendingApps;
};
}
}

#endif /* ALLJOYN_SECMGR_STORAGE_AJNCASTORAGE_H_ */
