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

#include <alljoyn/config/ConfigClient.h>
#include <alljoyn/config/LogModule.h>

#define CHECK_BREAK(x) if ((status = x) != ER_OK) { break; }

using namespace ajn;
using namespace services;

static const char* CONFIG_OBJECT_PATH = "/Config";
static const char* CONFIG_INTERFACE_NAME = "org.alljoyn.Config";

ConfigClient::ConfigClient(ajn::BusAttachment& bus) :
    m_BusAttachment(&bus)
{
    QCC_DbgTrace(("In ConfigClient basic Constructor"));

    QStatus status = ER_OK;

    const InterfaceDescription* getIface = NULL;
    getIface = m_BusAttachment->GetInterface(CONFIG_INTERFACE_NAME);
    if (!getIface) {
        InterfaceDescription* createIface = NULL;
        status = m_BusAttachment->CreateInterface(CONFIG_INTERFACE_NAME, createIface, AJ_IFC_SECURITY_REQUIRED);
        if (createIface && status == ER_OK) {
            do {
                CHECK_BREAK(createIface->AddMethod("FactoryReset", NULL, NULL, NULL))
                CHECK_BREAK(createIface->AddMemberAnnotation("FactoryReset", org::freedesktop::DBus::AnnotateNoReply, "true"))
                CHECK_BREAK(createIface->AddMethod("Restart", NULL, NULL, NULL))
                CHECK_BREAK(createIface->AddMemberAnnotation("Restart", org::freedesktop::DBus::AnnotateNoReply, "true"))
                CHECK_BREAK(createIface->AddMethod("SetPasscode", "say", NULL, "daemonRealm,newPasscode", 0))
                CHECK_BREAK(createIface->AddMethod("GetConfigurations", "s", "a{sv}", "languageTag,configData", 0))
                CHECK_BREAK(createIface->AddMethod("UpdateConfigurations", "sa{sv}", NULL, "languageTag,configMap", 0))
                CHECK_BREAK(createIface->AddMethod("ResetConfigurations", "sas", NULL, "languageTag,fieldList", 0))
                CHECK_BREAK(createIface->AddProperty("Version", "q", PROP_ACCESS_READ))
                createIface->Activate();
                return;
            } while (0);
        } //if (createIface)
        QCC_LogError(status, ("ConfigClientInterface could not be created."));
    } //if (!getIface)
}

QStatus ConfigClient::FactoryReset(const char* busName, ajn::SessionId sessionId)
{
    QCC_DbgTrace(("In ConfigClient FactoryReset"));

    QStatus status = ER_OK;

    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(CONFIG_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }

    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, CONFIG_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    status = proxyBusObj->AddInterface(*p_InterfaceDescription);
    if (status == ER_OK) {
        status = proxyBusObj->MethodCall(CONFIG_INTERFACE_NAME, "FactoryReset", NULL, 0);
    }
    delete proxyBusObj;
    return status;
}

QStatus ConfigClient::Restart(const char* busName, ajn::SessionId sessionId)
{
    QCC_DbgTrace(("In ConfigClient Restart"));
    QStatus status = ER_OK;

    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(CONFIG_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }

    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, CONFIG_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    status = proxyBusObj->AddInterface(*p_InterfaceDescription);
    if (status == ER_OK) {
        status = proxyBusObj->MethodCall(CONFIG_INTERFACE_NAME, "Restart", NULL, 0);
    }
    delete proxyBusObj;
    return status;
}

QStatus ConfigClient::SetPasscode(const char* busName, const char* daemonRealm, size_t newPasscodeSize,
                                  const uint8_t* newPasscode, ajn::SessionId sessionId)
{
    QCC_DbgTrace(("In ConfigClient SetPasscode"));
    QStatus status = ER_OK;

    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(CONFIG_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }

    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, CONFIG_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    do {
        CHECK_BREAK(proxyBusObj->AddInterface(*p_InterfaceDescription))
        Message replyMsg(*m_BusAttachment);
        MsgArg args[2];
        CHECK_BREAK(args[0].Set("s", daemonRealm))
        CHECK_BREAK(args[1].Set("ay", newPasscodeSize, newPasscode))
        CHECK_BREAK(proxyBusObj->MethodCall(CONFIG_INTERFACE_NAME, "SetPasscode", args, 2, replyMsg))
    } while (0);
    delete proxyBusObj;
    return status;
}

QStatus ConfigClient::GetConfigurations(const char* busName, const char* languageTag, Configurations& configs,
                                        ajn::SessionId sessionId)
{
    QCC_DbgTrace(("In ConfigClient GetConfigurations"));
    QStatus status = ER_OK;

    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(CONFIG_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }

    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, CONFIG_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    do {
        CHECK_BREAK(proxyBusObj->AddInterface(*p_InterfaceDescription))

        Message replyMsg(*m_BusAttachment);

        MsgArg args[1];
        CHECK_BREAK(args[0].Set("s", languageTag))
        status = proxyBusObj->MethodCall(CONFIG_INTERFACE_NAME, "GetConfigurations", args, 1, replyMsg);

        if (status == ER_OK) {

            const ajn::MsgArg* returnArgs = 0;
            size_t numArgs = 0;

            replyMsg->GetArgs(numArgs, returnArgs);
            if (numArgs == 1) {
                int languageTagNumElements;
                MsgArg* tempconfigMapDictEntries;
                CHECK_BREAK(returnArgs[0].Get("a{sv}", &languageTagNumElements, &tempconfigMapDictEntries))
                configs.clear();
                for (int i = 0; i < languageTagNumElements; i++) {
                    char* tempKey;
                    MsgArg* tempValue;
                    CHECK_BREAK(tempconfigMapDictEntries[i].Get("{sv}", &tempKey, &tempValue))
                    configs.insert(std::pair<qcc::String, ajn::MsgArg>(tempKey, *tempValue));

                }
            } else {
                status = ER_BUS_BAD_VALUE;
            }
        } else if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
#if !defined(NDEBUG)
            qcc::String errorMessage;
            const char* errorName = replyMsg->GetErrorName(&errorMessage);
#endif
            QCC_LogError(status, ("GetConfigurations errorName:%s errorMessage: %s", errorName ? errorName : "", errorMessage.c_str() ? errorMessage.c_str() : ""));
        }
    } while (0);
    delete proxyBusObj;
    return status;
}

QStatus ConfigClient::UpdateConfigurations(const char* busName, const char* languageTag, const Configurations& configs,
                                           ajn::SessionId sessionId)
{
    QCC_DbgTrace(("In ConfigClient UpdateConfigurations"));
    QStatus status = ER_OK;

    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(CONFIG_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }

    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, CONFIG_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    do {
        CHECK_BREAK(proxyBusObj->AddInterface(*p_InterfaceDescription))

        Message replyMsg(*m_BusAttachment);
        MsgArg args[2];
        CHECK_BREAK(args[0].Set("s", languageTag))

        std::vector<MsgArg> tempconfigMapDictEntries(configs.size());
        int i = 0;
        for (std::map<qcc::String, ajn::MsgArg>::const_iterator it = configs.begin(); it != configs.end(); ++it) {
            MsgArg* arg = new MsgArg(it->second);
            if ((status = tempconfigMapDictEntries[i].Set("{sv}", it->first.c_str(), arg)) != ER_OK) {
                delete arg;
                break;
            }
            tempconfigMapDictEntries[i].SetOwnershipFlags(MsgArg::OwnsArgs, true);
            i++;
        }
        if (status != ER_OK) {
            break;
        }

        CHECK_BREAK(args[1].Set("a{sv}", i, tempconfigMapDictEntries.data()))
        status = proxyBusObj->MethodCall(CONFIG_INTERFACE_NAME, "UpdateConfigurations", args, 2, replyMsg);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
#if !defined(NDEBUG)
            qcc::String errorMessage;
            const char* errorName = replyMsg->GetErrorName(&errorMessage);
#endif
            QCC_LogError(status, ("UpdateConfigurations errorName:%s errorMessage: %s", errorName ? errorName : "", errorMessage.c_str() ? errorMessage.c_str() : ""));
        }
    } while (0);
    delete proxyBusObj;
    return status;
}

QStatus ConfigClient::ResetConfigurations(const char* busName, const char* languageTag,
                                          const std::vector<qcc::String>& configNames, ajn::SessionId sessionId)
{
    QCC_DbgTrace(("In ConfigClient ResetConfigurations"));

    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(CONFIG_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }

    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, CONFIG_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    do {
        CHECK_BREAK(proxyBusObj->AddInterface(*p_InterfaceDescription))

        Message replyMsg(*m_BusAttachment);
        MsgArg args[2];
        CHECK_BREAK(args[0].Set("s", languageTag))
        if (!(configNames.size() == 0)) {
            std::vector<const char*> tempKeys(configNames.size());

            int i = 0;
            for (std::vector<qcc::String>::const_iterator it = configNames.begin(); it != configNames.end(); ++it) {
                tempKeys[i] = it->c_str();
                i++;
            }
            CHECK_BREAK(args[1].Set("as", i, tempKeys.data()))
            status = proxyBusObj->MethodCall(CONFIG_INTERFACE_NAME, "ResetConfigurations", args, 2, replyMsg);
            if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
#if !defined(NDEBUG)
                qcc::String errorMessage;
                const char* errorName = replyMsg->GetErrorName(&errorMessage);
#endif
                QCC_LogError(status, ("ResetConfigurations errorName:%s errorMessage: %s", errorName ? errorName : "", errorMessage.c_str() ? errorMessage.c_str() : ""));
            }
        } else {
            status = ER_INVALID_DATA;
        }
    } while (0);
    delete proxyBusObj;
    return status;
}

QStatus ConfigClient::GetVersion(const char* busName, int& version, ajn::SessionId sessionId)
{
    QCC_DbgTrace(("In ConfigClient GetVersion"));

    QStatus status = ER_OK;

    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(CONFIG_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }

    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, CONFIG_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    do {
        MsgArg arg;
        CHECK_BREAK(proxyBusObj->AddInterface(*p_InterfaceDescription))
        CHECK_BREAK(proxyBusObj->GetProperty(CONFIG_INTERFACE_NAME, "Version", arg))
        version = arg.v_variant.val->v_int16;
    } while (0);
    delete proxyBusObj;
    return status;
}
