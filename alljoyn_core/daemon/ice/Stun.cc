/**
 * @file
 *
 * This file implements the STUN interface class
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


#include <assert.h>
#include <algorithm>
#include <map>
#include <qcc/Crypto.h>
#include <qcc/Socket.h>
#include "ScatterGatherList.h"
#include <Stun.h>
#include <StunAttribute.h>
#include <StunIOInterface.h>
#include <StunMessage.h>
#include <StunTransactionID.h>

using namespace qcc;

#define QCC_MODULE "STUN"

using namespace std;
using namespace qcc;

namespace ajn {

Stun::Stun(SocketFd sockfd,
           SocketType type,
           IPAddress& remoteAddr,
           uint16_t remotePort,
           bool autoFraming) :
    rxThread(NULL), turnPort(0),
    remoteAddr(remoteAddr), remotePort(remotePort), localPort(0),
    sockfd(sockfd), type(type),
    connected(true), opened(true), usingTurn(false),
    autoFraming(autoFraming),
    rxFrameRemain(0), txFrameRemain(0),
    rxLeftoverBuf(NULL), rxLeftoverPos(NULL), rxLeftoverLen(0),
    maxMTU(0),
    component(NULL),
    hmacKeyLen(0)
{
    QCC_DbgTrace(("Stun::Stun(%p)", this));
}

Stun::Stun(SocketType type,
           Component* component,
           STUNServerInfo stunInfo,
           const uint8_t* key,
           size_t keyLen,
           size_t mtu,
           bool autoFraming) :
    rxThread(NULL),
    turnAddr(), turnPort(), remotePort(0), localPort(0), sockfd(-1),
    type(type), connected(false), opened(false), usingTurn(false),
    autoFraming(autoFraming),
    rxFrameRemain(0), txFrameRemain(0),
    rxLeftoverBuf(NULL), rxLeftoverPos(NULL), rxLeftoverLen(0),
    maxMTU(mtu),
    component(component),
    STUNInfo(stunInfo),
    hmacKey(key),
    hmacKeyLen(keyLen)
{
    QCC_DbgTrace(("Stun::Stun(%p) maxMTU(%d)", this, maxMTU));

    if (stunInfo.relayInfoPresent) {
        turnAddr = stunInfo.relay.address;
        turnPort = stunInfo.relay.port;
    }
}

Stun::~Stun(void)
{
    QCC_DbgTrace(("Stun::~Stun(%p)", this));
    ReleaseFD(true);
}

QStatus Stun::OpenSocket(AddressFamily af)
{
    QStatus status = ER_STUN_SOCKET_OPEN;

    QCC_DbgTrace(("Stun::OpenSocket(af = %d)", af));

    if (!opened) {
        status = Socket(af, type, sockfd);
        if (status == ER_OK) {
            String threadname("RX Thread ");
            opened = true;

            threadname += U32ToString(sockfd);
            rxThread = new Thread(threadname, RxThread);
            rxThread->Start(this);
        }
    }
    return status;
}


QStatus Stun::Connect(const IPAddress& remoteAddr, uint16_t remotePort, bool relayData)
{
    QStatus status = ER_STUN_SOCKET_NOT_OPEN;

    QCC_DbgTrace(("Stun::Connect(remoteAddr = %s, remotePort = %u, relayData = %s)",
                  remoteAddr.ToString().c_str(), remotePort,
                  relayData ? "true" : "false"));

    if (opened) {
        this->remoteAddr = remoteAddr;
        this->remotePort = remotePort;

        if (QCC_SOCK_STREAM == type) {
            status = qcc::Connect(sockfd, remoteAddr, remotePort);
        } else {
            status = ER_OK;
        }

        if (status == ER_OK) {
            usingTurn = relayData;
        }
    }
    return status;
}


QStatus Stun::Bind(const IPAddress& localAddr, uint16_t localPort)
{
    QStatus status = ER_STUN_SOCKET_NOT_OPEN;

    QCC_DbgTrace(("Stun::Bind(localAddr = %s, localPort = %u)",
                  localAddr.ToString().c_str(), localPort));

    if (opened) {
        status = qcc::Bind(sockfd, localAddr, localPort);

        /* Ensure that the MTU is set appropriately */
        if (maxMTU == 0) {
            status = ER_FAIL;
            QCC_LogError(status, ("Stun::Bind(): maxMTU = 0"));
        }
    }
    return status;
}


QStatus Stun::Listen(int backlog)
{
    QStatus status = ER_STUN_SOCKET_NOT_OPEN;

    QCC_DbgTrace(("Stun::Listen(backlog = %d)", backlog));

    if (opened) {
        status = qcc::Listen(sockfd, backlog);
    }
    return status;
}


QStatus Stun::Accept(Stun** newStun)
{
    QStatus status = ER_STUN_SOCKET_NOT_OPEN;
    SocketFd newSockfd;
    IPAddress remoteAddr;
    uint16_t remotePort;

    QCC_DbgTrace(("Stun::Accept(**newStun)"));

    assert(newStun != NULL);

    if (opened) {
        status = qcc::Accept(sockfd, remoteAddr, remotePort, newSockfd);
        if (status == ER_OK) {
            *newStun = new Stun(newSockfd, type, remoteAddr, remotePort, autoFraming);
        }
    }
    return status;
}

void Stun::DisableStunProcessing()
{
    QCC_DbgTrace(("Stun::DisableStunProcessing"));

    if (stunMsgQueue.size() > 0) {
        QCC_DbgPrintf(("%u entries left in the STUN message queue.  [thread = %s]",
                       stunMsgQueue.size(),
                       rxThread ? rxThread->GetName() : "<unknown>"));
    }
    if (appQueue.size() > 0) {
        QCC_DbgPrintf(("%u entries left in the app data queue.  [thread = %s]",
                       appQueue.size(),
                       rxThread ? rxThread->GetName() : "<unknown>"));
    }

    if (rxThread) {
        QCC_DbgPrintf(("Stopping thread: %s", rxThread ? rxThread->GetName() : "<unknown>"));
        rxThread->Stop();
        rxThread->Join();
        delete rxThread;
        rxThread = NULL;
    }
}

QStatus Stun::Shutdown(void)
{
    QStatus status = ER_STUN_SOCKET_NOT_OPEN;

    QCC_DbgTrace(("Stun::Shutdown() [sockfd = %d]", sockfd));

    DisableStunProcessing();

    if (opened && (type == QCC_SOCK_STREAM)) {
        status = qcc::Shutdown(sockfd);
    }
    usingTurn = false;
    opened = false;
    connected = false;
    return status;
}

QStatus Stun::Close(void)
{
    QStatus status = ER_STUN_SOCKET_NOT_OPEN;

    QCC_DbgTrace(("Stun::Close*() [sockfd = %d]", sockfd));

    if (SOCKET_ERROR != sockfd) {
        qcc::Close(sockfd);
        sockfd = SOCKET_ERROR;
        status = ER_OK;
    }

    return status;
}


QStatus Stun::GetLocalAddress(IPAddress& addr, uint16_t& port)
{
    QStatus status = ER_STUN_SOCKET_NOT_OPEN;

    QCC_DbgTrace(("Stun::GetLocalAddress(addr = <>, port = <>)"));

    if (opened) {
        status = qcc::GetLocalAddress(sockfd, addr, port);
    }
    return status;
}


STUNServerInfo Stun::GetSTUNServerInfo(void)
{
    return STUNInfo;
}


QStatus Stun::SendStunMessage(const StunMessage& msg, IPAddress addr, uint16_t port, bool relayMsg)
{
    QCC_DbgTrace(("Stun::SendStunMessage(msg = %s, addr = %s, port = %u, relayMsg = %s) [sockfd = %d]",
                  msg.ToString().c_str(),
                  addr.ToString().c_str(),
                  port,
                  relayMsg ? "YES" : "NO",
                  sockfd));

    QStatus status;
    uint8_t* renderBuf;
    uint8_t* pos;
    size_t renderBufSize;
    ScatterGatherList msgSG;
    size_t sent;
    size_t expectedSent = 0;

    if (!opened) {
        return ER_STUN_SOCKET_NOT_OPEN;
    }

    if (msg.GetTypeClass() == STUN_MSG_REQUEST_CLASS) {
        StunMessage::const_iterator iter = msg.End();

        --iter;
        if ((*iter)->GetType() == STUN_ATTR_FINGERPRINT) {
            --iter;
        }
        if ((*iter)->GetType() == STUN_ATTR_MESSAGE_INTEGRITY) {
            StunTransactionID t;
            msg.GetTransactionID(t);
            StunMessage::keyInfo keydata;
            keydata.key = const_cast<uint8_t*>(msg.GetHMACKey());
            keydata.keyLen = msg.GetHMACKeyLength();
            expectedResponses[t] = keydata;

            QCC_DbgLocalData(expectedResponses[t].key, expectedResponses[t].keyLen);

#if !defined(NDEBUG)
            for (iter = msg.Begin();
                 ((*iter)->GetType() != STUN_ATTR_USERNAME) && (iter != msg.End());
                 ++iter) {
            }
            assert(iter != msg.End());
#endif
        }
    }


    renderBufSize = msg.RenderSize();

    renderBuf = new uint8_t[renderBufSize];
    pos = renderBuf;

    status = msg.RenderBinary(pos, renderBufSize, msgSG);
    if (status != ER_OK) {
        QCC_LogError(status, ("Rendering STUN message or TX"));
        goto exit;
    }

    QCC_DbgPrintf(("TX: Sending %u byte STUN message", msgSG.DataSize()));
#if !defined(NDEBUG)
    if (1) {
        ScatterGatherList::const_iterator iter;
        for (iter = msgSG.Begin(); iter != msgSG.End(); ++iter) {
            QCC_DbgLocalData(iter->buf, iter->len);
        }
    }
#endif

    assert(renderBufSize == 0);

    frameLock.Lock();
    if (type == QCC_SOCK_STREAM) {
        status = ER_NOT_IMPLEMENTED;
        QCC_LogError(status, ("Sending STUN message"));
        frameLock.Unlock();
        goto exit;
    } else {
        if (relayMsg) {
            // Relayed UDP messages must be wrapped in a STUN message.
            ScatterGatherList rMsgSG;
            StunMessage rMsg(STUN_MSG_INDICATION_CLASS, STUN_MSG_SEND_METHOD, hmacKey, hmacKeyLen);
            uint8_t* rBuf;
            uint8_t* rPos;
            size_t rBufSize;

            status = rMsg.AddAttribute(new StunAttributeUsername(STUNInfo.acct));
            if (status == ER_OK) {
                status = rMsg.AddAttribute(new StunAttributeXorPeerAddress(rMsg, addr, port));
            }
            if (status == ER_OK) {
                status = rMsg.AddAttribute(new StunAttributeAllocatedXorServerReflexiveAddress(rMsg, localSrflxCandidate.addr, localSrflxCandidate.port));
            }
            if (status == ER_OK) {
                status = rMsg.AddAttribute(new StunAttributeIceCheckFlag());
            }
            if (status == ER_OK) {
                status = rMsg.AddAttribute(new StunAttributeData(msgSG));
            }
            if (status == ER_OK) {
                status = rMsg.AddAttribute(new StunAttributeMessageIntegrity(rMsg));
            }
            if (status == ER_OK) {
                status = rMsg.AddAttribute(new StunAttributeFingerprint(rMsg));
            }
            if (status == ER_OK) {
                rBufSize = rMsg.RenderSize();
                rBuf = new uint8_t[rBufSize];
                rPos = rBuf;

                expectedSent = rMsg.Size();

                status = rMsg.RenderBinary(rPos, rBufSize, rMsgSG);
                if (status == ER_OK) {
                    status = SendToSG(sockfd, turnAddr, turnPort, rMsgSG, sent);
                }
                delete[] rBuf;
            }
        } else {
            expectedSent = msg.Size();

            status = SendToSG(sockfd, addr, port, msgSG, sent);
        }
    }
    frameLock.Unlock();

    if ((status == ER_OK) && (sent != expectedSent)) {
        status = ER_STUN_FAILED_TO_SEND_MSG;
        QCC_LogError(status, ("Sent %u does not match expected (%u)", sent, expectedSent));
    }

exit:
    delete[] renderBuf;
    return status;
}

void Stun::ReceiveTCP()
{
    // To be implemented...
}


void Stun::ReceiveUDP()
{
    QStatus status;
    StunBuffer sb(maxMTU);

    //FIXME: This should be a non-blocking recv
    status = RecvFrom(sockfd, sb.addr, sb.port, sb.buf, sb.len, sb.len);
    if (status == ER_OK) {
        bool isStunMsg = ((sb.len >= StunMessage::MIN_MSG_SIZE) &&
                          StunMessage::IsStunMessage(sb.buf, sb.len));

        if (isStunMsg) {
            const uint8_t* buf = sb.buf;
            size_t bufSize = sb.len;
            uint16_t rawMsgType = 0;

            StunIOInterface::ReadNetToHost(buf, bufSize, rawMsgType);

            // Reset buf and bufSize
            buf = sb.buf;
            bufSize = sb.len;

            if (StunMessage::ExtractMessageMethod(rawMsgType) == STUN_MSG_DATA_METHOD) {
                // parse message and extract DATA attribute contents.
                uint8_t* dummyHmac = new uint8_t[hmacKeyLen];
                StunMessage msg("", dummyHmac, hmacKeyLen);
                QStatus status;

                status = msg.Parse(buf, bufSize);
                if (status == ER_OK) {
                    StunMessage::const_iterator iter;

                    for (iter = msg.Begin(); iter != msg.End(); ++iter) {
                        if ((*iter)->GetType() == STUN_ATTR_DATA) {
                            StunAttributeData* data = reinterpret_cast<StunAttributeData*>(*iter);
                            ScatterGatherList::const_iterator sgiter = data->GetData().Begin();

                            /*
                             * Because the message was parsed, the data
                             * SG-List in the DATA attribute is guaranteed to
                             * be have only a single element.  Furthermore,
                             * that element will refer to a region of memory
                             * that is fully contained within the space
                             * allocated for the StunBuffer that was allocated
                             * above.  Therefore, we just point the sb.buf to
                             * the data region instead of performing a data
                             * copy that will involve overlapping memory
                             * regions.
                             */
                            sb.buf = reinterpret_cast<uint8_t*>(sgiter->buf);
                            sb.len = sgiter->len;

                            // Now that STUN wrapped relayed msg is extracted,
                            // need to determine if wrapped message is a STUN
                            // message for ICE or not.
                            isStunMsg = ((sb.len >= StunMessage::MIN_MSG_SIZE) &&
                                         StunMessage::IsStunMessage(sb.buf, sb.len));
                        }
                        if ((*iter)->GetType() == STUN_ATTR_XOR_PEER_ADDRESS) {
                            StunAttributeXorPeerAddress* sa = reinterpret_cast<StunAttributeXorPeerAddress*>(*iter);
                            sa->GetAddress(sb.addr, sb.port);
                        }
                    }
                    sb.relayed = true;
                }
                delete [] dummyHmac;
            }
        }

        if (isStunMsg) {
            size_t sz;
            QCC_DbgPrintf(("Got a STUN message via UDP"));
            stunMsgQueueLock.Lock();
            stunMsgQueue.push(sb);
            sz = stunMsgQueue.size();
            stunMsgQueueLock.Unlock();
            QCC_DbgPrintf(("STUN message queue size: %u", sz));
            stunMsgQueueModified.SetEvent();
        } else {
            size_t sz;
            QCC_DbgPrintf(("Got an app data packet via UDP"));
            appQueueLock.Lock();
            appQueue.push(sb);
            sz = appQueue.size();
            appQueueLock.Unlock();
            QCC_DbgPrintf(("App data queue size: %u", sz));
            appQueueModified.SetEvent();
        }
    }
}


ThreadReturn STDCALL Stun::RxThread(void*arg)
{
    Thread* selfThread = Thread::GetThread();

    QCC_DbgTrace(("Stun::RxThread(arg = <>)  [%s]", selfThread ? selfThread->GetName() : "<unknown>"));

    assert(selfThread != NULL);

    Stun* stun = reinterpret_cast<Stun*>(arg);
    Event sockWait(stun->sockfd, Event::IO_READ, false);
    Event& stopEvent = selfThread->GetStopEvent();
    vector<Event*> waitEvents, signaledEvents;
    QStatus status;

    waitEvents.push_back(&sockWait);
    waitEvents.push_back(&stopEvent);

    for (;;) {
        QCC_DbgPrintf(("Waiting for data on socket %d  [%s]",
                       stun->sockfd, selfThread ? selfThread->GetName() : "<unknown>"));
        status = Event::Wait(waitEvents, signaledEvents);
        if (status != ER_OK) {
            QCC_LogError(status, ("Waiting for data to arrive on the socket  [%s]", selfThread ? selfThread->GetName() : "<unknown>"));
            break;
        }

        if (find(signaledEvents.begin(), signaledEvents.end(), &stopEvent) != signaledEvents.end()) {
            QCC_DbgPrintf(("Stopping %s", selfThread ? selfThread->GetName() : "<unknown>"));
            break;
        }

        if (stun->type == QCC_SOCK_STREAM) {
            // TCP - need to wait for room in the app RX queue if full

            stun->appQueueLock.Lock();
            while (stun->appQueue.size() >= MAX_APP_RX_QUEUE) {
                QCC_DbgPrintf(("Waiting for app to read %d packets from queue...",
                               stun->appQueue.size() - MAX_APP_RX_QUEUE + 1));
                // TODO: Is there a problem with waiting on appQueueModified
                // from 2 different threads that also both set that event?
                // What are the chances for a deadlock?
                stun->appQueueModified.ResetEvent();
                stun->appQueueLock.Unlock();
                status = Event::Wait(stun->appQueueModified);
                if (status != ER_OK) {
                    QCC_LogError(status, ("Waiting for application to empty app RX Queue a bit"));
                }
                stun->appQueueLock.Lock();
            }
            stun->appQueueModified.ResetEvent();
            stun->appQueueLock.Unlock();

            stun->ReceiveTCP();

        } else {
            //UDP - need to discard oldest entry in app RX queue if full

            stun->appQueueLock.Lock();
            while (stun->appQueue.size() >= MAX_APP_RX_QUEUE) {
                QCC_DbgPrintf(("Need to discard %d packets from app rx queue...",
                               stun->appQueue.size() - MAX_APP_RX_QUEUE + 1));
                StunBuffer sb(stun->appQueue.front());
                delete[] sb.storage;
                stun->appQueue.pop();
            }
            //stun->appQueueModified.ResetEvent();
            stun->appQueueLock.Unlock();

            stun->ReceiveUDP();
        }
    }

    return 0;
}

QStatus Stun::RecvStunMessage(StunMessage& msg, IPAddress& addr, uint16_t& port, bool& relayed, uint32_t maxMs)
{
    Thread* selfThread = Thread::GetThread();

    QCC_DbgTrace(("Stun::RecvStunMessage(msg = <>, addr = <>, port = <>, maxMs = %u) [sockfd = %d, therad = %s]",
                  maxMs, sockfd, selfThread ? selfThread->GetName() : "<unknown>"));

    QStatus status = ER_OK;
    uint8_t* buf = NULL;
    const uint8_t* pos = NULL;
    size_t parseSize = -1;

    if (!opened) {
        status = ER_STUN_SOCKET_NOT_OPEN;
        QCC_LogError(status, ("Receiving STUN message"));
        goto exit;
    }

    if (type == QCC_SOCK_STREAM) {
        // TCP
        status = ER_NOT_IMPLEMENTED;
        QCC_LogError(status, ("Receiving STUN message"));
        goto exit;
    } else {
        // UDP
        vector<Event*> waitEvents, signaledEvents;
        uint8_t* storage = NULL;

        waitEvents.push_back(&stunMsgQueueModified);
        if (selfThread != NULL) {
            Event& stopEvent = selfThread->GetStopEvent();
            waitEvents.push_back(&stopEvent);
        }

        QCC_DbgPrintf(("Waiting up to %u ms for a STUN message...", maxMs));
        status = Event::Wait(waitEvents, signaledEvents, maxMs);
        if (status != ER_OK) {
            if (status != ER_TIMEOUT) {
                QCC_LogError(status, ("Waiting for a STUN message to arrive"));
            }
            goto exit;
        }

        stunMsgQueueLock.Lock();
        if (stunMsgQueue.empty()) {
            stunMsgQueueLock.Unlock();
            status = ER_STOPPING_THREAD;
            goto exit;
        }

        StunBuffer& sb(stunMsgQueue.front());
        storage = sb.storage;
        buf = sb.buf;
        parseSize = sb.len;
        addr = sb.addr;
        port = sb.port;
        relayed = sb.relayed;
        stunMsgQueue.pop();

        if (stunMsgQueue.size() == 0) {
            stunMsgQueueModified.ResetEvent();
        }
        stunMsgQueueLock.Unlock();

        QCC_DbgPrintf(("Popped off %u byte STUN message (addr = %p)",
                       parseSize, buf));

        pos = buf;
        buf = storage;
    }

    QCC_DbgPrintf(("RX: Received %u bytes", parseSize));
    QCC_DbgRemoteData(pos, parseSize);

    status = msg.Parse(pos, parseSize, expectedResponses);

#if !defined(NDEBUG)
    if (parseSize > 0) {
        QCC_DbgPrintf(("RX: Received %d extra bytes.", parseSize));
    }
#endif

exit:
    //if (rxLeftoverBuf != buf) {
    if (1) {
        delete[] buf;
    }

    return status;
}



QStatus Stun::AppSend(const void* buf, size_t len, size_t& sent)
{
    ScatterGatherList txSG;
    QCC_DbgTrace(("Stun::AppSend(*buf, len = %u, sent = <>)", len));

    assert(buf != NULL);
    txSG.AddBuffer(buf, len);
    txSG.SetDataSize(len);
    return AppSendSG(txSG, sent);
}


QStatus Stun::AppSendSG(const ScatterGatherList& sg, size_t& sent)
{
    QStatus status = ER_OK;
    ScatterGatherList msgSG;
    uint16_t frameLen;
    uint8_t frameLenBuf[sizeof(frameLen)];

    QCC_DbgTrace(("Stun::AppSendSG(sg[%u:%u/%u], sent = <>)", sg.Size(), sg.DataSize(), sg.MaxDataSize()));

    if (!opened) {
        status = ER_STUN_SOCKET_NOT_OPEN;
        goto exit;
    }

    if (type == QCC_SOCK_STREAM && !usingTurn && autoFraming) {
        // Frame length will be set later; we're just putting the pointer to
        // the frame length buffer first in the SG List here.
        msgSG.AddBuffer(frameLenBuf, FRAMING_SIZE);
        msgSG.IncDataSize(FRAMING_SIZE);
    }

    if (type != QCC_SOCK_STREAM && usingTurn) {
        // UDP transmissions via a TURN server need to be encapsulated in a
        // STUN message.
        StunMessage msg(STUN_MSG_INDICATION_CLASS, STUN_MSG_SEND_METHOD, NULL, 0);
        uint8_t* renderBuf, *pos;
        size_t renderSize;

        status = msg.AddAttribute(new StunAttributeXorPeerAddress(msg, remoteAddr, remotePort));
        if (status == ER_OK) {
            status = msg.AddAttribute(new StunAttributeAllocatedXorServerReflexiveAddress(msg, localSrflxCandidate.addr, localSrflxCandidate.port));
        }
        if (status == ER_OK) {
            status = msg.AddAttribute(new StunAttributeIceCheckFlag());
        }
        if (status == ER_OK) {
            status = msg.AddAttribute(new StunAttributeData(sg));
        }
        if (status == ER_OK) {
            status = msg.AddAttribute(new StunAttributeFingerprint(msg));
        }
        if (status == ER_OK) {
            renderSize = msg.RenderSize();
            renderBuf = new uint8_t[renderSize];

            pos = renderBuf;

            status = msg.RenderBinary(pos, renderSize, msgSG);

            if (status == ER_OK) {
                QCC_DbgPrintf(("TX: Sending %u octet app data in a %u octet STUN message .",
                               sg.DataSize(), msgSG.DataSize()));

                status = SendToSG(sockfd, turnAddr, turnPort, msgSG, sent);
                sent -= msg.Size() - sg.DataSize(); // Modify sent to correspond to app's data.
            }
            delete[] renderBuf;
        }
    } else {
        // Direct UDP transmissions and all TCP transmissions.

        msgSG.AddSG(sg);
        msgSG.SetDataSize(sg.DataSize());

        if (type == QCC_SOCK_STREAM) {
            if (!usingTurn) {
                ScatterGatherList frameSG(sg);

                QCC_DbgPrintf(("TX: Sending Direct TCP: txFrameRemain = %u", txFrameRemain));
                if (txFrameRemain == 0) {
                    frameLock.Lock();

                    while (frameSG.DataSize() > 0) {
                        if (autoFraming) {
                            // Set the frame length for TCP transmissions not relayed via a
                            // TURN server.
                            frameLen = sg.DataSize();
                            frameLenBuf[0] = static_cast<uint8_t>((frameLen >> 8) & 0xff);
                            frameLenBuf[1] = static_cast<uint8_t>(frameLen & 0xff);
                            QCC_DbgPrintf(("frameLen = %u", frameLen));
                        } else {
                            if (frameSG.DataSize() < FRAMING_SIZE) {
                                status = ER_STUN_FRAMING_ERROR;
                                QCC_LogError(status, ("Application framing mismatch"));
                                goto exit;
                            }
                            frameSG.CopyToBuffer(frameLenBuf, sizeof(frameLenBuf));
                            frameLen = ((static_cast<size_t>(frameLenBuf[0]) << 8) |
                                        (static_cast<size_t>(frameLenBuf[1])));
                            frameSG.TrimFromBegining(FRAMING_SIZE);
                            QCC_DbgPrintf(("frameLen = %u  (%02x%02x)", frameLen, frameLenBuf[0], frameLenBuf[1]));
                        }
                        frameSG.TrimFromBegining(frameLen);
                        txFrameRemain += frameLen + FRAMING_SIZE;
                        QCC_DbgPrintf(("TX: Sending Direct TCP: txFrameRemain = %u", txFrameRemain));
                    }
                } else {
                    assert(!autoFraming);
                    frameSG.TrimFromBegining(txFrameRemain);
                }

                sent = 0;

                QCC_DbgPrintf(("TX: Sending Direct TCP: txFrameRemain = %u", txFrameRemain));
                QCC_DbgPrintf(("TX: Sending %u Application octets.", msgSG.DataSize()));

                while (msgSG.DataSize() > 0) {
                    size_t segmentSent;
                    status = SendSG(sockfd, msgSG, segmentSent);
                    if (status != ER_OK) {
                        txFrameRemain = 0;
                        frameLock.Unlock();
                        goto exit;
                    }

                    msgSG.TrimFromBegining(segmentSent);
                    txFrameRemain -= segmentSent;
                    sent += segmentSent;
                    QCC_DbgPrintf(("TX: Sending Direct TCP: txFrameRemain = %u", txFrameRemain));
                }

                if (txFrameRemain == 0) {
                    frameLock.Unlock();
                }
                QCC_DbgPrintf(("TX: Sent Direct TCP: txFrameRemain = %u leftover", txFrameRemain));
            } else {
                QCC_DbgPrintf(("TX: Sending %u Application octets.", msgSG.DataSize()));
                status = SendSG(sockfd, msgSG, sent);
            }
        } else {
            // Direct UDP transmissions.
            QCC_DbgPrintf(("TX: Sending %u Application octets.", sg.DataSize()));
            status = SendToSG(sockfd, remoteAddr, remotePort, sg, sent);
        }
    }

    if (type == QCC_SOCK_STREAM && !usingTurn && autoFraming) {
        sent -= FRAMING_SIZE;
    }

exit:
    return status;
}


QStatus Stun::AppRecv(void* buf, size_t len, size_t& received)
{
    ScatterGatherList rxSG;
    assert(buf != NULL);

    rxSG.AddBuffer(buf, len);
    return AppRecvSG(rxSG, received);
}



static QStatus SkipRX(SocketFd sockfd, size_t len)
{
    QStatus status = ER_OK;
    uint8_t* skipBuf(new uint8_t[len]);
    size_t received;

    QCC_DbgTrace(("SkipRX(sockfd = %d, len = %u", sockfd, len));

    while (len > 0) {
        status = Recv(sockfd, skipBuf, len, received);
        if (status != ER_OK) {
            goto exit;
        }
        len -= received;
    }

exit:
    delete[] skipBuf;
    skipBuf = reinterpret_cast<uint8_t*>(0xdeadbeef);
    return status;
}


QStatus Stun::ProcessLeftoverSTUNFrame(void)
{
    QStatus status = ER_OK;

    QCC_DbgTrace(("Stun::ProcessLeftoverSTUNFrame()"));

    if (rxFrameRemain < rxLeftoverLen) {
        // Leftover buffer has a whole STUN message; skip over it.

        rxLeftoverPos += rxFrameRemain;
        rxLeftoverLen -= rxFrameRemain;
    } else {
        if (rxFrameRemain > rxLeftoverLen) {
            status = SkipRX(sockfd, (rxFrameRemain - rxLeftoverLen));
        }
        // TODO: If there is an error on receive, then we will need to resync.
        // This can only be done by carefully searching the received data for
        // a successfully parsable STUN message.  The other option would be
        // for the application to drop the connection and start from scratch.
        delete[] rxLeftoverBuf;
        rxLeftoverBuf = NULL;
    }
    rxFrameRemain = 0;

    QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));

    return status;
}



void Stun::ProcessLeftoverAppFrame(size_t& appBufFill, size_t appBufSpace,
                                   ScatterGatherList& fillSG, bool checkingFrame,
                                   uint8_t*& extraBuf)
{
    size_t copyLen = min(min(rxFrameRemain, rxLeftoverLen), fillSG.MaxDataSize());

    QCC_DbgTrace(("Stun::ProcessLeftoverAppFrame(appBufFill = %u  appBufSpace = %u, fillSG[%u:%u/%u], checkingFrame = %s  extraBuf = %p)",
                  appBufFill, appBufSpace,
                  fillSG.Size(), fillSG.DataSize(), fillSG.MaxDataSize(),
                  checkingFrame ? "true" : "false", extraBuf));


    QCC_DbgPrintf(("Copying %u (0x%04x) octets into fillSG[%u:%u/%u]  (rxFrameRemain = %u   rxLeftoverLen = %u)", copyLen, copyLen,
                   fillSG.Size(), fillSG.DataSize(), fillSG.MaxDataSize(),
                   rxFrameRemain, rxLeftoverLen));
    QCC_DbgRemoteData(rxLeftoverPos, rxLeftoverLen);
    fillSG.CopyFromBuffer(rxLeftoverPos, copyLen);
    rxLeftoverLen -= copyLen;
    rxLeftoverPos += copyLen;
    rxFrameRemain -= checkingFrame ? 0 : copyLen;
    QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));
    appBufFill += checkingFrame ? 0 : copyLen;
    QCC_DbgPrintf(("TEMP_DEBUG: appBufFill = %u (%04x)", appBufFill, appBufFill));


    fillSG.TrimFromBegining(copyLen);

    // At this point, at least one or more of these three cases are true:
    // - The leftover buffer has been depleted.
    // - The end of the frame has been reached.
    // - The App RX buffer(s) are filled.

    if (rxLeftoverLen == 0) {
        // The 'leftover' buffer is depleted so free it up.
        delete[] rxLeftoverBuf;
        rxLeftoverBuf = NULL;
    }

    if (!checkingFrame && (appBufFill >= appBufSpace)) {
        // The leftover data overflowed the App's buffer(s)
        // into the extra buffer.  Now we have to deal with
        // the aftermath of copying too much data.  This can
        // only happen if the App uses small buffers.

        size_t overflow = appBufFill - appBufSpace;

        appBufFill = appBufSpace;
        QCC_DbgPrintf(("TEMP_DEBUG: appBufFill = %u (%04x)", appBufFill, appBufFill));

        rxFrameRemain += overflow;
        QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));

        if (rxLeftoverBuf != NULL) {
            // The 'leftover' buffer was not depleted, so back
            // up the pointer to what is really left over.
            rxLeftoverPos -= overflow;
            rxLeftoverLen += overflow;
        } else if (overflow > 0) {
            rxLeftoverBuf = extraBuf;
            rxLeftoverPos = extraBuf;
            rxLeftoverLen = overflow;
            extraBuf = NULL;
        }
        // The 'leftover' buffer was depleted and we need more
        // data to know what to do with it.
    }
}

QStatus Stun::ProcessLeftoverRXFrameData(size_t& appBufFill,
                                         size_t appBufSpace,
                                         ScatterGatherList& fillSG,
                                         ScatterGatherList& checkSG,
                                         uint8_t*& extraBuf)
{
    QStatus status = ER_OK;

    QCC_DbgTrace(("Stun::ProcessLeftoverRXFrameData(appBufFill = %u  appBufSpace = %u, fillSG[%u:%u/%u], checkSG[%u:%u/%u]  extraBuf = %p)",
                  appBufFill, appBufSpace,
                  fillSG.Size(), fillSG.DataSize(), fillSG.MaxDataSize(),
                  checkSG.Size(), checkSG.DataSize(), checkSG.MaxDataSize(),
                  extraBuf));

    QCC_DbgPrintf(("RX: %u octets leftover starting at offset 0x%04x", rxLeftoverLen, ((rxLeftoverBuf != NULL) ? rxLeftoverPos - rxLeftoverBuf : 0)));

    while ((rxLeftoverBuf != NULL) && (appBufFill < appBufSpace)) {
        if (rxFrameRemain == 0) {
            // Beginning of a frame.

            if (rxLeftoverLen >= FRAMING_SIZE) {
                uint8_t* bufPos = rxLeftoverPos + FRAMING_SIZE;
                size_t bufLen = rxLeftoverLen - FRAMING_SIZE;
                size_t frameLen;

                frameLen = ((static_cast<uint16_t>(rxLeftoverPos[0]) << 8) |
                            static_cast<uint16_t>(rxLeftoverPos[1]));

                rxFrameRemain = frameLen;
                QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));

                if (autoFraming) {
                    rxLeftoverPos = bufPos;
                    rxLeftoverLen = bufLen;
                } else {
                    rxFrameRemain += FRAMING_SIZE;
                    QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));
                }

                if (StunMessage::IsStunMessage(bufPos, bufLen)) {
                    QCC_DbgPrintf(("RX: Leftover Frame is a STUN message."));
                    status = ProcessLeftoverSTUNFrame();
                    if (status != ER_OK) {
                        goto exit;
                    }
                } else {
                    // At this point, one of these cases is true:
                    // - The frame is too small for a minimal STUN message,
                    //   thus it must be for the application.
                    // - There was enough leftover data to confirm that the
                    //   frame is for the application.
                    // - There is not enough data to make a determination one
                    //   way or another, in which case we will assume that it
                    //   is for the application.


                    if ((frameLen >= StunMessage::MIN_MSG_SIZE) &&
                        (rxLeftoverLen < StunMessage::MIN_MSG_SIZE)) {
                        QCC_DbgPrintf(("RX: Not enough info to determine STUN or App."));
                        checkSG = fillSG;
                    } else {
                        QCC_DbgPrintf(("RX: Frame is App data."));
                        assert(checkSG.Size() == 0);
                    }

                    ProcessLeftoverAppFrame(appBufFill, appBufSpace,
                                            fillSG, (checkSG.Size() > 0), extraBuf);
                    if (checkSG.Size() > 0) {
                        checkSG.SetDataSize(checkSG.MaxDataSize() - fillSG.MaxDataSize());

                        if (1) {
                            size_t copyLen = min(rxFrameRemain, checkSG.DataSize());
                            uint8_t* checkSGBuf = new uint8_t[copyLen];
                            checkSG.CopyToBuffer(checkSGBuf, copyLen);
                            QCC_DbgRemoteData(checkSGBuf, copyLen);
                            delete[] checkSGBuf;
                            checkSGBuf = reinterpret_cast<uint8_t*>(0xdeadbeef);
                        }

                    }
                }
            } else {
                // We can't tell if the next frame is for STUN or the App.  We
                // don't even know how large the frame is.  We'll just stuff
                // it in the App's RX buffer(s) and figure it out later.
                QCC_DbgPrintf(("RX: Not enough info to determine frame size."));

                checkSG = fillSG;
                ProcessLeftoverAppFrame(appBufFill, appBufSpace, fillSG, true, extraBuf);

                // At this point, rxFrameRemain is 0, appBufFill is
                // incremented by 1, rxLeftoverBuf is NULL, fillSG may be
                // empty.
            }
        } else {
            // Middle of a frame.  This had better be the middle of an App frame.
            QCC_DbgPrintf(("RX: Middle of an App frame."));
            ProcessLeftoverAppFrame(appBufFill, appBufSpace, fillSG, false, extraBuf);
        }
    }

exit:
    return status;
}


QStatus Stun::ProcessUncheckedRXFrameData(size_t& appBufFill, size_t appBufSpace,
                                          ScatterGatherList& checkSG, uint8_t*& extraBuf)
{
    QStatus status = ER_OK;
    ScatterGatherList destSG;
    const size_t checkSize = StunMessage::MIN_MSG_SIZE + (autoFraming ? 0 : FRAMING_SIZE);

    QCC_DbgTrace(("Stun::ProcessUncheckedRXFrameData(appBufFill = %u  appBufSpace = %u, checkSG[%u:%u/%u]  extraBuf = %p)",
                  appBufFill, appBufSpace,
                  checkSG.Size(), checkSG.DataSize(), checkSG.MaxDataSize(),
                  extraBuf));

    assert(checkSG.DataSize() > 0);

    destSG = checkSG;

    while ((checkSG.DataSize() >= checkSize) ||
           ((checkSG.DataSize() > FRAMING_SIZE) && (rxFrameRemain == 0)) ||
           ((rxFrameRemain > 0) && (checkSG.DataSize() >= rxFrameRemain))) {
        bool isStunMessage;

        if (rxFrameRemain == 0) {
            if (checkSG.DataSize() >= FRAMING_SIZE) {
                // There's now enough information for getting the frame size.
                uint8_t fs[FRAMING_SIZE];
                checkSG.CopyToBuffer(fs, FRAMING_SIZE);

                rxFrameRemain = (static_cast<size_t>(fs[0]) << 8) | static_cast<size_t>(fs[1]);
                QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));

                if (autoFraming) {
                    checkSG.TrimFromBegining(FRAMING_SIZE);
                } else {
                    rxFrameRemain += FRAMING_SIZE;
                    QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));
                }

                if (rxFrameRemain < checkSize) {
                    // This frame is for the App.
                    size_t fill = min(rxFrameRemain, checkSG.DataSize());
                    QCC_DbgPrintf(("RX: Frame is for the Application (too small for STUN)."));

                    if (autoFraming) {
                        destSG.CopyDataFrom(checkSG, fill);
                    }

                    appBufFill += fill;
                    QCC_DbgPrintf(("TEMP_DEBUG: appBufFill = %u (%04x)", appBufFill, appBufFill));
                    rxFrameRemain -= fill;
                    QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));
                    checkSG.TrimFromBegining(fill);
                    destSG.TrimFromBegining(fill);

                    continue;
                }
            }
        }

        if (1) {
            size_t copyLen = min(rxFrameRemain, checkSG.DataSize());
            uint8_t* checkBuf = new uint8_t[copyLen];
            checkSG.CopyToBuffer(checkBuf, copyLen);
            QCC_DbgRemoteData(checkBuf, copyLen);
            delete[] checkBuf;
            checkBuf = reinterpret_cast<uint8_t*>(0xdeadbeef);
        }

        if (checkSG.DataSize() >= checkSize) {
            if (checkSG.Begin()->len < checkSize) {
                uint8_t* tmpBuf = new uint8_t[checkSize];
                checkSG.CopyToBuffer(tmpBuf, checkSize);
                isStunMessage = StunMessage::IsStunMessage(tmpBuf + (autoFraming ? 0 : FRAMING_SIZE),
                                                           StunMessage::MIN_MSG_SIZE);
                delete[] tmpBuf;
            } else {
                uint8_t* tmpBuf = reinterpret_cast<uint8_t*>(checkSG.Begin()->buf);
                isStunMessage = StunMessage::IsStunMessage(tmpBuf + (autoFraming ? 0 : FRAMING_SIZE),
                                                           StunMessage::MIN_MSG_SIZE);
            }

            if (isStunMessage) {
                QCC_DbgPrintf(("RX: Frame is a STUN message; chuck it..."));
                if (rxFrameRemain > checkSG.DataSize()) {
                    // Need to read the rest of the STUN message.
                    status = SkipRX(sockfd, rxFrameRemain - checkSG.DataSize());
                    rxFrameRemain = 0;
                    QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));
                    if (status != ER_OK) {
                        goto exit;
                    }
                    checkSG.Clear();
                } else {
                    // The App's rx buffer(s) have a full STUN message that
                    // must be removed.  The data after the STUN message may
                    // be app data or another STUN message.
                    rxFrameRemain -= checkSG.TrimFromBegining(rxFrameRemain);
                    QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));
                }
                assert(rxFrameRemain == 0);
            } else {
                // This frame is for the App.
                size_t checkedLen = min(min(rxFrameRemain, checkSG.DataSize()),
                                        appBufSpace - appBufFill);

                QCC_DbgPrintf(("RX: Frame is for the App (%u octets  range = 0x%04x - 0x%04x)",
                               checkedLen, appBufFill, appBufFill + checkedLen));

                if (destSG.DataSize() - checkSG.DataSize() > 0) {
                    // Need to copy (bleh).
                    destSG.CopyDataFrom(checkSG, checkedLen);
                }

                rxFrameRemain -= checkedLen;
                QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));
                checkSG.TrimFromBegining(checkedLen);
                destSG.TrimFromBegining(checkedLen);
                appBufFill += checkedLen;
                QCC_DbgPrintf(("TEMP_DEBUG: appBufFill = %u (%04x)", appBufFill, appBufFill));
            }
        } else {
            QCC_DbgPrintf(("RX: Not enough information to determine if frame is for STUN or App."));
        }
    }

    if (checkSG.DataSize() > 0) {
        uint8_t* checkBuf = reinterpret_cast<uint8_t*>(checkSG.Begin()->buf);
        rxLeftoverLen = checkSG.DataSize() + (((rxFrameRemain > 0) && autoFraming) ? FRAMING_SIZE : 0);
        assert(rxLeftoverBuf == NULL);

        QCC_DbgPrintf(("RX: Need to save %u leftover bytes.", rxLeftoverLen));

        // Check if 'checkSG' points to somewhere inside 'extraBuf' (iff
        // 'extraBuf' is allocated).  If 'checkSG' is pointing to memory
        // inside extraBuf, then the end of the first (and only) buffer in
        // 'checkSG' will have the same address as the end of 'extraBuf'.
        if ((extraBuf != NULL) &&
            ((checkBuf + checkSG.Begin()->len) == (extraBuf + StunMessage::MIN_MSG_SIZE + FRAMING_SIZE))) {
            // The 'extraBuf' has all the unchecked data, so make it the
            // leftover buffer.

            size_t bufDiff = (reinterpret_cast<uint8_t*>(checkSG.Begin()->buf) - extraBuf);

            assert(checkBuf < (reinterpret_cast<uint8_t*>(destSG.End()->buf) +
                               StunMessage::MIN_MSG_SIZE + FRAMING_SIZE));

            rxLeftoverBuf = extraBuf;
            rxLeftoverPos = extraBuf;
            assert(rxLeftoverLen == checkSG.DataSize() + bufDiff);

            rxFrameRemain = bufDiff - (autoFraming ? FRAMING_SIZE : 0);
            QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));
            extraBuf = NULL;
            checkSG.SetDataSize(0);

        } else if (checkSG.DataSize() < FRAMING_SIZE) {
            // There is only 1 byte which is not enough for even the frame
            // length.  Save it in a small leftover buffer.

            rxLeftoverBuf = new uint8_t[1];
            rxLeftoverPos = rxLeftoverBuf;
            assert(rxLeftoverLen == 1);
            *rxLeftoverBuf = *checkBuf;
            checkSG.SetDataSize(0);

        } else {
            uint8_t* pos;
            pos = new uint8_t[rxLeftoverLen];

            rxLeftoverBuf = pos;
            rxLeftoverPos = pos;

            if (rxFrameRemain > 0) {
                if (autoFraming) {
                    *pos = static_cast<uint8_t>(rxFrameRemain >> 8);
                    ++pos;
                    *pos = static_cast<uint8_t>(rxFrameRemain & 0xff);
                    ++pos;
                }
                rxFrameRemain = 0;
                QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));
            }

            checkSG.CopyToBuffer(pos, checkSG.DataSize());
        }

        QCC_DbgLocalData(rxLeftoverPos, rxLeftoverLen);
    }

exit:
    return status;
}


QStatus Stun::ReceiveAppFramedSG(ScatterGatherList& appSG, size_t& received)
{
    QStatus status = ER_OK;
    size_t appBufFill = 0;
    ScatterGatherList fillSG(appSG);
    ScatterGatherList rxSG;
    ScatterGatherList checkSG;
    size_t rxCnt = 0;
    uint8_t fsBuf[FRAMING_SIZE];

    uint8_t* extraBuf = NULL;
    size_t minFillBufSize = max(StunMessage::MIN_MSG_SIZE + FRAMING_SIZE, rxLeftoverLen);

    QCC_DbgTrace(("Stun::ReceiveAppFramedSG(appSG[%u:%u/%u], received = <>)",
                  appSG.Size(), appSG.DataSize(), appSG.MaxDataSize()));

    if (appSG.MaxDataSize() < minFillBufSize) {
        // The App's buffer(s) are too small for our use so add on an extra
        // buffer.  This will mean more copying, but if the App insists on
        // using small buffers, then it doesn't care about performance anyway.
        extraBuf = new uint8_t[minFillBufSize];
        fillSG.AddBuffer(extraBuf, minFillBufSize);
    }

    status = ProcessLeftoverRXFrameData(appBufFill, appSG.MaxDataSize(), fillSG, checkSG, extraBuf);
    if (status != ER_OK) {
        goto exit;
    }

    // Whew.  Done with processing the leftovers.  Now to receive some more
    // data and figure out if the data so far is for the App or if it is a
    // STUN message.


    if ((checkSG.Size() > 0) || (fillSG.Size() > ((extraBuf != NULL) ? 1 : 0))) {
        rxSG = fillSG;

        if ((rxFrameRemain == 0) && (checkSG.DataSize() < FRAMING_SIZE)) {
            // We're at the beginning of a frame and about to receive the frame size.
            if (autoFraming) {
                checkSG.CopyToBuffer(fsBuf, FRAMING_SIZE);
                rxSG.TrimFromBegining(checkSG.DataSize());
                checkSG = rxSG;  // rxSG.Begin()->buf points to inside fsBuf[].
            }
        }

        if ((rxFrameRemain == 0) && (checkSG.Size() == 0)) {
            // Everything in the App's buffer(s) at this point is OK, but we are
            // about to receive a new frame that is unknown so it must be checked.
            checkSG = rxSG;
        }

        status = RecvSG(sockfd, rxSG, rxCnt);
        if (status != ER_OK) {
            goto exit;
        }

        QCC_DbgPrintf(("RX: Received %u bytes  (rxSG[%u:%u/%u])", rxCnt,
                       rxSG.Size(), rxSG.DataSize(), rxSG.MaxDataSize()));

#if !defined(NDEBUG)
        if (1) {
            size_t dbgRxCnt = rxCnt;
            ScatterGatherList::const_iterator rxIter;
            for (rxIter = rxSG.Begin(); dbgRxCnt > 0 && rxIter != rxSG.End(); ++rxIter) {
                QCC_DbgRemoteData(rxIter->buf, min(dbgRxCnt, static_cast<size_t>(rxIter->len)));
                dbgRxCnt -= min(dbgRxCnt, static_cast<size_t>(rxIter->len));
            }
        }
#endif

        if (checkSG.Size() > 0) {
            checkSG.IncDataSize(rxCnt);
        }
    }


    if ((checkSG.Size() == 0) && (rxFrameRemain > 0)) {
        if (rxCnt > rxFrameRemain) {
            // Received the rest of a known App frame.  Set the checkSG for
            // checking the frame after the App frame.
            checkSG = rxSG;
            appBufFill += rxFrameRemain;
            QCC_DbgPrintf(("TEMP_DEBUG: appBufFill = %u (%04x)", appBufFill, appBufFill));
            rxFrameRemain -= checkSG.TrimFromBegining(rxFrameRemain);
            QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));
        } else {
            rxFrameRemain -= rxCnt;
            QCC_DbgPrintf(("TEMP_DEBUG: rxFrameRemain = %u (%04x)", rxFrameRemain, rxFrameRemain));
            appBufFill += rxCnt;
            QCC_DbgPrintf(("TEMP_DEBUG: appBufFill = %u (%04x)", appBufFill, appBufFill));
        }
    }

    QCC_DbgPrintf(("RX: About to figure out what was received.  checkSG[%u:%u/%u]   rxFrameRemain = %u   appBufFill = %u",
                   checkSG.Size(), checkSG.DataSize(), checkSG.MaxDataSize(),
                   rxFrameRemain, appBufFill));

    // OK.  Now figure out what was received.
    if (checkSG.Size() > 0) {
        status = ProcessUncheckedRXFrameData(appBufFill, appSG.MaxDataSize(), checkSG, extraBuf);
    }

exit:
    if (extraBuf != NULL) {
        delete[] extraBuf;
        extraBuf = reinterpret_cast<uint8_t*>(0xdeadbeef);
    }
    appSG.SetDataSize(appBufFill);

    received = appBufFill;

    return status;
}

QStatus Stun::AppRecvSG(ScatterGatherList& sg, size_t& received)
{
    QStatus status = ER_OK;

    QCC_DbgTrace(("Stun::AppRecvSG(sg[%u:%u/%u], received = <>)",
                  sg.Size(), sg.DataSize(), sg.MaxDataSize()));

    if (opened) {
        if (type == QCC_SOCK_STREAM) {
            QCC_DbgPrintf(("RX: Receiving TCP App data."));
            if (usingTurn) {
                QCC_DbgPrintf(("RX: Doing a straight receive"));
                status = RecvSG(sockfd, sg, received);
            } else {
                QCC_DbgPrintf(("RX: Doing a framed receive"));
                do {
                    status = ReceiveAppFramedSG(sg, received);
                } while ((status == ER_OK) && (received == 0));
            }
        } else {
            QCC_DbgPrintf(("RX: Receiving UDP App data."));
            uint8_t* storage;
            uint8_t* buf;
            Thread* selfThread = Thread::GetThread();
            vector<Event*> waitEvents, signaledEvents;

            waitEvents.push_back(&appQueueModified);
            if (selfThread != NULL) {
                Event& stopEvent = selfThread->GetStopEvent();
                waitEvents.push_back(&stopEvent);
            }

            QCC_DbgPrintf(("Waiting app data..."));
            status = Event::Wait(waitEvents, signaledEvents);
            if (status != ER_OK) {
                QCC_LogError(status, ("Waiting for app data to arrive"));
                goto exit;
            }

            if (find(signaledEvents.begin(), signaledEvents.end(), &appQueueModified) == signaledEvents.end()) {
                QCC_DbgPrintf(("Aborting read on thread %s due to stop signal",
                               selfThread ? selfThread->GetName() : "<unknown>"));
                status = ER_STOPPING_THREAD;
                goto exit;
            }

            appQueueLock.Lock();
            assert(!appQueue.empty());
            StunBuffer& sb(appQueue.front());
            storage = sb.storage;
            buf = sb.buf;
            received = sb.len;
            appQueue.pop();

            if (appQueue.size() == 0) {
                appQueueModified.ResetEvent();
            }
            appQueueLock.Unlock();

            QCC_DbgPrintf(("Popped off %u byte app data (addr = %p)",
                           received, buf));

            sg.CopyFromBuffer(buf, received);

            delete[] storage;
        }
    } else {
        status = ER_STUN_SOCKET_NOT_OPEN;
    }

    QCC_DbgPrintf(("RX: Returning %u (0x%04x) octets to app.", received, received));

exit:
    return status;
}

void Stun::ReleaseFD(bool close)
{
    QCC_DbgTrace(("Stun::ReleaseFD(close = %d)", close));
    if (opened) {
        Shutdown();
    }

    if (close && (sockfd != SOCKET_ERROR)) {
        Close();
    }

    sockfd = SOCKET_ERROR;

    while (stunMsgQueue.size() > 0) {
        StunBuffer sb(stunMsgQueue.front());
        delete[] sb.storage;
        stunMsgQueue.pop();
    }

    while (appQueue.size() > 0) {
        StunBuffer sb(appQueue.front());
        delete[] sb.storage;
        appQueue.pop();
    }
}

} //namespace ajn
