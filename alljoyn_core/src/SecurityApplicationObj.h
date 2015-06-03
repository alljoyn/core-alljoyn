/**
 * @file
 * This contains the SecurityApplication class
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
#ifndef _ALLJOYN_SECURITYAPPLICATIONOBJ_H
#define _ALLJOYN_SECURITYAPPLICATIONOBJ_H

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <qcc/Debug.h>
#include "PermissionMgmtObj.h"

namespace ajn {
class SecurityApplicationObj : public PermissionMgmtObj {
  public:
    /**
     * version of org.alljoyn.Bus.Application interface
     */
    static const uint16_t APPLICATION_VERSION;
    /**
     * version of the org.alljoyn.Bus.Security.Application interface
     */
    static const uint16_t SECURITY_APPLICATION_VERSION;
    /**
     * version of the org.alljoyn.Bus.Security.ClaimableApplication interface
     */
    static const uint16_t SECURITY_CLAIMABLE_APPLICATION_VERSION;
    /**
     * version of the org.alljoyn.Bus.Security.ManagedApplicationObj interface
     */
    static const uint16_t SECURITY_MANAGED_APPLICATION_VERSION;

    /**
     * Construct a SecurityApplicationObj
     *
     * Must call SecurityApplicationObj.Init() before using this BusObject.
     */
    SecurityApplicationObj(BusAttachment& bus);

    /**
     * Initialize and Register this BusObject with to the BusAttachment.
     *
     * @return
     *  - #ER_OK on success
     *  - #ER_BUS_INTERFACE_MISSING if unable to find one of the interfaces implemented by this BusObject
     *  - An error status otherwise
     */
    virtual QStatus Init();

    /**
     * The State signal is used to advertise the state of an application.  It is
     * sessionless, because the signal is intended to discover applications.
     * Discovery is not done by using 'About'.  Applications must add extra code
     * to provide About.
     *
     * Not all applications will do this as pure consumer applications don't
     * need to be discovered by other applications.  Still they need to be
     * discovered by the framework to support certain some core framework
     * features. Furthermore we want to avoid interference between core
     * framework events and application events.
     *
     * The application state is an enumeration representing the current state of
     * the application.
     *
     * The list of valid values:
     * | Value | Description                                                       |
     * |-------|-------------------------------------------------------------------|
     * | 0     | NotClaimable.  The application is not claimed and not accepting   |
     * |       | claim requests.                                                   |
     * | 1     | Claimable.  The application is not claimed and is accepting claim |
     * |       | requests.                                                         |
     * | 2     | Claimed. The application is claimed and can be configured.        |
     * | 3     | NeedUpdate. The application is claimed, but requires a            |
     * |       | configuration update (after a software upgrade).                  |
     *
     * @param[in] publicKeyInfo the application public key
     * @param[in] state the application state
     *
     * @return
     *   - #ER_OK on success
     *   - An error status otherwise.
     */
    virtual QStatus State(const qcc::KeyInfoNISTP256& publicKeyInfo, PermissionConfigurator::ApplicationState state);

  private:
    /**
     * Claim the application, providing it an identity certificate along with a
     * set of permissions. A security group identity is passed. Members of this
     * group are allowed to manage the application.
     * @param[in] member interface member
     * @param[in] msg alljoyn message containing the information from the claiming
     *                application.
     */
    void Claim(const ajn::InterfaceDescription::Member* member, ajn::Message& msg);

    /**
     * Resets the managed application. All configuration is discarded. The
     * application is no longer claimed.
     *
     * @param[in] member interface member
     * @param[in] msg alljoyn message
     */
    void Reset(const ajn::InterfaceDescription::Member* member, ajn::Message& msg);

    /**
     * This method allows an admin to update the application's identity
     * certificate chain and its manifest.
     *
     * @param[in] member interface member
     * @param[in] msg alljoyn message
     */
    void UpdateIdentity(const ajn::InterfaceDescription::Member* member, ajn::Message& msg);

    /**
     * This method allows an admin to update the application's identity
     * certificate chain and its manifest.
     *
     * @param[in] member interface member
     * @param[in] msg alljoyn message
     */
    void UpdatePolicy(const ajn::InterfaceDescription::Member* member, ajn::Message& msg);

    /**
     * Reset the policy to the default policy generated during the claiming
     * process.
     *
     * @param[in] member interface member
     * @param[in] msg alljoyn message
     */
    void ResetPolicy(const ajn::InterfaceDescription::Member* member, ajn::Message& msg);

    /**
     * This method allows the amdin to install a membership cert chain to the
     * application.
     *
     * @param[in] member interface member
     * @param[in] msg alljoyn message
     */
    void InstallMembership(const ajn::InterfaceDescription::Member* member, ajn::Message& msg);

    /**
     * This method allows an admin to remove a membership certificate chain from
     * the application.
     *
     * @param[in] member interface member
     * @param[in] msg alljoyn message
     */
    void RemoveMembership(const ajn::InterfaceDescription::Member* member, ajn::Message& msg);

    /**
     * Handles the GetPropery request
     * @param[in]  ifcName  interface name
     * @param[in]  propName the name of the properly
     * @param[in]  val reference of MsgArg out parameter.
     * @return ER_OK if successful.
     */
    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);

    // org.alljoyn.Bus.Security.Application properties
    uint16_t applicationState;
    // TODO add in actual property values here
    //ManifestTemplateDigest manifestTemplateDigest;
    //EccPublicKey eccPublicKey;
    //ManufacturerCertificate* manufacturerCertificate;
    //size_t manufacturerCertificateSize;
    //ManifestTemplate* manifestTemplate;
    //size_t manifestTemplateSize;
    uint16_t claimCapabilities;
    uint16_t claimCapabilityAdditionalInfo;

    // org.alljoyn.Bus.Security.ManagedApplicationObj properties
    // TODO add in actual property values here
    //Certificate* identity;
    //size_t identitySize;
    //Rule* manifest;
    //size_t manifestSize;
    //IdentityCertificateId identityCertificateId;
    uint32_t policyVersion;
    //Policy policy;
    //Policy defaultPolicy;
    //CertificateId* membershipSummaries;
    //size_t membershipSummariesSize;
};
} //END namepace ajn

#endif //_ALLJOYN_SECURITYAPPLICATIONOBJ_H
