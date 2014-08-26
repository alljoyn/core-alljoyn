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

class MySessionPortListener : public SessionPortListener {
    bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
    {
        if (sessionPort != ASSIGNED_SESSION_PORT) {
            printf("Rejecting join attempt on unexpected session port %d\n", sessionPort);
            return false;
        }

//        std::cout << "Accepting JoinSessionRequest from " << joiner << " (opts.proximity= " << opts.proximity
//                << ", opts.traffic=" << opts.traffic << ", opts.transports=" << opts.transports << ")." << std::endl;
        return true;
    }
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
    {
        printf("Session Joined SessionId = %u\n", id);
    }
};

/** Main entry point */
int main(int argc, char** argv)
{
    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, SigIntHandler);

    QStatus status;

    BusAttachment bus("AboutServiceTest");

    status = bus.Start();
    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("FAILED to start BusAttachment (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    status = bus.Connect();
    if (ER_OK == status) {
        printf("BusAttachment connect succeeded.\n");
    } else {
        printf("FAILED to connect to router node (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionPort sp = ASSIGNED_SESSION_PORT;
    MySessionPortListener sessionPortListener;
    status = bus.BindSessionPort(sp, opts, sessionPortListener);
    if (ER_OK == status) {
        printf("BindSessionPort succeeded.\n");
    } else {
        printf("BindSessionPort failed (%s)\n", QCC_StatusText(status));
        exit(1);
    }

    // Setup the about data
    AboutData aboutData("en");

    uint8_t appId[] = { 0, 1, 2, 3, 4, 5 };
    status = aboutData.SetAppId(appId, 6);
    status = aboutData.SetDeviceName("My Device Name");
    status = aboutData.SetDeviceId("fakeID");
    status = aboutData.SetAppName("Application");
    status = aboutData.SetManufacturer("Manufacturer");
    status = aboutData.SetModelNumber("123456");
    status = aboutData.SetDescription("A poetic description of this application");
    status = aboutData.SetDateOfManufacture("2014-03-24");
    status = aboutData.SetSoftwareVersion("0.1.2");
    status = aboutData.SetHardwareVersion("0.0.1");
    status = aboutData.SetSupportUrl("http://www.alljoyn.org");
    if (!aboutData.IsValid()) {
        printf("failed to setup about data.\n");
    }

    // Setup the AboutObjectDescription
    AboutObjectDescription aod;

    status = aod.Add("/About/DeviceIcon", qcc::String("org.alljoyn.Icon"));
    const char* interfacesTest[] = { "org.alljoyn.test", "org.alljoyn.game", "org.alljoyn.mediaplayer" };
    status = aod.Add("/org/alljoyn/test", interfacesTest, 3);

    // Announce about signal
    AboutObj aboutObj(bus);
    status = aboutObj.Announce(ASSIGNED_SESSION_PORT, aod, aboutData);
    if (ER_OK == status) {
        printf("AboutObj Announce Succeeded.\n");
    } else {
        printf("AboutObj Announce failed (%s)\n", QCC_StatusText(status));
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
}
