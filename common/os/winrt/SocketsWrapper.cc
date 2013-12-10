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

#include <qcc/winrt/SocketsWrapper.h>
#include <qcc/IPAddress.h>
#include <qcc/Mutex.h>
#include <map>

using namespace Windows::Foundation;

namespace qcc {
namespace winrt {

// Fd map mutex for tracking ref counts
static qcc::Mutex _fdMapMutex;
static std::map<void*, int> _fdMap;

#define ADJUST_BAD_ARGUMENT_DOMAIN(arg) \
    if ((int)arg >= (int)ER_BAD_ARG_1 && (int)arg <= (int)ER_BAD_ARG_8) { \
        arg = (::QStatus)((int)arg + 1); \
    }

static inline void AddObjectReference(Platform::Object ^ obj)
{
    __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(obj);
    // Increment reference
    pUnk->__abi_AddRef();
}

static inline void RemoveObjectReference(Platform::Object ^ obj)
{
    __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(obj);
    // Decrement reference
    pUnk->__abi_Release();
}

uint32_t SocketsWrapper::Socket(AddressFamily addrFamily, SocketType type, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ socket)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket || socket->Length != 1) {
            status = ER_BAD_ARG_3;
            break;
        }

        // Create SocketWrapper
        qcc::winrt::SocketWrapper ^ sock = ref new qcc::winrt::SocketWrapper();
        if (nullptr == sock) {
            status = ER_OS_ERROR;
            break;
        }

        // Initialize it
        status = (::QStatus)sock->Init(addrFamily, type);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK == status) {
            // Store the result
            socket[0] = sock;
        }
        break;
    }

    return status;
}

uint32_t SocketsWrapper::SocketDup(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ dupSocket)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Check dupSocket for invalid values
        if (nullptr == dupSocket || dupSocket->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }

        // Duplicate the socket
        status = (::QStatus)socket->SocketDup(dupSocket);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Bind(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ pathName)
{
    ::QStatus status = ER_NOT_IMPLEMENTED;

    return status;
}

uint32_t SocketsWrapper::Bind(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ name, int localPort)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Bind the socket
        status = (::QStatus)socket->Bind(name, localPort);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Listen(qcc::winrt::SocketWrapper ^ socket, int backlog)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Start Listening for connections
        status = (::QStatus)socket->Listen(backlog);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Accept(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ newSocket)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Check remoteAddr for invalid values
        if (nullptr == remoteAddr || remoteAddr->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }

        // Check remotePort for invalid values
        if (nullptr == remotePort || remotePort->Length != 1) {
            status = ER_BAD_ARG_3;
            break;
        }

        // Check newSocket for invalid values
        if (nullptr == newSocket || newSocket->Length != 1) {
            status = ER_BAD_ARG_4;
            break;
        }

        // Accept waiting socket
        status = (::QStatus)socket->Accept(remoteAddr, remotePort, newSocket);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Accept(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ newSocket)
{
    ::QStatus status = ER_FAIL;
    Platform::Array<Platform::String ^> ^ remoteAddr = ref new Platform::Array<Platform::String ^>(1);
    Platform::Array<int> ^ remotePort = ref new Platform::Array<int>(1);

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Check newSocket for invalid values
        if (nullptr == newSocket || newSocket->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }

        // Accept waiting socket
        status = (::QStatus)socket->Accept(remoteAddr, remotePort, newSocket);
        break;
    }

    return status;
}

uint32_t SocketsWrapper::SetBlocking(qcc::winrt::SocketWrapper ^ socket, bool blocking)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Change the socket blocking mode
        status = (::QStatus)socket->SetBlocking(blocking);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::SetNagle(qcc::winrt::SocketWrapper ^ socket, bool useNagle)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Set Nagle mode
        status = (::QStatus)socket->SetNagle(useNagle);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Connect(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ pathName)
{
    ::QStatus status = ER_NOT_IMPLEMENTED;

    return status;
}

uint32_t SocketsWrapper::Connect(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ remoteAddr, int remotePort)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Connect remote host
        status = (::QStatus)socket->Connect(remoteAddr, remotePort);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::SendTo(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ remoteAddr, int remotePort,
                                const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Check sent for invalid values
        if (nullptr == sent || sent->Length != 1) {
            status = ER_BAD_ARG_6;
            break;
        }

        // Send the data
        status = (::QStatus)socket->SendTo(remoteAddr, remotePort, buf, len, sent);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::RecvFrom(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort,
                                  Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Check received for invalid values
        if (nullptr == received || received->Length != 1) {
            status = ER_BAD_ARG_6;
            break;
        }

        // Receive the data
        status = (::QStatus)socket->RecvFrom(remoteAddr, remotePort, buf, len, received);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Send(qcc::winrt::SocketWrapper ^ socket, const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Check sent for invalid values
        if (nullptr == sent || sent->Length != 1) {
            status = ER_BAD_ARG_4;
            break;
        }

        // Send the data
        status = (::QStatus)socket->Send(buf, len, sent);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Recv(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Check received for invalid values
        if (nullptr == received || received->Length != 1) {
            status = ER_BAD_ARG_4;
            break;
        }

        // Receive the data
        status = (::QStatus)socket->Recv(buf, len, received);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::GetLocalAddress(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<Platform::String ^> ^ addr, Platform::WriteOnlyArray<int> ^ port)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Check addr for invalid values
        if (nullptr == addr || addr->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }

        // Check port for invalid values
        if (nullptr == port || port->Length != 1) {
            status = ER_BAD_ARG_3;
            break;
        }

        // Get the local address associated with the socket
        status = (::QStatus)socket->GetLocalAddress(addr, port);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Close(qcc::winrt::SocketWrapper ^ socket)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // This forcibely tears down the socket and cleans up resources
        status = (::QStatus)socket->Close();
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Shutdown(qcc::winrt::SocketWrapper ^ socket)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // This prevents further read and write operations on the socket
        status = (::QStatus)socket->Shutdown();
        break;
    }

    return status;
}

uint32_t SocketsWrapper::JoinMulticastGroup(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ host)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        // Check socket for invalid values
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Joins socket to the specified host
        status = (::QStatus)socket->JoinMulticastGroup(host);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::SocketPair(Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ sockets)
{
    ::QStatus status = ER_FAIL;
    Platform::String ^ ipAddr("127.0.0.1");
    Platform::Array<qcc::winrt::SocketWrapper ^> ^ refSocket = ref new Platform::Array<qcc::winrt::SocketWrapper ^>(1);
    Platform::Array<qcc::winrt::SocketWrapper ^> ^ socketSet = ref new Platform::Array<qcc::winrt::SocketWrapper ^>(2);

    while (true) {
        // Check sockets for invalid values
        if (nullptr == sockets || sockets->Length != 2) {
            status = ER_BAD_ARG_1;
            break;
        }

        // Creates an AF_INET socket
        status = (::QStatus)Socket(AddressFamily::QCC_AF_INET, SocketType::QCC_SOCK_STREAM, refSocket);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }
        // Store the result
        socketSet[0] = refSocket[0];

        // Create an AF_INET socket
        status = (::QStatus)Socket(AddressFamily::QCC_AF_INET, SocketType::QCC_SOCK_STREAM, refSocket);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }
        // Store the result
        socketSet[1] = refSocket[0];

        // Bind the first socket for accepting incoming connections
        status = (::QStatus)Bind(socketSet[0], ipAddr, 0);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        // Listen for connections
        status = (::QStatus)Listen(socketSet[0], 1);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        // Get the port dynamically allocated from bind
        Platform::Array<int> ^ localPort = ref new Platform::Array<int>(1);
        Platform::Array<Platform::String ^> ^ localAddr = ref new Platform::Array<Platform::String ^>(1);
        status = (::QStatus)GetLocalAddress(socketSet[0], localAddr, localPort);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        // Connect other end to the server
        status = (::QStatus)Connect(socketSet[1], localAddr[0], localPort[0]);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        // Get the server connected socket
        status = (::QStatus)Accept(socketSet[0], refSocket);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }
        // Cleanup the bound socket
        socketSet[0]->Close();
        // Store the result
        socketSet[0] = refSocket[0];

        // Make socket blocking
        status = (::QStatus)SetBlocking(socketSet[0], true);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        // Make socket blocking
        status = (::QStatus)SetBlocking(socketSet[1], true);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        // Set the result pair
        sockets[0] = socketSet[0];
        sockets[1] = socketSet[1];
        break;
    }

    return status;
}

int SocketsWrapper::IncrementFDMap(qcc::winrt::SocketWrapper ^ socket)
{
    int count = 0;
    void* sockfd =  reinterpret_cast<void*>(socket);

    _fdMapMutex.Lock();

    if (_fdMap.find(sockfd) == _fdMap.end()) {
        // AddRef the socket to fixup ref counting in map
        AddObjectReference(socket);
        _fdMap[sockfd] = ++count;
    } else {
        // Increment the ref count
        count = ++_fdMap[sockfd];
    }

    _fdMapMutex.Unlock();

    return count;
}

int SocketsWrapper::DecrementFDMap(qcc::winrt::SocketWrapper ^ socket)
{
    int count = -1;
    void* sockfd =  reinterpret_cast<void*>(socket);

    _fdMapMutex.Lock();

    if (_fdMap.find(sockfd) != _fdMap.end()) {
        // Decrement the ref count
        count = --_fdMap[sockfd];
    }

    if (count == 0) {
        // Release the socket to fixup ref counting in map
        RemoveObjectReference(socket);
        _fdMap.erase(sockfd);
    }

    _fdMapMutex.Unlock();

    return count;
}

}
}

