/**
 * @file
 * Implementation of Android P2P Helper Interface class
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <alljoyn/Status.h>

#include <qcc/platform.h>
#include <qcc/Util.h>
#include <qcc/Debug.h>
#include <qcc/String.h>

#include <alljoyn/DBusStd.h>

#include "P2PHelperInterface.h"

#define QCC_MODULE "P2P_HELPER_INTERFACE"

using namespace ajn;

const char* P2PHelperInterface::INTERFACE_NAME = "org.alljoyn.bus.p2p.P2pInterface";
const char* P2PHelperInterface::WELL_KNOWN_NAME = "org.alljoyn.bus.p2p";
const char* P2PHelperInterface::OBJECT_PATH = "/P2pService";

P2PHelperInterface::P2PHelperInterface()
    : m_dbusProxyBusObject(0), m_proxyBusObject(0), m_interface(0), m_bus(0), m_listener(0), m_listenerInternal(0)
{
    QCC_DbgPrintf(("P2PHelperInterface::P2PHelperInterface()"));
}

P2PHelperInterface::~P2PHelperInterface()
{
    QCC_DbgPrintf(("P2PHelperInterface::~P2PHelperInterface()"));
    UnregisterSignalHandlers();
    delete m_proxyBusObject;
    delete m_listenerInternal;
}

bool P2PHelperInterface::ServiceExists(void)
{
    QCC_DbgPrintf(("P2PHelperInterface::ServiceExists()"));

    const InterfaceDescription* dbusInterface(m_bus->GetInterface(ajn::org::freedesktop::DBus::InterfaceName));
    if (dbusInterface == NULL) {
        QStatus status = ER_BUS_UNKNOWN_INTERFACE;
        QCC_LogError(status, ("P2PHelperInterface::ServiceExists(): DBus interface does not exist on the bus"));
        return status;
    }

    MsgArg arg;
    Message reply(*m_bus);

    arg.Set("s", "org.alljoyn.bus.p2p");
    const InterfaceDescription::Member* nameHasOwner = dbusInterface->GetMember("NameHasOwner");
    QStatus status = m_dbusProxyBusObject->MethodCall(*nameHasOwner, &arg, 1, reply);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::ServiceExists(): Cannot call NameHasOwner"));
        return status;
    }

    bool boolean = false;
    status = reply->GetArgs("b", &boolean);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::ServiceExists(): Cannot GetArgs()"));
        return status;
    }

    QCC_DbgPrintf(("P2PHelperInterface::ServiceExists(): Service does %sexist.", boolean ? "" : "not "));

    return boolean;
}

QStatus P2PHelperInterface::Init(BusAttachment* bus)
{
    QCC_DbgPrintf(("P2PHelperInterface::Init(%p)", bus));

    if (bus == NULL) {
        QCC_LogError(ER_FAIL, ("P2PHelperInterface::Init(): Invalid BusAttachment provided"));
        return ER_FAIL;
    }

    assert(m_bus == NULL && "P2PHelperInterface::Init(): Duplicate calls to Init are forbidden");
    m_bus = bus;

    QCC_DbgPrintf(("P2PHelperInterface::Init(): GetDBusProxyObj()"));
    m_dbusProxyBusObject = &(bus->GetDBusProxyObj());
    assert(m_dbusProxyBusObject && "P2PHelperInterface::Init(): Could not GetDBusProxyObj()");

    QCC_DbgPrintf(("P2PHelperInterface::Init(): new P2PHelperListenerInternal()"));
    m_listenerInternal = new P2PHelperListenerInternal(this);

    QCC_DbgPrintf(("P2PHelperInterface::Init(): CreateInterface()"));
    assert(m_interface == NULL && "P2PHelperInterface::Init(): m_interface not NULL");

    //
    // See if one of our counterparts has already created the interface used to
    // talk to the P2P helper service.  If it is there, use it; otherwise we
    // need to make it for ourselves.  The rule is that once an interface is
    // created, you cannot change it, so GetInterface() returns a pointer to a
    // const InterfaceDescription.  But we need to be able to create the
    // interface if it is not there, which means changing the interface
    // description, which means it can't be const.  We cast away the constness
    // when we look for the interface description, but play by the rules and
    // never change it later
    //
    m_interface = const_cast<InterfaceDescription*>(m_bus->GetInterface(INTERFACE_NAME));
    if (m_interface == NULL) {
        QStatus status = m_bus->CreateInterface(INTERFACE_NAME, m_interface);
        if (status == ER_OK) {
            m_interface->AddMethod("FindAdvertisedName",         "s",  "i",  "namePrefix,result");
            m_interface->AddMethod("CancelFindAdvertisedName",   "s",  "i",  "namePrefix,result");
            m_interface->AddMethod("AdvertiseName",              "ss", "i",  "name,guid,result");
            m_interface->AddMethod("CancelAdvertiseName",        "ss", "i",  "name,guid,result");
            m_interface->AddMethod("EstablishLink",              "si", "i",  "device,intent,result");
            m_interface->AddMethod("ReleaseLink",                "i",  "i",  "handle,result");
            m_interface->AddMethod("GetInterfaceNameFromHandle", "i",  "s",  "handle,interface");

            m_interface->AddSignal("OnFoundAdvertisedName",      "ssss", "name,namePrefix,guid,device");
            m_interface->AddSignal("OnLostAdvertisedName",       "ssss", "name,namePrefix,guid,device");
            m_interface->AddSignal("OnLinkEstablished",          "is",   "handle,interface");
            m_interface->AddSignal("OnLinkError",                "ii",   "handle,error");
            m_interface->AddSignal("OnLinkLost",                 "i",    "handle");

            QCC_DbgPrintf(("P2PHelperInterface::Init(): Activate()"));
            m_interface->Activate();
        } else {
            QCC_LogError(status, ("P2PHelperInterface::Init(): Error creating interface"));
            return status;
        }
    }

    assert(m_interface && "P2PHelperInterface::Init(): Required m_interface unexpectedly NULL");

    QCC_DbgPrintf(("P2PHelperInterface::Init(): new ProxyBusObject()"));
    m_proxyBusObject = new ProxyBusObject(*bus, WELL_KNOWN_NAME, OBJECT_PATH, 0);

    QCC_DbgPrintf(("P2PHelperInterface::Init(): AddInterface()"));
    m_proxyBusObject->AddInterface(*m_interface);

    RegisterSignalHandlers();

    QCC_DbgPrintf(("P2PHelperInterface::Init(): AddMatch() \"%s\"", INTERFACE_NAME));
    qcc::String rule = "type='signal',interface='" + qcc::String(INTERFACE_NAME) + "'";

    MsgArg arg;
    arg.Set("s", rule.c_str());
    Message reply(*m_bus);

    const InterfaceDescription* dbusInterface(m_bus->GetInterface(ajn::org::freedesktop::DBus::InterfaceName));
    const InterfaceDescription::Member* addMatch = dbusInterface->GetMember("AddMatch");

    QStatus status = m_dbusProxyBusObject->MethodCallAsync(*addMatch, this, static_cast<MessageReceiver::ReplyHandler>(&P2PHelperInterface::HandleAddMatchReply), &arg, 1, NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::Init(): Error calling MethodCallAsync()"));
        return status;
    }

    return ER_OK;
}

void P2PHelperInterface::HandleAddMatchReply(Message& message, void* context)
{
    QCC_DbgPrintf(("P2PHelperInterface::HandleAddMatchReply()"));
}

QStatus P2PHelperInterface::UnregisterSignalHandlers(void)
{
    const InterfaceDescription::Member* member;
    QStatus status;

    member = m_interface->GetMember("OnFoundAdvertisedName");
    if (member) {
        status =  m_bus->UnregisterSignalHandler(m_listenerInternal,
                                                 static_cast<MessageReceiver::SignalHandler>(&P2PHelperListenerInternal::OnFoundAdvertisedName),
                                                 member,
                                                 NULL);
        if (status != ER_OK) {
            QCC_LogError(status, ("P2PHelperInterface::UnregisterSignalHandlers(): Error calling UnregisterSignalHandler()"));
            return status;
        }
    }

    member = m_interface->GetMember("OnLostAdvertisedName");
    if (member) {
        status =  m_bus->UnregisterSignalHandler(m_listenerInternal,
                                                 static_cast<MessageReceiver::SignalHandler>(&P2PHelperListenerInternal::OnLostAdvertisedName),
                                                 member,
                                                 NULL);
        if (status != ER_OK) {
            QCC_LogError(status, ("P2PHelperInterface::UnregisterSignalHandlers(): Error calling UnregisterSignalHandler()"));
            return status;
        }
    }

    member = m_interface->GetMember("OnLinkEstablished");
    if (member) {
        status =  m_bus->UnregisterSignalHandler(m_listenerInternal,
                                                 static_cast<MessageReceiver::SignalHandler>(&P2PHelperListenerInternal::OnLinkEstablished),
                                                 member,
                                                 NULL);
        if (status != ER_OK) {
            QCC_LogError(status, ("P2PHelperInterface::UnregisterSignalHandlers(): Error calling UnregisterSignalHandler()"));
            return status;
        }
    }

    member = m_interface->GetMember("OnLinkError");
    if (member) {
        status =  m_bus->UnregisterSignalHandler(m_listenerInternal,
                                                 static_cast<MessageReceiver::SignalHandler>(&P2PHelperListenerInternal::OnLinkError),
                                                 member,
                                                 NULL);
        if (status != ER_OK) {
            QCC_LogError(status, ("P2PHelperInterface::UnregisterSignalHandlers(): Error calling UnregisterSignalHandler()"));
            return status;
        }
    }

    member = m_interface->GetMember("OnLinkLost");
    if (member) {
        status =  m_bus->UnregisterSignalHandler(m_listenerInternal,
                                                 static_cast<MessageReceiver::SignalHandler>(&P2PHelperListenerInternal::OnLinkLost),
                                                 member,
                                                 NULL);
        if (status != ER_OK) {
            QCC_LogError(status, ("P2PHelperInterface::UnregisterSignalHandlers(): Error calling UnregisterSignalHandler()"));
            return status;
        }
    }

    return ER_OK;
}

QStatus P2PHelperInterface::RegisterSignalHandlers(void)
{
    const InterfaceDescription::Member* member;

    member = m_interface->GetMember("OnFoundAdvertisedName");
    assert(member && "P2PHelperInterface::Init(): GetMember(\"OnFoundAdvertisedName\") failed");
    QStatus status =  m_bus->RegisterSignalHandler(m_listenerInternal,
                                                   static_cast<MessageReceiver::SignalHandler>(&P2PHelperListenerInternal::OnFoundAdvertisedName),
                                                   member,
                                                   NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::RegisterSignalHandler(): Error calling RegisterSignalHandler()"));
        assert(0);
        return status;
    }

    member = m_interface->GetMember("OnLostAdvertisedName");
    assert(member && "P2PHelperInterface::Init(): GetMember(\"OnLostAdvertisedName\") failed");
    status =  m_bus->RegisterSignalHandler(m_listenerInternal,
                                           static_cast<MessageReceiver::SignalHandler>(&P2PHelperListenerInternal::OnLostAdvertisedName),
                                           member,
                                           NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::RegisterSignalHandlers(): Error calling RegisterSignalHandler()"));
        assert(0);
        return status;
    }

    member = m_interface->GetMember("OnLinkEstablished");
    assert(member && "P2PHelperInterface::Init(): GetMember(\"OnLinkEstablished\") failed");
    status =  m_bus->RegisterSignalHandler(m_listenerInternal,
                                           static_cast<MessageReceiver::SignalHandler>(&P2PHelperListenerInternal::OnLinkEstablished),
                                           member,
                                           NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::RegisterSignalHandlers(): Error calling RegisterSignalHandler()"));
        assert(0);
        return status;
    }

    member = m_interface->GetMember("OnLinkError");
    assert(member && "P2PHelperInterface::Init(): GetMember(\"OnLinkError\") failed");
    status =  m_bus->RegisterSignalHandler(m_listenerInternal,
                                           static_cast<MessageReceiver::SignalHandler>(&P2PHelperListenerInternal::OnLinkError),
                                           member,
                                           NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::RegisterSignalHandlers(): Error calling RegisterSignalHandler()"));
        assert(0);
        return status;
    }

    member = m_interface->GetMember("OnLinkLost");
    assert(member && "P2PHelperInterface::Init(): GetMember(\"OnLinkLost\") failed");
    status =  m_bus->RegisterSignalHandler(m_listenerInternal,
                                           static_cast<MessageReceiver::SignalHandler>(&P2PHelperListenerInternal::OnLinkLost),
                                           member,
                                           NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::RegisterSignalHandlers(): Error calling RegisterSignalHandler()"));
        assert(0);
        return status;
    }

    return ER_OK;
}

void P2PHelperInterface::SetListener(P2PHelperListener* listener)
{
    QCC_DbgPrintf(("P2PHelperInterface::SetListener(%p)", listener));
    m_listener = listener;
}

QStatus P2PHelperInterface::FindAdvertisedName(const qcc::String& namePrefix, int32_t& result) const
{
    QCC_DbgPrintf(("P2PHelperInterface::FindAdvertisedName()"));

    const InterfaceDescription::Member* findAdvertisedName = m_interface->GetMember("FindAdvertisedName");

    MsgArg arg;
    arg.Set("s", namePrefix.c_str());
    Message reply(*m_bus);

    QStatus status = m_proxyBusObject->MethodCall(*findAdvertisedName, &arg, 1, reply);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::FindAdvertisedName(): MethodCall() failed"));
        return status;
    }

    status = reply->GetArgs("i", &result);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::FindAdvertisedName(): GetArgs() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::FindAdvertisedNameAsync(const qcc::String& namePrefix) const
{
    QCC_DbgPrintf(("P2PHelperInterface::FindAdvertisedNameAsync()"));

    const InterfaceDescription::Member* findAdvertisedName = m_interface->GetMember("FindAdvertisedName");

    MsgArg arg;
    arg.Set("s", namePrefix.c_str());

    QStatus status = m_proxyBusObject->MethodCallAsync(*findAdvertisedName, m_listenerInternal,
                                                       static_cast<MessageReceiver::ReplyHandler>(&P2PHelperInterface::P2PHelperListenerInternal::HandleFindAdvertisedNameReply),
                                                       &arg, 1, NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::FindAdvertisedNameAsync(): MethodCallAsync() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::CancelFindAdvertisedName(const qcc::String& namePrefix, int32_t& result) const
{
    QCC_DbgPrintf(("P2PHelperInterface::CancelFindAdvertisedName()"));

    const InterfaceDescription::Member* cancelFindAdvertisedName = m_interface->GetMember("CancelFindAdvertisedName");

    MsgArg arg;
    arg.Set("s", namePrefix.c_str());
    Message reply(*m_bus);

    QStatus status = m_proxyBusObject->MethodCall(*cancelFindAdvertisedName, &arg, 1, reply);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::CancelFindAdvertisedName(): MethodCall() failed"));
        return status;
    }

    status = reply->GetArgs("i", &result);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::CancelFindAdvertisedName(): GetArgs() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::CancelFindAdvertisedNameAsync(const qcc::String& namePrefix) const
{
    QCC_DbgPrintf(("P2PHelperInterface::CancelFindAdvertisedNameAsync()"));

    const InterfaceDescription::Member* cancelFindAdvertisedName = m_interface->GetMember("CancelFindAdvertisedName");

    MsgArg arg;
    arg.Set("s", namePrefix.c_str());

    QStatus status = m_proxyBusObject->MethodCallAsync(*cancelFindAdvertisedName, m_listenerInternal,
                                                       static_cast<MessageReceiver::ReplyHandler>(&P2PHelperInterface::P2PHelperListenerInternal::HandleCancelFindAdvertisedNameReply),
                                                       &arg, 1, NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::FindAdvertisedNameAsync(): MethodCallAsync() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::AdvertiseName(const qcc::String& namePrefix, const qcc::String& guid, int32_t& result) const
{
    QCC_DbgPrintf(("P2PHelperInterface::AdvertiseName()"));

    const InterfaceDescription::Member* advertiseName = m_interface->GetMember("AdvertiseName");

    MsgArg args[2];
    args[0].Set("s", namePrefix.c_str());
    args[1].Set("s", guid.c_str());
    Message reply(*m_bus);

    QStatus status = m_proxyBusObject->MethodCall(*advertiseName, args, ArraySize(args), reply);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::AdvertiseName(): MethodCall() failed"));
        return status;
    }

    status = reply->GetArgs("i", &result);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::AdvertiseName(): GetArgs() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::AdvertiseNameAsync(const qcc::String& namePrefix, const qcc::String& guid) const
{
    QCC_DbgPrintf(("P2PHelperInterface::AdvertiseNameAsync()"));

    const InterfaceDescription::Member* advertiseName = m_interface->GetMember("AdvertiseName");

    MsgArg args[2];
    args[0].Set("s", namePrefix.c_str());
    args[1].Set("s", guid.c_str());

    QStatus status = m_proxyBusObject->MethodCallAsync(*advertiseName, m_listenerInternal,
                                                       static_cast<MessageReceiver::ReplyHandler>(&P2PHelperInterface::P2PHelperListenerInternal::HandleAdvertiseNameReply),
                                                       args, ArraySize(args), NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::AdvertiseNameAsync(): MethodCallAsync() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::CancelAdvertiseName(const qcc::String& namePrefix, const qcc::String& guid, int32_t& result) const
{
    QCC_DbgPrintf(("P2PHelperInterface::CancelAdvertiseName()"));

    const InterfaceDescription::Member* cancelAdvertiseName = m_interface->GetMember("CancelAdvertiseName");

    MsgArg args[2];
    args[0].Set("s", namePrefix.c_str());
    args[1].Set("s", guid.c_str());
    Message reply(*m_bus);

    QStatus status = m_proxyBusObject->MethodCall(*cancelAdvertiseName, args, ArraySize(args), reply);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::CancelAdvertiseName(): MethodCall() failed"));
        return status;
    }

    status = reply->GetArgs("i", &result);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::CancelAdvertiseName(): GetArgs() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::CancelAdvertiseNameAsync(const qcc::String& namePrefix, const qcc::String& guid) const
{
    QCC_DbgPrintf(("P2PHelperInterface::CancelAdvertiseNameAsync()"));

    const InterfaceDescription::Member* cancelAdvertiseName = m_interface->GetMember("CancelAdvertiseName");

    MsgArg args[2];
    args[0].Set("s", namePrefix.c_str());
    args[1].Set("s", guid.c_str());

    QStatus status = m_proxyBusObject->MethodCallAsync(*cancelAdvertiseName, m_listenerInternal,
                                                       static_cast<MessageReceiver::ReplyHandler>(&P2PHelperInterface::P2PHelperListenerInternal::HandleCancelAdvertiseNameReply),
                                                       args, ArraySize(args), NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::CancelAdvertiseNameAsync(): MethodCallAsync() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::EstablishLink(const qcc::String& device, uint32_t intent, int32_t& handle) const
{
    QCC_DbgPrintf(("P2PHelperInterface::EstablishLink()"));

    const InterfaceDescription::Member* establishLink = m_interface->GetMember("EstablishLink");

    MsgArg args[2];
    args[0].Set("s", device.c_str());
    args[1].Set("i", intent);
    Message reply(*m_bus);

    QStatus status = m_proxyBusObject->MethodCall(*establishLink, args, ArraySize(args), reply);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::EstablishLink(): MethodCall() failed"));
        return status;
    }

    status = reply->GetArgs("i", &handle);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::EstablishLink(): GetArgs() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::EstablishLinkAsync(const qcc::String& device, uint32_t intent) const
{
    QCC_DbgPrintf(("P2PHelperInterface::EstablishLinkAsync()"));

    const InterfaceDescription::Member* establishLink = m_interface->GetMember("EstablishLink");

    MsgArg args[2];
    args[0].Set("s", device.c_str());
    args[1].Set("i", intent);

    QStatus status = m_proxyBusObject->MethodCallAsync(*establishLink, m_listenerInternal,
                                                       static_cast<MessageReceiver::ReplyHandler>(&P2PHelperInterface::P2PHelperListenerInternal::HandleEstablishLinkReply),
                                                       args, ArraySize(args), NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::EstablishLinkAsync(): MethodCallAsync() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::ReleaseLink(int32_t handle, int32_t& result) const
{
    QCC_DbgPrintf(("P2PHelperInterface::ReleaseLink()"));

    const InterfaceDescription::Member* releaseLink = m_interface->GetMember("ReleaseLink");

    MsgArg arg;
    arg.Set("i", handle);
    Message reply(*m_bus);

    QStatus status = m_proxyBusObject->MethodCall(*releaseLink, &arg, 1, reply);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::ReleaseLink(): MethodCall() failed"));
        return status;
    }

    status = reply->GetArgs("i", &result);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::ReleaseLink(): GetArgs() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::ReleaseLinkAsync(int32_t handle) const
{
    QCC_DbgPrintf(("P2PHelperInterface::ReleaseLinkAsync()"));

    const InterfaceDescription::Member* releaseLink = m_interface->GetMember("ReleaseLink");

    MsgArg arg;
    arg.Set("i", handle);

    QStatus status = m_proxyBusObject->MethodCallAsync(*releaseLink, m_listenerInternal,
                                                       static_cast<MessageReceiver::ReplyHandler>(&P2PHelperInterface::P2PHelperListenerInternal::HandleReleaseLinkReply),
                                                       &arg, 1, NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::ReleaseLinkAsync(): MethodCallAsync() failed"));
    }

    return status;
}

QStatus P2PHelperInterface::GetInterfaceNameFromHandle(int32_t handle, qcc::String& interface) const
{
    QCC_DbgPrintf(("P2PHelperInterface::GetInterfaceNamefromHandle()"));

    const InterfaceDescription::Member* getInterfaceNameFromHandle = m_interface->GetMember("GetInterfaceNameFromHandle");

    MsgArg arg;
    arg.Set("i", handle);
    Message reply(*m_bus);

    QStatus status = m_proxyBusObject->MethodCall(*getInterfaceNameFromHandle, &arg, 1, reply);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::GetInterfaceNameFromHandle(): MethodCall() failed"));
        return status;
    }

    const char* p = NULL;
    status = reply->GetArgs("s", &p);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::GetInterfaceNameFromHandle(): GetArgs() failed"));
        return status;
    }

    interface = qcc::String(p);
    return ER_OK;
}

QStatus P2PHelperInterface::GetInterfaceNameFromHandleAsync(int32_t handle) const
{
    QCC_DbgPrintf(("P2PHelperInterface::GetInterfaceNameFromHandleAsync()"));

    const InterfaceDescription::Member* getInterfaceNameFromHandle = m_interface->GetMember("GetInterfaceNameFromHandle");

    MsgArg arg;
    arg.Set("i", handle);

    QStatus status = m_proxyBusObject->MethodCallAsync(*getInterfaceNameFromHandle, m_listenerInternal,
                                                       static_cast<MessageReceiver::ReplyHandler>(&P2PHelperInterface::P2PHelperListenerInternal::HandleGetInterfaceNameFromHandleReply),
                                                       &arg, 1, NULL);
    if (status != ER_OK) {
        QCC_LogError(status, ("P2PHelperInterface::GetInterfaceNameFromHandleAsync(): MethodCallAsync() failed"));
    }

    return status;
}
