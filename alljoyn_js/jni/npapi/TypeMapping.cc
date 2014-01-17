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
#include "TypeMapping.h"

#include "NativeObject.h"
#include "SignatureUtils.h"
#include "SocketFdHost.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <qcc/Debug.h>

#ifndef PRIi64
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

#define QCC_MODULE "ALLJOYN_JS"

#ifndef NAN
/*
 * From http://msdn.microsoft.com/en-us/library/w22adx1s(v=vs.80).aspx
 */
static const unsigned long __nan[2] = { 0xffffffff, 0x7fffffff };
#define NAN (*(const double*)__nan);
#endif

static bool IsPrimitive(const NPVariant& value)
{
    return (value.type != NPVariantType_Object);
}

typedef enum {
    HINT_STRING,
    HINT_NUMBER
} Hint;

/**
 * @return the default value.  This must be released by the caller.
 */
static NPVariant DefaultValue(Plugin& plugin, NPObject* object, Hint hint, bool& typeError)
{
    typeError = false;
    int step = (hint == HINT_NUMBER) ? 1 : 2;
    while (0 < step && step < 3) {
        switch (step) {
        case 1: {
                NPVariant val;
                if (NPN_Invoke(plugin->npp, object, NPN_GetStringIdentifier("valueOf"), 0, 0, &val)) {
                    if (IsPrimitive(val)) {
                        return val;
                    } else {
                        NPN_ReleaseVariantValue(&val);
                    }
                }
                break;
            }

        case 2: {
                NPVariant str;
                if (NPN_Invoke(plugin->npp, object, NPN_GetStringIdentifier("toString"), 0, 0, &str)) {
                    if (IsPrimitive(str)) {
                        return str;
                    } else {
                        NPN_ReleaseVariantValue(&str);
                    }
                }
                break;
            }
        }
        step += (hint == HINT_NUMBER) ? 1 : -1;
    }
    typeError = true;
    NPVariant undefined = NPVARIANT_VOID;
    return undefined;
}

/**
 * @return the primitive value.  This must be released by the caller.
 */
static NPVariant ToPrimitive(Plugin& plugin, const NPVariant& value, Hint hint, bool& typeError)
{
    if (IsPrimitive(value)) {
        return value;
    } else {
        assert(NPVARIANT_IS_OBJECT(value));
        return DefaultValue(plugin, NPVARIANT_TO_OBJECT(value), hint, typeError);
    }
}

bool ToBoolean(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    typeError = false;
    switch (value.type) {
    case NPVariantType_Void:
    case NPVariantType_Null:
        return false;

    case NPVariantType_Bool:
        return NPVARIANT_TO_BOOLEAN(value);

    case NPVariantType_Int32:
        return (NPVARIANT_TO_INT32(value) != 0);

    case NPVariantType_Double:
        return !(fpclassify(NPVARIANT_TO_DOUBLE(value)) & (FP_NAN | FP_ZERO));

    case NPVariantType_String:
        return NPVARIANT_TO_STRING(value).UTF8Length != 0;

    case NPVariantType_Object:
        return true;

    default:
        assert(0); /* Will not reach here. */
        return false;
    }
}

static double ToNumber(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    switch (value.type) {
    case NPVariantType_Void:
        return NAN;

    case NPVariantType_Null:
        return +0;

    case NPVariantType_Bool:
        return (NPVARIANT_TO_BOOLEAN(value) ? 1 : 0);

    case NPVariantType_Int32:
        return NPVARIANT_TO_INT32(value);

    case NPVariantType_Double:
        return NPVARIANT_TO_DOUBLE(value);

    case NPVariantType_String: {
            double n = NAN;
            NPObject* window = NULL;
            if (NPERR_NO_ERROR == NPN_GetValue(plugin->npp, NPNVWindowNPObject, &window)) {
                NPVariant result;
                if (NPN_Invoke(plugin->npp, window, NPN_GetStringIdentifier("parseFloat"), &value, 1, &result)) {
                    if (NPVARIANT_IS_INT32(result)) {
                        n = NPVARIANT_TO_INT32(result);
                    } else if (NPVARIANT_IS_DOUBLE(result)) {
                        n = NPVARIANT_TO_DOUBLE(result);
                    }
                    NPN_ReleaseVariantValue(&result);
                }
                NPN_ReleaseObject(window);
            }
            return n;
        }

    case NPVariantType_Object: {
            NPVariant primitive = ToPrimitive(plugin, value, HINT_NUMBER, typeError);
            if (!typeError) {
                return ToNumber(plugin, primitive, typeError);
            }
            NPN_ReleaseVariantValue(&primitive);
            return NAN;
        }

    default:
        assert(0); /* Will not reach here. */
        return NAN;
    }
}

/*
 * Unlike many other programming languages JavaScript is not a typed language.
 * In JavaScript numbers are always stored as double precision floating point numbers
 * we must read the double and cast it to the Integer type desired.
 */
template <class T> T ToInteger(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    double n = ToNumber(plugin, value, typeError);
    if (fpclassify(n) & (FP_NAN | FP_ZERO | FP_INFINITE)) {
        return +0;
    }
    return static_cast<T>(n);
}

/**
 * The JavaScript Number.toString() function.
 *
 * Why use this?  The JavaScript specifies a particular algorithm for converting numbers to strings
 * that is not the same as printf().  This is used to ensure that the coercion done here is
 * identical to that done by JavaScript.
 */
static qcc::String NumberToString(Plugin& plugin, double value)
{
    qcc::String s("NaN");
    NPObject* window = NULL;
    if (NPERR_NO_ERROR == NPN_GetValue(plugin->npp, NPNVWindowNPObject, &window)) {
        char numberToString[50];
        snprintf(numberToString, 50, "new Number(%.16e).toString();", value);
        NPString script = { numberToString, (uint32_t)strlen(numberToString) };
        NPVariant variant = NPVARIANT_VOID;
        if (NPN_Evaluate(plugin->npp, window, &script, &variant) &&
            NPVARIANT_IS_STRING(variant) && NPVARIANT_TO_STRING(variant).UTF8Length) {
            s = qcc::String(NPVARIANT_TO_STRING(variant).UTF8Characters, NPVARIANT_TO_STRING(variant).UTF8Length);
        } else {
            QCC_LogError(ER_FAIL, ("new Number().toString() failed"));
        }
        NPN_ReleaseVariantValue(&variant);
        NPN_ReleaseObject(window);
    }
    return s;
}

static qcc::String ToString(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    switch (value.type) {
    case NPVariantType_Void:
        return "undefined";

    case NPVariantType_Null:
        return "null";

    case NPVariantType_Bool:
        return NPVARIANT_TO_BOOLEAN(value) ? "true" : "false";

    case NPVariantType_Int32:
        return NumberToString(plugin, NPVARIANT_TO_INT32(value));

    case NPVariantType_Double:
        return NumberToString(plugin, NPVARIANT_TO_DOUBLE(value));

    case NPVariantType_String:
        return NPVARIANT_TO_STRING(value).UTF8Length ? qcc::String(NPVARIANT_TO_STRING(value).UTF8Characters, NPVARIANT_TO_STRING(value).UTF8Length) : qcc::String();

    case NPVariantType_Object: {
            NPVariant primitive = ToPrimitive(plugin, value, HINT_STRING, typeError);
            if (!typeError) {
                return ToString(plugin, primitive, typeError);
            }
            NPN_ReleaseVariantValue(&primitive);
            return "undefined";
        }

    default:
        assert(0); /* Will not reach here. */
        return qcc::String();
    }
}

static void ToDictionaryKey(Plugin& plugin, const ajn::MsgArg& value, NPIdentifier& identifier)
{
    char str[32];
    switch (value.typeId) {
    case ajn::ALLJOYN_BYTE:
        snprintf(str, 32, "%u", value.v_byte);
        identifier = NPN_GetStringIdentifier(str);
        break;

    case ajn::ALLJOYN_BOOLEAN:
        identifier = NPN_GetStringIdentifier(value.v_bool ? "true" : "false");
        break;

    case ajn::ALLJOYN_INT16:
        snprintf(str, 32, "%d", value.v_int16);
        identifier = NPN_GetStringIdentifier(str);
        break;

    case ajn::ALLJOYN_UINT16:
        snprintf(str, 32, "%u", value.v_uint16);
        identifier = NPN_GetStringIdentifier(str);
        break;

    case ajn::ALLJOYN_INT32:
        snprintf(str, 32, "%d", value.v_int32);
        identifier = NPN_GetStringIdentifier(str);
        break;

    case ajn::ALLJOYN_UINT32:
        snprintf(str, 32, "%u", value.v_uint32);
        identifier = NPN_GetStringIdentifier(str);
        break;

    case ajn::ALLJOYN_INT64:
        snprintf(str, 32, "%" PRIi64, value.v_int64);
        identifier = NPN_GetStringIdentifier(str);
        break;

    case ajn::ALLJOYN_UINT64:
        snprintf(str, 32, "%" PRIu64, value.v_uint64);
        identifier = NPN_GetStringIdentifier(str);
        break;

    case ajn::ALLJOYN_DOUBLE: {
            qcc::String str = NumberToString(plugin, value.v_double);
            identifier = NPN_GetStringIdentifier(str.c_str());
            break;
        }

    case ajn::ALLJOYN_STRING:
        identifier = NPN_GetStringIdentifier(value.v_string.str);
        break;

    case ajn::ALLJOYN_OBJECT_PATH:
        identifier = NPN_GetStringIdentifier(value.v_objPath.str);
        break;

    case ajn::ALLJOYN_SIGNATURE:
        identifier = NPN_GetStringIdentifier(value.v_signature.sig);
        break;

    case ajn::ALLJOYN_HANDLE:
        switch (sizeof(qcc::SocketFd)) {
        case 4:
            snprintf(str, 32, "%d", (int32_t)value.v_handle.fd);
            identifier = NPN_GetStringIdentifier(str);
            break;

        case 8:
            snprintf(str, 32, "%" PRIi64, (int64_t)value.v_handle.fd);
            identifier = NPN_GetStringIdentifier(str);
            break;
        }
        break;

    default:
        /* This should not make it through the AllJoyn core. */
        assert(0);
        QCC_LogError(ER_FAIL, ("Unhandled MsgArg type: 0x%x", value.typeId));
        break;
    }
}

bool NewObject(Plugin& plugin, NPVariant& variant)
{
    NPObject* window = NULL;
    if (NPERR_NO_ERROR == NPN_GetValue(plugin->npp, NPNVWindowNPObject, &window)) {
        NPString script = { "new Object();", (uint32_t)strlen("new Object();") };
        bool evaluated = NPN_Evaluate(plugin->npp, window, &script, &variant);
        NPN_ReleaseObject(window);
        bool success = (evaluated && NPVARIANT_IS_OBJECT(variant));
        if (!success) {
            QCC_LogError(ER_FAIL, ("new Object() failed"));
        }
        return success;
    }
    VOID_TO_NPVARIANT(variant);
    return false;
}

bool NewArray(Plugin& plugin, NPVariant& variant)
{
    NPObject* window = NULL;
    if (NPERR_NO_ERROR == NPN_GetValue(plugin->npp, NPNVWindowNPObject, &window)) {
        NPString script = { "new Array();", (uint32_t)strlen("new Array();") };
        bool evaluated = NPN_Evaluate(plugin->npp, window, &script, &variant);
        NPN_ReleaseObject(window);
        bool success = (evaluated && NPVARIANT_IS_OBJECT(variant));
        if (!success) {
            QCC_LogError(ER_FAIL, ("new Array() failed"));
        }
        return success;
    }
    VOID_TO_NPVARIANT(variant);
    return false;
}

/**
 * @param signature a complete type signature.
 */
void ToAny(Plugin& plugin, const NPVariant& value, const qcc::String signature, ajn::MsgArg& arg, bool& typeError)
{
    typeError = false;
    QStatus status = ER_OK;
    switch (signature[0]) {
    case ajn::ALLJOYN_BOOLEAN:
        status = arg.Set("b", ToBoolean(plugin, value, typeError));
        break;

    case ajn::ALLJOYN_BYTE:
        status = arg.Set("y", ToOctet(plugin, value, typeError));
        break;

    case ajn::ALLJOYN_INT16:
        status = arg.Set("n", ToShort(plugin, value, typeError));
        break;

    case ajn::ALLJOYN_UINT16:
        status = arg.Set("q", ToUnsignedShort(plugin, value, typeError));
        break;

    case ajn::ALLJOYN_INT32:
        status = arg.Set("i", ToLong(plugin, value, typeError));
        break;

    case ajn::ALLJOYN_UINT32:
        status = arg.Set("u", ToUnsignedLong(plugin, value, typeError));
        break;

    case ajn::ALLJOYN_INT64: {
            int64_t x = ToLongLong(plugin, value, typeError);
            status = arg.Set("x", x);
            break;
        }

    case ajn::ALLJOYN_UINT64: {
            uint64_t t = ToUnsignedLongLong(plugin, value, typeError);
            status = arg.Set("t", t);
            break;
        }

    case ajn::ALLJOYN_DOUBLE: {
            double d = ToDouble(plugin, value, typeError);
            status = arg.Set("d", d);
            break;
        }

    case ajn::ALLJOYN_STRING: {
            qcc::String s = ToDOMString(plugin, value, typeError);
            status = arg.Set("s", s.c_str());
            arg.Stabilize();
            break;
        }

    case ajn::ALLJOYN_OBJECT_PATH: {
            qcc::String o = ToDOMString(plugin, value, typeError);
            status = arg.Set("o", o.c_str());
            arg.Stabilize();
            break;
        }

    case ajn::ALLJOYN_SIGNATURE: {
            qcc::String g = ToDOMString(plugin, value, typeError);
            status = arg.Set("g", g.c_str());
            arg.Stabilize();
            break;
        }

    case ajn::ALLJOYN_STRUCT_OPEN: {
            if (!NPVARIANT_IS_OBJECT(value)) {
                typeError = true;
                break;
            }
            NPVariant length = NPVARIANT_VOID;
            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetStringIdentifier("length"), &length) ||
                !(NPVARIANT_IS_INT32(length) || NPVARIANT_IS_DOUBLE(length))) {
                NPN_ReleaseVariantValue(&length);
                typeError = true;
                break;
            }
            bool ignored;
            size_t numMembers = ToLong(plugin, length, ignored);
            NPN_ReleaseVariantValue(&length);
            qcc::String substr = signature.substr(1, signature.size() - 1);
            if (numMembers != ajn::SignatureUtils::CountCompleteTypes(substr.c_str())) {
                typeError = true;
                break;
            }

            arg.typeId = ajn::ALLJOYN_STRUCT;
            arg.v_struct.numMembers = numMembers;
            arg.v_struct.members = new ajn::MsgArg[arg.v_struct.numMembers];
            arg.SetOwnershipFlags(ajn::MsgArg::OwnsArgs);

            const char* begin = signature.c_str() + 1;
            for (size_t i = 0; i < arg.v_struct.numMembers; ++i) {
                const char* end = begin;
                status = ajn::SignatureUtils::ParseCompleteType(end);
                if (ER_OK != status) {
                    break;
                }
                qcc::String typeSignature(begin, end - begin);
                NPVariant element = NPVARIANT_VOID;
                if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetIntIdentifier(i), &element)) {
                    typeError = true;
                    break;
                }
                ToAny(plugin, element, typeSignature, arg.v_struct.members[i], typeError);
                NPN_ReleaseVariantValue(&element);
                if (typeError) {
                    break;
                }
                begin = end;
            }
            break;
        }

    case ajn::ALLJOYN_ARRAY: {
            if (!NPVARIANT_IS_OBJECT(value)) {
                typeError = true;
                break;
            }
            qcc::String elemSignature = signature.substr(1);

            if (elemSignature[0] == ajn::ALLJOYN_DICT_ENTRY_OPEN) {
                const char* begin = elemSignature.c_str() + 1;
                const char* end = begin;
                status = ajn::SignatureUtils::ParseCompleteType(end);
                if (ER_OK != status) {
                    break;
                }
                qcc::String keySignature(begin, end - begin);
                begin = end;
                status = ajn::SignatureUtils::ParseCompleteType(end);
                if (ER_OK != status) {
                    break;
                }
                qcc::String valueSignature(begin, end - begin);

                NPIdentifier* properties = NULL;
                uint32_t propertiesCount = 0;
                if (!NPN_Enumerate(plugin->npp, NPVARIANT_TO_OBJECT(value), &properties, &propertiesCount)) {
                    typeError = true;
                    break;
                }
                size_t numElements = propertiesCount;
                ajn::MsgArg* elements = new ajn::MsgArg[numElements];

                arg.v_array.SetElements(elemSignature.c_str(), numElements, elements);
                if (ER_OK != status) {
                    NPN_MemFree(properties);
                    delete[] elements;
                    break;
                }
                arg.typeId = ajn::ALLJOYN_ARRAY;
                arg.SetOwnershipFlags(ajn::MsgArg::OwnsArgs);

                for (size_t i = 0; i < numElements; ++i) {
                    NPVariant key;
                    if (NPN_IdentifierIsString(properties[i])) {
                        NPUTF8* utf8 = NPN_UTF8FromIdentifier(properties[i]);
                        ToDOMString(plugin, utf8, strlen(utf8), key);
                        NPN_MemFree(utf8);
                    } else {
                        int32_t n = NPN_IntFromIdentifier(properties[i]);
                        INT32_TO_NPVARIANT(n, key);
                    }
                    NPVariant val;
                    if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), properties[i], &val)) {
                        typeError = true;
                        break;
                    }

                    elements[i].typeId = ajn::ALLJOYN_DICT_ENTRY;
                    elements[i].v_dictEntry.key = new ajn::MsgArg();
                    elements[i].v_dictEntry.val = new ajn::MsgArg();
                    elements[i].SetOwnershipFlags(ajn::MsgArg::OwnsArgs);
                    ToAny(plugin, key, keySignature, *elements[i].v_dictEntry.key, typeError);
                    if (!typeError) {
                        ToAny(plugin, val, valueSignature, *elements[i].v_dictEntry.val, typeError);
                    }
                    NPN_ReleaseVariantValue(&val);
                    NPN_ReleaseVariantValue(&key);
                }

                NPN_MemFree(properties);
            } else {
                NPVariant length = NPVARIANT_VOID;
                if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetStringIdentifier("length"), &length) ||
                    !(NPVARIANT_IS_INT32(length) || NPVARIANT_IS_DOUBLE(length))) {
                    NPN_ReleaseVariantValue(&length);
                    typeError = true;
                    break;
                }
                bool ignored;
                size_t numElements = ToLong(plugin, length, ignored);
                NPN_ReleaseVariantValue(&length);

                switch (elemSignature[0]) {
                case ajn::ALLJOYN_BOOLEAN: {
                        arg.typeId = ajn::ALLJOYN_BOOLEAN_ARRAY;
                        arg.v_scalarArray.numElements = numElements;
                        arg.v_scalarArray.v_bool = new bool[numElements];
                        arg.SetOwnershipFlags(ajn::MsgArg::OwnsData);

                        for (size_t i = 0; i < numElements; ++i) {
                            NPVariant element = NPVARIANT_VOID;
                            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetIntIdentifier(i), &element)) {
                                typeError = true;
                                break;
                            }
                            const_cast<bool*>(arg.v_scalarArray.v_bool)[i] = ToBoolean(plugin, element, typeError);
                            NPN_ReleaseVariantValue(&element);
                            if (typeError) {
                                break;
                            }
                        }
                        break;
                    }

                case ajn::ALLJOYN_BYTE: {
                        arg.typeId = ajn::ALLJOYN_BYTE_ARRAY;
                        arg.v_scalarArray.numElements = numElements;
                        arg.v_scalarArray.v_byte = new uint8_t[numElements];
                        arg.SetOwnershipFlags(ajn::MsgArg::OwnsData);

                        for (size_t i = 0; i < numElements; ++i) {
                            NPVariant element = NPVARIANT_VOID;
                            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetIntIdentifier(i), &element)) {
                                typeError = true;
                                break;
                            }
                            const_cast<uint8_t*>(arg.v_scalarArray.v_byte)[i] = ToOctet(plugin, element, typeError);
                            NPN_ReleaseVariantValue(&element);
                            if (typeError) {
                                break;
                            }
                        }
                        break;
                    }

                case ajn::ALLJOYN_INT32: {
                        arg.typeId = ajn::ALLJOYN_INT32_ARRAY;
                        arg.v_scalarArray.numElements = numElements;
                        arg.v_scalarArray.v_int32 = new int32_t[numElements];
                        arg.SetOwnershipFlags(ajn::MsgArg::OwnsData);

                        for (size_t i = 0; i < numElements; ++i) {
                            NPVariant element = NPVARIANT_VOID;
                            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetIntIdentifier(i), &element)) {
                                typeError = true;
                                break;
                            }
                            const_cast<int32_t*>(arg.v_scalarArray.v_int32)[i] = ToLong(plugin, element, typeError);
                            NPN_ReleaseVariantValue(&element);
                            if (typeError) {
                                break;
                            }
                        }
                        break;
                    }

                case ajn::ALLJOYN_INT16: {
                        arg.typeId = ajn::ALLJOYN_INT16_ARRAY;
                        arg.v_scalarArray.numElements = numElements;
                        arg.v_scalarArray.v_int16 = new int16_t[numElements];
                        arg.SetOwnershipFlags(ajn::MsgArg::OwnsData);

                        for (size_t i = 0; i < numElements; ++i) {
                            NPVariant element = NPVARIANT_VOID;
                            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetIntIdentifier(i), &element)) {
                                typeError = true;
                                break;
                            }
                            const_cast<int16_t*>(arg.v_scalarArray.v_int16)[i] = ToShort(plugin, element, typeError);
                            NPN_ReleaseVariantValue(&element);
                            if (typeError) {
                                break;
                            }
                        }
                        break;
                    }

                case ajn::ALLJOYN_UINT16: {
                        arg.typeId = ajn::ALLJOYN_UINT16_ARRAY;
                        arg.v_scalarArray.numElements = numElements;
                        arg.v_scalarArray.v_uint16 = new uint16_t[numElements];
                        arg.SetOwnershipFlags(ajn::MsgArg::OwnsData);

                        for (size_t i = 0; i < numElements; ++i) {
                            NPVariant element = NPVARIANT_VOID;
                            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetIntIdentifier(i), &element)) {
                                typeError = true;
                                break;
                            }
                            const_cast<uint16_t*>(arg.v_scalarArray.v_uint16)[i] = ToUnsignedShort(plugin, element, typeError);
                            NPN_ReleaseVariantValue(&element);
                            if (typeError) {
                                break;
                            }
                        }
                        break;
                    }

                case ajn::ALLJOYN_UINT64: {
                        arg.typeId = ajn::ALLJOYN_UINT64_ARRAY;
                        arg.v_scalarArray.numElements = numElements;
                        arg.v_scalarArray.v_uint64 = new uint64_t[numElements];
                        arg.SetOwnershipFlags(ajn::MsgArg::OwnsData);

                        for (size_t i = 0; i < numElements; ++i) {
                            NPVariant element = NPVARIANT_VOID;
                            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetIntIdentifier(i), &element)) {
                                typeError = true;
                                break;
                            }
                            const_cast<uint64_t*>(arg.v_scalarArray.v_uint64)[i] = ToUnsignedLongLong(plugin, element, typeError);
                            NPN_ReleaseVariantValue(&element);
                            if (typeError) {
                                break;
                            }
                        }
                        break;
                    }

                case ajn::ALLJOYN_UINT32: {
                        arg.typeId = ajn::ALLJOYN_UINT32_ARRAY;
                        arg.v_scalarArray.numElements = numElements;
                        arg.v_scalarArray.v_uint32 = new uint32_t[numElements];
                        arg.SetOwnershipFlags(ajn::MsgArg::OwnsData);

                        for (size_t i = 0; i < numElements; ++i) {
                            NPVariant element = NPVARIANT_VOID;
                            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetIntIdentifier(i), &element)) {
                                typeError = true;
                                break;
                            }
                            const_cast<uint32_t*>(arg.v_scalarArray.v_uint32)[i] = ToUnsignedLong(plugin, element, typeError);
                            NPN_ReleaseVariantValue(&element);
                            if (typeError) {
                                break;
                            }
                        }
                        break;
                    }

                case ajn::ALLJOYN_INT64: {
                        arg.typeId = ajn::ALLJOYN_INT64_ARRAY;
                        arg.v_scalarArray.numElements = numElements;
                        arg.v_scalarArray.v_int64 = new int64_t[numElements];
                        arg.SetOwnershipFlags(ajn::MsgArg::OwnsData);

                        for (size_t i = 0; i < numElements; ++i) {
                            NPVariant element = NPVARIANT_VOID;
                            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetIntIdentifier(i), &element)) {
                                typeError = true;
                                break;
                            }
                            const_cast<int64_t*>(arg.v_scalarArray.v_int64)[i] = ToLongLong(plugin, element, typeError);
                            NPN_ReleaseVariantValue(&element);
                            if (typeError) {
                                break;
                            }
                        }
                        break;
                    }

                case ajn::ALLJOYN_DOUBLE: {
                        arg.typeId = ajn::ALLJOYN_DOUBLE_ARRAY;
                        arg.v_scalarArray.numElements = numElements;
                        arg.v_scalarArray.v_double = new double[numElements];
                        arg.SetOwnershipFlags(ajn::MsgArg::OwnsData);

                        for (size_t i = 0; i < numElements; ++i) {
                            NPVariant element = NPVARIANT_VOID;
                            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetIntIdentifier(i), &element)) {
                                typeError = true;
                                break;
                            }
                            const_cast<double*>(arg.v_scalarArray.v_double)[i] = ToDouble(plugin, element, typeError);
                            NPN_ReleaseVariantValue(&element);
                            if (typeError) {
                                break;
                            }
                        }
                        break;
                    }

                default: {
                        ajn::MsgArg* elements = new ajn::MsgArg[numElements];
                        status = arg.v_array.SetElements(elemSignature.c_str(), numElements, elements);
                        if (ER_OK != status) {
                            delete[] elements;
                            break;
                        }
                        arg.typeId = ajn::ALLJOYN_ARRAY;
                        arg.SetOwnershipFlags(ajn::MsgArg::OwnsArgs);

                        for (size_t i = 0; i < numElements; ++i) {
                            NPVariant element = NPVARIANT_VOID;
                            if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetIntIdentifier(i), &element)) {
                                typeError = true;
                                break;
                            }
                            ToAny(plugin, element, elemSignature, elements[i], typeError);
                            NPN_ReleaseVariantValue(&element);
                            if (typeError) {
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            break;
        }

    case ajn::ALLJOYN_VARIANT: {
            if (!NPVARIANT_IS_OBJECT(value)) {
                typeError = true;
                break;
            }

            NPIdentifier* properties = NULL;
            uint32_t propertiesCount = 0;
            if (!NPN_Enumerate(plugin->npp, NPVARIANT_TO_OBJECT(value), &properties, &propertiesCount)) {
                typeError = true;
                break;
            }
            if (1 != propertiesCount) {
                NPN_MemFree(properties);
                typeError = true;
                break;
            }

            arg.typeId = ajn::ALLJOYN_VARIANT;
            arg.v_variant.val = new ajn::MsgArg;
            arg.SetOwnershipFlags(ajn::MsgArg::OwnsArgs);

            NPUTF8* utf8 = NPN_UTF8FromIdentifier(properties[0]);
            qcc::String signature(utf8);
            NPVariant val;
            if (NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), properties[0], &val)) {
                ToAny(plugin, val, signature, *arg.v_variant.val, typeError);
                NPN_ReleaseVariantValue(&val);
            } else {
                typeError = true;
            }

            NPN_MemFree(utf8);
            NPN_MemFree(properties);
            break;
        }

    case ajn::ALLJOYN_HANDLE: {
            qcc::SocketFd fd = qcc::INVALID_SOCKET_FD;

            switch (value.type) {
            case NPVariantType_Void:
            case NPVariantType_Null:
                break;

            case NPVariantType_Bool:
                typeError = true;
                break;

            case NPVariantType_Int32:
                fd = NPVARIANT_TO_INT32(value);
                break;

            case NPVariantType_Double:
                fd = static_cast<qcc::SocketFd>(NPVARIANT_TO_DOUBLE(value));
                break;

            case NPVariantType_String: {
                    NPObject* window = NULL;
                    if (NPERR_NO_ERROR == NPN_GetValue(plugin->npp, NPNVWindowNPObject, &window)) {
                        NPVariant result;
                        if (NPN_Invoke(plugin->npp, window, NPN_GetStringIdentifier("parseFloat"), &value, 1, &result)) {
                            if (NPVARIANT_IS_INT32(result)) {
                                fd = NPVARIANT_TO_INT32(result);
                            } else if (NPVARIANT_IS_DOUBLE(result)) {
                                fd = static_cast<qcc::SocketFd>(NPVARIANT_TO_DOUBLE(result));
                            }
                            NPN_ReleaseVariantValue(&result);
                        }
                        NPN_ReleaseObject(window);
                    }
                    break;
                }

            case NPVariantType_Object: {
                    SocketFdHost* socketFd = ToHostObject<SocketFdHost>(plugin, value, typeError);
                    if (typeError || !socketFd) {
                        typeError = true;
                        break;
                    }
                    fd = (*socketFd)->GetFd();
                    break;
                }

            default:
                assert(0); /* Will not reach here. */
                typeError = true;
                break;
            }

            if (!typeError) {
                arg.Set("h", fd);
            }
            break;
        }

    default:
        status = ER_BUS_BAD_SIGNATURE;
        break;
    }

    if ((ER_OK != status) || typeError) {
        QCC_LogError(status, ("ToAny failed typeError=%d", typeError));
        typeError = true;
    }
}

void ToAny(Plugin& plugin, const ajn::MsgArg& value, NPVariant& variant, QStatus& status)
{
    status = ER_OK;
    switch (value.typeId) {
    case ajn::ALLJOYN_BOOLEAN:
        ToBoolean(plugin, value.v_bool, variant);
        break;

    case ajn::ALLJOYN_BYTE:
        ToOctet(plugin, value.v_byte, variant);
        break;

    case ajn::ALLJOYN_INT16:
        ToShort(plugin, value.v_int16, variant);
        break;

    case ajn::ALLJOYN_UINT16:
        ToUnsignedShort(plugin, value.v_uint16, variant);
        break;

    case ajn::ALLJOYN_INT32:
        ToLong(plugin, value.v_int32, variant);
        break;

    case ajn::ALLJOYN_UINT32:
        ToUnsignedLong(plugin, value.v_uint32, variant);
        break;

    case ajn::ALLJOYN_INT64:
        ToLongLong(plugin, value.v_int64, variant);
        break;

    case ajn::ALLJOYN_UINT64:
        ToUnsignedLongLong(plugin, value.v_uint64, variant);
        break;

    case ajn::ALLJOYN_DOUBLE:
        ToDouble(plugin, value.v_double, variant);
        break;

    case ajn::ALLJOYN_STRING:
        ToDOMString(plugin, value.v_string.str, value.v_string.len, variant);
        break;

    case ajn::ALLJOYN_OBJECT_PATH:
        ToDOMString(plugin, value.v_objPath.str, value.v_objPath.len, variant);
        break;

    case ajn::ALLJOYN_SIGNATURE:
        ToDOMString(plugin, value.v_signature.sig, value.v_objPath.len, variant);
        break;

    case ajn::ALLJOYN_STRUCT: {
            if (NewArray(plugin, variant)) {
                for (size_t i = 0; (ER_OK == status) && (i < value.v_struct.numMembers); ++i) {
                    NPVariant element = NPVARIANT_VOID;
                    ToAny(plugin, value.v_struct.members[i], element, status);
                    if ((ER_OK == status) &&
                        !NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &element)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                    }
                    NPN_ReleaseVariantValue(&element);
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                VOID_TO_NPVARIANT(variant);
            }
            break;
        }

    case ajn::ALLJOYN_VARIANT: {
            if (ajn::ALLJOYN_VARIANT != value.v_variant.val->typeId) {
                ToAny(plugin, *value.v_variant.val, variant, status);
            } else if (NewObject(plugin, variant)) {
                qcc::String nestedSignature = value.v_variant.val->v_variant.val->Signature();
                NPVariant nestedValue = NPVARIANT_VOID;
                ToAny(plugin, *value.v_variant.val->v_variant.val, nestedValue, status);
                if ((ER_OK == status) &&
                    !NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetStringIdentifier(nestedSignature.c_str()), &nestedValue)) {
                    status = ER_FAIL;
                    QCC_LogError(status, ("NPN_SetProperty failed"));
                }
                NPN_ReleaseVariantValue(&nestedValue);
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("NewObject failed"));
                VOID_TO_NPVARIANT(variant);
            }
            break;
        }

    case ajn::ALLJOYN_ARRAY: {
            if (value.v_array.GetElemSig()[0] == ajn::ALLJOYN_DICT_ENTRY_OPEN) {
                if (NewObject(plugin, variant)) {
                    for (size_t i = 0; (ER_OK == status) && (i < value.v_array.GetNumElements()); ++i) {
                        NPIdentifier key = 0;
                        NPVariant val = NPVARIANT_VOID;
                        ToDictionaryKey(plugin, *value.v_array.GetElements()[i].v_dictEntry.key, key);
                        ToAny(plugin, *value.v_array.GetElements()[i].v_dictEntry.val, val, status);
                        if ((ER_OK == status) &&
                            !NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), key, &val)) {
                            status = ER_FAIL;
                            QCC_LogError(status, ("NPN_SetProperty failed"));
                        }
                        NPN_ReleaseVariantValue(&val);
                    }
                } else {
                    status = ER_FAIL;
                    QCC_LogError(status, ("NewObject failed"));
                    VOID_TO_NPVARIANT(variant);
                }
            } else {
                if (NewArray(plugin, variant)) {
                    for (size_t i = 0; (ER_OK == status) && (i < value.v_array.GetNumElements()); ++i) {
                        NPVariant element = NPVARIANT_VOID;
                        ToAny(plugin, value.v_array.GetElements()[i], element, status);
                        if ((ER_OK == status) &&
                            !NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &element)) {
                            status = ER_FAIL;
                            QCC_LogError(status, ("NPN_SetProperty failed"));
                        }
                        NPN_ReleaseVariantValue(&element);
                    }
                } else {
                    status = ER_FAIL;
                    QCC_LogError(status, ("NewArray failed"));
                    VOID_TO_NPVARIANT(variant);
                }
            }
            break;
        }


    case ajn::ALLJOYN_BOOLEAN_ARRAY: {
            if (NewArray(plugin, variant)) {
                for (size_t i = 0; (ER_OK == status) && (i < value.v_scalarArray.numElements); ++i) {
                    NPVariant element = NPVARIANT_VOID;
                    ToBoolean(plugin, value.v_scalarArray.v_bool[i], element);
                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &element)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                    }
                    NPN_ReleaseVariantValue(&element);
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                VOID_TO_NPVARIANT(variant);
            }
            break;
        }

    case ajn::ALLJOYN_DOUBLE_ARRAY: {
            if (NewArray(plugin, variant)) {
                for (size_t i = 0; (ER_OK == status) && (i < value.v_scalarArray.numElements); ++i) {
                    NPVariant element = NPVARIANT_VOID;
                    ToDouble(plugin, value.v_scalarArray.v_double[i], element);
                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &element)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                    }
                    NPN_ReleaseVariantValue(&element);
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                VOID_TO_NPVARIANT(variant);
            }
            break;
        }

    case ajn::ALLJOYN_INT32_ARRAY: {
            if (NewArray(plugin, variant)) {
                for (size_t i = 0; (ER_OK == status) && (i < value.v_scalarArray.numElements); ++i) {
                    NPVariant element = NPVARIANT_VOID;
                    ToLong(plugin, value.v_scalarArray.v_int32[i], element);
                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &element)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                    }
                    NPN_ReleaseVariantValue(&element);
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                VOID_TO_NPVARIANT(variant);
            }
            break;
        }

    case ajn::ALLJOYN_INT16_ARRAY: {
            if (NewArray(plugin, variant)) {
                for (size_t i = 0; (ER_OK == status) && (i < value.v_scalarArray.numElements); ++i) {
                    NPVariant element = NPVARIANT_VOID;
                    ToShort(plugin, value.v_scalarArray.v_int16[i], element);
                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &element)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                    }
                    NPN_ReleaseVariantValue(&element);
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                VOID_TO_NPVARIANT(variant);
            }
            break;
        }

    case ajn::ALLJOYN_UINT16_ARRAY: {
            if (NewArray(plugin, variant)) {
                for (size_t i = 0; (ER_OK == status) && (i < value.v_scalarArray.numElements); ++i) {
                    NPVariant element = NPVARIANT_VOID;
                    ToUnsignedShort(plugin, value.v_scalarArray.v_uint16[i], element);
                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &element)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                    }
                    NPN_ReleaseVariantValue(&element);
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                VOID_TO_NPVARIANT(variant);
            }
            break;
        }

    case ajn::ALLJOYN_UINT64_ARRAY: {
            if (NewArray(plugin, variant)) {
                for (size_t i = 0; (ER_OK == status) && (i < value.v_scalarArray.numElements); ++i) {
                    NPVariant element = NPVARIANT_VOID;
                    ToUnsignedLongLong(plugin, value.v_scalarArray.v_uint64[i], element);
                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &element)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                    }
                    NPN_ReleaseVariantValue(&element);
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                VOID_TO_NPVARIANT(variant);
            }
            break;
        }

    case ajn::ALLJOYN_UINT32_ARRAY: {
            if (NewArray(plugin, variant)) {
                for (size_t i = 0; (ER_OK == status) && (i < value.v_scalarArray.numElements); ++i) {
                    NPVariant element = NPVARIANT_VOID;
                    ToUnsignedLong(plugin, value.v_scalarArray.v_uint32[i], element);
                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &element)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                    }
                    NPN_ReleaseVariantValue(&element);
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                VOID_TO_NPVARIANT(variant);
            }
            break;
        }

    case ajn::ALLJOYN_INT64_ARRAY: {
            if (NewArray(plugin, variant)) {
                for (size_t i = 0; (ER_OK == status) && (i < value.v_scalarArray.numElements); ++i) {
                    NPVariant element = NPVARIANT_VOID;
                    ToLongLong(plugin, value.v_scalarArray.v_int64[i], element);
                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &element)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                    }
                    NPN_ReleaseVariantValue(&element);
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                VOID_TO_NPVARIANT(variant);
            }
            break;
        }

    case ajn::ALLJOYN_BYTE_ARRAY: {
            if (NewArray(plugin, variant)) {
                for (size_t i = 0; (ER_OK == status) && (i < value.v_scalarArray.numElements); ++i) {
                    NPVariant element = NPVARIANT_VOID;
                    ToOctet(plugin, value.v_scalarArray.v_byte[i], element);
                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(variant), NPN_GetIntIdentifier(i), &element)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                    }
                    NPN_ReleaseVariantValue(&element);
                }
            } else {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                VOID_TO_NPVARIANT(variant);
            }
            break;
        }

    case ajn::ALLJOYN_HANDLE: {
            qcc::SocketFd fd;
            status = qcc::SocketDup(value.v_handle.fd, fd);
            if (ER_OK == status) {
                SocketFdHost socketFdHost(plugin, fd);
                ToHostObject<SocketFdHost>(plugin, socketFdHost, variant);
            }
            break;
        }

    default:
        /* Should never reach this case. */
        assert(0);
        QCC_LogError(ER_FAIL, ("Unhandled MsgArg type: 0x%x", value.typeId));
        break;
    }
}

void ToBoolean(Plugin& plugin, bool value, NPVariant& variant)
{
    BOOLEAN_TO_NPVARIANT(value, variant);
}

uint8_t ToOctet(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    typeError = false;
    return ToInteger<uint8_t>(plugin, value, typeError);
}

void ToOctet(Plugin& plugin, uint8_t value, NPVariant& variant)
{
    INT32_TO_NPVARIANT(value, variant);
}

int16_t ToShort(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    typeError = false;
    return ToInteger<int16_t>(plugin, value, typeError);
}

void ToShort(Plugin& plugin, int16_t value, NPVariant& variant)
{
    INT32_TO_NPVARIANT(value, variant);
}

uint16_t ToUnsignedShort(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    typeError = false;
    return ToInteger<uint16_t>(plugin, value, typeError);
}

void ToUnsignedShort(Plugin& plugin, uint16_t value, NPVariant& variant)
{
    INT32_TO_NPVARIANT(value, variant);
}

int32_t ToLong(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    typeError = false;
    return ToInteger<int32_t>(plugin, value, typeError);
}

void ToLong(Plugin& plugin, int32_t value, NPVariant& variant)
{
    INT32_TO_NPVARIANT(value, variant);
}

uint32_t ToUnsignedLong(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    typeError = false;
    return ToInteger<uint32_t>(plugin, value, typeError);
}

void ToUnsignedLong(Plugin& plugin, uint32_t value, NPVariant& variant)
{
    DOUBLE_TO_NPVARIANT(value, variant);
}

int64_t ToLongLong(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    typeError = false;
    if (value.type == NPVariantType_String) {
        /*
         * Special processing for int64 to allow larger precision than JS allows by converting string
         * values directly to int64 instead of first to doubles.
         */
        qcc::String s = NPVARIANT_TO_STRING(value).UTF8Length ? qcc::String(NPVARIANT_TO_STRING(value).UTF8Characters, NPVARIANT_TO_STRING(value).UTF8Length) : qcc::String();
        const char* nptr = s.c_str();
        char* endptr;
        int64_t x = strtoll(nptr, &endptr, 0);
        typeError = (endptr == nptr);
        return x;
    } else {
        return ToInteger<int64_t>(plugin, value, typeError);
    }
}

void ToLongLong(Plugin& plugin, int64_t value, NPVariant& variant)
{
    /*
     * Special processing for int64 to allow larger precision than JS allows by converting int64
     * values to strings instead of first to doubles.
     */
    char val[32];
    snprintf(val, 32, "%" PRIi64, value);
    ToDOMString(plugin, val, strlen(val), variant);
}

uint64_t ToUnsignedLongLong(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    typeError = false;
    if (value.type == NPVariantType_String) {
        /*
         * Special processing for uint64 to allow larger precision than JS allows by converting string
         * values directly to uint64 instead of first to doubles.
         */
        qcc::String s = NPVARIANT_TO_STRING(value).UTF8Length ? qcc::String(NPVARIANT_TO_STRING(value).UTF8Characters, NPVARIANT_TO_STRING(value).UTF8Length) : qcc::String();
        const char* nptr = s.c_str();
        char* endptr;
        uint64_t x = strtoull(nptr, &endptr, 0);
        typeError = (endptr == nptr);
        return x;
    } else {
        return ToInteger<uint64_t>(plugin, value, typeError);
    }
}

void ToUnsignedLongLong(Plugin& plugin, uint64_t value, NPVariant& variant)
{
    /*
     * Special processing for uint64 to allow larger precision than JS allows by converting int64
     * values to strings instead of first to doubles.
     */
    char val[32];
    snprintf(val, 32, "%" PRIu64, value);
    ToDOMString(plugin, val, strlen(val), variant);
}

double ToDouble(Plugin& plugin, const NPVariant& value, bool& typeError)
{
    typeError = false;
    return ToNumber(plugin, value, typeError);
}

void ToDouble(Plugin& plugin, double value, NPVariant& variant)
{
    DOUBLE_TO_NPVARIANT(value, variant);
}

qcc::String ToDOMString(Plugin& plugin, const NPVariant& value, bool& typeError,
                        bool treatNullAsEmptyString, bool treatUndefinedAsEmptyString)
{
    typeError = false;
    if (NPVARIANT_IS_VOID(value) && treatUndefinedAsEmptyString) {
        return qcc::String();
    }
    if (NPVARIANT_IS_NULL(value) && treatNullAsEmptyString) {
        return qcc::String();
    }
    return ToString(plugin, value, typeError);
}

void ToDOMString(Plugin& plugin, qcc::String value, NPVariant& variant,
                 TreatEmptyStringAs treatEmptyStringAs)
{
    if (value.empty()) {
        switch (treatEmptyStringAs) {
        case TreatEmptyStringAsNull:
            NULL_TO_NPVARIANT(variant);
            return;

        case TreatEmptyStringAsUndefined:
            VOID_TO_NPVARIANT(variant);
            return;

        default:
            break;
        }
    }
    char* val = reinterpret_cast<char*>(NPN_MemAlloc(value.size() + 1));
    strncpy(val, value.c_str(), value.size() + 1);
    STRINGZ_TO_NPVARIANT(val, variant);
}

void ToDOMString(Plugin& plugin, const char* str, uint32_t len, NPVariant& variant,
                 TreatEmptyStringAs treatEmptyStringAs)
{
    if (!str || (0 == len)) {
        switch (treatEmptyStringAs) {
        case TreatEmptyStringAsNull:
            NULL_TO_NPVARIANT(variant);
            return;

        case TreatEmptyStringAsUndefined:
            VOID_TO_NPVARIANT(variant);
            return;

        default:
            break;
        }
    }
    char* val = reinterpret_cast<char*>(NPN_MemAlloc(len + 1));
    strncpy(val, str, len + 1);
    STRINGZ_TO_NPVARIANT(val, variant);
}
