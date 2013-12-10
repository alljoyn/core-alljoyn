/**
 * @file
 * BTAccessor declaration for BlueZ
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

#include <qcc/Util.h>

#include <alljoyn/Message.h>

#include "BlueZIfc.h"


using namespace ajn;
using namespace ajn::bluez;

namespace ajn {
namespace bluez {

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



} // namespace bluez
} // namespace ajn
