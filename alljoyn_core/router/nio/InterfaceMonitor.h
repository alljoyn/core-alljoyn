/*
 * InterfaceMonitor.h
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
#ifndef _ALLJOYN_INTERFACEMONITOR_H_
#define _ALLJOYN_INTERFACEMONITOR_H_

#ifndef __cplusplus
#error Only include TCPTransport.h in C++ code.
#endif

#include "SocketReadableEvent.h"

#include <list>
#include <map>
#include <mutex>
#include <functional>
#include <memory>

#include <qcc/IfConfig.h>

#ifdef QCC_OS_GROUP_WINDOWS
#include <ws2ipdef.h>
#include <iphlpapi.h>
#endif

namespace nio {

class Proactor;

class InterfaceMonitor {
  public:

    typedef std::function<void (const std::vector<qcc::IfConfigEntry>&)> IFCallback;

    InterfaceMonitor(Proactor& p);

    virtual ~InterfaceMonitor();

    void AddCallback(IFCallback cb);

  private:

    void Notify(const std::vector<qcc::IfConfigEntry>& entries);

    void DoInterfaceCheck();

    Proactor& proactor;

    void Register();

#ifdef QCC_OS_GROUP_POSIX
    void OnChange(qcc::SocketFd sock, QStatus status);

    qcc::SocketFd networkEventFd;
    std::shared_ptr<SocketReadableEvent> readEvent;
#elif defined QCC_OS_GROUP_WINDOWS
    HANDLE networkIfaceHandle;

    static VOID WINAPI IpInterfaceChangeCallback(PVOID arg, PMIB_IPINTERFACE_ROW row, MIB_NOTIFICATION_TYPE notificationType);
#endif

    typedef std::list<IFCallback> InterfaceMonitorCallbackList;
    InterfaceMonitorCallbackList m_callbacks;
    std::mutex m_callbacksLock;
};

} // namespace ajn

#endif /* _ALLJOYN_INTERFACEMONITOR_H_ */
