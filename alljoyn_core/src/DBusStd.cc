/**
 * @file
 *
 * This file provides definitions for standard DBus interfaces
 *
 */

/******************************************************************************
 * Copyright (c) 2009-2011, 2013-2014 AllSeen Alliance. All rights reserved.
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
#include <qcc/Debug.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/InterfaceDescription.h>

#define QCC_MODULE  "ALLJOYN"

namespace ajn {

/** org.freedesktop.DBus Definitions */
const char* org::freedesktop::DBus::ObjectPath = "/org/freedesktop/DBus";
const char* org::freedesktop::DBus::InterfaceName = "org.freedesktop.DBus";
const char* org::freedesktop::DBus::WellKnownName = "org.freedesktop.DBus";

const char* org::freedesktop::DBus::AnnotateNoReply = "org.freedesktop.DBus.Method.NoReply";
const char* org::freedesktop::DBus::AnnotateDeprecated = "org.freedesktop.DBus.Deprecated";
const char* org::freedesktop::DBus::AnnotateEmitsChanged = "org.freedesktop.DBus.Property.EmitsChangedSignal";

/** org.freedesktop.DBus.Properties Definitions */
const char* org::freedesktop::DBus::Properties::InterfaceName = "org.freedesktop.DBus.Properties";

/** org.freedesktop.DBus.Peer Definitions */
const char* org::freedesktop::DBus::Peer::InterfaceName = "org.freedesktop.DBus.Peer";

/** org.freedesktop.DBus.Introspectable Definitions */
const char* org::freedesktop::DBus::Introspectable::InterfaceName = "org.freedesktop.DBus.Introspectable";
const char* org::freedesktop::DBus::Introspectable::IntrospectDocType =
    "<!DOCTYPE"
    " node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://standards.freedesktop.org/dbus/introspect-1.0.dtd\""
    ">\n";


QStatus org::freedesktop::DBus::CreateInterfaces(BusAttachment& bus) {

    InterfaceDescription* intf = NULL;
    QStatus status = bus.CreateInterface(org::freedesktop::DBus::InterfaceName, intf);
    if ((ER_OK != status) || !intf) {
        if (ER_OK == status) {
            status = ER_FAIL;
        }
        QCC_LogError(status, ("Failed to create interface \"%s\"", org::freedesktop::DBus::InterfaceName));
        return status;
    }

    intf->AddMethod("Hello",                               NULL,    "s",  NULL,         0);
    intf->AddMethod("ListNames",                           NULL,    "as", "names",      0);
    intf->AddMethod("ListActivatableNames",                NULL,    "as", "names",      0);
    intf->AddMethod("RequestName",                         "su",    "u",  NULL,         0);
    intf->AddMethod("ReleaseName",                         "s",     "u",  NULL,         0);
    intf->AddMethod("NameHasOwner",                        "s",     "b",  NULL,         0);
    intf->AddMethod("StartServiceByName",                  "su",    "u",  NULL,         0);
    intf->AddMethod("GetNameOwner",                        "s",     "s",  "name,owner", 0);
    intf->AddMethod("GetConnectionUnixUser",               "s",     "u",  NULL,         0);
    intf->AddMethod("GetConnectionUnixProcessID",          "s",     "u",  NULL,         0);
    intf->AddMethod("AddMatch",                            "s",     NULL, NULL,         0);
    intf->AddMethod("RemoveMatch",                         "s",     NULL, NULL,         0);
    intf->AddMethod("GetId",                               NULL,    "s",  NULL,         0);

    intf->AddMethod("UpdateActivationEnvironment",         "a{ss}", NULL, "environment", 0);
    intf->AddMethod("ListQueuedOwners",                    "s",     "as", "name,names", 0);
    intf->AddMethod("GetAdtAuditSessionData",              "s",     "ay", NULL,         0);
    intf->AddMethod("GetConnectionSELinuxSecurityContext", "s",     "ay", NULL,         0);
    intf->AddMethod("ReloadConfig",                        NULL,    NULL, NULL,         0);

    intf->AddSignal("NameOwnerChanged",                    "sss",   NULL,               0);
    intf->AddSignal("NameLost",                            "s",     NULL,               0);
    intf->AddSignal("NameAcquired",                        "s",     NULL,               0);

    intf->Activate();

    /* Create the org.freedesktop.DBus.Introspectable interface */
    InterfaceDescription* introspectIntf = NULL;
    status = bus.CreateInterface(org::freedesktop::DBus::Introspectable::InterfaceName, introspectIntf, AJ_IFC_SECURITY_OFF);
    if ((ER_OK != status) || !introspectIntf) {
        if (ER_OK == status) {
            status = ER_FAIL;
        }
        QCC_LogError(status, ("Failed to create interface \"%s\"", org::freedesktop::DBus::Introspectable::InterfaceName));
        return status;
    }

    introspectIntf->AddMethod("Introspect",                  NULL, "s",  "data", 0);
    introspectIntf->Activate();

    /* Create the org.freedesktop.DBus.Peer interface */
    InterfaceDescription* peerIntf = NULL;
    status = bus.CreateInterface(org::freedesktop::DBus::Peer::InterfaceName, peerIntf, AJ_IFC_SECURITY_OFF);
    if ((ER_OK != status) || !peerIntf) {
        if (ER_OK == status) {
            status = ER_FAIL;
        }
        QCC_LogError(status, ("Failed to create interface \"%s\"", org::freedesktop::DBus::Peer::InterfaceName));
        return status;
    }
    peerIntf->AddMethod("Ping",         NULL, NULL, NULL, 0);
    peerIntf->AddMethod("GetMachineId", NULL, "s",  "machineid", 0);
    peerIntf->Activate();

    /* Create the org.freedesktop.DBus.Properties interface */
    InterfaceDescription* propsIntf = NULL;
    status = bus.CreateInterface(org::freedesktop::DBus::Properties::InterfaceName, propsIntf, AJ_IFC_SECURITY_OFF);
    if ((ER_OK != status) || !propsIntf) {
        if (ER_OK == status) {
            status = ER_FAIL;
        }
        QCC_LogError(status, ("Failed to create interface \"%s\"", org::freedesktop::DBus::Peer::InterfaceName));
        return status;
    }
    propsIntf->AddMethod("Get",    "ss",  "v",    "interface,propname,value", 0);
    propsIntf->AddMethod("Set",    "ssv", NULL,   "interface,propname,value", 0);
    propsIntf->AddMethod("GetAll", "s",  "a{sv}", "interface,props",          0);

    propsIntf->AddSignal("PropertiesChanged", "sa{sv}as", "interface,changed_props,invalidated_props", 0);

    propsIntf->Activate();

    return status;
}

}


