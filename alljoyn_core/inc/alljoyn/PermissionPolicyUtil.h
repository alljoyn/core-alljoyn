#ifndef _ALLJOYN_PERMISSION_POLICY_UTIL_H
#define _ALLJOYN_PERMISSION_POLICY_UTIL_H
/**
 * @file
 * This file defines the Permission Policy classes that provide the interface to
 * parse the authorization data
 */

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