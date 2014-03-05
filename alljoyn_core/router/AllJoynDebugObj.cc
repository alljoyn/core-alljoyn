/**
 * @file
 * BusObject responsible for implementing the AllJoyn methods (org.alljoyn.Debug)
 * for messages controlling debug output.
 */

/******************************************************************************
 * Copyright (c) 2011,2012,2014 AllSeen Alliance. All rights reserved.
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

// Include contents in debug builds only.
#ifndef NDEBUG

#include <qcc/platform.h>

#include <assert.h>

#include <map>

#include <qcc/Log.h>
#include <qcc/String.h>
#include <qcc/StringMapKey.h>

#include "AllJoynDebugObj.h"
#include "Bus.h"
#include "BusController.h"

using namespace ajn;
using namespace debug;


/*
 * Singleton
 */
AllJoynDebugObj* AllJoynDebugObj::self = NULL;


AllJoynDebugObj* AllJoynDebugObj::GetAllJoynDebugObj()
{
    assert(self && "Invalid access to AllJoynDebugObj; read the comments");
    return self;
}

QStatus AllJoynDebugObj::Init()
{
    QStatus status;

    /* Make this object implement org.alljoyn.Debug */
    const InterfaceDescription* alljoynDbgIntf = busController->GetBus().GetInterface(org::alljoyn::Daemon::Debug::InterfaceName);
    if (!alljoynDbgIntf) {
        status = ER_BUS_NO_SUCH_INTERFACE;
        return status;
    }

    status = AddInterface(*alljoynDbgIntf);
    if (status == ER_OK) {
        /* Hook up the methods to their handlers */
        const MethodEntry methodEntries[] = {
            { alljoynDbgIntf->GetMember("SetDebugLevel"),
              static_cast<MessageReceiver::MethodHandler>(&AllJoynDebugObj::SetDebugLevel) },
        };

        status = AddMethodHandlers(methodEntries, ArraySize(methodEntries));

        if (status == ER_OK) {
            status = busController->GetBus().RegisterBusObject(*this);
        }
    }
    return status;
}


QStatus AllJoynDebugObj::AddDebugInterface(AllJoynDebugObjAddon* addon,
                                           const char* ifaceName,
                                           const MethodInfo* methodInfo,
                                           size_t methodInfoSize,
                                           Properties& ifaceProperties)
{
    assert(addon);
    assert(ifaceName);

    InterfaceDescription* ifc;
    MethodEntry* methodEntries = methodInfoSize ? new MethodEntry[methodInfoSize] : NULL;
    qcc::String ifaceNameStr = ifaceName;

    busController->GetBus().UnregisterBusObject(*this);

    QStatus status = busController->GetBus().CreateInterface(ifaceName, ifc);
    if (status != ER_OK) {
        goto exit;
    }

    for (size_t i = 0; i < methodInfoSize; ++i) {
        ifc->AddMember(MESSAGE_METHOD_CALL, methodInfo[i].name, methodInfo[i].inputSig, methodInfo[i].outSig, methodInfo[i].argNames, 0);
        const InterfaceDescription::Member* member = ifc->GetMember(methodInfo[i].name);
        methodEntries[i].member = member;
        methodEntries[i].handler = static_cast<MessageReceiver::MethodHandler>(&AllJoynDebugObj::GenericMethodHandler);
        methodHandlerMap[member] = AddonMethodHandler(addon, methodInfo[i].handler);
    }

    const AllJoynDebugObj::Properties::Info* propInfo;
    size_t propInfoSize;

    ifaceProperties.GetProperyInfo(propInfo, propInfoSize);
    for (size_t i = 0; i < propInfoSize; ++i) {
        ifc->AddProperty(propInfo[i].name, propInfo[i].signature, propInfo[i].access);
    }

    ifc->Activate();

    status = AddInterface(*ifc);
    if (status != ER_OK) {
        goto exit;
    }
    if (methodEntries) {
        status = AddMethodHandlers(methodEntries, methodInfoSize);
        if (status != ER_OK) {
            goto exit;
        }
    }

    properties.insert(std::pair<qcc::StringMapKey, Properties*>(ifaceNameStr, &ifaceProperties));

exit:
    if (methodEntries) {
        delete[] methodEntries;
    }
    return status;
}


QStatus AllJoynDebugObj::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    PropertyStore::const_iterator it = properties.find(ifcName);
    if (it == properties.end()) {
        return ER_BUS_NO_SUCH_PROPERTY;
    }
    return it->second->Get(propName, val);
}


QStatus AllJoynDebugObj::Set(const char* ifcName, const char* propName, MsgArg& val)
{
    PropertyStore::const_iterator it = properties.find(propName);
    if (it == properties.end()) {
        return ER_BUS_NO_SUCH_PROPERTY;
    }
    return it->second->Set(propName, val);
}


void AllJoynDebugObj::GetProp(const InterfaceDescription::Member* member, Message& msg)
{
    const qcc::String guid(busController->GetBus().GetInternal().GetGlobalGUID().ToShortString());
    qcc::String sender(msg->GetSender());
    // Only allow local connections to get properties
    if (sender.substr(1, guid.size()) == guid) {
        BusObject::GetProp(member, msg);
    } // else someone off-device is trying to set our debug output, punish them by not responding.
}


AllJoynDebugObj::AllJoynDebugObj(BusController* busController) :
    BusObject(org::alljoyn::Daemon::Debug::ObjectPath),
    busController(busController)
{
    self = this;
}

AllJoynDebugObj::~AllJoynDebugObj()
{
    self = NULL;
}

/**
 * Need to let the bus contoller know when the registration is complete
 */
void AllJoynDebugObj::ObjectRegistered() {
    /*
     * Must call the base class first.
     */
    BusObject::ObjectRegistered();
    busController->ObjectRegistered(this);
}

/**
 * Handles the SetDebugLevel method call.
 *
 * @param member    Member
 * @param msg       The incoming message
 */
void AllJoynDebugObj::SetDebugLevel(const InterfaceDescription::Member* member, Message& msg)
{
    const qcc::String guid(busController->GetBus().GetInternal().GetGlobalGUID().ToShortString());
    qcc::String sender(msg->GetSender());
    // Only allow local connections to set the debug level
    if (sender.substr(1, guid.size()) == guid) {
        const char* module;
        uint32_t level;
        QStatus status = msg->GetArgs("su", &module, &level);
        if (status == ER_OK) {
            QCC_SetDebugLevel(module, level);
            MethodReply(msg, (MsgArg*)NULL, 0);
        } else {
            MethodReply(msg, "org.alljoyn.Debug.InternalError", QCC_StatusText(status));
        }
    } // else someone off-device is trying to set our debug output, punish them by not responding.
}


void AllJoynDebugObj::GenericMethodHandler(const InterfaceDescription::Member* member, Message& msg)
{
    AddonMethodHandlerMap::iterator it = methodHandlerMap.find(member);
    if (it != methodHandlerMap.end()) {
        // Call the addon's method handler
        std::vector<MsgArg> replyArgs;
        QStatus status = (it->second.first->*it->second.second)(msg, replyArgs);

        if (status == ER_OK) {
            MethodReply(msg, replyArgs.empty() ? NULL : &replyArgs.front(), replyArgs.size());
        } else {
            MethodReply(msg, "org.alljoyn.Debug.InternalError", "Failure processing method call");
        }
    } else {
        MethodReply(msg, "org.alljoyn.Debug.InternalError", "Unknown method");
    }
}

#endif //NDEBUG
