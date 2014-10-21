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
#include <qcc/CryptoECC.h>
#include <alljoyn/PermissionPolicy.h>
#include "CredentialAccessor.h"

namespace ajn {

class PermissionMgmtObj : public BusObject {

    friend class PermissionManager;

  public:

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
    QStatus RetrieveLocalPolicyMsg(Message& msg);
    QStatus RetrievePolicy(PermissionPolicy& policy);

    void GetPolicy(const InterfaceDescription::Member* member, Message& msg);
    /**
     * Bind to an exclusive port for PermissionMgmt object.
     */
    QStatus BindPort();

    QStatus NotifyConfig();

    BusAttachment& bus;
    CredentialAccessor* ca;
    const InterfaceDescription::Member* notifySignalName;
    ClaimableState claimableState;
    uint32_t serialNum;
    PortListener* portListener;
};

}
#endif
