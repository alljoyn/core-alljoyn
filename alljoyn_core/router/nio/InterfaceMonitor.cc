/*
 * InterfaceMonitor.cpp
 *
 *  Created on: Apr 1, 2015
 *      Author: erongo
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
#include "InterfaceMonitor.h"
#ifdef QCC_OS_GROUP_WINDOWS
#include <ws2ipdef.h>
#include <iphlpapi.h>
#else
#include "Proactor.h"
#endif

using namespace nio;

#ifdef QCC_OS_GROUP_WINDOWS
VOID WINAPI InterfaceMonitor::IpInterfaceChangeCallback(PVOID arg, PMIB_IPINTERFACE_ROW row, MIB_NOTIFICATION_TYPE notificationType)
{
    QCC_UNUSED(row);
    QCC_UNUSED(notificationType);

    InterfaceMonitor* monitor = static_cast<InterfaceMonitor*>(arg);
    monitor->DoInterfaceCheck();
}
#endif

InterfaceMonitor::InterfaceMonitor(Proactor& p) : proactor(p)
#ifdef QCC_OS_GROUP_WINDOWS
    , networkIfaceHandle(nullptr)
#endif
{
#ifdef QCC_OS_GROUP_POSIX
    Register();
#endif
}

InterfaceMonitor::~InterfaceMonitor()
{
#ifdef QCC_OS_GROUP_POSIX
    proactor.Cancel(readEvent);
    qcc::Close(networkEventFd);
#elif defined QCC_OS_GROUP_WINDOWS
    if (networkIfaceHandle != nullptr) {
        CancelMibChangeNotify2(networkIfaceHandle);
    }
#endif
}

void InterfaceMonitor::AddCallback(IFCallback cb)
{
    std::lock_guard<std::mutex> guard(m_callbacksLock);
    m_callbacks.push_back(cb);
}

void InterfaceMonitor::Notify(const std::vector<qcc::IfConfigEntry>& entries)
{
    InterfaceMonitorCallbackList callbacks;
    {
        std::lock_guard<std::mutex> guard(m_callbacksLock);
        callbacks = m_callbacks;
    }

    for (auto cb : callbacks) {
        cb(entries);
    }
}

void InterfaceMonitor::DoInterfaceCheck()
{
    std::vector<qcc::IfConfigEntry> entries;
    qcc::IfConfig(entries);
    Notify(entries);
}



void InterfaceMonitor::Register()
{
#ifdef QCC_OS_GROUP_POSIX
    networkEventFd = qcc::NetworkEventSocket();
    auto func = [this] (qcc::SocketFd sock, QStatus status) {
                    OnChange(sock, status);
                };
    readEvent = std::make_shared<nio::SocketReadableEvent>(networkEventFd, func);
    proactor.Register(readEvent);
#elif defined QCC_OS_GROUP_WINDOWS
    NotifyIpInterfaceChange(AF_UNSPEC, (PIPINTERFACE_CHANGE_CALLBACK) IpInterfaceChangeCallback, this, false, &networkIfaceHandle);
#endif
}

#ifdef QCC_OS_GROUP_POSIX
void InterfaceMonitor::OnChange(qcc::SocketFd sock, QStatus status)
{
    QCC_UNUSED(sock);
    QCC_UNUSED(status);

    assert(sock == networkEventFd);
    qcc::NetworkEventSet networkEvents;
    qcc::NetworkEventType eventType = qcc::NetworkEventReceive(networkEventFd, networkEvents);
    //bool update = false;

    switch (eventType) {
    case qcc::QCC_RTM_DELADDR:
    case qcc::QCC_RTM_NEWADDR:
        DoInterfaceCheck();
        break;

    case qcc::QCC_RTM_SUSPEND:
        //printf("QCC_RTM_SUSPEND\n");
        proactor.Cancel(readEvent);

        qcc::Close(networkEventFd);
        Register();
        break;

    default:
        break;
    }
}
#endif

