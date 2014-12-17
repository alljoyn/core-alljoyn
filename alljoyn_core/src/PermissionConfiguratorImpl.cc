/**
 * @file
 * This file defines the implementation of the Permission Configurator to allow app to setup some permission templates.
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#define QCC_MODULE "PERMISSION_MGMT"

using namespace std;
using namespace qcc;

namespace ajn {

QStatus PermissionConfiguratorImpl::SetPermissionManifest(PermissionPolicy::Rule* rules, size_t count)
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj) {
        QCC_DbgPrintf(("PermissionConfiguratorImpl::SetPermissionManifest does not have PermissionMgmtObj initialized"));
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->SetManifest(rules, count);
}

PermissionConfigurator::ClaimableState PermissionConfiguratorImpl::GetClaimableState()
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj) {
        return STATE_UNKNOWN;
    }
    return permissionMgmtObj->GetClaimableState();
}

QStatus PermissionConfiguratorImpl::SetClaimable(bool claimable)
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->SetClaimable(claimable);
}

QStatus PermissionConfiguratorImpl::Reset()
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->Reset();
}

QStatus PermissionConfiguratorImpl::GenerateSigningKeyPair()
{
    Crypto_ECC ecc;
    QStatus status = ecc.GenerateDSAKeyPair();

    if (status != ER_OK) {
        return ER_CRYPTO_KEY_UNAVAILABLE;
    }
    CredentialAccessor ca(bus);
    status = PermissionMgmtObj::StoreDSAKeys(&ca, ecc.GetDSAPrivateKey(), ecc.GetDSAPublicKey());
    if (status != ER_OK) {
        return ER_CRYPTO_KEY_UNAVAILABLE;
    }
    return ER_OK;
}

QStatus PermissionConfiguratorImpl::GetSigningPublicKey(KeyInfoECC& keyInfo)
{
    if (keyInfo.GetCurve() != Crypto_ECC::ECC_NIST_P256) {
        return ER_NOT_IMPLEMENTED;  /* currently only support NIST P256 curve */
    }
    CredentialAccessor ca(bus);
    ECCPublicKey publicKey;
    QStatus status = PermissionMgmtObj::RetrieveDSAPublicKey(&ca, &publicKey);
    if (status != ER_OK) {
        return status;
    }
    KeyInfoNISTP256* pKeyInfo = (KeyInfoNISTP256*) &keyInfo;
    qcc::GUID128 localGUID;
    status = ca.GetGuid(localGUID);
    if (ER_OK != status) {
        return status;
    }
    pKeyInfo->SetKeyId(localGUID.GetBytes(), GUID128::SIZE);
    pKeyInfo->SetPublicKey(&publicKey);
    return ER_OK;
}

QStatus PermissionConfiguratorImpl::SignCertificate(CertificateX509& cert)
{
    CredentialAccessor ca(bus);
    ECCPrivateKey privateKey;
    QStatus status = PermissionMgmtObj::RetrieveDSAPrivateKey(&ca, &privateKey);
    if (status != ER_OK) {
        return status;
    }
    return cert.Sign(&privateKey);
}

QStatus PermissionConfiguratorImpl::GetConnectedPeerPublicKey(const GUID128& guid, qcc::ECCPublicKey* publicKey)
{
    PermissionMgmtObj* permissionMgmtObj = bus.GetInternal().GetPermissionManager().GetPermissionMgmtObj();
    if (!permissionMgmtObj) {
        return ER_FEATURE_NOT_AVAILABLE;
    }
    return permissionMgmtObj->GetConnectedPeerPublicKey(guid, publicKey);
}

}
