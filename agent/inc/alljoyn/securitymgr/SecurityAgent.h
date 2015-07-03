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

#ifndef ALLJOYN_SECMGR_SECURITYAGENT_H_
#define ALLJOYN_SECMGR_SECURITYAGENT_H_

#include <memory>

#include <alljoyn/Status.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/PermissionConfigurator.h>

#include "AgentCAStorage.h"
#include "Application.h"
#include "IdentityInfo.h"
#include "ApplicationListener.h"
#include "Manifest.h"

using namespace qcc;

namespace ajn {
namespace securitymgr {
/**
 * @brief A class providing a callback for approving the manifest.
 */
class ManifestListener {
  public:
    ManifestListener() { }

    virtual ~ManifestListener() { }

    /**
     * @brief This method is called by the security agent when it requires
     * acceptance of a manifest.
     *
     * @param[in] app                 The application.
     *                                That the manifest belongs to.
     * @param[in] manifest            The manifest the application requested.
     *
     * @return true if the manifest is approved, false otherwise.
     *
     */
    virtual bool ApproveManifest(const OnlineApplication& app,
                                 const Manifest& manifest) = 0;
};

class SecurityAgentImpl;

class SecurityAgent {
  public:

    /**
     * @brief Claim a remote application, making this security agent the
     * sole peer that can change its security configuration. The application
     * should be online and in CLAIMABLE state, and the identity should be
     * known to the security agent.
     *
     * This method will also fetch the manifest of the application, that
     * should be approved by the ManifestListener. If no ManifestListener
     * is registered, this method will return ER_FAIL. If the listener rejects
     * the manifest, the application will be automatically reset.
     *
     * Once an application is claimed, it is persisted together with
     * its manifest in the local storage.
     *
     * @param[in] app      The application that will be claimed.
     * @param[in] idInfo   The identity that should be assigned to the
     *                     application.
     *
     * @return ER_OK                 On success.
     * @return ER_MANIFEST_REJECTED  When the ManifestListener rejects the manifest.
     * @return ER_FAIL               When no ManifestListener is registered or
     *                               in case of other failures.
     */
    virtual QStatus Claim(const OnlineApplication& app,
                          const IdentityInfo& idInfo) = 0;

    /**
     * @brief Register a ManifestListener to the security agent, which will
     * be called during Claim.
     *
     * This method should not be called when Claim is ongoing. This results in
     * an undefined behavior.
     *
     * @param[in] listener  A new ManifestListener to approve manifest or nullptr
     *                      to remove the one currently set; the security agent
     *                      does not take ownership of the passed pointer.
     */
    virtual void SetManifestListener(ManifestListener* listener) = 0;

    /**
     * @brief Add an ApplicationListener to the security agent.
     * Listener(s) will be used to notify about application(s) state changes
     * as well as about any synchronization errors that could emerge.

     * @param[in] listener  A new ApplicationListener. The security agent does
     *                      not take ownership of the passed pointer.
     */
    virtual void RegisterApplicationListener(ApplicationListener* applicationListener) = 0;

    /**
     * @brief Remove a previously registered ApplicationListener.
     *
     * @param[in] listener  Previously registered ApplicationListener.
     */
    virtual void UnregisterApplicationListener(ApplicationListener* applicationListener) = 0;

    /**
     * @brief Retrieve all running applications based on a specific claimable state;
     *   NOT_CLAIMABLE
     *   CLAIMABLE
     *   CLAIMED
     *   \see{PermissionConfigurator::ApplicationState}
     *
     * @param[in]     applicationState The claimable state you wish to filter online apps on.
     *                                 Default is CLAIMABLE.
     * @param[in,out] apps             The applications filtered by the claimable state selected.
     */
    virtual QStatus GetApplications(vector<OnlineApplication>& apps,
                                    const PermissionConfigurator::ApplicationState applicationState =
                                        PermissionConfigurator::CLAIMABLE) const = 0;

    /**
     * @brief Retrieve the online status of the application.
     *
     * @param[in,out] app   The application to get the status for.
     */
    virtual QStatus GetApplication(OnlineApplication& app) const = 0;

    /**
     * @brief This method will synchronize (all) claimed applications with the
     * persistent storage in an asynchronous manner.
     * By synchronization we mean a best-effort operation to line-up potentially
     * persisted updates, (e.g., new policy etc.) with (all) claimed applications.
     *
     * @param[in] app               The application that needs to be synchronized.
     *                              In case of nullptr, all claimed applications
     *                              will be synchronized.
     *
     */
    virtual void UpdateApplications(const vector<OnlineApplication>* apps = nullptr) = 0;

    /**
     * @brief This method will return the assigned public key info of the security agent.
     *
     * @return  The public key info used by this security agent.
     *
     */
    virtual const KeyInfoNISTP256& GetPublicKeyInfo() const = 0;

    /**
     * @brief Virtual destructor for derivable class.
     */
    virtual ~SecurityAgent() { }
};
}
}
#endif /* ALLJOYN_SECMGR_SECURITYAGENT_H_ */
