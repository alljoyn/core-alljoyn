/**
 * @file
 * BLEAccessor implementation for BlueZ
 */

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


/*
 * TODO:
 *
 * - Check if a discovered device via DeviceFound is already paired.  If so,
 *   don't bother calling CreateDevice, let BlueZ do so and let BlueZ continue
 *   to manage the device.
 *
 * - If we call CreateDevice for a discovered device, but another BlueZ device
 *   manager tool calls CreatePairedDevice, don't remove the device if it does
 *   not have AllJoyn support.  The 'Paired" property will be set if another BlueZ
 *   device manager calls CreatePairedDevice.
 *
 * - Work with BlueZ community to develop a better system to allow autonomous
 *   connections like that needed by AllJoyn.
 *   - Get SDP information without the need to call CreateDevice.
 *   - Add a method to allow BlueZ to update its UUID list for remote devices
 *     without the need to remove the device and re-add it.
 */


#include <qcc/platform.h>

#include <endian.h>
#include <errno.h>
#include <fcntl.h>

#include <qcc/SocketStream.h>
#include <qcc/String.h>
#include <qcc/StringSource.h>
#include <qcc/Timer.h>
#include <qcc/XmlElement.h>
#include <qcc/BDAddress.h>
#include <qcc/BLEStream.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/Message.h>
#include <alljoyn/MessageReceiver.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/version.h>

#include "AdapterObject.h"
#include "DeviceObject.h"
#include "BlueZIfc.h"
#include "BLEAccessor.h"
#include "BTController.h"
#include "DaemonBLETransport.h"

#include <alljoyn/Status.h>

#define QCC_MODULE "BLE"


#define SignalHandler(_a) static_cast<MessageReceiver::SignalHandler>(_a)
#define PropertyHandler(_a) static_cast<_AdapterObject::PropertyHandler>(_a)

using namespace ajn;
using namespace ajn::bluez;
using namespace qcc;
using namespace std;

namespace ajn {

/******************************************************************************/

/*
 * Timeout for various operations
 */
#define BT_DEFAULT_TO      10000
#define BT_GETPROP_TO      3000
#define BT_SDPQUERY_TO     60000
#define BT_CREATE_DEV_TO   60000

#define MAX_CONNECT_ATTEMPTS  3
#define MAX_CONNECT_WAITS    30

#define EXPIRE_DEVICE_TIME 15000
#define EXPIRE_DEVICE_TIME_EXT 5000

static bool connectable = false;
static const char alljoynUUIDBase[] = ALLJOYN_BT_UUID_BASE;
#define ALLJOYN_BT_UUID_REV_SIZE (sizeof("12345678") - 1)
#define ALLJOYN_BT_UUID_BASE_SIZE (sizeof(alljoynUUIDBase) - 1)


static const char sdpXmlTemplate[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<record>"
    "    <attribute id=\"0x0000\">"
    "        <uint32 value=\"0x4F492354\"/>"
    "    </attribute>"
    "    <attribute id=\"0x0002\">"
    "        <uint32 value=\"0x00000001\"/>"
    "    </attribute>"
    "    <attribute id=\"0x0008\">"
    "        <uint8 value=\"0xFF\"/>"
    "    </attribute>"
    "    <attribute id=\"0x0004\">"
    "        <sequence>"
    "            <sequence>"
    "                <uuid value=\"0x0100\"/>"
    "            </sequence>"
    "        </sequence>"
    "    </attribute>"
    "    <attribute id=\"0x0005\">"
    "        <sequence>"
    "            <uuid value=\"0x00001002\"/>"
    "        </sequence>"
    "    </attribute>"
    "    <attribute id=\"0x0001\">"
    "        <sequence>"
    "            <uuid value=\"%08x%s\"/>" // AllJoyn UUID - filled in later
    "        </sequence>"
    "    </attribute>"
    "    <attribute id=\"0x0400\">"    // AllJoyn Version number
    "        <uint32 value=\"%#08x\"/>" // filled in later
    "    </attribute>"
    "    <attribute id=\"0x0401\">"
    "        <text value=\"%s\"/>"      // Filled in with dynamically determined BD Address
    "    </attribute>"
    "    <attribute id=\"0x0402\">"
    "        <uint16 value=\"%#08x\"/>" // Filled in with dynamically determined L2CAP PSM number
    "    </attribute>"
    "    <attribute id=\"0x0404\">"
    "        <sequence>%s</sequence>"  // Filled in with advertisement information
    "    </attribute>"
    "    <attribute id=\"0x0100\">"
    "        <text value=\"AllJoyn\"/>"
    "    </attribute>"
    "    <attribute id=\"0x0101\">"
    "        <text value=\"AllJoyn Distributed Message Bus\"/>"
    "    </attribute>"
    "</record>";





/******************************************************************************/

DaemonBLETransport::BLEAccessor::BLEAccessor(DaemonBLETransport* transport,
                                             const qcc::String& busGuid) :
    bzBus("BlueZTransport"),
    busGuid(busGuid),
    transport(transport),
    recordHandle(0),
    timer("BT-Dispatcher"),
    bluetoothAvailable(false),
    discoverable(false),
    discoveryCtrl(0),
    l2capLFd(-1),
    l2capEvent(NULL),
    cod(0)
{
    size_t tableIndex, member;

    // Must be initialized after 'bzBus' is initialized!
    bzManagerObj = ProxyBusObject(bzBus, bzBusName, bzMgrObjPath, 0);

    for (tableIndex = 0; tableIndex < ifcTableSize; ++tableIndex) {
        InterfaceDescription* ifc;
        const InterfaceTable& table(ifcTables[tableIndex]);
        bzBus.CreateInterface(table.ifcName, ifc);

        if (ifc) {
            for (member = 0; member < table.tableSize; ++member) {
                if (table.desc[member].type) {
                    ifc->AddMember(table.desc[member].type,
                                   table.desc[member].name,
                                   table.desc[member].inputSig,
                                   table.desc[member].outSig,
                                   table.desc[member].argNames,
                                   table.desc[member].annotation);
                } else {
                    ifc->AddProperty(table.desc[member].name,
                                     table.desc[member].inputSig,
                                     table.desc[member].annotation);
                }
            }
            ifc->Activate();

            if (table.desc == bzObjMgrIfcTbl) {
                org.bluez.ObjMgr.interface =           ifc;
                org.bluez.ObjMgr.GetManagedObjects =   ifc->GetMember("GetManagedObjects");
                org.bluez.ObjMgr.InterfacesAdded =     ifc->GetMember("InterfacesAdded");
                org.bluez.ObjMgr.InterfacesRemoved =   ifc->GetMember("InterfacesRemoved");

            } else if (table.desc == bzAdapter1IfcTbl) {
                org.bluez.Adapter1.interface =           ifc;
                org.bluez.Adapter1.RemoveDevice =        ifc->GetMember("RemoveDevice");
                org.bluez.Adapter1.StartDiscovery =      ifc->GetMember("StartDiscovery");
                org.bluez.Adapter1.StopDiscovery =       ifc->GetMember("StopDiscovery");

            } else if (table.desc == bzDevice1IfcTbl) {
                org.bluez.Device1.interface =           ifc;
                org.bluez.Device1.Connect =             ifc->GetMember("Connect");
                org.bluez.Device1.Disconnect =          ifc->GetMember("Disconnect");
                org.bluez.Device1.ConnectProfile =      ifc->GetMember("ConnectProfile");
                org.bluez.Device1.DisconnectProfile =   ifc->GetMember("DisconnectProfile");
                org.bluez.Device1.Pair =                ifc->GetMember("Pair");
                org.bluez.Device1.CancelPairing =       ifc->GetMember("CancelPairing");

            } else if (table.desc == bzAllJoynMgrIfcTbl) {
                org.bluez.AllJoynMgr.interface =        ifc;
                org.bluez.AllJoynMgr.SetUuid =          ifc->GetMember("SetUuid");

            } else if (table.desc == bzAllJoynIfcTbl) {
                org.bluez.AllJoyn.interface =           ifc;
                org.bluez.AllJoyn.TxDataSend =          ifc->GetMember("TxDataSend");
                org.bluez.AllJoyn.RxDataRecv =          ifc->GetMember("RxDataRecv");

            }
        }
    }

    //bzManagerObj.AddInterface(*org.bluez.Manager.interface);
    bzManagerObj.AddInterface(*org.bluez.ObjMgr.interface);
    bzBus.RegisterBusListener(*this);

    bzBus.RegisterSignalHandler(this,
                                SignalHandler(&DaemonBLETransport::BLEAccessor::InterfacesAddedSignalHandler),
                                org.bluez.ObjMgr.InterfacesAdded, "/");

    bzBus.RegisterSignalHandler(this,
                                SignalHandler(&DaemonBLETransport::BLEAccessor::InterfacesRemovedSignalHandler),
                                org.bluez.ObjMgr.InterfacesRemoved, "/");

    timer.Start();
}


DaemonBLETransport::BLEAccessor::~BLEAccessor()
{
    adapterMap.clear();
    deviceMap.clear();
    if (l2capEvent) {
        delete l2capEvent;
    }
}


QStatus DaemonBLETransport::BLEAccessor::Start()
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::Start()"));

    QStatus status = ER_OK;
    bool alreadyStarted = bzBus.IsStarted();
    bool newlyStarted = false;


    /* Start the control bus */
    if (!alreadyStarted) {
        status = bzBus.Start();

        newlyStarted = status == ER_OK;
    }

    if (status == ER_OK) {
        MsgArg arg;
        Message reply(bzBus);
        const ProxyBusObject& dbusObj = bzBus.GetDBusProxyObj();
        const InterfaceDescription* ifc(bzBus.GetInterface(ajn::org::freedesktop::DBus::InterfaceName));
        const InterfaceDescription::Member* addMatch;
        const InterfaceDescription::Member* nameHasOwner;

        /* Get environment variable for the system bus */
        Environ* env(Environ::GetAppEnviron());
#ifdef QCC_OS_ANDROID
        connectArgs = env->Find("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/dev/socket/dbus");
#else
        connectArgs = env->Find("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/var/run/dbus/system_bus_socket");
#endif

        assert(ifc);
        if (!ifc) {
            status = ER_FAIL;
            QCC_LogError(status, ("Failed to get DBus interface description from AllJoyn"));
            goto exit;
        }

        addMatch = ifc->GetMember("AddMatch");
        nameHasOwner = ifc->GetMember("NameHasOwner");

        /* Create the endpoint for talking to the Bluetooth subsystem */
        status = bzBus.Connect(connectArgs.c_str());
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to create UNIX endpoint"));
            goto exit;
        }

        if (newlyStarted) {
            /* Add Match rules */
            qcc::String rules[] = {
                qcc::String("type='signal',sender='") + bzBusName + "',interface='" + bzAllJoynIfc + "'",
                qcc::String("type='signal',sender='") + bzBusName + "',interface='" + bzObjMgrIfc + "'",
                qcc::String("type='signal',sender='") + ajn::org::freedesktop::DBus::WellKnownName + "',interface='" + ajn::org::freedesktop::DBus::InterfaceName + "'"
            };

            for (size_t i = 0; (status == ER_OK) && (i < ArraySize(rules)); ++i) {
                arg.Set("s", rules[i].c_str());
                status = dbusObj.MethodCall(*addMatch, &arg, 1, reply);
                if (status != ER_OK) {
                    QCC_LogError(status, ("Failed to add match rule: \"%s\"", rules[i].c_str()));
                    QCC_DbgHLPrintf(("reply msg: %s\n", reply->ToString().c_str()));
                }
            }
        }

        // Find out if the Bluetooth subsystem is running...
        arg.Set("s", bzBusName);
        status = dbusObj.MethodCall(*nameHasOwner, &arg, 1, reply);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failure calling %s.NameHasOwner",
                                  ajn::org::freedesktop::DBus::InterfaceName));
            QCC_DbgHLPrintf(("reply msg: %s\n", reply->ToString().c_str()));
        } else if (reply->GetArg(0)->v_bool) {
            ConnectBlueZ();
        }
    }

exit:
    return status;
}


void DaemonBLETransport::BLEAccessor::Stop()
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::Stop()"));
    if (bluetoothAvailable) {
        DisconnectBlueZ();
    }
    bzBus.Disconnect(connectArgs.c_str());
}


void DaemonBLETransport::BLEAccessor::ConnectBlueZ()
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::ConnectBlueZ()"));
    /*
     * It's ok if no adapters were found, we'll tell the upper layers everything is OK so
     * that when an adapter does become available it can be used. If there is an adapter we
     * can update the service record.
     */
    if (!bluetoothAvailable &&
        (EnumerateAdapters() == ER_OK)) {
        AdapterObject adapter = GetDefaultAdapterObject();
        if (adapter->IsValid() && adapter->IsPowered()) {
            bluetoothAvailable = true;
            transport->BLEDeviceAvailable(true);
        }
    }
}


void DaemonBLETransport::BLEAccessor::DisconnectBlueZ()
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::DisconnectBlueZ()"));

    transport->BLEDeviceAvailable(false);

    /*
     * Shut down all endpoints
     */
    transport->DisconnectAll();
    bluetoothAvailable = false;

    /*
     * Invalidate the adapters.
     */
    adapterLock.Lock(MUTEX_CONTEXT);
    adapterMap.clear();
    deviceMap.clear();
    defaultAdapterObj = AdapterObject();
    anyAdapterObj = AdapterObject();
    adapterLock.Unlock(MUTEX_CONTEXT);
}


void DaemonBLETransport::BLEAccessor::NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
{
    if ((strcmp(busName, bzBusName) == 0) && !newOwner && bluetoothAvailable) {
        // Apparently bluetoothd crashed.  Let the upper layers know so they can reset themselves.
        QCC_DbgHLPrintf(("BlueZ's bluetoothd D-Bus service crashed!"));
        bluetoothAvailable = false;
        transport->BLEDeviceAvailable(false);
    }
}


QStatus DaemonBLETransport::BLEAccessor::StartDiscovery(const BDAddressSet& ignoreAddrs, uint32_t duration)
{
    this->ignoreAddrs = ignoreAddrs;

    deviceLock.Lock(MUTEX_CONTEXT);
    set<qcc::BDAddress>::const_iterator it;
    for (it = ignoreAddrs->begin(); it != ignoreAddrs->end(); ++it) {
        FoundInfoMap::iterator devit = foundDevices.find(*it);
        if (devit != foundDevices.end()) {
            foundDevices.erase(devit);
        }
    }
    deviceLock.Unlock(MUTEX_CONTEXT);

    QCC_DbgPrintf(("Start Discovery"));
    QStatus status = DiscoveryControl(true);
    if (duration > 0) {
        DispatchOperation(new DispatchInfo(DispatchInfo::STOP_DISCOVERY),  duration * 1000);
    }
    return status;
}


QStatus DaemonBLETransport::BLEAccessor::PushBytes(String remObj, const void* buf, size_t numBytes, size_t& actualBytes)
{
    QStatus status;
    DeviceObject*dev = deviceProxyMap[remObj];
    BLEController*controller = deviceMap[remObj];
    MsgArg arg("ay", numBytes, buf);

    if (!dev || !controller) {
        return ER_OK;
    }

    actualBytes = numBytes;
    QCC_DbgTrace(("PushBytes for %s (%p)", remObj.c_str(), dev));
    if (!controller->IsConnected()) {
        status = (*dev)->MethodCall(*org.bluez.Device1.Connect, NULL, 0);
        QCC_LogError(status, ("Connect"));
    } else {
        status = (*dev)->MethodCall(*org.bluez.AllJoyn.TxDataSend, &arg, 1);
        QCC_LogError(status, ("PushBytes"));
    }
    return status;
}

QStatus DaemonBLETransport::BLEAccessor::StopDiscovery()
{
    QCC_DbgPrintf(("Stop Discovery"));

    return ER_OK;
}


QStatus DaemonBLETransport::BLEAccessor::StartConnectable()
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::StartConnectable()"));

    QCC_DbgPrintf(("Starting BLE Scanning..."));
    DiscoveryControl(true);
    connectable = true;

    return ER_OK;
}


void DaemonBLETransport::BLEAccessor::StopConnectable()
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::StopConnectable()"));
}


QStatus DaemonBLETransport::BLEAccessor::InitializeAdapterInformation(AdapterObject& adapter,
                                                                      const MsgArg* props)
{
    QStatus status = ER_FAIL;

    if (adapter->IsValid()) {
        Message rsp(bzBus);
        const MsgArg* arg;
        const char* bdAddrStr;
        bool powered;
        bool disc;

        if (props) {
            arg = props;
        }

        if (!arg) {
            status = ER_FAIL;
            goto exit;
        }

        status = arg->GetElement("{ss}", "Address", &bdAddrStr);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to get Address"));
            goto exit;
        }

        status = arg->GetElement("{su}", "Class", &cod);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to get Class"));
            goto exit;
        }

        status = arg->GetElement("{sb}", "Powered", &powered);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to get Powered"));
            goto exit;
        }
        status = arg->GetElement("{sb}", "Discovering", &disc);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to get Discovering"));
            goto exit;
        }

        status = adapter->SetAddress(bdAddrStr);
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to set Address"));
            goto exit;
        }

        adapter->SetDiscovering(disc);
        adapter->SetPowered(powered);

        if (adapter == GetDefaultAdapterObject()) {
            if (powered != bluetoothAvailable) {
                bluetoothAvailable = powered;
                transport->BLEDeviceAvailable(powered);
            }
        }
    }

exit:
    return status;
}


RemoteEndpoint DaemonBLETransport::BLEAccessor::Accept(BusAttachment& alljoyn,
                                                       Event* connectEvent)
{
    // Delete
    RemoteEndpoint conn;
    return conn;
}


RemoteEndpoint DaemonBLETransport::BLEAccessor::Connect(BusAttachment& alljoyn,
                                                        qcc::String objPath)
{

    //Delete
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::Connect(dev = %s)",
                  objPath.c_str()));
    RemoteEndpoint conn;

    QStatus status = ER_OK;
    bool connected = false;

    QCC_DbgPrintf(("Pause Discovery"));
    DiscoveryControl(false);

    for (int tries = 0; tries < MAX_CONNECT_ATTEMPTS; ++tries) {
        // Use dev connect method
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("Connect to %s failed (errno: %d - %s)",
                              objPath.c_str(), errno, strerror(errno)));
        goto exit;
    }

    if (!connected) {
        status = ER_FAIL;
        QCC_LogError(status, ("Failed to establish connection with %s", objPath.c_str()));
        goto exit;
    }

    QCC_DbgPrintf(("DaemonBLETransport::BLEAccessor::Connect() success dev = %s",
                   objPath.c_str()));

exit:

    QCC_DbgPrintf(("Resume Discovery"));
    DiscoveryControl(true);

    return conn;
}


QStatus DaemonBLETransport::BLEAccessor::EnumerateAdapters()
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::EnumerateAdapters()"));
    QStatus status;
    Message rsp(bzBus);
    bool adapter_found = false;
    const char* adapter_object;

    //status = bzManagerObj.MethodCall(*org.bluez.Manager.ListAdapters, NULL, 0, rsp, BT_DEFAULT_TO);
    status = bzManagerObj.MethodCall(*org.bluez.ObjMgr.GetManagedObjects, NULL, 0, rsp, BT_DEFAULT_TO);
    if (status == ER_OK) {
        size_t numRecs;
        const MsgArg* records;

        QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::GetManagedObjects() responded"));
        rsp->GetArg(0)->Get("a{oa{sa{sv}}}", &numRecs, &records);

        QCC_DbgTrace(("GetManagedObjects() == %d", numRecs));
        for (size_t i = 0; i < numRecs; ++i) {
            bool connected;
            const char* object;
            size_t numIfcs;
            const MsgArg* ifcs;
            const MsgArg* props;

            records[i].Get("{oa{sa{sv}}}", &object, &numIfcs, &ifcs);
            QCC_DbgTrace(("   GetManagedObjects() == object:%s", object));
            connected = false;
            for (size_t j = 0; j < numIfcs; ++j) {
                const char* ifc;
                ifcs[j].Get("{s*}", &ifc, &props);
                if (strcmp(ifc, bzAdapter1Ifc) == 0) {
                    QCC_DbgTrace(("      GetManagedObjects() == ifc:%s", ifc));
                    AdapterAdded(object, props);
                    adapter_found = true;
                    adapter_object = object;
                } else if (strcmp(ifc, bzDevice1Ifc) == 0) {
                    QCC_DbgTrace(("      GetManagedObjects() == ifc:%s", ifc));
                    status = props->GetElement("{sb}", "Connected", &connected);
                    if (status == ER_OK && connected) {
                        BLEController* controller = deviceMap[object];
                        DeviceObject* dev = deviceProxyMap[object];
                        if (controller) {
                            controller->SetConnected(connected);
                        }
                        if (dev) {
                            (*dev)->SetConnected(connected);
                        }
                    } else {
                        connected = false;
                    }
                } else if (strcmp(ifc, bzAllJoynIfc) == 0) {
                    const char* watchProps[] = {
                        "Connected"
                    };

                    QCC_DbgTrace(("      GetManagedObjects() == ifc:%s", ifc));
                    DeviceObject* dev = new DeviceObject(bzBus, object);
                    (*dev)->AddInterface(*org.bluez.Device1.interface);
                    (*dev)->AddInterface(*org.bluez.AllJoyn.interface);
                    deviceProxyMap[object] = dev;
                    (*dev)->RegisterPropertiesChangedHandler(bzDevice1Ifc, watchProps, ArraySize(watchProps), *this, NULL);
                    QCC_DbgTrace(("Register RxDataRecv for %s (%p)", object, dev));
                    bzBus.RegisterSignalHandler(this,
                                                SignalHandler(&DaemonBLETransport::BLEAccessor::RxDataRecvSignalHandler),
                                                org.bluez.AllJoyn.RxDataRecv, object);

                    /* Notify Transport of device if new */
                    if (!deviceMap[object]) {
                        BLEController* controller = transport->NewDeviceFound(object);
                        if (controller) {
                            deviceMap[object] = controller;
                            QCC_DbgTrace(("Save BLEController %p for \"%s\" %p %p", controller, object, &deviceMap, deviceMap[object]));
                        }
                    }
                    if (connected) {
                        BLEController* controller = deviceMap[object];
                        if (controller) {
                            controller->SetConnected(connected);
                        }
                        if (dev) {
                            (*dev)->SetConnected(connected);
                        }
                    }
                }
            }
        }
    } else {
        QCC_LogError(status, ("EnumerateAdapters(): 'GetManagedObjects' method call failed"));
    }

    if (adapter_found) {
        adapterLock.Lock(MUTEX_CONTEXT);
        defaultAdapterObj = GetAdapterObject(adapter_object);
        if (defaultAdapterObj->IsValid()) {
            MsgArg arg("s", ALLJOYN_UUID);
            status = defaultAdapterObj->MethodCall(*org.bluez.AllJoynMgr.SetUuid, &arg, 1);

            const char* watchProps[] = {
                "Powered",
                "Discovering"
            };

            status = defaultAdapterObj->RegisterPropertiesChangedHandler("org.bluez.Adapter1",
                                                                         watchProps, ArraySize(watchProps), *this, NULL);
            if (status != ER_OK) {
                QCC_LogError(status, ("RegisterPropertiesChangedHandler"));
            }

            if (!defaultAdapterObj->IsPowered()) {
                MsgArg discVal("b", true);
                // Power up adapter if not already powered.
                status = defaultAdapterObj->SetProperty("org.bluez.Adapter1", "Powered", discVal, BT_DEFAULT_TO);
                if (status == ER_OK) {
                    QCC_DbgTrace(("Bluetooth: Powered by AllJoyn"));
                    bluetoothAvailable = true;
                    //defaultAdapterObj->SetPowered(true);
                    transport->BLEDeviceAvailable(true);
                }
            } else {
                transport->BLEDeviceAvailable(true);
                DiscoveryControl(org.bluez.Adapter1.StartDiscovery);
            }
        } else {
            QCC_DbgHLPrintf(("Invalid object path: \"%s\"", adapter_object));
            status = ER_FAIL;
        }
        adapterLock.Unlock(MUTEX_CONTEXT);
    } else {
        QCC_DbgHLPrintf(("Finding default adapter path failed, most likely no bluetooth device connected (status = %s)",
                         QCC_StatusText(status)));
    }

    return status;
}


void DaemonBLETransport::BLEAccessor::AdapterAdded(const char* adapterObjPath,
                                                   const MsgArg* props)
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::AdapterAdded(adapterObjPath = \"%s\")", adapterObjPath));

    AdapterObject ao = GetAdapterObject(adapterObjPath);
    if (ao->IsValid()) {
        QCC_LogError(ER_FAIL, ("Adapter %s already exists", adapterObjPath));
        return;
    }

    qcc::String objPath(adapterObjPath);
    AdapterObject newAdapterObj(bzBus, objPath);

    newAdapterObj->AddInterface(*org.bluez.Adapter1.interface);
    newAdapterObj->AddInterface(*org.bluez.AllJoynMgr.interface);

    QStatus status = InitializeAdapterInformation(newAdapterObj, props);
    if (status != ER_OK) {
        return;
    }

    adapterLock.Lock(MUTEX_CONTEXT);
    adapterMap[newAdapterObj->GetPath()] = newAdapterObj;

    /* Device Found Signal now: /, org.freedesktop.DBus.ObjectManager, InterfacesAdded
     */

    adapterLock.Unlock(MUTEX_CONTEXT);
}

void DaemonBLETransport::BLEAccessor::AdapterAdded(const char* adapterObjPath)
{
    AdapterAdded(adapterObjPath, NULL);
}

void DaemonBLETransport::BLEAccessor::AdapterRemoved(const char* adapterObjPath)
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::AdapterRemoved(adapterObjPath = \"%s\")", adapterObjPath));

    adapterLock.Lock(MUTEX_CONTEXT);
    AdapterMap::iterator ait(adapterMap.find(adapterObjPath));
    if (ait != adapterMap.end()) {
        if (ait->second == defaultAdapterObj) {
            defaultAdapterObj = AdapterObject();
            bluetoothAvailable = false;
            transport->BLEDeviceAvailable(false);
        }
        adapterMap.erase(ait);
    }
    adapterLock.Unlock(MUTEX_CONTEXT);
}


void DaemonBLETransport::BLEAccessor::DefaultAdapterChanged(const char* adapterObjPath)
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::DefaultAdapterChanged(adapterObjPath = \"%s\")", adapterObjPath));

    adapterLock.Lock(MUTEX_CONTEXT);
    defaultAdapterObj = GetAdapterObject(adapterObjPath);
    if (defaultAdapterObj->IsValid()) {
        bluetoothAvailable = true;
        transport->BLEDeviceAvailable(true);
    }
    adapterLock.Unlock(MUTEX_CONTEXT);

    if (discoveryCtrl == 1) {
        DiscoveryControl(org.bluez.Adapter1.StartDiscovery);
    }
}


void DaemonBLETransport::BLEAccessor::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    DispatchInfo* op = static_cast<DispatchInfo*>(alarm->GetContext());

    if (reason == ER_OK) {
        switch (op->operation) {
        case DispatchInfo::STOP_DISCOVERY:
            QCC_DbgPrintf(("Stopping Discovery"));
            StopDiscovery();
            break;

        case DispatchInfo::ADAPTER_ADDED:
            AdapterAdded(static_cast<AdapterDispatchInfo*>(op)->adapterPath.c_str());
            break;

        case DispatchInfo::ADAPTER_REMOVED:
            AdapterRemoved(static_cast<AdapterDispatchInfo*>(op)->adapterPath.c_str());
            break;

        case DispatchInfo::DEFAULT_ADAPTER_CHANGED:
            DefaultAdapterChanged(static_cast<AdapterDispatchInfo*>(op)->adapterPath.c_str());
            break;
        }
    }

    delete op;
}


void DaemonBLETransport::BLEAccessor::AdapterAddedSignalHandler(const InterfaceDescription::Member* member,
                                                                const char* sourcePath,
                                                                Message& msg)
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::AdapterAddedSignalHandler - signal from \"%s\"", sourcePath));
    DispatchOperation(new AdapterDispatchInfo(DispatchInfo::ADAPTER_ADDED, msg->GetArg(0)->v_objPath.str));
}


void DaemonBLETransport::BLEAccessor::AdapterRemovedSignalHandler(const InterfaceDescription::Member* member,
                                                                  const char* sourcePath,
                                                                  Message& msg)
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::AdapterRemovedSignalHandler - signal from \"%s\"", sourcePath));
    DispatchOperation(new AdapterDispatchInfo(DispatchInfo::ADAPTER_REMOVED, msg->GetArg(0)->v_objPath.str));
}


void DaemonBLETransport::BLEAccessor::DefaultAdapterChangedSignalHandler(const InterfaceDescription::Member* member,
                                                                         const char* sourcePath,
                                                                         Message& msg)
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::DefaultAdapterChangedSignalHandler - signal from \"%s\"", sourcePath));
    /*
     * We are in a signal handler so kick off the restart in a new thread.
     */
    DispatchOperation(new AdapterDispatchInfo(DispatchInfo::DEFAULT_ADAPTER_CHANGED, msg->GetArg(0)->v_objPath.str));
}

void DaemonBLETransport::BLEAccessor::RxDataRecvSignalHandler(const InterfaceDescription::Member* member,
                                                              const char* sourcePath,
                                                              Message& msg)
{
    const uint8_t* rx_data = NULL;
    int count = 0;
    QStatus status;

    status = msg->GetArgs("ay", &count, &rx_data);
    if (status != ER_OK) {
        QCC_LogError(status, ("Parsing args from RxDataRecvSignalHandler signal"));
        return;
    }
    BLEController* controller = deviceMap[sourcePath];
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::RxDataRecvSignalHandler %s: (%d) %p %p", sourcePath, count, controller, &deviceMap));
    controller->ReadCallback(rx_data, count);
}

void DaemonBLETransport::BLEAccessor::InterfacesAddedSignalHandler(const InterfaceDescription::Member* member,
                                                                   const char* sourcePath,
                                                                   Message& msg)
{
    const char* objStr;
    const char* addrStr;
    MsgArg* dictionary;
    MsgArg* props;
    const MsgArg* uuids;
    size_t uuidCnt;
    bool connected = false;

    QStatus status;

    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::InterfacesAddedSignalHandler - signal from \"%s\"", sourcePath));

    /* This signal will pick up found remote devices, and new local BT Adapters */
    status = msg->GetArgs("o*", &objStr, &dictionary);
    if (status != ER_OK) {
        QCC_LogError(status, ("Parsing args from DeviceFound signal"));
        return;
    }

    DeviceObject* dev = deviceProxyMap[objStr];
    const char* watchProps[] = {
        "Connected"
    };


    status = dictionary->GetElement("{s*}", bzDevice1Ifc, &props);
    if (status == ER_OK) {
        QCC_LogError(status, ("Device1 interface Found for %s", objStr));
        if (!dev) {
            dev = new DeviceObject(bzBus, objStr);
            (*dev)->AddInterface(*org.bluez.Device1.interface);
            (*dev)->AddInterface(*org.bluez.AllJoyn.interface);
            deviceProxyMap[objStr] = dev;
            (*dev)->RegisterPropertiesChangedHandler(bzDevice1Ifc, watchProps, ArraySize(watchProps), *this, NULL);
        }
    }

    status = dictionary->GetElement("{s*}", bzAllJoynIfc, &props);
    if (status == ER_OK) {
        QCC_LogError(status, ("AllJoyn interface Found for %s", objStr));
        if (!dev) {
            dev = new DeviceObject(bzBus, objStr);
            (*dev)->AddInterface(*org.bluez.Device1.interface);
            (*dev)->AddInterface(*org.bluez.AllJoyn.interface);
            deviceProxyMap[objStr] = dev;
            (*dev)->RegisterPropertiesChangedHandler(bzDevice1Ifc, watchProps, ArraySize(watchProps), *this, NULL);
        }
        QCC_DbgTrace(("Register RxDataRecv for %s (%p)", objStr, dev));
        bzBus.RegisterSignalHandler(this,
                                    SignalHandler(&DaemonBLETransport::BLEAccessor::RxDataRecvSignalHandler),
                                    org.bluez.AllJoyn.RxDataRecv, objStr);

        /* Notify Transport of device if new */
        if (!deviceMap[objStr]) {
            BLEController* controller = transport->NewDeviceFound(objStr);
            if (controller) {
                deviceMap[objStr] = controller;
                QCC_DbgTrace(("Save BLEController %p for \"%s\" %p %p", controller, objStr, &deviceMap, deviceMap[objStr]));
            }
        }
    }

    /* As an Else here, we might want to put handling for New Adapter, keyed
     * on the existance of the interface "org.bluez.Adapter1"
     */

    status = props->GetElement("{ss}", "Address", &addrStr);
    if (status != ER_OK) {
        QCC_LogError(status, ("Address Property on Device1 Interface not Found for %s", objStr));
        return;
    } else if (dev) {
        (*dev)->SetAddress(addrStr);
    }

    status = props->GetElement("{sb}", "Connected", &connected);
    if (status != ER_OK) {
        QCC_LogError(status, ("Connected Property on Device1 Interface not Found for %s", objStr));
    } else {
        BLEController* controller = deviceMap[objStr];
        if (controller) {
            controller->SetConnected(connected);
        }
        if (dev) {
            (*dev)->SetConnected(connected);
        }
    }

    QCC_LogError(status, ("New Remote Device:%s Address:%s", objStr, addrStr));

    if (!connectable) {
        QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::InterfacesAddedSignalHandler: Not Connectable"));
    }

    status = props->GetElement("{sas}", "UUIDs", &uuidCnt, &uuids);
    if (status != ER_OK) {
        QCC_LogError(status, ("No UUIDs found for %s", objStr));
        return;
    }

    QCC_DbgTrace(("UUID cnt: %d", uuidCnt));
    if (uuidCnt) {
        for (size_t i = 0; i < uuidCnt; ++i) {
            const char* uuid;

            status = uuids[i].Get("s", &uuid);
            QCC_LogError(status, ("New UUID:%s Address:%s", uuid, addrStr));
            if ((status == ER_OK) &&
                (strcmp(uuid, ALLJOYN_UUID) == 0)) {
                /* Notify Transport of device if new */
                if (!deviceMap[objStr]) {
                    BLEController* controller = transport->NewDeviceFound(objStr);
                    if (controller) {
                        controller->SetConnected(connected);
                        deviceMap[objStr] = controller;
                        QCC_DbgTrace(("Save BLEController %p for \"%s\" %p %p", controller, objStr, &deviceMap, deviceMap[objStr]));
                    }
                }
            }
        }
    }
}

void DaemonBLETransport::BLEAccessor::InterfacesRemovedSignalHandler(const InterfaceDescription::Member* member,
                                                                     const char* sourcePath,
                                                                     Message& msg)
{
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::InterfacesRemovedSignalHandler - signal from \"%s\"", sourcePath));
}


QStatus DaemonBLETransport::BLEAccessor::GetDeviceObjPath(const qcc::BDAddress& bdAddr,
                                                          qcc::String& devObjPath)
{
    const qcc::String& bdAddrStr(bdAddr.ToString());
    QCC_DbgTrace(("DaemonBLETransport::BLEAccessor::GetDeviceObjPath(bdAddr = %s)", bdAddrStr.c_str()));
    QStatus status(ER_NONE);
    Message rsp(bzBus);
    MsgArg arg("s", bdAddrStr.c_str());
    vector<AdapterObject> adapterList;
    AdapterObject adapter;

    /*
     * Getting the object path for a device is inherently racy.  The
     * FindDevice method call will return an error if the device has not been
     * created and the CreateDevice method call will return an error if the
     * device already exists.  The problem is that anyone with access to the
     * BlueZ d-bus service can create and remove devices from the list.  In
     * theory another process could add or remove a device between the time we
     * call CreateDevice and FindDevice.
     */

    // Get a copy of all the adapter objects to check.
    adapterList.reserve(adapterMap.size());
    adapterLock.Lock(MUTEX_CONTEXT);
    for (AdapterMap::const_iterator ait = adapterMap.begin(); ait != adapterMap.end(); ++ait) {
        adapterList.push_back(ait->second);
    }
    adapterLock.Unlock(MUTEX_CONTEXT);

    // Call
    // status = *bzManagerObj.MethodCall(*org.bluez.ObjMgr.GetManagedObjects, NULL, 0, rsp, BT_DEFAULT_TO);
    if (status != ER_OK) {
        //status = (*it)->MethodCall(*org.bluez.Adapter.FindDevice, &arg, 1, rsp, BT_DEFAULT_TO);
    }

    if (status == ER_OK) {
        char* objPath;
        rsp->GetArg(0)->Get("o", &objPath);
        devObjPath = objPath;
    }

    return status;
}


QStatus DaemonBLETransport::BLEAccessor::DiscoveryControl(bool start)
{
    const InterfaceDescription::Member* method = NULL;
    QStatus status = ER_OK;

    int32_t ctrl;

    // The discovery control value can range between -2 and +1 where -2, -1
    // and 0 mean discovery should be off and +1 means discovery should be on.
    // The initial value is 0 and is incremented to +1 when BTController
    // starts discovery.  Connect and GetDeviceInfo both try to pause
    // discovery thus decrementing the count to 0, -1, or possibly (but not
    // likely) -2.  The count should never exceed +1 nor be less than -2.
    // (The only way to reach -2 would be if we were trying to get device
    // information while connecting to another device, and BTController
    // decided to stop discovery.  When the get device information and connect
    // operations complete, the count will return to 0).
    if (start) {
        ctrl = IncrementAndFetch(&discoveryCtrl);
        if (ctrl == 1) {
            method = org.bluez.Adapter1.StartDiscovery;
        }
    } else {
        ctrl = DecrementAndFetch(&discoveryCtrl);
        if (ctrl == 0) {
            method = org.bluez.Adapter1.StopDiscovery;
        }
    }

    QCC_DbgPrintf(("discovery control: %d", ctrl));
    assert((ctrl >= -20) && (ctrl <= 2));

    if (method) {
        status = DiscoveryControl(method);
    }
    return status;
}


QStatus DaemonBLETransport::BLEAccessor::DiscoveryControl(const InterfaceDescription::Member* method)
{
    QStatus status = ER_FAIL;
    AdapterObject adapter = GetDefaultAdapterObject();
    bool start = method == org.bluez.Adapter1.StartDiscovery;

    if (adapter->IsValid()) {
        Message rsp(bzBus);

        status = adapter->MethodCall(*method, NULL, 0, rsp, BT_DEFAULT_TO);
        if (status == ER_OK) {
            QCC_DbgHLPrintf(("%s discovery", start ? "Started" : "Stopped"));
        } else {
            qcc::String errMsg;
            const char* errName = rsp->GetErrorName(&errMsg);
            QCC_LogError(status, ("Call to org.bluez.Adapter1.%s failed %s - %s",
                                  method->name.c_str(),
                                  errName, errMsg.c_str()));
        }

        if (true) {
            uint64_t stopTime = GetTimestamp64() + 10000;  // give up after 10 seconds
            while ((GetTimestamp64() < stopTime) && adapter->IsValid() && (adapter->IsDiscovering() != start)) {
                QCC_DbgPrintf(("Waiting 100 ms for discovery to %s.", start ? "start" : "stop"));
                qcc::Sleep(100);
                adapter = GetDefaultAdapterObject();  // In case adapter goes away

                /* Need fix for org.freedesktop.DBus.Properties signals */
                /* But for now, assume call to start succeeded. */
                if (adapter->IsValid()) {
                    adapter->SetDiscovering(start);
                }
            }
        }
    }
    return status;
}


void DaemonBLETransport::BLEAccessor::PropertiesChanged(ProxyBusObject& obj,
                                                        const char* ifaceName,
                                                        const MsgArg& changed,
                                                        const MsgArg& invalidated,
                                                        void* context)
{
    QStatus status;
    bool val = 0;

    QCC_LogError(ER_OK, ("I WANT THIS CALLED AS THE BLEACCESSOR!!!!!!"));

    status = changed.GetElement("{sb}", "Powered", &val);
    if (status == ER_OK) {
        QCC_LogError(status, ("Adapter %s property Changed - Power: %s", obj.GetPath().c_str(), val ? "On" : "Off"));
    }

    status = changed.GetElement("{sb}", "Discovering", &val);
    if (status == ER_OK) {
        QCC_LogError(status, ("Adapter %s property Changed - Discovering: %s", obj.GetPath().c_str(), val ? "On" : "Off"));
    }

    status = changed.GetElement("{sb}", "Connected", &val);
    if (status == ER_OK) {
        QCC_LogError(status, ("Device %s property Changed - Connected: %s", obj.GetPath().c_str(), val ? "True" : "False"));

        if (val) {
            BLEController* controller = deviceMap[obj.GetPath()];
            if (controller) {
                controller->SetConnected(val);
            }
        }
    }
}


} // namespace ajn
