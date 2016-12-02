#ifndef _FLETCHER32_H
#define _FLETCHER32_H
/**
 * @file
 *
 * This file implements the Fletcher32 hash function.
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