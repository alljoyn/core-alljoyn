/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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
#include <qcc/Crypto.h>
#include <qcc/String.h>
#include <vector>

using namespace sample::securitymgr::door;
using namespace ajn;

class SPListener :
    public SessionPortListener {
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) { return true; }
};

int main(int arg, char** argv)
{
    //Do the common set-up
    DoorCommon common("DoorProvider");     //TODO make name commandline param
    QStatus status = common.init("/tmp/provdb.ks", true); //TODO allow keystore to be defined from cmdline

    if (status != ER_OK) {
        exit(1);
    }

    BusAttachment& ba = common.GetBusAttachment();
    //Create bus object
    Door door(ba);

    status = ba.RegisterBusObject(door, DOOR_INTF_SECURE ? true : false);
    if (status != ER_OK) {
        exit(1);
    }

    status = door.RegisterToAbout();
    if (status != ER_OK) {
        exit(1);
    }
    //Host session
    SessionOpts opts;
    SessionPort port = DOOR_APPLICATION_PORT;
    SPListener spl;
    ba.BindSessionPort(port, opts, spl);

    printf("Door provider initialized; Waiting for consumers ...");
    printf("Type 'q' to quit\n");

    while ((std::cin.get()) != 'q') {
    }

    return 0;
}
