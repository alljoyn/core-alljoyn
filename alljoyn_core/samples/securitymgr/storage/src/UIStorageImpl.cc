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

#include "UIStorageImpl.h"

#include <alljoyn/PermissionPolicyUtil.h>
#define QCC_MODULE "SECMGR_STORAGE"

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
QStatus UIStorageImpl::ResetApplication(Application& app)
{
    updateLock.Lock();
    app.syncState = SYNC_WILL_RESET;
    QStatus status = storage->StoreApplication(app, true);
    updateCounter++;
    updateLock.Unlock();
    if (status == ER_OK) {
        NotifyListeners(app);
    }
    return status;
}

QStatus UIStorageImpl::RemoveApplication(Application& app)
{
    QStatus status = storage->RemoveApplication(app);
    NotifyListeners(app, APPLICATIONS_REMOVED);
    return status;
}

QStatus UIStorageImpl::GetManagedApplications(vector<Application>& apps) const
{
    return storage->GetManagedApplications(apps);
}

QStatus UIStorageImpl::GetManagedApplication(Application& app) const
{
    return storage->GetManagedApplication(app);
}

QStatus UIStorageImpl::StoreGroup(GroupInfo& groupInfo)
{
    if (groupInfo.authority.empty()) {
        ca->GetCaPublicKeyInfo(groupInfo.authority);
    }
    return storage->StoreGroup(groupInfo);
}

QStatus UIStorageImpl::RemoveGroup(const GroupInfo& groupInfo)
{
    vector<Application> appsToSync;
    QStatus status;
    GroupInfo tmpGroupInfo = groupInfo;
    if (groupInfo.authority.empty()) {
        if (ER_OK != (status = ca->GetCaPublicKeyInfo(tmpGroupInfo.authority))) {
            return status;
        }
    }
    status = storage->RemoveGroup(tmpGroupInfo, appsToSync);

    if (ER_OK != status || appsToSync.empty()) {
        return status;
    }

    return ApplicationsUpdated(appsToSync);
}

QStatus UIStorageImpl::GetGroup(GroupInfo& groupInfo) const
{
    if (groupInfo.authority.empty()) {
        ca->GetCaPublicKeyInfo(groupInfo.authority);
    }
    return storage->GetGroup(groupInfo);
}

QStatus UIStorageImpl::GetGroups(vector<GroupInfo>& groupsInfo) const
{
    return storage->GetGroups(groupsInfo);
}

QStatus UIStorageImpl::StoreIdentity(IdentityInfo& idInfo)
{
    if (idInfo.authority.empty()) {
        ca->GetCaPublicKeyInfo(idInfo.authority);
    }
    return storage->StoreIdentity(idInfo);
}

QStatus UIStorageImpl::RemoveIdentity(const IdentityInfo& idInfo)
{
    QStatus status;
    vector<Application> appsToSync;
    IdentityInfo tmpInfo = idInfo;
    if (idInfo.authority.empty()) {
        if (ER_OK != (status = ca->GetCaPublicKeyInfo(tmpInfo.authority))) {
            return status;
        }
    }
    status = storage->RemoveIdentity(tmpInfo, appsToSync);
    if (ER_OK != status || appsToSync.empty()) {
        return status;
    }

    vector<Application>::iterator appItr = appsToSync.begin();
    for (; appItr != appsToSync.end(); appItr++) {
        status = ResetApplication(*appItr);
        if (ER_OK != status) {
            return status;
        }
    }
    return status;
}

QStatus UIStorageImpl::GetIdentity(IdentityInfo& idInfo) const
{
    if (idInfo.authority.empty()) {
        ca->GetCaPublicKeyInfo(idInfo.authority);
    }
    return storage->GetIdentity(idInfo);
}

QStatus UIStorageImpl::GetIdentities(vector<IdentityInfo>& idInfos) const
{
    return storage->GetIdentities(idInfos);
}

QStatus UIStorageImpl::SetAppMetaData(const Application& app, const ApplicationMetaData& appMetaData)
{
    return storage->SetAppMetaData(app, appMetaData);
}

QStatus UIStorageImpl::GetAppMetaData(const Application& app, ApplicationMetaData& appMetaData) const
{
    return storage->GetAppMetaData(app, appMetaData);
}

void UIStorageImpl::Reset()
{
    storage->Reset();
    NotifyListeners(STORAGE_RESET);
}

QStatus UIStorageImpl::StartUpdates(Application& app, uint64_t& updateID)
{
    updateLock.Lock();
    QStatus status = storage->GetManagedApplication(app);
    updateID = updateCounter;
    updateLock.Unlock();
    return status;
}

QStatus UIStorageImpl::UpdatesCompleted(Application& app, uint64_t& updateID)
{
    QStatus status = ER_FAIL;
    Application managedApp;

    updateLock.Lock();

    switch (app.syncState) {
    case SYNC_RESET: // application was successfully reset by agent
        status = storage->RemoveApplication(app);
        updateLock.Unlock();
        NotifyListeners(app, APPLICATIONS_REMOVED);
        return status;

    case SYNC_OK: // application was successfully updated by agent
        // get latest application from storage
        managedApp.keyInfo = app.keyInfo;
        status = storage->GetManagedApplication(managedApp);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to GetManagedApplication"));
            break;
        }

        // retrigger updates if needed
        if (updateID != updateCounter) {
            app = managedApp;
            updateID = updateCounter;
            status = ER_OK;
            break;
        }

        // persist new state
        managedApp.syncState = app.syncState;
        status = storage->StoreApplication(managedApp, true);
        updateLock.Unlock();
        NotifyListeners(managedApp, PENDING_CHANGES_COMPLETED);
        return status;

    default:
        break;
    }

    updateLock.Unlock();
    return status;
}

QStatus UIStorageImpl::ApplicationClaimed(Application& app, IdentityCertificate& cert, Manifest& mnf)
{
    QStatus status =  storage->StoreApplication(app);
    if (ER_OK != status) {
        QCC_LogError(status, ("StoreApplication failed"));
        return status;
    }

    status = storage->StoreCertificate(app, cert);
    if (ER_OK != status) {
        QCC_LogError(status, ("StoreCertificate failed"));
    } else {
        status = storage->StoreManifest(app, mnf);
        if (ER_OK != status) {
            QCC_LogError(status, ("StoreManifest failed"));
        }
    }

    if (ER_OK != status) {
        storage->RemoveApplication(app);
        return status;
    }

    NotifyListeners(app, APPLICATIONS_ADDED);
    return status;
}

void UIStorageImpl::RegisterStorageListener(StorageListener* listener)
{
    listenerLock.Lock();
    if (listener) {
        listeners.push_back(listener);
    }
    listenerLock.Unlock();
}

void UIStorageImpl::UnRegisterStorageListener(StorageListener* listener)
{
    listenerLock.Lock();
    if (listener) {
        vector<StorageListener*>::iterator it = listeners.begin();
        for (; it != listeners.end(); it++) {
            if (*it == listener) {
                listeners.erase(it);
                break;
            }
        }
    }
    listenerLock.Unlock();
}

void UIStorageImpl::NotifyListeners(const StorageEvent event)
{
    vector<Application> empty;
    NotifyListeners(empty, event);
}

void UIStorageImpl::NotifyListeners(const Application& app, const StorageEvent event)
{
    vector<Application> apps;
    apps.push_back(app);
    NotifyListeners(apps, event);
}

void UIStorageImpl::NotifyListeners(vector<Application>& apps, const StorageEvent event)
{
    listenerLock.Lock();
    for (size_t i = 0; i < listeners.size(); ++i) {
        switch (event) {
        case PENDING_CHANGES:
            listeners[i]->OnPendingChanges(apps);
            break;

        case PENDING_CHANGES_COMPLETED:
            listeners[i]->OnPendingChangesCompleted(apps);
            break;

        case APPLICATIONS_ADDED:
            listeners[i]->OnApplicationsAdded(apps);
            break;

        case APPLICATIONS_REMOVED:
            listeners[i]->OnApplicationsRemoved(apps);
            break;

        case STORAGE_RESET:
            listeners[i]->OnStorageReset();
            break;

        default:
            break;
        }
    }
    listenerLock.Unlock();
}

QStatus UIStorageImpl::GetStoredGroupAndAppInfo(Application& app, GroupInfo& groupInfo)
{
    QStatus status = storage->GetGroup(groupInfo);
    if (ER_OK != status) {
        return status;
    }
    return storage->GetManagedApplication(app);
}

QStatus UIStorageImpl::InstallMembership(const Application& app, const GroupInfo& groupInfo)
{
    GroupInfo storedGroup(groupInfo);
    Application storedApp(app);
    QStatus status = GetStoredGroupAndAppInfo(storedApp, storedGroup);
    if (ER_OK != status) {
        return status;
    }
    MembershipCertificate certificate;
    status = ca->GenerateMembershipCertificate(storedApp, storedGroup, certificate);
    if (ER_OK != status) {
        return status;
    }
    status = storage->StoreCertificate(app, certificate);
    if (ER_OK != status) {
        return status;
    }

    return ApplicationUpdated(storedApp);
}

QStatus UIStorageImpl::RemoveMembership(const Application& app, const GroupInfo& groupInfo)
{
    GroupInfo storedGroup(groupInfo);
    Application storedApp(app);
    QStatus status = GetStoredGroupAndAppInfo(storedApp, storedGroup);
    if (ER_OK != status) {
        return status;
    }
    MembershipCertificate cert;
    cert.SetGuild(storedGroup.guid);
    cert.SetSubjectPublicKey(storedApp.keyInfo.GetPublicKey());
    status = storage->GetCertificate(app, cert);
    if (ER_OK != status) {
        return status;
    }
    status = storage->RemoveCertificate(app, cert);
    if (ER_OK != status) {
        return status;
    }
    return ApplicationUpdated(storedApp);
}

QStatus UIStorageImpl::UpdatePolicy(Application& app, PermissionPolicy& policy)
{
    QStatus status = ER_FAIL;

    if (!PermissionPolicyUtil::HasValidDenyRules(policy)) {
        status = ER_FAIL;
        QCC_LogError(status, ("Policy contains invalid deny rules"));
        return status;
    }

    status = GetManagedApplication(app);
    if (ER_OK != status) {
        return status;
    }

    PermissionPolicy local;
    status = storage->GetPolicy(app, local);
    if (ER_OK != status && ER_END_OF_DATA != status) {
        return status;
    }

    if (policy.GetVersion() == 0) {
        policy.SetVersion(local.GetVersion() + 1);
    } else if (local.GetVersion() >= policy.GetVersion()) {
        status = ER_POLICY_NOT_NEWER;
        QCC_LogError(status, ("Provided policy is not newer"));
        return status;
    }

    status = storage->StorePolicy(app, policy);
    if (ER_OK != status) {
        return status;
    }

    return ApplicationUpdated(app, false);
}

QStatus UIStorageImpl::GetPolicy(const Application& app, PermissionPolicy& policy)
{
    return storage->GetPolicy(app, policy);
}

QStatus UIStorageImpl::RemovePolicy(Application& app)
{
    QStatus status = storage->RemovePolicy(app);
    if (ER_OK != status) {
        return status;
    }
    return ApplicationUpdated(app, false);
}

QStatus UIStorageImpl::UpdateIdentity(Application& app,
                                      const IdentityInfo& identityInfo,
                                      const Manifest& manifest)
{
    QStatus status = storage->GetManagedApplication(app);
    if (ER_OK != status) {
        return status;
    }

    IdentityCertificate cert;
    status = ca->GenerateIdentityCertificate(app, identityInfo, manifest, cert);
    if (ER_OK != status) {
        return status;
    }

    status = storage->StoreCertificate(app, cert, true);
    if (ER_OK != status) {
        QCC_LogError(status, ("StoreCertificate failed"));
        return status;
    } else {
        status = storage->StoreManifest(app, manifest);
        if (ER_OK != status) {
            QCC_LogError(status, ("StoreManifest failed"));
            return status;
        }
    }

    return ApplicationUpdated(app);
}

QStatus UIStorageImpl::ApplicationUpdated(Application& app, bool policyUpdateNeeded)
{
    updateLock.Lock();
    QStatus status = storage->GetManagedApplication(app);
    if (status == ER_OK) {
        updateCounter++;

        switch (app.syncState) {
        case SYNC_OK:
            app.syncState = SYNC_PENDING;
            status = storage->StoreApplication(app, true, policyUpdateNeeded);
            updateLock.Unlock();
            NotifyListeners(app);
            return status;

        case SYNC_WILL_RESET: // implicit fallthrough
        case SYNC_PENDING:
            updateLock.Unlock();
            NotifyListeners(app);
            return status;

        default:
            break;
        }
    }
    updateLock.Unlock();
    return status;
}

QStatus UIStorageImpl::ApplicationsUpdated(vector<Application>& appsToSync)
{
    QStatus status = ER_OK;
    vector<Application>::iterator appItr = appsToSync.begin();
    for (; appItr != appsToSync.end(); appItr++) {
        if (ER_OK != (status = ApplicationUpdated(*appItr))) {
            return status;
        }
    }

    return status;
}

QStatus UIStorageImpl::GetManifest(const Application& app, Manifest& manifest) const
{
    return storage->GetManifest(app, manifest);
}

QStatus UIStorageImpl::GetAdminGroup(GroupInfo& groupInfo) const
{
    return ca->GetAdminGroup(groupInfo);
}
}
}
#undef QCC_MODULE
