/**
 * @file
 * Bundled daemon bus attachment stress test
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <vector>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Environ.h>
#include <qcc/Util.h>
#include <qcc/Thread.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"
#define THREAD_COUNT 5

using namespace std;
using namespace qcc;
using namespace ajn;

static bool noDestruct = false;

class ThreadClass : public Thread {

  public:
    ThreadClass(char*name) : Thread(name), name(name) { }

  protected:
    qcc::ThreadReturn STDCALL Run(void* arg) {

        BusAttachment*b1 = new BusAttachment(name.c_str(), true);
        QStatus status =  b1->Start();
        /* Get env vars */
        Environ* env = Environ::GetAppEnviron();

        /* Force bundled daemon */
        qcc::String connectArgs = env->Find("BUS_ADDRESS", "null:");
        status = b1->Connect(connectArgs.c_str());

        char buf[256];
        sprintf(buf, "Thread.i%d", 100 * qcc::Rand8());
        status = b1->RequestName(name.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
        if (status != ER_OK) {
            QCC_LogError(status, ("RequestName(%s) failed.", name.c_str()));
        }
        /* Begin Advertising the well-known name */
        status = b1->AdvertiseName(name.c_str(), TRANSPORT_ANY);
        if (ER_OK != status) {
            QCC_LogError(status, ("Could not advertise (%s)", name.c_str()));
        }

        BusObject bo("/org/cool");
        b1->RegisterBusObject(bo);
        b1->UnregisterBusObject(bo);

        if (!noDestruct) {
            delete b1;
        }

        return this;
    }

  private:
    String name;

};

static void usage(void)
{
    printf("Usage: bastress [-s] [-i <iterations>] [-t <threads>]\n\n");
    printf("Options:\n");
    printf("   -h                    = Print this help message\n");
    printf("   -i                    = Number of iterations, default is 1000\n");
    printf("   -t                    = Number of threads, default is 5\n");
    printf("   -s                    = Stop the threads before joining them\n");
    printf("   -d                    = Don't delete the bus attachments - implies \"-i 1\"r\n");
}

/** Main entry point */
int main(int argc, char**argv)
{
    QStatus status = ER_OK;
    uint32_t iterations = 1000;
    uint32_t threads = 5;
    bool stop = false;

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-i", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                iterations = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-t", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                threads = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-d", argv[i])) {
            noDestruct = true;
        } else if (0 == strcmp("-s", argv[i])) {
            stop = true;
        } else {
            usage();
            exit(1);
        }
    }

    if (noDestruct) {
        iterations = 1;
    }

    ThreadClass** threadList = new ThreadClass *[threads];

    while (iterations--) {

        QCC_SyncPrintf("Starting threads... \n");
        for (unsigned int i = 0; i < threads; i++) {
            char buf[256];
            sprintf(buf, "Thread.n%d", i);
            threadList[i] = new ThreadClass((char*)buf);
            threadList[i]->Start();
        }

        if (stop) {
            /*
             * Sleep a random time before stopping of bus attachments is tested at different states of up and running
             */
            qcc::Sleep(32 * (qcc::Rand8() / 8));
            QCC_SyncPrintf("stopping threads... \n");
            for (unsigned int i = 0; i < threads; i++) {
                threadList[i]->Stop();
            }
        }

        QCC_SyncPrintf("deleting threads... \n");
        for (unsigned int i = 0; i < threads; i++) {
            threadList[i]->Join();
            delete threadList[i];
        }

    }

    return (int) status;
}
