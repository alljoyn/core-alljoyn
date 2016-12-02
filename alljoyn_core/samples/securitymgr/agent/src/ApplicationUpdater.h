/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#ifndef ALLJOYN_SECMGR_APPLICATIONUPDATER_H_
#define ALLJOYN_SECMGR_APPLICATIONUPDATER_H_

#include <qcc/GUID.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Status.h>

#include <alljoyn/securitymgr/Application.h>
#include <alljoyn/securitymgr/AgentCAStorage.h>
#include <memory>

#include "ProxyObjectManager.h"
#include "SecurityInfoListener.h"
#include "TaskQueue.h"
#include "SecurityAgentImpl.h"

namespace ajn {
namespace securitymgr {
class SecurityEvent {
  public:
    SecurityInfo* newInfo;
    SecurityInfo* oldInfo;
    SecurityEvent(const SecurityInfo* n,
                  const SecurityInfo* o) :
        newInfo(n == nullptr ? nullptr : new SecurityInfo(*n)),
        oldInfo(o == nullptr ? nullptr : new SecurityInfo(*o))
    {
    }

    ~SecurityEvent()
    {
        delete newInfo;
        newInfo = nullptr;
        delete oldInfo;
        oldInfo = nullptr;
    }
};

class SecurityAgentImpl; //needed because of cyclic dependency between Agent and Updater.

class ApplicationUpdater :
    public SecurityInfoListener,
    public StorageListener {
  public:
    ApplicationUpdater(BusAttachment* ba, // No ownership.
                       const shared_ptr<AgentCAStorage>& s,
                       shared_ptr<ProxyObjectManager>& _pom,
                       shared_ptr<ApplicationMonitor>& _monitor,
                       SecurityAgentImpl* smi // No ownership.
                       ) :
        busAttachment(ba), storage(s), proxyObjectManager(_pom),
        monitor(_monitor), securityAgentImpl(smi),
        queue(this)
    {
        monitor->RegisterSecurityInfoListener(this);
        storage->RegisterStorageListener(this);
    }

    ~ApplicationUpdater()
    {
        storage->UnRegisterStorageListener(this);
        monitor->UnregisterSecurityInfoListener(this);
        queue.Stop();
    }

    QStatus UpdateApplication(const OnlineApplication& app);

    QStatus UpdateApplication(const SecurityInfo& secInfo);

    void HandleTask(SecurityEvent* event);

  private:
    static bool IsSameCertificate(const MembershipSummary& summary,
                                  const MembershipCertificate& cert);

    QStatus ResetApplication(const OnlineApplication& app);

    QStatus UpdateApplication(const OnlineApplication& app,
                              const SecurityInfo& secInfo);

    QStatus UpdatePolicy(ProxyObjectManager::ManagedProxyObject& app,
                         const PermissionPolicy* localPolicy);

    QStatus UpdateMemberships(ProxyObjectManager::ManagedProxyObject& mngdProxy,
                              const vector<MembershipCertificateChain>& local);

    QStatus UpdateIdentity(ProxyObjectManager::ManagedProxyObject& mngdProxy,
                           const IdentityCertificateChain& persistedIdCerts,
                           const Manifest& mf);

    QStatus InstallMissingMemberships(ProxyObjectManager::ManagedProxyObject& mngdProxy,
                                      const vector<MembershipCertificateChain>& local,
                                      const vector<MembershipSummary>& remote);

    QStatus RemoveRedundantMemberships(ProxyObjectManager::ManagedProxyObject& mngdProxy,
                                       const vector<MembershipCertificateChain>& local,
                                       const vector<MembershipSummary>& remote);

    virtual void OnSecurityStateChange(const SecurityInfo* oldSecInfo,
                                       const SecurityInfo* newSecInfo);

    virtual void OnPendingChanges(vector<Application>& apps);

    virtual void OnPendingChangesCompleted(vector<Application>& apps)
    {
        QCC_UNUSED(apps);
    }

  private:
    BusAttachment* busAttachment;
    shared_ptr<AgentCAStorage> storage;
    shared_ptr<ProxyObjectManager> proxyObjectManager;
    shared_ptr<ApplicationMonitor> monitor;
    SecurityAgentImpl* securityAgentImpl;

    TaskQueue<SecurityEvent*, ApplicationUpdater> queue;

    void NotifyAboutSyncError(const OnlineApplication& app, QStatus errorStatus, SyncErrorType errorType);
};
}
}

#endif /* ALLJOYN_SECMGR_APPLICATIONUPDATER_H_ */