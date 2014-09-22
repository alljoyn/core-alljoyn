#ifndef SECMGR_PUBKEY_H_
#define SECMGR_PUBKEY_H_

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <qcc/CryptoECC.h>
#include <qcc/String.h>

#include <alljoyn/Message.h>

namespace ajn {
namespace securitymgr {
struct PublicKey :
    qcc::ECCPublicKey {
    bool operator==(const PublicKey& k) const
    {
        int n = memcmp(data, k.data, qcc::ECC_PUBLIC_KEY_SZ);
        return (n == 0);
    }

    bool operator!=(const PublicKey& k) const
    {
        int n = memcmp(data, k.data, qcc::ECC_PUBLIC_KEY_SZ);
        return (n != 0);
    }

    bool operator<(const PublicKey& k) const
    {
        int n = memcmp(data, k.data, qcc::ECC_PUBLIC_KEY_SZ);
        if (n < 0) {
            return true;
        } else {
            return false;
        }
    }

    const qcc::String ToString() const
    {
        qcc::String s;
        for (int i = 0; i < (int)qcc::ECC_PUBLIC_KEY_SZ; ++i) {
            char buff[4];
            sprintf(buff, "%02x", (unsigned char)(data[i]));
            s.append(buff);
        }
        return s;
    }

    void SetData(const ajn::AllJoynScalarArray bytes)
    {
        if (bytes.numElements == qcc::ECC_PUBLIC_KEY_SZ) {
            memcpy(data, bytes.v_byte, qcc::ECC_PUBLIC_KEY_SZ);
        }
    }
};
}
}

#endif
