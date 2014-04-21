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
#include <alljoyn/about/AnnounceHandler.h>
#include <qcc/Debug.h>

using namespace ajn;
using namespace services;

#define QCC_MODULE "ALLJOYN_ABOUT_ANNOUNCE_HANDLER"

AnnounceHandler::AnnounceHandler() :
    announceSignalMember(NULL) {
    QCC_DbgTrace(("AnnounceHandler::%s", __FUNCTION__));
}

void AnnounceHandler::AnnounceSignalHandler(const ajn::InterfaceDescription::Member* member, const char* srcPath,
                                            ajn::Message& message) {

    QCC_DbgTrace(("AnnounceHandler::%s", __FUNCTION__));

    QCC_DbgPrintf(("received signal interface=%s method=%s", message->GetInterface(), message->GetMemberName()));

    if (strcmp(message->GetInterface(), "org.alljoyn.About") != 0 || strcmp(message->GetMemberName(), "Announce") != 0) {
        QCC_DbgPrintf(("This is not the signal we are looking for"));
        return;
    }
    const ajn::MsgArg* args = 0;
    size_t numArgs = 0;
    QStatus status;
    message->GetArgs(numArgs, args);
    if (numArgs == 4) {
#if !defined(NDEBUG)
        for (int i = 0; i < 4; i++) {
            QCC_DbgPrintf(("args[%d]=%s", i, args[i].ToString().c_str()));
        }
#endif
        uint16_t version = 0;
        uint16_t receivedPort = 0;
        AboutData aboutData;
        ObjectDescriptions objectDescriptions;

        status = args[0].Get("q", &version);
        if (status != ER_OK) {
            return;
        }
        status = args[1].Get("q", &receivedPort);
        if (status != ER_OK) {
            return;
        }

        MsgArg* objectDescriptionsArgs;
        size_t objectNum;
        status = args[2].Get("a(oas)", &objectNum, &objectDescriptionsArgs);
        if (status != ER_OK) {
            return;
        }

        for (size_t i = 0; i < objectNum; i++) {
            char* objectDescriptionPath;
            MsgArg* interfaceEntries;
            size_t interfaceNum;
            status = objectDescriptionsArgs[i].Get("(oas)", &objectDescriptionPath, &interfaceNum, &interfaceEntries);
            if (status != ER_OK) {
                return;
            }

            std::vector<qcc::String> localVector;
            for (size_t i = 0; i < interfaceNum; i++) {
                char* interfaceName;
                status = interfaceEntries[i].Get("s", &interfaceName);
                if (status != ER_OK) {
                    return;
                }
                localVector.push_back(interfaceName);
            }
            objectDescriptions.insert(std::pair<qcc::String, std::vector<qcc::String> >(objectDescriptionPath, localVector));
        }
        MsgArg* tempControlArg2;
        size_t languageTagNumElements;
        status = args[3].Get("a{sv}", &languageTagNumElements, &tempControlArg2);
        if (status != ER_OK) {
            return;
        }
        for (size_t i = 0; i < languageTagNumElements; i++) {
            char* tempKey;
            MsgArg* tempValue;
            status = tempControlArg2[i].Get("{sv}", &tempKey, &tempValue);
            if (status != ER_OK) {
                return;
            }
            aboutData.insert(std::pair<qcc::String, ajn::MsgArg>(tempKey, *tempValue));
        }

        Announce(version, receivedPort, message->GetSender(), objectDescriptions, aboutData);
    }
}
