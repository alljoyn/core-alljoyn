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

#ifndef _EVENT_INFO_
#define _EVENT_INFO_

#include <qcc/String.h>

class EventInfo {
  public:
    EventInfo(const char* uniqueName, const char* path, const char* iface,
              const char* member, const char* sig)
        : mUniqueName(uniqueName), mPath(path), mIfaceName(iface),
        mMember(member), mSignature(sig), mDeviceId(qcc::String::Empty), mAppId(qcc::String::Empty)
    { };

    virtual ~EventInfo() { };

    void setSenderInfo(qcc::String deviceId, qcc::String appId) {
        mDeviceId = deviceId;
        mAppId = appId;
    }

    virtual qcc::String toString() {
        qcc::String ret = "";
        ret.append(mUniqueName);
        ret.append(",");
        ret.append(mPath);
        ret.append(",");
        ret.append(mIfaceName);
        ret.append(",");
        ret.append(mMember);
        ret.append(",");
        ret.append(mSignature);
        ret.append(",");
        ret.append(mDeviceId);
        ret.append(",");
        ret.append(mAppId);
        ret.append(",");
        return ret;
    }

//private:
    qcc::String mUniqueName;
    qcc::String mPath;
    qcc::String mIfaceName;
    qcc::String mMember;
    qcc::String mSignature;
    qcc::String mDeviceId;
    qcc::String mAppId;
};

#endif //_EVENT_INFO_
