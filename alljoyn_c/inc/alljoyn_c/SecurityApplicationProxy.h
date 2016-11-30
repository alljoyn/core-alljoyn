/**
 * @file
 * This file defines the alljoyn_securityapplicationproxy and related functions.
 */
/******************************************************************************
 * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#ifndef _ALLJOYN_C_SECURITY_APPLICATION_PROXY_H
#define _ALLJOYN_C_SECURITY_APPLICATION_PROXY_H

#include <alljoyn_c/AjAPI.h>
#include <alljoyn_c/PermissionConfigurator.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/Session.h>
#include <qcc/KeyInfoECC.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * alljoyn_securityapplicationproxy is a handle which exposes Security 2.0
 * proxy object used to managed the remote application's security settings.
 */
typedef struct _alljoyn_securityapplicationproxy_handle* alljoyn_securityapplicationproxy;

/**
 * Get the session port reserved for SecurityApplicationProxy connections.
 *
 * @return alljoyn_sessionport with the value of the reserved port.
 */
AJ_API alljoyn_sessionport AJ_CALL alljoyn_securityapplicationproxy_getpermissionmanagementsessionport();

/**
 * Creates a Security 2.0 proxy object used to manage the remote application's security settings.
 *
 * @param[in]    bus        The bus attachment from which we're creating the proxy.
 * @param[in]    appBusName The unique bus name of the managed application.
 * @param[in]    sessionId  The session id to be used for communicating with the managed application.
 *                          It needs to be separately obtained using alljoyn_busattachment_joinsession.
 *
 * @return   The alljoyn_securityapplicationproxy. Has to be later destroyed using alljoyn_securityapplicationproxy_destroy.
 */
AJ_API alljoyn_securityapplicationproxy AJ_CALL alljoyn_securityapplicationproxy_create(alljoyn_busattachment bus,
                                                                                        AJ_PCSTR appBusName,
                                                                                        alljoyn_sessionid sessionId);

/**
 * Destroys the alljoyn_securityapplicationproxy created in alljoyn_securityapplicationproxy_create.
 *
 * @param[in]    proxy   The alljoyn_securityapplicationproxy being destroyed.
 */
AJ_API void AJ_CALL alljoyn_securityapplicationproxy_destroy(alljoyn_securityapplicationproxy proxy);

/**
 * Claims the application. The application is provided an identity certificate chain with
 * its own cert as the leaf. The application automatically installs the policy to
 * allow all communication from the provided admin group.
 * The application's manifests are installed as well.
 *
 * Note: After this call the remote application should wait for the
 * "alljoyn_securityapplicationproxy_endmanagement" call before it can begin regular operation.
 * Since "alljoyn_securityapplicationproxy_startmanagement" calls are not possible before the
 * application is claimed, that call is made internally on the application's side before
 * the claiming procedure begins.
 *
 * @param[in]    proxy                      The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[in]    caKey                      Null-terminated C string representing the CA's PEM-encoded public key.
 * @param[in]    identityCertificateChain   Null-terminated C string representing PEM-encoded identity certificates
 *                                          with the app's certificate being the leaf. The leaf is listed first
 *                                          followed by each intermediate Certificate Authority's certificate, ending
 *                                          in the trusted root's certificate.
 * @param[in]    groupId                    Byte array representing the admin group GUID.
 * @param[in]    groupSize                  Size of "groupId" array. It must be equal to 16.
 * @param[in]    groupAuthority             Null-terminated C string containing the PEM-encoded public key of the
 *                                          security group authority.
 * @param[in]    manifestsXmls              An array of null-terminated strings containing the application's
 *                                          signed manifests in XML format. The signed manifest's XSD is located under
 *                                          alljoyn_core/docs/manifest.xsd.
 * @param[in]    manifestsCount             Count of "manifestsXmls" array elements.
 *
 * @return
 *          - #ER_OK If successful.
 *          - Other error status codes indicating a failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_claim(alljoyn_securityapplicationproxy proxy,
                                                              AJ_PCSTR caKey,
                                                              AJ_PCSTR identityCertificateChain,
                                                              const uint8_t* groupId,
                                                              size_t groupSize,
                                                              AJ_PCSTR groupAuthority,
                                                              AJ_PCSTR* manifestsXmls, size_t manifestsCount);

/**
 * This method allows the admin to retrieve the claimable application version from an application.
 *
 * @param[in]    proxy                      The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   version                    The claimable application version.
 *
 * @return
 *          - #ER_OK                If successful.
 *          - An error status indicating failure.
 */
AJ_API QStatus alljoyn_securityapplicationproxy_getclaimableapplicationversion(alljoyn_securityapplicationproxy proxy, uint16_t* version);

/**
 * This method allows the admin to retrieve the managed application version from an application.
 *
 * @param[in]    proxy                      The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   version                    The managed application version
 *
 * @return
 *          - #ER_OK                If successful.
 *          - An error status indicating failure.
 */
AJ_API QStatus alljoyn_securityapplicationproxy_getmanagedapplicationversion(alljoyn_securityapplicationproxy proxy, uint16_t* version);

/**
 * Retrieves the manifest in XML form from the application.
 *
 * @param[in]    proxy                  The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   manifestArray          The struct object contains the count and a null-terminated array of manifest strings
 *                                      The object is managed by the caller and must be later destroyed by calling
 *                                      alljoyn_securityapplicationproxy_manifestarray_cleanup API.
 *
 * @return
 *          - #ER_OK If successful.
 *          - Other error status codes indicating a failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_getmanifests(alljoyn_securityapplicationproxy proxy, alljoyn_manifestarray* manifestArray);

/**
 * This method deallocates an array of strings containing manifests returned by alljoyn_securityapplicationproxy_getmanifests.
 *
 * @param[in]    manifestArray              Pointer to alljoyn_manifestarray populated by a call to
 *                                          alljoyn_securityapplicationproxy_getmanifests.
 */
AJ_API void AJ_CALL alljoyn_securityapplicationproxy_manifestarray_cleanup(alljoyn_manifestarray* manifestArray);

/**
 * Retrieves the manifest template in XML form from the application.
 *
 * @param[in]    proxy                  The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   manifestTemplateXml    A pointer to null-terminated manifest template XML string.
 *                                      The string is managed by the caller and must be later destroyed by calling
 *                                      alljoyn_securityapplicationproxy_manifesttemplate_destroy API.
 *
 * @return
 *          - #ER_OK If successful.
 *          - Other error status codes indicating a failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_getmanifesttemplate(alljoyn_securityapplicationproxy proxy, AJ_PSTR* manifestTemplateXml);

/**
 * Retrieves the manifest template digest from the application.
 *
 * @param[in]    proxy                  The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   digest                 A byte array containing the digest, this array is allocated by the caller, and will be filled by this
 *                                      function with the digest. This array is managed by the caller and must be later destroyed using
 *                                      alljoyn_securityapplicationproxy_digest_destroy.
 *
 * @param[in]    expectedSize           Expected size of the digest array.
 *
 * @return
 *          - #ER_OK If successful.
 *          - Other error status codes indicating a failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_getmanifesttemplatedigest(alljoyn_securityapplicationproxy proxy, uint8_t* digest, size_t expectedSize);

/**
 * Frees the memory allocated for the manifest template inside alljoyn_securityapplicationproxy_get_manifest_template.
 *
 * @param[in]   manifestTemplateXml Manifest string created using alljoyn_securityapplicationproxy_getmanifesttemplate.
 */
AJ_API void AJ_CALL alljoyn_securityapplicationproxy_manifesttemplate_destroy(AJ_PSTR manifestTemplateXml);

/**
 * Frees the memory allocated for alljoyn_securityapplicationproxy_signmanifest.
 *
 * @param[in]   manifestXml Manifest string created using get or sign manifest functions.
 */
AJ_API void AJ_CALL alljoyn_securityapplicationproxy_manifest_destroy(AJ_PSTR manifestXml);

/**
 * This method allows the admin to retrieve the active security application version from an application.
 *
 * @param[in]    proxy              The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   version            The active security application version
 *
 * @return
 *          - #ER_OK                If successful.
 *          - An error status indicating failure.
 */
AJ_API QStatus alljoyn_securityapplicationproxy_getsecurityapplicationversion(alljoyn_securityapplicationproxy proxy, uint16_t* version);

/**
 * Representation of the current state of the application.
 * @see alljoyn_applicationstate in PermissionConfigurator.h for available values.
 *
 * @param[in]    proxy              The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   applicationState   Current state of the application.
 *
 * @return
 *          - #ER_OK If successful.
 *          - Other error status codes indicating a failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_getapplicationstate(alljoyn_securityapplicationproxy proxy, alljoyn_applicationstate* applicationState);

/**
 * The authentication mechanisms the managed application supports for the claim process.
 * It is a bit mask.
 * @see alljoyn_claimcapability in PermissionConfigurator.h for available values.
 *
 * @param[in]    proxy           The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   capabilities    Application's claim capabilities masks.
 *
 * @return
 *          - #ER_OK If successful.
 *          - Other error status codes indicating a failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_getclaimcapabilities(alljoyn_securityapplicationproxy proxy, alljoyn_claimcapabilities* capabilities);

/**
 * The additional information on the claim capabilities supported by the managed application.
 * It is a bit mask.
 * @see alljoyn_claimcapabilityadditionalinfo in PermissionConfigurator.h for available values.
 *
 * @param[in]    proxy           The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   additionalInfo  Application's claim capabilities additional info.
 *
 * @return
 *          - #ER_OK If successful.
 *          - Other error status codes indicating a failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_getclaimcapabilitiesadditionalinfo(alljoyn_securityapplicationproxy proxy, alljoyn_claimcapabilitiesadditionalinfo* additionalInfo);

/**
 * This method allows the admin to retrieve the active policy version from an application.
 *
 * @param[in]    proxy              The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   version            The active policy version which is the policy's serial number
 *
 * @return
 *          - #ER_OK                If successful.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_getpolicyversion(alljoyn_securityapplicationproxy proxy, uint32_t* version);

/**
 * This method allows the admin to retrieve the active policy from an application.
 *
 * @param[in]    proxy              The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   policyXml          The active policy in XML format. This string must be freed using
 *                                  alljoyn_securityapplicationproxy_policy_destroy.
 * @return
 *          - #ER_OK                If successful.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_getpolicy(alljoyn_securityapplicationproxy proxy,
                                                                  AJ_PSTR* policyXml);

/**
 * This method allows the admin to retrieve the default policy from an application.
 *
 * @param[in]    proxy                      The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   policyXml                  The default policy in XML format. This string must be freed using
 *                                          alljoyn_securityapplicationproxy_policy_destroy.
 * @return
 *          - #ER_OK                If successful.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_getdefaultpolicy(alljoyn_securityapplicationproxy proxy,
                                                                         AJ_PSTR* policyXml);

/**
 * This method deallocates strings returned by a call to alljoyn_securityapplicationproxy_getpolicy or
 * alljoyn_securityapplicationproxy_getdefaultpolicy.
 *
 * @param[in]    policyXml                  A string containing a policy in XML format returned by a call to either
 *                                          alljoyn_securityapplicationproxy_getpolicy or
 *                                          alljoyn_securityapplicationproxy_getdefaultpolicy.
 */
AJ_API void AJ_CALL alljoyn_securityapplicationproxy_policy_destroy(AJ_PSTR policyXml);

/**
 * This method allows an admin to install the permission policy to the
 * application. Any existing policy will be replaced if the new policy version
 * number is greater than the existing policy's version number.
 *
 * Until ASACORE-2755 is fixed the caller must include all default policies
 * (containing information about the trust anchors) with each call, so they
 * would not be removed.
 *
 * After having a new policy installed, the target bus clears out all of
 * its peer's secret and session keys, so so any existing secure session will
 * need to be re-established: alljoyn_busattachment_secureconnectionasync(true)
 * should be called to force the peers to create a new set of secret and session keys.
 *
 * @param[in]    proxy      The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[in]    policyXml  The new policy in XML format. For the policy XSD refer to
 *                          alljoyn_core/docs/policy.xsd.
 *
 * @return
 *          - #ER_OK                If successful.
 *          - #ER_PERMISSION_DENIED Error raised when the caller does not have permission.
 *          - #ER_POLICY_NOT_NEWER  Error raised when the new policy does not have a
 *                                  greater version number than the existing policy.
 *          - #ER_XML_MALFORMED     Error raised when the provided XML is not in supported policy format.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_updatepolicy(alljoyn_securityapplicationproxy proxy, AJ_PCSTR policyXml);

/**
 * This method allows an admin to update the application's identity certificate and manifests.
 * All old manifests will be overwritten by the new set.
 *
 * After having a new identity certificate installed, the target bus clears
 * out all of its peer's secret and session keys, so the next call will get
 * security violation. After calling alljoyn_securityapplicationproxy_updateidentity,
 * alljoyn_busattachment_secureconnectionasync(true) should be called to force
 * the peers to create a new set of secret and session keys.
 *
 * @param[in]    proxy                      The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[in]    identityCertificateChain   Null-terminated C string representing PEM-encoded identity certificate chain
 *                                          with the application's certificate being the leaf. The leaf is listed first
 *                                          followed by each intermediate Certificate Authority's certificate, ending
 *                                          in the trusted root's certificate.
 * @param[in]    manifestsXmls              An array of null-terminated strings containing the application's
 *                                          signed manifests in XML format.
 * @param[in]    manifestsCount             Count of "manifestsXmls" array elements.
 *
 * @return
 *          - #ER_OK                        If successful.
 *          - #ER_PERMISSION_DENIED         Error raised when the caller does not have permission.
 *          - #ER_INVALID_CERTIFICATE       Error raised when the identity certificate chain is not valid.
 *          - #ER_INVALID_CERTIFICATE_USAGE Error raised when the Extended Key Usage is not AllJoyn specific.
 *          - #ER_DIGEST_MISMATCH           Error raised when none of the provided signed manifests are
 *                                          valid for the given identity certificate.
 *          - #ER_XML_MALFORMED             Error raised the manifest is not compliant with the required format.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_updateidentity(alljoyn_securityapplicationproxy proxy,
                                                                       AJ_PCSTR identityCertificateChain,
                                                                       AJ_PCSTR* manifestsXmls, size_t manifestsCount);

/**
 * This method allows the admin to install a membership certificate chain to the
 * application.
 *
 * @param[in]    proxy                      The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[in]    membershipCertificateChain Null-terminated C string representing PEM encoded membership certificates
 *                                          with the app's certificate being listed first followed by each intermediate
 *                                          Certificate Authority's certificate, ending in the trusted root's certificate.
 *
 * @return
 *          - #ER_OK                    If successful.
 *          - #ER_PERMISSION_DENIED     Error raised when the caller does not have permission.
 *          - #ER_DUPLICATE_CERTIFICATE Error raised when the membership certificate
 *                                      is already installed.
 *          - #ER_INVALID_CERTIFICATE   Error raised when the membership certificate is not valid.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_installmembership(alljoyn_securityapplicationproxy proxy, AJ_PCSTR membershipCertificateChain);

/**
 * This method allows the admin to remove a membership certificate chain from the
 * application.
 *
 * @param[in]   proxy           The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[in]   serial          Certificate's serial number.
 * @param[in]   serialLen       Certificate's serial number length.
 * @param[in]   pubKey          Null-terminated C String representing the public key of the certificate to be removed in PEM format
 * @param[in]   issuerAki       Certificate issuer's AKI.
 * @param[in]   issuerAkiLen    Certificate issuer's AKI length.
 *
 * @return
 *          - #ER_OK if successful
 *          - #ER_PERMISSION_DENIED Error raised when the caller does not have permission
 *          - #ER_CERTIFICATE_NOT_FOUND Error raised when the certificate is not found
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_removemembership(alljoyn_securityapplicationproxy proxy,
                                                                         const uint8_t* serial,
                                                                         size_t serialLen,
                                                                         AJ_PCSTR pubKey,
                                                                         const uint8_t* issuerAki,
                                                                         size_t issuerAkiLen);

/**
 * Get the summaries for installed memberships
 *
 * @param[in]   proxy           The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]  certificateIds  The object containing the size and an array of alljoyn_certificateid. This array is allocated by the caller and must be later
 *                              destroyed by calling alljoyn_securityapplicationproxy_certificateidarray_cleanup API.
 *
 * @return
 *  - #ER_OK if successful
 *  - An error status indicating failure
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_getmembershipsummaries(alljoyn_securityapplicationproxy proxy, alljoyn_certificateidarray* certificateIds);

/**
 * This method deallocates the object filled by alljoyn_securityapplicationproxy_getmembershipsummaries
 *
 * @param[in]    certificateIds  Pointer to alljoyn_certificateidarray populated by a call by alljoyn_securityapplicationproxy_getmembershipsummaries.
 */
AJ_API void AJ_CALL alljoyn_securityapplicationproxy_certificateidarray_cleanup(alljoyn_certificateidarray* certificateIds);

/**
 * This method allows an admin to reset the application to its original state
 * prior to claim. The application's security 2.0 related configuration is
 * discarded. The application is no longer claimed, but this is not a complete factory reset.
 * The managed application keeps its private key.
 *
 * Note: After this call the remote application will automatically call
 * "alljoyn_securityapplicationproxy_endmanagement" on itself.
 *
 * @param[in]    proxy  The alljoyn_securityapplicationproxy connected to the managed application.
 *
 * @return
 *          - #ER_OK                If successful.
 *          - #ER_PERMISSION_DENIED Error raised when the caller does not have permission.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_reset(alljoyn_securityapplicationproxy proxy);

/**
 * This method allows an admin to remove the currently installed policy. The
 * application reverts back to the default policy generated during the claiming
 * process.
 *
 * @param[in]    proxy  The alljoyn_securityapplicationproxy connected to the managed application.
 *
 * @return
 *          - #ER_OK                If successful.
 *          - #ER_PERMISSION_DENIED Error raised when the caller does not have permission.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_resetpolicy(alljoyn_securityapplicationproxy proxy);

/**
 * Informs the remote appllication that the Security Manager will start changing its
 * Security 2.0 configuration, so it has an opportunity to gracefully terminate all
 * open sessions.
 * After the setup is finished the remote application has to be informed about this fact
 * using alljoyn_securityapplicationproxy_endmanagement.
 *
 * @param[in]    proxy  The alljoyn_securityapplicationproxy connected to the managed application.
 *
 * @return
 *          - #ER_OK If successful.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_startmanagement(alljoyn_securityapplicationproxy proxy);

/**
 * Informs the remote application that all of the Security 2.0 configuration started with
 * alljoyn_securityapplicationproxy_endmanagement has finished.
 *
 * @param[in]    proxy  The alljoyn_securityapplicationproxy connected to the managed application.
 *
 * @return
 *          - #ER_OK If successful.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_endmanagement(alljoyn_securityapplicationproxy proxy);

/**
 * Retrieves manufacturer certificate chain.
 *
 * @param[in]    proxy                          The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   manufacturerCertificateChain   Null-terminated C string representing PEM-encoded manufacturer certificate chain.
 *                                              The string is managed by the caller and must be later destroyed using
 *                                              alljoyn_securityapplicationproxy_manifest_destroy.
 *
 * @return
 *          - #ER_OK                        If successful.
 *          - #ER_CERTIFICATE_NOT_FOUND     Error raised when no identity certificate chain is installed.
 *          - An error status indicating failure.
 */
QStatus AJ_CALL alljoyn_securityapplicationproxy_getmanufacturercerticate(alljoyn_securityapplicationproxy proxy, AJ_PSTR* manufacturerCertificateChain);

/**
 * This method deallocates a string of PEM-encoded certificates returned by alljoyn_permissionconfigurator_getidentity.
 *
 * @param[in]   certificateChain            String returned by a call to alljoyn_permissionconfigurator_getidentity.
 */
AJ_API void AJ_CALL alljoyn_securityapplicationproxy_certificatechain_destroy(AJ_PSTR certificateChain);

/**
 * Retrieves (in PEM format) the public ECC key used by the managed application.
 *
 * @param[in]    proxy          The alljoyn_securityapplicationproxy connected to the managed application.
 * @param[out]   eccPublicKey   Application's ECC public key in PEM format. It should be later destroyed
 *                              using the alljoyn_securityapplicationproxy_eccpublickey_destroy call.
 *
 * @return
 *          - #ER_OK If successful.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_geteccpublickey(alljoyn_securityapplicationproxy proxy, AJ_PSTR* eccPublicKey);

/**
 * Frees the memory allocated while calling alljoyn_securityapplicationproxy_geteccpublickey.
 *
 * @param[in]   eccPublicKey   The public ECC key returned by alljoyn_securityapplicationproxy_geteccpublickey.
 */
AJ_API void AJ_CALL alljoyn_securityapplicationproxy_eccpublickey_destroy(AJ_PSTR eccPublicKey);

/**
 * Adds an identity certificate thumbprint to and signs the manifest XML.
 *
 * @param[in]    unsignedManifestXml    The unsigned manifest in XML format. The XML schema can be found
 *                                      under alljoyn_core/docs/manifest_template.xsd.
 * @param[in]    identityCertificatePem The identity certificate of the remote application that will
 *                                      use the signed manifest.
 * @param[in]    signingPrivateKeyPem   The PEM-encoded private ECC key used to sign the manifests. This must
 *                                      be the same private key used to sign the identity certificate.
 * @param[out]   signedManifestXml      The signed manifest in XML format.
 *                                      The string is managed by the caller and must be later destroyed
 *                                      using alljoyn_securityapplicationproxy_manifest_destroy.
 *
 * @return
 *          - #ER_OK            If successful.
 *          - #ER_XML_MALFORMED If the unsigned manifest is not compliant with the required format.
 *          - Other error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_signmanifest(AJ_PCSTR unsignedManifestXml,
                                                                     AJ_PCSTR identityCertificatePem,
                                                                     AJ_PCSTR signingPrivateKeyPem,
                                                                     AJ_PSTR* signedManifestXml);

/**
 * Frees the memory allocated while calling alljoyn_securityapplicationproxy_signmanifest.
 *
 * @param[in]   signedManifestXml   The signed manifest returned by alljoyn_securityapplicationproxy_signmanifest.
 */
AJ_API void AJ_CALL alljoyn_securityapplicationproxy_manifest_destroy(AJ_PSTR signedManifestXml);

/**
 * Install signed manifests to the application to which the proxy is connected by adding the manifests to the already-installed manifests
 * present on the app. This method only verifies that manifests have a signature; it does not verify that the signature is valid.
 *
 * @param[in]    proxy                      The alljoyn_securityapplicationproxy for the application's bus attachment.
 * @param[in]    manifestsXmls              Array of null-terminated C strings, each of which is a signed manifest in XML format.
 * @param[in]    manifestsCount             The number of elements in the manifests array.
 *
 * @return
 *          - #ER_OK                        If successful.
 *          - #ER_DIGEST_MISMATCH           Error raised if no manifests can be installed because none are signed.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_installmanifests(alljoyn_securityapplicationproxy proxy,
                                                                         AJ_PCSTR* manifestsXmls,
                                                                         size_t manifestsCount);

/**
 * Adds an identity certificate thumbprint and retrieves the digest of the manifest XML for signing.
 *
 * @param[in]    unsignedManifestXml    The unsigned manifest in XML format. The XML schema can be found
 *                                      under alljoyn_core/docs/manifest_template.xsd.
 * @param[in]    identityCertificatePem The identity certificate of the remote application that will use
 *                                      the signed manifest.
 * @param[out]   digest                 Pointer to receive the byte array containing the digest to be
 *                                      signed with ECDSA-SHA256. This array is managed by the caller and must
 *                                      be later destroyed using alljoyn_securityapplicationproxy_digest_destroy.
 * @param[out]   digestSize             size_t to receive the size of the digest array.
 *
 * @return
 *          - #ER_OK            If successful.
 *          - #ER_XML_MALFORMED If the unsigned manifest is not compliant with the required format.
 *          - Other error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_computemanifestdigest(AJ_PCSTR unsignedManifestXml,
                                                                              AJ_PCSTR identityCertificatePem,
                                                                              uint8_t** digest,
                                                                              size_t* digestSize);

/**
 * Destroys a digest buffer returned by a call to alljoyn_securityapplicationproxy_computemanifestdigest.
 *
 * @param[in]   digest                   Digest array returned by alljoyn_securityapplicationproxy_computemanifestdigest.
 */
AJ_API void AJ_CALL alljoyn_securityapplicationproxy_digest_destroy(uint8_t* digest);

/**
 * Adds an identity certificate thumbprint and sets the signature to a provided signature. The signature should
 * have been generated by an earlier call to alljoyn_securityapplicationproxy_computemanifestdigest using
 * the same values for the unsignedManifestXml and identityCertificatePem parameters.
 *
 * @param[in]    unsignedManifestXml    The unsigned manifest in XML format. The XML schema can be found
 *                                      under alljoyn_core/docs/manifest_template.xsd.
 * @param[in]    identityCertificatePem The identity certificate of the remote application that will use
 *                                      the signed manifest.
 * @param[in]    signature              Caller-generated ECDSA-SHA256 signature of the manifest.
 * @param[in]    signatureSize          Size of the signature array.
 * @param[out]   signedManifestXml      The signed manifest in XML format.
 *                                      The string is managed by the caller and must be later destroyed
 *                                      using alljoyn_securityapplicationproxy_manifest_destroy.
 *
 * @return
 *          - #ER_OK            If successful.
 *          - #ER_XML_MALFORMED If the unsigned manifest is not compliant with the required format.
 *          - Other error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_setmanifestsignature(AJ_PCSTR unsignedManifestXml,
                                                                             AJ_PCSTR identityCertificatePem,
                                                                             const uint8_t* signature,
                                                                             size_t signatureSize,
                                                                             AJ_PSTR* signedManifestXml);

/**
 * Retrieves identity certificate chain.
 *
 * @param[in]    proxy                      The alljoyn_securityapplicationproxy for the application's bus attachment.
 * @param[out]   identityCertificateChain   Null-terminated C string representing PEM-encoded identity certificate chain
 *                                          with the application's certificate being the leaf. The leaf is listed first
 *                                          followed by each intermediate Certificate Authority's certificate, ending in
 *                                          the trusted root's certificate. This string must be freed with
 *                                          alljoyn_securityapplicationproxy_certificatechain_destroy.
 * @param[out] size                         a pointer to the size of the certificate Chain.
 *
 * @return
 *          - #ER_OK                        If successful.
 *          - #ER_CERTIFICATE_NOT_FOUND     Error raised when no identity certificate chain is installed.
 *          - An error status indicating failure.
 */
AJ_API QStatus AJ_CALL alljoyn_securityapplicationproxy_getidentity(alljoyn_securityapplicationproxy proxy, AJ_PSTR* identityCertificateChain, size_t* size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif