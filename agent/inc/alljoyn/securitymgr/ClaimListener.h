/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#ifndef ALLJOYN_SECMGR_CLAIMLISTENER_H_
#define ALLJOYN_SECMGR_CLAIMLISTENER_H_

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
 * @brief A class providing details and choices for a specific claim action
 */
class ClaimContext {
  public:
    /**
     * @brief retrieves the information on the application to be claimed.
     *
     * @return the OnlineApplication to claim.
     */
    const OnlineApplication& GetApplication() const
    {
        return app;
    }

    /**
     * @brief retrieves the manifest (template) requested by the application.
     *
     * This manifest should be approved or rejected by calling ApproveManifest.
     *
     * @return The manifest requested by the application.
     */
    const Manifest& GetManifest() const
    {
        return mnf;
    }

    /**
     * @brief returns the claim schemes supported by the application.
     *
     * @return A bit mask of supported claim schemes
     * @see PermissionConfigurator::CAPABLE_ECDHE_NULL
     * @see PermissionConfigurator::CAPABLE_ECDHE_PSK
     * @see PermissionConfigurator::CAPABLE_ECDHE_ECDSA
     */
    const PermissionConfigurator::ClaimCapabilities& GetClaimCapabilities() const
    {
        return capabilities;
    }

    /**
     * @brief returns the additional claim schemes info supported by the application.
     *
     * @return A bit mask of supported claim schemes info
     * @see PermissionConfigurator::PSK_GENERATED_BY_SECURITY_MANAGER
     * @see PermissionConfigurator::PSK_GENERATED_BY_APPLICATION
     */
    const PermissionConfigurator::ClaimCapabilityAdditionalInfo GetClaimCapabilityInfo() const
    {
        return capInfo;
    }

    /**
     * A constant indicating not claim type is selected
     */
    static const PermissionConfigurator::ClaimCapabilities CLAIM_TYPE_NOT_SET;

    /**
     * @brief selects a claim type for this claim action.
     *
     * The provided claim type must be supported by the application.
     *
     * @param[in] newType the claim type to be used within this claim context.
     * @return ER_OK in case of success, ER_BAD_ARG1 if the new claim type is not valid
     * @see PermissionConfigurator::CAPABLE_ECDHE_NULL
     * @see PermissionConfigurator::CAPABLE_ECDHE_PSK
     * @see PermissionConfigurator::CAPABLE_ECDHE_ECDSA
     */
    QStatus SetClaimType(PermissionConfigurator::ClaimCapabilities newType)
    {
        QStatus status;
        switch (newType) {
        case PermissionConfigurator::CAPABLE_ECDHE_NULL: //deliberate fall through
        case PermissionConfigurator::CAPABLE_ECDHE_PSK:  //deliberate fall through
        case PermissionConfigurator::CAPABLE_ECDHE_ECDSA:
            if ((newType & capabilities) == 0) {
                return ER_BAD_ARG_1;
            }
            claimType = newType;
            status = ER_OK;
            break;

        default:
            status = ER_BAD_ARG_1;
        }
        return status;
    }

    /**
     * @brief retrieves the current selected claim type.
     *
     * When no claim type is set on this context, it will return CLAIM_TYPE_NOT_SET.
     *
     * @return the current selected claim type
     */
    PermissionConfigurator::ClaimCapabilities GetClaimType() const
    {
        return claimType;
    }

    /**
     * @brief retrieves the manifest approval status.
     *
     * @return true if the manifest is approved, false otherwise.
     */
    bool IsManifestApproved() const
    {
        return manifestApproved;
    }

    /**
     * @brief Updates the manifest approval status
     *
     * @param[in] true if the manifest is approved, false otherwise.
     */
    void ApproveManifest(bool approved = true)
    {
        manifestApproved = approved;
    }

    /**
     * @brief sets a pre-shared key to be used for this claim action
     *
     * param[in] psk the pre-shared key data or NULL to removed the previous set data. No transfer
     *               of ownership of memory.
     * param[in] pskSize The size of the pre-shared data. The size must be 16 or greater.
     *
     * @return ER_OK in case of success, an error code in all other cases
     */
    virtual QStatus SetPreSharedKey(const uint8_t* psk,
                                    size_t pskSize) = 0;

    /**
     * The destructor of this ClaimContext
     */
    virtual ~ClaimContext() { }

  protected:
    ClaimContext(const OnlineApplication& application,
                 const Manifest& manifest,
                 const PermissionConfigurator::ClaimCapabilities _capabilities,
                 const PermissionConfigurator::ClaimCapabilityAdditionalInfo _capInfo) :
        app(application), mnf(manifest), capabilities(_capabilities), capInfo(_capInfo), claimType(CLAIM_TYPE_NOT_SET),
        manifestApproved(
            false) { }

  private:

    OnlineApplication app;
    Manifest mnf;
    PermissionConfigurator::ClaimCapabilities capabilities;
    PermissionConfigurator::ClaimCapabilityAdditionalInfo capInfo;

    PermissionConfigurator::ClaimCapabilities claimType;
    bool manifestApproved;
};

/**
 * @brief A class providing a callback for approving the manifest.
 */
class ClaimListener {
  public:
    ClaimListener() { }

    virtual ~ClaimListener() { }

    /**
     * @brief This method is called by the security agent during the claim.
     *
     * A ClaimContext is passed containing the details of the claim request.
     *
     * @param[in,out] claimContext  The claim context contains the scope of the current ongoing
     *                              claim action. The context requires the listener to explicit
     *                              approve or deny the manifest and to select a session type to
     *                              do the actual claim on. Failing to approve the manifest or to
     *                              select the session type will cause the claim to fail.
     *
     * @return ER_OK for success, any other code will cause the claim method to return with
     *            this other code.
     *
     */
    virtual QStatus ApproveManifestAndSelectSessionType(ClaimContext& claimContext) = 0;
};
}
}
#endif /* ALLJOYN_SECMGR_CLAIMLISTENER_H_ */
