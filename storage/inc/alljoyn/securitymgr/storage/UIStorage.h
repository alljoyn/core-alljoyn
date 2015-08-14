/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#ifndef ALLJOYN_SECMGR_STORAGE_UISTORAGE_H_
#define ALLJOYN_SECMGR_STORAGE_UISTORAGE_H_

#include <vector>
#include <memory>

#include <qcc/CertificateECC.h>
#include <qcc/CryptoECC.h>

#include <alljoyn/Status.h>

#include <alljoyn/securitymgr/Application.h>
#include <alljoyn/securitymgr/IdentityInfo.h>
#include <alljoyn/securitymgr/Manifest.h>
#include <alljoyn/securitymgr/GroupInfo.h>
#include <alljoyn/securitymgr/AgentCAStorage.h>

#include "ApplicationMetaData.h"

namespace ajn {
namespace securitymgr {
/**
 * @brief An abstract class that is meant to define the
 *        interfacing with a persistent storage means.
 *
 *  Applications and Groups can be managed persistently through this API.
 *
 */
class UIStorage {
  public:

    /**
     * @brief Persist a generated membership certificate for the application.
     *
     * @param[in] app             The application, ONLY the keyInfo is mandatory here.
     * @param[in] groupInfo       A valid groupInfo.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus InstallMembership(const Application& app,
                                      const GroupInfo& groupInfo) = 0;

    /**
     * @brief Remove a given membership certificate for an application from persistency.
     *
     * @param[in] app             The application, ONLY the keyInfo is mandatory here.
     * @param[in] groupInfo       A valid groupInfo.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus RemoveMembership(const Application& app,
                                     const GroupInfo& groupInfo) = 0;

    /**
     * @brief Update the application's policy in persistency.
     *
     * @param[in] app             The application, ONLY the keyInfo is mandatory here.
     * @param[in] policy          A policy.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus UpdatePolicy(Application& app,
                                 PermissionPolicy& policy) = 0;

    /**
     * @brief Retrieve the application's policy from persistency.
     *
     * @param[in] app             The application, ONLY the keyInfo is mandatory here.
     * @param[in] policy          A policy.
     *
     * @return ER_OK  On success.
     * @return ER_END_OF_DATA     If no data is found.
     * @return others On failure.
     */
    virtual QStatus GetPolicy(const Application& app,
                              PermissionPolicy& policy) = 0;

    /**
     * @brief Remove the policy of an application from persistency.
     *
     * @param[in] app             The application, ONLY the keyInfo is mandatory here.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus RemovePolicy(Application& app) = 0;

    /**
     * @brief Update the application's identity in persistency.
     *
     * @param[in] app             The application, ONLY the keyInfo is mandatory here.
     * @param[in] identityInfo    A valid identity info.
     * @param[in] manifest        The manifest for this application.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus UpdateIdentity(Application& app,
                                   const IdentityInfo& identityInfo,
                                   const Manifest& manifest) = 0;

    /**
     * @brief Persist the meta application data relevant to the app passed in.
     *
     * @param[in] app               the application, ONLY the publicKey is mandatory here.
     * @param[in] appMetaData       the meta application data to presist.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus SetAppMetaData(const Application& app,
                                   const ApplicationMetaData& appMetaData) = 0;

    /**
     * @brief Retrieve the persisted meta application data relevant to the app passed in.
     *
     * @param[in] app               The application, ONLY the publicKey is mandatory here.
     * @param[in,out] appMetaData   The retrieved meta application data.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus GetAppMetaData(const Application& app,
                                   ApplicationMetaData& appMetaData) const = 0;

    /**
     * @brief Remove a previously managed application, including
     *        its certificates.
     *
     * @param[in] application the application, ONLY the publicKey is mandatory here
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus RemoveApplication(Application& app) = 0;

    /**
     * @brief Retrieve a list of managed applications.
     *
     * @param[in,out] apps A vector of managed applications.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus GetManagedApplications(vector<Application>& apps) const = 0;

    /**
     * @brief Get a managed application if it already exists.
     *
     *
     * @param[in] app  The managed application to be filled in.
     *                 Only the publicKey field is required
     *
     * @return ER_OK           On success.
     * @return ER_END_OF_DATA  If no data is found.
     * @return others          On failure.
     */
    virtual QStatus GetManagedApplication(Application& app) const = 0;

    /**
     * @brief Store a group. If a group with the same keys was stored before,
     * it will be updated.
     *
     * @param[in] groupInfo  The info of a group that needs to be stored;
     *                       both authority and guid must be provided.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus StoreGroup(GroupInfo& groupInfo) = 0;

    /**
     * @brief Remove a group from storage.
     *
     * @param[in] groupInfo  The info of a group that needs to be removed;
     *                       both authority and guid must be provided.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus RemoveGroup(const GroupInfo& groupInfo) = 0;

    /**
     * @brief Get the stored info for a provided group.
     *
     * @param[in,out] groupInfo   The info of a group that should be retrieved;
     *                            both authority and guid must be provided.
     *
     * @return ER_OK           On success.
     * @return ER_END_OF_DATA  If no data is found.
     * @return others          On failure.
     */
    virtual QStatus GetGroup(GroupInfo& groupInfo) const = 0;

    /**
     * @brief Get all stored group information.
     *
     * @param[in,out] groupInfos   A vector to which any stored group info
     *                             object will be pushed.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus GetGroups(vector<GroupInfo>& groupInfos) const = 0;

    /**
     * @brief Store an identity. If an identity with the same keys was stored
     * before, it will be updated.
     *
     * @param[in] idInfo  The info of an identity that needs to be stored;
     *                    both authority and guid must be provided.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus StoreIdentity(IdentityInfo& idInfo) = 0;

    /**
     * @brief Remove an identity from storage.
     *
     * @param[in] idInfo  The info of an identity that needs to be removed;
     *                    both authority and guid must be provided.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus RemoveIdentity(const IdentityInfo& idInfo) = 0;

    /**
     * @brief Get the stored info for a provided identity.
     *
     * @param[in,out] idInfo   The info of an identity that should be
     *                         retrieved; both authority and guid must be
     *                         provided.
     *
     * @return ER_OK           On success.
     * @return ER_END_OF_DATA  If no data is found.
     * @return others          On failure.
     */
    virtual QStatus GetIdentity(IdentityInfo& idInfo) const = 0;

    /**
     * @brief Get all stored identity information.
     *
     * @param[in,out] idInfos  a vector to which any stored identity info
     *                          object will be pushed
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus GetIdentities(vector<IdentityInfo>& idInfos) const = 0;

    /**
     * @brief Retrieve the application's manifest from persistency.
     *
     * @param[in] app             The application, ONLY the keyInfo is mandatory here.
     * @param[in] policy          A policy.
     *
     * @return ER_OK              On success.
     * @return ER_END_OF_DATA     If no data is found.
     * @return others             On failure.
     */
    virtual QStatus GetManifest(const Application& app,
                                Manifest& manifest) const = 0;

    /**
     * @brief Get the admin group of the CA. Members of this group should have
     * the rights to change the security configuration of applications claimed
     * by this CA. When updating the policy of such an application, the policy
     * should contain a special section allowing members of this group to do so.
     *
     * @param[in,out] groupInfo  The admin group.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus GetAdminGroup(GroupInfo& groupInfo) const = 0;

    /**
     * @brief Reset the storage and delete the database.
     */
    virtual void Reset() = 0;

    /**
     * @brief Get the CaStorage linked to this UIStorage.
     *
     * @param[out] caStorageRef         Will be updated with CaStorage ref in case of success.
     *
     * @return ER_OK                    If successful. An error code otherwise.
     */
    virtual QStatus GetCaStorage(shared_ptr<AgentCAStorage>& caStorageRef) = 0;

    virtual ~UIStorage()
    {
    }
};
}
}
#endif /* ALLJOYN_SECMGR_STORAGE_UISTORAGE_H_ */
