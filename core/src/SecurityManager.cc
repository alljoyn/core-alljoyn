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

#include <SecurityManager.h>
#include <SecurityManagerImpl.h>

#include <qcc/CryptoECC.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <alljoyn/about/AnnouncementRegistrar.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>

#include "StorageConfig.h"
#include "StorageFactory.h"
#include <SecLibDef.h>

#include <iostream>

#define QCC_MODULE "SEC_MGR"
using namespace ajn::services;

namespace ajn {
namespace securitymgr {
SecurityManager::SecurityManager(IdentityData* id,
                                 ajn::BusAttachment* ba,
                                 const qcc::ECCPublicKey& pubKey,
                                 const qcc::ECCPrivateKey& privKey,
                                 const StorageConfig& storageCfg,
                                 const SecurityManagerConfig& smCfg) :
    securityManagerImpl(new SecurityManagerImpl(id, ba, pubKey, privKey, storageCfg, smCfg))
{
}

SecurityManager::~SecurityManager()
{
    delete securityManagerImpl;
}

QStatus SecurityManager::GetStatus() const
{
    return securityManagerImpl->GetStatus();
}

QStatus SecurityManager::ClaimApplication(const ApplicationInfo& app,
                                          const IdentityInfo& id,
                                          AcceptManifestCB amcb,
                                          void* cookie)
{
    return securityManagerImpl->ClaimApplication(app, id, amcb, cookie);
}

QStatus SecurityManager::Claim(ApplicationInfo& app, const IdentityInfo& identityInfo)
{
    return securityManagerImpl->Claim(app, identityInfo);
}

QStatus SecurityManager::GetManifest(const ApplicationInfo& appInfo,
                                     PermissionPolicy::Rule** manifestRules,
                                     size_t* manifestRulesCount)
{
    return securityManagerImpl->GetManifest(appInfo, manifestRules, manifestRulesCount);
}

QStatus SecurityManager::InstallIdentity(const ApplicationInfo& app, const IdentityInfo& id)
{
    return securityManagerImpl->InstallIdentity(app, id);
}

QStatus SecurityManager::GetIdentityCertificate(const ApplicationInfo& appInfo, IdentityCertificate& idCert) const
{
    return securityManagerImpl->GetRemoteIdentityCertificate(appInfo, idCert);
}

const ECCPublicKey& SecurityManager::GetPublicKey() const
{
    return securityManagerImpl->GetPublicKey();
}

std::vector<ApplicationInfo> SecurityManager::GetApplications(ajn::PermissionConfigurator::ClaimableState acs) const
{
    return securityManagerImpl->GetApplications(acs);
}

void SecurityManager::RegisterApplicationListener(ApplicationListener* al)
{
    return securityManagerImpl->RegisterApplicationListener(al);
}

void SecurityManager::UnregisterApplicationListener(ApplicationListener* al)
{
    return securityManagerImpl->UnregisterApplicationListener(al);
}

QStatus SecurityManager::GetApplication(ApplicationInfo& ai) const
{
    return securityManagerImpl->GetApplication(ai);
}

QStatus SecurityManager::StoreGuild(const GuildInfo& guildInfo, const bool update)
{
    return securityManagerImpl->StoreGuild(guildInfo, update);
}

QStatus SecurityManager::RemoveGuild(const GUID128& guildId)
{
    return securityManagerImpl->RemoveGuild(guildId);
}

QStatus SecurityManager::GetGuild(GuildInfo& guildInfo) const
{
    return securityManagerImpl->GetGuild(guildInfo);
}

QStatus SecurityManager::GetManagedGuilds(std::vector<GuildInfo>& guildsInfo) const
{
    return securityManagerImpl->GetManagedGuilds(guildsInfo);
}

QStatus SecurityManager::InstallMembership(const ApplicationInfo& appInfo,
                                           const GuildInfo& guildInfo,
                                           const PermissionPolicy* authorizationData)
{
    return securityManagerImpl->InstallMembership(appInfo, guildInfo, authorizationData);
}

QStatus SecurityManager::RemoveMembership(const ApplicationInfo& appInfo,
                                          const GuildInfo& guildInfo)
{
    return securityManagerImpl->RemoveMembership(appInfo, guildInfo);
}

QStatus SecurityManager::InstallPolicy(const ApplicationInfo& appInfo,
                                       PermissionPolicy& policy)
{
    return securityManagerImpl->InstallPolicy(appInfo, policy);
}

QStatus SecurityManager::GetPolicy(const ApplicationInfo& appInfo,
                                   PermissionPolicy& policy, bool remote)
{
    return securityManagerImpl->GetPolicy(appInfo, policy, remote);
}

QStatus SecurityManager::Reset(const ApplicationInfo& appInfo)
{
    return securityManagerImpl->Reset(appInfo);
}

QStatus SecurityManager::StoreIdentity(const IdentityInfo& identityInfo,
                                       const bool update)
{
    return securityManagerImpl->StoreIdentity(identityInfo, update);
}

QStatus SecurityManager::RemoveIdentity(const GUID128& idId)
{
    return securityManagerImpl->RemoveIdentity(idId);
}

QStatus SecurityManager::GetIdentity(IdentityInfo& idInfo) const
{
    return securityManagerImpl->GetIdentity(idInfo);
}

QStatus SecurityManager::GetManagedIdentities(std::vector<IdentityInfo>& identityInfos) const
{
    return securityManagerImpl->GetManagedIdentities(identityInfos);
}
}
}
#undef QCC_MODULE
