/**
 * @file
 *
 * This file implements a sett of utility functions to handling AllJoyn signatures.
 */

/******************************************************************************
 * Copyright (c) 2009-2012, AllSeen Alliance. All rights reserved.
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

#include <assert.h>
#include <cstdarg>
#include <string>

#include <qcc/Debug.h>
#include <qcc/String.h>

#include <alljoyn/MsgArg.h>
#include "SignatureUtils.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

bool SignatureUtils::IsBasicType(AllJoynTypeId typeId)
{
    switch (typeId) {
    case ALLJOYN_BYTE:
    case ALLJOYN_INT16:
    case ALLJOYN_UINT16:
    case ALLJOYN_BOOLEAN:
    case ALLJOYN_INT32:
    case ALLJOYN_UINT32:
    case ALLJOYN_DOUBLE:
    case ALLJOYN_UINT64:
    case ALLJOYN_INT64:
    case ALLJOYN_OBJECT_PATH:
    case ALLJOYN_STRING:
    case ALLJOYN_SIGNATURE:
    case ALLJOYN_HANDLE:
        return true;

    default:
        return false;
    }
}

/*
 * Composes the signature for an array of types. Signatures have a maximum length of 255 the
 * buffer passed in must be at least this long.
 */
QStatus SignatureUtils::MakeSignature(const MsgArg* values, uint8_t numValues, char* sig, size_t& len)
{
    QStatus status = ER_OK;

    if (values == NULL) {
        if (numValues == 0) {
            return ER_OK;
        } else {
            return ER_BUS_BAD_VALUE;
        }
    }
    if (len > 254) {
        return ER_BUS_BAD_VALUE;
    }
    while (numValues--) {
        char typeChar = 0;
        switch (values->typeId) {
        case ALLJOYN_DICT_ENTRY:
            sig[len++] = (char)ALLJOYN_DICT_ENTRY_OPEN;
            status = MakeSignature(values->v_dictEntry.key, 1, sig, len);
            if (status == ER_OK) {
                status = MakeSignature(values->v_dictEntry.val, 1, sig, len);
            }
            typeChar = ALLJOYN_DICT_ENTRY_CLOSE;
            break;

        case ALLJOYN_STRUCT:
            sig[len++] = (char)ALLJOYN_STRUCT_OPEN;
            status = MakeSignature(values->v_struct.members, values->v_struct.numMembers, sig, len);
            typeChar = (char)ALLJOYN_STRUCT_CLOSE;
            break;

        case ALLJOYN_ARRAY:
            sig[len++] = (char)ALLJOYN_ARRAY;
            {
                size_t elemSigLen = strlen(values->v_array.GetElemSig());
                if ((len + elemSigLen) < 254) {
                    memcpy(&sig[len], values->v_array.GetElemSig(), elemSigLen);
                }
                len += elemSigLen;
                typeChar = sig[--len];
            }
            break;

        case ALLJOYN_BOOLEAN_ARRAY:
        case ALLJOYN_INT32_ARRAY:
        case ALLJOYN_UINT32_ARRAY:
        case ALLJOYN_DOUBLE_ARRAY:
        case ALLJOYN_UINT64_ARRAY:
        case ALLJOYN_INT64_ARRAY:
        case ALLJOYN_INT16_ARRAY:
        case ALLJOYN_UINT16_ARRAY:
        case ALLJOYN_BYTE_ARRAY:
            sig[len++] = (char)ALLJOYN_ARRAY;
            typeChar = (char)(values->typeId >> 8);
            break;

        case ALLJOYN_BOOLEAN:
        case ALLJOYN_INT32:
        case ALLJOYN_UINT32:
        case ALLJOYN_DOUBLE:
        case ALLJOYN_UINT64:
        case ALLJOYN_INT64:
        case ALLJOYN_SIGNATURE:
        case ALLJOYN_INT16:
        case ALLJOYN_UINT16:
        case ALLJOYN_OBJECT_PATH:
        case ALLJOYN_STRING:
        case ALLJOYN_VARIANT:
        case ALLJOYN_BYTE:
        case ALLJOYN_HANDLE:
            typeChar = (char)values->typeId;
            break;

        default:
            status = ER_BUS_BAD_VALUE_TYPE;
            break;
        }
        if (len > 254) {
            status = ER_BUS_BAD_VALUE;
        }
        if (status != ER_OK) {
            break;
        }
        sig[len++] = typeChar;
        values++;
    }
    sig[len] = 0;

    return status;
}

size_t SignatureUtils::AlignmentForType(AllJoynTypeId typeId)
{
    switch (typeId) {
    case ALLJOYN_BOOLEAN:
    case ALLJOYN_INT32:
    case ALLJOYN_UINT32:
    case ALLJOYN_HANDLE:
        return 4;

    case ALLJOYN_OBJECT_PATH:
    case ALLJOYN_STRING:
    case ALLJOYN_ARRAY:
    case ALLJOYN_BOOLEAN_ARRAY:
    case ALLJOYN_DOUBLE_ARRAY:
    case ALLJOYN_INT32_ARRAY:
    case ALLJOYN_INT16_ARRAY:
    case ALLJOYN_UINT16_ARRAY:
    case ALLJOYN_UINT64_ARRAY:
    case ALLJOYN_UINT32_ARRAY:
    case ALLJOYN_INT64_ARRAY:
    case ALLJOYN_BYTE_ARRAY:
        return 4; /* for the length */

    case ALLJOYN_INT16:
    case ALLJOYN_UINT16:
        return 2;

    case ALLJOYN_VARIANT:
    case ALLJOYN_SIGNATURE:
    case ALLJOYN_BYTE:
        return 1;

    case ALLJOYN_STRUCT:
    case ALLJOYN_STRUCT_OPEN:
    case ALLJOYN_DICT_ENTRY:
    case ALLJOYN_DICT_ENTRY_OPEN:
    case ALLJOYN_DOUBLE:
    case ALLJOYN_UINT64:
    case ALLJOYN_INT64:
        return 8;

    default:
        return 0;
    }
}


#define PadUp(n, i)   (((n) + (i) - 1) & ~((i) - 1))


size_t SignatureUtils::GetSize(const MsgArg* values, size_t numValues, size_t offset)
{
    if (values == NULL) {
        return offset;
    }
    size_t sz = offset;
    while (numValues--) {
        // QCC_DbgPrintf(("GetSize @%ld %s", sz, values->ToString().c_str()));
        switch (values->typeId) {
        case ALLJOYN_DICT_ENTRY:
            sz = GetSize(values->v_dictEntry.key, 1, PadUp(sz, 8));
            sz = GetSize(values->v_dictEntry.val, 1, sz);
            break;

        case ALLJOYN_STRUCT:
            sz = GetSize(values->v_struct.members, values->v_struct.numMembers, PadUp(sz, 8));
            break;

        case ALLJOYN_ARRAY:
            sz = PadUp(sz, 4) + 4;
            if (values->v_array.numElements) {
                sz = GetSize(values->v_array.elements, values->v_array.numElements, sz);
            } else {
                size_t alignment = AlignmentForType((AllJoynTypeId)(values->v_array.elemSig[0]));
                sz = PadUp(sz, alignment);
            }
            break;

        case ALLJOYN_BOOLEAN_ARRAY:
        case ALLJOYN_INT32_ARRAY:
        case ALLJOYN_UINT32_ARRAY:
            sz = PadUp(sz, 4) + 4 + 4 * values->v_scalarArray.numElements;
            break;

        case ALLJOYN_DOUBLE_ARRAY:
        case ALLJOYN_UINT64_ARRAY:
        case ALLJOYN_INT64_ARRAY:
            sz = PadUp(sz, 4) + 4;
            sz = PadUp(sz, 8) + 8 * values->v_scalarArray.numElements;
            break;

        case ALLJOYN_INT16_ARRAY:
        case ALLJOYN_UINT16_ARRAY:
            sz = PadUp(sz, 4) + 4 + 2 * values->v_scalarArray.numElements;
            break;

        case ALLJOYN_BYTE_ARRAY:
            sz = PadUp(sz, 4) + 4 + values->v_scalarArray.numElements;
            break;

        case ALLJOYN_BOOLEAN:
        case ALLJOYN_INT32:
        case ALLJOYN_UINT32:
        case ALLJOYN_HANDLE:
            sz = PadUp(sz, 4) + 4;
            break;

        case ALLJOYN_DOUBLE:
        case ALLJOYN_UINT64:
        case ALLJOYN_INT64:
            sz = PadUp(sz, 8) + 8;
            break;

        case ALLJOYN_SIGNATURE:
            sz += 1 + values->v_signature.len + 1;
            break;

        case ALLJOYN_INT16:
        case ALLJOYN_UINT16:
            sz = PadUp(sz, 2) + 2;
            break;

        case ALLJOYN_OBJECT_PATH:
        case ALLJOYN_STRING:
            sz = PadUp(sz, 4) + 4 + values->v_string.len + 1;
            break;

        case ALLJOYN_VARIANT:
            {
                char sig[256];
                size_t len = 0;
                MakeSignature(values->v_variant.val, 1, sig, len);
                sz = GetSize(values->v_variant.val, 1, sz + 1 + len + 1);
            }
            break;

        case ALLJOYN_BYTE:
            sz += 1;
            break;

        default:
            return 0;
        }
        ++values;
    }
    return sz;
}

uint8_t SignatureUtils::CountCompleteTypes(const char* signature)
{
    uint8_t count = 0;
    if (signature != NULL) {
        while (*signature) {
            if (ParseCompleteType(signature) == ER_OK) {
                count++;
            } else {
                break;
            }
        }
    }
    return count;
}

bool SignatureUtils::IsValidSignature(const char* signature)
{
    if (!signature) {
        return false;
    }
    const char* s = signature;
    while (*s) {
        if (ParseCompleteType(s) != ER_OK) {
            return false;
        }
    }
    return (s - signature) <= (ptrdiff_t)255;
}

QStatus SignatureUtils::ParseCompleteType(const char*& sigPtr)
{
    MsgArg container;

    switch (*sigPtr++) {
    case ALLJOYN_BYTE:
    case ALLJOYN_INT16:
    case ALLJOYN_UINT16:
    case ALLJOYN_BOOLEAN:
    case ALLJOYN_INT32:
    case ALLJOYN_UINT32:
    case ALLJOYN_DOUBLE:
    case ALLJOYN_UINT64:
    case ALLJOYN_INT64:
    case ALLJOYN_OBJECT_PATH:
    case ALLJOYN_STRING:
    case ALLJOYN_SIGNATURE:
    case ALLJOYN_VARIANT:
    case ALLJOYN_STRUCT:
    case ALLJOYN_WILDCARD:
    case ALLJOYN_HANDLE:
        return ER_OK;

    case ALLJOYN_DICT_ENTRY_OPEN:
        container.typeId = ALLJOYN_DICT_ENTRY;
        container.v_dictEntry.key = NULL;
        container.v_dictEntry.val = NULL;
        return ParseContainerSignature(container, sigPtr);

    case ALLJOYN_STRUCT_OPEN:
        container.typeId = ALLJOYN_STRUCT;
        container.v_struct.members = NULL;
        return ParseContainerSignature(container, sigPtr);

    case ALLJOYN_ARRAY:
        container.typeId = ALLJOYN_ARRAY;
        container.v_array.elements = NULL;
        return ParseContainerSignature(container, sigPtr);

    default:
        return ER_BUS_BAD_SIGNATURE;
    }
}


typedef struct {
    AllJoynTypeId typeId;
    uint8_t members;
} ContainerStack;


/*
 * Parses a STRUCT, DICT_ENTRY, or ARRAY signature
 *
 * Example parameter values:
 * -For '(xyz)', container is STRUCT, sigPtr is 'xyz)'.
 * -For '{sv}', container is DICT_ENTRY, sigPtr is 'sv}'.
 * -For 'as', container is ARRAY, sigPtr is 's'.
 */
QStatus SignatureUtils::ParseContainerSignature(MsgArg& container, const char*& sigPtr)
{
    assert((container.typeId == ALLJOYN_STRUCT) ||
           (container.typeId == ALLJOYN_DICT_ENTRY) ||
           (container.typeId == ALLJOYN_ARRAY));

    QStatus status = ER_OK;
    ContainerStack containerStack[64];
    ContainerStack* outer = containerStack;
    uint8_t structDepth = 0;
    uint8_t arrayDepth = 0;

#define PushContainer(t)  do {                  \
        assert(outer < &containerStack[64]);        \
        outer++;                                    \
        outer->typeId = (t);                        \
        outer->members = 0;                         \
} \
    while (0);

#define PopContainer() do {                     \
        if (outer > &containerStack[0]) {           \
            outer--;                                \
        }                                           \
        outer->members++;                           \
} while (0);

    memset(containerStack, 0, sizeof(containerStack));
    outer->typeId = container.typeId;
    outer->members = 0;

    if (outer->typeId == ALLJOYN_ARRAY) {
        ++arrayDepth;
    } else {
        ++structDepth;
    }

    do {
        AllJoynTypeId typeId = (AllJoynTypeId)(*sigPtr++);

        switch (typeId) {
        case ALLJOYN_INVALID:
            /*
             * ALLJOYN_INVALID is 0, so this is the end of sigPtr.
             */
            if ((structDepth + arrayDepth) > 0) {
                status = ER_BUS_BAD_SIGNATURE;
            }
            break;

        case ALLJOYN_WILDCARD:
        case ALLJOYN_BYTE:
        case ALLJOYN_INT16:
        case ALLJOYN_UINT16:
        case ALLJOYN_BOOLEAN:
        case ALLJOYN_INT32:
        case ALLJOYN_UINT32:
        case ALLJOYN_DOUBLE:
        case ALLJOYN_UINT64:
        case ALLJOYN_INT64:
        case ALLJOYN_OBJECT_PATH:
        case ALLJOYN_STRING:
        case ALLJOYN_SIGNATURE:
        case ALLJOYN_VARIANT:
        case ALLJOYN_STRUCT:
        case ALLJOYN_HANDLE:
            ++outer->members;
            break;

        case ALLJOYN_ARRAY:
            if (++arrayDepth > 32) {
                status = ER_BUS_BAD_SIGNATURE;
            } else {
                PushContainer(ALLJOYN_ARRAY);
            }
            break;

        case ALLJOYN_DICT_ENTRY_OPEN:
            if (++structDepth > 32) {
                status = ER_BUS_BAD_SIGNATURE;
            } else {
                if ((outer->typeId == ALLJOYN_ARRAY) && IsBasicType((AllJoynTypeId) * sigPtr++)) {
                    PushContainer(ALLJOYN_DICT_ENTRY);
                    ++outer->members;
                } else {
                    status = ER_BUS_BAD_SIGNATURE;
                }
            }
            break;

        case ALLJOYN_DICT_ENTRY_CLOSE:
            if ((outer->typeId == ALLJOYN_DICT_ENTRY) && (outer->members == 2)) {
                --structDepth;
                PopContainer();
            } else {
                status = ER_BUS_BAD_SIGNATURE;
            }
            break;

        case ALLJOYN_STRUCT_OPEN:
            if (++structDepth > 32) {
                status = ER_BUS_BAD_SIGNATURE;
            } else {
                PushContainer(ALLJOYN_STRUCT);
            }
            break;

        case ALLJOYN_STRUCT_CLOSE:
            if ((outer->typeId == ALLJOYN_STRUCT) && (outer->members > 0)) {
                if ((--structDepth == 0) && (container.typeId == ALLJOYN_STRUCT)) {
                    container.v_struct.numMembers = outer->members;
                }
                PopContainer();
            } else {
                status = ER_BUS_BAD_SIGNATURE;
            }
            break;

        default:
            status = ER_BUS_BAD_SIGNATURE;
            break;
        }
        while ((outer->typeId == ALLJOYN_ARRAY) && (outer->members == 1)) {
            if (arrayDepth > 0) {
                --arrayDepth;
                PopContainer();
            }
        }
        /*
         * Got a complete type?
         */
        if ((structDepth + arrayDepth) == 0) {
            break;
        }
    } while (status == ER_OK);

#undef PushContainer
#undef PopContainer

    return status;
}


}
