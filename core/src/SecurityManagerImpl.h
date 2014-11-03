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
#include <alljoyn/PermissionPolicy.h>
#include <qcc/CryptoECC.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/GUID.h>

#include <RootOfTrust.h>
#include <ApplicationListener.h>
#include <ApplicationInfo.h>
#include <AppGuildInfo.h>
#include <ApplicationMonitor.h>
#include <X509CertificateGenerator.h>
#include <SecurityManager.h>
#include "ProxyObjectManager.h"
#include <Storage.h>
#include <StorageConfig.h>
#include <IdentityData.h>
#include <SecurityManagerConfig.h>

#include <memory>

#define QCC_MODULE "SEC_MGR"

using namespace qcc;

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
                        const StorageConfig& _storageCfg,
                        const SecurityManagerConfig& smCfg);

    ~SecurityManagerImpl();

    QStatus ClaimApplication(const ApplicationInfo &app, const IdentityInfo &id, AcceptManifestCB, void* cookie);

    QStatus Claim(ApplicationInfo& app,
                  const IdentityInfo& identityInfo);

    QStatus GetManifest(const ApplicationInfo& appInfo,
                        PermissionPolicy::Rule** manifestRules,
                        size_t* manifestRulesCount);

    QStatus InstallIdentity(const ApplicationInfo& app,
                            const IdentityInfo& id);

    QStatus GetRemoteIdentityCertificate(const ApplicationInfo& appInfo,
                                         IdentityCertificate& idCert);

    const RootOfTrust& GetRootOfTrust() const;

    std::vector<ApplicationInfo> GetApplications(ajn::PermissionConfigurator::ClaimableState acs =
                                                     ajn::PermissionConfigurator::STATE_UNKNOWN)
    const;

    void RegisterApplicationListener(ApplicationListener* al);

    void UnregisterApplicationListener(ApplicationListener* al);

    QStatus GetApplication(ApplicationInfo& ai) const;

    QStatus StoreGuild(const GuildInfo& guildInfo,
                       const bool update = false);

    QStatus RemoveGuild(const GUID128& guildId);

    QStatus GetGuild(GuildInfo& guildInfo) const;

    QStatus GetManagedGuilds(std::vector<GuildInfo>& guildsInfo) const;

    QStatus StoreIdentity(const IdentityInfo& identityInfo,
                          const bool update = false);

    QStatus RemoveIdentity(const GUID128& idId);

    QStatus GetIdentity(IdentityInfo& idInfo) const;

    QStatus GetManagedIdentities(std::vector<IdentityInfo>& identityInfos) const;

    QStatus InstallMembership(const ApplicationInfo& appInfo,
                              const GuildInfo& guildInfo,
                              const PermissionPolicy* authorizationData);

    QStatus RemoveMembership(const ApplicationInfo& appInfo,
                             const GuildInfo& guildInfo);

    QStatus InstallPolicy(const ApplicationInfo& appInfo,
                          PermissionPolicy& policy);

    QStatus GetPolicy(const ApplicationInfo& appInfo,
                      PermissionPolicy& policy,
                      bool remote);

    QStatus GetStatus() const;

    // TODO: move to ECCPublicKey class
    static QStatus MarshalPublicKey(const ECCPublicKey* pubKey,
                                    const GUID128& peerID,
                                    MsgArg& ma);

    // TODO: move to ECCPublicKey class
    static QStatus UnmarshalPublicKey(const MsgArg* ma,
                                      ECCPublicKey& pubKey);

  private:

    typedef std::map<ECCPublicKey, ApplicationInfo> ApplicationInfoMap; /* key = peerID of app, value = info */

    qcc::String GetString(::ajn::services::PropertyStoreKey key, const AboutData &aboutData) const;

    qcc::String GetAppId(const AboutData& aboutData) const;

    QStatus CreateStubInterface(BusAttachment* bus,
                                InterfaceDescription*& intf);

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

    ApplicationInfoMap::iterator SafeAppExist(const ECCPublicKey key,
                                              bool& exist);

    QStatus GetIdentityCertificate(X509IdentityCertificate& idCert,
                                   const IdentityInfo& idInfo,
                                   const ApplicationInfo& appInfo);

    QStatus InstallIdentityCertificate(X509IdentityCertificate& idCert,
                                       const ProxyBusObject* remoteObj,
                                       const uint32_t timeout);

    QStatus PersistApplication(const ApplicationInfo& appInfo,
                               const PermissionPolicy::Rule* manifestRules,
                               size_t manifestRulesCount);

    void CopySecurityInfo(ApplicationInfo& ai,
                          const SecurityInfo& si);

    bool IsPermissionDeniedError(QStatus status,
                                 Message& msg);

    QStatus SerializeManifest(ManagedApplicationInfo& managedAppInfo,
                              const PermissionPolicy::Rule* manifestRules,
                              size_t manifestRulesCount);

    QStatus DeserializeManifest(const ManagedApplicationInfo managedAppInfo,
                                const PermissionPolicy::Rule** manifestRules,
                                size_t* manifestRulesCount);

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
    SecurityManagerConfig config;
    qcc::GUID128 localGuid;
    std::map<qcc::String, PermissionPolicy*> manifestCache;
};
}
}
#undef QCC_MODULE
#endif /* SECURITYMANAGERIMPL_H_ */
