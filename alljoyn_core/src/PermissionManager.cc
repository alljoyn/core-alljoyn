/**
 * @file
 * This file defines the Permission DB classes that provide the interface to
 * parse the authorization data
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

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Message.h>
#include <alljoyn/PermissionPolicy.h>
#include <qcc/Debug.h>
#include "PermissionManager.h"
#include "BusUtil.h"
#include "KeyExchanger.h"
#include "AuthMechSRP.h"
#include "AuthMechLogon.h"

#define QCC_MODULE "PERMISSION_MGMT"

using namespace std;
using namespace qcc;

namespace ajn {

struct Request {
    bool outgoing;
    bool propertyRequest;
    bool isSetProperty;
    const char* objPath;
    const char* iName;
    const char* mbrName;
    PermissionPolicy::Rule::Member::MemberType mbrType;


    Request(Message& msg, bool outgoing) : outgoing(outgoing), propertyRequest(false), isSetProperty(false), objPath(msg->GetObjectPath()), iName(NULL), mbrName(NULL)
    {
        if (msg->GetType() == MESSAGE_METHOD_CALL) {
            mbrType = PermissionPolicy::Rule::Member::METHOD_CALL;
        } else if (msg->GetType() == MESSAGE_SIGNAL) {
            mbrType = PermissionPolicy::Rule::Member::SIGNAL;
        } else {
            mbrType = PermissionPolicy::Rule::Member::NOT_SPECIFIED;
        }
    }

    Request(const char* objPath, const char* iName, const char* mbrName, const PermissionPolicy::Rule::Member::MemberType mbrType, bool outgoing, bool isProperty) : outgoing(outgoing), propertyRequest(isProperty), isSetProperty(false), objPath(objPath), iName(iName), mbrName(mbrName), mbrType(mbrType)
    {
    }

  private:
    Request& operator=(const Request& other);
};

struct Right {
    uint8_t authByPolicy;   /* remote peer is authorized by local policy */

    Right() : authByPolicy(0)
    {
    }
};

static bool MatchesPrefix(const char* str, String prefix)
{
    return !WildcardMatch(String(str), prefix);
}

/**
 * Validates whether the request action is explicited denied.
 * @param allowedActions the allowed actions
 * @return true is the requested action is denied; false, otherwise.
 */
static bool IsActionDenied(uint8_t allowedActions)
{
    return (allowedActions == 0);
}

/**
 * Validates whether the request action is allowed based on the allow action.
 * @param allowedActions the allowed actions
 * @param requestedAction the request action
 * @return true is the requested action is allowed; false, otherwise.
 */
static bool IsActionAllowed(uint8_t allowedActions, uint8_t requestedAction)
{
    return (allowedActions & requestedAction) == requestedAction;
}

/**
 * Verify whether the given rule is a match for the given message.
 * If the rule has object path, the message must prefix match the object path.
 * If the rule has interface name, the message must prefix match the interface name.
 * Find match in member name
 * Verify whether the requested right is allowed by the authorization at the
 * member.
 * The deny scan is only performed upon request.  In case of explicit deny, the
 * object path, interface name, and member name must equal *
 *
 */

static bool IsRuleMatched(const PermissionPolicy::Rule& rule, const Request& request, uint8_t requiredAuth, bool scanForDenied, bool& denied)
{
    if (rule.GetMembersSize() == 0) {
        return false;
    }
    if (!rule.GetObjPath().empty()) {
        /* rule has an object path */
        if (!((rule.GetObjPath() == request.objPath) || MatchesPrefix(request.objPath, rule.GetObjPath()))) {
            return false;  /* object path not matched */
        }
    }
    if (!rule.GetInterfaceName().empty()) {
        if (!((rule.GetInterfaceName() == request.iName) || MatchesPrefix(request.iName, rule.GetInterfaceName()))) {
            return false;  /* interface name not matched */
        }
    }

    /**
     * Explicit deny rule must have object path = *, interface name = * and member name = *
     */

    if (scanForDenied) {
        if (rule.GetObjPath() != "*") {
            scanForDenied = false; /* not qualified for denied */
        } else if (rule.GetInterfaceName() != "*") {
            scanForDenied = false; /* not qualified for denied */
        }
    }

    const PermissionPolicy::Rule::Member* members = rule.GetMembers();
    /* the member name is not specified when the caller wants to get all allowed properties */
    bool msgMbrNameEmpty = !request.mbrName || (strlen(request.mbrName) == 0);
    if (msgMbrNameEmpty) {
        return true;
    } else {
        /* typical message with a member name */
        /* scan all member entries to look for denied and one allowed */
        bool allowed = false;
        for (size_t cnt = 0; cnt < rule.GetMembersSize(); cnt++) {
            /* match member name */
            if (!members[cnt].GetMemberName().empty()) {
                if (!((members[cnt].GetMemberName() == request.mbrName) || MatchesPrefix(request.mbrName, members[cnt].GetMemberName()))) {
                    continue;  /* member name not matched */
                }
            }
            /* match member type */
            if (members[cnt].GetMemberType() != PermissionPolicy::Rule::Member::NOT_SPECIFIED) {
                if (request.mbrType != members[cnt].GetMemberType()) {
                    continue;  /* member type not matched */
                }
            }

            /* now check the action mask for explicit deny if requested and
             * when member name = *
             */
            if (scanForDenied && (members[cnt].GetMemberName() == "*") && IsActionDenied(members[cnt].GetActionMask())) {
                denied = true;
                return false;
            }
            /* now check the action mask for at least one allowed */
            if (!allowed) {
                allowed = IsActionAllowed(members[cnt].GetActionMask(), requiredAuth);
            }
        }
        return allowed;
    }
}

static bool IsPolicyAclMatched(const PermissionPolicy::Acl& acl, const Request& request, uint8_t requiredAuth, bool scanForDenied, bool& denied)
{
    const PermissionPolicy::Rule* rules = acl.GetRules();
    bool allowed = false;
    for (size_t cnt = 0; cnt < acl.GetRulesSize(); cnt++) {
        if (IsRuleMatched(rules[cnt], request, requiredAuth, scanForDenied, denied)) {
            allowed = true; /* track it */
        } else if (denied) {
            /* skip the remainder of the search */
            return allowed;
        }
    }
    return allowed;
}

static void GenRight(const Request& request, Right& right)
{
    if (request.propertyRequest) {
        if (request.isSetProperty) {
            if (request.outgoing) {
                /* send SetProperty */
                right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
            } else {
                /* receive SetProperty */
                right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_MODIFY;
            }
        } else {
            if (request.outgoing) {
                /* send GetProperty */
                right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
            } else {
                /* receive GetProperty */
                right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_OBSERVE;
            }
        }
    } else if (request.mbrType == PermissionPolicy::Rule::Member::METHOD_CALL) {
        if (request.outgoing) {
            /* send method call */
            right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
        } else {
            /* receive method call */
            right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_MODIFY;
        }
    } else if (request.mbrType == PermissionPolicy::Rule::Member::SIGNAL) {
        if (request.outgoing) {
            /* send a signal */
            right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_OBSERVE;
        } else {
            /* receive a signal */
            right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
        }
    }
}

/**
 * Enforce the peer's manifest
 */

static bool EnforcePeerManifest(const Request& request, const Right& right, PeerState& peerState)
{
    /* no manifest then default not allowed */
    if ((peerState->manifest == NULL) || (peerState->manifestSize == 0)) {
        return false;
    }

    for (size_t cnt = 0; cnt < peerState->manifestSize; cnt++) {
        /* validate the peer manifest to make sure it was granted the same thing */
        bool denied = false;
        if (IsRuleMatched(peerState->manifest[cnt], request, right.authByPolicy, false, denied)) {
            return true;
        } else if (denied) {
            /* skip the remainder of the search */
            return false;
        }
    }
    return false;
}

static bool IsPeerQualifiedForAcl(const PermissionPolicy::Acl& acl, PeerState& peerState, bool trustedPeer, const qcc::ECCPublicKey* peerPublicKey, const std::vector<ECCPublicKey>& issuerChain, bool& qualifiedPeerWithPublicKey)
{
    qualifiedPeerWithPublicKey = false;
    const PermissionPolicy::Peer* peers = acl.GetPeers();
    for (size_t idx = 0; idx < acl.GetPeersSize(); idx++) {
        if (peers[idx].GetType() == PermissionPolicy::Peer::PEER_ALL) {
            return true;
        }
        if (!trustedPeer) {
            continue;
        }
        if (peers[idx].GetType() == PermissionPolicy::Peer::PEER_ANY_TRUSTED) {
            return true;
        }
        if (peerPublicKey == NULL) {
            continue;
        }
        if ((peers[idx].GetType() == PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY) && peers[idx].GetKeyInfo()) {
            if (*peers[idx].GetKeyInfo()->GetPublicKey() == *peerPublicKey) {
                qualifiedPeerWithPublicKey = true;
                return true;
            }
        }
        if ((peers[idx].GetType() == PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY) && peers[idx].GetKeyInfo()) {
            for (std::vector<ECCPublicKey>::const_iterator it = issuerChain.begin(); it != issuerChain.end(); it++) {
                if (*peers[idx].GetKeyInfo()->GetPublicKey() == *it) {
                    return true;
                }
            }
        }
        if (peers[idx].GetType() == PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP) {
            for (_PeerState::GuildMap::iterator it = peerState->guildMap.begin(); it != peerState->guildMap.end(); it++) {
                _PeerState::GuildMetadata* metadata = it->second;
                if (metadata->certChain.size() > 0) {
                    if (metadata->certChain[0]->GetType() != CertificateX509::MEMBERSHIP_CERTIFICATE) {
                        continue;
                    }
                    MembershipCertificate* membershipCert = (MembershipCertificate*) metadata->certChain[0];
                    if (peers[idx].GetSecurityGroupId() == membershipCert->GetGuild()) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/**
 * Is this peer authorized?
 * Search all the applicable ACLs that the peer qualifies for a denied or
 * at least one allow.
 * The peer is authorized if there is no applicable deny and at least one allow.
 */

static bool IsPeerAuthorized(const Request& request, const PermissionPolicy* policy, PeerState& peerState, bool trustedPeer, const qcc::ECCPublicKey* peerPublicKey, const std::vector<ECCPublicKey>& issuerChain, uint8_t requiredAuth, bool& denied)
{
    bool allowed = false;
    denied = false;
    bool qualifiedPeerWithPublicKey = false;
    const PermissionPolicy::Acl* acls = policy->GetAcls();
    for (size_t cnt = 0; cnt < policy->GetAclsSize(); cnt++) {
        if (!IsPeerQualifiedForAcl(acls[cnt], peerState, trustedPeer, peerPublicKey, issuerChain, qualifiedPeerWithPublicKey)) {
            continue;
        }
        if (IsPolicyAclMatched(acls[cnt], request, requiredAuth, qualifiedPeerWithPublicKey, denied)) {
            allowed = true;   /* track it */
        }
        if (denied) {
            /* skip the remainder */
            return allowed;
        }
    }
    return allowed;
}

/**
 * The search order through the Acls:
 * 1. peer public key
 * 2. security group membership
 * 3. from specific certificate authority
 * 4. any trusted peer
 * 5. all peers
 */

static bool IsAuthorized(const Request& request, const PermissionPolicy* policy, PeerState& peerState, PermissionMgmtObj* permissionMgmtObj)
{
    Right right;
    GenRight(request, right);

    bool authorized = false;
    bool denied = false;
    bool enforceManifest = true;

    QCC_DbgPrintf(("IsAuthorized with required permission %d\n", right.authByPolicy));

    if (right.authByPolicy) {
        if (policy == NULL) {
            authorized = false;  /* no policy deny all */
            QCC_DbgPrintf(("Not authorized because of missing policy"));
            return false;
        }
        /* validate the remote peer auth data to make sure it was granted to perform such action */
        ECCPublicKey peerPublicKey;
        KeyInfoNISTP256 publicKeyInfo;
        std::vector<ECCPublicKey> issuerPublicKeys;
        const ECCPublicKey* trustedPeerPublicKey = NULL;
        bool trustedPeer = false;
        if (peerState->IsLocalPeer()) {
            if (ER_OK == permissionMgmtObj->GetPublicKey(publicKeyInfo)) {
                trustedPeerPublicKey = publicKeyInfo.GetPublicKey();
                trustedPeer = true;
                enforceManifest = false;
            }
        } else {
            bool publicKeyFound = false;
            qcc::String authMechanism;
            if (ER_OK == permissionMgmtObj->GetConnectedPeerAuthMetadata(peerState->GetGuid(), authMechanism, publicKeyFound, &peerPublicKey, NULL, issuerPublicKeys)) {
                /* trusted peer */
                if (publicKeyFound) {
                    trustedPeerPublicKey = &peerPublicKey;
                    trustedPeer = true;
                } else if ((authMechanism == KeyExchangerECDHE_PSK::AuthName()) ||
                           (authMechanism == AuthMechSRP::AuthName()) ||
                           (authMechanism == AuthMechLogon::AuthName())) {
                    trustedPeer = true;
                    enforceManifest = false;
                } else {
                    enforceManifest = false;
                }
            }
        }
        authorized = IsPeerAuthorized(request, policy, peerState, trustedPeer, trustedPeerPublicKey, issuerPublicKeys, right.authByPolicy, denied);
#ifndef NDEBUG
        for (_PeerState::GuildMap::iterator it = peerState->guildMap.begin(); it != peerState->guildMap.end(); it++) {
            _PeerState::GuildMetadata* metadata = it->second;
            if (metadata->certChain.size() == 0) {
                QCC_DbgPrintf(("Peer has no membership"));
            } else if (metadata->certChain[0]->GetType() == CertificateX509::MEMBERSHIP_CERTIFICATE) {
                MembershipCertificate* membershipCert = (MembershipCertificate*) metadata->certChain[0];
                QCC_DbgPrintf(("Peer membership guid %s", membershipCert->GetGuild().ToString().c_str()));
            }
        }
#endif
        QCC_DbgPrintf(("Peer's public key: %s Authorized: %d Denied: %d", trustedPeerPublicKey ? trustedPeerPublicKey->ToString().c_str() : "N/A", authorized, denied));
        if (denied || !authorized) {
            return false;
        }
    }

    if (authorized && enforceManifest) {
        authorized = EnforcePeerManifest(request, right, peerState);
    }
    return authorized;
}

static bool IsStdInterface(const char* iName)
{
    if (strcmp(iName, org::alljoyn::Bus::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Daemon::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Daemon::Debug::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Bus::Peer::Authentication::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Bus::Peer::Session::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::allseen::Introspectable::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Bus::Peer::HeaderCompression::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::freedesktop::DBus::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::freedesktop::DBus::Peer::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::freedesktop::DBus::Introspectable::InterfaceName) == 0) {
        return true;
    }
    return false;
}

static bool IsPropertyInterface(const char* iName)
{
    if (strcmp(iName, org::freedesktop::DBus::Properties::InterfaceName) == 0) {
        return true;
    }
    return false;
}

static bool IsPermissionMgmtInterface(const char* iName)
{
    if (strcmp(iName, org::alljoyn::Bus::Security::Application::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Bus::Security::ClaimableApplication::InterfaceName) == 0) {
        return true;
    }
    if (strcmp(iName, org::alljoyn::Bus::Security::ManagedApplication::InterfaceName) == 0) {
        return true;
    }
    return false;
}

static QStatus ParsePropertiesMessage(Request& request, Message& msg)
{
    QStatus status;
    const char* mbrName = msg->GetMemberName();
    const char* propIName;
    const char* propName = "";

    if (strncmp(mbrName, "GetAll", 6) == 0) {
        propName = NULL;
        if (request.outgoing) {
            const MsgArg* args;
            size_t numArgs;
            msg->GetRefArgs(numArgs, args);
            if (numArgs < 1) {
                return ER_INVALID_DATA;
            }
            status = args[0].Get("s", &propIName);
        } else {
            status = msg->GetArgs("s", &propIName);
        }
        if (status != ER_OK) {
            return status;
        }
        request.propertyRequest = true;
        request.mbrType = PermissionPolicy::Rule::Member::PROPERTY;
        QCC_DbgPrintf(("PermissionManager::ParsePropertiesMessage %s %s", mbrName, propIName));
    } else if ((strncmp(mbrName, "Get", 3) == 0) || (strncmp(mbrName, "Set", 3) == 0)) {
        const MsgArg* args;
        size_t numArgs;
        if (request.outgoing) {
            msg->GetRefArgs(numArgs, args);
        } else {
            msg->GetArgs(numArgs, args);
        }
        if (numArgs < 2) {
            return ER_INVALID_DATA;
        }
        /* only interested in the first two arguments */
        status = args[0].Get("s", &propIName);
        if (ER_OK != status) {
            return status;
        }
        status = args[1].Get("s", &propName);
        if (status != ER_OK) {
            return status;
        }
        request.propertyRequest = true;
        request.mbrType = PermissionPolicy::Rule::Member::PROPERTY;
        request.isSetProperty = (strncmp(mbrName, "Set", 3) == 0);
        QCC_DbgPrintf(("PermissionManager::ParsePropertiesMessage %s %s.%s", mbrName, propIName, propName));
    } else {
        return ER_FAIL;
    }
    request.iName = propIName;
    request.mbrName = propName;
    return ER_OK;
}

bool PermissionManager::PeerHasAdminPriv(PeerState& peerState)
{
    /* now check the admin security group membership */
    for (_PeerState::GuildMap::iterator it = peerState->guildMap.begin(); it != peerState->guildMap.end(); it++) {
        _PeerState::GuildMetadata* metadata = it->second;
        if (permissionMgmtObj->IsAdminGroup(metadata->certChain)) {
            return true;
        }
    }
    return false;
}

bool PermissionManager::AuthorizePermissionMgmt(bool outgoing, PeerState& peerState, const char* iName, const char* mbrName)
{
    if (outgoing) {
        return true;  /* always allow send action */
    }
    bool authorized = false;
    if (strncmp(mbrName, "Version", 7) == 0) {
        return true;
    }

    if (strcmp(iName, org::alljoyn::Bus::Security::ClaimableApplication::InterfaceName) == 0) {
        if (strncmp(mbrName, "Claim", 5) == 0) {
            /* only allowed when there is no trust anchor */
            return (!permissionMgmtObj->HasTrustAnchors());
        }
    } else if (strcmp(iName, org::alljoyn::Bus::Security::ManagedApplication::InterfaceName) == 0) {
        if (
            (strncmp(mbrName, "Identity", 8) == 0) ||
            (strncmp(mbrName, "Manifest", 8) == 0) ||
            (strncmp(mbrName, "IdentityCertificateId", 8) == 0) ||
            (strncmp(mbrName, "DefaultPolicy", 13) == 0)
            ) {
            return true;
        } else if (
            (strncmp(mbrName, "ReplaceIdentity", 15) == 0) ||
            (strncmp(mbrName, "Reset", 5) == 0) ||
            (strncmp(mbrName, "PolicyVersion", 13) == 0) ||
            (strncmp(mbrName, "Policy", 6) == 0) ||
            (strncmp(mbrName, "MembershipSummaries", 19) == 0)
            ) {
            /* these actions require admin privilege */
            return PeerHasAdminPriv(peerState);
        }
    } else if (strcmp(iName, org::alljoyn::Bus::Security::Application::InterfaceName) == 0) {
        if (
            (strncmp(mbrName, "ApplicationState", 16) == 0) ||
            (strncmp(mbrName, "ManifestTemplateDigest", 22) == 0) ||
            (strncmp(mbrName, "EccPublicKey", 12) == 0) ||
            (strncmp(mbrName, "ManufacturerCertificate", 23) == 0) ||
            (strncmp(mbrName, "ManifestTemplate", 16) == 0) ||
            (strncmp(mbrName, "ClaimCapabilities", 17) == 0) ||
            (strncmp(mbrName, "ClaimCapabilityAdditionalInfo", 29) == 0)
            ) {
            return true;
        }
    }
    return authorized;
}

QStatus PermissionManager::AuthorizeMessage(bool outgoing, Message& msg, PeerState& peerState)
{
    QStatus status = ER_PERMISSION_DENIED;
    bool authorized = false;

    /* only checks for method call and signal */
    if ((msg->GetType() != MESSAGE_METHOD_CALL) &&
        (msg->GetType() != MESSAGE_SIGNAL)) {
        return ER_OK;
    }

    /* skip the AllJoyn Std interfaces */
    if (IsStdInterface(msg->GetInterface())) {
        return ER_OK;
    }
    Request request(msg, outgoing);
    if (IsPropertyInterface(msg->GetInterface())) {
        status = ParsePropertiesMessage(request, msg);
        if (status != ER_OK) {
            return status;
        }
    } else {
        request.iName = msg->GetInterface();
        request.mbrName = msg->GetMemberName();
    }
    if (!permissionMgmtObj) {
        return ER_PERMISSION_DENIED;
    }
    bool isPermissionMgmt = IsPermissionMgmtInterface(request.iName);
    if (isPermissionMgmt) {
        if (AuthorizePermissionMgmt(outgoing, peerState, request.iName, request.mbrName)) {
            return ER_OK;
        }
    }
    /* Is the app claimed? If not claimed, no enforcement unless it's one of
       the permission management interfaces */
    if (!permissionMgmtObj->HasTrustAnchors()) {
        if (isPermissionMgmt) {
            return ER_PERMISSION_DENIED;
        }
        return ER_OK;
    }

    QCC_DbgPrintf(("PermissionManager::AuthorizeMessage with outgoing: %d msg %s", outgoing, msg->ToString().c_str()));
    QCC_DbgPrintf(("PermissionManager::AuthorizeMessage: local policy %s", GetPolicy() ? GetPolicy()->ToString().c_str() : "NULL"));

    authorized = IsAuthorized(request, GetPolicy(), peerState, permissionMgmtObj);
    if (!authorized) {
        QCC_DbgPrintf(("PermissionManager::AuthorizeMessage IsAuthorized returns ER_PERMISSION_DENIED\n"));
        return ER_PERMISSION_DENIED;
    }
    return ER_OK;
}

QStatus PermissionManager::AuthorizeGetProperty(const char* objPath, const char* ifcName, const char* propName, PeerState& peerState)
{
    if (!permissionMgmtObj) {
        return ER_PERMISSION_DENIED;
    }

    /* is the app claimed? If not claimed, no enforcement */
    if (!permissionMgmtObj->HasTrustAnchors()) {
        return ER_OK;
    }

    QCC_DbgPrintf(("PermissionManager::AuthorizeGetProperty: local policy %s", GetPolicy() ? GetPolicy()->ToString().c_str() : "NULL"));

    Request request(objPath, ifcName, propName, PermissionPolicy::Rule::Member::PROPERTY, false, true);
    if (!IsAuthorized(request, GetPolicy(), peerState, permissionMgmtObj)) {
        QCC_DbgPrintf(("PermissionManager::AuthorizeGetProperty IsAuthorized returns ER_PERMISSION_DENIED\n"));
        return ER_PERMISSION_DENIED;
    }
    return ER_OK;
}

} /* namespace ajn */

