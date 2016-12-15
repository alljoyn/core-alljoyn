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

#ifndef ALLJOYN_SAMPLE_C_SECURE_DOOR_COMMON_H_
#define ALLJOYN_SAMPLE_C_SECURE_DOOR_COMMON_H_

#include <cstdio>
#include <cstring>
#include <stdlib.h>

#include <alljoyn_c/AboutObj.h>
#include <alljoyn_c/AuthListener.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/BusObject.h>
#include <alljoyn_c/Init.h>
#include <alljoyn_c/PermissionConfigurationListener.h>

extern AJ_PCSTR g_doorInterfaceXml;

/* Door sample common definitions. */
#define DOOR_INTERFACE "sample.securitymgr.door.Door"
#define DOOR_OPEN "Open"
#define DOOR_CLOSE "Close"
#define DOOR_GET_STATE "GetState"
#define DOOR_STATE "State"
#define DOOR_STATE_CHANGED "StateChanged"

#define DOOR_OBJECT_PATH "/sample/security/Door"

#define KEYX_ECDHE_NULL "ALLJOYN_ECDHE_NULL"
#define KEYX_ECDHE_SPEKE "ALLJOYN_ECDHE_SPEKE"
#define KEYX_ECDHE_DSA "ALLJOYN_ECDHE_ECDSA"

#define DOOR_APPLICATION_PORT 12345
#define CLAIM_WAIT_SLEEP_DURATION_MS 500
#define TRUE 1
#define FALSE 0

#define ArraySize(a)  (sizeof(a) / sizeof(a[0]))

typedef struct {
    alljoyn_busattachment bus;
    alljoyn_aboutdata aboutData;
    alljoyn_aboutobj aboutObj;
    alljoyn_sessionportlistener spl;
    alljoyn_authlistener authListener;
    alljoyn_permissionconfigurationlistener permissionConfigurationListener;
    volatile QCC_BOOL endManagementCalled;
} CommonDoorData;

QStatus CommonDoorSetUp(AJ_PCSTR appName, CommonDoorData* doorData);
void SetPermissionConfigurationListener(CommonDoorData* doorData);
void AJ_CALL EndManagementCallback(const void* context);
QStatus HostSession(const CommonDoorData* doorData);
QStatus AnnounceAboutData(CommonDoorData* doorData, AJ_PCSTR appName);
QStatus WaitToBeClaimed(CommonDoorData* doorData);
QStatus SetSecurityForClaimedMode(CommonDoorData* doorData);
void CommonDoorTearDown(CommonDoorData* doorData);

#endif /* ALLJOYN_SAMPLE_C_SECURE_DOOR_COMMON_H_ */