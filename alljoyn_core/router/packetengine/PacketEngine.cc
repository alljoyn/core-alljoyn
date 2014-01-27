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

#include <qcc/platform.h>

#include <algorithm>
#include <limits>

#include <qcc/Crypto.h>
#include <qcc/Util.h>
#include "PacketEngine.h"

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

struct AlarmContext {
    enum ContextType {
        CONTEXT_CONNECT_REQ,
        CONTEXT_CONNECT_RSP,
        CONTEXT_CONNECT_RSP_ACK,
        CONTEXT_DISCONNECT_REQ,
        CONTEXT_DISCONNECT_RSP,
        CONTEXT_XON,
        CONTEXT_DELAY_ACK,
        CONTEXT_CLOSING
    };

    ContextType contextType;
    uint32_t chanId;

    AlarmContext(AlarmContext::ContextType t, uint32_t chanId) : contextType(t), chanId(chanId) { }

    virtual ~AlarmContext() { }
};

struct ConnectReqAlarmContext : public AlarmContext {
    void* context;
    PacketDest dest;
    uint32_t retries;
    uint32_t connReq[3];
    ConnectReqAlarmContext(uint32_t chanId, const PacketDest& dest, void* context) :
        AlarmContext(AlarmContext::CONTEXT_CONNECT_REQ, chanId), context(context), dest(dest), retries(0) { }
};

struct ConnectRspAlarmContext : public AlarmContext {
    PacketDest dest;
    uint32_t retries;
    uint32_t connRsp[4];
    ConnectRspAlarmContext(uint32_t chanId, const PacketDest& dest) :
        AlarmContext(AlarmContext::CONTEXT_CONNECT_RSP, chanId), dest(dest), retries(0) { }
};

struct DisconnectReqAlarmContext : public AlarmContext {
    uint32_t retries;
    uint32_t disconnReq[1];
    DisconnectReqAlarmContext(uint32_t chanId) :
        AlarmContext(AlarmContext::CONTEXT_DISCONNECT_REQ, chanId), retries(0) { }
};

struct DisconnectRspAlarmContext : public AlarmContext {
    uint32_t disconnRsp[1];
    DisconnectRspAlarmContext(uint32_t chanId) : AlarmContext(AlarmContext::CONTEXT_DISCONNECT_RSP, chanId) { }
};

struct XOnAlarmContext : public AlarmContext {
    uint32_t retries;
    uint32_t xon[3];
    uint16_t xoffSeqNum;
    XOnAlarmContext(uint32_t chanId, uint16_t seqNum) : AlarmContext(AlarmContext::CONTEXT_XON, chanId), retries(0), xoffSeqNum(seqNum) { }
};

struct DelayAckAlarmContext : public AlarmContext {
    DelayAckAlarmContext(uint32_t chanId) : AlarmContext(AlarmContext::CONTEXT_DELAY_ACK, chanId) { }
};

struct ClosingAlarmContext : public AlarmContext {
    ClosingAlarmContext(uint32_t chanId) : AlarmContext(AlarmContext::CONTEXT_CLOSING, chanId) { }
};

static uint32_t GetValidWindowSize(uint32_t inWinSize)
{
    uint32_t allowedSize = 0x400;  /* max allowed window size is 1k packets */
    while (allowedSize > inWinSize) {
        allowedSize = allowedSize >> 1;
    }
    return allowedSize;
}

PacketEngine::PacketEngine(const qcc::String& name, uint32_t maxWindowSize) :
    name(name),
    rxPacketThread(name),
    txPacketThread(name),
    timer("PacketEngineTimer"),
    maxWindowSize(maxWindowSize),
    isRunning(false),
    rxPacketThreadReload(false)
{
    QCC_DbgTrace(("PacketEngine::PacketEngine(%p)", this));

    /* Check that window size is a power of 2 */
#ifndef NDEBUG
    int setBits = 0;
    uint32_t t = maxWindowSize;
    while (t) {
        if (t & 0x01) {
            ++setBits;
        }
        t = t >> 1;
    }
    assert(setBits == 1);
#endif
}

PacketEngine::~PacketEngine()
{
    QCC_DbgTrace(("~PacketEngine(%p)", this));
    rxPacketThreadReload = true;
    Stop();
    Join();
}

QStatus PacketEngine::Start(uint32_t mtu) {
    QCC_DbgTrace(("PacketEngine::Start()"));
    isRunning = true;
    QStatus status = pool.Start(mtu);
    QStatus tStatus = rxPacketThread.Start(this);
    status = (status == ER_OK) ? tStatus : status;
    tStatus = txPacketThread.Start(this);
    status = (status == ER_OK) ? tStatus : status;
    tStatus = timer.Start();
    status = (status == ER_OK) ? tStatus : status;
    isRunning = (status == ER_OK);
    return status;
}

QStatus PacketEngine::Stop() {
    QCC_DbgTrace(("PacketEngine::Stop()"));
    QStatus status = timer.Stop();
    QStatus tStatus = txPacketThread.Stop();
    status = (status == ER_OK) ? tStatus : status;
    tStatus = rxPacketThread.Stop();
    status = (status == ER_OK) ? tStatus : status;
    tStatus = pool.Stop();
    isRunning = false;
    return (status == ER_OK) ? tStatus : status;
}

QStatus PacketEngine::Join() {
    QCC_DbgTrace(("PacketEngine::Join()"));

    QStatus status = rxPacketThread.Join();
    QStatus tStatus = txPacketThread.Join();
    status = (status == ER_OK) ? tStatus : status;
    tStatus = timer.Join();
    return (status == ER_OK) ? tStatus : status;
}

QStatus PacketEngine::AddPacketStream(PacketStream& stream, PacketEngineListener& listener)
{
    QCC_DbgTrace(("PacketEngine::AddPacketStream(%p)", &stream));

    channelInfoLock.Lock();
    packetStreams[&stream.GetSourceEvent()] = pair<PacketStream*, PacketEngineListener*>(&stream, &listener);
    channelInfoLock.Unlock();
    rxPacketThread.Alert();
    return ER_OK;
}

QStatus PacketEngine::RemovePacketStream(PacketStream& pktStream)
{
    QCC_DbgTrace(("PacketEngine::RemovePacketStream(%p)", &pktStream));

    QStatus status = ER_OK;

    /* Abruptly disconnect any channels that are still using pktStream */
    ChannelInfo* ci = NULL;
    while ((ci = AcquireNextChannelInfo(ci)) != NULL) {
        if (&ci->packetStream == &pktStream) {
            QCC_DbgPrintf(("PacketEngine: Disconnecting PacketEngineStream %p because its PacketStream (%p) has been removed", &ci->stream, &ci->packetStream));
            Disconnect(ci->stream);
            /* Wait for ci to be closed */
            while (ci && isRunning && (ci->state != ChannelInfo::CLOSED)) {
                uint32_t chanId = ci->id;
                ReleaseChannelInfo(*ci);
                qcc::Sleep(10);
                ci = AcquireChannelInfo(chanId);
            }
        }
    }

    /* Remove packetStream itself */
    channelInfoLock.Lock();
    map<Event*, pair<PacketStream*, PacketEngineListener*> >::iterator it = packetStreams.find(&pktStream.GetSourceEvent());
    if (it != packetStreams.end()) {
        packetStreams.erase(it);
        rxPacketThreadReload = false;
        channelInfoLock.Unlock();
        rxPacketThread.Alert();
        while (isRunning && !rxPacketThreadReload && (Thread::GetThread() != &rxPacketThread)) {
            qcc::Sleep(20);
        }
    } else {
        channelInfoLock.Unlock();
        status = ER_FAIL;
        QCC_LogError(status, ("Cannot find PacketStream"));
    }
    return status;
}

QStatus PacketEngine::Connect(const PacketDest& dest, PacketStream& packetStream, PacketEngineListener& listener, void* context)
{
    QCC_DbgTrace(("PacketEngine::Connect(%s)", ToString(packetStream, dest).c_str()));

    /* Generate a new channel id */
    uint32_t chanId;
    QStatus status = qcc::Crypto_GetRandomBytes(reinterpret_cast<uint8_t*>(&chanId), sizeof(chanId));
    if (status != ER_OK) {
        return status;
    }

    /* Create the connect request */
    ConnectReqAlarmContext* cctx = new ConnectReqAlarmContext(chanId, dest, context);
    cctx->connReq[0] = htole32(PACKET_COMMAND_CONNECT_REQ);
    cctx->connReq[1] = htole32(PACKET_ENGINE_VERSION);
    cctx->connReq[2] = htole32(maxWindowSize);

    /* Create a channel info */
    ChannelInfo* ci = CreateChannelInfo(chanId, dest, packetStream, listener, maxWindowSize);
    if (ci) {
        /* Put an entry on the callback timer */
        uint32_t zero = 0;
        uint32_t timeout = CONNECT_RETRY_TIMEOUT;
        qcc::AlarmListener* packetEngineListener = this;
        ci->connectReqAlarm = Alarm(timeout, packetEngineListener, cctx, zero);
        status = timer.AddAlarm(ci->connectReqAlarm);
        if (status == ER_OK) {
            /* Send connect request */
            status = DeliverControlMsg(*ci, cctx->connReq, sizeof(cctx->connReq));
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to send CONNECT_REQ"));
            }
        } else {
            /* Immediately abort */
            ci->state = ChannelInfo::CLOSED;
        }
        ReleaseChannelInfo(*ci);
    } else {
        /* Cant create channel */
        status = ER_PACKET_CHANNEL_FAIL;
        delete cctx;
    }
    return status;
}

void PacketEngine::CloseChannel(ChannelInfo& ci)
{
    QCC_DbgTrace(("PacketEngine::CloseChannel(id=0x%x)", ci.id));

    /* Return early if disconnect already in progress */
    ci.txLock.Lock();
    DisconnectReqAlarmContext* ctx = static_cast<DisconnectReqAlarmContext*>(ci.disconnectReqAlarm->GetContext());
    if (ctx) {
        ci.txLock.Unlock();
        return;
    }

    /* Create disconnect context */
    ctx = new DisconnectReqAlarmContext(ci.id);
    ctx->disconnReq[0] = htole32(PACKET_COMMAND_DISCONNECT_REQ);
    uint32_t timeout = DISCONNECT_RETRY_TIMEOUT;
    uint32_t zero = 0;
    qcc::AlarmListener* packetEngineListener = this;
    ci.disconnectReqAlarm = Alarm(timeout, packetEngineListener, ctx, zero);

    /* Update state and send the message */
    ci.state = ChannelInfo::CLOSING;
    QStatus status = DeliverControlMsg(ci, ctx->disconnReq, sizeof(ctx->disconnReq));
    if (status == ER_OK) {
        status = timer.AddAlarm(ci.disconnectReqAlarm);
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("PacketEngine::CloseChannel failed. Deleting chan=0x%x", ci.id));
        ci.state = ChannelInfo::CLOSED;
    }
    ci.txLock.Unlock();
}

void PacketEngine::Disconnect(PacketEngineStream& stream)
{
    QCC_DbgTrace(("PacketEngine::Disconnect(%p)", &stream));

    /* Get the channel info */
    ChannelInfo* ci = AcquireChannelInfo(stream.chanId);
    if (ci) {
        CloseChannel(*ci);
        ReleaseChannelInfo(*ci);
    }
}

QStatus PacketEngine::DeliverControlMsg(PacketEngine::ChannelInfo& ci, const void* buf, size_t len, uint16_t seqNum)
{
    /* Check size of caller's message */
    size_t maxPayload = pool.GetMTU() - Packet::payloadOffset;
    if (len > maxPayload) {
        return ER_PACKET_TOO_LARGE;
    }

    /* Write packet */
    Packet* p = pool.GetPacket();
    p->SetPayload(reinterpret_cast<const uint8_t*>(buf), len);
    p->chanId = ci.id;
    p->seqNum = seqNum;
    p->flags = PACKET_FLAG_CONTROL;
    p->expireTs = static_cast<uint64_t>(-1);
    ci.txLock.Lock();
    ci.txControlQueue.push_back(p);
    ci.txLock.Unlock();
    QStatus status = txPacketThread.Alert();
    return status;
}

void PacketEngine::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    AlarmContext* ctx = reinterpret_cast<AlarmContext*>(alarm->GetContext());
    //printf("rx(%d): AlarmTriggered cctx=%d\n", (GetTimestamp() / 100) % 100000, ctx->contextType);

    // To make klocwork happy
    if (!ctx) {
        return;
    }

    switch (ctx->contextType) {
    case AlarmContext::CONTEXT_DISCONNECT_REQ:
        {
            /* Retry the DISCONNECT_REQ if retries still remain */
            DisconnectReqAlarmContext* cctx = static_cast<DisconnectReqAlarmContext*>(ctx);
            ChannelInfo* ci = AcquireChannelInfo(cctx->chanId);
            QStatus status = ER_FAIL;
            if (ci) {
                if ((++cctx->retries < DISCONNECT_RETRIES) && (ci->state == ChannelInfo::CLOSING)) {
                    QCC_DbgPrintf(("PacketEngine: cid=0x%x disconnect timeout. Retrying...", ci->id));
                    /* Rearm the timer and resend the disconnect request */
                    status = DeliverControlMsg(*ci, cctx->disconnReq, sizeof(cctx->disconnReq));
                    if (status == ER_OK) {
                        uint32_t timeout = DISCONNECT_RETRY_TIMEOUT * cctx->retries;
                        uint32_t zero = 0;
                        qcc::AlarmListener* packetEngineListener = this;
                        ci->disconnectReqAlarm = Alarm(timeout, packetEngineListener, ctx, zero);
                        status = timer.AddAlarm(ci->disconnectReqAlarm);
                    }
                }
                if (status != ER_OK) {
                    QCC_LogError(status, ("PacketEngine: cid=0x%x disconnect failed. Closing channel.", ci->id));
                    ci->state = ChannelInfo::CLOSED;
                }
                ReleaseChannelInfo(*ci);
            }
            break;
        }

    case AlarmContext::CONTEXT_DISCONNECT_RSP:
        {
            /* Done waiting for DISCONNECT_REQ retries from remote. Close channel */
            DisconnectRspAlarmContext* cctx = static_cast<DisconnectRspAlarmContext*>(ctx);
            ChannelInfo* ci = AcquireChannelInfo(cctx->chanId);
            if (ci) {
                QCC_DbgPrintf(("Received DisconnectRsp for id=0x%x", ci->id));
                ci->state = ChannelInfo::CLOSED;
                ReleaseChannelInfo(*ci);
            }
            break;
        }

    case AlarmContext::CONTEXT_CONNECT_REQ:
        {
            ConnectReqAlarmContext* cctx = static_cast<ConnectReqAlarmContext*>(ctx);
            ChannelInfo* ci = AcquireChannelInfo(cctx->chanId);
            QStatus status = ER_FAIL;
            if (ci) {
                if (++cctx->retries < CONNECT_RETRIES) {
                    /* Rearm the timer and resend the connect request */
                    status = DeliverControlMsg(*ci, cctx->connReq, sizeof(cctx->connReq));
                    if (status == ER_OK) {
                        uint32_t timeout = CONNECT_RETRY_TIMEOUT * cctx->retries;
                        uint32_t zero = 0;
                        qcc::AlarmListener* packetEngineListener = this;
                        ci->connectReqAlarm = Alarm(timeout, packetEngineListener, ctx, zero);
                        status = timer.AddAlarm(ci->connectReqAlarm);
                    }
                }
                if (status != ER_OK) {
                    /* Retries exhauseted. Notify the connect cb and remove the context and channel */
                    QCC_DbgPrintf(("PacketEngine: cid=0x%x connnect response timeout", ci->id));
                    ci->listener.PacketEngineConnectCB(*this, ER_PACKET_CONNECT_TIMEOUT, NULL, cctx->dest, cctx->context);
                    ci->state = ChannelInfo::CLOSED;
                }
                ReleaseChannelInfo(*ci);
            }
            break;
        }

    case AlarmContext::CONTEXT_CONNECT_RSP:
        {
            ConnectRspAlarmContext* cctx = static_cast<ConnectRspAlarmContext*>(ctx);
            ChannelInfo* ci = AcquireChannelInfo(cctx->chanId);
            QStatus status = ER_FAIL;
            if (ci) {
                if (++cctx->retries < CONNECT_RETRIES) {
                    /* Rearm the timer and resend the connect response */
                    status = DeliverControlMsg(*ci, cctx->connRsp, sizeof(cctx->connRsp));
                    if (status == ER_OK) {
                        uint32_t zero = 0;
                        uint32_t timeout = CONNECT_RETRY_TIMEOUT * cctx->retries;
                        qcc::AlarmListener* packetEngineListener = this;
                        ci->connectRspAlarm = Alarm(timeout, packetEngineListener, ctx, zero);
                        status = timer.AddAlarm(ci->connectRspAlarm);
                    }
                }
                if (status != ER_OK) {
                    /* Retries exhauseted. */
                    QCC_DbgPrintf(("PacketEngine: cid=0x%x connect response ack timeout", ci->id));
                    ci->state = ChannelInfo::CLOSED;
                }
                ReleaseChannelInfo(*ci);
            }
            break;
        }

    case AlarmContext::CONTEXT_XON:
        {
            XOnAlarmContext* cctx = static_cast<XOnAlarmContext*>(ctx);

            // To make klocwork happy
            if (!cctx) {
                break;
            }

            ChannelInfo* ci = AcquireChannelInfo(cctx->chanId);
            QStatus status = ER_FAIL;
            if (ci) {
                ci->rxLock.Lock();
                /* Retry the Xon and reload the alarm only if this alarm is the
                 * latest Xon alarm that has been setup. */
                if (cctx->xoffSeqNum == ci->rxFlowSeqNum) {
                    if (++cctx->retries < XON_RETRIES) {
                        /* Rearm the timer and resend the XON */
                        cctx->xon[1] = htole32(ci->rxAck);
                        cctx->xon[2] = htole32(ci->rxDrain);
                        status = DeliverControlMsg(*ci, cctx->xon, sizeof(cctx->xon), ci->rxFlowSeqNum);
                        if (status != ER_OK) {
                            QCC_LogError(status, ("Failed to send XON"));
                        }

                        uint32_t nextTime = GetRetryMs(*ci, cctx->retries);
                        uint32_t zero = 0;
                        qcc::AlarmListener* packetEngineListener = this;
                        ci->xOnAlarm = Alarm(nextTime, packetEngineListener, ctx, zero);
                        status = timer.AddAlarm(ci->xOnAlarm);
                        //printf("rx(%d): xon retry=%d rxD=0x%x, next=%d\n", (GetTimestamp() / 100) % 100000, cctx->retries + 1, ci->rxDrain, nextTime);
                    }
                } else {
                    QCC_DbgPrintf(("PacketEngine: cid=0x%x Not retrying stale XON", ci->id));

                    /* Delete the context associated with the stale Xon alarm */
                    if (cctx) {
                        delete cctx;
                        cctx = NULL;
                    }

                    status = ER_OK;
                }

                ci->rxLock.Unlock();
                if (status != ER_OK) {
                    /* Retries exhauseted. */
                    QCC_DbgPrintf(("PacketEngine: cid=0x%x XON retries exhausted. Attempting graceful disconnect", ci->id));
                    CloseChannel(*ci);
                }
                ReleaseChannelInfo(*ci);
            }
            break;
        }

    case AlarmContext::CONTEXT_DELAY_ACK:
        {
            DelayAckAlarmContext* cctx = static_cast<DelayAckAlarmContext*>(ctx);
            ChannelInfo* ci = AcquireChannelInfo(cctx->chanId);
            if (ci) {
                ci->rxLock.Lock();
                ci->isAckAlarmArmed = false;
                SendAckNow(*ci, ci->rxAdvancedSeqNum);
                ci->rxLock.Unlock();
                ReleaseChannelInfo(*ci);
            }
            break;
        }

    case AlarmContext::CONTEXT_CLOSING:
        {
            ClosingAlarmContext* cctx = static_cast<ClosingAlarmContext*>(ctx);
            ChannelInfo* ci = AcquireChannelInfo(cctx->chanId);
            if (ci) {
                QCC_DbgPrintf(("PacketEngine::AlarmTriggered(CLOSING_CONTEXT): Closing id=0x%x", ci->id));
                ci->state = ChannelInfo::CLOSED;
                ReleaseChannelInfo(*ci);
            }
            break;
        }

    default:
        {
            uint32_t t = static_cast<uint32_t>(ctx->contextType);
            QCC_LogError(ER_FAIL, ("Received AlarmContext with unknown type (%u)", t));
            break;
        }
    }
}

PacketEngine::ChannelInfo::ChannelInfo(PacketEngine& engine, uint32_t id, const PacketDest& dest, PacketStream& packetStream,
                                       PacketEngineListener& listener, uint16_t windowSize) :
    engine(engine),
    id(id),
    state(OPENING),
    dest(dest),
    sourceEvent(),
    sinkEvent(),
    stream(engine, id, sourceEvent, sinkEvent),
    packetStream(packetStream),
    listener(listener),
    useCount(0),
    connectReqAlarm(),
    connectRspAlarm(),
    disconnectReqAlarm(),
    disconnectRspAlarm(),
    xOnAlarm(),
    ackAlarmContext(new DelayAckAlarmContext(id)),
    isAckAlarmArmed(false),
    rxFill(0),
    rxDrain(0),
    rxAck(0),
    rxPayloadOffset(0),
    rxAdvancedSeqNum(0),
    rxFlowOff(false),
    rxFlowSeqNum(0),
    rxIsMidMessage(false),
    txFill(0),
    txDrain(0),
    remoteRxDrain(0),
    xOffSeqNum(0),
    txControlQueue(),
    txRttMean(0),
    txRttMeanVar(0),
    txRttInit(false),
    txCongestionWindow(1),
    txSlowStartThresh(windowSize),
    txConsecutiveAcks(0),
    txLastMarshalSeqNum(numeric_limits<uint16_t>::max()),
    protocolVersion(0),
    windowSize(windowSize),
    wasOpen(false)
{
    /* Initialize rxPackets, txPackets and rxMask */
    rxPackets = new Packet *[windowSize];
    for (size_t i = 0; i < windowSize; ++i) {
        rxPackets[i] = NULL;
    }
    txPackets = new Packet *[windowSize];
    for (size_t i = 0; i < windowSize; ++i) {
        txPackets[i] = NULL;
    }
    rxMaskSize = windowSize / 8;
    rxMask = new uint32_t[rxMaskSize / sizeof(uint32_t)];
    ::memset(rxMask, 0, rxMaskSize);

    /* create ack response buffer */
    ackResp = new uint32_t[3 + rxMaskSize / sizeof(uint32_t)];

    /* Initialize sink Event */
    sinkEvent.SetEvent();
}

PacketEngine::ChannelInfo::ChannelInfo(const ChannelInfo& other) :
    engine(other.engine),
    id(other.id),
    state(other.state),
    dest(other.dest),
    sourceEvent(),
    sinkEvent(),
    stream(*other.stream.engine, other.id, sourceEvent, sinkEvent),
    packetStream(other.packetStream),
    listener(other.listener),
    useCount(other.useCount),
    connectReqAlarm(other.connectReqAlarm),
    connectRspAlarm(other.connectRspAlarm),
    disconnectReqAlarm(other.disconnectReqAlarm),
    disconnectRspAlarm(other.disconnectRspAlarm),
    xOnAlarm(other.xOnAlarm),
    ackAlarmContext(new DelayAckAlarmContext(other.id)),
    isAckAlarmArmed(false),
    rxFill(other.rxFill),
    rxDrain(other.rxDrain),
    rxAck(other.rxAck),
    rxPayloadOffset(other.rxPayloadOffset),
    rxAdvancedSeqNum(other.rxAdvancedSeqNum),
    rxFlowOff(other.rxFlowOff),
    rxFlowSeqNum(other.rxFlowSeqNum),
    rxIsMidMessage(other.rxIsMidMessage),
    txFill(other.txFill),
    txDrain(other.txDrain),
    remoteRxDrain(other.remoteRxDrain),
    xOffSeqNum(other.xOffSeqNum),
    txControlQueue(),
    txRttMean(other.txRttMean),
    txRttMeanVar(other.txRttMeanVar),
    txRttInit(other.txRttInit),
    txCongestionWindow(other.txCongestionWindow),
    txSlowStartThresh(other.txSlowStartThresh),
    txConsecutiveAcks(other.txConsecutiveAcks),
    txLastMarshalSeqNum(other.txLastMarshalSeqNum),
    protocolVersion(other.protocolVersion),
    windowSize(other.windowSize),
    wasOpen(other.wasOpen)
{
    /* Initialize rxPackets, txPackets and rxMask */
    rxPackets = new Packet *[windowSize];
    for (size_t i = 0; i < windowSize; ++i) {
        rxPackets[i] = NULL;
    }
    txPackets = new Packet *[windowSize];
    for (size_t i = 0; i < windowSize; ++i) {
        txPackets[i] = NULL;
    }
    rxMaskSize = windowSize / 8;
    rxMask = new uint32_t[rxMaskSize / sizeof(uint32_t)];
    ::memset(rxMask, 0, rxMaskSize - (rxMaskSize % sizeof(uint32_t)));

    /* create ack response buffer */
    ackResp = new uint32_t[3 + rxMaskSize / sizeof(uint32_t)];

    /* Initialize sink Event */
    sinkEvent.SetEvent();
}

PacketEngine::ChannelInfo::~ChannelInfo()
{
    for (size_t i = 0; i < windowSize; ++i) {
        if (txPackets[i] != NULL) {
            engine.pool.ReturnPacket(txPackets[i]);
            txPackets[i] = NULL;
        }
        if (rxPackets[i] != NULL) {
            engine.pool.ReturnPacket(rxPackets[i]);
            rxPackets[i] = NULL;
        }
    }

    while (engine.isRunning && (useCount > 0)) {
        qcc::Sleep(5);
    }

    AlarmContext* ac = static_cast<AlarmContext*>(connectReqAlarm->GetContext());
    if (ac) {
        engine.timer.RemoveAlarm(connectReqAlarm);
        delete ac;
    }
    ac = static_cast<AlarmContext*>(connectRspAlarm->GetContext());
    if (ac) {
        engine.timer.RemoveAlarm(connectRspAlarm);
        delete ac;
    }
    ac = static_cast<AlarmContext*>(disconnectReqAlarm->GetContext());
    if (ac) {
        engine.timer.RemoveAlarm(disconnectReqAlarm);
        delete ac;
    }
    ac = static_cast<AlarmContext*>(disconnectRspAlarm->GetContext());
    if (ac) {
        engine.timer.RemoveAlarm(disconnectRspAlarm);
        delete ac;
    }
    ac = static_cast<AlarmContext*>(xOnAlarm->GetContext());
    if (ac) {
        engine.timer.RemoveAlarm(xOnAlarm);
        delete ac;
    }

    txLock.Lock();
    while (!txControlQueue.empty()) {
        engine.pool.ReturnPacket(txControlQueue.front());
        txControlQueue.pop_front();
    }
    txLock.Unlock();

    delete ackAlarmContext;
    delete[] rxPackets;
    delete[] txPackets;
    delete[] rxMask;
    delete[] ackResp;
}

PacketEngine::ChannelInfo* PacketEngine::CreateChannelInfo(uint32_t chanId, const PacketDest& dest, PacketStream& packetStream,
                                                           PacketEngineListener& listener, uint16_t windowSize)
{
    ChannelInfo* ret = NULL;
    channelInfoLock.Lock();
    if (channelInfos.find(chanId) == channelInfos.end()) {
        /* Make sure packetStream is still on the list while holding channelInfos lock */
        bool found = false;
        map<Event*, pair<PacketStream*, PacketEngineListener*> >::iterator it = packetStreams.begin();
        while (it != packetStreams.end()) {
            if (it->second.first == &packetStream) {
                found = true;
                break;
            }
            ++it;
        }

        /* Add ChannelInfo if packetStream was valid */
        if (found) {
            ret = (channelInfos.insert(pair<uint32_t, ChannelInfo*>(chanId, new ChannelInfo(*this, chanId, dest, packetStream, listener, windowSize))).first->second);
            ret->useCount = 1;
        }
    }
    channelInfoLock.Unlock();
    return ret;
}

PacketEngine::ChannelInfo* PacketEngine::AcquireChannelInfo(uint32_t chanId)
{
    ChannelInfo* ret = NULL;
    channelInfoLock.Lock();
    map<uint32_t, ChannelInfo*>::iterator it = channelInfos.find(chanId);
    if (it != channelInfos.end()) {
        ret = it->second;
        ret->useCount++;
    }
    channelInfoLock.Unlock();
    return ret;
}

PacketEngine::ChannelInfo* PacketEngine::AcquireNextChannelInfo(PacketEngine::ChannelInfo* inCi)
{
    ChannelInfo* ret = NULL;
    channelInfoLock.Lock();
    map<uint32_t, ChannelInfo*>::iterator it = channelInfos.begin();
    if (inCi) {
        it = channelInfos.find(inCi->id);
        if (it != channelInfos.end()) {
            ++it;
        }
    }
    if (it != channelInfos.end()) {
        ret = it->second;
        ret->useCount++;
    }
    channelInfoLock.Unlock();
    if (inCi) {
        ReleaseChannelInfo(*inCi);
    }
    return ret;
}

void PacketEngine::ReleaseChannelInfo(ChannelInfo& ci)
{
    channelInfoLock.Lock();
    if ((--ci.useCount == 0) && (ci.state == ChannelInfo::CLOSED)) {
        PacketEngineStream stream = ci.stream;
        PacketEngineListener& listener = ci.listener;
        PacketDest dest = ci.dest;
        /* Erase entry in channelInfos */
        map<uint32_t, ChannelInfo*>::iterator it = channelInfos.find(ci.id);
        ChannelInfo* ciEntry = it->second;
        channelInfos.erase(ci.id);

        /* Notify disconnect cb (Must be done without holding channelInfoLock) */
        channelInfoLock.Unlock();
        listener.PacketEngineDisconnectCB(*this, stream, dest);
        delete ciEntry;
    } else {
        channelInfoLock.Unlock();
    }
}

void PacketEngine::SendAck(ChannelInfo& ci, uint16_t seqNum, bool allowDelay)
{
    QCC_DbgTrace(("SendAck(dst=%s, seqNum=0x%x, allowDelay=%s, rxDrain=0x%x, rxAck=0x%x)",
                  ToString(ci.packetStream, ci.dest).c_str(), seqNum, allowDelay ? "true" : "false", ci.rxDrain, ci.rxAck));

    /* Decide between delayed and immediate ack */
    if ((ACK_DELAY_MS > 0) && allowDelay) {
        ci.rxLock.Lock();
        if (!ci.isAckAlarmArmed) {
            uint32_t zero = 0;
            uint32_t timeout = ACK_DELAY_MS;
            qcc::AlarmListener* packetEngineListener = this;
            Alarm a(timeout, packetEngineListener, ci.ackAlarmContext, zero);
            QStatus status = timer.AddAlarm(a);
            ci.isAckAlarmArmed = (status == ER_OK);
            if (status != ER_OK) {
                QCC_LogError(status, ("SendAck failed to add alarm"));
            }
        }
        ci.rxLock.Unlock();
    } else {
        /* ACK_DELAY_MS == 0. Send ack synchronously */
        SendAckNow(ci, seqNum);
    }
}

void PacketEngine::SendAckNow(ChannelInfo& ci, uint16_t seqNum)
{
    QCC_DbgTrace(("SendAckNow(dst=%s, seqNum=0x%x, rxDrain=0x%x, rxAck=0x%x)", ToString(ci.packetStream, ci.dest).c_str(), seqNum, ci.rxDrain, ci.rxAck));
    ci.rxLock.Lock();
    ci.ackResp[0] = htole32(PACKET_COMMAND_ACK);
    ci.ackResp[1] = htole32(ci.rxAck);
    ci.ackResp[2] = htole32(ci.rxDrain);
    for (size_t i = 0; i < (ci.rxMaskSize / sizeof(uint32_t)); ++i) {
        ci.ackResp[3 + i] = htole32(ci.rxMask[i]);
    }
    ci.rxLock.Unlock();
    QStatus status = DeliverControlMsg(ci, ci.ackResp, 3 * sizeof(uint32_t) + ci.rxMaskSize, seqNum);
    if (status != ER_OK) {
        QCC_LogError(status, ("SendAckNow failed"));
    }
}

uint32_t PacketEngine::GetRetryMs(const ChannelInfo& ci, uint32_t sendAttempt) const
{
    /*
     * Retry delay = backoff * max(1000, txRttMean + (4 * txRttMeanVar))
     */
    uint32_t ret = ci.txRttInit ? ::min(8, (1 << (sendAttempt - 1))) * ::max((uint32_t)1000, static_cast<uint32_t>((ci.txRttMean + (4 * ci.txRttMeanVar)) >> 10)) : 3000;
    return ret;
}

void PacketEngine::SendXOn(ChannelInfo& ci)
{
    QCC_DbgTrace(("PacketEngine::SendXOn(chan=0x%x, rxFill=0x%x, rxDrain=0x%x, rxAck=0x%x, rxFlowSeqNum=0x%x)", ci.id, ci.rxFill, ci.rxDrain, ci.rxAck, ci.rxFlowSeqNum));
    /* Create the XON context and message */
    //printf("rx(%d): xon rF=0x%x, rD=0x%x, rA=0x%x, flowSeq=0x%x\n", (GetTimestamp() / 100) % 100000, ci.rxFill, ci.rxDrain, ci.rxAck, ci.rxFlowSeqNum);
    ci.rxLock.Lock();

    XOnAlarmContext* cctx = new XOnAlarmContext(ci.id, ci.rxFlowSeqNum);
    cctx->xon[0] = htole32(PACKET_COMMAND_XON);
    cctx->xon[1] = htole32(ci.rxAck);
    cctx->xon[2] = htole32(ci.rxDrain);
    uint32_t zero = 0;
    uint32_t timeout = GetRetryMs(ci, ++cctx->retries);
    qcc::AlarmListener* packetEngineListener = this;
    ci.xOnAlarm = Alarm(timeout, packetEngineListener, cctx, zero);
    QStatus status = timer.AddAlarm(ci.xOnAlarm);
    if (status == ER_OK) {
        status = DeliverControlMsg(ci, cctx->xon, sizeof(cctx->xon), ci.rxFlowSeqNum);
    } else {
        QCC_LogError(status, ("PacketEngine::SendXON failed"));
        delete cctx;
        ci.xOnAlarm = Alarm();
    }

    ci.rxLock.Unlock();
}

PacketEngine::RxPacketThread::RxPacketThread(const qcc::String& engineName) : Thread(engineName + "-rx"), engine(NULL)
{
}

qcc::ThreadReturn STDCALL PacketEngine::RxPacketThread::Run(void* arg)
{
    engine = reinterpret_cast<PacketEngine*>(arg);
    vector<Event*> checkEvents, sigEvents;
    QStatus status = ER_OK;
    Event& stopEvent = GetStopEvent();
    while (!IsStopping() && (status == ER_OK)) {
        checkEvents.clear();
        sigEvents.clear();
        checkEvents.push_back(&stopEvent);
        engine->rxPacketThreadReload = true;
        engine->channelInfoLock.Lock();
        map<Event*, pair<PacketStream*, PacketEngineListener*> >::iterator sit = engine->packetStreams.begin();
        while (sit != engine->packetStreams.end()) {
            checkEvents.push_back(sit->first);
            sit++;
        }
        engine->channelInfoLock.Unlock();
        status = Event::Wait(checkEvents, sigEvents, Event::WAIT_FOREVER);
        if (status == ER_OK) {
            while (!sigEvents.empty()) {
                engine->channelInfoLock.Lock();
                map<Event*, pair<PacketStream*, PacketEngineListener*> >::const_iterator it = engine->packetStreams.find(sigEvents.back());
                if (it != engine->packetStreams.end()) {
                    PacketStream& stream = *(it->second.first);
                    PacketEngineListener& listener = *(it->second.second);
                    Packet* p = engine->pool.GetPacket();
                    status = p->Unmarshal(stream);
                    engine->channelInfoLock.Unlock();
                    if (status == ER_OK) {
                        /* Handle control or data packet */
                        if (p->flags & PACKET_FLAG_CONTROL) {
                            HandleControlPacket(p, stream, listener);
                        } else {
                            HandleDataPacket(p);
                        }
                    } else {
                        /* Failed to unmarshal a single packet. This is not fatal */
                        QCC_DbgPrintf(("Packet::Unmarshal failed with %s", QCC_StatusText(status)));
                        engine->pool.ReturnPacket(p);
                        status = ER_OK;
                    }
                } else {
                    engine->channelInfoLock.Unlock();
                    if (sigEvents.back() == &stopEvent) {
                        GetStopEvent().ResetEvent();
                    }
                }
                sigEvents.pop_back();
            }
        }
    }
    if (status != ER_STOPPING_THREAD) {
        QCC_DbgPrintf(("RxPacketThread::Run() exiting with %s", QCC_StatusText(status)));
    }
    return (qcc::ThreadReturn) status;
}

void PacketEngine::RxPacketThread::HandleControlPacket(Packet* p, PacketStream& packetStream, PacketEngineListener& listener)
{
    uint32_t cmd = letoh32(p->payload[0]);
    switch (cmd) {
    case PACKET_COMMAND_CONNECT_REQ:
        HandleConnectReq(p, packetStream, listener);
        break;

    case PACKET_COMMAND_CONNECT_RSP:
        HandleConnectRsp(p);
        break;

    case PACKET_COMMAND_CONNECT_RSP_ACK:
        HandleConnectRspAck(p);
        break;

    case PACKET_COMMAND_DISCONNECT_REQ:
        HandleDisconnectReq(p);
        break;

    case PACKET_COMMAND_DISCONNECT_RSP:
        HandleDisconnectRsp(p);
        break;

    case PACKET_COMMAND_ACK:
        HandleAck(p);
        break;

    case PACKET_COMMAND_XON:
        HandleXOn(p);
        break;

    case PACKET_COMMAND_XON_ACK:
        HandleXOnAck(p);
        break;

    default:
        break;
    }
    engine->pool.ReturnPacket(p);
}

void PacketEngine::RxPacketThread::HandleDataPacket(Packet* p)
{
    QCC_DbgTrace(("HandleDataPacket(seqNum=0x%x, payloadLen=%d, flow=%s)", p->seqNum, p->payloadLen, (p->flags & PACKET_FLAG_FLOW_OFF) ? "off" : "nc"));

    /* Get the channel info for this packet */
    ChannelInfo* ci = engine->AcquireChannelInfo(p->chanId);
    if (ci) {
        /* Validate that packet is in the window */
        ci->rxLock.Lock();
        if (IN_WINDOW(uint16_t, ci->rxDrain, ci->windowSize - 1, p->seqNum)) {
            uint16_t seqNum = p->seqNum;
            uint32_t idx = (seqNum % ci->windowSize);
            if (ci->rxPackets[idx] == NULL) {
                /* Received in-range packet */
                ci->rxPackets[idx] = p;

                /* Monitor flow off */
                if (p->flags & PACKET_FLAG_FLOW_OFF) {
                    ci->rxFlowOff = true;
                    ci->rxFlowSeqNum = seqNum;

                    /* Check to see if gratuitous XON is needed */
                    /* Flow on is triggered if packet that caused flow off is not at end of rcv window or if rcv buf is empty */
                    if ((ci->rxDrain == ci->rxAck) || IN_WINDOW(uint16_t, ci->rxDrain, ci->windowSize - 2 - XON_THRESHOLD, ci->rxFlowSeqNum)) {
                        ci->rxFlowOff = false;
                        engine->SendXOn(*ci);
                    }
                }

                /* Update rxmask */
                ci->rxMask[idx / 32] |= (0x01 << (idx % 32));
                //printf("rx(%d): m[0x%x]|=%x (s=0x%x)\n", (GetTimestamp() / 100) % 100000, idx / 32, (0x01 << (idx % 32)), seqNum);

                /* Track hightest acked packet in window */
                uint16_t drain = (ci->rxDrain == 0) ? (ci->windowSize - 1) : (ci->rxDrain - 1);
                uint16_t ackSize = ci->rxAdvancedSeqNum - drain;
                if (ackSize > ci->windowSize) {
                    ackSize += ci->windowSize;
                }
                if (!IN_WINDOW(uint16_t, drain, ackSize, p->seqNum)) {
                    ci->rxAdvancedSeqNum = p->seqNum;
                }

                /* Check for complete message (both PACKET_FLAG_BOM and PACKET_FLAG_EOM) and advance rxFill if necessary */
                uint16_t tIdx = ci->rxAdvancedSeqNum % ci->windowSize;
                uint16_t fillIdx = ((ci->rxFill == 0) ? (ci->windowSize - 1) : (ci->rxFill - 1)) % ci->windowSize;
                uint16_t ackIdx = ((ci->rxAck == 0) ? (ci->windowSize - 1) : (ci->rxAck - 1)) % ci->windowSize;
                uint16_t nextRxAck = ci->rxAdvancedSeqNum;
                uint16_t nextRxFill = ci->rxAdvancedSeqNum;
                uint16_t gap = 0;
                bool isExpired = false;
                bool isRxFillSet = false;
                bool isRxAckSet = false;
                enum {MISSING, IN_MSG, OUT_MSG} state = MISSING;
                //printf("rx_enter(%d): advSeq=0x%x, rxFill=0x%x, rxAck=0x%x\n", (GetTimestamp() / 100) % 100000, ci->rxAdvancedSeqNum, ci->rxFill, ci->rxAck);
                while (!((tIdx == fillIdx) || ((tIdx == ackIdx) && !isExpired))) {
                    Packet*& tp = ci->rxPackets[tIdx];
                    //printf("rx(%d): state=%d, tIdx=0x%x, seqNum=0x%x, gap=%d flags=0x%x\n", (GetTimestamp() / 100) % 100000, state, tIdx, tp ? tp->seqNum : 0, gap, tp ? tp->flags : 0x5555);

                    if (tp) {
                        assert(gap == 0);
                        gap = tp->gap;
                        if (!isRxAckSet) {
                            isRxAckSet = true;
                            nextRxAck = tp->seqNum + 1;
                        }
                        if (!isRxFillSet && (tp->flags & PACKET_FLAG_EOM)) {
                            isRxFillSet = true;
                            nextRxFill = tp->seqNum + 1;
                        } else if (!isRxFillSet && (gap > 0)) {
                            isRxFillSet = true;
                            nextRxFill = tp->seqNum;
                        }
                        if (isExpired && (tp->flags & PACKET_FLAG_BOM)) {
                            tp->expireTs = 0;
                        }
                        if (state == MISSING) {
                            state = (tp->flags & PACKET_FLAG_EOM) ? IN_MSG : OUT_MSG;
                        } else if (state == IN_MSG) {
                            state = (tp->flags & PACKET_FLAG_BOM) ? OUT_MSG : IN_MSG;
                        } else if (state == OUT_MSG) {
                            state = (tp->flags & PACKET_FLAG_EOM) ? IN_MSG : OUT_MSG;
                        }
                    } else {
                        if (gap) {
                            --gap;
                            isExpired = true;
                            state = OUT_MSG;
                        } else {
                            isRxFillSet = false;
                            isRxAckSet = false;
                            state = MISSING;
                        }
                    }
                    tIdx = (tIdx == 0) ? (ci->windowSize - 1) : (tIdx - 1);
                }

                /* Advance rxFill and rxAck if indicated */
                if (isRxAckSet) {
                    /* Clear rxMask bits between [rxAck, nextRxAck) (with wrap-around) */
                    uint16_t from = ci->rxAck % ci->windowSize;
                    uint16_t to = nextRxAck % ci->windowSize;
                    uint16_t maxRxMaskIdx = ci->rxMaskSize / sizeof(uint32_t);
                    bool isInverted = from > to;
                    uint32_t fromMask = ((1 << (from % 32)) - 1);
                    uint32_t toMask = ~((1 << (to % 32)) - 1);
                    to = to / 32;
                    from = from / 32;
                    if (!isInverted && (to == from)) {
                        ci->rxMask[to] &= (fromMask | toMask);
                        //printf("rx(%d): m[0x%x]&=%x (s=0x%x)\n", (GetTimestamp() / 100) % 100000, to, fromMask | toMask, ci->rxAck);
                    } else {
                        ci->rxMask[from] &= fromMask;
                        //printf("rx(%d): m[0x%x]&=%x (s=0x%x)\n", (GetTimestamp() / 100) % 100000, from, fromMask, ci->rxAck);
                        ci->rxMask[to] &= toMask;
                        //printf("rx(%d): m[0x%x]&=%x (s=0x%x)\n", (GetTimestamp() / 100) % 100000, to, toMask, nextRxAck);
                        from = (from + 1) % maxRxMaskIdx;
                        while (from != to) {
                            ci->rxMask[from] = 0;
                            //printf("rx(%d): m[0x%x]&=0\n", (GetTimestamp() / 100) % 100000, from);
                            from = (from + 1) % maxRxMaskIdx;
                        }
                    }
                    /* Update rxfill and rxAck */
                    ci->rxAck = nextRxAck;
                    if (isRxFillSet) {
                        ci->rxFill = nextRxFill;
                        ci->sourceEvent.SetEvent();
                    }
                }
                //printf("rx_exit(%d): rxFill=0x%x, rxDrain=0x%x, rxAck=0x%x\n", (GetTimestamp() / 100) % 100000, ci->rxFill, ci->rxDrain, ci->rxAck);
            } else {
                /* Received resend */
                QCC_DbgPrintf(("Received resend of 0x%x from %s (existing=0x%x). Ignoring", seqNum, engine->ToString(ci->packetStream, p->GetSender()).c_str(), p->seqNum));
                engine->pool.ReturnPacket(p);
            }
            engine->SendAck(*ci, seqNum, (p->flags & PACKET_FLAG_DELAY_ACK));
            ci->rxLock.Unlock();
        } else {
            /*
             * Send ack even though packet appears to be outside the window
             * This is necessary when the transmitter misses an ack and therefore is out of sync.
             * The ack will (hopefully) get the transmitter back into sync.
             */
            engine->SendAck(*ci, p->seqNum, false);
            ci->rxLock.Unlock();
            QCC_DbgPrintf(("Received packet from %s with id 0x%x out of range [%x, %x)", engine->ToString(ci->packetStream, p->GetSender()).c_str(), p->seqNum, ci->rxDrain, (ci->rxDrain + ci->windowSize - 1) % ci->windowSize));
            engine->pool.ReturnPacket(p);
        }
        engine->ReleaseChannelInfo(*ci);
    } else {
        QCC_DbgPrintf(("Received packet with invalid chanId (0x%x)", p->chanId));
        engine->pool.ReturnPacket(p);
    }
}

void PacketEngine::RxPacketThread::HandleConnectReq(Packet* p, PacketStream& packetStream, PacketEngineListener& listener)
{
    QCC_DbgTrace(("PacketEngine::HandleConnectReq(%s)", engine->ToString(packetStream, p->GetSender()).c_str()));

    /* Make sure that this connect request doesn't already have a channel */
    uint32_t reqProtoVersion = letoh32(p->payload[1]);
    uint32_t reqWindowSize = letoh32(p->payload[2]);
    ChannelInfo* ci = engine->CreateChannelInfo(p->chanId, p->GetSender(), packetStream, listener, GetValidWindowSize(::min(engine->maxWindowSize, reqWindowSize)));
    if (ci) {
        /* Ask listener for to accept/reject */
        bool accepted = ci->listener.PacketEngineAcceptCB(*engine, ci->stream, ci->dest);
        ci->wasOpen = accepted;

        /* Update protocol version for this channel */
        ci->protocolVersion = ::min(reqProtoVersion, (uint32_t)PACKET_ENGINE_VERSION);

        /* Create the connect response */
        ConnectRspAlarmContext* cctx = new ConnectRspAlarmContext(ci->id, ci->dest);
        cctx->connRsp[0] = htole32(PACKET_COMMAND_CONNECT_RSP);
        cctx->connRsp[1] = htole32(ci->protocolVersion);
        cctx->connRsp[2] = htole32(accepted ? ER_OK : ER_BUS_CONNECTION_REJECTED);
        cctx->connRsp[3] = htole32(ci->windowSize);

        /* Put an entry on the callback timer */
        uint32_t timeout = CONNECT_RETRY_TIMEOUT;
        uint32_t zero = 0;
        ci->connectRspAlarm = Alarm(timeout, engine, cctx, zero);
        QStatus status = engine->timer.AddAlarm(ci->connectRspAlarm);

        if (status == ER_OK) {
            ci->state = ChannelInfo::OPENING;
            status = engine->DeliverControlMsg(*ci, cctx->connRsp, sizeof(cctx->connRsp));
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to send ConnectRsp to %s", engine->ToString(ci->packetStream, p->GetSender()).c_str()));
            }
            if (!accepted) {
                ci->state = ChannelInfo::CLOSING;
            }
        } else {
            /* Failed to add alarm */
            QCC_LogError(status, ("AddAlarm failed"));
            ci->state = ChannelInfo::CLOSED;
        }
        engine->ReleaseChannelInfo(*ci);
    }
}

void PacketEngine::RxPacketThread::HandleConnectRsp(Packet* p)
{

    uint32_t reqProtoVersion = letoh32(p->payload[1]);
    QStatus status = ER_OK;
    QStatus rspStatus = static_cast<QStatus>(letoh32(p->payload[2]));
    uint32_t reqWindowSize = letoh32(p->payload[3]);

    /* Channel for this connectRsp should already exist and should be in OPENING state */
    ChannelInfo* ci = engine->AcquireChannelInfo(p->chanId);
    QCC_DbgTrace(("PacketEngine::HandleConnectRsp(%s)", ci ? engine->ToString(ci->packetStream, p->GetSender()).c_str() : ""));
    if (ci) {
        ConnectReqAlarmContext* ctx = static_cast<ConnectReqAlarmContext*>(ci->connectReqAlarm->GetContext());
        if (ctx) {
            /* Disable any connectReqAlarm retry timer */
            engine->timer.RemoveAlarm(ci->connectReqAlarm);

            /* Call user callback (once) */
            if (ci->state == ChannelInfo::OPENING) {
                /* Validate protocol version and set window size */
                if (reqProtoVersion > PACKET_ENGINE_VERSION) {
                    rspStatus = ER_PACKET_BAD_PARAMETER;
                    QCC_LogError(rspStatus, ("Invalid PACKET_ENGINE_VERSION (%d) received in ConnectRsp from %s", reqProtoVersion, engine->ToString(ci->packetStream, ci->dest).c_str()));
                    rspStatus = ER_FAIL;
                }
                /* Validate window size */
                if (reqWindowSize > engine->maxWindowSize) {
                    rspStatus = ER_PACKET_BAD_PARAMETER;
                    QCC_LogError(ER_FAIL, ("Invalid WindowSize (%d) received in ConnectRsp from %s", reqWindowSize, engine->ToString(ci->packetStream, ci->dest).c_str()));
                }
                /* Update channelInfo and call the user's callback */
                ci->state = (rspStatus == ER_OK) ? ChannelInfo::OPEN : ChannelInfo::CLOSING;
                ci->windowSize = reqWindowSize;
                ci->wasOpen = (ci->state == ChannelInfo::OPEN);
                ci->listener.PacketEngineConnectCB(*engine, rspStatus, &ci->stream, ci->dest, ctx->context);

                /* Arm the close timer if needed */
                if (ci->state == ChannelInfo::CLOSING && !ci->closingAlarmContext) {
                    ci->closingAlarmContext = new ClosingAlarmContext(ci->id);
                    uint32_t timeout = CLOSING_TIMEOUT;
                    uint32_t zero = 0;
                    engine->timer.AddAlarm(Alarm(timeout, engine, ci->closingAlarmContext, zero));
                }
            } else if ((ci->state != ChannelInfo::OPEN) && (ci->state != ChannelInfo::CLOSING)) {
                /* Only allow retry of ack if state OPEN or CLOSING */
                status = ER_FAIL;
                QCC_LogError(status, ("Received unexpected ConnectRsp from %s (id=0x%x). Ignoring...", engine->ToString(ci->packetStream, ci->dest).c_str(), ci->id));
            }
        }

        /* Send Connect Response Ack */
        if (status == ER_OK) {
            uint32_t connRspAck[1];
            connRspAck[0] = htole32(PACKET_COMMAND_CONNECT_RSP_ACK);
            status = engine->DeliverControlMsg(*ci, connRspAck, sizeof(connRspAck));
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to send ConnectRspAck"));
            }
        }
        engine->ReleaseChannelInfo(*ci);
    }
}

void PacketEngine::RxPacketThread::HandleConnectRspAck(Packet* p)
{

    /* Channel for this connectRsp should already exist and should be in OPENING state */
    ChannelInfo* ci = engine->AcquireChannelInfo(p->chanId);
    ConnectRspAlarmContext* ctx = static_cast<ConnectRspAlarmContext*>(ci ? ci->connectRspAlarm->GetContext() : NULL);
    QCC_DbgTrace(("PacketEngine::HandleConnectRspAck(%s)", ci ? engine->ToString(ci->packetStream, p->GetSender()).c_str() : ""));
    if (ci && ctx) {
        /* Disable any connect(Rsp)Alarm retry timer */
        engine->timer.RemoveAlarm(ci->connectRspAlarm);
        ci->connectRspAlarm = Alarm();
        delete ctx;
        if (ci->state == ChannelInfo::OPENING) {
            ci->state = ChannelInfo::OPEN;
        }
    }
    if (ci) {
        engine->ReleaseChannelInfo(*ci);
    }
}

void PacketEngine::RxPacketThread::HandleDisconnectReq(Packet* p)
{
    ChannelInfo* ci = engine->AcquireChannelInfo(p->chanId);
    if (ci) {
        /* Create disconnect response context if necessary */
        DisconnectRspAlarmContext* ctx = static_cast<DisconnectRspAlarmContext*>(ci->disconnectRspAlarm->GetContext());
        if (!ctx) {
            ctx = new DisconnectRspAlarmContext(ci->id);
            ctx->disconnRsp[0] = htole32(PACKET_COMMAND_DISCONNECT_RSP);
            uint32_t timeout = DISCONNECT_TIMEOUT;
            uint32_t zero = 0;
            ci->disconnectRspAlarm = Alarm(timeout, engine, ctx, zero);
            engine->timer.AddAlarm(ci->disconnectRspAlarm);
            ci->state = ChannelInfo::CLOSING;
        }
        /* Send disconnect response */
        QStatus status = engine->DeliverControlMsg(*ci, ctx->disconnRsp, sizeof(ctx->disconnRsp));
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to send DisconnectReq to %s", engine->ToString(ci->packetStream, p->GetSender()).c_str()));
        }
        engine->ReleaseChannelInfo(*ci);
    }
}

void PacketEngine::RxPacketThread::HandleDisconnectRsp(Packet* p)
{
    ChannelInfo* ci = engine->AcquireChannelInfo(p->chanId);
    DisconnectReqAlarmContext* ctx = static_cast<DisconnectReqAlarmContext*>(ci ? ci->disconnectReqAlarm->GetContext() : NULL);
    if (ci && ctx) {
        /* Ignore disconnect rsp that has already timed out */
        engine->timer.RemoveAlarm(ci->disconnectReqAlarm);
        ci->disconnectReqAlarm = Alarm();
        delete ctx;
        QCC_DbgPrintf(("PacketEngine::HandleDisconnectRsp: Closing id=0x%x", ci->id));
        ci->state = ChannelInfo::CLOSED;
    }
    if (ci) {
        engine->ReleaseChannelInfo(*ci);
    }
}

void PacketEngine::RxPacketThread::HandleAck(Packet* controlPacket)
{
    QCC_DbgTrace(("PacketEngine::HandleAck(seqNum=0x%x, remRxDrain=0x%x, remRxAck=0x%x)", controlPacket->seqNum, letoh16(controlPacket->payload[2]), letoh16(controlPacket->payload[1])));
    //printf("tx(%d): ack s=0x%x, remRxDrain=0x%x, remRxAck=0x%x\n", (GetTimestamp() / 100) % 100000, controlPacket->seqNum, letoh16(controlPacket->payload[2]), letoh16(controlPacket->payload[1]));
    ChannelInfo* ci = engine->AcquireChannelInfo(controlPacket->chanId);
    if (ci) {
        ci->txLock.Lock();

        /* Validate that ack is in the window */
        uint16_t remoteRxAck = letoh32(controlPacket->payload[1]);
        uint16_t remoteRxDrain = letoh32(controlPacket->payload[2]);
        uint16_t delta = remoteRxAck - remoteRxDrain;
        uint16_t ackedPackets = 0;

        if (delta >= ci->windowSize) {
            delta += ci->windowSize;
        }
        if (IN_WINDOW(uint16_t, ci->remoteRxDrain, ci->windowSize - 1, controlPacket->seqNum) &&
            IN_WINDOW(uint16_t, ci->txDrain, numeric_limits<uint16_t>::max() >> 1, remoteRxAck) &&
            (delta < ci->windowSize)) {

            ci->remoteRxDrain = remoteRxDrain;

            /* Find and validate the packet that this ack refers to */
            Packet*& p = ci->txPackets[controlPacket->seqNum % ci->windowSize];
            if (p && (p->seqNum == controlPacket->seqNum)) {
                /*
                 * Adjust RTT .
                 * err = txRttMean - sample
                 * txRttMean = txRttMean + (err / 8)
                 * txRttMeanDev = txRttMeanDev + ((|err| - txRttMeanDev) / 4)
                 */
                if (p->sendAttempts == 1) {
                    uint64_t now = GetTimestamp64();
                    int32_t rtt = static_cast<int32_t>((now - p->sendTs + 1) << 10);
                    if (ci->txRttInit) {
                        int32_t err = (rtt - ci->txRttMean);
                        ci->txRttMean = ci->txRttMean + (err >> 3);
                        ci->txRttMeanVar = ci->txRttMeanVar + ((((err > 0) ? err : -err) - ci->txRttMeanVar) >> 2);
                    } else {
                        ci->txRttMean = rtt;
                        ci->txRttInit = true;
                    }
                }
                /* Remove packet from tx queue */
                //printf("tx(%d): clr0 s=0x%x, txD=0x%x, idx=0x%x\n", (GetTimestamp() / 100) % 100000, p->seqNum, ci->txDrain, controlPacket->seqNum % ci->windowSize);
                engine->pool.ReturnPacket(p);
                p = NULL;
                ackedPackets++;
            }
            /* Advance txDrain to remoteRxAck */
            AdvanceTxDrain(*ci, remoteRxAck, ackedPackets);

            /* Clear acked packets (set bits in mask) between remoteRxAck and txDrain */
            uint16_t ackIdx = controlPacket->seqNum % ci->windowSize;
            uint16_t drainIdx = ci->txDrain % ci->windowSize;
            while (ackIdx != drainIdx) {
                /* If bit is set in mask, then packet is acked and can be cleared */
                uint32_t m = letoh32(controlPacket->payload[3 + (drainIdx / 32)]);
                if (m & (0x01 << (drainIdx % 32))) {
                    if (ci->txPackets[drainIdx]) {
                        //printf("tx(%d): ack clr2 s=0x%x, txD=0x%x, idx=0x%x, txF=0x%x\n", (GetTimestamp() / 100) % 100000, ci->txPackets[drainIdx]->seqNum, ci->txDrain, drainIdx, ci->txFill);
                        engine->pool.ReturnPacket(ci->txPackets[drainIdx]);
                        ci->txPackets[drainIdx] = NULL;
                        ackedPackets++;
                    }
                }
                drainIdx = (drainIdx == (ci->windowSize - 1)) ? 0 : (drainIdx + 1);
            }

            /*
             * Check for fast retransmit by examining packets between remoteRxAck and current packet's seqNum.
             * Fast retransmit occurs if there is a hole in acked packets that is 3 or more back from the packet
             * seqNum which hasn't already been fast retransmitted.
             */
            uint32_t idx = controlPacket->seqNum % ci->windowSize;
            ackIdx = ((remoteRxAck == 0) ? (ci->windowSize - 1) : (remoteRxAck - 1)) % ci->windowSize;
            uint16_t ackCount = 0;
            while (idx != ackIdx) {
                uint32_t m = letoh32(controlPacket->payload[3 + (idx / 32)]);
                if (m & (0x01 << (idx % 32))) {
                    ++ackCount;
                } else if ((ackCount >= 3) && ci->txPackets[idx] && (ci->txPackets[idx]->sendAttempts > 0) && !ci->txPackets[idx]->fastRetransmit) {
                    ci->txPackets[idx]->fastRetransmit = true;
                    ci->txPackets[idx]->sendTs = 0;
                    //printf("tx(%d): fast retrans s=0x%x\n", (GetTimestamp() / 100) % 100000, ci->txPackets[idx]->seqNum);
                }
                idx = (idx == 0) ? (ci->windowSize - 1) : (idx - 1);
            }

            /* Receiving ack indicates no/reduced congestion. Increase window */
            while (ackedPackets && (ci->txCongestionWindow < ci->windowSize)) {
                if ((ci->txCongestionWindow < ci->txSlowStartThresh) || (ci->txConsecutiveAcks >= ci->txCongestionWindow)) {
                    ++ci->txCongestionWindow;
                    ci->txConsecutiveAcks = 0;
                    QCC_DbgPrintf(("Increasing congestion window of %s to %d", engine->ToString(ci->packetStream, ci->dest).c_str(), ci->txCongestionWindow));
                } else {
                    ci->txConsecutiveAcks++;
                }
                ackedPackets--;
            }
            engine->txPacketThread.Alert();
        } else {
            QCC_DbgPrintf(("Invalid ack window: seqNum=0x%x, drain=0x%x, ack=0x%x", controlPacket->seqNum, ci->remoteRxDrain, remoteRxAck));
        }
        ci->txLock.Unlock();
        engine->ReleaseChannelInfo(*ci);
    }
}

void PacketEngine::RxPacketThread::AdvanceTxDrain(ChannelInfo& ci, uint16_t newTxDrain, uint16_t& advCount)
{
    /* Advance txDrain to newTxDrain */
    bool txDrainMoved = newTxDrain != ci.txDrain;
    while (newTxDrain != ci.txDrain) {
        Packet*& tp = ci.txPackets[ci.txDrain % ci.windowSize];
        if (tp != NULL) {
            //printf("tx(%d): advtxdrain clr s=0x%x, txD=0x%x, idx=0x%x\n", (GetTimestamp() / 100) % 100000, tp->seqNum, ci.txDrain, ci.txDrain % ci.windowSize);
            engine->pool.ReturnPacket(tp);
            tp = NULL;
            advCount++;
        }
        ci.txDrain++;
    }
    if (txDrainMoved) {
        ci.sinkEvent.SetEvent();
    }
}

void PacketEngine::RxPacketThread::HandleXOn(Packet* controlPacket)
{
    uint16_t remRxAck = letoh32(controlPacket->payload[1]);
    uint16_t remRxDrain = letoh32(controlPacket->payload[2]);
    QCC_DbgTrace(("PacketEngine::HandleXOn(id=0x%x, remRxAck=0x%x, remRxDrain=0x%x, seqNum=0x%x)", controlPacket->chanId, remRxAck, remRxDrain, controlPacket->seqNum));
    //printf("tx(%d): handlexon remRxAck=0x%x remRxDrain=0x%x\n", (GetTimestamp() / 100) % 100000, remRxAck, remRxDrain);
    ChannelInfo* ci = engine->AcquireChannelInfo(controlPacket->chanId);
    if (ci) {
        ci->txLock.Lock();
        QCC_DbgTrace(("PacketEngine::HandleXOn(ci->xOffSeqNum=0x%x)", ci->xOffSeqNum));
        /* Advance the drain values only if the received Xon packet is in response to the
         * latest packet with Xoff. Otherwise just send an XonAck without advancing the
         * drains. We also need to account for back compatibility with previous versions
         * of PacketEngine. So we should still handle the case when controlPacket->seqNum==0
         * the same way as before */
        if ((controlPacket->seqNum == ci->xOffSeqNum) || (controlPacket->seqNum == 0)) {
            /* Update txDrain */
            uint16_t cnt = 0;
            if (IN_WINDOW(uint16_t, ci->txDrain, numeric_limits<uint16_t>::max() >> 1, remRxAck)) {
                AdvanceTxDrain(*ci, remRxAck, cnt);
            }

            /* Update remoteRxDrain */
            if (IN_WINDOW(uint16_t, ci->remoteRxDrain, ci->windowSize, remRxDrain)) {
                ci->remoteRxDrain = remRxDrain;
            }

            ci->txLock.Unlock();
            engine->txPacketThread.Alert();
        } else {
            ci->txLock.Unlock();
        }

        /* Send XON_ACK */
        uint32_t xOnAck[1];
        xOnAck[0] = htole32(PACKET_COMMAND_XON_ACK);
        QStatus status = engine->DeliverControlMsg(*ci, xOnAck, sizeof(xOnAck), controlPacket->seqNum);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to send XOnAck to %s", engine->ToString(ci->packetStream, ci->dest).c_str()));
        }

        engine->ReleaseChannelInfo(*ci);
    }
}

void PacketEngine::RxPacketThread::HandleXOnAck(Packet* controlPacket)
{
    QCC_DbgTrace(("PacketEngine::HandleXOnAck(id=0x%x)", controlPacket->chanId));
    ChannelInfo* ci = engine->AcquireChannelInfo(controlPacket->chanId);
    if (ci) {
        ci->rxLock.Lock();
        QCC_DbgTrace(("PacketEngine::HandleXOnAck(ci->rxFlowSeqNum=0x%x) (controlPacket->seqNum=0x%x)", ci->rxFlowSeqNum, controlPacket->seqNum));
        /* Remove the Xon alarm only if the received XonAck is in response to the latest
         * Xon packet for which the alarm was initialized. We also need to account for back
         * compatibility with previous versions of PacketEngine. So we should still handle
         * the case when controlPacket->seqNum==0 the same way as before */
        if ((ci->rxFlowSeqNum == controlPacket->seqNum) || (controlPacket->seqNum == 0)) {
            XOnAlarmContext* cctx = static_cast<XOnAlarmContext*>(ci->xOnAlarm->GetContext());
            if (cctx) {
                engine->timer.RemoveAlarm(ci->xOnAlarm);
                ci->xOnAlarm = Alarm();
                delete cctx;
            }
        }
        ci->rxLock.Unlock();
        engine->ReleaseChannelInfo(*ci);
    }
}

PacketEngine::TxPacketThread::TxPacketThread(const qcc::String& engineName) : Thread(engineName + "-tx"), engine(NULL)
{
}

qcc::ThreadReturn STDCALL PacketEngine::TxPacketThread::Run(void* arg)
{
    uint32_t waitMs = Event::WAIT_FOREVER;
    engine = reinterpret_cast<PacketEngine*>(arg);
    while (!IsStopping()) {
        QStatus status = ER_OK;
        if (waitMs > 0) {
            /* Wait for waitTs - now  milliseconds */
            Event evt(waitMs);
            //printf("tx(%d): wait(%d)\n", (GetTimestamp() / 100) % 100000, waitMs);
            status = Event::Wait(evt);
            //printf("tx(%d): wake %s\n", (GetTimestamp() / 100) % 100000, QCC_StatusText(status));
            if (status == ER_ALERTED_THREAD) {
                GetStopEvent().ResetEvent();
                status = ER_OK;
            }
        }
        waitMs = Event::WAIT_FOREVER;
        if (!IsStopping() && (status == ER_OK)) {
            /* Iterate over tx queue and send, resend or expire */
            ChannelInfo* ci = NULL;
            while ((ci = engine->AcquireNextChannelInfo(ci)) != NULL) {
                ci->txLock.Lock();
                /* Send all control messages */
                while (!ci->txControlQueue.empty()) {
                    Packet* p = ci->txControlQueue.front();
                    ci->txControlQueue.pop_front();
                    p->Marshal();
                    status = ci->packetStream.PushPacketBytes(p->buffer, p->payloadLen + Packet::payloadOffset, ci->dest);
                    /* Closedown if control message was a disconnectRsp */
                    if (letoh32(p->payload[0]) == PACKET_COMMAND_DISCONNECT_RSP) {
                        QCC_DbgPrintf(("PacketEngine::TxThread: Send DisconnectRsp. Closing id=0x%x", ci->id));
                        ci->state = ChannelInfo::CLOSED;
                        engine->pool.ReturnPacket(p);
                        break;
                    }
                    engine->pool.ReturnPacket(p);
                }
                /* Walk from [txDrain, min(txFill,congestion_window,remoteRxDrain+window)) and (re)send any user packets */
                if (ci && ci->state == ChannelInfo::OPEN) {
                    uint16_t nonExpiredPackets = 0;
                    uint16_t drain = ci->txDrain;
                    while ((drain != ci->txFill) && IN_WINDOW(uint16_t, ci->remoteRxDrain, ci->windowSize - 1, drain) && (nonExpiredPackets < ci->txCongestionWindow)) {
                        Packet*& p = ci->txPackets[drain % ci->windowSize];
                        if (p) {
                            uint64_t now = GetTimestamp64();
                            /*
                             * Send the packet if it:
                             *  a) hasn't expired
                             *  b) has already been sent at least once
                             *     or
                             *  c) packet has expired but is needed to trigger XOFF
                             */
                            uint16_t xOffSeqNum = ci->remoteRxDrain + ci->windowSize - 2;
                            if (((p->expireTs > now) || (p->sendAttempts >= 1) || (p->seqNum == xOffSeqNum) || (drain == (ci->txFill - 1))) && (p->sendAttempts <= MAX_PACKET_SEND_ATTEMPTS)) {
                                ++nonExpiredPackets;
                                uint32_t retryMs = engine->GetRetryMs(*ci, p->sendAttempts);
                                bool needMarshal = false;
                                if ((p->sendTs == 0) || ((now - p->sendTs) > retryMs)) {
                                    ++p->sendAttempts;
                                    /* Marshal if this is the first send attempt */
                                    if (p->sendAttempts == 1) {
                                        if (ci->txCongestionWindow > ci->txSlowStartThresh) {
                                            p->flags |= PACKET_FLAG_DELAY_ACK;
                                        }
                                        uint16_t gap = p->seqNum - ci->txLastMarshalSeqNum - 1;
                                        if (gap > (ci->windowSize - 2)) {
                                            gap = numeric_limits<uint16_t>::max();
                                        }
                                        p->gap = gap;
                                        p->Marshal();
                                        ci->txLastMarshalSeqNum = p->seqNum;
                                        needMarshal = true;
                                    }
                                    /* Indicate flow off if we have reached the receiver's drain limit */
                                    if ((p->seqNum == xOffSeqNum) && ((p->flags & PACKET_FLAG_FLOW_OFF) == 0)) {
                                        /* Record the sequence number of the latest packet which has the
                                         * Xoff bit set. This is required to appropriately process the
                                         * received Xon packets*/
                                        ci->xOffSeqNum = xOffSeqNum;
                                        p->flags |= PACKET_FLAG_FLOW_OFF;
                                        needMarshal = true;
                                    } else if ((p->seqNum != xOffSeqNum) && (p->flags & PACKET_FLAG_FLOW_OFF)) {
                                        p->flags &= ~PACKET_FLAG_FLOW_OFF;
                                        needMarshal = true;
                                    }
                                    if (needMarshal) {
                                        p->Marshal();
                                    }
                                    status = ci->packetStream.PushPacketBytes(p->buffer, p->payloadLen + Packet::payloadOffset, ci->dest);
                                    //printf("tx(%d): s=0x%x, len=%d, gap=%d, retry=%d txFill=0x%x, txDrain=0x%x, drain=0x%x, retryMs=%d, actMs=%d, xoff=%s\n", (GetTimestamp() / 100) % 100000, p->seqNum, (int) p->payloadLen, p->gap, p->sendAttempts, ci->txFill, ci->txDrain, drain, retryMs, (int) (now - p->sendTs), (p->flags & PACKET_FLAG_FLOW_OFF) ? "off" : "nc");
                                    QCC_DbgPrintf(("TxPacketThread sent seqNum=0x%x to %s (try=%d, gap=%d, drain=0x%x) %s", p->seqNum, engine->ToString(ci->packetStream, ci->dest).c_str(), p->sendAttempts, p->gap, drain, QCC_StatusText(status)));
                                    if (status == ER_OK) {
                                        /* Update sendTs and update (next) wait time */
                                        p->sendTs = GetTimestamp64();
                                        waitMs = ::min(waitMs, engine->GetRetryMs(*ci, p->sendAttempts));
                                    } else {
                                        /* Return packet and close this channel */
                                        QCC_LogError(status, ("TxPacketThread: PushPacketBytes(%s) failed. Closing channel", engine->ToString(ci->packetStream, ci->dest).c_str()));
                                        ci->state = ChannelInfo::CLOSED;
                                        status = ER_OK;
                                        break;
                                    }
                                    /* Adjust congestion window down (by factor of 2) if this was a retry */
                                    if ((p->sendAttempts > 1) && (ci->txCongestionWindow > 1)) {
                                        ci->txCongestionWindow = ci->txCongestionWindow >> 1;
                                        ci->txSlowStartThresh = ::max(ci->txCongestionWindow, (uint16_t)2);
                                        QCC_DbgPrintf(("Decreasing congestion window of %s to %d (ssThresh=%d)", engine->ToString(ci->packetStream, ci->dest).c_str(), ci->txCongestionWindow, ci->txSlowStartThresh));
                                    }
                                } else {
                                    /* Calcualte next retry time */
                                    waitMs = ::min(waitMs, retryMs);
                                }
                            } else {
                                /* packet has expired or retries are exhausted */
                                //printf("tx(%d): expire pkt s=0x%x (r=%d)\n", (GetTimestamp() / 100) % 100000, p->seqNum, p->sendAttempts);
                                QCC_DbgPrintf(("TxPacketThread: Expiring tx packet seqNum=0x%x to %s (sendAttempts=%d)", p->seqNum, engine->ToString(ci->packetStream, ci->dest).c_str(), p->sendAttempts));
                                engine->pool.ReturnPacket(p);
                                p = NULL;
                            }
                        }
                        ++drain;
                    }
                    //printf("tx(%d): while exited d=0x%x, tD=0x%x, tF=0x%x, rrD=0x%x, nep=%d, cw=%d\n", (GetTimestamp() / 100) % 100000, drain, ci->txDrain, ci->txFill, ci->remoteRxDrain, nonExpiredPackets, ci->txCongestionWindow);
                }
                ci->txLock.Unlock();
            }
        }
        if ((status != ER_OK) && (status != ER_STOPPING_THREAD)) {
            QCC_DbgPrintf(("TxPacketThread::Run() error (%s). Continuing...", QCC_StatusText(status)));
        }
    }
    return (qcc::ThreadReturn) 0;
}

PacketStream* PacketEngine::GetPacketStream(const PacketEngineStream& stream)
{
    PacketStream* ret = NULL;
    channelInfoLock.Lock();
    map<uint32_t, ChannelInfo*>::iterator it = channelInfos.begin();
    while (it != channelInfos.end()) {
        if (&(it->second->stream) == &stream) {
            ret = &(it->second->packetStream);
            break;
        }
        ++it;
    }
    channelInfoLock.Unlock();
    return ret;
}

}
