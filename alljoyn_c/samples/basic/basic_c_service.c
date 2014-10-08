/**
 * @file
 * @brief Sample implementation of an AllJoyn service in C.
 *
 * This sample will show how to set up an AllJoyn service that will registered with the
 * wellknown name 'org.alljoyn.Bus.method_sample'.  The service will register a method call
 * with the name 'cat'  this method will take two input strings and return a
 * Concatenated version of the two strings.
 *
 */

/******************************************************************************
 * Copyright (c) 2010-2014, AllSeen Alliance. All rights reserved.
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
#ifndef _WIN32
#define _BSD_SOURCE /* usleep */
#endif
#ifdef _WIN32
/* To avoid compilation error in VS generated due to use of snprintf */
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <qcc/platform.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <alljoyn_c/DBusStdDefines.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/version.h>
#include <Status.h>

#include <Status.h>

/** Static top level message bus object */
static alljoyn_busattachment g_msgBus = NULL;

/* Static SessionPortListener */
static alljoyn_sessionportlistener s_sessionPortListener = NULL;

/* Static BusListener */
static alljoyn_buslistener g_busListener = NULL;

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.Bus.sample";
static const char* OBJECT_NAME = "org.alljoyn.Bus.sample";
static const char* OBJECT_PATH = "/sample";
static const alljoyn_sessionport SERVICE_PORT = 25;

static volatile sig_atomic_t g_interrupt = QCC_FALSE;

static void SigIntHandler(int sig)
{
    g_interrupt = QCC_TRUE;
}

/* ObjectRegistered callback */
void AJ_CALL busobject_object_registered(const void* context)
{
    printf("ObjectRegistered has been called\n");
}

/* NameOwnerChanged callback */
void AJ_CALL name_owner_changed(const void* context, const char* busName, const char* previousOwner, const char* newOwner)
{
    if (newOwner && (0 == strcmp(busName, OBJECT_NAME))) {
        printf("name_owner_changed: name=%s, oldOwner=%s, newOwner=%s\n",
               busName,
               previousOwner ? previousOwner : "<none>",
               newOwner ? newOwner : "<none>");
    }
}

/* AcceptSessionJoiner callback */
QCC_BOOL AJ_CALL accept_session_joiner(const void* context, alljoyn_sessionport sessionPort,
                                       const char* joiner,  const alljoyn_sessionopts opts)
{
    QCC_BOOL ret = QCC_FALSE;
    if (sessionPort != SERVICE_PORT) {
        printf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
    } else {
        printf("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x)\n",
               joiner, alljoyn_sessionopts_get_proximity(opts), alljoyn_sessionopts_get_traffic(opts), alljoyn_sessionopts_get_transports(opts));
        ret = QCC_TRUE;
    }
    return ret;
}

/* Exposed concatenate method */
void AJ_CALL cat_method(alljoyn_busobject bus, const alljoyn_interfacedescription_member* member, alljoyn_message msg)
{
    QStatus status;
    alljoyn_msgarg outArg;
    char* str1;
    char* str2;
    /* Concatenate the two input strings and reply with the result. */
    char result[256] = { 0 };
    status = alljoyn_msgarg_get(alljoyn_message_getarg(msg, 0), "s", &str1);
    if (ER_OK != status) {
        printf("Ping: Error reading alljoyn_message\n");
    }
    status = alljoyn_msgarg_get(alljoyn_message_getarg(msg, 1), "s", &str2);
    if (ER_OK != status) {
        printf("Ping: Error reading alljoyn_message\n");
    }
    snprintf(result, sizeof(result), "%s%s", str1, str2);
    outArg = alljoyn_msgarg_create_and_set("s", result);
    status = alljoyn_busobject_methodreply_args(bus, msg, outArg, 1);
    if (ER_OK != status) {
        printf("Ping: Error sending reply\n");
    }
    alljoyn_msgarg_destroy(outArg);
}

/** Main entry point */
int main(int argc, char** argv, char** envArg)
{
    QStatus status = ER_OK;
    char* connectArgs = NULL;
    alljoyn_interfacedescription testIntf = NULL;
    alljoyn_busobject_callbacks busObjCbs = {
        NULL,
        NULL,
        &busobject_object_registered,
        NULL
    };
    alljoyn_busobject testObj;
    alljoyn_interfacedescription exampleIntf;
    alljoyn_interfacedescription_member cat_member;
    QCC_BOOL foundMember = QCC_FALSE;
    alljoyn_busobject_methodentry methodEntries[] = {
        { &cat_member, cat_method },
    };
    alljoyn_sessionportlistener_callbacks spl_cbs = {
        accept_session_joiner,
        NULL
    };
    alljoyn_sessionopts opts;

    printf("AllJoyn Library version: %s\n", alljoyn_getversion());
    printf("AllJoyn Library build info: %s\n", alljoyn_getbuildinfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Create message bus */
    g_msgBus = alljoyn_busattachment_create("myApp", QCC_TRUE);

    /* Add org.alljoyn.Bus.method_sample interface */
    status = alljoyn_busattachment_createinterface(g_msgBus, INTERFACE_NAME, &testIntf);
    if (status == ER_OK) {
        alljoyn_interfacedescription_addmember(testIntf, ALLJOYN_MESSAGE_METHOD_CALL, "cat", "ss",  "s", "inStr1,inStr2,outStr", 0);
        alljoyn_interfacedescription_activate(testIntf);
        printf("Interface Created.\n");
    } else {
        printf("Failed to create interface 'org.alljoyn.Bus.method_sample'\n");
    }

    /* Register a bus listener */
    if (ER_OK == status) {
        /* Create a bus listener */
        alljoyn_buslistener_callbacks callbacks = {
            NULL,
            NULL,
            NULL,
            NULL,
            &name_owner_changed,
            NULL,
            NULL,
            NULL
        };
        g_busListener = alljoyn_buslistener_create(&callbacks, NULL);
        alljoyn_busattachment_registerbuslistener(g_msgBus, g_busListener);
    }

    /* Set up bus object */
    testObj = alljoyn_busobject_create(OBJECT_PATH, QCC_FALSE, &busObjCbs, NULL);
    exampleIntf = alljoyn_busattachment_getinterface(g_msgBus, INTERFACE_NAME);
    assert(exampleIntf);
    alljoyn_busobject_addinterface(testObj, exampleIntf);

    foundMember = alljoyn_interfacedescription_getmember(exampleIntf, "cat", &cat_member);
    assert(foundMember == QCC_TRUE);
    if (!foundMember) {
        printf("Failed to get cat member of interface\n");
    }

    status = alljoyn_busobject_addmethodhandlers(testObj, methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    if (ER_OK != status) {
        printf("Failed to register method handlers for BasicSampleObject");
    }

    /* Start the msg bus */
    status = alljoyn_busattachment_start(g_msgBus);
    if (ER_OK == status) {
        printf("alljoyn_busattachment started.\n");
        /* Register  local objects and connect to the daemon */
        status = alljoyn_busattachment_registerbusobject(g_msgBus, testObj);

        /* Create the client-side endpoint */
        if (ER_OK == status) {
            status = alljoyn_busattachment_connect(g_msgBus, connectArgs);
            if (ER_OK != status) {
                printf("alljoyn_busattachment_connect(\"%s\") failed\n", (connectArgs) ? connectArgs : "NULL");
            } else {
                printf("alljoyn_busattachment connected to \"%s\"\n", alljoyn_busattachment_getconnectspec(g_msgBus));
            }
        }
    } else {
        printf("alljoyn_busattachment_start failed\n");
    }

    /*
     * Advertise this service on the bus
     * There are three steps to advertising this service on the bus
     * 1) Request a well-known name that will be used by the client to discover
     *    this service
     * 2) Create a session
     * 3) Advertise the well-known name
     */
    /* Request name */
    if (ER_OK == status) {
        uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
        QStatus status = alljoyn_busattachment_requestname(g_msgBus, OBJECT_NAME, flags);
        if (ER_OK != status) {
            printf("alljoyn_busattachment_requestname(%s) failed (status=%s)\n", OBJECT_NAME, QCC_StatusText(status));
        }
    }

    /* Create session port listener */
    s_sessionPortListener = alljoyn_sessionportlistener_create(&spl_cbs, NULL);

    /* Create session */
    opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
    if (ER_OK == status) {
        alljoyn_sessionport sp = SERVICE_PORT;
        status = alljoyn_busattachment_bindsessionport(g_msgBus, &sp, opts, s_sessionPortListener);
        if (ER_OK != status) {
            printf("alljoyn_busattachment_bindsessionport failed (%s)\n", QCC_StatusText(status));
        }
    }

    /* Advertise name */
    if (ER_OK == status) {
        status = alljoyn_busattachment_advertisename(g_msgBus, OBJECT_NAME, alljoyn_sessionopts_get_transports(opts));
        if (status != ER_OK) {
            printf("Failed to advertise name %s (%s)\n", OBJECT_NAME, QCC_StatusText(status));
        }
    }

    if (ER_OK == status) {
        while (g_interrupt == QCC_FALSE) {
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100 * 1000);
#endif
        }
    }

    /* Deallocate sessionopts */
    if (opts) {
        alljoyn_sessionopts_destroy(opts);
    }
    /* Deallocate bus */
    if (g_msgBus) {
        alljoyn_busattachment deleteMe = g_msgBus;
        g_msgBus = NULL;
        alljoyn_busattachment_destroy(deleteMe);
    }

    /* Deallocate bus listener */
    if (g_busListener) {
        alljoyn_buslistener_destroy(g_busListener);
    }

    /* Deallocate session port listener */
    if (s_sessionPortListener) {
        alljoyn_sessionportlistener_destroy(s_sessionPortListener);
    }

    /* Deallocate the bus object */
    if (testObj) {
        alljoyn_busobject_destroy(testObj);
    }

    return (int) status;
}
