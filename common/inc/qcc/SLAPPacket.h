/**
 * @file
 *
 * This file defines the Packet used within the SLAP protocol.
 */

/******************************************************************************
 *
 *
 * Copyright AllSeen Alliance. All rights reserved.
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
#ifndef _QCC_SLAPPACKET_H
#define _QCC_SLAPPACKET_H
#include <qcc/platform.h>
#include <qcc/Stream.h>
#include <Status.h>


#define SLAP_HDR_LEN 4U

#define SLAP_BOUNDARY_BYTES 2
#define SLAP_CRC_LEN 2
/* The SLAP packets start with a Boundary byte and are followed with 4 Header bytes(Slipped length=8) */
#define SLAP_PAYLOAD_START_POS 9

/* Calculate SLAP packet sizes from payload sizes. */
#define SLAP_SLIPPED_LENGTH(payloadSize) ((SLAP_HDR_LEN + payloadSize + SLAP_CRC_LEN) * 2 + SLAP_BOUNDARY_BYTES)
#define SLAP_DESLIPPED_LENGTH(payloadSize) (SLAP_HDR_LEN + payloadSize + SLAP_BOUNDARY_BYTES)

#define SLAP_CTRL_PAYLOAD_HDR_SIZE      4
#define SLAP_MIN_PACKET_SIZE            (SLAP_HDR_LEN + SLAP_CRC_LEN)

namespace qcc {

/** Different packet types supported by this stream */
enum PacketType {
    INVALID_PACKET = -1,
    RELIABLE_DATA_PACKET = 0,
    CTRL_PACKET = 14,
    ACK_PACKET = 15
};

/** Different Control packet types supported by this stream */
enum ControlPacketType {
    UNKNOWN_PKT = 0,
    CONN_PKT = 1,
    ACCEPT_PKT = 2,
    NEGO_PKT = 3,
    NEGO_RESP_PKT = 4,
    DISCONN_PKT = 5,
    DISCONN_RESP_PKT = 6
};

class SLAPReadPacket {
  public:
    /**
     * Construct a SLAPReadPacket
     * @param packetSize The max allowed packetSize.
     */
    SLAPReadPacket(size_t packetSize);

    /** Destructor */
    ~SLAPReadPacket();

    /**
     * Deslip the input buffer into this packet
     * @param bufIn[in,out]  input buffer
     * @param lenIn[in,out]  length of input buffer
     * @return ER_OK if packet is complete
     *         ER_TIMEOUT more bytes are required to complete this read packet
     */
    QStatus DeSlip(uint8_t*& bufIn, size_t& lenIn);

    /**
     * Validate the packet that has just been read.
     * @return ER_OK if packet is verified
     *         other - if error in packet
     */
    QStatus Validate();

    /** Clear the packet */
    void Clear();

    /**
     * Get the packet type
     * @return The packet type of this packet. One of enum PacketType values.
     *
     */
    PacketType GetPacketType() { return m_packetType; };

    /**
     * Get the control packet type
     * @return The control packet type of this packet. One of enum ControlPacketType values.
     *
     */
    ControlPacketType GetControlType() { return m_controlType; };

    /**
     * Get the Ack number in this packet
     * @return The Ack number.
     */
    uint8_t GetAckNum() { return m_ackNum; }

    /**
     * Get the sequence number in this packet
     * @return          The sequence number.
     */
    uint8_t GetSeqNum() { return m_sequenceNum; }

    /**
     * Get the config field in this packet
     * @return          The config field value.
     */
    uint8_t GetConfigField(uint8_t index) { return (index < 3) ? m_configField[index] : 0; }

    /**
     * Fill the buffer with the requested number of bytes.
     * @param buf               Buffer to fill
     * @param reqBytes          Number of bytes requested
     * @param actualBytes[out]  Actual number of bytes copied
     * @return                  true - if this packet is exhausted i.e. no more bytes remaining in the packet
     *                          false - if there are bytes remaining in the packet
     */
    bool FillBuffer(void* buf, size_t reqBytes, size_t& actualBytes);

  private:

    /** Private default constructor - does nothing */
    SLAPReadPacket();

    /**
     * Private Copy-constructor - does nothing
     *
     * @param other  SLAPReadPacket to copy from.
     */
    SLAPReadPacket(const SLAPReadPacket& other);

    /**
     * Private Assignment operator - does nothing.
     *
     * @param other  SLAPReadPacket to assign from.
     */
    SLAPReadPacket operator=(const SLAPReadPacket&);

    enum PACKET_READ_STATE {
        PACKET_NEW,
        PACKET_OPEN,
        PACKET_FLUSH,
        PACKET_ESCAPE
    };
    size_t m_maxPacketSize;               /**< Maximum packet size supported. */
    uint8_t* m_buffer;                    /**< Buffer of deslipped bytes. */
    uint8_t* m_readPtr;                   /**< Current read position in the buffer. */
    uint16_t m_totalLen;                  /**< Total length of received packet. */
    uint16_t m_remainingLen;              /**< Bytes remaining to be read in the packet. */
    PACKET_READ_STATE m_readState;        /**< The current state of the packet during read. */

    PacketType m_packetType;              /**< Packet type */
    ControlPacketType m_controlType;      /**< Control Packet type */

    uint8_t m_ackNum;                     /**< Acknowledge number contained in this packet */
    uint8_t m_sequenceNum;                /**< Sequence number contained in this packet */

    uint8_t m_configField[3];                /**< Config field in the packet. This is sent as a part of CONF_PKT and CONF_RESP_PKT */
};
class SLAPWritePacket {
  public:
    /**
     * Construct a SLAPWritePacket
     * @param packetSize The max allowed packetSize.
     */
    SLAPWritePacket(size_t packetSize);

    /** Destructor */
    ~SLAPWritePacket();

    /** Clear the packet */
    void Clear();

    /**
     * Construct the SLAPWritePacket(Data).
     * @param buf          Input buffer
     * @param num          Length of Input buffer
     * @param numSent[out] Number of bytes that were actually copied in
     * @return
     *
     */
    void DataPacket(const void* buf, size_t num, size_t& numSent);

    /**
     * Construct the SLAPWritePacket(Control).
     * @param type ControlPacketType to use
     * @param val Optional field value (Used in case of CONF_PKT)
     *
     */
    void ControlPacket(ControlPacketType type, uint8_t* configField);

    /**
     * Construct the SLAPWritePacket(Ack).
     */
    void AckPacket();


    /**
     * Prepend the header to the data/control/ack packet.
     */
    QStatus PrependHeader();

    /**
     * Deliver this packet to a link.
     * @param link	The link to deliver this packet to.
     */
    QStatus Deliver(Stream* link);

    /**
     * Set the sequence number in this packet
     * @param           The sequence number.
     */
    void SetSeqNum(uint8_t seq) { m_sequenceNum  = seq; };

    /**
     * Set the Ack number in this packet
     * @param           The ack number.
     */
    void SetAck(uint8_t num) { m_ackNum = num; };

    /**
     * Get the packet type
     * @return The packet type of this packet. One of enum PacketType values.
     *
     */
    uint8_t GetPacketType() { return m_pktType; };

    /**
     * Get the sequence number in this packet
     * @return          The sequence number.
     */
    uint8_t GetSeqNum() { return m_sequenceNum; };

    /**
     * Get the Ack number in this packet
     * @return The Ack number.
     */
    uint8_t GetAckNum() { return m_ackNum; };


    /**
     * Slip the payload bytes into the buffer to send
     */
    void SlipPayload();
  private:

    /** Private default constructor - does nothing */
    SLAPWritePacket();

    /**
     * Private Copy-constructor - does nothing
     *
     * @param other  SLAPWritePacket to copy from.
     */
    SLAPWritePacket(const SLAPWritePacket& other);

    /**
     * Private Assignment operator - does nothing.
     *
     * @param other  SLAPWritePacket to assign from.
     */
    SLAPWritePacket operator=(const SLAPWritePacket&);

    size_t m_maxPacketSize;               /**< Maximum packet size supported. */

    uint8_t m_ackNum;                     /**< Acknowledge number contained in this packet */
    uint8_t m_sequenceNum;                /**< Sequence number contained in this packet */

    uint8_t* m_payloadBuffer;             /**< payload from app */
    uint16_t m_payloadLen;                /**< payload length */

    uint8_t* m_buffer;                    /**< slipped bytes */
    uint8_t* m_bufEOD;
    uint16_t m_startPos;                  /**< Starting position in the slipped buffer */
    uint8_t* m_writePtr;                  /**< Pointer to the current write position in the buffer. */
    uint16_t m_slippedLen;                /**< length of slipped bytes buffer */
    uint16_t m_endPos;                    /**< length of slipped bytes buffer */
    PacketType m_pktType;                 /**< Type of this SLAPWritePacket */

};
}
#endif

