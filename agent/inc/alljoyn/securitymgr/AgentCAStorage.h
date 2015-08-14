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

#ifndef ALLJOYN_SECMGR_AGENTCASTORAGE_H_
#define ALLJOYN_SECMGR_AGENTCASTORAGE_H_

#include <vector>

#include <qcc/CryptoECC.h>
#include <qcc/Crypto.h>

#include <alljoyn/Status.h>

#include "Application.h"
#include "IdentityInfo.h"
#include "GroupInfo.h"
#include "Manifest.h"

using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
/**
 * @brief A vector of MembershipCertificates to emulate a chain of MembershipCertificates.
 * */
typedef vector<MembershipCertificate> MembershipCertificateChain;

/**
 * @brief A vector of IdentityCertificates to emulate a chain of identityCertificate.
 * */
typedef vector<IdentityCertificate> IdentityCertificateChain;

/**
 * @brief StorageListener abstract class.
 *
 * This allows the agent to register itself to the storage in order to receive events for pending
 * changes. This callback can be triggered when a commit is done locally or if the storage receives
 * configuration changes via alternative paths (directly accessed by UI, multiple agents, ...).
 */
class StorageListener {
  public:

    /**
     * Called by storage whenever the applications have new pending changes.
     *
     * @param apps         A vector of applications with new pending changes.
     */
    virtual void OnPendingChanges(vector<Application>& apps) = 0;

    /**
     * Called by storage whenever pending changes on applications will have been handled.
     *
     * @param apps         A vector of applications that have been updated.
     */
    virtual void OnPendingChangesCompleted(vector<Application>& apps) = 0;

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~StorageListener() { }
};

/**
 * @brief The AgentCAStorage abstract class defines all interactions between a security agent and storage.
 *
 * This class provides a given Security Agent with all its needs to register itself, claim applications,
 * (un)register a storage listener, retrieve identity and membership certificates as well as policy and manifest per application.
 */

class AgentCAStorage {
  public:

    /**
     * Default constructor for AgentCAStorage.
     */
    AgentCAStorage() { }

    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AgentCAStorage() { }

    /**
     * @brief Register a Security Agent with storage.
     *
     * @param[in] agentKey                   A KeyInfoNISTP256 unique to this agent.
     * @param[in] manifest                   The manifest used in generating the identity certificate chain
     *                                       for the registering agent.
     * @param[in,out] adminGroup             The group info that will be filled-in for the registering agent.
     * @param[in,out] identityCertificates   The identity certificate chain generated for the registering agent.
     * @param[in,out] adminGroupMemberships  The membership certificate chain generated for the registering agent.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus RegisterAgent(const KeyInfoNISTP256& agentKey,
                                  const Manifest& manifest,
                                  GroupInfo& adminGroup,
                                  IdentityCertificateChain& identityCertificates,
                                  vector<MembershipCertificateChain>& adminGroupMemberships) = 0;

    /**
     * @brief Inform storage that a new application is found and it will be claimed.
     *
     * This method must be called prior to calling the claim method to actually claim
     * the application.
     *
     * @param[in] app                      The application with a valid keyInfo set.
     * @param[in] idInfo                   The identity of the application.
     * @param[in] manifest                 The application's aspired manifest.
     * @param[in,out] adminGroup           The group the application will be part of after claiming.
     * @param[in,out] identityCertificate  The identity certificate that will be generated for
     *                                     the application.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus StartApplicationClaiming(const Application& app,
                                             const IdentityInfo& idInfo,
                                             const Manifest& manifest,
                                             GroupInfo& adminGroup,
                                             IdentityCertificateChain& identityCertificate) = 0;

    /**
     * @brief Inform storage that a new application was claimed with a given success/failure
     *  status.
     *
     * This method must be called after trying to claim an application with the result represented
     * in a boolean.
     *
     * @param[in] app                      The application with a valid keyInfo set.
     * @param[in] status                   The success/failure of the attempted claiming. ER_OK
     *                                     on success, others for failures
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus FinishApplicationClaiming(const Application& app,
                                              QStatus status) = 0;

    /**
     * @brief Retrieve a managed application.
     *
     * @param[in,out] app  The managed application.
     *                     The keyInfo must be set.
     *
     * @return ER_OK  On success
     * @return others On failure
     */
    virtual QStatus GetManagedApplication(Application& app) const = 0;

    /**
     * @brief Inform storage that the agent will start updating a certain application.
     *
     * @param[in,out] app                 The application with a valid keyInfo set.
     *                                    It will be aligned with storage.
     * @param[out] updateID               The transaction id for the current update.
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus StartUpdates(Application& app,
                                 uint64_t& updateID) = 0;

    /**
     * @brief Inform storage that the agent has finished updating a certain application.
     *
     * @param[in,out] app                 The application with a valid keyInfo set.
     *                                    It will be aligned with storage.
     * @param[in,out] updateID            In, is the updateID currently ongoing and
     *                                    out, is the new id if more updates are needed.
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus UpdatesCompleted(Application& app,
                                     uint64_t& updateID) = 0;

    /**
     * @brief Retrieve the public key info from storage.
     *
     * @param[in,out] keyInfoOfCA         The public key info used by this storage.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus GetCaPublicKeyInfo(KeyInfoNISTP256& keyInfoOfCA) const = 0;

    /**
     * @brief Get the admin group of the CA.
     *
     * @param[in,out] groupInfo  The admin group.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus GetAdminGroup(GroupInfo& groupInfo) const = 0;

    /**
     * @brief Retrieve the chain of membership certificates for a given application.
     *
     * @param[in] app                         The application with a valid keyInfo set.
     * @param[in,out] membershipCertificates  The retrieved chains of membership certificates pertaining to the application.
     *                                        Each chain must at least contain 1 certificate.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus GetMembershipCertificates(const Application& app,
                                              vector<MembershipCertificateChain>& membershipCertificates) const = 0;

    /**
     * @brief Retrieve the chain of identity certificates as well as the manifest for a given application.
     *
     * @param[in] app                         The application with a valid keyInfo set.
     * @param[in,out] identityCertificates    The retrieved chain of identity certificates pertaining to the application.
     * @param[in,out] manifest                The retrieved manifest of the application.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus GetIdentityCertificatesAndManifest(const Application& app,
                                                       IdentityCertificateChain& identityCertificates,
                                                       Manifest& manifest) const = 0;

    /**
     * @brief Retrieve the policy of a given application.
     *
     * @param[in] app                         The application with a valid keyInfo set.
     * @param[in,out] policy                  The retrieved policy pertaining to the application.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual QStatus GetPolicy(const Application& app,
                              PermissionPolicy& policy) const = 0;

    /**
     * @brief Register a storage listener with storage.
     *
     * @param[in] listener           The listener to register.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual void RegisterStorageListener(StorageListener* listener) = 0;

    /**
     * @brief Unregister a storage listener from storage.
     *
     * @param[in] listener           The listener to unregister.
     *
     * @return ER_OK  On success.
     * @return others On failure.
     */
    virtual void UnRegisterStorageListener(StorageListener* listener) = 0;
};
}
}

#endif /* ALLJOYN_SECMGR_AGENTCASTORAGE_H_ */
