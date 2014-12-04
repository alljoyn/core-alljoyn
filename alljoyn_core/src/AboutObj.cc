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

AboutObj::~AboutObj()
{
    Unannounce();
    m_busAttachment->UnregisterBusObject(*this);
}

QStatus AboutObj::Announce(SessionPort sessionPort, ajn::AboutDataListener& aboutData)
{
    QCC_DbgTrace(("AboutService::%s", __FUNCTION__));

    QStatus status = ER_OK;
    if (m_busAttachment->GetInternal().IsSessionPortBound(sessionPort) == false) {
        return ER_ABOUT_SESSIONPORT_NOT_BOUND;
    }

    m_aboutDataListener = &aboutData;

    MsgArg aboutDataArg;
    status = m_aboutDataListener->GetAboutData(&aboutDataArg, "");
    if (ER_OK != status) {
        return status;
    }

    MsgArg announcedDataArg;
    status = m_aboutDataListener->GetAnnouncedAboutData(&announcedDataArg);
    if (ER_OK != status) {
        return status;
    }

    if (!HasAllRequiredFields(aboutDataArg)) {
        return ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD;
    }

    if (!HasAllAnnouncedFields(announcedDataArg)) {
        return ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD;
    }

    if (!AnnouncedDataAgreesWithAboutData(aboutDataArg, announcedDataArg)) {
        return ER_ABOUT_INVALID_ABOUTDATA_LISTENER;
    }

    // ASACORE-1229
    // We want to return an error if the AppId is is not 128-bits since the
    // announced signal will not pass compliance and certification but we still
    // send out the signal.
    QStatus validate_status = ValidateAboutDataFields(aboutDataArg);
    if (ER_OK != validate_status && ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE != validate_status) {
        return validate_status;
    }
    m_busAttachment->GetInternal().GetAnnouncedObjectDescription(m_objectDescription);

    const InterfaceDescription* aboutIntf = m_busAttachment->GetInterface(org::alljoyn::About::InterfaceName);

    if (!aboutIntf) {
        return ER_BUS_CANNOT_ADD_INTERFACE;
    }

    const ajn::InterfaceDescription::Member* announceSignalMember = aboutIntf->GetMember("Announce");
    assert(announceSignalMember);

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
    announceArgs[3] = announcedDataArg;

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
    if (status != ER_OK) {
        return status;
    }
    return validate_status;
}

QStatus AboutObj::Unannounce() {
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
        status = m_aboutDataListener->GetAboutData(retargs, args[0].v_string.str);
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

bool AboutObj::HasAllRequiredFields(MsgArg& aboutDataArg)
{
    //Required Fields are:
    // AppId
    // DefaultLanguage
    // DeviceId
    // AppName
    // Manufacture
    // ModelNumber
    // SupportedLanguages
    // Description
    // SoftwareVersion
    // AJSoftwareVersion
    QStatus status = ER_FAIL;
    if (aboutDataArg.Signature().compare("a{sv}") != 0) {
        return false;
    }
    MsgArg* field;
    status = aboutDataArg.GetElement("{sv}", AboutData::APP_ID, &field);
    if (ER_OK != status || field->Signature().compare("ay") != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::APP_ID));
        return false;
    }
    status = aboutDataArg.GetElement("{sv}", AboutData::DEFAULT_LANGUAGE, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::DEFAULT_LANGUAGE));
        return false;
    }
    status = aboutDataArg.GetElement("{sv}", AboutData::DEVICE_ID, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::DEVICE_ID));
        return false;
    }
    status = aboutDataArg.GetElement("{sv}", AboutData::APP_NAME, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::APP_NAME));
        return false;
    }
    status = aboutDataArg.GetElement("{sv}", AboutData::MANUFACTURER, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::MANUFACTURER));
        return false;
    }
    status = aboutDataArg.GetElement("{sv}", AboutData::MODEL_NUMBER, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::MODEL_NUMBER));
        return false;
    }
    status = aboutDataArg.GetElement("{sv}", AboutData::SUPPORTED_LANGUAGES, &field);
    if (ER_OK != status || field->Signature().compare("as") != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::SUPPORTED_LANGUAGES));
        return false;
    }
    status = aboutDataArg.GetElement("{sv}", AboutData::DESCRIPTION, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::DESCRIPTION));
        return false;
    }
    status = aboutDataArg.GetElement("{sv}", AboutData::SOFTWARE_VERSION, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::SOFTWARE_VERSION));
        return false;
    }
    status = aboutDataArg.GetElement("{sv}", AboutData::AJ_SOFTWARE_VERSION, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::AJ_SOFTWARE_VERSION));
        return false;
    }
    return true;
}

bool AboutObj::HasAllAnnouncedFields(MsgArg& announcedDataArg)
{
    //Announced Fields are:
    // AppId
    // DefaultLanguage
    // DeviceId
    // AppName
    // Manufacture
    // ModelNumber
    QStatus status = ER_FAIL;
    if (announcedDataArg.Signature().compare("a{sv}") != 0) {
        return false;
    }
    MsgArg* field;
    status = announcedDataArg.GetElement("{sv}", AboutData::APP_ID, &field);
    if (ER_OK != status || field->Signature().compare("ay") != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::APP_ID));
        return false;
    }
    status = announcedDataArg.GetElement("{sv}", AboutData::DEFAULT_LANGUAGE, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::DEFAULT_LANGUAGE));
        return false;
    }
    status = announcedDataArg.GetElement("{sv}", AboutData::DEVICE_ID, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::DEVICE_ID));
        return false;
    }
    status = announcedDataArg.GetElement("{sv}", AboutData::APP_NAME, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::APP_NAME));
        return false;
    }
    status = announcedDataArg.GetElement("{sv}", AboutData::MANUFACTURER, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::MANUFACTURER));
        return false;
    }
    status = announcedDataArg.GetElement("{sv}", AboutData::MODEL_NUMBER, &field);
    if (ER_OK != status || field->Signature().compare(ALLJOYN_STRING) != 0) {
        QCC_LogError(ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD, ("AboutData Missing %s field", AboutData::MODEL_NUMBER));
        return false;
    }
    return true;
}

bool AboutObj::AnnouncedDataAgreesWithAboutData(MsgArg& aboutDataArg, MsgArg& announcedDataArg)
{
    // This code makes some assumptions that the member functions
    // HasAllRequiredFields, and HasAllAnnouncedFields have alread been run
    // and they both return true. Because of this assumption we don't check the
    // return values from the MsgArg.GetElement methods

    //Announced Fields are:
    // AppId
    // DefaultLanguage
    // DeviceId
    // AppName
    // Manufacture
    // ModelNumber
    // DeviceName (optional)
    QStatus status = ER_FAIL;

    MsgArg* field;
    MsgArg* afield;
    aboutDataArg.GetElement("{sv}", AboutData::APP_ID, &field);
    announcedDataArg.GetElement("{sv}", AboutData::APP_ID, &afield);
    if (*field != *afield) {
        QCC_LogError(ER_ABOUT_INVALID_ABOUTDATA_LISTENER, ("AboutDataListner %s field error", AboutData::APP_ID));
        return false;
    }
    aboutDataArg.GetElement("{sv}", AboutData::DEFAULT_LANGUAGE, &field);
    announcedDataArg.GetElement("{sv}", AboutData::DEFAULT_LANGUAGE, &afield);
    if (*field != *afield) {
        QCC_LogError(ER_ABOUT_INVALID_ABOUTDATA_LISTENER, ("AboutDataListner %s field error", AboutData::DEFAULT_LANGUAGE));
        return false;
    }
    aboutDataArg.GetElement("{sv}", AboutData::DEVICE_ID, &field);
    announcedDataArg.GetElement("{sv}", AboutData::DEVICE_ID, &afield);
    if (*field != *afield) {
        QCC_LogError(ER_ABOUT_INVALID_ABOUTDATA_LISTENER, ("AboutDataListner %s field error", AboutData::DEVICE_ID));
        return false;
    }
    aboutDataArg.GetElement("{sv}", AboutData::APP_NAME, &field);
    announcedDataArg.GetElement("{sv}", AboutData::APP_NAME, &afield);
    if (*field != *afield) {
        QCC_LogError(ER_ABOUT_INVALID_ABOUTDATA_LISTENER, ("AboutDataListner %s field error", AboutData::APP_NAME));
        return false;
    }

    aboutDataArg.GetElement("{sv}", AboutData::MANUFACTURER, &field);
    announcedDataArg.GetElement("{sv}", AboutData::MANUFACTURER, &afield);
    if (*field != *afield) {
        QCC_LogError(ER_ABOUT_INVALID_ABOUTDATA_LISTENER, ("AboutDataListner %s field error", AboutData::MANUFACTURER));
        return false;
    }
    aboutDataArg.GetElement("{sv}", AboutData::MODEL_NUMBER, &field);
    announcedDataArg.GetElement("{sv}", AboutData::MODEL_NUMBER, &afield);
    if (*field != *afield) {
        QCC_LogError(ER_ABOUT_INVALID_ABOUTDATA_LISTENER, ("AboutDataListner %s field error", AboutData::MODEL_NUMBER));
        return false;
    }

    status = aboutDataArg.GetElement("{sv}", AboutData::DEVICE_NAME, &field);
    QStatus astatus = announcedDataArg.GetElement("{sv}", AboutData::DEVICE_NAME, &afield);
    if (status == ER_OK && astatus == ER_OK) {
        if (*field != *afield) {
            QCC_LogError(ER_ABOUT_INVALID_ABOUTDATA_LISTENER, ("AboutDataListner %s field error", AboutData::DEVICE_NAME));
            return false;
        }
        // DEVICE_NAME is optional so if is not found we are still good
    } else if (!(status == ER_BUS_ELEMENT_NOT_FOUND && astatus == ER_BUS_ELEMENT_NOT_FOUND)) {
        QCC_LogError(ER_ABOUT_INVALID_ABOUTDATA_LISTENER, ("AboutDataListner %s field error", AboutData::DEVICE_NAME));
        return false;
    }
    return true;
}

QStatus AboutObj::ValidateAboutDataFields(MsgArg& aboutDataArg) {
    MsgArg* field = NULL;
    QStatus status;
    status = aboutDataArg.GetElement("{sv}", AboutData::APP_ID, &field);
    if (ER_OK != status) {
        return status;
    }
    if (field->v_scalarArray.numElements != 16) {
        QCC_DbgPrintf(("ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE - AboutData AppId should be a 128-bit (16-byte) UUID"));
        return ER_ABOUT_INVALID_ABOUTDATA_FIELD_APPID_SIZE;
    }
    return ER_OK;
}
} //endnamespace
