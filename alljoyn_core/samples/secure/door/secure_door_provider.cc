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

#include "secure_door_common.h"

#include <vector>

#include <qcc/Crypto.h>

#include <alljoyn/Init.h>

using namespace sample::secure::door;
using namespace ajn;
using namespace std;

void UpdateDoorProviderManifest(DoorCommon& common)
{
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(DOOR_INTERFACE);

    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);

    rules[0].SetMembers(3, prms);

    PermissionPolicy::Acl manifest;
    manifest.SetRules(1, rules);

    common.UpdateManifest(manifest);
}

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
    if (status != ER_OK) {
        fprintf(stderr, "Failed to initialize common layer\n");
        exit(1);
    }

    do {
        if (status != ER_OK) {
            break;
        }
        // Create bus object
        Door door(ba);

        status = ba->RegisterBusObject(door, true);
        if (ER_OK != status) {
            printf("Failed to RegisterBusObject\n");
            break;
        }

        status = common.AnnounceAbout();
        if (ER_OK != status) {
            printf("Failed to announce about\n");
            break;
        }

        //Wait until we are claimed
        pcl.WaitForClaimedState();

        printf("Door provider initialized; Waiting for consumers ...\n");
        printf("Type 'q' to quit\n");
        printf(">");

        char cmd;
        while ((cmd = cin.get()) != 'q') {
            printf(">");

            switch (cmd) {
            case 'u':
                printf("Enabling automatic signaling of door events ...\n");
                UpdateDoorProviderManifest(common);
                door.autoSignal = true;
                break;

            case 's':
                door.SendDoorEvent();
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

    common.Fini();

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif

    AllJoynShutdown();
    return (int)status;
}
