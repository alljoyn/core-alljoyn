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
#define AJNKEY_STORE "/.alljoyn_keystore/c_ecdhe.ks"

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

    QStatus Claim(const OnlineApplication& app,
                  KeyInfoNISTP256& certificateAuthority,
                  GroupInfo& adminGroup,
                  IdentityCertificateChain identityCertChain,
                  const Manifest& manifest,
                  const SessionType type,
                  AuthListener& listener);

    QStatus GetIdentity(const OnlineApplication& app,
                        IdentityCertificateChain& cert);

    QStatus UpdateIdentity(const OnlineApplication& app,
                           IdentityCertificateChain certChain,
                           const Manifest& mf);

    QStatus InstallMembership(const OnlineApplication& app,
                              const MembershipCertificateChain certificateChain);

    QStatus RemoveMembership(const OnlineApplication& app,
                             const string& serial,
                             const KeyInfoNISTP256& issuerKeyInfo);

    QStatus GetMembershipSummaries(const OnlineApplication& app,
                                   vector<MembershipSummary>& summaries);

    QStatus GetPolicy(const OnlineApplication& app,
                      PermissionPolicy& policy);

    QStatus GetPolicyVersion(const OnlineApplication& app,
                             uint32_t& policyVersion);

    QStatus GetDefaultPolicy(const OnlineApplication& app,
                             PermissionPolicy& policy);

    QStatus UpdatePolicy(const OnlineApplication& app,
                         const PermissionPolicy& policy);

    QStatus ResetPolicy(const OnlineApplication& app);

    QStatus GetManifestTemplate(const OnlineApplication& app,
                                Manifest& mf);

    QStatus GetManifest(const OnlineApplication& app,
                        Manifest& manifest);

    QStatus GetClaimCapabilities(const OnlineApplication& app,
                                 PermissionConfigurator::ClaimCapabilities& claimCapabilities,
                                 PermissionConfigurator::ClaimCapabilityAdditionalInfo& claimCapInfo);

    QStatus Reset(const OnlineApplication& app);

    static AuthListener* listener;

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
     * @brief Ask the ProxyObjectManager to provide a ProxyBusObject to given
     * application. For every GetProxyObject, a matching
     * ReleaseProxyObject must be done.
     * @param[in] app           The application to connect to.
     * @param[in] type          The type of session required.
     * @param[out] remoteObject The ProxyBusObject will be stored in this.
     * pointer on success.
     */
    QStatus GetProxyObject(const OnlineApplication app,
                           SessionType type,
                           SecurityApplicationProxy** remoteObject,
                           AuthListener* al = nullptr);

    /**
     * @brief Release the remoteObject.
     * @param[in] remoteObject The object to be released.
     */
    QStatus ReleaseProxyObject(SecurityApplicationProxy* remoteObject,
                               bool resetListener = false);
};
}
}

#endif /* ALLJOYN_SECMGR_PROXYOBJECTMANAGER_H_ */
