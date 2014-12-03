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

class MyAboutListener : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg) {
        AboutObjectDescription objectDescription;
        objectDescription.CreateFromMsgArg(objectDescriptionArg);
        printf("*********************************************************************************\n");
        printf("Announce signal discovered\n");
        printf("\tFrom bus %s\n", busName);
        printf("\tAbout version %hu\n", version);
        printf("\tSessionPort %hu\n", port);
        printf("\tObjectDescription\n%s\n", objectDescriptionArg.ToString().c_str());
        printf("\tAboutData:\n%s\n", aboutDataArg.ToString().c_str());
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
                printf("AboutProxy.GetObjectDescriptions:\n%s\n", objArg.ToString().c_str());
                printf("*********************************************************************************\n");

                MsgArg aArg;
                aboutProxy.GetAboutData("en", aArg);
                printf("*********************************************************************************\n");
                printf("AboutProxy.GetAboutData:\n%s\n", aArg.ToString().c_str());
                printf("*********************************************************************************\n");

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
