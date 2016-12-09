/**
 * @file
 * Helper functions for the Security 2.0 Test Cases
 */

/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
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

#ifndef _ALLJOYN_SECURITY_TEST_HELPER_H
#define _ALLJOYN_SECURITY_TEST_HELPER_H

#include <string>
#include <vector>

#include <qcc/CertificateECC.h>
#include <qcc/CryptoECC.h>
#include <qcc/GUID.h>
#include <qcc/platform.h>
#include <qcc/String.h>

#include <alljoyn/AuthListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/PermissionPolicy.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/Session.h>

class SecurityTestHelper {
  public:
    static QStatus GetGUID(ajn::BusAttachment& bus, qcc::GUID128& guid);
    static QStatus GetPeerGUID(ajn::BusAttachment& bus, qcc::String& peerName, qcc::GUID128& peerGuid);

    static QStatus GetAppPublicKey(ajn::BusAttachment& bus, qcc::ECCPublicKey& publicKey);
    static QStatus RetrievePublicKeyFromMsgArg(const ajn::MsgArg& arg, qcc::ECCPublicKey* pubKey);
    static QStatus RetrieveDSAPublicKeyFromKeyStore(ajn::BusAttachment& bus, qcc::ECCPublicKey* publicKey);

    static void CreatePermissivePolicyAll(ajn::PermissionPolicy& policy, uint32_t version);
    static void CreatePermissivePolicyAnyTrusted(ajn::PermissionPolicy& policy, uint32_t version);

    static void UpdatePolicyWithValuesFromDefaultPolicy(const ajn::PermissionPolicy& defaultPolicy,
                                                        ajn::PermissionPolicy& policy,
                                                        bool keepCAentry = true,
                                                        bool keepAdminGroupEntry = false,
                                                        bool keepInstallMembershipEntry = false);

    static QStatus CreateAllInclusiveManifest(ajn::Manifest& manifest);
    static QStatus SignManifest(ajn::BusAttachment& issuerBus,
                                const std::vector<uint8_t>& subjectThumbprint,
                                ajn::Manifest& manifest);
    static QStatus SignManifest(ajn::BusAttachment& issuerBus,
                                const qcc::CertificateX509& subjectCertificate,
                                ajn::Manifest& manifest);
    static QStatus SignManifest(ajn::BusAttachment& issuerBus,
                                const qcc::CertificateX509& subjectCertificate,
                                AJ_PCSTR unsignedManifestXml,
                                std::string& signedManifestXml);
    static QStatus SignManifests(ajn::BusAttachment& issuerBus,
                                 const qcc::CertificateX509& subjectCertificate,
                                 std::vector<ajn::Manifest>& manifests);

    static QStatus CreateIdentityCert(ajn::BusAttachment& issuerBus,
                                      const qcc::String& serial,
                                      const qcc::String& subject,
                                      const qcc::ECCPublicKey* subjectPubKey,
                                      const qcc::String& alias,
                                      qcc::IdentityCertificate& cert,
                                      uint32_t expiredInSecs = 3600,
                                      bool setEmptyAKI = false);
    static QStatus CreateIdentityCert(ajn::BusAttachment& issuerBus,
                                      const qcc::String& serial,
                                      const qcc::String& subject,
                                      const qcc::ECCPublicKey* subjectPubKey,
                                      const qcc::String& alias,
                                      qcc::String& der,
                                      uint32_t expiredInSecs = 3600);
    static QStatus CreateIdentityCertChain(ajn::BusAttachment& caBus,
                                           ajn::BusAttachment& issuerBus,
                                           const qcc::String& serial,
                                           const qcc::String& subject,
                                           const qcc::ECCPublicKey* subjectPubKey,
                                           const qcc::String& alias,
                                           qcc::IdentityCertificate* certChain,
                                           size_t chainCount,
                                           uint32_t expiredInSecs = 3600);

    static QStatus CreateMembershipCert(const qcc::String& serial,
                                        ajn::BusAttachment& signingBus,
                                        const qcc::String& subject,
                                        const qcc::ECCPublicKey* subjectPubKey,
                                        const qcc::GUID128& guild,
                                        qcc::MembershipCertificate& cert,
                                        bool delegate = false,
                                        uint32_t expiredInSecs = 3600,
                                        bool setEmptyAKI = false);
    static QStatus CreateMembershipCert(const qcc::String& serial,
                                        ajn::BusAttachment& signingBus,
                                        const qcc::String& subject,
                                        const qcc::ECCPublicKey* subjectPubKey,
                                        const qcc::GUID128& guild,
                                        qcc::String& der,
                                        bool delegate = false,
                                        uint32_t expiredInSecs = 3600);

    static QStatus InstallMembership(const qcc::String& serial, ajn::BusAttachment& bus, const qcc::String& remoteObjName, ajn::BusAttachment& signingBus, const qcc::String& subject, const qcc::ECCPublicKey* subjectPubKey, const qcc::GUID128& guild);
    static QStatus InstallMembershipChain(ajn::BusAttachment& topBus, ajn::BusAttachment& secondBus, const qcc::String& serial0, const qcc::String& serial1, const qcc::String& remoteObjName, const qcc::String& secondSubject, const qcc::ECCPublicKey* secondPubKey, const qcc::String& targetSubject, const qcc::ECCPublicKey* targetPubKey, const qcc::GUID128& guild, bool setEmptyAKI = false);
    static QStatus InstallMembershipChain(ajn::BusAttachment& caBus, ajn::BusAttachment& intermediateBus, ajn::BusAttachment& targetBus, qcc::String& leafSerial, const qcc::GUID128& sgID);

    static QStatus SetCAFlagOnCert(ajn::BusAttachment& issuerBus, qcc::CertificateX509& certificate);
    static QStatus LoadCertificateBytes(ajn::Message& msg, qcc::CertificateX509& cert);

    static bool IsPermissionDeniedError(QStatus status, ajn::Message& msg);
    static QStatus ReadClaimResponse(ajn::Message& msg, qcc::ECCPublicKey* pubKey);

    static QStatus JoinPeerSession(ajn::BusAttachment& initiator, ajn::BusAttachment& responder, ajn::SessionId& sessionId);
    static void CallDeprecatedSetPSK(ajn::DefaultECDHEAuthListener* authListener, const uint8_t* pskBytes, size_t pskLength);

    static void UnwrapStrings(const std::vector<std::string>& strings, std::vector<AJ_PCSTR>& unwrapped);

  private:
    static void BuildValidity(qcc::CertificateX509::ValidPeriod& validity, uint32_t expiredInSecs);
};

#endif