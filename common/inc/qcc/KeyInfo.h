#ifndef _QCC_KEYINFO_H
#define _QCC_KEYINFO_H
/**
 * @file
 *
 * This file provide public key info
 */

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

#include <qcc/platform.h>

namespace qcc {

/**
 * KeyInfo
 */
class KeyInfo {

  public:

    /**
     * KeyInfo format
     */
    typedef enum {
        FORMAT_ALLJOYN = 0,   ///< AllJoyn format
        FORMAT_JWK = 1,   ///< JSON Web Key format
        FORMAT_X509 = 2,   ///< X.509 format
    } FormatType;

    /**
     * Key usage
     */
    typedef enum {
        USAGE_SIGNING = 0,   ///< key is used for signing
        USAGE_ENCRYPTION = 1   ///< key is used for encryption
    } KeyUsageType;

    /**
     * Default constructor.
     */
    KeyInfo(FormatType format) : format(format), keyId(NULL), keyIdLen(0)
    {
    }

    /**
     * Default destructor.
     */
    virtual ~KeyInfo()
    {
        delete [] keyId;
    }

    /**
     * Assign the key id
     * @param keyID the key id to copy
     * @param len the key len
     */
    void SetKeyId(const uint8_t* keyID, size_t len)
    {
        delete [] keyId;
        keyId = NULL;
        keyIdLen = 0;
        if (len == 0) {
            return;
        }
        keyId = new uint8_t[len];
        if (keyId == NULL) {
            return;
        }
        keyIdLen = len;
        memcpy(keyId, keyID, keyIdLen);
    }

    /**
     * Retrieve the key ID.
     * @return  the key ID.  It's a pointer to an internal buffer. Its lifetime is the same as the object's lifetime.
     */
    const uint8_t* GetKeyId()
    {
        return keyId;
    }

    /**
     * Retrieve the key ID length.
     * @return  the key ID length.
     */
    const size_t GetKeyIdLen()
    {
        return keyIdLen;
    }

  private:
    FormatType format;
    uint8_t* keyId;
    size_t keyIdLen;
};

} /* namespace qcc */


#endif
