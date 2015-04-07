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

#define QCC_MODULE "PERMISSION_MGMT"

using namespace std;
using namespace qcc;

namespace ajn {

struct MessageHolder {
    Message& msg;
    bool outgoing;
    bool propertyRequest;
    bool isSetProperty;
    const char* objPath;
    const char* iName;
    const char* mbrName;
    PermissionPolicy::Rule::Member::MemberType mbrType;


    MessageHolder(Message& msg, bool outgoing) : msg(msg), outgoing(outgoing), propertyRequest(false), isSetProperty(false), iName(NULL), mbrName(NULL)
    {
        objPath = msg->GetObjectPath();
        mbrType = PermissionPolicy::Rule::Member::NOT_SPECIFIED;
        if (msg->GetType() == MESSAGE_METHOD_CALL) {
            mbrType = PermissionPolicy::Rule::Member::METHOD_CALL;
        } else if (msg->GetType() == MESSAGE_SIGNAL) {
            mbrType = PermissionPolicy::Rule::Member::SIGNAL;
        }
    }

  private:
    MessageHolder& operator=(const MessageHolder& other);
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
    return (allowedActions& PermissionPolicy::Rule::Member::ACTION_DENIED) == PermissionPolicy::Rule::Member::ACTION_DENIED;
}

/**
 * Validates whether the request action is allowed based on the allow action.
 * @param allowedActions the allowed actions
 * @param requestedAction the request action
 * @return true is the requested action is allowed; false, otherwise.
 */
static bool IsActionAllowed(uint8_t allowedActions, uint8_t requestedAction)
{
    if ((allowedActions & requestedAction) == requestedAction) {
        return true;
    }
    if ((requestedAction == PermissionPolicy::Rule::Member::ACTION_OBSERVE) && ((allowedActions& PermissionPolicy::Rule::Member::ACTION_MODIFY) == PermissionPolicy::Rule::Member::ACTION_MODIFY)) {
        return true; /* lesser right is allowed */
    }
    return false;
}

/**
 * Verify whether the given rule is a match for the given message.
 * If the rule has both object path and interface name, the message must prefix match both.
 * If the rule has object path, the message must prefix match the object path.
 * If the rule has interface name, the message must prefix match the interface name.
 * Find match in member name
 * Verify whether the requested right is allowed by the authorization at the member.
 *      When a member name has an exact match and is explicitly denied access then the rule is not a match.
 *      When a member name has an exact match and is authorized then the rule isa match
 *      When a member name has a prefix match and is authorized then the rule is a match
 *
 */

static bool IsRuleMatched(const PermissionPolicy::Rule& rule, const MessageHolder& msgHolder, uint8_t requiredAuth, bool& denied)
{
    if (rule.GetMembersSize() == 0) {
        return false;
    }
    bool firstPartMatch = false;
    if (!rule.GetObjPath().empty()) {
        /* rule has an object path */
        if ((rule.GetObjPath() == msgHolder.objPath) || MatchesPrefix(msgHolder.objPath, rule.GetObjPath())) {
            if (!rule.GetInterfaceName().empty()) {
                /* rule has a specific interface name */
                firstPartMatch = (rule.GetInterfaceName() == msgHolder.iName) || MatchesPrefix(msgHolder.iName, rule.GetInterfaceName());
            } else {
                firstPartMatch = true;
            }
        }
    } else if (!rule.GetInterfaceName().empty()) {
        if ((rule.GetInterfaceName() == msgHolder.iName) || MatchesPrefix(msgHolder.iName, rule.GetInterfaceName())) {
            /* rule has a specific interface name */
            firstPartMatch = true;
        }
    }

    if (!firstPartMatch) {
        return false;
    }

    bool msgMbrNameEmpty = !msgHolder.mbrName || (strlen(msgHolder.mbrName) == 0);
    const PermissionPolicy::Rule::Member* members = rule.GetMembers();
    int8_t* buckets = new int8_t[rule.GetMembersSize()];
    for (size_t cnt = 0; cnt < rule.GetMembersSize(); cnt++) {
        buckets[cnt] = 0;
        if (msgMbrNameEmpty) {
            /* potential to match all members.  Additional check later */
            buckets[cnt] = 1;
        } else if (!members[cnt].GetMemberName().empty()) {
            if (members[cnt].GetMemberName() == msgHolder.mbrName) {
                /* rule has a specific member name match */
                buckets[cnt] = 2;
            } else if (MatchesPrefix(msgHolder.mbrName, members[cnt].GetMemberName())) {
                /* rule has a prefix match member name */
                buckets[cnt] = 1;
            } else {
                continue;  /* the names are different.  Skip it */
            }
        }
        /* match member type */
        if (members[cnt].GetMemberType() != PermissionPolicy::Rule::Member::NOT_SPECIFIED) {
            if (msgHolder.mbrType != members[cnt].GetMemberType()) {
                buckets[cnt] = 0;  /* reset since it's not a match */
                continue;  /* no matching type */
            }
            if (buckets[cnt] == 0) {
                /* rule has no name but type matches */
                buckets[cnt] = 1;
            }
        }

        if (buckets[cnt] > 0) {
            /* now check the action mask */
            if (IsActionDenied(members[cnt].GetActionMask())) {
                denied = true;
                buckets[cnt] = -buckets[cnt];
            } else if (!IsActionAllowed(members[cnt].GetActionMask(), requiredAuth)) {
                if (msgMbrNameEmpty) {
                    /* When only the interface name is specified, all rules
                       for the given member type must be satisfied. If any of the member fails to authorize then the whole thing would fail authorization */
                    buckets[cnt] = -buckets[cnt];
                } else {
                    buckets[cnt] = 0;
                }
            }
        }
    }
    /* now go through the findings */
    for (size_t cnt = 0; cnt < rule.GetMembersSize(); cnt++) {
        if (buckets[cnt] == -2) {
            delete [] buckets;
            return false; /* specifically denied by exact name */
        }
    }
    for (size_t cnt = 0; cnt < rule.GetMembersSize(); cnt++) {
        if (buckets[cnt] == 2) {
            delete [] buckets;
            return true;   /* there is an authorized match with exact name */
        }
    }
    for (size_t cnt = 0; cnt < rule.GetMembersSize(); cnt++) {
        if (buckets[cnt] < 0) {
            delete [] buckets;
            return false;   /* there is a denial based on prefix name match */
        }
    }
    for (size_t cnt = 0; cnt < rule.GetMembersSize(); cnt++) {
        if (buckets[cnt] > 0) {
            delete [] buckets;
            return true;   /* there is an authorized match */
        }
    }
    delete [] buckets;
    return false;
}

static bool IsPolicyTermMatched(const PermissionPolicy::Term& term, const MessageHolder& msgHolder, uint8_t requiredAuth, bool& denied)
{
    const PermissionPolicy::Rule* rules = term.GetRules();
    for (size_t cnt = 0; cnt < term.GetRulesSize(); cnt++) {
        if (IsRuleMatched(rules[cnt], msgHolder, requiredAuth, denied)) {
            return true;
        } else if (denied) {
            /* skip the remainder of the search */
            return false;
        }
    }
    return false;
}

bool IsPeerLevelMatched(PermissionPolicy::Peer::PeerAuthLevel ruleLevel, PermissionPolicy::Peer::PeerAuthLevel requestingLevel)
{
    return requestingLevel >= ruleLevel;
}

static bool IsAuthorizedByAnyUserPolicy(const PermissionPolicy* policy, const MessageHolder& msgHolder, PermissionPolicy::Peer::PeerAuthLevel peerLevel, uint8_t requiredAuth, bool& denied)
{
    denied = false;
    const PermissionPolicy::Term* terms = policy->GetTerms();
    for (size_t cnt = 0; cnt < policy->GetTermsSize(); cnt++) {
        const PermissionPolicy::Peer* peers = terms[cnt].GetPeers();
        bool qualified = false;
        for (size_t idx = 0; idx < terms[cnt].GetPeersSize(); idx++) {
            if ((peers[idx].GetType() == PermissionPolicy::Peer::PEER_ANY) && IsPeerLevelMatched(peers[idx].GetLevel(), peerLevel)) {
                qualified = true;
                break;
            }
        }
        if (!qualified) {
            continue;
        }
        if (IsPolicyTermMatched(terms[cnt], msgHolder, requiredAuth, denied)) {
            return true;
        } else if (denied) {
            /* skip the remainder of the search */
            return false;
        }
    }
    return false;
}

static bool TermHasMatchingGuild(const PermissionPolicy::Term& term, const GUID128& guildGUID)
{
    /* is this peer entry has matching guild GUID */
    const PermissionPolicy::Peer* peers = term.GetPeers();
    for (size_t idx = 0; idx < term.GetPeersSize(); idx++) {
        if (peers[idx].GetType() == PermissionPolicy::Peer::PEER_GUILD) {
            if (peers[idx].GetGuildId() == guildGUID) {
                return true;
            }
        }
    }
    return false;
}

static bool IsAuthorizedByMembership(const GUID128& guildGUID, PermissionPolicy& authData, const MessageHolder& msgHolder, uint8_t requiredAuth, bool& denied)
{
    const PermissionPolicy::Term* terms = authData.GetTerms();
    for (size_t cnt = 0; cnt < authData.GetTermsSize(); cnt++) {
        bool qualified = false;
        if (terms[cnt].GetPeersSize() == 0) {
            qualified = true;  /* there is no peer restriction for this term */
        } else {
            /* look for peer entry with matching guild GUID */
            qualified = TermHasMatchingGuild(terms[cnt], guildGUID);
        }
        if (qualified) {
            if (IsPolicyTermMatched(terms[cnt], msgHolder, requiredAuth, denied)) {
                return true;
            } else if (denied) {
                /* skip the remainder of the search */
                return false;
            }
        }
    }
    return false;
}

static bool IsAuthorizedByMembershipChain(_PeerState::GuildMetadata& metadata, const MessageHolder& msgHolder, uint8_t requiredAuth, bool& denied)
{
    /* The message must be authorized by each of the auth data in the membership cert chain */

    /* check the leaf cert first */
    bool authorized = IsAuthorizedByMembership(metadata.cert.GetGuild(), metadata.authData, msgHolder, requiredAuth, denied);
    if (!authorized) {
        return false;
    }

    /* check the remaining certs in the cert chain */
    for (std::vector<_PeerState::MembershipMetaPair*>::iterator ccit = metadata.certChain.begin(); ccit != metadata.certChain.end(); ccit++) {
        authorized = IsAuthorizedByMembership((*ccit)->cert.GetGuild(), (*ccit)->authData, msgHolder, requiredAuth, denied);
        if (!authorized) {
            return false;
        }
    }

    return authorized;
}

/**
 * Is the given message authorized by a guild policy that is common between the peer.
 * The consumer must be both authorized in its membership and in the provider's policy for any guild in common.
 */
static bool IsAuthorizedByGuildsInCommonPolicies(const PermissionPolicy* policy, const MessageHolder& msgHolder, uint8_t policyAuth, PeerState& peerState, bool& denied)
{
    denied = false;
    for (_PeerState::GuildMap::iterator it = peerState->guildMap.begin(); it != peerState->guildMap.end(); it++) {
        _PeerState::GuildMetadata* metadata = it->second;
        const PermissionPolicy::Term* terms = policy->GetTerms();
        for (size_t cnt = 0; cnt < policy->GetTermsSize(); cnt++) {
            /* look for peer entry with matching guild GUID */
            if (!TermHasMatchingGuild(terms[cnt], metadata->cert.GetGuild())) {
                continue;
            }
            if (IsPolicyTermMatched(terms[cnt], msgHolder, policyAuth, denied)) {
                /* validate the peer auth data to make sure it was granted the same thing */
                if (IsAuthorizedByMembershipChain(*metadata, msgHolder, policyAuth, denied)) {
                    return true;
                }
            }
            if (denied) {
                /* skip the remainder */
                return false;
            }
        }
    }
    return false;
}

static bool IsAuthorizedByPeerPublicKey(const PermissionPolicy* policy, const ECCPublicKey& peerPublicKey, const MessageHolder& msgHolder, uint8_t requiredAuth, bool& denied)
{
    denied = false;
    const PermissionPolicy::Term* terms = policy->GetTerms();
    for (size_t cnt = 0; cnt < policy->GetTermsSize(); cnt++) {
        const PermissionPolicy::Peer* peers = terms[cnt].GetPeers();
        bool qualified = false;
        for (size_t idx = 0; idx < terms[cnt].GetPeersSize(); idx++) {
            if ((peers[idx].GetType() == PermissionPolicy::Peer::PEER_GUID) && peers[idx].GetKeyInfo()) {
                if (memcmp(peers[idx].GetKeyInfo()->GetPublicKey(), &peerPublicKey, sizeof(ECCPublicKey)) == 0) {
                    qualified = true;
                    break;
                }
            }
        }
        if (!qualified) {
            continue;
        }
        if (IsPolicyTermMatched(terms[cnt], msgHolder, requiredAuth, denied)) {
            return true;
        } else if (denied) {
            /* skip the remainder */
            return false;
        }
    }
    return false;
}

static void GenRight(const MessageHolder& msgHolder, Right& right)
{
    if (msgHolder.propertyRequest) {
        if (msgHolder.isSetProperty) {
            if (msgHolder.outgoing) {
                /* send SetProperty */
                right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
            } else {
                /* receive SetProperty */
                right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_MODIFY;
            }
        } else {
            if (msgHolder.outgoing) {
                /* send GetProperty */
                right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
            } else {
                /* receive GetProperty */
                right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_OBSERVE;
            }
        }
    } else if (msgHolder.msg->GetType() == MESSAGE_METHOD_CALL) {
        if (msgHolder.outgoing) {
            /* send method call */
            right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
        } else {
            /* receive method call */
            right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_MODIFY;
        }
    } else if (msgHolder.msg->GetType() == MESSAGE_SIGNAL) {
        if (msgHolder.outgoing) {
            /* send a signal */
            right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_OBSERVE;
        } else {
            /* receive a signal */
            right.authByPolicy = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
        }
    }
}

/**
 * The search order through the terms:
 * 1. Peer specific terms
 * 2. guild-in-common terms
 * 3. ANY-USER terms
 */

static bool IsAuthorized(const MessageHolder& msgHolder, const PermissionPolicy* policy, PeerState& peerState, PermissionMgmtObj* permissionMgmtObj)
{
    Right right;
    GenRight(msgHolder, right);

    bool authorized = false;
    bool denied = false;

    QCC_DbgPrintf(("IsAuthorized with required permission policy %d\n", right.authByPolicy));

    if (right.authByPolicy) {
        if (policy == NULL) {
            authorized = false;  /* no policy deny all */
            QCC_DbgPrintf(("Not authorized because of missing policy"));
            return false;
        }
        /* validate the remote peer auth data to make sure it was granted to perform such action */
        ECCPublicKey peerPublicKey;
        PermissionPolicy::Peer::PeerAuthLevel peerAuthLevel = PermissionPolicy::Peer::PEER_LEVEL_ENCRYPTED;
        QStatus status = permissionMgmtObj->GetConnectedPeerPublicKey(peerState->GetGuid(), &peerPublicKey);
        if (ER_OK == status) {
            peerAuthLevel = PermissionPolicy::Peer::PEER_LEVEL_AUTHENTICATED;
            authorized = IsAuthorizedByPeerPublicKey(policy, peerPublicKey, msgHolder, right.authByPolicy, denied);
            QCC_DbgPrintf(("authorized by peer specific policy terms: %d denied %d", authorized, denied));
            if (denied) {
                QCC_DbgPrintf(("Denied by peer specific policy"));
                return false;
            }
        }
        if (!authorized) {
            authorized = IsAuthorizedByGuildsInCommonPolicies(policy, msgHolder, right.authByPolicy, peerState, denied);
            if (denied) {
                QCC_DbgPrintf(("Denied by guild specific policy"));
                return false;
            }
            QCC_DbgPrintf(("authorized by guild policy terms in common: %d", authorized));
        }
        if (!authorized) {
            authorized = IsAuthorizedByAnyUserPolicy(policy, msgHolder, peerAuthLevel, right.authByPolicy, denied);
            QCC_DbgPrintf(("authorized by any user peer auth level %d policy: %d", peerAuthLevel, authorized));
            if (denied) {
                QCC_DbgPrintf(("Denied by any-user policy"));
                return false;
            }
        }
        if (!authorized) {
            QCC_DbgPrintf(("Not authorized by policy"));
            return false;
        }
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
    return (strcmp(iName, org::allseen::Security::PermissionMgmt::InterfaceName) == 0);
}

static QStatus ParsePropertiesMessage(MessageHolder& holder)
{
    QStatus status;
    const char* mbrName = holder.msg->GetMemberName();
    const char* propIName;
    const char* propName = "";

    if (strncmp(mbrName, "GetAll", 6) == 0) {
        propName = NULL;
        if (holder.outgoing) {
            const MsgArg* args;
            size_t numArgs;
            holder.msg->GetRefArgs(numArgs, args);
            if (numArgs < 1) {
                return ER_INVALID_DATA;
            }
            status = args[0].Get("s", &propIName);
        } else {
            status = holder.msg->GetArgs("s", &propIName);
        }
        if (status != ER_OK) {
            return status;
        }
        holder.propertyRequest = true;
        holder.mbrType = PermissionPolicy::Rule::Member::PROPERTY;
        QCC_DbgPrintf(("PermissionManager::ParsePropertiesMessage %s %s", mbrName, propIName));
    } else if ((strncmp(mbrName, "Get", 3) == 0) || (strncmp(mbrName, "Set", 3) == 0)) {
        const MsgArg* args;
        size_t numArgs;
        if (holder.outgoing) {
            holder.msg->GetRefArgs(numArgs, args);
        } else {
            holder.msg->GetArgs(numArgs, args);
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
        holder.propertyRequest = true;
        holder.mbrType = PermissionPolicy::Rule::Member::PROPERTY;
        holder.isSetProperty = (strncmp(mbrName, "Set", 3) == 0);
        QCC_DbgPrintf(("PermissionManager::ParsePropertiesMessage %s %s.%s", mbrName, propIName, propName));
    } else {
        return ER_FAIL;
    }
    holder.iName = propIName;
    holder.mbrName = propName;
    return ER_OK;
}

bool PermissionManager::PeerHasAdminPriv(const GUID128& peerGuid)
{
    ECCPublicKey peerPublicKey;
    QStatus status = permissionMgmtObj->GetConnectedPeerPublicKey(peerGuid, &peerPublicKey);
    if (ER_OK != status) {
        QCC_DbgPrintf(("PermissionManager::PeerHasAdminPriv failed to retrieve public key for peer %s", peerGuid.ToString().c_str()));
        return false;
    }
    return permissionMgmtObj->IsAdmin(&peerPublicKey);
}

bool PermissionManager::AuthorizePermissionMgmt(bool outgoing, const GUID128& peerGuid, const char* mbrName)
{
    if (outgoing) {
        return true;  /* always allow send action */
    }
    bool authorized = false;

    if (strncmp(mbrName, "Claim", 5) == 0) {
        /* only allowed when there is no trust anchor */
        return (!permissionMgmtObj->HasTrustAnchors());
    } else if (
        (strncmp(mbrName, "InstallPolicy", 14) == 0) ||
        (strncmp(mbrName, "InstallEncryptedPolicy", 22) == 0) ||
        (strncmp(mbrName, "GetPolicy", 9) == 0) ||
        (strncmp(mbrName, "RemovePolicy", 12) == 0) ||
        (strncmp(mbrName, "InstallMembership", 17) == 0) ||
        (strncmp(mbrName, "InstallMembershipAuthData", 25) == 0) ||
        (strncmp(mbrName, "RemoveMembership", 16) == 0) ||
        (strncmp(mbrName, "InstallIdentity", 15) == 0) ||
        (strncmp(mbrName, "InstallGuildEquivalence", 23) == 0) ||
        (strncmp(mbrName, "RemoveGuildEquivalence", 22) == 0) ||
        (strncmp(mbrName, "InstallCredential", 17) == 0) ||
        (strncmp(mbrName, "RemoveCredential", 16) == 0) ||
        (strncmp(mbrName, "Reset", 5) == 0)
        ) {
        /* these actions require admin privilege */
        return PeerHasAdminPriv(peerGuid);
    } else if (
        (strncmp(mbrName, "NotifyConfig", 12) == 0) ||
        (strncmp(mbrName, "GetPublicKey", 12) == 0) ||
        (strncmp(mbrName, "GetIdentity", 11) == 0) ||
        (strncmp(mbrName, "GetManifest", 11) == 0) ||
        (strncmp(mbrName, "Version", 7) == 0)
        ) {
        return true;
    }
    return authorized;
}

/*
 * the apply order is:
 *  1. applies ANY-USER policy
 *  2. applies all guilds-in-common policies
 *  3. applies peer policies
 */
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
    MessageHolder holder(msg, outgoing);
    if (IsPropertyInterface(msg->GetInterface())) {
        status = ParsePropertiesMessage(holder);
        if (status != ER_OK) {
            return status;
        }
    } else {
        holder.iName = msg->GetInterface();
        holder.mbrName = msg->GetMemberName();
    }
    if (IsPermissionMgmtInterface(holder.iName)) {
        if (!permissionMgmtObj) {
            return ER_PERMISSION_DENIED;
        }
        if (AuthorizePermissionMgmt(outgoing, peerState->GetGuid(), holder.mbrName)) {
            return ER_OK;
        }
        QCC_DbgPrintf(("PermissionManager::AuthorizeMessage on PermissionMgmt::%s returns ER_PERMISSION_DENIED\n", holder.mbrName));
        return ER_PERMISSION_DENIED;
    }
    if (!permissionMgmtObj) {
        return ER_PERMISSION_DENIED;
    }
    /* is the app claimed? If not claimed, no enforcement */
    if (!permissionMgmtObj->HasTrustAnchors()) {
        return ER_OK;
    }

    if (!outgoing && PeerHasAdminPriv(peerState->GetGuid())) {
        QCC_DbgPrintf(("PermissionManager::AuthorizeMessage peer has admin prividege"));
        return ER_OK;  /* admin has full access */
    }

    QCC_DbgPrintf(("PermissionManager::AuthorizeMessage with outgoing: %d msg %s\nLocal policy %s", outgoing, msg->ToString().c_str(), GetPolicy() ? GetPolicy()->ToString().c_str() : "NULL"));
    authorized = IsAuthorized(holder, GetPolicy(), peerState, permissionMgmtObj);
    if (!authorized) {
        QCC_DbgPrintf(("PermissionManager::AuthorizeMessage IsAuthorized returns ER_PERMISSION_DENIED\n"));
        return ER_PERMISSION_DENIED;
    }
    return ER_OK;
}

} /* namespace ajn */

