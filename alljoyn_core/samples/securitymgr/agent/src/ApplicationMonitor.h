/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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
#include <alljoyn/ApplicationStateListener.h>

#include "SecurityInfo.h"
#include "SecurityInfoListener.h"
#include "TaskQueue.h"

using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
class ApplicationMonitor :
    public ApplicationStateListener {
  private:
    map<string, SecurityInfo> applications; /* key = busName */
    vector<SecurityInfoListener*> listeners; /* no ownership */
    BusAttachment* busAttachment;
    mutable Mutex securityListenersMutex;
    mutable Mutex appsMutex;

    ApplicationMonitor();

    void operator=(ApplicationMonitor const&) { }

    void State(const char* busName,
               const KeyInfoNISTP256& publicKeyInfo,
               PermissionConfigurator::ApplicationState state);

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