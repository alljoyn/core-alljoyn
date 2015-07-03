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
    status = proxyObjectManager->Reset(app);

    // TODO: Re-enable after completing AS-1535
    // if (ER_OK != status) {
    //    SyncError* error = new SyncError(app, status, SYNC_ER_RESET);
    //    securityAgentImpl->NotifyApplicationListeners(error);
    // }

    QCC_DbgPrintf(("Resetting application returned %s", QCC_StatusText(status)));

    return status;
}

QStatus ApplicationUpdater::UpdatePolicy(const OnlineApplication& app)
{
    QCC_DbgPrintf(("Updating policy"));
    QStatus status = ER_FAIL;

    PermissionPolicy remotePolicy;
    status = proxyObjectManager->GetPolicy(app, remotePolicy);
    if (ER_OK != status) {
        // errors logged in ProxyObjectManager.
        return status;
    }
    uint32_t remoteVersion = remotePolicy.GetVersion();
    QCC_DbgPrintf(("Remote policy version %i", remoteVersion));

    PermissionPolicy localPolicy;
    status = storage->GetPolicy(app, localPolicy);
    if (ER_OK != status && ER_END_OF_DATA != status) {
        QCC_LogError(status, ("Failed to retrieve local policy"));
        SyncError* error = new SyncError(app, status, SYNC_ER_STORAGE);
        securityAgentImpl->NotifyApplicationListeners(error);
        return status;
    }
    uint32_t localVersion = localPolicy.GetVersion();
    QCC_DbgPrintf(("Local policy version %i", localVersion));

    if (localVersion == remoteVersion) {
        QCC_DbgPrintf(("Policy already up to date"));
        return ER_OK;
    }

    status = proxyObjectManager->UpdatePolicy(app, localPolicy);
    QCC_DbgPrintf(("Installing new policy returned %i", status));

    if (ER_OK != status) {
        SyncError* error = new SyncError(app, status, localPolicy);
        securityAgentImpl->NotifyApplicationListeners(error);
    }

    return status;
}

QStatus ApplicationUpdater::InstallMissingMemberships(const OnlineApplication& app,
                                                      const vector<MembershipCertificate>& local,
                                                      const vector<MembershipSummary>& remote)
{
    QStatus status = ER_OK;

    vector<MembershipCertificate>::const_iterator localIt;
    vector<MembershipSummary>::const_iterator remoteIt;

    QCC_DbgPrintf(("Installing membership certificates"));

    for (localIt = local.begin(); localIt != local.end(); ++localIt) {
        string localSerial = string((const char*)localIt->GetSerial(), localIt->GetSerialLen());
        bool install = true;

        for (remoteIt = remote.begin(); remoteIt != remote.end(); ++remoteIt) {
            if (remoteIt->serial == localSerial) {
                install = false;
                break;
            }
        }

        if (install) {
            status = proxyObjectManager->InstallMembership(app, &(*localIt), 1);
            QCC_DbgPrintf(("Installing membership certificate %s returned %i",
                           localSerial.c_str(), status));
            if (ER_OK != status) {
                SyncError* error = new SyncError(app, status, *localIt);
                securityAgentImpl->NotifyApplicationListeners(error);
                QCC_LogError(status, ("Failed to InstallMembership"));
                break;
            }
        }
    }

    return status;
}

QStatus ApplicationUpdater::RemoveRedundantMemberships(const OnlineApplication& app,
                                                       const vector<MembershipCertificate>& local,
                                                       const vector<MembershipSummary>& remote)
{
    QStatus status = ER_OK;

    vector<MembershipCertificate>::const_iterator localIt;
    vector<MembershipSummary>::const_iterator remoteIt;

    QCC_DbgPrintf(("Removing membership certificates"));

    for (remoteIt = remote.begin(); remoteIt != remote.end(); ++remoteIt) {
        string remoteSerial = remoteIt->serial;
        bool remove = true;

        for (localIt = local.begin(); localIt != local.end(); ++localIt) {
            if (string((const char*)localIt->GetSerial(), localIt->GetSerialLen()) == remoteSerial) {
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

    vector<MembershipCertificate> local;
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

    IdentityCertificate remoteIdCert;
    IdentityCertificateChain persistedIdCerts;
    SyncError* error = nullptr;

    do {
        Manifest mf;

        if (ER_OK != (status = storage->GetIdentityCertificatesAndManifest(app, persistedIdCerts, mf))) {
            error = new SyncError(app, status, SYNC_ER_STORAGE);
            QCC_LogError(status, ("Could not get identity certificate from storage"));
            break;
        }

        if (ER_OK != (status = proxyObjectManager->GetIdentity(app, &remoteIdCert))) {
            error = new SyncError(app, status, persistedIdCerts[0]);
            QCC_LogError(status, ("Could not fetch identity certificate"));
            break;
        }

        if (remoteIdCert.GetSerialLen() == persistedIdCerts[0].GetSerialLen()) {
            if (memcmp(persistedIdCerts[0].GetSerial(), remoteIdCert.GetSerial(), remoteIdCert.GetSerialLen()) == 0) {
                QCC_DbgPrintf(("Identity certificate is already up to date"));
                break;
            }
        }
        status = proxyObjectManager->UpdateIdentity(app, &(persistedIdCerts[0]), 1, mf);
        if (ER_OK != status) {
            error = new SyncError(app, status, persistedIdCerts[0]);
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
    Application application;
    application.keyInfo = secInfo.keyInfo;
    uint64_t transactionID = 0;
    status = storage->StartUpdates(application, transactionID);
    if (status != ER_OK && status != ER_END_OF_DATA) {
        return status;
    }
    QCC_DbgPrintf(("StartUpdates returned %s -  transaction = %llu", QCC_StatusText(status), transactionID));
    do {
        if (ER_END_OF_DATA == status) {
            status = ResetApplication(app);
        } else {
            do {
                if (ER_OK != (status = UpdatePolicy(app))) {
                    break;
                }
                if (ER_OK != (status = UpdateMemberships(app))) {
                    break;
                }
                status = UpdateIdentity(app);
            } while (0);
        }

        QCC_DbgPrintf(("Updates completed %s returned %s ", secInfo.busName.c_str(), QCC_StatusText(status)));
        if (ER_OK == status) {
            uint64_t currentTransaction = transactionID;
            status = storage->UpdatesCompleted(application, transactionID);
            if (currentTransaction != transactionID) {
                //We fetch the application set status to ER_END_OF_DATA if the application would need reset.
                status = storage->GetManagedApplication(application);
                if (status != ER_OK && status != ER_END_OF_DATA) {
                    QCC_LogError(status, ("GetManagedApplication Failed. Abort update."));
                    break;
                }
            } else {
                //update successful.
                break;
            }
        } else {
            QCC_LogError(status, ("Failed to update application %s. Aborting update", secInfo.busName.c_str()));
            return status;
        }
    } while (1);
    QCC_DbgPrintf(("Updating %s returned %s ", secInfo.busName.c_str(), QCC_StatusText(status)));

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
    status = securityAgentImpl->SetUpdatesPending(tmp, true);
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

    if ((nullptr == oldSecInfo) && (nullptr != newSecInfo)) {
        // New security info.
        QCC_DbgPrintf(("Detected new busName %s", newSecInfo->busName.c_str()));
        UpdateApplication(*newSecInfo);
    }
}

#undef QCC_MODULE
