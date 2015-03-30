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

#include <cstdio>
#include <iostream>
#include <vector>
#include <memory>
#include <stdlib.h>
#include <errno.h>
#include <string>

#include <alljoyn/Status.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Init.h>

#define INTF_NAME "com.example.Door"

using namespace std;
using namespace ajn;

class Door;
vector<Door*> g_doors;
vector<bool> g_doors_registered;
int g_turn = 0;
SessionPort port = 123;

class SPL : public SessionPortListener {
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {
        QCC_UNUSED(sessionPort);
        QCC_UNUSED(joiner);
        QCC_UNUSED(opts);
        return true;
    }
};

SPL g_session_port_listener;

static QStatus BuildInterface(BusAttachment& bus)
{
    QStatus status;

    InterfaceDescription* intf = NULL;
    status = bus.CreateInterface(INTF_NAME, intf);
    assert(ER_OK == status);
    status = intf->AddProperty("IsOpen", "b", PROP_ACCESS_READ);
    assert(ER_OK == status);
    status = intf->AddPropertyAnnotation("IsOpen", "org.freedesktop.DBus.Property.EmitsChangedSignal", "true");
    assert(ER_OK == status);
    status = intf->AddProperty("Location", "s", PROP_ACCESS_READ);
    assert(ER_OK == status);
    status = intf->AddPropertyAnnotation("Location", "org.freedesktop.DBus.Property.EmitsChangedSignal", "true");
    assert(ER_OK == status);
    status = intf->AddProperty("KeyCode", "u", PROP_ACCESS_READ);
    assert(ER_OK == status);
    status = intf->AddPropertyAnnotation("KeyCode", "org.freedesktop.DBus.Property.EmitsChangedSignal", "invalidates");
    assert(ER_OK == status);

    status = intf->AddMethod("Open", "", "", "");
    assert(ER_OK == status);
    status = intf->AddMethod("Close", "", "", "");
    assert(ER_OK == status);
    status = intf->AddMethod("KnockAndRun", "", "", "", MEMBER_ANNOTATE_NO_REPLY);
    assert(ER_OK == status);

    status = intf->AddSignal("PersonPassedThrough", "s", "name");
    assert(ER_OK == status);

    intf->Activate();

    return status;
}

static QStatus SetupBusAttachment(BusAttachment& bus, AboutData& aboutData)
{
    QStatus status;
    status = bus.Start();
    assert(ER_OK == status);
    status = bus.Connect();
    if (status != ER_OK) {
        return status;
    }

    status = BuildInterface(bus);
    assert(ER_OK == status);

    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    bus.BindSessionPort(port, opts, g_session_port_listener);

    /* set up totally uninteresting about data */
    //AppId is a 128bit uuid
    uint8_t appId[] = { 0x01, 0xB3, 0xBA, 0x14,
                        0x1E, 0x82, 0x11, 0xE4,
                        0x86, 0x51, 0xD1, 0x56,
                        0x1D, 0x5D, 0x46, 0xB0 };
    aboutData.SetAppId(appId, 16);
    aboutData.SetDeviceName("Foobar 2000 Door Security");
    //DeviceId is a string encoded 128bit UUID
    aboutData.SetDeviceId("93c06771-c725-48c2-b1ff-6a2a59d445b8");
    aboutData.SetAppName("Application");
    aboutData.SetManufacturer("Manufacturer");
    aboutData.SetModelNumber("123456");
    aboutData.SetDescription("A poetic description of this application");
    aboutData.SetDateOfManufacture("2014-03-24");
    aboutData.SetSoftwareVersion("0.1.2");
    aboutData.SetHardwareVersion("0.0.1");
    aboutData.SetSupportUrl("http://www.example.org");
    if (!aboutData.IsValid()) {
        cerr << "Invalid about data." << endl;
        return ER_FAIL;
    }
    return status;
}


class Door : public BusObject {
  private:
    uint32_t code;
    bool open;
    string location;

    BusAttachment& bus;

  public:
    Door(BusAttachment& bus, const string& location)
        : BusObject(("/doors/" + location).c_str()),
        code(1234),
        open(false),
        location(location),
        bus(bus)
    {
        const InterfaceDescription* intf = bus.GetInterface(INTF_NAME);
        assert(intf);
        AddInterface(*intf, ANNOUNCED);

        /** Register the method handlers with the object */
        const MethodEntry methodEntries[] = {
            { intf->GetMember("Open"), static_cast<MessageReceiver::MethodHandler>(&Door::Open) },
            { intf->GetMember("Close"), static_cast<MessageReceiver::MethodHandler>(&Door::Close) },
            { intf->GetMember("KnockAndRun"), static_cast<MessageReceiver::MethodHandler>(&Door::KnockAndRun) },
        };
        QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
        if (ER_OK != status) {
            cerr << "Failed to register method handlers for Door." << endl;
        }
    }

    ~Door()
    {
    }

    /* property getters */
    QStatus Get(const char*ifcName, const char*propName, MsgArg& val)
    {
        if (strcmp(ifcName, INTF_NAME)) {
            return ER_FAIL;
        }

        if (!strcmp(propName, "IsOpen")) {
            val.Set("b", open);
        } else if (!strcmp(propName, "Location")) {
            val.Set("s", location.c_str());
        } else if (!strcmp(propName, "KeyCode")) {
            val.Set("u", code);
        } else {
            return ER_FAIL;
        }

        return ER_OK;
    }

    void Open(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);

        cout << "Door @ " << location.c_str() << " was requested to open." << endl;
        if (open) {
            cout << "\t... but it was already open." << endl;
            /* Send an errorCode */
            MethodReply(msg, ER_FAIL);
        } else {
            cout << "\t... and it was closed, so we can comply." << endl;
            open = true;
            MsgArg propval("b", open);
            EmitPropChanged(INTF_NAME, "IsOpen", propval, SESSION_ID_ALL_HOSTED);
            MethodReply(msg, NULL, (size_t)0);
        }
        cout << "[next up is " << g_doors[g_turn]->location.c_str() << "] >";
        cout.flush();
    }

    void Close(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);

        cout << "Door @ " << location.c_str() << " was requested to close." << endl;
        if (open) {
            cout << "\t... and it was open, so we can comply." << endl;
            open = false;
            MsgArg propval("b", open);
            EmitPropChanged(INTF_NAME, "IsOpen", propval, SESSION_ID_ALL_HOSTED);
            MethodReply(msg, NULL, (size_t)0);
        } else {
            cout << "\t... but it was already closed." << endl;
            /* Send an error with a description */
            MethodReply(msg, "org.allseenalliance.sample.Door.CloseError", "Could not close the door, already closed");
        }
        cout << "[next up is " << g_doors[g_turn]->location.c_str() << "] >";
        cout.flush();
    }

    void KnockAndRun(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);
        QCC_UNUSED(msg);

        if (!open) {
            // see who's there
            cout << "Someone knocked on door @ " << location.c_str() << endl;
            cout << "\t... opening door" << endl;
            open = true;
            MsgArg propval("b", open);
            EmitPropChanged(INTF_NAME, "IsOpen", propval, SESSION_ID_ALL_HOSTED);
            cout << "\t... GRRRR nobody there!!!" << endl;
            cout << "\t... slamming door shut" << endl;
            open = false;
            MsgArg propval2("b", open);
            EmitPropChanged(INTF_NAME, "IsOpen", propval2, SESSION_ID_ALL_HOSTED);
        } else {
            // door was open while knocking
            cout << "GOTCHA!!! @ " << location.c_str() << " door" << endl;
        }
        /* this method is annotated as "no-reply", so we don't send any reply, obviously */
    }

    void FlipOpen()
    {
        const char* action = open ? "Closing" : "Opening";
        cout << action << " door @ " << location.c_str() << "." << endl;
        open = !open;
        MsgArg propval("b", open);
        EmitPropChanged(INTF_NAME, "IsOpen", propval, SESSION_ID_ALL_HOSTED);
    }

    void ChangeCode()
    {
        cout << "door @ " << location.c_str() << ": change code" << endl;
        code = rand() % 10000; //code of max 4 digits
        /* KeyCode is an invalidating property, no use passing the value */
        MsgArg dummy;
        EmitPropChanged(INTF_NAME, "KeyCode", dummy, SESSION_ID_ALL_HOSTED);
    }

    string GetLocation() const {
        return location;
    }

    // only here to be able to do extra tracing
    void PersonPassedThrough(const string& who)
    {
        cout << who.c_str() << " will pass through door @ " << location.c_str() << "." << endl;
        const InterfaceDescription* intf = bus.GetInterface(INTF_NAME);
        if (intf == NULL) {
            cerr << "Failed to obtain the " << INTF_NAME <<  "interface. Unable to invoke the 'PersonPassedThrough' signal for"
                 << who.c_str() << "." << endl;
            return;
        }
        MsgArg arg("s", who.c_str());
        Signal(NULL, SESSION_ID_ALL_HOSTED, *(intf->GetMember("PersonPassedThrough")), &arg, 1);
    }
};

static void help()
{
    cout << "q         quit" << endl;
    cout << "f         flip the open state of the door" << endl;
    cout << "p <who>   signal that <who> passed through the door" << endl;
    cout << "r         remove or reattach the door to the bus" << endl;
    cout << "n         move to the next door in the list" << endl;
    cout << "c         change the code of the door" << endl;
    cout << "h         show this help message" << endl;
}

QStatus shutdown() {

#ifdef ROUTER
    return AllJoynRouterShutdown();
#endif
    return AllJoynShutdown();
}

int CDECL_CALL main(int argc, char** argv)
{

    /* parse command line arguments */
    if (argc == 1) {
        cerr << "Usage: " << argv[0] << " location1 [location2 [... [locationN] ...]]" << endl;
        return EXIT_FAILURE;
    }

    if (AllJoynInit() != ER_OK) {
        return EXIT_FAILURE;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return EXIT_FAILURE;
    }
#endif

    BusAttachment* bus = NULL;
    bus = new BusAttachment("door_provider", true);
    assert(bus != NULL);
    AboutData aboutData("en");
    AboutObj* aboutObj = new AboutObj(*bus);
    assert(aboutObj != NULL);

    if (ER_OK != SetupBusAttachment(*bus, aboutData)) {
        return EXIT_FAILURE;
    }

    aboutObj->Announce(port, aboutData);

    for (int i = 1; i < argc; ++i) {
        Door* door = new Door(*bus, argv[i]);
        bus->RegisterBusObject(*door);
        g_doors.push_back(door);
        g_doors_registered.push_back(true);
        aboutObj->Announce(port, aboutData);
    }

    if (g_doors.empty()) {
        cerr << "No doors available" << endl;
        delete bus;
        bus = NULL;
        shutdown();
        return EXIT_FAILURE;
    }

    bool done = false;
    while (!done) {
        cout << "[next up is " << g_doors[g_turn]->GetLocation().c_str() << "] >";
        string input;
        getline(cin, input);
        if (input.length() == 0) {
            continue;
        }
        bool nextDoor = true;
        char cmd = input[0];
        switch (cmd) {
        case 'q': {
                done = true;
                break;
            }

        case 'f': {
                g_doors[g_turn]->FlipOpen();
                break;
            }

        case 'c': {
                g_doors[g_turn]->ChangeCode();
                break;
            }

        case 'p': {
                size_t whopos = input.find_first_not_of(" \t", 1);
                if (whopos == input.npos) {
                    help();
                    break;
                }
                string who = input.substr(whopos);
                g_doors[g_turn]->PersonPassedThrough(who);
                break;
            }

        case 'r': {
                Door& d = *g_doors[g_turn];
                if (g_doors_registered[g_turn]) {
                    bus->UnregisterBusObject(d);
                } else {
                    bus->RegisterBusObject(d);
                }
                g_doors_registered[g_turn] = !g_doors_registered[g_turn];
                aboutObj->Announce(port, aboutData);
                break;
            }

        case 'n': {
                break;
            }

        case 'h':
        default: {
                help();
                nextDoor = false;
                break;
            }
        }

        if (true == nextDoor) {
            g_turn = (g_turn + 1) % g_doors.size();
        }
    }

    delete aboutObj;
    aboutObj = NULL;
    delete bus;
    bus = NULL;

    shutdown();
    return EXIT_SUCCESS;
}
