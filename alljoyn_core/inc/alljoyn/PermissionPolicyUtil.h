#ifndef _ALLJOYN_PERMISSION_POLICY_UTIL_H
#define _ALLJOYN_PERMISSION_POLICY_UTIL_H
/**
 * @file
 * This file defines the Permission Policy classes that provide the interface to
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

#ifndef __cplusplus
#error Only include PermissionPolicyUtil.h in C++ code.
#endif

#include <alljoyn/PermissionPolicy.h>

namespace ajn {

/**
 * Class for permission policy utility functions.
 */
class PermissionPolicyUtil {

  public:
    /*
     * Checks whether each explicit deny rule of this policy is linked to one
     * or more peers of type PEER_WITH_PUBLIC_KEY.
     *
     * Checks whether each Member that has an explicit deny ActionMask (== 0)
     * - has a MemberName which is equal to "*"
     * - has a MemberType which is NOT_SPECIFIED
     * - the Rule it is included in has only one Member
     * - the ObjPath and InterfaceName of that Rule are both equal to "*"
     * - the ACL it is included in has only one Rule
     * - the Peers of that ACL are all of type PEER_WITH_PUBLIC_KEY and
     *   have unique KeyInfo objects
     *
     * @return
     *      - true   if all deny rules in this policy are valid
     *      - false  if the policy contains at least one invalid deny rule
     */
    static bool HasValidDenyRules(PermissionPolicy& pol);
};

}
#endif
