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

#include <alljoyn/about/AboutClient.h>
#include <alljoyn/about/AnnouncementRegistrar.h>

#include <stdio.h>
#include <signal.h>
#include <iostream>
#include <iomanip>
#include <deque>
#include "AboutClientSessionListener.h"
#include "AboutClientAnnounceHandler.h"
#include "AboutClientSessionJoiner.h"

using namespace ajn;
using namespace services;

static BusAttachment* busAttachment;

static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig)
{
    s_interrupt = true;
}

#define SERVICE_EXIT_OK       0
#define SERVICE_OPTION_ERROR  1
#define SERVICE_CONFIG_ERROR  2

// when pinging a remote bus wait a max of 5 seconds
#define PING_WAIT_TIME    5000

struct JoinedSession {
    qcc::String busName;
    SessionId id;
    JoinedSession(qcc::String const& busName, SessionId id) : busName(busName), id(id) { }
};

static std::deque<JoinedSession> s_joinedSessions;

void sessionJoinedCallback(qcc::String const& busName, SessionId id)
{
    /*
     * If we start calling remote methods from inside an AllJoyn callback we must
     * call the BusAttachment.EnableConcurrentCallbacks() method. There is a
     * limit to the maximum number of concurrent callbacks that can be called
     * at any time. The default limit is 4 but can be changed when the
     * BusAttachment is created.
     *
     * Depending on how many devices are advertising an Announce signal its possible
     * to quickly join lots of sessions one after another.  If we start calling
     * remote methods from the callback we could cause a deadlock situation where
     * there are no threads to respond to method responses. The threads are all
     * busy up making method calls and waiting on the response they can't not receive.
     *
     * To prevent this deadlock situation we create a list of BusAttachements
     * that have formed a session an the session id.
     *
     * This list of sessions will checked by the main wait loop to see if any
     * new session has been added to the list.  If so it will call all the methods
     * associated with the AboutSerivice in the order the sessions were formed.
     * Once that is done we leave the session.
     */
    s_joinedSessions.push_back(JoinedSession(busName, id));
}

void ViewAboutServiceData(qcc::String const& busName, SessionId id) {
    QStatus status;
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
                std::cout << "Object Path = " << key.c_str() << std::endl;
                for (std::vector<qcc::String>::const_iterator itv = vector.begin(); itv != vector.end(); ++itv) {
                    if (key.compare("/About/DeviceIcon") == 0 && itv->compare("org.alljoyn.Icon") == 0) {
                        hasIconInterface = true;
                    }
                    std::cout << "\tInterface = " << itv->c_str() << std::endl;
                }
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
                        std::cout << "Key name = " << std::setfill(' ') << std::setw(20) << std::left << key.c_str()
                                  << " value = " << value.v_string.str << std::endl;
                    } else if (value.typeId == ALLJOYN_ARRAY && value.Signature().compare("as") == 0) {
                        std::cout << "Key name = " << std::setfill(' ') << std::setw(20) << std::left << key.c_str() << " values: ";
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
                        std::cout << "Key name = " << std::setfill(' ') << std::setw(20) << std::left << key.c_str()
                                  << " value = " << std::hex << std::uppercase << std::setfill('0');
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
            std::cout << "Version = " << ver << std::endl;
        }
    } //if (aboutClient)

    if (hasIconInterface) {
        iconClient = new AboutIconClient(*busAttachment);
        if (iconClient) {
            std::cout << std::endl << busName.c_str() << " AboutIconClient GetUrl" << std::endl;
            std::cout << "-----------------------------------" << std::endl;

            size_t contentSize;
            qcc::String url;
            status = iconClient->GetUrl(busName.c_str(), url, id);
            if (status != ER_OK) {
                std::cout << "Call to getUrl failed: " << QCC_StatusText(status) << std::endl;
            } else {
                std::cout << "url = " << url.c_str() << std::endl;
            }

            std::cout << std::endl << busName.c_str() << " AboutIconClient GetVersion" << std::endl;
            std::cout << "-----------------------------------" << std::endl;

            int ver;
            status = iconClient->GetVersion(busName.c_str(), ver, id);
            if (status != ER_OK) {
                std::cout << "Call to getVersion failed: " << QCC_StatusText(status) << std::endl;
            } else {
                std::cout << "Version = " << ver << std::endl;
            }

            std::cout << std::endl << busName.c_str() << " AboutIconClient GetMimeType" << std::endl;
            std::cout << "-----------------------------------" << std::endl;

            qcc::String mimetype;

            status = iconClient->GetMimeType(busName.c_str(), mimetype, id);
            if (status != ER_OK) {
                std::cout << "Call to getMimetype failed: " << QCC_StatusText(status) << std::endl;
            } else {
                std::cout << "Mimetype : " << mimetype.c_str() << std::endl;
            }

            std::cout << std::endl << busName.c_str() << " AboutIconClient GetSize" << std::endl;
            std::cout << "-----------------------------------" << std::endl;

            status = iconClient->GetSize(busName.c_str(), contentSize, id);
            if (status != ER_OK) {
                std::cout << "Call to getSize failed: " << QCC_StatusText(status) << std::endl;
            } else {
                std::cout << "Size = " << contentSize << std::endl;
            }

            std::cout << std::endl << busName.c_str() << " AboutIconClient GetIcon" << std::endl;
            std::cout << "-----------------------------------" << std::endl;
            AboutIconClient::Icon icon;
            status = iconClient->GetIcon(busName.c_str(), icon, id);
            if (status != ER_OK) {
                std::cout << "Call to GetIcon failed: " << QCC_StatusText(status) << std::endl;
            } else {
                std::cout << "Content size = " << icon.contentSize << std::endl;
                std::cout << "Content :\t";
                for (size_t i = 0; i < contentSize; i++) {
                    if (i % 8 == 0 && i > 0) {
                        std::cout << "\n\t\t";
                    }
                    std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (unsigned int)icon.content[i]
                              << std::nouppercase << std::dec;
                }
                std::cout << std::endl;
                std::cout << "Mimetype :\t" << icon.mimetype.c_str() << std::endl;
            }

        } //if (iconClient)
    } //if (isIconInterface)

    if (aboutClient) {
        delete aboutClient;
        aboutClient = NULL;
    }
    if (iconClient) {
        delete iconClient;
        iconClient = NULL;
    }
}

/*
 * Container use to pass the BusName and Port that was discoverd from the
 * Announce signal.
 */
struct PingContext {
    qcc::String busName;
    SessionPort port;
    PingContext(qcc::String bus, SessionPort p) : busName(bus), port(p) { }
};

class AboutClientPingAsyncCB : public BusAttachment::PingAsyncCB {
  public:
    void PingCB(QStatus status, void* context) {

        PingContext* ctx = (PingContext*)context;
        if (ER_OK == status) {
            status = AttemptToJoinSession(ctx->busName, ctx->port);
            if (status != ER_OK) {
                std::cout << "Unable to JoinSession with " << ctx->busName.c_str() << std::endl;
            }
        } else {
            std::cout << "Unable to ping " << ctx->busName.c_str() << ". The Bus is either unreachable or is running an version of AllJoyn older than v14.06." << std::endl;
            std::cout << "Attempting to Join a session with " << ctx->busName.c_str() << ". Just in case the remote device is running an older version of AllJoyn." << std::endl;
            /*
             * If all the services are running Code that is AllJoyn v14.06
             * or newer then nothing needs to be done.  If some of the services
             * are older then AllJoyn v14.06 the only way to find out if the
             * service is reachable is to try and create a session. Unlike calling
             * ping there is no timeout option when calling JoinSessionAsync.
             * We must wait for the default timeout (90 seconds) to know that the
             * service is unreachable.
             */
            status = AttemptToJoinSession(ctx->busName, ctx->port);
            if (status != ER_OK) {
                std::cout << "Unable to JoinSession with " << ctx->busName.c_str() << std::endl;
            }
        }
        delete ctx;
        ctx = NULL;
        delete this;
    }
  private:
    QStatus AttemptToJoinSession(qcc::String busName, SessionPort port) {
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

        AboutClientSessionListener* aboutClientSessionListener = new AboutClientSessionListener(busName);
        AboutClientSessionJoiner* joincb = new AboutClientSessionJoiner(*busAttachment, busName, sessionJoinedCallback);
        printf("Calling JoinSession BusName = %s port = %d\n", busName.c_str(), port);
        QStatus status = busAttachment->JoinSessionAsync(busName.c_str(), port, aboutClientSessionListener,
                                                         opts, joincb, aboutClientSessionListener);
        return status;
    }
};

void announceHandlerCallback(qcc::String const& busName, unsigned short port)
{
    QStatus status;
    /*
     * create a new PingContextPointer to pass to the PingAsyncCB
     */
    PingContext* pingContext = new PingContext(busName, (SessionPort)port);

    /*
     * Create an instance of the PingAsyncCB that will respond to the PingAsync
     * method call.
     */
    AboutClientPingAsyncCB* pingAsyncCB = new AboutClientPingAsyncCB();

    /*
     * Check that the unique bus name found by the Announce handler is reachable
     * before forming a session.
     *
     * It possible for the Announce signal to contain stale information.
     * By pinging the bus name we are able to determine if it is still present
     * and responsive before joining a session to it.
     *
     * PingAsync is being used in favor of the the regular ping since many announce
     * signals can come in at the same time using the non Async version of ping
     * could result in running out of concurrent callback threads which results
     * in a deadlock.
     */
    printf("Calling PingAsync BusName = %s\n", busName.c_str());
    status = busAttachment->PingAsync(busName.c_str(), PING_WAIT_TIME, pingAsyncCB, pingContext);
    /*
     * The PingAsync call failed make sure pointers are cleaned up so no memory
     * leaks occur.
     */
    if (ER_OK != status) {
        std::cout << "Unable to ping " << busName.c_str() << " reason reported: "
                  << QCC_StatusText(status);
        delete pingContext;
        pingContext = NULL;
        delete pingAsyncCB;
        pingAsyncCB = NULL;
    }
}

/** Wait for SIGINT before continuing. */
void WaitForSigInt(void)
{
    QStatus status;
    while (s_interrupt == false) {
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10 * 1000);
#endif
        while ((!s_joinedSessions.empty()) && s_interrupt == false) {
            /*
             * for each session joined display all of the data associated with the
             * AboutService and the AboutIconService.  Once we have displayed the
             * about service information we simply leave the session.
             */
            ViewAboutServiceData(s_joinedSessions.front().busName, s_joinedSessions.front().id);
            status = busAttachment->LeaveSession(s_joinedSessions.front().id);
            std::cout << "Leaving session id = " << s_joinedSessions.front().id << " with " << s_joinedSessions.front().busName.c_str() << " status: " << QCC_StatusText(status) << std::endl;
            s_joinedSessions.pop_front();
        }
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

    // Uncomment the following lines to view About Service debug information.
    //QCC_SetLogLevels("ALLJOYN_ABOUT_CLIENT=7");
    //QCC_SetLogLevels("ALLJOYN_ABOUT_ICON_CLIENT=7");
    //QCC_SetLogLevels("ALLJOYN_ABOUT_ANNOUNCE_HANDLER=7");
    //QCC_SetLogLevels("ALLJOYN_ABOUT_ANNOUNCEMENT_REGISTRAR=7");

    //set Daemon password only for bundled app
    #ifdef QCC_USING_BD
    PasswordManager::SetCredentials("ALLJOYN_PIN_KEYX", "000000");
    #endif

    // Install SIGINT handler
    signal(SIGINT, SigIntHandler);

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
    const char* interfaces[] = { "org.alljoyn.About", "org.alljoyn.Icon" };
    AnnouncementRegistrar::RegisterAnnounceHandler(*busAttachment, *announceHandler,
                                                   interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    // Perform the service asynchronously until the user signals for an exit.
    if (ER_OK == status) {
        WaitForSigInt();
    }

    AnnouncementRegistrar::UnRegisterAnnounceHandler(*busAttachment, *announceHandler, interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
    delete announceHandler;

    busAttachment->Stop();
    busAttachment->Join();
    delete busAttachment;

    return 0;

} /* main() */
