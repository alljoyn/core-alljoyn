/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include <qcc/platform.h>
#include <alljoyn/PermissionPolicy.h>

namespace ajn {
class PolicyOverwriteUtils {
  public:

    static void ChangeMemberName(PermissionPolicy::Rule& rule, size_t memberIndex, AJ_PCSTR newName);

    static void ChangeMemberType(PermissionPolicy::Rule& rule, size_t memberIndex, PermissionPolicy::Rule::Member::MemberType newType);

    static void ChangeMemberActionMask(PermissionPolicy::Rule& rule, size_t memberIndex, uint8_t newActionMask);

    static void ChangeRecommendedSecurityLevel(PermissionPolicy::Rule::SecurityLevel securityLevel, PermissionPolicy& policy);

    static void ChangeRules(size_t rulesCount, PermissionPolicy::Rule* rules, PermissionPolicy& policy);

    static void ChangePeers(size_t peersCount, PermissionPolicy::Peer* peers, PermissionPolicy& policy);

    static void ChangePeerType(size_t m_peerIndex, PermissionPolicy::Peer::PeerType peerType, PermissionPolicy& policy);

    static void ChangePeerPublicKey(size_t m_peerIndex, AJ_PCSTR publicKeyPem, PermissionPolicy& policy);

    static void ChangePeerSgId(size_t m_peerIndex, AJ_PCSTR sgIdHex, PermissionPolicy& policy);

    static PermissionPolicy::Peer BuildPeer(PermissionPolicy::Peer::PeerType peerType, AJ_PCSTR publicKeyPem = nullptr, AJ_PCSTR sgIdHex = nullptr);

  private:

    static void SetPeerPublicKey(AJ_PCSTR publicKeyPem, PermissionPolicy::Peer& peer);
};
};