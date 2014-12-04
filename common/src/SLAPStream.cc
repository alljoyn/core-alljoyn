/**
 * @file
 *
 * This file implements the Stream with the SLAP protocol for error detection,
 * flow control and retransmission.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/SLAPStream.h>

#define QCC_MODULE "SLAP"
using namespace qcc;

/* The SLAP version that adds the disconnect feature */
#define SLAP_VERSION_DISCONNECT_FEATURE 1

#define SLAP_PROTOCOL_VERSION_NUMBER 1
#define SLAP_DEFAULT_WINDOW_SIZE 4
#define SLAP_MAX_WINDOW_SIZE 4
#define SLAP_MAX_PACKET_SIZE 0xFFFF
/**
 * controls whether to send an ack for each data packet received. Can be used to
 * increase throughput.
 */
//#define ALWAYS_ACK

#define MS_PER_SECOND 1000

// 1 start bit, 8 data bits, 1 parity and 2 stop bits. i.e. 11 bits sent per byte.
#define BITS_SENT_PER_BYTE 11

/**
 * controls rate at which we send CONN packets when the link is down in milliseconds
 */
const uint32_t CONN_TIMEOUT = 200;

/**
 * controls rate at which we send NEGO packets when the link is being established in milliseconds
 */
const uint32_t NEGO_TIMEOUT = 200;

/**
 * controls rate at which we send DISCONN packets when the link is down in milliseconds
 */
const uint32_t DISCONN_TIMEOUT = 200;

SLAPStream::SLAPStream(Stream* rawStream, Timer& timer, uint16_t maxPacketSize, uint16_t maxWindowSize, uint32_t baudrate) :
    m_rawStream(rawStream),
    m_linkState(LINK_UNINITIALIZED),
    m_sendTimeout(Event::WAIT_FOREVER),
    m_sendDataCtxt(new CallbackContext(SEND_DATA_ALARM)),
    m_resendDataCtxt(new CallbackContext(RESEND_DATA_ALARM)),
    m_ackCtxt(new CallbackContext(ACK_ALARM)),
    m_resendControlCtxt(new CallbackContext(RESEND_CONTROL_ALARM)),
    m_timer(timer), m_txState(TX_IDLE), m_getNextPacket(true),
    m_expectedSeq(0), m_txSeqNum(0),
    m_currentTxAck(0), m_pendingAcks(0)
{
    m_linkParams.baudrate = baudrate;
    m_linkParams.packetSize = maxPacketSize;
    m_linkParams.maxPacketSize = maxPacketSize;

    if (maxWindowSize != 1 && maxWindowSize != 2 && maxWindowSize != 4) {
        /* Size not allowed. window size 8 not yet allowed. */
        QCC_LogError(ER_FAIL, ("Invalid window size specified %d. Using max window size %d", maxWindowSize, SLAP_MAX_WINDOW_SIZE));
        maxWindowSize = SLAP_MAX_WINDOW_SIZE;
    }
    m_linkParams.maxWindowSize = (maxWindowSize > SLAP_MAX_WINDOW_SIZE) ? SLAP_MAX_WINDOW_SIZE : maxWindowSize;
    m_linkParams.windowSize = m_linkParams.maxWindowSize;

    memset(m_configField, '\0', 3);

    /* Initially m_rxCurrent will only be used for Link Ctrl packets - 32 bytes is sufficient */
    m_rxCurrent = new SLAPReadPacket(32);
    m_txCtrl = new SLAPWritePacket(32);
}

SLAPStream::~SLAPStream()
{
    Close();
    std::list<SLAPWritePacket*>::iterator it = m_txFreeList.begin();
    while (it != m_txFreeList.end()) {
        delete *it;
        ++it;
    }
    m_txFreeList.clear();

    it = m_txQueue.begin();
    while (it != m_txQueue.end() && *it != m_txCtrl) {
        delete *it;
        ++it;
    }
    m_txQueue.clear();
    it = m_txSent.begin();
    while (it != m_txSent.end()) {
        delete *it;
        ++it;
    }
    m_txSent.clear();
    std::list<SLAPReadPacket*>::iterator it1 = m_rxQueue.begin();
    while (it1 != m_rxQueue.end()) {
        delete *it1;
        ++it1;
    }
    m_rxQueue.clear();
    it1 = m_rxFreeList.begin();
    while (it1 != m_rxFreeList.end()) {
        delete *it1;
        ++it1;
    }
    m_rxFreeList.clear();

    delete m_rxCurrent;
    delete m_txCtrl;

    delete m_sendDataCtxt;
    delete m_resendDataCtxt;
    delete m_ackCtxt;
    delete m_resendControlCtxt;
}

QStatus SLAPStream::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    uint8_t* bufPtr = static_cast<uint8_t*>(buf);
    m_streamLock.Lock(MUTEX_CONTEXT);
    QStatus status = ER_OK;
    size_t ret = 0;
    size_t bytesRemaining = reqBytes;
    size_t bytesRead = 0;
    while (bytesRead < reqBytes) {
        if (m_linkState == LINK_DEAD) {
            status = ER_SLAP_OTHER_END_CLOSED;
            break;
        }

        if (m_rxQueue.empty()) {
            /* Receive Queue is empty. Wait until it is filled */
            m_streamLock.Unlock(MUTEX_CONTEXT);

            status = Event::Wait(m_sourceEvent, timeout);
            m_streamLock.Lock(MUTEX_CONTEXT);

            if (status != ER_OK) {
                break;
            } else {
                continue;
            }
        }
        /* Get the first packet from the m_rxQueue and start filling bytes into the buffer. */
        SLAPReadPacket* h = m_rxQueue.front();

        if (h->FillBuffer(bufPtr, bytesRemaining, ret)) {
            /* The first packet in the m_rxQueue is exhausted, pop it off the list */
            m_rxQueue.pop_front();
            m_rxFreeList.push_back(h);
        }
        bufPtr += ret;
        bytesRead += ret;
        bytesRemaining -= ret;
        if (m_rxQueue.empty()) {
            /* No more packets available, so reset the source event */
            m_sourceEvent.ResetEvent();
        }

    }
    if (status == ER_TIMEOUT && bytesRead > 0) {
        /* Have read at least 1 packet, and then timedout */
        status = ER_OK;
    }
    if (status == ER_OK) {
        actualBytes = bytesRead;
    }
    bufPtr = static_cast<uint8_t*>(buf);
    m_streamLock.Unlock(MUTEX_CONTEXT);
    return status;
}
/**
 * Determine relative ordering of two sequence numbers. Sequence numbers are
 * modulo 8 so 0 > 7.
 *
 * This is used to test for ACKs and to detect gaps in the sequence of received
 * packets.
 */
#define SEQ_GT(s1, s2)  (((7 + (s1) - (s2)) & 7) < m_linkParams.windowSize)
/*
 * This function is called from the receive side with the sequence number of
 * the last packet received.
 * This function must be called with the m_streamLock
 */
void SLAPStream::ProcessDataSeqNum(uint8_t seq)
{
    /*
     * If we think we have already acked this sequence number we don't adjust
     * the ack count.
     */
    if (!SEQ_GT(m_currentTxAck, seq)) {
        m_currentTxAck = (seq + 1) & 0x7;
    }
    /*
     * If there are packets to send the ack will go out with the next packet.
     */
    if (m_txState != TX_IDLE) {
        return;
    }
    QStatus status = ER_TIMER_FULL;
    Alarm ackAlarm;

#ifdef ALWAYS_ACK
    ++m_pendingAcks;
    /*
     * If there are no packets to send we are allowed to accumulate a
     * backlog of pending ACKs up to a maximum equal to the window size.
     * In any case we are required to send an ack within a timeout
     * period so if this is the first pending ack we need to prime a timer.
     */

    while (m_pendingAcks && !m_timer.HasAlarm(m_ackAlarm) && status == ER_TIMER_FULL) {
        AlarmListener* listener = this;
        uint32_t when = 0;

        ackAlarm = Alarm(when, listener, m_ackCtxt);
        /* Call the non-blocking version of AddAlarm, while holding the
         * locks to ensure that the state of the dispatchEntry is valid.
         */
        status = m_timer.AddAlarmNonBlocking(ackAlarm);

        if (status == ER_TIMER_FULL) {
            m_streamLock.Unlock();
            qcc::Sleep(2);
            m_streamLock.Lock();
        }
        if (status == ER_OK) {
            m_ackAlarm = ackAlarm;
        }
    }

#else
    ++m_pendingAcks;

    /*
     * If there are no packets to send we are allowed to accumulate a
     * backlog of pending ACKs up to a maximum equal to the window size.
     * In any case we are required to send an ack within a timeout
     * period so if this is the first pending ack we need to prime a timer.
     */
    while (m_pendingAcks && !m_timer.HasAlarm(m_ackAlarm) && status == ER_TIMER_FULL) {
        AlarmListener* listener = this;
        uint32_t when = (m_pendingAcks == m_linkParams.windowSize) ? 0 : m_linkParams.ackTimeout;

        ackAlarm = Alarm(when, listener, m_ackCtxt);
        /* Call the non-blocking version of AddAlarm, while holding the
         * locks to ensure that the state of the dispatchEntry is valid.
         */
        status = m_timer.AddAlarmNonBlocking(ackAlarm);

        if (status == ER_TIMER_FULL) {
            m_streamLock.Unlock();
            qcc::Sleep(2);
            m_streamLock.Lock();
        }
        if (status == ER_OK) {
            m_ackAlarm = ackAlarm;
        }
    }

#endif
}

/**
 * This function is called by the receive layer when a data packet or an explicit ACK
 * has been received. The ACK value is one greater (modulo 8) than the seq number of the
 * last packet successfully received.
 * This function must be called with the m_streamLock
 */
void SLAPStream::ProcessAckNum(uint8_t ack)
{
    SLAPWritePacket* pkt;
    m_timer.RemoveAlarm(m_resendAlarm, false);
    /* Look through the m_txSent list and remove any data packets that have already been sent out. */
    while (!m_txSent.empty()) {
        pkt = m_txSent.front();
        if (SEQ_GT(ack, pkt->GetSeqNum())) {
            assert(pkt->GetPacketType() == RELIABLE_DATA_PACKET);
            m_txSent.pop_front();
            m_txFreeList.push_back(pkt);
            /* If there is space available in the m_txFreeList, set the sink event */
            m_sinkEvent.SetEvent();
        } else {
            break;
        }

    }
    Alarm resendAlarm;
    QStatus status = ER_TIMER_FULL;
    while (!m_txSent.empty()  && !m_timer.HasAlarm(m_resendAlarm) && status == ER_TIMER_FULL) {
        AlarmListener* listener = this;
        uint32_t when = m_linkParams.resendTimeout;

        resendAlarm = Alarm(when, listener, m_resendDataCtxt);
        /* Call the non-blocking version of AddAlarm, while holding the
         * locks to ensure that the state of the dispatchEntry is valid.
         */
        status = m_timer.AddAlarmNonBlocking(resendAlarm);

        if (status == ER_TIMER_FULL) {
            m_streamLock.Unlock();
            qcc::Sleep(2);
            m_streamLock.Lock();
        }
        if (status == ER_OK) {
            m_resendAlarm = resendAlarm;
        }
    }


}


void SLAPStream::ReadEventTriggered(uint8_t* buffer, size_t bytes)
{
    QStatus status = ER_OK;
    m_streamLock.Lock(MUTEX_CONTEXT);

    while (status == ER_OK) {
        /* Deslip the received bytes into a packet */
        status = m_rxCurrent->DeSlip(buffer, bytes);
        if (status == ER_OK) {
            /* Validate the header, CRC etc */
            status = m_rxCurrent->Validate();
            if (status != ER_OK) {
                m_rxCurrent->Clear();
                continue;
            }
            uint8_t seq = m_rxCurrent->GetSeqNum();
            switch (m_rxCurrent->GetPacketType()) {

            case INVALID_PACKET:
                break;

            case RELIABLE_DATA_PACKET:
                ProcessAckNum(m_rxCurrent->GetAckNum());

                /*
                 * If a reliable packet does not have the expected sequence number, then
                 * it is either a repeated packet or we missed a packet. In either case,
                 * we must ignore the packet but we need to ACK repeated packets.
                 */
                if (seq != m_expectedSeq) {
                    if (SEQ_GT(seq, m_expectedSeq)) {
                        QCC_DbgPrintf(("Missing packet - expected = %d, got %d", m_expectedSeq, seq));
                    } else {
                        QCC_DbgPrintf(("Repeated packet seq = %d, expected %d", seq, m_expectedSeq));
                        ProcessDataSeqNum(seq);
                    }
                } else {
                    /*
                     * If the correct sequence number has been received, but we do not have space in
                     * the m_rxFreeList, we ignore this packet.
                     */

                    if (!m_rxFreeList.empty()) {
                        QCC_DbgPrintf(("Correct packet seq = %d, expected %d", seq, m_expectedSeq));

                        /*
                         * modulo 8 increment of the expected sequence number
                         */
                        m_expectedSeq = (m_expectedSeq + 1) & 0x07;
                        ProcessDataSeqNum(seq);

                        m_rxQueue.push_back(m_rxCurrent);
                        m_sourceEvent.SetEvent();
                        m_rxCurrent = m_rxFreeList.front();
                        m_rxFreeList.pop_front();
                    } else {
                        QCC_DbgPrintf(("Ignoring packet - expected = %d, got %d", m_expectedSeq, seq));
                    }
                }
                break;

            case ACK_PACKET:
                ProcessAckNum(m_rxCurrent->GetAckNum());
                break;

            case CTRL_PACKET:
                ProcessControlPacket();
                break;
            }
            assert(m_rxCurrent);
            m_rxCurrent->Clear();
        }
    }
    m_streamLock.Unlock(MUTEX_CONTEXT);
}

void SLAPStream::ProcessControlPacket()
{
    ControlPacketType pktType = m_rxCurrent->GetControlType();
    if (pktType == UNKNOWN_PKT) {
        QCC_DbgPrintf(("Unknown link packet type %d", pktType));
        return;
    }
    switch (m_linkState) {

    case LINK_UNINITIALIZED:
        if (pktType == CONN_PKT) {
            EnqueueCtrl(ACCEPT_PKT);
            return;
        }
        if (pktType == ACCEPT_PKT) {
            QCC_DbgPrintf(("Received sync response - moving to LINK_INITIALIZED"));
            m_linkState = LINK_INITIALIZED;
            m_configField[0] = m_linkParams.maxPacketSize >> 8;
            m_configField[1] = m_linkParams.maxPacketSize & 0xFF;
            uint8_t encodedWinSize = ((m_linkParams.maxWindowSize == 1) ? 0 : ((m_linkParams.maxWindowSize == 2) ? 1 : ((m_linkParams.maxWindowSize == 4) ? 2 : 3)));
            m_configField[2] = (SLAP_PROTOCOL_VERSION_NUMBER << 2) | encodedWinSize;
            QCC_DbgPrintf(("PCP sending NEGO pkt %d win %d conf %X %X %X", m_linkParams.maxPacketSize, m_linkParams.maxWindowSize, m_configField[0], m_configField[1], m_configField[2]));
            /*
             * Initialize the link configuration packet - we are not allowed
             * to change this during link establishment.
             */
            EnqueueCtrl(NEGO_PKT, m_configField);
            return;
        }
        break;

    case LINK_INITIALIZED:

        /*
         * In the initialized state we need to respond to sync packets, conf
         * packets, and conf-response packets.
         */
        if (pktType == CONN_PKT) {
            EnqueueCtrl(ACCEPT_PKT);
            return;
        }
        if (pktType == NEGO_RESP_PKT) {
            /*
             * Configure the link.
             */

            uint8_t encodedWindowSize = m_rxCurrent->GetConfigField(2) & 0x03;
            m_linkParams.packetSize = (m_rxCurrent->GetConfigField(0) << 8) | m_rxCurrent->GetConfigField(1);

            m_linkParams.windowSize = 1 << encodedWindowSize;
            m_linkParams.protocolVersion = m_rxCurrent->GetConfigField(2) >> 2;

            /*
             * Check that the configuration response is valid.
             */
            if (m_linkParams.packetSize > m_linkParams.packetSize) {
                QCC_LogError(ER_FAIL, ("Configuration failed - device is not configuring link correctly %d %d", m_linkParams.packetSize, m_linkParams.maxPacketSize));
                m_linkState = LINK_DEAD;
                return;
            }
            /*
             * Check that the configuration response is valid.
             */
            if (m_linkParams.windowSize > m_linkParams.maxWindowSize) {
                QCC_LogError(ER_FAIL, ("Configuration failed - device is not configuring link correctly %d %d", m_linkParams.windowSize, m_linkParams.maxWindowSize));
                m_linkState = LINK_DEAD;
                return;
            }
            QCC_DbgPrintf(("Allocating buffers win %d pkt %d", m_linkParams.windowSize, m_linkParams.packetSize));

            /* The m_txFreeList is initially allocated the window size */
            for (int i = 0; i < m_linkParams.windowSize; ++i) {
                SLAPWritePacket*h = new SLAPWritePacket(m_linkParams.packetSize);
                m_txFreeList.push_back(h);
            }
            for (int i = 0; i < m_linkParams.windowSize; ++i) {
                SLAPReadPacket*h = new SLAPReadPacket(m_linkParams.packetSize);
                m_rxFreeList.push_back(h);
            }
            delete m_rxCurrent;
            m_rxCurrent = new SLAPReadPacket(m_linkParams.packetSize);
            QCC_DbgPrintf(("Link configured - packetsize =%d window size = %d", m_linkParams.packetSize,
                           m_linkParams.windowSize));

            /* Re-calculate timeouts based on the agreed packet size */
            /* Ack timeout should be twice the amount of max transmission time per packet. */
            m_linkParams.ackTimeout = (m_linkParams.packetSize * BITS_SENT_PER_BYTE * MS_PER_SECOND * 2) / m_linkParams.baudrate;

            /* Resend timeout should be thrice the amount of max transmission time per packet. */
            m_linkParams.resendTimeout = (m_linkParams.packetSize * BITS_SENT_PER_BYTE * MS_PER_SECOND * 3) / m_linkParams.baudrate;

            m_linkState = LINK_ACTIVE;
            m_sinkEvent.SetEvent();
            return;
        }
        if (pktType == NEGO_PKT) {
            uint8_t requestedEncWindowSize = m_rxCurrent->GetConfigField(2) & 0x03;
            uint8_t requestedWindowSize = 1 << requestedEncWindowSize;
            uint8_t requestedProtocolVersion = m_rxCurrent->GetConfigField(2) >> 2;
            uint16_t requestedPacketSize = (m_rxCurrent->GetConfigField(0) << 8) | m_rxCurrent->GetConfigField(1);

            uint16_t agreedPacketSize = (requestedPacketSize < m_linkParams.maxPacketSize) ? requestedPacketSize : m_linkParams.packetSize;
            uint16_t agreedWindowSize = (requestedWindowSize < m_linkParams.maxWindowSize) ? requestedWindowSize : m_linkParams.maxWindowSize;
            uint8_t agreedProtocolVersion = (requestedProtocolVersion < SLAP_PROTOCOL_VERSION_NUMBER) ? requestedProtocolVersion : SLAP_PROTOCOL_VERSION_NUMBER;
            QCC_DbgPrintf(("Got NEGO req:win %d pkt %d, pv %d agr:win %d pkt %d pv %d", requestedWindowSize, requestedPacketSize, requestedProtocolVersion,
                           agreedWindowSize, agreedPacketSize, agreedProtocolVersion));

            m_linkParams.packetSize = agreedPacketSize;
            m_linkParams.windowSize = agreedWindowSize;
            m_linkParams.protocolVersion = agreedProtocolVersion;

            m_configField[0] = agreedPacketSize >> 8;
            m_configField[1] = agreedPacketSize & 0xFF;
            uint8_t agreedEncWindowSize = ((agreedWindowSize == 1) ? 0 : ((agreedWindowSize == 2) ? 1 : ((agreedWindowSize == 4) ? 2 : 3)));
            m_configField[2] = (agreedProtocolVersion << 2) | agreedEncWindowSize;

            QCC_DbgPrintf(("PCP sending NEGORESP pkt %d win %d conf %X %X %X", agreedPacketSize, agreedWindowSize, m_configField[0], m_configField[1], m_configField[2]));

            EnqueueCtrl(NEGO_RESP_PKT, m_configField);
            return;
        }
        break;

    case LINK_ACTIVE:
        /*
         * In the active state, we need to respond to conf packets,
         * sync packets cause a transport reset.
         */
        if (pktType == NEGO_PKT) {
            m_configField[0] = m_linkParams.packetSize >> 8;
            m_configField[1] = m_linkParams.packetSize & 0xFF;
            uint8_t agreedEncWindowSize = ((m_linkParams.windowSize == 1) ? 0 : ((m_linkParams.windowSize == 2) ? 1 : ((m_linkParams.windowSize == 4) ? 2 : 3)));
            m_configField[2] = (m_linkParams.protocolVersion << 2) | agreedEncWindowSize;

            QCC_DbgPrintf(("PCP sending NEGORESP conf %X %X %X", m_configField[0], m_configField[1], m_configField[2]));

            /*
             * Ignore the configuration field, the link is already configured.
             */
            EnqueueCtrl(NEGO_RESP_PKT, m_configField);
            return;
        }
        if (pktType == CONN_PKT) {
            /*
             * The other end went down and came back up.
             * Declare the link as dead so the app will close and re-open this port.
             */
            m_linkState = LINK_DEAD;
            m_sourceEvent.SetEvent();
            m_sinkEvent.SetEvent();
            return;
        }
        if (pktType == DISCONN_PKT) {
            QCC_DbgPrintf(("Got disconn, setting link to dead"));
            EnqueueCtrl(DISCONN_RESP_PKT);
            m_linkState = LINK_DEAD;
            m_sourceEvent.SetEvent();
            m_sinkEvent.SetEvent();
            return;
        }
        break;

    case LINK_DYING:
        if (pktType == DISCONN_RESP_PKT) {
            QCC_DbgPrintf(("Got disconn resp, setting link to dead"));
            m_linkState = LINK_DEAD;
            m_sourceEvent.SetEvent();
            m_sinkEvent.SetEvent();
            m_deadEvent.SetEvent();
            return;
        }

        if (pktType == DISCONN_PKT) {
            QCC_DbgPrintf(("Got disconn, queuing DRSP"));
            EnqueueCtrl(DISCONN_RESP_PKT);
            return;
        }
        break;

    case LINK_DEAD:
        break;
    }
    /*
     * Ignore any other packets.
     */
    QCC_DbgPrintf(("Discarding link packet %d", pktType));
}

void SLAPStream::TransmitToLink()
{
    m_streamLock.Lock(MUTEX_CONTEXT);
    m_txState = TX_SENDING;

    QStatus status = ER_OK;
    while (status == ER_OK && !m_txQueue.empty()) {
        if (m_getNextPacket) {
            /*
             * The next packet to set is the head of the queue
             */
            m_txCurrent = m_txQueue.front();
            m_txQueue.pop_front();
            m_txCurrent->SetAck(m_currentTxAck);
            m_txCurrent->PrependHeader();
            if (m_txCurrent->GetPacketType() != CTRL_PACKET) {

                if (m_pendingAcks) {
                    m_pendingAcks = 0;
                    m_timer.RemoveAlarm(m_ackAlarm, false);
                }
            }
            m_getNextPacket = false;
        }
        status = m_txCurrent->Deliver(m_rawStream);
        if (status == ER_OK) {
            /*
             * If the packet we just sent was a data packet, we add it to the
             * sent queue. We do this now even though the final chunk of the
             * packet has not yet been sent because we may receive an ACK for
             * this packet before the send below returns.
             */
            if (m_txCurrent != m_txCtrl) {
                m_txSent.push_back(m_txCurrent);
            }
            m_getNextPacket = true;
        }
    }
    Alarm resendAlarm;
    status = ER_TIMER_FULL;
    while (!m_txSent.empty() && !m_timer.HasAlarm(m_resendAlarm) && status == ER_TIMER_FULL) {
        AlarmListener* listener = this;
        uint32_t when = m_linkParams.resendTimeout;

        resendAlarm = Alarm(when, listener, m_resendDataCtxt);
        /* Call the non-blocking version of AddAlarm, while holding the
         * locks to ensure that the state of the dispatchEntry is valid.
         */
        status = m_timer.AddAlarmNonBlocking(resendAlarm);

        if (status == ER_TIMER_FULL) {
            m_streamLock.Unlock();
            qcc::Sleep(2);
            m_streamLock.Lock();
        }
        if (status == ER_OK) {
            m_resendAlarm = resendAlarm;
        }
    }


    /*
     * Nothing to send so we go idle
     */
    m_txState = TX_IDLE;
    m_streamLock.Unlock(MUTEX_CONTEXT);
}

QStatus SLAPStream::ScheduleLinkControlPacket() {
    m_streamLock.Lock(MUTEX_CONTEXT);
    AlarmListener* listener = this;
    uint32_t when = 10;
    QStatus status = ER_OK;
    bool addCtrlAlarm = false;
    switch (m_linkState) {

    case LINK_UNINITIALIZED:
        /*
         * Send a sync packet.
         */
        EnqueueCtrl(CONN_PKT);
        when = CONN_TIMEOUT;

        m_ctrlAlarm = Alarm(when, listener, m_resendControlCtxt);
        addCtrlAlarm = true;
        break;

    case LINK_INITIALIZED:
        /*
         * Send a conf packet.
         */
        EnqueueCtrl(NEGO_PKT, m_configField);
        when = NEGO_TIMEOUT;
        m_ctrlAlarm = Alarm(when, listener, m_resendControlCtxt);

        addCtrlAlarm = true;
        break;

    case LINK_DYING:
        /*
         * Send a conf packet.
         */
        EnqueueCtrl(DISCONN_PKT, m_configField);
        when = DISCONN_TIMEOUT;
        m_ctrlAlarm = Alarm(when, listener, m_resendControlCtxt);

        addCtrlAlarm = true;
        break;

    default:
        /**
         * Do nothing.
         */
        break;
    }
    if (addCtrlAlarm) {
        Alarm ctrlAlarm;
        status = ER_TIMER_FULL;
        while (!m_timer.HasAlarm(m_ctrlAlarm)  && status == ER_TIMER_FULL) {
            AlarmListener* listener = this;


            ctrlAlarm = Alarm(when, listener, m_resendControlCtxt);
            /* Call the non-blocking version of AddAlarm, while holding the
             * locks to ensure that the state of the dispatchEntry is valid.
             */
            status = m_timer.AddAlarmNonBlocking(ctrlAlarm);

            if (status == ER_TIMER_FULL) {
                m_streamLock.Unlock();
                qcc::Sleep(2);
                m_streamLock.Lock();
            }
            if (status == ER_OK) {
                m_ctrlAlarm = ctrlAlarm;
            }
        }


    }
    m_streamLock.Unlock(MUTEX_CONTEXT);
    return status;
}

QStatus SLAPStream::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{
    m_streamLock.Lock(MUTEX_CONTEXT);

    const uint8_t* tempBuf = static_cast<const uint8_t*>(buf);
    QStatus status = ER_OK;
    size_t bytesWritten = 0;
    while (bytesWritten < numBytes) {
        if (m_linkState == LINK_DEAD) {
            status = ER_SLAP_OTHER_END_CLOSED;
            break;
        }
        /*
         * Waiting for an space in the free list before we can queue any more data
         */
        if (m_txFreeList.empty()) {
            m_streamLock.Unlock(MUTEX_CONTEXT);
            if (m_sendTimeout == Event::WAIT_FOREVER) {
                status = Event::Wait(m_sinkEvent);
            } else {
                status = Event::Wait(m_sinkEvent, m_sendTimeout);
            }
            m_streamLock.Lock(MUTEX_CONTEXT);
            if (ER_OK != status) {
                break;
            } else {
                continue;
            }
        }
        /*
         * Fill as many packets as we can
         */
        bool queued = false;
        while (!m_txFreeList.empty() && (bytesWritten < numBytes)) {
            SLAPWritePacket* pkt = m_txFreeList.front();
            queued = true;
            m_txFreeList.pop_front();
            size_t ret;
            pkt->DataPacket(tempBuf, numBytes - bytesWritten, ret);
            tempBuf += ret;
            bytesWritten += ret;
            /**
             * Reliable packets are sent in order so are appended to the end of the transmit queue.
             */
            pkt->SetSeqNum(m_txSeqNum);

            /*
             * updates sequence number
             */
            m_txSeqNum = (m_txSeqNum + 1) & 0x7;
            /*
             * Add to the end of the transmit queue.
             */
            m_txQueue.push_back(pkt);
        }
        if (m_txFreeList.empty()) {
            m_sinkEvent.ResetEvent();
        }
        Alarm sendAlarm;
        QStatus status = ER_TIMER_FULL;
        while (queued && (m_txState == TX_IDLE)  && !m_timer.HasAlarm(m_sendAlarm) && status == ER_TIMER_FULL) {
            AlarmListener* listener = this;
            uint32_t when = 0;

            sendAlarm = Alarm(when, listener, m_sendDataCtxt);
            /* Call the non-blocking version of AddAlarm, while holding the
             * locks to ensure that the state of the dispatchEntry is valid.
             */
            status = m_timer.AddAlarmNonBlocking(sendAlarm);

            if (status == ER_TIMER_FULL) {
                m_streamLock.Unlock();
                qcc::Sleep(2);
                m_streamLock.Lock();
            }
            if (status == ER_OK) {
                m_sendAlarm = sendAlarm;
            }
        }


    }
    if (status == ER_TIMEOUT && bytesWritten > 0) {
        /* Have written at least 1 packet, and then timedout */
        status = ER_OK;
    }
    if (status == ER_OK) {
        numSent = bytesWritten;
    }

    m_streamLock.Unlock(MUTEX_CONTEXT);
    return status;
}


void SLAPStream::EnqueueCtrl(ControlPacketType type, uint8_t* config)
{
    m_streamLock.Lock(MUTEX_CONTEXT);
    /*
     * Unreliable packets are given special treatment because they are sent
     * ahead of any packets already in the m_txQueue and do not require
     * acknowledgment.
     */
    m_txCtrl->Clear();
    m_txCtrl->ControlPacket(type, config);
    /*
     * Add to front of transmit queue. It is possible that the unreliable packet
     * structure has already been queued due to a ACK or due to an earlier
     * link control packet.  In any case, because these are unreliable packets it is OK
     * for the new packet to overwrite the older packet. It would NOT be OK for
     * the unreliable packet to get queued twice.
     */
    if (m_txQueue.front() == m_txCtrl) {
        QCC_DbgPrintf(("Unreliable packet already queued %d", type));
    } else {

        m_txQueue.push_front(m_txCtrl);
    }
    Alarm sendAlarm;
    QStatus status = ER_TIMER_FULL;
    while (!m_timer.HasAlarm(m_sendAlarm) && status == ER_TIMER_FULL) {
        AlarmListener* listener = this;
        uint32_t when = 0;

        sendAlarm = Alarm(when, listener, m_sendDataCtxt);
        /* Call the non-blocking version of AddAlarm, while holding the
         * locks to ensure that the state of the dispatchEntry is valid.
         */
        status = m_timer.AddAlarmNonBlocking(sendAlarm);

        if (status == ER_TIMER_FULL) {
            m_streamLock.Unlock();
            qcc::Sleep(2);
            m_streamLock.Lock();
        }
        if (status == ER_OK) {
            m_sendAlarm = sendAlarm;
        }
    }

    m_streamLock.Unlock(MUTEX_CONTEXT);
}

void SLAPStream::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    m_streamLock.Lock(MUTEX_CONTEXT);
    CallbackContext* ctxt = static_cast<CallbackContext*>(alarm->GetContext());
    if (reason != ER_OK) {
        m_streamLock.Unlock(MUTEX_CONTEXT);
        return;
    }
    switch (ctxt->type) {
    case SEND_DATA_ALARM:
        /*
         * Start sending again.
         */
        TransmitToLink();
        break;

    case RESEND_DATA_ALARM:
        /*
         * No resends if the link is not up.
         */
        if (m_linkState != LINK_ACTIVE) {
            m_streamLock.Unlock(MUTEX_CONTEXT);
            return;
        }
        /*
         * To preserve packet order, all unacknowleged packets must be resent. This
         * simply means moving packets on m_txSent to the head of m_txQueue.
         */
        if (!m_txSent.empty()) {
            std::list<SLAPWritePacket*>::iterator it = m_txQueue.begin();
            if ((it == m_txQueue.end()) || (m_txQueue.front() != m_txCtrl)) {
                m_txQueue.insert(it, m_txSent.begin(), m_txSent.end());
            } else {
                m_txQueue.insert(++it, m_txSent.begin(), m_txSent.end());
            }
            m_txSent.clear();
            /*
             * Start sending again.
             */
            TransmitToLink();
        }

        break;

    case ACK_ALARM:
        if (m_pendingAcks) {
            m_pendingAcks = 0;
            m_txCtrl->Clear();
            m_txCtrl->AckPacket();
            /*
             * ACK packets have a non-zero ack number.
             */
            m_txCtrl->SetAck(m_currentTxAck);
            /*
             * Add to front of transmit queue. It is possible that the unreliable packet
             * structure has already been queued due to a ACK or due to an earlier
             * link control packet.  In any case, because these are unreliable packets it is OK
             * for the new packet to overwrite the older packet. It would NOT be OK for
             * the unreliable packet to get queued twice.
             */
            if (m_txQueue.front() == m_txCtrl) {
                QCC_DbgPrintf(("Unreliable packet already queued"));
            } else {
                m_txQueue.push_front(m_txCtrl);
            }
            TransmitToLink();
        }

    case RESEND_CONTROL_ALARM:
        ScheduleLinkControlPacket();
        break;

    }

    m_streamLock.Unlock(MUTEX_CONTEXT);
}
void SLAPStream::Close() {
    m_streamLock.Lock(MUTEX_CONTEXT);
    if (m_linkParams.protocolVersion >= SLAP_VERSION_DISCONNECT_FEATURE) {
        if (m_linkState != LINK_DEAD) {
            m_linkState = LINK_DYING;
            ScheduleLinkControlPacket();
            m_streamLock.Unlock(MUTEX_CONTEXT);
            /* Wait until DRSP is received from the other end upto a
             * max timeout of 4*resendTimeout.
             */
            Event::Wait(m_deadEvent, DISCONN_TIMEOUT * 4);
            m_streamLock.Lock(MUTEX_CONTEXT);
            if (m_linkState != LINK_DEAD) {
                QCC_DbgPrintf(("Couldnt kill link gracefully"));
                m_linkState = LINK_DEAD;
                m_sourceEvent.SetEvent();
                m_sinkEvent.SetEvent();
            } else {
                QCC_DbgPrintf(("Killed link gracefully."));
            }
        }
    } else {
        m_linkState = LINK_DEAD;
    }
    m_streamLock.Unlock(MUTEX_CONTEXT);
}

