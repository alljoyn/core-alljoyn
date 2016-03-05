/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#include <stdio.h>

#ifdef _WIN32
/* Disable deprecation warnings */
#pragma warning(disable: 4996)
#endif

#include <alljoyn/config/ConfigService.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/config/LogModule.h>
#include <alljoyn/AboutData.h>

#define CHECK_RETURN(x) if ((status = x) != ER_OK) { return status; }
#define CHECK_BREAK(x) if ((status = x) != ER_OK) { break; }

using namespace ajn;
using namespace services;

static const char* CONFIG_INTERFACE_NAME = "org.alljoyn.Config";

ConfigService::ConfigService(ajn::BusAttachment& bus, AboutDataStoreInterface& store, Listener& listener) :
    BusObject("/Config"), m_BusAttachment(&bus), m_AboutDataStore(&store), m_PropertyStore(NULL), m_Listener(&listener)
{
    QCC_DbgTrace(("In ConfigService new Constructor"));
}

ConfigService::ConfigService(ajn::BusAttachment& bus, PropertyStore& store, Listener& listener) :
    BusObject("/Config"), m_BusAttachment(&bus), m_AboutDataStore(NULL), m_PropertyStore(&store), m_Listener(&listener)
{
    QCC_DbgTrace(("In ConfigService 14.06 Constructor"));
}

ConfigService::~ConfigService()
{
    QCC_DbgTrace(("In ConfigService destructor"));
}

QStatus ConfigService::Register()
{
    QStatus status = ER_OK;
    QCC_DbgTrace(("In ConfigService Register"));

    InterfaceDescription* intf = const_cast<InterfaceDescription*>(m_BusAttachment->GetInterface(CONFIG_INTERFACE_NAME));
    if (!intf) {
        CHECK_RETURN(m_BusAttachment->CreateInterface(CONFIG_INTERFACE_NAME, intf,
                                                      AJ_IFC_SECURITY_REQUIRED))
        if (!intf) {
            return ER_BUS_CANNOT_ADD_INTERFACE;
        }

        CHECK_RETURN(intf->AddMethod("FactoryReset", NULL, NULL, NULL))
        CHECK_RETURN(intf->AddMemberAnnotation("FactoryReset", org::freedesktop::DBus::AnnotateNoReply, "true"))
        CHECK_RETURN(intf->AddMethod("Restart", NULL, NULL, NULL))
        CHECK_RETURN(intf->AddMemberAnnotation("Restart", org::freedesktop::DBus::AnnotateNoReply, "true"))
        CHECK_RETURN(intf->AddMethod("SetPasscode", "say", NULL, "DaemonRealm,newPasscode"))
        CHECK_RETURN(intf->AddMethod("GetConfigurations", "s", "a{sv}", "languageTag,languages"))
        CHECK_RETURN(intf->AddMethod("UpdateConfigurations", "sa{sv}", NULL, "languageTag,configMap"))
        CHECK_RETURN(intf->AddMethod("ResetConfigurations", "sas", NULL, "languageTag,fieldList"))
        CHECK_RETURN(intf->AddProperty("Version", "q", (uint8_t) PROP_ACCESS_READ))
        intf->Activate();
    } //if (!intf)

    CHECK_RETURN(AddInterface(*intf, ANNOUNCED))
    CHECK_RETURN(AddMethodHandler(intf->GetMember("FactoryReset"),
                                  static_cast<MessageReceiver::MethodHandler>(&ConfigService::FactoryResetHandler)))
    CHECK_RETURN(AddMethodHandler(intf->GetMember("Restart"),
                                  static_cast<MessageReceiver::MethodHandler>(&ConfigService::RestartHandler)))
    CHECK_RETURN(AddMethodHandler(intf->GetMember("SetPasscode"),
                                  static_cast<MessageReceiver::MethodHandler>(&ConfigService::SetPasscodeHandler)))
    CHECK_RETURN(AddMethodHandler(intf->GetMember("GetConfigurations"),
                                  static_cast<MessageReceiver::MethodHandler>(&ConfigService::GetConfigurationsHandler)))
    CHECK_RETURN(AddMethodHandler(intf->GetMember("UpdateConfigurations"),
                                  static_cast<MessageReceiver::MethodHandler>(&ConfigService::UpdateConfigurationsHandler)))
    CHECK_RETURN(AddMethodHandler(intf->GetMember("ResetConfigurations"),
                                  static_cast<MessageReceiver::MethodHandler>(&ConfigService::ResetConfigurationsHandler)))
    return status;
}

void ConfigService::SetPasscodeHandler(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    QCC_DbgTrace(("In ConfigService SetPassCodeHandler"));
    QStatus status = ER_OK;
    const ajn::MsgArg* args = 0;
    size_t numArgs = 0;
    msg->GetArgs(numArgs, args);
    do {
        if (numArgs != 2) {
            break;
        }

        char* newPasscode;
        size_t newPasscodeNumElements;
        CHECK_BREAK(args[1].Get("ay", &newPasscodeNumElements, &newPasscode))

        if (args[0].typeId != ALLJOYN_STRING) {
            break;
        }

        if (newPasscodeNumElements == 0) {
            QCC_LogError(ER_INVALID_DATA, ("Password can not be empty."));
            MethodReply(msg, ER_INVALID_DATA);
            return;
        }

        m_Listener->SetPassphrase(args[0].v_string.str, newPasscodeNumElements, newPasscode, msg.unwrap()->GetSessionId());
        MethodReply(msg, args, 0);
        return;
    } while (0);
    MethodReply(msg, ER_INVALID_DATA);
}

void ConfigService::GetConfigurationsHandler(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    QCC_DbgTrace(("In ConfigService GetConfigurationsHandler"));

    const ajn::MsgArg* args;
    size_t numArgs;
    QStatus status = ER_OK;
    msg->GetArgs(numArgs, args);
    do {
        if (numArgs != 1) {
            break;
        }

        if (args[0].typeId != ALLJOYN_STRING) {
            break;
        }

        ajn::MsgArg writeData[1];
        if (m_AboutDataStore) {
            QCC_DbgTrace(("m_AboutDataStore->ReadAll"));
            CHECK_BREAK(m_AboutDataStore->ReadAll(args[0].v_string.str, DataPermission::WRITE, writeData[0]))
        } else if (m_PropertyStore) {
            QCC_DbgTrace(("m_PropertyStore->ReadAll"));
            CHECK_BREAK(m_PropertyStore->ReadAll(args[0].v_string.str, PropertyStore::WRITE, writeData[0]))
        } else {
            QCC_DbgHLPrintf(("ConfigService::GetConfigurationsHandler no valid pointer"));
        }


        QCC_DbgPrintf(("ReadAll called with PropertyStore::WRITE for language: %s data: %s", args[0].v_string.str ? args[0].v_string.str : "", writeData[0].ToString().c_str() ? writeData[0].ToString().c_str() : ""));

        MethodReply(msg, writeData, 1);
        return;
    } while (0);

    /*
     * This looks hacky. But we need this because ER_LANGUAGE_NOT_SUPPORTED was not a part of
     * AllJoyn Core in 14.06 and is defined in alljoyn/services/about/cpp/inc/alljoyn/about/PropertyStore.h
     * with a value 0xb001 whereas in 14.12 the About support was incorporated in AllJoyn Core and
     * ER_LANGUAGE_NOT_SUPPORTED is now a part of QStatus enum with a value of 0x911a and AboutData
     * returns this if a language is not supported
     */
    if ((status == ER_LANGUAGE_NOT_SUPPORTED) || (status == ((QStatus)0x911a))) {
        MethodReply(msg, "org.alljoyn.Error.LanguageNotSupported", "The language specified is not supported");
    } else if (status != ER_OK) {
        MethodReply(msg, status);
    } else {
        MethodReply(msg, ER_INVALID_DATA);
    }
}

void ConfigService::UpdateConfigurationsHandler(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    QCC_DbgTrace(("In ConfigService UpdateConfigurationsHandler"));

    const ajn::MsgArg* args;
    size_t numArgs;
    QStatus status = ER_OK;
    msg->GetArgs(numArgs, args);
    do {
        if (numArgs != 2) {
            break;
        }

        const MsgArg* configMapDictEntries;
        size_t configMapNumElements;
        char* languageTag;
        CHECK_BREAK(args[0].Get("s", &languageTag))
        CHECK_BREAK(args[1].Get("a{sv}", &configMapNumElements, &configMapDictEntries))

        for (unsigned int i = 0; i < configMapNumElements; i++) {
            char* tempKey;
            MsgArg* tempValue;
            CHECK_BREAK(configMapDictEntries[i].Get("{sv}", &tempKey, &tempValue))
            if (m_AboutDataStore) {
                CHECK_BREAK(m_AboutDataStore->Update(tempKey, languageTag, tempValue))
            } else if (m_PropertyStore) {
                CHECK_BREAK(m_PropertyStore->Update(tempKey, languageTag, tempValue))
            } else {
                QCC_DbgHLPrintf(("%s no valid pointer", __FUNCTION__));
            }

            QCC_DbgPrintf(("Calling update for %s with lang: %s Value: %s", tempKey ? tempKey : "", languageTag ? languageTag : "", tempValue->ToString().c_str() ? tempValue->ToString().c_str() : ""));
        }
        CHECK_BREAK(status)

        MsgArg * args = NULL;
        MethodReply(msg, args, 0);
        return;

    } while (0);

    QCC_DbgHLPrintf(("UpdateConfigurationsHandler Failed"));

    if (status == ER_MAX_SIZE_EXCEEDED) {
        MethodReply(msg, "org.alljoyn.Error.MaxSizeExceeded", "Maximum size exceeded");
    } else if (status == ER_INVALID_VALUE) {
        MethodReply(msg, "org.alljoyn.Error.InvalidValue", "Invalid value");
    } else if (status == ER_FEATURE_NOT_AVAILABLE) {
        MethodReply(msg, "org.alljoyn.Error.FeatureNotAvailable", "Feature not available");
        return;
    } else if ((status == ER_LANGUAGE_NOT_SUPPORTED) || (status == ((QStatus)0x911a))) {
        /*
         * This looks hacky. But we need this because ER_LANGUAGE_NOT_SUPPORTED was not a part of
         * AllJoyn Core in 14.06 and is defined in alljoyn/services/about/cpp/inc/alljoyn/about/PropertyStore.h
         * with a value 0xb001 whereas in 14.12 the About support was incorporated in AllJoyn Core and
         * ER_LANGUAGE_NOT_SUPPORTED is now a part of QStatus enum with a value of 0x911a and AboutData
         * returns this if a language is not supported
         */
        MethodReply(msg, "org.alljoyn.Error.LanguageNotSupported",
                    "The language specified is not supported");
    } else if (status != ER_OK) {
        MethodReply(msg, status);
    } else {
        //default error such as when num params does not match
        MethodReply(msg, ER_INVALID_DATA);
    }
}

void ConfigService::ResetConfigurationsHandler(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    QCC_DbgTrace(("In ConfigService ResetConfigurationsHandler"));

    const ajn::MsgArg* args;
    size_t numArgs;
    QStatus status = ER_OK;
    msg->GetArgs(numArgs, args);
    do {
        if (numArgs != 2) {
            break;
        }

        char* languageTag;
        CHECK_BREAK(args[0].Get("s", &languageTag))

        const MsgArg * stringArray;
        size_t fieldListNumElements;
        CHECK_BREAK(args[1].Get("as", &fieldListNumElements, &stringArray))
        for (unsigned int i = 0; i < fieldListNumElements; i++) {
            char* tempString;
            CHECK_BREAK(stringArray[i].Get("s", &tempString))
            if (m_AboutDataStore) {
                CHECK_BREAK(m_AboutDataStore->Delete(tempString, languageTag))
            } else if (m_PropertyStore) {
                CHECK_BREAK(m_PropertyStore->Delete(tempString, languageTag))
            } else {
                QCC_DbgHLPrintf(("%s no valid pointer", __FUNCTION__));
            }
            QCC_DbgPrintf(("Calling delete for %s with lang: %s", tempString ? tempString : "", languageTag ? languageTag : ""));
        }
        CHECK_BREAK(status)

        MsgArg * args = NULL;
        MethodReply(msg, args, 0);
        return;
    } while (0);

    QCC_DbgHLPrintf(("ResetConfigurationsHandler Failed"));

    if (status == ER_MAX_SIZE_EXCEEDED) {
        MethodReply(msg, "org.alljoyn.Error.MaxSizeExceeded", "Maximum size exceeded");
    } else if (status == ER_INVALID_VALUE) {
        MethodReply(msg, "org.alljoyn.Error.InvalidValue", "Invalid value");
    } else if (status == ER_FEATURE_NOT_AVAILABLE) {
        MethodReply(msg, "org.alljoyn.Error.FeatureNotAvailable", "Feature not available");
    } else if ((status == ER_LANGUAGE_NOT_SUPPORTED) || (status == ((QStatus)0x911a))) {
        /*
         * This looks hacky. But we need this because ER_LANGUAGE_NOT_SUPPORTED was not a part of
         * AllJoyn Core in 14.06 and is defined in alljoyn/services/about/cpp/inc/alljoyn/about/PropertyStore.h
         * with a value 0xb001 whereas in 14.12 the About support was incorporated in AllJoyn Core and
         * ER_LANGUAGE_NOT_SUPPORTED is now a part of QStatus enum with a value of 0x911a and AboutData
         * returns this if a language is not supported
         */
        MethodReply(msg, "org.alljoyn.Error.LanguageNotSupported",
                    "The language specified is not supported");
    } else if (status != ER_OK) {
        MethodReply(msg, status);
    } else {
        //in case number of arguments is different from the one expected
        MethodReply(msg, ER_INVALID_DATA);
    }
}

void ConfigService::FactoryResetHandler(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    QCC_DbgTrace(("In ConfigService FactoryResetHandler"));

    const ajn::MsgArg* args;
    size_t numArgs;
    msg->GetArgs(numArgs, args);
    if (numArgs == 0) {
        m_Listener->FactoryReset();
        //check it the ALLJOYN_FLAG_NO_REPLY_EXPECTED exists if so send response
        if (!(msg->GetFlags() & ALLJOYN_FLAG_NO_REPLY_EXPECTED)) {
            MsgArg* args = NULL;
            MethodReply(msg, args, 0);
        }
    } else {
        //check it the ALLJOYN_FLAG_NO_REPLY_EXPECTED exists if so send response ER_INVALID_DATA
        if (!(msg->GetFlags() & ALLJOYN_FLAG_NO_REPLY_EXPECTED)) {
            MethodReply(msg, ER_INVALID_DATA);
        }
    }
}

void ConfigService::RestartHandler(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_UNUSED(member);
    QCC_DbgTrace(("In ConfigService RestartHandler"));

    const ajn::MsgArg* args;
    size_t numArgs;
    msg->GetArgs(numArgs, args);
    if (numArgs == 0) {
        m_Listener->Restart();
        //check it the ALLJOYN_FLAG_NO_REPLY_EXPECTED exists if so send response
        if (!(msg->GetFlags() & ALLJOYN_FLAG_NO_REPLY_EXPECTED)) {
            MsgArg* args = NULL;
            MethodReply(msg, args, 0);
        }
    } else {
        //check it the ALLJOYN_FLAG_NO_REPLY_EXPECTED exists if so send response ER_INVALID_DATA
        if (!(msg->GetFlags() & ALLJOYN_FLAG_NO_REPLY_EXPECTED)) {
            MethodReply(msg, ER_INVALID_DATA);
        }
    }
}

QStatus ConfigService::Get(const char*ifcName, const char*propName, MsgArg& val)
{
    QCC_DbgTrace(("In ConfigService Get"));
    QStatus status = ER_BUS_NO_SUCH_PROPERTY;
    // Check the requested property and return the value if it exists
    if (0 == strcmp(ifcName, CONFIG_INTERFACE_NAME)) {
        if (0 == strcmp("Version", propName)) {
            status = val.Set("q", 1);
        }
    }
    return status;
}
