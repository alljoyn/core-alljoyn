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
#include "HttpRequestHost.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>
#include <qcc/time.h>

#define QCC_MODULE "ALLJOYN_JS"

_HttpRequestHost::_HttpRequestHost(Plugin& plugin, HttpServer& httpServer, Http::Headers& requestHeaders, qcc::SocketStream& stream, qcc::SocketFd sessionFd) :
    ScriptableObject(plugin),
    httpServer(httpServer),
    requestHeaders(requestHeaders),
    stream(stream),
    sessionFd(sessionFd)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    /* Default response */
    status = 200;
    statusText = "OK";
    responseHeaders["Date"] = qcc::UTCTime();
    responseHeaders["Content-Type"] = "application/octet-stream";

    ATTRIBUTE("status", &_HttpRequestHost::getStatus, &_HttpRequestHost::setStatus);
    ATTRIBUTE("statusText", &_HttpRequestHost::getStatusText, &_HttpRequestHost::setStatusText);

    OPERATION("getAllRequestHeaders", &_HttpRequestHost::getAllRequestHeaders);
    OPERATION("getRequestHeader", &_HttpRequestHost::getRequestHeader);
    OPERATION("setResponseHeader", &_HttpRequestHost::setResponseHeader);
    OPERATION("send", &_HttpRequestHost::send);
}

_HttpRequestHost::~_HttpRequestHost()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool _HttpRequestHost::getStatus(NPVariant* result)
{
    ToUnsignedShort(plugin, status, *result);
    return true;
}

bool _HttpRequestHost::setStatus(const NPVariant* value)
{
    bool typeError = false;
    status = ToUnsignedShort(plugin, *value, typeError);
    return !typeError;
}

bool _HttpRequestHost::getStatusText(NPVariant* result)
{
    ToDOMString(plugin, statusText, *result);
    return true;
}

bool _HttpRequestHost::setStatusText(const NPVariant* value)
{
    bool typeError = false;
    statusText = ToDOMString(plugin, *value, typeError);
    return !typeError;
}

bool _HttpRequestHost::getAllRequestHeaders(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String headers;
    for (Http::Headers::iterator it = requestHeaders.begin(); it != requestHeaders.end(); ++it) {
        headers += it->first + ": " + it->second + "\r\n";
    }
    ToDOMString(plugin, headers, *result);
    return true;
}

bool _HttpRequestHost::getRequestHeader(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String header;
    qcc::String value;
    Http::Headers::iterator it;

    bool typeError = false;
    if (argCount < 1) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }
    header = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }

    it = requestHeaders.find(header); // TODO should be case-insensitive
    if (it != requestHeaders.end()) {
        value = it->second;
    }

exit:
    ToDOMString(plugin, value, *result, TreatEmptyStringAsNull);
    return !typeError;
}

bool _HttpRequestHost::setResponseHeader(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String header;
    qcc::String value;

    bool typeError = false;
    if (argCount < 2) {
        typeError = true;
        plugin->RaiseTypeError("not enough arguments");
        goto exit;
    }
    header = ToDOMString(plugin, args[0], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 0 is not a string");
        goto exit;
    }
    value = ToDOMString(plugin, args[1], typeError);
    if (typeError) {
        plugin->RaiseTypeError("argument 1 is not a string");
        goto exit;
    }
    QCC_DbgTrace(("%s: %s", header.c_str(), value.c_str()));

    responseHeaders[header] = value;

exit:
    VOID_TO_NPVARIANT(*result);
    return !typeError;
}

bool _HttpRequestHost::send(const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    httpServer->SendResponse(stream, status, statusText, responseHeaders, sessionFd);
    return true;
}

