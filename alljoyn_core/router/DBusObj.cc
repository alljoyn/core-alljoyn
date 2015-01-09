/**
 * @file
 *
 * This file implements the standard DBus interfaces
 */

/******************************************************************************
 * Copyright (c) 2009-2012, 2014, AllSeen Alliance. All rights reserved.
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

#include <qcc/Debug.h>
#include <qcc/Logger.h>
#include <qcc/String.h>
#include <qcc/Util.h>

#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/Message.h>
#include <alljoyn/MessageReceiver.h>

#include "Bus.h"
#include "BusController.h"
#include "BusInternal.h"
#include "BusUtil.h"
#include "ConfigDB.h"
#include "DBusObj.h"
#include "NameTable.h"
#ifdef ENABLE_POLICYDB
#include "PolicyDB.h"
#endif

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

#define MethodHander(a) static_cast<MessageReceiver::MethodHandler>(a)

namespace ajn {

DBusObj::DBusObj(Bus& bus, BusController* busController) :
    BusObject(org::freedesktop::DBus::ObjectPath, false),
    bus(bus),
    router(reinterpret_cast<DaemonRouter&>(bus.GetInternal().GetRouter())),
    dbusIntf(NULL),
    busController(busController)
{
}

DBusObj::~DBusObj()
{
    bus.UnregisterBusObject(*this);
    router.RemoveBusNameListener(this);
}

QStatus DBusObj::Init()
{
    QStatus status;

    /* Make this object implement org.freedesktop.DBus */
    dbusIntf = bus.GetInterface(org::freedesktop::DBus::InterfaceName);
    if (!dbusIntf) {
        status = ER_BUS_NO_SUCH_INTERFACE;
        QCC_LogError(status, ("Failed to get %s interface", org::freedesktop::DBus::InterfaceName));
        return status;
    }

    /* Hook up the methods to their handlers */
    AddInterface(*dbusIntf);
    const MethodEntry methodEntries[] = {
        { dbusIntf->GetMember("ListNames"),                           MethodHander(&DBusObj::ListNames) },
        { dbusIntf->GetMember("ListActivatableNames"),                MethodHander(&DBusObj::ListActivatableNames) },
        { dbusIntf->GetMember("RequestName"),                         MethodHander(&DBusObj::RequestName) },
        { dbusIntf->GetMember("ReleaseName"),                         MethodHander(&DBusObj::ReleaseName) },
        { dbusIntf->GetMember("NameHasOwner"),                        MethodHander(&DBusObj::NameHasOwner) },
        { dbusIntf->GetMember("StartServiceByName"),                  MethodHander(&DBusObj::StartServiceByName) },
        { dbusIntf->GetMember("GetNameOwner"),                        MethodHander(&DBusObj::GetNameOwner) },
        { dbusIntf->GetMember("GetConnectionUnixUser"),               MethodHander(&DBusObj::GetConnectionUnixUser) },
        { dbusIntf->GetMember("GetConnectionUnixProcessID"),          MethodHander(&DBusObj::GetConnectionUnixProcessID) },
        { dbusIntf->GetMember("AddMatch"),                            MethodHander(&DBusObj::AddMatch) },
        { dbusIntf->GetMember("RemoveMatch"),                         MethodHander(&DBusObj::RemoveMatch) },
        { dbusIntf->GetMember("GetId"),                               MethodHander(&DBusObj::GetId) },


        { dbusIntf->GetMember("UpdateActivationEnvironment"),         MethodHander(&DBusObj::UpdateActivationEnvironment) },
        { dbusIntf->GetMember("ListQueuedOwners"),                    MethodHander(&DBusObj::ListQueuedOwners) },
        { dbusIntf->GetMember("GetAdtAuditSessionData"),              MethodHander(&DBusObj::GetAdtAuditSessionData) },
        { dbusIntf->GetMember("GetConnectionSELinuxSecurityContext"), MethodHander(&DBusObj::GetConnectionSELinuxSecurityContext) },
        { dbusIntf->GetMember("ReloadConfig"),                        MethodHander(&DBusObj::ReloadConfig) }




    };

    status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));
    if (ER_OK != status) {
        QCC_LogError(status, ("AddMethods failed"));
    }

    /* Listen for changes to the name table and register object */
    if (ER_OK == status) {
        router.AddBusNameListener(this);
        status = bus.RegisterBusObject(*this);
    }

    return status;
}

void DBusObj::ObjectRegistered()
{
    /* Acquire org.freedesktop.DBus name (locally) */
    uint32_t disposition = DBUS_REQUEST_NAME_REPLY_EXISTS;
    QStatus status = router.AddAlias(org::freedesktop::DBus::WellKnownName,
                                     bus.GetInternal().GetLocalEndpoint()->GetUniqueName(),
                                     DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                     disposition,
                                     NULL,
                                     NULL);
    if ((ER_OK != status) || (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != disposition)) {
        status = (ER_OK == status) ? ER_FAIL : status;
        QCC_LogError(status, ("Failed to register well-known name \"%s\" (disposition=%d)", org::freedesktop::DBus::WellKnownName, disposition));
    }

    if (status == ER_OK) {
        BusObject::ObjectRegistered();
        busController->ObjectRegistered(this);
    }
}


void DBusObj::ListNames(const InterfaceDescription::Member* member, Message& msg)
{
    /* Get the name list */
    vector<qcc::String> namesVec;
    router.GetBusNames(namesVec);

    /* Send the response */
    size_t numNames = namesVec.size();
    MsgArg* names = new MsgArg[numNames];
    vector<qcc::String>::const_iterator it = namesVec.begin();
    size_t i = 0;
    while (it != namesVec.end()) {
        names[i].typeId = ALLJOYN_STRING;
        names[i].v_string.str = it->c_str();
        names[i].v_string.len = it->size();
        ++it;
        ++i;
    }
    MsgArg namesArray(ALLJOYN_ARRAY);
    namesArray.v_array.SetElements("s", numNames, names);

    QStatus status = MethodReply(msg, &namesArray, 1);
    if (ER_OK != status) {
        QCC_LogError(status, ("DBusObj::ListNames failed"));
    }
}

void DBusObj::ListActivatableNames(const InterfaceDescription::Member* member, Message& msg)
{
    MsgArg namesArray(ALLJOYN_ARRAY);
    namesArray.v_array.SetElements("s", 0, NULL);

    QStatus status = MethodReply(msg, &namesArray, 1);
    if (ER_OK != status) {
        QCC_LogError(status, ("DBusObj::ListActivatableNames failed"));
    }
}

void DBusObj::NameHasOwner(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    MsgArg boolArg = MsgArg(ALLJOYN_BOOLEAN);

    /* Get the name */
    const MsgArg* nameArg = msg->GetArg(0);
    assert(nameArg && (ALLJOYN_STRING == nameArg->typeId));

    /* Find name */
    boolArg.v_bool = router.FindEndpoint(nameArg->v_string.str)->IsValid();

    /* Send the response */
    status = MethodReply(msg, &boolArg, 1);

    if (ER_OK != status) {
        QCC_LogError(status, ("DBusObj::NameHasOwner failed"));
    }
}


void DBusObj::RequestName(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    void* context = (void*) &msg;

    const char* nameArg = msg->GetArg(0)->v_string.str;
    const uint32_t flagsArg = msg->GetArg(1)->v_uint32;
#ifdef ENABLE_POLICYDB
    PolicyDB policyDB = ConfigDB::GetConfigDB()->GetPolicyDB();
    String sender = msg->GetSender();
    BusEndpoint ep = router.FindEndpoint(sender);

    if (*nameArg != ':' && IsLegalBusName(nameArg) && policyDB->OKToOwn(nameArg, ep)) {
#else
    if (*nameArg != ':' && IsLegalBusName(nameArg)) {
#endif
        /* Attempt to add the alias */
        /* Response will be handled in AddAliasCB */
        uint32_t disposition;
        status = router.AddAlias(nameArg, msg->GetSender(), flagsArg, disposition, this, context);
        if (ER_OK != status) {
            QCC_LogError(status, ("Router::AddAlias failed"));
            MethodReply(msg, "FAILURE");
        }
    } else {
        qcc::String errMsg("\"");
        errMsg += "Request for invalid busname, \"";
        errMsg += nameArg;
        errMsg += "\", not allowed.";
        MethodReply(msg, "org.freedesktop.DBus.Error.InvalidArgs", errMsg.c_str());
    }
}

void DBusObj::ReleaseName(const InterfaceDescription::Member* member, Message& msg)
{
    void* context = (void*) &msg;

    const MsgArg* nameArg = msg->GetArg(0);
    assert(nameArg && (ALLJOYN_STRING == nameArg->typeId));

    /* Attempt to remove the alias */
    uint32_t disposition;
    router.RemoveAlias(nameArg->v_string.str, msg->GetSender(), disposition, this, context);
}

void DBusObj::StartServiceByName(const InterfaceDescription::Member* member, Message& msg)
{
    qcc::String description("Unable to start service: ");
    description += msg->GetDestination();
    description += "(";
    description += QCC_StatusText(ER_NOT_IMPLEMENTED);
    description += ")";
    MethodReply(msg, "org.freedesktop.DBus.Error.Spawn.Failed", description.c_str());
}

void DBusObj::GetNameOwner(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    const MsgArg* nameArg = msg->GetArg(0);

    BusEndpoint ep = router.FindEndpoint(nameArg->v_string.str);
    if (!ep->IsValid()) {
        status = MethodReply(msg, "org.freedesktop.DBus.Error.NameHasNoOwner");
    } else {
        MsgArg replyArg(ALLJOYN_STRING);
        const qcc::String& uniqueName = ep->GetUniqueName();
        replyArg.v_string.str = uniqueName.c_str();
        replyArg.v_string.len = uniqueName.size();
        status = MethodReply(msg, &replyArg, 1);
    }

    /* Log any error */
    if (ER_OK != status) {
        QCC_LogError(status, ("DBusObj::GetNameOwner failed"));
    }
}

void DBusObj::GetConnectionUnixUser(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    const MsgArg* nameArg = msg->GetArg(0);

    BusEndpoint ep = router.FindEndpoint(nameArg->v_string.str);
    if (!ep->IsValid()) {
        status = MethodReply(msg, "org.freedesktop.DBus.Error.NameHasNoOwner");
    } else {
        if (ep->SupportsUnixIDs()) {
            MsgArg rsp("u", ep->GetUserId());
            status = MethodReply(msg, &rsp, 1);
        } else {
            status = MethodReply(msg, "org.freedestop.DBus.Error.Failed");
        }
    }

    /* Log any error */
    if (ER_OK != status) {
        QCC_LogError(status, ("DBusObj::GetConnectionUnixUser failed"));
    }
}

void DBusObj::GetConnectionUnixProcessID(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    const MsgArg* nameArg = msg->GetArg(0);

    BusEndpoint ep = router.FindEndpoint(nameArg->v_string.str);
    if (!ep->IsValid()) {
        status = MethodReply(msg, "org.freedesktop.DBus.Error.NameHasNoOwner");
    } else {
        if (ep->SupportsUnixIDs()) {
            MsgArg rsp("u", ep->GetProcessId());
            status = MethodReply(msg, &rsp, 1);
        } else {
            status = MethodReply(msg, "org.freedestop.DBus.Error.Failed");
        }
    }

    /* Log any error */
    if (ER_OK != status) {
        QCC_LogError(status, ("DBusObj::GetConnectionUnixProcessID failed"));
    }
}

void DBusObj::AddMatch(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    const MsgArg* nameArg = msg->GetArg(0);

    assert(nameArg && (nameArg->typeId == ALLJOYN_STRING));

    Rule rule(nameArg->v_string.str, &status);
    if (ER_OK == status) {
        BusEndpoint ep = router.FindEndpoint(msg->GetSender());
        if (ep->IsValid()) {
            status = router.AddRule(ep, rule);
        } else {
            status = ER_BUS_NO_ENDPOINT;
        }
    }
    if (ER_OK == status) {
        status = MethodReply(msg, (const MsgArg*) NULL, 0);
    } else {
        QCC_LogError(status, ("AddMatch failed"));
        status = MethodReply(msg, "org.freedesktop.DBus.Error.OOM", QCC_StatusText(status));
    }
}

void DBusObj::RemoveMatch(const InterfaceDescription::Member* member, Message& msg)
{
    QStatus status;
    const MsgArg* nameArg = msg->GetArg(0);

    assert(nameArg && (nameArg->typeId == ALLJOYN_STRING));

    Rule rule(nameArg->v_string.str, &status);
    if (ER_OK == status) {
        BusEndpoint ep = router.FindEndpoint(msg->GetSender());
        if (ep->IsValid()) {
            status = router.RemoveRule(ep, rule);
        } else {
            status = ER_BUS_NO_ENDPOINT;
        }
    }
    if (ER_OK == status) {
        status = MethodReply(msg, (const MsgArg*)NULL, 0);
    } else {
        QCC_LogError(status, ("RemoveMatch failed"));
        status = MethodReply(msg, "org.freedesktop.DBus.Error.MatchRuleNotFound", QCC_StatusText(status));
    }
}

void DBusObj::GetId(const InterfaceDescription::Member* member, Message& msg)
{
    MsgArg replyArg(ALLJOYN_STRING);

    const qcc::String& guid = bus.GetInternal().GetGlobalGUID().ToString();
    replyArg.v_string.str = guid.c_str();
    replyArg.v_string.len = guid.size();

    QStatus status = MethodReply(msg, &replyArg, 1);

    if (ER_OK != status) {
        QCC_LogError(status, ("GetId failed"));
    }
}

void DBusObj::UpdateActivationEnvironment(const InterfaceDescription::Member* member, Message& msg)
{
    // TODO: Implement me.
    QStatus status = MethodReply(msg, "org.freedesktop.DBus.Error.NotSupported", NULL);
    if (ER_OK != status) {
        QCC_LogError(status, ("Reply failed"));
    }
}

void DBusObj::ListQueuedOwners(const InterfaceDescription::Member* member, Message& msg)
{
    const MsgArg* nameArg = msg->GetArg(0);
    assert(nameArg && (ALLJOYN_STRING == nameArg->typeId));

    vector<qcc::String> namesVec;
    router.GetQueuedNames(nameArg->v_string.str, namesVec);

    /* Send the response */
    /*
     * The first name in the list returned by the GetQueuedNames is the primary
     * owner.  ListQueuedNames returns a list of queued secondary owners.
     * The messaged returned by org.freedesktop.DBus.ListQueuedOwners should
     * only send a list of secondary owners.
     */
    size_t numNames = namesVec.size();
    if (numNames > 0) {
        --numNames;
    }
    MsgArg* names = new MsgArg[numNames];
    vector<qcc::String>::const_iterator it = namesVec.begin();
    size_t i = 0;
    if (it != namesVec.end()) {
        ++it; //skip the first Queued Name it is the primary owner.
        while (it != namesVec.end()) {
            names[i].typeId = ALLJOYN_STRING;
            names[i].v_string.str = it->c_str();
            names[i].v_string.len = it->size();
            ++it;
            ++i;
        }
    }
    MsgArg namesArray(ALLJOYN_ARRAY);
    namesArray.v_array.SetElements("s", numNames, names);
    /* Call Stabilize so that the names array can be deleted */
    namesArray.Stabilize();
    delete [] names;
    QStatus status = MethodReply(msg, &namesArray, 1);
    if (ER_OK != status) {
        QCC_LogError(status, ("DBusObj::ListQueuedOwners failed"));
    }
}

void DBusObj::GetAdtAuditSessionData(const InterfaceDescription::Member* member, Message& msg)
{
    // TODO: Implement me.
    QStatus status = MethodReply(msg, "org.freedesktop.DBus.Error.NotSupported", NULL);
    if (ER_OK != status) {
        QCC_LogError(status, ("Reply failed"));
    }
}

void DBusObj::GetConnectionSELinuxSecurityContext(const InterfaceDescription::Member* member, Message& msg)
{
    // TODO: Implement me.
    QStatus status = MethodReply(msg, "org.freedesktop.DBus.Error.NotSupported", NULL);
    if (ER_OK != status) {
        QCC_LogError(status, ("Reply failed"));
    }
}

void DBusObj::ReloadConfig(const InterfaceDescription::Member* member, Message& msg)
{
    MethodReply(msg, "org.freedesktop.DBus.Error.Failed");
}

void DBusObj::AddAliasComplete(const qcc::String& aliasName, uint32_t disposition, void* context)
{
    assert(context);
    Message* msg = (Message*) context;
    MsgArg replyArg(ALLJOYN_UINT32);
    replyArg.v_uint32 = (uint32_t) disposition;
    QStatus status = MethodReply(*msg, &replyArg, 1);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to send RequestName reply"));
    }
}

void DBusObj::RemoveAliasComplete(const qcc::String& aliasName,
                                  uint32_t disposition,
                                  void* context)
{
    assert(context);
    Message* msg = (Message*) context;
    MsgArg replyArg(ALLJOYN_UINT32);
    replyArg.v_uint32 = (uint32_t)disposition;
    QStatus status = MethodReply(*msg, &replyArg, 1);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to send ReleaseName reply"));
    }
}

void DBusObj::NameOwnerChanged(const qcc::String& alias,
                               const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                               const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer)
{
    QStatus status;
    const qcc::String& shortGuidStr = bus.GetInternal().GetGlobalGUID().ToShortString();

    /* Silently ignore addition of reserved bus names */
    if (!dbusIntf
        || (0 == strcmp(alias.c_str(), org::alljoyn::Bus::WellKnownName))
        || (0 == strcmp(alias.c_str(), org::freedesktop::DBus::WellKnownName))) {
        return;
    }

    MsgArg aliasArg(ALLJOYN_STRING);
    aliasArg.v_string.str = alias.c_str();
    aliasArg.v_string.len = alias.size();

    /* When newOwner and oldOwner are the same, only the name transfer changed. */
    if (newOwner != oldOwner) {

        /* Send a NameLost signal if necessary */
        /* Don't send lost signal for a lost unique name since the endpoint is already gone */
        if (oldOwner && !oldOwner->empty() && (alias[0] != ':') && (0 == ::strncmp(oldOwner->c_str() + 1,
                                                                                   shortGuidStr.c_str(),
                                                                                   shortGuidStr.size()))) {
            const InterfaceDescription::Member* nameLost = dbusIntf->GetMember("NameLost");
            assert(nameLost);
            status = Signal(oldOwner->c_str(), 0, *nameLost, &aliasArg, 1);
            if (ER_OK != status) {
                QCC_DbgPrintf(("Failed to send NameLost signal for %s to %s (%s)", alias.c_str(), oldOwner->c_str(), QCC_StatusText(status)));
            }
        }

        /* Send a NameAcquired signal if necessary */
        if (newOwner && !newOwner->empty() && (0 == ::strncmp(newOwner->c_str() + 1, shortGuidStr.c_str(),
                                                              shortGuidStr.size()))) {
            const InterfaceDescription::Member* nameAcquired = dbusIntf->GetMember("NameAcquired");
            assert(nameAcquired);
            status = Signal(newOwner->c_str(), 0, *nameAcquired, &aliasArg, 1);
            if (ER_OK != status) {
                QCC_DbgPrintf(("Failed to send NameAcquired signal for %s to %s (%s)", alias.c_str(), newOwner->c_str(), QCC_StatusText(status)));
            }
        }
    }

    /* Send NameOwnerChanged signal */
    if ((oldOwner && SessionOpts::ALL_NAMES == oldOwnerNameTransfer) ||
        (newOwner && SessionOpts::ALL_NAMES == newOwnerNameTransfer)) {
        MsgArg ownerChangedArgs[3];
        size_t numArgs = ArraySize(ownerChangedArgs);
        MsgArg::Set(ownerChangedArgs, numArgs, "sss",
                    alias.c_str(),
                    (oldOwner && SessionOpts::ALL_NAMES == oldOwnerNameTransfer) ? oldOwner->c_str() : "",
                    (newOwner && SessionOpts::ALL_NAMES == newOwnerNameTransfer) ? newOwner->c_str() : "");

        const InterfaceDescription::Member* nameOwnerChanged = dbusIntf->GetMember("NameOwnerChanged");
        assert(nameOwnerChanged);
        status = Signal(NULL, 0, *nameOwnerChanged, ownerChangedArgs, numArgs);
        if (status != ER_OK) {
            QCC_DbgPrintf(("Failed to send NameOwnerChanged signal for %s to %s (%s)", alias.c_str(), newOwner ? newOwner->c_str() : "", QCC_StatusText(status)));
        }
    }
}


}
