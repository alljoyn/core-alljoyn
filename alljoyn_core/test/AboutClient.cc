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

using namespace ajn;

static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig) {
    s_interrupt = true;
}

BusAttachment* g_bus;

class MyAboutListener : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg) {
        printf("*********************************************************************************\n");
        printf("Anounce signal discovered\n");
        printf("\tFrom bus %s\n", busName);
        printf("\tAbout version %hu\n", version);
        printf("\tSessionPort %hu\n", port);

        printf("*********************************************************************************\n");
        QStatus status;

        if (g_bus != NULL) {
            SessionListener sessionListener;
            SessionId sessionId;
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            g_bus->EnableConcurrentCallbacks();
            status = g_bus->JoinSession(busName, port, &sessionListener, sessionId, opts);
            printf("SessionJoined sessionId = %u, status = %s\n", sessionId, QCC_StatusText(status));
            if (ER_OK == status && 0 != sessionId) {
                AboutProxy aboutProxy(*g_bus, busName, sessionId);

                MsgArg objArg;
                aboutProxy.GetObjectDescriptions(objArg);
                printf("*********************************************************************************\n");
                printf("AboutProxy.GetObjectDescriptions:\n%s\n", objArg.ToString().c_str());
                printf("*********************************************************************************\n");

                MsgArg aArg;
                aboutProxy.GetAboutData("en", aArg);
                printf("*********************************************************************************\n");
                printf("AboutProxy.GetObjectDescriptions:\n%s\n", aArg.ToString().c_str());
                printf("*********************************************************************************\n");

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
};

int main(int argc, char** argv)
{
    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, SigIntHandler);


    QStatus status;

    BusAttachment bus("AboutServiceTest");

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

    const char* interfaces[] = { "org.alljoyn.test" };

    status = bus.RegisterAboutListener(aboutListener, interfaces, sizeof(interfaces) / sizeof(interfaces[0]));

    if (ER_OK == status) {
        printf("AboutListener registered.\n");
    } else {
        printf("FAILED to registered the AboutListener (%s)\n", QCC_StatusText(status));
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
}
