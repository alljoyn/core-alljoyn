/**
 * @file
 * This file defines the implementation of the Permission Configurator to allow app to setup some permission templates.
 */

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

#include <qcc/CryptoECC.h>
#include <qcc/KeyInfoECC.h>
#include <qcc/StringUtil.h>
#include "PermissionConfiguratorImpl.h"
#include "PermissionMgmtObj.h"
#include "BusInternal.h"
#include "CredentialAccessor.h"
#include "KeyInfoHelper.h"

#define QCC_MODULE "PERMISSION_MGMT"

using namespace std;
using namespace qcc;

namespace ajn {

QStatus PermissionConfiguratorImpl::SetPermissionManifest(PermissionPolicy::Rule* rules, size_t count)
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        QCC_DbgPrintf(("PermissionConfiguratorImpl::SetPermissionManifest does not have PermissionMgmtObj initialized"));
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->SetManifestTemplate(rules, count);
}

QStatus PermissionConfiguratorImpl::GetApplicationState(ApplicationState& applicationState)
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    applicationState = permissionMgmtObj->GetApplicationState();
    return ER_OK;
}

QStatus PermissionConfiguratorImpl::SetApplicationState(ApplicationState newState)
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->SetApplicationState(newState);
}

QStatus PermissionConfiguratorImpl::Reset()
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->Reset();
}

QStatus PermissionConfiguratorImpl::GetSigningPublicKey(KeyInfoECC& keyInfo)
{
    if (keyInfo.GetCurve() != Crypto_ECC::ECC_NIST_P256) {
        return ER_NOT_IMPLEMENTED;  /* currently only support NIST P256 curve */
    }
    CredentialAccessor ca(bus);
    ECCPublicKey publicKey;
    QStatus status = ca.GetDSAPublicKey(publicKey);
    if (status != ER_OK) {
        return status;
    }
    KeyInfoNISTP256* pKeyInfo = (KeyInfoNISTP256*) &keyInfo;
    pKeyInfo->SetPublicKey(&publicKey);
    KeyInfoHelper::GenerateKeyId(*pKeyInfo);
    return ER_OK;
}

QStatus PermissionConfiguratorImpl::SignCertificate(CertificateX509& cert)
{
    CredentialAccessor ca(bus);
    ECCPrivateKey privateKey;
    QStatus status = ca.GetDSAPrivateKey(privateKey);
    if (status != ER_OK) {
        return status;
    }
    ECCPublicKey publicKey;
    status = ca.GetDSAPublicKey(publicKey);
    if (status != ER_OK) {
        return status;
    }
    return cert.SignAndGenerateAuthorityKeyId(&privateKey, &publicKey);
}

QStatus PermissionConfiguratorImpl::GetConnectedPeerPublicKey(const GUID128& guid, qcc::ECCPublicKey* publicKey)
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->GetConnectedPeerPublicKey(guid, publicKey);
}

QStatus PermissionConfiguratorImpl::SetClaimCapabilities(PermissionConfigurator::ClaimCapabilities claimCapabilities)
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->SetClaimCapabilities(claimCapabilities);
}

QStatus PermissionConfiguratorImpl::SetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo additionalInfo)
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->SetClaimCapabilityAdditionalInfo(additionalInfo);
}

QStatus PermissionConfiguratorImpl::GetClaimCapabilities(PermissionConfigurator::ClaimCapabilities& claimCapabilities)
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->GetClaimCapabilities(claimCapabilities);
}

QStatus PermissionConfiguratorImpl::GetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo& additionalInfo)
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj || !permissionMgmtObj->IsReady()) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->GetClaimCapabilityAdditionalInfo(additionalInfo);
}

}
