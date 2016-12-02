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

#ifndef ALLJOYN_SECMGR_GROUPINFO_H_
#define ALLJOYN_SECMGR_GROUPINFO_H_

#include <string>

#include <qcc/KeyInfoECC.h>
#include <qcc/GUID.h>

using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
/**
 * @brief Represents a security group. A group has two key values: a GUID and a
 *        group authority. It also contains some additional meta information,
 *        like a name and a description.
 */
struct GroupInfo {
    /**
     * @brief The authority of this group. It is the public key of the security
     *        agent that created this group. It is part of the composite key of this
     *        object.
     */
    KeyInfoNISTP256 authority;
    /**
     * @brief The guid of this group. It is part of the composite key of this
     *        object.
     */
    GUID128 guid;
    /**
     * @brief The name of this group.
     */
    string name;
    /**
     * @brief A description for this group.
     */
    string desc;

    /**
     * @brief The equality operator for GroupInfo.
     *
     * @param[in] gi  The other GroupInfo to compare with.
     */
    bool operator==(const GroupInfo& gi) const
    {
        if (authority != gi.authority) {
            return false;
        }

        if (guid != gi.guid) {
            return false;
        }

        return true;
    }

    /**
     * @brief The inequality operator for GroupInfo.
     *
     * @param[in] gi  The other GroupInfo to compare with.
     */
    bool operator!=(const GroupInfo& gi) const
    {
        return !(*this == gi);
    }

    /**
     * @brief The less than comparator for GroupInfo.
     *
     * @param[in] gi  The other GroupInfo to compare with.
     */
    bool operator<(const GroupInfo& gi) const
    {
        if (authority < gi.authority) {
            return true;
        }

        if (guid < gi.guid) {
            return true;
        }

        return false;
    }

    /**
     * @brief Returns a string representation of this GroupInfo.
     *
     * @return  A string representation of this GroupInfo.
     */
    string ToString() const
    {
        string s = "GroupInfo:";
        s += "\n  authority: ";
        s += authority.ToString().c_str();
        s += "\n  guid: ";
        s += guid.ToString().c_str();
        s += "\n  name: " + name + "\n  desc: " + desc + "\n";
        return s;
    }
};
}
}
#endif /* ALLJOYN_SECMGR_GROUPINFO_H_ */