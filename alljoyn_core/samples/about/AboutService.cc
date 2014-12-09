/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusListener.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/AboutObj.h>

#include <signal.h>
#include <stdio.h>

using namespace ajn;

static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig) {
    s_interrupt = true;
}

static SessionPort ASSIGNED_SESSION_PORT = 900;
static const char* INTERFACE_NAME = "com.example.about.feature.interface.sample";

class MySessionPortListener : public SessionPortListener {
    bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
    {
        if (sessionPort != ASSIGNED_SESSION_PORT) {
            printf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
            return false;
        }
        return true;
    }
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
    {
        printf("Session Joined SessionId = %u\n", id);
    }
};

class MyBusObject : public BusObject {
  public:
    MyBusObject(BusAttachment& bus, const char* path)
        : BusObject(path) {
        QStatus status;
        const InterfaceDescription* iface = bus.GetInterface(INTERFACE_NAME);
        assert(iface != NULL);

        // Here the value ANNOUNCED tells AllJoyn that this interface
        // should be announced
        status = AddInterface(*iface, ANNOUNCED);
        if (status != ER_OK) {
            printf("Failed to add %s interface to the BusObject\n", INTERFACE_NAME);
        }

        /* Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { iface->GetMember("Echo"), static_cast<MessageReceiver::MethodHandler>(&MyBusObject::Echo) }
        };
        AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
    }

    // Respond to remote method call `Echo` by returning the string back to the
    // sender.
    void Echo(const InterfaceDescription::Member* member, Message& msg) {
        printf("Echo method called: %s", msg->GetArg(0)->v_string.str);
        const MsgArg* arg((msg->GetArg(0)));
        QStatus status = MethodReply(msg, arg, 1);
        if (status != ER_OK) {
            printf("Failed to created MethodReply.\n");
        }
    }
};

/** Main entry point */
int main(int argc, char** argv)
{
    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, SigIntHandler);

    QStatus status;

    BusAttachment bus("About Service Example", true);
    status = bus.Start();
    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("FAILED to start BusAttachment (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    status = bus.Connect();
    if (ER_OK == status) {
        printf("BusAttachment connect succeeded. BusName %s\n", bus.GetUniqueName().c_str());
    } else {
        printf("FAILED to connect to router node (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionPort sessionPort = ASSIGNED_SESSION_PORT;
    MySessionPortListener sessionPortListener;
    status = bus.BindSessionPort(sessionPort, opts, sessionPortListener);
    if (ER_OK != status) {
        printf("Failed to BindSessionPort (%s)", QCC_StatusText(status));
        exit(1);
    }

    // Setup the about data
    // The default language is specified in the constructor. If the default language
    // is not specified any Field that should be localized will return an error
    AboutData aboutData("en");
    //AppId is a 128bit uuid
    uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
                        0x1E, 0x82, 0x11, 0xE4,
                        0x86, 0x51, 0xD1, 0x56,
                        0x1D, 0x5D, 0x46, 0xB0 };
    status = aboutData.SetAppId(appId, 16);
    status = aboutData.SetDeviceName("My Device Name");
    //DeviceId is a string encoded 128bit UUID
    status = aboutData.SetDeviceId("93c06771-c725-48c2-b1ff-6a2a59d445b8");
    status = aboutData.SetAppName("Application");
    status = aboutData.SetManufacturer("Manufacturer");
    status = aboutData.SetModelNumber("123456");
    status = aboutData.SetDescription("A poetic description of this application");
    status = aboutData.SetDateOfManufacture("2014-03-24");
    status = aboutData.SetSoftwareVersion("0.1.2");
    status = aboutData.SetHardwareVersion("0.0.1");
    status = aboutData.SetSupportUrl("http://www.example.org");

    // The default language is automatically added to the `SupportedLanguages`
    // Users don't have to specify the AJSoftwareVersion its automatically added
    // to the AboutData

    // Adding Spanish Localization values to the AboutData. All strings MUST be
    // UTF-8 encoded.
    status = aboutData.SetDeviceName("Mi dispositivo Nombre", "es");
    status = aboutData.SetAppName("aplicación", "es");
    status = aboutData.SetManufacturer("fabricante", "es");
    status = aboutData.SetDescription("Una descripción poética de esta aplicación", "es");

    // Check to see if the aboutData is valid before sending the About Announcement
    if (!aboutData.IsValid()) {
        printf("failed to setup about data.\n");
    }

    qcc::String interface = "<node>"
                            "<interface name='" + qcc::String(INTERFACE_NAME) + "'>"
                            "  <method name='Echo'>"
                            "    <arg name='out_arg' type='s' direction='in' />"
                            "    <arg name='return_arg' type='s' direction='out' />"
                            "  </method>"
                            "</interface>"
                            "</node>";

    status = bus.CreateInterfacesFromXml(interface.c_str());
    if (ER_OK != status) {
        printf("Failed to parse the xml interface definition (%s)", QCC_StatusText(status));
        exit(1);
    }

    MyBusObject busObject(bus, "/example/path");

    status = bus.RegisterBusObject(busObject);
    if (ER_OK != status) {
        printf("Failed to register BusObject (%s)", QCC_StatusText(status));
        exit(1);
    }

    // Announce about signal
    AboutObj aboutObj(bus);
    // Note the ObjectDescription that is part of the Announce signal is found
    // automatically by introspecting the BusObjects registered with the bus
    // attachment.
    status = aboutObj.Announce(sessionPort, aboutData);
    if (ER_OK == status) {
        printf("AboutObj Announce Succeeded.\n");
    } else {
        printf("AboutObj Announce failed (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    /* Perform the service asynchronously until the user signals for an exit. */
    if (ER_OK == status) {
        while (s_interrupt == false) {
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100 * 1000);
#endif
        }
    }

    return 0;
}
