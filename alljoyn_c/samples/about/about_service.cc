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

#include <alljoyn_c/AboutData.h>
#include <alljoyn_c/AboutListener.h>
#include <alljoyn_c/AboutObj.h>
#include <alljoyn_c/AboutObjectDescription.h>
#include <alljoyn_c/AboutProxy.h>
#include <alljoyn_c/BusAttachment.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/AboutObj.h>

#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

using namespace ajn;

static volatile sig_atomic_t s_interrupt = QCC_FALSE;

static void CDECL_CALL sig_int_handler(int sig)
{
    s_interrupt = QCC_TRUE;
}

static alljoyn_sessionport ASSIGNED_SESSION_PORT = 900;
static const char INTERFACE_NAME[] = "com.example.about.feature.interface.sample";

static void sessionportlistener_sessionjoined_cb(const void* context,
                                                 alljoyn_sessionport sessionPort,
                                                 alljoyn_sessionid id,
                                                 const char* joiner)
{
    printf("Session Joined SessionId = %u\n", id);
}

static QCC_BOOL sessionportlistener_acceptsessionjoiner_cb(const void* context,
                                                           alljoyn_sessionport sessionPort,
                                                           const char* joiner,
                                                           const alljoyn_sessionopts opts)
{
    if (sessionPort != ASSIGNED_SESSION_PORT) {
        printf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
        return QCC_FALSE;
    }
    return QCC_TRUE;
}


static alljoyn_sessionportlistener create_my_alljoyn_sessionportlistener()
{
    alljoyn_sessionportlistener_callbacks* callbacks =
        (alljoyn_sessionportlistener_callbacks*)
        malloc(sizeof(alljoyn_sessionportlistener_callbacks));

    callbacks->accept_session_joiner = sessionportlistener_acceptsessionjoiner_cb;
    callbacks->session_joined = sessionportlistener_sessionjoined_cb;
    return alljoyn_sessionportlistener_create(callbacks, NULL);
}

/**
 * Respond to remote method call `Echo` by returning the string back to the sender
 */
static void echo_cb(alljoyn_busobject object,
                    const alljoyn_interfacedescription_member* member,
                    alljoyn_message msg) {
    alljoyn_msgarg arg = alljoyn_message_getarg(msg, 0);
    printf("Echo method called %s\n", ((ajn::MsgArg*)arg)->v_string.str);

    QStatus status = alljoyn_busobject_methodreply_args(object, msg, arg, 1);
    if (status != ER_OK) {
        printf("Failed to created MethodReply.\n");
    }
}

static alljoyn_busobject create_my_alljoyn_busobject(alljoyn_busattachment bus,
                                                     const char* path)
{
    QStatus status = ER_FAIL;
    alljoyn_busobject result = NULL;
    result = alljoyn_busobject_create(path, QCC_FALSE, NULL, NULL);

    alljoyn_interfacedescription iface = alljoyn_busattachment_getinterface(bus, INTERFACE_NAME);
    assert(iface != NULL);

    status = alljoyn_busobject_addinterface(result, iface);
    alljoyn_busobject_setannounceflag(result, iface, ANNOUNCED);
    if (status != ER_OK) {
        printf("Failed to add %s interface to the BusObject\n", INTERFACE_NAME);
    }

    alljoyn_interfacedescription_member echomember;
    alljoyn_interfacedescription_getmember(iface, "Echo", &echomember);
    const alljoyn_busobject_methodentry methodEntries[] = {
        { &echomember, echo_cb }
    };
    status =
        alljoyn_busobject_addmethodhandlers(result, methodEntries,
                                            sizeof(methodEntries) / sizeof(methodEntries[0]));

    return result;
}

int main(int argc, char** argv)
{
    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, sig_int_handler);

    QStatus status = ER_FAIL;

    alljoyn_busattachment bus =
        alljoyn_busattachment_create("About Service Example", QCC_TRUE);

    status = alljoyn_busattachment_start(bus);
    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("FAILED to start BusAttachment (%s)\n",
               QCC_StatusText(status));
        return 1;
    }

    status = alljoyn_busattachment_connect(bus, NULL);
    if (ER_OK == status) {
        printf("BusAttachment connect succeeded. BusName %s\n",
               alljoyn_busattachment_getuniquename(bus));
    } else {
        printf("FAILED to connect to router node (%s)\n",
               QCC_StatusText(status));
        return 1;
    }

    alljoyn_sessionopts opts =
        alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_FALSE,
                                   ALLJOYN_PROXIMITY_ANY,
                                   ALLJOYN_TRANSPORT_ANY);
    alljoyn_sessionport sessionPort = ASSIGNED_SESSION_PORT;
    alljoyn_sessionportlistener listener = create_my_alljoyn_sessionportlistener();
    status = alljoyn_busattachment_bindsessionport(bus, &sessionPort,
                                                   opts, listener);
    if (ER_OK != status) {
        printf("Failed to BindSessionPort (%s)", QCC_StatusText(status));
        return 1;
    }

    /*
     * Setup the about data
     * The default language is specified in the constructor. If the default language
     * is not specified any Field that should be localized will return an error
     */
    alljoyn_aboutdata aboutData = alljoyn_aboutdata_create("en");
    /* AppId is a 128bit uuid */
    uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
                        0x1E, 0x82, 0x11, 0xE4,
                        0x86, 0x51, 0xD1, 0x56,
                        0x1D, 0x5D, 0x46, 0xB0 };
    status = alljoyn_aboutdata_setappid(aboutData, appId, 16);
    status = alljoyn_aboutdata_setdevicename(aboutData, "My Device Name", "en");
    /* DeviceId is a string encoded 128bit UUID */
    status = alljoyn_aboutdata_setdeviceid(aboutData,
                                           "93c06771-c725-48c2-b1ff-6a2a59d445b8");
    status = alljoyn_aboutdata_setappname(aboutData, "Application", "en");
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "Manufacturer", "en");
    status = alljoyn_aboutdata_setmodelnumber(aboutData, "123456");
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "A poetic description of this application",
                                              "en");
    status = alljoyn_aboutdata_setdateofmanufacture(aboutData, "2014-03-24");
    status = alljoyn_aboutdata_setsoftwareversion(aboutData, "0.1.2");
    status = alljoyn_aboutdata_sethardwareversion(aboutData, "0.0.1");
    status = alljoyn_aboutdata_setsupporturl(aboutData, "http://www.example.org");

    /*
     * The default language is automatically added to the `SupportedLanguages`
     * Users don't have to specify the AJSoftwareVersion its automatically added
     * to the AboutData/
     * Adding Spanish Localization values to the AboutData. All strings MUST be
     * UTF-8 encoded.
     */
    status = alljoyn_aboutdata_setdevicename(aboutData,
                                             "Mi dispositivo Nombre", "es");
    status = alljoyn_aboutdata_setappname(aboutData, "aplicación", "es");
    status = alljoyn_aboutdata_setmanufacturer(aboutData, "fabricante", "es");
    status = alljoyn_aboutdata_setdescription(aboutData,
                                              "Una descripción poética de esta aplicación",
                                              "es");

    /* Check to see if the aboutData is valid before sending the About Announcement */
    if (!alljoyn_aboutdata_isvalid(aboutData, "en")) {
        printf("failed to setup about data.\n");
    }

    char interface[256] = { 0 };
    snprintf(interface, 256, "<node>"                                           \
             "<interface name='%s'>"                                  \
             "  <method name='Echo'>"                                 \
             "    <arg name='out_arg' type='s' direction='in' />"     \
             "    <arg name='return_arg' type='s' direction='out' />" \
             "  </method>"                                            \
             "</interface>"                                           \
             "</node>", INTERFACE_NAME);

    printf("Interface = %s\n", interface);
    status = alljoyn_busattachment_createinterfacesfromxml(bus, interface);
    if (ER_OK != status) {
        printf("Failed to parse the xml interface definition (%s)", QCC_StatusText(status));
        return 1;
    }

    alljoyn_busobject busObject = create_my_alljoyn_busobject(bus,
                                                              "/example/path");

    status = alljoyn_busattachment_registerbusobject(bus, busObject);
    if (ER_OK != status) {
        printf("Failed to register BusObject (%s)", QCC_StatusText(status));
        return 1;
    }

    /* Announce about signal */
    alljoyn_aboutobj aboutObj = alljoyn_aboutobj_create(bus, UNANNOUNCED);
    /*
     * Note the ObjectDescription that is part of the Announce signal is found
     * automatically by introspecting the BusObjects registered with the bus
     * attachment.
     */
    status = alljoyn_aboutobj_announce(aboutObj, sessionPort, aboutData);
    if (ER_OK == status) {
        printf("AboutObj Announce Succeeded.\n");
    } else {
        printf("AboutObj Announce failed (%s)\n", QCC_StatusText(status));
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
    alljoyn_sessionportlistener_destroy(listener);

    alljoyn_busattachment_stop(bus);
    alljoyn_busattachment_join(bus);

    alljoyn_sessionopts_destroy(opts);
    alljoyn_aboutdata_destroy(aboutData);
    alljoyn_busobject_destroy(busObject);
    alljoyn_aboutobj_destroy(aboutObj);
    alljoyn_busattachment_destroy(bus);
    return 0;
}
