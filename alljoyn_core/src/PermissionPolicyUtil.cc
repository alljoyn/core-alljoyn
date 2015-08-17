/**
 * @file
 * This file defines the utility functions for PermissionPolicy.
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

#include <alljoyn/PermissionPolicyUtil.h>

#define QCC_MODULE "PERMISSION_MGMT"

using namespace std;
using namespace qcc;

namespace ajn {

bool PermissionPolicyUtil::HasValidDenyRules(PermissionPolicy& pol)
{
    const PermissionPolicy::Acl* acls = pol.GetAcls();
    for (size_t i = 0; i < pol.GetAclsSize(); i++) {

        bool deny = false; // will be set to true if any member has ActionMask == 0

        const PermissionPolicy::Rule* rules = acls[i].GetRules();
        for (size_t j = 0; j < acls[i].GetRulesSize(); j++) {

            const PermissionPolicy::Rule::Member* members = rules[j].GetMembers();
            for (size_t k = 0; k < rules[j].GetMembersSize(); k++) {

                if (members[k].GetActionMask() == 0) {
                    if (members[k].GetMemberName() != "*" || members[k].GetMemberType() !=
                        PermissionPolicy::Rule::Member::NOT_SPECIFIED) {
                        return false;
                    }

                    deny = true;
                    break;
                }
            }

            if (deny && rules[j].GetMembersSize() != 1) {
                return false;
            }
        }

        if (deny) {
            if (acls[i].GetRulesSize() != 1) {
                return false;
            }

            if (rules[0].GetObjPath() != "*" || rules[0].GetInterfaceName() != "*") {
                return false;
            }

            if (acls[i].GetPeersSize() == 0) {
                return false;
            }

            const PermissionPolicy::Peer* peers = acls[i].GetPeers();
            for (size_t j = 0; j < acls[i].GetPeersSize(); j++) {

                if (peers[j].GetType() != PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY) {
                    return false;
                }
            }
        }
    }

    return true;
}

}
