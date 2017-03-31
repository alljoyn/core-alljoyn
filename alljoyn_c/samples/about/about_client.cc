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

#include <alljoyn_c/AboutData.h>
#include <alljoyn_c/AboutListener.h>
#include <alljoyn_c/AboutObj.h>
#include <alljoyn_c/AboutObjectDescription.h>
#include <alljoyn_c/AboutProxy.h>
#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/Init.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Please note that we don't have error handling logic here to make the sample
 * code easier to read.  This is for demonstration purpose only; actual
 * programs should check the return values of all AllJoyn API calls.
 */

static volatile sig_atomic_t s_interrupt = QCC_FALSE;

static const char* INTERFACE_NAME = "com.example.about.feature.interface.sample";

static void CDECL_CALL sig_int_handler(int sig)
{
    QCC_UNUSED(sig);
    s_interrupt = QCC_TRUE;
}

alljoyn_busattachment g_bus;

/**
 * Print out the fields found in the AboutData. Only fields with known signatures
 * are printed out.  All others will be treated as an unknown field.
 */
static void printAboutData(alljoyn_aboutdata aboutData, const char* language, int tabNum) {
    size_t count = alljoyn_aboutdata_getfields(aboutData, NULL, 0);
    size_t i = 0;
    size_t k = 0;
    const char** fields = (const char**) malloc(sizeof(char*) * count);
    alljoyn_aboutdata_getfields(aboutData, fields, count);
    for (i = 0; i < count; ++i) {
        for (int j = 0; j < tabNum; ++j) {
            printf("\t");
        }
        printf("Key: %s", fields[i]);

        alljoyn_msgarg tmp;
        alljoyn_aboutdata_getfield(aboutData, fields[i], &tmp, language);
        printf("\t");
        char signature[256] = { 0 };
        alljoyn_msgarg_signature(tmp, signature, 16);
        if (!strncmp(signature, "s", 1)) {
            const char* tmp_s;
            alljoyn_msgarg_get(tmp, "s", &tmp_s);
            printf("%s", tmp_s);
        } else if (!strncmp(signature, "as", 2)) {
            size_t las;
            alljoyn_msgarg as_arg;
            alljoyn_msgarg_get(tmp, "as", &las, &as_arg);

            for (size_t j = 0; j < las; ++j) {
                const char* tmp_s;
                alljoyn_msgarg_get(alljoyn_msgarg_array_element(as_arg, j), "s", &tmp_s);
                printf("%s ", tmp_s);
            }
        } else if (!strncmp(signature, "ay", 2)) {
            size_t lay;
            uint8_t* pay;
            alljoyn_msgarg_get(tmp, "ay", &lay, &pay);
            for (k = 0; k < lay; ++k) {
                printf("%02x ", pay[k]);
            }
        } else {
            printf("User Defined Value\tSignature: %s", signature);
        }
        printf("\n");
    }
    free((void*)fields);
}

typedef struct my_about_listener_t {
    alljoyn_sessionlistener sessionlistener;
    alljoyn_aboutlistener aboutlistener;
} my_about_listener;

static void AJ_CALL alljoyn_sessionlistener_connect_lost_cb(const void* context,
                                                            alljoyn_sessionid sessionId,
                                                            alljoyn_sessionlostreason reason)
{
    QCC_UNUSED(context);
    printf("SessionLost sessionId = %u, Reason = %d\n", sessionId, reason);
}

static alljoyn_sessionlistener create_my_alljoyn_sessionlistener()
{
    alljoyn_sessionlistener_callbacks callbacks = {
        &alljoyn_sessionlistener_connect_lost_cb,
        NULL,
        NULL
    };
    return alljoyn_sessionlistener_create(&callbacks, NULL);
}

struct about_metadata_t {
    char* busName;
    alljoyn_aboutobjectdescription objectDescription;
};

static void AJ_CALL print_about_info_cb(alljoyn_sessionid sessionId, about_metadata_t* about_metadata)
{
    QStatus status;
    size_t path_num;
    const char** paths;
    alljoyn_aboutdata aboutData;
    alljoyn_aboutproxy aboutProxy =
        alljoyn_aboutproxy_create(g_bus, about_metadata->busName, sessionId);
    alljoyn_msgarg objArg = alljoyn_msgarg_create();
    /*
     * alljoyn_busattachment_enableconcurrentcallbacks should be avoided as
     * it may cause a deadlock.
     * It is used here to not introduce synchronization mechanism required
     * to be able to process about data without concurrent callbacks.
     */
    alljoyn_busattachment_enableconcurrentcallbacks(g_bus);
    alljoyn_aboutproxy_getobjectdescription(aboutProxy, objArg);
    printf("*********************************************************************************\n");
    printf("AboutProxy.GetObjectDescription:\n");
    alljoyn_aboutobjectdescription aod2 =
        alljoyn_aboutobjectdescription_create();
    alljoyn_aboutobjectdescription_createfrommsgarg(aod2, objArg);

    path_num = alljoyn_aboutobjectdescription_getpaths(aod2, NULL, 0);
    paths = (const char**) malloc(sizeof(const char*) * path_num);
    alljoyn_aboutobjectdescription_getpaths(aod2, paths, path_num);

    for (size_t i = 0; i < path_num; ++i) {
        printf("\t\t%s\n", paths[i]);
        size_t intf_num =
            alljoyn_aboutobjectdescription_getinterfaces(aod2,
                                                         paths[i],
                                                         NULL, 0);
        const char** intfs =
            (const char**) malloc(sizeof(const char*) * intf_num);
        alljoyn_aboutobjectdescription_getinterfaces(aod2, paths[i],
                                                     intfs, intf_num);

        for (size_t j = 0; j < intf_num; ++j) {
            printf("\t\t\t%s\n", intfs[j]);
        }
        free((void*)intfs);
        intfs = NULL;
    }
    free((void*) paths);
    paths = NULL;

    alljoyn_msgarg aArg = alljoyn_msgarg_create();
    alljoyn_aboutproxy_getaboutdata(aboutProxy, "en", aArg);
    printf("*********************************************************************************\n");
    printf("AboutProxy.GetAboutData: (Default Language)\n");
    aboutData = alljoyn_aboutdata_create("en");
    alljoyn_aboutdata_createfrommsgarg(aboutData, aArg, "en");
    printAboutData(aboutData, NULL, 1);
    size_t lang_num =
        alljoyn_aboutdata_getsupportedlanguages(aboutData, NULL, 0);
    if (lang_num > 1) {
        const char** langs =
            (const char**) malloc(sizeof(char*) * lang_num);
        alljoyn_aboutdata_getsupportedlanguages(aboutData,
                                                langs,
                                                lang_num);
        char* defaultLanguage;
        alljoyn_aboutdata_getdefaultlanguage(aboutData,
                                             &defaultLanguage);
        /*
         * Print out the AboutData for every language but the
         * default it has already been printed.
         */
        for (size_t i = 0; i < lang_num; ++i) {
            if (strcmp(defaultLanguage, langs[i]) != 0) {
                status = alljoyn_aboutproxy_getaboutdata(aboutProxy,
                                                         langs[i],
                                                         aArg);
                if (ER_OK == status) {
                    alljoyn_aboutdata_createfrommsgarg(aboutData,
                                                       aArg,
                                                       langs[i]);
                    printAboutData(aboutData, langs[i], 1);
                }
            }
        }
        free((void*)langs);
        langs = NULL;
        uint16_t ver;
        alljoyn_aboutproxy_getversion(aboutProxy, &ver);
        printf("*********************************************************************************\n");
        printf("AboutProxy.GetVersion %hd\n", ver);
        printf("*********************************************************************************\n");

        const char* path;
        alljoyn_aboutobjectdescription_getinterfacepaths(about_metadata->objectDescription,
                                                         INTERFACE_NAME,
                                                         &path, 1);

        alljoyn_proxybusobject proxyObject =
            alljoyn_proxybusobject_create(g_bus, about_metadata->busName,
                                          path, sessionId);

        status =
            alljoyn_proxybusobject_introspectremoteobject(proxyObject);

        if (status != ER_OK) {
            printf("Failed to introspect remote object.\n");
        }
        alljoyn_msgarg arg =
            alljoyn_msgarg_create_and_set("s", "ECHO Echo echo...\n");
        alljoyn_message replyMsg = alljoyn_message_create(g_bus);

        alljoyn_proxybusobject_methodcall(proxyObject,
                                          INTERFACE_NAME,
                                          "Echo", arg,
                                          1, replyMsg,
                                          25000, 0);
        if (status != ER_OK) {
            printf("Failed to call Echo method.\n");
            return;
        }

        char* echoReply;
        alljoyn_msgarg reply_msgarg =
            alljoyn_message_getarg(replyMsg, 0);
        status = alljoyn_msgarg_get(reply_msgarg, "s", &echoReply);
        if (status != ER_OK) {
            printf("Failed to read Echo method reply.\n");
        }
        printf("Echo method reply: %s\n", echoReply);
        alljoyn_message_destroy(replyMsg);
        alljoyn_msgarg_destroy(arg);
        alljoyn_proxybusobject_destroy(proxyObject);
    }

    alljoyn_aboutdata_destroy(aboutData);
    alljoyn_msgarg_destroy(aArg);
    alljoyn_aboutobjectdescription_destroy(aod2);
    alljoyn_msgarg_destroy(objArg);
    alljoyn_aboutproxy_destroy(aboutProxy);
    alljoyn_aboutobjectdescription_destroy(about_metadata->objectDescription);
    free(about_metadata->busName);
    free(about_metadata);
}

void AJ_CALL my_joinsessioncb(QStatus status, alljoyn_sessionid sessionId, const alljoyn_sessionopts opts, void* context)
{
    QCC_UNUSED(opts);
    if (ER_OK == status) {
        printf("JoinSession SUCCESS (Session id=%u).\n", sessionId);
        print_about_info_cb(sessionId, (about_metadata_t*)context);
    } else {
        printf("JoinSession failed (status=%s).\n", QCC_StatusText(status));
    }
}

static void AJ_CALL announced_cb(const void* context,
                                 const char* busName,
                                 uint16_t version,
                                 alljoyn_sessionport port,
                                 const alljoyn_msgarg objectDescriptionArg,
                                 const alljoyn_msgarg aboutDataArg)
{
    my_about_listener* mylistener = (my_about_listener*) context;
    alljoyn_aboutobjectdescription objectDescription = alljoyn_aboutobjectdescription_create();
    alljoyn_aboutobjectdescription_createfrommsgarg(objectDescription, objectDescriptionArg);
    printf("*********************************************************************************\n");
    printf("Announce signal discovered\n");
    printf("\tFrom bus %s\n", busName);
    printf("\tAbout version %hu\n", version);
    printf("\tSessionPort %hu\n", port);
    printf("\tObjectDescription:\n");

    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create();
    alljoyn_aboutobjectdescription_createfrommsgarg(aod, objectDescriptionArg);

    size_t path_num = alljoyn_aboutobjectdescription_getpaths(aod, NULL, 0);
    const char** paths = (const char**) malloc(sizeof(const char*) * path_num);
    alljoyn_aboutobjectdescription_getpaths(aod, paths, path_num);

    for (size_t i = 0; i < path_num; ++i) {
        printf("\t\t%s\n", paths[i]);
        size_t intf_num = alljoyn_aboutobjectdescription_getinterfaces(aod, paths[i], NULL, 0);
        const char** intfs = (const char**) malloc(sizeof(const char*) * intf_num);
        alljoyn_aboutobjectdescription_getinterfaces(aod, paths[i], intfs, intf_num);

        for (size_t j = 0; j < intf_num; ++j) {
            printf("\t\t\t%s\n", intfs[j]);
        }
        free((void*)intfs);
        intfs = NULL;
    }
    free((void*) paths);
    paths = NULL;

    printf("\tAboutData:\n");
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create_full(aboutDataArg, "en");
    printAboutData(aboutData, NULL, 2);
    printf("*********************************************************************************\n");
    QStatus status;
    if (g_bus != NULL) {
        alljoyn_sessionopts sessionOpts =
            alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES,
                                       QCC_FALSE, ALLJOYN_PROXIMITY_ANY,
                                       ALLJOYN_TRANSPORT_ANY);
        if (NULL != sessionOpts) {
            about_metadata_t* about_metadata = (about_metadata_t*) malloc(sizeof(about_metadata_t));
            size_t len = strlen(busName);
            about_metadata->busName = (char*) malloc(len);
            strcpy(about_metadata->busName, busName);
            about_metadata->objectDescription = objectDescription;
            printf("calling alljoyn_busattachment_joinsessionasync...\n");
            status = alljoyn_busattachment_joinsessionasync(g_bus, busName, port, mylistener->sessionlistener, sessionOpts, &my_joinsessioncb, about_metadata);

            if (ER_OK != status) {
                printf("alljoyn_busattachment_joinsessionasync failed (status=%s)\n", QCC_StatusText(status));
                free(about_metadata->busName);
                free(about_metadata);
            }
            alljoyn_sessionopts_destroy(sessionOpts);
        }
    } else {
        printf("BusAttachment is NULL\n");
    }
    alljoyn_aboutdata_destroy(aboutData);
    alljoyn_aboutobjectdescription_destroy(aod);
}

static my_about_listener* create_my_alljoyn_aboutlistener()
{
    my_about_listener* result =
        (my_about_listener*) malloc(sizeof(my_about_listener));
    alljoyn_aboutlistener_callback callbacks = {
        &announced_cb
    };
    result->aboutlistener = alljoyn_aboutlistener_create(&callbacks, result);
    result->sessionlistener = create_my_alljoyn_sessionlistener();
    return result;
}

static void destroy_my_alljoyn_aboutlistener(my_about_listener* listener)
{
    if (listener != NULL) {
        if (listener->aboutlistener != NULL) {
            alljoyn_aboutlistener_destroy(listener->aboutlistener);
            listener->aboutlistener = NULL;
        }
        if (listener->sessionlistener != NULL) {
            alljoyn_sessionlistener_destroy(listener->sessionlistener);
            listener->sessionlistener = NULL;
        }
        free(listener);
        listener = NULL;
    }
}

int CDECL_CALL main(void)
{
    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, sig_int_handler);

    QStatus status = alljoyn_init();
    if (ER_OK != status) {
        printf("alljoyn_init failed (%s)\n", QCC_StatusText(status));
        return 1;
    }
#ifdef ROUTER
    status = alljoyn_routerinit();
    if (ER_OK != status) {
        printf("alljoyn_routerinit failed (%s)\n", QCC_StatusText(status));
        alljoyn_shutdown();
        return 1;
    }
#endif

    g_bus = alljoyn_busattachment_create("AboutServiceTest", QCC_TRUE);

    status = alljoyn_busattachment_start(g_bus);
    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("FAILED to start BusAttachment (%s)\n",
               QCC_StatusText(status));
        return 1;
    }

    status = alljoyn_busattachment_connect(g_bus, NULL);
    if (ER_OK == status) {
        printf("BusAttachment connect succeeded. BusName %s\n",
               alljoyn_busattachment_getuniquename(g_bus));
    } else {
        printf("FAILED to connect to router node (%s)\n",
               QCC_StatusText(status));
        return 1;
    }

    my_about_listener* listener = create_my_alljoyn_aboutlistener();
    alljoyn_busattachment_registeraboutlistener(g_bus, listener->aboutlistener);

    const char* interfaces[] = { INTERFACE_NAME };
    alljoyn_busattachment_whoimplements_interfaces(g_bus, interfaces,
                                                   sizeof(interfaces) / sizeof(interfaces[0]));

    if (ER_OK == status) {
        printf("WhoImplements called.\n");
    } else {
        printf("WhoImplements call FAILED with status %s\n", QCC_StatusText(status));
        return 1;
    }

    /* Perform the service asynchronously until the user signals for an exit */
    if (ER_OK == status) {
        while (s_interrupt == QCC_FALSE) {
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100 * 1000);
#endif
        }
    }

    alljoyn_busattachment_unregisteraboutlistener(g_bus, listener->aboutlistener);
    destroy_my_alljoyn_aboutlistener(listener);
    alljoyn_busattachment_stop(g_bus);
    alljoyn_busattachment_join(g_bus);
    alljoyn_busattachment_destroy(g_bus);

#ifdef ROUTER
    alljoyn_routershutdown();
#endif
    alljoyn_shutdown();
    return 0;
}
