/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/BusObject.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/InterfaceDescription.h>
#include <cassert>
#include <qcc/CryptoECC.h>

#define ONLYCLAIMSTATE 0

using namespace ajn;

#define SECINTFNAME "org.alljoyn.Security.PermissionMgmt"
#define UNSECINTFNAME "org.alljoyn.Security.SecInfo"

/* Used to announce the application's claimable state */
enum ClaimableState { UNCLAIMED, CLAIMED, CLAIMABLE };

class ClaimListener {
  private:
    virtual bool OnClaimRequest(const qcc::ECCPublicKey* pubKeyRoT, void* ctx)
    {
        return true;
    }

    virtual void OnClaimed(void* ctx)
    {
    }

    virtual void OnIdentityInstalled(const qcc::String& pemIdentityCertificate)
    {
    }

    friend class PermissionMgmt;

  public:
    virtual ~ClaimListener() { }
};

class PermissionMgmt :
    public ajn::BusObject {
  private:
    qcc::Crypto_ECC* crypto;
    std::vector<qcc::ECCPublicKey*> pubKeyRoTs;
    qcc::String pemIdentityCertificate;
    ClaimListener* cl;
    ClaimableState claimableState;
    void* ctx;
    const InterfaceDescription::Member* unsecInfoSignalMember;

    static qcc::String PubKeyToString(const qcc::ECCPublicKey* pubKey);

    void Claim(const ajn::InterfaceDescription::Member* member,
               ajn::Message& msg);

    void InstallIdentity(const ajn::InterfaceDescription::Member* member,
                         ajn::Message& msg);

  public:
    PermissionMgmt(ajn::BusAttachment& ba,
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

    ClaimableState GetClaimableState() const;

    std::vector<qcc::ECCPublicKey*> GetRoTKeys() const;

    qcc::String GetInstalledIdentityCertificate() const;
};

#endif /* PERMISSIONMGMT_H_ */
