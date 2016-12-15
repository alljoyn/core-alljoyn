/******************************************************************************
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
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
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "secure_door_common.h"

#define REMOTE_CALL_TIMEOUT_MS 5000

#define ENGLISH_LANGUAGE "en"
#define SECURITY_VIOLATION_ERROR_NAME "org.alljoyn.Bus.SecurityViolation"
#define DOOR_SIGNAL_MATCH_RULE  "type='signal',interface='" DOOR_INTERFACE "',member='" DOOR_STATE_CHANGED "'"

typedef struct Provider {
    AJ_PSTR m_providerBusName;
    alljoyn_proxybusobject m_remoteObject;
    alljoyn_sessionid m_sessionId;
    Provider* m_next;
} Provider;

typedef struct {
    Provider* head;
} ProviderList;

static AJ_PCSTR s_consumerManifestTemplate =
    "<manifest>"
    "<node>"
    "<interface = \"" DOOR_INTERFACE "\">"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

static ProviderList s_providersList = { nullptr };

static Provider* NewProvider(AJ_PCSTR busName)
{
    Provider* provider = (Provider*)malloc(sizeof(Provider));
    QCC_ASSERT(nullptr != provider);

    provider->m_next = nullptr;
    provider->m_remoteObject = nullptr;
    provider->m_sessionId = 0U;

    size_t providerBusNameLength = strlen(busName);
    provider->m_providerBusName = (AJ_PSTR)malloc(providerBusNameLength + 1);
    memcpy(provider->m_providerBusName, busName, providerBusNameLength + 1);

    return provider;
}

static void DeleteProviders(alljoyn_busattachment bus)
{
    Provider* current = s_providersList.head;
    while (nullptr != current) {
        Provider* next = current->m_next;

        free(current->m_providerBusName);

        if (nullptr != current->m_remoteObject) {
            alljoyn_proxybusobject_destroy(current->m_remoteObject);
            current->m_remoteObject = nullptr;

            alljoyn_busattachment_leavesession(bus, current->m_sessionId);
        }

        free(current);

        current = next;
    }
}

static void InsertNewProvider(AJ_PCSTR providerBusName)
{
    Provider* newProvider = NewProvider(providerBusName);

    if (nullptr == s_providersList.head) {
        s_providersList.head = newProvider;
    } else {
        Provider* current = s_providersList.head;

        while (nullptr != current->m_next) {
            current = current->m_next;
        }
        current->m_next = newProvider;
    }
}

static QStatus SetUpConsumerSecurity(CommonDoorData* doorData)
{
    QStatus status = ER_OK;

    alljoyn_authlistener_callbacks emptyCallbacks = { 0 };
    doorData->authListener = alljoyn_authlistener_create(&emptyCallbacks, nullptr);

    status = alljoyn_busattachment_enablepeersecuritywithpermissionconfigurationlistener(doorData->bus,
                                                                                         KEYX_ECDHE_DSA " " KEYX_ECDHE_NULL,
                                                                                         doorData->authListener,
                                                                                         nullptr,
                                                                                         QCC_TRUE,
                                                                                         doorData->permissionConfigurationListener);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to enablepeersecurity - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    const alljoyn_permissionconfigurator configurator = alljoyn_busattachment_getpermissionconfigurator(doorData->bus);

    status = alljoyn_permissionconfigurator_setmanifesttemplatefromxml(configurator, s_consumerManifestTemplate);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to SetPermissionManifestTemplate - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    return status;
}

static QStatus ReadBooleanFromMsgArg(alljoyn_msgarg result, unsigned char* value)
{
    QStatus status = alljoyn_msgarg_get(result, "b", value);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to Get boolean - status (%s)\n", QCC_StatusText(status));
    }

    return status;
}

static QStatus ReadBoolean(alljoyn_message message, unsigned char* value)
{
    alljoyn_msgarg result = alljoyn_message_getarg(message, 0);
    return ReadBooleanFromMsgArg(result, value);
}

static void AJ_CALL ReceiveDoorSignal(const alljoyn_interfacedescription_member* member, AJ_PCSTR srcPath, alljoyn_message message)
{
    QCC_UNUSED(srcPath);
    QCC_UNUSED(member);

    unsigned char value;
    QStatus status = ReadBoolean(message, &value);
    if (ER_OK == status) {
        printf("Received door %s event ...\n", value == TRUE ? "opened" : "closed");
    }
}

static QStatus RegisterDoorSignalHandler(alljoyn_busattachment bus)
{
    QStatus status;
    alljoyn_interfacedescription_member stateSignal;

    alljoyn_interfacedescription doorInterface = alljoyn_busattachment_getinterface(bus, DOOR_INTERFACE);
    if (QCC_FALSE == alljoyn_interfacedescription_getmember(doorInterface, DOOR_STATE_CHANGED, &stateSignal)) {
        fprintf(stderr, "Failed to get member %s\n", DOOR_STATE_CHANGED);
        return ER_FAIL;
    }

    status = alljoyn_busattachment_registersignalhandlerwithrule(bus, ReceiveDoorSignal, stateSignal, DOOR_SIGNAL_MATCH_RULE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to register signal handler - status (%s)\n", QCC_StatusText(status));
    }

    return status;
}

static void AJ_CALL ProviderAboutReceived(const void* context,
                                          AJ_PCSTR busName,
                                          uint16_t version,
                                          alljoyn_sessionport port,
                                          const alljoyn_msgarg objectDescriptionArg,
                                          const alljoyn_msgarg aboutDataArg)
{
    QCC_UNUSED(objectDescriptionArg);
    QCC_UNUSED(port);
    QCC_UNUSED(version);
    QCC_UNUSED(context);

    AJ_PSTR appName = nullptr;
    AJ_PSTR deviceName = nullptr;
    alljoyn_aboutdata providerAboutData = alljoyn_aboutdata_create(ENGLISH_LANGUAGE);
    QStatus status = alljoyn_aboutdata_createfrommsgarg(providerAboutData, aboutDataArg, ENGLISH_LANGUAGE);

    if (ER_OK != status) {
        fprintf(stderr, "Failed to createfrommsgarg - status (%s)\n", QCC_StatusText(status));
        return;
    }

    alljoyn_aboutdata_getappname(providerAboutData, &appName, ENGLISH_LANGUAGE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to getappname - status (%s)\n", QCC_StatusText(status));
        return;
    }

    alljoyn_aboutdata_getdevicename(providerAboutData, &deviceName, ENGLISH_LANGUAGE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to getappname - status (%s)\n", QCC_StatusText(status));
        return;
    }

    printf("Found door %s @ %s (%s)\n", appName, busName, deviceName);

    InsertNewProvider(busName);
}

static QStatus JoinSession(alljoyn_busattachment bus, Provider* provider)
{
    QStatus status;
    alljoyn_sessionopts sessionOpts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES,
                                                                 QCC_FALSE,
                                                                 ALLJOYN_PROXIMITY_ANY,
                                                                 ALLJOYN_TRANSPORT_ANY);

    status = alljoyn_busattachment_joinsession(bus,
                                               provider->m_providerBusName,
                                               DOOR_APPLICATION_PORT,
                                               nullptr,
                                               &(provider->m_sessionId),
                                               sessionOpts);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to joinsession - status (%s)\n", QCC_StatusText(status));
    }
    alljoyn_sessionopts_destroy(sessionOpts);

    return status;
}

static QStatus GetProxyDoorObject(alljoyn_busattachment bus, Provider* provider)
{
    if (nullptr != provider->m_remoteObject) {
        return ER_OK;
    }

    QStatus status = ER_OK;

    status = JoinSession(bus, provider);
    if (ER_OK != status) {
        return status;
    }

    const alljoyn_interfacedescription doorInterface = alljoyn_busattachment_getinterface(bus, DOOR_INTERFACE);
    if (nullptr == doorInterface) {
        fprintf(stderr, "Failed to getinterface\n");
        alljoyn_busattachment_leavesession(bus, provider->m_sessionId);
        provider->m_sessionId = 0U;
        return ER_FAIL;
    }

    provider->m_remoteObject = alljoyn_proxybusobject_create(bus,
                                                             provider->m_providerBusName,
                                                             DOOR_OBJECT_PATH,
                                                             provider->m_sessionId);
    if (nullptr == provider->m_remoteObject) {
        fprintf(stderr, "Failed to create alljoyn_proxybusobject - status (%s)\n", QCC_StatusText(status));
        alljoyn_busattachment_leavesession(bus, provider->m_sessionId);
        provider->m_sessionId = 0U;
        return ER_FAIL;
    }

    status = alljoyn_proxybusobject_addinterface(provider->m_remoteObject, doorInterface);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to addinterface - status (%s)\n", QCC_StatusText(status));
        alljoyn_proxybusobject_destroy(provider->m_remoteObject);
        provider->m_remoteObject = nullptr;
        alljoyn_busattachment_leavesession(bus, provider->m_sessionId);
        provider->m_sessionId = 0U;
    }

    return status;
}

static void MethodCall(alljoyn_busattachment bus, Provider* provider, AJ_PCSTR methodName)
{
    unsigned char value;
    alljoyn_message reply;
    size_t errorNameLength;
    alljoyn_proxybusobject remoteObject;
    AJ_PCSTR errorName = nullptr;

    QStatus status = GetProxyDoorObject(bus, provider);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to GetProxyDoorObject - status (%s)\n", QCC_StatusText(status));
        return;
    }

    printf("Calling %s on '%s'\n", methodName, provider->m_providerBusName);
    remoteObject = provider->m_remoteObject;
    reply = alljoyn_message_create(bus);
    status = alljoyn_proxybusobject_methodcall(remoteObject,
                                               DOOR_INTERFACE,
                                               methodName,
                                               nullptr,
                                               0,
                                               reply,
                                               REMOTE_CALL_TIMEOUT_MS,
                                               0);

    // Retry on policy/identity update
    errorNameLength = 0U;
    errorName = alljoyn_message_geterrorname(reply, nullptr, &errorNameLength);
    if ((ER_BUS_REPLY_IS_ERROR_MESSAGE == status) &&
        (nullptr != errorName) &&
        (strcmp(errorName, SECURITY_VIOLATION_ERROR_NAME) == 0)) {
        status = alljoyn_proxybusobject_methodcall(remoteObject,
                                                   DOOR_INTERFACE,
                                                   methodName,
                                                   nullptr,
                                                   0,
                                                   reply,
                                                   REMOTE_CALL_TIMEOUT_MS,
                                                   0);
    }

    if (ER_OK != status) {
        fprintf(stderr, "Failed to call method %s - status (%s)\n", methodName, QCC_StatusText(status));
        alljoyn_message_destroy(reply);
        return;
    }

    status = ReadBoolean(reply, &value);
    if (ER_OK == status) {
        printf("%s returned %d\n", methodName, value);
    }

    alljoyn_message_destroy(reply);
}

static void GetProperty(alljoyn_busattachment bus, Provider* provider, AJ_PCSTR propertyName)
{
    unsigned char value;
    alljoyn_msgarg msgArg;
    alljoyn_proxybusobject remoteObject;

    QStatus status = GetProxyDoorObject(bus, provider);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to GetProxyDoorObject - status (%s)\n", QCC_StatusText(status));
        return;
    }

    printf("Retrieving property %s on '%s'\n", propertyName, provider->m_providerBusName);
    remoteObject = provider->m_remoteObject;
    msgArg = alljoyn_msgarg_create();
    status = alljoyn_proxybusobject_getproperty(remoteObject, DOOR_INTERFACE, propertyName, msgArg);

    // Retry on policy/identity update
    if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
        // Impossible to check specific error message (see ASACORE-1811)
        status = alljoyn_proxybusobject_getproperty(remoteObject, DOOR_INTERFACE, propertyName, msgArg);
    }

    if (ER_OK != status) {
        fprintf(stderr, "Failed to get property %s - status (%s)\n", propertyName, QCC_StatusText(status));
        alljoyn_msgarg_destroy(msgArg);
        return;
    }

    status = ReadBooleanFromMsgArg(msgArg, &value);
    if (ER_OK == status) {
        printf("%s returned %d\n", propertyName, value);
    }

    alljoyn_msgarg_destroy(msgArg);
}

static void PerformDoorAction(alljoyn_busattachment bus, char cmd, Provider* provider)
{
    AJ_PCSTR methodName = nullptr;
    AJ_PCSTR propertyName = DOOR_STATE;

    switch (cmd) {
    case 'o':
        methodName = DOOR_OPEN;
        break;

    case 'c':
        methodName = DOOR_CLOSE;
        break;

    case 's':
        methodName = DOOR_GET_STATE;
        break;

    case 'g':
        break;
    }

    if (nullptr != methodName) {
        MethodCall(bus, provider, methodName);
    } else {
        GetProperty(bus, provider, propertyName);
    }
}

static void PrintHelp()
{
    printf("Welcome to the door consumer - enter 'h' for this menu\n"
           "Menu\n"
           ">o : Open doors\n"
           ">c : Close doors\n"
           ">s : Doors state - using ProxyBusObject->MethodCall\n"
           ">g : Get doors state - using ProxyBusObject->GetProperty\n"
           ">q : Quit\n");
}

static void ExecuteCommands(const CommonDoorData* doorData)
{
    PrintHelp();

    char cmd = 'h';
    do {
        printf(">");
        if (scanf("%c", &cmd) == EOF) {
            break;
        }

        switch (cmd) {
        case 'o':
        case 's':
        case 'c':
        case 'g':
            {
                Provider* currentProvider = s_providersList.head;
                if (nullptr == currentProvider) {
                    printf("No doors found.\n");
                }
                while (nullptr != currentProvider) {
                    PerformDoorAction(doorData->bus, cmd, currentProvider);
                    currentProvider = currentProvider->m_next;
                }
            }
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
    AJ_PCSTR appName = "DoorConsumer";
    if (argc > 1) {
        appName = argv[1];
    }

    printf("Starting door consumer %s\n", appName);

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
    alljoyn_aboutlistener aboutListener = nullptr;
    alljoyn_aboutlistener_callback aboutListenerCallbacks = {
        ProviderAboutReceived
    };
    QStatus status = CommonDoorSetUp(appName, &doorData);

    if (ER_OK != status) {
        fprintf(stderr, "Failed to initialize common door settings - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = SetUpConsumerSecurity(&doorData);

    if (ER_OK != status) {
        fprintf(stderr, "Failed to set up consumer security settings - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = HostSession(&doorData);

    if (ER_OK != status) {
        fprintf(stderr, "Failed to host consumer session - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = AnnounceAboutData(&doorData, appName);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to AnnounceAboutData - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = WaitToBeClaimed(&doorData);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to WaitToBeClaimed - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = SetSecurityForClaimedMode(&doorData);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to SetSecurityForClaimedMode - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = alljoyn_busattachment_whoimplements_interface(doorData.bus, DOOR_INTERFACE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to call whoimplements_interface - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    status = RegisterDoorSignalHandler(doorData.bus);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to RegisterDoorSignalHandler - status (%s)\n", QCC_StatusText(status));
        goto Exit;
    }

    aboutListener = alljoyn_aboutlistener_create(&aboutListenerCallbacks, nullptr);
    alljoyn_busattachment_registeraboutlistener(doorData.bus, aboutListener);

    ExecuteCommands(&doorData);

Exit:
    DeleteProviders(doorData.bus);
    alljoyn_busattachment_unregisterallaboutlisteners(doorData.bus);
    CommonDoorTearDown(&doorData);

#ifdef ROUTER
    alljoyn_routershutdown();
#endif

    alljoyn_shutdown();
    return (int)status;
}