/**
 * @file
 *
 * This file defines the Stream with the SLAP protocol for error detection,
 * flow control and retransmission.
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

#ifndef _QCC_SLAPSTREAM_H
#define _QCC_SLAPSTREAM_H

#include <qcc/platform.h>
#include <qcc/Stream.h>
#include <qcc/UARTStream.h>
#include <qcc/Timer.h>
#include <qcc/SLAPPacket.h>
#include <list>

namespace qcc {
class SLAPStream : public Stream, public UARTReadListener, public AlarmListener {
  public:

    /**
     * Constructor
     * @param link	        The link associated with this stream.
     * @param timer             The timer to be used for triggering alarms associated with this stream.
     * @param baudrate          The baudrate for this stream.
     * @param maxPacketSize     The maximum packet size to be supported by the stream.
     * @param maxWindowSize     The maximum window size to be supported by the stream.
     */
    SLAPStream(Stream* rawStream, Timer& timer, uint16_t maxPacketSize, uint16_t maxWindowSize, uint32_t baudrate);

    /** Destructor */
    ~SLAPStream();

    /** Close this stream and the associated link */
    void Close();

    /**
     * Set the send timeout for this sink.
     *
     * @param m_sendTimeout   Send timeout in ms.
     */
    void SetSendTimeout(uint32_t sendTimeout) { this->m_sendTimeout = sendTimeout; }

    /**
     * Push zero or more bytes into the sink with infinite ttl.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param numSent      Number of bytes actually consumed by sink.
     * @return   ER_OK if successful.
     */
    QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent);
    /**
     * Get the Event that indicates when data can be pushed to sink.
     *
     * @return Event that is signaled when sink can accept more bytes.
     */
    Event& GetSinkEvent() { return m_sinkEvent; }

    /**
     * Pull bytes from the source.
     * The source is exhausted when ER_EOF is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @param timeout      Time to wait to pull the requested bytes.
     * @return   ER_OK if successful. ER_EOF if source is exhausted. Otherwise an error.
     */
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER);
    /**
     * Get the Event indicating that data is available when signaled.
     *
     * @return Event that is signaled when data is available.
     */
    Event& GetSourceEvent() { return m_sourceEvent; }

    /**
     * Read callback from the link associated with this stream
     * @param buf       The buffer read
     * @param numBytes	Number of bytes read
     */
    void ReadEventTriggered(uint8_t* buf, size_t numBytes);
    /**
     * Schedule a link control packet to be sent out depending on the link state.
     * @return ER_OK if the alarm to send the packet repeatedly was added successfully.
     */
    QStatus ScheduleLinkControlPacket();

  private:

    /** Private copy constructor */
    SLAPStream(const SLAPStream& other);

    /**
     * Private Assignment operator - does nothing.
     *
     * @param other  SLAPStream to assign from.
     */
    SLAPStream operator=(const SLAPStream&);

    /** Internal functions used by SLAPStream */
    void EnqueueCtrl(ControlPacketType type, uint8_t* config = NULL);
    void TransmitToLink(void);
    void ProcessControlPacket();
    void AlarmTriggered(const Alarm& alarm, QStatus reason);
    void ProcessDataSeqNum(uint8_t seq);
    void ProcessAckNum(uint8_t ack);
    void RefreshSleepTimer();

    Stream* m_rawStream;                 /**< The underlying physical link abstraction to send/receive data */

    struct LinkParams {
        uint16_t packetSize;
        uint16_t maxPacketSize;
        uint8_t windowSize;
        uint8_t maxWindowSize;
        uint32_t baudrate;
        uint32_t resendTimeout;
        uint32_t ackTimeout;
        uint32_t protocolVersion;
    };
    LinkParams m_linkParams;      /**< Parameters associated with the SLAP stream */

    enum LinkState {
        LINK_UNINITIALIZED,     /**< Link is uninitialized */
        LINK_INITIALIZED,       /**< Link is in the process of configuration */
        LINK_ACTIVE,            /**< the link is active - can send/receive data */
        LINK_DYING,             /**< the link is in the process of being shutdown by this end */
        LINK_DEAD               /**< Link is dead */
    };
    LinkState m_linkState;        /**< Current state of the SLAP Stream link */
    uint8_t m_configField[3];                /**< link configuration information */

    Event m_sourceEvent;          /**< Source event for this stream */
    Event m_sinkEvent;            /**< Sink event for this stream */
    Event m_deadEvent;            /**< Event used to indicate that the other end has
                                       successfully processed the DISC packet and is shutting down. */
    uint32_t m_sendTimeout;       /**< Send timeout for the stream */

    enum CallbackType {
        SEND_DATA_ALARM,        /**< An alarm to send data that is in the transmit Queue. */
        RESEND_DATA_ALARM,      /**< An alarm to resend any data packets that havent been acknowledged. */
        ACK_ALARM,              /**< An alarm to send acknowledgement for packets. */
        RESEND_CONTROL_ALARM   /**< An alarm to periodically send link control packets. */
    };

    struct CallbackContext {
        CallbackType type;
        CallbackContext(CallbackType type) : type(type) { }
    };

    /*
     * Contexts associated with this SLAPStream
     */
    CallbackContext* m_sendDataCtxt;
    CallbackContext* m_resendDataCtxt;
    CallbackContext* m_ackCtxt;
    CallbackContext* m_resendControlCtxt;

    /*
     * Alarms associated with this SLAPStream
     */
    Alarm m_sendAlarm;
    Alarm m_resendAlarm;
    Alarm m_ackAlarm;
    Alarm m_ctrlAlarm;

    Timer& m_timer;          /**< The timer used for triggering one of the above alarms */

    /**
     * current state of the transmit side
     */
    enum {
        TX_IDLE, /**< Transport is ready to send but m_txQueue is empty. */
        TX_SENDING, /**< A packet is being sent. */
        TX_COMPLETE /**< A packet has been sent. */
    } m_txState;
    bool m_getNextPacket;                 /**< Whether to get the next packet to be transmitted */
    uint8_t m_expectedSeq;   /**< sequence number expected in the next packet */
    uint8_t m_txSeqNum;      /**< current transmit sequence number */
    uint8_t m_currentTxAck;   /**< sequence number of the packet we expect to ACK next */
    uint8_t m_pendingAcks;    /**< number of received packets waiting to be ACKed */

    Mutex m_streamLock;                   /**< Lock used to protect the private data structures */
    SLAPReadPacket* m_rxCurrent;            /**< Packet currently being received */
    SLAPWritePacket* m_txCtrl;              /**< Link control packet */
    SLAPWritePacket* m_txCurrent;           /**< Packet currently being transmitted */
    std::list<SLAPReadPacket*> m_rxFreeList; /**< List of receive packets that are available to be filled and put into m_rxQueue */
    std::list<SLAPReadPacket*> m_rxQueue;   /**< List of data packets that have been received */
    std::list<SLAPWritePacket*> m_txFreeList; /**< List of transmit packets that are available to be filled and put into the m_txQueue */
    std::list<SLAPWritePacket*> m_txQueue;  /**< List of packets to be transmitted */
    std::list<SLAPWritePacket*> m_txSent;   /**< List of transmitted packets that havent been acked */

};

}

#endif

