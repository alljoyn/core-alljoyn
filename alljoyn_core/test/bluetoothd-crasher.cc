/**
 * @file
 * Sample implementation of an AllJoyn client.
 */

/******************************************************************************
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
#include <qcc/platform.h>

#include <assert.h>
#include <list>
#include <set>
#include <stdio.h>

#include <pthread.h>

#include <qcc/Environ.h>
#include <qcc/Event.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <qcc/Util.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>

#include "../daemon/bluetooth/BDAddress.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

#define METHODCALL_TIMEOUT 30000

using namespace std;
using namespace qcc;
using namespace ajn;

namespace test {
static struct {
    struct {
        struct {
            const InterfaceDescription* interface;
            // Methods (not all; only those needed)
            const InterfaceDescription::Member* DefaultAdapter;
            const InterfaceDescription::Member* ListAdapters;
            // Signals
            const InterfaceDescription::Member* AdapterAdded;
            const InterfaceDescription::Member* AdapterRemoved;
            const InterfaceDescription::Member* DefaultAdapterChanged;
        } Manager;

        struct {
            const InterfaceDescription* interface;
            // Methods (not all; only those needed)
            const InterfaceDescription::Member* AddRecord;
            const InterfaceDescription::Member* RemoveRecord;
        } Service;

        struct {
            const InterfaceDescription* interface;
            // Methods (not all; only those needed)
            const InterfaceDescription::Member* CreateDevice;
            const InterfaceDescription::Member* FindDevice;
            const InterfaceDescription::Member* GetProperties;
            const InterfaceDescription::Member* ListDevices;
            const InterfaceDescription::Member* RemoveDevice;
            const InterfaceDescription::Member* SetProperty;
            const InterfaceDescription::Member* StartDiscovery;
            const InterfaceDescription::Member* StopDiscovery;
            // Signals
            const InterfaceDescription::Member* DeviceCreated;
            const InterfaceDescription::Member* DeviceDisappeared;
            const InterfaceDescription::Member* DeviceFound;
            const InterfaceDescription::Member* DeviceRemoved;
            const InterfaceDescription::Member* PropertyChanged;
        } Adapter;

        struct {
            const InterfaceDescription* interface;
            // Methods (not all; only those needed)
            const InterfaceDescription::Member* DiscoverServices;
            const InterfaceDescription::Member* GetProperties;
            // Signals
            const InterfaceDescription::Member* DisconnectRequested;
            const InterfaceDescription::Member* PropertyChanged;
        } Device;
    } bluez;
} org;
}

struct InterfaceDesc {
    AllJoynMessageType type;
    const char* name;
    const char* inputSig;
    const char* outSig;
    const char* argNames;
    uint8_t annotation;
};

struct InterfaceTable {
    const char* ifcName;
    const InterfaceDesc* desc;
    size_t tableSize;
};


const char* bzBusName = "org.bluez";
const char* bzMgrObjPath = "/";
const char* bzManagerIfc = "org.bluez.Manager";
const char* bzServiceIfc = "org.bluez.Service";
const char* bzAdapterIfc = "org.bluez.Adapter";
const char* bzDeviceIfc = "org.bluez.Device";

const InterfaceDesc bzManagerIfcTbl[] = {
    { MESSAGE_METHOD_CALL, "DefaultAdapter",        NULL, "o",     NULL, 0 },
    { MESSAGE_METHOD_CALL, "FindAdapter",           "s",  "o",     NULL, 0 },
    { MESSAGE_METHOD_CALL, "GetProperties",         NULL, "a{sv}", NULL, 0 },
    { MESSAGE_METHOD_CALL, "ListAdapters",          NULL, "ao",    NULL, 0 },
    { MESSAGE_SIGNAL,      "AdapterAdded",          "o",  NULL,    NULL, 0 },
    { MESSAGE_SIGNAL,      "AdapterRemoved",        "o",  NULL,    NULL, 0 },
    { MESSAGE_SIGNAL,      "DefaultAdapterChanged", "o",  NULL,    NULL, 0 },
    { MESSAGE_SIGNAL,      "PropertyChanged",       "sv", NULL,    NULL, 0 }
};

const InterfaceDesc bzAdapterIfcTbl[] = {
    { MESSAGE_METHOD_CALL, "CancelDeviceCreation", "s",      NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "CreateDevice",         "s",      "o",     NULL, 0 },
    { MESSAGE_METHOD_CALL, "CreatePairedDevice",   "sos",    "o",     NULL, 0 },
    { MESSAGE_METHOD_CALL, "FindDevice",           "s",      "o",     NULL, 0 },
    { MESSAGE_METHOD_CALL, "GetProperties",        NULL,     "a{sv}", NULL, 0 },
    { MESSAGE_METHOD_CALL, "ListDevices",          NULL,     "ao",    NULL, 0 },
    { MESSAGE_METHOD_CALL, "RegisterAgent",        "os",     NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "ReleaseSession",       NULL,     NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "RemoveDevice",         "o",      NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "RequestSession",       NULL,     NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "SetProperty",          "sv",     NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "StartDiscovery",       NULL,     NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "StopDiscovery",        NULL,     NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "UnregisterAgent",      "o",      NULL,    NULL, 0 },
    { MESSAGE_SIGNAL,      "DeviceCreated",        "o",      NULL,    NULL, 0 },
    { MESSAGE_SIGNAL,      "DeviceDisappeared",    "s",      NULL,    NULL, 0 },
    { MESSAGE_SIGNAL,      "DeviceFound",          "sa{sv}", NULL,    NULL, 0 },
    { MESSAGE_SIGNAL,      "DeviceRemoved",        "o",      NULL,    NULL, 0 },
    { MESSAGE_SIGNAL,      "PropertyChanged",      "sv",     NULL,    NULL, 0 }
};

const InterfaceDesc bzServiceIfcTbl[] = {
    { MESSAGE_METHOD_CALL, "AddRecord",            "s",  "u",  NULL, 0 },
    { MESSAGE_METHOD_CALL, "CancelAuthorization",  NULL, NULL, NULL, 0 },
    { MESSAGE_METHOD_CALL, "RemoveRecord",         "u",  NULL, NULL, 0 },
    { MESSAGE_METHOD_CALL, "RequestAuthorization", "su", NULL, NULL, 0 },
    { MESSAGE_METHOD_CALL, "UpdateRecord",         "us", NULL, NULL, 0 }
};

const InterfaceDesc bzDeviceIfcTbl[] = {
    { MESSAGE_METHOD_CALL, "CancelDiscovery",     NULL, NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "Disconnect",          NULL, NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "DiscoverServices",    "s",  "a{us}", NULL, 0 },
    { MESSAGE_METHOD_CALL, "GetProperties",       NULL, "a{sv}", NULL, 0 },
    { MESSAGE_METHOD_CALL, "SetProperty",         "sv", NULL,    NULL, 0 },
    { MESSAGE_SIGNAL,      "DisconnectRequested", NULL, NULL,    NULL, 0 },
    { MESSAGE_SIGNAL,      "PropertyChanged",     "sv", NULL,    NULL, 0 }
};

const InterfaceTable ifcTables[] = {
    { "org.bluez.Manager", bzManagerIfcTbl, ArraySize(bzManagerIfcTbl) },
    { "org.bluez.Adapter", bzAdapterIfcTbl, ArraySize(bzAdapterIfcTbl) },
    { "org.bluez.Service", bzServiceIfcTbl, ArraySize(bzServiceIfcTbl) },
    { "org.bluez.Device",  bzDeviceIfcTbl,  ArraySize(bzDeviceIfcTbl)  }
};

const size_t ifcTableSize = ArraySize(ifcTables);


static int TSPrintf(const char* fmt, ...)
{
    uint32_t timestamp = GetTimestamp();
    int ret = 0;
    va_list ap;
    printf("%4d.%03d | ", timestamp / 1000, timestamp % 1000);

    va_start(ap, fmt);
    ret = vprintf(fmt, ap);
    va_end(ap);
    return ret;
}


class MyBusListener : public BusListener, public SessionListener {
  public:

    MyBusListener() { }

    void NameOwnerChanged(const char* name, const char* previousOwner, const char* newOwner)
    {
        if (previousOwner && !newOwner && (strcmp(name, bzBusName) == 0)) {
            TSPrintf("org.bluez has crashed.  Stopping...\n");
            exit(0);
        }
    }

  private:
};



class Crasher : public Thread, public MessageReceiver {
  public:
    Crasher(BusAttachment& bus, ProxyBusObject& bzAdapterObj, bool wait, uint64_t stopTime) :
        bus(bus),
        bzAdapterObj(bzAdapterObj),
        wait(wait),
        stopTime(stopTime),
        discovering(false)
    {
        QStatus status = bus.RegisterSignalHandler(this,
                                                   static_cast<MessageReceiver::SignalHandler>(&Crasher::DeviceFoundSignalHandler),
                                                   test::org.bluez.Adapter.DeviceFound, NULL);
        if (status != ER_OK) {
            TSPrintf("Failed to register signal handler: %s\n", QCC_StatusText(status));
            exit(1);
        }

        status = bus.RegisterSignalHandler(this,
                                           static_cast<MessageReceiver::SignalHandler>(&Crasher::PropertyChangedSignalHandler),
                                           test::org.bluez.Adapter.PropertyChanged, NULL);
        if (status != ER_OK) {
            TSPrintf("Failed to register signal handler: %s\n", QCC_StatusText(status));
            exit(1);
        }
        pthread_cond_init(&notDiscovering, NULL);
        pthread_mutex_init(&discMutex, NULL);
    }

    ~Crasher()
    {
        pthread_mutex_destroy(&discMutex);
        pthread_cond_destroy(&notDiscovering);
    }


    void DeviceFoundSignalHandler(const InterfaceDescription::Member* member,
                                  const char* sourcePath,
                                  Message& msg);
    void PropertyChangedSignalHandler(const InterfaceDescription::Member* member,
                                      const char* sourcePath,
                                      Message& msg);

    void* Run(void* arg);

  private:
    BusAttachment& bus;
    ProxyBusObject& bzAdapterObj;
    set<BDAddress> foundSet;
    list<BDAddress> checkList;
    Event newAddr;
    bool wait;
    uint64_t stopTime;

    bool discovering;
    pthread_cond_t notDiscovering;
    pthread_mutex_t discMutex;
};


void Crasher::DeviceFoundSignalHandler(const InterfaceDescription::Member* member,
                                       const char* sourcePath,
                                       Message& msg)
{
    BDAddress addr(msg->GetArg(0)->v_string.str);
    if (foundSet.find(addr) == foundSet.end()) {
        pthread_mutex_lock(&discMutex);
        foundSet.insert(addr);
        checkList.push_back(addr);
        pthread_mutex_unlock(&discMutex);
        TSPrintf("Found: %s\n", addr.ToString().c_str());
        if (foundSet.size() == 1) {
            newAddr.SetEvent();
        }
    }
}


void Crasher::PropertyChangedSignalHandler(const InterfaceDescription::Member* member,
                                           const char* sourcePath,
                                           Message& msg)
{
    const char* property;
    const MsgArg* value;

    msg->GetArgs("sv", &property, &value);

    if (strcmp(property, "Discovering") == 0) {
        value->Get("b", &discovering);
        TSPrintf("Discovering %s.\n", discovering ? "on" : "off");

        if (wait && !discovering) {
            pthread_cond_signal(&notDiscovering);
        }
    }
}


void* Crasher::Run(void* arg)
{
    QStatus status = ER_OK;

    status = Event::Wait(newAddr);
    if (status != ER_OK) {
        TSPrintf("Wait failed: %s\n", QCC_StatusText(status));
        bzAdapterObj.MethodCall(*test::org.bluez.Adapter.StopDiscovery, NULL, 0);
        return (void*) 1;
    }

    list<BDAddress>::iterator check;
    String objPath;
    ProxyBusObject deviceObject;
    MsgArg allSrv("s", "");

    while (!IsStopping() && (GetTimestamp64() < stopTime)) {
        pthread_mutex_lock(&discMutex);
        if (wait && discovering) {
            TSPrintf("waiting for discovery to stop...\n");
            pthread_cond_wait(&notDiscovering, &discMutex);
        }
        for (check = checkList.begin(); (!wait || !discovering) && check != checkList.end(); ++check) {
            pthread_mutex_unlock(&discMutex);
            TSPrintf("Checking: %s\n", check->ToString().c_str());
            MsgArg arg("s", check->ToString().c_str());
            Message reply(bus);
            status = bzAdapterObj.MethodCall(*test::org.bluez.Adapter.FindDevice, &arg, 1, reply);
            if (status != ER_OK) {
                status = bzAdapterObj.MethodCall(*test::org.bluez.Adapter.CreateDevice, &arg, 1, reply);
            }
            if (status != ER_OK) {
                if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
                    String errMsg;
                    const char* errName = reply->GetErrorName(&errMsg);
                    TSPrintf("Failed find/create %s: %s - %s\n", check->ToString().c_str(), errName, errMsg.c_str());
                    if (strcmp(errName, "org.freedesktop.DBus.Error.NameHasNoOwner") == 0) {
                        TSPrintf("bluetoothd crashed\n");
                        exit(0);
                    }
                } else {
                    TSPrintf("Failed find/create %s: %s\n", check->ToString().c_str(), QCC_StatusText(status));
                }
                pthread_mutex_lock(&discMutex);
                continue;
            }
            objPath = reply->GetArg(0)->v_objPath.str;
            deviceObject = ProxyBusObject(bus, bzBusName, objPath.c_str(), 0);
            deviceObject.AddInterface(*test::org.bluez.Device.interface);

            status = deviceObject.MethodCall(*test::org.bluez.Device.DiscoverServices, &allSrv, 1, reply);
            if (status != ER_OK) {
                if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
                    String errMsg;
                    const char* errName = reply->GetErrorName(&errMsg);
                    TSPrintf("Failed to get service info for %s: %s - %s\n", check->ToString().c_str(), errName, errMsg.c_str());
                } else {
                    TSPrintf("Failed to get service info for %s: %s\n", check->ToString().c_str(), QCC_StatusText(status));
                }
            } else {
                TSPrintf("Completed getting SDP info for %s.\n", check->ToString().c_str());
            }

            arg.Set("o", objPath.c_str());
            bzAdapterObj.MethodCall(*test::org.bluez.Adapter.RemoveDevice, &arg, 1, reply);

            pthread_mutex_lock(&discMutex);
        }
        pthread_mutex_unlock(&discMutex);
        qcc::Sleep(500 + Rand32() % 500);
    }

    bzAdapterObj.MethodCall(*test::org.bluez.Adapter.StopDiscovery, NULL, 0);
    return (void*) 0;
}


static void usage(void)
{
    printf("bluetoothd-crasher [-w] [-t #]\n"
           "  -w    Wait for discovery to stop to do SDP query.\n"
           "  -t #  Only run for the specified number of minutes.\n");
}


int main(int argc, char** argv)
{
    QStatus status;
    Environ* env = Environ::GetAppEnviron();
    bool wait = false;
    uint32_t runTime = -1;

#ifdef QCC_OS_ANDROID
    qcc::String connectArgs(env->Find("DBUS_SYSTEM_BUS_ADDRESS",
                                      "unix:path=/dev/socket/dbus"));
#else
    qcc::String connectArgs(env->Find("DBUS_SYSTEM_BUS_ADDRESS",
                                      "unix:path=/var/run/dbus/system_bus_socket"));
#endif

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-w") == 0) {
            wait = true;
        } else if (strcmp(argv[i], "-t") == 0) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                runTime = strtoul(argv[i], NULL, 10);
            }
        }
    }


    /* Create message bus */
    BusAttachment bus("bluetoothd-crasher");

    status = bus.Start();
    if (status != ER_OK) {
        printf("Failed to start bus: %s\n", QCC_StatusText(status));
        return 1;
    }

    status = bus.Connect(connectArgs.c_str());
    if (status != ER_OK) {
        printf("Failed to connect bus: %s\n", QCC_StatusText(status));
        return 1;
    }

    status = bus.AddMatch("type='signal',sender='org.bluez',interface='org.bluez.Adapter'");
    if (status != ER_OK) {
        printf("Failed to add match rule: %s\n", QCC_StatusText(status));
        return 1;
    }

    ProxyBusObject bzManagerObj(bus, bzBusName, bzMgrObjPath, 0);
    ProxyBusObject bzAdapterObj;

    Message reply(bus);
    vector<MsgArg> args;

    size_t tableIndex, member;
    uint64_t stopTime = GetTimestamp64() + (1000ULL * 60ULL * static_cast<uint64_t>(runTime));

    for (tableIndex = 0; tableIndex < ifcTableSize; ++tableIndex) {
        InterfaceDescription* ifc;
        const InterfaceTable& table(ifcTables[tableIndex]);
        bus.CreateInterface(table.ifcName, ifc);

        if (ifc) {
            for (member = 0; member < table.tableSize; ++member) {
                ifc->AddMember(table.desc[member].type,
                               table.desc[member].name,
                               table.desc[member].inputSig,
                               table.desc[member].outSig,
                               table.desc[member].argNames,
                               table.desc[member].annotation);
            }
            ifc->Activate();

            if (table.desc == bzManagerIfcTbl) {
                test::org.bluez.Manager.interface =             ifc;
                test::org.bluez.Manager.DefaultAdapter =        ifc->GetMember("DefaultAdapter");
                test::org.bluez.Manager.ListAdapters =          ifc->GetMember("ListAdapters");
                test::org.bluez.Manager.AdapterAdded =          ifc->GetMember("AdapterAdded");
                test::org.bluez.Manager.AdapterRemoved =        ifc->GetMember("AdapterRemoved");
                test::org.bluez.Manager.DefaultAdapterChanged = ifc->GetMember("DefaultAdapterChanged");

            } else if (table.desc == bzAdapterIfcTbl) {
                test::org.bluez.Adapter.interface =          ifc;
                test::org.bluez.Adapter.CreateDevice =       ifc->GetMember("CreateDevice");
                test::org.bluez.Adapter.FindDevice =         ifc->GetMember("FindDevice");
                test::org.bluez.Adapter.GetProperties =      ifc->GetMember("GetProperties");
                test::org.bluez.Adapter.ListDevices =        ifc->GetMember("ListDevices");
                test::org.bluez.Adapter.RemoveDevice =       ifc->GetMember("RemoveDevice");
                test::org.bluez.Adapter.SetProperty =        ifc->GetMember("SetProperty");
                test::org.bluez.Adapter.StartDiscovery =     ifc->GetMember("StartDiscovery");
                test::org.bluez.Adapter.StopDiscovery =      ifc->GetMember("StopDiscovery");
                test::org.bluez.Adapter.DeviceCreated =      ifc->GetMember("DeviceCreated");
                test::org.bluez.Adapter.DeviceDisappeared =  ifc->GetMember("DeviceDisappeared");
                test::org.bluez.Adapter.DeviceFound =        ifc->GetMember("DeviceFound");
                test::org.bluez.Adapter.DeviceRemoved =      ifc->GetMember("DeviceRemoved");
                test::org.bluez.Adapter.PropertyChanged =    ifc->GetMember("PropertyChanged");

            } else if (table.desc == bzServiceIfcTbl) {
                test::org.bluez.Service.interface =          ifc;
                test::org.bluez.Service.AddRecord =          ifc->GetMember("AddRecord");
                test::org.bluez.Service.RemoveRecord =       ifc->GetMember("RemoveRecord");

            } else {
                test::org.bluez.Device.interface =           ifc;
                test::org.bluez.Device.DiscoverServices =    ifc->GetMember("DiscoverServices");
                test::org.bluez.Device.GetProperties =       ifc->GetMember("GetProperties");
                test::org.bluez.Device.DisconnectRequested = ifc->GetMember("DisconnectRequested");
                test::org.bluez.Device.PropertyChanged =     ifc->GetMember("PropertyChanged");
            }
        }
    }

    bzManagerObj.AddInterface(*test::org.bluez.Manager.interface);


    status = bzManagerObj.MethodCall(*test::org.bluez.Manager.DefaultAdapter, NULL, 0, reply);
    if (status != ER_OK) {
        printf("bzManagerObj.MethodCall() failed: %s\n", QCC_StatusText(status));
        return 1;
    }

    String adapterObjPath = reply->GetArg(0)->v_objPath.str;

    bzAdapterObj = ProxyBusObject(bus, bzBusName, adapterObjPath.c_str(), 0);
    bzAdapterObj.AddInterface(*test::org.bluez.Adapter.interface);

    Crasher crasher(bus, bzAdapterObj, wait, stopTime);
    crasher.Start();

    status = bzAdapterObj.MethodCall(*test::org.bluez.Adapter.StartDiscovery, NULL, 0);
    if (status != ER_OK) {
        printf("Failed to start discovery: %s\n", QCC_StatusText(status));
        return 1;
    }

    crasher.Join();

    bzAdapterObj.MethodCall(*test::org.bluez.Adapter.StopDiscovery, NULL, 0);

    return 0;
}
