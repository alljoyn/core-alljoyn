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

#include <alljoyn/securitymgr/SecurityManager.h>

#include "SecurityManagerImpl.h"

#include <qcc/CryptoECC.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <alljoyn/about/AnnouncementRegistrar.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>

#include <SecLibDef.h>

#include <iostream>

#define QCC_MODULE "SEC_MGR"

using namespace ajn::services;
using namespace ajn;
using namespace qcc;
using namespace securitymgr;

SecurityManager::SecurityManager(BusAttachment* ba,
                                 const Storage* storage) :
    securityManagerImpl(NULL)
{
    securityManagerImpl = new SecurityManagerImpl(ba, storage);
}

SecurityManager::~SecurityManager()
{
    delete securityManagerImpl;
}

QStatus SecurityManager::Claim(const ApplicationInfo& app,
                               const IdentityInfo& id)
{
    return securityManagerImpl->Claim(app, id);
}

void SecurityManager::SetManifestListener(ManifestListener* listener)
{
    securityManagerImpl->SetManifestListener(listener);
}

QStatus SecurityManager::GetManifest(const ApplicationInfo& appInfo,
                                     const PermissionPolicy::Rule** manifestRules,
                                     size_t* manifestRulesCount)
{
    return securityManagerImpl->GetManifest(appInfo, manifestRules, manifestRulesCount);
}

QStatus SecurityManager::UpdateIdentity(const ApplicationInfo& app, const IdentityInfo& id)
{
    return securityManagerImpl->UpdateIdentity(app, id);
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

QStatus SecurityManager::SetApplicationName(ApplicationInfo& appInfo)
{
    return securityManagerImpl->SetApplicationName(appInfo);
}

QStatus SecurityManager::StoreGuild(GuildInfo& guildInfo)
{
    return securityManagerImpl->StoreGuild(guildInfo);
}

QStatus SecurityManager::RemoveGuild(GuildInfo& guildInfo)
{
    return securityManagerImpl->RemoveGuild(guildInfo);
}

QStatus SecurityManager::GetGuild(GuildInfo& guildInfo) const
{
    return securityManagerImpl->GetGuild(guildInfo);
}

QStatus SecurityManager::GetGuilds(std::vector<GuildInfo>& guildInfos) const
{
    return securityManagerImpl->GetGuilds(guildInfos);
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

QStatus SecurityManager::UpdatePolicy(const ApplicationInfo& appInfo,
                                      PermissionPolicy& policy)
{
    return securityManagerImpl->UpdatePolicy(appInfo, policy);
}

QStatus SecurityManager::GetPolicy(const ApplicationInfo& appInfo,
                                   PermissionPolicy& policy)
{
    return securityManagerImpl->GetPolicy(appInfo, policy);
}

QStatus SecurityManager::Reset(const ApplicationInfo& appInfo)
{
    return securityManagerImpl->Reset(appInfo);
}

QStatus SecurityManager::StoreIdentity(IdentityInfo& idInfo)
{
    return securityManagerImpl->StoreIdentity(idInfo);
}

QStatus SecurityManager::RemoveIdentity(IdentityInfo& idInfo)
{
    return securityManagerImpl->RemoveIdentity(idInfo);
}

QStatus SecurityManager::GetIdentity(IdentityInfo& idInfo) const
{
    return securityManagerImpl->GetIdentity(idInfo);
}

QStatus SecurityManager::GetIdentities(std::vector<IdentityInfo>& idInfos) const
{
    return securityManagerImpl->GetIdentities(idInfos);
}

QStatus SecurityManager::Init()
{
    return securityManagerImpl->Init();
}

#undef QCC_MODULE
