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

#ifndef SECURITYMANAGER_H_
#define SECURITYMANAGER_H_

#include <alljoyn/Status.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/PermissionPolicy.h>
#include <alljoyn/securitymgr/ApplicationInfo.h>
#include <alljoyn/securitymgr/GuildInfo.h>
#include <alljoyn/securitymgr/IdentityInfo.h>
#include <alljoyn/securitymgr/ApplicationListener.h>
#include <alljoyn/securitymgr/Storage.h>
#include <alljoyn/securitymgr/cert/X509Certificate.h>

#include <memory>

#include <qcc/String.h>
#include <qcc/CryptoECC.h>

#include <qcc/Debug.h>
#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
class SecurityManagerImpl;

/**
 * @brief A class providing a callback for approving the manifest.
 */
class ManifestListener {
  public:
    ManifestListener() { }

    virtual ~ManifestListener() { }

    /**
     * @brief This method is called by the security manager when it requires
     * acceptance of a manifest.
     *
     * \param[in] appInfo             the application info describing the application
     *                                that the manifest belongs to
     * \param[in] manifestRules       the permission rules of the manifest
     * \param[in] manifestRulesCount  the number of permission rules in the
     *                                manifest
     *
     * \retval true if the manifest is approved, false otherwise
     *
     */
    virtual bool ApproveManifest(const ApplicationInfo& appInfo,
                                 const PermissionPolicy::Rule* manifestRules,
                                 const size_t manifestRulesCount) = 0;
};

/**
 * @brief The %SecurityManager allows an administrator to claim remote
 * applications. Once an application is claimed, the security manager
 * can be used to set up their security configurations. It also provides
 * functionality to assign an identity to each application and to group
 * applications in guilds.
 */
class SecurityManager {
  public:
    /**
     * @brief Claim a remote application, making this security manager the
     * sole peer that can change its security configuration. The application
     * should be in CLAIMABLE and RUNNING state, and the identity should be
     * known to the security manager.
     *
     * This method will also retrieve the manifest of the application, that
     * should be approved by the ManifestListener. If no ManifestListener
     * is registered, this method will return ER_FAIL. If the listener rejects
     * the manifest, the application will be automatically reset.
     *
     * Once an application is claimed, its application info together with
     * its manifest is persisted in the local storage.
     *
     * \param[in] appInfo  info of the application that will be claimed
     * \param[in] idInfo   the identity that should be assigned to the
     *                     application
     *
     * \retval ER_OK                 on success
     * \retval ER_MANIFEST_REJECTED  when the ManifestListener rejected the manifest
     *                               and the application was reset successfully
     * \retval ER_FAIL               when no ManifestListener is registered or
     *                               in case of other failures
     */
    QStatus Claim(const ApplicationInfo& appInfo,
                  const IdentityInfo& idInfo);

    /**
     * @brief Registers a ManifestListener to the SecurityManager, which will
     * be called during Claim.
     *
     * This method should not be called when Claim is ongoing. This results in
     * an undefined behavior.
     *
     * \param[in] listener  a new ManifestListener to approve manifest or NULL
     *                      to remove the one currently set; the security manager
     *                      does not take ownership of the passed pointer.
     */
    void SetManifestListener(ManifestListener* listener);

    /**
     * @brief Retrieve the persisted manifest of the application.
     *
     * \param[in] appInfo              info of the application of which the manifest
     *                                 should be retrieved.
     * \param[out] manifestRules       an array of rules that represents the
     *                                 manifest of the application
     * \param[out] manifestRulesCount  the number of rules in the manifestRules
     *                                 array
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    QStatus GetManifest(const ApplicationInfo& appInfo,
                        const PermissionPolicy::Rule** manifestRules,
                        size_t* manifestRulesCount);

    /**
     * @brief Persists an identity certificate for an application. If the
     * remote application is online, the certificate is installed immediately.
     * Otherwise, the membership certificate will be installed when the
     * application comes online.
     *
     * \param[in] app  info of the application for which a new identity certificate
     *                 should be installed
     * \param[in] id   the identity info describing the user of the application
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    QStatus UpdateIdentity(const ApplicationInfo& app,
                           const IdentityInfo& id);

    /**
     * @brief Get the public key of this security manager.
     *
     * \retval publicKey the public key that of this security manager.
     */
    const qcc::ECCPublicKey& GetPublicKey() const;

    /**
     * @brief Get a list of all applications that are known to the security
     * manager.
     *
     * \param[in] state  only return the applications that have a matching
     *                   claimable state. If UNKOWN, then all applications are
     *                   returned.
     *
     * \retval vector<ApplicationInfo>  on success
     * \retval empty vector             otherwise
     */
    std::vector<ApplicationInfo> GetApplications(ajn::PermissionConfigurator::ClaimableState state =
                                                     ajn::PermissionConfigurator::STATE_UNKNOWN) const;

    /**
     * @brief Register a listener that is called whenever an application state
     * is changed.
     *
     * Only new events are sent to the listener. Use GetApplication(s) to get the
     * current state before registering a listener.
     *
     * \param[in] appListener  the application listener that should be
     *                         registered
     */
    void RegisterApplicationListener(ApplicationListener* appListener);

    /**
     * @brief Unregister a previously registered application listener.
     *
     * \param[in] appListener  the application listener that should be
     *                         unregistered
     */
    void UnregisterApplicationListener(ApplicationListener* appListener);

    /**
     * @brief Get the application info based on the provided public key or
     * busName. If the public key is set, the busName will be ignored.
     *
     * \param[in, out] appInfo  the application info containing the public key
     *                          or the busName, to which any information known
     *                          to the security manager will be returned
     *
     * \retval ER_OK  on success
     * \retval ER_END_OF_DATA when no application is found
     */
    QStatus GetApplication(ApplicationInfo& appInfo) const;

    /**
     * @brief Set a user defined name for an application. If a name was
     * previously defined, it will be overwritten. An application needs to be
     * claimed before its user-defined name can be set.
     *
     * \param[in] appInfo  the application info containing the userDefinedName
     *                     to be set
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    QStatus SetApplicationName(ApplicationInfo& appInfo);

    /**
     * @brief Persists a guild to local storage. If a guild id with the same
     * key values was persisted before, it is then updated.
     *
     * \param[in] guildInfo  the info of a given guild; if no authority is set,
     *                       it is automatically set to the public key of the
     *                       security manager
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    QStatus StoreGuild(GuildInfo& guildInfo);

    /**
     * @brief Remove a guild from local storage.
     *
     * \param[in] guildInfo  the info of a given guild; if no authority is set,
     *                       it is automatically set to the public key of the
     *                       security manager
     *
     * \retval ER_OK           on success
     * \retval ER_END_OF_DATA  if guild was not found
     * \retval others          on failure
     */
    QStatus RemoveGuild(GuildInfo& guildInfo);

    /**
     * @brief Retrieve a guild from local storage.
     *
     * \param[in, out] guildInfo  the guild that should be retrieved; the guid
     *                            must be provided; if no authority is set,
     *                            it is automatically set to the public key
     *                            of the security manager
     *
     * \retval ER_OK  on success
     * \retval ER_END_OF_DATA if no data is found
     * \retval others on failure
     */
    QStatus GetGuild(GuildInfo& guildInfo) const;

    /**
     * @brief Retrieve all guilds from local storage.
     *
     * \param[in, out] guildInfos  a vector to which all persisted guilds will
     *                             be pushed
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    QStatus GetGuilds(std::vector<GuildInfo>& guildInfos) const;

    /**
     * @brief Persists an identity to local storage. If an identity with the
     * same key values was persisted before, it is then updated.
     *
     * \param[in] idInfo  the info of a given identity; if no authority is set,
     *                    it is automatically set to the public key of the
     *                    security manager
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    QStatus StoreIdentity(IdentityInfo& idInfo);

    /**
     * @brief Remove an identity from local storage.
     *
     * \param[in] idInfo  the info of an identity; if no authority is set,
     *                    it is automatically set to the public key of the
     *                    security manager
     *
     * \retval ER_OK           on success
     * \retval ER_END_OF_DATA  if identity was not found
     * \retval others          on failure
     */
    QStatus RemoveIdentity(IdentityInfo& idInfo);

    /**
     * @brief Retrieve an identity from local storage.
     *
     * \param[in, out] idInfo  the identity that should be retrieved; the guid
     *                         must be provided; if no authority is set,
     *                         it is automatically set to the public key
     *                         of the security manager
     *
     * \retval ER_OK  on success
     * \retval ER_END_OF_DATA if no data is found
     * \retval others on failure
     */
    QStatus GetIdentity(IdentityInfo& idInfo) const;

    /**
     * @brief Retrieve all identities from local storage.
     *
     * \param[in, out] idInfo  a vector to which all persisted identities will
     *                         be pushed
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    QStatus GetIdentities(std::vector<IdentityInfo>& idInfos) const;

    /**
     * @brief Persists a membership certificate for an application. If the
     * remote application is online, the certificate is installed immediately.
     * Otherwise, the membership certificate will be installed when the
     * application comes online.
     *
     * \param[in] appInfo            the application that should be added to a
     *                               guild
     * \param[in] guildInfo          the guild to which the application should
     *                               added
     * \param[in] authorizationData  the permissions the application will have
     *                               within that guild or NULL to use the
     *                               manifest that was persisted during
     *                               claiming
     *
     * \retval ER_OK  on successful persisting of the the certificate
     * \retval others on failure
     */
    QStatus InstallMembership(const ApplicationInfo& appInfo,
                              const GuildInfo& guildInfo,
                              const PermissionPolicy* authorizationData = NULL);

    /**
     * @brief Removes a membership certificate from persistent storage. If the remote
     * application is online, the certificate is removed immediately.
     * Otherwise, the certificate will be removed when the application comes
     * online.
     *
     * \param[in] appInfo    the application for which the membership
     *                       certificate should be removed
     * \param[in] guildInfo  the guild from which the application should be
     *                       removed
     *
     * \retval ER_OK           on successful removal of the certificate from
     *                         persistent storage
     * \retval ER_END_OF_DATA  when certificate could not be found
     * \retval others          on failure
     */
    QStatus RemoveMembership(const ApplicationInfo& appInfo,
                             const GuildInfo& guildInfo);

    /**
     * @brief Update a policy on an application. This method always persists
     * the policy and might update the remote application if it is online.
     * If the serial number of the policy == 0, this method automatically
     * determines the next serial number for that application, based on the
     * latest persisted policy for that application.
     *
     * \param[in] appInfo the application on which the policy should be
     * installed.
     * \param[in] policy the policy that needs to be installed on the
     * application.
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    QStatus UpdatePolicy(const ApplicationInfo& appInfo,
                         PermissionPolicy& policy);

    /**
     * @brief Retrieve the policy of an  application.
     *
     * \param[in] appInfo      the application from which the policy should be
     *                         received
     * \param[in, out] policy  the policy of the application
     *
     * \retval ER_OK             on success
     * \retval ER_END_OF_DATA    no persisted policy for this application
     * \retval others            on failure
     */
    QStatus GetPolicy(const ApplicationInfo& appInfo,
                      PermissionPolicy& policy);

    /**
     * @brief Removes any security configuration from a remote application. It
     * removes any installed root of trust, identity certificate, membership
     * certificate and policy. This method also removes any reference to the
     * application from local storage.
     *
     * \param[in] appInfo the application from which the security config will
     *                    be removed
     *
     * \retval ER_OK  on success
     * \retval others on failure
     */
    QStatus Reset(const ApplicationInfo& appInfo);

    ~SecurityManager();

  private:
    /* This constructor can only be called by the factory */
    SecurityManager(ajn::BusAttachment* ba,
                    const Storage* storage);
    SecurityManager(const SecurityManager&);
    QStatus Init();

    void operator=(const SecurityManager&);

    friend class SecurityManagerFactory;

  private:
    SecurityManagerImpl* securityManagerImpl;
};
}
}
#undef QCC_MODULE
#endif /* SECURITYMANAGER_H_ */
