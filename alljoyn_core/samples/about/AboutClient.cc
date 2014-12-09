/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
#include <alljoyn/Session.h>
#include <alljoyn/SessionListener.h>

#include <signal.h>
#include <stdio.h>

/*
 * Note the removal of almost all Error handling to make the sample code more
 * straight forward to read.  This is only used here for demonstration actual
 * programs should check the return values of all method calls.
 */
using namespace ajn;

static volatile sig_atomic_t s_interrupt = false;

// The interface name should be the only thing required to find and form a
// connection between the service and the client using the about feature.
static const char* INTERFACE_NAME = "com.example.about.feature.interface.sample";

static void SigIntHandler(int sig) {
    s_interrupt = true;
}

BusAttachment* g_bus;

class MySessionListener : public SessionListener {
    void SessionLost(SessionId sessionId, SessionLostReason reason) {
        printf("SessionLost sessionId = %u, Reason = %d\n", sessionId, reason);
    }
};

// Print out the fields found in the AboutData. Only fields with known signatures
// are printed out.  All others will be treated as an unknown field.
void printAboutData(AboutData& aboutData, const char* language, int tabNum)
{
    size_t count = aboutData.GetFields();

    const char** fields = new const char*[count];
    aboutData.GetFields(fields, count);

    for (size_t i = 0; i < count; ++i) {
        for (int j = 0; j < tabNum; ++j) {
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
    void Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg) {
        AboutObjectDescription objectDescription;
        objectDescription.CreateFromMsgArg(objectDescriptionArg);
        printf("*********************************************************************************\n");
        printf("Announce signal discovered\n");
        printf("\tFrom bus %s\n", busName);
        printf("\tAbout version %hu\n", version);
        printf("\tSessionPort %hu\n", port);
        printf("\tObjectDescription:\n");
        AboutObjectDescription aod(objectDescriptionArg);
        size_t path_num = aod.GetPaths(NULL, 0);
        const char** paths = new const char*[path_num];
        aod.GetPaths(paths, path_num);
        for (size_t i = 0; i < path_num; ++i) {
            printf("\t\t%s\n", paths[i]);
            size_t intf_num = aod.GetInterfaces(paths[i], NULL, 0);
            const char** intfs = new const char*[intf_num];
            aod.GetInterfaces(paths[i], intfs, intf_num);
            for (size_t j = 0; j < intf_num; ++j) {
                printf("\t\t\t%s\n", intfs[j]);
            }
            delete [] intfs;
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
                size_t path_num = aod.GetPaths(NULL, 0);
                const char** paths = new const char*[path_num];
                aod.GetPaths(paths, path_num);
                for (size_t i = 0; i < path_num; ++i) {
                    printf("\t%s\n", paths[i]);
                    size_t intf_num = aod.GetInterfaces(paths[i], NULL, 0);
                    const char** intfs = new const char*[intf_num];
                    aod.GetInterfaces(paths[i], intfs, intf_num);
                    for (size_t j = 0; j < intf_num; ++j) {
                        printf("\t\t%s\n", intfs[j]);
                    }
                    delete [] intfs;
                }
                delete [] paths;

                MsgArg aArg;
                aboutProxy.GetAboutData("en", aArg);
                printf("*********************************************************************************\n");
                printf("AboutProxy.GetAboutData: (Default Language)\n");
                AboutData aboutData(aArg);
                printAboutData(aboutData, NULL, 1);
                size_t lang_num;
                lang_num = aboutData.GetSupportedLanguages();
                // If the lang_num == 1 we only have a default language
                if (lang_num > 1) {
                    const char** langs = new const char*[lang_num];
                    aboutData.GetSupportedLanguages(langs, lang_num);
                    char* defaultLanguage;
                    aboutData.GetDefaultLanguage(&defaultLanguage);
                    // print out the AboutData for every language but the
                    // default it has already been printed.
                    for (size_t i = 0; i < lang_num; ++i) {
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

                const char* path;
                objectDescription.GetInterfacePaths(INTERFACE_NAME, &path, 1);
                printf("Calling %s/%s\n", path, INTERFACE_NAME);
                ProxyBusObject proxyObject(*g_bus, busName, path, sessionId);
                status = proxyObject.IntrospectRemoteObject();
                if (status != ER_OK) {
                    printf("Failed to introspect remote object.\n");
                }
                MsgArg arg("s", "ECHO Echo echo...\n");
                Message replyMsg(*g_bus);
                status = proxyObject.MethodCall(INTERFACE_NAME, "Echo", &arg, 1, replyMsg);
                if (status != ER_OK) {
                    printf("Failed to call Echo method.\n");
                    return;
                }
                char* echoReply;
                status = replyMsg->GetArg(0)->Get("s", &echoReply);
                if (status != ER_OK) {
                    printf("Failed to read Echo method reply.\n");
                }
                printf("Echo method reply: %s\n", echoReply);
            }
        } else {
            printf("BusAttachment is NULL\n");
        }
    }
    MySessionListener sessionListener;
};

int main(int argc, char** argv)
{
    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, SigIntHandler);


    QStatus status;

    BusAttachment bus("AboutServiceTest", true);

    g_bus = &bus;

    status = bus.Start();
    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("FAILED to start BusAttachment (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    status = bus.Connect();
    if (ER_OK == status) {
        printf("BusAttachment connect succeeded.\n");
    } else {
        printf("FAILED to connect to router node (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    MyAboutListener aboutListener;
    bus.RegisterAboutListener(aboutListener);

    const char* interfaces[] = { INTERFACE_NAME };
    status = bus.WhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
    if (ER_OK == status) {
        printf("WhoImplements called.\n");
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

    return 0;
}
