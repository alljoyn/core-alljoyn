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

#ifndef SECMGR_PUBKEY_H_
#define SECMGR_PUBKEY_H_

#include <qcc/CryptoECC.h>
#include <qcc/String.h>

#include <alljoyn/Message.h>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
using namespace qcc;

struct PublicKey :
    ECCPublicKey {
    PublicKey(const ECCPublicKey* eccPublicKey = NULL) :
        ECCPublicKey()
    {
        if (!eccPublicKey) {
            memset(x, 0, ECC_COORDINATE_SZ);
            memset(y, 0, ECC_COORDINATE_SZ);
        } else {
            memcpy(x, eccPublicKey->x, ECC_COORDINATE_SZ);
            memcpy(y, eccPublicKey->y, ECC_COORDINATE_SZ);
        }
    }

    void operator=(const PublicKey& k)
    {
        memcpy(x, k.x, ECC_COORDINATE_SZ);
        memcpy(y, k.y, ECC_COORDINATE_SZ);
    }

    bool operator==(const ECCPublicKey& k) const
    {
        int n = memcmp(x, k.x, ECC_COORDINATE_SZ);
        return (n == 0) && (0 == memcmp(y, k.y, ECC_COORDINATE_SZ));
    }

    bool operator!=(const ECCPublicKey& k) const
    {
        int n = memcmp(x, k.x, ECC_COORDINATE_SZ);
        return (n != 0) || (0 != memcmp(y, k.y, ECC_COORDINATE_SZ));
    }

    bool operator<(const ECCPublicKey& k) const
    {
        int n = memcmp(x, k.x, ECC_COORDINATE_SZ);
        if (n == 0) {
            n = memcmp(y, k.y, ECC_COORDINATE_SZ);
        }
        return (n < 0);
    }

    const qcc::String ToString() const
    {
        qcc::String s = "x=[";
        qcc::String yString;
        for (int i = 0; i < (int)ECC_COORDINATE_SZ; ++i) {
            char buff[4];
            sprintf(buff, "%02x", (unsigned char)(x[i]));
            s.append(buff);
            sprintf(buff, "%02x", (unsigned char)(y[i]));
            yString.append(buff);
        }
        s.append("], y=[");
        s.append(yString);
        s.append("]");
        return s;
    }

    QStatus SetData(size_t xLen, uint8_t* xCoord, size_t yLen, uint8_t* yCoord)
    {
        if (xLen == ECC_COORDINATE_SZ && yLen == ECC_COORDINATE_SZ) {
            memcpy(x, xCoord, ECC_COORDINATE_SZ);
            memcpy(y, yCoord, ECC_COORDINATE_SZ);
        } else {
            QCC_LogError(ER_FAIL, ("Wrong data size."));
            return ER_FAIL;
        }
        return ER_OK;
    }

    size_t GetStorablePubKey(uint8_t(&data)[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ]) const
    {
        size_t combinedSize = qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ;
        uint8_t combined[combinedSize];
        memcpy(combined, x, qcc::ECC_COORDINATE_SZ);
        memcpy(combined + qcc::ECC_COORDINATE_SZ, y, qcc::ECC_COORDINATE_SZ);
        memcpy(data, combined, combinedSize);
        return combinedSize;
    }

    QStatus SetPubKeyFromStorage(const uint8_t* data, size_t size)
    {
        if ((size != (qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ)) || (!data)) {
            return ER_FAIL;
        }

        uint8_t x[qcc::ECC_COORDINATE_SZ];
        uint8_t y[qcc::ECC_COORDINATE_SZ];

        memcpy(x, data, qcc::ECC_COORDINATE_SZ);
        memcpy(y, data + qcc::ECC_COORDINATE_SZ, qcc::ECC_COORDINATE_SZ);

        return SetData(qcc::ECC_COORDINATE_SZ, x, qcc::ECC_COORDINATE_SZ, y);
    }
};
}
}
#undef QCC_MODULE
#endif
