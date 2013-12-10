/******************************************************************************
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/about/AboutIconClient.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_ABOUT_ICON_CLIENT"

using namespace ajn;
using namespace services;

static const char* ABOUT_ICON_OBJECT_PATH = "/About/DeviceIcon";
static const char* ABOUT_ICON_INTERFACE_NAME = "org.alljoyn.Icon";

#define CHECK_BREAK(x) if ((status = x) != ER_OK) break;

AboutIconClient::AboutIconClient(ajn::BusAttachment& bus)
    : m_BusAttachment(&bus)
{
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    const InterfaceDescription* p_InterfaceDescription = NULL;
    QStatus status = ER_OK;
    p_InterfaceDescription = m_BusAttachment->GetInterface(ABOUT_ICON_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        InterfaceDescription* p_InterfaceDescription = NULL;
        status = m_BusAttachment->CreateInterface(ABOUT_ICON_INTERFACE_NAME, p_InterfaceDescription, false);
        if (p_InterfaceDescription && status == ER_OK) {
            do {
                CHECK_BREAK(p_InterfaceDescription->AddMethod("GetUrl", NULL, "s", "url"))
                CHECK_BREAK(p_InterfaceDescription->AddMethod("GetContent", NULL, "ay", "content"))
                CHECK_BREAK(p_InterfaceDescription->AddProperty("Version", "q", (uint8_t) PROP_ACCESS_READ))
                CHECK_BREAK(p_InterfaceDescription->AddProperty("MimeType", "s", (uint8_t) PROP_ACCESS_READ))
                CHECK_BREAK(p_InterfaceDescription->AddProperty("Size", "u", (uint8_t) PROP_ACCESS_READ))
                p_InterfaceDescription->Activate();
                return;
            } while (0);
        }
        QCC_DbgPrintf(("AboutIconClient::AboutIconClient - interface=[%s] could not be created. status=[%s]",
                       ABOUT_ICON_INTERFACE_NAME, QCC_StatusText(status)));
    }
}

QStatus AboutIconClient::GetUrl(const char* busName, qcc::String& url, ajn::SessionId sessionId) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(ABOUT_ICON_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, ABOUT_ICON_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    status = proxyBusObj->AddInterface(*p_InterfaceDescription);
    if (status != ER_OK) {
        delete proxyBusObj;
        return status;
    }
    Message replyMsg(*m_BusAttachment);
    status = proxyBusObj->MethodCall(ABOUT_ICON_INTERFACE_NAME, "GetUrl", NULL, 0, replyMsg);
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

QStatus AboutIconClient::GetContent(const char* busName, uint8_t** content, size_t& contentSize, ajn::SessionId sessionId) {

    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(ABOUT_ICON_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject*proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, ABOUT_ICON_OBJECT_PATH, sessionId);
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
    status = proxyBusObj->MethodCall(ABOUT_ICON_INTERFACE_NAME, "GetContent", NULL, 0, replyMsg);
    if (status == ER_OK) {
        const ajn::MsgArg* returnArgs;
        size_t numArgs;
        replyMsg->GetArgs(numArgs, returnArgs);
        if (numArgs == 1) {
            status = returnArgs[0].Get("ay", &contentSize, content);
        } else {
            status = ER_BUS_BAD_VALUE;
        }
    }
    delete proxyBusObj;
    proxyBusObj = NULL;
    return status;

}

QStatus AboutIconClient::GetVersion(const char* busName, int& version, ajn::SessionId sessionId) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(ABOUT_ICON_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, ABOUT_ICON_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    MsgArg arg;
    if (ER_OK == proxyBusObj->AddInterface(*p_InterfaceDescription)) {
        status = proxyBusObj->GetProperty(ABOUT_ICON_INTERFACE_NAME, "Version", arg);
        if (ER_OK == status) {
            version = arg.v_variant.val->v_int16;
        }
    }
    delete proxyBusObj;
    proxyBusObj = NULL;
    return status;
}

QStatus AboutIconClient::GetSize(const char* busName, size_t& size, ajn::SessionId sessionId) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(ABOUT_ICON_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject*proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, ABOUT_ICON_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    MsgArg arg;
    if (ER_OK == proxyBusObj->AddInterface(*p_InterfaceDescription)) {
        status = proxyBusObj->GetProperty(ABOUT_ICON_INTERFACE_NAME, "Size", arg);
        if (ER_OK == status) {
            size = arg.v_variant.val->v_uint64;
        }
    }
    delete proxyBusObj;
    proxyBusObj = NULL;
    return status;
}

QStatus AboutIconClient::GetMimeType(const char* busName, qcc::String& mimeType, ajn::SessionId sessionId) {
    QCC_DbgTrace(("AboutIcontClient::%s", __FUNCTION__));
    QStatus status = ER_OK;

    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(ABOUT_ICON_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject*proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, ABOUT_ICON_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    MsgArg arg;
    if (ER_OK == proxyBusObj->AddInterface(*p_InterfaceDescription)) {
        status = proxyBusObj->GetProperty(ABOUT_ICON_INTERFACE_NAME, "MimeType", arg);
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

