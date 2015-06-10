#ifndef _ALLJOYN_PERMISSION_CONFIGURATOR_H
#define _ALLJOYN_PERMISSION_CONFIGURATOR_H
/**
 * @file
 * This file defines the Permission Configurator class that exposes some permission
 * management capabilities to the application.
 */

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

#ifndef __cplusplus
#error Only include PermissionConfigurator.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/CertificateECC.h>
#include <alljoyn/Status.h>
#include <alljoyn/PermissionPolicy.h>

namespace ajn {

/**
 * Class to allow the application to manage some limited permission feature.
 */

class PermissionConfigurator {

  public:

    typedef enum {
        NOT_CLAIMABLE = 0, ///< The application is not claimed and not accepting claim requests.
        CLAIMABLE = 1,     ///< The application is not claimed and is accepting claim requests.
        CLAIMED = 2,       ///< The application is claimed and can be configured.
        NEED_UPDATE = 3    ///< The application is claimed, but requires a configuration update (after a software upgrade).
    } ApplicationState;

    /**@name ClaimCapabilities */
    // {@
    typedef uint16_t ClaimCapabilities;
    static const ClaimCapabilities CAPABLE_ECDHE_NULL      = 0x01;
    static const ClaimCapabilities CAPABLE_ECDHE_PSK = 0x02;
    static const ClaimCapabilities CAPABLE_ECDHE_ECDSA  = 0x04;
    // @}

    /**@name ClaimCapabilityAdditionalInfo */
    // {@
    typedef uint16_t ClaimCapabilityAdditionalInfo;
    static const ClaimCapabilityAdditionalInfo PSK_GENERATED_BY_SECURITY_MANAGER = 0x01;
    static const ClaimCapabilityAdditionalInfo PSK_GENERATED_BY_APPLICATION = 0x02;
    // @}
    /**
     * Constructor
     *
     */
    PermissionConfigurator()
    {
    }

    /**
     * virtual destructor
     */
    virtual ~PermissionConfigurator()
    {
    }

    /**
     * Set the permission manifest for the application.
     * @params rules the permission rules.
     * @params count the number of permission rules
     * @return ER_OK if successful; otherwise, an error code.
     */
    virtual QStatus SetPermissionManifest(PermissionPolicy::Rule* rules, size_t count) = 0;

    /**
     * Retrieve the state of the application.
     * @param[out] applicationState the application state
     * @return
     *      - #ER_OK if successful
     *      - #ER_NOT_IMPLEMENTED if the method is not implemented
     *      - #ER_FEATURE_NOT_AVAILABLE if the value is not known
     */
    virtual QStatus GetApplicationState(ApplicationState& applicationState)
    {
        QCC_UNUSED(applicationState);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Set the application state.  The state can't be changed from CLAIMED to
     * CLAIMABLE.
     * @param newState The new application state
     * @return
     *      - #ER_OK if action is allowed.
     *      - #ER_INVALID_APPLICATION_STATE if the state can't be changed
     *      - #ER_NOT_IMPLEMENTED if the method is not implemented
     */
    virtual QStatus SetApplicationState(ApplicationState newState)
    {
        QCC_UNUSED(newState);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Generate the signing key pair and store it in the key store.
     * @return ER_OK if successful; otherwise, an error code.
     */
    virtual QStatus GenerateSigningKeyPair()
    {
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Retrieve the public key info fo the signing key.
     * @param[out] the public key info
     * @return ER_OK if successful; otherwise, an error code.
     */
    virtual QStatus GetSigningPublicKey(qcc::KeyInfoECC& keyInfo)
    {
        QCC_UNUSED(keyInfo);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Sign the X509 certificate using the signing key
     * @param[out] the certificate to be signed
     * @return ER_OK if successful; otherwise, an error code.
     */
    virtual QStatus SignCertificate(qcc::CertificateX509& cert)
    {
        QCC_UNUSED(cert);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Reset the Permission module by removing all the trust anchors, DSA keys,
     * installed policy and certificates.  This call must be invoked after the
     * bus attachment has enable peer security.
     * @return ER_OK if successful; otherwise, an error code.
     * @see BusAttachment::EnablePeerSecurity
     */
    virtual QStatus Reset()
    {
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Get the connected peer ECC public key if the connection uses the
     * ECDHE_ECDSA key exchange.
     * @param guid the peer guid
     * @param[out] the buffer to hold the ECC public key.
     * @return ER_OK if successful; otherwise, error code.
     */
    virtual QStatus GetConnectedPeerPublicKey(const qcc::GUID128& guid, qcc::ECCPublicKey* publicKey)
    {
        QCC_UNUSED(guid);
        QCC_UNUSED(publicKey);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Set the authentication mechanisms the application supports for the
     * claim process.  It is a bit mask.
     *
     * | Mask  | Description                                                   |
     * |-------|---------------------------------------------------------------|
     * | 0x1   | claiming via ECDHE_NULL                                       |
     * | 0x2   | claiming via ECDHE_PSK                                        |
     * | 0x4   | claiming via ECDHE_ECDSA                                      |
     *
     * @param[in] claimCapabilities The authentication mechanisms the application supports
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    virtual QStatus SetClaimCapabilities(PermissionConfigurator::ClaimCapabilities claimCapabilities)
    {
        QCC_UNUSED(claimCapabilities);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Get the authentication mechanisms the application supports for the
     * claim process.
     *
     * @param[out] claimCapabilities The authentication mechanisms the application supports
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    virtual QStatus GetClaimCapabilities(PermissionConfigurator::ClaimCapabilities& claimCapabilities)
    {
        QCC_UNUSED(claimCapabilities);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Set the additional information on the claim capabilities.
     * It is a bit mask.
     *
     * | Mask  | Description                                                   |
     * |-------|---------------------------------------------------------------|
     * | 0x1   | PSK generated by Security Manager                             |
     * | 0x2   | PSK generated by application                                  |
     *
     * @param[in] additionalInfo The additional info
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    virtual QStatus SetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo additionalInfo)
    {
        QCC_UNUSED(additionalInfo);
        return ER_NOT_IMPLEMENTED;
    }

    /**
     * Get the additional information on the claim capabilities.
     * @param[out] additionalInfo The additional info
     *
     * @return
     *  - #ER_OK if successful
     *  - an error status indicating failure
     */
    virtual QStatus GetClaimCapabilityAdditionalInfo(PermissionConfigurator::ClaimCapabilityAdditionalInfo& additionalInfo)
    {
        QCC_UNUSED(additionalInfo);
        return ER_NOT_IMPLEMENTED;
    }
};

}
#endif
