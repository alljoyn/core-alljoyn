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

AboutIconProxy::AboutIconProxy(ajn::BusAttachment& bus)
    : m_BusAttachment(&bus)
{
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
}

QStatus AboutIconProxy::GetUrl(const char* busName, qcc::String& url, ajn::SessionId sessionId) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(org::alljoyn::Icon::InterfaceName);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, org::alljoyn::Icon::ObjectPath, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    status = proxyBusObj->AddInterface(*p_InterfaceDescription);
    if (status != ER_OK) {
        delete proxyBusObj;
        return status;
    }
    Message replyMsg(*m_BusAttachment);
    status = proxyBusObj->MethodCall(org::alljoyn::Icon::InterfaceName, "GetUrl", NULL, 0, replyMsg);
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
    delete proxyBusObj;
    return status;
}

QStatus AboutIconProxy::GetIcon(const char* busName, Icon& icon, ajn::SessionId sessionId) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(org::alljoyn::Icon::InterfaceName);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject*proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, org::alljoyn::Icon::ObjectPath, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    status = proxyBusObj->AddInterface(*p_InterfaceDescription);
    if (status != ER_OK) {
        delete proxyBusObj;
        proxyBusObj = NULL;
        return status;
    }
    Message replyMsg(*m_BusAttachment);
    status = proxyBusObj->MethodCall(org::alljoyn::Icon::InterfaceName, "GetContent", NULL, 0, replyMsg);
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
    status = proxyBusObj->GetProperty(org::alljoyn::Icon::InterfaceName, "MimeType", arg);
    if (ER_OK == status) {
        char* temp;
        arg.Get("s", &temp);
        icon.mimetype = temp;
    }

    delete proxyBusObj;
    proxyBusObj = NULL;
    return status;
}

QStatus AboutIconProxy::GetVersion(const char* busName, uint16_t& version, ajn::SessionId sessionId) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(org::alljoyn::Icon::InterfaceName);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, org::alljoyn::Icon::ObjectPath, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    MsgArg arg;
    if (ER_OK == proxyBusObj->AddInterface(*p_InterfaceDescription)) {
        status = proxyBusObj->GetProperty(org::alljoyn::Icon::InterfaceName, "Version", arg);
        if (ER_OK == status) {
            version = arg.v_variant.val->v_int16;
        }
    }
    delete proxyBusObj;
    proxyBusObj = NULL;
    return status;
}

QStatus AboutIconProxy::GetSize(const char* busName, size_t& size, ajn::SessionId sessionId) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(org::alljoyn::Icon::InterfaceName);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject*proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, org::alljoyn::Icon::ObjectPath, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    MsgArg arg;
    if (ER_OK == proxyBusObj->AddInterface(*p_InterfaceDescription)) {
        status = proxyBusObj->GetProperty(org::alljoyn::Icon::InterfaceName, "Size", arg);
        if (ER_OK == status) {
            size = arg.v_variant.val->v_uint64;
        }
    }
    delete proxyBusObj;
    proxyBusObj = NULL;
    return status;
}

QStatus AboutIconProxy::GetMimeType(const char* busName, qcc::String& mimeType, ajn::SessionId sessionId) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;

    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(org::alljoyn::Icon::InterfaceName);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject*proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, org::alljoyn::Icon::ObjectPath, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    MsgArg arg;
    if (ER_OK == proxyBusObj->AddInterface(*p_InterfaceDescription)) {
        status = proxyBusObj->GetProperty(org::alljoyn::Icon::InterfaceName, "MimeType", arg);
        if (ER_OK == status) {
            char* temp;
            arg.Get("s", &temp);
            mimeType.assign(temp);
        }
    }
    delete proxyBusObj;
    proxyBusObj = NULL;
    return status;
}

