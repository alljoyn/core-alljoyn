/**
 * @file
 * PacketEngine converts Streams to packets and vice-versa.
 */

/******************************************************************************
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>

#include <algorithm>
#include <limits>
#include <cstring>

#include <qcc/Util.h>
#include <qcc/time.h>

#include "Packet.h"
#include "PacketStream.h"

#if defined(QCC_OS_DARWIN)

#include <CoreFoundation/CFByteOrder.h>
#define htole16(x) CFSwapInt16HostToLittle(x)
#define letoh16(x) CFSwapInt16LittleToHost(x)
#define htole32(x) CFSwapInt32HostToLittle(x)
#define letoh32(x) CFSwapInt32LittleToHost(x)
#define htole64(x) CFSwapInt64HostToLittle(x)
#define letoh64(x) CFSwapInt64LittleToHost(x)

#endif

#define QCC_MODULE "PACKET"

using namespace std;
using namespace qcc;

namespace ajn {

#define CHAN_ID_OFFSET      0
#define SEQ_NUM_OFFSET      4
#define GAP_OFFSET          6
#define VERSION_OFFSET      8
#define FLAGS_OFFSET        9
#define CRC_OFFSET         10
#define TTL_OFFSET         12
#define PAYLOAD_OFFSET     16     /* Must be 4-byte aligned */

#define PACKET_ENGINE_VERSION 1

const size_t Packet::payloadOffset = PAYLOAD_OFFSET;

Packet::Packet(size_t _mtu) :
    chanId(0),
    seqNum(0),
    gap(0),
    flags(0),
    payloadLen(0),
    payload(NULL),
    buffer(new uint32_t[(_mtu + sizeof(uint32_t) - 1) / sizeof(uint32_t)]),
    expireTs(0),
    sendTs(0),
    sendAttempts(0),
    fastRetransmit(false),
    mtu(_mtu),
    crc16(0),
    version(0)
{
}

Packet::Packet(const Packet& other) :
    chanId(other.chanId),
    seqNum(other.seqNum),
    gap(other.gap),
    flags(other.flags),
    payloadLen(other.payloadLen),
    payload(other.payload),
    buffer(new uint32_t[(other.mtu + sizeof(uint32_t) - 1) / sizeof(uint32_t)]),
    expireTs(other.expireTs),
    sendTs(other.sendTs),
    sendAttempts(other.sendAttempts),
    fastRetransmit(other.fastRetransmit),
    mtu(other.mtu),
    crc16(other.crc16),
    version(other.version)
{
}

Packet& Packet::operator=(const Packet& other)
{
    if (&other != this) {
        chanId = other.chanId;
        seqNum = other.seqNum;
        gap = other.gap;
        flags = other.flags;
        payloadLen = other.payloadLen;
        payload = other.payload;
        if (mtu != other.mtu) {
            delete buffer;
            buffer = new uint32_t[(other.mtu + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
        }
        expireTs = other.expireTs;
        sendTs = other.sendTs;
        sendAttempts = other.sendAttempts;
        fastRetransmit = other.fastRetransmit;
        mtu = other.mtu;
        crc16 = other.crc16;
        version = other.version;
    }
    return *this;
}

Packet::~Packet()
{
    delete[] buffer;
}

size_t Packet::SetPayload(const void* _payload, size_t _payloadLen)
{
    if (!_payload) {
        _payloadLen = 0;
    }
    payloadLen = std::min(_payloadLen, mtu - PAYLOAD_OFFSET);
    if (_payload) {
        payload = buffer + (PAYLOAD_OFFSET / sizeof(uint32_t));
        if (payload != _payload) {
            ::memmove(payload, _payload, payloadLen);
        }
    }
    return payloadLen;
}

QStatus Packet::Unmarshal(PacketSource& source)
{
    /* Get bytes from source */
    size_t actBytes;
    QStatus status = source.PullPacketBytes(buffer, mtu, actBytes, sender, 3000);
    uint8_t* tBuf = reinterpret_cast<uint8_t*>(buffer);

    if (actBytes < PAYLOAD_OFFSET) {
        status = ER_PACKET_BAD_FORMAT;
    }

    if (status == ER_OK) {
        /* Crc check */
        uint16_t crc = 0;
        uint16_t packetCrc = letoh16(*reinterpret_cast<uint16_t*>(tBuf + CRC_OFFSET));
        CRC16_Compute(tBuf, CRC_OFFSET, &crc);
        CRC16_Compute(tBuf + PAYLOAD_OFFSET, actBytes - PAYLOAD_OFFSET, &crc);
        status = (crc == packetCrc) ? ER_OK : ER_PACKET_BAD_CRC;
    }

    if (status == ER_OK) {
        chanId = letoh32(*reinterpret_cast<uint32_t*>(tBuf + CHAN_ID_OFFSET));
        seqNum = letoh16(*reinterpret_cast<uint16_t*>(tBuf + SEQ_NUM_OFFSET));
        gap = letoh16(*reinterpret_cast<uint16_t*>(tBuf + GAP_OFFSET));
        version = tBuf[VERSION_OFFSET];
        flags = tBuf[FLAGS_OFFSET];
        uint32_t ttl = letoh32(*reinterpret_cast<uint32_t*>(tBuf + TTL_OFFSET));
        payload = reinterpret_cast<uint32_t*>(tBuf + PAYLOAD_OFFSET);
        payloadLen = actBytes - PAYLOAD_OFFSET;
        expireTs = (ttl == numeric_limits<uint32_t>::max()) ? numeric_limits<uint64_t>::max() : GetTimestamp64() + ttl;
    } else {
        chanId = 0;
        seqNum = 0;
        gap = 0;
        version = 0;
        flags = 0;
        payload = NULL;
        payloadLen = 0;
        expireTs = 0;
    }

    return status;
}

void Packet::Marshal()
{
    assert(payloadLen <= (mtu - PAYLOAD_OFFSET));

    uint8_t* tBuf = reinterpret_cast<uint8_t*>(buffer);
    *reinterpret_cast<uint32_t*>(tBuf + CHAN_ID_OFFSET) = htole32(chanId);
    *reinterpret_cast<uint16_t*>(tBuf + SEQ_NUM_OFFSET) = htole16(seqNum);
    *reinterpret_cast<uint16_t*>(tBuf + GAP_OFFSET) = htole16(gap);
    *(tBuf + VERSION_OFFSET) = PACKET_ENGINE_VERSION;
    *(tBuf + FLAGS_OFFSET) = flags;
    uint64_t now = GetTimestamp64();
    uint32_t ttl = 0;
    if (expireTs == numeric_limits<uint64_t>::max()) {
        ttl = numeric_limits<uint32_t>::max();
    } else if (now < expireTs) {
        if ((expireTs - now) > numeric_limits<uint32_t>::max()) {
            ttl = numeric_limits<uint32_t>::max();
        } else {
            ttl = static_cast<uint32_t>(expireTs - now);
        }
    }
    *reinterpret_cast<uint32_t*>(tBuf + TTL_OFFSET) = htole32(ttl);
    if ((tBuf + PAYLOAD_OFFSET) != reinterpret_cast<uint8_t*>(payload)) {
        ::memmove(tBuf + PAYLOAD_OFFSET, payload, payloadLen);
    }
    uint16_t crc = 0;
    CRC16_Compute(tBuf, CRC_OFFSET, &crc);
    if (payloadLen) {
        CRC16_Compute(tBuf + PAYLOAD_OFFSET, payloadLen, &crc);
    }
    *reinterpret_cast<uint16_t*>(tBuf + CRC_OFFSET) = htole16(crc);
}

void Packet::Clean()
{
    chanId = 0;
    seqNum = 0;
    gap = 0;
    flags = 0;
    payloadLen = 0;
    payload = NULL;
    expireTs = 0;
    sendTs = 0;
    sendAttempts = 0;
    fastRetransmit = false;
    crc16 = 0;
    version = 0;
}

PacketDest GetPacketDest(const qcc::String& addr, uint16_t port)
{
    PacketDest pd;
    ::memset((uint8_t*)(&pd), 0, sizeof(PacketDest));
    qcc::IPAddress tmpIpAddr(addr);
    tmpIpAddr.RenderIPBinary(pd.ip, IPAddress::IPv6_SIZE);
    pd.addrSize = tmpIpAddr.Size();
    pd.port = port;
    return pd;
}

PacketDest GetPacketDest(const qcc::IPAddress& addr, uint16_t port)
{
    PacketDest pd;
    qcc::IPAddress tmpIpAddr(addr);
    tmpIpAddr.RenderIPBinary(pd.ip, IPAddress::IPv6_SIZE);
    pd.addrSize = tmpIpAddr.Size();
    pd.port = port;
    return pd;
}

void GetAddressAndPort(const PacketDest& dest, qcc::IPAddress& addr, uint16_t& port)
{
    IPAddress ipAddr(dest.ip, dest.addrSize);
    addr = ipAddr;
    port = dest.port;
}

}
