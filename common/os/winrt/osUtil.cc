/**
 * @file
 *
 * OS specific utility functions
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

#include <string.h>
#include <windows.h>
#include <process.h>

#include <qcc/Util.h>
#include <qcc/String.h>
#include <qcc/Crypto.h>
#include <qcc/Debug.h>
#include <qcc/winrt/utility.h>
#include "ppltasks.h"

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;

#define QCC_MODULE  "UTIL"


uint32_t qcc::GetPid()
{
    return 0;
}

static uint32_t ComputeId(const char* buf, size_t len)
{
    QCC_DbgPrintf(("ComputeId %s", buf));
    uint32_t digest[qcc::Crypto_SHA1::DIGEST_SIZE / sizeof(uint32_t)];
    qcc::Crypto_SHA1 sha1;
    sha1.Init();
    sha1.Update((const uint8_t*)buf, (size_t)len);
    sha1.GetDigest((uint8_t*)digest);
    return digest[0];
}

uint32_t qcc::GetUid()
{
    return ComputeId("nobody", sizeof("nobody") - 1);
}

uint32_t qcc::GetGid()
{
    return ComputeId("nogroup", sizeof("nogroup") - 1);
}

uint32_t qcc::GetUsersUid(const char* name)
{
    return ComputeId(name, strlen(name));
}

uint32_t qcc::GetUsersGid(const char* name)
{
    return ComputeId(name, strlen(name));
}

qcc::String qcc::GetHomeDir()
{
    return Environ::GetAppEnviron()->Find("APPLICATIONDATA");
}

qcc::OSType qcc::GetSystemOSType(void)
{
    return qcc::WINRT_OS;
}

QStatus qcc::GetDirListing(const char* path, DirListing& listing)
{
    return ER_NOT_IMPLEMENTED;
}


QStatus qcc::Exec(const char* exec, const ExecArgs& args, const qcc::Environ& envs)
{
    return ER_NOT_IMPLEMENTED;
}


QStatus qcc::ExecAs(const char* user, const char* exec, const ExecArgs& args, const qcc::Environ& envs)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus qcc::ResolveHostName(qcc::String hostname, uint8_t addr[], size_t addrSize, size_t& addrLen, uint32_t timeoutMs)
{
    try {
        IAsyncOperation<Collections::IVectorView<EndpointPair ^> ^> ^ op = DatagramSocket::GetEndpointPairsAsync(ref new HostName(MultibyteToPlatformString(hostname.c_str())), L"0");
        concurrency::task<Collections::IVectorView<EndpointPair ^>^> dnsTask(op);
        dnsTask.wait();
        Collections::IVectorView<EndpointPair ^> ^ entries = dnsTask.get();
        if (entries->Size > 0) {
            Platform::String ^ remoteIp = entries->GetAt(0)->RemoteHostName->RawName;
            qcc::String mbIp = PlatformToMultibyteString(remoteIp);
            IPAddress tmpIpAddr(mbIp);
            addrLen = tmpIpAddr.Size();
            if (addrLen == qcc::IPAddress::IPv4_SIZE) {
                tmpIpAddr.RenderIPv4Binary(&addr[qcc::IPAddress::IPv6_SIZE - qcc::IPAddress::IPv4_SIZE], addrSize);
            } else {
                tmpIpAddr.RenderIPv6Binary(addr, addrSize);
            }
            return ER_OK;
        }
    } catch (Platform::Exception ^ e) {
        auto m = e->Message;
    }
    return ER_BAD_HOSTNAME;
}
