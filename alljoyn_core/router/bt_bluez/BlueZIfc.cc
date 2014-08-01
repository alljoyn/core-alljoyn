/**
 * @file
 * BTAccessor declaration for BlueZ
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/InterfaceDescription.h>

#include "BlueZIfc.h"


using namespace ajn;
using namespace ajn::bluez;

namespace ajn {
namespace bluez {

const char* bzBusName = "org.bluez";
const char* bzMgrObjPath = "/";

const char* bzObjMgrIfc = "org.freedesktop.DBus.ObjectManager";
const char* bzAdapter1Ifc = "org.bluez.Adapter1";
const char* bzDevice1Ifc = "org.bluez.Device1";
const char* bzAllJoynIfc = "org.AllSeen.AllJoyn";

const InterfaceDesc bzObjMgrIfcTbl[] = {
    { MESSAGE_METHOD_CALL, "GetManagedObjects", NULL,         "a{oa{sa{sv}}}", NULL, 0 },
    { MESSAGE_SIGNAL,      "InterfacesAdded",   "oa{sa{sv}}", NULL,            NULL, 0 },
    { MESSAGE_SIGNAL,      "InterfacesRemoved", "oas",        NULL,            NULL, 0 },
};

const InterfaceDesc bzAdapter1IfcTbl[] = {
    { MESSAGE_METHOD_CALL, "RemoveDevice",      "o",     NULL,  NULL, 0 },
    { MESSAGE_METHOD_CALL, "StartDiscovery",    NULL,    NULL,  NULL, 0 },
    { MESSAGE_METHOD_CALL, "StopDiscovery",     NULL,    NULL,  NULL, 0 },
    { MESSAGE_INVALID,     "Powered",           "b",     NULL,  NULL, PROP_ACCESS_RW },
    { MESSAGE_INVALID,     "Discovering",       "b",     NULL,  NULL, PROP_ACCESS_READ },
};

const InterfaceDesc bzAllJoynMgrIfcTbl[] = {
    { MESSAGE_METHOD_CALL, "SetUuid",   "s",     NULL,  NULL, 0 },
};

const InterfaceDesc bzAllJoynIfcTbl[] = {
    { MESSAGE_METHOD_CALL, "TxDataSend",   "ay",     NULL,  NULL, 0 },
    { MESSAGE_SIGNAL,      "RxDataRecv",   "ay",     NULL,  NULL, 0 },
};

const InterfaceDesc bzDevice1IfcTbl[] = {
    { MESSAGE_METHOD_CALL, "CancelPairing",       NULL, NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "Connect",             NULL, NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "ConnectProfile",      "s",  NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "Disconnect",          NULL, NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "DisconnectProfile",   "s",  NULL,    NULL, 0 },
    { MESSAGE_METHOD_CALL, "Pair",                NULL, NULL,    NULL, 0 },
    /* Properties */
    { MESSAGE_INVALID,     "UUIDs",               "as", NULL,    NULL, PROP_ACCESS_READ },
    { MESSAGE_INVALID,     "Connected",           "b",  NULL,    NULL, PROP_ACCESS_READ },
};


const InterfaceTable ifcTables[] = {
    { "org.freedesktop.DBus.ObjectManager", bzObjMgrIfcTbl, ArraySize(bzObjMgrIfcTbl) },
    { "org.bluez.Adapter1",       bzAdapter1IfcTbl,     ArraySize(bzAdapter1IfcTbl) },
    { "org.bluez.Device1",        bzDevice1IfcTbl,      ArraySize(bzDevice1IfcTbl) },
    { "org.AllSeen.AllJoynMgr",   bzAllJoynMgrIfcTbl,   ArraySize(bzAllJoynMgrIfcTbl) },
    { "org.AllSeen.AllJoyn",      bzAllJoynIfcTbl,      ArraySize(bzAllJoynIfcTbl) },
};

const size_t ifcTableSize = ArraySize(ifcTables);



} // namespace bluez
} // namespace ajn
