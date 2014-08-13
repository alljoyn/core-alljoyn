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

#include <qcc/Debug.h>
#include <alljoyn/AboutIconObj.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>

#define QCC_MODULE "ALLJOYN_ABOUT_ICON_OBJ"

using namespace ajn;

const uint16_t AboutIconObj::VERSION = 1;

AboutIconObj::AboutIconObj(ajn::BusAttachment& bus, qcc::String const& mimetype,
                           qcc::String const& url, uint8_t* content, size_t contentSize) :
    BusObject(org::alljoyn::Icon::ObjectPath),
    m_busAttachment(&bus),
    m_MimeType(mimetype),
    m_Url(url),
    m_Content(content),
    m_ContentSize(contentSize)
{
    QCC_DbgTrace(("AboutIconObj::%s", __FUNCTION__));
    QStatus status;
    const InterfaceDescription* intf = NULL;
    if (m_busAttachment) {
        intf = m_busAttachment->GetInterface(org::alljoyn::Icon::InterfaceName);
    }
    assert(intf != NULL);

    status = AddInterface(*intf, true);
    QCC_DbgPrintf(("Add AboutIcon interface %s\n", QCC_StatusText(status)));

    AddMethodHandler(intf->GetMember("GetUrl"),
                     static_cast<MessageReceiver::MethodHandler>(&AboutIconObj::GetUrl));

    AddMethodHandler(intf->GetMember("GetContent"),
                     static_cast<MessageReceiver::MethodHandler>(&AboutIconObj::GetContent));

    status = m_busAttachment->RegisterBusObject(*this);
    QCC_DbgHLPrintf(("AboutObj RegisterBusOBject %s", QCC_StatusText(status)));
}

void AboutIconObj::GetUrl(const ajn::InterfaceDescription::Member* member, ajn::Message& msg) {
    QCC_DbgTrace(("AboutIconObj::%s", __FUNCTION__));
    const ajn::MsgArg* args;
    size_t numArgs;
    msg->GetArgs(numArgs, args);
    if (numArgs == 0) {
        ajn::MsgArg retargs[1];
        QStatus status = retargs[0].Set("s", m_Url.c_str());
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
        QStatus status = retargs[0].Set("ay", m_ContentSize, m_Content);
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
            status = val.Set("s", m_MimeType.c_str());
        } else if (0 == strcmp("Size", propName)) {
            status = val.Set("u", m_ContentSize);
        }
    }
    return status;
}
