/**
 * @file
 * ProximityNameService implements a wrapper layer to utilize the WinRT proximity API
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

#include <qcc/IPAddress.h>
#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>
#include <qcc/IfConfig.h>
#include <qcc/winrt/utility.h>

#include "ns/IpNameService.h"
#include "ProximityNameService.h"
#include "ppltasks.h"

#define QCC_MODULE "PROXIMITY_NAME_SERVICE"
#define ENCODE_SHORT_GUID 1
#define EMPTY_DISPLAY_NAME " "

using namespace std;
using namespace qcc;
using namespace Platform;
using namespace Windows::Networking::Proximity;
using namespace Windows::Networking::Sockets;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage::Streams;
using namespace Windows::System::Threading;

namespace ajn {

/* Android-based Wi-Fi Direct provides Bonjour-style names. Bonjour is an extension of DNS (mDNS). DNS names are explicitly case-insensitive.
 * Because DNS names are case-insensitive, Android downshifts (convert to lower-case) all names that go through its Android Application Framework.
 * AllJoyn is an extension of D-Bus. D-Bus names are explicitly case-sensitive. When we compare endpoint names, we take into account case. Therein lies the rub.
 * For the purpose of inter-operability between Android and Win8/WinRT, we use a simple encoding/decoding mechanism to preserve the case sensibility:
 * each uppercase character is converted to lowercase preceded by '-'.
 */

static QStatus EncodeWFDBusName(const qcc::String& orig, qcc::String& encoded)
{
    QStatus status = ER_OK;
    for (int i = 0; i < orig.size(); i++) {
        if (isupper(orig[i]) || orig[i] == '-') {
            encoded.append('-');
            encoded.append(tolower(orig[i]));
        } else {
            encoded.append(orig[i]);
        }
    }
    QCC_DbgPrintf(("EncodeWFDBusName: orig(%s), encoded(%s)", orig.c_str(), encoded.c_str()));
    return status;
}

static QStatus DecodeWFDBusName(const qcc::String& orig, qcc::String& decoded)
{
    QStatus status = ER_OK;
    int i = 0;
    while (i < orig.size()) {
        if (orig[i] == '-') {
            i++;
            if (i < orig.size()) {
                decoded.append(toupper(orig[i]));
            } else {
                break;
            }
        } else {
            decoded.append(orig[i]);
        }
        i++;
    }
    QCC_DbgPrintf(("DecodeWFDBusName: orig(%s), decoded(%s)", orig.c_str(), decoded.c_str()));
    return status;
}

ProximityNameService::ProximityNameService(const qcc::String& guid) :
    m_timer(nullptr),
    m_peerFinderStarted(false),
    m_doDiscovery(false),
    m_port(0),
    m_tDuration(DEFAULT_DURATION),
    m_namePrefix(qcc::String::Empty),
    m_connRefCount(0)
{
    m_currentP2PLink.state = PROXIM_DISCONNECTED;
    m_currentP2PLink.localIp = qcc::String::Empty;
    m_currentP2PLink.remoteIp = qcc::String::Empty;
    m_currentP2PLink.localPort = 0;
    m_currentP2PLink.remotePort = 0;
    m_currentP2PLink.peerGuid = qcc::String::Empty;
    m_currentP2PLink.socket = nullptr;
    m_currentP2PLink.dataReader = nullptr;
    m_currentP2PLink.dataWriter = nullptr;
    m_currentP2PLink.socketClosed = true;
    GUID128 id(guid);
    m_sguid = id.ToShortString();
}

ProximityNameService::~ProximityNameService()
{
    QCC_DbgPrintf(("ProximityNameService::~ProximityNameService()"));
    ResetConnection();
}

void ProximityNameService::SetCallback(Callback<void, const qcc::String&, const qcc::String&, vector<qcc::String>&, uint8_t>* cb)
{
    QCC_DbgPrintf(("ProximityNameService::SetCallback()"));

    delete m_callback;
    m_callback = cb;
}

bool ProximityNameService::ShouldDoDiscovery()
{
    if (m_namePrefix == qcc::String::Empty) {
        return false;
    }
    if (m_namePrefix.size() > 0 && m_advertised.size() == 0) {
        return true;
    }

    return false;
}

void ProximityNameService::Start()
{
    QCC_DbgPrintf(("ProximityNameService::Start()"));
    Windows::Foundation::EventRegistrationToken m_token = PeerFinder::ConnectionRequested += ref new TypedEventHandler<Platform::Object ^, Windows::Networking::Proximity::ConnectionRequestedEventArgs ^>(this, \
                                                                                                                                                                                                           &ProximityNameService::ConnectionRequestedEventHandler, CallbackContext::Same);
    IpNameService::Instance().Acquire(m_sguid);
}

void ProximityNameService::Stop()
{
    QCC_DbgPrintf(("ProximityNameService::Stop()"));
    ResetConnection();
    NotifyDisconnected();
    m_connRefCount = 0;
    PeerFinder::ConnectionRequested -= m_token;
    IpNameService::Instance().Release();
}

void ProximityNameService::ConnectionRequestedEventHandler(Platform::Object ^ sender, ConnectionRequestedEventArgs ^ TriggeredConnectionStateChangedEventArgs)
{
    QCC_DbgPrintf(("ProximityNameService::ConnectionRequestedEventHandler() m_currentP2PLink.state(%d)", m_currentP2PLink.state));
    if (m_currentP2PLink.state == PROXIM_CONNECTING) {
        QCC_LogError(ER_OS_ERROR, ("Receive connection request while in PROXIM_CONNECTING state"));
        return;
    }
    auto requestingPeer = TriggeredConnectionStateChangedEventArgs->PeerInformation;
    auto op = PeerFinder::ConnectAsync(requestingPeer);

    Concurrency::task<StreamSocket ^> connectTask(op);
    connectTask.then([this](Concurrency::task<StreamSocket ^> resultTask) {
                         try {
                             m_currentP2PLink.socket = resultTask.get();
                             m_currentP2PLink.socketClosed = false;
                             m_currentP2PLink.state = PROXIM_CONNECTED;
                             m_currentP2PLink.dataReader = ref new DataReader(m_currentP2PLink.socket->InputStream);
                             m_currentP2PLink.dataWriter = ref new DataWriter(m_currentP2PLink.socket->OutputStream);
                             StartReader();
                             qcc::String loAddrStr = PlatformToMultibyteString(m_currentP2PLink.socket->Information->LocalAddress->CanonicalName);
                             size_t pos = loAddrStr.find_first_of('%');
                             if (qcc::String::npos != pos) {
                                 loAddrStr = loAddrStr.substr(0, pos);
                             }
                             m_currentP2PLink.localIp = loAddrStr;

                             IfConfigEntry wfdEntry;
                             wfdEntry.m_name = "win-wfd";
                             wfdEntry.m_addr = loAddrStr;
                             wfdEntry.m_prefixlen = static_cast<uint32_t>(-1);
                             wfdEntry.m_family = qcc::QCC_AF_INET6;
                             wfdEntry.m_flags = qcc::IfConfigEntry::UP;
                             wfdEntry.m_flags |= qcc::IfConfigEntry::MULTICAST;
                             wfdEntry.m_mtu = 1500;
                             wfdEntry.m_index = 18;
                             IpNameService::Instance().CreateVirtualInterface(wfdEntry);
                             IpNameService::Instance().OpenInterface(TRANSPORT_WFD, "win-wfd");
                             QCC_DbgPrintf(("P2P keep-live connection is established"));
                             assert(m_port && "m_port is invalid port");
                             IpNameService::Instance().Enable(TRANSPORT_WFD, 0, m_port, 0, 0, false, true, false, false);
                             if (m_advertised.size() > 0) {
                                 IpNameService::Instance().AdvertiseName(TRANSPORT_WFD, *(m_advertised.begin()));
                             }
#if DO_P2P_NAME_ADVERTISE
                             TransmitMyWKNs();
                             StartMaintainanceTimer();
#endif
                         } catch (Exception ^ e) {
                             m_currentP2PLink.state = PROXIM_DISCONNECTED;
                             qcc::String err = PlatformToMultibyteString(e->Message);
                             QCC_LogError(ER_OS_ERROR, ("ConnectionRequestedEventHandler ConnectAsync() Error (%s)", err.c_str()));
                             RestartPeerFinder();
                             if (m_doDiscovery) {
                                 BrowsePeers();
                             }
                         }
                     });
}

bool ProximityNameService::IsTriggeredConnectSupported()
{
    return (PeerFinder::SupportedDiscoveryTypes & PeerDiscoveryTypes::Triggered) ==
           PeerDiscoveryTypes::Triggered;
}

bool ProximityNameService::IsBrowseConnectSupported()
{
    return (PeerFinder::SupportedDiscoveryTypes & PeerDiscoveryTypes::Browse) ==
           PeerDiscoveryTypes::Browse;
}

void ProximityNameService::EnableAdvertisement(const qcc::String& name)
{
    QCC_DbgPrintf(("ProximityNameService::EnableAdvertisement (%s)", name.c_str()));
    assert(m_advertised.size() == 0 && "Only one service is allowed");
    m_mutex.Lock(MUTEX_CONTEXT);
    try {
        if (IsBrowseConnectSupported()) {
            if (m_namePrefix == qcc::String::Empty) {
                size_t pos = name.find_last_of('.');
                if (pos != qcc::String::npos) {
                    m_namePrefix = name.substr(0, pos);
                    QCC_DbgPrintf(("Get name prefix (%s) from well-known name (%s)", m_namePrefix.c_str(), name.c_str()));
                    assert(m_namePrefix.size() <= MAX_PROXIMITY_ALT_ID_SIZE);
                    if (!PeerFinder::AlternateIdentities->HasKey(L"Browse")) {
                        qcc::String encoded;
                        EncodeWFDBusName(m_namePrefix, encoded);
                        PeerFinder::AlternateIdentities->Insert(L"Browse", MultibyteToPlatformString(encoded.c_str()));
                        QCC_DbgPrintf(("Set Alt Id (%s)", encoded.c_str()));
                    }
                }
            } else {
                if (m_namePrefix.size() >= name.size() ||
                    name.compare(0, m_namePrefix.size(), m_namePrefix) != 0) {
                    QCC_LogError(ER_BUS_BAD_BUS_NAME, ("ProximityNameService::EnableAdvertisement() well-known name(%s) does not match the prefix(%s)", name.c_str(), m_namePrefix.c_str()));
                    m_mutex.Unlock(MUTEX_CONTEXT);
                    return;
                }
            }
            m_advertised.insert(name.substr(m_namePrefix.size() + 1));
            m_doDiscovery = ShouldDoDiscovery();
            if (IsConnected()) {
#if DO_P2P_NAME_ADVERTISE
                QCC_DbgPrintf(("EnableAdvertisement() already connected, TransmitMyWKNs Immidiately"));
                TransmitMyWKNs();
#endif
                m_mutex.Unlock(MUTEX_CONTEXT);
                return;
            }
            PeerFinder::Stop();
            PeerFinder::DisplayName = EncodeWknAdvertisement();
            Windows::Networking::Proximity::PeerFinder::Start();
            m_peerFinderStarted = true;
            QCC_DbgPrintf(("EnableAdvertisement Now DisplayName is (%s)", PlatformToMultibyteString(PeerFinder::DisplayName).c_str()));
        }
    } catch (Exception ^ e) {
        QCC_LogError(ER_FAIL, ("EnableAdvertisement() Error (%s)", PlatformToMultibyteString(e->Message).c_str()));
    }
    m_mutex.Unlock(MUTEX_CONTEXT);
}

void ProximityNameService::DisableAdvertisement(vector<qcc::String>& wkns)
{
    QCC_DbgPrintf(("ProximityNameService::DisableAdvertisement()"));
    assert(wkns.size() == 1 && "Only one service name is expected");
    m_mutex.Lock(MUTEX_CONTEXT);
    try {
        if (IsBrowseConnectSupported()) {
            Platform::String ^ updatedName;
            bool changed = false;
            for (uint32_t i = 0; i < wkns.size(); ++i) {
                qcc::String name = wkns[i];
                size_t pos = name.find_last_of('.');
                if (pos != qcc::String::npos) {
                    name = name.substr(pos + 1);
                }

                set<qcc::String>::iterator j = m_advertised.find(name);
                if (j != m_advertised.end()) {
                    m_advertised.erase(j);
                    changed = true;
                }
            }
            if (!changed) {
                m_mutex.Unlock(MUTEX_CONTEXT);
                return;
            }
            m_doDiscovery = ShouldDoDiscovery();
            if (IsConnected()) {
                IpNameService::Instance().CancelAdvertiseName(TRANSPORT_WFD, *(wkns.begin()));
#if DO_P2P_NAME_ADVERTISE
                QCC_DbgPrintf(("DisableAdvertisement() already connected, Transm)itMyWKNs Immidiately"));
                TransmitMyWKNs();
#endif
                m_mutex.Unlock(MUTEX_CONTEXT);
                return;
            }
            if (m_advertised.size() == 0) {
                updatedName = EMPTY_DISPLAY_NAME;
            } else {
                updatedName = EncodeWknAdvertisement();
            }

            PeerFinder::Stop();
            PeerFinder::DisplayName = updatedName;
            PeerFinder::Start();
            m_peerFinderStarted = true;
        }
    } catch (Exception ^ e) {
        QCC_LogError(ER_FAIL, ("DisableAdvertisement() Error (%s)", PlatformToMultibyteString(e->Message).c_str()));
    }
    m_mutex.Unlock(MUTEX_CONTEXT);
}

void ProximityNameService::EnableDiscovery(const qcc::String& namePrefix)
{
    QCC_DbgPrintf(("ProximityNameService::EnableDiscovery (%s)", namePrefix.c_str()));
    assert(namePrefix.size() > 0 && "The name prefix is expected to be non-empty");
    m_mutex.Lock(MUTEX_CONTEXT);
    try {
        if (IsBrowseConnectSupported()) {
            // Only one name prefix per app is allowed
            if (m_namePrefix != qcc::String::Empty) {
                QCC_LogError(ER_FAIL, ("ProximityNameService::EnableDiscovery() Only one name prefix is allowed"));
                m_mutex.Lock(MUTEX_CONTEXT);
                return;
            }
            qcc::String actualPrefix;
            if (namePrefix[namePrefix.size() - 1] == '*') {
                m_namePrefix = namePrefix.substr(0, namePrefix.size() - 1);
            } else {
                m_namePrefix = namePrefix;
            }

            assert(m_namePrefix.size() <= MAX_PROXIMITY_ALT_ID_SIZE);
            if (!PeerFinder::AlternateIdentities->HasKey(L"Browse")) {
                qcc::String encoded;
                EncodeWFDBusName(m_namePrefix, encoded);
                PeerFinder::AlternateIdentities->Insert(L"Browse", MultibyteToPlatformString(encoded.c_str()));
                QCC_DbgPrintf(("Set Alt Id (%s)", m_namePrefix.c_str()));
            }

            m_doDiscovery = ShouldDoDiscovery();

            if (!m_peerFinderStarted) {
                PeerFinder::DisplayName = EMPTY_DISPLAY_NAME;
                Windows::Networking::Proximity::PeerFinder::Start();
                m_peerFinderStarted = true;
            }

            if (m_doDiscovery && m_currentP2PLink.state == PROXIM_DISCONNECTED) {
                BrowsePeers();
            }
        }
    } catch (Exception ^ e) {
        QCC_LogError(ER_FAIL, ("EnableDiscovery() Error (%s)", PlatformToMultibyteString(e->Message).c_str()));
    }
    m_mutex.Unlock(MUTEX_CONTEXT);
}

void ProximityNameService::DisableDiscovery(const qcc::String& namePrefix)
{
    QCC_DbgPrintf(("ProximityNameService::DisableDiscovery (%s)", namePrefix.c_str()));
    m_mutex.Lock(MUTEX_CONTEXT);
    if (namePrefix.compare(m_namePrefix) == 0) {
        if (PeerFinder::AlternateIdentities->HasKey(L"Browse")) {
            PeerFinder::AlternateIdentities->Remove(L"Browse");
        }
        m_namePrefix = qcc::String::Empty;
        m_doDiscovery = ShouldDoDiscovery();
    } else {
        QCC_DbgPrintf(("ProximityNameService::DisableDiscovery() namePrefix(%s) does not match m_namePrefix(%s)", namePrefix.c_str(), m_namePrefix.c_str()));
    }
    m_mutex.Unlock(MUTEX_CONTEXT);
}

void ProximityNameService::BrowsePeers()
{
    QCC_DbgPrintf(("ProximityNameService::BrowsePeers()"));
    if (!IsBrowseConnectSupported()) {
        m_currentP2PLink.state = PROXIM_DISCONNECTED;
        return;
    }
    if (!m_peerFinderStarted || !m_doDiscovery) {
        return;
    }
    m_currentP2PLink.state = PROXIM_BROWSING;
    auto op = PeerFinder::FindAllPeersAsync();
    Concurrency::task<IVectorView<PeerInformation ^>^> findAllPeersTask(op);

    findAllPeersTask.then([this](Concurrency::task<IVectorView<PeerInformation ^>^> resultTask)
                          {
                              try{
                                  auto peerInfoList = resultTask.get();
                                  bool foundValidPeer = false;
                                  QCC_DbgPrintf(("peerInfoList size (%d)", peerInfoList->Size));
                                  if (peerInfoList->Size > 0) {
                                      unsigned int i = 0;
                                      for (; i < peerInfoList->Size; i++) {
                                          Platform::String ^ platStr = peerInfoList->GetAt(i)->DisplayName;
                                          qcc::String mbStr = PlatformToMultibyteString(platStr);
                                          QCC_DbgPrintf(("Peer (%d) DisplayName = (%s)", i, mbStr.c_str()));
                                          size_t startPos = 0;
                                          if (mbStr.compare(EMPTY_DISPLAY_NAME) == 0) {
                                              continue;
                                          }

                                          size_t pos = mbStr.find_first_of('|');
                                          if (pos == qcc::String::npos) {
                                              QCC_LogError(ER_OS_ERROR, ("separator '|' is expected in (%s)", mbStr.c_str()));
                                              continue;
                                          }

                                          QCC_DbgPrintf(("Parse short GUID string"));
                                          // short version, 8 bytes
                                          assert(pos == GUID128::SHORT_SIZE);
                                          qcc::String guidStr = mbStr.substr(0, GUID128::SHORT_SIZE);

                                          std::vector<qcc::String> nameList;
                                          while (true) {
                                              startPos = ++pos;
                                              pos = mbStr.find_first_of('|', startPos);
                                              if (pos != qcc::String::npos) {
                                                  qcc::String wkn = m_namePrefix;
                                                  wkn += '.';
                                                  DecodeWFDBusName(mbStr.substr(startPos, (pos - startPos)), wkn);
                                                  nameList.push_back(wkn);
                                              } else {
                                                  // the last one
                                                  if (startPos < mbStr.size()) {
                                                      qcc::String wkn = m_namePrefix;
                                                      wkn += '.';
                                                      DecodeWFDBusName(mbStr.substr(startPos), wkn);
                                                      QCC_DbgPrintf(("name=(%s)", wkn.c_str()));
                                                      nameList.push_back(wkn);
                                                  }
                                                  break;
                                              }
                                          }

                                          if (nameList.size() > 0) {
                                              qcc::String busAddress = "proximity:guid=";
                                              busAddress += guidStr;
                                              (*m_callback)(busAddress, guidStr, nameList, DEFAULT_PREASSOCIATION_TTL);
                                              m_peersMap[guidStr] = std::make_pair(peerInfoList->GetAt(i), nameList);
                                              foundValidPeer = true;
                                          }
                                      }

                                      // stop browsing peers
                                      if (foundValidPeer) {
                                          return;
                                      }
                                  }
                              } catch (Exception ^ e) {
                                  RestartPeerFinder();
                                  qcc::String err = PlatformToMultibyteString(e->Message);
                                  QCC_LogError(ER_OS_ERROR, ("Exception (%s) occurred while finding peers", err.c_str()));
                              }

                              if (m_doDiscovery && ((m_currentP2PLink.state == PROXIM_DISCONNECTED) || (m_currentP2PLink.state == PROXIM_BROWSING))) {
                                  qcc::Sleep(1024 + qcc::Rand32() % 1024);
                                  BrowsePeers();
                              }
                          });
}

QStatus ProximityNameService::EstasblishProximityConnection(qcc::String guidStr)
{
    QStatus status = ER_OK;
    // If there is already a P2P connection established
    if (m_currentP2PLink.state == PROXIM_CONNECTED) {
        if (guidStr.compare(m_currentP2PLink.peerGuid) != 0) {
            status = ER_OS_ERROR;
            QCC_LogError(ER_OS_ERROR, ("Trying to establish P2P connection to peer (%s) while already connected to peer(%s)", m_currentP2PLink.peerGuid.c_str(), guidStr.c_str()));
        }
        return status;
    }
    std::map<qcc::String, std::pair<PeerInformation ^, std::vector<qcc::String> > >::iterator it = m_peersMap.find(guidStr);
    if (it != m_peersMap.end()) {
        PeerInformation ^ peerInfo = (it->second).first;
        assert(peerInfo != nullptr);
        QCC_DbgPrintf(("Connecting to Peer ... %d", m_peersMap.size()));

        m_currentP2PLink.state = PROXIM_CONNECTING;

        try {
            auto op = PeerFinder::ConnectAsync(peerInfo);
            Concurrency::task<StreamSocket ^> connectTask(op);
            connectTask.wait();
            m_currentP2PLink.socket = connectTask.get();
            m_currentP2PLink.peerGuid = guidStr;
            qcc::String busAddress = "proximity:guid=" + it->first;
            if (m_callback) {
                (*m_callback)(busAddress, it->first, (it->second).second, 0xFF);
            }
        } catch (Exception ^ e) {
            status = ER_PROXIMITY_CONNECTION_ESTABLISH_FAIL;
            qcc::String err = PlatformToMultibyteString(e->Message);
            QCC_LogError(status, ("ProximityNameService::Connect Error (%s) %x", err.c_str(), e->HResult));
            RestartPeerFinder();
            if (m_doDiscovery) {
                BrowsePeers();
            }
        }

        if (status == ER_OK) {
            qcc::String rAddrStr = PlatformToMultibyteString(m_currentP2PLink.socket->Information->RemoteAddress->CanonicalName);
            size_t pos = rAddrStr.find_first_of('%');
            if (qcc::String::npos != pos) {
                rAddrStr = rAddrStr.substr(0, pos);
            }
            m_currentP2PLink.remoteIp = rAddrStr;

            qcc::String loAddrStr = PlatformToMultibyteString(m_currentP2PLink.socket->Information->LocalAddress->CanonicalName);
            pos = loAddrStr.find_first_of('%');
            if (qcc::String::npos != pos) {
                loAddrStr = loAddrStr.substr(0, pos);
            }
            m_currentP2PLink.localIp = rAddrStr;
            m_currentP2PLink.state = PROXIM_CONNECTED;
            m_currentP2PLink.socketClosed = false;
            m_currentP2PLink.dataReader = ref new DataReader(m_currentP2PLink.socket->InputStream);
            m_currentP2PLink.dataWriter = ref new DataWriter(m_currentP2PLink.socket->OutputStream);

            IfConfigEntry wfdEntry;
            wfdEntry.m_name = "win-wfd";
            wfdEntry.m_addr = loAddrStr;
            wfdEntry.m_prefixlen = static_cast<uint32_t>(-1);
            wfdEntry.m_family = qcc::QCC_AF_INET6;
            wfdEntry.m_flags = qcc::IfConfigEntry::UP;
            wfdEntry.m_flags |= qcc::IfConfigEntry::MULTICAST;
            wfdEntry.m_mtu = 1500;
            wfdEntry.m_index = 18;
            IpNameService::Instance().CreateVirtualInterface(wfdEntry);
            IpNameService::Instance().OpenInterface(TRANSPORT_WFD, "win-wfd");
            assert(m_port && "m_port is invalid port");
            IpNameService::Instance().Enable(TRANSPORT_WFD, 0, m_port, 0, 0, false, true, false, false);
            if (m_advertised.size() > 0) {
                IpNameService::Instance().AdvertiseName(TRANSPORT_WFD, *(m_advertised.begin()));
            }
            QCC_DbgPrintf(("P2P keep-live connection is established"));
            StartReader();
#if DO_P2P_NAME_ADVERTISE
            TransmitMyWKNs();
            StartMaintainanceTimer();
#endif
        }
    } else {
        status = ER_PROXIMITY_NO_PEERS_FOUND;
    }

    return status;
}

void ProximityNameService::RestartPeerFinder()
{
    ResetConnection();
    PeerFinder::Start();
    m_peerFinderStarted = true;
}

void ProximityNameService::ResetConnection()
{
    QCC_DbgPrintf(("ProximityNameService::Reset()"));
    if (m_peerFinderStarted) {
        PeerFinder::Stop();
        m_peerFinderStarted = false;
        if (m_currentP2PLink.socket != nullptr) {
            m_currentP2PLink.socketClosed = true;
            delete m_currentP2PLink.socket;
            m_currentP2PLink.socket = nullptr;
            delete m_currentP2PLink.dataReader;
            m_currentP2PLink.dataReader = nullptr;
            delete m_currentP2PLink.dataWriter;
            m_currentP2PLink.dataWriter = nullptr;
            m_currentP2PLink.state = PROXIM_DISCONNECTED;
            m_currentP2PLink.localIp = qcc::String::Empty;
            m_currentP2PLink.remoteIp = qcc::String::Empty;
            m_currentP2PLink.localPort = 0;
            m_currentP2PLink.remotePort = 0;
            m_currentP2PLink.peerGuid = qcc::String::Empty;
            IpNameService::Instance().CloseInterface(TRANSPORT_WFD, "win-wfd");
            IpNameService::Instance().DeleteVirtualInterface("win-wfd");
        }
    }

    if (m_callback && m_peersMap.size() > 0) {
        auto it = m_peersMap.begin();
        for (; it != m_peersMap.end(); it++) {
            qcc::String busAddress = "proximity:guid=";
            qcc::String guidStr = it->first;
            busAddress += guidStr;
            (*m_callback)(busAddress, guidStr, (it->second).second, 0);
        }
    }
    m_peersMap.clear();

    if (m_timer != nullptr) {
        QCC_DbgPrintf(("ProximityNameService::StopMaintainanceTimer"));
        m_timer->Cancel();
        delete m_timer;
        m_timer = nullptr;
    }

    IpNameService::Instance().Enable(TRANSPORT_WFD, 0, 0, 0, 0, false, false, false, false);
}

Platform::String ^ ProximityNameService::EncodeWknAdvertisement()
{
    qcc::String encodedStr;
    QCC_DbgPrintf(("ProximityNameService::EncodeWknAdvertisement() guid(%s) name size(%d)", m_sguid.c_str(), m_advertised.size()));
    encodedStr.append(m_sguid);
    assert((encodedStr.size() + 1) <= MAX_DISPLAYNAME_SIZE);
    assert(m_advertised.size() > 0);
    std::set<qcc::String>::iterator it = m_advertised.begin();
    while (encodedStr.size() < MAX_DISPLAYNAME_SIZE && it != m_advertised.end()) {
        encodedStr.append("|");
        qcc::String encoded;
        EncodeWFDBusName(*it, encoded);
        if ((encodedStr.size() + encoded.size()) <= MAX_DISPLAYNAME_SIZE) {
            encodedStr.append(encoded);
            it++;
        } else {
            break;
        }
    }
    Platform::String ^ platformStr = MultibyteToPlatformString(encodedStr.c_str());
    return platformStr;
}

void ProximityNameService::StartReader()
{
    QCC_DbgPrintf(("ProximityNameService::StartReader()"));
    Concurrency::task<unsigned int> loadTask(m_currentP2PLink.dataReader->LoadAsync(sizeof(unsigned int)));
    loadTask.then([this](Concurrency::task<unsigned int> stringBytesTask)
                  {
                      try{
                          unsigned int bytesRead = stringBytesTask.get();
                          if (bytesRead > 0) {
                              unsigned int nbytes = (unsigned int)m_currentP2PLink.dataReader->ReadUInt32();
                              Concurrency::task<unsigned int> loadStringTask(m_currentP2PLink.dataReader->LoadAsync(nbytes));
                              loadStringTask.then([this, nbytes](Concurrency::task<unsigned int> resultTask) {
                                                      try{
                                                          unsigned int bytesRead = resultTask.get();
                                                          if (bytesRead > 0) {
                                                              Platform::Array<unsigned char> ^ buffer = ref new Platform::Array<unsigned char>(nbytes);
                                                              m_currentP2PLink.dataReader->ReadBytes(buffer);
                                                              qcc::String addrStr = PlatformToMultibyteString(m_currentP2PLink.socket->Information->RemoteAddress->CanonicalName);
                                                              size_t pos = addrStr.find_first_of('%');
                                                              if (qcc::String::npos != pos) {
                                                                  addrStr = addrStr.substr(0, pos);
                                                              }
                                                              qcc::IPAddress address(addrStr);
#if DO_P2P_NAME_ADVERTISE
                                                              HandleProtocolMessage(buffer->Data, nbytes, address);
#endif
                                                              StartReader();
                                                          } else {
                                                              qcc::String err = "The remote side closed the socket";
                                                              SocketError(err);
                                                          }
                                                      }catch (Exception ^ e) {
                                                          if (!m_currentP2PLink.socketClosed) {
                                                              qcc::String err = "Failed to read from socket: ";
                                                              err += PlatformToMultibyteString(e->Message).c_str();
                                                              SocketError(err);
                                                          }
                                                      }
                                                  });
                          } else {
                              qcc::String err = "The remote side closed the socket";
                              SocketError(err);
                          }
                      }catch (Exception ^ e) {
                          if (!m_currentP2PLink.socketClosed) {
                              qcc::String err = "Failed to read from socket: ";
                              err += PlatformToMultibyteString(e->Message).c_str();
                              SocketError(err);
                          }
                      }
                  });
}

void ProximityNameService::SocketError(qcc::String& errMsg)
{
    QCC_LogError(ER_FAIL, ("ProximityNameService::SocketError (%s)", errMsg.c_str()));
    NotifyDisconnected();
    m_connRefCount = 0;

    if (!m_currentP2PLink.socketClosed) {
        m_currentP2PLink.socketClosed = true;
        RestartPeerFinder();
        // start-over again
        if (m_doDiscovery) {
            BrowsePeers();
        }
    }
}

QStatus ProximityNameService::GetEndpoints(qcc::String& ipv6address, uint16_t& port)
{
    QCC_DbgPrintf(("ProximityNameService::GetEndpoints()"));
    QStatus status = ER_OK;
    if (!m_currentP2PLink.localIp.empty()) {
        ipv6address = m_currentP2PLink.localIp;
        port = m_port;
    } else {
        status = ER_FAIL;
        QCC_LogError(status, ("The listen address is empy"));
    }
    return status;
}

void ProximityNameService::SetEndpoints(const qcc::String& ipv6address, const uint16_t port)
{
    QCC_DbgPrintf(("ProximityNameService::SetEndpoints(port(%d))", port));
    m_port = port;
}

int32_t ProximityNameService::IncreaseP2PConnectionRef()
{
    IncrementAndFetch(&m_connRefCount);
    QCC_DbgPrintf(("ProximityNameService::IncreaseP2PConnectionRef(%d)", m_connRefCount));
    return m_connRefCount;
}

int32_t ProximityNameService::DecreaseP2PConnectionRef() {
    QCC_DbgPrintf(("ProximityNameService::DecreaseP2PConnectionRef(%d)", m_connRefCount));
    if (DecrementAndFetch(&m_connRefCount) == 0) {
        // tear down the P2P connection
        ResetConnection();
    }
    return m_connRefCount;
}

void ProximityNameService::RegisterProximityListener(ProximityListener* listener)
{
    QCC_DbgPrintf(("ProximityNameService::RegisterProximityListener(%p)", listener));
    m_listeners.push_back(listener);
}

void ProximityNameService::UnRegisterProximityListener(ProximityListener* listener)
{
    QCC_DbgPrintf(("ProximityNameService::UnRegisterProximityListener(%p)", listener));
    m_listeners.remove(listener);
}

void ProximityNameService::NotifyDisconnected()
{
    QCC_DbgPrintf(("ProximityNameService::NotifyDisconnected()"));
    std::list<ProximityListener*>::iterator it = m_listeners.begin();
    for (; it != m_listeners.end(); it++) {
        (*it)->OnProximityDisconnected();
    }
}

bool ProximityNameService::GetPeerConnectSpec(const qcc::String guid, qcc::String& connectSpec)
{
    assert(m_currentP2PLink.peerGuid == guid);
    if (m_currentP2PLink.state == PROXIM_CONNECTED) {
        char addrbuf[70];
        snprintf(addrbuf, sizeof(addrbuf), "proximity:addr=%s,port=%d", m_currentP2PLink.remoteIp.c_str(), m_port);
        connectSpec = addrbuf;
        return true;
    } else {
        QCC_LogError(ER_OS_ERROR, ("No valid P2P link available"));
    }

    return false;
}

extern bool IpNameServiceImplWildcardMatch(qcc::String str, qcc::String pat);

#if DO_P2P_NAME_ADVERTISE
void ProximityNameService::StartMaintainanceTimer()
{
    QCC_DbgPrintf(("ProximityNameService::StartMaintainanceTimer(interval = %d)", TRANSMIT_INTERVAL));
    #define HUNDRED_NANOSECONDS_PER_MILLISECOND 10000
    Windows::Foundation::TimeSpan ts = { TRANSMIT_INTERVAL* HUNDRED_NANOSECONDS_PER_MILLISECOND };
    m_timer = ThreadPoolTimer::CreatePeriodicTimer(ref new TimerElapsedHandler([&] (ThreadPoolTimer ^ timer) {
                                                                                   TimerCallback(timer);
                                                                               }), ts);
    assert(m_timer != nullptr);
}

void ProximityNameService::TimerCallback(Windows::System::Threading::ThreadPoolTimer ^ timer)
{
    TransmitMyWKNs();
}

void ProximityNameService::Locate(const qcc::String& namePrefix)
{
    QCC_DbgHLPrintf(("ProximityNameService::Locate(): %s", namePrefix.c_str()));

    //
    // Send a request to the network over our multicast channel,
    // asking for anyone who supports the specified well-known name.
    //
    WhoHas whoHas;
    whoHas.SetTcpFlag(true);
    whoHas.SetIPv6Flag(true);
    whoHas.AddName(namePrefix);

    Header header;
    header.SetVersion(0, 0);
    header.SetTimer(m_tDuration);
    header.AddQuestion(whoHas);

    // Send the message out over the proximity link.
    //
    SendProtocolMessage(header);
}

void ProximityNameService::TransmitMyWKNs(void)
{
    QCC_DbgPrintf(("ProximityNameService::TransmitMyWKNs() m_currentP2PLink.state(%d)", m_currentP2PLink.state));
    if (m_currentP2PLink.state != PROXIM_CONNECTED) {
        return;
    }
    //
    // We need a valid port before we send something out to the local subnet.
    // Note that this is the daemon contact port, not the name service port
    // to which we send advertisements.
    //
    if (m_port == 0) {
        QCC_DbgPrintf(("ProximityNameService::TransmitMyWKNs(): Port not set"));
        return;
    }

    //
    // There are at least two threads wandering through the advertised list.
    // We are running short on toes, so don't shoot any more off by not being
    // thread-unaware.
    //
    m_mutex.Lock();

    //
    // The underlying protocol is capable of identifying both TCP and UDP
    // services.  Right now, the only possibility is TCP.
    //
    IsAt isAt;
    isAt.SetTcpFlag(true);
    isAt.SetUdpFlag(false);

    //
    // Always send the provided daemon GUID out with the reponse.
    //
    isAt.SetGuid(m_sguid);

    //
    // Send a protocol message describing the entire (complete) list of names
    // we have.
    //
    isAt.SetCompleteFlag(true);

    isAt.SetPort(m_port);

    //
    // Add all of our adversised names to the protocol answer message.
    //
    for (set<qcc::String>::iterator i = m_advertised.begin(); i != m_advertised.end(); ++i) {
        isAt.AddName(*i);
    }
    m_mutex.Unlock();

    //
    // The header ties the whole protocol message together.  By setting the
    // timer, we are asking for everyone who hears the message to remember
    // the advertisements for that number of seconds.
    //
    Header header;
    header.SetVersion(0, 0);
    header.SetTimer(m_tDuration);
    header.AddAnswer(isAt);

    //
    // Send the message out over the proximity link.
    //
    SendProtocolMessage(header);
}

void ProximityNameService::SendProtocolMessage(Header& header)
{
    QCC_DbgPrintf(("ProximityNameService::SendProtocolMessage()"));
    if (!m_currentP2PLink.socketClosed && m_currentP2PLink.socket != nullptr && m_currentP2PLink.dataWriter != nullptr) {
        size_t size = header.GetSerializedSize();
        uint8_t* buffer = new uint8_t[size];
        header.Serialize(buffer);
        Platform::Array<unsigned char>^ byteArry = ref new Platform::Array<unsigned char>(buffer, size);
        m_currentP2PLink.dataWriter->WriteUInt32(size);
        m_currentP2PLink.dataWriter->WriteBytes(byteArry);

        concurrency::task<unsigned int> storeTask(m_currentP2PLink.dataWriter->StoreAsync());
        storeTask.then([this](concurrency::task<unsigned int> resultTask)
                       {
                           try {
                               unsigned int nBytesWritten = resultTask.get();
                               if (nBytesWritten == 0) {
                                   qcc::String errMsg = "The remote side closed the socket";
                                   SocketError(errMsg);
                               }
                           } catch (Platform::Exception ^ e) {
                               qcc::String errMsg = "Fail to send message with Error";
                               errMsg += PlatformToMultibyteString(e->Message);
                               SocketError(errMsg);
                           }
                       });
    } else {
        QCC_DbgPrintf(("ProximityNameService::SendProtocolMessage m_currentP2PLink.socketClosed(%d) m_currentP2PLink.socket(%p) m_currentP2PLink.dataWriter(%p)", m_currentP2PLink.socketClosed, m_currentP2PLink.socket, m_currentP2PLink.dataWriter));
    }
}

void ProximityNameService::HandleProtocolQuestion(WhoHas whoHas, qcc::IPAddress address)
{
    QCC_DbgHLPrintf(("ProximityNameService::HandleProtocolQuestion()"));

    //
    // There are at least two threads wandering through the advertised list.
    // We are running short on toes, so don't shoot any more off by not being
    // thread-unaware.
    //
    m_mutex.Lock();

    //
    // Loop through the names we are being asked about, and if we have
    // advertised any of them, we are going to need to respond to this
    // question.
    //
    bool respond = false;
    for (uint32_t i = 0; i < whoHas.GetNumberNames(); ++i) {
        qcc::String wkn = whoHas.GetName(i);

        //
        // Zero length strings are unmatchable.  If you want to do a wildcard
        // match, you've got to send a wildcard character.
        //
        if (wkn.size() == 0) {
            continue;
        }

        //
        // check to see if this name on the list of names we advertise.
        //
        for (set<qcc::String>::iterator j = m_advertised.begin(); j != m_advertised.end(); ++j) {

            //
            // The requested name comes in from the WhoHas message and we
            // allow wildcards there.
            //
            if (IpNameServiceImplWildcardMatch((*j), wkn)) {
                QCC_DbgHLPrintf(("ProximityNameService::HandleProtocolQuestion(): request for %s does not match my %s",
                                 wkn.c_str(), (*j).c_str()));
                continue;
            } else {
                respond = true;
                break;
            }
        }

        //
        // If we find a match, don't bother going any further since we need
        // to respond in any case.
        //
        if (respond) {
            break;
        }
    }

    m_mutex.Unlock();

    //
    // Since any response we send must include all of the advertisements we
    // are exporting; this just means to TransmitMyWKNs all of our advertisements.
    //
    if (respond) {
        TransmitMyWKNs();
    }
}

void ProximityNameService::HandleProtocolAnswer(IsAt isAt, uint32_t timer, qcc::IPAddress address)
{
    QCC_DbgHLPrintf(("ProximityNameService::HandleProtocolAnswer()"));

    //
    // If there are no callbacks we can't tell the user anything about what is
    // going on the net, so it's pointless to go any further.
    //

    if (m_callback == 0) {
        QCC_DbgHLPrintf(("ProximityNameService::HandleProtocolAnswer(): No callback, so nothing to do"));
        return;
    }

    vector<qcc::String> wkn;

    for (uint8_t i = 0; i < isAt.GetNumberNames(); ++i) {
        QCC_DbgHLPrintf(("ProximityNameService::HandleProtocolAnswer(): Got well-known name %s", isAt.GetName(i).c_str()));
        wkn.push_back(isAt.GetName(i));
    }

    sort(wkn.begin(), wkn.end());

    qcc::String guid = isAt.GetGuid();
    qcc::String busAddress = "proximity:guid=" + guid;
    QCC_DbgHLPrintf(("ProximityNameService::HandleProtocolAnswer(): Calling back with %s", busAddress.c_str()));
    if (m_callback) {
        (*m_callback)(busAddress, guid, wkn, timer);
    }
}

void ProximityNameService::HandleProtocolMessage(uint8_t const* buffer, uint32_t nbytes, qcc::IPAddress address)
{
    QCC_DbgHLPrintf(("ProximityNameService::HandleProtocolMessage(0x%x, %d, %s)", buffer, nbytes, address.ToString().c_str()));

    Header header;
    size_t bytesRead = header.Deserialize(buffer, nbytes);
    if (bytesRead != nbytes) {
        QCC_DbgPrintf(("ProximityNameService::HandleProtocolMessage(): Deserialize(): Error"));
        return;
    }

    //
    // We only understand version zero packets for now.
    //
    uint32_t nsVersion, msgVersion;
    header.GetVersion(nsVersion, msgVersion);
    if (msgVersion != 0) {
        QCC_DbgPrintf(("ProximityNameService::HandleProtocolMessage(): Unknown version: Error"));
        return;
    }

    //
    // If the received packet contains questions, see if we can answer them.
    // We have the underlying device in loopback mode so we can get receive
    // our own questions.  We usually don't have an answer and so we don't
    // reply, but if we do have the requested names, we answer ourselves
    // to pass on this information to other interested bystanders.
    //
    for (uint8_t i = 0; i < header.GetNumberQuestions(); ++i) {
        HandleProtocolQuestion(header.GetQuestion(i), address);
    }

    //
    // If the received packet contains answers, see if they are answers to
    // questions we think are interesting.  Make sure we are not talking to
    // ourselves unless we are told to for debugging purposes
    //
    for (uint8_t i = 0; i < header.GetNumberAnswers(); ++i) {
        IsAt isAt = header.GetAnswer(i);
        HandleProtocolAnswer(isAt, header.GetTimer(), address);
    }
}

#endif

} // namespace ajn

