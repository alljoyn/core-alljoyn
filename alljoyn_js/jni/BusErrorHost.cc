/*
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
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
