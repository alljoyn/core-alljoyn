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

#ifndef ALLJOYN_SECMGR_IDENTITYINFO_H_
#define ALLJOYN_SECMGR_IDENTITYINFO_H_

#include <string>

#include <qcc/CryptoECC.h>
#include <qcc/GUID.h>

using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
/*
 * @brief Represents an identity. An identity has two key values: a GUID and an
 *        authority. It also contains some additional meta information,
 *        like a name.
 */
struct IdentityInfo {
    /**
     * @brief The authority of this identity. It is the public key info of the
     *        security agent that created this identity. It is part of the composite
     *       key of this object.
     */
    KeyInfoNISTP256 authority;
    /**
     * @brief The guid of this identity. It is part of the composite key of this
     * object.
     */
    GUID128 guid;
    /**
     * @brief The name of this identity.
     */
    string name;

    /**
     * @brief The equality operator for IdentityInfo.
     *
     * @param[in] ii  The other IdentityInfo to compare with.
     */
    bool operator==(const IdentityInfo& ii) const
    {
        return (authority == ii.authority) && (guid == ii.guid);
    }

    /**
     * @brief The inequality operator for IdentityInfo.
     *
     * @param[in] ii  The other IdentityInfo to compare with.
     */
    bool operator!=(const IdentityInfo& ii) const
    {
        return !(*this == ii);
    }

    /**
     * @brief The less than comparator for IdentityInfo.
     *
     * @param[in] ii  The other IdentityInfo to compare with.
     */
    bool operator<(const IdentityInfo& ii) const
    {
        return (authority < ii.authority) || (guid < ii.guid);
    }

    /**
     * @brief Returns a string representation of this IdentityInfo.
     *
     * @return  A string representation of this IdentityInfo.
     */
    string ToString() const
    {
        string s = "IdentityInfo:";
        s += "\n  authority: ";
        s += authority.ToString().c_str();
        s += "\n  guid: ";
        s += guid.ToString().c_str();
        s += "\n  name: " + name + "\n";
        return s;
    }
};
}
}
#endif /* ALLJOYN_SECMGR_IDENTITYINFO_H_ */