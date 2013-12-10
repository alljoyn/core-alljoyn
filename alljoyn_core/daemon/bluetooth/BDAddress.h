/**
 * @file
 * Abstraction class for Bluetooth Device address.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
#ifndef _BDADDRESS_H
#define _BDADDRESS_H

#include <qcc/platform.h>

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

namespace ajn {

class BDAddress {
  public:
    const static size_t ADDRESS_SIZE = 6;   /**< BT addresses are 6 octets in size. */

    /**
     * Default Constructor - initializes the BD Address to 00:00:00:00:00:00.
     */
    BDAddress() : buf(0ULL), separator(255) { }

    /**
     * Copy Constructor
     *
     * @param other     BD Address to copy from.
     */
    BDAddress(const BDAddress& other) : buf(other.buf & 0xffffffffffffULL), separator(255) { }

    /**
     * Constructor that initializes the BD Address from a string in one of the
     * following forms:
     * - 123456789abc
     * - 12.34.56.78.9a.bc
     * - 12:34:56:78:9a:bc
     *
     * @param addr  BD address in a string.
     */
    BDAddress(const qcc::String& addr) : separator(255) {
        if (FromString(addr) != ER_OK) {
            // Failed to parse the string.  Gotta intialize to something...
            buf = 0ULL;
        }
    }

    /**
     * Constructor that initializes the BD Address from an array of bytes.
     *
     * @param addr          An array of 6 bytes that contains the BD Address.
     * @param littleEndian  [Optional] Flag to indicate if bytes are arranged in
     *                      little-endian (BlueZ) order (default is no).
     */
    BDAddress(const uint8_t* addr, bool littleEndian = false) : separator(255) {
        this->CopyFrom(addr, littleEndian);
    }

    /**
     * Constructor that initializes the BD Address from a uint64_t.
     *
     * @param addr  A uint64_t containing the BD Address in the lower 48 bits.
     */
    BDAddress(const uint64_t& addr) : buf(addr & 0xffffffffffffULL), separator(255) { }

    /**
     * Function to set the BD Address from an array of bytes.
     *
     * @param addr          An array of 6 bytes that contains the BD Address.
     * @param littleEndian  [Optional] Flag to indicate if bytes are arranged in
     *                      little-endian (BlueZ) order (default is no).
     */
    void CopyFrom(const uint8_t* addr, bool littleEndian = false) {
        if (littleEndian) {
            buf = ((uint64_t)letoh32(*(uint32_t*)&addr[2]) << 16) | (uint64_t)letoh16(*(uint16_t*)&addr[0]);
        } else {
            buf = ((uint64_t)betoh32(*(uint32_t*)&addr[0]) << 16) | (uint64_t)betoh16(*(uint16_t*)&addr[4]);
        }
        separator = 255;
    }

    /**
     * Function write the BD Address to an array of bytes.
     *
     * @param addr          [Out] An array of 6 bytes for writing the BD Address.
     * @param littleEndian  [Optional] Flag to indicate if bytes are arranged in
     *                      little-endian (BlueZ) order (default is no).
     */
    void CopyTo(uint8_t* addr, bool littleEndian = false) const {
        if (littleEndian) {
            *(uint32_t*)&addr[0] = htole32((uint32_t)buf & 0xffffffff);
            *(uint16_t*)&addr[4] = htole16((uint16_t)(buf >> 32) & 0xffff);
        } else {
            *(uint32_t*)&addr[0] = htobe32((uint32_t)(buf >> 16) & 0xffffffff);
            *(uint16_t*)&addr[4] = htobe16((uint16_t)buf & 0xffff);
        }
    }

    /**
     * Function to represent the BD Address in a string.  By default, the
     * parts of the address are separated by a ":", this may be changed by
     * setting the separator parameter.
     *
     * @param separator     [Optional] Separator character to use when generating the string.
     *
     * @return  A string representation of the BD Address.
     */
    const qcc::String& ToString(char separator = ':') const {
        /* Need to regenerate the string if a different separator is specified
         * that the last time the cache was generated.
         */
        if (separator != this->separator) {
            /* Humans accustomed to reading left-to-right script tend to
             * prefer bytes to be in big endian order so that is the
             * convention used for string representations. */
            const uint64_t be = htobe64(buf);
            cache = qcc::BytesToHexString(((const uint8_t*)&be) + 2, 6, true, separator);
            this->separator = separator;
        }
        return cache;
    }

    /**
     * Function to set the BD Address from a string in one of the
     * following forms:
     * - 123456789abc
     * - 12.34.56.78.9a.bc
     * - 12:34:56:78:9a:bc
     *
     * @param addr  BD address in a string.
     *
     * @return  Indication of success or failure.
     */
    QStatus FromString(const qcc::String addr) {
        uint64_t be;
        if ((HexStringToBytes(addr, ((uint8_t*)&be) + 2, ADDRESS_SIZE) != ADDRESS_SIZE) &&
            (HexStringToBytes(addr, ((uint8_t*)&be) + 2, ADDRESS_SIZE, '.') != ADDRESS_SIZE) &&
            (HexStringToBytes(addr, ((uint8_t*)&be) + 2, ADDRESS_SIZE, ':') != ADDRESS_SIZE)) {
            return ER_FAIL;
        }
        buf = betoh64(be) & 0xffffffffffffULL;
        return ER_OK;
    }

    /**
     * Get the BDAddress in raw form as a uint64_t value.
     */
    uint64_t GetRaw() const { return buf; }

    /**
     * Set the BDAddress from a raw uint64_t value - only lower 48 bits used.
     */
    void SetRaw(uint64_t addr) { buf = addr & 0xffffffffffffULL; }

    /**
     * Assignment operator.
     *
     * @param other     Reference to BD Address to copy from.
     *
     * @return  Reference to *this.
     */
    BDAddress& operator=(const BDAddress& other) {
        buf = other.buf & 0xffffffffffffULL;
        separator = 255;
        return *this;
    }

    /**
     * Test if 2 BD Address are the same.
     *
     * @param other     The other BD Address used for comparison.
     *
     * @return  True if they are the same, false otherwise.
     */
    bool operator==(const BDAddress& other) const {
        return (buf == other.buf);
    }

    /**
     * Test if 2 BD Address are different.
     *
     * @param other      The other BD Address used for comparison.
     *
     * @return  True if they are the same, false otherwise.
     */
    bool operator!=(const BDAddress& other) const {
        return (buf != other.buf);
    }

    /**
     * Test if *this BD Address is less than the other BD Address.
     *
     * @param other      The other BD Address used for comparison.
     *
     * @return  True if *this < other, false otherwise.
     */
    bool operator<(const BDAddress& other) const { return buf < other.buf; }

    /**
     * Test if *this BD Address is greater than the other BD Address.
     *
     * @param other      The other BD Address used for comparison.
     *
     * @return  True if *this > other, false otherwise.
     */
    bool operator>(const BDAddress& other) const { return buf > other.buf; }

  private:

    uint64_t buf;               /**< BD Address storage. */

    mutable qcc::String cache;  /**< Cache storage for the string representation.  Make usage of BDAddress::ToString() easier. */
    mutable char separator;     /**< Used to check if cache is valid. */
};

}

#endif
