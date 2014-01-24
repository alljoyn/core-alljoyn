/**
 * @file
 *
 * Define the abstracted socket interface for WinRT
 */

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

#include <qcc/platform.h>

#include <qcc/IPAddress.h>
#include <qcc/Socket.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>
#include <qcc/StringUtil.h>
#include <qcc/winrt/utility.h>
#include <qcc/winrt/SocketWrapper.h>
#include <qcc/winrt/SocketsWrapper.h>
#include <Status.h>

#define QCC_MODULE "NETWORK"

namespace qcc {

const SocketFd INVALID_SOCKET_FD = (SocketFd) reinterpret_cast<void*>(nullptr);
const int MAX_LISTEN_CONNECTIONS = 10;
static uint32_t _lastError = (uint32_t)ER_OK;

uint32_t GetLastError()
{
    return _lastError;
}

qcc::String GetLastErrorString()
{
    return QCC_StatusText((QStatus)_lastError);
}

QStatus Socket(AddressFamily addrFamily, SocketType type, SocketFd& sockfd)
{
    QStatus status = ER_FAIL;

    while (true) {
        Platform::Array<qcc::winrt::SocketWrapper ^> ^ tempSocket = ref new Platform::Array<qcc::winrt::SocketWrapper ^>(1);
        if (nullptr == tempSocket) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Socket((qcc::winrt::AddressFamily)(int)addrFamily,
                                                                  (qcc::winrt::SocketType)(int)type,
                                                                  tempSocket);
        sockfd = (SocketFd) reinterpret_cast<void*>(tempSocket[0]);
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}


QStatus Connect(SocketFd sockfd, const IPAddress& remoteAddr, uint16_t remotePort)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::String ^ strRemoteAddr = MultibyteToPlatformString(remoteAddr.ToString().c_str());
        if (nullptr == strRemoteAddr) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Connect(socket,
                                                                   strRemoteAddr,
                                                                   remotePort);
        if (ER_OK == status) {
            status = (QStatus)(int)qcc::winrt::SocketsWrapper::SetBlocking(socket,
                                                                           false);
        }
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}


QStatus Connect(SocketFd sockfd, const char* pathName)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::String ^ strPathName = MultibyteToPlatformString(pathName);
        if (nullptr == strPathName) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Connect(socket,
                                                                   strPathName);
        if (ER_OK == status) {
            status = (QStatus)(int)qcc::winrt::SocketsWrapper::SetBlocking(socket,
                                                                           false);
        }
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}


QStatus Bind(SocketFd sockfd, const IPAddress& localAddr, uint16_t localPort)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::String ^ strLocalAddr = MultibyteToPlatformString(localAddr.ToString().c_str());
        if (nullptr == strLocalAddr) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Bind(socket,
                                                                strLocalAddr,
                                                                localPort);
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}


QStatus Bind(SocketFd sockfd, const char* pathName)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::String ^ strPathName = MultibyteToPlatformString(pathName);
        if (nullptr == strPathName) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Bind(socket,
                                                                strPathName);
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}


QStatus Listen(SocketFd sockfd, int backlog)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Listen(socket,
                                                                  backlog);
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}


QStatus Accept(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort, SocketFd& newSockfd)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::Array<Platform::String ^> ^ tempAddress = ref new Platform::Array<Platform::String ^>(1);
        if (nullptr == tempAddress) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        Platform::Array<int> ^ tempPort = ref new Platform::Array<int>(1);
        if (nullptr == tempPort) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        Platform::Array<qcc::winrt::SocketWrapper ^> ^ tempSocket = ref new Platform::Array<qcc::winrt::SocketWrapper ^>(1);
        if (nullptr == tempSocket) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Accept(socket,
                                                                  tempAddress,
                                                                  tempPort,
                                                                  tempSocket);
        qcc::String address = PlatformToMultibyteString(tempAddress[0]);
        if (nullptr != tempAddress[0] && address.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        size_t pos = address.find_first_of('%');
        if (qcc::String::npos != pos) {
            address = address.substr(0, pos);
        }
        qcc::IPAddress tempAddr(address);
        remoteAddr = tempAddr;
        remotePort = (uint16_t)tempPort[0];
        newSockfd = (SocketFd) reinterpret_cast<void*>(tempSocket[0]);
        if (ER_OK == status) {
            status = (QStatus)(int)qcc::winrt::SocketsWrapper::SetBlocking(tempSocket[0],
                                                                           false);
        }
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}


QStatus Accept(SocketFd sockfd, SocketFd& newSockfd)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::Array<qcc::winrt::SocketWrapper ^> ^ tempSocket = ref new Platform::Array<qcc::winrt::SocketWrapper ^>(1);
        if (nullptr == tempSocket) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Accept(socket,
                                                                  tempSocket);
        newSockfd = (SocketFd) reinterpret_cast<void*>(tempSocket[0]);
        if (ER_OK == status) {
            status = (QStatus)(int)qcc::winrt::SocketsWrapper::SetBlocking(tempSocket[0],
                                                                           false);
        }
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}


QStatus Shutdown(SocketFd sockfd)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Shutdown(socket);
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}


void Close(SocketFd sockfd)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Close(socket);
        break;
    }

    _lastError = (uint32_t)status;
}

QStatus SocketDup(SocketFd sockfd, SocketFd& dupSock)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::Array<qcc::winrt::SocketWrapper ^> ^ tempSocket = ref new Platform::Array<qcc::winrt::SocketWrapper ^>(1);
        if (nullptr == tempSocket) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::SocketDup(socket,
                                                                     tempSocket);
        dupSock = (SocketFd) reinterpret_cast<void*>(tempSocket[0]);
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}

QStatus GetLocalAddress(SocketFd sockfd, IPAddress& addr, uint16_t& port)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::Array<Platform::String ^> ^ tempAddress = ref new Platform::Array<Platform::String ^>(1);
        if (nullptr == tempAddress) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        Platform::Array<int> ^ tempPort = ref new Platform::Array<int>(1);
        if (nullptr == tempPort) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::GetLocalAddress(socket,
                                                                           tempAddress,
                                                                           tempPort);
        qcc::String address = PlatformToMultibyteString(tempAddress[0]);
        if (nullptr != tempAddress[0] && address.empty()) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        qcc::IPAddress tempAddr;
        status = tempAddr.SetAddress(address, true);
        addr = tempAddr;
        port = (uint16_t)tempPort[0];
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}

QStatus Send(SocketFd sockfd, const void* buf, size_t len, size_t& sent)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::ArrayReference<uint8> arrRef((unsigned char*)buf, len);
        Platform::Array<int> ^ tempSent = ref new Platform::Array<int>(1);
        if (nullptr == tempSent) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Send(socket,
                                                                arrRef,
                                                                len,
                                                                tempSent);
        sent = tempSent[0];
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}


QStatus SendTo(SocketFd sockfd, IPAddress& remoteAddr, uint16_t remotePort,
               const void* buf, size_t len, size_t& sent)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::String ^ strRemoteAddr = MultibyteToPlatformString(remoteAddr.ToString().c_str());
        if (nullptr == strRemoteAddr) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        Platform::ArrayReference<uint8> arrRef((unsigned char*)buf, len);
        Platform::Array<int> ^ tempSent = ref new Platform::Array<int>(1);
        if (nullptr == tempSent) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::SendTo(socket,
                                                                  strRemoteAddr,
                                                                  remotePort,
                                                                  arrRef,
                                                                  len,
                                                                  tempSent);
        sent = tempSent[0];
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}

QStatus Recv(SocketFd sockfd, void* buf, size_t len, size_t& received)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::ArrayReference<uint8> arrRef((unsigned char*)buf, len);
        Platform::Array<int> ^ tempReceived = ref new Platform::Array<int>(1);
        if (nullptr == tempReceived) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::Recv(socket,
                                                                arrRef,
                                                                len,
                                                                tempReceived);
        received = tempReceived[0];
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}


QStatus RecvFrom(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort,
                 void* buf, size_t len, size_t& received)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        Platform::ArrayReference<uint8> arrRef((unsigned char*)buf, len);
        Platform::Array<Platform::String ^> ^ tempRemoteAddr = ref new Platform::Array<Platform::String ^>(1);
        if (nullptr == tempRemoteAddr) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        Platform::Array<int> ^ tempRemotePort = ref new Platform::Array<int>(1);
        if (nullptr == tempRemotePort) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        Platform::Array<int> ^ tempReceived = ref new Platform::Array<int>(1);
        if (nullptr == tempReceived) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::RecvFrom(socket,
                                                                    tempRemoteAddr,
                                                                    tempRemotePort,
                                                                    arrRef,
                                                                    len,
                                                                    tempReceived);
        qcc::String strRemoteAddress = PlatformToMultibyteString(tempRemoteAddr[0]);
        if (strRemoteAddress.empty() && nullptr != tempRemoteAddr[0]) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        size_t pos = strRemoteAddress.find_first_of('%');
        if (qcc::String::npos != pos) {
            strRemoteAddress = strRemoteAddress.substr(0, pos);
        }
        remoteAddr = IPAddress(strRemoteAddress);
        remotePort = tempRemotePort[0];
        received = tempReceived[0];
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}

QStatus RecvWithFds(SocketFd sockfd, void* buf, size_t len, size_t& received, SocketFd* fdList, size_t maxFds, size_t& recvdFds)
{
    QStatus status = ER_NOT_IMPLEMENTED;

    // OOB data is not supported

    return status;
}

QStatus SendWithFds(SocketFd sockfd, const void* buf, size_t len, size_t& sent, SocketFd* fdList, size_t numFds, uint32_t pid)
{
    QStatus status = ER_NOT_IMPLEMENTED;

    // OOB data is not supported

    return status;
}

QStatus SocketPair(SocketFd(&socketpair)[2])
{
    QStatus status = ER_FAIL;

    while (true) {
        Platform::Array<qcc::winrt::SocketWrapper ^> ^ tempSockets = ref new Platform::Array<qcc::winrt::SocketWrapper ^>(2);
        if (nullptr == tempSockets) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::SocketPair(tempSockets);
        socketpair[0] = (SocketFd) reinterpret_cast<void*>(tempSockets[0]);
        socketpair[1] = (SocketFd) reinterpret_cast<void*>(tempSockets[1]);
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}

QStatus SetBlocking(SocketFd sockfd, bool blocking)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::SetBlocking(socket,
                                                                       blocking);
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}

QStatus SetNagle(SocketFd sockfd, bool useNagle)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockfd);
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::SetNagle(socket,
                                                                    useNagle);
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}

QStatus SetReuseAddress(SocketFd sockfd, bool reuse)
{
    QStatus status = ER_NOT_IMPLEMENTED;

    // Not available in WinRT

    return status;
}

QStatus SetReusePort(SocketFd sockfd, bool reuse)
{
    QStatus status = ER_NOT_IMPLEMENTED;

    // Not available in WinRT

    return status;
}

QStatus JoinMulticastGroup(SocketFd sockFd, AddressFamily family, String multicastGroup, String iface)
{
    QStatus status = ER_FAIL;

    while (true) {
        qcc::winrt::SocketWrapper ^ socket = reinterpret_cast<qcc::winrt::SocketWrapper ^>((void*)sockFd);
        Platform::String ^ strMulticastGroup = MultibyteToPlatformString(multicastGroup.c_str());
        if (nullptr == strMulticastGroup) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)qcc::winrt::SocketsWrapper::JoinMulticastGroup(socket,
                                                                              strMulticastGroup);
        break;
    }

    _lastError = (uint32_t)status;
    return status;
}

QStatus LeaveMulticastGroup(SocketFd sockFd, AddressFamily family, String multicastGroup, String iface)
{
    QStatus status = ER_NOT_IMPLEMENTED;

    // Not available in WinRT

    return status;
}

QStatus SetMulticastInterface(SocketFd sockFd, AddressFamily family, qcc::String iface)
{
    QStatus status = ER_NOT_IMPLEMENTED;

    // Not available in WinRT

    return status;
}

QStatus SetMulticastHops(SocketFd sockFd, AddressFamily family, uint32_t hops)
{
    QStatus status = ER_NOT_IMPLEMENTED;

    // Not available in WinRT

    return status;
}

QStatus SetBroadcast(SocketFd sockfd, bool broadcast)
{
    QStatus status = ER_NOT_IMPLEMENTED;

    // Not available in WinRT

    return status;
}

}   /* namespace */
