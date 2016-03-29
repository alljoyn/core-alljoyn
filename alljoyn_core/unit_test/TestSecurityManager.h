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

#ifndef _ALLJOYN_TESTSECURITYMANAGER_H
#define _ALLJOYN_TESTSECURITYMANAGER_H

#include <string>

#include <alljoyn/BusAttachment.h>
#include "InMemoryKeyStore.h"

using namespace std;
using namespace ajn;
using namespace qcc;

class TestSecurityManager :
    public SessionListener {

  public:
    TestSecurityManager(string appName = "TestSecurityManager");

    ~TestSecurityManager();

    QStatus Init();

    QStatus Claim(BusAttachment& peerBus, const PermissionPolicy::Acl& manifest);

    QStatus UpdateIdentity(BusAttachment& peerBus, const PermissionPolicy::Acl& manifest);

    QStatus InstallMembership(BusAttachment& peerBus, const GUID128& group);

    QStatus UpdatePolicy(const BusAttachment& peerBus, const PermissionPolicy& policy);

    QStatus Reset(const BusAttachment& peerBus);

    const KeyInfoNISTP256& GetCaPublicKeyInfo()
    {
        return caPublicKeyInfo;
    }

    qcc::String GetUniqueName()
    {
        return bus.GetUniqueName();
    }

    /* Support for waiting to complete authentication */
    void DeleteAllAuthenticationEvents();
    void AddAuthenticationEvent(const qcc::String& peerName, Event* authEvent);
    QStatus WaitAllAuthenticationEvents(uint32_t timeout);
    void AuthCompleteCallback(qcc::String peerName);

  private:

    class MyAuthListener : public DefaultECDHEAuthListener {
      public:
        MyAuthListener(TestSecurityManager* sm) : m_sm(sm) { };

        virtual ~MyAuthListener() { };

        virtual void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success)
        {
            DefaultECDHEAuthListener::AuthenticationComplete(authMechanism, peerName, success);

            if (!success) {
                cerr << __FUNCTION__ << " auth failed" << endl;
            }

            if (m_sm != nullptr) {
                m_sm->AuthCompleteCallback(peerName);
            }
        }

      private:
        TestSecurityManager* m_sm;
    };

    BusAttachment bus;
    SessionOpts opts;
    MyAuthListener authListener;
    Crypto_ECC caKeyPair;
    KeyInfoNISTP256 caPublicKeyInfo;
    CertificateX509 caCertificate;
    GUID128 adminGroup;
    GUID128 identityGuid;
    string identityName;
    int certSerialNumber;
    int policyVersion;
    InMemoryKeyStoreListener keyStoreListener;
    std::map<qcc::String, qcc::Event*> authEvents;

    /* SessionListener */
    virtual void SessionLost(SessionId sessionId,
                             SessionLostReason reason);

    void IssueCertificate(const ECCPublicKey& appPubKey,
                          CertificateX509& cert,
                          bool isCA = false);

    void GenerateIdentityCertificate(const ECCPublicKey& appPubKey,
                                     const PermissionPolicy::Acl& manifest,
                                     IdentityCertificate& cert);

    void GenerateMembershipCertificate(const ECCPublicKey& appPubKey,
                                       const GUID128& group,
                                       MembershipCertificate& cert);

    QStatus ClaimSelf();

    void AddAdminAcl(const PermissionPolicy& in,
                     PermissionPolicy& out);
};

#endif /* _ALLJOYN_TESTSECURITYMANAGER_H */
