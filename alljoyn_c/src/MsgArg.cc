/**
 * @file
 *
 * This file implements the MsgArg class
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>

#include <cstdarg>
#include <assert.h>

#include <stdio.h>

#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn_c/Message.h>
#include <alljoyn_c/MsgArg.h>
#include "MsgArgC.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_C"

struct _alljoyn_msgarg_handle {
    /* Empty by design, this is just to allow the type restrictions to save coders from themselves */
};

alljoyn_msgarg AJ_CALL alljoyn_msgarg_create()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::MsgArgC* arg = new ajn::MsgArgC[1];
    return (alljoyn_msgarg)arg;
}

alljoyn_msgarg AJ_CALL alljoyn_msgarg_create_and_set(const char* signature, ...)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::MsgArgC* arg = new ajn::MsgArgC[1];
    arg->typeId = ajn::ALLJOYN_INVALID;
    va_list argp;
    va_start(argp, signature);
    QStatus status = ER_OK;

    ((ajn::MsgArgC*)arg)->Clear();
    size_t sigLen = (signature ? strlen(signature) : 0);
    if ((sigLen < 1) || (sigLen > 255)) {
        status = ER_BUS_BAD_SIGNATURE;
    } else {
        status = ((ajn::MsgArgC*)arg)->VBuildArgsC(signature, sigLen, ((ajn::MsgArgC*)arg), 1, &argp);
        if ((status == ER_OK) && (*signature != 0)) {
            status = ER_BUS_NOT_A_COMPLETE_TYPE;
        }
    }
    va_end(argp);
    return (alljoyn_msgarg)arg;
}

void AJ_CALL alljoyn_msgarg_destroy(alljoyn_msgarg arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (arg != NULL) {
        delete [] (ajn::MsgArgC*)arg;
    }
}

alljoyn_msgarg AJ_CALL alljoyn_msgarg_array_create(size_t size)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ajn::MsgArgC* args = new ajn::MsgArgC[size];
    for (size_t i = 0; i < size; i++) {
        args[i].Clear();
    }
    return (alljoyn_msgarg)args;
}

alljoyn_msgarg AJ_CALL alljoyn_msgarg_array_element(alljoyn_msgarg arg, size_t index)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!arg) {
        return NULL;
    }
    ajn::MsgArgC* array_arg = (ajn::MsgArgC*)arg;
    return (alljoyn_msgarg)(&array_arg[index]);
}

QStatus AJ_CALL alljoyn_msgarg_set(alljoyn_msgarg arg, const char* signature, ...)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!arg) {
        return ER_BAD_ARG_1;
    }
    va_list argp;
    va_start(argp, signature);
    QStatus status = ER_OK;

    ((ajn::MsgArgC*)arg)->Clear();
    size_t sigLen = (signature ? strlen(signature) : 0);
    if ((sigLen < 1) || (sigLen > 255)) {
        status = ER_BUS_BAD_SIGNATURE;
    } else {
        status = ((ajn::MsgArgC*)arg)->VBuildArgsC(signature, sigLen, ((ajn::MsgArgC*)arg), 1, &argp);
        if ((status == ER_OK) && (*signature != 0)) {
            status = ER_BUS_NOT_A_COMPLETE_TYPE;
        }
    }
    va_end(argp);
    return status;
}

QStatus AJ_CALL alljoyn_msgarg_get(alljoyn_msgarg arg, const char* signature, ...)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!arg) {
        return ER_BAD_ARG_1;
    }
    size_t sigLen = (signature ? strlen(signature) : 0);
    if (sigLen == 0) {
        return ER_BAD_ARG_2;
    }
    va_list argp;
    va_start(argp, signature);
    QStatus status = ((ajn::MsgArgC*)arg)->VParseArgsC(signature, sigLen, ((ajn::MsgArgC*)arg), 1, &argp);
    va_end(argp);
    return status;
}

alljoyn_msgarg AJ_CALL alljoyn_msgarg_copy(const alljoyn_msgarg source)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!source) {
        return NULL;
    }
    ajn::MsgArgC* ret = new ajn::MsgArgC[1];
    *ret = *(ajn::MsgArgC*)source;
    return (alljoyn_msgarg) ret;
}

void AJ_CALL alljoyn_msgarg_clone(alljoyn_msgarg destination, const alljoyn_msgarg source)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    (*(ajn::MsgArgC*)destination) = *(ajn::MsgArgC*)source;
}

QCC_BOOL AJ_CALL alljoyn_msgarg_equal(alljoyn_msgarg lhv, alljoyn_msgarg rhv)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!lhv || !rhv) {
        return QCC_FALSE;
    }
    return (*(ajn::MsgArgC*)lhv) == (*(ajn::MsgArgC*)rhv);
}

QStatus AJ_CALL alljoyn_msgarg_array_set(alljoyn_msgarg args, size_t* numArgs, const char* signature, ...)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!args) {
        return ER_BAD_ARG_1;
    }
    va_list argp;
    va_start(argp, signature);
    QStatus status = ((ajn::MsgArgC*)args)->MsgArgUtilsSetVC(((ajn::MsgArgC*)args), *numArgs, signature, &argp);
    va_end(argp);
    return status;
}

QStatus AJ_CALL alljoyn_msgarg_array_get(const alljoyn_msgarg args, size_t numArgs, const char* signature, ...)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!args) {
        return ER_BAD_ARG_1;
    }
    if (!numArgs) {
        return ER_BAD_ARG_2;
    }
    size_t sigLen = (signature ? strlen(signature) : 0);
    if (sigLen == 0) {
        return ER_BAD_ARG_3;
    }
    va_list argp;
    va_start(argp, signature);
    QStatus status = ((ajn::MsgArgC*)args)->VParseArgsC(signature, sigLen, ((ajn::MsgArgC*)args), numArgs, &argp);
    va_end(argp);
    return status;
}

size_t AJ_CALL alljoyn_msgarg_tostring(alljoyn_msgarg arg, char* str, size_t buf, size_t indent)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!arg) {
        return (size_t)0;
    }
    qcc::String s = ((ajn::MsgArgC*)arg)->ToString(indent);
    /*
     * it is ok to send in NULL for str when the user is only interested in the
     * size of the resulting string.
     */
    if (str) {
        strncpy(str, s.c_str(), buf);
        str[buf - 1] = '\0'; //prevent sting not being null terminated.
    }
    return s.size() + 1;
}

size_t AJ_CALL alljoyn_msgarg_array_tostring(const alljoyn_msgarg args, size_t numArgs, char* str, size_t buf, size_t indent)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!args) {
        return (size_t)0;
    }
    qcc::String s = ((ajn::MsgArgC*)args)->ToString((ajn::MsgArgC*)args, numArgs, indent);
    /*
     * it is ok to send in NULL for str when the user is only interested in the
     * size of the resulting string.
     */
    if (str) {
        strncpy(str, s.c_str(), buf);
        str[buf - 1] = '\0'; //prevent sting not being null terminated.
    }
    return s.size() + 1;
}

size_t AJ_CALL alljoyn_msgarg_signature(alljoyn_msgarg arg, char* str, size_t buf)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!arg) {
        return (size_t)0;
    }
    qcc::String s = ((ajn::MsgArgC*)arg)->Signature();
    /*
     * it is ok to send in NULL for str when the user is only interested in the
     * size of the resulting string.
     */
    if (str) {
        strncpy(str, s.c_str(), buf);
        str[buf - 1] = '\0'; //prevent sting not being null terminated.
    }
    return s.size() + 1;
}

size_t AJ_CALL alljoyn_msgarg_array_signature(alljoyn_msgarg values, size_t numValues, char* str, size_t buf)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!values) {
        return (size_t)0;
    }
    qcc::String s = ((ajn::MsgArgC*)values)->Signature((ajn::MsgArgC*)values, numValues).c_str();
    /*
     * it is ok to send in NULL for str when the user is only interested in the
     * size of the resulting string.
     */
    if (str) {
        strncpy(str, s.c_str(), buf);
        str[buf - 1] = '\0'; //prevent sting not being null terminated.
    }
    return s.size() + 1;

}

QCC_BOOL AJ_CALL alljoyn_msgarg_hassignature(alljoyn_msgarg arg, const char* signature)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!arg) {
        return QCC_FALSE;
    }
    return ((ajn::MsgArgC*)arg)->HasSignature(signature);
}

QStatus AJ_CALL alljoyn_msgarg_getdictelement(alljoyn_msgarg arg, const char* elemSig, ...)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!arg) {
        return ER_BAD_ARG_1;
    }
    size_t sigLen = (elemSig ? strlen(elemSig) : 0);
    if (sigLen < 4) {
        return ER_BAD_ARG_2;
    }

    /* Check this MsgArg is an array of dictionary entries */
    if ((((ajn::MsgArgC*)arg)->typeId != ajn::ALLJOYN_ARRAY || ((ajn::MsgArgC*)arg)->v_array.GetElemSig()[0] != '{')) {
        return ER_BUS_NOT_A_DICTIONARY;
    }
    /* Check key has right type */
    if (((ajn::MsgArgC*)arg)->v_array.GetElemSig()[1] != elemSig[1]) {
        return ER_BUS_SIGNATURE_MISMATCH;
    }
    va_list argp;
    va_start(argp, elemSig);
    /* Get the key as a MsgArg */
    ajn::MsgArgC key;
    size_t numArgs;
    ++elemSig;
    QStatus status = ((ajn::MsgArgC*)arg)->VBuildArgsC(elemSig, 1, &key, 1, &argp, &numArgs);
    if (status == ER_OK) {
        status = ER_BUS_ELEMENT_NOT_FOUND;
        /* Linear search to match the key */
        const ajn::MsgArg* entry = ((ajn::MsgArgC*)arg)->v_array.GetElements();
        for (size_t i = 0; i < ((ajn::MsgArgC*)arg)->v_array.GetNumElements(); ++i, ++entry) {
            if (*entry->v_dictEntry.key == key) {
                status = ER_OK;
                break;
            }
        }
        if (status == ER_OK) {
            status = ((ajn::MsgArgC*)arg)->VParseArgsC(elemSig, sigLen - 3, entry->v_dictEntry.val, 1, &argp);
        }
    }
    va_end(argp);
    return status;
}

void AJ_CALL alljoyn_msgarg_clear(alljoyn_msgarg arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!arg) {
        return;
    }
    ((ajn::MsgArgC*)arg)->Clear();
}

alljoyn_typeid AJ_CALL alljoyn_msgarg_gettype(alljoyn_msgarg arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!arg) {
        return ALLJOYN_INVALID;
    }
    return (alljoyn_typeid)((ajn::MsgArgC*)arg)->typeId;
}

void AJ_CALL alljoyn_msgarg_stabilize(alljoyn_msgarg arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!arg) {
        return;
    }
    ((ajn::MsgArgC*)arg)->Stabilize();
}

/*******************************************************************************
 * This set of functions were originally designed for the alljoyn_unity bindings
 * however they did not not properly map with the C++ MsgArg Class.
 *
 * No Functions below this point should be used by any code except for the
 * AllJoyn Unity Extension. The functions could be changed at any time.
 ******************************************************************************/

QStatus AJ_CALL alljoyn_msgarg_array_set_offset(alljoyn_msgarg args, size_t argOffset, size_t* numArgs, const char* signature, ...)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    va_list argp;
    va_start(argp, signature);
    QStatus status = ((ajn::MsgArgC*)args)->MsgArgUtilsSetVC(((ajn::MsgArgC*)args) + argOffset, *numArgs, signature, &argp);
    va_end(argp);
    return status;
}

QStatus AJ_CALL alljoyn_msgarg_set_and_stabilize(alljoyn_msgarg arg, const char* signature, ...) {
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (!arg) {
        return ER_BAD_ARG_1;
    }
    va_list argp;
    va_start(argp, signature);
    QStatus status = ER_OK;

    ((ajn::MsgArgC*)arg)->Clear();
    size_t sigLen = (signature ? strlen(signature) : 0);
    if ((sigLen < 1) || (sigLen > 255)) {
        status = ER_BUS_BAD_SIGNATURE;
    } else {
        status = ((ajn::MsgArgC*)arg)->VBuildArgsC(signature, sigLen, ((ajn::MsgArgC*)arg), 1, &argp);
        if ((status == ER_OK) && (*signature != 0)) {
            status = ER_BUS_NOT_A_COMPLETE_TYPE;
        }
    }
    va_end(argp);
    ((ajn::MsgArgC*)arg)->Stabilize();
    return status;
}

QStatus AJ_CALL alljoyn_msgarg_set_uint8(alljoyn_msgarg arg, uint8_t y)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "y", y);
}
QStatus AJ_CALL alljoyn_msgarg_set_bool(alljoyn_msgarg arg, QCC_BOOL b)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "b", b);
}
QStatus AJ_CALL alljoyn_msgarg_set_int16(alljoyn_msgarg arg, int16_t n)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "n", n);
}
QStatus AJ_CALL alljoyn_msgarg_set_uint16(alljoyn_msgarg arg, uint16_t q)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "q", q);
}
QStatus AJ_CALL alljoyn_msgarg_set_int32(alljoyn_msgarg arg, int32_t i)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "i", i);
}
QStatus AJ_CALL alljoyn_msgarg_set_uint32(alljoyn_msgarg arg, uint32_t u)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "u", u);
}
QStatus AJ_CALL alljoyn_msgarg_set_int64(alljoyn_msgarg arg, int64_t x)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "x", x);
}
QStatus AJ_CALL alljoyn_msgarg_set_uint64(alljoyn_msgarg arg, uint64_t t)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "t", t);
}
QStatus AJ_CALL alljoyn_msgarg_set_double(alljoyn_msgarg arg, double d)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "d", d);
}
QStatus AJ_CALL alljoyn_msgarg_set_string(alljoyn_msgarg arg, const char* s)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "s", s);
}
QStatus AJ_CALL alljoyn_msgarg_set_objectpath(alljoyn_msgarg arg, const char* o)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "o", o);
}
QStatus AJ_CALL alljoyn_msgarg_set_signature(alljoyn_msgarg arg, const char* g)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "g", g);
}

QStatus AJ_CALL alljoyn_msgarg_get_uint8(const alljoyn_msgarg arg, uint8_t* y)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "y", y);
}
QStatus AJ_CALL alljoyn_msgarg_get_bool(const alljoyn_msgarg arg, QCC_BOOL* b)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "b", b);
}
QStatus AJ_CALL alljoyn_msgarg_get_int16(const alljoyn_msgarg arg, int16_t* n)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "n", n);
}
QStatus AJ_CALL alljoyn_msgarg_get_uint16(const alljoyn_msgarg arg, uint16_t* q)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "q", q);
}
QStatus AJ_CALL alljoyn_msgarg_get_int32(const alljoyn_msgarg arg, int32_t* i)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "i", i);
}
QStatus AJ_CALL alljoyn_msgarg_get_uint32(const alljoyn_msgarg arg, uint32_t* u)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "u", u);
}
QStatus AJ_CALL alljoyn_msgarg_get_int64(const alljoyn_msgarg arg, int64_t* x)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "x", x);
}
QStatus AJ_CALL alljoyn_msgarg_get_uint64(const alljoyn_msgarg arg, uint64_t* t)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "t", t);
}
QStatus AJ_CALL alljoyn_msgarg_get_double(const alljoyn_msgarg arg, double* d)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "d", d);
}
QStatus AJ_CALL alljoyn_msgarg_get_string(const alljoyn_msgarg arg, char** s)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "s", s);
}
QStatus AJ_CALL alljoyn_msgarg_get_objectpath(const alljoyn_msgarg arg, char** o)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "o", o);
}
QStatus AJ_CALL alljoyn_msgarg_get_signature(const alljoyn_msgarg arg, char** g)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "g", g);
}

QStatus AJ_CALL alljoyn_msgarg_get_variant(const alljoyn_msgarg arg, alljoyn_msgarg v)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "v", v);
}

/*
 * MsgArg set function for arrays of each basic data type
 */
QStatus AJ_CALL alljoyn_msgarg_set_uint8_array(alljoyn_msgarg arg, size_t length, uint8_t* ay)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "ay", length, ay);
}
QStatus AJ_CALL alljoyn_msgarg_set_bool_array(alljoyn_msgarg arg, size_t length, QCC_BOOL* ab)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "ab", length, ab);
}
QStatus AJ_CALL alljoyn_msgarg_set_int16_array(alljoyn_msgarg arg, size_t length, int16_t* an)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "an", length, an);
}
QStatus AJ_CALL alljoyn_msgarg_set_uint16_array(alljoyn_msgarg arg, size_t length, uint16_t* aq)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "aq", length, aq);
}
QStatus AJ_CALL alljoyn_msgarg_set_int32_array(const alljoyn_msgarg arg, size_t length, int32_t* ai)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "ai", length, ai);
}
QStatus AJ_CALL alljoyn_msgarg_set_uint32_array(alljoyn_msgarg arg, size_t length, uint32_t* au)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "au", length, au);
}
QStatus AJ_CALL alljoyn_msgarg_set_int64_array(alljoyn_msgarg arg, size_t length, int64_t* ax)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "ax", length, ax);
}
QStatus AJ_CALL alljoyn_msgarg_set_uint64_array(alljoyn_msgarg arg, size_t length, uint64_t* at)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "at", length, at);
}
QStatus AJ_CALL alljoyn_msgarg_set_double_array(alljoyn_msgarg arg, size_t length, double* ad)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_set(arg, "ad", length, ad);
}
QStatus AJ_CALL alljoyn_msgarg_set_string_array(alljoyn_msgarg arg, size_t length, const char** as)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    QStatus status = alljoyn_msgarg_set(arg, "as", length, as);
    alljoyn_msgarg_stabilize(arg);
    return status;
}
QStatus AJ_CALL alljoyn_msgarg_set_objectpath_array(alljoyn_msgarg arg, size_t length, const char** ao)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    QStatus status = alljoyn_msgarg_set(arg, "ao", length, ao);
    alljoyn_msgarg_stabilize(arg);
    return status;
}
QStatus AJ_CALL alljoyn_msgarg_set_signature_array(alljoyn_msgarg arg, size_t length, const char** ag)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    QStatus status = alljoyn_msgarg_set(arg, "ag", length, ag);
    alljoyn_msgarg_stabilize(arg);
    return status;
}

/*
 * MsgArg get funtion for arrays of each basic data type
 */
QStatus AJ_CALL alljoyn_msgarg_get_uint8_array(const alljoyn_msgarg arg, size_t* length, uint8_t* ay)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "ay", length, ay);
}
QStatus AJ_CALL alljoyn_msgarg_get_bool_array(const alljoyn_msgarg arg, size_t* length, QCC_BOOL* ab)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    QStatus status = alljoyn_msgarg_get(arg, "ab", length, ab);
    return status;
}
QStatus AJ_CALL alljoyn_msgarg_get_int16_array(const alljoyn_msgarg arg, size_t* length, int16_t* an)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "an", length, an);
}
QStatus AJ_CALL alljoyn_msgarg_get_uint16_array(const alljoyn_msgarg arg, size_t* length, uint16_t* aq)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "aq", length, aq);
}
QStatus AJ_CALL alljoyn_msgarg_get_int32_array(const alljoyn_msgarg arg, size_t* length,  int32_t* ai)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "ai", length, ai);
}
QStatus AJ_CALL alljoyn_msgarg_get_uint32_array(const alljoyn_msgarg arg, size_t* length, uint32_t* au)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "au", length, au);
}
QStatus AJ_CALL alljoyn_msgarg_get_int64_array(const alljoyn_msgarg arg, size_t* length, int64_t* ax)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "ax", length, ax);
}
QStatus AJ_CALL alljoyn_msgarg_get_uint64_array(const alljoyn_msgarg arg, size_t* length, uint64_t* at)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "at", length, at);
}
QStatus AJ_CALL alljoyn_msgarg_get_double_array(const alljoyn_msgarg arg, size_t* length, double* ad)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, "ad", length, ad);
}

QStatus AJ_CALL alljoyn_msgarg_get_variant_array(const alljoyn_msgarg arg, const char* signature,  size_t* length, alljoyn_msgarg* av)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return alljoyn_msgarg_get(arg, signature, length, av);
}

size_t AJ_CALL alljoyn_msgarg_get_array_numberofelements(const alljoyn_msgarg arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(ALLJOYN_ARRAY == (alljoyn_typeid)((ajn::MsgArgC*)arg)->typeId);
    return ((ajn::MsgArgC*)arg)->v_array.GetNumElements();
}

void AJ_CALL alljoyn_msgarg_get_array_element(const alljoyn_msgarg arg, size_t index, alljoyn_msgarg* element)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(ALLJOYN_ARRAY == (alljoyn_typeid)((ajn::MsgArgC*)arg)->typeId);
    assert(index < ((ajn::MsgArgC*)arg)->v_array.GetNumElements());
    *element = (alljoyn_msgarg) & (((ajn::MsgArgC*)arg)->v_array.GetElements()[index]);
}

const char* AJ_CALL alljoyn_msgarg_get_array_elementsignature(const alljoyn_msgarg arg, size_t index)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(ALLJOYN_ARRAY == (alljoyn_typeid)((ajn::MsgArgC*)arg)->typeId);
    return ((ajn::MsgArgC*)arg)->v_array.GetElemSig();
}

alljoyn_msgarg AJ_CALL alljoyn_msgarg_getkey(alljoyn_msgarg arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    assert(ALLJOYN_DICT_ENTRY == (alljoyn_typeid)((ajn::MsgArgC*)arg)->typeId);
    return (alljoyn_msgarg)((ajn::MsgArgC*)arg)->v_dictEntry.key;
}

alljoyn_msgarg AJ_CALL alljoyn_msgarg_getvalue(alljoyn_msgarg arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    switch ((alljoyn_typeid)((ajn::MsgArgC*)arg)->typeId) {
    case ALLJOYN_VARIANT:
        return (alljoyn_msgarg)((ajn::MsgArgC*)arg)->v_variant.val;

    case ALLJOYN_DICT_ENTRY:
        return (alljoyn_msgarg)((ajn::MsgArgC*)arg)->v_dictEntry.val;

    default:
        assert(0);
        return NULL;
    }
}

QStatus AJ_CALL alljoyn_msgarg_setdictentry(alljoyn_msgarg arg, alljoyn_msgarg key, alljoyn_msgarg value)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::MsgArgC*)arg)->v_dictEntry.key = ((ajn::MsgArgC*)key);
    ((ajn::MsgArgC*)arg)->v_dictEntry.val = ((ajn::MsgArgC*)value);
    ((ajn::MsgArgC*)arg)->typeId = (ajn::AllJoynTypeId)ALLJOYN_DICT_ENTRY;
    ((ajn::MsgArgC*)arg)->Stabilize();
    return ER_OK;
}

QStatus AJ_CALL alljoyn_msgarg_setstruct(alljoyn_msgarg arg, alljoyn_msgarg struct_members, size_t num_members)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    ((ajn::MsgArgC*)arg)->v_struct.numMembers = num_members;
    ((ajn::MsgArgC*)arg)->v_struct.members = (ajn::MsgArgC*)struct_members;
    ((ajn::MsgArgC*)arg)->typeId = (ajn::AllJoynTypeId)ALLJOYN_STRUCT;
    ((ajn::MsgArgC*)arg)->Stabilize();
    return ER_OK;
}

size_t AJ_CALL alljoyn_msgarg_getnummembers(alljoyn_msgarg arg)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    return ((ajn::MsgArgC*)arg)->v_struct.numMembers;
}

alljoyn_msgarg AJ_CALL alljoyn_msgarg_getmember(alljoyn_msgarg arg, size_t index)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    if (index >= ((ajn::MsgArgC*)arg)->v_struct.numMembers) {
        return NULL;
    }
    return (alljoyn_msgarg)(&((ajn::MsgArgC*)arg)->v_struct.members[index]);
}
