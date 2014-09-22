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

#ifndef SECURITYMANAGERIMPL_H_
#define SECURITYMANAGERIMPL_H_

#include <alljoyn/about/AnnounceHandler.h>
#include <alljoyn/Status.h>
#include <qcc/CryptoECC.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>

#include <RootOfTrust.h>
#include <ApplicationListener.h>
#include <ApplicationInfo.h>
#include <AppGuildInfo.h>
#include <ApplicationMonitor.h>
#include <X509CertificateGenerator.h>
#include <AuthorizationData.h>
#include <SecurityManager.h>
#include "ProxyObjectManager.h"
#include <Storage.h>
#include <StorageConfig.h>
#include <IdentityData.h>

#include <memory>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
class Identity;
class SecurityInfoListener;
class SecurityInfo;

/**
 * \class SecurityManagerImpl
 *
 * \brief the class provides for the SecurityManager implementation hiding
 */

class SecurityManagerImpl :
    public ajn::services::AnnounceHandler,
    private SecurityInfoListener {
  public:

    SecurityManagerImpl(qcc::String userName,
                        qcc::String password,
                        IdentityData* id,                                             // !!! Changed to pointer to support NULL identity
                        ajn::BusAttachment* ba,
                        const qcc::ECCPublicKey& pubKey,
                        const qcc::ECCPrivateKey& privKey,
                        const StorageConfig& _storageCfg);

    ~SecurityManagerImpl();

    QStatus ClaimApplication(const ApplicationInfo &app, AcceptManifestCB);

    QStatus InstallIdentity(const ApplicationInfo& app);

    QStatus AddRootOfTrust(const ApplicationInfo& app,
                           const RootOfTrust& rot);

    QStatus RemoveRootOfTrust(const ApplicationInfo& app,
                              const RootOfTrust& rot);

    const RootOfTrust& GetRootOfTrust() const;

    std::vector<ApplicationInfo> GetApplications(
        ajn::securitymgr::ApplicationClaimState acs =
            ajn::securitymgr::ApplicationClaimState::UNKNOWN_CLAIM_STATE) const;

    void RegisterApplicationListener(ApplicationListener* al);

    void UnregisterApplicationListener(ApplicationListener* al);

    QStatus GetApplication(ApplicationInfo& ai) const;

    QStatus StoreGuild(const GuildInfo& guildInfo,
                       const bool update = false);

    QStatus RemoveGuild(const qcc::String& guildId);

    QStatus GetGuild(GuildInfo& guildInfo) const;

    QStatus GetManagedGuilds(std::vector<GuildInfo>& guildsInfo) const;

    QStatus InstallMembership(const ApplicationInfo& appInfo,
                              const GuildInfo& guildInfo,
                              const AuthorizationData* authorizationData);

    QStatus RemoveMembership(const ApplicationInfo& appInfo,
                             const GuildInfo& guildInfo);

    QStatus GetStatus() const;

  private:

    typedef std::map<PublicKey, ApplicationInfo> ApplicationInfoMap; /* key= pubkey of app, value = info */

    qcc::String GetString(::ajn::services::PropertyStoreKey key, const AboutData &aboutData) const;

    qcc::String GetAppId(const AboutData& aboutData) const;

    QStatus CreateInterface(ajn::BusAttachment* bus,
                            ajn::InterfaceDescription*& intf);

    QStatus EstablishPSKSession(const ApplicationInfo& app,
                                uint8_t* bytes,
                                size_t size);

    virtual void OnSecurityStateChange(const SecurityInfo* oldSecInfo,
                                       const SecurityInfo* newSecInfo);

    void Announce(unsigned short version,
                  SessionPort port,
                  const char* busName,
                  const ObjectDescriptions& objectDescs,
                  const AboutData& aboutData);

    ApplicationInfoMap::iterator SafeAppExist(const PublicKey& publicKey, bool& exist);

  private:
    QStatus status;
    IdentityData* id;
    const qcc::ECCPrivateKey privKey;
    const RootOfTrust rot;  // !!! Verify this (added to get compiled)!!
    StorageConfig storageCfg;
    ApplicationInfoMap applications;
    std::map<qcc::String, ApplicationInfo> aboutCache; /* key=busname of app, value = info */
    std::vector<ApplicationListener*> listeners;
    X509CertificateGenerator* CertificateGen;
    ProxyObjectManager* proxyObjMgr;
    ApplicationMonitor* appMonitor;
    ajn::BusAttachment* busAttachment;
    Storage* storage;
    mutable qcc::Mutex appsMutex;
    mutable qcc::Mutex storageMutex;
    qcc::Mutex aboutCacheMutex;
};
}
}
#undef QCC_MODULE
#endif /* SECURITYMANAGERIMPL_H_ */
