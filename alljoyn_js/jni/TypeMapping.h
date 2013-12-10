/*
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
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
#ifndef _TYPEMAPPING_H
#define _TYPEMAPPING_H

#include "HostObject.h"
#include "npn.h"
#include <alljoyn/Status.h>
#include <alljoyn/MsgArg.h>
#include <qcc/String.h>

bool NewObject(Plugin& plugin, NPVariant& variant);
bool NewArray(Plugin& plugin, NPVariant& variant);

void ToAny(Plugin& plugin, const NPVariant& value, const qcc::String signature, ajn::MsgArg& arg, bool& typeError);
void ToAny(Plugin& plugin, const ajn::MsgArg& value, NPVariant& variant, QStatus& status);

bool ToBoolean(Plugin& plugin, const NPVariant& value, bool& typeError);
void ToBoolean(Plugin& plugin, bool value, NPVariant& variant);

uint8_t ToOctet(Plugin& plugin, const NPVariant& value, bool& typeError);
void ToOctet(Plugin& plugin, uint8_t value, NPVariant& variant);

int16_t ToShort(Plugin& plugin, const NPVariant& value, bool& typeError);
void ToShort(Plugin& plugin, int16_t value, NPVariant& variant);

uint16_t ToUnsignedShort(Plugin& plugin, const NPVariant& value, bool& typeError);
void ToUnsignedShort(Plugin& plugin, uint16_t value, NPVariant& variant);

int32_t ToLong(Plugin& plugin, const NPVariant& value, bool& typeError);
void ToLong(Plugin& plugin, int32_t value, NPVariant& variant);

uint32_t ToUnsignedLong(Plugin& plugin, const NPVariant& value, bool& typeError);
void ToUnsignedLong(Plugin& plugin, uint32_t value, NPVariant& variant);

int64_t ToLongLong(Plugin& plugin, const NPVariant& value, bool& typeError);
void ToLongLong(Plugin& plugin, int64_t value, NPVariant& variant);

uint64_t ToUnsignedLongLong(Plugin& plugin, const NPVariant& value, bool& typeError);
void ToUnsignedLongLong(Plugin& plugin, uint64_t value, NPVariant& variant);

double ToDouble(Plugin& plugin, const NPVariant& value, bool& typeError);
void ToDouble(Plugin& plugin, double value, NPVariant& variant);

qcc::String ToDOMString(Plugin& plugin, const NPVariant& value, bool& typeError,
                        bool treatNullAsEmptyString = false,
                        bool treatUndefinedAsEmptyString = false);

typedef enum {
    TreatEmptyStringAsEmptyString,
    TreatEmptyStringAsNull,
    TreatEmptyStringAsUndefined,
} TreatEmptyStringAs;

/**
 * @param variant the string. It must be released by the caller.
 */
void ToDOMString(Plugin& plugin, qcc::String value, NPVariant& variant,
                 TreatEmptyStringAs treatEmptyStringAs = TreatEmptyStringAsEmptyString);

/**
 * @param variant the string. It must be released by the caller.
 */
void ToDOMString(Plugin& plugin, const char* str, uint32_t len, NPVariant& variant,
                 TreatEmptyStringAs treatEmptyStringAs = TreatEmptyStringAsEmptyString);

/**
 * @return a NativeObject.  The native object must be released by the caller with delete.  Note that
 *         the return value may be 0 if the variant is undefined or null.
 */
template <class T> T* ToNativeObject(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    typeError = false;
    if (NPVARIANT_IS_VOID(value) || NPVARIANT_IS_NULL(value)) {
        return 0;
    } else if (!NPVARIANT_IS_OBJECT(value)) {
        typeError = true;
        return 0;
    } else {
        return new T(plugin, NPVARIANT_TO_OBJECT(value));
    }
}

/**
 * @param value a NativeObject.
 * @param variant the native object.  It must be released by the caller.
 */
template <class T> void ToNativeObject(Plugin& plugin, T* value, NPVariant& variant)
{
    if (value && value->objectValue) {
        NPN_RetainObject(value->objectValue);
        OBJECT_TO_NPVARIANT(value->objectValue, variant);
    } else {
        NULL_TO_NPVARIANT(variant);
    }
}

template <class T> void ToHostObject(Plugin& plugin, T& value, NPVariant& variant)
{
    OBJECT_TO_NPVARIANT(HostObject<T>::GetInstance(plugin, value), variant);
}

/*
 * This is a bit of a hack to get the impl pointer out of a native object pointer.  Ideally it would
 * return a reference, but there's no such thing as a null/uninitialized reference.
 */
template <class T> T* ToHostObject(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    typeError = false;
    if (NPVARIANT_IS_VOID(value) || NPVARIANT_IS_NULL(value)) {
        return 0;
    }
    if (!NPVARIANT_IS_OBJECT(value)) {
        typeError = true;
        return 0;
    }
    NPObject* npobj = NPVARIANT_TO_OBJECT(value);
    if (&HostObject<T>::Class != npobj->_class) {
        typeError = true;
        return 0;
    }
    return HostObject<T>::GetImpl(plugin, npobj);
}

#endif // _TYPEMAPPING_H
