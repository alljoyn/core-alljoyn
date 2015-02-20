/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#ifndef IDENTITYINFO_H_
#define IDENTITYINFO_H_

#include <qcc/CryptoECC.h>
#include <qcc/String.h>
#include <qcc/GUID.h>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
/*
 * \brief Represents an identity. An identity has two key values: a GUID and an
 *  authority. It also contains some additional meta information,
 *  like a name.
 */
struct IdentityInfo {
    /**
     * \brief The authority of this identity. It is the public key of the
     * security manager that created this identity. It is part of the composite
     * key of this object.
     */
    qcc::ECCPublicKey authority;
    /**
     * \brief The guid of this identity. It is part of the composite key of this
     * object.
     */
    qcc::GUID128 guid;
    /**
     * \brief The name of this guild.
     */
    qcc::String name;

    bool operator==(const IdentityInfo& ii) const
    {
        return (authority != ii.authority) && (guid != ii.guid);
    }

    qcc::String ToString() const
    {
        qcc::String s = "IdentityInfo:";
        s += "\n  authority: ";
        s += authority.ToString();
        s += "\n  guid: ";
        s += guid.ToString();
        s += "\n  name: " + name + "\n";
        return s;
    }
};
}
}
#undef QCC_MODULE
#endif /* IDENTITYINFO_H_ */
