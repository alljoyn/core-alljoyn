/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#ifndef _EVENT_INFO_
#define _EVENT_INFO_

#include "RuleInfo.h"

class EventInfo : public RuleInfo {
  public:
    EventInfo(const char* uniqueName, const char* path, const char* iface,
              const char* member, const char* sig)
        : RuleInfo(uniqueName, path, iface, member, sig), mDeviceId(qcc::String::Empty), mAppId(qcc::String::Empty)
    { };

    EventInfo(const char* uniqueName, const char* path, const char* iface,
              const char* member, const char* sig, const char* deviceId, const char* appId)
        : RuleInfo(uniqueName, path, iface, member, sig), mDeviceId(deviceId), mAppId(appId)
    { };

    ~EventInfo() { };

    void setSenderInfo(qcc::String deviceId, qcc::String appId) {
        mDeviceId = deviceId;
        mAppId = appId;
    }

    qcc::String toString() {
        qcc::String ret = RuleInfo::toString();
        ret.append(",");
        ret.append(mDeviceId);
        ret.append(",");
        ret.append(mAppId);
        return ret;
    }

//private:
    qcc::String mDeviceId;
    qcc::String mAppId;
};

#endif //_EVENT_INFO_

