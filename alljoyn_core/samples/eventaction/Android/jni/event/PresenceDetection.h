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

#include <alljoyn/Status.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/SessionPortListener.h>
#include <alljoyn/SessionListener.h>
#include <alljoyn/Session.h>
#include <qcc/String.h>
#include <qcc/platform.h>
#include "../Constants.h"
#include <pthread.h>
#include <vector>

#ifndef _PRESENCE_DETECTION_H_
#define _PRESENCE_DETECTION_H_

class PresenceDetection : public ajn::BusAttachment::PingAsyncCB {
  public:
    PresenceDetection(ajn::BusAttachment* busAttachment, JavaVM* vm, jobject jobj);

    ~PresenceDetection();

    void StartPing(qcc::String bus, ajn::SessionPort p);

    void Stop();

  private:
    struct PingContext {
        qcc::String busName;
        ajn::SessionPort port;
        bool isFound;
        PingContext(qcc::String bus, ajn::SessionPort p) : busName(bus), port(p), isFound(0) { }
    };

    static void* PingThreadLoop(void* context);

    void PingCB(QStatus status, void* context);

    pthread_t mThread;
    pthread_mutex_t mLock;
    std::vector<PingContext*> mPingList;

    ajn::BusAttachment* mBusAttachment;
    JavaVM* vm;
    jobject jobj;

};


#endif /* _PRESENCE_DETECTION_H_ */