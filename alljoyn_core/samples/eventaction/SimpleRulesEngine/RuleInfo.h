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

#ifndef _RULE_INFO_
#define _RULE_INFO_

#include <qcc/String.h>
#include <stdio.h>

class RuleInfo {
  public:
    RuleInfo(const char* uniqueName, const char* path, const char* iface,
             const char* member, const char* sig)
        : mUniqueName(uniqueName), mPath(path), mIfaceName(iface),
        mMember(member), mSignature(sig), mDeviceId(qcc::String::Empty), mAppId(qcc::String::Empty), mPort(0)
    { };

    RuleInfo(const char* uniqueName, const char* path, const char* iface,
             const char* member, const char* sig, const char* deviceId, const char* appId, short port)
        : mUniqueName(uniqueName), mPath(path), mIfaceName(iface),
        mMember(member), mSignature(sig), mDeviceId(deviceId), mAppId(appId), mPort(port)
    { };

    virtual ~RuleInfo() { };

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
        char temp[16];
        sprintf(temp, "%d", mPort);
        ret.append(temp);
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
    short mPort;
};

#endif //_RULE_INFO_
