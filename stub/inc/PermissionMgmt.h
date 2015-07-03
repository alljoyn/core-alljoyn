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

#ifndef PERMISSIONMGMT_H_
#define PERMISSIONMGMT_H_

#include <cassert>
#include <map>
#include <vector>

#include <qcc/CryptoECC.h>
#include <qcc/GUID.h>

#include <alljoyn/BusObject.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/PermissionPolicy.h>

using namespace ajn;
using namespace qcc;
using namespace std;

const char SECINTFNAME[] = "org.allseen.Security.PermissionMgmt.Stub";
const char UNSECINTFNAME[] = "org.allseen.Security.PermissionMgmt.Stub.Notification";

class ClaimListener {
  private:
    virtual bool OnClaimRequest(const ECCPublicKey* pubKeyRoT, void* ctx)
    {
        QCC_UNUSED(pubKeyRoT);
        QCC_UNUSED(ctx);

        return true;
    }

    virtual void OnClaimed(void* ctx)
    {
        QCC_UNUSED(ctx);
    }

    virtual void OnIdentityInstalled(const string& pemIdentityCertificate)
    {
        QCC_UNUSED(pemIdentityCertificate);
    }

    virtual void OnMembershipInstalled(const string& pemMembershipCertificate)
    {
        QCC_UNUSED(pemMembershipCertificate);
    }

    virtual void OnAuthData(const PermissionPolicy* data)
    {
        QCC_UNUSED(data);
    }

    virtual void OnPolicyInstalled(PermissionPolicy& policy)
    {
        QCC_UNUSED(policy);
    }

    friend class PermissionMgmt;

  public:
    virtual ~ClaimListener() { }
};

class PermissionMgmt :
    public BusObject {
  private:
    Crypto_ECC* crypto;
    vector<ECCPublicKey*> pubKeyRoTs;
    map<GUID128, string> memberships; //guildId, certificate (in PEM)
    string pemIdentityCertificate;
    ClaimListener* cl;
    PermissionConfigurator::ApplicationState claimableState;
    void* ctx;
    const InterfaceDescription::Member* unsecInfoSignalMember;
    PermissionPolicy::Rule* manifestRules;
    size_t manifestRulesCount;
    PermissionPolicy policy;
    GUID128 peerID;

    static string PubKeyToString(const ECCPublicKey* pubKey);

    void Claim(const InterfaceDescription::Member* member,
               Message& msg);

    void InstallIdentity(const InterfaceDescription::Member* member,
                         Message& msg);

    void InstallMembership(const InterfaceDescription::Member* member,
                           Message& msg);

    void RemoveMembership(const InterfaceDescription::Member* member,
                          Message& msg);

    void InstallMembershipAuthData(const InterfaceDescription::Member* member,
                                   Message& msg);

    void GetManifest(const InterfaceDescription::Member* member,
                     Message& msg);

    void InstallPolicy(const InterfaceDescription::Member* member,
                       Message& msg);

    void GetPolicy(const InterfaceDescription::Member* member,
                   Message& msg);

    QStatus InstallIdentityCertificate(MsgArg& msgArg);

  public:
    PermissionMgmt(BusAttachment& ba,
                   ClaimListener* _cl,
                   void* ctx);

    ~PermissionMgmt();

    QStatus SendClaimDataSignal();

    /* This creates the interface for that specific BusAttachment */
    static QStatus CreateInterface(BusAttachment& ba);

    /* TODO: maybe add a timer that keeps the claimable state open for x time */
    void SetClaimableState(bool on);

    /* TODO: add a function that allows the application to reset the claimable state
     * and maybe even the public keys and certificates */

    PermissionConfigurator::ApplicationState GetClaimableState() const;

    vector<ECCPublicKey*> GetRoTKeys() const;

    string GetInstalledIdentityCertificate() const;

    map<GUID128, string> GetMembershipCertificates() const;

    void SetUsedManifest(PermissionPolicy::Rule* manifestRules,
                         size_t manifestRulesCount);

    void GetUsedManifest(PermissionPolicy::Rule* manifestRules,
                         size_t manifestRulesCount) const;
};

#endif /* PERMISSIONMGMT_H_ */
