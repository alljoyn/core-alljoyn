/**
 * @file
 * Helper functions for the SecurityApplicationProxyTest
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

#ifndef _ALLJOYN_SECURITY_APPLICATION_PROXY_TEST_HELPER_H
#define _ALLJOYN_SECURITY_APPLICATION_PROXY_TEST_HELPER_H

#include <qcc/platform.h>
#include <gtest/gtest.h>
#include <qcc/GUID.h>
#include <qcc/StringUtil.h>

#include <alljoyn/Status.h>
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>

namespace ajn {

class SecurityApplicationProxyTestHelper {
  public:
    static void CreateIdentityCertChain(AJ_PCSTR issuerCert, AJ_PCSTR receiverCert, AJ_PSTR* certificateChainPem);
    static void RetrieveDSAPrivateKeyFromKeyStore(alljoyn_busattachment bus, AJ_PSTR* privateKey);
    static void RetrieveDSAPublicKeyFromKeyStore(alljoyn_busattachment bus, AJ_PSTR* publicKey);
    static void ReplaceString(std::string& original, const AJ_PCSTR from, AJ_PCSTR to);
    static void CreateIdentityCert(alljoyn_busattachment issuerBus,
                                   alljoyn_busattachment receiverBus,
                                   AJ_PSTR* certificatePem,
                                   bool delegate = true);
    static void CreateMembershipCert(alljoyn_busattachment signingBus,
                                     alljoyn_busattachment memberBus,
                                     const uint8_t* groupId,
                                     bool delegate,
                                     AJ_PSTR* membershipCertificatePem);
    static void DestroyCertificate(AJ_PSTR cert);
    static void DestroyKey(AJ_PSTR key);

  private:
    static const uint32_t oneHourInSeconds;

    static QStatus RetrieveDSAPublicKeyFromKeyStore(BusAttachment* bus, qcc::ECCPublicKey& publicKey);
    static QStatus GetGUID(BusAttachment& bus, qcc::GUID128& guid);
    static QStatus CreateIdentityCert(BusAttachment& issuerBus,
                                      const std::string& serial,
                                      const std::string& subject,
                                      const qcc::ECCPublicKey* subjectPubKey,
                                      const std::string& alias,
                                      qcc::IdentityCertificate& cert,
                                      bool delegate);
    static QStatus CreateIdentityCert(BusAttachment& issuerBus,
                                      const std::string& serial,
                                      const std::string& subject,
                                      const qcc::ECCPublicKey* subjectPubKey,
                                      const std::string& alias,
                                      std::string& der,
                                      bool delegate);
    static QStatus CreateMembershipCert(const std::string& serial,
                                        BusAttachment& signingBus,
                                        const std::string& subject,
                                        const qcc::ECCPublicKey* subjectPubKey,
                                        const qcc::GUID128& guild,
                                        bool delegate,
                                        qcc::MembershipCertificate& cert,
                                        bool setEmptyAKI = false);
    static QStatus CreateMembershipCert(const std::string& serial,
                                        BusAttachment& signingBus,
                                        const std::string& subject,
                                        const qcc::ECCPublicKey* subjectPubKey,
                                        const qcc::GUID128& guild,
                                        bool delegate,
                                        std::string& der);
};
}
#endif