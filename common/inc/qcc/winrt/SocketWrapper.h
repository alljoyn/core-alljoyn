/******************************************************************************
 *
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
 *
 *****************************************************************************/

#pragma once

#include <qcc/winrt/SocketWrapperTypes.h>
#include <qcc/Mutex.h>
#include <qcc/Event.h>
#include <qcc/Semaphore.h>
#include <qcc/Thread.h>
#include <map>
#include <list>
#include <ctxtcall.h>
#include <ppltasks.h>

namespace qcc {
namespace winrt {

typedef enum {
    None = 1 << 0,
    Bind = 1 << 1,
    Listen = 1 << 2,
    Connect = 1 << 3,
    Exception = 1 << 4,
} BindingState;

#ifdef COMMON_WINRT_PUBLIC
public
#else
private
#endif
enum class Events {
    None = 0,
    Read = 1 << 0,
    Write = 1 << 1,
    Exception = 1 << 2,
    All = (int)Read | (int)Write | (int)Exception,
};

ref class UDPMessage sealed {
  public:
    UDPMessage(Windows::Networking::Sockets::DatagramSocket ^ socket, Windows::Storage::Streams::DataReader ^ reader, Platform::String ^ remoteHostname, int remotePort)
    {
        Socket = socket;
        Reader = reader;
        RemoteHostname = remoteHostname;
        RemotePort = remotePort;
    }

    property Windows::Networking::Sockets::DatagramSocket ^ Socket;
    property Windows::Storage::Streams::DataReader ^ Reader;
    property Platform::String ^ RemoteHostname;
    property int RemotePort;
};

#ifdef COMMON_WINRT_PUBLIC
public
#else
private
#endif
delegate void SocketWrapperEventsChangedHandler(Platform::Object ^ source, int events);

#ifdef COMMON_WINRT_PUBLIC
public
#else
private
#endif
ref class SocketWrapper sealed {
  public:
    SocketWrapper();
    virtual ~SocketWrapper();

    uint32_t Init(AddressFamily addrFamily, SocketType type);
    uint32_t SocketDup(Platform::WriteOnlyArray<SocketWrapper ^> ^ dupSocket);
    uint32_t Bind(Platform::String ^ bindName, int localPort);
    uint32_t Listen(int backlog);
    uint32_t Accept(Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort, Platform::WriteOnlyArray<SocketWrapper ^> ^ newSocket);
    uint32_t SetBlocking(bool blocking);
    uint32_t SetNagle(bool useNagle);
    uint32_t Connect(Platform::String ^ remoteAddr, int remotePort);
    uint32_t SendTo(Platform::String ^ remoteAddr, int remotePort,
                    const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent);
    uint32_t RecvFrom(Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort,
                      Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received);
    uint32_t Send(const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent);
    uint32_t Recv(Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received);
    uint32_t GetLocalAddress(Platform::WriteOnlyArray<Platform::String ^> ^ addr, Platform::WriteOnlyArray<int> ^ port);
    uint32_t Close();
    uint32_t Shutdown();
    uint32_t JoinMulticastGroup(Platform::String ^ host);
    void SetEventMask(int mask);
    int GetEvents();

    event SocketWrapperEventsChangedHandler ^ SocketEventsChanged;
    property uint32_t LastError;
    property bool Ssl
    {
        void set(bool ssl);
        bool get();
    }

  protected:
    friend class SocketWrapperEvents;
    void ExecuteSocketEventsChanged(int flags);

  private:
    Platform::String ^ SanitizeAddress(Platform::String ^ hostName);
    uint32_t IsValidAddress(Platform::String ^ hostName);
    uint32_t Init(Windows::Networking::Sockets::StreamSocket ^ socket, Windows::Storage::Streams::DataReader ^ reader, AddressFamily addrFamily);
    uint32_t QueueTraffic();
    void Cleanup();
    void TCPSocketConnectionReceived(Windows::Networking::Sockets::StreamSocketListener ^ sender, Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs ^ args);
    void UDPSocketMessageReceived(Windows::Networking::Sockets::DatagramSocket ^ sender, Windows::Networking::Sockets::DatagramSocketMessageReceivedEventArgs ^ e);
    void TCPSocketConnectCompleted(Windows::Foundation::IAsyncAction ^ target, Windows::Foundation::AsyncStatus status);
    void TCPSocketSendComplete(Windows::Foundation::IAsyncOperationWithProgress<uint32, uint32> ^ target, Windows::Foundation::AsyncStatus status);
    void UDPSocketSendComplete(Windows::Foundation::IAsyncOperationWithProgress<uint32, uint32> ^ target, Windows::Foundation::AsyncStatus status);
    void TCPStreamLoadComplete(Windows::Foundation::IAsyncOperation<uint32> ^ target, Windows::Foundation::AsyncStatus status);
    void ConsumeReaderBytes(Platform::WriteOnlyArray<uint8> ^ buf, uint32 len, Platform::WriteOnlyArray<int> ^ received);
    void SetLastError(uint32_t status, bool isFinal = false);
    void SetEvent(Events e);
    void ClearEvent(Events e);
    void SetBindingState(BindingState state);
    int  GetBindingState();
    void ClearBindingState(BindingState state);
    uint32_t COMExceptionToQStatus(uint32_t hresult);

    qcc::Mutex _mutex;
    SocketType _socketType;
    AddressFamily _socketAddrFamily;
    bool _initialized;
    bool _ssl;
    bool _blocking;
    int _backlog;
    Windows::Networking::Sockets::StreamSocketListener ^ _tcpSocketListener;
    Windows::Networking::Sockets::DatagramSocket ^ _udpSocket;
    Windows::Networking::Sockets::StreamSocket ^ _tcpSocket;
    std::list<Windows::Networking::Sockets::StreamSocket ^> _tcpBacklog;
    std::list<UDPMessage ^> _udpBacklog;
    qcc::Semaphore _semAcceptQueue;
    qcc::Semaphore _semReceiveDataQueue;
    Platform::String ^ _lastBindHostname;
    int _lastBindPort;
    Platform::String ^ _lastConnectHostname;
    int _lastConnectPort;
    volatile int32_t _callbackCount;
    int _bindingState;

    Windows::Storage::Streams::DataReader ^ _dataReader;
    std::map<uint32, Windows::Storage::Streams::DataReaderLoadOperation ^> _receiveOperationsMap;
    std::map<uint32, concurrency::task<void> > _receiveTasksMap;
    std::map<uint32, Windows::Foundation::IAsyncOperationWithProgress<uint32, uint32> ^> _sendOperationsMap;
    std::map<uint32, concurrency::task<void> > _sendTasksMap;
    std::map<uint32, Windows::Foundation::IAsyncAction ^> _connectOperationsMap;
    std::map<uint32, concurrency::task<void> > _connectTasksMap;
    int _eventMask;
    int _events;
};

}
}
// SocketWrapper.h
