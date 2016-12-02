#ifndef ALLJOYN_SECMGR_POLICYUTIL_H_
#define ALLJOYN_SECMGR_POLICYUTIL_H_

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

#include <alljoyn/PermissionPolicy.h>

namespace ajn {
namespace securitymgr {
/**
 * @brief Utilities for PermissionPolicies.
 */
class PolicyUtil {
  public:

    /**
     * @brief Normalizes a permission policy by making sure each InterfaceName, ObjPath, MemberName
     *        and MemberType appears only once in each ACL. The action masks for matching rules are
     *        ORed together. Additionally, this method orders the rules and members for each ACL
     *        alphabetically.
     *
     * @param[in,out] policy   In, the policy that needs to be normalized.
     *                         Out, the normalized policy. For each ACl, the Rules will be
     *                         normalizes and the Peers will remain identical.
     */
    static void NormalizePolicy(PermissionPolicy& policy);
};
}
}

#endif  /* ALLJOYN_SECMGR_POLICYUTIL_H_ */