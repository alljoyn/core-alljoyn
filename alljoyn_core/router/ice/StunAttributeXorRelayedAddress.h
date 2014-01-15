#ifndef _STUNATTRIBUTEXORRELAYEDADDRESS_H
#define _STUNATTRIBUTEXORRELAYEDADDRESS_H
/**
 * @file
 *
 * This file defines the XOR-RELAYED-ADDRESS STUN message attribute.
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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
#error Only include StunAddr/StunAttributeXorRelayedAddress.h in C++ code.
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
 * XOR Relayed Address STUN attribute class.
 */
class StunAttributeXorRelayedAddress : public StunAttributeXorMappedAddress {
  public:
    /**
     * This constructor just sets the attribute type to
     * STUN_ATTR_XOR_RELAYED_ADDRESS.
     *
     * @param msg Reference to the containing message.
     */
    StunAttributeXorRelayedAddress(const StunMessage& msg) :
        StunAttributeXorMappedAddress(STUN_ATTR_XOR_RELAYED_ADDRESS,
                                      "XOR_RELAYED_ADDRESS",
                                      msg)
    { }

    /**
     * This constructor just sets the attribute type to
     * STUN_ATTR_XOR_RELAYED_ADDRESS and initializes the IP address and port.
     *
     * @param msg Reference to the containing message.
     *
     * @param addr  IP Address.
     * @param port  IP Port.
     */
    StunAttributeXorRelayedAddress(const StunMessage& msg, const IPAddress& addr, uint16_t port) :
        StunAttributeXorMappedAddress(STUN_ATTR_XOR_RELAYED_ADDRESS,
                                      "XOR_RELAYED_ADDRESS",
                                      msg, addr, port)
    { }
};


#undef QCC_MODULE
#endif
