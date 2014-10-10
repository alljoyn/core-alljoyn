/**
 * @file
 * This implments the About class
 */
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

#include <alljoyn/AboutObj.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/MsgArg.h>

#include <qcc/Debug.h>
#include "BusInternal.h"

#define QCC_MODULE "ALLJOYN_ABOUT"

namespace ajn {

const uint16_t AboutObj::VERSION = 1;

AboutObj::AboutObj(ajn::BusAttachment& bus, AnnounceFlag isAboutIntfAnnounced) :
    BusObject(org::alljoyn::About::ObjectPath),
    m_busAttachment(&bus),
    m_objectDescription(),
    m_aboutDataListener(),
    m_announceSerialNumber(0)
{
    const InterfaceDescription* aboutIntf = NULL;

    aboutIntf = m_busAttachment->GetInterface(org::alljoyn::About::InterfaceName);
    assert(aboutIntf);


    QStatus status = AddInterface(*aboutIntf, isAboutIntfAnnounced);
    QCC_DbgPrintf(("Add About interface %s\n", QCC_StatusText(status)));

    if (status == ER_OK) {
        AddMethodHandler(aboutIntf->GetMember("GetAboutData"),
                         static_cast<MessageReceiver::MethodHandler>(&AboutObj::GetAboutData));
        AddMethodHandler(aboutIntf->GetMember("GetObjectDescription"),
                         static_cast<MessageReceiver::MethodHandler>(&AboutObj::GetObjectDescription));
    }

    status = m_busAttachment->RegisterBusObject(*this);
    QCC_DbgHLPrintf(("AboutObj RegisterBusOBject %s", QCC_StatusText(status)));
}

QStatus AboutObj::Announce(SessionPort sessionPort, ajn::AboutDataListener& aboutData)
{
    QCC_DbgTrace(("AboutService::%s", __FUNCTION__));

    if (m_busAttachment->GetInternal().IsSessionPortBound(sessionPort) == false) {
        return ER_ABOUT_SESSIONPORT_NOT_BOUND;
    }

    m_busAttachment->GetInternal().GetAnnouncedObjectDescription(m_objectDescription);
    m_aboutDataListener = &aboutData;
    const InterfaceDescription* aboutIntf = m_busAttachment->GetInterface(org::alljoyn::About::InterfaceName);

    if (!aboutIntf) {
        return ER_BUS_CANNOT_ADD_INTERFACE;
    }

    const ajn::InterfaceDescription::Member* announceSignalMember = aboutIntf->GetMember("Announce");
    assert(announceSignalMember);


    QStatus status = ER_OK;
    if (announceSignalMember == NULL) {
        return ER_FAIL;
    }
    MsgArg announceArgs[4];
    status = announceArgs[0].Set("q", AboutObj::VERSION);
    if (status != ER_OK) {
        QCC_DbgPrintf(("AboutObj::Announce set version failed %s", QCC_StatusText(status)));
        return status;
    }
    status = announceArgs[1].Set("q", sessionPort);
    if (status != ER_OK) {
        QCC_DbgPrintf(("AboutObj::Announce set version set sessionport failed %s", QCC_StatusText(status)));
        return status;
    }
    announceArgs[2] = m_objectDescription;
    m_aboutDataListener->GetMsgArgAnnounce(&announceArgs[3]);

    // TODO ASACORE-1006 check the AboutData to make sure it has all the required fields.
    // if it does not have the required fields return ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD;

    Message msg(*m_busAttachment);
    uint8_t flags = ALLJOYN_FLAG_SESSIONLESS;
#if !defined(NDEBUG)
    for (int i = 0; i < 4; i++) {
        QCC_DbgPrintf(("announceArgs[%d]=%s", i, announceArgs[i].ToString().c_str()));
    }
#endif
    status = Signal(NULL, 0, *announceSignalMember, announceArgs, 4, (unsigned char) 0, flags, &msg);
    m_announceSerialNumber = msg->GetCallSerial();
    QCC_DbgPrintf(("Sent AnnounceSignal from %s  = %s", m_busAttachment->GetUniqueName().c_str(), QCC_StatusText(status)));
    return status;
}

QStatus AboutObj::CancelAnnouncement() {
    if (m_announceSerialNumber == 0) {
        return ER_OK;
    }
    return CancelSessionlessMessage(m_announceSerialNumber);
}

void AboutObj::GetAboutData(const ajn::InterfaceDescription::Member* member, ajn::Message& msg) {
    QCC_DbgTrace(("AboutService::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const ajn::MsgArg* args = NULL;
    size_t numArgs = 0;
    msg->GetArgs(numArgs, args);
    if (numArgs == 1) {
        ajn::MsgArg retargs[1];
        //status = m_PropertyStore->ReadAll(args[0].v_string.str, PropertyStore::READ, retargs[0]);
        QCC_DbgPrintf(("GetAboutData for GetMsgArg for lang=%s", args[0].v_string.str));
        status = m_aboutDataListener->GetMsgArg(retargs, args[0].v_string.str);
        //QCC_DbgPrintf(("m_pPropertyStore->ReadAll(%s,PropertyStore::READ)  =%s", args[0].v_string.str, QCC_StatusText(status)));
        if (status != ER_OK) {
            QCC_DbgPrintf(("AboutService::%s : Call to GetMsgArg failed with %s", __FUNCTION__, QCC_StatusText(status)));
            if (status == ER_LANGUAGE_NOT_SUPPORTED) {
                MethodReply(msg, "org.alljoyn.Error.LanguageNotSupported", "The language specified is not supported");
                return;
            }
            MethodReply(msg, status);
            return;
        } else {
            MethodReply(msg, retargs, 1);
        }
    } else {
        MethodReply(msg, ER_INVALID_DATA);
    }
}

void AboutObj::GetObjectDescription(const ajn::InterfaceDescription::Member* member, ajn::Message& msg) {
    QCC_DbgTrace(("About::%s", __FUNCTION__));
    const ajn::MsgArg* args = 0;
    size_t numArgs = 0;
    msg->GetArgs(numArgs, args);
    if (numArgs == 0) {
        MethodReply(msg, &m_objectDescription, 1);
    } else {
        MethodReply(msg, ER_INVALID_DATA);
    }
}

QStatus AboutObj::Get(const char*ifcName, const char*propName, MsgArg& val) {
    QCC_DbgTrace(("About::%s", __FUNCTION__));
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    if (0 == strcmp(ifcName, org::alljoyn::About::InterfaceName)) {
        if (0 == strcmp("Version", propName)) {
            status = val.Set("q", AboutObj::VERSION);
        }
    }
    return status;
}

} //endnamespace
