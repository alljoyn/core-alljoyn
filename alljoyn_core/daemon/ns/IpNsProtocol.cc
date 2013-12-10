/**
 * @file
 * The simple name service protocol implementation
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

#include <assert.h>
#include <qcc/Debug.h>
#include <qcc/SocketTypes.h>
#include <qcc/IPAddress.h>

#include "IpNsProtocol.h"

#define QCC_MODULE "NS"

//
// Strangely, Android doesn't define the IPV4 presentation format string length
// even though it does define the IPv6 version.
//
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

using namespace std;
using namespace qcc;

namespace ajn {

StringData::StringData()
    : m_size(0)
{
}

StringData::~StringData()
{
}

void StringData::Set(qcc::String string)
{
    m_size = string.size();
    m_string = string;
}

qcc::String StringData::Get(void) const
{
    return m_string;
}

size_t StringData::GetSerializedSize(void) const
{
    return 1 + m_size;
}

size_t StringData::Serialize(uint8_t* buffer) const
{
    QCC_DbgPrintf(("StringData::Serialize(): %s to buffer 0x%x", m_string.c_str(), buffer));
    assert(m_size == m_string.size());
    buffer[0] = static_cast<uint8_t>(m_size);
    memcpy(reinterpret_cast<void*>(&buffer[1]), const_cast<void*>(reinterpret_cast<const void*>(m_string.c_str())), m_size);

    return 1 + m_size;
}

size_t StringData::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    QCC_DbgPrintf(("StringData::Deserialize()"));

    //
    // If there's not enough data in the buffer to even get the string size out
    // then bail.
    //
    if (bufsize < 1) {
        QCC_DbgPrintf(("StringData::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }

    m_size = buffer[0];
    --bufsize;

    //
    // If there's not enough data in the buffer then bail.
    //
    if (bufsize < m_size) {
        QCC_DbgPrintf(("StringData::Deserialize(): Insufficient bufsize %d", bufsize));
        m_size = 0;
        return 0;
    }
    if (m_size > 0) {
        m_string.assign(reinterpret_cast<const char*>(buffer + 1), m_size);
    } else {
        m_string.clear();
    }
    QCC_DbgPrintf(("StringData::Deserialize(): %s from buffer", m_string.c_str()));
    return 1 + m_size;
}

IsAt::IsAt()
    : m_version(0), m_flagG(false), m_flagC(false),
    m_flagT(false), m_flagU(false), m_flagS(false), m_flagF(false),
    m_flagR4(false), m_flagU4(false), m_flagR6(false), m_flagU6(false),
    m_port(0),
    m_reliableIPv4Port(0), m_unreliableIPv4Port(0), m_reliableIPv6Port(0), m_unreliableIPv6Port(0)
{
}

IsAt::~IsAt()
{
}

void IsAt::SetGuid(const qcc::String& guid)
{
    m_guid = guid;
    m_flagG = true;
}

qcc::String IsAt::GetGuid(void) const
{
    return m_guid;
}

void IsAt::SetPort(uint16_t port)
{
    m_port = port;
}

uint16_t IsAt::GetPort(void) const
{
    return m_port;
}

void IsAt::ClearIPv4(void)
{
    m_ipv4.clear();
    m_flagF = false;
}

void IsAt::SetIPv4(qcc::String ipv4)
{
    m_ipv4 = ipv4;
    m_flagF = true;
}

qcc::String IsAt::GetIPv4(void) const
{
    return m_ipv4;
}

void IsAt::ClearIPv6(void)
{
    m_ipv6.clear();
    m_flagS = false;
}

void IsAt::SetIPv6(qcc::String ipv6)
{
    m_ipv6 = ipv6;
    m_flagS = true;
}

qcc::String IsAt::GetIPv6(void) const
{
    return m_ipv6;
}

void IsAt::ClearReliableIPv4(void)
{
    m_reliableIPv4Address.clear();
    m_reliableIPv4Port = 0;
    m_flagR4 = false;
}

void IsAt::SetReliableIPv4(qcc::String addr, uint16_t port)
{
    m_reliableIPv4Address = addr;
    m_reliableIPv4Port = port;
    m_flagR4 = true;
}

qcc::String IsAt::GetReliableIPv4Address(void) const
{
    return m_reliableIPv4Address;
}

uint16_t IsAt::GetReliableIPv4Port(void) const
{
    return m_reliableIPv4Port;
}

void IsAt::ClearUnreliableIPv4(void)
{
    m_unreliableIPv4Address.clear();
    m_unreliableIPv4Port = 0;
    m_flagU4 = false;
}

void IsAt::SetUnreliableIPv4(qcc::String addr, uint16_t port)
{
    m_unreliableIPv4Address = addr;
    m_unreliableIPv4Port = port;
    m_flagU4 = true;
}

qcc::String IsAt::GetUnreliableIPv4Address(void) const
{
    return m_unreliableIPv4Address;
}

uint16_t IsAt::GetUnreliableIPv4Port(void) const
{
    return m_unreliableIPv4Port;
}

void IsAt::ClearReliableIPv6(void)
{
    m_reliableIPv6Address.clear();
    m_reliableIPv6Port = 0;
    m_flagR6 = false;
}

void IsAt::SetReliableIPv6(qcc::String addr, uint16_t port)
{
    m_reliableIPv6Address = addr;
    m_reliableIPv6Port = port;
    m_flagR6 = true;
}

qcc::String IsAt::GetReliableIPv6Address(void) const
{
    return m_reliableIPv6Address;
}

uint16_t IsAt::GetReliableIPv6Port(void) const
{
    return m_reliableIPv6Port;
}

void IsAt::ClearUnreliableIPv6(void)
{
    m_unreliableIPv6Address.clear();
    m_unreliableIPv6Port = 0;
    m_flagU6 = false;
}

void IsAt::SetUnreliableIPv6(qcc::String addr, uint16_t port)
{
    m_unreliableIPv6Address = addr;
    m_unreliableIPv6Port = port;
    m_flagU6 = true;
}

qcc::String IsAt::GetUnreliableIPv6Address(void) const
{
    return m_unreliableIPv6Address;
}

uint16_t IsAt::GetUnreliableIPv6Port(void) const
{
    return m_unreliableIPv6Port;
}

void IsAt::Reset(void)
{
    m_names.clear();
}

void IsAt::AddName(qcc::String name)
{
    m_names.push_back(name);
}

uint32_t IsAt::GetNumberNames(void) const
{
    return m_names.size();
}

qcc::String IsAt::GetName(uint32_t index) const
{
    assert(index < m_names.size());
    return m_names[index];
}

size_t IsAt::GetSerializedSize(void) const
{
    size_t size;

    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    switch (m_version & 0xf) {
    case 0:
        //
        // We have one octet for type and flags, one octet for count and
        // two octets for port.  Four octets to start.
        //
        size = 4;

        //
        // If the F bit is set, we are going to include an IPv4 address
        // which is 32 bits long;
        //
        if (m_flagF) {
            size += 32 / 8;
        }

        //
        // If the S bit is set, we are going to include an IPv6 address
        // which is 128 bits long;
        //
        if (m_flagS) {
            size += 128 / 8;
        }

        //
        // Let the string data decide for themselves how long the rest of the
        // message will be.  The rest of the message will be a possible GUID
        // string and the names.
        //
        if (m_flagG) {
            StringData s;
            s.Set(m_guid);
            size += s.GetSerializedSize();
        }

        for (uint32_t i = 0; i < m_names.size(); ++i) {
            StringData s;
            s.Set(m_names[i]);
            size += s.GetSerializedSize();
        }
        break;

    case 1:
        //
        // We have one octet for type and flags, one octet for count and
        // two octets for the transport mask.  Four octets to start.
        //
        size = 4;

        //
        // If the R4 bit is set, we are going to include an IPv4 address
        // which is 32 bits long and a port which is 16 bits long.
        //
        if (m_flagR4) {
            size += 6;
        }

        //
        // If the U4 bit is set, we are going to include an IPv4 address
        // which is 32 bits long and a port which is 16 bits long.
        //
        if (m_flagU4) {
            size += 6;
        }

        //
        // If the R6 bit is set, we are going to include an IPv6 address
        // which is 128 bits long and a port which is 16 bits long.
        //
        if (m_flagR6) {
            size += 18;
        }

        //
        // If the U6 bit is set, we are going to include an IPv6 address
        // which is 32 bits long and a port which is 16 bits long.
        //
        if (m_flagU6) {
            size += 18;
        }

        //
        // Let the string data decide for themselves how long the rest of the
        // message will be.  The rest of the message will be a possible GUID
        // string and the names.
        //
        if (m_flagG) {
            StringData s;
            s.Set(m_guid);
            size += s.GetSerializedSize();
        }

        for (uint32_t i = 0; i < m_names.size(); ++i) {
            StringData s;
            s.Set(m_names[i]);
            size += s.GetSerializedSize();
        }
        break;

    default:
        assert(false && "IsAt::GetSerializedSize(): Unexpected version");
        break;
    }

    return size;
}

size_t IsAt::Serialize(uint8_t* buffer) const
{
    QCC_DbgPrintf(("IsAt::Serialize(): to buffer 0x%x", buffer));

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    uint8_t typeAndFlags;
    uint8_t* p = NULL;

    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    switch (m_version & 0xf) {
    case 0:
        //
        // The first octet is type (M = 1) and flags.
        //
        typeAndFlags = 1 << 6;

        if (m_flagG) {
            QCC_DbgPrintf(("IsAt::Serialize(): G flag"));
            typeAndFlags |= 0x20;
        }
        if (m_flagC) {
            QCC_DbgPrintf(("IsAt::Serialize(): C flag"));
            typeAndFlags |= 0x10;
        }
        if (m_flagT) {
            QCC_DbgPrintf(("IsAt::Serialize(): T flag"));
            typeAndFlags |= 0x8;
        }
        if (m_flagU) {
            QCC_DbgPrintf(("IsAt::Serialize(): U flag"));
            typeAndFlags |= 0x4;
        }
        if (m_flagS) {
            QCC_DbgPrintf(("IsAt::Serialize(): S flag"));
            typeAndFlags |= 0x2;
        }
        if (m_flagF) {
            QCC_DbgPrintf(("IsAt::Serialize(): F flag"));
            typeAndFlags |= 0x1;
        }

        buffer[0] = typeAndFlags;
        size += 1;

        //
        // The second octet is the count of bus names.
        //
        assert(m_names.size() < 256);
        buffer[1] = static_cast<uint8_t>(m_names.size());
        QCC_DbgPrintf(("IsAt::Serialize(): Count %d", m_names.size()));
        size += 1;

        //
        // The following two octets are the port number in network byte
        // order (big endian, or most significant byte first).
        //
        buffer[2] = static_cast<uint8_t>(m_port >> 8);
        buffer[3] = static_cast<uint8_t>(m_port);
        QCC_DbgPrintf(("IsAt::Serialize(): Port %d", m_port));
        size += 2;

        //
        // From this point on, things are not at fixed addresses
        //
        p = &buffer[4];

        //
        // If the F bit is set, we need to include the IPv4 address.
        //
        if (m_flagF) {
            qcc::IPAddress::StringToIPv4(m_ipv4, p, 4);
            QCC_DbgPrintf(("IsAt::Serialize(): IPv4: %s", m_ipv4.c_str()));
            p += 4;
            size += 4;
        }

        //
        // If the S bit is set, we need to include the IPv6 address.
        //
        if (m_flagS) {
            qcc::IPAddress::StringToIPv6(m_ipv6, p, 16);
            QCC_DbgPrintf(("IsAt::Serialize(): IPv6: %s", m_ipv6.c_str()));
            p += 16;
            size += 16;
        }

        //
        // Let the string data decide for themselves how long the rest of the
        // message will be.  If the G bit is set, we need to include the GUID
        // string.
        //
        if (m_flagG) {
            StringData stringData;
            stringData.Set(m_guid);
            QCC_DbgPrintf(("IsAt::Serialize(): GUID %s", m_guid.c_str()));
            size_t stringSize = stringData.Serialize(p);
            size += stringSize;
            p += stringSize;
        }

        for (uint32_t i = 0; i < m_names.size(); ++i) {
            StringData stringData;
            stringData.Set(m_names[i]);
            QCC_DbgPrintf(("IsAt::Serialize(): name %s", m_names[i].c_str()));
            size_t stringSize = stringData.Serialize(p);
            size += stringSize;
            p += stringSize;
        }
        break;

    case 1:
        //
        // The first octet is type (M = 1) and flags.
        //
        typeAndFlags = 1 << 6;

        if (m_flagG) {
            QCC_DbgPrintf(("IsAt::Serialize(): G flag"));
            typeAndFlags |= 0x20;
        }
        if (m_flagC) {
            QCC_DbgPrintf(("IsAt::Serialize(): C flag"));
            typeAndFlags |= 0x10;
        }
        if (m_flagR4) {
            QCC_DbgPrintf(("IsAt::Serialize(): R4 flag"));
            typeAndFlags |= 0x8;
        }
        if (m_flagU4) {
            QCC_DbgPrintf(("IsAt::Serialize(): U4 flag"));
            typeAndFlags |= 0x4;
        }
        if (m_flagR6) {
            QCC_DbgPrintf(("IsAt::Serialize(): R6 flag"));
            typeAndFlags |= 0x2;
        }
        if (m_flagU6) {
            QCC_DbgPrintf(("IsAt::Serialize(): U6 flag"));
            typeAndFlags |= 0x1;
        }

        buffer[0] = typeAndFlags;
        size += 1;

        //
        // The second octet is the count of bus names.
        //
        assert(m_names.size() < 256);
        buffer[1] = static_cast<uint8_t>(m_names.size());
        QCC_DbgPrintf(("IsAt::Serialize(): Count %d", m_names.size()));
        size += 1;

        //
        // The following two octets are the transport mask in network byte
        // order (big endian, or most significant byte first).
        //
        buffer[2] = static_cast<uint8_t>(m_transportMask >> 8);
        buffer[3] = static_cast<uint8_t>(m_transportMask);
        QCC_DbgPrintf(("IsAt::Serialize(): TransportMask 0x%x", m_transportMask));
        size += 2;

        //
        // From this point on, things are not at fixed addresses
        //
        p = &buffer[4];

        //
        // If the R4 bit is set, we need to include the reliable IPv4 address
        // and port.
        //
        if (m_flagR4) {
            qcc::IPAddress::StringToIPv4(m_reliableIPv4Address, p, 4);
            QCC_DbgPrintf(("IsAt::Serialize(): Reliable IPv4: %s", m_reliableIPv4Address.c_str()));
            p += 4;
            size += 4;

            *p++ = static_cast<uint8_t>(m_reliableIPv4Port >> 8);
            *p++ = static_cast<uint8_t>(m_reliableIPv4Port);
            QCC_DbgPrintf(("IsAt::Serialize(): Reliable IPv4 port %d", m_reliableIPv4Port));
            size += 2;
        }

        //
        // If the U4 bit is set, we need to include the unreliable IPv4 address
        // and port.
        //
        if (m_flagU4) {
            qcc::IPAddress::StringToIPv4(m_unreliableIPv4Address, p, 4);
            QCC_DbgPrintf(("IsAt::Serialize(): Unreliable IPv4: %s", m_unreliableIPv4Address.c_str()));
            p += 4;
            size += 4;

            *p++ = static_cast<uint8_t>(m_unreliableIPv4Port >> 8);
            *p++ = static_cast<uint8_t>(m_unreliableIPv4Port);
            QCC_DbgPrintf(("IsAt::Serialize(): Unreliable IPv4 port %d", m_unreliableIPv4Port));
            size += 2;
        }

        //
        // If the R6 bit is set, we need to include the reliable IPv6 address
        // and port.
        //
        if (m_flagR6) {
            qcc::IPAddress::StringToIPv6(m_reliableIPv6Address, p, 16);
            QCC_DbgPrintf(("IsAt::Serialize(): Reliable IPv6: %s", m_reliableIPv6Address.c_str()));
            p += 16;
            size += 16;

            *p++ = static_cast<uint8_t>(m_reliableIPv6Port >> 8);
            *p++ = static_cast<uint8_t>(m_reliableIPv6Port);
            QCC_DbgPrintf(("IsAt::Serialize(): Reliable IPv6 port %d", m_reliableIPv6Port));
            size += 2;
        }

        //
        // If the U6 bit is set, we need to include the unreliable IPv6 address
        // and port.
        //
        if (m_flagU6) {
            qcc::IPAddress::StringToIPv6(m_unreliableIPv6Address, p, 16);
            QCC_DbgPrintf(("IsAt::Serialize(): Unreliable IPv6: %s", m_unreliableIPv6Address.c_str()));
            p += 16;
            size += 16;

            *p++ = static_cast<uint8_t>(m_unreliableIPv6Port >> 8);
            *p++ = static_cast<uint8_t>(m_unreliableIPv6Port);
            QCC_DbgPrintf(("IsAt::Serialize(): Unreliable IPv6 port %d", m_unreliableIPv6Port));
            size += 2;
        }

        //
        // Let the string data decide for themselves how long the rest of the
        // message will be.  If the G bit is set, we need to include the GUID
        // string.
        //
        if (m_flagG) {
            StringData stringData;
            stringData.Set(m_guid);
            QCC_DbgPrintf(("IsAt::Serialize(): GUID %s", m_guid.c_str()));
            size_t stringSize = stringData.Serialize(p);
            size += stringSize;
            p += stringSize;
        }

        for (uint32_t i = 0; i < m_names.size(); ++i) {
            StringData stringData;
            stringData.Set(m_names[i]);
            QCC_DbgPrintf(("IsAt::Serialize(): name %s", m_names[i].c_str()));
            size_t stringSize = stringData.Serialize(p);
            size += stringSize;
            p += stringSize;
        }
        break;

    default:
        assert(false && "IsAt::Serialize(): Unexpected version");
        break;
    }

    return size;
}

size_t IsAt::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    QCC_DbgPrintf(("IsAt::Deserialize()"));

    //
    // We keep track of the size (the size of the buffer we read) so testers
    // can check coherence between GetSerializedSize() and Serialize() and
    // Deserialize().
    //
    size_t size = 0;

    uint8_t typeAndFlags = 0;
    uint8_t const* p = NULL;
    uint8_t numberNames = 0;

    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    switch (m_version & 0xf) {
    case 0:
        //
        // If there's not enough room in the buffer to get the fixed part out then
        // bail (one byte of type and flags, one byte of name count and two bytes
        // of port).
        //
        if (bufsize < 4) {
            QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
            return 0;
        }

        size = 0;

        //
        // The first octet is type (1) and flags.
        //
        typeAndFlags = buffer[0];
        size += 1;

        //
        // This had better be an IsAt message we're working on
        //
        if ((typeAndFlags & 0xc0) != 1 << 6) {
            QCC_DbgPrintf(("IsAt::Deserialize(): Incorrect type %d", typeAndFlags & 0xc0));
            return 0;
        }

        m_flagG = (typeAndFlags & 0x20) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): G flag %d", m_flagG));

        m_flagC = (typeAndFlags & 0x10) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): C flag %d", m_flagC));

        m_flagT = (typeAndFlags & 0x8) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): T flag %d", m_flagT));

        m_flagU = (typeAndFlags & 0x4) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): U flag %d", m_flagU));

        m_flagS = (typeAndFlags & 0x2) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): S flag %d", m_flagS));

        m_flagF = (typeAndFlags & 0x1) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): F flag %d", m_flagF));

        //
        // The second octet is the count of bus names.
        //
        numberNames = buffer[1];
        QCC_DbgPrintf(("IsAt::Deserialize(): Count %d", numberNames));
        size += 1;

        //
        // The following two octets are the port number in network byte
        // order (big endian, or most significant byte first).
        //
        m_port = (static_cast<uint16_t>(buffer[2]) << 8) | (static_cast<uint16_t>(buffer[3]) & 0xff);
        QCC_DbgPrintf(("IsAt::Deserialize(): Port %d", m_port));
        size += 2;

        //
        // From this point on, things are not at fixed addresses
        //
        p = &buffer[4];
        bufsize -= 4;

        //
        // If the F bit is set, we need to read off an IPv4 address; and we'd better
        // have enough buffer to read it out of.
        //
        if (m_flagF) {
            if (bufsize < 4) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                return 0;
            }
            m_ipv4 = qcc::IPAddress::IPv4ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): IPv4: %s", m_ipv4.c_str()));
            p += 4;
            size += 4;
            bufsize -= 4;
        }

        //
        // If the S bit is set, we need to read off an IPv6 address; and we'd better
        // have enough buffer to read it out of.
        //
        if (m_flagS) {
            if (bufsize < 16) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                return 0;
            }
            m_ipv6 = qcc::IPAddress::IPv6ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): IPv6: %s", m_ipv6.c_str()));
            p += 16;
            size += 16;
            bufsize -= 16;
        }

        //
        // If the G bit is set, we need to read off a GUID string.
        //
        if (m_flagG) {
            QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize() GUID"));
            StringData stringData;

            //
            // Tell the string to read itself out.  If there's not enough buffer
            // it will complain by returning 0.  We pass the complaint on up.
            //
            size_t stringSize = stringData.Deserialize(p, bufsize);
            if (stringSize == 0) {
                QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize():  Error"));
                return 0;
            }
            SetGuid(stringData.Get());
            size += stringSize;
            p += stringSize;
            bufsize -= stringSize;
        }

        //
        // Now we need to read out <numberNames> names that the packet has told us
        // will be there.
        //
        for (uint32_t i = 0; i < numberNames; ++i) {
            QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize() name %d", i));
            StringData stringData;

            //
            // Tell the string to read itself out.  If there's not enough buffer
            // it will complain by returning 0.  We pass the complaint on up.
            //
            size_t stringSize = stringData.Deserialize(p, bufsize);
            if (stringSize == 0) {
                QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize():  Error"));
                return 0;
            }
            AddName(stringData.Get());
            size += stringSize;
            p += stringSize;
            bufsize -= stringSize;
        }
        break;

    case 1:
        //
        // If there's not enough room in the buffer to get the fixed part out then
        // bail (one byte of type and flags, one byte of name count)
        //
        if (bufsize < 2) {
            QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
            return 0;
        }

        //
        // We keep track of the size (the size of the buffer we read) so testers
        // can check coherence between GetSerializedSize() and Serialize() and
        // Deserialize().
        //
        size = 0;

        //
        // The first octet is type (1) and flags.
        //
        typeAndFlags = buffer[0];
        size += 1;

        //
        // This had better be an IsAt message we're working on
        //
        if ((typeAndFlags & 0xc0) != 1 << 6) {
            QCC_DbgPrintf(("IsAt::Deserialize(): Incorrect type %d", typeAndFlags & 0xc0));
            return 0;
        }

        m_flagG = (typeAndFlags & 0x20) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): G flag %d", m_flagG));

        m_flagC = (typeAndFlags & 0x10) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): C flag %d", m_flagC));

        m_flagR4 = (typeAndFlags & 0x8) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): R4 flag %d", m_flagR4));

        m_flagU4 = (typeAndFlags & 0x4) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): U4 flag %d", m_flagU4));

        m_flagR6 = (typeAndFlags & 0x2) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): R6 flag %d", m_flagR6));

        m_flagU6 = (typeAndFlags & 0x1) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): F flag %d", m_flagU6));

        //
        // The second octet is the count of bus names.
        //
        numberNames = buffer[1];
        QCC_DbgPrintf(("IsAt::Deserialize(): Count %d", numberNames));
        size += 1;

        m_transportMask = (static_cast<uint16_t>(buffer[2]) << 8) | (static_cast<uint16_t>(buffer[3]) & 0xff);
        QCC_DbgPrintf(("IsAt::Serialize(): TransportMask 0x%x", m_transportMask));
        size += 2;

        //
        // From this point on, things are not at fixed addresses
        //
        p = &buffer[4];
        bufsize -= 4;

        //
        // If the R4 bit is set, we need to read off an IPv4 address and port;
        // and we'd better have enough buffer to read it out of.
        //
        if (m_flagR4) {
            if (bufsize < 6) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                return 0;
            }

            m_reliableIPv4Address = qcc::IPAddress::IPv4ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): Reliable IPv4: %s", m_reliableIPv4Address.c_str()));
            p += 4;
            size += 4;
            bufsize -= 4;

            m_reliableIPv4Port = (static_cast<uint16_t>(p[0]) << 8) | (static_cast<uint16_t>(p[1]) & 0xff);
            QCC_DbgPrintf(("IsAt::Deserialize(): Reliable IPv4 port %d", m_reliableIPv4Port));
            p += 2;
            size += 2;
            bufsize -= 2;
        }

        //
        // If the R4 bit is set, we need to read off an IPv4 address and port;
        // and we'd better have enough buffer to read it out of.
        //
        if (m_flagU4) {
            if (bufsize < 6) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                return 0;
            }

            m_unreliableIPv4Address = qcc::IPAddress::IPv4ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): Unreliable IPv4: %s", m_unreliableIPv4Address.c_str()));
            p += 4;
            size += 4;
            bufsize -= 4;

            m_unreliableIPv4Port = (static_cast<uint16_t>(p[0]) << 8) | (static_cast<uint16_t>(p[1]) & 0xff);
            QCC_DbgPrintf(("IsAt::Deserialize(): Unreliable IPv4 port %d", m_unreliableIPv4Port));
            p += 2;
            size += 2;
            bufsize -= 2;
        }

        //
        // If the R6 bit is set, we need to read off an IPv6 address and port; and we'd better
        // have enough buffer to read it out of.
        //
        if (m_flagR6) {
            if (bufsize < 18) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                return 0;
            }

            m_reliableIPv6Address = qcc::IPAddress::IPv6ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): Reliable IPv6: %s", m_reliableIPv6Address.c_str()));
            p += 16;
            size += 16;
            bufsize -= 16;

            m_reliableIPv6Port = (static_cast<uint16_t>(p[0]) << 8) | (static_cast<uint16_t>(p[1]) & 0xff);
            QCC_DbgPrintf(("IsAt::Deserialize(): Reliable IPv6 port %d", m_reliableIPv6Port));
            p += 2;
            size += 2;
            bufsize -= 2;
        }

        //
        // If the U6 bit is set, we need to read off an IPv6 address and port; and we'd better
        // have enough buffer to read it out of.
        //
        if (m_flagU6) {
            if (bufsize < 18) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                return 0;
            }

            m_unreliableIPv6Address = qcc::IPAddress::IPv6ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): Unreliable IPv6: %s", m_unreliableIPv6Address.c_str()));
            p += 16;
            size += 16;
            bufsize -= 16;

            m_unreliableIPv6Port = (static_cast<uint16_t>(p[0]) << 8) | (static_cast<uint16_t>(p[1]) & 0xff);
            QCC_DbgPrintf(("IsAt::Deserialize(): Unreliable IPv6 port %d", m_unreliableIPv6Port));
            p += 2;
            size += 2;
            bufsize -= 2;
        }

        //
        // If the G bit is set, we need to read off a GUID string.
        //
        if (m_flagG) {
            QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize() GUID"));
            StringData stringData;

            //
            // Tell the string to read itself out.  If there's not enough buffer
            // it will complain by returning 0.  We pass the complaint on up.
            //
            size_t stringSize = stringData.Deserialize(p, bufsize);
            if (stringSize == 0) {
                QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize():  Error"));
                return 0;
            }
            SetGuid(stringData.Get());
            size += stringSize;
            p += stringSize;
            bufsize -= stringSize;
        }

        //
        // Now we need to read out <numberNames> names that the packet has told us
        // will be there.
        //
        for (uint32_t i = 0; i < numberNames; ++i) {
            QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize() name %d", i));
            StringData stringData;

            //
            // Tell the string to read itself out.  If there's not enough buffer
            // it will complain by returning 0.  We pass the complaint on up.
            //
            size_t stringSize = stringData.Deserialize(p, bufsize);
            if (stringSize == 0) {
                QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize():  Error"));
                return 0;
            }
            AddName(stringData.Get());
            size += stringSize;
            p += stringSize;
            bufsize -= stringSize;
        }
        break;

    default:
        assert(false && "IsAt::Deserialize(): Unexpected version");
        break;
    }
    return size;
}

WhoHas::WhoHas()
    : m_version(0), m_transportMask(TRANSPORT_NONE), m_flagT(false), m_flagU(false), m_flagS(false), m_flagF(false)
{
}

WhoHas::~WhoHas()
{
}

void WhoHas::Reset(void)
{
    m_names.clear();
}

void WhoHas::AddName(qcc::String name)
{
    m_names.push_back(name);
}

uint32_t WhoHas::GetNumberNames(void) const
{
    return m_names.size();
}

qcc::String WhoHas::GetName(uint32_t index) const
{
    assert(index < m_names.size());
    return m_names[index];
}

size_t WhoHas::GetSerializedSize(void) const
{
    //
    // Version zero and one are identical with the exeption of the definition
    // of the flags, so the size is the same.
    //
    size_t size = 0;

    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    switch (m_version & 0xf) {
    case 0:
    case 1:
        //
        // We have one octet for type and flags and one octet for count.
        // Two octets to start.
        //
        size = 2;

        //
        // Let the string data decide for themselves how long the rest
        // of the message will be.
        //
        for (uint32_t i = 0; i < m_names.size(); ++i) {
            StringData s;
            s.Set(m_names[i]);
            size += s.GetSerializedSize();
        }
        break;

    default:
        assert(false && "WhoHas::GetSerializedSize(): Unexpected version");
        break;
    }

    return size;
}

size_t WhoHas::Serialize(uint8_t* buffer) const
{
    QCC_DbgPrintf(("WhoHas::Serialize(): to buffer 0x%x", buffer));

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first octet is type (M = 2) and flags.
    //
    uint8_t typeAndFlags = 2 << 6;

    //
    // The only difference between version zero and one is that in version one
    // the flags are deprecated and revert to reserved.  So we just don't
    // serialize them if we are writing a version one object.
    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    if ((m_version & 0xf) == 0) {
        if (m_flagT) {
            QCC_DbgPrintf(("WhoHas::Serialize(): T flag"));
            typeAndFlags |= 0x8;
        }
        if (m_flagU) {
            QCC_DbgPrintf(("WhoHas::Serialize(): U flag"));
            typeAndFlags |= 0x4;
        }
        if (m_flagS) {
            QCC_DbgPrintf(("WhoHas::Serialize(): S flag"));
            typeAndFlags |= 0x2;
        }
        if (m_flagF) {
            QCC_DbgPrintf(("WhoHas::Serialize(): F flag"));
            typeAndFlags |= 0x1;
        }
    }

    buffer[0] = typeAndFlags;
    size += 1;

    //
    // The second octet is the count of bus names.
    //
    assert(m_names.size() < 256);
    buffer[1] = static_cast<uint8_t>(m_names.size());
    QCC_DbgPrintf(("WhoHas::Serialize(): Count %d", m_names.size()));
    size += 1;

    //
    // From this point on, things are not at fixed addresses
    //
    uint8_t* p = &buffer[2];

    //
    // Let the string data decide for themselves how long the rest
    // of the message will be.
    //
    for (uint32_t i = 0; i < m_names.size(); ++i) {
        StringData stringData;
        stringData.Set(m_names[i]);
        QCC_DbgPrintf(("Whohas::Serialize(): name %s", m_names[i].c_str()));
        size_t stringSize = stringData.Serialize(p);
        size += stringSize;
        p += stringSize;
    }

    return size;
}

size_t WhoHas::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    QCC_DbgPrintf(("WhoHas::Deserialize()"));

    //
    // If there's not enough room in the buffer to get the fixed part out then
    // bail (one byte of type and flags, one byte of name count).
    //
    if (bufsize < 2) {
        QCC_DbgPrintf(("WhoHas::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first octet is type (1) and flags.
    //
    uint8_t typeAndFlags = buffer[0];
    size += 1;

    //
    // This had better be an WhoHas message we're working on
    //
    if ((typeAndFlags & 0xc0) != 2 << 6) {
        QCC_DbgPrintf(("WhoHas::Deserialize(): Incorrect type %d", typeAndFlags & 0xc0));
        return 0;
    }

    //
    // Due to an oversight, the transport mask was not actually serialized,
    // so we initialize it to 0, which means no transport.
    //
    m_transportMask = TRANSPORT_NONE;

    //
    // The only difference between the version zero and version one protocols
    // is that the flags are deprecated in version one.  In the case of
    // deserializing a version one object, we just don't set the old flags.
    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    switch (m_version & 0xf) {
    case 0:
        m_flagT = (typeAndFlags & 0x8) != 0;
        QCC_DbgPrintf(("WhoHas::Deserialize(): T flag %d", m_flagT));

        m_flagU = (typeAndFlags & 0x4) != 0;
        QCC_DbgPrintf(("WhoHas::Deserialize(): U flag %d", m_flagU));

        m_flagS = (typeAndFlags & 0x2) != 0;
        QCC_DbgPrintf(("WhoHas::Deserialize(): S flag %d", m_flagS));

        m_flagF = (typeAndFlags & 0x1) != 0;
        QCC_DbgPrintf(("WhoHas::Deserialize(): F flag %d", m_flagF));
        break;

    case 1:
        m_flagT = m_flagU = m_flagS = m_flagF = false;
        break;

    default:
        assert(false && "WhoHas::Deserialize(): Unexpected version");
        break;
    }

    //
    // The second octet is the count of bus names.
    //
    uint8_t numberNames = buffer[1];
    QCC_DbgPrintf(("WhoHas::Deserialize(): Count %d", numberNames));
    size += 1;

    //
    // From this point on, things are not at fixed addresses
    //
    uint8_t const* p = &buffer[2];
    bufsize -= 2;

    //
    // Now we need to read out <numberNames> names that the packet has told us
    // will be there.
    //
    for (uint32_t i = 0; i < numberNames; ++i) {
        QCC_DbgPrintf(("WhoHas::Deserialize(): StringData::Deserialize() name %d", i));
        StringData stringData;

        //
        // Tell the string to read itself out.  If there's not enough buffer
        // it will complain by returning 0.  We pass the complaint on up.
        //
        size_t stringSize = stringData.Deserialize(p, bufsize);
        if (stringSize == 0) {
            QCC_DbgPrintf(("WhoHas::Deserialize(): StringData::Deserialize():  Error"));
            return 0;
        }

        AddName(stringData.Get());
        size += stringSize;
        p += stringSize;
        bufsize -= stringSize;
    }

    return size;
}

Header::Header()
    : m_version(0), m_timer(0), m_destination("0.0.0.0", 0), m_destinationSet(false), m_retries(0), m_tick(0)
{
}

Header::~Header()
{
}

void Header::SetTimer(uint8_t timer)
{
    m_timer = timer;
}

uint8_t Header::GetTimer(void) const
{
    return m_timer;
}

void Header::Reset(void)
{
    m_questions.clear();
    m_answers.clear();
}

void Header::AddQuestion(WhoHas question)
{
    m_questions.push_back(question);
}

uint32_t Header::GetNumberQuestions(void) const
{
    return m_questions.size();
}

WhoHas Header::GetQuestion(uint32_t index) const
{
    assert(index < m_questions.size());
    return m_questions[index];
}

void Header::GetQuestion(uint32_t index, WhoHas** question)
{
    assert(index < m_questions.size());
    *question = &m_questions[index];
}

void Header::AddAnswer(IsAt answer)
{
    m_answers.push_back(answer);
}

uint32_t Header::GetNumberAnswers(void) const
{
    return m_answers.size();
}

IsAt Header::GetAnswer(uint32_t index) const
{
    assert(index < m_answers.size());
    return m_answers[index];
}

void Header::GetAnswer(uint32_t index, IsAt** answer)
{
    assert(index < m_answers.size());
    *answer = &m_answers[index];
}

size_t Header::GetSerializedSize(void) const
{
    //
    // We have one octet for version, one four question count, one for answer
    // count and one for timer.  Four octets to start.
    //
    size_t size = 4;

    //
    // Let the questions data decide for themselves how long the question part
    // of the message will be.
    //
    for (uint32_t i = 0; i < m_questions.size(); ++i) {
        WhoHas whoHas = m_questions[i];
        size += whoHas.GetSerializedSize();
    }

    //
    // Let the answers decide for themselves how long the answer part
    // of the message will be.
    //
    for (uint32_t i = 0; i < m_answers.size(); ++i) {
        IsAt isAt = m_answers[i];
        size += isAt.GetSerializedSize();
    }

    return size;
}

size_t Header::Serialize(uint8_t* buffer) const
{
    QCC_DbgPrintf(("Header::Serialize(): to buffer 0x%x", buffer));
    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first octet is version
    //
    buffer[0] = m_version;;
    QCC_DbgPrintf(("Header::Serialize(): version = %d", m_version));
    size += 1;

    //
    // The second octet is the count of questions.
    //
    buffer[1] = static_cast<uint8_t>(m_questions.size());
    QCC_DbgPrintf(("Header::Serialize(): QCount = %d", m_questions.size()));
    size += 1;

    //
    // The third octet is the count of answers.
    //
    buffer[2] = static_cast<uint8_t>(m_answers.size());
    QCC_DbgPrintf(("Header::Serialize(): ACount = %d", m_answers.size()));
    size += 1;

    //
    // The fourth octet is the timer for the answers.
    //
    buffer[3] = m_timer;
    QCC_DbgPrintf(("Header::Serialize(): timer = %d", m_timer));
    size += 1;

    //
    // From this point on, things are not at fixed addresses
    //
    uint8_t* p = &buffer[4];

    //
    // Let the questions push themselves out.
    //
    for (uint32_t i = 0; i < m_questions.size(); ++i) {
        QCC_DbgPrintf(("Header::Serialize(): WhoHas::Serialize() question %d", i));
        WhoHas whoHas = m_questions[i];
        size_t questionSize = whoHas.Serialize(p);
        size += questionSize;
        p += questionSize;
    }

    //
    // Let the answers push themselves out.
    //
    for (uint32_t i = 0; i < m_answers.size(); ++i) {
        QCC_DbgPrintf(("Header::Serialize(): IsAt::Serialize() answer %d", i));
        IsAt isAt = m_answers[i];
        size_t answerSize = isAt.Serialize(p);
        size += answerSize;
        p += answerSize;
    }

    return size;
}

size_t Header::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    //
    // If there's not enough room in the buffer to get the fixed part out then
    // bail (one byte of version, one byte of question count, one byte of answer
    // count and one byte of timer).
    //
    if (bufsize < 4) {
        QCC_DbgPrintf(("Header::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first octet is version.  We need to filter out bogus versions here
    // since we are going to promptly set this version in the included who-has
    // and is-at messages and they will assert that the versions we set actually
    // make sense.
    //
    uint8_t nsVersion, msgVersion;
    nsVersion = buffer[0] >> 4;
    msgVersion = buffer[0] & 0xf;
    if (nsVersion != 0 && nsVersion != 1) {
        QCC_DbgPrintf(("Header::Deserialize(): Bad remote name service version %d", nsVersion));
        return 0;
    }

    if (msgVersion != 0 && msgVersion != 1) {
        QCC_DbgPrintf(("Header::Deserialize(): Bad message version %d", msgVersion));
        return 0;
    }

    m_version = buffer[0];
    size += 1;

    //
    // The second octet is the count of questions.
    //
    uint8_t qCount = buffer[1];
    size += 1;

    //
    // The third octet is the count of answers.
    //
    uint8_t aCount = buffer[2];
    size += 1;

    //
    // The fourth octet is the timer for the answers.
    //
    m_timer = buffer[3];
    size += 1;

    //
    // From this point on, things are not at fixed addresses
    //
    uint8_t const* p = &buffer[4];
    bufsize -= 4;

    //
    // Now we need to read out <qCount> questions that the packet has told us
    // will be there.
    //
    for (uint8_t i = 0; i < qCount; ++i) {
        QCC_DbgPrintf(("Header::Deserialize(): WhoHas::Deserialize() question %d", i));
        WhoHas whoHas;
        whoHas.SetVersion(m_version >> 4, m_version & 0xf);

        //
        // Tell the question to read itself out.  If there's not enough buffer
        // it will complain by returning 0.  We pass the complaint on up.
        //
        size_t qSize = whoHas.Deserialize(p, bufsize);
        if (qSize == 0) {
            QCC_DbgPrintf(("Header::Deserialize(): WhoHas::Deserialize():  Error"));
            return 0;
        }
        m_questions.push_back(whoHas);
        size += qSize;
        p += qSize;
        bufsize -= qSize;
    }

    //
    // Now we need to read out <aCount> answers that the packet has told us
    // will be there.
    //
    for (uint8_t i = 0; i < aCount; ++i) {
        QCC_DbgPrintf(("Header::Deserialize(): IsAt::Deserialize() answer %d", i));
        IsAt isAt;
        isAt.SetVersion(m_version >> 4, m_version & 0xf);

        //
        // Tell the answer to read itself out.  If there's not enough buffer
        // it will complain by returning 0.  We pass the complaint on up.
        //
        size_t aSize = isAt.Deserialize(p, bufsize);
        if (aSize == 0) {
            QCC_DbgPrintf(("Header::Deserialize(): IsAt::Deserialize():  Error"));
            return 0;
        }
        m_answers.push_back(isAt);
        size += aSize;
        p += aSize;
        bufsize -= aSize;
    }

    return size;
}

} // namespace ajn
