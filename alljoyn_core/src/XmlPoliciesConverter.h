#ifndef _ALLJOYN_POLICIES_XML_CONVERTER_H
#define _ALLJOYN_POLICIES_XML_CONVERTER_H
/**
 * @file
 * This file defines the converter for Security 2.0 policies in XML format
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

#ifndef __cplusplus
#error Only include XmlPoliciesConverter.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/XmlElement.h>
#include <alljoyn/Status.h>
#include <alljoyn/PermissionPolicy.h>
#include <unordered_map>
#include "XmlPoliciesValidator.h"

namespace ajn {

#define POLICY_VERSION_INDEX 0
#define SERIAL_NUMBER_INDEX 1
#define ACLS_INDEX 2
#define PEERS_INDEX 0
#define RULES_INDEX 1
#define PEER_TYPE_INDEX 0
#define PEER_PUBLIC_KEY_INDEX 1
#define PEER_SGID_INDEX 2
#define DECIMAL_BASE 10

#define XML_PEER_ALL "ALL"
#define XML_PEER_ANY_TRUSTED "ANY_TRUSTED"
#define XML_PEER_FROM_CERTIFICATE_AUTHORITY "FROM_CERTIFICATE_AUTHORITY"
#define XML_PEER_WITH_PUBLIC_KEY "WITH_PUBLIC_KEY"
#define XML_PEER_WITH_MEMBERSHIP "WITH_MEMBERSHIP"

class XmlPoliciesConverter {
  public:

    /**
     * Extract policy from an XML. The policy XML schema
     * is available under alljoyn_core/docs/policy.xsd.
     *
     * @param[in]    policyXml   Policy in XML format.
     * @param[out]   policy      Reference to the new policy.
     *
     * @return   #ER_OK if extracted correctly.
     *           #ER_INVALID_ADDRESS if policyXml is null.
     *           #ER_XML_MALFORMED if the XML does not follow the policy XML schema.
     */
    static QStatus FromXml(AJ_PCSTR policyXml, PermissionPolicy& policy);

  private:

    /**
     * Sets all of the PermissionPolicy values according to the input XML.
     *
     * @param[in]    root    Root element of the policy XML.
     * @param[out]   policy  Reference to the new policy.
     */
    static void BuildPolicy(const qcc::XmlElement* root, PermissionPolicy& policy);

    /**
     * Set the PermissionPolicy's version according to the input XML.
     *
     * @param[in]    xmlPolicyVerion The "policyVersion" element of the policy XML.
     * @param[out]   policy          Reference to the new policy.
     */
    static void SetPolicyVersion(const qcc::XmlElement* xmlPolicyVerion, PermissionPolicy& policy);

    /**
     * Set the PermissionPolicy's serial number according to the input XML.
     *
     * @param[in]    xmlSerialNumber The "serialNumber" element of the policy XML.
     * @param[out]   policy          Reference to the new policy.
     */
    static void SetPolicySerialNumber(const qcc::XmlElement* xmlSerialNumber, PermissionPolicy& policy);

    /**
     * Set the PermissionPolicy's ACLs according to the input XML.
     *
     * @param[in]    acls    The "acls" element of the policy XML.
     * @param[out]   policy  Reference to the new policy.
     */
    static void SetPolicyAcls(const qcc::XmlElement* acls, PermissionPolicy& policy);

    /**
     * Adds a PermissionPolicy::Acl object to the vector using the provided XML element.
     *
     * @param[in]    aclXml      The "acl" element of the policy XML.
     * @param[in]    aclsVector  Vector to add the new ACL to.
     */
    static void AddAcl(const qcc::XmlElement* aclXml, std::vector<PermissionPolicy::Acl>& aclsVector);

    /**
     * Sets all of the PermissionPolicy::Acl values according to the input XML.
     *
     * @param[in]    aclXml  The "acl" element of the policy XML.
     * @param[out]   acl     Reference to the new ACL.
     */
    static void BuildAcl(const qcc::XmlElement* aclXml, PermissionPolicy::Acl& acl);

    /**
     * Set the PermissionPolicy::Acl's peers according to the input XML.
     *
     * @param[in]    peersXml    The "peers" element of the policy XML.
     * @param[out]   acl         Reference to the new ACL.
     */
    static void SetAclPeers(const qcc::XmlElement* peersXml, PermissionPolicy::Acl& acl);

    /**
     * Set the PermissionPolicy::Acl's rules according to the input XML.
     *
     * @param[in]    rulesXml    The "rules" element of the policy XML.
     * @param[out]   acl         Reference to the new ACL.
     */
    static void SetAclRules(const qcc::XmlElement* rulesXml, PermissionPolicy::Acl& acl);

    /**
     * Adds a PermissionPolicy::Peer object to the vector using the provided XML element.
     *
     * @param[in]    peerXml The "peer" element of the policy XML.
     * @param[in]    peers   Vector to add the new peer to.
     */
    static void AddPeer(const qcc::XmlElement* peerXml, std::vector<PermissionPolicy::Peer>& peers);

    /**
     * Sets all of the PermissionPolicy::Peer values according to the input XML.
     *
     * @param[in]    peerXml The "peer" element of the policy XML.
     * @param[out]   peer    Reference to the new peer.
     */
    static void BuildPeer(const qcc::XmlElement* peerXml, PermissionPolicy::Peer& peer);

    /**
     * Set the PermissionPolicy::Peer's type according to the input XML.
     *
     * @param[in]    peerXml The "peer" element of the policy XML.
     * @param[out]   peer    Reference to the new peer.
     */
    static void SetPeerType(const qcc::XmlElement* peerXml, PermissionPolicy::Peer& peer);

    /**
     * Set the PermissionPolicy::Peer's public key according to the input XML.
     *
     * @param[in]    peerXml The "peer" element of the policy XML.
     * @param[out]   peer    Reference to the new peer.
     */
    static void SetPeerPublicKey(const qcc::XmlElement* peerXml, PermissionPolicy::Peer& peer);

    /**
     * Set the PermissionPolicy::Peer's sgID according to the input XML.
     *
     * @param[in]    peerXml The "peer" element of the policy XML.
     * @param[out]   peer    Reference to the new peer.
     */
    static void SetPeerSgId(const qcc::XmlElement* peerXml, PermissionPolicy::Peer& peer);

    /**
     * Determines if the given XmlElement contains the "publicKey" element inside it.
     *
     * @param[in]    peerXml The "peer" element of the policy XML.
     *
     * @return   True if the "peer" element contains the "publicKey" element.
     *           False otherwise.
     */
    static bool PeerContainsPublicKey(const qcc::XmlElement* peerXml);

    /**
     * Determines if the given XmlElement contains the "sgID" element inside it.
     *
     * @param[in]    peerXml The "peer" element of the policy XML.
     *
     * @return   True if the "peer" element contains the "sgID" element.
     *           False otherwise.
     */
    static bool PeerContainsSgId(const qcc::XmlElement* peerXml);
};
} /* namespace ajn */
#endif