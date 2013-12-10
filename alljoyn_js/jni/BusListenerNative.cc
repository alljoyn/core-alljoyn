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
#include "BusListenerNative.h"

#include "BusAttachmentHost.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

BusListenerNative::BusListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

BusListenerNative::~BusListenerNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

void BusListenerNative::onRegistered(BusAttachmentHost& busAttachment)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    NPIdentifier onRegistered = NPN_GetStringIdentifier("onRegistered");
    if (NPN_HasMethod(plugin->npp, objectValue, onRegistered)) {
        NPVariant npargs[1];
        ToHostObject<BusAttachmentHost>(plugin, busAttachment, npargs[0]);

        NPVariant result = NPVARIANT_VOID;
        NPN_Invoke(plugin->npp, objectValue, onRegistered, npargs, 1, &result);

        NPN_ReleaseVariantValue(&npargs[0]);
        NPN_ReleaseVariantValue(&result);
    }
}

void BusListenerNative::onUnregistered()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    NPIdentifier onUnregistered = NPN_GetStringIdentifier("onUnregistered");
    if (NPN_HasMethod(plugin->npp, objectValue, onUnregistered)) {
        NPVariant result = NPVARIANT_VOID;
        NPN_Invoke(plugin->npp, objectValue, onUnregistered, 0, 0, &result);
        NPN_ReleaseVariantValue(&result);
    }
}

void BusListenerNative::onFoundAdvertisedName(const qcc::String& name, ajn::TransportMask transport, const qcc::String& namePrefix)
{
    QCC_DbgTrace(("%s(name=%s,transport=0x%x,namePrefix=%s)", __FUNCTION__, name.c_str(), transport, namePrefix.c_str()));
    NPIdentifier onFoundAdvertisedName = NPN_GetStringIdentifier("onFoundAdvertisedName");
    if (NPN_HasMethod(plugin->npp, objectValue, onFoundAdvertisedName)) {
        NPVariant npargs[3];
        ToDOMString(plugin, name, npargs[0]);
        ToUnsignedShort(plugin, transport, npargs[1]);
        ToDOMString(plugin, namePrefix, npargs[2]);

        NPVariant result = NPVARIANT_VOID;
        NPN_Invoke(plugin->npp, objectValue, onFoundAdvertisedName, npargs, 3, &result);
        NPN_ReleaseVariantValue(&result);

        NPN_ReleaseVariantValue(&npargs[2]);
        NPN_ReleaseVariantValue(&npargs[0]);
    }
}

void BusListenerNative::onLostAdvertisedName(const qcc::String& name, ajn::TransportMask transport, const qcc::String& namePrefix)
{
    QCC_DbgTrace(("%s(name=%s,transport=0x%x,namePrefix=%s)", __FUNCTION__, name.c_str(), transport, namePrefix.c_str()));
    NPIdentifier onLostAdvertisedName = NPN_GetStringIdentifier("onLostAdvertisedName");
    if (NPN_HasMethod(plugin->npp, objectValue, onLostAdvertisedName)) {
        NPVariant npargs[3];
        ToDOMString(plugin, name, npargs[0]);
        ToUnsignedShort(plugin, transport, npargs[1]);
        ToDOMString(plugin, namePrefix, npargs[2]);

        NPVariant result = NPVARIANT_VOID;
        NPN_Invoke(plugin->npp, objectValue, onLostAdvertisedName, npargs, 3, &result);
        NPN_ReleaseVariantValue(&result);

        NPN_ReleaseVariantValue(&npargs[2]);
        NPN_ReleaseVariantValue(&npargs[0]);
    }
}

void BusListenerNative::onNameOwnerChanged(const qcc::String& busName, const qcc::String& previousOwner, const qcc::String& newOwner)
{
    QCC_DbgTrace(("%s(busName=%s,previousOwner=%s,newOwner=%s)", __FUNCTION__, busName.c_str(), previousOwner.c_str(), newOwner.c_str()));
    NPIdentifier onNameOwnerChanged = NPN_GetStringIdentifier("onNameOwnerChanged");
    if (NPN_HasMethod(plugin->npp, objectValue, onNameOwnerChanged)) {
        NPVariant npargs[3];
        ToDOMString(plugin, busName, npargs[0]);
        ToDOMString(plugin, previousOwner, npargs[1], TreatEmptyStringAsNull);
        ToDOMString(plugin, newOwner, npargs[2], TreatEmptyStringAsNull);

        NPVariant result = NPVARIANT_VOID;
        NPN_Invoke(plugin->npp, objectValue, onNameOwnerChanged, npargs, 3, &result);
        NPN_ReleaseVariantValue(&result);

        NPN_ReleaseVariantValue(&npargs[2]);
        NPN_ReleaseVariantValue(&npargs[1]);
        NPN_ReleaseVariantValue(&npargs[0]);
    }
}

void BusListenerNative::onPropertyChanged(const qcc::String& propName, const ajn::MsgArg* propValue)
{
    QCC_DbgTrace(("%s(propName=%s,propValue=%s)", __FUNCTION__, propName.c_str(), propValue ? propValue->ToString().c_str() : "<NULL>"));
    NPIdentifier onPropertyChanged = NPN_GetStringIdentifier("onPropertyChanged");
    if (NPN_HasMethod(plugin->npp, objectValue, onPropertyChanged)) {
        NPVariant npargs[2];
        ToDOMString(plugin, propName, npargs[0]);

        if (propValue != NULL) {
            QStatus status = ER_OK;
            ToAny(plugin, *propValue, npargs[1], status);
            assert(status == ER_OK);
        } else {
            // null string indicates no propValue
            ToDOMString(plugin, NULL, 0, npargs[1], TreatEmptyStringAsNull);
        }

        NPVariant result = NPVARIANT_VOID;
        NPN_Invoke(plugin->npp, objectValue, onPropertyChanged, npargs, 2, &result);
        NPN_ReleaseVariantValue(&result);
        NPN_ReleaseVariantValue(&npargs[1]);
        NPN_ReleaseVariantValue(&npargs[0]);
    }
}

void BusListenerNative::onStopping()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    NPIdentifier onStopping = NPN_GetStringIdentifier("onStopping");
    if (NPN_HasMethod(plugin->npp, objectValue, onStopping)) {
        NPVariant result = NPVARIANT_VOID;
        NPN_Invoke(plugin->npp, objectValue, onStopping, 0, 0, &result);
        NPN_ReleaseVariantValue(&result);
    }
}

void BusListenerNative::onDisconnected()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    NPIdentifier onDisconnected = NPN_GetStringIdentifier("onDisconnected");
    if (NPN_HasMethod(plugin->npp, objectValue, onDisconnected)) {
        NPVariant result = NPVARIANT_VOID;
        NPN_Invoke(plugin->npp, objectValue, onDisconnected, 0, 0, &result);
        NPN_ReleaseVariantValue(&result);
    }
}
