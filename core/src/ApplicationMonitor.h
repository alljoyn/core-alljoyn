/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#ifndef APPLICATIONMONITOR_H_
#define APPLICATIONMONITOR_H_

#include <vector>
#include <map>
#include <SecurityInfo.h>
#include <SecurityInfoListener.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/AutoPinger.h>
#include <alljoyn/MessageReceiver.h>
#include <qcc/String.h>
#include <qcc/GUID.h>

#include "TaskQueue.h"

#include <qcc/Debug.h>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
class ApplicationMonitor :
    public ajn::PingListener,
    public ajn::MessageReceiver {
  private:
    std::map<qcc::String, SecurityInfo> applications;     /* key=busname of app, value = busName */
    std::vector<SecurityInfoListener*> listeners; /*Ownership lies with the application that asks for listener registration*/
    ajn::AutoPinger* pinger;
    ajn::BusAttachment* busAttachment;
    mutable qcc::Mutex securityListenersMutex;

    ApplicationMonitor();
    ApplicationMonitor(ajn::BusAttachment* ba,
                       qcc::String signalIfn);

    void operator=(ApplicationMonitor const&) { }

    QStatus UnmarshalSecuritySignal(Message& msg,
                                    SecurityInfo& secInfo);

    void StateChangedSignalHandler(const InterfaceDescription::Member* member,
                                   const char* sourcePath,
                                   Message& msg);

    void DestinationLost(const qcc::String& group,
                         const qcc::String& destination);

    void DestinationFound(const qcc::String& group,
                          const qcc::String& destination);

    void NotifySecurityInfoListeners(const SecurityInfo* oldSecInfo,
                                     const SecurityInfo* newSecInfo);

  public:
    static ApplicationMonitor* GetApplicationMonitor(ajn::BusAttachment* ba, qcc::String signalIfn)
    {
        return new ApplicationMonitor(ba, signalIfn);
    }

    ~ApplicationMonitor();

    /* Get a list of all aboutOnlyApplications which currently have been discovered */
    std::vector<SecurityInfo> GetApplications() const;

    void RegisterSecurityInfoListener(SecurityInfoListener* al);

    void UnregisterSecurityInfoListener(SecurityInfoListener* al);
};
}
}
#undef QCC_MODULE
#endif /* APPLICATIONLISTENER_H_ */
