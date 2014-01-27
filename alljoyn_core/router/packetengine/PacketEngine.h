/**
 * @file
 * PacketEngine converts Streams to packets and vice-versa.
 */

/******************************************************************************
 * Copyright (c) 2011-2012, 2014 AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_PACKETENGINE_H
#define _ALLJOYN_PACKETENGINE_H

#include <qcc/platform.h>
#include <map>
#include <deque>

#include <qcc/Stream.h>
#include <qcc/SocketStream.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/Timer.h>
#include <qcc/Event.h>
#include "Packet.h"
#include "PacketStream.h"
#include "PacketPool.h"
#include "PacketEngineStream.h"

/**
 * Inside window calculation.
 * Returns true if p is in range [beg, beg+sz)
 * This function properly accounts for possible wrap-around in [beg, beg+sz) region.
 */
#define IN_WINDOW(tp, beg, sz, p) (((static_cast<tp>((beg) + (sz)) > (beg)) && ((p) >= (beg)) && ((p) < static_cast<tp>((beg) + (sz)))) || \
                                   ((static_cast<tp>((beg) + (sz)) < (beg)) && !(((p) < (beg)) && (p) >= static_cast<tp>((beg) + (sz)))))


/* Constants */
#define PACKET_ENGINE_VERSION     1          /**<  PacketEngine compatibility level */
#define CONNECT_RETRIES           6          /**<  Number of ConnectReq and/or ConnectRsp retries */
#define DISCONNECT_RETRIES        4          /**<  Number of DisconectReq retries */
#define CONNECT_RETRY_TIMEOUT     500        /**<  MS to wait befroe retrying ConnectReq and ConnectRsp */
#define DISCONNECT_RETRY_TIMEOUT  500        /**<  MS to wait before retrying DisconnectReq */
#define DISCONNECT_TIMEOUT        (3 * 1000) /**<  MS to wait for graceful disconnect to complete */
#define MAX_PACKET_SEND_ATTEMPTS  5          /**<  Max data packet retries before declaring link dead */
#define XON_RETRIES               10         /**<  Num or XON retries before declaring link dead */
#define ACK_DELAY_MS              10         /**<  Ms of delay before sending acks */
#define XON_THRESHOLD             4          /**<  Min number of empty slots in rx buffer necessary to send XON */
#define CLOSING_TIMEOUT           4000       /**< Max num of ms to wait for channel to stay in CLOSING state before being forced to CLOSED */

namespace ajn {

/* Forward Declaration */
class PacketEngine;
struct DelayAckAlarmContext;
struct ClosingAlarmContext;

/**
 * PacketEngineListener provides connect/accept/disconnect event information to PacketEngine users.
 */
class PacketEngineListener {
  public:
    virtual ~PacketEngineListener() { }

    virtual void PacketEngineConnectCB(PacketEngine& engine, QStatus status, const PacketEngineStream* stream, const PacketDest& dest, void* context) = 0;

    virtual bool PacketEngineAcceptCB(PacketEngine& engine, const PacketEngineStream& stream, const PacketDest& dest) = 0;

    virtual void PacketEngineDisconnectCB(PacketEngine& engine, const PacketEngineStream& stream, const PacketDest& dest) = 0;
};

/**
 * PacketEngine converts qcc:Streams to packets suitable for sending over UDP or other
 * packet oriented comm transports.
 */
class PacketEngine : public qcc::AlarmListener {

    friend class PacketEngineStream;

  private:
    struct ChannelInfo {

        enum State {
            OPENING,
            OPEN,
            CLOSING,
            CLOSED,
            ABORTED
        };

        /* ChannelInfo constructor */
        ChannelInfo(PacketEngine& engine, uint32_t id, const PacketDest& dest, PacketStream& packetStream,
                    PacketEngineListener& listener, uint16_t windowSize);

        /**
         * Copy constructor.
         * Creates a "detached" copy with new events.
         * This constructor should ONLY be used to copy ChannelInfos that are in their
         * initialized state since internal state such as events and stream are not
         * transferred to the new ChannelInfo.
         */
        ChannelInfo(const ChannelInfo& other);

        /* Destructor */
        ~ChannelInfo();

        PacketEngine& engine;
        uint32_t id;
        State state;
        PacketDest dest;
        qcc::Event sourceEvent;
        qcc::Event sinkEvent;
        PacketEngineStream stream;
        PacketStream& packetStream;
        PacketEngineListener& listener;
        int useCount;
        qcc::Alarm connectReqAlarm;
        qcc::Alarm connectRspAlarm;
        qcc::Alarm disconnectReqAlarm;
        qcc::Alarm disconnectRspAlarm;
        qcc::Alarm xOnAlarm;
        DelayAckAlarmContext* ackAlarmContext;
        ClosingAlarmContext* closingAlarmContext;
        bool isAckAlarmArmed;

        Packet**   rxPackets;
        uint16_t rxFill, rxDrain, rxAck;
        uint32_t rxPayloadOffset;
        uint32_t* rxMask;
        uint32_t rxMaskSize;
        uint32_t rxAdvancedSeqNum;
        bool rxFlowOff;
        uint16_t rxFlowSeqNum;
        bool rxIsMidMessage;
        qcc::Mutex rxLock;

        Packet** txPackets;
        uint16_t txFill, txDrain;
        uint16_t remoteRxDrain;
        uint16_t xOffSeqNum;
        std::deque<Packet*> txControlQueue;
        int32_t txRttMean;
        int32_t txRttMeanVar;
        bool txRttInit;
        uint32_t* ackResp;
        uint16_t txCongestionWindow;
        uint16_t txSlowStartThresh;
        uint16_t txConsecutiveAcks;
        uint16_t txLastMarshalSeqNum;
        qcc::Mutex txLock;

        uint32_t protocolVersion;
        uint16_t windowSize;
        bool wasOpen;

      private:
        /** Private assignment operator */
        ChannelInfo& operator=(const ChannelInfo& other);
    };

    class RxPacketThread : public qcc::Thread {
      public:
        RxPacketThread(const qcc::String& engineName);

      protected:
        qcc::ThreadReturn STDCALL Run(void* arg);

      private:
        PacketEngine* engine;

        void HandleControlPacket(Packet* p, PacketStream& packetStream, PacketEngineListener& listener);
        void HandleDataPacket(Packet* p);

        void HandleConnectReq(Packet* p, PacketStream& packetStream, PacketEngineListener& listener);
        void HandleConnectRsp(Packet* p);
        void HandleConnectRspAck(Packet* p);
        void HandleDisconnectReq(Packet* p);
        void HandleDisconnectRsp(Packet* p);
        void HandleDisconnectRspAck(Packet* p);
        void HandleAck(Packet* p);
        void HandleXOn(Packet* p);
        void HandleXOnAck(Packet* p);

        void AdvanceTxDrain(ChannelInfo& ci, uint16_t newTxDrain, uint16_t& advanceCount);
    };

    class TxPacketThread : public qcc::Thread {
      public:
        TxPacketThread(const qcc::String& engineName);

      protected:
        qcc::ThreadReturn STDCALL Run(void* arg);

      private:
        PacketEngine* engine;
    };

    void CloseChannel(ChannelInfo& ci);

  public:

    PacketEngine(const qcc::String& name, uint32_t maxWindowSize = 128);

    virtual ~PacketEngine();

    QStatus Start(uint32_t maxMTU = 1472);

    QStatus Stop();

    QStatus Join();

    QStatus AddPacketStream(PacketStream& packetStream, PacketEngineListener& listener);

    QStatus RemovePacketStream(PacketStream& packetStream);

    QStatus Connect(const PacketDest& dest, PacketStream& packetStream, PacketEngineListener& listener, void* context);

    PacketStream* GetPacketStream(const PacketEngineStream& stream);

    /**
     * Request graceful disconnect of stream.
     * Note taht stream is not actually disconnected until PacketEngineDisconnectCB is called.
     *
     * @param stream   PacketEngineStream to be disconnected.
     */
    void Disconnect(PacketEngineStream& stream);

    QStatus DeliverControlMsg(ChannelInfo& ci, const void* buf, size_t len, uint16_t seqNum = 0);

    void AlarmTriggered(const qcc::Alarm& alarm, QStatus status);

    qcc::String ToString(const PacketStream& stream, const PacketDest& dest) const { return stream.ToString(dest); }

    void SendXOn(ChannelInfo& ci);

  private:

    qcc::String name;
    PacketPool pool;
    RxPacketThread rxPacketThread;
    TxPacketThread txPacketThread;
    std::map<qcc::Event*, std::pair<PacketStream*, PacketEngineListener*> > packetStreams;
    qcc::Timer timer;
    qcc::Mutex channelInfoLock;
    std::map<uint32_t, ChannelInfo*> channelInfos;
    uint32_t maxWindowSize;
    bool isRunning;
    bool rxPacketThreadReload;

    ChannelInfo* CreateChannelInfo(uint32_t chanId, const PacketDest& dest, PacketStream& packetStream, PacketEngineListener& listener, uint16_t windowSize);

    ChannelInfo* AcquireChannelInfo(uint32_t chanId);

    ChannelInfo* AcquireNextChannelInfo(ChannelInfo* inCi);

    void ReleaseChannelInfo(ChannelInfo& ci);

    void SendAck(ChannelInfo& ci, uint16_t seqNum, bool allowDelay);

    void SendAckNow(ChannelInfo& ci, uint16_t seqNum);

    uint32_t GetRetryMs(const ChannelInfo& ci, uint32_t sendAttempt) const;
};

}

#endif
