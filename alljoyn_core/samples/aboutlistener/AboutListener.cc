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

#include <alljoyn/AboutData.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/AboutProxy.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Init.h>
#include <alljoyn/Session.h>
#include <alljoyn/SessionListener.h>

#include <qcc/Mutex.h>

#include <signal.h>
#include <stdio.h>
#include <queue>

using namespace ajn;

static volatile sig_atomic_t s_interrupt = false;

static void CDECL_CALL SigIntHandler(int sig) {
    QCC_UNUSED(sig);
    s_interrupt = true;
}

BusAttachment* g_bus = nullptr;

struct JoinedSessionData {
    JoinedSessionData(const qcc::String& hostBusName, SessionId sessionId)
        : hostBusName(hostBusName), sessionId(sessionId) {
    }

    qcc::String hostBusName;
    SessionId sessionId;
};

static std::queue<JoinedSessionData> s_joinedSessions;
static qcc::Mutex* s_joinedSessionsLock = nullptr;

void printHorizontalLine() {
    const qcc::String horizontalLine = "*********************************************************************************";
    std::cout << horizontalLine << std::endl;
}

/* Print out the fields found in the AboutData. Only fields with known signatures
   are printed out. All others will be treated as an unknown field. */
void printAboutDataForLanguage(AboutData& aboutData, const char* language, int tabLevel)
{
    std::ios::fmtflags initialFlags(std::cout.flags());
    size_t numFields = aboutData.GetFields();

    const char** fields = new const char*[numFields];
    aboutData.GetFields(fields, numFields);

    for (size_t i = 0; i < numFields; ++i) {
        for (int j = 0; j < tabLevel; ++j) {
            std::cout << "\t";
        }
        std::cout << "Key: " << fields[i];

        MsgArg* fieldArg;
        QStatus status = aboutData.GetField(fields[i], fieldArg, language);
        if (status != ER_OK) {
            std::cout << "printAboutDataForLanguage: AboutData.GetField failed for ";
            if (language == nullptr) {
                std::cout << "default language";
            } else {
                std::cout << "language " << language;
            }
            std::cout << " (" << status << ")" << std::endl;
            continue;
        }
        std::cout << "\t";
        if (fieldArg->Signature() == "s") {
            const char* toPrint;
            fieldArg->Get("s", &toPrint);
            std::cout << toPrint;
        } else if (fieldArg->Signature() == "as") {
            size_t asLen;
            MsgArg* asArg;
            fieldArg->Get("as", &asLen, &asArg);
            for (size_t j = 0; j < asLen; ++j) {
                const char* toPrint;
                asArg[j].Get("s", &toPrint);
                std::cout << toPrint << " ";
            }
        } else if (fieldArg->Signature() == "ay") {
            size_t ayLen;
            uint8_t* toPrint;
            fieldArg->Get("ay", &ayLen, &toPrint);
            for (size_t j = 0; j < ayLen; ++j) {
                std::cout << std::hex << static_cast<int>(toPrint[j]) << " ";
            }
        } else {
            std::cout << "User Defined Value\tSignature: " << fieldArg->Signature();
        }
        std::cout << std::endl;
    }
    std::cout.flags(initialFlags);
    delete [] fields;
}

void printObjectDescription(const AboutObjectDescription& objectDescription, size_t tabLevel) {
    size_t pathCount = objectDescription.GetPaths(nullptr, 0);
    const char** paths = new const char*[pathCount];
    objectDescription.GetPaths(paths, pathCount);

    for (size_t p = 0; p < pathCount; ++p) {
        for (size_t t = 0; t < tabLevel; ++t) {
            std::cout << "\t";
        }
        std::cout << paths[p] << std::endl;

        size_t intfCount = objectDescription.GetInterfaces(paths[p], nullptr, 0);
        const char** intfs = new const char*[intfCount];
        objectDescription.GetInterfaces(paths[p], intfs, intfCount);
        for (size_t i = 0; i < intfCount; ++i) {
            for (size_t t = 0; t < tabLevel + 1; ++t) {
                std::cout << "\t";
            }
            std::cout << intfs[i] << std::endl;
        }
        delete [] intfs;
    }
    delete [] paths;
}

QStatus printObjectDescription(AboutProxy& aboutProxy) {
    MsgArg objArg;
    QStatus status = aboutProxy.GetObjectDescription(objArg);
    if (status != ER_OK) {
        std::cout << "AboutProxy.GetObjectDescription failed(" << status << ")" << std::endl;
        return status;
    }

    AboutObjectDescription aboutObjectDescription(objArg);
    std::cout << "AboutProxy.GetObjectDescription:" << std::endl;
    printObjectDescription(aboutObjectDescription, 1);
    return ER_OK;
}

QStatus printAboutData(AboutProxy& aboutProxy) {
    MsgArg aboutArg;
    QStatus status = aboutProxy.GetAboutData(nullptr, aboutArg);
    if (status != ER_OK) {
        std::cout << "printAboutData: AboutProxy.GetAboutData for default language failed "
                  << "(" << status << ")" << std::endl;
        return status;
    }

    AboutData defaultLangAboutData(aboutArg);
    std::cout << "AboutProxy.GetAboutData: (Default Language)" << std::endl;
    printAboutDataForLanguage(defaultLangAboutData, nullptr, 1);
    size_t langCount;
    langCount = defaultLangAboutData.GetSupportedLanguages();
    /* If the langCount == 1 we only have a default language. */
    if (langCount > 1) {
        const char** langs = new const char*[langCount];
        defaultLangAboutData.GetSupportedLanguages(langs, langCount);
        char* defaultLanguage;
        QStatus defaultLanguageStatus = defaultLangAboutData.GetDefaultLanguage(&defaultLanguage);
        if (defaultLanguageStatus != ER_OK) {
            std::cout << "printAboutData: AboutData.GetDefaultLanguage failed "
                      << "(" << defaultLanguageStatus << ")" << std::endl;
        }
        /* Print out the AboutData for every language but the
           default as it has already been printed. */
        for (size_t l = 0; l < langCount; ++l) {
            if (defaultLanguageStatus == ER_OK && strcmp(defaultLanguage, langs[l]) == 0) {
                continue;
            }
            status = aboutProxy.GetAboutData(langs[l], aboutArg);
            if (status != ER_OK) {
                std::cout << "printAboutData: AboutProxy.GetAboutData for language "
                          << langs[l] << " failed (" << status << ")" << std::endl;
                continue;
            }

            AboutData nonDefaultAboutData;
            nonDefaultAboutData.CreatefromMsgArg(aboutArg, langs[l]);
            std::cout << "AboutProxy.GetAboutData: (" << langs[l] << ")" << std::endl;
            printAboutDataForLanguage(nonDefaultAboutData, langs[l], 1);
        }
        delete [] langs;
    }
    return ER_OK;
}

QStatus printVersion(AboutProxy& aboutProxy) {
    uint16_t ver;
    QStatus status = aboutProxy.GetVersion(ver);
    if (status != ER_OK) {
        std::cout << "printVersion: AboutProxy.GetVersion failed(" << status << ")" << std::endl;
        return status;
    }

    std::cout << "AboutProxy.GetVersion: " << ver << std::endl;
    return ER_OK;
}

QStatus processJoinedSession(const JoinedSessionData& joinedSession) {
    MsgArg objArg;
    AboutProxy aboutProxy(*g_bus, joinedSession.hostBusName.c_str(), joinedSession.sessionId);

    printHorizontalLine();
    QStatus status = printObjectDescription(aboutProxy);
    if (status != ER_OK) {
        return status;
    }
    printHorizontalLine();
    status = printAboutData(aboutProxy);
    if (status != ER_OK) {
        return status;
    }
    printHorizontalLine();
    status = printVersion(aboutProxy);
    printHorizontalLine();
    return status;
}

class MySessionListener : public SessionListener {
    void SessionLost(SessionId sessionId, SessionLostReason reason) {
        std::cout << "SessionLost. SessionId = " << sessionId << ", Reason = " << reason << std::endl;
    }
};

class MyJoinCallback : public BusAttachment::JoinSessionAsyncCB {
    void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context) {
        QCC_UNUSED(opts);
        qcc::String* busName = reinterpret_cast<qcc::String*>(context);

        printHorizontalLine();
        std::cout << "SessionJoined sessionId = " << sessionId << ", status = " << QCC_StatusText(status) << std::endl;
        /* Instead of processing the joined session data in the callback thread, we add the data to the
           application's work queue (s_joinedSessions). This way, the processing will be
           delegated to the application thread and the callback thread will not be overloaded with
           time-consuming work.
           See also the documentation for BusAttachment::EnableConcurrentCallbacks(). */
        s_joinedSessionsLock->Lock(MUTEX_CONTEXT);
        s_joinedSessions.push(JoinedSessionData(*busName, sessionId));
        s_joinedSessionsLock->Unlock(MUTEX_CONTEXT);
        delete busName;
    }
};

void printAnnouncedParameters(const char* busName, uint16_t version, SessionPort port) {
    std::cout << "Announce signal discovered" << std::endl;
    std::cout << "\tFrom bus " << busName << std::endl;
    std::cout << "\tAbout version " << version << std::endl;
    std::cout << "\tSessionPort " << port << std::endl;
}

void printAnnouncedObjectDescription(const MsgArg& objectDescriptionArg) {
    AboutObjectDescription objectDescription(objectDescriptionArg);
    std::cout << "\tObjectDescription:" << std::endl;
    const size_t tabLevel = 2;
    printObjectDescription(objectDescription, tabLevel);
}

void printAnnouncedAboutData(const MsgArg& aboutDataArg) {
    std::cout << "\tAboutData:" << std::endl;
    AboutData aboutData(aboutDataArg);
    const size_t tabLevel = 2;
    printAboutDataForLanguage(aboutData, nullptr, tabLevel);
}

class MyAboutListener : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg) {
        AboutObjectDescription objectDescription;
        objectDescription.CreateFromMsgArg(objectDescriptionArg);

        printHorizontalLine();
        printAnnouncedParameters(busName, version, port);
        printAnnouncedObjectDescription(objectDescriptionArg);
        printAnnouncedAboutData(aboutDataArg);

        QStatus status;
        if (g_bus != nullptr) {
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            /* Use the asynchronous variant of JoinSession. Calling a synchronous remote method call from
               a callback (and Announced() is one) would require using the BusAttachment::EnableConcurrentCallbacks()
               functionality which is resource-consuming and may lead to a deadlock if there are not enough
               callback-processing threads.
               For details, see the documentation for BusAttachment::EnableConcurrentCallbacks(). */
            status = g_bus->JoinSessionAsync(busName, port, &sessionListener, opts, &joinCb, new qcc::String(busName));
            if (status != ER_OK) {
                std::cout << "Error joining session: " << QCC_StatusText(status) << std::endl;
            }
        } else {
            std::cout << "BusAttachment is NULL" << std::endl;
        }
    }

  private:
    MySessionListener sessionListener;
    MyJoinCallback joinCb;
};

QStatus processJoinedSessions() {
    if (!s_joinedSessions.empty()) {
        /* Process a single entry from the application's work queue:
           print out a joined session's About data and try to do a remote method call,
           then leave the session. */
        s_joinedSessionsLock->Lock(MUTEX_CONTEXT);
        JoinedSessionData joinedSession = s_joinedSessions.front();
        s_joinedSessions.pop();
        s_joinedSessionsLock->Unlock(MUTEX_CONTEXT);
        QStatus status = processJoinedSession(joinedSession);
        if (status != ER_OK) {
            std::cout << "processJoinedSessions: processJoinedSession failed (" << status << ")" << std::endl;
            return status;
        }

        std::cout << "Leaving session id = " << joinedSession.sessionId << " with " << joinedSession.hostBusName.c_str() << " status: " << QCC_StatusText(status) << std::endl;
        status = g_bus->LeaveSession(joinedSession.sessionId);
        if (status != ER_OK) {
            std::cout << "processJoinedSessions: LeaveSession failed (" << status << ")" << std::endl;
            return status;
        }
    }
    return ER_OK;
}

int CDECL_CALL main(int argc, char** argv)
{
    QCC_UNUSED(argc);
    QCC_UNUSED(argv);

    if (AllJoynInit() != ER_OK) {
        return 1;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return 1;
    }
#endif

    /* Install SIGINT handler so Ctrl + C deallocates memory properly. */
    signal(SIGINT, SigIntHandler);

    g_bus = new BusAttachment("AboutListener", true);

    QStatus status = g_bus->Start();
    if (ER_OK == status) {
        std::cout << "BusAttachment started." << std::endl;
    } else {
        std::cout << "FAILED to start BusAttachment (" << QCC_StatusText(status) << ")" << std::endl;
        delete g_bus;
        exit(1);
    }

    status = g_bus->Connect();
    if (ER_OK == status) {
        std::cout << "BusAttachment connect succeeded." << std::endl;
    } else {
        std::cout << "FAILED to connect to router node (" << QCC_StatusText(status) << ")" << std::endl;
        delete g_bus;
        exit(1);
    }

    s_joinedSessionsLock = new qcc::Mutex();

    MyAboutListener aboutListener;
    g_bus->RegisterAboutListener(aboutListener);

    /* Passing nullptr into WhoImplements will listen for all About announcements */
    status = g_bus->WhoImplements(nullptr);
    if (ER_OK == status) {
        std::cout << "WhoImplements called." << std::endl;
    } else {
        std::cout << "WhoImplements call FAILED with status " << QCC_StatusText(status) << std::endl;
        delete g_bus;
        delete s_joinedSessionsLock;
        exit(1);
    }

    /* Perform the service asynchronously until the user signals for an exit. */
    if (ER_OK == status) {
        while (s_interrupt == false) {
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100 * 1000);
#endif
            /* Process the work queue. */
            status = processJoinedSessions();
            if (status != ER_OK) {
                std::cout << "processJoinedSessions failed (" << status << ")" << std::endl;
            }
        }
    }

    g_bus->UnregisterAboutListener(aboutListener);
    delete s_joinedSessionsLock;
    delete g_bus;
#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return 0;
}
