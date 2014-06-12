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

#include "AboutClientAnnounceHandler.h"
#include <alljoyn/about/AboutClient.h>
#include <iostream>
#include <iomanip>

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
    std::cout << std::endl << std::endl << "*********************************************************************************"
              << std::endl;
    std::cout << "version   " << version << std::endl;
    std::cout << "port      " << port << std::endl;
    std::cout << "busName   " << busName << std::endl;
    std::cout << "ObjectDescriptions :" << std::endl;
    for (AboutClient::ObjectDescriptions::const_iterator it = objectDescs.begin(); it != objectDescs.end(); ++it) {
        qcc::String key = it->first;
        std::vector<qcc::String> vector = it->second;
        std::cout << "Object path = " << key.c_str() << std::endl;
        for (std::vector<qcc::String>::const_iterator itv = vector.begin(); itv != vector.end(); ++itv) {
            std::cout << "\tInterface = " << itv->c_str() << std::endl;
        }
    }

    std::cout << "AnnounceData :" << std::endl;
    for (AboutClient::AboutData::const_iterator it = aboutData.begin(); it != aboutData.end(); ++it) {
        qcc::String key = it->first;
        ajn::MsgArg value = it->second;
        if (value.typeId == ALLJOYN_STRING) {
            std::cout << "Key name = "  << std::setfill(' ') << std::setw(20) << std::left << key.c_str()
                      << " value = " << value.v_string.str << std::endl;
        } else if (value.typeId == ALLJOYN_BYTE_ARRAY) {
            std::cout << "Key name = "  << std::setfill(' ') << std::setw(20) << std::left << key.c_str()
                      << " value = " << std::hex << std::uppercase;
            uint8_t* AppIdBuffer;
            size_t numElements;
            value.Get("ay", &numElements, &AppIdBuffer);
            for (size_t i = 0; i < numElements; i++) {
                std::cout << (unsigned int)AppIdBuffer[i];
            }
            std::cout << std::nouppercase << std::dec << std::endl;
        }
    }

    std::cout << "*********************************************************************************" << std::endl << std::endl;

    if (m_Callback) {
        std::cout << "Calling AnnounceHandler Callback" << std::endl;
        m_Callback(busName, port);
    }
}

