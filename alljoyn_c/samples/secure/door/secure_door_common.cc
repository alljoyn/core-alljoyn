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

#if defined(QCC_OS_GROUP_WINDOWS)
#include <Winsock2.h> // gethostname
#endif

AJ_PCSTR g_doorInterfaceXml =
    "<interface name=\"" DOOR_INTERFACE "\">"
    "  <annotation name = \"org.alljoyn.Bus.Secure\" value=\"true\"/>"
    "  <method name=\"" DOOR_OPEN "\">"
    "    <arg name=\"success\" type=\"b\" direction=\"out\"/>\n"
    "  </method>"
    "  <method name=\"" DOOR_CLOSE "\">"
    "    <arg name=\"success\" type=\"b\" direction=\"out\"/>\n"
    "  </method>"
    "  <method name=\"" DOOR_GET_STATE "\">"
    "    <arg name=\"state\" type=\"b\" direction=\"out\"/>\n"
    "  </method>"
    "  <signal name=\"" DOOR_STATE_CHANGED "\">"
    "    <arg name=\"state\" type=\"b\" direction=\"out\"/>\n"
    "  </signal>"
    "  <property name=\"" DOOR_STATE "\" type=\"b\" access=\"readwrite\"/>"
    "</interface>";

static uint8_t s_appId[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };

static QCC_BOOL AJ_CALL AcceptSessionJoin(const void* context,
                                          alljoyn_sessionport sessionPort,
                                          AJ_PCSTR joiner,
                                          const alljoyn_sessionopts opts)
{
    QCC_UNUSED(context);
    QCC_UNUSED(sessionPort);
    QCC_UNUSED(joiner);
    QCC_UNUSED(opts);

    return QCC_TRUE;
}

static alljoyn_sessionportlistener_callbacks s_splCallbacks = {
    AcceptSessionJoin,
    nullptr
};

static QStatus CommonInit(CommonDoorData* doorData)
{
    QStatus status = alljoyn_busattachment_createinterfacesfromxml(doorData->bus, g_doorInterfaceXml);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to create door's interface - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_busattachment_start(doorData->bus);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to Start bus attachment - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_busattachment_connect(doorData->bus, nullptr);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to Connect bus attachment - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    return status;
}

static QStatus SetAboutData(CommonDoorData* doorData, AJ_PCSTR appName)
{
    QStatus status = alljoyn_aboutdata_setappid(doorData->aboutData, s_appId, ArraySize(s_appId));
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setappid - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    char buf[64];
    gethostname(buf, ArraySize(buf));
    status = alljoyn_aboutdata_setdevicename(doorData->aboutData, buf, nullptr);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setdevicename - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_aboutdata_setdeviceid(doorData->aboutData, appName);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setdeviceid - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_aboutdata_setappname(doorData->aboutData, appName, nullptr);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setappname - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_aboutdata_setmanufacturer(doorData->aboutData, "Manufacturer", nullptr);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setmanufacturer - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_aboutdata_setmodelnumber(doorData->aboutData, "1");
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setmodelnumber - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_aboutdata_setdescription(doorData->aboutData, appName, nullptr);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setmodelnumber - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_aboutdata_setdateofmanufacture(doorData->aboutData, "2016-07-21");
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setdateofmanufacture - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_aboutdata_setsoftwareversion(doorData->aboutData, "0.1");
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setsoftwareversion - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_aboutdata_sethardwareversion(doorData->aboutData, "0.0.1");
    if (ER_OK != status) {
        fprintf(stderr, "Failed to sethardwareversion - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_aboutdata_setsupporturl(doorData->aboutData, "https://allseenalliance.org/");
    if (ER_OK != status) {
        fprintf(stderr, "Failed to setsupporturl - status (%s)\n", QCC_StatusText(status));
    }

    return status;
}

QStatus CommonDoorSetUp(AJ_PCSTR appName, CommonDoorData* doorData)
{
    doorData->bus = alljoyn_busattachment_create(appName, QCC_TRUE);
    doorData->aboutData = alljoyn_aboutdata_create("en");
    doorData->aboutObj = alljoyn_aboutobj_create(doorData->bus, UNANNOUNCED);
    doorData->spl = alljoyn_sessionportlistener_create(&s_splCallbacks, nullptr);
    doorData->authListener = nullptr;

    return CommonInit(doorData);
}

QStatus HostSession(const CommonDoorData* doorData)
{
    QStatus status = ER_OK;
    alljoyn_sessionopts opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
    alljoyn_sessionport port = DOOR_APPLICATION_PORT;

    status = alljoyn_busattachment_bindsessionport(doorData->bus, &port, opts, doorData->spl);
    alljoyn_sessionopts_destroy(opts);

    return status;
}

QStatus AnnounceAboutData(CommonDoorData* doorData, AJ_PCSTR appName)
{
    QStatus status = SetAboutData(doorData, appName);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to SetAboutData - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    if (!alljoyn_aboutdata_isvalid(doorData->aboutData, nullptr)) {
        fprintf(stderr, "Invalid aboutData\n");
        return ER_FAIL;
    }

    return alljoyn_aboutobj_announce(doorData->aboutObj, DOOR_APPLICATION_PORT, doorData->aboutData);
}

QStatus WaitToBeClaimed(alljoyn_busattachment bus)
{
    uint32_t waitIteration = 0;
    QStatus status = ER_OK;
    alljoyn_applicationstate appState = CLAIMABLE;
    alljoyn_permissionconfigurator configurator = alljoyn_busattachment_getpermissionconfigurator(bus);

    status = alljoyn_permissionconfigurator_getapplicationstate(configurator, &appState);
    while ((CLAIMED != appState) && (ER_OK == status)) {
#if defined(QCC_OS_GROUP_WINDOWS)
        Sleep(250);
#else
        usleep(250 * 1000);
#endif
        printf("Waiting to be claimed... %u\n", waitIteration++);
        status = alljoyn_permissionconfigurator_getapplicationstate(configurator, &appState);
    }

    if (ER_OK != status) {
        fprintf(stderr, "Failed to retrieve application's state - status (%s)\n", QCC_StatusText(status));
    } else {
        printf("App has been claimed.\n");
    }

    return status;
}

QStatus SetSecurityForClaimedMode(CommonDoorData* doorData)
{
    QStatus status = alljoyn_busattachment_enablepeersecurity(doorData->bus, "", nullptr, nullptr, QCC_TRUE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to clear peer security - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    status = alljoyn_busattachment_enablepeersecurity(doorData->bus, KEYX_ECDHE_DSA, doorData->authListener, nullptr, QCC_FALSE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to set peer security for claimed mode - status (%s)\n", QCC_StatusText(status));
        return status;
    }

    return ER_OK;
}

void CommonDoorTearDown(CommonDoorData* doorData)
{
    QStatus status = alljoyn_busattachment_enablepeersecurity(doorData->bus, "", nullptr, nullptr, QCC_TRUE);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to shut down peer security - status (%s)\n", QCC_StatusText(status));
    }

    alljoyn_aboutobj_destroy(doorData->aboutObj);
    doorData->aboutObj = nullptr;

    alljoyn_aboutdata_destroy(doorData->aboutData);
    doorData->aboutData = nullptr;

    alljoyn_sessionportlistener_destroy(doorData->spl);
    doorData->spl = nullptr;

    status = alljoyn_busattachment_disconnect(doorData->bus, nullptr);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to disconnect bus attachment - status (%s)\n", QCC_StatusText(status));
    }

    status = alljoyn_busattachment_stop(doorData->bus);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to stop bus attachment - status (%s)\n", QCC_StatusText(status));
    }

    status = alljoyn_busattachment_join(doorData->bus);
    if (ER_OK != status) {
        fprintf(stderr, "Failed to join bus attachment's threads - status (%s)\n", QCC_StatusText(status));
    }

    alljoyn_busattachment_destroy(doorData->bus);
    doorData->bus = nullptr;

    if (nullptr != doorData->authListener) {
        alljoyn_authlistener_destroy(doorData->authListener);
    }
}
