#ifndef _STUNATTRIBUTEALLOCATEDXORSERVERREFLEXIVEADDRESS_H
#define _STUNATTRIBUTEALLOCATEDXORSERVERREFLEXIVEADDRESS_H
/**
 * @file
 *
 * This file defines the ALLOCATED-XOR-SERVER-REFLEXIVE-ADDRESS  STUN message attribute.
 */

/******************************************************************************
 * Copyright (c) 2012 AllSeen Alliance. All rights reserved.
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
#error Only include attr/StunAttributeAllocatedXorServerReflexiveAddress.h in C++ code.
#endif


#include <string>
#include <qcc/platform.h>
#include <StunAttributeXorMappedAddress.h>
#include <types.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"

/**
 * Allocated XOR Server Reflexive Address STUN attribute class.
 */
class StunAttributeAllocatedXorServerReflexiveAddress : public StunAttributeXorMappedAddress {
  public:
    /**
     * This constructor just sets the attribute type to STUN_ATTR_ALLOCATED_XOR_SERVER_REFLEXIVE_ADDRESS.
     *
     * @param msg Reference to the containing message.
     */
    StunAttributeAllocatedXorServerReflexiveAddress(const StunMessage& msg) :
        StunAttributeXorMappedAddress(STUN_ATTR_ALLOCATED_XOR_SERVER_REFLEXIVE_ADDRESS,
                                      "ALLOCATED-XOR-SERVER-REFLEXIVE-ADDRESS",
                                      msg)
    { }

    /**
     * This constructor just sets the attribute type to STUN_ATTR_ALLOCATED_XOR_SERVER_REFLEXIVE_ADDRESS
     * and initializes the IP address and port.
     *
     * @param msg Reference to the containing message.
     * @param addr  IP Address.
     * @param port  IP Port.
     */
    StunAttributeAllocatedXorServerReflexiveAddress(const StunMessage& msg, const IPAddress& addr, uint16_t port) :
        StunAttributeXorMappedAddress(STUN_ATTR_ALLOCATED_XOR_SERVER_REFLEXIVE_ADDRESS,
                                      "ALLOCATED-XOR-SERVER-REFLEXIVE-ADDRESS",
                                      msg, addr, port)
    { }

    static const uint16_t ATTR_SIZE = sizeof(uint8_t) +   // Unused octet.
                                      sizeof(uint8_t) +                                       // Address family.
                                      sizeof(uint16_t) +                                      // Port.
                                      sizeof(IPAddress);                                      // IP Address.

    static const uint16_t ATTR_SIZE_WITH_HEADER = ((StunAttribute::ATTR_HEADER_SIZE + ATTR_SIZE + 3) & 0xfffc);
};


#undef QCC_MODULE
#endif
