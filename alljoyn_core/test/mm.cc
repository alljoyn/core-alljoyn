/******************************************************************************
 * Copyright (c) 2010 - 2011, AllSeen Alliance. All rights reserved.
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
 *
 ******************************************************************************/
#include <qcc/platform.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

#include <map>
#include <vector>

#include <qcc/GUID.h>
#include <qcc/IPAddress.h>
#include <qcc/Socket.h>
#include <qcc/SocketStream.h>
#include <qcc/string.h>
#include <qcc/Util.h>

#include <alljoyn/Bus.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/Router.h>
#include <alljoyn/UnixTransport.h>
#include <alljoyn/version.h>

/*
   #include <string.h>
   #include <alljoyn/Bus.h>
   #include <alljoyn/InterfaceDescription.h>
   #include <alljoyn/MsgArg.h>
   #include <alljoyn/MsgArgList.h>
   #include <alljoyn/Router.h>
   #include <alljoyn/BusObject.h>
   #include <alljoyn/BlueZTransport.h>
   #include <alljoyn/UnixTransport.h>
 */


using namespace std;
using namespace qcc;
using namespace ajn;

// static const char *mDeviceIf="org.alljoyn.ModemManager";

static const InterfaceDescription::Member org_alljoyn_Device[] = {
    { MESSAGE_METHOD_CALL, "EnumerateDevices", NULL, "ao", "objects", 0 },
    { MESSAGE_SIGNAL, "DeviceAdded", "o", NULL, "object", 0 },
    { MESSAGE_SIGNAL, "DeviceRemoved", "o", NULL, "object", 0 }
};


static const InterfaceDescription RILalljoynDeviceIfc = {
    "org.alljoyn.Device",
    org_alljoyn_Device,
    ArraySize(org_alljoyn_Device),
    NULL,
    0
};



class DeviceServiceAllJoynObj : public BusObject {
  public:
    // Constructor
    DeviceServiceAllJoynObj(Bus& bus, const char* path);

    // Enumerate Devices
    void EnumerateDevices(const alljoyn::InterfaceDescription::Member* member,
                          alljoyn::Message& msg);

    // Add the Device
    void DeviceAdded(const alljoyn::InterfaceDescription::Member* member,
                     alljoyn::Message& msg);

    // Add the Device
    void DeviceRemoved(const alljoyn::InterfaceDescription::Member* member,
                       alljoyn::Message& msg);

    // Bus notification that object is registered
    void ObjectRegistered(void);
};


void DeviceServiceAllJoynObj::ObjectRegistered(void) {
    BusObject::ObjectRegistered();

    /* Allow method returns and errors */
    const char* rules[] = { "type='method_return'", "type='method_call'", "type='error'", "type='signal'" };
    QStatus status = AddRules(rules, ArraySize(rules));
    if (ER_OK != status) {
        printf("AddRules failed: %s\n", QCC_StatusText(status));
    }
}


static const alljoyn::BusObject::MethodHandler Device_methods[] = {
    { &RILalljoynDeviceIfc, "EnumerateDevices",
      static_cast<alljoyn::BusObject::MethodHandlerFunction>(&DeviceServiceAllJoynObj::EnumerateDevices) },
    { &RILalljoynDeviceIfc, "DeviceAdded",
      static_cast<alljoyn::BusObject::MethodHandlerFunction>(&DeviceServiceAllJoynObj::DeviceAdded) },
    { &RILalljoynDeviceIfc, "DeviceRemoved",
      static_cast<alljoyn::BusObject::MethodHandlerFunction>(&DeviceServiceAllJoynObj::DeviceRemoved) }
};

/*
   static const alljoyn::BusObject::SignalHandler Device_signals[] = {
   {
   &RILalljoynDeviceIfc,
   "DeviceAdded",
   static_cast<BusObject::SignalHandlerFunction>(&DeviceServiceAllJoynObj::DeviceAdded)
   },
   {
   &RILalljoynDeviceIfc,
   "DeviceRemoved",
   static_cast<BusObject::SignalHandlerFunction>(&DeviceServiceAllJoynObj::DeviceRemoved)
   }
   };*/

// DeviceServiceAllJoynObj constructor

DeviceServiceAllJoynObj::DeviceServiceAllJoynObj(Bus& bus, const char* path) : BusObject(bus, path)
{
    AddInterface(&RILalljoynDeviceIfc);
    for (size_t i = 0; i < ArraySize(Device_methods); ++i) {
        Add__MethodHandler(Device_methods[i]);
    }
    // for (size_t i = 0; i < ArraySize(Device_signals); ++i) {
    //	Add__SignalHandler(Device_signals[i]);
    // }
}

void DeviceServiceAllJoynObj::EnumerateDevices(const alljoyn::InterfaceDescription::Member* member,
                                               alljoyn::Message& msg)
{
    // QStatus status = ER_OK;
    printf("\nShirish Enumerated Devices function called");
    // size_t numArgs;
    // MsgArg retArgs[1];

    // size_t arraySize = 1;
    // MsgArg *arrayEntries;
    // size_t i = 0;
    MsgArg tempString;

    // ToDo Call the actual function for device remove provided by Modem Manager.
    tempString.typeId = ALLJOYN_STRING;
    tempString.v_string.str = "Enumerate Function";
    tempString.v_string.len = 19;
    /*
       for (i = 0; i < arraySize; ++i) {
       arrayEntries[i].typeId = ALLJOYN_OBJECT_PATH;
       arrayEntries[i].v_objPath = &tempString;
       }

       retArgs[0].typeId = ALLJOYN_ARRAY;
       retArgs[0].v_array.numElements = 1;
       retArgs[0].v_array.elements = arrayEntries;
     */
    MethodReply(msg, &tempString, size_t(1));
}

void DeviceServiceAllJoynObj::DeviceAdded(const alljoyn::InterfaceDescription::Member* member,
                                          alljoyn::Message& msg)
{
    printf("\nShirish Device Added function called");
    const alljoyn::MsgArg* args;
    size_t numArgs;
    QStatus status = msg->GetArgs(numArgs, args);
    if (ER_OK == status) {
        // Send the string from the map
        printf("\n value of the device received to add = %s", args[0].v_string.str);
        // ToDo Call the actual function for device add provided by Modem Manager.
        MethodReply(msg, status);
    } else {
        // Return an error
        MethodReply(msg, status);
    }
}

void DeviceServiceAllJoynObj::DeviceRemoved(const alljoyn::InterfaceDescription::Member* member,
                                            alljoyn::Message& msg)
{
    const alljoyn::MsgArg* args;
    size_t numArgs;
    QStatus status = msg->GetArgs(numArgs, args);
    if (ER_OK == status) {
        printf("\n value of the device received to remove =%s", args[0].v_string.str);
        // ToDo Call the actual function for device remove provided by Modem Manager.
        MethodReply(msg, status);
    } else {
        // Return an error
        MethodReply(msg, status);
    }


}


void dummy_function(void)
{
    printf("Dummy function\n");
}

int main(int argc, char** argv) {

    // Create message bus and sample specific bus object
    qcc::GUID128 guid;
    Bus msgBus(false);
    DeviceServiceAllJoynObj busObj(msgBus, "/obj1");

    printf("AllJoyn Library version: %s\n", alljoyn::GetVersion());
    printf("AllJoyn Library build info: %s\n", alljoyn::GetBuildInfo());

    /* Create transport for accessing the BlueZ subsystem */
    Transport* unixTransport = new UnixTransport(msgBus, &guid);
    vector<Transport*> transports;
    transports.push_back(unixTransport);

    printf("Program started...\n");

    /* Get environment variable for the system bus */
    Environ* env = Environ::GetAppEnviron();
    string connectArgs = env->Find("BUS_ADDRESS", "unix:path=/var/run/dbus/system_bus_socket");
    // string connectArgs = env->Find("BUS_ADDRESS", "unix:abstract=/tmp/dbus-Yh4GcuFPI6,guid=61767eb67b0305737872dcc74b7241ac"); //, "unix:path=/var/run/dbus/system_bus_socket");

    // Start ALLJOYN
    QStatus status = msgBus.Start(transports);
    printf("msgBus started\n");

    dummy_function();

    // Register sample object
    if (ER_OK == status) {
        status = msgBus.RegisterObject(&busObj);
        printf("Object Registered\n");
    }

    /* Connect to the bus */
    if (ER_OK == status) {
        printf("Creating endpoint\n");
        status = unixTransport->CreateEndpoint(connectArgs);
    }
    printf("Endpoint created...\n");

    // Wait for bus to exit before exiting main
    if (ER_OK == status) {
        printf("Joining...\n");
        status = msgBus.Join();
    }

    return (int) status;
}
