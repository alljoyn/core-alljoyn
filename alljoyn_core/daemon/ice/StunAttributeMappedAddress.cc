/**
 * @file
 *
 * This file implements the STUN Attribute Generic Address base class
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

#include <string>
#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/IPAddress.h>
#include <qcc/StringUtil.h>
#include <StunAttributeMappedAddress.h>
#include <alljoyn/Status.h>

using namespace qcc;

#define QCC_MODULE "STUN_ATTRIBUTE"


enum IPFamily {
    IPV4 = 0x01,
    IPV6 = 0x02
};

static const uint16_t MIN_ATTR_SIZE = (sizeof(uint8_t) +        // Unused octet.
                                       sizeof(uint8_t) +        // Address family.
                                       sizeof(uint16_t) +       // Port.
                                       IPAddress::IPv4_SIZE);   // IPv4 address.


QStatus StunAttributeMappedAddress::Parse(const uint8_t*& buf, size_t& bufSize)
{
    QStatus status = ER_OK;
    IPFamily family;
    size_t addrLen;

    if (bufSize < MIN_ATTR_SIZE) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Parsing Mapped Address attribute"));
        goto exit;
    }

    ++buf;      // Skip unused octet.
    --bufSize;  // Maintain accounting.

    family = static_cast<IPFamily>(*buf);
    ++buf;
    --bufSize;

    ReadNetToHost(buf, bufSize, port);

    switch (family) {
    case IPV4:
        addrLen = IPAddress::IPv4_SIZE;
        break;

    case IPV6:
        addrLen = IPAddress::IPv6_SIZE;
        break;

    default:
        status = ER_STUN_INVALID_ADDR_FAMILY;
        QCC_LogError(status, ("Parsing Mapped Address attribute"));
        goto exit;
    }

    addr = IPAddress(buf, addrLen);

    buf += addrLen;;
    bufSize -= addrLen;

    status = StunAttribute::Parse(buf, bufSize);

exit:
    return status;
}

QStatus StunAttributeMappedAddress::RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
{
    QStatus status;

    status = StunAttribute::RenderBinary(buf, bufSize, sg);
    if (status != ER_OK) {
        goto exit;
    }

    WriteHostToNet(buf, bufSize, static_cast<uint8_t>(0), sg);

    switch (addr.Size()) {
    case IPAddress::IPv4_SIZE:
        WriteHostToNet(buf, bufSize, static_cast<uint8_t>(IPV4), sg);
        break;

    case IPAddress::IPv6_SIZE:
        WriteHostToNet(buf, bufSize, static_cast<uint8_t>(IPV6), sg);
        break;

    default:
        status = ER_STUN_INVALID_ADDR_FAMILY;
        QCC_LogError(status, ("Rendering %s", name));
        goto exit;
    }

    WriteHostToNet(buf, bufSize, port, sg);

    status = addr.RenderIPBinary(&buf[0], bufSize);

    if (status == ER_OK) {
        sg.AddBuffer(&buf[0], addr.Size());
        sg.IncDataSize(addr.Size());

        buf += addr.Size();
        bufSize -= addr.Size();
    }

exit:
    return status;
}

#if !defined(NDEBUG)
String StunAttributeMappedAddress::ToString(void) const
{
    String oss;

    oss.append(StunAttribute::ToString());
    oss.append(": IP Address: ");
    oss.append(addr.ToString());
    oss.append("  Port: ");
    oss.append(U32ToString(port, 10));

    return oss;
}
#endif
