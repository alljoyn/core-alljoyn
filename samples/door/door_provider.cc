/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#include "door_common.h"

#include <vector>

#include <qcc/Crypto.h>

#include <alljoyn/Init.h>

using namespace sample::securitymgr::door;
using namespace ajn;
using namespace std;

class SPListener :
    public SessionPortListener {
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        QCC_UNUSED(opts);
        QCC_UNUSED(joiner);
        QCC_UNUSED(sessionPort);

        return true;
    }
};

int main(int arg, char** argv)
{
    QCC_UNUSED(arg);
    QCC_UNUSED(argv);

    if (AllJoynInit() != ER_OK) {
        return EXIT_FAILURE;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return EXIT_FAILURE;
    }
#endif

    //Do the common set-up
    DoorCommon common("DoorProvider");     //TODO make name commandline param
    QStatus status = common.Init("/tmp/provdb.ks", true); //TODO allow keystore to be defined from cmdline
    SessionOpts opts;
    SessionPort port = DOOR_APPLICATION_PORT;
    SPListener spl;
    BusAttachment* ba = common.GetBusAttachment();;

    do {
        if (status != ER_OK) {
            break;
        }
        //Create bus object
        Door door(ba);

        status = ba->RegisterBusObject(door, DOOR_INTF_SECURE ? true : false);
        if (ER_OK != status) {
            printf("Failed to RegisterBusObject\n");
            break;
        }

        //Host session
        status = ba->BindSessionPort(port, opts, spl);
        if (ER_OK != status) {
            printf("Failed to BindSesssionPort\n");
            break;
        }

        // Announce about
        status = common.AnnounceAbout();
        if (ER_OK != status) {
            printf("Failed to announce about\n");
            break;
        }

        printf("Door provider initialized; Waiting for consumers ...");
        printf("Type 'q' to quit\n");

        while ((cin.get()) != 'q') {
        }
    } while (0);

    common.Fini();

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return (int)status;
}
