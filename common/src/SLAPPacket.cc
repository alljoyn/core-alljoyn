/**
 * @file
 *
 * This file implements the Packet used within the SLAP protocol.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
#include <qcc/SLAPPacket.h>
#include <qcc/Debug.h>
#include <qcc/Util.h>
#include <assert.h>
#include <memory.h>

#define QCC_MODULE "SLAP"
using namespace qcc;


/*
 * link establishment packets
 */
static const char* LinkCtrlPacketNames[7] = { "NONE", "CONN", "ACPT", "NEGO", "NRSP", "DISC", "DRSP" };
static const size_t ExpectedLen[7] = { 0, 4, 4, 7, 7, 4, 4 };

const uint8_t BOUNDARY_BYTE         = 0xC0;
const uint8_t BOUNDARY_SUBSTITUTE   = 0xDC;
const uint8_t ESCAPE_BYTE           = 0xDB;
const uint8_t ESCAPE_SUBSTITUTE     = 0xDD;

#define CRC_INIT  0xFFFF
static void CRC16_Complete(uint16_t crc,
                           uint8_t* crcBlock)
{
    static const uint8_t rev[] = { 0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf };
    crcBlock[0] = (uint8_t) (rev[crc & 0xF] << 4) | rev[(crc >> 4) & 0xF];
    crcBlock[1] = (uint8_t) (rev[(crc >> 8) & 0xF] << 4) | rev[crc >> 12];
}

SLAPReadPacket::SLAPReadPacket(size_t packetSize) :
    m_maxPacketSize(packetSize),
    m_buffer(new uint8_t[SLAP_DESLIPPED_LENGTH(packetSize)]),
    m_readPtr(NULL), m_totalLen(0), m_remainingLen(0),
    m_readState(PACKET_NEW),
    m_packetType(INVALID_PACKET),
    m_controlType(UNKNOWN_PKT),
    m_ackNum(0), m_sequenceNum(0)
{
    memset(m_configField, '\0', 3);
}

SLAPReadPacket::~SLAPReadPacket()
{
    delete [] m_buffer;
}

void SLAPReadPacket::Clear()
{
    m_readPtr = NULL;
    m_totalLen = 0;
    m_remainingLen = 0;
    m_readState = PACKET_NEW;
    m_packetType = INVALID_PACKET;
    m_controlType = UNKNOWN_PKT;
    m_ackNum = 0;
    m_sequenceNum = 0;
    memset(m_configField, '\0', 3);
}

QStatus SLAPReadPacket::DeSlip(uint8_t*& bufIn, size_t& lenIn)
{
    uint8_t rx;

    QStatus status = ER_TIMEOUT;
    while (status == ER_TIMEOUT && lenIn-- > 0) {
        rx = *bufIn++;
        switch (m_readState) {
        case PACKET_FLUSH:
            /*
             * If we are not at a packet boundary as expected, then we need to flush
             * the receive system until we see a closing packet boundary.
             */
            if (rx == BOUNDARY_BYTE) {
                m_readState = PACKET_NEW;
            }
            break;

        case PACKET_NEW:
            /*
             * If we are not at a packet boundary as expected we need to flush
             * rx until we see a closing packet boundary.
             */
            if (rx == BOUNDARY_BYTE) {
                m_readState = PACKET_OPEN;
            } else {
                m_readState = PACKET_FLUSH;
                QCC_DbgPrintf(("SLAPReadPacket::DeSlip: Flushing input at %2x\n", rx));
            }
            m_totalLen = 0;
            m_packetType = INVALID_PACKET;
            m_controlType = UNKNOWN_PKT;
            break;

        case PACKET_ESCAPE:
            /*
             * Handle a SLIP escape sequence.
             */
            m_readState = PACKET_OPEN;
            if (rx == BOUNDARY_SUBSTITUTE) {
                m_buffer[m_totalLen++] = BOUNDARY_BYTE;
                break;
            }
            if (rx == ESCAPE_SUBSTITUTE) {
                m_buffer[m_totalLen++] = ESCAPE_BYTE;
                break;
            }
            QCC_DbgPrintf(("SLAPReadPacket::DeSlip: Bad escape sequence %2x\n", rx));
            /*
             * Bad escape sequence: discard everything up to the current
             * byte. This means that we need to restore the current byte.
             */
            ++lenIn;
            --bufIn;
            m_readState = PACKET_NEW;
            break;

        case PACKET_OPEN:
            /*
             * Decode received bytes and transfer them to the receive packet.
             */
            if (rx == BOUNDARY_BYTE) {
                status = ER_OK;
                break;
            }
            if (rx == ESCAPE_BYTE) {
                m_readState = PACKET_ESCAPE;
                break;
            }
            if (m_totalLen == (m_maxPacketSize + SLAP_HDR_LEN + SLAP_BOUNDARY_BYTES)) {
                /*
                 * Packet overrun: discard the packet.
                 */
                m_readState = PACKET_NEW;
                QCC_DbgPrintf(("SLAPReadPacket::DeSlip: Packet overrun %d\n", m_totalLen));
                break;
            }

            m_buffer[m_totalLen++] = rx;
            break;

        default:
            assert(0);
        }
    }

    /*
     * Only read as many bytes as we can consume.
     */
    return status;
}

QStatus SLAPReadPacket::Validate()
{
    uint8_t*rcvdCrc = NULL;
    uint8_t checkCrc[2];

    uint16_t expectedLen;

    if (m_totalLen < SLAP_MIN_PACKET_SIZE) {
        /*
         * Packet is too small.
         */
        QCC_LogError(ER_SLAP_INVALID_PACKET_LEN, ("Short packet %d\n", m_totalLen));
        return ER_SLAP_INVALID_PACKET_LEN;
    }
    /*
     * The last two bytes of the packet
     * are the CRC and are not counted in the packet header length.
     */
    QStatus status = ER_OK;
    /*
     * If we are checking data integrity validate the CRC.
     */
    m_totalLen -= SLAP_CRC_LEN;
    rcvdCrc = &m_buffer[m_totalLen];
    /*
     * Compute the CRC on the packet header and payload.
     */
    uint16_t crc = CRC_INIT;
    CRC16_Compute(m_buffer, m_totalLen, &crc);
    CRC16_Complete(crc, checkCrc);

    /*
     * Check the computed and received CRC's match.
     */
    if ((rcvdCrc[0] != checkCrc[0]) || (rcvdCrc[1] != checkCrc[1])) {
        QCC_LogError(ER_SLAP_CRC_ERROR, ("Data integrity error - discarding packet %X %X, %X %X", rcvdCrc[0], rcvdCrc[1], checkCrc[0], checkCrc[1]));
        status = ER_SLAP_CRC_ERROR;
    }

    if (status == ER_OK) {
        /*
         * Parse packet header.
         */
        m_ackNum = m_buffer[0] & 0x0F;
        m_sequenceNum = (m_buffer[0] >> 4) & 0x0F;
        m_packetType = (PacketType)(m_buffer[1] & 0x0F);
        if ((m_packetType != RELIABLE_DATA_PACKET) && (m_packetType != ACK_PACKET) && (m_packetType != CTRL_PACKET)) {
            m_packetType = INVALID_PACKET;
            status = ER_SLAP_INVALID_PACKET_TYPE;
        }
    }
    if (status == ER_OK) {
        /*
         * Check that the payload length in the header matches the bytes read.
         */
        expectedLen = (m_buffer[2] << 8) | m_buffer[3];
        if (expectedLen != (m_totalLen - SLAP_HDR_LEN)) {
            QCC_LogError(ER_SLAP_LEN_MISMATCH, ("Wrong packet length header says %d read %d bytes.\n", expectedLen, m_totalLen - SLAP_HDR_LEN));
            status = ER_SLAP_LEN_MISMATCH;
        }
    }

    if (status == ER_OK) {

        switch (m_packetType) {
        case RELIABLE_DATA_PACKET:

            /* Set the m_readPtr to the start of payload. */
            m_readPtr = m_buffer + SLAP_HDR_LEN;
            m_remainingLen = m_totalLen - SLAP_HDR_LEN;

            break;

        case ACK_PACKET:
            break;

        case CTRL_PACKET:
            if (status == ER_OK) {
                status = ER_SLAP_ERROR;

                for (uint8_t i = 1; i < sizeof(LinkCtrlPacketNames) / sizeof(char*); i++) {
                    if ((memcmp(&m_buffer[4], LinkCtrlPacketNames[i], SLAP_CTRL_PAYLOAD_HDR_SIZE) == 0)) {
                        if ((m_totalLen - SLAP_HDR_LEN) == ExpectedLen[i]) {
                            m_controlType = static_cast<ControlPacketType>(i);
                            status = ER_OK;
                            if ((m_controlType ==  NEGO_PKT) || (m_controlType == NEGO_RESP_PKT)) {
                                memcpy(m_configField, &m_buffer[8], 3);
                                QCC_DbgPrintf(("SLAP Received control packet %s. m_configField = %X %X %X", LinkCtrlPacketNames[m_controlType], m_configField[0], m_configField[1], m_configField[2]));
                            } else {
                                QCC_DbgPrintf(("SLAP Received control packet %s.", LinkCtrlPacketNames[m_controlType]));

                            }
                        } else {
                            status = ER_SLAP_INVALID_PACKET_LEN;
                        }
                        break;
                    }

                }
            }

            break;

        default:
            status = ER_SLAP_ERROR;
            break;
        }
    }
    return status;
}

bool SLAPReadPacket::FillBuffer(void* buf, size_t reqBytes, size_t& actualBytes)
{
    uint16_t toRead = reqBytes;
    actualBytes = (toRead < m_remainingLen) ? toRead : m_remainingLen;
    memcpy(buf, m_readPtr, actualBytes);

    if (actualBytes == m_remainingLen) {
        Clear();
        return true;
    } else {
        m_readPtr += actualBytes;
        m_remainingLen -= actualBytes;
        return false;
    }

}
SLAPWritePacket::SLAPWritePacket(size_t packetSize) :
    m_maxPacketSize(packetSize), m_ackNum(0), m_sequenceNum(0),
    m_payloadBuffer(new uint8_t[packetSize]),
    m_payloadLen(0),
    m_buffer(new uint8_t[SLAP_SLIPPED_LENGTH(packetSize)]),
    m_bufEOD(m_buffer), m_startPos(0),
    m_writePtr(m_buffer), m_slippedLen(0),
    m_endPos(0),
    m_pktType(INVALID_PACKET)
{
}

SLAPWritePacket::~SLAPWritePacket()
{
    delete [] m_buffer;
    delete [] m_payloadBuffer;
}

void SLAPWritePacket::Clear()
{
    m_ackNum = 0;
    m_sequenceNum = 0;
    m_payloadLen = 0;
    m_bufEOD = m_buffer;
    m_startPos = 0;
    m_writePtr = m_buffer;
    m_slippedLen = 0;
    m_endPos = 0;
    m_pktType = INVALID_PACKET;
}

void SLAPWritePacket::DataPacket(const void* buf, size_t num, size_t& numSent)
{
    m_pktType = RELIABLE_DATA_PACKET;
    m_payloadLen = (num < m_maxPacketSize) ? num : m_maxPacketSize;
    num -= m_payloadLen;
    memcpy(m_payloadBuffer, buf, m_payloadLen);
    numSent  = m_payloadLen;
    SlipPayload();
}

void SLAPWritePacket::SlipPayload() {

    m_slippedLen = SLAP_PAYLOAD_START_POS;
    for (int i = 0; i < m_payloadLen; i++) {
        uint8_t b = m_payloadBuffer[i];
        switch (b) {
        case BOUNDARY_BYTE:
            m_buffer[m_slippedLen++] = ESCAPE_BYTE;
            m_buffer[m_slippedLen++] = BOUNDARY_SUBSTITUTE;
            break;

        case ESCAPE_BYTE:
            m_buffer[m_slippedLen++] = ESCAPE_BYTE;
            m_buffer[m_slippedLen++] = ESCAPE_SUBSTITUTE;
            break;

        default:
            m_buffer[m_slippedLen++] = b;
            break;
        }
    }
}
void SLAPWritePacket::AckPacket()
{
    m_payloadLen = 0;
    m_pktType = ACK_PACKET;
    SlipPayload();

}
void SLAPWritePacket::ControlPacket(ControlPacketType type, uint8_t* configField)
{
    m_pktType = CTRL_PACKET;
    m_payloadLen = 4;
    memcpy(m_payloadBuffer, LinkCtrlPacketNames[type], 4);

    if (type == NEGO_PKT || type == NEGO_RESP_PKT) {
        assert(configField);
        memcpy(&m_payloadBuffer[4], configField, 3);
        m_payloadLen += 3;
        QCC_DbgPrintf(("SLAP Sending control packet %s. m_configField = %X %X %X", LinkCtrlPacketNames[type], configField[0], configField[1], configField[2]));

    }
    SlipPayload();
}
QStatus SLAPWritePacket::PrependHeader()
{
    uint8_t header[4];

    header[0] = ((m_pktType == RELIABLE_DATA_PACKET) ? (m_sequenceNum << 4) : 0x00);
    header[0] |= (m_pktType != CTRL_PACKET) ? m_ackNum : 0x00;

    /* Flow off is for future use. */
    header[1] = m_pktType;

    /*
     * high-order 8 bits of packet size
     */
    header[2] = (m_payloadLen >> 8);
    /*
     * low-order 8 bits of packet size
     */
    header[3] = (m_payloadLen & 0xFF);

    m_startPos = SLAP_PAYLOAD_START_POS - 1;
    for (int i = 3; i >= 0; i--) {
        switch (header[i]) {
        case BOUNDARY_BYTE:
            m_buffer[m_startPos--] = BOUNDARY_SUBSTITUTE;
            m_buffer[m_startPos--] = ESCAPE_BYTE;
            break;

        case ESCAPE_BYTE:
            m_buffer[m_startPos--] = ESCAPE_SUBSTITUTE;
            m_buffer[m_startPos--] = ESCAPE_BYTE;
            break;

        default:
            m_buffer[m_startPos--] = header[i];
            break;
        }

    }
    m_endPos = m_slippedLen;
    /* Add CRC to every packet */
    uint16_t crc = CRC_INIT;
    CRC16_Compute(&header[0], 4, &crc);
    CRC16_Compute(m_payloadBuffer, m_payloadLen, &crc);
    uint8_t checkCrc[2];
    CRC16_Complete(crc, checkCrc);
    for (int i = 0; i < 2; i++) {
        switch (checkCrc[i]) {
        case BOUNDARY_BYTE:
            m_buffer[m_endPos++] = ESCAPE_BYTE;
            m_buffer[m_endPos++] = BOUNDARY_SUBSTITUTE;
            break;

        case ESCAPE_BYTE:
            m_buffer[m_endPos++] = ESCAPE_BYTE;
            m_buffer[m_endPos++] = ESCAPE_SUBSTITUTE;
            break;

        default:
            m_buffer[m_endPos++] = checkCrc[i];
            break;
        }

    }
    m_buffer[m_startPos] = BOUNDARY_BYTE;
    m_buffer[m_endPos] = BOUNDARY_BYTE;
    m_writePtr = &m_buffer[m_startPos];
    m_bufEOD = &m_buffer[m_endPos];
    return ER_OK;
}
QStatus SLAPWritePacket::Deliver(Stream* link)
{
    size_t actual;
    QStatus status = link->PushBytes(m_writePtr, m_bufEOD - m_writePtr + 1, actual);
    m_writePtr += actual;
    return status;
}

