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

#include "ApplicationUpdater.h"
#include <alljoyn/securitymgr/ManifestUpdate.h>

#include <stdlib.h>
#include <string>

#include <qcc/Debug.h>
#define QCC_MODULE "SECMGR_UPDATER"

using namespace std;
using namespace qcc;
using namespace ajn;
using namespace ajn::securitymgr;

QStatus ApplicationUpdater::ResetApplication(const OnlineApplication& app)
{
    QStatus status = ER_FAIL;

    QCC_DbgPrintf(("Resetting application"));

    switch (app.applicationState) {
    case PermissionConfigurator::NOT_CLAIMABLE: // implicit fallthrough
    case PermissionConfigurator::CLAIMABLE:
        QCC_DbgPrintf(("Application was already reset"));
        status = ER_OK;
        break;

    case PermissionConfigurator::CLAIMED: // implicit fallthrough
    case PermissionConfigurator::NEED_UPDATE:
        status = proxyObjectManager->Reset(app);
        if (ER_OK != status) {
            SyncError* error = new SyncError(app, status, SYNC_ER_RESET);
            securityAgentImpl->NotifyApplicationListeners(error);
        }
        QCC_DbgPrintf(("Resetting application returned %s",
                       QCC_StatusText(status)));
        break;

    default:
        QCC_LogError(status, ("Unexpected ApplicationState"));
    }

    return status;
}

QStatus ApplicationUpdater::UpdatePolicy(const OnlineApplication& app)
{
    QCC_DbgPrintf(("Updating policy"));
    QStatus status = ER_FAIL;

    uint32_t remoteVersion;
    status = proxyObjectManager->GetPolicyVersion(app, remoteVersion);
    if (ER_OK != status) {
        QCC_DbgPrintf(("Failed to get remote policy version"));
        SyncError* error = new SyncError(app, status, SYNC_ER_REMOTE);
        securityAgentImpl->NotifyApplicationListeners(error);
        return status;
    }
    QCC_DbgPrintf(("Remote policy version is %i", remoteVersion));

    PermissionPolicy persistedPolicy;
    status = storage->GetPolicy(app, persistedPolicy);
    if (ER_OK != status && ER_END_OF_DATA != status) {
        QCC_LogError(status, ("Failed to retrieve local policy"));
        SyncError* error = new SyncError(app, status, SYNC_ER_STORAGE);
        securityAgentImpl->NotifyApplicationListeners(error);
        return status;
    }
    QCC_DbgPrintf(("GetPolicy from storage returned %i", status));

    if (ER_END_OF_DATA == status) {
        status = ER_OK;
        QCC_DbgPrintf(("No policy in local storage"));

        // hard coded to 0 as GetDefaultPolicy might fail (see ASACORE-2200)
        if (remoteVersion != 0) {
            status = proxyObjectManager->ResetPolicy(app);
            if (ER_OK != status) {
                QCC_DbgPrintf(("Failed to reset policy"));
                SyncError* error = new SyncError(app, status, SYNC_ER_REMOTE);
                securityAgentImpl->NotifyApplicationListeners(error);
                return status;
            }
            QCC_DbgPrintf(("Policy reset successfully"));
        } else {
            QCC_DbgPrintf(("Policy already on default"));
        }

        return status;
    }

    uint32_t localVersion = persistedPolicy.GetVersion();
    QCC_DbgPrintf(("Local policy version %i", localVersion));
    if (localVersion == remoteVersion) {
        QCC_DbgPrintf(("Policy already up to date"));
        return ER_OK;
    }

    status = proxyObjectManager->UpdatePolicy(app, persistedPolicy);
    QCC_DbgPrintf(("Installing new policy returned %i", status));
    if (ER_OK != status) {
        SyncError* error = new SyncError(app, status, persistedPolicy);
        securityAgentImpl->NotifyApplicationListeners(error);
    }

    return status;
}

QStatus ApplicationUpdater::InstallMissingMemberships(const OnlineApplication& app,
                                                      const vector<MembershipCertificateChain>& local,
                                                      const vector<MembershipSummary>& remote)
{
    QStatus status = ER_OK;

    vector<MembershipCertificateChain>::const_iterator localIt;
    vector<MembershipSummary>::const_iterator remoteIt;

    QCC_DbgPrintf(("Installing membership certificates"));

    for (localIt = local.begin(); localIt != local.end(); ++localIt) {
        bool install = true;
        const MembershipCertificate leaf = (*localIt)[0];

        for (remoteIt = remote.begin(); remoteIt != remote.end(); ++remoteIt) {
            if (IsSameCertificate(*remoteIt, leaf)) {
                install = false;
                break;
            }
        }

        if (install) {
            status = proxyObjectManager->InstallMembership(app, *localIt);
            QCC_DbgPrintf(("Installing membership certificate %s returned %i",
                           leaf.GetGuild().ToString().c_str(), status));
            if (ER_OK != status) {
                SyncError* error = new SyncError(app, status, (*localIt)[0]);
                securityAgentImpl->NotifyApplicationListeners(error);
                QCC_LogError(status, ("Failed to InstallMembership"));
                break;
            }
        }
    }

    return status;
}

QStatus ApplicationUpdater::RemoveRedundantMemberships(const OnlineApplication& app,
                                                       const vector<MembershipCertificateChain>& local,
                                                       const vector<MembershipSummary>& remote)
{
    QStatus status = ER_OK;

    vector<MembershipCertificateChain>::const_iterator localIt;
    vector<MembershipSummary>::const_iterator remoteIt;

    QCC_DbgPrintf(("Removing membership certificates"));

    for (remoteIt = remote.begin(); remoteIt != remote.end(); ++remoteIt) {
        string remoteSerial = remoteIt->serial;
        bool remove = true;

        for (localIt = local.begin(); localIt != local.end(); ++localIt) {
            if (IsSameCertificate(*remoteIt, (*localIt)[0])) {
                remove = false;
                break;
            }
        }

        if (remove) {
            status = proxyObjectManager->RemoveMembership(app, remoteSerial,
                                                          remoteIt->issuer);
            QCC_DbgPrintf(("Removing membership certificate %s returned %i",
                           remoteSerial.c_str(), status));
            if (ER_OK != status) {
                QCC_LogError(status, ("Failed to RemoveMembership"));
                SyncError* error = new SyncError(app, status, SYNC_ER_REMOTE);
                securityAgentImpl->NotifyApplicationListeners(error);
                break;
            }
        }
    }

    return status;
}

QStatus ApplicationUpdater::UpdateMemberships(const OnlineApplication& app)
{
    QCC_DbgPrintf(("Updating membership certificates"));

    QStatus status = ER_OK;

    vector<MembershipCertificateChain> local;
    if (ER_OK != (status = storage->GetMembershipCertificates(app, local))) {
        QCC_DbgPrintf(("Failed to GetMembershipCertificates"));
        SyncError* error = new SyncError(app, status, SYNC_ER_STORAGE);
        securityAgentImpl->NotifyApplicationListeners(error);
        return status;
    }
    QCC_DbgPrintf(("Found %i local membership certificates", local.size()));

    vector<MembershipSummary> remote;
    status = proxyObjectManager->GetMembershipSummaries(app, remote);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to GetMembershipSummaries"));
        SyncError* error = new SyncError(app, status, SYNC_ER_REMOTE);
        securityAgentImpl->NotifyApplicationListeners(error);
        return status;
    }
    QCC_DbgPrintf(("Retrieved %i membership summaries", remote.size()));

    status = InstallMissingMemberships(app, local, remote);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to install membership certificates"));
        return status;
    }

    status = RemoveRedundantMemberships(app, local, remote);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to remove membership certificates"));
        return status;
    }

    return status;
}

QStatus ApplicationUpdater::UpdateIdentity(const OnlineApplication& app)
{
    QCC_DbgPrintf(("Updating identity certificate"));

    QStatus status = ER_FAIL;

    IdentityCertificateChain remoteIdCertChain;
    IdentityCertificateChain persistedIdCerts;
    SyncError* error = nullptr;

    do {
        Manifest mf;

        if (ER_OK != (status = storage->GetIdentityCertificatesAndManifest(app, persistedIdCerts, mf))) {
            error = new SyncError(app, status, SYNC_ER_STORAGE);
            QCC_LogError(status, ("Could not get identity certificate from storage"));
            break;
        }

        if (ER_OK != (status = proxyObjectManager->GetIdentity(app, remoteIdCertChain))) {
            error = new SyncError(app, status, persistedIdCerts[0]);
            QCC_LogError(status, ("Could not fetch identity certificate"));
            break;
        }
        bool needUpdate = false;
        if (remoteIdCertChain.size() != persistedIdCerts.size()) {
            needUpdate = true;
        } else {
            for (size_t i = 0; i < persistedIdCerts.size(); i++) {
                if (remoteIdCertChain[i].GetSerialLen() != persistedIdCerts[i].GetSerialLen()) {
                    needUpdate = true;
                    break;
                }
                if (memcmp(persistedIdCerts[i].GetSerial(), remoteIdCertChain[i].GetSerial(),
                           remoteIdCertChain[i].GetSerialLen())
                    || ((remoteIdCertChain[i].GetAuthorityKeyId() != persistedIdCerts[i].GetAuthorityKeyId()))) {
                    needUpdate = true;
                    break;
                }
            }
        }
        if (needUpdate) {
            status = proxyObjectManager->UpdateIdentity(app, persistedIdCerts, mf);
            if (ER_OK != status) {
                error = new SyncError(app, status, persistedIdCerts[0]);
            }
        } else {
            QCC_DbgPrintf(("Identity certificate is already up to date"));
        }

        if (app.applicationState == PermissionConfigurator::NEED_UPDATE) {
            Manifest remoteManifest;
            if (ER_OK != (status = proxyObjectManager->GetManifestTemplate(app, remoteManifest))) {
                error = new SyncError(app, status, persistedIdCerts[0]);
                QCC_LogError(status, ("Could not fetch manifest template"));
                break;
            }

            ManifestUpdate* mfUpdate = new ManifestUpdate(app, mf, remoteManifest);
            securityAgentImpl->NotifyApplicationListeners(mfUpdate);
        }
    } while (0);

    if (nullptr != error) {
        securityAgentImpl->NotifyApplicationListeners(error);
    }

    return status;
}

QStatus ApplicationUpdater::UpdateApplication(const OnlineApplication& app,
                                              const SecurityInfo& secInfo)
{
    QStatus status = ER_FAIL;
    QCC_DbgPrintf(("Updating %s", secInfo.busName.c_str()));

    Application managedApp;
    managedApp.keyInfo = secInfo.keyInfo;
    uint64_t transactionID = 0;
    status = storage->StartUpdates(managedApp, transactionID);
    if (ER_OK != status) {
        QCC_DbgPrintf(("Failed to start transaction for %s (%s)",
                       secInfo.busName.c_str(),  QCC_StatusText(status)));
        if (ER_END_OF_DATA != status) {
            SyncError* error = new SyncError(app, status, SYNC_ER_STORAGE);
            securityAgentImpl->NotifyApplicationListeners(error);
        }
        return status;
    }
    QCC_DbgPrintf(("Started transaction %llu for %s", transactionID,
                   secInfo.busName.c_str()));

    do {
        switch (managedApp.syncState) {
        case SYNC_WILL_RESET:
            status = ResetApplication(app);
            if (ER_OK == status) {
                managedApp.syncState = SYNC_RESET;
            }
            break;

        default:
            do {
                if (ER_OK != (status = UpdatePolicy(app))) {
                    break;
                }
                if (ER_OK != (status = UpdateMemberships(app))) {
                    break;
                }
                if (ER_OK != (status = UpdateIdentity(app))) {
                    break;
                }
                managedApp.syncState = SYNC_OK;
            } while (0);
        }

        QCC_DbgPrintf(("Transaction %llu returned %s", transactionID,
                       QCC_StatusText(status)));

        uint64_t currentTransactionID = transactionID;
        QStatus storageStatus =
            storage->UpdatesCompleted(managedApp, transactionID);

        if ((ER_OK == storageStatus) && (currentTransactionID != transactionID)) {
            // restart updates
            QCC_DbgPrintf(("Restarted transaction %llu for %s", transactionID,
                           secInfo.busName.c_str()));
        } else {
            // we're done !!
            break;
        }
    } while (1);

    return status;
}

QStatus ApplicationUpdater::UpdateApplication(const SecurityInfo& secInfo)
{
    OnlineApplication app(secInfo.applicationState, secInfo.busName);
    app.keyInfo = secInfo.keyInfo;
    return UpdateApplication(app, secInfo);
}

QStatus ApplicationUpdater::UpdateApplication(const OnlineApplication& app)
{
    OnlineApplication tmp = app;
    QStatus status = securityAgentImpl->GetApplication(tmp);
    if (status != ER_OK) {
        return ER_OK;
    }
    status = securityAgentImpl->SetSyncState(tmp, SYNC_OK);
    SecurityInfo secInfo;
    secInfo.busName = app.busName;
    status = securityAgentImpl->GetApplicationSecInfo(secInfo);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to fetch security info !"));
        return status;
    }
    return UpdateApplication(app, secInfo);
}

void ApplicationUpdater::OnPendingChanges(vector<Application>& apps)
{
    QCC_DbgPrintf(("Changes needed from DB"));
    std::vector<Application>::iterator it = apps.begin();
    for (; it != apps.end(); it++) {
        OnlineApplication app;
        app.keyInfo = it->keyInfo;
        QStatus status = securityAgentImpl->GetApplication(app);
        if (status == ER_OK && app.busName.size() != 0) {
            SecurityInfo secInfo;
            secInfo.busName = app.busName;
            if (ER_OK == monitor->GetApplication(secInfo)) {
                QCC_DbgPrintf(("Added to queue ..."));
                queue.AddTask(new SecurityEvent(&secInfo, nullptr));
            }
        }
    }
}

void ApplicationUpdater::OnSecurityStateChange(const SecurityInfo* oldSecInfo,
                                               const SecurityInfo* newSecInfo)
{
    queue.AddTask(new SecurityEvent(newSecInfo, oldSecInfo));
}

void ApplicationUpdater::HandleTask(SecurityEvent* event)
{
    const SecurityInfo* oldSecInfo = event->oldInfo;
    const SecurityInfo* newSecInfo = event->newInfo;

    // new bus name
    if ((nullptr == oldSecInfo) && (nullptr != newSecInfo)) {
        QCC_DbgPrintf(("Detected new busName %s", newSecInfo->busName.c_str()));
        UpdateApplication(*newSecInfo);
    }

    // application changed to NEED_UPDATE
    if ((nullptr != oldSecInfo) && (nullptr != newSecInfo)
        && (oldSecInfo->applicationState != PermissionConfigurator::NEED_UPDATE)
        && (newSecInfo->applicationState == PermissionConfigurator::NEED_UPDATE)) {
        QCC_DbgPrintf(("Application %s changed to NEED_UPDATE", newSecInfo->busName.c_str()));
        UpdateApplication(*newSecInfo);
    }
}

bool ApplicationUpdater::IsSameCertificate(const MembershipSummary& summary, const MembershipCertificate& cert)
{
    if (summary.serial.size() != cert.GetSerialLen()) {
        return false;
    }
    if (memcmp(cert.GetSerial(), summary.serial.data(), cert.GetSerialLen()) != 0) {
        return false;
    }
    qcc::String aki = cert.GetAuthorityKeyId();
    if (summary.issuer.GetKeyIdLen() != aki.size()) {
        return false;
    }
    if (memcmp(summary.issuer.GetKeyId(), aki.data(), aki.size()) != 0) {
        return false;
    }
    return true;
}

#undef QCC_MODULE
