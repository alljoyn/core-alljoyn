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

#include <alljoyn/about/AboutClient.h>
#include <alljoyn/about/AnnouncementRegistrar.h>

#include <signal.h>
#include <iostream>
#include <iomanip>
#include "AboutClientSessionListener.h"
#include "AboutClientAnnounceHandler.h"
#include "AboutClientSessionJoiner.h"

using namespace ajn;
using namespace services;

static volatile sig_atomic_t quit;

static BusAttachment* busAttachment;

static void SignalHandler(int sig)
{
    switch (sig) {
    case SIGINT:
    case SIGTERM:
        quit = 1;
        break;
    }
}

#define SERVICE_EXIT_OK       0
#define SERVICE_OPTION_ERROR  1
#define SERVICE_CONFIG_ERROR  2

void sessionJoinedCallback(qcc::String const& busName, SessionId id)
{
    QStatus status;
    busAttachment->EnableConcurrentCallbacks();
    AboutClient* aboutClient = new AboutClient(*busAttachment);
    AboutIconClient* iconClient = NULL;
    bool hasIconInterface = false;
    std::cout << std::endl << busName.c_str() << " AboutClient ObjectDescriptions" << std::endl;
    std::cout << "-----------------------------------" << std::endl;
    AboutClient::ObjectDescriptions objectDescriptions;

    if (aboutClient) {
        status = aboutClient->GetObjectDescriptions(busName.c_str(), objectDescriptions, id);
        if (status != ER_OK) {
            std::cout << "Call to getObjectDescriptions failed: " << QCC_StatusText(status) << std::endl;
        } else {
            for (AboutClient::ObjectDescriptions::const_iterator it = objectDescriptions.begin();
                 it != objectDescriptions.end(); ++it) {
                qcc::String key = it->first;
                std::vector<qcc::String> vector = it->second;
                std::cout << "key=" << key.c_str();
                for (std::vector<qcc::String>::const_iterator itv = vector.begin(); itv != vector.end(); ++itv) {
                    if (key.compare("/About/DeviceIcon") == 0 && itv->compare("org.alljoyn.Icon") == 0) {
                        hasIconInterface = true;
                    }
                    std::cout << " value=" << itv->c_str() << " ";
                }
                std::cout << std::endl;
            }
        }

        std::cout << std::endl << busName.c_str() << " AboutClient AboutData Get Supported Languages" << std::endl;
        std::cout << "-----------------------------------" << std::endl;

        AboutClient::AboutData aboutData;

        std::vector<qcc::String> supportedLanguages;
        status = aboutClient->GetAboutData(busName.c_str(), NULL, aboutData);
        if (status != ER_OK) {
            std::cout << "Call to getAboutData failed: "  << QCC_StatusText(status) << std::endl;
        } else {
            AboutClient::AboutData::iterator it = aboutData.find("SupportedLanguages");
            if (it != aboutData.end()) {
                const MsgArg* stringArray;
                size_t fieldListNumElements;
                status = it->second.Get("as", &fieldListNumElements, &stringArray);
                for (unsigned int i = 0; i < fieldListNumElements; i++) {
                    char* tempString;
                    stringArray[i].Get("s", &tempString);
                    supportedLanguages.push_back(tempString);
                }
            }
        }

        for (std::vector<qcc::String>::iterator it = supportedLanguages.begin(); it != supportedLanguages.end(); ++it) {
            std::cout << std::endl << busName.c_str() << " AboutClient AboutData using language=" << it->c_str() << std::endl;
            std::cout << "-----------------------------------" << std::endl;
            status = aboutClient->GetAboutData(busName.c_str(), it->c_str(), aboutData);
            if (status != ER_OK) {
                std::cout << "Call to getAboutData failed: " << QCC_StatusText(status) << std::endl;
            } else {
                for (AboutClient::AboutData::iterator itx = aboutData.begin(); itx != aboutData.end(); ++itx) {
                    qcc::String key = itx->first;
                    ajn::MsgArg value = itx->second;
                    if (value.typeId == ALLJOYN_STRING) {
                        std::cout << "Key name=" << key.c_str() << " value=" << value.v_string.str << std::endl;
                    } else if (value.typeId == ALLJOYN_ARRAY && value.Signature().compare("as") == 0) {
                        std::cout << "Key name=" << key.c_str() << " values: ";
                        const MsgArg* stringArray;
                        size_t fieldListNumElements;
                        status = value.Get("as", &fieldListNumElements, &stringArray);
                        for (unsigned int i = 0; i < fieldListNumElements; i++) {
                            char* tempString;
                            stringArray[i].Get("s", &tempString);
                            std::cout << tempString << " ";
                        }
                        std::cout << std::endl;
                    } else if (value.typeId == ALLJOYN_BYTE_ARRAY) {
                        std::cout << "Key name=" << key.c_str() << " value:" << std::hex << std::uppercase << std::setfill('0');
                        uint8_t* AppIdBuffer;
                        size_t numElements;
                        value.Get("ay", &numElements, &AppIdBuffer);
                        for (size_t i = 0; i < numElements; i++) {
                            std::cout <<  std::setw(2) << (unsigned int)AppIdBuffer[i];
                        }
                        std::cout << std::nouppercase << std::dec << std::endl;
                    }
                }                                     // end of for
            }
        }

        std::cout << std::endl << busName.c_str() << " AboutClient GetVersion" << std::endl;
        std::cout << "-----------------------------------" << std::endl;

        int ver;
        status = aboutClient->GetVersion(busName.c_str(), ver, id);
        if (status != ER_OK) {
            std::cout << "Call to to getVersion failed " << QCC_StatusText(status) << std::endl;
        } else {
            std::cout << "Version=" << ver << std::endl;
        }
    } //if (aboutClient)

    if (hasIconInterface) {
        iconClient = new AboutIconClient(*busAttachment);
        if (iconClient) {
            std::cout << std::endl << busName.c_str() << " AboutIcontClient GetUrl" << std::endl;
            std::cout << "-----------------------------------" << std::endl;

            size_t contentSize;
            qcc::String url;
            status = iconClient->GetUrl(busName.c_str(), url, id);
            if (status != ER_OK) {
                std::cout << "Call to getUrl failed: " << QCC_StatusText(status) << std::endl;
            } else {
                std::cout << "url=" << url.c_str() << std::endl;
            }

            std::cout << std::endl << busName.c_str() << " AboutIcontClient GetContent" << std::endl;
            std::cout << "-----------------------------------" << std::endl;

            uint8_t* content = NULL;
            status = iconClient->GetContent(busName.c_str(), &content, contentSize, id);
            if (status != ER_OK) {
                std::cout << "Call to GetContent failed: " << QCC_StatusText(status) << std::endl;
            } else {
                std::cout << "Content size=" << contentSize << std::endl;
                std::cout << "Content :\t";
                for (size_t i = 0; i < contentSize; i++) {
                    if (i % 8 == 0 && i > 0) {
                        std::cout << "\n\t\t";
                    }
                    std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (unsigned int)content[i]
                              << std::nouppercase << std::dec;
                }
                std::cout << std::endl;
            }

            std::cout << std::endl << busName.c_str() << " AboutIcontClient GetVersion" << std::endl;
            std::cout << "-----------------------------------" << std::endl;

            int ver;
            status = iconClient->GetVersion(busName.c_str(), ver, id);
            if (status != ER_OK) {
                std::cout << "Call to getVersion failed: " << QCC_StatusText(status) << std::endl;
            } else {
                std::cout << "Version=" << ver << std::endl;
            }

            std::cout << std::endl << busName.c_str() << " AboutIcontClient GetMimeType" << std::endl;
            std::cout << "-----------------------------------" << std::endl;

            qcc::String mimetype;

            status = iconClient->GetMimeType(busName.c_str(), mimetype, id);
            if (status != ER_OK) {
                std::cout << "Call to getMimetype failed: " << QCC_StatusText(status) << std::endl;
            } else {
                std::cout << "Mimetype" << mimetype.c_str() << std::endl;
            }

            std::cout << std::endl << busName.c_str() << " AboutIcontClient GetSize" << std::endl;
            std::cout << "-----------------------------------" << std::endl;

            status = iconClient->GetSize(busName.c_str(), contentSize, id);
            if (status != ER_OK) {
                std::cout << "Call to getSize failed: " << QCC_StatusText(status) << std::endl;
            } else {
                std::cout << "Size=" << contentSize << std::endl;
            }
        } //if (iconClient)
    } //if (isIconInterface)
    status = busAttachment->LeaveSession(id);
    std::cout << "Leaving session id = " << id << " with " << busName.c_str() << " status: " << QCC_StatusText(status) << std::endl;

    if (aboutClient) {
        delete aboutClient;
        aboutClient = NULL;
    }
    if (iconClient) {
        delete iconClient;
        iconClient = NULL;
    }
}

void announceHandlerCallback(qcc::String const& busName, unsigned short port)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    AboutClientSessionListener* aboutClientSessionListener = new AboutClientSessionListener(busName);
    AboutClientSessionJoiner* joincb = new AboutClientSessionJoiner(busName.c_str(), sessionJoinedCallback);

    QStatus status = busAttachment->JoinSessionAsync(busName.c_str(), (ajn::SessionPort)port, aboutClientSessionListener,
                                                     opts, joincb, aboutClientSessionListener);

    if (status != ER_OK) {
        std::cout << "Unable to JoinSession with " << busName.c_str() << std::endl;
        return;
    }
}

/**
 *  client main function.
 *
 * @return
 *      - 0 if successful.
 *      - 1 if error.
 */
int main(int argc, char**argv, char**envArg)
{
    QStatus status = ER_OK;
    std::cout << "AllJoyn Library version: " << ajn::GetVersion() << std::endl;
    std::cout << "AllJoyn Library build info: " << ajn::GetBuildInfo() << std::endl;

    QCC_SetLogLevels("ALLJOYN_ABOUT_CLIENT=7");
    QCC_SetLogLevels("ALLJOYN_ABOUT_ICON_CLIENT=7");
    QCC_SetLogLevels("ALLJOYN_ABOUT_ANNOUNCE_HANDLER=7");
    QCC_SetLogLevels("ALLJOYN_ABOUT_ANNOUNCEMENT_REGISTRAR=7");

    //set Daemon password only for bundled app
    #ifdef QCC_USING_BD
    PasswordManager::SetCredentials("ALLJOYN_PIN_KEYX", "000000");
    #endif

    // QCC_SetLogLevels("ALLJOYN=7;ALL=1");
    struct sigaction act, oldact;
    sigset_t sigmask, waitmask;

    // Block all signals by default for all threads.
    sigfillset(&sigmask);
    sigdelset(&sigmask, SIGSEGV);
    pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

    // Setup a handler for SIGINT and SIGTERM
    act.sa_handler = SignalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGINT, &act, &oldact);
    sigaction(SIGTERM, &act, &oldact);

    busAttachment = new BusAttachment("AboutClientMain", true);

    status = busAttachment->Start();
    if (status == ER_OK) {
        std::cout << "BusAttachment started." << std::endl;
    } else {
        std::cout << "Unable to start BusAttachment. Status: " << QCC_StatusText(status) << std::endl;
        return 1;
    }

    status = busAttachment->Connect();

    if (ER_OK == status) {
        std::cout << "Daemon Connect succeeded." << std::endl;
    } else {
        std::cout << "Failed to connect daemon. Status: " << QCC_StatusText(status) << std::endl;
        return 1;
    }

    AboutClientAnnounceHandler* announceHandler = new AboutClientAnnounceHandler(announceHandlerCallback);
    AnnouncementRegistrar::RegisterAnnounceHandler(*busAttachment, *announceHandler);

    busAttachment->AddMatch("sessionless='t',type='error'");

    // Setup signals to wait for.
    sigfillset(&waitmask);
    sigdelset(&waitmask, SIGINT);
    sigdelset(&waitmask, SIGTERM);

    quit = 0;

    while (!quit) {
        // Wait for a signal.
        sigsuspend(&waitmask);
    }

    AnnouncementRegistrar::UnRegisterAnnounceHandler(*busAttachment, *announceHandler);
    delete announceHandler;

    busAttachment->Stop();
    delete busAttachment;

    return 0;

} /* main() */
