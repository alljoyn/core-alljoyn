#ifndef _FLETCHER32_H
#define _FLETCHER32_H
/**
 * @file
 *
 * This file implements the Fletcher32 hash function.
 *
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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
#error Only include Fletcher32.h in C++ code.
#endif

#include <qcc/platform.h>

#include <alljoyn/Status.h>

namespace ajn {

/**
 * This class implements the Fletcher32 hash function
 */
class Fletcher32 {

  public:

    /**
     * Constructor
     */
    Fletcher32() : fletch1(0xFFFF), fletch2(0xFFFF) { }

    /**
     * Update the running checksum.
     *
     * @param data The data to compute the hash over.
     * @param len  The length of the data (number of uint16_t's)
     *
     * @return The current checksum value.
     */
    uint32_t Update(const uint16_t* data, size_t len) {
        while (data && len) {
            size_t l = (len <= 360) ? len : 360;
            len -= l;
            while (l--) {
                fletch1 += *data++;
                fletch2 += fletch1;
            }
            fletch1 = (fletch1 & 0xFFFF) + (fletch1 >> 16);
            fletch2 = (fletch2 & 0xFFFF) + (fletch2 >> 16);
        }
        return (fletch2 << 16) | (fletch1 & 0xFFFF);
    }

  private:

    uint32_t fletch1;
    uint32_t fletch2;
};


}

#endif
