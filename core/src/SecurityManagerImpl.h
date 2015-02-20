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

#ifndef SECURITYMANAGERIMPL_H_
#define SECURITYMANAGERIMPL_H_

#include <alljoyn/about/AnnounceHandler.h>
#include <alljoyn/Status.h>
#include <alljoyn/PermissionPolicy.h>
#include <qcc/CryptoECC.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/GUID.h>

#include <alljoyn/securitymgr/ApplicationListener.h>
#include <alljoyn/securitymgr/ApplicationInfo.h>
#include <alljoyn/securitymgr/ManagedApplicationInfo.h>
#include <alljoyn/securitymgr/GuildInfo.h>
#include <alljoyn/securitymgr/IdentityInfo.h>
#include <alljoyn/securitymgr/SecurityManager.h>
#include <alljoyn/securitymgr/Storage.h>

#include "ApplicationMonitor.h"
#include "X509CertificateGenerator.h"
#include "ProxyObjectManager.h"
#include "RemoteApplicationManager.h"
#include "ApplicationUpdater.h"
#include "TaskQueue.h"

#include <memory>

#define QCC_MODULE "SEC_MGR"

using namespace qcc;

namespace ajn {
namespace securitymgr {
class Identity;
class SecurityInfoListener;
struct SecurityInfo;

/**
 * \class SecurityManagerImpl
 *
 * \brief the class provides for the SecurityManager implementation hiding
 */
class AppInfoEvent {
  public:
    AppInfoEvent(const ApplicationInfo* oldInfo,
                 const ApplicationInfo* newInfo) :
        oldAppInfo(oldInfo), newAppInfo(newInfo)
    {
    }

    ~AppInfoEvent()
    {
        delete oldAppInfo;
        delete newAppInfo;
    }

    const ApplicationInfo* oldAppInfo;
    const ApplicationInfo* newAppInfo;
};

class SecurityManagerImpl :
    public ajn::services::AnnounceHandler,
    private SecurityInfoListener {
  public:

    SecurityManagerImpl(ajn::BusAttachment* ba,
                        const Storage* _storage);

    QStatus Init();

    ~SecurityManagerImpl();

    void SetManifestListener(ManifestListener* listerner);

    QStatus Claim(const ApplicationInfo& app,
                  const IdentityInfo& identityInfo);

    QStatus GetManifest(const ApplicationInfo& appInfo,
                        const PermissionPolicy::Rule** manifestRules,
                        size_t* manifestRulesCount);

    QStatus UpdateIdentity(const ApplicationInfo& app,
                           const IdentityInfo& id);

    const qcc::ECCPublicKey& GetPublicKey() const;

    std::vector<ApplicationInfo> GetApplications(ajn::PermissionConfigurator::ClaimableState acs =
                                                     ajn::PermissionConfigurator::STATE_UNKNOWN)
    const;

    void RegisterApplicationListener(ApplicationListener* al);

    void UnregisterApplicationListener(ApplicationListener* al);

    QStatus GetApplication(ApplicationInfo& ai) const;

    QStatus SetApplicationName(ApplicationInfo& appInfo);

    QStatus StoreGuild(GuildInfo& guildInfo);

    QStatus RemoveGuild(GuildInfo& guildInfo);

    QStatus GetGuild(GuildInfo& guildInfo) const;

    QStatus GetGuilds(std::vector<GuildInfo>& guildInfos) const;

    QStatus StoreIdentity(IdentityInfo& idInfo);

    QStatus RemoveIdentity(IdentityInfo& idInfo);

    QStatus GetIdentity(IdentityInfo& idInfo) const;

    QStatus GetIdentities(std::vector<IdentityInfo>& idInfos) const;

    QStatus InstallMembership(const ApplicationInfo& appInfo,
                              const GuildInfo& guildInfo,
                              const PermissionPolicy* authorizationData);

    QStatus RemoveMembership(const ApplicationInfo& appInfo,
                             const GuildInfo& guildInfo);

    QStatus UpdatePolicy(const ApplicationInfo& appInfo,
                         PermissionPolicy& policy);

    QStatus GetPolicy(const ApplicationInfo& appInfo,
                      PermissionPolicy& policy);

    QStatus Reset(const ApplicationInfo& appInfo);

    // TODO: move to ECCPublicKey class
    static QStatus MarshalPublicKey(const ECCPublicKey* pubKey,
                                    const GUID128& peerID,
                                    MsgArg& ma);

    // TODO: move to ECCPublicKey class
    static QStatus UnmarshalPublicKey(const MsgArg* ma,
                                      ECCPublicKey& pubKey);

    void HandleTask(AppInfoEvent* event);

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

    QStatus GenerateIdentityCertificate(X509IdentityCertificate& idCert,
                                        const IdentityInfo& idInfo,
                                        const ApplicationInfo& appInfo);

    QStatus PersistApplication(const ApplicationInfo& appInfo,
                               bool update = false,
                               const PermissionPolicy::Rule* manifestRules = NULL,
                               size_t manifestRulesCount = 0);

    /**
     * \brief Persists the manifest to storage. Assumes the application is
     * already persisted using PersistApplication.
     *
     * \param[in] appInfo      the application for which the policy should
     *                         be retrieved from storage
     * \param[out] policy      the persisted policy of the application, iff
     *                         the function returns ER_OK
     *
     * \retval ER_OK           on success
     * \retval ER_END_OF_DATA  no known policy for application
     * \retval others          on failure
     */
    QStatus GetPersistedPolicy(const ApplicationInfo& appInfo,
                               PermissionPolicy& policy);

    QStatus PartialClaim(const ApplicationInfo& app,
                         const IdentityInfo& identityInfo);

    void AddAboutInfo(ApplicationInfo& si);

    void AddSecurityInfo(ApplicationInfo& ai,
                         const SecurityInfo& si);

    void RemoveSecurityInfo(ApplicationInfo& ai,
                            const SecurityInfo& si);

    QStatus SerializeManifest(ManagedApplicationInfo& managedAppInfo,
                              const PermissionPolicy::Rule* manifestRules,
                              size_t manifestRulesCount);

    QStatus DeserializeManifest(const ManagedApplicationInfo managedAppInfo,
                                const PermissionPolicy::Rule** manifestRules,
                                size_t* manifestRulesCount);

    void NotifyApplicationListeners(const ApplicationInfo* oldAppInfo,
                                    const ApplicationInfo* newAppInfo);

  private:
    qcc::ECCPublicKey pubKey;
    ApplicationInfoMap applications;
    std::map<qcc::String, ApplicationInfo> aboutCache; /* key=busname of app, value = info */
    std::vector<ApplicationListener*> listeners;
    X509CertificateGenerator* CertificateGen;
    RemoteApplicationManager* remoteApplicationManager;
    ProxyObjectManager* proxyObjMgr; // To be removed once remoteApplicationManager will provide all needed remote calls.
    ApplicationUpdater* applicationUpdater;
    ApplicationMonitor* appMonitor;
    ajn::BusAttachment* busAttachment;
    Storage* storage;
    mutable qcc::Mutex appsMutex;
    mutable qcc::Mutex applicationListenersMutex;
    qcc::Mutex aboutCacheMutex;
    qcc::GUID128 localGuid;
    std::map<qcc::String, PermissionPolicy*> manifestCache;
    TaskQueue<AppInfoEvent*, SecurityManagerImpl> queue;
    ManifestListener* mfListener;
};
}
}
#undef QCC_MODULE
#endif /* SECURITYMANAGERIMPL_H_ */
