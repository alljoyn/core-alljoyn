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

/* The door_consumer sample is an illustration of the use of the alljoyn_observer.
 * To make it do something useful, you also need a companion door_provider application.
 * You can find one in the samples of alljoyn_core (the C++ language binding), or
 * in the samples of the data-driven API.
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>

#include <alljoyn_c/BusAttachment.h>
#include <alljoyn_c/Observer.h>

#define INTF_NAME "com.example.Door"
#define MAX_WAIT_MS 3000

static QStatus proxy_get_location(alljoyn_proxybusobject proxy, char** location_ret)
{
    *location_ret = NULL;
    char* location;
    alljoyn_msgarg value = alljoyn_msgarg_create();
    QStatus status = alljoyn_proxybusobject_getproperty(proxy, INTF_NAME, "Location", value);
    if (ER_OK == status) {
        status = alljoyn_msgarg_get_string(value, &location);
        if (ER_OK == status) {
            *location_ret = malloc(strlen(location) + 1);
            strcpy(*location_ret, location);
        }
    }

    alljoyn_msgarg_destroy(value);

    return status;
}

static QStatus proxy_get_isopen(alljoyn_proxybusobject proxy, QCC_BOOL* isopen_ret)
{
    *isopen_ret = QCC_FALSE;
    alljoyn_msgarg value = alljoyn_msgarg_create();
    QStatus status = alljoyn_proxybusobject_getproperty(proxy, INTF_NAME, "IsOpen", value);
    if (ER_OK == status) {
        status = alljoyn_msgarg_get_bool(value, isopen_ret);
    }
    alljoyn_msgarg_destroy(value);

    return status;
}

static QStatus proxy_get_keycode(alljoyn_proxybusobject proxy, uint32_t* keycode_ret)
{
    *keycode_ret = 0;
    alljoyn_msgarg value = alljoyn_msgarg_create();
    QStatus status = alljoyn_proxybusobject_getproperty(proxy, INTF_NAME, "KeyCode", value);
    if (ER_OK == status) {
        status = alljoyn_msgarg_get_uint32(value, keycode_ret);
    }
    alljoyn_msgarg_destroy(value);

    return status;
}

static void help()
{
    printf("q             quit\n");
    printf("l             list all discovered doors\n");
    printf("o <location>  open door at <location>\n");
    printf("c <location>  close door at <location>\n");
    printf("k <location>  knock-and-run at <location>\n");
    printf("h             display this help message\n");
}

static void list_doors(alljoyn_busattachment bus, alljoyn_observer observer)
{
    alljoyn_proxybusobject_ref proxyref;
    for (proxyref = alljoyn_observer_getfirst(observer); proxyref; proxyref = alljoyn_observer_getnext(observer, proxyref)) {
        alljoyn_proxybusobject proxy = alljoyn_proxybusobject_ref_get(proxyref);
        char* location;
        QCC_BOOL isOpen;

        QStatus status = proxy_get_isopen(proxy, &isOpen);
        if (ER_OK != status) {
            fprintf(stderr, "Could not get IsOpen property for object %s:%s.\n", alljoyn_proxybusobject_getuniquename(proxy), alljoyn_proxybusobject_getpath(proxy));
            continue;
        }
        status = proxy_get_location(proxy, &location);
        if (ER_OK != status) {
            fprintf(stderr, "Could not get Location property for object %s:%s.\n", alljoyn_proxybusobject_getuniquename(proxy), alljoyn_proxybusobject_getpath(proxy));
            continue;
        }
        printf("Door location: %s open: %s\n", location, isOpen ? "yes" : "no");
        free(location);
    }
}

/** returns a reference that must be decref'ed by the application */
static alljoyn_proxybusobject_ref get_door_at_location(alljoyn_busattachment bus, alljoyn_observer observer, const char* find_location)
{
    alljoyn_proxybusobject_ref proxyref;
    for (proxyref = alljoyn_observer_getfirst(observer); proxyref; proxyref = alljoyn_observer_getnext(observer, proxyref)) {
        alljoyn_proxybusobject proxy = alljoyn_proxybusobject_ref_get(proxyref);
        char* location;
        QStatus status = proxy_get_location(proxy, &location);
        if (ER_OK != status) {
            fprintf(stderr, "Could not get Location property for object %s:%s.\n", alljoyn_proxybusobject_getuniquename(proxy), alljoyn_proxybusobject_getpath(proxy));
            continue;
        }
        if (!strcmp(find_location, location)) {
            free(location);
            return proxyref;
        }
        free(location);
    }
    return NULL;
}

static void open_door(alljoyn_busattachment bus, alljoyn_observer observer, const char* location)
{
    alljoyn_proxybusobject_ref proxyref = get_door_at_location(bus, observer, location);

    if (proxyref) {
        alljoyn_proxybusobject proxy = alljoyn_proxybusobject_ref_get(proxyref);
        alljoyn_message reply = alljoyn_message_create(bus);
        QStatus status = alljoyn_proxybusobject_methodcall(proxy, INTF_NAME, "Open", NULL, 0, reply, MAX_WAIT_MS, 0);

        if (ER_OK == status) {
            /* No error */
            printf("Opening of door succeeded\n");
        } else if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
            /* MethodReply Error received (an error string) */
            char errmsg[200] = { 0 };
            size_t size = sizeof(errmsg);
            const char* errname = alljoyn_message_geterrorname(reply, errmsg, &size);
            printf("Opening of door @ location %s returned an error: %s (%s).\n", location, errname, errmsg);
        } else {
            /* Framework error or MethodReply error code */
            printf("Opening of door @ location %s returned an error: %s.\n", location, QCC_StatusText(status));
        }

        alljoyn_message_destroy(reply);
        alljoyn_proxybusobject_ref_decref(proxyref);
    }
}

static void close_door(alljoyn_busattachment bus, alljoyn_observer observer, const char* location)
{
    alljoyn_proxybusobject_ref proxyref = get_door_at_location(bus, observer, location);

    if (proxyref) {
        alljoyn_proxybusobject proxy = alljoyn_proxybusobject_ref_get(proxyref);
        alljoyn_message reply = alljoyn_message_create(bus);
        QStatus status = alljoyn_proxybusobject_methodcall(proxy, INTF_NAME, "Close", NULL, 0, reply, MAX_WAIT_MS, 0);

        if (ER_OK == status) {
            /* No error */
            printf("Closing of door succeeded\n");
        } else if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
            /* MethodReply Error received (an error string) */
            char errmsg[200] = { 0 };
            size_t size = sizeof(errmsg);
            const char* errname = alljoyn_message_geterrorname(reply, errmsg, &size);
            printf("Closing of door @ location %s returned an error: %s (%s).\n", location, errname, errmsg);
        } else {
            /* Framework error or MethodReply error code */
            printf("Closing of door @ location %s returned an error: %s.\n", location, QCC_StatusText(status));
        }

        alljoyn_message_destroy(reply);
        alljoyn_proxybusobject_ref_decref(proxyref);
    }
}

static void knock_and_run(alljoyn_busattachment bus, alljoyn_observer observer, const char* location)
{
    alljoyn_proxybusobject_ref proxyref = get_door_at_location(bus, observer, location);

    if (proxyref) {
        alljoyn_proxybusobject proxy = alljoyn_proxybusobject_ref_get(proxyref);
        QStatus status = alljoyn_proxybusobject_methodcall_noreply(proxy, INTF_NAME, "Close", NULL, 0, 0);

        if (ER_OK != status) {
            printf("A framework error occurred while trying to knock on door @ location %s\n", location);
        }

        alljoyn_proxybusobject_ref_decref(proxyref);
    }
}

static QCC_BOOL parse(alljoyn_busattachment bus, alljoyn_observer observer, char* input)
{
    char* cmd;
    char* arg;
    char* argend;

    /* ignore leading whitespace */
    for (cmd = input; *cmd && isspace(*cmd); ++cmd) {
    }
    if (!*cmd) {
        return QCC_TRUE;
    }

    /* look for an argument */
    for (arg = cmd + 1; *arg && isspace(*arg); ++arg) {
    }
    for (argend = arg; *argend && !isspace(*argend); ++argend) {
    }
    *argend = '\0';

    switch (*cmd) {
    case 'q':
        return QCC_FALSE;

    case 'l':
        list_doors(bus, observer);
        break;

    case 'o':
        open_door(bus, observer, arg);
        break;

    case 'c':
        close_door(bus, observer, arg);
        break;

    case 'k':
        knock_and_run(bus, observer, arg);
        break;

    case 'h':
    default:
        help();
        break;
    }

    return QCC_TRUE;
}

static QStatus BuildInterface(alljoyn_busattachment bus)
{
    QStatus status;

    alljoyn_interfacedescription intf = NULL;
    status = alljoyn_busattachment_createinterface(bus, INTF_NAME, &intf);
    assert(ER_OK == status);
    status = alljoyn_interfacedescription_addproperty(intf, "IsOpen", "b", ALLJOYN_PROP_ACCESS_READ);
    assert(ER_OK == status);
    status = alljoyn_interfacedescription_addpropertyannotation(intf, "IsOpen", "org.freedesktop.DBus.Property.EmitsChangedSignal", "true");
    assert(ER_OK == status);
    status = alljoyn_interfacedescription_addproperty(intf, "Location", "s", ALLJOYN_PROP_ACCESS_READ);
    assert(ER_OK == status);
    status = alljoyn_interfacedescription_addpropertyannotation(intf, "Location", "org.freedesktop.DBus.Property.EmitsChangedSignal", "true");
    assert(ER_OK == status);
    status = alljoyn_interfacedescription_addproperty(intf, "KeyCode", "u", ALLJOYN_PROP_ACCESS_READ);
    assert(ER_OK == status);
    status = alljoyn_interfacedescription_addpropertyannotation(intf, "KeyCode", "org.freedesktop.DBus.Property.EmitsChangedSignal", "invalidates");
    assert(ER_OK == status);

    status = alljoyn_interfacedescription_addmethod(intf, "Open", "", "", "", 0, NULL);
    assert(ER_OK == status);
    status = alljoyn_interfacedescription_addmethod(intf, "Close", "", "", "", 0, NULL);
    assert(ER_OK == status);
    status = alljoyn_interfacedescription_addmethod(intf, "KnockAndRun", "", "", "", ALLJOYN_MEMBER_ANNOTATE_NO_REPLY, NULL);
    assert(ER_OK == status);

    status = alljoyn_interfacedescription_addsignal(intf, "PersonPassedThrough", "s", "name", 0, NULL);
    assert(ER_OK == status);

    alljoyn_interfacedescription_activate(intf);

    return status;
}

static QStatus SetupBusAttachment(alljoyn_busattachment bus)
{
    QStatus status;
    status = alljoyn_busattachment_start(bus);
    assert(ER_OK == status);
    status = alljoyn_busattachment_connect(bus, NULL);
    assert(ER_OK == status);

    status = BuildInterface(bus);
    assert(ER_OK == status);

    return status;
}

struct _listener_ctx {
    alljoyn_busattachment bus;
    alljoyn_observer observer;
};

static struct _listener_ctx listener_ctx;

static const char* door_intf_props[] = {
    "IsOpen", "Location", "KeyCode"
};

static void properties_changed(alljoyn_proxybusobject proxy, const char* intf, const alljoyn_msgarg changed, const alljoyn_msgarg invalidated, void* context)
{
    struct _listener_ctx* ctx = (struct _listener_ctx*) context;
    printf("[listener] Door %s:%s has changed some properties.\n",
           alljoyn_proxybusobject_getuniquename(proxy), alljoyn_proxybusobject_getpath(proxy));


    alljoyn_busattachment_enableconcurrentcallbacks(ctx->bus);
    QStatus status;
    char* location = NULL;
    status = proxy_get_location(proxy, &location);
    if (status == ER_OK) {
        printf("\tThat's actually the door at location %s.\n", location);
        free(location);
    }

    size_t nelem;
    alljoyn_msgarg elems;
    if (ER_OK == status) {
        status = alljoyn_msgarg_get(changed, "a{sv}", &nelem, &elems);
    }
    if (ER_OK == status) {
        size_t i;
        for (i = 0; i < nelem; ++i) {
            const char* prop;
            alljoyn_msgarg val;
            status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(elems, i), "{sv}", &prop, &val);
            if (ER_OK == status) {
                if (!strcmp(prop, "Location")) {
                    char* newloc;
                    status = alljoyn_msgarg_get_string(val, &newloc);
                    if (ER_OK == status) {
                        printf("->  location: %s\n", newloc);
                    }
                } else if (!strcmp(prop, "IsOpen")) {
                    QCC_BOOL isopen = QCC_FALSE;
                    status = alljoyn_msgarg_get_bool(val, &isopen);
                    if (ER_OK == status) {
                        printf("->   is open: %s\n", isopen ? "yes" : "no");
                    }
                }
            } else {
                break;
            }
        }
    }

    if (ER_OK == status) {
        status = alljoyn_msgarg_get(invalidated, "as", &nelem, &elems);
    }
    if (ER_OK == status) {
        size_t i;
        for (i = 0; i < nelem; ++i) {
            char* prop;
            status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(elems, i), "s", &prop);
            if (status == ER_OK) {
                printf("  invalidated %s\n", prop);
            }
        }
    }

    printf("> ");
    fflush(stdout);
}

static void object_discovered(const void* context, alljoyn_proxybusobject_ref proxyref)
{
    struct _listener_ctx* ctx = (struct _listener_ctx*) context;
    alljoyn_proxybusobject proxy = alljoyn_proxybusobject_ref_get(proxyref);
    printf("[listener] Door %s:%s has just been discovered.\n",
           alljoyn_proxybusobject_getuniquename(proxy), alljoyn_proxybusobject_getpath(proxy));

    alljoyn_busattachment_enableconcurrentcallbacks(ctx->bus);
    QStatus status;

    status = alljoyn_proxybusobject_registerpropertieschangedlistener(proxy, INTF_NAME,
                                                                      door_intf_props, 3,
                                                                      properties_changed,
                                                                      &listener_ctx);
    if (status != ER_OK) {
        fprintf(stderr, "Could not register properties changed listener\n");
    }

    /* get properties */
    char* location = NULL;
    QCC_BOOL isopen;
    uint32_t keycode;
    status = proxy_get_location(proxy, &location);
    if (status == ER_OK) {
        status = proxy_get_isopen(proxy, &isopen);
    }
    if (status == ER_OK) {
        status = proxy_get_keycode(proxy, &keycode);
    }

    if (status == ER_OK) {
        printf("  location: %s\n", location);
        printf("   is open: %s\n", isopen ? "yes" : "no");
        printf("   keycode: %u\n", (unsigned)keycode);
    } else {
        fprintf(stderr, "Could not retrieve door properties.\n");
    }

    if (location) {
        free(location);
    }
    printf("> ");
    fflush(stdout);
}


static void object_lost(const void* context, alljoyn_proxybusobject_ref proxyref)
{
    alljoyn_proxybusobject proxy = alljoyn_proxybusobject_ref_get(proxyref);
    printf("[listener] Door %s:%s no longer exists.\n",
           alljoyn_proxybusobject_getuniquename(proxy), alljoyn_proxybusobject_getpath(proxy));
    printf("> ");
    fflush(stdout);
}

static alljoyn_observerlistener_callback listener_cbs = { object_discovered, object_lost };

/** the PersonPassedThrough signal handler */
static void person_passed_through(const alljoyn_interfacedescription_member* member, const char* path, alljoyn_message message)
{
    struct _listener_ctx* ctx = &listener_ctx; //alljoyn_c caveat: no way to pass a cookie into signal handlers.
    alljoyn_proxybusobject_ref proxyref = alljoyn_observer_get(ctx->observer, alljoyn_message_getsender(message), path);
    if (proxyref) {
        alljoyn_proxybusobject proxy = alljoyn_proxybusobject_ref_get(proxyref);
        QStatus status;

        alljoyn_busattachment_enableconcurrentcallbacks(ctx->bus);

        char* location = NULL;
        const char* who = NULL;
        status = proxy_get_location(proxy, &location);
        if (status == ER_OK) {
            status = alljoyn_message_parseargs(message, "s", &who);
        }
        if (status == ER_OK) {
            printf("[listener] %s passed through the door at location %s\n", who, location);
        } else {
            fprintf(stderr, "Something went wrong while parsing the received signal: %s\n", QCC_StatusText(status));
        }
        if (location) {
            free(location);
        }

        alljoyn_proxybusobject_ref_decref(proxyref);
    } else {
        fprintf(stderr, "Got PersonPassedThrough signal from an unknown door: %s:%s\n", alljoyn_message_getsender(message), path);
    }

    printf("> "); fflush(stdout);
}


int main(int argc, char** argv)
{
    alljoyn_busattachment bus = alljoyn_busattachment_create("door_consumer_c", QCC_TRUE);
    SetupBusAttachment(bus);

    const char* intfname = INTF_NAME;
    alljoyn_observer obs = alljoyn_observer_create(bus, &intfname, 1);
    listener_ctx.observer = obs;
    listener_ctx.bus = bus;
    alljoyn_observerlistener listener = alljoyn_observerlistener_create(&listener_cbs, &listener_ctx);
    alljoyn_observer_registerlistener(obs, listener, QCC_TRUE);

    alljoyn_interfacedescription intf = alljoyn_busattachment_getinterface(bus, INTF_NAME);
    alljoyn_interfacedescription_member member;
    alljoyn_interfacedescription_getmember(intf, "PersonPassedThrough", &member);
    alljoyn_busattachment_registersignalhandler(bus, person_passed_through, member, NULL);

    QCC_BOOL done = QCC_FALSE;
    while (!done) {
        char input[200];
        printf("> "); fflush(stdout);
        char* str = fgets(input, sizeof(input), stdin);
        if (str == NULL) {
            break;
        }
        str = strchr(input, '\n');
        if (str) {
            *str = '\0';
        }
        done = !parse(bus, obs, input);
    }

    // Cleanup
    alljoyn_observer_destroy(obs);
    alljoyn_observerlistener_destroy(listener);
    alljoyn_busattachment_stop(bus);
    alljoyn_busattachment_join(bus);
    alljoyn_busattachment_destroy(bus);

    return 0;
}
