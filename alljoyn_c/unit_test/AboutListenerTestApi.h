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

#ifndef _ABOUT_LISTENER_API_H
#define _ABOUT_LISTENER_API_H

#include <alljoyn_c/AboutData.h>
#include <alljoyn_c/AboutListener.h>
#include <alljoyn_c/AboutObj.h>
#include <alljoyn_c/AboutObjectDescription.h>
#include <alljoyn_c/AboutProxy.h>
#include <alljoyn_c/BusAttachment.h>

#include <qcc/Thread.h>
#include <qcc/GUID.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct about_test_about_listener_t {
    int i;
    alljoyn_aboutlistener listener;
}about_test_about_listener;

typedef struct about_test_wildcard_about_listener_t {
    uint32_t announcelistenercount;
    alljoyn_aboutlistener listener;
}about_test_wildcard_about_listener;

typedef struct announce_app_id_with_non_128_bit_length_about_listener_t {
    alljoyn_msgarg aboutData;
    alljoyn_aboutlistener listener;
}announce_non_128_bit_app_id_about_listener;

typedef about_test_wildcard_about_listener remove_object_description_about_listener;

typedef struct filtered_about_listener_t {
    char* expectedinterfaceset[256];
    const char* announcedinterfaceset[256];
    uint32_t announcelistenercount;
    size_t interfacecnt;
    char objpath[256];
    alljoyn_aboutlistener listener;
} filtered_about_listener;

void zero_announceListenerFlags();

int setExpectInterfaces(filtered_about_listener* listener, const char* path,
                        const char** interfaces, size_t infCount);

about_test_about_listener* create_about_test_about_listener(int i);

alljoyn_busobject create_about_obj_test_bus_object(alljoyn_busattachment bus,
                                                   const char* path,
                                                   const char* interfaceName);

alljoyn_busobject create_about_obj_test_bus_object_2(alljoyn_busattachment bus,
                                                     const char* path,
                                                     const char** interfaceNames,
                                                     size_t ifaceNameSize);


about_test_wildcard_about_listener*create_about_test_wildcard_about_listener();

announce_non_128_bit_app_id_about_listener*create_announce_non_128_bit_app_id_about_listener();

remove_object_description_about_listener*create_remove_object_description_about_listener();

filtered_about_listener* create_filtered_about_listener();

void destroy_about_test_about_listener(about_test_about_listener* aboutListener);

void destroy_about_obj_test_bus_object(alljoyn_busobject obj);

void destroy_about_test_wildcard_about_listener(about_test_wildcard_about_listener* aboutListener);

void destroy_announce_non_128_bit_app_id_about_listener(announce_non_128_bit_app_id_about_listener* aboutListener);

void destroy_remove_object_description_about_listener(remove_object_description_about_listener* aboutListener);

void destroy_filtered_about_listener(filtered_about_listener* aboutListener);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ABOUT_LISTENER_API_H */
