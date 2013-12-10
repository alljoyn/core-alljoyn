#ifndef _STUNATTRIBUTEMAPPEDADDRESS_H
#define _STUNATTRIBUTEMAPPEDADDRESS_H
/**
 * @file
 *
 * This file defines the MAPPED-ADDRESS STUN message attribute.
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
#error Only include StunAttributeBase.h in C++ code.
#endif

#include <string>
#include <qcc/platform.h>
#include <qcc/IPAddress.h>
#include <StunAttributeMappedAddress.h>
#include <StunAttributeBase.h>
#include <types.h>
#include <alljoyn/Status.h>

using namespace qcc;

/** @internal */
#define QCC_MODULE "STUN_ATTRIBUTE"

/**
 * Mapped Address STUN attribute base class.
 */
class StunAttributeMappedAddress : public StunAttribute {
  protected:
    IPAddress addr;   ///< Reflexive IP address.
    uint16_t port;    ///< Reflexive port number.

    /**
     * This constructor just sets the attribute type to passed in value.
     *
     * @param attrType  The STUN Attribute Type.
     * @param attrName  The STUN Attribute name in human readable form.
     */
    StunAttributeMappedAddress(StunAttrType attrType, const char* const attrName) :
        StunAttribute(attrType, attrName)
    { }

    /**
     * This constructor sets the attribute type to passed in value and
     * initializes the IP address and port.
     *
     * @param attrType  The STUN Attribute Type.
     * @param attrName  The STUN Attribute name in human readable form.
     * @param addr      IP Address.
     * @param port      IP Port.
     */
    StunAttributeMappedAddress(StunAttrType attrType, const char* const attrName,
                               const IPAddress& addr, uint16_t port) :
        StunAttribute(attrType, attrName),
        addr(addr),
        port(port)
    { }

  public:
    /**
     * This constructor just sets the attribute type to STUN_ATTR_MAPPED_ADDRESS.
     */
    StunAttributeMappedAddress(void) :
        StunAttribute(STUN_ATTR_MAPPED_ADDRESS, "MAPPED-ADDRESS")
    { }

    /**
     * This constructor just sets the attribute type to STUN_ATTR_MAPPED_ADDRESS.
     * and initializes the IP address and port.
     *
     * @param addr  IP Address.
     * @param port  IP Port.
     */
    StunAttributeMappedAddress(const IPAddress& addr, uint16_t port) :
        StunAttribute(STUN_ATTR_MAPPED_ADDRESS, "MAPPED-ADDRESS"),
        addr(addr),
        port(port)
    { }

    QStatus Parse(const uint8_t*& buf, size_t& bufSize);
    QStatus RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const;
#ifndef NDEBUG
    String ToString(void) const;
#endif
    size_t RenderSize(void) const { return Size(); }
    uint16_t AttrSize(void) const
    {
        return (sizeof(uint8_t) +   // Unused octet.
                sizeof(uint8_t) +   // Address family.
                sizeof(port) +      // Port.
                addr.Size());       // IP Address.
    }

    /**
     * Get the reflexive address and port number.
     *
     * @param addr OUT: A reference to where to copy the IP address.
     * @param port OUT: A reference to where to copy the port number.
     */
    void GetAddress(IPAddress& addr, uint16_t& port) const
    {
        addr = this->addr;
        port = this->port;
    }

    /**
     * Set the reflexive address and port number.
     *
     * @param addr A reference to the IP address.
     * @param port The port number.
     */
    void SetAddress(const IPAddress& addr, uint16_t port)
    {
        this->addr = addr;
        this->port = port;
    }
};


#undef QCC_MODULE
#endif
