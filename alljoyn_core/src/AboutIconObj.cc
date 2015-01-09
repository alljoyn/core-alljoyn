/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#include <qcc/Debug.h>
#include <alljoyn/AboutIconObj.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>

#define QCC_MODULE "ALLJOYN_ABOUT"

using namespace ajn;

const uint16_t AboutIconObj::VERSION = 1;

AboutIconObj::AboutIconObj(ajn::BusAttachment& bus, AboutIcon& icon) :
    BusObject(org::alljoyn::Icon::ObjectPath),
    m_busAttachment(&bus),
    m_icon(&icon)
{
    QCC_DbgTrace(("AboutIconObj::%s", __FUNCTION__));
    QStatus status;
    const InterfaceDescription* intf = NULL;
    if (m_busAttachment) {
        intf = m_busAttachment->GetInterface(org::alljoyn::Icon::InterfaceName);
    }
    assert(intf != NULL);

    status = AddInterface(*intf, ANNOUNCED);
    QCC_DbgPrintf(("Add AboutIcon interface %s\n", QCC_StatusText(status)));
    if (status == ER_OK) {
        AddMethodHandler(intf->GetMember("GetUrl"),
                         static_cast<MessageReceiver::MethodHandler>(&AboutIconObj::GetUrl));

        AddMethodHandler(intf->GetMember("GetContent"),
                         static_cast<MessageReceiver::MethodHandler>(&AboutIconObj::GetContent));

        status = m_busAttachment->RegisterBusObject(*this);
        QCC_DbgHLPrintf(("AboutIconObj RegisterBusOBject %s", QCC_StatusText(status)));
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to register AboutIcon BusObject"));
        }
    } else {
        QCC_LogError(status, ("Failed to Add AboutIcon"));
    }
}

AboutIconObj::~AboutIconObj()
{
    m_busAttachment->UnregisterBusObject(*this);
}

void AboutIconObj::GetUrl(const ajn::InterfaceDescription::Member* member, ajn::Message& msg) {
    QCC_DbgTrace(("AboutIconObj::%s", __FUNCTION__));
    const ajn::MsgArg* args;
    size_t numArgs;
    msg->GetArgs(numArgs, args);
    if (numArgs == 0) {
        ajn::MsgArg retargs[1];
        QStatus status = retargs[0].Set("s", m_icon->url.c_str());
        if (status != ER_OK) {
            MethodReply(msg, status);
        } else {
            MethodReply(msg, retargs, 1);
        }
    } else {
        MethodReply(msg, ER_INVALID_DATA);
    }
}

void AboutIconObj::GetContent(const ajn::InterfaceDescription::Member* member, ajn::Message& msg) {
    QCC_DbgTrace(("AboutIconObj::%s", __FUNCTION__));
    const ajn::MsgArg* args;
    size_t numArgs;
    msg->GetArgs(numArgs, args);
    if (numArgs == 0) {
        ajn::MsgArg retargs[1];
        QStatus status = retargs[0].Set("ay", m_icon->contentSize, m_icon->content);
        if (status != ER_OK) {
            MethodReply(msg, status);
        } else {
            MethodReply(msg, retargs, 1);
        }
    } else {
        MethodReply(msg, ER_INVALID_DATA);
    }
}

QStatus AboutIconObj::Get(const char*ifcName, const char*propName, MsgArg& val) {
    QCC_DbgTrace(("AboutIconObj::%s", __FUNCTION__));
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    if (0 == strcmp(ifcName, org::alljoyn::Icon::InterfaceName)) {
        if (0 == strcmp("Version", propName)) {
            status = val.Set("q", VERSION);
        } else if (0 == strcmp("MimeType", propName)) {
            status = val.Set("s", m_icon->mimetype.c_str());
        } else if (0 == strcmp("Size", propName)) {
            status = val.Set("u", static_cast<uint32_t>(m_icon->contentSize));
        }
    }
    return status;
}
