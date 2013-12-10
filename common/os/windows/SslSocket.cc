/**
 * @file
 *
 * SSL stream-based socket implementation for Windows
 *
 */

/******************************************************************************
 * Copyright (c) 2012 AllSeen Alliance. All rights reserved.
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
#include <qcc/IPAddress.h>
#include <qcc/SslSocket.h>


#define QCC_MODULE  "SSL"


namespace qcc {

SslSocket::SslSocket(String host, const char* rootCert, const char* caCert) :
    internal(NULL),
    sourceEvent(&qcc::Event::neverSet),
    sinkEvent(&qcc::Event::neverSet),
    Host(host),
    sock(-1)
{
}

SslSocket::~SslSocket() {

}

QStatus SslSocket::Connect(const qcc::String hostName, uint16_t port)
{
    return ER_FAIL;
}

void SslSocket::Close()
{
}

QStatus SslSocket::PullBytes(void*buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    return ER_FAIL;
}

QStatus SslSocket::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{
    return ER_FAIL;
}

QStatus SslSocket::ImportPEM(const char* rootCert, const char* caCert)
{
    return ER_FAIL;
}

}  /* namespace */
