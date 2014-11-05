#ifndef _ALLJOYN_PERMISSION_MGMT_OBJ_H
#define _ALLJOYN_PERMISSION_MGMT_OBJ_H
/**
 * @file
 * This file defines the Permission DB classes that provide the interface to
 * parse the authorization data
 */

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

#ifndef __cplusplus
#error Only include PermissionMgmtObj.h in C++ code.
#endif

#include <alljoyn/BusObject.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <qcc/KeyBlob.h>
#include <qcc/CryptoECC.h>
#include <alljoyn/PermissionPolicy.h>
#include "CredentialAccessor.h"
#include "ProtectedAuthListener.h"

namespace ajn {

class PermissionMgmtObj : public BusObject {

    friend class PermissionManager;

  public:

    class KeyExchangeListener : public ProtectedAuthListener {
      public:
        KeyExchangeListener()
        {
        }
        void SetPermissionMgmtObj(PermissionMgmtObj* pmo)
        {
            this->pmo = pmo;
        }

        /**
         * Simply wraps the call of the same name to the inner AuthListener
         */
        bool RequestCredentials(const char* authMechanism, const char* peerName, uint16_t authCount, const char* userName, uint16_t credMask, Credentials& credentials);

        /**
         * Simply wraps the call of the same name to the inner ProtectedAuthListener
         */
        bool VerifyCredentials(const char* authMechanism, const char* peerName, const Credentials& credentials);

      private:
        PermissionMgmtObj* pmo;
    };

    typedef enum {
        STATE_UNCLAIMABLE = 0,   ///< not claimable
        STATE_CLAIMABLE = 1,         ///< claimable
        STATE_CLAIMED = 2 ///< the app/device is already claimed
    } ClaimableState;

    /**
     * Constructor
     *
     */
    PermissionMgmtObj(BusAttachment& bus);

    /**
     * virtual destructor
     */
    virtual ~PermissionMgmtObj();

    /**
     * Set the array of guilds.
     * @param guilds the array of guilds must be new'd by the caller.  It will be deleted by this object.
     */
    void SetGuilds(size_t count, qcc::GUID128* guilds)
    {
        delete [] this->guilds;
        guildsSize = count;
        this->guilds = guilds;
    }

    size_t GetGuildsSize() {
        return guildsSize;
    }

    const qcc::GUID128* GetGuilds()
    {
        return guilds;
    }

    /**
     * Called by the message bus when the object has been successfully registered. The object can
     * perform any initialization such as adding match rules at this time.
     */

    virtual void ObjectRegistered(void);

  private:

    typedef enum {
        ENTRY_TRUST_ANCHOR,   ///< Trust anchor.
        ENTRY_POLICY,         ///< Local policy data
        ENTRY_MEMBERSHIPS,  ///< the list of membership certificates and associated policies
        ENTRY_IDENTITY,      ///< the identity cert
        ENTRY_EQUIVALENCES  ///< The equivalence certs
    } ACLEntryType;

    struct TrustAnchor {
        uint8_t guid[qcc::GUID128::SIZE];
        qcc::ECCPublicKey publicKey;

        TrustAnchor(const uint8_t* pGuid, qcc::ECCPublicKey* pPublicKey)
        {
            memcpy(guid, pGuid, qcc::GUID128::SIZE);
            memcpy(&publicKey, pPublicKey, sizeof(qcc::ECCPublicKey));
        }
        TrustAnchor()
        {
        }
    };

    class PortListener : public SessionPortListener {

      public:

        PortListener() : SessionPortListener()
        {
        }

        bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
        {
            return (ALLJOYN_SESSIONPORT_PERMISSION_MGMT == sessionPort);
        }
    };

    void Claim(const InterfaceDescription::Member* member, Message& msg);
    void InstallPolicy(const InterfaceDescription::Member* member, Message& msg);
    QStatus GetACLGUID(ACLEntryType aclEntryType, qcc::GUID128& guid);

    QStatus InstallTrustAnchor(const qcc::GUID128& guid, const uint8_t* pubKey, size_t pubKeyLen);

    QStatus GetPeerGUID(Message& msg, qcc::GUID128& guid);

    QStatus GetTrustAnchor(TrustAnchor& trustAnchor);
    QStatus StoreDSAKeys(const qcc::ECCPrivateKey* privateKey, const qcc::ECCPublicKey* publicKey);
    QStatus RetrieveDSAPublicKey(qcc::ECCPublicKey* publicKey);
    QStatus RetrieveDSAPrivateKey(qcc::ECCPrivateKey* privateKey);

    QStatus StorePolicy(PermissionPolicy& policy);
    QStatus RetrievePolicy(PermissionPolicy& policy);
    void RemovePolicy(const InterfaceDescription::Member* member, Message& msg);
    void GetPolicy(const InterfaceDescription::Member* member, Message& msg);
    QStatus NotifyConfig();

    void InstallIdentity(const InterfaceDescription::Member* member, Message& msg);
    QStatus GetIdentityBlob(qcc::KeyBlob& kb);
    void GetIdentity(const InterfaceDescription::Member* member, Message& msg);
    void RemoveIdentity(const InterfaceDescription::Member* member, Message& msg);
    void InstallMembership(const InterfaceDescription::Member* member, Message& msg);
    void InstallMembershipAuthData(const InterfaceDescription::Member* member, Message& msg);
    void RemoveMembership(const InterfaceDescription::Member* member, Message& msg);
    bool ValidateCertChain(const qcc::String& certChainPEM, bool& authorized);
    void BuildListOfGuilds();
    QStatus LocateMembershipEntry(qcc::String& serialNum, qcc::String& issuer, qcc::GUID128& membershipGuid);

    /**
     * Bind to an exclusive port for PermissionMgmt object.
     */
    QStatus BindPort();

    BusAttachment& bus;
    CredentialAccessor* ca;
    const InterfaceDescription::Member* notifySignalName;
    ClaimableState claimableState;
    uint32_t serialNum;
    qcc::GUID128* guilds;
    size_t guildsSize;
    PortListener* portListener;
};

}
#endif
