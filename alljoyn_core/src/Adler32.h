#ifndef _ADLER32_H
#define _ADLER32_H
/**
 * @file
 *
 * This file implements the Adler32 hash function.
 *
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014, AllSeen Alliance. All rights reserved.
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
#error Only include Adler32.h in C++ code.
#endif

#include <qcc/platform.h>

#include <alljoyn/Status.h>

namespace ajn {

/**
 * This class implements the Adler32 hash function
 */
class Adler32 {

  public:

    /**
     * Constructor
     */
    Adler32() : adler(1) { }

    /**
     * Update the running hash.
     *
     * @param data The data to compute the hash over.
     * @param len  The length of the data
     *
     * @return The current hash value.
     */
    uint32_t Update(const uint8_t* data, size_t len) {
        if (data) {
            while (len) {
                size_t l = (len < 4095) ? len : 4095; // Max number of iterations before modulus must be computed
                uint32_t a = adler & 0xFFFF;
                uint32_t b = adler >> 16;
                len -= l;
                while (l--) {
                    a += *data++;
                    b += a;
                }
                adler = ((b % ADLER_PRIME) << 16) | (a % ADLER_PRIME);
            }
        }
        return adler;
    }

  private:

    /**
     * The largest prime number that will fit in 16 bits
     */
    static const uint32_t ADLER_PRIME = 65521;

    /**
     * The running hash value.
     */
    uint32_t adler;
};

}

#endif
