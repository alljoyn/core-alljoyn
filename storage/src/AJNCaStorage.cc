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

#include "AJNCa.h"
#include "AJNCaStorage.h"

#include <qcc/Debug.h>

#include <PermissionMgmtObj.h> // Still in alljoyn_core/src!

#include <alljoyn/securitymgr/CertificateUtil.h>

#define QCC_MODULE "SECMGR_STORAGE"

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
QStatus AJNCaStorage::Init(const string storeName, shared_ptr<SQLStorage>& sqlStorage)
{
    ca = unique_ptr<AJNCa>(new AJNCa());
    QStatus status = ca->Init(storeName);
    if (status != ER_OK) {
        return status;
    }
    sql = sqlStorage;
    return ER_OK;
}

QStatus AJNCaStorage::GetAdminGroup(GroupInfo& adminGroup) const
{
    adminGroup.name = "Admin group";
    adminGroup.guid = GUID128(0xab);
    QStatus status = GetCaPublicKeyInfo(adminGroup.authority);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to set DSA key on adminGroup"));
    }
    return status;
}

QStatus AJNCaStorage::StartApplicationClaiming(const Application& app,
                                               const IdentityInfo& idInfo,
                                               const Manifest& mf,
                                               GroupInfo& adminGroup,
                                               IdentityCertificateChain& idCertChain)
{
    QStatus status;

    status = sql->GetIdentity(const_cast<IdentityInfo&>(idInfo));
    if (ER_OK != status) {
        QCC_LogError(status, ("Identity does not exist"));
        return status;
    }

    status = GetAdminGroup(adminGroup);
    if (ER_OK != status) {
        QCC_LogError(status, ("No admin group found"));
        return status;
    }

    IdentityCertificate idCert;
    status = GenerateIdentityCertificate(app, idInfo, mf, idCert);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to create IdentityCertificate"));
        return status;
    }
    idCertChain.push_back(idCert);

    CachedData data;
    data.cert = idCert;
    data.mnf = mf;
    pendingLock.Lock();
    claimPendingApps[app] = data;
    pendingLock.Unlock();
    return status;
}

QStatus AJNCaStorage::FinishApplicationClaiming(const Application& app, QStatus status)
{
    CachedData data;
    pendingLock.Lock();
    auto search = claimPendingApps.find(app);
    if (search != claimPendingApps.end()) {
        data = search->second;
        claimPendingApps.erase(search);
    } else {
        pendingLock.Unlock();
        return ER_END_OF_DATA;
    }
    pendingLock.Unlock();
    Application _app = app;

    if (ER_OK != status) {
        return ER_OK;
    }

    _app.syncState = SYNC_OK;

    return handler->ApplicationClaimed(_app, data.cert, data.mnf);
}

QStatus AJNCaStorage::GetManagedApplication(Application& app) const
{
    return sql->GetManagedApplication(app);
}

QStatus AJNCaStorage::StartUpdates(Application& app, uint64_t& updateID)
{
    return handler->StartUpdates(app, updateID);
}

QStatus AJNCaStorage::UpdatesCompleted(Application& app, uint64_t& updateID)
{
    return handler->UpdatesCompleted(app, updateID);
}

QStatus AJNCaStorage::RegisterAgent(const KeyInfoNISTP256& agentKey,
                                    const Manifest& manifest,
                                    GroupInfo& adminGroup,
                                    IdentityCertificateChain& identityCertificates,
                                    vector<MembershipCertificateChain>& adminGroupMemberships)
{
    QStatus status = GetAdminGroup(adminGroup);
    if (status != ER_OK) {
        return status;
    }

    Application agentInfo;
    if (agentKey.empty()) {
        QCC_LogError(status, ("Bad Name: key empty ? %i", agentKey.empty()));
        return ER_FAIL;
    }
    agentInfo.keyInfo = agentKey;

    MembershipCertificate memberShip;
    GenerateMembershipCertificate(agentInfo, adminGroup, memberShip);
    MembershipCertificateChain chain;
    chain.push_back(memberShip);
    adminGroupMemberships.push_back(chain);

    IdentityInfo agentID;
    agentID.name = "Admin";
    agentID.guid = GUID128(0xab);
    IdentityCertificate idCert;
    status = GenerateIdentityCertificate(agentInfo, agentID, manifest, idCert);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to generate identity certificate for agent"));
        return status;
    }
    identityCertificates.push_back(idCert);
    return ER_OK;
}

QStatus AJNCaStorage::SignCertifcate(CertificateX509& certificate) const
{
    if (certificate.GetSerialLen() == 0) {
        sql->GetNewSerialNumber(certificate);
    }
    KeyInfoNISTP256 caInfo;

    QStatus status = GetCaPublicKeyInfo(caInfo);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to get public key"));
        return status;
    }
    certificate.SetIssuerCN(caInfo.GetKeyId(), caInfo.GetKeyIdLen());
    ECCPrivateKey epk;
    status = ca->GetDSAPrivateKey(epk);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to load key"));
        return status;
    }
    status = certificate.SignAndGenerateAuthorityKeyId(&epk, caInfo.GetPublicKey());
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to sign certificate"));
    }
    return status;
}

QStatus AJNCaStorage::GenerateIdentityCertificate(const Application& app,
                                                  const IdentityInfo& idInfo,
                                                  const Manifest& mf,
                                                  IdentityCertificate& idCertificate)
{
    QStatus status = CertificateUtil::ToIdentityCertificate(app, idInfo, 3600 * 24 * 10 * 365, idCertificate);
    if (status != ER_OK) {
        return status;
    }
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    mf.GetDigest(digest);
    idCertificate.SetDigest(digest, Crypto_SHA256::DIGEST_SIZE);
    return SignCertifcate(idCertificate);
}

QStatus AJNCaStorage::GenerateMembershipCertificate(const Application& app,
                                                    const GroupInfo& groupInfo, MembershipCertificate& memberShip)
{
    QStatus status = CertificateUtil::ToMembershipCertificate(app, groupInfo, 3600 * 24 * 10 * 365, memberShip);
    if (status != ER_OK) {
        return status;
    }
    return SignCertifcate(memberShip);
}

QStatus AJNCaStorage::GetCaPublicKeyInfo(KeyInfoNISTP256& CAKeyInfo) const
{
    ECCPublicKey key;
    QStatus status = ca->GetDSAPublicKey(key);
    if (status != ER_OK || key.empty()) {
        return status == ER_OK ? ER_BUS_KEY_UNAVAILABLE : status;
    }
    CAKeyInfo.SetPublicKey(&key);
    String id;
    status = CertificateX509::GenerateAuthorityKeyId(&key, id);
    if (status != ER_OK) {
        return status;
    }
    CAKeyInfo.SetKeyId((uint8_t*)id.data(), id.size());
    return ER_OK;
}

QStatus AJNCaStorage::GetMembershipCertificates(const Application& app,
                                                vector<MembershipCertificateChain>& membershipCertificateChains) const
{
    MembershipCertificate cert;
    cert.SetSubjectPublicKey(app.keyInfo.GetPublicKey());
    vector<MembershipCertificate> membershipCertificates;
    QStatus status = sql->GetMembershipCertificates(app, cert, membershipCertificates);
    if (ER_OK == status) {
        for (size_t i = 0; i < membershipCertificates.size(); i++) {
            MembershipCertificateChain chain;
            chain.push_back(membershipCertificates[i]);
            membershipCertificateChains.push_back(chain);
        }
    }
    return status;
}

QStatus AJNCaStorage::GetIdentityCertificatesAndManifest(const Application& app,
                                                         IdentityCertificateChain& identityCertificates,
                                                         Manifest& mf) const
{
    Application _app(app);
    QStatus status = GetManagedApplication(_app);
    if (ER_OK != status) {
        return status;
    }
    // We only support one id certificate now.
    IdentityCertificate tmpCert;
    status = sql->GetCertificate(app, tmpCert);
    if (ER_OK != status) {
        return status;
    }
    identityCertificates.push_back(tmpCert);
    return sql->GetManifest(app, mf);
}

QStatus AJNCaStorage::GetPolicy(const Application& app, PermissionPolicy& policy) const
{
    Application _app(app);
    QStatus status = GetManagedApplication(_app);
    if (ER_OK != status) {
        return status;
    }
    return sql->GetPolicy(app, policy);
}
}
}
#undef QCC_MODULE
