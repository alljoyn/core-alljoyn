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

