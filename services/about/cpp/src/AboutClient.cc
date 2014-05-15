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

#include <stdio.h>
#include <alljoyn/about/AboutClient.h>
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_ABOUT_CLIENT"

using namespace ajn;
using namespace services;

static const char* ABOUT_OBJECT_PATH = "/About";
static const char* ABOUT_INTERFACE_NAME = "org.alljoyn.About";

AboutClient::AboutClient(ajn::BusAttachment& bus) :
    m_BusAttachment(&bus)
{
    QCC_DbgTrace(("AboutClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = NULL;
    p_InterfaceDescription = m_BusAttachment->GetInterface(ABOUT_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        InterfaceDescription* p_InterfaceDescription = NULL;
        status = m_BusAttachment->CreateInterface(ABOUT_INTERFACE_NAME, p_InterfaceDescription, false);
        if (p_InterfaceDescription && status == ER_OK) {
            do {
                status = p_InterfaceDescription->AddMethod("GetAboutData", "s", "a{sv}", "languageTag,aboutData");
                if (status != ER_OK) {
                    break;
                }
                status = p_InterfaceDescription->AddMethod("GetObjectDescription", NULL, "a(oas)", "Control");
                if (status != ER_OK) {
                    break;
                }
                status = p_InterfaceDescription->AddProperty("Version", "q", PROP_ACCESS_READ);
                if (status != ER_OK) {
                    break;
                }
                status = p_InterfaceDescription->AddSignal("Announce", "qqa(oas)a{sv}", "version,port,objectDescription,servMetadata", 0);
                if (status != ER_OK) {
                    break;
                }
                p_InterfaceDescription->Activate();
                return;
            } while (0);
        }
        QCC_DbgPrintf(("AboutClient::AboutClient - interface=[%s] could not be created. status=[%s]",
                       ABOUT_INTERFACE_NAME, QCC_StatusText(status)));
    }
}

AboutClient::~AboutClient()
{
}

QStatus AboutClient::GetObjectDescriptions(const char* busName, AboutClient::ObjectDescriptions& objectDescs,
                                           ajn::SessionId sessionId)
{
    QCC_DbgTrace(("AboutClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(ABOUT_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }

    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, ABOUT_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }

    do {
        status = proxyBusObj->AddInterface(*p_InterfaceDescription);
        if (status != ER_OK) {
            break;
        }

        Message replyMsg(*m_BusAttachment);
        status = proxyBusObj->MethodCall(ABOUT_INTERFACE_NAME, "GetObjectDescription", NULL, 0, replyMsg);
        if (status != ER_OK) {
            break;
        }

        const ajn::MsgArg* returnArgs = 0;
        size_t numArgs = 0;
        replyMsg->GetArgs(numArgs, returnArgs);
        if (numArgs == 1) {
            MsgArg* objectDescriptionsArgs;
            size_t objectNum;
            status = returnArgs[0].Get("a(oas)", &objectNum, &objectDescriptionsArgs);
            if (status != ER_OK) {
                break;
            }

            for (size_t i = 0; i < objectNum; i++) {
                char* objectDescriptionPath;
                MsgArg* interfaceEntries;
                size_t interfaceNum;
                status = objectDescriptionsArgs[i].Get("(oas)", &objectDescriptionPath, &interfaceNum, &interfaceEntries);
                if (status != ER_OK) {
                    break;
                }

                std::vector<qcc::String> localVector;
                for (size_t i = 0; i < interfaceNum; i++) {
                    char* interfaceName;
                    status = interfaceEntries[i].Get("s", &interfaceName);
                    if (status != ER_OK) {
                        break;
                    }
                    localVector.push_back(interfaceName);
                }
                if (status != ER_OK) {
                    break;
                }
                objectDescs.insert(std::pair<qcc::String, std::vector<qcc::String> >(objectDescriptionPath, localVector));
            }
        }
    } while (0);

    delete proxyBusObj;
    proxyBusObj = NULL;
    return status;
}

QStatus AboutClient::GetAboutData(const char* busName, const char* languageTag, AboutClient::AboutData& data,
                                  ajn::SessionId sessionId)
{
    QCC_DbgTrace(("AboutClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(ABOUT_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, ABOUT_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    do {
        status = proxyBusObj->AddInterface(*p_InterfaceDescription);
        if (status != ER_OK) {
            break;
        }

        Message replyMsg(*m_BusAttachment);
        MsgArg args[1];
        status = args[0].Set("s", languageTag);
        if (status != ER_OK) {
            break;
        }
        status = proxyBusObj->MethodCall(ABOUT_INTERFACE_NAME, "GetAboutData", args, 1, replyMsg);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            qcc::String errorMessage;
            QCC_LogError(status, ("GetAboutData ::Error name=%s ErorrMessage=%s", replyMsg->GetErrorName(&errorMessage),
                                  errorMessage.c_str()));
        }
        if (status != ER_OK) {
            break;
        }

        const ajn::MsgArg* returnArgs = 0;
        size_t numArgs = 0;
        replyMsg->GetArgs(numArgs, returnArgs);
        if (numArgs == 1) {
            size_t languageTagNumElements;
            MsgArg* tempControlArg;
            status = returnArgs[0].Get("a{sv}", &languageTagNumElements, &tempControlArg);
            if (status != ER_OK) {
                break;
            }
            data.clear();
            for (unsigned int i = 0; i < languageTagNumElements; i++) {
                char* tempKey;
                MsgArg* tempValue;
                status = tempControlArg[i].Get("{sv}", &tempKey, &tempValue);
                if (status != ER_OK) {
                    break;
                }
                data.insert(std::pair<qcc::String, ajn::MsgArg>(tempKey, *tempValue));
            }
        }
    } while (0);

    delete proxyBusObj;
    proxyBusObj = NULL;
    return status;
}

QStatus AboutClient::GetVersion(const char* busName, int& version, ajn::SessionId sessionId)
{
    QCC_DbgTrace(("AboutClient::%s", __FUNCTION__));
    QStatus status = ER_OK;
    const InterfaceDescription* p_InterfaceDescription = m_BusAttachment->GetInterface(ABOUT_INTERFACE_NAME);
    if (!p_InterfaceDescription) {
        return ER_FAIL;
    }
    ProxyBusObject* proxyBusObj = new ProxyBusObject(*m_BusAttachment, busName, ABOUT_OBJECT_PATH, sessionId);
    if (!proxyBusObj) {
        return ER_FAIL;
    }
    MsgArg arg;
    if (ER_OK == proxyBusObj->AddInterface(*p_InterfaceDescription)) {
        status = proxyBusObj->GetProperty(ABOUT_INTERFACE_NAME, "Version", arg);
        if (ER_OK == status) {
            version = arg.v_variant.val->v_int16;
        }
    }
    delete proxyBusObj;
    proxyBusObj = NULL;
    return status;
}

