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

#ifndef ALLJOYN_SECMGR_SECURITYAGENTIMPL_H_
#define ALLJOYN_SECMGR_SECURITYAGENTIMPL_H_

#include <memory>

#include <qcc/CryptoECC.h>
#include <qcc/Mutex.h>
#include <qcc/GUID.h>

#include <alljoyn/AboutListener.h>
#include <alljoyn/Status.h>
#include <alljoyn/PermissionPolicy.h>

#include <alljoyn/securitymgr/ApplicationListener.h>
#include <alljoyn/securitymgr/Application.h>
#include <alljoyn/securitymgr/GroupInfo.h>
#include <alljoyn/securitymgr/IdentityInfo.h>
#include <alljoyn/securitymgr/SecurityAgent.h>
#include <alljoyn/securitymgr/AgentCAStorage.h>
#include <alljoyn/securitymgr/ManifestUpdate.h>

#include "ApplicationMonitor.h"
#include "ProxyObjectManager.h"
#include "ApplicationUpdater.h"
#include "TaskQueue.h"

using namespace qcc;

namespace ajn {
namespace securitymgr {
class Identity;
class SecurityInfoListener;
class ApplicationUpdater;
struct SecurityInfo;

class AppListenerEvent {
  public:
    AppListenerEvent(const OnlineApplication* _oldInfo,
                     const OnlineApplication* _newInfo) :
        oldApp(_oldInfo), newApp(_newInfo), syncError(nullptr), manifestUpdate(nullptr)
    {
    }

    AppListenerEvent(const SyncError* _error) :
        oldApp(nullptr), newApp(nullptr), syncError(_error), manifestUpdate(nullptr)
    {
    }

    AppListenerEvent(const ManifestUpdate* _manifestUpdate) :
        oldApp(nullptr), newApp(nullptr), syncError(nullptr), manifestUpdate(_manifestUpdate)
    {
    }

    ~AppListenerEvent()
    {
        delete oldApp;
        oldApp = nullptr;
        delete newApp;
        newApp = nullptr;
        delete syncError;
        syncError = nullptr;
        delete manifestUpdate;
        manifestUpdate = nullptr;
    }

    const OnlineApplication* oldApp;
    const OnlineApplication* newApp;
    const SyncError* syncError;
    const ManifestUpdate* manifestUpdate;
};

/**
 * @brief the class provides for the SecurityManager implementation hiding
 */
class SecurityAgentImpl :
    public SecurityAgent,
    private SecurityInfoListener,
    private StorageListener {
  public:

    SecurityAgentImpl(const shared_ptr<AgentCAStorage>& _caStorage,
                      BusAttachment* ba = nullptr);

    ~SecurityAgentImpl();

    QStatus Init();

    QStatus Claim(const OnlineApplication& app,
                  const IdentityInfo& identityInfo);

    QStatus GetApplications(vector<OnlineApplication>& apps,
                            const PermissionConfigurator::ApplicationState applicationState =
                                PermissionConfigurator::CLAIMABLE) const;

    QStatus GetApplication(OnlineApplication& _application) const;

    QStatus SetSyncState(const OnlineApplication& app,
                         const ApplicationSyncState syncState);

    void UpdateApplications(const vector<OnlineApplication>* apps = nullptr);

    QStatus GetApplicationSecInfo(SecurityInfo& secInfo) const;

    const KeyInfoNISTP256& GetPublicKeyInfo() const;

    void NotifyApplicationListeners(const ManifestUpdate* manifestUpdate);

    void NotifyApplicationListeners(const SyncError* syncError);

    void SetClaimListener(ClaimListener* listener);

    void RegisterApplicationListener(ApplicationListener* al);

    void UnregisterApplicationListener(ApplicationListener* al);

    virtual void OnSecurityStateChange(const SecurityInfo* oldSecInfo,
                                       const SecurityInfo* newSecInfo);

    void HandleTask(AppListenerEvent* event);

  private:

    typedef map<KeyInfoNISTP256, OnlineApplication> OnlineApplicationMap;

    QStatus ClaimSelf();

    virtual void OnPendingChanges(vector<Application>& apps);

    virtual void OnPendingChangesCompleted(vector<Application>& apps);

    OnlineApplicationMap::iterator SafeAppExist(const KeyInfoNISTP256 key,
                                                bool& exist);

    void AddSecurityInfo(OnlineApplication& app,
                         const SecurityInfo& si);

    void RemoveSecurityInfo(OnlineApplication& app,
                            const SecurityInfo& si);

    void NotifyApplicationListeners(const OnlineApplication* oldApp,
                                    const OnlineApplication* newApp);

    // To prevent compilation warning on MSCV.
    SecurityAgentImpl& operator=(const SecurityAgentImpl& other);

  private:

    class PendingClaim {
      public:
        PendingClaim(const OnlineApplication& _app, vector<OnlineApplication>* _list, Mutex& _lock)
            : remove(false), app(_app), list(_list), lock(_lock)
        {
        }

        QStatus Init()
        {
            QStatus status = ER_BAD_ARG_1;
            lock.Lock();
            for (size_t i = 0; i < list->size(); i++) {
                if (app == (*list)[i]) {
                    goto out;
                }
            }
            remove = true;
            list->push_back(app);
            status = ER_OK;
        out:
            lock.Unlock();
            return status;
        }

        ~PendingClaim()
        {
            if (remove) {
                lock.Lock();
                vector<OnlineApplication>::iterator it = list->begin();
                for (; it != list->end(); it++) {
                    if (app == *it) {
                        list->erase(it);
                        break;
                    }
                }
                lock.Unlock();
            }
        }

      private:
        bool remove;
        OnlineApplication app;
        vector<OnlineApplication>* list;
        Mutex lock;
    };

    KeyInfoNISTP256 publicKeyInfo;
    OnlineApplicationMap applications;
    vector<ApplicationListener*> listeners;
    shared_ptr<ProxyObjectManager> proxyObjectManager;
    shared_ptr<ApplicationUpdater> applicationUpdater;
    shared_ptr<ApplicationMonitor> appMonitor;
    BusAttachment* busAttachment;
    const shared_ptr<AgentCAStorage>& caStorage;
    mutable Mutex appsMutex;
    mutable Mutex applicationListenersMutex;
    vector<OnlineApplication> pendingClaims;
    TaskQueue<AppListenerEvent*, SecurityAgentImpl> queue;
    ClaimListener* claimListener;
};
}
}

#endif /* ALLJOYN_SECMGR_SECURITYAGENTIMPL_H_ */
