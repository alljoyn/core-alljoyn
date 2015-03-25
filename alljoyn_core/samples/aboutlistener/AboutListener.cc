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

#include <signal.h>
#include <stdio.h>

/*
 * Note the removal of almost all error handling to make the sample code more
 * straightforward to read.  This is only used here for demonstration; actual
 * programs should check the return values of all method calls.
 */
using namespace ajn;

static volatile sig_atomic_t s_interrupt = false;

static void CDECL_CALL SigIntHandler(int sig)
{
    QCC_UNUSED(sig);
    s_interrupt = true;
}

BusAttachment* g_bus = NULL;

class MySessionListener : public SessionListener {
    void SessionLost(SessionId sessionId, SessionLostReason reason)
    {
        printf("SessionLost sessionId = %u, Reason = %d\n", sessionId, reason);
    }
};

/*
 * Print out the fields found in the AboutData. Only fields with known signatures
 * are printed out.  All others will be treated as an unknown field.
 */
void printAboutData(AboutData& aboutData, const char* language, int tabCount)
{
    size_t fieldCount = aboutData.GetFields();

    const char** fields = new const char*[fieldCount];
    aboutData.GetFields(fields, fieldCount);

    for (size_t i = 0; i < fieldCount; ++i) {
        for (int j = 0; j < tabCount; ++j) {
            printf("\t");
        }
        printf("Key: %s", fields[i]);

        MsgArg* tmp;
        aboutData.GetField(fields[i], tmp, language);
        printf("\t");
        if (tmp->Signature() == "s") {
            const char* tmp_s;
            tmp->Get("s", &tmp_s);
            printf("%s", tmp_s);
        } else if (tmp->Signature() == "as") {
            size_t las;
            MsgArg* as_arg;
            tmp->Get("as", &las, &as_arg);
            for (size_t j = 0; j < las; ++j) {
                const char* tmp_s;
                as_arg[j].Get("s", &tmp_s);
                printf("%s ", tmp_s);
            }
        } else if (tmp->Signature() == "ay") {
            size_t lay;
            uint8_t* pay;
            tmp->Get("ay", &lay, &pay);
            for (size_t j = 0; j < lay; ++j) {
                printf("%02x ", pay[j]);
            }
        } else {
            printf("User Defined Value\tSignature: %s", tmp->Signature().c_str());
        }
        printf("\n");
    }
    delete [] fields;
}

class MyAboutListener : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg)
    {
        AboutObjectDescription objectDescription;
        objectDescription.CreateFromMsgArg(objectDescriptionArg);
        printf("*********************************************************************************\n");
        printf("Announce signal discovered\n");
        printf("\tFrom bus %s\n", busName);
        printf("\tAbout version %hu\n", version);
        printf("\tSessionPort %hu\n", port);
        printf("\tObjectDescription:\n");
        AboutObjectDescription aod(objectDescriptionArg);
        size_t pathCount = aod.GetPaths(NULL, 0);
        const char** paths = new const char*[pathCount];
        aod.GetPaths(paths, pathCount);
        for (size_t i = 0; i < pathCount; ++i) {
            printf("\t\t%s\n", paths[i]);
            size_t interfaceCount = aod.GetInterfaces(paths[i], NULL, 0);
            const char** interfaces = new const char*[interfaceCount];
            aod.GetInterfaces(paths[i], interfaces, interfaceCount);
            for (size_t j = 0; j < interfaceCount; ++j) {
                printf("\t\t\t%s\n", interfaces[j]);
            }
            delete [] interfaces;
        }
        delete [] paths;
        printf("\tAboutData:\n");
        AboutData aboutData(aboutDataArg);
        printAboutData(aboutData, NULL, 2);
        printf("*********************************************************************************\n");
        QStatus status;

        if (g_bus != NULL) {
            SessionId sessionId;
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            g_bus->EnableConcurrentCallbacks();
            status = g_bus->JoinSession(busName, port, &sessionListener, sessionId, opts);
            printf("SessionJoined sessionId = %u, status = %s\n", sessionId, QCC_StatusText(status));
            if (ER_OK == status && 0 != sessionId) {
                AboutProxy aboutProxy(*g_bus, busName, sessionId);

                MsgArg objArg;
                aboutProxy.GetObjectDescription(objArg);
                printf("*********************************************************************************\n");
                printf("AboutProxy.GetObjectDescription:\n");
                AboutObjectDescription aod(objArg);
                size_t pathCount = aod.GetPaths(NULL, 0);
                const char** paths = new const char*[pathCount];
                aod.GetPaths(paths, pathCount);
                for (size_t i = 0; i < pathCount; ++i) {
                    printf("\t%s\n", paths[i]);
                    size_t interfaceCount = aod.GetInterfaces(paths[i], NULL, 0);
                    const char** interfaces = new const char*[interfaceCount];
                    aod.GetInterfaces(paths[i], interfaces, interfaceCount);
                    for (size_t j = 0; j < interfaceCount; ++j) {
                        printf("\t\t%s\n", interfaces[j]);
                    }
                    delete [] interfaces;
                }
                delete [] paths;

                MsgArg aArg;
                aboutProxy.GetAboutData("en", aArg);
                printf("*********************************************************************************\n");
                printf("AboutProxy.GetAboutData: (Default Language)\n");
                AboutData aboutData(aArg);
                printAboutData(aboutData, NULL, 1);
                size_t languageCount;
                languageCount = aboutData.GetSupportedLanguages();
                /* If the languageCount == 1 we only have a default language */
                if (languageCount > 1) {
                    const char** langs = new const char*[languageCount];
                    aboutData.GetSupportedLanguages(langs, languageCount);
                    char* defaultLanguage;
                    aboutData.GetDefaultLanguage(&defaultLanguage);
                    /*
                     * Print out the AboutData for every language but the
                     * default; it has already been printed.
                     */
                    for (size_t i = 0; i < languageCount; ++i) {
                        if (strcmp(defaultLanguage, langs[i]) != 0) {
                            status = aboutProxy.GetAboutData(langs[i], aArg);
                            if (ER_OK == status) {
                                aboutData.CreatefromMsgArg(aArg, langs[i]);
                                printf("AboutProxy.GetAboutData: (%s)\n", langs[i]);
                                printAboutData(aboutData, langs[i], 1);
                            }
                        }
                    }
                    delete [] langs;
                }

                uint16_t ver;
                aboutProxy.GetVersion(ver);
                printf("*********************************************************************************\n");
                printf("AboutProxy.GetVersion %hd\n", ver);
                printf("*********************************************************************************\n");
            }
        } else {
            printf("BusAttachment is NULL\n");
        }
    }
    MySessionListener sessionListener;
};

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

    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, SigIntHandler);

    QStatus status;

    g_bus = new BusAttachment("AboutListener", true);

    status = g_bus->Start();
    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("FAILED to start BusAttachment (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    status = g_bus->Connect();
    if (ER_OK == status) {
        printf("BusAttachment connect succeeded.\n");
    } else {
        printf("FAILED to connect to router node (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    MyAboutListener* aboutListener = new MyAboutListener();
    g_bus->RegisterAboutListener(*aboutListener);

    /* Passing NULL into WhoImplements will listen for all About announcements */
    status = g_bus->WhoImplements(NULL);
    if (ER_OK == status) {
        printf("WhoImplements NULL called.\n");
    } else {
        printf("WhoImplements call FAILED with status %s\n", QCC_StatusText(status));
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
        }
    }

    g_bus->UnregisterAboutListener(*aboutListener);
    delete aboutListener;
    delete g_bus;
#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return 0;
}
