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

#include "AboutListenerTestApi.h"

#include <gtest/gtest.h>
#include <qcc/Thread.h>
#include <qcc/GUID.h>
#include <string.h>

QCC_BOOL announceListenerFlags[] = { 0, 0, 0, 0 };

void zero_announceListenerFlags()
{
    unsigned i = 0;
    for (; i < sizeof(announceListenerFlags) / sizeof(announceListenerFlags[0]); ++i) {
        announceListenerFlags[i] = 0;
    }
}

static void foo_cb(alljoyn_busobject object,
                   const alljoyn_interfacedescription_member* member,
                   alljoyn_message msg)
{
    QCC_UNUSED(member);
    alljoyn_busobject_methodreply_args(object, msg, NULL, 0);
}

static void announced_cb(const void* context,
                         const char* busName,
                         uint16_t version,
                         alljoyn_sessionport port,
                         const alljoyn_msgarg objectDescriptionArg,
                         const alljoyn_msgarg aboutDataArg)
{
    QCC_UNUSED(busName);
    QCC_UNUSED(version);
    QCC_UNUSED(port);
    QCC_UNUSED(objectDescriptionArg);
    QCC_UNUSED(aboutDataArg);
    int* idx = (int*) context;
    announceListenerFlags[*idx] = QCC_TRUE;
}

static void about_test_wildcard_about_listener_announced_cb(const void* context,
                                                            const char* busName,
                                                            uint16_t version,
                                                            alljoyn_sessionport port,
                                                            const alljoyn_msgarg objectDescriptionArg,
                                                            const alljoyn_msgarg aboutDataArg)
{
    QCC_UNUSED(busName);
    QCC_UNUSED(version);
    QCC_UNUSED(port);
    QCC_UNUSED(objectDescriptionArg);
    QCC_UNUSED(aboutDataArg);
    about_test_wildcard_about_listener* listener =
        (about_test_wildcard_about_listener*)(context);
    listener->announcelistenercount++;
}

static void non_128_bit_app_id_about_listener_announced_cb(const void* context,
                                                           const char* busName,
                                                           uint16_t version,
                                                           alljoyn_sessionport port,
                                                           const alljoyn_msgarg objectDescriptionArg,
                                                           const alljoyn_msgarg aboutDataArg)
{
    QCC_UNUSED(busName);
    QCC_UNUSED(version);
    QCC_UNUSED(port);
    QCC_UNUSED(objectDescriptionArg);
    announce_non_128_bit_app_id_about_listener* listener =
        (announce_non_128_bit_app_id_about_listener*)(context);
    announceListenerFlags[0] = QCC_TRUE;
    listener->aboutData = alljoyn_msgarg_copy(aboutDataArg);
    alljoyn_msgarg_stabilize(listener->aboutData);
}

static void remove_object_description_about_listener_cb(const void* context,
                                                        const char* busName,
                                                        uint16_t version,
                                                        alljoyn_sessionport port,
                                                        const alljoyn_msgarg objectDescriptionArg,
                                                        const alljoyn_msgarg aboutDataArg)
{
    QCC_UNUSED(busName);
    QCC_UNUSED(version);
    QCC_UNUSED(port);
    QCC_UNUSED(objectDescriptionArg);
    QCC_UNUSED(aboutDataArg);
    QStatus status = ER_FAIL;
    alljoyn_aboutobjectdescription objectDescription = alljoyn_aboutobjectdescription_create();
    status = alljoyn_aboutobjectdescription_createfrommsgarg(objectDescription, objectDescriptionArg);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    remove_object_description_about_listener* listener =
        (remove_object_description_about_listener*)(context);
    EXPECT_TRUE(alljoyn_aboutobjectdescription_haspath(objectDescription, "/org/test/about/a"));
    if (listener->announcelistenercount == 0) {
        EXPECT_TRUE(alljoyn_aboutobjectdescription_haspath(objectDescription, "/org/test/about/b"));
    } else {
        EXPECT_FALSE(alljoyn_aboutobjectdescription_haspath(objectDescription, "/org/test/about/b"));
    }
    listener->announcelistenercount++;
    alljoyn_aboutobjectdescription_destroy(objectDescription);
}

static void filtered_about_listener_cb(const void* context,
                                       const char* busName,
                                       uint16_t version,
                                       alljoyn_sessionport port,
                                       const alljoyn_msgarg objectDescriptionArg,
                                       const alljoyn_msgarg aboutDataArg)
{
    QCC_UNUSED(busName);
    QCC_UNUSED(version);
    QCC_UNUSED(port);
    QCC_UNUSED(objectDescriptionArg);
    QCC_UNUSED(aboutDataArg);
    QStatus status = ER_OK;
    alljoyn_aboutobjectdescription aod = alljoyn_aboutobjectdescription_create();
    status = alljoyn_aboutobjectdescription_createfrommsgarg(aod, objectDescriptionArg);
    EXPECT_EQ(ER_OK, status);

    filtered_about_listener* listener = (filtered_about_listener*)(context);
    size_t i = 0;
    size_t j = 0;
    size_t intersections = 0;
    if (listener->objpath) {
        if (alljoyn_aboutobjectdescription_haspath(aod, listener->objpath)) {
            size_t numInterfaces =
                alljoyn_aboutobjectdescription_getinterfaces(aod, listener->objpath,
                                                             NULL, 0);
            EXPECT_EQ(listener->interfacecnt, numInterfaces);
            alljoyn_aboutobjectdescription_getinterfaces(aod, listener->objpath,
                                                         listener->announcedinterfaceset,
                                                         numInterfaces);

            for (i = 0; i < listener->interfacecnt; ++i) {
                const char* iface = listener->announcedinterfaceset[i];
                for (j = 0; j < listener->interfacecnt; ++j) {
                    if (!strcmp(iface, listener->expectedinterfaceset[j])) {
                        intersections++;
                    }
                }
            }

            if (listener->interfacecnt == intersections) {
                listener->announcelistenercount++;
            }
        }
    }
}

int setExpectInterfaces(filtered_about_listener* listener, const char* path,
                        const char** interfaces, size_t infCount)
{
    size_t i = 0;
    strcpy(listener->objpath, path);
    for (; i < infCount; ++i) {
        listener->expectedinterfaceset[i] =
            (char*) malloc(sizeof(char) * strlen(interfaces[i]));
        strcpy(listener->expectedinterfaceset[i], interfaces[i]);
    }
    listener->interfacecnt = infCount;
    return 0;
}

about_test_about_listener* create_about_test_about_listener(int i)
{
    about_test_about_listener* result =
        (about_test_about_listener*) malloc(sizeof(about_test_about_listener));
    result->i = i;
    alljoyn_aboutlistener_callback* callback =
        (alljoyn_aboutlistener_callback*) malloc(sizeof(alljoyn_aboutlistener_callback));
    callback->about_listener_announced = announced_cb;
    result->listener = alljoyn_aboutlistener_create(callback, &(result->i));

    return result;
}

alljoyn_busobject create_about_obj_test_bus_object(alljoyn_busattachment bus,
                                                   const char* path,
                                                   const char* interfaceName)
{
    QStatus status = ER_FAIL;
    alljoyn_busobject result = NULL;
    result = alljoyn_busobject_create(path, QCC_FALSE, NULL, NULL);

    alljoyn_interfacedescription iface = alljoyn_busattachment_getinterface(bus, interfaceName);
    EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for " << interfaceName;

    if (iface == NULL) {
        printf("The interfaceDescription pointer for %s was NULL    \
                when it should not have been.", interfaceName);
        return NULL;
    }
    alljoyn_busobject_addinterface(result, iface);
    alljoyn_busobject_setannounceflag(result, iface, ANNOUNCED);

    alljoyn_interfacedescription_member foomember;
    alljoyn_interfacedescription_getmember(iface, "Foo", &foomember);

    const alljoyn_busobject_methodentry methodEntries[] = { { &foomember, foo_cb } };
    status =
        alljoyn_busobject_addmethodhandlers(result, methodEntries,
                                            sizeof(methodEntries) / sizeof(methodEntries[0]));
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    return result;
}

alljoyn_busobject create_about_obj_test_bus_object_2(alljoyn_busattachment bus,
                                                     const char* path,
                                                     const char** interfaceNames,
                                                     size_t ifaceNameSize)
{

    QStatus status = ER_FAIL;
    alljoyn_busobject result = NULL;
    result = alljoyn_busobject_create(path, QCC_FALSE, NULL, NULL);
    for (size_t i = 0; i < ifaceNameSize; ++i) {
        alljoyn_interfacedescription iface = alljoyn_busattachment_getinterface(bus, interfaceNames[i]);
        EXPECT_TRUE(iface != NULL) << "NULL InterfaceDescription* for " << interfaceNames[i];

        if (iface == NULL) {
            printf("The interfaceDescription pointer for %s was NULL    \
                    when it should not have been.", interfaceNames[i]);
            return NULL;
        }
        alljoyn_busobject_addinterface(result, iface);
        alljoyn_busobject_setannounceflag(result, iface, ANNOUNCED);

        alljoyn_interfacedescription_member foomember;
        alljoyn_interfacedescription_getmember(iface, "Foo", &foomember);

        const alljoyn_busobject_methodentry methodEntries[] = { { &foomember, foo_cb } };
        status = alljoyn_busobject_addmethodhandlers(result, methodEntries,
                                                     sizeof(methodEntries) / sizeof(methodEntries[0]));
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    }
    return result;
}


about_test_wildcard_about_listener* create_about_test_wildcard_about_listener()
{
    about_test_wildcard_about_listener* result =
        (about_test_wildcard_about_listener*) malloc(sizeof(about_test_wildcard_about_listener));
    result->announcelistenercount = 0;
    alljoyn_aboutlistener_callback* callback =
        (alljoyn_aboutlistener_callback*) malloc(sizeof(alljoyn_aboutlistener_callback));
    callback->about_listener_announced = about_test_wildcard_about_listener_announced_cb;
    result->listener = alljoyn_aboutlistener_create(callback, result);

    return result;
}

announce_non_128_bit_app_id_about_listener* create_announce_non_128_bit_app_id_about_listener()
{
    announce_non_128_bit_app_id_about_listener* result =
        (announce_non_128_bit_app_id_about_listener*)
        malloc(sizeof(announce_non_128_bit_app_id_about_listener));

    alljoyn_aboutlistener_callback* callback =
        (alljoyn_aboutlistener_callback*) malloc(sizeof(alljoyn_aboutlistener_callback));
    callback->about_listener_announced = non_128_bit_app_id_about_listener_announced_cb;
    result->listener = alljoyn_aboutlistener_create(callback, result);
    result->aboutData = NULL;
    return result;
}

remove_object_description_about_listener* create_remove_object_description_about_listener()
{
    remove_object_description_about_listener* result =
        (remove_object_description_about_listener*)
        malloc(sizeof(remove_object_description_about_listener));
    result->announcelistenercount = 0;
    alljoyn_aboutlistener_callback* callback =
        (alljoyn_aboutlistener_callback*)
        malloc(sizeof(alljoyn_aboutlistener_callback));
    callback->about_listener_announced = remove_object_description_about_listener_cb;
    result->listener = alljoyn_aboutlistener_create(callback, result);

    return result;
}

filtered_about_listener* create_filtered_about_listener()
{
    filtered_about_listener* result =
        (filtered_about_listener*) malloc(sizeof(filtered_about_listener));
    result->announcelistenercount = 0;
    result->interfacecnt = 0;
    alljoyn_aboutlistener_callback* callback =
        (alljoyn_aboutlistener_callback*) malloc(sizeof(alljoyn_aboutlistener_callback));
    callback->about_listener_announced = filtered_about_listener_cb;
    result->listener = alljoyn_aboutlistener_create(callback, result);
    return result;
}

void destroy_about_test_about_listener(about_test_about_listener* aboutListener)
{
    if (aboutListener) {
        if (aboutListener->listener) {
            alljoyn_aboutlistener_destroy(aboutListener->listener);
            aboutListener->listener = NULL;
        }
        free(aboutListener);
        aboutListener = NULL;
    }
}

void destroy_about_obj_test_bus_object(alljoyn_busobject obj)
{
    if (obj) {
        alljoyn_busobject_destroy(obj);
        obj = NULL;
    }
}

void destroy_about_test_wildcard_about_listener(about_test_wildcard_about_listener* aboutListener)
{
    if (aboutListener) {
        if (aboutListener->listener) {
            alljoyn_aboutlistener_destroy(aboutListener->listener);
            aboutListener->listener = NULL;
        }
        free(aboutListener);
        aboutListener = NULL;
    }
}

void destroy_announce_non_128_bit_app_id_about_listener(announce_non_128_bit_app_id_about_listener* aboutListener)
{
    if (aboutListener) {
        if (aboutListener->listener) {
            alljoyn_aboutlistener_destroy(aboutListener->listener);
            aboutListener->listener = NULL;
        }
        if (aboutListener->aboutData) {
            alljoyn_msgarg_destroy(aboutListener->aboutData);
            aboutListener->aboutData = NULL;
        }
        free(aboutListener);
        aboutListener = NULL;
    }
}

void destroy_remove_object_description_about_listener(remove_object_description_about_listener* aboutListener) {
    destroy_about_test_wildcard_about_listener(aboutListener);
}

void destroy_filtered_about_listener(filtered_about_listener* aboutListener)
{
    if (!aboutListener) {
        return;
    }
    if (aboutListener->listener) {
        alljoyn_aboutlistener_destroy(aboutListener->listener);
        aboutListener->listener = NULL;
    }
}
