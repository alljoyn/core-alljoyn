/******************************************************************************
 *
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
 *
 *****************************************************************************/

#include <qcc/IPAddress.h>
#include <qcc/Debug.h>

#include <qcc/winrt/utility.h>
#include <qcc/winrt/SslSocketWrapper.h>


using namespace Windows::Foundation;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

/** @internal */
#define QCC_MODULE "SSL_SOCKET_WRAPPER"

namespace qcc {
namespace winrt {


SslSocketWrapper::SslSocketWrapper()
{
    LastError = ER_OK;
    _sw = ref new SocketWrapper();
}

SslSocketWrapper::~SslSocketWrapper()
{
    Close();
}

uint32_t SslSocketWrapper::Init(AddressFamily addrFamily, SocketType type)
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Init(addrFamily, type);

    if (result == ER_OK) {
        _sw->Ssl = true;
    }

    SetLastError(result);
    return result;
}


uint32_t SslSocketWrapper::Connect(Platform::String ^ remoteAddr, int remotePort)
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Connect(remoteAddr, remotePort);
    SetLastError(result);
    return result;
}


uint32_t SslSocketWrapper::Send(const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent)
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Send(buf, len, sent);
    SetLastError(result);
    return result;
}

uint32_t SslSocketWrapper::Recv(Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received)
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Recv(buf, len, received);
    SetLastError(result);
    return result;
}

uint32_t SslSocketWrapper::Close()
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Close();
    SetLastError(result);
    return result;
}

uint32_t SslSocketWrapper::Shutdown()
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Shutdown();
    SetLastError(result);
    return result;
}

}
}
