/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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
