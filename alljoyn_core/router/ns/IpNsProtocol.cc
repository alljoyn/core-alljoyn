/**
 * @file
 * The simple name service protocol implementation
 */

/******************************************************************************
 * Copyright (c) 2010-2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/StringUtil.h>
#include <alljoyn/AllJoynStd.h>
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
_Packet::_Packet()
    : m_timer(0), m_destination("0.0.0.0", 0), m_destinationSet(false), m_interfaceIndex(-1), m_interfaceIndexSet(false), m_addressFamily(qcc::QCC_AF_UNSPEC), m_addressFamilySet(false), m_retries(0), m_tick(0), m_version(0)
{
}

_Packet::~_Packet()
{
}
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

void IsAt::RemoveName(uint32_t index)
{
    if (index < m_names.size()) {
        m_names.erase(m_names.begin() + index);
    }
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
        QCC_LogError(ER_WARNING, ("IsAt::GetSerializedSize(): Unexpected version %d", m_version & 0xf));
        size = 0;
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
        if (bufsize < 4) {
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
    } else {
        typeAndFlags |= 0x4;
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
        m_flagU = (typeAndFlags & 0x4) != 0;
        m_flagT = m_flagS = m_flagF = false;

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

_NSPacket::_NSPacket()
{
}

_NSPacket::~_NSPacket()
{
}

void _Packet::SetTimer(uint8_t timer)
{
    m_timer = timer;
}

uint8_t _Packet::GetTimer(void) const
{
    return m_timer;
}

void _NSPacket::Reset(void)
{
    m_questions.clear();
    m_answers.clear();
}

void _NSPacket::AddQuestion(WhoHas question)
{
    m_questions.push_back(question);
}

uint32_t _NSPacket::GetNumberQuestions(void) const
{
    return m_questions.size();
}

WhoHas _NSPacket::GetQuestion(uint32_t index) const
{
    assert(index < m_questions.size());
    return m_questions[index];
}


void _NSPacket::GetQuestion(uint32_t index, WhoHas** question)
{
    assert(index < m_questions.size());
    *question = &m_questions[index];
}


void _NSPacket::AddAnswer(IsAt answer)
{
    m_answers.push_back(answer);
}

void _NSPacket::RemoveAnswer(uint32_t index)
{
    if (index < m_answers.size()) {
        m_answers.erase(m_answers.begin() + index);
    }
}

uint32_t _NSPacket::GetNumberAnswers(void) const
{
    return m_answers.size();
}

IsAt _NSPacket::GetAnswer(uint32_t index) const
{
    assert(index < m_answers.size());
    return m_answers[index];
}

void _NSPacket::GetAnswer(uint32_t index, IsAt** answer)
{
    assert(index < m_answers.size());
    *answer = &m_answers[index];
}

size_t _NSPacket::GetSerializedSize(void) const
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

size_t _NSPacket::Serialize(uint8_t* buffer) const
{
    QCC_DbgPrintf(("NSPacket::Serialize(): to buffer 0x%x", buffer));
    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first octet is version
    //
    buffer[0] = m_version;
    QCC_DbgPrintf(("NSPacket::Serialize(): version = %d", m_version));
    size += 1;

    //
    // The second octet is the count of questions.
    //
    buffer[1] = static_cast<uint8_t>(m_questions.size());
    QCC_DbgPrintf(("NSPacket::Serialize(): QCount = %d", m_questions.size()));
    size += 1;

    //
    // The third octet is the count of answers.
    //
    buffer[2] = static_cast<uint8_t>(m_answers.size());
    QCC_DbgPrintf(("NSPacket::Serialize(): ACount = %d", m_answers.size()));
    size += 1;

    //
    // The fourth octet is the timer for the answers.
    //
    buffer[3] = GetTimer();
    QCC_DbgPrintf(("NSPacket::Serialize(): timer = %d", GetTimer()));
    size += 1;

    //
    // From this point on, things are not at fixed addresses
    //
    uint8_t* p = &buffer[4];

    //
    // Let the questions push themselves out.
    //
    for (uint32_t i = 0; i < m_questions.size(); ++i) {
        QCC_DbgPrintf(("NSPacket::Serialize(): WhoHas::Serialize() question %d", i));
        WhoHas whoHas = m_questions[i];
        size_t questionSize = whoHas.Serialize(p);
        size += questionSize;
        p += questionSize;
    }

    //
    // Let the answers push themselves out.
    //
    for (uint32_t i = 0; i < m_answers.size(); ++i) {
        QCC_DbgPrintf(("NSPacket::Serialize(): IsAt::Serialize() answer %d", i));
        IsAt isAt = m_answers[i];
        size_t answerSize = isAt.Serialize(p);
        size += answerSize;
        p += answerSize;
    }

    return size;
}

size_t _NSPacket::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    //
    // If there's not enough room in the buffer to get the fixed part out then
    // bail (one byte of version, one byte of question count, one byte of answer
    // count and one byte of timer).
    //
    if (bufsize < 4) {
        QCC_DbgPrintf(("NSPacket::Deserialize(): Insufficient bufsize %d", bufsize));
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
    uint8_t msgVersion;
    msgVersion = buffer[0] & 0xf;

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
    SetTimer(buffer[3]);
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
        QCC_DbgPrintf(("NSPacket::Deserialize(): WhoHas::Deserialize() question %d", i));
        WhoHas whoHas;
        whoHas.SetVersion(m_version >> 4, m_version & 0xf);

        //
        // Tell the question to read itself out.  If there's not enough buffer
        // it will complain by returning 0.  We pass the complaint on up.
        //
        size_t qSize = whoHas.Deserialize(p, bufsize);
        if (qSize == 0) {
            QCC_DbgPrintf(("NSPacket::Deserialize(): WhoHas::Deserialize():  Error"));
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
        QCC_DbgPrintf(("NSPacket::Deserialize(): IsAt::Deserialize() answer %d", i));
        IsAt isAt;
        isAt.SetVersion(m_version >> 4, m_version & 0xf);

        //
        // Tell the answer to read itself out.  If there's not enough buffer
        // it will complain by returning 0.  We pass the complaint on up.
        //
        size_t aSize = isAt.Deserialize(p, bufsize);
        if (aSize == 0) {
            QCC_DbgPrintf(("NSPacket::Deserialize(): IsAt::Deserialize():  Error"));
            return 0;
        }
        m_answers.push_back(isAt);
        size += aSize;
        p += aSize;
        bufsize -= aSize;
    }

    return size;
}

//MDNSDomainName
void MDNSDomainName::SetName(String name)
{
    m_name = name;
}

String MDNSDomainName::GetName() const
{
    return m_name;
}

MDNSDomainName::MDNSDomainName()
{
}

MDNSDomainName::~MDNSDomainName()
{
}

size_t MDNSDomainName::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    size_t size = 0;
    String name = m_name;
    while (true) {
        if (name.empty()) {
            size++;
            break;
        } else if (offsets.find(name) != offsets.end()) {
            size++;
            size++;
            break;
        } else {
            offsets[name] = 0; /* 0 is used as a placeholder so that the serialized size is computed correctly */
            size_t newPos = name.find_first_of('.');
            String temp = name.substr(0, newPos);
            size++;
            size += temp.length();
            size_t pos = (newPos == String::npos) ? String::npos : (newPos + 1);
            name = name.substr(pos);
        }
    }
    return size;
}

size_t MDNSDomainName::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    size_t size = 0;
    String name = m_name;
    while (true) {
        if (name.empty()) {
            buffer[size++] = 0;
            break;
        } else if (offsets.find(name) != offsets.end()) {
            buffer[size++] = 0xc0 | ((offsets[name] & 0xFF00) >> 8);
            buffer[size++] = (offsets[name] & 0xFF);
            break;
        } else {
            offsets[name] = size + headerOffset;
            size_t newPos = name.find_first_of('.');
            String temp = name.substr(0, newPos);
            buffer[size++] = temp.length();
            memcpy(reinterpret_cast<void*>(&buffer[size]), const_cast<void*>(reinterpret_cast<const void*>(temp.c_str())), temp.size());
            size += temp.length();
            size_t pos = (newPos == String::npos) ? String::npos : (newPos + 1);
            name = name.substr(pos);
        }
    }
    return size;
}

size_t MDNSDomainName::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    m_name.clear();
    size_t size = 0;
    if (bufsize < 1) {
        QCC_DbgPrintf(("MDNSDomainName::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }
    vector<uint32_t> offsets;

    while (bufsize > 0) {
        if (((buffer[size] & 0xc0) >> 6) == 3 && bufsize > 1) {
            uint32_t pointer = ((buffer[size] << 8 | buffer[size + 1]) & 0x3FFF);
            if (compressedOffsets.find(pointer) != compressedOffsets.end()) {
                if (m_name.length() > 0) {
                    m_name.append('.');
                }
                m_name.append(compressedOffsets[pointer]);
                size += 2;
                break;
            } else {
                return 0;
            }
        }
        size_t temp_size = buffer[size++];
        bufsize--;
        //
        // If there's not enough data in the buffer then bail.
        //
        if (bufsize < temp_size) {
            QCC_DbgPrintf(("MDNSDomainName::Deserialize(): Insufficient bufsize %d", bufsize));
            return 0;
        }
        if (m_name.length() > 0) {
            m_name.append('.');
        }
        if (temp_size > 0) {
            offsets.push_back(headerOffset + size - 1);
            m_name.append(reinterpret_cast<const char*>(buffer + size), temp_size);
            bufsize -= temp_size;
            size += temp_size;
        } else {
            break;
        }

    }

    for (uint32_t i = 0; i < offsets.size(); ++i) {
        compressedOffsets[offsets[i]] = m_name.substr(offsets[i] - headerOffset);
    }

    return size;
}

//MDNSQuestion
MDNSQuestion::MDNSQuestion(qcc::String qName, uint16_t qType, uint16_t qClass) :
    m_qType(qType),
    m_qClass(qClass | QU_BIT)
{
    m_qName.SetName(qName);
}
void MDNSQuestion::SetQName(String qName)
{
    m_qName.SetName(qName);
}

String MDNSQuestion::GetQName()
{
    return m_qName.GetName();
}

void MDNSQuestion::SetQType(uint16_t qType)
{
    m_qType = qType;
}

uint16_t MDNSQuestion::GetQType()
{
    return m_qType;
}

void MDNSQuestion::SetQClass(uint16_t qClass)
{
    m_qClass = qClass | QU_BIT;
    QCC_DbgPrintf(("%X %X ", qClass, m_qClass));
}

uint16_t MDNSQuestion::GetQClass()
{
    return m_qClass & ~QU_BIT;
}

size_t MDNSQuestion::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    return m_qName.GetSerializedSize(offsets) + 4;
}

size_t MDNSQuestion::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    // Serialize the QNAME first
    size_t size = m_qName.Serialize(buffer, offsets, headerOffset);

    //Next two octets are QTYPE
    buffer[size] = (m_qType & 0xFF00) >> 8;
    buffer[size + 1] = (m_qType & 0xFF);

    //Next two octets are QCLASS
    buffer[size + 2] = (m_qClass & 0xFF00) >> 8;
    buffer[size + 3] = (m_qClass & 0xFF);
    QCC_DbgPrintf(("Set %X %X", buffer[size + 2], buffer[size + 3]));
    return size + 4;
}

size_t MDNSQuestion::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    // Deserialize the QNAME first
    size_t size = m_qName.Deserialize(buffer, bufsize, compressedOffsets, headerOffset);
    if (size >= bufsize) {
        return 0;
    }
    bufsize -= size;
    if (size == 0 || bufsize < 4) {
        //Error while deserializing QNAME or insufficient buffer size
        QCC_DbgPrintf(("MDNSQuestion::Deserialize Error while deserializing QName"));
        return 0;
    }

    //Next two octets are QTYPE
    m_qType = (buffer[size] << 8) | buffer[size + 1];
    size += 2;

    //Next two octets are QCLASS
    m_qClass = (buffer[size] << 8) | buffer[size + 1];
    size += 2;

    return size;
}

MDNSRData::~MDNSRData()
{
}

//MDNSResourceRecord
MDNSResourceRecord::MDNSResourceRecord() : m_rdata(NULL)
{
}

MDNSResourceRecord::MDNSResourceRecord(qcc::String domainName, RRType rrType, RRClass rrClass, uint16_t ttl, MDNSRData* rdata) :

    m_rrType(rrType),
    m_rrClass(rrClass),
    m_rrTTL(ttl)
{
    m_rrDomainName.SetName(domainName);
    m_rdata = rdata->GetDeepCopy();
}

MDNSResourceRecord::MDNSResourceRecord(const MDNSResourceRecord& r) :
    m_rrDomainName(r.m_rrDomainName),
    m_rrType(r.m_rrType),
    m_rrClass(r.m_rrClass),
    m_rrTTL(r.m_rrTTL)
{
    m_rdata = r.m_rdata->GetDeepCopy();
}

MDNSResourceRecord& MDNSResourceRecord::operator=(const MDNSResourceRecord& r)
{
    if (this != &r) {
        m_rrDomainName = r.m_rrDomainName;
        m_rrType = r.m_rrType;
        m_rrClass = r.m_rrClass;
        m_rrTTL = r.m_rrTTL;
        if (m_rdata) {
            delete m_rdata;
        }
        m_rdata = r.m_rdata->GetDeepCopy();
    }
    return *this;
}

MDNSResourceRecord::~MDNSResourceRecord()
{

    if (m_rdata) {
        delete m_rdata;
        m_rdata = NULL;
    }
}

size_t MDNSResourceRecord::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    assert(m_rdata);
    size_t size = m_rrDomainName.GetSerializedSize(offsets);
    size += 8;
    size += m_rdata->GetSerializedSize(offsets);
    return size;
}

size_t MDNSResourceRecord::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    assert(m_rdata);

    //
    // Serialize the NAME first
    //
    size_t size = m_rrDomainName.Serialize(buffer, offsets, headerOffset);

    //Next two octets are TYPE
    buffer[size] = (m_rrType & 0xFF00) >> 8;
    buffer[size + 1] = (m_rrType & 0xFF);

    //Next two octets are CLASS
    buffer[size + 2] = (m_rrClass & 0xFF00) >> 8;
    buffer[size + 3] = (m_rrClass & 0xFF);

    //Next four octets are TTL
    buffer[size + 4] = (m_rrTTL & 0xFF000000) >> 24;
    buffer[size + 5] = (m_rrTTL & 0xFF0000) >> 16;
    buffer[size + 6] = (m_rrTTL & 0xFF00) >> 8;
    buffer[size + 7] = (m_rrTTL & 0xFF);
    size += 8;

    uint8_t* p = &buffer[size];
    size += m_rdata->Serialize(p, offsets, headerOffset + size);
    return size;
}

size_t MDNSResourceRecord::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    if (m_rdata) {
        delete m_rdata;
        m_rdata = NULL;
    }
    //
    // Deserialize the NAME first
    //
    size_t size = m_rrDomainName.Deserialize(buffer, bufsize, compressedOffsets, headerOffset);
    if (size == 0 || bufsize < 8) {
        //error
        QCC_DbgPrintf((" MDNSResourceRecord::Deserialize() Error occured while deserializing domain name or insufficient buffer"));
        return 0;
    }

    //Next two octets are TYPE
    if (size > bufsize || ((size + 8) > bufsize)) {
        return 0;
    }
    m_rrType = (RRType)((buffer[size] << 8) | buffer[size + 1]);
    switch (m_rrType) {
    case A:
        m_rdata = new MDNSARData();
        break;

    case NS:
    case MD:
    case MF:
    case CNAME:
    case MB:
    case MG:
    case MR:
    case PTR:
        m_rdata = new MDNSPtrRData();
        break;

    case RNULL:
        m_rdata = new MDNSDefaultRData();
        break;

    case HINFO:
    case TXT:
        m_rdata = new MDNSTextRData();
        break;

    case AAAA:
        m_rdata = new MDNSAAAARData();
        break;

    case SRV:
        m_rdata = new MDNSSrvRData();
        break;

    default:
        m_rdata = new MDNSDefaultRData();
        QCC_DbgPrintf(("Ignoring unrecognized rrtype %d", m_rrType));
        break;
    }

    if (!m_rdata) {
        return 0;
    }
    //Next two octets are CLASS
    m_rrClass = (RRClass)((buffer[size + 2] << 8) | buffer[size + 3]);

    //Next four octets are TTL
    m_rrTTL = (buffer[size + 4] << 24) | (buffer[size + 5] << 16) | (buffer[size + 6] << 8) | buffer[size + 7];
    bufsize -= (size + 8);
    size += 8;
    headerOffset += size;
    uint8_t const* p = &buffer[size];
    size_t processed = m_rdata->Deserialize(p, bufsize, compressedOffsets, headerOffset);
    if (!processed) {
        QCC_DbgPrintf(("MDNSResourceRecord::Deserialize() Error occured while deserializing resource data"));
        return 0;
    }
    size += processed;
    return size;
}

void MDNSResourceRecord::SetDomainName(qcc::String domainName)
{
    m_rrDomainName.SetName(domainName);
}

qcc::String MDNSResourceRecord::GetDomainName() const
{
    return m_rrDomainName.GetName();
}

void MDNSResourceRecord::SetRRType(RRType rrtype)
{
    m_rrType = rrtype;
}

MDNSResourceRecord::RRType MDNSResourceRecord::GetRRType() const
{
    return m_rrType;
}

void MDNSResourceRecord::SetRRClass(RRClass rrclass)
{
    m_rrClass = rrclass;
}

MDNSResourceRecord::RRClass MDNSResourceRecord::GetRRClass() const
{
    return m_rrClass;
}

void MDNSResourceRecord::SetRRttl(uint16_t ttl)
{
    m_rrTTL = ttl;
}

uint16_t MDNSResourceRecord::GetRRttl() const
{
    return m_rrTTL;
}

void MDNSResourceRecord::SetRData(MDNSRData* rdata)
{
    if (m_rdata) {
        delete m_rdata;
        m_rdata = NULL;
    }
    m_rdata = rdata;
}

MDNSRData* MDNSResourceRecord::GetRData()
{
    return m_rdata;
}

//MDNSDefaultRData
size_t MDNSDefaultRData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    return 0;
}

size_t MDNSDefaultRData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    return 0;
}

size_t MDNSDefaultRData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    //
    // If there's not enough data in the buffer to even get the string size out
    // then bail.
    //
    if (bufsize < 2) {
        QCC_DbgPrintf(("MDNSDefaultRData::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }
    uint16_t rdlen = buffer[0] << 8 | buffer[1];
    bufsize -= 2;
    if (bufsize < rdlen) {
        QCC_DbgPrintf(("MDNSDefaultRData::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }
    return rdlen + 2;
}

//MDNSTextRData
const uint16_t MDNSTextRData::TXTVERS = 0;

MDNSTextRData::MDNSTextRData(uint16_t version, bool uniquifyKeys)
    : version(version), uniquifier(uniquifyKeys ? 1 : 0)
{
    m_fields["txtvers"] = U32ToString(version);
}

void MDNSTextRData::SetUniqueCount(uint16_t count)
{
    uniquifier = count;
}

uint16_t MDNSTextRData::GetUniqueCount()
{
    return uniquifier;
}

void MDNSTextRData::Reset()
{
    m_fields.clear();
    m_fields["txtvers"] = U32ToString(version);
    if (uniquifier) {
        uniquifier = 1;
    }
}

void MDNSTextRData::RemoveEntry(qcc::String key)
{
    m_fields.erase(key);
}

void MDNSTextRData::SetValue(String key, String value, bool shared)
{
    if (uniquifier && !shared) {
        key += "_" + U32ToString(uniquifier++);
    }
    m_fields[key] = value;
}

void MDNSTextRData::SetValue(String key, bool shared)
{
    if (uniquifier && !shared) {
        key += "_" + U32ToString(uniquifier++);
    }
    m_fields[key] = String();
}

String MDNSTextRData::GetValue(String key)
{
    if (m_fields.find(key) != m_fields.end()) {
        return m_fields[key];
    } else {
        return "";
    }
}

bool MDNSTextRData::HasKey(qcc::String key)
{
    if (m_fields.find(key) != m_fields.end()) {
        return true;
    } else {
        return false;
    }
}

uint16_t MDNSTextRData::GetU16Value(String key) {
    if (m_fields.find(key) != m_fields.end()) {
        return StringToU32(m_fields[key]);
    } else {
        return 0;
    }
}

uint16_t MDNSTextRData::GetNumFields(String key)
{
    key += "_";
    uint16_t numNames = 0;
    for (Fields::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find(key) == 0) {
            ++numNames;
        }
    }
    return numNames;
}

qcc::String MDNSTextRData::GetFieldAt(String key, int i)
{
    key += "_";
    Fields::const_iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find(key) == 0 && (i-- == 0)) {
            break;
        }
    }
    if (it != m_fields.end()) {
        return it->second;
    } else {
        return String();
    }
}

void MDNSTextRData::RemoveFieldAt(String key, int i)
{
    key += "_";
    Fields::const_iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find(key) == 0 && (i-- == 0)) {
            break;
        }
    }
    if (it != m_fields.end()) {
        MDNSTextRData::RemoveEntry(it->first);
    }
}

size_t MDNSTextRData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    size_t rdlen = 0;
    Fields::const_iterator it = m_fields.begin();

    while (it != m_fields.end()) {
        String str = it->first;
        if (!it->second.empty()) {
            str += "=" + it->second;
        }
        rdlen += str.length() + 1;
        it++;
    }
    return rdlen + 2;
}

size_t MDNSTextRData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    size_t rdlen = 0;
    uint8_t* p = &buffer[2];

    //
    // txtvers must appear first in the record
    //
    Fields::const_iterator txtvers = m_fields.find("txtvers");
    assert(txtvers != m_fields.end());
    String str = txtvers->first + "=" + txtvers->second;
    *p++ = str.length();
    memcpy(reinterpret_cast<void*>(p), const_cast<void*>(reinterpret_cast<const void*>(str.c_str())), str.length());
    p += str.length();
    rdlen += str.length() + 1;

    //
    // Then we rely on the Fields comparison object so make sure things are
    // serialized in the proper order
    //
    for (Fields::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it == txtvers) {
            continue;
        }
        String str = it->first;
        if (!it->second.empty()) {
            str += "=" + it->second;
        }
        *p++ = str.length();
        memcpy(reinterpret_cast<void*>(p), const_cast<void*>(reinterpret_cast<const void*>(str.c_str())), str.length());
        p += str.length();
        rdlen += str.length() + 1;
    }

    buffer[0] = (rdlen & 0xFF00) >> 8;
    buffer[1] = (rdlen & 0xFF);

    return rdlen + 2;
}

size_t MDNSTextRData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    //
    // If there's not enough data in the buffer to even get the string size out
    // then bail.
    //
    if (bufsize < 2) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }
    uint16_t rdlen = buffer[0] << 8 | buffer[1];

    bufsize -= 2;
    size_t size = 2 + rdlen;

    //
    // If there's not enough data in the buffer then bail.
    //
    if (bufsize < rdlen) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }
    uint8_t const* p = &buffer[2];
    while (rdlen > 0 && bufsize > 0) {
        uint8_t sz = *p++;
        String str;
        bufsize--;
        if (bufsize < sz) {
            QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Insufficient bufsize %d", bufsize));
            return 0;
        }
        str.assign(reinterpret_cast<const char*>(p), sz);
        size_t eqPos = str.find_first_of('=', 0);
        if (eqPos != String::npos) {
            m_fields[str.substr(0, eqPos)] = str.substr(eqPos + 1);
        } else {
            m_fields[str.substr(0, eqPos)] = String();
        }
        p += sz;
        rdlen -= sz + 1;
        bufsize -= sz;
    }

    if (rdlen != 0) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Mismatched RDLength"));
        return 0;
    }

    return size;
}

//MDNSARecord
void MDNSARData::SetAddr(qcc::String ipAddr)
{
    m_ipv4Addr = ipAddr;
}

qcc::String MDNSARData::GetAddr()
{
    return m_ipv4Addr;
}

size_t MDNSARData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    //4 bytes for address, 2 bytes length
    return 4 + 2;
}

size_t MDNSARData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    buffer[0] = 0;
    buffer[1] = 4;
    uint8_t* p = &buffer[2];
    QStatus status = qcc::IPAddress::StringToIPv4(m_ipv4Addr, p, 4);
    if (status != ER_OK) {
        QCC_DbgPrintf(("MDNSARData::Serialize Error occured during conversion of String to IPv4 address"));
        return 0;
    }
    return 6;
}

size_t MDNSARData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{

    if (bufsize < 6) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }
    if (buffer[0] != 0 || buffer[1] != 4) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Invalid RDLength"));
        return 0;

    }
    uint8_t const* p = &buffer[2];
    m_ipv4Addr = qcc::IPAddress::IPv4ToString(p);
    bufsize -= 6;
    return 6;
}

//MDNSAAAARecord
void MDNSAAAARData::SetAddr(qcc::String ipAddr)
{
    m_ipv6Addr = ipAddr;
}

qcc::String MDNSAAAARData::GetAddr() const
{
    return m_ipv6Addr;
}

size_t MDNSAAAARData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    //16 bytes for address, 2 bytes length
    return 16 + 2;
}

size_t MDNSAAAARData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    buffer[0] = 0;
    buffer[1] = 16;
    uint8_t* p = &buffer[2];
    qcc::IPAddress::StringToIPv6(m_ipv6Addr, p, 16);
    return 18;
}

size_t MDNSAAAARData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    if (bufsize < 18) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }
    if (buffer[0] != 0 || buffer[1] != 16) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Invalid RDLength"));
        return 0;
    }
    uint8_t const* p = &buffer[2];
    m_ipv6Addr = qcc::IPAddress::IPv6ToString(p);
    bufsize -= 18;
    return 18;
}

//MDNSPtrRData
void MDNSPtrRData::SetPtrDName(qcc::String rdataStr)
{
    m_rdataStr = rdataStr;
}

qcc::String MDNSPtrRData::GetPtrDName() const
{
    return m_rdataStr;
}

size_t MDNSPtrRData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    size_t size = 2;
    String name = m_rdataStr;
    while (true) {
        if (name.empty()) {
            size++;
            break;
        } else if (offsets.find(name) != offsets.end()) {
            size++;
            size++;
            break;
        } else {
            offsets[name] = 0; /* 0 is used as a placeholder so that the serialized size is computed correctly */
            size_t newPos = name.find_first_of('.');
            String temp = name.substr(0, newPos);
            size++;
            size += temp.length();
            size_t pos = (newPos == String::npos) ? String::npos : (newPos + 1);
            name = name.substr(pos);
        }
    }
    return size;
}

size_t MDNSPtrRData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    size_t size = 2;
    String name = m_rdataStr;
    while (true) {
        if (name.empty()) {
            buffer[size++] = 0;
            break;
        } else if (offsets.find(name) != offsets.end()) {
            buffer[size++] = 0xc0 | ((offsets[name] & 0xFF00) >> 8);
            buffer[size++] = (offsets[name] & 0xFF);
            break;
        } else {
            offsets[name] = size + headerOffset;
            size_t newPos = name.find_first_of('.');
            String temp = name.substr(0, newPos);
            buffer[size++] = temp.length();
            memcpy(reinterpret_cast<void*>(&buffer[size]), const_cast<void*>(reinterpret_cast<const void*>(temp.c_str())), temp.size());
            size += temp.length();
            size_t pos = (newPos == String::npos) ? String::npos : (newPos + 1);
            name = name.substr(pos);
        }
    }
    buffer[0] = ((size - 2) & 0xFF00) >> 8;
    buffer[1] = ((size - 2) & 0xFF);
    return size;
}

size_t MDNSPtrRData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    m_rdataStr.clear();
    //
    // If there's not enough data in the buffer to even get the string size out
    // then bail.
    //
    if (bufsize < 2) {
        QCC_DbgPrintf(("MDNSPtrRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }

    uint16_t szStr = buffer[0] << 8 | buffer[1];
    bufsize -= 2;

    size_t size = 2;

    vector<uint32_t> offsets;

    // If there's not enough data in the buffer then bail.
    //
    if (bufsize < szStr) {
        QCC_DbgPrintf(("MDNSPtrRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }

    while (bufsize > 0) {
        if (((buffer[size] & 0xc0) >> 6) == 3 && bufsize > 1) {
            uint32_t pointer = ((buffer[size] << 8 | buffer[size + 1]) & 0x3FFF);
            if (compressedOffsets.find(pointer) != compressedOffsets.end()) {
                if (m_rdataStr.length() > 0) {
                    m_rdataStr.append('.');
                }
                m_rdataStr.append(compressedOffsets[pointer]);
                size += 2;
                break;
            } else {
                return 0;
            }
        }
        size_t temp_size = buffer[size++];
        bufsize--;
        //
        // If there's not enough data in the buffer then bail.
        //
        if (bufsize < temp_size) {
            QCC_DbgPrintf(("MDNSPtrRecord::Deserialize(): Insufficient bufsize %d", bufsize));
            return 0;
        }
        if (m_rdataStr.length() > 0) {
            m_rdataStr.append('.');
        }
        if (temp_size > 0) {
            offsets.push_back(headerOffset + size - 1);
            m_rdataStr.append(reinterpret_cast<const char*>(buffer + size), temp_size);
            bufsize -= temp_size;
            size += temp_size;
        } else {
            break;
        }
    }

    for (uint32_t i = 0; i < offsets.size(); ++i) {
        compressedOffsets[offsets[i]] = m_rdataStr.substr(offsets[i] - 2 - headerOffset);
    }

    return size;
}

//MDNSSrvRData
MDNSSrvRData::MDNSSrvRData(uint16_t priority, uint16_t weight, uint16_t port, qcc::String target) :
    m_priority(priority),
    m_weight(weight),
    m_port(port)
{
    m_target.SetName(target);
}

void MDNSSrvRData::SetPriority(uint16_t priority)
{
    m_priority = priority;
}

uint16_t MDNSSrvRData::GetPriority()  const
{
    return m_priority;
}

void MDNSSrvRData::SetWeight(uint16_t weight)
{
    m_weight = weight;
}

uint16_t MDNSSrvRData::GetWeight()  const
{
    return m_weight;
}

void MDNSSrvRData::SetPort(uint16_t port)
{
    m_port = port;
}

uint16_t MDNSSrvRData::GetPort()  const
{
    return m_port;
}

void MDNSSrvRData::SetTarget(qcc::String target)
{
    m_target.SetName(target);
}

qcc::String MDNSSrvRData::GetTarget()  const
{
    return m_target.GetName();
}

size_t MDNSSrvRData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    //2 bytes length
    return 2 + 6 + m_target.GetSerializedSize(offsets);
}

size_t MDNSSrvRData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    //length filled in after we know it below

    //priority
    buffer[2] = (m_priority & 0xFF00) >> 8;
    buffer[3] = (m_priority & 0xFF);

    //weight
    buffer[4] = (m_weight & 0xFF00) >> 8;
    buffer[5] = (m_weight & 0xFF);

    //port
    buffer[6] = (m_port & 0xFF00) >> 8;
    buffer[7] = (m_port & 0xFF);
    size_t size = 2 + 6;

    uint8_t* p = &buffer[size];
    size += m_target.Serialize(p, offsets, headerOffset + size);

    uint16_t length = size - 2;
    buffer[0] = (length & 0xFF00) >> 8;
    buffer[1] = (length & 0xFF);

    return size;
}

size_t MDNSSrvRData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{

//
    // If there's not enough data in the buffer to even get the string size out
    // then bail.
    //
    if (bufsize < 2) {
        QCC_DbgPrintf(("MDNSSrvRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }

    uint16_t length = buffer[0] << 8 | buffer[1];
    bufsize -= 2;

    if (bufsize < length || length < 6) {
        QCC_DbgPrintf(("MDNSSrvRecord::Deserialize(): Insufficient bufsize %d or invalid length %d", bufsize, length));
        return 0;
    }
    m_priority = buffer[2] << 8 | buffer[3];
    bufsize -= 2;

    m_weight = buffer[4] << 8 | buffer[5];
    bufsize -= 2;

    m_port = buffer[6] << 8 | buffer[7];
    bufsize -= 2;

    size_t size = 8;
    headerOffset += 8;
    uint8_t const* p = &buffer[size];
    size += m_target.Deserialize(p, bufsize, compressedOffsets, headerOffset);

    return size;
}

//MDNSAdvertiseRecord
void MDNSAdvertiseRData::Reset()
{
    MDNSTextRData::Reset();
}

void MDNSAdvertiseRData::SetTransport(TransportMask tm)
{
    MDNSTextRData::SetValue("t", U32ToString(tm, 16));
}

void MDNSAdvertiseRData::AddName(qcc::String name)
{
    MDNSTextRData::SetValue("n", name);
}

void MDNSAdvertiseRData::SetValue(String key, String value)
{
    //
    // Commonly used keys name and implements get abbreviated over the air.
    //
    if (key == "name") {
        MDNSTextRData::SetValue("n", value);
    }  else if (key == "transport") {
        MDNSTextRData::SetValue("t", value);
    } else if (key == "implements") {
        MDNSTextRData::SetValue("i", value);
    } else {
        MDNSTextRData::SetValue(key, value);
    }
}

void MDNSAdvertiseRData::SetValue(String key)
{
    MDNSTextRData::SetValue(key);
}

uint16_t MDNSAdvertiseRData::GetNumFields()
{
    return m_fields.size();
}
uint16_t MDNSAdvertiseRData::GetNumNames(TransportMask transportMask)
{
    uint16_t numNames = 0;
    Fields::const_iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find("t_") != String::npos && (StringToU32(it->second, 16) == transportMask)) {
            it++;
            while (it != m_fields.end() && it->first.find("t_") == String::npos) {
                if (it->first.find("n_") != String::npos) {
                    numNames++;
                }
                it++;
            }
            break;
        }
    }
    return numNames;
}

qcc::String MDNSAdvertiseRData::GetNameAt(TransportMask transportMask, int index)
{
    Fields::const_iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find("t_") != String::npos && (StringToU32(it->second, 16) == transportMask)) {
            it++;
            while (it != m_fields.end() && it->first.find("t_") == String::npos) {
                if (it->first.find("n_") != String::npos && index-- == 0) {
                    return it->second;
                }
                it++;
            }
            break;
        }
    }
    return "";
}

void MDNSAdvertiseRData::RemoveNameAt(TransportMask transportMask, int index)
{
    Fields::const_iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find("t_") != String::npos && (StringToU32(it->second, 16) == transportMask)) {
            uint32_t numNames = 0;
            Fields::const_iterator trans = it;
            it++;
            while (it != m_fields.end() && it->first.find("t_") == String::npos) {
                Fields::const_iterator nxt = it;
                nxt++;
                if (it->first.find("n_") != String::npos) {
                    if (index-- == 0) {
                        MDNSTextRData::RemoveEntry(it->first);
                    } else {
                        numNames++;
                    }
                }
                it = nxt;
            }
            if (!numNames) {
                MDNSTextRData::RemoveEntry(trans->first);
            }
            break;
        }
    }

}
std::pair<qcc::String, qcc::String> MDNSAdvertiseRData::GetFieldAt(int i)
{
    Fields::const_iterator it = m_fields.begin();
    while (i-- && it != m_fields.end()) {
        ++it;
    }
    if (it == m_fields.end()) {
        return pair<String, String>("", "");
    }
    String key = it->first;
    key = key.substr(0, key.find_last_of('_'));
    if (key == "n") {
        key = "name";
    } else if (key == "i") {
        key = "implements";
    } else if (key == "t") {
        key = "transport";
    }
    return pair<String, String>(key, it->second);
}

//MDNSSearchRecord
MDNSSearchRData::MDNSSearchRData(qcc::String name, uint16_t version)
    : MDNSTextRData(version, true)
{
    MDNSTextRData::SetValue("n", name);
}

void MDNSSearchRData::SetValue(String key, String value)
{
    //
    // Commonly used keys name and implements get abbreviated over the air.
    //
    if (key == "name") {
        MDNSTextRData::SetValue("n", value);
    } else if (key == "implements") {
        MDNSTextRData::SetValue("i", value);
    } else if (key == "send_match_only" || key == "m") {
        MDNSTextRData::SetValue("m", value, true);
    } else {
        MDNSTextRData::SetValue(key, value);
    }
}
void MDNSSearchRData::SetValue(String key)
{
    MDNSTextRData::SetValue(key);
}

bool MDNSSearchRData::SendMatchOnly() {
    return MDNSTextRData::HasKey("m");
}

uint16_t MDNSSearchRData::GetNumFields()
{
    return m_fields.size();
}

uint16_t MDNSSearchRData::GetNumSearchCriteria() {
    int numEntries = GetNumFields() - 1 /*txtvers*/;

    return (numEntries > 0) ? (MDNSTextRData::GetNumFields(";") + 1) : 0;
}
String MDNSSearchRData::GetSearchCriterion(int index)
{
    Fields::const_iterator it = m_fields.begin();
    String ret = "";
    while (it != m_fields.end()) {
        String key = it->first;
        key = key.substr(0, key.find_last_of('_'));
        if (key == ";") {
            if (index-- == 0) {
                break;
            }
            ret = "";
        } else if (key != "txtvers") {
            if (key == "n") {
                key = "name";
            } else if (key == "i") {
                key = "implements";
            }
            if (!ret.empty()) {
                ret += ",";
            }
            ret += key + "='" + it->second + "'";
        }
        ++it;
    }
    return ret;


}
void MDNSSearchRData::RemoveSearchCriterion(int index)
{
    Fields::iterator it = m_fields.begin();
    String ret = "";
    while (it != m_fields.end() && index > 0) {
        String key = it->first;
        key = key.substr(0, key.find_last_of('_'));
        if (key == ";") {
            if (--index == 0) {
                it++;
                break;
            }
            ret = "";
        } else if (key != "txtvers") {
            if (key == "n") {
                key = "name";
            } else if (key == "i") {
                key = "implements";
            }
            if (!ret.empty()) {
                ret += ",";
            }
            ret += key + "='" + it->second + "'";
        }
        ++it;
    }
    if (it != m_fields.end()) {
        while (it != m_fields.end()) {
            String key = it->first;
            key = key.substr(0, key.find_last_of('_'));
            if (key == ";") {
                m_fields.erase(it);
                break;
            } else if (key == "txtvers") {
                it++;
            } else {
                m_fields.erase(it++);
            }
        }

    }
}
std::pair<qcc::String, qcc::String> MDNSSearchRData::GetFieldAt(int i)
{
    Fields::const_iterator it = m_fields.begin();
    while (i-- && it != m_fields.end()) {
        ++it;
    }
    if (it == m_fields.end()) {
        return pair<String, String>("", "");
    }

    String key = it->first;
    key = key.substr(0, key.find_last_of('_'));
    if (key == "n") {
        key = "name";
    } else if (key == "i") {
        key = "implements";
    }
    return pair<String, String>(key, it->second);
}

//MDNSPingRecord
MDNSPingRData::MDNSPingRData(qcc::String name, uint16_t version)
    : MDNSTextRData(version)
{
    MDNSTextRData::SetValue("n", name);
}

String MDNSPingRData::GetWellKnownName()
{
    return MDNSTextRData::GetValue("n");
}

void MDNSPingRData::SetWellKnownName(qcc::String name)
{
    MDNSTextRData::SetValue("n", name);
}

//MDNSPingReplyRecord
MDNSPingReplyRData::MDNSPingReplyRData(qcc::String name, uint16_t version)
    : MDNSTextRData(version)
{
    MDNSTextRData::SetValue("n", name);
}

String MDNSPingReplyRData::GetWellKnownName()
{
    return MDNSTextRData::GetValue("n");
}

void MDNSPingReplyRData::SetWellKnownName(qcc::String name)
{
    MDNSTextRData::SetValue("n", name);
}

String MDNSPingReplyRData::GetReplyCode()
{
    return MDNSTextRData::GetValue("replycode");
}

void MDNSPingReplyRData::SetReplyCode(qcc::String replyCode)
{
    MDNSTextRData::SetValue("replycode", replyCode);
}


//MDNSSenderRData
MDNSSenderRData::MDNSSenderRData(uint16_t version)
    : MDNSTextRData(version)
{
    MDNSTextRData::SetValue("pv", U32ToString(NS_VERSION));
    MDNSTextRData::SetValue("ajpv", U32ToString(ALLJOYN_PROTOCOL_VERSION));
}

uint16_t MDNSSenderRData::GetSearchID()
{
    return MDNSTextRData::GetU16Value("sid");
}

void MDNSSenderRData::SetSearchID(uint16_t searchId)
{
    MDNSTextRData::SetValue("sid", U32ToString(searchId));
}

uint16_t MDNSSenderRData::GetIPV4ResponsePort()
{
    return MDNSTextRData::GetU16Value("upcv4");
}

void MDNSSenderRData::SetIPV4ResponsePort(uint16_t ipv4Port)
{
    MDNSTextRData::SetValue("upcv4", U32ToString(ipv4Port));
}

qcc::String MDNSSenderRData::GetIPV4ResponseAddr()
{
    return MDNSTextRData::GetValue("ipv4");
}

void MDNSSenderRData::SetIPV4ResponseAddr(qcc::String ipv4Addr)
{
    MDNSTextRData::SetValue("ipv4", ipv4Addr);
}

//MDNSHeader
MDNSHeader::MDNSHeader() :
    m_queryId(0),
    m_qrType(0),
    m_authAnswer(0),
    m_rCode(NOT_ERROR),
    m_qdCount(0),
    m_anCount(0),
    m_nsCount(0),
    m_arCount(0)
{

}

MDNSHeader::MDNSHeader(uint16_t id, bool qrType, uint16_t qdCount, uint16_t anCount, uint16_t nsCount, uint16_t arCount) :
    m_queryId(id),
    m_qrType(qrType),
    m_authAnswer(0),
    m_rCode(NOT_ERROR),
    m_qdCount(qdCount),
    m_anCount(anCount),
    m_nsCount(nsCount),
    m_arCount(arCount)
{

}

MDNSHeader::MDNSHeader(uint16_t id, bool qrType) :
    m_queryId(id),
    m_qrType(qrType),
    m_authAnswer(0),
    m_rCode(NOT_ERROR),
    m_qdCount(0),
    m_anCount(0),
    m_nsCount(0),
    m_arCount(0)
{

}
MDNSHeader::~MDNSHeader()
{
}

void MDNSHeader::SetId(uint16_t id)
{
    m_queryId = id;
}

uint16_t MDNSHeader::GetId()
{
    return m_queryId;
}

void MDNSHeader::SetQRType(bool qrType)
{
    m_qrType = qrType;
}

bool MDNSHeader::GetQRType()
{
    return m_qrType;
}


void MDNSHeader::SetQDCount(uint16_t qdCount)
{
    m_qdCount = qdCount;
}
uint16_t MDNSHeader::GetQDCount()
{
    return m_qdCount;
}

void MDNSHeader::SetANCount(uint16_t anCount)
{
    m_anCount = anCount;
}
uint16_t MDNSHeader::GetANCount()
{
    return m_anCount;
}

void MDNSHeader::SetNSCount(uint16_t nsCount)
{
    m_nsCount = nsCount;
}
uint16_t MDNSHeader::GetNSCount()
{
    return m_nsCount;
}

void MDNSHeader::SetARCount(uint16_t arCount)
{
    m_arCount = arCount;
}

uint16_t MDNSHeader::GetARCount()
{
    return m_arCount;
}

size_t MDNSHeader::Serialize(uint8_t* buffer) const
{
    QCC_DbgPrintf(("MDNSHeader::Serialize(): to buffer 0x%x", buffer));

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first two octets are ID
    //
    buffer[0] = (m_queryId & 0xFF00) >> 8;
    buffer[1] = (m_queryId & 0xFF);
    size += 2;

    //
    // The third octet is |QR|   Opcode  |AA|TC|RD|RA|
    //
    // QR = packet type Query/Response.
    // Opcode = 0 a standard query (QUERY)
    // AA = 0 for now
    // TC = 0 for now
    // RD = 0 for now
    // RA = 0 for now
    buffer[2] = m_qrType << 7;
    QCC_DbgPrintf(("Header::Serialize(): Third octet = %x", buffer[2]));
    size += 1;

    //
    // The fourth octet is |   Z    |   RCODE   |
    // Z = reserved for future use. Must be zero.
    buffer[3] = m_rCode;
    QCC_DbgPrintf(("Header::Serialize(): RCode = %d", buffer[3]));
    size += 1;

    //
    // The next two octets are QDCOUNT
    //
    //assert((m_qdCount == 0) || (m_qdCount == 1));
    buffer[4] = 0;
    buffer[5] = m_qdCount;
    QCC_DbgPrintf(("Header::Serialize(): QDCOUNT = %d", buffer[5]));
    size += 2;


    //
    // The next two octets are ANCOUNT
    //
    //assert((m_anCount == 0) || (m_anCount == 1));
    buffer[6] = 0;
    buffer[7] = m_anCount;
    QCC_DbgPrintf(("Header::Serialize(): ANCOUNT = %d", buffer[7]));
    size += 2;

    //
    // The next two octets are NSCOUNT
    //
    buffer[8] = m_nsCount >> 8;
    buffer[9] = m_nsCount & 0xFF;
    QCC_DbgPrintf(("Header::Serialize(): NSCOUNT = %d", m_nsCount));
    size += 2;

    //
    // The next two octets are ARCOUNT
    //
    buffer[10] = m_arCount >> 8;
    buffer[11] = m_arCount & 0xFF;
    QCC_DbgPrintf(("Header::Serialize(): ARCOUNT = %d", m_arCount));
    size += 2;

    return size;
}

size_t MDNSHeader::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    //
    // If there's not enough room in the buffer to get the fixed part out then
    // bail (one byte of version, one byte of question count, one byte of answer
    // count and one byte of timer).
    //
    if (bufsize < 12) {
        QCC_DbgPrintf(("Header::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first two octets are ID
    //
    m_queryId = (buffer[0] << 8) | buffer[1];

    size += 2;

    //
    // The third octet is |QR|   Opcode  |AA|TC|RD|RA|
    //
    // QR = packet type Query/Response.
    // Opcode = 0 a standard query (QUERY)
    // AA = 0 for now
    // TC = 0 for now
    // RD = 0 for now
    // RA = 0 for now
    // Extract QR type
    m_qrType = buffer[2] >> 7;
    size += 1;

    //
    // The fourth octet is |   Z    |   RCODE   |
    // Z = reserved for future use. Must be zero.
    // Extract RCode.
    m_rCode = (RCodeType)(buffer[3] & 0x0F);
    size += 1;

    //
    // The next two octets are QDCOUNT
    //
    m_qdCount = buffer[5];
    //assert((m_qdCount == 0) || (m_qdCount == 1));
    size += 2;

    //
    // The next two octets are ANCOUNT
    //
    m_anCount = (buffer[6] << 8) | buffer[7];
    //assert((m_anCount == 0) || (m_anCount == 1));
    size += 2;

    //
    // The next two octets are NSCOUNT
    //
    m_nsCount = (buffer[8] << 8) | buffer[9];
    QCC_DbgPrintf(("Header::Deserialize(): NSCOUNT = %d", m_nsCount));
    size += 2;

    //
    // The next two octets are ARCOUNT
    //
    m_arCount = (buffer[10] << 8) | buffer[11];
    size += 2;

    bufsize -= 12;

    return size;
}

size_t MDNSHeader::GetSerializedSize(void) const
{
    //
    // We have 12 octets in the header.
    //
    return 12;
}

//MDNSPacket
_MDNSPacket::_MDNSPacket() {
    m_questions.reserve(MIN_RESERVE);
    m_answers.reserve(MIN_RESERVE);
    m_authority.reserve(MIN_RESERVE);
    m_additional.reserve(MIN_RESERVE);
}

_MDNSPacket::~_MDNSPacket() {
}

void _MDNSPacket::Clear()
{
    m_questions.clear();
    m_answers.clear();
    m_authority.clear();
    m_additional.clear();
}

void _MDNSPacket::SetHeader(MDNSHeader header)
{
    m_header = header;
}

MDNSHeader _MDNSPacket::GetHeader()
{
    return m_header;
}

void _MDNSPacket::AddQuestion(MDNSQuestion question)
{
    m_questions.push_back(question);
    assert(m_questions.size() <= MIN_RESERVE);
    m_header.SetQDCount(m_questions.size());
}

bool _MDNSPacket::GetQuestionAt(uint32_t i, MDNSQuestion** question)
{
    if (i >= m_questions.size()) {
        return false;
    }
    *question = &m_questions[i];
    return true;
}
bool _MDNSPacket::GetQuestion(qcc::String str, MDNSQuestion** question)
{
    std::vector<MDNSQuestion>::iterator it1 = m_questions.begin();
    while (it1 != m_questions.end()) {
        if (((*it1).GetQName() == str)) {
            *question = &(*it1);
            return true;
        }
        it1++;
    }
    return false;
}
uint16_t _MDNSPacket::GetNumQuestions()
{
    return m_questions.size();
}

void _MDNSPacket::AddAdditionalRecord(MDNSResourceRecord record)
{

    m_additional.push_back(record);
    assert(m_additional.size() <= MIN_RESERVE);
    m_header.SetARCount(m_additional.size());
}

bool _MDNSPacket::GetAdditionalRecordAt(uint32_t i, MDNSResourceRecord** additional)
{
    if (i >= m_additional.size()) {
        return false;
    }
    *additional = &m_additional[i];
    return true;
}

uint16_t _MDNSPacket::GetNumAdditionalRecords()
{
    return m_additional.size();
}

void _MDNSPacket::RemoveAdditionalRecord(qcc::String str, MDNSResourceRecord::RRType type)
{
    std::vector<MDNSResourceRecord>::iterator it1 = m_additional.begin();
    while (it1 != m_additional.end()) {
        if (((*it1).GetDomainName() == str) && ((*it1).GetRRType() == type)) {
            m_additional.erase(it1++);
            m_header.SetARCount(m_additional.size());
            return;
        }
        it1++;
    }
}

bool _MDNSPacket::GetAdditionalRecord(qcc::String str, MDNSResourceRecord::RRType type, MDNSResourceRecord** additional)
{
    size_t starPos = str.find_last_of('*');
    String name = str.substr(0, starPos);
    std::vector<MDNSResourceRecord>::iterator it1 = m_additional.begin();
    while (it1 != m_additional.end()) {
        String dname = (*it1).GetDomainName();
        bool nameMatches = (starPos == String::npos) ? (dname == name) : (dname.find(name) == 0);
        if (nameMatches && ((*it1).GetRRType() == type)) {
            *additional = &(*it1);
            return true;
        }
        it1++;
    }
    return false;
}

bool _MDNSPacket::GetAdditionalRecord(qcc::String str, MDNSResourceRecord::RRType type, uint16_t version, MDNSResourceRecord** additional)
{
    if (type != MDNSResourceRecord::TXT) {
        return false;
    }
    size_t starPos = str.find_last_of('*');
    String name = str.substr(0, starPos);
    std::vector<MDNSResourceRecord>::iterator it1 = m_additional.begin();
    while (it1 != m_additional.end()) {
        String dname = (*it1).GetDomainName();
        bool nameMatches = (starPos == String::npos) ? (dname == name) : (dname.find(name) == 0);
        if (nameMatches && ((*it1).GetRRType() == type) &&
            (static_cast<MDNSTextRData*>((*it1).GetRData())->GetU16Value("txtvers") == version)) {
            *additional = &(*it1);
            return true;
        }
        it1++;
    }
    return false;
}

uint32_t _MDNSPacket::GetNumMatches(qcc::String str, MDNSResourceRecord::RRType type, uint16_t version)
{
    if (type != MDNSResourceRecord::TXT) {
        return false;
    }
    uint32_t numMatches =  0;
    size_t starPos = str.find_last_of('*');
    String name = str.substr(0, starPos);
    std::vector<MDNSResourceRecord>::iterator it1 = m_additional.begin();
    while (it1 != m_additional.end()) {
        String dname = (*it1).GetDomainName();
        bool nameMatches = (starPos == String::npos) ? (dname == name) : (dname.find(name) == 0);
        if (nameMatches && ((*it1).GetRRType() == type) &&
            (static_cast<MDNSTextRData*>((*it1).GetRData())->GetU16Value("txtvers") == version)) {
            numMatches++;
        }
        it1++;
    }
    return numMatches;
}

bool _MDNSPacket::GetAdditionalRecordAt(qcc::String str, MDNSResourceRecord::RRType type, uint16_t version, uint32_t index, MDNSResourceRecord** additional)
{
    if (type != MDNSResourceRecord::TXT) {
        return false;
    }
    size_t starPos = str.find_last_of('*');
    String name = str.substr(0, starPos);
    uint32_t i = 0;
    std::vector<MDNSResourceRecord>::iterator it1 = m_additional.begin();
    while (it1 != m_additional.end()) {
        String dname = (*it1).GetDomainName();
        bool nameMatches = (starPos == String::npos) ? (dname == name) : (dname.find(name) == 0);
        if (nameMatches && ((*it1).GetRRType() == type) &&
            (static_cast<MDNSTextRData*>((*it1).GetRData())->GetU16Value("txtvers") == version)) {
            if (i++ == index) {
                *additional = &(*it1);
                return true;
            }
        }
        it1++;
    }
    return false;
}

bool _MDNSPacket::GetAnswer(qcc::String str, MDNSResourceRecord::RRType type, MDNSResourceRecord** answer)
{
    std::vector<MDNSResourceRecord>::iterator it1 = m_answers.begin();
    while (it1 != m_answers.end()) {
        if (((*it1).GetDomainName() == str) && ((*it1).GetRRType() == type)) {
            *answer = &(*it1);
            return true;
        }
        it1++;
    }
    return false;
}

bool _MDNSPacket::GetAnswer(qcc::String str, MDNSResourceRecord::RRType type, uint16_t version, MDNSResourceRecord** answer)
{
    if (type != MDNSResourceRecord::TXT) {
        return false;
    }
    std::vector<MDNSResourceRecord>::iterator it1 = m_answers.begin();
    while (it1 != m_answers.end()) {
        if (((*it1).GetDomainName() == str) && ((*it1).GetRRType() == type) &&
            (static_cast<MDNSTextRData*>((*it1).GetRData())->GetU16Value("txtvers") == version)) {
            *answer = &(*it1);
            return true;
        }
        it1++;
    }
    return false;
}

void _MDNSPacket::AddAnswer(MDNSResourceRecord record)
{
    m_answers.push_back(record);
    assert(m_answers.size() <= MIN_RESERVE);
    m_header.SetANCount(m_answers.size());
}

bool _MDNSPacket::GetAnswerAt(uint32_t i, MDNSResourceRecord** answer)
{
    if (i >= m_answers.size()) {
        return false;
    }
    *answer = &m_answers[i];
    return true;
}

uint16_t _MDNSPacket::GetNumAnswers()
{
    return m_answers.size();
}

size_t _MDNSPacket::GetSerializedSize(void) const
{
    std::map<qcc::String, uint32_t> offsets;
    size_t ret;

    size_t size = m_header.GetSerializedSize();

    std::vector<MDNSQuestion>::const_iterator it = m_questions.begin();
    while (it != m_questions.end()) {
        ret = (*it).GetSerializedSize(offsets);
        size += ret;
        it++;
    }

    std::vector<MDNSResourceRecord>::const_iterator it1 = m_answers.begin();
    while (it1 != m_answers.end()) {
        ret = (*it1).GetSerializedSize(offsets);
        size += ret;
        it1++;
    }

    it1 = m_authority.begin();
    while (it1 != m_authority.end()) {
        ret = (*it1).GetSerializedSize(offsets);
        size += ret;
        it1++;
    }

    it1 =  m_additional.begin();
    while (it1 != m_additional.end()) {
        ret = (*it1).GetSerializedSize(offsets);
        size += ret;
        it1++;
    }

    return size;
}

size_t _MDNSPacket::Serialize(uint8_t* buffer) const
{
    std::map<qcc::String, uint32_t> offsets;
    size_t ret;

    size_t size = m_header.Serialize(buffer);
    size_t headerOffset = size;

    uint8_t* p = &buffer[size];
    std::vector<MDNSQuestion>::const_iterator it = m_questions.begin();
    while (it != m_questions.end()) {
        ret = (*it).Serialize(p, offsets, headerOffset);
        size += ret;
        headerOffset += ret;
        p += ret;
        it++;
    }

    std::vector<MDNSResourceRecord>::const_iterator it1 = m_answers.begin();
    while (it1 != m_answers.end()) {
        ret = (*it1).Serialize(p, offsets, headerOffset);
        size += ret;
        headerOffset += ret;
        p += ret;
        it1++;

    }

    it1 = m_authority.begin();
    while (it1 != m_authority.end()) {
        ret = (*it1).Serialize(p, offsets, headerOffset);
        size += ret;
        headerOffset += ret;
        p += ret;
        it1++;
    }

    it1 =  m_additional.begin();
    while (it1 != m_additional.end()) {
        ret = (*it1).Serialize(p, offsets, headerOffset);
        size += ret;
        headerOffset += ret;
        p += ret;
        it1++;
    }

    return size;
}

size_t _MDNSPacket::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    Clear();
    std::map<uint32_t, qcc::String> compressedOffsets;
    size_t size = m_header.Deserialize(buffer, bufsize);
    size_t ret;
    if (size == 0) {
        QCC_DbgPrintf(("Error occured while deserializing header"));
        return size;
    }
    if (m_header.GetQRType() == MDNSHeader::MDNS_QUERY && m_header.GetQDCount() == 0) {
        //QRType = 0  and QDCount = 0. Invalid
        return 0;
    }
    if (size >= bufsize) {
        return 0;
    }
    bufsize -= size;
    uint8_t const* p = &buffer[size];
    size_t headerOffset = size;
    for (int i = 0; i < m_header.GetQDCount(); i++) {
        MDNSQuestion q;
        ret = q.Deserialize(p, bufsize, compressedOffsets, headerOffset);
        if (ret == 0 || ret > bufsize) {
            QCC_DbgPrintf(("Error while deserializing question"));
            return 0;
        }

        size += ret;
        bufsize -= ret;
        p += ret;
        headerOffset += ret;
        m_questions.push_back(q);

    }
    for (int i = 0; i < m_header.GetANCount(); i++) {
        MDNSResourceRecord r;
        ret = r.Deserialize(p, bufsize, compressedOffsets, headerOffset);
        if (ret == 0 || ret > bufsize) {
            QCC_DbgPrintf(("Error while deserializing answer"));
            return 0;
        }

        size += ret;
        bufsize -= ret;
        p += ret;
        headerOffset += ret;
        m_answers.push_back(r);
    }
    for (int i = 0; i < m_header.GetNSCount(); i++) {
        MDNSResourceRecord r;
        ret = r.Deserialize(p, bufsize, compressedOffsets, headerOffset);
        if (ret == 0 || ret > bufsize) {
            QCC_DbgPrintf(("Error while deserializing NS"));
            return 0;
        }

        size += ret;
        bufsize -= ret;
        p += ret;
        headerOffset += ret;
        m_authority.push_back(r);
    }
    for (int i = 0; i < m_header.GetARCount(); i++) {
        MDNSResourceRecord r;
        ret = r.Deserialize(p, bufsize, compressedOffsets, headerOffset);

        if (ret == 0 || ret > bufsize) {
            QCC_DbgPrintf(("Error while deserializing additional"));
            return 0;
        }

        size += ret;
        bufsize -= ret;
        p += ret;
        headerOffset += ret;
        m_additional.push_back(r);
    }
    return size;

}
TransportMask _MDNSPacket::GetTransportMask()
{
    TransportMask transportMask = TRANSPORT_NONE;
    if (GetHeader().GetQRType() == MDNSHeader::MDNS_QUERY) {
        MDNSQuestion* question;
        if (GetQuestion("_alljoyn._tcp.local.", &question)) {
            transportMask |= TRANSPORT_TCP;

        }
        if (GetQuestion("_alljoyn._udp.local.", &question)) {
            transportMask |= TRANSPORT_UDP;

        }
    } else { MDNSResourceRecord* answer;
             if (GetAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, &answer)) {
                 transportMask |= TRANSPORT_TCP;


             }
             if (GetAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR, &answer)) {
                 transportMask |= TRANSPORT_UDP;
             }
    }
    return transportMask;
}
void _MDNSPacket::RemoveAnswer(qcc::String str, MDNSResourceRecord::RRType type)
{
    std::vector<MDNSResourceRecord>::iterator it1 = m_answers.begin();
    while (it1 != m_answers.end()) {
        if (((*it1).GetDomainName() == str) && ((*it1).GetRRType() == type)) {
            m_answers.erase(it1++);
            m_header.SetANCount(m_answers.size());
            return;
        }
        it1++;
    }
}

void _MDNSPacket::RemoveQuestion(qcc::String str)
{
    std::vector<MDNSQuestion>::iterator it1 = m_questions.begin();
    while (it1 != m_questions.end()) {
        if (((*it1).GetQName() == str)) {
            m_questions.erase(it1);
            m_header.SetQDCount(m_questions.size());
            return;
        }
        it1++;
    }
}

} // namespace ajn
