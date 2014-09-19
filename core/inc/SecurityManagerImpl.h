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

#include <RootOfTrust.h>
#include <ApplicationListener.h>
#include <ApplicationInfo.h>
#include <ApplicationMonitor.h>
#include <CertificateGenerator.h>

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
    public SessionListener,
    public ajn::services::AnnounceHandler,
    private ajn::BusAttachment::JoinSessionAsyncCB,
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

    QStatus ClaimApplication(const ApplicationInfo& app);

    QStatus InstallIdentity(const ApplicationInfo& app);

    QStatus AddRootOfTrust(const ApplicationInfo& app,
                           const RootOfTrust& rot);

    QStatus RemoveRootOfTrust(const ApplicationInfo& app,
                              const RootOfTrust& rot);

    const RootOfTrust& GetRootOfTrust() const;

    std::vector<ApplicationInfo> GetApplications(
        ajn::securitymgr::ApplicationClaimState acs = ajn::securitymgr::ApplicationClaimState::UNKNOWN) const;

    void RegisterApplicationListener(ApplicationListener* al);

    void UnregisterApplicationListener(ApplicationListener* al);

    ApplicationInfo* GetApplication(qcc::String& busName) const;

    QStatus GetStatus() const;

  private:

    qcc::String GetString(::ajn::services::PropertyStoreKey key, const AboutData &aboutData) const;

    qcc::String GetAppId(const AboutData& aboutData) const;

    QStatus CreateInterface(ajn::BusAttachment* bus,
                            ajn::InterfaceDescription*& intf);

    QStatus EstablishPSKSession(const ApplicationInfo& app,
                                uint8_t* bytes,
                                size_t size);

    /* ajn::BusAttachment::JoinSessionAsyncCB */
    virtual void JoinSessionCB(QStatus status,
                               ajn::SessionId id,
                               const ajn::SessionOpts& opts,
                               void* context);

    /* ajn::SessionListener */
    virtual void SessionLost(ajn::SessionId sessionId,
                             SessionLostReason reason);

    virtual void OnSecurityStateChange(const SecurityInfo* oldSecInfo,
                                       const SecurityInfo* newSecInfo);

    void Announce(unsigned short version,
                  SessionPort port,
                  const char* busName,
                  const ObjectDescriptions& objectDescs,
                  const AboutData& aboutData);

  private:
    IdentityData* id;
    const qcc::ECCPrivateKey privKey;
    const RootOfTrust rot;  // !!! Verify this (added to get compiled)!!
    std::unique_ptr<Storage> storage;
    StorageConfig storageCfg;
    std::map<qcc::String, ApplicationInfo> applications; /* key=key of app, value = info */
    std::map<qcc::String, ApplicationInfo> aboutCache; /* key=busname of app, value = info */
    std::vector<ApplicationListener*> listeners;
    std::unique_ptr<CertificateGenerator> CertificateGen;
    QStatus status;
    std::unique_ptr<ApplicationMonitor> am;
    ajn::BusAttachment* busAttachment;
};
}
}
#undef QCC_MODULE
#endif /* SECURITYMANAGERIMPL_H_ */
