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
#include "AuthListenerNative.h"

#include "BusAttachmentHost.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

AuthListenerNative::AuthListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

AuthListenerNative::~AuthListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

bool AuthListenerNative::onRequest(qcc::String& authMechanism, qcc::String& peerName, uint16_t authCount,
                                   qcc::String& userName, uint16_t credMask, CredentialsHost& credentials)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    bool requested = false;
    NPIdentifier onRequest = NPN_GetStringIdentifier("onRequest");
    if (NPN_HasMethod(plugin->npp, objectValue, onRequest)) {
        NPVariant npargs[6];
        ToDOMString(plugin, authMechanism, npargs[0]);
        ToDOMString(plugin, peerName, npargs[1]);
        ToUnsignedShort(plugin, authCount, npargs[2]);
        ToDOMString(plugin, userName, npargs[3]);
        ToUnsignedShort(plugin, credMask, npargs[4]);
        ToHostObject<CredentialsHost>(plugin, credentials, npargs[5]);

        NPVariant result = NPVARIANT_VOID;
        if (NPN_Invoke(plugin->npp, objectValue, onRequest, npargs, 6, &result)) {
            bool ignore;
            requested = ToBoolean(plugin, result, ignore);
        } else {
            QCC_LogError(ER_FAIL, ("NPN_Invoke failed"));
        }
        NPN_ReleaseVariantValue(&result);

        NPN_ReleaseVariantValue(&npargs[5]);
        NPN_ReleaseVariantValue(&npargs[3]);
        NPN_ReleaseVariantValue(&npargs[1]);
        NPN_ReleaseVariantValue(&npargs[0]);
    } else {
        QCC_LogError(ER_FAIL, ("No such method 'onRequest'"));
    }
    return requested;
}

bool AuthListenerNative::onVerify(qcc::String& authMechanism, qcc::String& peerName, CredentialsHost& credentials)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    bool verified = false;
    NPIdentifier onVerify = NPN_GetStringIdentifier("onVerify");
    if (NPN_HasMethod(plugin->npp, objectValue, onVerify)) {
        NPVariant npargs[3];
        ToDOMString(plugin, authMechanism, npargs[0]);
        ToDOMString(plugin, peerName, npargs[1]);
        ToHostObject<CredentialsHost>(plugin, credentials, npargs[2]);

        NPVariant result = NPVARIANT_VOID;
        if (NPN_Invoke(plugin->npp, objectValue, onVerify, npargs, 3, &result)) {
            bool ignore;
            verified = ToBoolean(plugin, result, ignore);
        } else {
            QCC_LogError(ER_FAIL, ("NPN_Invoke failed"));
        }
        NPN_ReleaseVariantValue(&result);

        NPN_ReleaseVariantValue(&npargs[2]);
        NPN_ReleaseVariantValue(&npargs[1]);
        NPN_ReleaseVariantValue(&npargs[0]);
    } else {
        QCC_LogError(ER_FAIL, ("No such method 'onVerify'"));
    }
    return verified;
}

void AuthListenerNative::onSecurityViolation(QStatus status, MessageHost& message)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    NPIdentifier onSecurityViolation = NPN_GetStringIdentifier("onSecurityViolation");
    if (NPN_HasMethod(plugin->npp, objectValue, onSecurityViolation)) {
        NPVariant npargs[2];
        ToUnsignedShort(plugin, status, npargs[0]);
        ToHostObject<MessageHost>(plugin, message, npargs[1]);

        NPVariant result = NPVARIANT_VOID;
        NPN_Invoke(plugin->npp, objectValue, onSecurityViolation, npargs, 2, &result);
        NPN_ReleaseVariantValue(&result);

        NPN_ReleaseVariantValue(&npargs[1]);
    }
}

void AuthListenerNative::onComplete(qcc::String& authMechanism, qcc::String& peerName, bool success)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    NPIdentifier onComplete = NPN_GetStringIdentifier("onComplete");
    if (NPN_HasMethod(plugin->npp, objectValue, onComplete)) {
        NPVariant npargs[3];
        ToDOMString(plugin, authMechanism, npargs[0]);
        ToDOMString(plugin, peerName, npargs[1]);
        ToBoolean(plugin, success, npargs[2]);

        NPVariant result = NPVARIANT_VOID;
        NPN_Invoke(plugin->npp, objectValue, onComplete, npargs, 3, &result);
        NPN_ReleaseVariantValue(&result);

        NPN_ReleaseVariantValue(&npargs[1]);
        NPN_ReleaseVariantValue(&npargs[0]);
    }
}
