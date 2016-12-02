#ifndef _ADLER32_H
#define _ADLER32_H
/**
 * @file
 *
 * This file implements the Adler32 hash function.
 *
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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