/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *    
 *    SPDX-License-Identifier: Apache-2.0
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *    
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/

#include "secure_door_common.h"

#include <vector>

#include <alljoyn/Init.h>

using namespace sample::secure::door;
using namespace ajn;
using namespace std;

static AJ_PCSTR s_providerManifestTemplateWithSignal =
    "<manifest>"
    "<node>"
    "<interface = \"" DOOR_INTERFACE "\">"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

int CDECL_CALL main(int argc, char** argv)
{
    string appName = "DoorProvider";
    if (argc > 1) {
        appName = string(argv[1]);
    }
    printf("Starting door provider %s\n", appName.c_str());

    if (AllJoynInit() != ER_OK) {
        return EXIT_FAILURE;
    }

#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return EXIT_FAILURE;
    }
#endif

    // Do the common set-up
    DoorCommon common(appName);
    BusAttachment* ba = common.GetBusAttachment();
    DoorCommonPCL pcl(*ba);

    QStatus status = common.Init(true, &pcl);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to initialize DoorCommon - status (%s)\n",
                QCC_StatusText(status));
        goto Exit;
    }

    do {
        // Create bus object
        Door door(ba);

        status = door.Init();
        if (ER_OK != status) {
            fprintf(stderr, "Failed to initialize Door - status (%s)\n",
                    QCC_StatusText(status));
            goto Exit;
        }

        status = ba->RegisterBusObject(door, true);
        if (ER_OK != status) {
            fprintf(stderr, "Failed to RegisterBusObject - status (%s)\n",
                    QCC_StatusText(status));
            goto Exit;
        }

        status = common.AnnounceAbout();
        if (ER_OK != status) {
            fprintf(stderr, "Failed to AnnounceAbout - status (%s)\n",
                    QCC_StatusText(status));
            goto Exit;
        }

        //Wait until we are claimed
        status = pcl.WaitForClaimedState();
        if (ER_OK != status) {
            fprintf(stderr, "Failed to WaitForClaimedState - status (%s)\n",
                    QCC_StatusText(status));
            goto Exit;
        }

        // After claiming, only allow ALLJOYN_ECDHE_ECDSA connections
        status = common.SetSecurityForClaimedMode();
        if (ER_OK != status) {
            fprintf(stderr, "Failed to SetSecurityForClaimedMode - status (%s)\n",
                    QCC_StatusText(status));
            goto Exit;
        }

        printf("Door provider initialized; Waiting for consumers ...\n");
        printf("Type 'q' to quit\n");
        printf(">");

        char cmd;
        while ((cmd = cin.get()) != 'q') {
            printf(">");

            switch (cmd) {
            case 'u':
                printf("Enabling automatic signaling of door events ... ");
                status = common.UpdateManifestTemplate(s_providerManifestTemplateWithSignal);
                if (ER_OK != status) {
                    break;
                }
                door.autoSignal = true;
                printf("done\n");
                break;

            case 's':
                status = door.SendDoorEvent();
                if (ER_OK != status) {
                    fprintf(stderr, "Failed to SendDoorEvent - status (%s)\n",
                            QCC_StatusText(status));
                }
                break;

            case '\n':
                break;

            case '\r':
                break;

            default:
                break;
            }
        }
    } while (0);

Exit:
    common.Fini();

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif

    AllJoynShutdown();
    return (int)status;
}