/**
 * @file
 *
 * OS specific utility functions
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014, AllSeen Alliance. All rights reserved.
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

#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Environ.h>
#include <qcc/Thread.h>
#include <qcc/Util.h>

#include <Status.h>


#define QCC_MODULE  "UTIL"

using namespace std;

uint32_t qcc::GetPid()
{
    return static_cast<uint32_t>(getpid());
}

uint32_t qcc::GetUid()
{
    return static_cast<uint32_t>(getuid());
}

uint32_t qcc::GetGid()
{
    return static_cast<uint32_t>(getgid());
}

uint32_t qcc::GetUsersUid(const char* name)
{
    uint32_t uid = -1;
    if (name) {
        struct passwd* pwent = getpwnam(name);
        if (pwent) {
            uid = pwent->pw_uid;
        }
    }
    return uid;
}

uint32_t qcc::GetUsersGid(const char* name)
{
    uint32_t gid = -1;
    if (name) {
        struct group* grent = getgrnam(name);
        if (grent) {
            gid = grent->gr_gid;
        }
    }
    return gid;
}

qcc::String qcc::GetHomeDir()
{
    /* Defaulting to '/' handles both the plain posix and Android cases. */
    return Environ::GetAppEnviron()->Find("HOME", "/");
}

qcc::OSType qcc::GetSystemOSType(void)
{
#if defined (QCC_OS_ANDROID)
    return qcc::ANDROID_OS;
#elif defined (QCC_OS_LINUX)
    return qcc::LINUX_OS;
#elif defined (QCC_OS_DARWIN)
    return qcc::DARWIN_OS;
#endif
    return qcc::NONE;
}

QStatus qcc::GetDirListing(const char* path, DirListing& listing)
{
    DIR* dir;
    struct dirent* entry;

    dir = opendir(path);
    if (!dir) {
        return ER_OS_ERROR;
    }
    while ((entry = readdir(dir)) != NULL) {
        listing.push_back(entry->d_name);
    }
    closedir(dir);
    return ER_OK;
}


QStatus qcc::Exec(const char* exec, const ExecArgs& args, const Environ& envs)
{
    pid_t pid;

    pid = fork();

    if (pid == 0) {
        pid_t sid = setsid();
        if (sid < 0) {
            QCC_LogError(ER_OS_ERROR, ("Failed to set session ID for new process"));
            return ER_OS_ERROR;
        }
        char** argv(new char*[args.size() + 2]);    // Need extra entry for executable name
        char** env(new char*[envs.Size() + 1]);
        int index = 0;
        ExecArgs::const_iterator it;
        Environ::const_iterator envit;

        argv[index] = strdup(exec);
        ++index;

        for (it = args.begin(); it != args.end(); ++it, ++index) {
            argv[index] = strdup(it->c_str());
        }
        argv[index] = NULL;

        for (envit = envs.Begin(), index = 0; envit != envs.End(); ++envit, ++index) {
            qcc::String var((envit->first + "=" + envit->second));
            env[index] = strdup(var.c_str());
        }
        env[index] = NULL;

        execve(exec, argv, env); // will never return if successful.
    } else if (pid == -1) {
        return ER_OS_ERROR;
#ifndef NDEBUG
    } else {
        QCC_DbgPrintf(("Started %s with PID: %d", exec, pid));
#endif
    }
    return ER_OK;
}


QStatus qcc::ExecAs(const char* user, const char* exec, const ExecArgs& args, const Environ& envs)
{
    pid_t pid;

    pid = fork();

    if (pid == 0) {
        pid_t sid = setsid();
        if (sid < 0) {
            QCC_LogError(ER_OS_ERROR, ("Failed to set session ID for new process"));
            return ER_OS_ERROR;
        }
        char** argv(new char*[args.size() + 2]);    // Need extra entry for executable name
        char** env(new char*[envs.Size() + 1]);
        int index = 0;
        ExecArgs::const_iterator it;
        Environ::const_iterator envit;

        argv[index] = strdup(exec);
        ++index;

        for (it = args.begin(); it != args.end(); ++it, ++index) {
            argv[index] = strdup(it->c_str());
        }
        argv[index] = NULL;

        for (envit = envs.Begin(), index = 0; envit != envs.End(); ++envit, ++index) {
            qcc::String var((envit->first + "=" + envit->second));
            env[index] = strdup(var.c_str());
        }
        env[index] = NULL;

        struct passwd* pwent = getpwnam(user);
        if (!pwent) {
            return ER_FAIL;
        }

        if (setuid(pwent->pw_uid) == -1) {
            return ER_OS_ERROR;
        }

        execve(exec, argv, env); // will never return if successful.
    } else if (pid == -1) {
        return ER_OS_ERROR;
#ifndef NDEBUG
    } else {
        QCC_DbgPrintf(("Started %s with PID: %d", exec, pid));
#endif
    }
    return ER_OK;
}

class ResolverThread : public qcc::Thread, public qcc::ThreadListener {
  public:
    ResolverThread(qcc::String& hostname, uint8_t*addr, size_t* addrLen);
    virtual ~ResolverThread() { }
    QStatus Get(uint32_t timeoutMs);

  protected:
    qcc::ThreadReturn STDCALL Run(void* arg);
    void ThreadExit(Thread* thread);

  private:
    qcc::String hostname;
    uint8_t*addr;
    size_t* addrLen;
    QStatus status;
    qcc::Mutex lock;
    qcc::Event complete;
    bool threadHasExited;
};

ResolverThread::ResolverThread(qcc::String& hostname, uint8_t* addr, size_t* addrLen)
    : hostname(hostname), addr(addr), addrLen(addrLen), threadHasExited(false)
{
    status = Start(NULL, this);
    if (ER_OK != status) {
        addr = NULL;
        addrLen = NULL;
    }
}

QStatus ResolverThread::Get(uint32_t timeoutMs)
{
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
    QCC_DbgTrace(("ResolverThread::Run()"));
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

    QCC_DbgTrace(("ResolverThread::Run() complete"));
    complete.SetEvent();
    return (void*) status;
}

QStatus qcc::ResolveHostName(qcc::String hostname, uint8_t addr[], size_t addrSize, size_t& addrLen, uint32_t timeoutMs)
{
    QCC_DbgTrace(("%s(hostname=%s,timeoutMs=%u)", __FUNCTION__, hostname.c_str(), timeoutMs));
    if (qcc::IPAddress::IPv6_SIZE != addrSize) {
        return ER_BAD_HOSTNAME;
    }
    return (new ResolverThread(hostname, addr, &addrLen))->Get(timeoutMs);
}
