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

#include "SocketStream.h"

#include <algorithm>
#include <qcc/atomic.h>
#include <qcc/IPAddress.h>

#include <qcc/winrt/SocketWrapper.h>
#include <qcc/winrt/SocketsWrapper.h>
#include <qcc/winrt/utility.h>
#include <qcc/winrt/exception.h>

using namespace Windows::Foundation;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

/** @internal */
#define QCC_MODULE "SOCKETSTREAM"

namespace AllJoyn {

SocketStream::SocketStream() : _sockfd(nullptr)
{
}

SocketStream::SocketStream(qcc::winrt::SocketWrapper ^ sockfd) : _sockfd(sockfd)
{
    // Change to blocking mode
    sockfd->SetBlocking(true);
}

SocketStream::~SocketStream()
{
    // Cleanup the underlying sockfd
    if (nullptr != _sockfd) {
        _sockfd->Close();
        _sockfd = nullptr;
    }
}

void SocketStream::SocketDup(Platform::WriteOnlyArray<SocketStream ^> ^ dupSocket)
{
    ::QStatus result = ER_FAIL;

    while (true) {
        // Check dupSocket for invalid values
        if (nullptr == dupSocket || dupSocket->Length != 1) {
            result = ER_BAD_ARG_1;
            break;
        }
        if (nullptr != _sockfd) {
            // Allocate temporary SocketWrapper array
            Platform::Array<qcc::winrt::SocketWrapper ^> ^ dup = ref new Platform::Array<qcc::winrt::SocketWrapper ^>(1);
            if (nullptr == dup) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            // Call the SocketWrapper SocketDup API
            result = (::QStatus)_sockfd->SocketDup(dup);
            if (ER_OK == result) {
                // Create SocketStream
                SocketStream ^ ret = ref new SocketStream(dup[0]);
                if (nullptr == ret) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                // Store the result
                dupSocket[0] = ret;
            }
        }
        break;
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != result) {
        QCC_THROW_EXCEPTION(result);
    }
}

void SocketStream::Send(const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent)
{
    ::QStatus result = ER_FAIL;

    if (nullptr != _sockfd) {
        // Call the SocketWrapper Send API
        result = (::QStatus)_sockfd->Send(buf, len, sent);
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != result) {
        QCC_THROW_EXCEPTION(result);
    }
}

void SocketStream::Recv(Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received)
{
    ::QStatus result = ER_FAIL;

    if (nullptr != _sockfd) {
        // Call teh SocketWrapper Recv API
        result = (::QStatus)_sockfd->Recv(buf, len, received);
    }

    // Bubble up any QStatus errors as an exception
    if (ER_OK != result) {
        QCC_THROW_EXCEPTION(result);
    }
}

bool SocketStream::CanRead()
{
    // Return if can read
    return (_sockfd->GetEvents() & (int)qcc::winrt::Events::Read) != 0;
}

bool SocketStream::CanWrite()
{
    // Return if can write
    return (_sockfd->GetEvents() & (int)qcc::winrt::Events::Write) != 0;
}

void SocketStream::SetBlocking(bool block)
{
    // Change the blocking mode of the sockfd
    _sockfd->SetBlocking(block);
}

}
