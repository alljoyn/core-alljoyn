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
#include <stdio.h>
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
    printf("\n%s AboutClient ObjectDescriptions\n-----------------------------------\n", busName.c_str());
    AboutClient::ObjectDescriptions objectDescriptions;

    if (aboutClient) {
        status = aboutClient->GetObjectDescriptions(busName.c_str(), objectDescriptions, id);
        if (status != ER_OK) {
            printf("Unable get  ObjectDescriptions \n");
        } else {
            for (AboutClient::ObjectDescriptions::const_iterator it = objectDescriptions.begin();
                 it != objectDescriptions.end(); ++it) {
                qcc::String key = it->first;
                std::vector<qcc::String> vector = it->second;
                printf("key=%s", key.c_str());
                for (std::vector<qcc::String>::const_iterator itv = vector.begin(); itv != vector.end(); ++itv) {
                    if (key.compare("/About/DeviceIcon") == 0 && itv->compare("org.alljoyn.Icon") == 0) {
                        hasIconInterface = true;
                    }
                    printf(" value=%s ", itv->c_str());
                }
                printf("\n");
            }
        }

        printf("\n %s AboutClient AboutData Get Supported Languages\n-----------------------------------\n",
               busName.c_str());
        AboutClient::AboutData aboutData;

        std::vector<qcc::String> supportedLanguages;
        status = aboutClient->GetAboutData(busName.c_str(), NULL, aboutData);
        if (status != ER_OK) {
            printf("Unable get  AboutData \n");
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
            printf("\n%s AboutClient AboutData using language=%s\n-----------------------------------\n",
                   busName.c_str(), it->c_str());
            status = aboutClient->GetAboutData(busName.c_str(), it->c_str(), aboutData);
            if (status != ER_OK) {
                printf("Unable get  AboutData \n");
            } else {
                for (AboutClient::AboutData::iterator itx = aboutData.begin(); itx != aboutData.end(); ++itx) {
                    qcc::String key = itx->first;
                    ajn::MsgArg value = itx->second;
                    if (value.typeId == ALLJOYN_STRING) {
                        printf("Key name=%s value=%s\n", key.c_str(), value.v_string.str);
                    } else if (value.typeId == ALLJOYN_ARRAY && value.Signature().compare("as") == 0) {
                        printf("Key name=%s values: ", key.c_str());
                        const MsgArg* stringArray;
                        size_t fieldListNumElements;
                        status = value.Get("as", &fieldListNumElements, &stringArray);
                        for (unsigned int i = 0; i < fieldListNumElements; i++) {
                            char* tempString;
                            stringArray[i].Get("s", &tempString);
                            printf("%s ", tempString);
                        }
                        printf("\n");
                    } else if (value.typeId == ALLJOYN_BYTE_ARRAY) {
                        printf("Key name=%s value:", key.c_str());
                        uint8_t* AppIdBuffer;
                        size_t numElements;
                        value.Get("ay", &numElements, &AppIdBuffer);
                        for (size_t i = 0; i < numElements; i++) {
                            printf("%02X", AppIdBuffer[i]);
                        }
                        printf("\n");
                    }
                }                                     // end of for
            }
        }

        printf("\n%s AboutClient GetVersion\n-----------------------------------\n", busName.c_str());
        int ver;
        status = aboutClient->GetVersion(busName.c_str(), ver, id);
        if (status != ER_OK) {
            printf("Unable get  Version \n");
        } else {
            printf("Version=%d \n", ver);
        }
    } //if (aboutClient)

    if (hasIconInterface) {
        iconClient = new AboutIconClient(*busAttachment);
        if (iconClient) {
            size_t contentSize;
            printf("\n%s AboutIcontClient GetUrl \n-----------------------------------\n", busName.c_str());
            qcc::String url;
            status = iconClient->GetUrl(busName.c_str(), url, id);
            if (status != ER_OK) {
                printf("Unable get  url \n");
            } else {
                printf("url=%s\n", url.c_str());
            }

            printf("\n%s AboutIcontClient GetContent \n-----------------------------------\n",
                   busName.c_str());
            uint8_t* content = NULL;
            status = iconClient->GetContent(busName.c_str(), &content, contentSize, id);
            if (status != ER_OK) {
                printf("Unable get  GetContent \n");
            } else {
                printf("Content size=%zu\n", contentSize);
                printf("Content :\t");
                for (size_t i = 0; i < contentSize; i++) {
                    if (i % 8 == 0 && i > 0)
                        printf("\n\t\t");
                    printf("%02X", content[i]);
                }
                printf("\n");
            }

            printf("\n%s AboutIcontClient GetVersion \n-----------------------------------\n",
                   busName.c_str());
            int ver;
            status = iconClient->GetVersion(busName.c_str(), ver, id);
            if (status != ER_OK) {
                printf("Unable get  Version \n");
            } else {
                printf("Version=%d \n", ver);
            }

            printf("\n%s AboutIcontClient GetMimeType \n-----------------------------------\n",
                   busName.c_str());
            qcc::String mimetype;

            status = iconClient->GetMimeType(busName.c_str(), mimetype, id);
            if (status != ER_OK) {
                printf("Unable get  Mimetype \n");
            } else {
                printf("Mimetype %s \n", mimetype.c_str());
            }

            printf("\n%s AboutIcontClient GetSize \n-----------------------------------\n", busName.c_str());

            status = iconClient->GetSize(busName.c_str(), contentSize, id);
            if (status != ER_OK) {
                printf("Unable get  Size \n");
            } else {
                printf("Size=%zu\n", contentSize);
            }
        } //if (iconClient)
    } //if (isIconInterface)
    status = busAttachment->LeaveSession(id);
    printf("Leaving (%s, ...) status=%s with id = %u\n", busName.c_str(), QCC_StatusText(status), id);

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
        printf("Unable to JoinSession with %s  \n", busName.c_str());
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
    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

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
        printf("BusAttachment started.\n");
    } else {
        printf("unable to connect to Start : %s\n", QCC_StatusText(status));
        return 1;
    }

    status = busAttachment->Connect();

    if (ER_OK == status) {
        printf("Daemon Connect succeeded.\n");
    } else {
        printf("Failed to connect daemon (%s).\n", QCC_StatusText(status));
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
