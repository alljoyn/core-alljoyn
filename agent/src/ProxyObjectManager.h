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
#ifndef ALLJOYN_SECMGR_PROXYOBJECTMANAGER_H_
#define ALLJOYN_SECMGR_PROXYOBJECTMANAGER_H_

#include <map>
#include <vector>
#include <string>

#include <qcc/Mutex.h>

#include <alljoyn/Status.h>
#include <alljoyn/Session.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/SecurityApplicationProxy.h>

#include <alljoyn/securitymgr/GroupInfo.h>
#include <alljoyn/securitymgr/Application.h>
#include <alljoyn/securitymgr/Manifest.h>
#include <alljoyn/securitymgr/AgentCAStorage.h>

#define KEYX_ECDHE_NULL "ALLJOYN_ECDHE_NULL"
#define KEYX_ECDHE_PSK "ALLJOYN_ECDHE_PSK"
#define ECDHE_KEYX "ALLJOYN_ECDHE_ECDSA"
#define AJNKEY_STORE "/.alljoyn_keystore/secmgr_ecdhe.ks"

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
struct MembershipSummary {
    KeyInfoNISTP256 issuer;
    string serial;
};

class ProxyObjectManager :
    public SessionListener {
  public:
    enum SessionType {
        ECDHE_NULL,
        ECDHE_DSA,
        ECDHE_PSK
    };

    ProxyObjectManager(BusAttachment* ba);

    ~ProxyObjectManager();

    class ManagedProxyObject {
      public:
        ManagedProxyObject(const OnlineApplication& app) : remoteApp(app), remoteObj(nullptr), resetAuthListener(false),
            needReAuth(false), proxyObjectManager(nullptr)
        {
        }

        ~ManagedProxyObject();

        QStatus Claim(KeyInfoNISTP256& certificateAuthority,
                      GroupInfo& adminGroup,
                      IdentityCertificateChain identityCertChain,
                      const Manifest& manifest);

        QStatus GetIdentity(IdentityCertificateChain& cert);

        QStatus UpdateIdentity(IdentityCertificateChain certChain,
                               const Manifest& mf);

        QStatus InstallMembership(const MembershipCertificateChain certificateChain);

        QStatus RemoveMembership(const string& serial,
                                 const KeyInfoNISTP256& issuerKeyInfo);

        QStatus GetMembershipSummaries(vector<MembershipSummary>& summaries);

        QStatus GetPolicy(PermissionPolicy& policy);

        QStatus GetPolicyVersion(uint32_t& policyVersion);

        QStatus GetDefaultPolicy(PermissionPolicy& policy);

        QStatus UpdatePolicy(const PermissionPolicy& policy);

        QStatus ResetPolicy();

        QStatus GetManifestTemplate(Manifest& mf);

        QStatus GetManifest(Manifest& manifest);

        QStatus GetClaimCapabilities(PermissionConfigurator::ClaimCapabilities& claimCapabilities,
                                     PermissionConfigurator::ClaimCapabilityAdditionalInfo& claimCapInfo);

        QStatus Reset();

        const OnlineApplication& GetApplication()
        {
            return remoteApp;
        }

      private:
        OnlineApplication remoteApp;
        SecurityApplicationProxy* remoteObj;
        bool resetAuthListener;
        bool needReAuth;
        ProxyObjectManager* proxyObjectManager;

        void CheckReAuthenticate();

        ManagedProxyObject(const ManagedProxyObject& other);
        ManagedProxyObject& operator=(const ManagedProxyObject& other);

        friend class ProxyObjectManager;
    };

    static AuthListener* listener;

    /**
     * @brief Ask the ProxyObjectManager to provide a ProxyBusObject to a given
     * application.
     *
     *  The ManagedProxyObject passed will be initialized by the ProxyObjectManager.
     *  It should only be used by the thread making the call. The life-cycle of the
     *  ManagedProxyObject should be short. So, ideally it is allocated on the stack
     *  in a limited scope so the destructor will be called soon after the last usage
     *  of the ManagedProxyObject. A single thread should only have one ManagedProxyObject
     *  at a time. A ManagedProxyObject should only be offered once to this function.
     *
     * @param[in]  managedProxy The application to initialize and to connect to.
     * @param[in]  type         The type of session required.
     * @param[out] al           The AuthListener to use for the setting up the session or nullptr to
     *                          use the default. No ownership is taken.
     *
     */
    QStatus GetProxyObject(ManagedProxyObject& managedProxy,
                           SessionType type = ECDHE_DSA,
                           AuthListener* al = nullptr);

  private:
    /*
     * Lock made static as different instances of ProxyObjectManager
     * might use the same busattachment.
     */
    static Mutex lock;
    BusAttachment* bus;

    /* SessionListener */
    virtual void SessionLost(SessionId sessionId,
                             SessionLostReason reason);

    /**
     * @brief Release the remoteObject.
     * @param[in] managedProxy The object to be released.
     */
    QStatus ReleaseProxyObject(SecurityApplicationProxy* managedProxy,
                               bool resetListener = false);
};
}
}

#endif /* ALLJOYN_SECMGR_PROXYOBJECTMANAGER_H_ */
