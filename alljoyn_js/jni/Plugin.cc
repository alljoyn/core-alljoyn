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
#include "Plugin.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>
#include <qcc/Util.h>
#include <assert.h>
#include <string.h>

#define QCC_MODULE "ALLJOYN_JS"

_Plugin::_Plugin(NPP npp) :
    npp(npp), params(0)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_Plugin::_Plugin() :
    npp(0), params(0)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

_Plugin::~_Plugin()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

qcc::String _Plugin::ToFilename(const qcc::String& in)
{
    qcc::String url = in;
    QCC_DbgPrintf(("unencoded url=%s", url.c_str()));
    for (size_t i = 0; i < url.size(); ++i) {
        switch (url[i]) {
        case '$':
        case '-':
        case '_':
        case '.':
        case '+':
        case '!':
        case '*':
        case '\'':
        case '(':
        case ')':
        case ',':
        case ';':
        case '/':
        case '?':
        case ':':
        case '@':
        case '=':
        case '&': {
                char encoded[3];
                snprintf(encoded, 3, "%02X", url[i]);
                url[i] = '%';
                url.insert(i + 1, encoded, 2);
                i += 2;
                break;
            }

        default:
            /* Do nothing */
            break;
        }
    }
    QCC_DbgPrintf(("encoded url=%s", url.c_str()));
    return url;
}

bool _Plugin::RaiseBusError(QStatus code, const char* message)
{
    _error.name = "BusError";
    _error.message = message;
    _error.code = code;
    QCC_LogError(_error.code, ("%s: %s", _error.name.c_str(), _error.message.c_str()));
    return false;
}

bool _Plugin::RaiseTypeError(const char* message)
{
    _error.name = "TypeError";
    _error.message = message;
    QCC_LogError(_error.code, ("%s: %s", _error.name.c_str(), _error.message.c_str()));
    return false;
}

void _Plugin::CheckError(NPObject* npobj)
{
    if (!_error.name.empty()) {
        error = _error;
        _error.Clear();
        NPN_SetException(npobj, error.name.c_str());
    }
}