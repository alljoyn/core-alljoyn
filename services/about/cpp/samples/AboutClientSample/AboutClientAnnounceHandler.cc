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

#include "AboutClientAnnounceHandler.h"
#include <alljoyn/about/AboutClient.h>
#include <stdio.h>

using namespace ajn;
using namespace services;

AboutClientAnnounceHandler::AboutClientAnnounceHandler(AnnounceHandlerCallback callback) :
    AnnounceHandler(), m_Callback(callback)
{

}

AboutClientAnnounceHandler::~AboutClientAnnounceHandler()
{
}

void AboutClientAnnounceHandler::Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs,
                                          const AboutData& aboutData)
{
    printf("\n\n*********************************************************************************\n");
    printf("version  %hu\n", version);
    printf("port  %hu\n", port);
    printf("busName  %s\n", busName);
    printf("ObjectDescriptions\n");
    for (AboutClient::ObjectDescriptions::const_iterator it = objectDescs.begin(); it != objectDescs.end(); ++it) {
        qcc::String key = it->first;
        std::vector<qcc::String> vector = it->second;
        printf("key=%s", key.c_str());
        for (std::vector<qcc::String>::const_iterator itv = vector.begin(); itv != vector.end(); ++itv) {
            printf(" value=%s\n", itv->c_str());
        }
        printf("\n");
    }

    printf("Announcedata\n");
    for (AboutClient::AboutData::const_iterator it = aboutData.begin(); it != aboutData.end(); ++it) {
        qcc::String key = it->first;
        ajn::MsgArg value = it->second;
        if (value.typeId == ALLJOYN_STRING) {
            printf("Key name=%s value=%s\n", key.c_str(), value.v_string.str);
        } else if (value.typeId == ALLJOYN_BYTE_ARRAY) {
            printf("Key name=%s value:", key.c_str());
            uint8_t* AppIdBuffer;
            size_t numElements;
            value.Get("ay", &numElements, &AppIdBuffer);
            for (size_t i = 0; i < numElements; i++) {
                printf("%X", AppIdBuffer[i]);
            }
            printf("\n");
        }
    }

    printf("*********************************************************************************\n\n");

    if (m_Callback) {
        printf("Calling AnnounceHandler Callback\n");
        m_Callback(busName, port);
    }
}

