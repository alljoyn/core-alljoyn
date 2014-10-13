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
#include <qcc/Thread.h>
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
using namespace qcc;

static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig) {
    s_interrupt = true;
}

BusAttachment* g_bus;

class AboutThread : public Thread, public ThreadListener {
  public:
    static AboutThread* Launch(const char* busName, SessionPort port)
    {
        AboutThread* bgThread = new AboutThread(busName, port);
        bgThread->Start();

        return bgThread;
    }

    AboutThread(const char* busName, SessionPort port)
        : sender(busName), sessionPort(port) { }

    void ThreadExit(Thread* thread)
    {
        printf("Thread exit...\n");

        thread->Join();
        delete thread;
    }

  protected:

    ThreadReturn STDCALL Run(void* args)
    {
        QStatus status = ER_OK;

        SessionListener sessionListener;
        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

        printf("Sender%s\n", sender);

        status = g_bus->JoinSession(sender, sessionPort, &sessionListener, sessionId, opts);

        if (ER_OK == status) {
            AboutProxy aboutProxy(*g_bus, sender, sessionId);

            MsgArg objArg;
            status = aboutProxy.GetObjectDescription(objArg);

            if (status == ER_OK) {
                printf("*********************************************************************************\n");
                printf("AboutProxy.GetObjectDescription:\n%s\n", objArg.ToString().c_str());
                printf("*********************************************************************************\n");

                MsgArg aArg;
                status = aboutProxy.GetAboutData("en", aArg);

                if (status == ER_OK) {
                    printf("*********************************************************************************\n");
                    printf("AboutProxy.GetAboutData:\n%s\n", aArg.ToString().c_str());
                    printf("*********************************************************************************\n");

                    uint16_t ver;
                    status = aboutProxy.GetVersion(ver);

                    if (status == ER_OK) {
                        printf("*********************************************************************************\n");
                        printf("AboutProxy.GetVersion %hd\n", ver);
                        printf("*********************************************************************************\n");
                    } else {
                        printf("AboutProxy.GetVersion failed(%s)\n", QCC_StatusText(status));
                    }
                } else {
                    printf("AboutProxy.GetAboutData failed(%s)\n", QCC_StatusText(status));
                }
            } else {
                printf("AboutProxy.GetObjectDescription failed(%s)\n", QCC_StatusText(status));
            }

            g_bus->LeaveSession(sessionId);

        } else {
            printf("JoinSession failed(%s)\n", QCC_StatusText(status));
        }
        return 0;
    }

  private:
    const char* sender;
    SessionPort sessionPort;

};

class MyAboutListener : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg) {
        printf("*********************************************************************************\n");
        printf("Anounce signal discovered\n");
        printf("\tFrom bus %s\n", busName);
        printf("\tAbout version %hu\n", version);
        printf("\tSessionPort %hu\n", port);

        printf("*********************************************************************************\n");

        if (g_bus != NULL) {
            // Start a seperate thread to joinSession and GetAboutData
            AboutThread::Launch(::strdup(busName), port);
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
    bus.RegisterAboutListener(aboutListener);

    const char* interfaces[] = { "org.alljoyn.test" };
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
            qcc::Sleep(100);
        }
    }
}
