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

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/AboutIconProxy.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_ABOUT_ICON_PROXY"

using namespace ajn;

QStatus AboutIconProxy::Icon::SetContent(const MsgArg& arg) {
    m_arg = arg;
    m_arg.Stabilize();
    return m_arg.Get("ay", &contentSize, &content);
}

AboutIconProxy::AboutIconProxy(ajn::BusAttachment& bus, const char* busName, SessionId sessionId)
    : m_BusAttachment(&bus),
    m_aboutIconProxyObj(bus, busName, org::alljoyn::Icon::ObjectPath, sessionId)
{
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    const InterfaceDescription* p_InterfaceDescription = bus.GetInterface(org::alljoyn::Icon::InterfaceName);
    assert(p_InterfaceDescription);
    m_aboutIconProxyObj.AddInterface(*p_InterfaceDescription);
}

QStatus AboutIconProxy::GetUrl(qcc::String& url) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;

    Message replyMsg(*m_BusAttachment);
    status = m_aboutIconProxyObj.MethodCall(org::alljoyn::Icon::InterfaceName, "GetUrl", NULL, 0, replyMsg);
    if (status == ER_OK) {
        const ajn::MsgArg* returnArgs;
        size_t numArgs;
        replyMsg->GetArgs(numArgs, returnArgs);
        if (numArgs == 1) {
            char* temp;
            status = returnArgs[0].Get("s", &temp);
            if (status == ER_OK) {
                url.assign(temp);
            }
        } else {
            status = ER_BUS_BAD_VALUE;
        }
    }
    return status;
}

QStatus AboutIconProxy::GetIcon(Icon& icon) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;

    Message replyMsg(*m_BusAttachment);
    status = m_aboutIconProxyObj.MethodCall(org::alljoyn::Icon::InterfaceName, "GetContent", NULL, 0, replyMsg);
    if (status == ER_OK) {
        const ajn::MsgArg* returnArgs;
        size_t numArgs;
        replyMsg->GetArgs(numArgs, returnArgs);
        if (numArgs == 1) {
            status = icon.SetContent(returnArgs[0]);
        } else {
            status = ER_BUS_BAD_VALUE;
        }
    }

    MsgArg arg;
    status = m_aboutIconProxyObj.GetProperty(org::alljoyn::Icon::InterfaceName, "MimeType", arg);
    if (ER_OK == status) {
        char* temp;
        arg.Get("s", &temp);
        icon.mimetype = temp;
    }

    return status;
}

QStatus AboutIconProxy::GetVersion(uint16_t& version) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = m_aboutIconProxyObj.GetProperty(org::alljoyn::Icon::InterfaceName, "Version", arg);
    if (ER_OK == status) {
        version = arg.v_variant.val->v_int16;
    }
    return status;
}

QStatus AboutIconProxy::GetSize(size_t& size) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = m_aboutIconProxyObj.GetProperty(org::alljoyn::Icon::InterfaceName, "Size", arg);
    if (ER_OK == status) {
        size = arg.v_variant.val->v_uint64;
    }
    return status;
}

QStatus AboutIconProxy::GetMimeType(qcc::String& mimeType) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;

    MsgArg arg;
    status = m_aboutIconProxyObj.GetProperty(org::alljoyn::Icon::InterfaceName, "MimeType", arg);
    if (ER_OK == status) {
        char* temp;
        arg.Get("s", &temp);
        mimeType.assign(temp);
    }
    return status;
}

