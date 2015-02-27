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
