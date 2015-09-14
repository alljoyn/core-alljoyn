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

#ifndef ALLJOYN_SECMGR_AGENTSTORAGEWRAPPER_H_
#define ALLJOYN_SECMGR_AGENTSTORAGEWRAPPER_H_

class AgentStorageWrapper :
    public AgentCAStorage {
  public:
    AgentStorageWrapper(shared_ptr<AgentCAStorage>& _ca) :
        ca(_ca) { }

    virtual ~AgentStorageWrapper() { }

    virtual QStatus GetManagedApplication(Application& app) const
    {
        return ca->GetManagedApplication(app);
    }

    virtual QStatus RegisterAgent(const KeyInfoNISTP256& agentKey,
                                  const Manifest& manifest,
                                  GroupInfo& adminGroup,
                                  IdentityCertificateChain& identityCertificates,
                                  vector<MembershipCertificateChain>& adminGroupMemberships)
    {
        return ca->RegisterAgent(agentKey, manifest, adminGroup, identityCertificates, adminGroupMemberships);
    }

    virtual QStatus StartApplicationClaiming(const Application& app,
                                             const IdentityInfo& idInfo,
                                             const Manifest& manifest,
                                             GroupInfo& adminGroup,
                                             IdentityCertificateChain& idCert)
    {
        return ca->StartApplicationClaiming(app, idInfo, manifest, adminGroup, idCert);
    }

    virtual QStatus FinishApplicationClaiming(const Application& app,
                                              QStatus status)
    {
        return ca->FinishApplicationClaiming(app, status);
    }

    virtual QStatus UpdatesCompleted(Application& app, uint64_t& updateID)
    {
        return ca->UpdatesCompleted(app, updateID);
    }

    virtual QStatus StartUpdates(Application& app, uint64_t& updateID)
    {
        return ca->StartUpdates(app, updateID);
    }

    virtual QStatus GetCaPublicKeyInfo(KeyInfoNISTP256& keyInfoOfCA) const
    {
        return ca->GetCaPublicKeyInfo(keyInfoOfCA);
    }

    virtual QStatus GetAdminGroup(GroupInfo& groupInfo) const
    {
        return ca->GetAdminGroup(groupInfo);
    }

    virtual QStatus GetMembershipCertificates(const Application& app,
                                              vector<MembershipCertificateChain>& membershipCertificates) const
    {
        return ca->GetMembershipCertificates(app, membershipCertificates);
    }

    virtual QStatus GetIdentityCertificatesAndManifest(const Application& app,
                                                       IdentityCertificateChain& identityCertificates,
                                                       Manifest& manifest) const
    {
        return ca->GetIdentityCertificatesAndManifest(app, identityCertificates, manifest);
    }

    virtual QStatus GetPolicy(const Application& app, PermissionPolicy& policy) const
    {
        return ca->GetPolicy(app, policy);
    }

    virtual void RegisterStorageListener(StorageListener* listener)
    {
        return ca->RegisterStorageListener(listener);
    }

    virtual void UnRegisterStorageListener(StorageListener* listener)
    {
        return ca->UnRegisterStorageListener(listener);
    }

  protected:
    shared_ptr<AgentCAStorage> ca;
};

class FailingStorageWrapper :
    public AgentStorageWrapper {
  public:
    FailingStorageWrapper(shared_ptr<AgentCAStorage>& _ca,
                          shared_ptr<UIStorage>& _storage) :
        AgentStorageWrapper(_ca),
        failOnUpdatesCompleted(false), failOnFinishApplicationClaiming(false),
        storage(_storage)
    {
    }

    QStatus FinishApplicationClaiming(const Application& app,  QStatus status)
    {
        if (failOnFinishApplicationClaiming) {
            return ER_FAIL;
        }
        return ca->FinishApplicationClaiming(app, status);
    }

    QStatus UpdatesCompleted(Application& app, uint64_t& updateID)
    {
        if (failOnUpdatesCompleted) {
            return ER_FAIL;
        }

        return ca->UpdatesCompleted(app, updateID);
    }

  public:
    bool failOnUpdatesCompleted;
    bool failOnFinishApplicationClaiming;

  private:
    FailingStorageWrapper& operator=(const FailingStorageWrapper);

    shared_ptr<UIStorage>& storage;
};
#endif /* ALLJOYN_SECMGR_AGENTSTORAGEWRAPPER_H_ */
