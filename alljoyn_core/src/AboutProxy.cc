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


#include <alljoyn/AboutProxy.h>
#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/AboutData.h>
#include <alljoyn/AllJoynStd.h>
#include <qcc/Debug.h>

#include <stdio.h>

#define QCC_MODULE "ALLJOYN_ABOUT"

namespace ajn {


//#define CHECK_BREAK(x) if ((status = x) != ER_OK) { break; }

AboutProxy::AboutProxy(BusAttachment& bus, const char* busName, SessionId sessionId) :
    m_BusAttachment(&bus),
    m_aboutProxyObj(bus, busName, org::alljoyn::About::ObjectPath, sessionId)
{
    QCC_DbgTrace(("AboutProxy::%s", __FUNCTION__));
    const InterfaceDescription* p_InterfaceDescription = bus.GetInterface(org::alljoyn::About::InterfaceName);
    assert(p_InterfaceDescription);
    m_aboutProxyObj.AddInterface(*p_InterfaceDescription);
}

AboutProxy::~AboutProxy()
{
}

QStatus AboutProxy::GetObjectDescription(MsgArg& objectDesc)
{
    QCC_DbgTrace(("AboutProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    Message replyMsg(*m_BusAttachment);
    status = m_aboutProxyObj.MethodCall(org::alljoyn::About::InterfaceName, "GetObjectDescription", NULL, 0, replyMsg);
    if (ER_OK != status) {
        return status;
    }
    const ajn::MsgArg* returnArgs = NULL;
    size_t numArgs = 0;
    replyMsg->GetArgs(numArgs, returnArgs);
    if (numArgs == 1) {
        objectDesc = returnArgs[0];
        // Since the returnArgs are const we cannot change its ownership flags
        // as soon as this function ends the returnArgs will go out of scope and
        // any information it points to will be freed unless we call Stabilize to
        // copy that information.
        objectDesc.Stabilize();
    } else {
        //TODO change to more meaningfull error status
        status = ER_FAIL;
    }
    return status;
}

QStatus AboutProxy::GetAboutData(const char* languageTag, MsgArg& data)
{
    QCC_DbgTrace(("AboutClient::%s", __FUNCTION__));
    QStatus status = ER_OK;

    Message replyMsg(*m_BusAttachment);
    MsgArg args[1];
    status = args[0].Set("s", languageTag);
    if (ER_OK != status) {
        return status;
    }
    status = m_aboutProxyObj.MethodCall(org::alljoyn::About::InterfaceName, "GetAboutData", args, 1, replyMsg);
    if (ER_OK != status) {
        return status;
    }
    const ajn::MsgArg* returnArgs = NULL;
    size_t numArgs = 0;
    replyMsg->GetArgs(numArgs, returnArgs);
    if (numArgs == 1) {
        data = returnArgs[0];
        // Since the returnArgs are const we cannot change its ownership flags
        // as soon as this function ends the returnArgs will go out of scope and
        // any information it points to will be freed unless we call Stabilize to
        // copy that information.
        data.Stabilize();
    } else {
        //TODO change to more meaningfull error status
        status = ER_FAIL;
    }
    return status;
}

QStatus AboutProxy::GetVersion(uint16_t& version)
{
    QCC_DbgTrace(("AboutProxy::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = m_aboutProxyObj.GetProperty(org::alljoyn::About::InterfaceName, "Version", arg);
    if (ER_OK == status) {
        version = arg.v_variant.val->v_int16;
    }
    return status;
}
}
