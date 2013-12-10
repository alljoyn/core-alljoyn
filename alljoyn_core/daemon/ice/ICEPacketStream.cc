/**
 * @file
 * ICEPacketStream is a UDP based implementation of the PacketStream interface.
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#include <errno.h>
#include <assert.h>

#include <qcc/Event.h>
#include <qcc/Debug.h>
#include <alljoyn/version.h>
#include "ScatterGatherList.h"
#include "ICECandidatePair.h"
#include "Stun.h"
#include "ICEPacketStream.h"

#define QCC_MODULE "PACKET"

using namespace std;
using namespace qcc;

namespace ajn {

ICEPacketStream::ICEPacketStream(ICESession& iceSession, Stun& stun, const ICECandidatePair& selectedPair) :
    ipAddress(stun.GetLocalAddr()),
    port(stun.GetLocalPort()),
    remoteAddress(selectedPair.remote->GetEndpoint().addr),
    remotePort(selectedPair.remote->GetEndpoint().port),
    remoteMappedAddress(),
    remoteMappedPort(0),
    turnAddress(stun.GetTurnAddr()),
    turnPort(stun.GetTurnPort()),
    relayServerAddress(iceSession.GetRelayServerAddr()),
    relayServerPort(iceSession.GetRelayServerPort()),
    sock(stun.GetSocketFD()),
    sourceEvent(new Event(sock, Event::IO_READ, false)),
    sinkEvent(new Event(sock, Event::IO_WRITE, false)),
    interfaceMtu(stun.GetMtu()),
    maxPacketStreamMtu((interfaceMtu <= ajn::MAX_ICE_INTERFACE_MTU) ? interfaceMtu : ajn::MAX_ICE_INTERFACE_MTU),
    mtuWithStunOverhead(maxPacketStreamMtu - ajn::STUN_OVERHEAD_SIZE),
    usingTurn((selectedPair.local->GetType() == _ICECandidate::Relayed_Candidate) || (selectedPair.remote->GetType() == _ICECandidate::Relayed_Candidate)),
    localTurn((selectedPair.local->GetType() == _ICECandidate::Relayed_Candidate)),
    localHost((selectedPair.local->GetType() == _ICECandidate::Host_Candidate)),
    remoteHost((selectedPair.remote->GetType() == _ICECandidate::Host_Candidate)),
    hmacKey(reinterpret_cast<const char*>(stun.GetHMACKey()), stun.GetHMACKeyLength(), stun.GetHMACKeyLength()),
    turnUsername(iceSession.GetusernameForShortTermCredential()),
    turnRefreshPeriod((selectedPair.local->GetAllocationLifetimeSeconds() - ajn::TURN_REFRESH_WARNING_PERIOD_SECS) * 1000),
    turnRefreshTimestamp(0),
    stunKeepAlivePeriod(iceSession.GetSTUNKeepAlivePeriod()),
    rxRenderBuf(new uint8_t[maxPacketStreamMtu]),
    txRenderBuf(new uint8_t[maxPacketStreamMtu])
{
    QCC_DbgTrace(("ICEPacketStream::ICEPacketStream(sock=%d)", sock));

    /* Retrieve the local server reflexive candidate */
    stun.GetLocalSrflxCandidate(localSrflxAddress, localSrflxPort);

    /* Set remoteMappedAddress to the remote's most external address (regardless of candidate type) */
    switch (selectedPair.remote->GetType()) {
    case _ICECandidate::Relayed_Candidate :
    case _ICECandidate::ServerReflexive_Candidate:
    case _ICECandidate::PeerReflexive_Candidate:
        remoteMappedAddress = selectedPair.remote->GetMappedAddress().addr;
        remoteMappedPort = selectedPair.remote->GetMappedAddress().port;
        break;

    default:
        remoteMappedAddress = selectedPair.remote->GetEndpoint().addr;
        remoteMappedPort = selectedPair.remote->GetEndpoint().port;
        break;
    }
}

ICEPacketStream::ICEPacketStream() :
    ipAddress(),
    port(0),
    remoteAddress(),
    remotePort(0),
    remoteMappedAddress(),
    remoteMappedPort(0),
    turnAddress(),
    turnPort(0),
    relayServerAddress(),
    relayServerPort(0),
    localSrflxAddress(),
    localSrflxPort(0),
    sock(SOCKET_ERROR),
    sourceEvent(&Event::neverSet),
    sinkEvent(&Event::alwaysSet),
    interfaceMtu(0),
    maxPacketStreamMtu(0),
    mtuWithStunOverhead(0),
    usingTurn(false),
    localTurn(false),
    localHost(false),
    remoteHost(false),
    hmacKey(),
    turnUsername(),
    turnRefreshPeriod(0),
    turnRefreshTimestamp(0),
    stunKeepAlivePeriod(0),
    rxRenderBuf(NULL),
    txRenderBuf(NULL)
{
}

ICEPacketStream::ICEPacketStream(const ICEPacketStream& other) :
    ipAddress(other.ipAddress),
    port(other.port),
    remoteAddress(other.remoteAddress),
    remotePort(other.remotePort),
    remoteMappedAddress(other.remoteMappedAddress),
    remoteMappedPort(other.remoteMappedPort),
    turnAddress(other.turnAddress),
    turnPort(other.turnPort),
    relayServerAddress(other.relayServerAddress),
    relayServerPort(other.relayServerPort),
    localSrflxAddress(other.localSrflxAddress),
    localSrflxPort(other.localSrflxPort),
    interfaceMtu(other.interfaceMtu),
    maxPacketStreamMtu(other.maxPacketStreamMtu),
    mtuWithStunOverhead(other.mtuWithStunOverhead),
    usingTurn(other.usingTurn),
    localTurn(other.localTurn),
    localHost(other.localHost),
    remoteHost(other.remoteHost),
    hmacKey(other.hmacKey),
    turnUsername(other.turnUsername),
    turnRefreshPeriod(other.turnRefreshPeriod),
    turnRefreshTimestamp(other.turnRefreshTimestamp),
    stunKeepAlivePeriod(other.stunKeepAlivePeriod)
{
    if (other.sock == SOCKET_ERROR) {
        sock = SOCKET_ERROR;
        sourceEvent = &Event::neverSet;
        sinkEvent = &Event::alwaysSet;
        rxRenderBuf = NULL;
        txRenderBuf = NULL;
    } else {
        QStatus status = SocketDup(other.sock, sock);
        if (status == ER_OK) {
            sourceEvent = new Event(sock, Event::IO_READ, false);
            sinkEvent = new Event(sock, Event::IO_WRITE, false);
            rxRenderBuf = new uint8_t[maxPacketStreamMtu];
            txRenderBuf = new uint8_t[maxPacketStreamMtu];
        } else {
            QCC_LogError(status, ("SocketDup failed"));
            sock = SOCKET_ERROR;
            sourceEvent = &Event::neverSet;
            sinkEvent = &Event::alwaysSet;
            rxRenderBuf = txRenderBuf = NULL;
        }
    }
}

ICEPacketStream& ICEPacketStream::operator=(const ICEPacketStream& other)
{
    if (this != &other) {
        ipAddress = other.ipAddress;
        port = other.port;
        remoteAddress = other.remoteAddress;
        remotePort = other.remotePort;
        remoteMappedAddress = other.remoteMappedAddress;
        remoteMappedPort = other.remoteMappedPort;
        turnAddress = other.turnAddress;
        turnPort = other.turnPort;
        relayServerAddress = other.relayServerAddress;
        relayServerPort = other.relayServerPort;
        localSrflxAddress = other.localSrflxAddress;
        localSrflxPort = other.localSrflxPort;
        interfaceMtu = other.interfaceMtu;
        maxPacketStreamMtu = other.maxPacketStreamMtu;
        mtuWithStunOverhead = other.mtuWithStunOverhead;
        usingTurn = other.usingTurn;
        localTurn = other.localTurn;
        localHost = other.localHost;
        remoteHost = other.remoteHost;
        hmacKey = other.hmacKey;
        turnUsername = other.turnUsername;
        turnRefreshPeriod = other.turnRefreshPeriod;
        turnRefreshTimestamp = other.turnRefreshTimestamp;
        stunKeepAlivePeriod = other.stunKeepAlivePeriod;

        if (sock != SOCKET_ERROR) {
            Close(sock);
            delete sourceEvent;
            delete sinkEvent;
            delete [] rxRenderBuf;
            delete [] txRenderBuf;
        }

        if (other.sock == SOCKET_ERROR) {
            sock = SOCKET_ERROR;
            sourceEvent = &Event::neverSet;
            sinkEvent = &Event::alwaysSet;
            rxRenderBuf = NULL;
            txRenderBuf = NULL;
        } else {
            QStatus status = SocketDup(other.sock, sock);
            if (status == ER_OK) {
                sourceEvent = new Event(sock, Event::IO_READ, false);
                sinkEvent = new Event(sock, Event::IO_WRITE, false);
                rxRenderBuf = new uint8_t[maxPacketStreamMtu];
                txRenderBuf = new uint8_t[maxPacketStreamMtu];
            } else {
                QCC_LogError(status, ("SocketDup failed"));
                sock = SOCKET_ERROR;
                sourceEvent = &Event::neverSet;
                sinkEvent = &Event::alwaysSet;
                rxRenderBuf = txRenderBuf = NULL;
            }
        }
    }

    return *this;
}

ICEPacketStream::~ICEPacketStream()
{
    Stop();
    if (sourceEvent != &Event::neverSet) {
        delete sourceEvent;
        sourceEvent = &Event::neverSet;
    }
    if (sinkEvent != &Event::alwaysSet) {
        delete sinkEvent;
        sinkEvent = &Event::alwaysSet;
    }
    if (rxRenderBuf) {
        delete[] rxRenderBuf;
    }
    if (txRenderBuf) {
        delete[] txRenderBuf;
    }
    if (sock != SOCKET_ERROR) {
        Close(sock);
    }
}

QStatus ICEPacketStream::Start()
{
    return ER_OK;
}

String ICEPacketStream::GetIPAddr() const
{
    return ipAddress.ToString();
}

QStatus ICEPacketStream::Stop()
{
    return ER_OK;
}

QStatus ICEPacketStream::PushPacketBytes(const void* buf, size_t numBytes, PacketDest& dest)
{
    QCC_DbgTrace(("ICEPacketStream::PushPacketBytes numBytes =%d", numBytes));

    size_t messageMtu = usingTurn ? mtuWithStunOverhead : maxPacketStreamMtu;
    assert(numBytes <= messageMtu);

    QStatus status = ER_OK;
    size_t sendBytes = numBytes;
    size_t sent = 0;

    if (localHost && remoteHost) {
        IPAddress ipAddr(dest.ip, dest.addrSize);
        status = qcc::SendTo(sock, ipAddr, dest.port, buf, sendBytes, sent);

        status = (sent == sendBytes) ? ER_OK : ER_OS_ERROR;
        if (status != ER_OK) {
            if (sent == (size_t) -1) {
                QCC_LogError(status, ("sendto failed: %s (%d)", ::strerror(errno), errno));
            } else {
                QCC_LogError(status, ("Short udp send: exp=%d, act=%d", numBytes, sent));
            }
        }
    } else {
        sendLock.Lock();
        if (usingTurn) {
            ScatterGatherList sgList;
            status = ComposeStunMessage(buf, numBytes, sgList);
            if (status == ER_OK) {
                status = SendToSG(sock, turnAddress, turnPort, sgList, sent);
            } else {
                QCC_LogError(status, ("ComposeStunMessage failed"));
            }
        } else {
            IPAddress ipAddr(dest.ip, dest.addrSize);
            status = qcc::SendTo(sock, ipAddr, dest.port, buf, sendBytes, sent);

            status = (sent == sendBytes) ? ER_OK : ER_OS_ERROR;
            if (status != ER_OK) {
                if (sent == (size_t) -1) {
                    QCC_LogError(status, ("sendto failed: %s (%d)", ::strerror(errno), errno));
                } else {
                    QCC_LogError(status, ("Short udp send: exp=%d, act=%d", numBytes, sent));
                }
            }
        }
        sendLock.Unlock();
    }

    return status;
}

QStatus ICEPacketStream::PullPacketBytes(void* buf, size_t reqBytes, size_t& actualBytes,
                                         PacketDest& sender, uint32_t timeout)
{
    QCC_DbgTrace(("ICEPacketStream::PullPacketBytes reqBytes=%d", reqBytes));

    QStatus status = ER_OK;

    void* recvBuf = buf;
    size_t recvBytes = reqBytes;

    if (usingTurn) {
        recvBuf = (void*)rxRenderBuf;
        recvBytes = maxPacketStreamMtu;
    }

    IPAddress tmpIpAddr;
    uint16_t tmpPort = 0;
    status =  qcc::RecvFrom(sock, tmpIpAddr, tmpPort, recvBuf, recvBytes, actualBytes);

    if (status == ER_OK) {
        tmpIpAddr.RenderIPBinary(sender.ip, IPAddress::IPv6_SIZE);
        sender.addrSize = tmpIpAddr.Size();
        sender.port = tmpPort;

        if (usingTurn) {
            status = StripStunOverhead(actualBytes, buf, reqBytes, actualBytes);
        }
    } else {
        QCC_LogError(status, ("recvfrom failed: %s", ::strerror(errno)));
    }

    QCC_DbgTrace(("ICEPacketStream::PullPacketBytes Done actualBytes=%d", actualBytes));
    return status;
}

String ICEPacketStream::ToString(const PacketDest& dest) const
{

    IPAddress ipAddr(dest.ip, dest.addrSize);
    String ret = ipAddr.ToString();
    ret += " (";
    ret += U32ToString(dest.port);
    ret += ")";
    return ret;
}

QStatus ICEPacketStream::ComposeStunMessage(const void* buf,
                                            size_t numBytes,
                                            ScatterGatherList& msgSG)
{
    QCC_DbgPrintf(("ICEPacketStream::ComposeStunMessage()"));

    assert(buf != NULL);

    QStatus status = ER_OK;

    ScatterGatherList sg;
    sg.AddBuffer(buf, numBytes);
    sg.SetDataSize(numBytes);

    StunMessage msg(STUN_MSG_INDICATION_CLASS, STUN_MSG_SEND_METHOD, reinterpret_cast<const uint8_t*>(hmacKey.c_str()), hmacKey.size());

    status = msg.AddAttribute(new StunAttributeUsername(turnUsername));
    if (status == ER_OK) {
        status = msg.AddAttribute(new StunAttributeXorPeerAddress(msg, remoteMappedAddress, remoteMappedPort));
    }
    if (status == ER_OK) {
        status = msg.AddAttribute(new StunAttributeAllocatedXorServerReflexiveAddress(msg, localSrflxAddress, localSrflxPort));
    }
    if (status == ER_OK) {
        status = msg.AddAttribute(new StunAttributeData(sg));
    }
    if (status == ER_OK) {
        status = msg.AddAttribute(new StunAttributeMessageIntegrity(msg));
    }
    if (status == ER_OK) {
        status = msg.AddAttribute(new StunAttributeFingerprint(msg));
    }
    if (status == ER_OK) {
        size_t renderSize = msg.RenderSize();
        assert(renderSize <= maxPacketStreamMtu);
        uint8_t* _txRenderBuf = txRenderBuf;
        status = msg.RenderBinary(_txRenderBuf, renderSize, msgSG);
    }

    return status;
}

QStatus ICEPacketStream::SendNATKeepAlive(void)
{
    QCC_DbgTrace(("ICEPacketStream::SendNATKeepAlive()"));

    QStatus status = ER_OK;

    StunMessage msg(STUN_MSG_INDICATION_CLASS, STUN_MSG_BINDING_METHOD, reinterpret_cast<const uint8_t*>(hmacKey.c_str()), hmacKey.size());

    size_t renderSize = msg.RenderSize();
    assert(renderSize <= maxPacketStreamMtu);
    ScatterGatherList msgSG;
    size_t sent;

    sendLock.Lock();
    uint8_t* _txRenderBuf = txRenderBuf;
    status = msg.RenderBinary(_txRenderBuf, renderSize, msgSG);

    qcc::IPAddress destnAddress = remoteAddress;
    uint16_t destnPort = remotePort;

    /* If we are using the relay candidate, we need to send the NAT keepalives to the
     * Relay allocation */
    if (usingTurn) {
        destnAddress = turnAddress;
        destnPort = turnPort;
    }

    if (status == ER_OK) {
        status = SendToSG(sock, destnAddress, destnPort, msgSG, sent);
        QCC_DbgPrintf(("ICEPacketStream::SendNATKeepAlive()(): Sent NAT keep-alive"));
    } else {
        QCC_LogError(status, ("ICEPacketStream::SendNATKeepAlive()(): Failed to send NAT keep-alive"));
    }

    sendLock.Unlock();

    return status;
}

QStatus ICEPacketStream::SendTURNRefresh(uint64_t time)
{
    QCC_DbgTrace(("ICEPacketStream::SendTURNRefresh()"));

    QStatus status = ER_OK;

    StunMessage msg(STUN_MSG_REQUEST_CLASS, STUN_MSG_REFRESH_METHOD, reinterpret_cast<const uint8_t*>(hmacKey.c_str()), hmacKey.size());

    status = msg.AddAttribute(new StunAttributeUsername(turnUsername));

    if (status == ER_OK) {
        status = msg.AddAttribute(new StunAttributeSoftware(String("AllJoyn ") + String(GetVersion())));
    }
    if (status == ER_OK) {
        status = msg.AddAttribute(new StunAttributeLifetime(ajn::TURN_PERMISSION_REFRESH_PERIOD_SECS));
    }
    if (status == ER_OK) {
        status = msg.AddAttribute(new StunAttributeRequestedTransport(ajn::REQUESTED_TRANSPORT_TYPE_UDP));
    }
    if (status == ER_OK) {
        status = msg.AddAttribute(new StunAttributeMessageIntegrity(msg));
    }
    if (status == ER_OK) {
        status = msg.AddAttribute(new StunAttributeFingerprint(msg));
    }
    if (status == ER_OK) {
        size_t renderSize = msg.RenderSize();
        assert(renderSize <= maxPacketStreamMtu);
        ScatterGatherList msgSG;
        size_t sent;

        sendLock.Lock();
        uint8_t* _txRenderBuf = txRenderBuf;
        status = msg.RenderBinary(_txRenderBuf, renderSize, msgSG);

        if (status == ER_OK) {
            status = SendToSG(sock, relayServerAddress, relayServerPort, msgSG, sent);
            QCC_DbgPrintf(("ICEPacketStream::SendTURNRefresh(): Sent TURN refresh"));

            // Set the TURN refresh time-stamp
            turnRefreshTimestamp = time;
        } else {
            QCC_LogError(status, ("ICEPacketStream::SendTURNRefresh(): Failed to send TURN refresh"));
        }

        sendLock.Unlock();
    }

    return status;
}

QStatus ICEPacketStream::StripStunOverhead(size_t rcvdBytes, void* dataBuf, size_t dataBufLen, size_t& actualBytes)
{
    QCC_DbgTrace(("ICEPacketStream::StripStunOverhead()"));

    QStatus status = ER_OK;

    size_t _rcvdBytes = rcvdBytes;
    const uint8_t* _rxRenderBuf = rxRenderBuf;

    if (((_rcvdBytes >= StunMessage::MIN_MSG_SIZE) && StunMessage::IsStunMessage(_rxRenderBuf, _rcvdBytes))) {

        uint16_t rawMsgType = 0;

        _rcvdBytes = rcvdBytes;
        _rxRenderBuf = rxRenderBuf;
        StunIOInterface::ReadNetToHost(_rxRenderBuf, _rcvdBytes, rawMsgType);

        if (StunMessage::ExtractMessageMethod(rawMsgType) == STUN_MSG_DATA_METHOD) {

            QCC_DbgPrintf(("%s: Received STUN_MSG_DATA_METHOD", __FUNCTION__));

            // Parse message and extract DATA attribute contents.
            size_t keyLen  = hmacKey.size();
            uint8_t* dummyHmac = new uint8_t[keyLen];
            StunMessage msg("", dummyHmac, keyLen);
            QStatus status;

            _rcvdBytes = rcvdBytes;
            _rxRenderBuf = rxRenderBuf;
            status = msg.Parse(_rxRenderBuf, _rcvdBytes);
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
                        assert(dataBufLen >= sgiter->len);
                        actualBytes = (dataBufLen <= sgiter->len) ? dataBufLen : sgiter->len;
                        ::memcpy(dataBuf, sgiter->buf, actualBytes);
                    }
                }
            }
            delete [] dummyHmac;
        } else {

            QCC_DbgPrintf(("%s: Received NAT keepalive or TURN refresh response", __FUNCTION__));

            // If there is no STUN_MSG_DATA_METHOD in the response, it means that this is a response for either a NAT keep alive request
            // or a TURN refresh request. We dont need to handle if the response was for a NAT keepalive. Whereas if it is a TURN
            // refresh response, it will have the lifetime attribute. We need to update the turnRefreshPeriod according to the value
            // of the lifetime attribute. In either case, we dont need to pass on any data to the PacketEngine. So actualBytes
            // should be set to 0.
            actualBytes = 0;

            uint16_t rawMsgType = 0;  // Initializer to make GCC 4.3.2 happy.
            uint16_t rawMsgSize = 0;
            uint32_t magicCookie = 0;

            _rcvdBytes = rcvdBytes;
            _rxRenderBuf = rxRenderBuf;

            StunIOInterface::ReadNetToHost(_rxRenderBuf, _rcvdBytes, rawMsgType);
            StunIOInterface::ReadNetToHost(_rxRenderBuf, _rcvdBytes, rawMsgSize);
            StunIOInterface::ReadNetToHost(_rxRenderBuf, _rcvdBytes, magicCookie);

            if (StunMessage::IsTypeOK(rawMsgType)) {
                QCC_DbgPrintf(("%s: StunMessage::IsTypeOK() successful", __FUNCTION__));
                // Check to ensure that we have indeed received a STUN response
                if (StunMessage::ExtractMessageClass(rawMsgType) == STUN_MSG_RESPONSE_CLASS) {
                    QCC_DbgPrintf(("%s: Received a STUN response message", __FUNCTION__));

                    // Parse message and extract Lifetime attribute contents.
                    size_t keyLen  = hmacKey.size();
                    uint8_t* dummyHmac = new uint8_t[keyLen];
                    StunMessage msg("", dummyHmac, keyLen);
                    QStatus status;

                    _rcvdBytes = rcvdBytes;
                    _rxRenderBuf = rxRenderBuf;
                    status = msg.Parse(_rxRenderBuf, _rcvdBytes);

                    if (status == ER_OK) {
                        StunMessage::const_iterator iter;

                        for (iter = msg.Begin(); iter != msg.End(); ++iter) {
                            if ((*iter)->GetType() == STUN_ATTR_LIFETIME) {
                                const StunAttributeLifetime& sa = *reinterpret_cast<StunAttributeLifetime*>(*iter);

                                turnRefreshPeriodUpdateLock.Lock();
                                turnRefreshPeriod = ((sa.GetLifetime() - ajn::TURN_REFRESH_WARNING_PERIOD_SECS) * 1000);
                                turnRefreshPeriodUpdateLock.Unlock();

                                QCC_DbgPrintf(("%s: Found Lifetime attribute(%d) in the received STUN response", __FUNCTION__, sa.GetLifetime()));

                                /* Break out of the for loop if we have found the Lifetime attribute as we are not interested
                                 * in any other attribute in this section */
                                break;
                            }
                        }
                    }
                    delete [] dummyHmac;
                } else {
                    QCC_DbgPrintf(("%s: Received message is not a STUN response", __FUNCTION__));
                }
            } else {
                QCC_DbgPrintf(("%s: Invalid STUN message type: %04x (%s, %s)",
                               __FUNCTION__,
                               rawMsgType,
                               StunMessage::MessageClassToString(StunMessage::ExtractMessageClass(rawMsgType)).c_str(),
                               StunMessage::MessageMethodToString(StunMessage::ExtractMessageMethod(rawMsgType)).c_str()));
            }

        }

    } else {
        status = ER_FAIL;
        QCC_LogError(status, ("ICEPacketStream::StripStunOverhead(): Received message is not a STUN message"));
    }

    return status;
}

}
