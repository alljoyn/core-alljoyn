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

#include <ctxtcall.h>
#include <ppltasks.h>
#include <algorithm>

#include <qcc/atomic.h>
#include <qcc/IPAddress.h>
#include <qcc/Debug.h>
#include <qcc/StringUtil.h>

#include <qcc/winrt/utility.h>
#include <qcc/winrt/SocketsWrapper.h>
#include <qcc/winrt/SocketWrapper.h>

#define DEFAULT_READ_SIZE_BYTES 16384

using namespace Windows::Foundation;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

/** @internal */
#define QCC_MODULE "SOCKET_WRAPPER"

namespace qcc {
namespace winrt {

// Statics initialized to the ANY pattern for IPv4 & IPv6
static uint8_t _anyAddrIPV4[4] = { 0 };
static uint8_t _anyAddrIPV6[16] = { 0 };

SocketWrapper::SocketWrapper() : _initialized(false), _blocking(true), _lastBindHostname(nullptr), _backlog(1),
    _lastBindPort(0), _tcpSocketListener(nullptr), _callbackCount(0), _bindingState(BindingState::None), _ssl(false),
    _udpSocket(nullptr), _tcpSocket(nullptr), _dataReader(nullptr), _lastConnectHostname(nullptr), _lastConnectPort(0),
    _events((int) Events::Write), _eventMask((int) Events::All)
{
    LastError = ER_OK;
}

SocketWrapper::~SocketWrapper()
{
    // Forciblely perform cleanup
    Close();
}

Platform::String ^ SocketWrapper::SanitizeAddress(Platform::String ^ hostName)
{
    uint32_t status = ER_FAIL;;
    Platform::String ^ result = hostName;
    while (true) {
        // Check hostName for invalid values
        if (nullptr == hostName) {
            status = ER_OK;
            break;
        }
        // Convert hostName to qcc::String
        qcc::String strHostName = PlatformToMultibyteString(hostName);
        // Check for failed conversion
        if (strHostName.empty() && nullptr != hostName) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Check the address family of the socket
        if (_socketAddrFamily == AddressFamily::QCC_AF_INET) {
            uint8_t addrBuf[4];
            // Convert address to packed IPV4
            status = (uint32_t)(int)qcc::IPAddress::StringToIPv4(strHostName, addrBuf, sizeof(addrBuf));
            if (ER_OK == status) {
                // IPV4 Any is a special case
                if (memcmp(addrBuf, _anyAddrIPV4, sizeof(_anyAddrIPV4)) == 0) {
                    result = nullptr;
                    break;
                }
            }
        } else if (_socketAddrFamily == AddressFamily::QCC_AF_INET6) {
            uint8_t addrBuf[16];
            // Convert address to packet IPV6
            status = (uint32_t)(int)qcc::IPAddress::StringToIPv6(strHostName, addrBuf, sizeof(addrBuf));
            if (ER_OK == status) {
                // IPV6 Any is a special case
                if (memcmp(addrBuf, _anyAddrIPV6, sizeof(_anyAddrIPV6)) == 0) {
                    result = nullptr;
                    break;
                }
            }
        }
        break;
    }
    // Store last error
    SetLastError(status);
    return result;
}

uint32_t SocketWrapper::IsValidAddress(Platform::String ^ hostName)
{
    ::QStatus result = ER_FAIL;
    while (true) {
        // Check if address is IPV4 or IPV6 Any (nullptr and empty string mean the same thing)
        if (nullptr == hostName) {
            result = ER_OK;
            break;
        }
        // Convert hostName to qcc::String
        qcc::String strHostName = PlatformToMultibyteString(hostName);
        // Check for failed conversion
        if (strHostName.empty() && nullptr != hostName) {
            result = ER_OUT_OF_MEMORY;
            break;
        }
        // Create HostName from specified string
        Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(hostName);
        // Check for allocation error
        if (nullptr == hostname) {
            result = ER_OUT_OF_MEMORY;
            break;
        }
        if ((_ssl == true) && (hostname->Type == Windows::Networking::HostNameType::DomainName)) {
            // SSL requires a string hostname to verify the server certificate is valid
            result = ER_OK;
            break;
        } else if (_socketAddrFamily == AddressFamily::QCC_AF_INET) {
            uint8_t addrBuf[4];
            // Convert address to packed IPv4
            result = (::QStatus)qcc::IPAddress::StringToIPv4(strHostName, addrBuf, sizeof(addrBuf));
            break;
        } else if (_socketAddrFamily == AddressFamily::QCC_AF_INET6) {
            uint8_t addrBuf[16];
            // Convert address to packed IPv6
            result = (::QStatus)qcc::IPAddress::StringToIPv6(strHostName, addrBuf, sizeof(addrBuf));
            break;
        }
        break;
    }
    // Store last error
    SetLastError(result);
    return result;
}

void SocketWrapper::Cleanup()
{
    ::QStatus result = ER_OK;
    while (_initialized) {
        // Mark socket as uninitialized
        _initialized = false;
        // Explicitly mark the bound state of the socket as exception
        SetBindingState(BindingState::Exception);
        // Abort any receiver operations which have not been started
        for (std::map<uint32, Windows::Storage::Streams::DataReaderLoadOperation ^>::iterator it = _receiveOperationsMap.begin();
             it != _receiveOperationsMap.end();
             ++it) {
            // Cancel the receive operation
            it->second->Cancel();
        }
        // Clear the load map
        _receiveOperationsMap.clear();
        // Wait for any tasks associated with load operations to complete
        for (std::map<uint32, concurrency::task<void> >::iterator it = _receiveTasksMap.begin();
             it != _receiveTasksMap.end();) {
            // Get the task
            concurrency::task<void> task = it->second;
            uint32 id = it->first;
            // Give up the lock to allow callbacks to continue
            _mutex.Unlock();
            try {
                // Wait for task to complete
                task.wait();
            } catch (...) {
                // Don't except out if task completion throws
            }
            // Re-acquire the lock
            _mutex.Lock();
            // See if task id is still in map
            if (_receiveTasksMap.find(id) != _receiveTasksMap.end()) {
                // Erase it
                _receiveTasksMap.erase(id);
            }
            // Refresh the iterator
            it = _receiveTasksMap.begin();
        }
        // Clear the receive tasks map
        _receiveTasksMap.clear();
        // Iterate the send operations and cancel any which haven't started
        for (std::map<uint32, Windows::Foundation::IAsyncOperationWithProgress<uint32, uint32> ^>::iterator it = _sendOperationsMap.begin();
             it != _sendOperationsMap.end();
             ++it) {
            // Cancel the send operation
            it->second->Cancel();
        }
        // Clear the send operation map
        _sendOperationsMap.clear();
        // Wait for any tasks associated with send operations to complete
        for (std::map<uint32, concurrency::task<void> >::iterator it = _sendTasksMap.begin();
             it != _sendTasksMap.end();) {
            // Get the task
            concurrency::task<void> task = it->second;
            uint32 id = it->first;
            // Give up the lock to allow callbacks to continue
            _mutex.Unlock();
            try {
                // Wait for task to complete
                task.wait();
            } catch (...) {
                // Don't except out if task completion throws
            }
            // Re-acquire the lock
            _mutex.Lock();
            // See if task id is still in map
            if (_sendTasksMap.find(id) != _sendTasksMap.end()) {
                // Erase it
                _sendTasksMap.erase(id);
            }
            // Refresh the iterator
            it = _sendTasksMap.begin();
        }
        // Clear the send tasks map
        _sendTasksMap.clear();
        // Iterate the connect operations map and cancel any which haven't started
        for (std::map<uint32, Windows::Foundation::IAsyncAction ^>::iterator it = _connectOperationsMap.begin();
             it != _connectOperationsMap.end();
             ++it) {
            // Cancel the connect operation
            it->second->Cancel();
        }
        // Clear the conect operations map
        _connectOperationsMap.clear();
        for (std::map<uint32, concurrency::task<void> >::iterator it = _connectTasksMap.begin();
             it != _connectTasksMap.end();) {
            // Get the task
            concurrency::task<void> task = it->second;
            uint32 id = it->first;
            // Give up the lock to allow callbacks to continue
            _mutex.Unlock();
            try {
                // Wait for the task to complete
                task.wait();
            } catch (...) {
            }
            // Re-acquire the lock
            _mutex.Lock();
            // See if task id is still in map
            if (_connectTasksMap.find(id) != _connectTasksMap.end()) {
                // Erase it
                _connectTasksMap.erase(id);
            }
            // Refresh the iterator
            it = _connectTasksMap.begin();
        }
        // Clear the connect task map
        _connectTasksMap.clear();
        qcc::Event waiter;
        // Release lock to allow any callbacks floating to exit
        _mutex.Unlock();
        // Wait for any pending callbacks to flush
        while (_callbackCount != 0) {
            waiter.Wait(waiter, 10);
        }
        // Re-acquire the lock to flush rest of state
        _mutex.Lock();
        // Dispose of the tcp socket
        if (nullptr != _tcpSocket) {
            delete _tcpSocket;
            _tcpSocket = nullptr;
        }
        // Dispose of the tcp socket listener
        if (nullptr != _tcpSocketListener) {
            delete _tcpSocketListener;
            _tcpSocketListener = nullptr;
        }
        // Dispose of the udp socket
        if (nullptr != _udpSocket) {
            delete _udpSocket;
            _udpSocket = nullptr;
        }
        // Dispose of the data reader
        if (nullptr != _dataReader) {
            delete _dataReader;
            _dataReader = nullptr;
        }
        // Cleanup the semaphore for accepted sockets
        _semAcceptQueue.Close();
        // Cleanup the semaphore for udp data
        _semReceiveDataQueue.Close();
        _lastBindHostname = nullptr;
        _lastBindPort = 0;
        _lastConnectHostname = nullptr;
        _lastConnectPort = 0;
        // Clear the backlogs
        _tcpBacklog.clear();
        _udpBacklog.clear();
        break;
    }
    // Store the last error
    SetLastError(result);
}

uint32_t SocketWrapper::Init(StreamSocket ^ socket, DataReader ^ reader, AddressFamily addrFamily)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Call the base Init function to do the bulk intialization
        result = (::QStatus)Init(addrFamily, SocketType::QCC_SOCK_STREAM);
        if (ER_OK != result) {
            break;
        }
        // Mark socket as connected (this init is for connected sockets)
        SetBindingState(BindingState::Connect);
        // Store the reader and socket
        _dataReader = reader;
        _tcpSocket = socket;
        // Queue up traffic
        result = (::QStatus)QueueTraffic();
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Init(AddressFamily addrFamily, SocketType type)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check for previous initialization
        if (!_initialized) {
            // Check addrFamily for invalid values
            if ((int)addrFamily == -1) {
                result = ER_BAD_ARG_1;
                break;
            }
            // Check type for invalid values
            if ((int)type == -1) {
                result = ER_BAD_ARG_2;
                break;
            }
            // Store addrFamily and type
            _socketAddrFamily = addrFamily;
            _socketType = type;
            // Initialize semaphore for accepted sockets
            result = (::QStatus)((int)_semAcceptQueue.Init(0, 0x7FFFFFFF));
            if (ER_OK != result) {
                break;
            }
            // Intialize semaphore for received udp datagrams
            result = (::QStatus)((int)_semReceiveDataQueue.Init(0, 0x7FFFFFFF));
            if (ER_OK != result) {
                break;
            }
            // Enter this socket into the FD map
            qcc::winrt::SocketsWrapper::IncrementFDMap(this);
            // Create default handler for socket events
            SocketWrapperEventsChangedHandler ^ handler = ref new SocketWrapperEventsChangedHandler([ = ] (Platform::Object ^ source, int events) {
                                                                                                    });
            // Check for allocation error
            if (nullptr == handler) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            // Add default handler for socket events
            SocketEventsChanged::add(handler);
            result = ER_OK;
            // Mark final state of init as succesful
            _initialized = true;
        } else {
            result = ER_INIT_FAILED;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::SocketDup(Platform::WriteOnlyArray<SocketWrapper ^> ^ dupSocket)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check for initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Check dupSocket for invalid values
        if (nullptr == dupSocket || dupSocket->Length != 1) {
            result = ER_BAD_ARG_1;
            break;
        }
        // Store the result
        dupSocket[0] = this;
        // Increment the ref count for this socket
        SocketsWrapper::IncrementFDMap(this);
        result = ER_OK;
        break;
    }
    if (ER_OK != result) {
        if (nullptr != dupSocket && dupSocket->Length > 0) {
            // Make sure dupSocket has something sensible for failed operations when appropriate
            dupSocket[0] = nullptr;
        }
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Bind(Platform::String ^ bindName, int localPort)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check for initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        switch (_socketType) {
        case SocketType::QCC_SOCK_STREAM:
            {
                // Check if bind operation is necessary to re-do
                if (nullptr == _tcpSocketListener ||
                    _lastBindHostname != bindName ||
                    _lastBindPort != localPort) {
                    // Validate bind address
                    result = (::QStatus)IsValidAddress(bindName);
                    if (ER_OK != result) {
                        break;
                    }
                    // Store bind information
                    _lastBindHostname = bindName;
                    _lastBindPort = localPort;
                    try {
                        // Sanitize the bind address
                        Platform::String ^ name = SanitizeAddress(bindName);
                        if (nullptr == _tcpSocketListener) {
                            // Create StreamSocketListener
                            _tcpSocketListener = ref new StreamSocketListener();
                            // Check for allocation error
                            if (nullptr == _tcpSocketListener) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            // Create event handler for receiving connections
                            TypedEventHandler<StreamSocketListener ^, StreamSocketListenerConnectionReceivedEventArgs ^> ^ handler = ref new TypedEventHandler<StreamSocketListener ^, StreamSocketListenerConnectionReceivedEventArgs ^>(
                                [ = ] (StreamSocketListener ^ sender, StreamSocketListenerConnectionReceivedEventArgs ^ args) {
                                    TCPSocketConnectionReceived(sender, args);
                                });
                            // Check for allocation error
                            if (nullptr == handler) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            // Add event handler for ConnectionReceived
                            _tcpSocketListener->ConnectionReceived::add(handler);
                        }
                        // Check if bound name is ANY
                        if (nullptr != name) {
                            // Create HostName from string
                            Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(name);
                            // Check for allocation error
                            if (nullptr == hostname) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            // Bind socket with address
                            IAsyncAction ^ op = _tcpSocketListener->BindEndpointAsync(hostname, localPort != 0 ? localPort.ToString() : "");
                            // Check for allocation error
                            if (nullptr == op) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            concurrency::task<void> bindTask(op);
                            // Wait for bind to complete
                            bindTask.wait();
                        } else {
                            // Bind socket with port
                            IAsyncAction ^ op = _tcpSocketListener->BindServiceNameAsync(localPort != 0 ? localPort.ToString() : "");
                            // Check for allocation error
                            if (nullptr == op) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            concurrency::task<void> bindTask(op);
                            // Wait for bind to complete
                            bindTask.wait();
                        }
                        // Store the bind port
                        _lastBindPort = _wtoi(_tcpSocketListener->Information->LocalPort->Data());
                        // Set the bind state
                        SetBindingState(BindingState::Bind);
                        result = ER_OK;
                        break;
                    } catch (Platform::COMException ^ ex) {
                        result = ER_OS_ERROR;
                        SetLastError(COMExceptionToQStatus(ex->HResult));
                        break;
                    } catch (...) {
                        result = ER_OS_ERROR;
                        SetLastError(ER_OS_ERROR);
                        break;
                    }
                }
            }
            break;

        case SocketType::QCC_SOCK_DGRAM:
            {
                // Check if bind operation is necessary to re-do
                if (nullptr == _udpSocket ||
                    _lastBindHostname != bindName ||
                    _lastBindPort != localPort) {
                    // Validate bind address
                    result = (::QStatus)IsValidAddress(bindName);
                    if (ER_OK != result) {
                        break;
                    }
                    // Store bind information
                    _lastBindHostname = bindName;
                    _lastBindPort = localPort;
                    try{
                        // Sanitize the bind address
                        Platform::String ^ name = SanitizeAddress(bindName);
                        if (nullptr == _udpSocket) {
                            // Create DatagramSocket
                            _udpSocket = ref new DatagramSocket();
                            // Check for allocation error
                            if (nullptr == _udpSocket) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            // Create event handler for receiving messages
                            TypedEventHandler<DatagramSocket ^, DatagramSocketMessageReceivedEventArgs ^> ^ handler = ref new TypedEventHandler<DatagramSocket ^, DatagramSocketMessageReceivedEventArgs ^>(
                                [ = ] (DatagramSocket ^ sender, DatagramSocketMessageReceivedEventArgs ^ args) {
                                    UDPSocketMessageReceived(sender, args);
                                });
                            // Check for allocation error
                            if (nullptr == handler) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            // Add handler for MessageReceived
                            _udpSocket->MessageReceived::add(handler);
                        }
                        // Check if bound name is ANY
                        if (nullptr != name) {
                            // Create HostName from string
                            Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(name);
                            // Check for allocation error
                            if (nullptr == hostname) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            // Bind socket with address
                            IAsyncAction ^ op = _udpSocket->BindEndpointAsync(hostname, localPort != 0 ? localPort.ToString() : "");
                            // Check for allocation error
                            if (nullptr == op) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            concurrency::task<void> bindTask(op);
                            // Wait for bind to complete
                            bindTask.wait();
                        } else {
                            // Bind socket with port
                            IAsyncAction ^ op = _udpSocket->BindServiceNameAsync(localPort != 0 ? localPort.ToString() : "");
                            // Check for allocation error
                            if (nullptr == op) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            concurrency::task<void> bindTask(op);
                            // Wait for bind to complete
                            bindTask.wait();
                        }
                        // Store the bind port
                        _lastBindPort = _wtoi(_udpSocket->Information->LocalPort->Data());
                        // Set binding state
                        SetBindingState(BindingState::Bind);
                        result = ER_OK;
                        break;
                    } catch (Platform::COMException ^ ex) {
                        SetLastError(COMExceptionToQStatus(ex->HResult));
                        break;
                    } catch (...) {
                        SetLastError(ER_OS_ERROR);
                        break;
                    }
                }
            }
            break;

        default:
            break;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    if (ER_OK != result) {
        // Wipe down any bind state on failure
        _lastBindHostname = nullptr;
        _lastBindPort = 0;
    }
    // Store last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Listen(int backlog)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check for initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Check for socket type
        if (_socketType != SocketType::QCC_SOCK_STREAM) {
            result = ER_FAIL;
            break;
        }
        // Check for bind
        if ((GetBindingState() & BindingState::Bind) == 0) {
            result = ER_FAIL;
            break;
        }
        // Set binding state
        SetBindingState(BindingState::Listen);
        // Store backlog count
        _backlog = std::min(backlog, MAX_LISTEN_CONNECTIONS);
        _backlog = std::max(_backlog, 1);
        result = ER_OK;
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

// Callback for when udp messages are received through the udp socket
void SocketWrapper::UDPSocketMessageReceived(DatagramSocket ^ sender, DatagramSocketMessageReceivedEventArgs ^ e)
{
    ::QStatus result = ER_OK;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        try {
            // Drain events if in a tear-down only state
            if ((GetBindingState() & BindingState::Exception) != 0) {
                break;
            }
            // Get the reader for the udp message
            DataReader ^ reader = e->GetDataReader();
            // Check for allocation error
            if (nullptr == reader) {
                result = ER_FAIL;
                break;
            }
            // Take ownership of the IBuffer
            IBuffer ^ buffer = reader->DetachBuffer();
            // Check for allocation error
            if (nullptr == buffer) {
                result = ER_FAIL;
                break;
            }
            // Create DataReader
            reader = DataReader::FromBuffer(buffer);
            // Check for allocation error
            if (nullptr == reader) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            // Set default options for data reader
            reader->InputStreamOptions = InputStreamOptions::Partial;
            Platform::String ^ remoteHostname = e->RemoteAddress->RawName;
            int remoteBindPort = _wtoi(e->RemotePort->Data());
            // Create UDPMessage
            UDPMessage ^ msg = ref new UDPMessage(sender, reader, remoteHostname, remoteBindPort);
            // Check for allocation error
            if (nullptr == msg) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            // Push message onto the udp backlog
            _udpBacklog.push_back(msg);
            // Set event for read ready
            SetEvent(Events::Read);
            // Add a resource to the semaphore for waking one
            _semReceiveDataQueue.Release();
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
}

// Callback for when TCP connections arrive
void SocketWrapper::TCPSocketConnectionReceived(StreamSocketListener ^ sender, StreamSocketListenerConnectionReceivedEventArgs ^ args)
{
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        try {
            // Drain events if in a tear-down only state
            if ((GetBindingState() & BindingState::Exception) != 0) {
                break;
            }
            // Get SocketStream from args
            StreamSocket ^ socket = args->Socket;
            // Backlog connection if it fits within the backlog size
            if (socket != nullptr && _tcpBacklog.size() < _backlog) {
                // Push connection onto the tcp backlog
                _tcpBacklog.push_back(socket);
                // Set socket as read ready
                SetEvent(Events::Read);
                // Add a resource to the semaphore to waking one
                _semAcceptQueue.Release();
            } else {
                // reject the connection
                delete socket;
                socket = nullptr;
            }
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
}

uint32_t SocketWrapper::Accept(Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort, Platform::WriteOnlyArray<SocketWrapper ^> ^ newSocket)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check for initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Check remoteAddr for invalid values
        if (nullptr == remoteAddr || remoteAddr->Length != 1) {
            result = ER_BAD_ARG_1;
            break;
        }
        // Check remotePort for invalid values
        if (nullptr == remotePort || remotePort->Length != 1) {
            result = ER_BAD_ARG_2;
            break;
        }
        // Check newSocket for invalid values
        if (nullptr == newSocket || newSocket->Length != 1) {
            result = ER_BAD_ARG_3;
            break;
        }
        // Check socket type
        if (_socketType != SocketType::QCC_SOCK_STREAM) {
            result = ER_FAIL;
            break;
        }
        // Check binding state for listen
        if ((GetBindingState() & (int)BindingState::Listen) == 0) {
            result = ER_FAIL;
            break;
        }
        // Check if socket is blocking
        if (_blocking) {
            StreamSocket ^ s = nullptr;
            while (nullptr == s && (GetBindingState() & BindingState::Exception) == 0) {
                // Release the API lock
                _mutex.Unlock();
                // Wait for a socket to connect
                result = (::QStatus)((int)_semAcceptQueue.Wait());
                // Re-acquire the API lock
                _mutex.Lock();
                if (ER_OK != result) {
                    break;
                }
                // Grab the new socket from the backlog
                s = _tcpBacklog.front();
                _tcpBacklog.pop_front();
                if (0 == _tcpBacklog.size()) {
                    // Reset read ready state if nothing left to read
                    ClearEvent(Events::Read);
                }
                // Fail operations on sockets in an exception state
                if ((GetBindingState() & BindingState::Exception) != 0) {
                    result = ER_FAIL;
                    break;
                }
            }
            if (ER_OK != result) {
                break;
            }
            if (nullptr != s) {
                // Create DataReader
                DataReader ^ reader = ref new DataReader(s->InputStream);
                // Check for allocation error
                if (nullptr == reader) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                // Set reader default options
                reader->InputStreamOptions = InputStreamOptions::Partial;
                // Create SocketWrapper for new connection
                SocketWrapper ^ tempSocket = ref new SocketWrapper();
                // Check for allocation error
                if (nullptr == tempSocket) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                // Initialize SoketWrapper
                result = (::QStatus)tempSocket->Init(s, reader, _socketAddrFamily);
                if (ER_OK != result) {
                    break;
                }
                // Set the output parameters
                remoteAddr[0] = s->Information->LocalAddress->RawName;
                remotePort[0] = _wtoi(s->Information->LocalPort->Data());
                newSocket[0] = tempSocket;
                result = ER_OK;
                break;
            }
        }  else {
            // Check if tcp socket is ready to be read
            if (_tcpBacklog.size() > 0) {
                // Acquire the resource
                result = (::QStatus)((int)_semAcceptQueue.Wait());
                if (ER_OK != result) {
                    break;
                }
                // Grab the new socket from the backlog
                StreamSocket ^ s = nullptr;
                s = _tcpBacklog.front();
                _tcpBacklog.pop_front();
                if (0 == _tcpBacklog.size()) {
                    // Reset read ready state if nothing left to read
                    ClearEvent(Events::Read);
                }
                // Fail operations on sockets in an exception state
                if ((GetBindingState() & BindingState::Exception) != 0) {
                    result = ER_FAIL;
                    break;
                }
                if (nullptr != s) {
                    // Create DataReader
                    DataReader ^ reader = ref new DataReader(s->InputStream);
                    // Check for allocation error
                    if (nullptr == reader) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Set reader default options
                    reader->InputStreamOptions = InputStreamOptions::Partial;
                    // Create SocketWrapper
                    SocketWrapper ^ tempSocket = ref new SocketWrapper();
                    // Check for allocation error
                    if (nullptr == tempSocket) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Intialize SocketWrapper
                    result = (::QStatus)tempSocket->Init(s, reader, _socketAddrFamily);
                    if (ER_OK != result) {
                        break;
                    }
                    // Set the output parameters
                    tempSocket->SetBindingState(BindingState::Connect);
                    remoteAddr[0] = s->Information->LocalAddress->RawName;
                    remotePort[0] = _wtoi(s->Information->LocalPort->Data());
                    newSocket[0] = tempSocket;
                    result = ER_OK;
                    break;
                }
            }
            result = ER_WOULDBLOCK;
            break;
        }
        break;
    }
    if (ER_OK != result) {
        // Set reasonable values for failure
        if (nullptr != remoteAddr && remoteAddr->Length > 0) {
            remoteAddr[0] = nullptr;
        }
        if (nullptr != remotePort && remotePort->Length > 0) {
            remotePort[0] = 0;
        }
        if (nullptr != newSocket && newSocket->Length > 0) {
            newSocket[0] = nullptr;
        }
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::SetBlocking(bool blocking)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Store the result
        _blocking = blocking;
        result = ER_OK;
        break;
    }
    // Releas the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::SetNagle(bool useNagle)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Check socket type
        if (_socketType != SocketType::QCC_SOCK_STREAM) {
            result = ER_FAIL;
            break;
        }
        try {
            if (nullptr == _tcpSocket) {
                // Create StreamSocket
                _tcpSocket = ref new StreamSocket();
                // Check for allocation error
                if (nullptr == _tcpSocket) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
            }
            // Store the result
            _tcpSocket->Control->NoDelay = !useNagle;
            result = ER_OK;
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult));
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR);
            break;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

// Callback for when TCP connect (client side) completes
void SocketWrapper::TCPSocketConnectCompleted(IAsyncAction ^ target, AsyncStatus status)
{
    ::QStatus result = ER_OK;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        try {
            // Remove connect operation from map if exists
            if (_connectOperationsMap.find(target->Id) != _connectOperationsMap.end()) {
                // Remove the operation
                _connectOperationsMap.erase(target->Id);
            }
            // Remove connect task from map if exists
            if (_connectTasksMap.find(target->Id) != _connectTasksMap.end()) {
                // Remove the task
                _connectTasksMap.erase(target->Id);
            }
            // Drain events if in a tear-down only state
            if ((GetBindingState() & BindingState::Exception) != 0) {
                break;
            }
            // Verify target successful completed without error
            target->GetResults();
            // Create DataReader
            _dataReader = ref new DataReader(_tcpSocket->InputStream);
            // Check for allocation error
            if (nullptr == _tcpSocket) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            // Set reader default options
            _dataReader->InputStreamOptions = InputStreamOptions::Partial;
            // Set binding state
            SetBindingState(BindingState::Connect);
            // Queue up the traffic
            QueueTraffic();
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
}

uint32_t SocketWrapper::Connect(Platform::String ^ remoteAddr, int remotePort)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check intialized
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Check socket type
        if (_socketType != SocketType::QCC_SOCK_STREAM) {
            result = ER_FAIL;
            break;
        }
        // Check remote information to see if there's a need to validate
        if (_lastConnectHostname != remoteAddr ||
            _lastConnectPort != remotePort) {
            result = (::QStatus)IsValidAddress(remoteAddr);
            if (ER_OK != result) {
                break;
            }
            // Store the result
            _lastConnectHostname = remoteAddr;
            _lastConnectPort = remotePort;
        }
        // Check if socket is blocking
        if (_blocking) {
            if (nullptr == _tcpSocket) {
                // Create StreamSocket
                _tcpSocket = ref new StreamSocket();
                // Check for allocation error
                if (nullptr == _tcpSocket) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
            }
            // Ignore multiple connect requests
            if (0 == _connectOperationsMap.size()) {
                try {
                    // Create HostName from string
                    Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(remoteAddr);
                    // Check for allocation error
                    if (nullptr == hostname) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Connect the socket
                    Windows::Foundation::IAsyncAction ^ op = _tcpSocket->ConnectAsync(hostname, remotePort.ToString(), _ssl ? SocketProtectionLevel::Ssl : SocketProtectionLevel::PlainSocket);
                    // Check for allocation error
                    if (nullptr == op) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Store the task and operation associated with task
                    concurrency::task<void> connectTask(op);
                    _connectOperationsMap[op->Id] = op;
                    _connectTasksMap[op->Id] = connectTask.then([ = ] () {
                                                                    TCPSocketConnectCompleted(op, op->Status);
                                                                });
                    concurrency::task<void> connectTask2 = _connectTasksMap[op->Id];
                    // Release the API lock
                    _mutex.Unlock();
                    try {
                        // Wait for connect to complete
                        connectTask2.wait();
                        // Re-acquire the API lock
                        _mutex.Lock();
                        result = ER_OK;
                        break;
                    } catch (Platform::COMException ^ ex) {
                        // Re-acquire the API lock
                        _mutex.Lock();
                        SetLastError(COMExceptionToQStatus(ex->HResult));
                        break;
                    } catch (...) {
                        // Re-acquire the API lock
                        _mutex.Lock();
                        SetLastError(ER_OS_ERROR);
                        break;
                    }
                    break;
                } catch (Platform::COMException ^ ex) {
                    SetLastError(COMExceptionToQStatus(ex->HResult));
                    break;
                } catch (...) {
                    SetLastError(ER_OS_ERROR);
                    break;
                }
            }
        } else {
            // Check bind state for connected
            if ((GetBindingState() & BindingState::Connect) != 0) {
                result = ER_OK;
                break;
            }
            // Create StreamSocket
            if (nullptr == _tcpSocket) {
                _tcpSocket = ref new StreamSocket();
                if (nullptr == _tcpSocket) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
            }
            // Ignore multiple connect requests
            if (0 == _connectOperationsMap.size()) {
                try{
                    // Create HostName from string
                    Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(remoteAddr);
                    // Check for allocation error
                    if (nullptr == hostname) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Connect the socket
                    Windows::Foundation::IAsyncAction ^ op = _tcpSocket->ConnectAsync(hostname, remotePort.ToString(), _ssl ? SocketProtectionLevel::Ssl : SocketProtectionLevel::PlainSocket);
                    // Check for allocation error
                    if (nullptr == op) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Store the task and operation associated with task
                    concurrency::task<void> connectTask(op);
                    _connectOperationsMap[op->Id] = op;
                    _connectTasksMap[op->Id] = connectTask.then([ = ] () {
                                                                    TCPSocketConnectCompleted(op, op->Status);
                                                                });
                    break;
                } catch (Platform::COMException ^ ex) {
                    SetLastError(COMExceptionToQStatus(ex->HResult));
                    break;
                } catch (...) {
                    SetLastError(ER_OS_ERROR);
                    break;
                }
            }
            result = ER_WOULDBLOCK;
            break;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    if (ER_OK != result) {
        // Store reasonable values for API failure
        _lastConnectHostname = nullptr;
        _lastConnectPort = 0;
    }
    // Store the last error
    SetLastError(result);
    return result;
}

// Callback is received when a tcp send operation completes
void SocketWrapper::TCPSocketSendComplete(IAsyncOperationWithProgress<uint32, uint32> ^ target, AsyncStatus status)
{
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        try {
            // Check if operation is still in map
            if (_sendOperationsMap.find(target->Id) != _sendOperationsMap.end()) {
                // Remove it
                _sendOperationsMap.erase(target->Id);
            }
            // Check if task is still in map
            if (_sendTasksMap.find(target->Id) != _sendTasksMap.end()) {
                // Remove it
                _sendTasksMap.erase(target->Id);
            }
            // Drain events if in a tear-down only state
            if ((GetBindingState() & BindingState::Exception) != 0) {
                break;
            }
            int operationCount = _sendOperationsMap.size();
            if (0 == target->GetResults()) {
                // This is a funny way that the tcp stream indicates the other end has closed
                SetLastError(ER_SOCK_OTHER_END_CLOSED, true);
                // Set socket in exception state
                SetEvent(Events::Exception);
            } else if (operationCount == 0) {
                // Set socket state to write ready
                SetEvent(Events::Write);
            }
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
}

// Callback is received when a udp send completes
void SocketWrapper::UDPSocketSendComplete(IAsyncOperationWithProgress<uint32, uint32> ^ target, AsyncStatus status)
{
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        try {
            // Check if operation is still in map
            if (_sendOperationsMap.find(target->Id) != _sendOperationsMap.end()) {
                // Remove it
                _sendOperationsMap.erase(target->Id);
            }
            // Check if task is still in map
            if (_sendTasksMap.find(target->Id) != _sendTasksMap.end()) {
                // Remove it
                _sendTasksMap.erase(target->Id);
            }
            // Drain events if in a tear-down only state
            if ((GetBindingState() & BindingState::Exception) != 0) {
                break;
            }
            int operationCount = _sendOperationsMap.size();
            // Check for errors in completion
            target->GetResults();
            if (operationCount == 0) {
                // Set state to write ready
                SetEvent(Events::Write);
            }
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
}

uint32_t SocketWrapper::SendTo(Platform::String ^ remoteAddr, int remotePort,
                               const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    DatagramSocket ^ sender = nullptr;
    while (true) {
        // Check initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Check sent for invalid values
        if (nullptr == sent || sent->Length != 1) {
            result = ER_BAD_ARG_5;
            break;
        }
        // Verify remoteAddr is valid address
        result = (::QStatus)IsValidAddress(remoteAddr);
        if (ER_OK != result) {
            break;
        }
        switch (_socketType) {
        case SocketType::QCC_SOCK_STREAM:
            // Send the data
            result = (::QStatus)Send(buf, len, sent);
            break;

        case SocketType::QCC_SOCK_DGRAM:
            {
                // Create DataWriter
                DataWriter ^ dw = ref new DataWriter();
                // Check for allocation error
                if (nullptr == dw) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                // Store bytes in buf array
                dw->WriteBytes(buf);
                // Take ownership of the IBuffer
                IBuffer ^ buffer = dw->DetachBuffer();
                // Check for allocation error
                if (nullptr == buffer) {
                    result = ER_FAIL;
                    break;
                }
                // Don't create DatagramSocket if already bound
                if (_udpSocket != nullptr) {
                    sender = _udpSocket;
                } else {
                    // Create DatagramSocket
                    sender = ref new DatagramSocket();
                    if (nullptr == sender) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                }
                IOutputStream ^ writeStream = nullptr;
                try {
                    // Create HostName from string
                    Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(remoteAddr);
                    // Check for allocation error
                    if (nullptr == hostname) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Convert port to string
                    Platform::String ^ strRemotePort = remotePort.ToString();
                    // Check for allocation error
                    if (nullptr == strRemotePort) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Get stream associated with the udp socket
                    IAsyncOperation<IOutputStream ^> ^ getStreamOp = sender->GetOutputStreamAsync(hostname, strRemotePort);
                    // Check for allocation error
                    if (nullptr == getStreamOp) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    concurrency::task<IOutputStream ^> getStreamTask(getStreamOp);
                    // Wait for task to complete
                    getStreamTask.wait();
                    // Get the output stream
                    writeStream = getStreamTask.get();
                    // Check for allocation error
                    if (nullptr == writeStream) {
                        result = ER_OS_ERROR;
                        break;
                    }
                } catch (Platform::COMException ^ ex) {
                    SetLastError(COMExceptionToQStatus(ex->HResult), true);
                    SetEvent(Events::Exception);
                    break;
                } catch (...) {
                    SetLastError(ER_OS_ERROR, true);
                    SetEvent(Events::Exception);
                    break;
                }
                // Check if socket is blocking
                if (_blocking) {
                    try {
                        // Write the bytes
                        IAsyncOperationWithProgress<uint32, uint32>  ^ op = writeStream->WriteAsync(buffer);
                        // Check for allocation error
                        if (nullptr == op) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        // Store the task and operation associated with task
                        concurrency::task<uint32> sendTask(op);
                        _sendOperationsMap[op->Id] = op;
                        ClearEvent(Events::Write);
                        _sendTasksMap[op->Id] = sendTask.then([ = ] (uint32 progress) {
                                                                  UDPSocketSendComplete(op, op->Status);
                                                              });
                        concurrency::task<void> sendTask2 = _sendTasksMap[op->Id];
                        // Release the API lock
                        _mutex.Unlock();
                        try {
                            // Wait for send task to complete
                            sendTask2.wait();
                            // Get the result of the send
                            sent[0] = sendTask.get();
                            // Re-acquire the API lock
                            _mutex.Lock();
                            result = ER_OK;
                            break;
                        } catch (Platform::COMException ^ ex) {
                            _mutex.Lock();
                            SetLastError(COMExceptionToQStatus(ex->HResult), true);
                            SetEvent(Events::Exception);
                            break;
                        } catch (...) {
                            _mutex.Lock();
                            SetLastError(ER_OS_ERROR, true);
                            SetEvent(Events::Exception);
                            break;
                        }
                        break;
                    } catch (Platform::COMException ^ ex) {
                        SetLastError(COMExceptionToQStatus(ex->HResult), true);
                        SetEvent(Events::Exception);
                        break;
                    } catch (...) {
                        SetLastError(ER_OS_ERROR, true);
                        SetEvent(Events::Exception);
                        break;
                    }
                    break;
                } else {
                    try{
                        // Only one send operation is allow at any time
                        if (_sendOperationsMap.size() == 0) {
                            // Write the bytes
                            IAsyncOperationWithProgress<uint32, uint32> ^ op = writeStream->WriteAsync(buffer);
                            // Check for allocation error
                            if (nullptr == op) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            // Store the task and operation associated with task
                            concurrency::task<uint32> sendTask(op);
                            _sendOperationsMap[op->Id] = op;
                            ClearEvent(Events::Write);
                            _sendTasksMap[op->Id] = sendTask.then([ = ] (uint32 progress) {
                                                                      UDPSocketSendComplete(op, op->Status);
                                                                  });
                            sent[0] = len;
                            result = ER_OK;
                            break;
                        } else {
                            // Operation already in progress
                            sent[0] = 0;
                            result = ER_WOULDBLOCK;
                            break;
                        }
                        break;
                    } catch (Platform::COMException ^ ex) {
                        SetLastError(COMExceptionToQStatus(ex->HResult), true);
                        SetEvent(Events::Exception);
                        break;
                    } catch (...) {
                        SetLastError(ER_OS_ERROR, true);
                        SetEvent(Events::Exception);
                        break;
                    }
                }
            }

        default:
            break;
        }
        break;
    }
    if (ER_OK != result &&
        ER_WOULDBLOCK != result) {
        // Set reasonable output values on error
        if (nullptr != sent && sent->Length > 0) {
            sent[0] = 0;
        }
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

void SocketWrapper::ConsumeReaderBytes(Platform::WriteOnlyArray<uint8> ^ buf, uint32 len, Platform::WriteOnlyArray<int> ^ received)
{
    // Inspect the dataReader for what's available to read. We only consume what's been requested
    uint32 readCount = std::min(_dataReader->UnconsumedBufferLength, len);
    received[0] = readCount;
    if (readCount > 0) {
        // Fill buffer if readCount and length happen to be the same
        if (readCount == len) {
            // Pull bytes from reader
            _dataReader->ReadBytes(buf);
        } else {
            // Create array sized for the read operation
            Platform::Array<uint8> ^ tempBuffer = ref new Platform::Array<uint8>(readCount);
            // Pull bytes from reader
            _dataReader->ReadBytes(tempBuffer);
            // Store the result
            memcpy(buf->Data, tempBuffer->Data, readCount);
        }
    }
    // Check if all bytes have been received
    if (_dataReader->UnconsumedBufferLength == 0) {
        if (_socketType == SocketType::QCC_SOCK_STREAM) {
            // Socket is no longer read ready
            ClearEvent(Events::Read);
        } else {
            // udp
            if (_udpBacklog.size() == 0) {
                // Socket is no longer read ready
                ClearEvent(Events::Read);
            }
        }
    }
}

uint32_t SocketWrapper::RecvFrom(Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort,
                                 Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Check remoteAddr for invalid values
        if (nullptr == remoteAddr || remoteAddr->Length != 1) {
            result = ER_BAD_ARG_1;
            break;
        }
        // Check remotePort for invalid values
        if (nullptr == remotePort || remotePort->Length != 1) {
            result = ER_BAD_ARG_2;
            break;
        }
        // Check buf for invalid values
        if (nullptr == buf || buf->Length < 1) {
            result = ER_BAD_ARG_3;
            break;
        }
        // Check received for invalid values
        if (nullptr == received || received->Length != 1) {
            result = ER_BAD_ARG_5;
            break;
        }
        switch (_socketType) {
        case SocketType::QCC_SOCK_STREAM:
            // Call Recv for the socket stream
            result = (::QStatus)Recv(buf, len, received);
            // Set the output parameters
            remoteAddr[0] = _lastConnectHostname;
            remotePort[0] = _lastConnectPort;
            break;

        case SocketType::QCC_SOCK_DGRAM:
            {
                // Check udp socket for bind
                if ((GetBindingState() & (int)BindingState::Bind) == 0) {
                    result = ER_FAIL;
                    break;
                }
                // Check if data is available for read
                if (nullptr != _dataReader && _dataReader->UnconsumedBufferLength > 0) {
                    // Pull bytes
                    ConsumeReaderBytes(buf, len, received);
                    result = ER_OK;
                    break;
                } else if (nullptr == _dataReader || _dataReader->UnconsumedBufferLength == 0) {
                    // Check if socket is blocking
                    if (_blocking) {
                        bool haveData = false;
                        UDPMessage ^ m = nullptr;
                        while (!haveData) {
                            // Release the API lock
                            _mutex.Unlock();
                            // Wait for data to arrive
                            result = (::QStatus)((int)_semReceiveDataQueue.Wait());
                            // Re-acquire the API lock
                            _mutex.Lock();
                            if (ER_OK != result) {
                                break;
                            }
                            // Grab a message from the backlog
                            m = _udpBacklog.front();
                            _udpBacklog.pop_front();
                            if (nullptr != m) {
                                haveData = true;
                            }
                            // Fail operations on sockets in an exception state
                            if ((GetBindingState() & (int)BindingState::Exception) != 0) {
                                result = ER_FAIL;
                                break;
                            }
                        }
                        if (ER_OK != result) {
                            break;
                        }
                        // Update the current reader
                        _dataReader = m->Reader;
                        // Pull bytes
                        ConsumeReaderBytes(buf, len, received);
                        // Set output parameters
                        remoteAddr[0] = m->RemoteHostname;
                        remotePort[0] = m->RemotePort;
                        result = ER_OK;
                        break;
                    } else {
                        // See if there's a message and buffer immediately available to consume
                        if (_udpBacklog.size() > 0) {
                            // update the resource count
                            result = (::QStatus)((int)_semReceiveDataQueue.Wait());
                            if (ER_OK != result) {
                                break;
                            }
                            UDPMessage ^ m;
                            // Grab a message from the backlog
                            m = _udpBacklog.front();
                            _udpBacklog.pop_front();
                            if (nullptr != m) {
                                // Update the current reader
                                _dataReader = m->Reader;
                                // Pull bytes
                                ConsumeReaderBytes(buf, len, received);
                                // Set output parameters
                                remoteAddr[0] = m->RemoteHostname;
                                remotePort[0] = m->RemotePort;
                                result = ER_OK;
                                break;
                            }
                        }
                        // not blocking, no data
                        received[0] = 0;
                        result = ER_WOULDBLOCK;
                        break;
                    }
                }
            }
            break;

        default:
            break;
        }
        break;
    }
    if (ER_OK != result &&
        ER_WOULDBLOCK != result) {
        if (nullptr != received && received->Length > 0) {
            received[0] = 0;
        }
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Send(const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent)
{
    ::QStatus result = ER_FAIL;
    // Get the API lock
    _mutex.Lock();
    while (true) {
        // Check initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Check sent for invalid values
        if (nullptr == sent || sent->Length != 1) {
            result = ER_BAD_ARG_3;
            break;
        }
        switch (_socketType) {
        case SocketType::QCC_SOCK_STREAM:
            {
                // Check for connected state
                if ((GetBindingState() & BindingState::Connect) != 0) {
                    // Create DataWriter
                    DataWriter ^ dw = ref new DataWriter();
                    // Check for allocation error
                    if (nullptr == dw) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Write the bytes
                    dw->WriteBytes(buf);
                    // Take ownership of the buffer
                    IBuffer ^ buffer = dw->DetachBuffer();
                    // Check for allocation error
                    if (nullptr == buffer) {
                        result = ER_FAIL;
                        break;
                    }
                    // Check if socket is blocking
                    if (_blocking) {
                        try {
                            // Write the bytes
                            IAsyncOperationWithProgress<uint32, uint32>  ^ op = _tcpSocket->OutputStream->WriteAsync(buffer);
                            // Check for allocation error
                            if (nullptr == op) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            // Store the operation and task associated with the operation
                            concurrency::task<uint32> sendTask(op);
                            _sendOperationsMap[op->Id] = op;
                            ClearEvent(Events::Write);
                            _sendTasksMap[op->Id] = sendTask.then([ = ] (uint32 progress) {
                                                                      TCPSocketSendComplete(op, op->Status);
                                                                  });
                            concurrency::task<void> sendTask2 = _sendTasksMap[op->Id];
                            // Release the API lock
                            _mutex.Unlock();
                            try {
                                // Wait for send task to complete
                                sendTask2.wait();
                                // Get the send result
                                sent[0] = sendTask.get();
                                // Re-acquire the API lock
                                _mutex.Lock();
                                result = ER_OK;
                                break;
                            } catch (Platform::COMException ^ ex) {
                                // Re-acquire the API lock
                                _mutex.Lock();
                                SetLastError(COMExceptionToQStatus(ex->HResult), true);
                                SetEvent(Events::Exception);
                                break;
                            } catch (...) {
                                // Re-acquire the API lock
                                _mutex.Lock();
                                SetLastError(ER_OS_ERROR, true);
                                SetEvent(Events::Exception);
                                break;
                            }
                            break;
                        } catch (Platform::COMException ^ ex) {
                            SetLastError(COMExceptionToQStatus(ex->HResult), true);
                            SetEvent(Events::Exception);
                            break;
                        } catch (...) {
                            SetLastError(ER_OS_ERROR, true);
                            SetEvent(Events::Exception);
                            break;
                        }
                    } else {
                        try{
                            // Only one send operation is allowed at once
                            if (_sendOperationsMap.size() == 0) {
                                // Send the bytes
                                IAsyncOperationWithProgress<uint32, uint32> ^ op = _tcpSocket->OutputStream->WriteAsync(buffer);
                                // Check for allocation error
                                if (nullptr == op) {
                                    result = ER_OUT_OF_MEMORY;
                                    break;
                                }
                                // Store the operation and the task associated with the operation
                                concurrency::task<uint32> sendTask(op);
                                _sendOperationsMap[op->Id] = op;
                                ClearEvent(Events::Write);
                                _sendTasksMap[op->Id] = sendTask.then([ = ] (uint32 progress) {
                                                                          TCPSocketSendComplete(op, op->Status);
                                                                      });
                                // not blocking, wait for data to say it completed
                                sent[0] = len;
                                result = ER_OK;
                                break;
                            } else {
                                sent[0] = 0;
                                result = ER_WOULDBLOCK;
                                break;
                            }
                            break;
                        } catch (Platform::COMException ^ ex) {
                            SetLastError(COMExceptionToQStatus(ex->HResult), true);
                            SetEvent(Events::Exception);
                            break;
                        } catch (...) {
                            SetLastError(ER_OS_ERROR, true);
                            SetEvent(Events::Exception);
                            break;
                        }
                    }
                } else {
                    // Not connected
                    result = ER_FAIL;
                    break;
                }
                break;
            }

        case SocketType::QCC_SOCK_DGRAM:
            {
                // Check for bind
                if ((GetBindingState() & BindingState::Bind) != 0) {
                    // Send the data
                    result = (::QStatus)SendTo(_lastBindHostname, _lastBindPort, buf, len, sent);
                    break;
                }
                // Must have bind
                result = ER_FAIL;
                break;
            }

        default:
            break;
        }
        break;
    }
    if (ER_OK != result &&
        ER_WOULDBLOCK != result) {
        // For failure, set the output parameter to reasonable values
        if (nullptr != sent && sent->Length > 0) {
            sent[0] = 0;
        }
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

// Callback for tcp stream load operations
void SocketWrapper::TCPStreamLoadComplete(IAsyncOperation<uint32> ^ target, AsyncStatus status)
{
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        try {
            // Check if operation is in the map
            if (_receiveOperationsMap.find(target->Id) != _receiveOperationsMap.end()) {
                // Remove it
                _receiveOperationsMap.erase(target->Id);
            }
            // Check if task is in the map
            if (_receiveTasksMap.find(target->Id) != _receiveTasksMap.end()) {
                // Remove it
                _receiveTasksMap.erase(target->Id);
            }
            // Drain events if in a tear-down only state
            if ((GetBindingState() & BindingState::Exception) != 0) {
                break;
            }
            if (0 == target->GetResults()) {
                // This is a funny way that the tcp stream indicates the other end has closed
                SetLastError(ER_SOCK_OTHER_END_CLOSED, true);
                // Set the socket to an exception state
                SetEvent(Events::Exception);
            } else {
                // Set socket read ready
                SetEvent(Events::Read);
            }
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
}

uint32_t SocketWrapper::QueueTraffic()
{
    ::QStatus result = ER_FAIL;
    while (true) {
        // (tcp) If there's no data waiting, and no pending requests to get data, request data
        // (udp) This is already handled with the udp message receiver
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Drain events if in a tear-down only state
            break;
        }
        if (_socketType == SocketType::QCC_SOCK_STREAM &&
            (GetBindingState() & BindingState::Connect) != 0 &&
            _dataReader->UnconsumedBufferLength == 0 &&
            0 == _receiveOperationsMap.size()) {
            // Start a load operation
            Windows::Storage::Streams::DataReaderLoadOperation ^ loadOp = _dataReader->LoadAsync(DEFAULT_READ_SIZE_BYTES);
            // Check for allocation error
            if (nullptr == loadOp) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            // Store the operation and the task associated with the operation
            concurrency::task<uint32> rTask(loadOp);
            _receiveOperationsMap[loadOp->Id] = loadOp;
            _receiveTasksMap[loadOp->Id] = rTask.then([ = ] (uint32 progress) {
                                                          TCPStreamLoadComplete(loadOp, loadOp->Status);
                                                      });
            result = ER_OK;
            break;
        } else {
            result = ER_OK;
            break;
        }
        break;
    }
    // Drain events if in a tear-down only state
    if ((GetBindingState() & (int)BindingState::Exception) != 0) {
        result = ER_FAIL;
    }
    // Store the last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Recv(Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Check buf for invalid values
        if (nullptr == buf || buf->Length < 1) {
            result = ER_BAD_ARG_1;
            break;
        }
        // Check received for invalid values
        if (nullptr == received || received->Length != 1) {
            result = ER_BAD_ARG_3;
            break;
        }
        switch (_socketType) {
        case SocketType::QCC_SOCK_STREAM:
            {
                // Check socket for connected state
                if ((GetBindingState() & BindingState::Connect) != 0) {
                    // Check if data is ready to read
                    if (_dataReader->UnconsumedBufferLength == 0) {
                        // Check for blocking
                        if (_blocking) {
                            bool haveData = false;
                            while (!haveData) {
                                // Queue up the traffic
                                result = (::QStatus)QueueTraffic();
                                if (ER_OK != result) {
                                    break;
                                }
                                // Store operation and task associated with operation
                                concurrency::task<void> rTask = _receiveTasksMap.begin()->second;
                                uint32 id = _receiveOperationsMap.begin()->first;
                                // Release the API lock
                                _mutex.Unlock();
                                result = ER_FAIL;
                                try {
                                    // Wait for recv task to complete
                                    rTask.wait();
                                    // Re-acquire the API lock
                                    _mutex.Lock();
                                    if (_dataReader->UnconsumedBufferLength != 0) {
                                        haveData = true;
                                        result = ER_OK;
                                    }
                                    // Fail operations on sockets in an exception state
                                    if ((GetBindingState() & (int)BindingState::Exception) != 0) {
                                        result = ER_FAIL;
                                        break;
                                    }
                                } catch (Platform::COMException ^ ex) {
                                    // Re-acquire the API lock
                                    _mutex.Lock();
                                    SetLastError(COMExceptionToQStatus(ex->HResult), true);
                                    SetEvent(Events::Exception);
                                    break;
                                } catch (...) {
                                    // Re-acquire the API lock
                                    _mutex.Lock();
                                    SetLastError(ER_OS_ERROR, true);
                                    SetEvent(Events::Exception);
                                    break;
                                }
                            }
                            if (ER_OK != result) {
                                break;
                            }
                            // Pull bytes
                            ConsumeReaderBytes(buf, len, received);
                            // If we just flushed the reader, try and queue
                            if (_dataReader->UnconsumedBufferLength == 0) {
                                // Queue up the traffic
                                result = (::QStatus)QueueTraffic();
                            } else {
                                result = ER_OK;
                            }
                            break;
                        } else {
                            // Queue up the traffic
                            result = (::QStatus)QueueTraffic();
                            if (ER_OK != result) {
                                break;
                            }
                            // Set the output parameters
                            received[0] = 0;
                            result = ER_WOULDBLOCK;
                            break;
                        }
                    } else {
                        // Pull bytes
                        ConsumeReaderBytes(buf, len, received);
                        // If we just flushed the reader, try and queue
                        if (_dataReader->UnconsumedBufferLength == 0) {
                            // Queue up the traffic
                            result = (::QStatus)QueueTraffic();
                        } else {
                            result = ER_OK;
                        }
                        break;
                    }
                }
                // Not connected
                result = ER_FAIL;
                break;
            }

        case SocketType::QCC_SOCK_DGRAM:
            {
                // Create string array to store the remote address
                Platform::Array<Platform::String ^> ^ remoteAddr = ref new Platform::Array<Platform::String ^>(1);
                // Check for allocation error
                if (nullptr == remoteAddr) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                // Create int array to store the remote port
                Platform::Array<int> ^ remotePort = ref new Platform::Array<int>(1);
                // Check for allocation error
                if (nullptr == remotePort) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                // Receive data
                result = (::QStatus)RecvFrom(remoteAddr, remotePort, buf, len, received);
                break;
            }

        default:
            break;
        }
        break;
    }
    if (ER_OK != result &&
        ER_WOULDBLOCK != result) {
        // Set reasonable output parameters on failure
        if (nullptr != received && received->Length > 0) {
            received[0] = 0;
        }
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::GetLocalAddress(Platform::WriteOnlyArray<Platform::String ^> ^ addr, Platform::WriteOnlyArray<int> ^ port)
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Check binding state for either bind or connect
        if ((GetBindingState() & BindingState::Bind) == 0 && (GetBindingState() & BindingState::Connect) == 0) {
            result = ER_FAIL;
            break;
        }
        // Check addr for invalid values
        if (nullptr == addr || addr->Length != 1) {
            result = ER_BAD_ARG_1;
            break;
        }
        // Check port for invalid values
        if (nullptr == port || port->Length != 1) {
            result = ER_BAD_ARG_2;
            break;
        }
        // For connected sockets, return connect information and for bound sockets return bind information
        if ((GetBindingState() & BindingState::Bind) == 0  &&
            (GetBindingState() & BindingState::Connect) != 0 &&
            _socketType == SocketType::QCC_SOCK_STREAM) {
            // Set the output parameters
            addr[0] = _lastConnectHostname;
            port[0] = _lastConnectPort;
        } else {
            // Set the output parameters
            addr[0] = _lastBindHostname;
            port[0] = _lastBindPort;
        }
        result = ER_OK;
        break;
    }
    if (ER_OK != result) {
        // Set reasonable output parameters on failure
        if (nullptr != addr && addr->Length > 0) {
            addr[0] = nullptr;
        }
        if (nullptr != port && port->Length > 0) {
            port[0] = 0;
        }
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Close()
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Check initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Decrement and check fd map count
        if (0 == qcc::winrt::SocketsWrapper::DecrementFDMap(this)) {
            try {
                // Set last error to tear down
                SetLastError(ER_SOCK_OTHER_END_CLOSED, true);
                // Cleanup
                Cleanup();
                result = ER_OK;
                break;
            } catch (Platform::COMException ^ ex) {
                SetLastError(COMExceptionToQStatus(ex->HResult), true);
                SetEvent(Events::Exception);
                break;
            } catch (...) {
                SetLastError(ER_OS_ERROR, true);
                SetEvent(Events::Exception);
                break;
            }
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Shutdown()
{
    ::QStatus result = ER_FAIL;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        try {
            // Set last error to tear down
            SetLastError(ER_SOCK_OTHER_END_CLOSED, true);
            // Cleanup
            Cleanup();
            result = ER_OK;
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::JoinMulticastGroup(Platform::String ^ host)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        // Check initialization
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        // Validate host is valid IPv4/IPv6
        result = (::QStatus)IsValidAddress(host);
        if (ER_OK != result) {
            break;
        }
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            result = (::QStatus)LastError;
            break;
        }
        // Check bind state
        if ((GetBindingState() & BindingState::Bind) == 0) {
            // Must not be bound
            result = ER_FAIL;
            break;
        }
        // Check socket type
        if (_socketType != SocketType::QCC_SOCK_DGRAM) {
            // Must be a UDP socket
            result = ER_FAIL;
            break;
        }
        // Create HostName from string
        Windows::Networking::HostName ^ hostName = ref new Windows::Networking::HostName(host);
        // Check for failed allocation
        if (nullptr == hostName) {
            result = ER_OUT_OF_MEMORY;
            break;
        }
        try {
            // Join the multicast group
            _udpSocket->JoinMulticastGroup(hostName);
            result = ER_OK;
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult));
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR);
            break;
        }
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
    return result;
}

void SocketWrapper::Ssl::set(bool ssl)
{
    ::QStatus result = ER_OK;
    // Grab the API lock
    _mutex.Lock();
    while (true) {
        // Fail operations on sockets in an exception state
        if ((GetBindingState() & BindingState::Exception) != 0) {
            break;
        }
        // Check socket type
        if (_socketType != SocketType::QCC_SOCK_STREAM) {
            result = ER_FAIL;
            break;
        }
        // Check state for connect
        if ((GetBindingState() & BindingState::Connect) != 0) {
            // Must not be connected
            result = ER_FAIL;
            break;
        }
        _ssl = ssl;
        break;
    }
    // Release the API lock
    _mutex.Unlock();
    // Store the last error
    SetLastError(result);
}

bool SocketWrapper::Ssl::get()
{
    // Grab the API lock
    _mutex.Lock();
    bool ssl = _ssl;
    // Release the API lock
    _mutex.Unlock();
    // Return Ssl
    return ssl;
}

void SocketWrapper::SetLastError(uint32_t status, bool isFinal)
{
    // Grab the API lock
    _mutex.Lock();
    // Check if error is a final status and set as appopriate
    if (isFinal && (GetBindingState() & BindingState::Exception) == 0) {
        SetBindingState(BindingState::Exception);
        LastError = status;
    } else if ((GetBindingState() & BindingState::Exception) == 0) {
        LastError = status;
    }
    // Release the API lock
    _mutex.Unlock();
}

void SocketWrapper::SetEventMask(int mask)
{
    // Get the API lock
    _mutex.Lock();
    int previousMask = _eventMask;
    _eventMask = mask;
    int currentMask = _eventMask;
    int currentEvents = _events;
    // Compute if mask has changed
    if ((previousMask & currentEvents) !=
        (currentMask & currentEvents)) {
        if (_initialized) {
            // Dispatch the socket events
            ExecuteSocketEventsChanged(currentEvents);
        }
    }
    // Release the API lock
    _mutex.Unlock();
}

int SocketWrapper::GetEvents()
{
    // Grab the API lock
    _mutex.Lock();
    // Get the effective set of events
    int events = _events & _eventMask;
    // Release the API lock
    _mutex.Unlock();
    // Return Events
    return events;
}

void SocketWrapper::ExecuteSocketEventsChanged(int flags)
{
    // Grab the API lock
    _mutex.Lock();
    if (_initialized) {
        // Call the SocketEventsChanged handlers with flags
        SocketEventsChanged(this, flags);
    }
    // Release the API lock
    _mutex.Unlock();
}

void SocketWrapper::SetEvent(Events e)
{
    // Grab the API lock
    _mutex.Lock();
    int previousEvents = _events;
    _events |= (int)e;
    int currentMask = _eventMask;
    int currentEvents = _events;
    // Compute if event mask changed
    if ((currentMask & previousEvents) !=
        (currentMask & currentEvents)) {
        if (_initialized) {
            // Dispatch the socket events
            ExecuteSocketEventsChanged(currentEvents);
        }
    }
    // Release the API lock
    _mutex.Unlock();
}

void SocketWrapper::ClearEvent(Events e)
{
    // Grab the API lock
    _mutex.Lock();
    int previousEvents = _events;
    // Clear the event
    _events &= ~(int)e;
    int currentMask = _eventMask;
    int currentEvents = _events;
    // Release the API lock
    _mutex.Unlock();
}

void SocketWrapper::SetBindingState(BindingState state)
{
    // Grab the API lock
    _mutex.Lock();
    // Set the binding state
    _bindingState |= state;
    // Release the API lock
    _mutex.Unlock();
}

int SocketWrapper::GetBindingState()
{
    // Grab the API lock
    _mutex.Lock();
    // Get the binding state
    int state = _bindingState;
    // Release the API lock
    _mutex.Unlock();
    // Return state
    return state;
}

void SocketWrapper::ClearBindingState(BindingState state)
{
    // Grab the API lock
    _mutex.Lock();
    // Clear the state
    _bindingState &= ~state;
    // Release the API lock
    _mutex.Unlock();
}

uint32_t SocketWrapper::COMExceptionToQStatus(uint32_t hresult)
{
    // Translate hresult from WinRT API to a proper QStatus
    uint32_t status = ER_OS_ERROR;
    if (HRESULT_FROM_WIN32(WSAECONNREFUSED) == hresult) {
        status = ER_CONN_REFUSED;
    } else if (HRESULT_FROM_WIN32(WSAECONNRESET) == hresult) {
        status = ER_SOCK_OTHER_END_CLOSED;
    } else if (HRESULT_FROM_WIN32(WSAETIMEDOUT) == hresult) {
        status = ER_TIMEOUT;
    } else {
        status = ER_OS_ERROR;
    }
    return status;
}

}
}
