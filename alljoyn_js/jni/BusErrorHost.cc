/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
#include "BusErrorHost.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

_BusErrorHost::_BusErrorHost(Plugin& plugin, const qcc::String& name, const qcc::String& message, QStatus code) :
    ScriptableObject(plugin, _BusErrorInterface::Constants()),
    name(name),
    message(message),
    code(code)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ATTRIBUTE("name", &_BusErrorHost::getName, 0);
    ATTRIBUTE("message", &_BusErrorHost::getMessage, 0);
    ATTRIBUTE("code", &_BusErrorHost::getCode, 0);
}

_BusErrorHost::_BusErrorHost(Plugin& plugin, QStatus code) :
    ScriptableObject(plugin, _BusErrorInterface::Constants()),
    name("BusError"),
    code(code)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    ATTRIBUTE("name", &_BusErrorHost::getName, 0);
    ATTRIBUTE("message", &_BusErrorHost::getMessage, 0);
    ATTRIBUTE("code", &_BusErrorHost::getCode, 0);
}

_BusErrorHost::~_BusErrorHost()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

qcc::String _BusErrorHost::ToString()
{
    qcc::String string;
    if (!name.empty()) {
        string += name + ": ";
    }
    if (!message.empty()) {
        string += message + " ";
    }
    string += qcc::String("(") + QCC_StatusText(code) + ")";
    return string;
}

bool _BusErrorHost::getName(NPVariant* result)
{
    ToDOMString(plugin, name, *result);
    return true;
}

bool _BusErrorHost::getMessage(NPVariant* result)
{
    ToDOMString(plugin, message, *result);
    return true;
}

bool _BusErrorHost::getCode(NPVariant* result)
{
    ToUnsignedShort(plugin, code, *result);
    return true;
}