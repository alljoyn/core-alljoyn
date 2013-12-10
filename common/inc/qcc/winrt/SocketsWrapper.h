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

#include <qcc/SocketTypes.h>
#include <qcc/Socket.h>
#include <qcc/winrt/SocketWrapper.h>
#include <qcc/winrt/SocketWrapperTypes.h>

namespace qcc {
namespace winrt {

#ifdef COMMON_WINRT_PUBLIC
public
#else
private
#endif
ref class SocketsWrapper sealed {
  public:
    static uint32_t Socket(AddressFamily addrFamily, SocketType type, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ socket);
    static uint32_t SocketDup(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ dupSocket);
    static uint32_t Bind(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ pathName);
    static uint32_t Bind(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ name, int localPort);
    static uint32_t Listen(qcc::winrt::SocketWrapper ^ socket, int backlog);
    static uint32_t Accept(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ newSocket);
    static uint32_t Accept(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ newSocket);
    static uint32_t SetBlocking(qcc::winrt::SocketWrapper ^ socket, bool blocking);
    static uint32_t SetNagle(qcc::winrt::SocketWrapper ^ socket, bool useNagle);
    static uint32_t Connect(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ pathName);
    static uint32_t Connect(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ remoteAddr, int remotePort);
    static uint32_t SendTo(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ remoteAddr, int remotePort,
                           const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent);
    static uint32_t RecvFrom(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort,
                             Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received);
    static uint32_t Send(qcc::winrt::SocketWrapper ^ socket, const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent);
    static uint32_t Recv(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received);
    static uint32_t GetLocalAddress(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<Platform::String ^> ^ addr, Platform::WriteOnlyArray<int> ^ port);
    static uint32_t Close(qcc::winrt::SocketWrapper ^ socket);
    static uint32_t Shutdown(qcc::winrt::SocketWrapper ^ socket);
    static uint32_t JoinMulticastGroup(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ host);
    static uint32_t SocketPair(Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ sockets);

  protected:
    friend ref class qcc::winrt::SocketWrapper;
    static int IncrementFDMap(qcc::winrt::SocketWrapper ^ socket);
    static int DecrementFDMap(qcc::winrt::SocketWrapper ^ socket);
};

}
}
// Sockets.h
