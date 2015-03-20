#ifndef _QCC_GUID_H
#define _QCC_GUID_H
/**
 * @file
 *
 * This file defines the a class for creating 128 bit GUIDs
 *
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

#ifndef __cplusplus
#error Only include GUID128.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>

#include <string.h>

/**
 * GUIDs for local keys.
 */
#define GUID_KEYBLOB_PRIVATE "a62655061e8295e2462794065f2a1c95"
#define GUID_KEYBLOB_AES     "b4dc47954ce6e94f6669f31b343b91d8"
#define GUID_KEYBLOB_PEM     "29ebe36c0ac308c8eb808cfdf1f36953"
#define GUID_KEYBLOB_PUBLIC  "48b020fc3a65c6bc5ac22b949a869dab"
#define GUID_KEYBLOB_SPKI_CERT "9ddf8d784fef4b57d5103e3bef656067"
#define GUID_KEYBLOB_DSA_PRIVATE "d1b60ce37ba71ea4b870d73b6cd676f5"
#define GUID_KEYBLOB_DSA_PUBLIC "19409269762da560d7812cb8a542f024"

/**
 * GUID for storing and loading a self-signed cert.
 */
#define GUID_AUTHMECHRSA_SELF_CERT_GUID "9D689C804B9C47C1ADA7397AE0215B26"
#define GUID_AUTHMECHRSA_SELF_PRIV_GUID "B125ABEF3724453899E04B6B1D5C2CC4"

namespace qcc {

class GUID128 {

  public:

    /**
     * Size of a GUID128 in bytes
     */
    static const size_t SIZE = 16;

    /**
     * Size of string returned by ToShortString() in bytes.
     */
    static const size_t SIZE_SHORT = 8;

    /**
     * Compare two GUIDs for equality
     */
    bool operator==(const GUID128& other) const { return memcmp(other.guid, guid, SIZE) == 0; }

    /**
     * Compare two GUIDs
     */
    bool operator<(const GUID128& other) const { return memcmp(other.guid, guid, SIZE) < 0; }

    /**
     * Compare two GUIDs for non-equality
     */
    bool operator!=(const GUID128& other) const { return memcmp(other.guid, guid, SIZE) != 0; }

    /**
     * Compare a GUID with a string (case insensitive)
     *
     * @param other   The other GUID to compare with
     */
    bool Compare(const qcc::String& other);

    /**
     * Returns true if the string is a guid or starts with a guid
     *
     * @param  str      The string to check
     * @param exactLen  If true the string must be the exact length for a GUID128 otherwise only check
     *                  that the string starts with a GUID128
     *
     * @return  true if the string is a guid
     */
    static bool IsGUID(const qcc::String& str, bool exactLen = true);

    /**
     * Returns string representation of a GUID128
     */
    const qcc::String& ToString() const;

    /**
     * Returns a shortened and compressed representation of a GUID128.
     * The result string is composed of the following characters:
     *
     *    [0-9][A-Z][a-z]-
     *
     * These 64 characters (6 bits) are stored in an 8 string. This gives
     * a 48 string that is generated uniquely from the original 128-bit GUID value.
     * The mapping of GUID128 to "shortened string" is therefore many to one.
     *
     * This representation does NOT have the full 128 bits of randomness
     */
    const qcc::String& ToShortString() const;

    /**
     * GUID128 constructor - intializes GUID with a random number
     */
    GUID128();

    /**
     * GUID128 constructor - fills GUID with specified value.
     */
    GUID128(uint8_t init);

    /**
     * GUID128 constructor - intializes GUID from a hex encoded string
     */
    GUID128(const qcc::String& hexStr);

    /**
     * Assignment operator
     */
    GUID128& operator =(const GUID128& other);

    /**
     * Copy constructor
     */
    GUID128(const GUID128& other);

    /**
     * Render a GUID as an array of bytes
     */
    uint8_t* Render(uint8_t* data, size_t len) const;

    /**
     * Render a GUID as a byte string.
     */
    const qcc::String RenderByteString() const { return qcc::String((char*)guid, (size_t)SIZE); }

    /**
     * Set the GUID raw bytes.
     *
     * @param rawBytes  Pointer to 16 raw (binary) bytes for guid
     */
    void SetBytes(const uint8_t* buf);

    /**
     * Get the GUID raw bytes.
     *
     * @return   Pointer to GUID128::SIZE bytes that make up this guid value.
     */
    const uint8_t* GetBytes() const { return guid; }

    /**
     * Determine if this guid is one of our special, protected guids.
     * Use this when receiving a guid from a remote peer, as a remote peer should
     * never legitimately try to use one of these.
     *
     * @return    true if guid is a protected guid, false if not
     */
    bool IsProtectedGuid() const;

  private:

    uint8_t guid[SIZE];
    mutable qcc::String value;
    mutable qcc::String shortValue;

    static const GUID128 ProtectedGuids[];
};

}  /* namespace qcc */

#endif
