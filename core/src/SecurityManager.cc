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
SecurityManager::SecurityManager(qcc::String userName,
                                 qcc::String password,
                                 IdentityData* id,
                                 ajn::BusAttachment* ba,
                                 const qcc::ECCPublicKey& pubKey,
                                 const qcc::ECCPrivateKey& privKey,
                                 const StorageConfig& storageCfg) :
    securityManagerImpl(new SecurityManagerImpl(userName, password, id, ba, pubKey, privKey, storageCfg))
{
}

SecurityManager::~SecurityManager()
{
    SecurityManagerImpl* smImpl = securityManagerImpl.release();
    delete smImpl;
}

QStatus SecurityManager::GetStatus() const
{
    return securityManagerImpl->GetStatus();
}

QStatus SecurityManager::ClaimApplication(const ApplicationInfo& app)
{
    return securityManagerImpl->ClaimApplication(app);
}

QStatus SecurityManager::InstallIdentity(const ApplicationInfo& app)
{
    return securityManagerImpl->InstallIdentity(app);
}

QStatus SecurityManager::AddRootOfTrust(const ApplicationInfo& app, const RootOfTrust& rot)
{
    return securityManagerImpl->AddRootOfTrust(app, rot);
}

QStatus SecurityManager::RemoveRootOfTrust(const ApplicationInfo& app, const RootOfTrust& rot)
{
    return securityManagerImpl->RemoveRootOfTrust(app, rot);
}

const RootOfTrust& SecurityManager::GetRootOfTrust() const
{
    return securityManagerImpl->GetRootOfTrust();
}

std::vector<ApplicationInfo> SecurityManager::GetApplications(ApplicationClaimState acs) const
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

ApplicationInfo* SecurityManager::GetApplication(qcc::String& busName) const
{
    return securityManagerImpl->GetApplication(busName);
}

void SecurityManager::FlushStorage()
{
}
}
}
#undef QCC_MODULE
