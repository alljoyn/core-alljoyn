/**
 * @file
 * This class maintains a hash of the authentication conversation, used as part
 * of secure session establishment to guarantee integrity and negotiate a shared
 * encryption key.
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

#ifndef _ALLJOYN_CONVERSATIONHASH_H
#define _ALLJOYN_CONVERSATIONHASH_H

#ifndef __cplusplus
#error Only include ConversationHash.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/Crypto.h>

namespace ajn {

class ConversationHash {

  public:

    /**
     * Constructor.
     */
    ConversationHash();

    /**
     * Initialize the hash. This must be called before any calls to Update
     * or GetDigest.
     * @return ER_OK if the hash is successfully initialized; error otherwise
     */
    QStatus Init();

    /**
     * Update the conversation hash with a single byte
     * @param[in] byte Byte with which to update the hash.
     * @return ER_OK if the hash is successfully updated; error otherwise
     */
    QStatus Update(uint8_t byte);

    /**
     * Update the conversation hash with a byte array.
     * @param[in] buf Data with which to update the hash.
     * @param[in] bufSize Size of buf.
     * @param[in] includeSizeInHash true to hash the array size as a little-endian uint32_t first; false if not
     * @return ER_OK if the hash is successfully updated; error otherwise
     */
    QStatus Update(const uint8_t* buf, size_t bufSize, bool includeSizeInHash);

    /**
     * Get the current conversation hash digest.
     * @param[out] digest A buffer of appropriate size to receive the digest. Currently
     *                    only SHA-256 is used, and so 32 bytes will be returned.
     * @param[in] keepAlive Whether or not to keep the hash alive for continuing hash.
     * @return ER_OK if the hash is successfully retrieved and placed in digest; error otherwise
     */
    QStatus GetDigest(uint8_t* digest, bool keepAlive = false);

    /**
     * Destructor.
     */
    ~ConversationHash();

    /** 
     * Enable or disable "sensitive mode," where byte arrays that get hashed aren't
     * logged verbatim. When enabled, calling the overload of Update that takes a byte array will
     * log the size of the data, but then log secret data was hashed without showing the data.
     *
     * Logging for Update for a single byte or GetDigest is unaffected.
     * @param[in] mode true to enable sensitive mode; false to disable it.
     */
    void SetSensitiveMode(bool mode);

  private:

    /**
     * The crypto primitive for the conversation hash.
     */
    qcc::Crypto_SHA256 m_hashUtil;

    /**
     * Flag to indicate sensitive data is being hashed, and not to log it verbatim.
     */
    bool m_sensitiveMode;
};

}

#endif