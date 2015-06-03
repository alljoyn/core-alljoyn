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

#include <qcc/Debug.h>
#define QCC_MODULE "SEC_MGR"

using namespace ajn;
using namespace securitymgr;

QStatus ApplicationUpdater::ResetApplication(const ApplicationInfo& appInfo,
                                             const SecurityInfo& secInfo)
{
    QCC_UNUSED(secInfo);

    QStatus status = ER_FAIL;

    QCC_DbgPrintf(("Resetting application"));
    status = applicationManager->Reset(appInfo);
    if (ER_OK != status) {
        SyncError* error = new SyncError(appInfo, status, SYNC_ER_RESET);
        securityManagerImpl->NotifyApplicationListeners(error);
    }
    QCC_DbgPrintf(("Resetting application returned %s", QCC_StatusText(status)));

    return status;
}

QStatus ApplicationUpdater::UpdatePolicy(const ApplicationInfo& appInfo,
                                         const SecurityInfo& secInfo,
                                         const ManagedApplicationInfo& mgdAppInfo)
{
    QCC_DbgPrintf(("Updating policy"));
    QStatus status = ER_FAIL;

    uint32_t remoteSerialNum = secInfo.policySerialNum;
    QCC_DbgPrintf(("Remote policy number %i", remoteSerialNum));

    PermissionPolicy localPolicy;
    uint32_t localSerialNum = 0;
    if (!mgdAppInfo.policy.empty()) {
        Message tmpMsg(*busAttachment);
        DefaultPolicyMarshaller marshaller(tmpMsg);
        if (ER_OK !=
            (status =
                 localPolicy.Import(marshaller, (const uint8_t*)mgdAppInfo.policy.data(), mgdAppInfo.policy.size()))) {
            SyncError* error = new SyncError(appInfo, status, SYNC_ER_STORAGE);
            securityManagerImpl->NotifyApplicationListeners(error);
            return status;
        }
        localSerialNum = localPolicy.GetSerialNum();
    }
    QCC_DbgPrintf(("Local policy number %i", localSerialNum));

    if (localSerialNum == remoteSerialNum) {
        QCC_DbgPrintf(("Policy already up to date"));
        return ER_OK;
    } else {
        status = applicationManager->InstallPolicy(appInfo, localPolicy);
        QCC_DbgPrintf(("Installing new policy returned %i", status));
        SyncError* error = new SyncError(appInfo, status, localPolicy);
        securityManagerImpl->NotifyApplicationListeners(error);
        return status;
    }
}

QStatus ApplicationUpdater::UpdateMembershipCertificates(const ApplicationInfo& appInfo,
                                                         const SecurityInfo& secInfo,
                                                         const ManagedApplicationInfo& mgdAppInfo)
{
    QCC_DbgPrintf(("Updating membership certificates"));

    QStatus status = ER_OK;

    qcc::ECCPublicKey eccAppPubKey(secInfo.publicKey);

    qcc::MembershipCertificate queryCert;
    queryCert.SetSubjectPublicKey(&eccAppPubKey);
    vector<qcc::MembershipCertificate> localCerts;

    if (ER_OK != (status = storage->GetCertificates(queryCert, localCerts))) {
        SyncError* error = new SyncError(appInfo, status, SYNC_ER_STORAGE);
        securityManagerImpl->NotifyApplicationListeners(error);
    }

    QCC_DbgPrintf(("Found %i local membership certificates", localCerts.size()));
    std::vector<qcc::MembershipCertificate>::iterator it;
    for (it = localCerts.begin(); it != localCerts.end(); ++it) {
        QCC_DbgPrintf(("Local membership certificate %s", it->GetSerial().c_str()));

        it->SetSubjectCN((const uint8_t*)mgdAppInfo.peerID.data(), mgdAppInfo.peerID.size());
        certificateGenerator->GenerateMembershipCertificate(*it);

        status = applicationManager->InstallMembership(appInfo, *it);
        QCC_DbgPrintf(("Install membership certificate %s returned %i", it->GetSerial().c_str(), status));

        if (ER_DUPLICATE_CERTIFICATE == status) {
            status = ER_OK;
        }

        if (ER_OK != status) {
            SyncError* error = new SyncError(appInfo, status, *it);
            securityManagerImpl->NotifyApplicationListeners(error);
            break;
        }
    }

    return status;
}

QStatus ApplicationUpdater::UpdateIdentityCert(const ApplicationInfo& appInfo,
                                               const SecurityInfo& secInfo,
                                               const ManagedApplicationInfo& mgdAppInfo)
{
    QCC_DbgPrintf(("Updating identity certificate"));

    QStatus status = ER_FAIL;

    qcc::IdentityCertificate remoteIdCert;
    qcc::IdentityCertificate persistedIdCert;
    SyncError* error = NULL;

    do {
        persistedIdCert.SetSubjectPublicKey(&(secInfo.publicKey));

        if (ER_OK != (status = storage->GetCertificate(persistedIdCert))) {
            error = new SyncError(appInfo, status, SYNC_ER_STORAGE);
            QCC_LogError(status, ("Could not get identity certificate from storage"));
            break;
        }

        uint32_t localSerialNum = strtoul(persistedIdCert.GetSerial().c_str(), NULL, 0);
        QCC_DbgPrintf(("Local identity certificate serial number is %u", localSerialNum));

        if (ER_OK != (status = applicationManager->GetIdentity(appInfo, remoteIdCert))) {
            error = new SyncError(appInfo, status, persistedIdCert);
            QCC_LogError(status, ("Could not fetch identity certificate"));
            break;
        }

        uint32_t remoteSerialNum = strtoul(remoteIdCert.GetSerial().c_str(), NULL, 0);
        QCC_DbgPrintf(("Remote identity certificate serial number is %u", remoteSerialNum));

        if (localSerialNum == remoteSerialNum) {
            QCC_DbgPrintf(("Identity certificate is already up to date"));
            break;
        }

        const PermissionPolicy::Rule* manifest;
        size_t manifestSize;
        status = securityManagerImpl->DeserializeManifest(mgdAppInfo, &manifest, &manifestSize);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not retrieve persisted manifest"));
            error = new SyncError(appInfo, status, persistedIdCert);
            break;
        }

        persistedIdCert.SetSubjectCN((const uint8_t*)mgdAppInfo.peerID.data(), mgdAppInfo.peerID.size());
        if (ER_OK != (status = certificateGenerator->GetIdentityCertificate(persistedIdCert))) {
            error = new SyncError(appInfo, status, persistedIdCert);
            QCC_LogError(status, ("Could not generate identity certificate"));
            break;
        }

        status = applicationManager->InstallIdentity(appInfo, &persistedIdCert, 1, manifest, manifestSize);
        if (ER_OK != status) {
            error = new SyncError(appInfo, status, persistedIdCert);
        }
    } while (0);

    if (NULL != error) {
        securityManagerImpl->NotifyApplicationListeners(error);
    }

    return status;
}

QStatus ApplicationUpdater::UpdateApplication(const ApplicationInfo& appInfo,
                                              const SecurityInfo& secInfo)
{
    QStatus status = ER_FAIL;

    if (std::find(secInfo.rootsOfTrust.begin(), secInfo.rootsOfTrust.end(),
                  securityManagerPubkey) == secInfo.rootsOfTrust.end()) {
        QCC_DbgPrintf(("Not updating unmanaged %s", secInfo.busName.c_str()));
        return ER_OK;
    }

    QCC_DbgPrintf(("Updating %s", secInfo.busName.c_str()));
    busAttachment->EnableConcurrentCallbacks();

    ManagedApplicationInfo managedAppInfo;
    managedAppInfo.publicKey = secInfo.publicKey;
    status = storage->GetManagedApplication(managedAppInfo);
    QCC_DbgPrintf(("GetManagedApplication returned %s", QCC_StatusText(status)));

    if (ER_END_OF_DATA == status) {
        status = ResetApplication(appInfo, secInfo);
    } else {
        do {
            if (ER_OK != (status = UpdatePolicy(appInfo, secInfo, managedAppInfo))) {
                break;
            }
            if (ER_OK != (status = UpdateMembershipCertificates(appInfo, secInfo, managedAppInfo))) {
                break;
            }
            status = UpdateIdentityCert(appInfo, secInfo, managedAppInfo);
        } while (0);
    }

    // This assumes no database changes have been made while updating an
    // application.

    if (ER_OK == status) {
        status = securityManagerImpl->SetUpdatesPending(appInfo, false);
    }

    QCC_DbgPrintf(("Updating %s returned %s ", secInfo.busName.c_str(), QCC_StatusText(status)));

    return status;
}

QStatus ApplicationUpdater::UpdateApplication(const SecurityInfo& secInfo)
{
    ApplicationInfo appInfo;
    appInfo.busName = secInfo.busName;
    appInfo.publicKey = secInfo.publicKey;
    return UpdateApplication(appInfo, secInfo);
}

QStatus ApplicationUpdater::UpdateApplication(const ApplicationInfo& appInfo)
{
    QStatus status = securityManagerImpl->SetUpdatesPending(appInfo, true);
    SecurityInfo secInfo;
    secInfo.busName = appInfo.busName;
    status = securityManagerImpl->GetApplicationSecInfo(secInfo);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to fetch security info !"));
        return status;
    }
    vector<qcc::ECCPublicKey> rots;
    rots.push_back(securityManagerPubkey);
    secInfo.rootsOfTrust = rots;
    return UpdateApplication(appInfo, secInfo);
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

    if ((NULL == oldSecInfo) && (NULL != newSecInfo)) {
        // new security info
        QCC_DbgPrintf(("Detected new busName %s", newSecInfo->busName.c_str()));
        UpdateApplication(*newSecInfo);
    }
}

#undef QCC_MODULE
