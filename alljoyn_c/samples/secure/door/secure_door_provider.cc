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

static AJ_PCSTR s_providerManifestTemplate =
    "<manifest>"
    "<node>"
    "<interface = \"" DOOR_INTERFACE "\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</property>"
    "</interface>"
    "</node>"
    "</manifest>";

static AJ_PCSTR s_providerManifestTemplateWithSignal =
    "<manifest>"
    "<node>"
    "<interface = \"" DOOR_INTERFACE "\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</signal>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</property>"
    "</interface>"
    "</node>"
    "</manifest>";

static uint8_t s_sharedSecret[] = "secureDoorSharedSecret";

typedef struct {
    alljoyn_busobject busObject;
    alljoyn_interfacedescription_member stateSignal;
    unsigned char doorOpened;
    unsigned char signalDoorOperations;
} ProviderDoorObject;

static ProviderDoorObject s_door;

static void ReplyWithBoolean(alljoyn_busobject object, unsigned char answer, alljoyn_message message)
{
    alljoyn_msgarg outArg = alljoyn_msgarg_create();
    QStatus status = alljoyn_msgarg_set(outArg, "b", answer);

    if (ER_OK != status) {
        fprintf(stderr, "Failed to do alljoyn_msgarg_set - status (%s)\n", QCC_StatusText(status));
        alljoyn_msgarg_destroy(outArg);
        return;
    }

    status = alljoyn_busobject_methodreply_args(object, message, outArg, 1);
    if (status != ER_OK) {
        fprintf(stderr, "Failed to do alljoyn_busobject_methodreply_args - status (%s)\n", QCC_StatusText(status));
    }

    alljoyn_msgarg_destroy(outArg);
}

static void SendDoorEvent(alljoyn_busobject object)
{
    printf("Sending door event ...\n");
    alljoyn_msgarg outArg = alljoyn_msgarg_create();
    QStatus status = alljoyn_msgarg_set(outArg, "b", s_door.doorOpened);

    if (ER_OK != status) {
        fprintf(stderr, "Failed to do alljoyn_msgarg_set - status (%s)\n", QCC_StatusText(status));
        alljoyn_msgarg_destroy(outArg);
        return;
    }

    status = alljoyn_busobject_signal(object,
                                      nullptr,
                                      ALLJOYN_SESSION_ID_ALL_HOSTED,
                                      s_door.stateSignal,
                                      outArg,
                                      1,
                                      0,
                                      0,
                                      nullptr);
    if (status != ER_OK) {
        fprintf(stderr, "Failed to do alljoyn_busobject_signal - status (%s)\n", QCC_StatusText(status));
    }

    alljoyn_msgarg_destroy(outArg);
}

static void AJ_CALL DoorOpenHandler(alljoyn_busobject object, const alljoyn_interfacedescription_member* member, alljoyn_message message)
{
    QCC_UNUSED(member);

    printf("Door Open method was called\n");
    if (!(s_door.doorOpened)) {
        s_door.doorOpened = TRUE;

        if (s_door.signalDoorOperations) {
            SendDoorEvent(object);
        }
    }

    ReplyWithBoolean(object, TRUE, message);
}

static void AJ_CALL DoorCloseHandler(alljoyn_busobject object, const alljoyn_interfacedescription_member* member, alljoyn_message message)
{
    QCC_UNUSED(member);

    printf("Door Close method was called\n");
    if (s_door.doorOpened) {
        s_door.doorOpened = FALSE;

        if (s_door.signalDoorOperations) {
            SendDoorEvent(object);
        }
    }

    ReplyWithBoolean(object, TRUE, message);
}

static void AJ_CALL DoorGetStateHandler(alljoyn_busobject object, const alljoyn_interfacedescription_member* member, alljoyn_message message)
{
    QCC_UNUSED(member);

    printf("Door GetState method was called\n");
    ReplyWithBoolean(object, s_door.doorOpened, message);
}

static QStatus AJ_CALL ProviderPropertyGet(const void* context,
                                           AJ_PCSTR ifcName,
                                           AJ_PCSTR propName,
                                           alljoyn_msgarg val)
{
    printf("alljoyn_busobject_prop_get_ptr(%s)@%s\n", propName, ifcName);
    unsigned char doorOpened = *((unsigned char*)context);
    // Only one property is available
    if (strcmp(ifcName, DOOR_INTERFACE) == 0 && strcmp(propName, DOOR_STATE) == 0) {
        alljoyn_msgarg_set(val, "b", doorOpened);
        return ER_OK;
    }
    return ER_BUS_NO_SUCH_PROPERTY;
}

static alljoyn_busobject_callbacks s_providerCallbacks = {
    ProviderPropertyGet,
    nullptr,
    nullptr,
    nullptr
};

static QStatus SetMethodHandler(alljoyn_busattachment bus, AJ_PCSTR memberName, alljoyn_messagereceiver_methodhandler_ptr methodHandler)
{
    const alljoyn_interfacedescription doorInterface = alljoyn_busattachment_getinterface(bus, DOOR_INTERFACE);
    if (nullptr == doorInterface) {
        fprintf(stderr, "Failed to getinterface %s\n", DOOR_INTERFACE);
        return ER_FAIL;
    }

    alljoyn_interfacedescription_member member;
    if (QCC_FALSE == alljoyn_interfacedescription_getmember(doorInterface, memberName, &member)) {
        fprintf(stderr, "Failed to get member %s\n", memberName);
        return ER_FAIL;
    }

    QStatus status = alljoyn_busobject_addmethodhandler(s_door.busObject, member, methodHandler, nullptr);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to do alljoyn_busobject_addmethodhandler - status (%s)\n", QCC_StatusText(status));
    }

    return status;
}

static QStatus SetMethodHandlers(alljoyn_busattachment bus)
{
    QStatus status = SetMethodHandler(bus, DOOR_OPEN, DoorOpenHandler);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to add %s handler - status (%s)\n", DOOR_OPEN, QCC_StatusText(status));
        return status;
    }

    status = SetMethodHandler(bus, DOOR_CLOSE, DoorCloseHandler);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to add %s handler - status (%s)\n", DOOR_CLOSE, QCC_StatusText(status));
        return status;
    }

    status = SetMethodHandler(bus, DOOR_GET_STATE, DoorGetStateHandler);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to add %s handler - status (%s)\n", DOOR_GET_STATE, QCC_StatusText(status));
    }

    return status;
}

static QStatus SetupProviderObject(alljoyn_busattachment bus)
{
    QStatus status = ER_OK;
    s_door.doorOpened = FALSE;
    s_door.signalDoorOperations = FALSE;
    s_door.busObject = alljoyn_busobject_create(DOOR_OBJECT_PATH, QCC_FALSE, &s_providerCallbacks, &(s_door.doorOpened));

    const alljoyn_interfacedescription doorInterface = alljoyn_busattachment_getinterface(bus, DOOR_INTERFACE);
    if (nullptr == doorInterface) {
        fprintf(stderr, "Failed to getinterface\n");
        return ER_FAIL;
    }

    status = alljoyn_busobject_addinterface_announced(s_door.busObject, doorInterface);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to addinterface_announced - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    if (QCC_FALSE == alljoyn_interfacedescription_getmember(doorInterface, DOOR_STATE_CHANGED, &(s_door.stateSignal))) {
        fprintf(stderr, "Failed to get member %s\n", DOOR_STATE_CHANGED);
        return ER_FAIL;
    }

    status = SetMethodHandlers(bus);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to SetMethodHandlers - status (%s)\n", QCC_StatusText(status));
    }

    return status;
}

static QStatus SetUpProviderSecurity(CommonDoorData* doorData)
{
    QStatus status = ER_OK;

    alljoyn_authlistener_callbacks emptyCallbacks = { 0 };
    doorData->authListener = alljoyn_authlistener_create(&emptyCallbacks, nullptr);

    status = alljoyn_authlistener_setsharedsecret(doorData->authListener, s_sharedSecret, ArraySize(s_sharedSecret));
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setup shared secret - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_busattachment_enablepeersecurity(doorData->bus,
                                                      KEYX_ECDHE_DSA " " KEYX_ECDHE_NULL " " KEYX_ECDHE_SPEKE,
                                                      doorData->authListener,
                                                      nullptr,
                                                      QCC_FALSE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to enablepeersecurity - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    const alljoyn_permissionconfigurator configurator = alljoyn_busattachment_getpermissionconfigurator(doorData->bus);
    status = alljoyn_permissionconfigurator_setclaimcapabilities(configurator, CAPABLE_ECDHE_NULL | CAPABLE_ECDHE_SPEKE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setclaimcapabilities - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_permissionconfigurator_setclaimcapabilitiesadditionalinfo(configurator, PASSWORD_GENERATED_BY_APPLICATION);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setclaimcapabilitiesadditionalinfo - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_permissionconfigurator_setmanifesttemplatefromxml(configurator, s_providerManifestTemplate);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to SetPermissionManifestTemplate - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    alljoyn_applicationstate appState;
    status = alljoyn_permissionconfigurator_getapplicationstate(configurator, &appState);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to GetApplicationState - status (%s)\n", QCC_StatusText(status));
    }

    if (CLAIMABLE == appState) {
        printf("Door provider is not claimed.\n");
        printf("The provider can be claimed using SPEKE.\n");
        printf("Shared secret = (%s)\n", s_sharedSecret);
    }

    return status;
}

static QStatus UpdateDoorProviderManifest(const CommonDoorData* doorData)
{
    alljoyn_permissionconfigurator configurator = alljoyn_busattachment_getpermissionconfigurator(doorData->bus);

    QStatus status = alljoyn_permissionconfigurator_setmanifesttemplatefromxml(configurator, s_providerManifestTemplateWithSignal);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setmanifesttemplatefromxml - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_permissionconfigurator_setapplicationstate(configurator, NEED_UPDATE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setapplicationstate - status (%s)\n", QCC_StatusText(status));
    }

    return status;
}

static void PrintHelp()
{
    printf("Welcome to the door provider - enter 'h' for this menu\n"
           "Menu\n"
           ">u : Enable automatic signaling of door events.\n"
           ">s : Signal current door state\n"
           ">q : Quit\n");
}

static void ExecuteCommands(const CommonDoorData* doorData)
{
    QStatus status;
    char cmd = 'h';
    do {
        printf(">");
        int scanResult = scanf("%c", &cmd);
#ifdef NDEBUG
        QCC_UNUSED(scanResult);
#else
        QCC_ASSERT(EOF != scanResult);
#endif

        switch (cmd) {
        case 'u':
            printf("Enabling automatic signaling of door events ... ");
            status = UpdateDoorProviderManifest(doorData);
            if (ER_OK != status) {
                fprintf(stderr, "Failed to UpdateDoorProviderManifest - status (%s)\n", QCC_StatusText(status));
                break;
            }
            s_door.signalDoorOperations = true;
            printf("done\n");
            break;

        case 's':
            SendDoorEvent(s_door.busObject);
            break;

        case 'h':
            PrintHelp();
            break;

        case 'q':
        case '\n':
        case '\r':
            break;

        default:
            fprintf(stderr, "Unknown command!\n");
            PrintHelp();
        }
    } while ('q' != cmd);
}

int CDECL_CALL main(int argc, char** argv)
{
    AJ_PCSTR appName = "DoorProvider";
    if (argc > 1) {
        appName = argv[1];
    }

    printf("Starting door provider %s\n", appName);

    if (alljoyn_init() != ER_OK) {
        return EXIT_FAILURE;
    }

#ifdef ROUTER
    if (alljoyn_routerinit() != ER_OK) {
        alljoyn_shutdown();
        return EXIT_FAILURE;
    }
#endif

    // Do the common set-up

    CommonDoorData doorData = { 0 };
    QStatus status = CommonDoorSetUp(appName, &doorData);

    if (ER_OK != status) {
        fprintf(stderr, "Failed to initialize common door settings - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = SetUpProviderSecurity(&doorData);

    if (ER_OK != status) {
        fprintf(stderr, "Failed to set up provider security settings - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = HostSession(&doorData);

    if (ER_OK != status) {
        fprintf(stderr, "Failed to host provider session - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = SetupProviderObject(doorData.bus);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to SetupProviderObject - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = alljoyn_busattachment_registerbusobject(doorData.bus, s_door.busObject);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to registerbusobject - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = AnnounceAboutData(&doorData, appName);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to AnnounceAboutData - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = WaitToBeClaimed(doorData.bus);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to WaitToBeClaimed - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    printf("Door provider initialized; Waiting for consumers ...\n");
    PrintHelp();
    ExecuteCommands(&doorData);

Exit:
    CommonDoorTearDown(&doorData);

#ifdef ROUTER
    alljoyn_routershutdown();
#endif

    alljoyn_shutdown();
    return (int)status;
}
