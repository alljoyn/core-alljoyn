/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#ifndef ALLJOYN_SECMGR_STORAGE_UISTORAGEIMPL_H_
#define ALLJOYN_SECMGR_STORAGE_UISTORAGEIMPL_H_

#include <qcc/Mutex.h>

#include <alljoyn/Status.h>

#include <alljoyn/securitymgr/storage/UIStorage.h>
#include <alljoyn/securitymgr/Application.h>
#include <alljoyn/securitymgr/GroupInfo.h>
#include <alljoyn/securitymgr/IdentityInfo.h>

#include "AJNCaStorage.h"
#include "SQLStorage.h"

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
class UIStorageImpl :
    public UIStorage, public StorageListenerHandler {
  public:

    UIStorageImpl(shared_ptr<AJNCaStorage>& _ca, shared_ptr<SQLStorage>& localStorage) : ca(_ca),
        storage(localStorage), updateCounter(0)
    {
    }

    QStatus RemoveApplication(Application& app);

    QStatus GetManagedApplications(vector<Application>& apps) const;

    QStatus GetManagedApplication(Application& app) const;

    QStatus StoreGroup(GroupInfo& groupInfo);

    QStatus RemoveGroup(const GroupInfo& groupInfo);

    QStatus GetGroup(GroupInfo& groupInfo) const;

    QStatus GetGroups(vector<GroupInfo>& groupsInfo) const;

    QStatus StoreIdentity(IdentityInfo& idInfo);

    QStatus RemoveIdentity(const IdentityInfo& idInfo);

    QStatus GetIdentity(IdentityInfo& idInfo) const;

    QStatus GetIdentities(vector<IdentityInfo>& idInfos) const;

    QStatus SetAppMetaData(const Application& app,
                           const ApplicationMetaData& appMetaData);

    QStatus GetAppMetaData(const Application& app,
                           ApplicationMetaData& appMetaData) const;

    void Reset();

    void RegisterStorageListener(StorageListener* listener);

    void UnRegisterStorageListener(StorageListener* listener);

    QStatus StartUpdates(Application& app,
                         uint64_t& updateID);

    QStatus UpdatesCompleted(Application& app,
                             uint64_t& updateID);

    virtual QStatus InstallMembership(const Application& app,
                                      const GroupInfo& groupInfo);

    virtual QStatus RemoveMembership(const Application& app,
                                     const GroupInfo& groupInfo);

    virtual QStatus UpdatePolicy(Application& app,
                                 PermissionPolicy& policy);

    virtual QStatus GetPolicy(const Application& app,
                              PermissionPolicy& policy);

    virtual QStatus RemovePolicy(Application& app);

    virtual QStatus UpdateIdentity(Application& app,
                                   const IdentityInfo& identityInfo,
                                   const Manifest& manifest);

    virtual QStatus GetManifest(const Application& app,
                                Manifest& manifest) const;

    virtual QStatus GetAdminGroup(GroupInfo& groupInfo) const;

    virtual QStatus GetCaStorage(shared_ptr<AgentCAStorage>& ref)
    {
        ref = ca;
        return ER_OK;
    }

  private:

    QStatus GetStoredGroupAndAppInfo(Application& app,
                                     GroupInfo& groupInfo);

    QStatus ApplicationUpdated(Application& app);

    QStatus ApplicationsUpdated(vector<Application>& app);

    void NotifyListeners(const Application& app,
                         bool completed = false);

    Mutex listenerLock;
    Mutex updateLock;
    vector<StorageListener*> listeners;
    shared_ptr<AJNCaStorage> ca;
    shared_ptr<SQLStorage> storage;
    uint64_t updateCounter;
};
}
}
#endif /* ALLJOYN_SECMGR_STORAGE_UISTORAGEIMPL_H_ */
