/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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
    QStatus status = ER_FAIL;

    QCC_DbgPrintf(("Resetting application"));
    status = applicationManager->Reset(appInfo);
    QCC_DbgPrintf(("Resetting application returned %i", status));

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
            return status;
        }
        localSerialNum = localPolicy.GetSerialNum();
    }
    QCC_DbgPrintf(("Local policy number %i", localSerialNum));

    if (localSerialNum == remoteSerialNum) {
        QCC_DbgPrintf(("Policy already up to date"));
        return ER_OK;
    } else if (localSerialNum > remoteSerialNum) {
        status = applicationManager->InstallPolicy(appInfo, localPolicy);
        QCC_DbgPrintf(("Installing new policy returned %i", status));
        return status;
    } else {
        QCC_DbgPrintf(("Unknown remote policy"));
        return ER_FAIL;
    }
}

QStatus ApplicationUpdater::UpdateMembershipCertificates(const ApplicationInfo& appInfo,
                                                         const SecurityInfo& secInfo,
                                                         const ManagedApplicationInfo& mgdAppInfo)
{
    QCC_DbgPrintf(("Updating membership certificates"));

    QStatus status = ER_OK;

    qcc::ECCPublicKey eccAppPubKey(secInfo.publicKey);

    qcc::X509MemberShipCertificate queryCert;
    queryCert.SetSubject(&eccAppPubKey);
    vector<qcc::X509MemberShipCertificate> localCerts;

    storage->GetCertificates(queryCert, localCerts);
    QCC_DbgPrintf(("Found %i local membership certificates", localCerts.size()));
    std::vector<qcc::X509MemberShipCertificate>::iterator it;
    for (it = localCerts.begin(); it != localCerts.end(); ++it) {
        QCC_DbgPrintf(("Local membership certificate %s", it->GetSerialNumber().c_str()));
        QStatus certStatus = ER_FAIL;

        qcc::GUID128 appID = mgdAppInfo.peerID;
        it->SetApplicationID(appID);
        certificateGenerator->GenerateMembershipCertificate(*it);

        qcc::String data;
        storage->GetAssociatedData(*it, data);
        PermissionPolicy authData;
        if (!data.empty()) {
            Message tmpMsg(*busAttachment);
            DefaultPolicyMarshaller marshaller(tmpMsg);
            certStatus = authData.Import(marshaller, (const uint8_t*)data.data(), data.size());
            if (ER_OK != certStatus) {
                status = certStatus;
                break;
            }
        }

        certStatus = applicationManager->InstallMembership(appInfo, *it, authData, securityManagerGUID);
        QCC_DbgPrintf(("Install membership certificate %s returned %i", it->GetSerialNumber().c_str(), certStatus));

        if (ER_DUPLICATE_CERTIFICATE == certStatus) {
            certStatus = ER_OK;
        }

        if (ER_OK != certStatus) {
            status = certStatus;
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
    qcc::X509IdentityCertificate persistedIdCert;

    do {
        persistedIdCert.SetSubject(&(secInfo.publicKey));

        if (ER_OK != (status = storage->GetCertificate(persistedIdCert))) {
            QCC_LogError(status, ("Could not get identity certificate from storage"));
            break;
        }

        uint32_t localSerialNum = strtoul(persistedIdCert.GetSerialNumber().c_str(), NULL, 0);
        QCC_DbgPrintf(("Local identity certificate serial number is %u", localSerialNum));

        if (ER_OK != (status = applicationManager->GetIdentity(appInfo, remoteIdCert))) {
            QCC_LogError(status, ("Could not fetch identity certificate"));
            break;
        }

        uint32_t remoteSerialNum = strtoul(remoteIdCert.GetSerial().c_str(), NULL, 0);
        QCC_DbgPrintf(("Remote identity certificate serial number is %u", remoteSerialNum));

        persistedIdCert.SetApplicationID(mgdAppInfo.peerID);
        if (ER_OK != (status = certificateGenerator->GetIdentityCertificate(persistedIdCert))) {
            QCC_LogError(status, ("Could not generate identity certificate"));
            break;
        }

        if (localSerialNum == remoteSerialNum) {
            QCC_DbgPrintf(("Identity certificate is already up to date"));
            break;
        }
        if (localSerialNum > remoteSerialNum) {
            status = applicationManager->InstallIdentityCertificate(appInfo, persistedIdCert);
        } else {
            QCC_DbgPrintf(("Unknown remote identity certificate"));
        }
    } while (0);

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
    QCC_DbgPrintf(("GetManagedApplication returned %i", status));

    if (ER_END_OF_DATA == status) {
        status = ResetApplication(appInfo, secInfo);
    } else if (ER_OK == status) {
        QStatus policyUpdateStatus =
            UpdatePolicy(appInfo, secInfo, managedAppInfo);
        QStatus membershipUpdateStatus =
            UpdateMembershipCertificates(appInfo, secInfo, managedAppInfo);
        QStatus identityUpdateStatus = UpdateIdentityCert(appInfo, secInfo, managedAppInfo);
        if ((policyUpdateStatus != ER_OK) || (membershipUpdateStatus != ER_OK) || (identityUpdateStatus != ER_OK)) {
            status = ER_FAIL;
        }
    }

    QCC_DbgPrintf(("Updating %s returned %i ", secInfo.busName.c_str(), status));

    return status;
}

QStatus ApplicationUpdater::UpdateApplication(const SecurityInfo& secInfo)
{
    ApplicationInfo appInfo;
    appInfo.busName = secInfo.busName;
    return UpdateApplication(appInfo, secInfo);
}

QStatus ApplicationUpdater::UpdateApplication(const ApplicationInfo& appInfo)
{
    SecurityInfo secInfo(appInfo);
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
