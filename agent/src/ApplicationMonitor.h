/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#ifndef ALLJOYN_SECMGR_APPLICATIONMONITOR_H_
#define ALLJOYN_SECMGR_APPLICATIONMONITOR_H_

#include <vector>
#include <map>
#include <string>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/GUID.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/AutoPinger.h>
#include <alljoyn/ApplicationStateListener.h>

#include "SecurityInfo.h"
#include "SecurityInfoListener.h"
#include "TaskQueue.h"

using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
class ApplicationMonitor :
    public PingListener,
    public ApplicationStateListener {
  private:
    map<string, SecurityInfo> applications; /* key = busName */
    vector<SecurityInfoListener*> listeners; /* no ownership */
    AutoPinger* pinger;
    BusAttachment* busAttachment;
    mutable Mutex securityListenersMutex;
    mutable Mutex appsMutex;

    ApplicationMonitor();

    void operator=(ApplicationMonitor const&) { }

    void State(const char* busName,
               const KeyInfoNISTP256& publicKeyInfo,
               PermissionConfigurator::ApplicationState state);

    void DestinationLost(const String& group,
                         const String& destination);

    void DestinationFound(const String& group,
                          const String& destination);

    void NotifySecurityInfoListeners(const SecurityInfo* oldSecInfo,
                                     const SecurityInfo* newSecInfo);

  public:
    ApplicationMonitor(BusAttachment* ba);

    ~ApplicationMonitor();

    vector<SecurityInfo> GetApplications() const;

    QStatus GetApplication(SecurityInfo& secInfo) const;

    void RegisterSecurityInfoListener(SecurityInfoListener* al);

    void UnregisterSecurityInfoListener(SecurityInfoListener* al);
};
}
}
#endif /* ALLJOYN_SECMGR_APPLICATIONMONITOR_H_ */
