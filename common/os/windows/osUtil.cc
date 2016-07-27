/**
 * @file
 *
 * OS specific utility functions
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#include <string.h>
#include <windows.h>
#include <process.h>
#include <lmcons.h>
#define SECURITY_WIN32
#include <security.h>
#include <secext.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <qcc/Util.h>
#include <qcc/IPAddress.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/Crypto.h>
#include <qcc/Debug.h>

#define QCC_MODULE  "UTIL"

/**
 * These are dummy number with no specific meaning.
 * They should not be used for security-related decissions
 * inside PolicyDb (and anywhere else), since these numbers
 * are returned by the app itself and can be manipulated.
 */
static const uint32_t s_dummyWindowsUid = 0xABCD1234;
static const uint32_t s_dummyWindowsGid = 0x4321DCBA;

void qcc::ClearMemory(void* s, size_t n)
{
    SecureZeroMemory(s, n);
}

uint32_t qcc::GetPid()
{
    return static_cast<uint32_t>(GetCurrentProcessId());
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
    return s_dummyWindowsUid;
}

uint32_t qcc::GetGid()
{
    return s_dummyWindowsGid;
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
    qcc::String homeDir = Environ::GetAppEnviron()->Find("LOCALAPPDATA");
    if (homeDir.empty()) {
        homeDir = Environ::GetAppEnviron()->Find("USERPROFILE");
    }
    return homeDir;
}

qcc::OSType qcc::GetSystemOSType(void)
{
    return qcc::WINDOWS_OS;
}

QStatus qcc::GetDirListing(const char* path, DirListing& listing)
{
    QCC_UNUSED(path);
    QCC_UNUSED(listing);
    return ER_NOT_IMPLEMENTED;
}


QStatus qcc::Exec(const char* exec, const ExecArgs& args, const qcc::Environ& envs)
{
    QCC_UNUSED(exec);
    QCC_UNUSED(args);
    QCC_UNUSED(envs);
    return ER_NOT_IMPLEMENTED;
}


QStatus qcc::ExecAs(const char* user, const char* exec, const ExecArgs& args, const qcc::Environ& envs)
{
    QCC_UNUSED(user);
    QCC_UNUSED(exec);
    QCC_UNUSED(args);
    QCC_UNUSED(envs);
    return ER_NOT_IMPLEMENTED;
}

class ResolverThread : public qcc::Thread, public qcc::ThreadListener {
  public:
    ResolverThread(qcc::String& hostname, uint8_t* addr, size_t* addrLen);
    virtual ~ResolverThread() { }
    QStatus Get(uint32_t timeoutMs);

  protected:
    qcc::ThreadReturn STDCALL Run(void* arg);
    void ThreadExit(Thread* thread);

  private:
    qcc::String hostname;
    uint8_t*addr;
    size_t* addrLen;
    qcc::Mutex lock;
    qcc::Event complete;
    bool threadHasExited;
};

ResolverThread::ResolverThread(qcc::String& hostname, uint8_t* addr, size_t* addrLen)
    : hostname(hostname), addr(addr), addrLen(addrLen), threadHasExited(false)
{
    QStatus status = Start(NULL, this);
    if (ER_OK != status) {
        addr = NULL;
        addrLen = NULL;
    }
}

QStatus ResolverThread::Get(uint32_t timeoutMs)
{
    QStatus status = ER_OK;

    if (addr && addrLen) {
        status = qcc::Event::Wait(complete, timeoutMs);
        if (ER_OK == status) {
            Join();
            status = static_cast<QStatus>(reinterpret_cast<uintptr_t>(GetExitValue()));
        }
    }

    lock.Lock(MUTEX_CONTEXT);
    addr = NULL;
    addrLen = NULL;
    bool deleteThis = threadHasExited;
    QStatus sts = status;
    lock.Unlock(MUTEX_CONTEXT);

    if (deleteThis) {
        Join();
        delete this;
    }
    return sts;
}

void ResolverThread::ThreadExit(Thread*)
{
    lock.Lock(MUTEX_CONTEXT);
    threadHasExited = true;
    bool deleteThis = !addr && !addrLen;
    lock.Unlock(MUTEX_CONTEXT);

    if (deleteThis) {
        Join();
        delete this;
    }
}

void* ResolverThread::Run(void*)
{
    QStatus status = ER_OK;

    struct addrinfo* info = NULL;
    int ret = getaddrinfo(hostname.c_str(), NULL, NULL, &info);
    if (0 == ret) {
        lock.Lock(MUTEX_CONTEXT);
        if (addr && addrLen) {
            if (info->ai_family == AF_INET6) {
                struct sockaddr_in6* sa = (struct sockaddr_in6*) info->ai_addr;
                memcpy(addr, &sa->sin6_addr, qcc::IPAddress::IPv6_SIZE);
                *addrLen = qcc::IPAddress::IPv6_SIZE;
            } else if (info->ai_family == AF_INET) {
                struct sockaddr_in* sa = (struct sockaddr_in*) info->ai_addr;
                memcpy(&addr[qcc::IPAddress::IPv6_SIZE - qcc::IPAddress::IPv4_SIZE], &sa->sin_addr, qcc::IPAddress::IPv4_SIZE);
                *addrLen = qcc::IPAddress::IPv4_SIZE;
            } else {
                status = ER_FAIL;
            }
        }
        lock.Unlock(MUTEX_CONTEXT);
        freeaddrinfo(info);
    } else {
        status = ER_BAD_HOSTNAME;
        QCC_LogError(status, ("getaddrinfo - %s", gai_strerror(ret)));
    }

    complete.SetEvent();
    return (void*) status;
}

QStatus qcc::ResolveHostName(qcc::String hostname, uint8_t addr[], size_t addrSize, size_t& addrLen, uint32_t timeoutMs)
{
    if (qcc::IPAddress::IPv6_SIZE != addrSize) {
        return ER_BAD_HOSTNAME;
    }
    return (new ResolverThread(hostname, addr, &addrLen))->Get(timeoutMs);
}

/**
 * Non-standard/non-compliant snprintf implementation, based on _vsnprintf.
 *
 * @remark This function always adds a zero character string terminator,
 *         even when the output is truncated due to the output buffer
 *         being too small. However, it is non-compliant - mainly because
 *         it returns -1 when the output buffer is too small, rather than
 *         returning the required length of the output buffer as required
 *         by the C99 standard.
 *
 * @param[out] buffer    Storage location for the output.
 * @param[in]  count     Maximum number of characters to store.
 * @param[in]  format    Format-control string.
 * @param[in]  ...       String format parameters.
 *
 * @return
 *      - The length of the output buffer, not including the zero terminator, on success
 *      - Value -1 if the output has been truncated
 */
int32_t AJ_snprintf(char* buffer, size_t count, const char* format, ...)
{
    /* These three values are invalid when calling _vsnprintf */
    if ((buffer == nullptr) || (count == 0) || (format == nullptr)) {
        QCC_ASSERT(false);
        return -1;
    }

    va_list argp;
    va_start(argp, format);
    int32_t length = _vsnprintf(buffer, count, format, argp);
    QCC_ASSERT(length >= -1);

    if (length == -1) {
        buffer[count - 1] = '\0';
    }

    return length;
}
