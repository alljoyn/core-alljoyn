/******************************************************************************
 *
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *    
 *    SPDX-License-Identifier: Apache-2.0
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *    
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
*
 *****************************************************************************/

#pragma once

#include <qcc/winrt/SocketWrapperTypes.h>
#include <qcc/winrt/SocketWrapper.h>

namespace qcc {
namespace winrt {


#ifdef COMMON_WINRT_PUBLIC
public
#else
private
#endif
ref class SslSocketWrapper sealed {
  public:
    SslSocketWrapper();
    virtual ~SslSocketWrapper();

    uint32_t Init(AddressFamily addrFamily, SocketType type);
    uint32_t Connect(Platform::String ^ remoteAddr, int remotePort);
    uint32_t Send(const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent);
    uint32_t Recv(Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received);
    uint32_t Close();
    uint32_t Shutdown();

    property uint32_t LastError;

  private:
    friend class SslSocket;
    SocketWrapper ^ _sw;
};

}
}
// SocketWrapper.h