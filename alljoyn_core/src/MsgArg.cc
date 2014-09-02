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

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>

#include "MsgArgUtils.h"
#include "SignatureUtils.h"
#include "BusUtil.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {

qcc::String MsgArg::ToString(size_t indent) const
{
    qcc::String str;
#ifndef NDEBUG
#define CHK_STR(s)  (((s) == NULL) ? "" : (s))
    qcc::String in = qcc::String(indent, ' ');

    str = in;

    indent += 2;

    switch (typeId) {
    case ALLJOYN_ARRAY:
        str += "<array type_sig=\"" + qcc::String(CHK_STR(v_array.GetElemSig())) + "\">";
        for (uint32_t i = 0; i < v_array.numElements; i++) {
            str += "\n" + v_array.elements[i].ToString(indent);
        }
        str += "\n" + in + "</array>";
        break;

    case ALLJOYN_BOOLEAN:
        str += v_bool ? "<boolean>1</boolean>" : "<boolean>0</boolean>";
        break;

    case ALLJOYN_DOUBLE:
        // To be bit-exact stringify double as a 64 bit hex value
        str += "<double>" "0x" + U64ToString(v_uint64, 16) + "</double>";
        break;

    case ALLJOYN_DICT_ENTRY:
        str += "<dict_entry>\n" +
               v_dictEntry.key->ToString(indent) + "\n" +
               v_dictEntry.val->ToString(indent) + "\n" +
               in + "</dict_entry>";
        break;

    case ALLJOYN_SIGNATURE:
        str += "<signature>" + qcc::String(CHK_STR(v_signature.sig)) + "</signature>";
        break;

    case ALLJOYN_INT32:
        str += "<int32>" + I32ToString(v_int32) + "</int32>";
        break;

    case ALLJOYN_INT16:
        str += "<int16>" + I32ToString(v_int16) + "</int16>";
        break;

    case ALLJOYN_OBJECT_PATH:
        str += "<object_path>" + qcc::String(CHK_STR(v_objPath.str)) + "</object_path>";
        break;

    case ALLJOYN_UINT16:
        str += "<uint16>" + U32ToString(v_uint16) + "</uint16>";
        break;

    case ALLJOYN_STRUCT:
        str += "<struct>\n";
        for (uint32_t i = 0; i < v_struct.numMembers; i++) {
            str += v_struct.members[i].ToString(indent) + "\n";
        }
        str += in + "</struct>";
        break;

    case ALLJOYN_STRING:
        str += "<string>" + qcc::String(CHK_STR(v_string.str)) + "</string>";
        break;

    case ALLJOYN_UINT64:
        str += "<uint64>" + U64ToString(v_uint64) + "</uint64>";
        break;

    case ALLJOYN_UINT32:
        str += "<uint32>" + U32ToString(v_uint32) + "</uint32>";
        break;

    case ALLJOYN_VARIANT:
        str += "<variant signature=\"" + v_variant.val->Signature() + "\">\n";
        str += v_variant.val->ToString(indent);
        str += "\n" + in + "</variant>";
        break;

    case ALLJOYN_INT64:
        str += "<int64>" + I64ToString(v_int64) + "</int64>";
        break;

    case ALLJOYN_BYTE:
        str += "<byte>" + U32ToString(v_byte) + "</byte>";
        break;

    case ALLJOYN_HANDLE:
        str += "<handle>" + qcc::BytesToHexString((const uint8_t*)&v_handle.fd, sizeof(v_handle.fd)) + "</handle>";
        break;

    case ALLJOYN_BOOLEAN_ARRAY:
        str += "<array type=\"boolean\">";
        if (v_scalarArray.numElements) {
            str += "\n" + qcc::String(indent, ' ');
            for (uint32_t i = 0; i < v_scalarArray.numElements; i++) {
                str += v_scalarArray.v_bool[i] ? "1 " : "0 ";
            }
        }
        str += "\n" + in + "</array>";
        break;

    case ALLJOYN_DOUBLE_ARRAY:
        str += "<array type=\"double\">";
        if (v_scalarArray.numElements) {
            str += "\n" + qcc::String(indent, ' ');
            for (uint32_t i = 0; i < v_scalarArray.numElements; i++) {
                str += U64ToString((uint64_t)v_scalarArray.v_double[i]) + " ";
            }
        }
        str += "\n" + in + "</array>";
        break;

    case ALLJOYN_INT32_ARRAY:
        str += "<array type=\"int32\">";
        if (v_scalarArray.numElements) {
            str += "\n" + qcc::String(indent, ' ');
            for (uint32_t i = 0; i < v_scalarArray.numElements; i++) {
                str += I32ToString(v_scalarArray.v_int32[i]) + " ";
            }
        }
        str += "\n" + in + "</array>";
        break;

    case ALLJOYN_INT16_ARRAY:
        str += "<array type=\"int16\">";
        if (v_scalarArray.numElements) {
            str += "\n" + qcc::String(indent, ' ');
            for (uint32_t i = 0; i < v_scalarArray.numElements; i++) {
                str += I32ToString(v_scalarArray.v_int16[i]) + " ";
            }
        }
        str += "\n" + in + "</array>";
        break;

    case ALLJOYN_UINT16_ARRAY:
        str += "<array type=\"uint16\">";
        if (v_scalarArray.numElements) {
            str += "\n" + qcc::String(indent, ' ');
            for (uint32_t i = 0; i < v_scalarArray.numElements; i++) {
                str += U32ToString(v_scalarArray.v_uint16[i]) + " ";
            }
        }
        str += "\n" + in + "</array>";
        break;

    case ALLJOYN_UINT64_ARRAY:
        str += "<array type=\"uint64\">";
        if (v_scalarArray.numElements) {
            str += "\n" + qcc::String(indent, ' ');
            for (uint32_t i = 0; i < v_scalarArray.numElements; i++) {
                str += U64ToString(v_scalarArray.v_uint64[i]) + " ";
            }
        }
        str += "\n" + in + "</array>";
        break;

    case ALLJOYN_UINT32_ARRAY:
        str += "<array type=\"uint32\">";
        if (v_scalarArray.numElements) {
            str += "\n" + qcc::String(indent, ' ');
            for (uint32_t i = 0; i < v_scalarArray.numElements; i++) {
                str += U32ToString(v_scalarArray.v_uint32[i]) + " ";
            }
        }
        str += "\n" + in + "</array>";
        break;

    case ALLJOYN_INT64_ARRAY:
        str += "<array type=\"int64\">";
        if (v_scalarArray.numElements) {
            str += "\n" + qcc::String(indent, ' ');
            for (uint32_t i = 0; i < v_scalarArray.numElements; i++) {
                str += I64ToString(v_scalarArray.v_int64[i]) + " ";
            }
        }
        str += "\n" + in + "</array>";
        break;

    case ALLJOYN_BYTE_ARRAY:
        str += "<array type=\"byte\">";
        if (v_scalarArray.numElements) {
            str += "\n" + qcc::String(indent, ' ');
            for (uint32_t i = 0; i < v_scalarArray.numElements; i++) {
                str += U32ToString(v_scalarArray.v_byte[i]) + " ";
            }
        }
        str += "\n" + in + "</array>";
        break;

    default:
        str += "<invalid/>";
        break;
    }
#undef CHK_STR
#endif
    return str;
}

QStatus AllJoynArray::SetElements(const char* elemSig, size_t numElements, MsgArg* elements)
{
    QStatus status = ER_OK;
    if ((numElements != 0) && (elements == NULL)) {
        status = ER_BAD_ARG_2;
    } else {
        /*
         * Check signature is valid for an array element type.
         */
        if (SignatureUtils::CountCompleteTypes(elemSig) != 1) {
            status = ER_BUS_BAD_SIGNATURE;
        } else if (numElements) {
            /*
             * Check elements all have the same type. Note that a more rigorous check that the
             * signatures all match is performed when the array is marshaled.
             */
            AllJoynTypeId typeId = elements[0].typeId;
            for (size_t i = 1; i < numElements; i++) {
                if (elements[i].typeId != typeId) {
                    status = ER_BUS_BAD_VALUE;
                    QCC_LogError(status, ("Array element[%d] does not have expected type", i, elemSig));
                    break;
                }
            }
        }
    }
    if (status == ER_OK) {
        size_t len = strlen(elemSig);
        this->elemSig = new char[len + 1];
        memcpy(this->elemSig, elemSig, len);
        this->elemSig[len] = 0;
        this->numElements = numElements;
        this->elements = elements;
    } else {
        this->elemSig = NULL;
        this->numElements = 0;
        this->elements = NULL;
    }
    return status;
}

bool MsgArg::HasSignature(const char* signature) const
{
    char sig[256];
    size_t len = 0;
    SignatureUtils::MakeSignature(this, 1, sig, len);
    return strcmp(signature, sig) == 0;

}

void MsgArg::Stabilize()
{
    /*
     * If the MsgArg doesn't own the MsgArgs it references they need to be cloned.
     */
    if (!(flags & OwnsArgs)) {
        MsgArg* tmp;
        flags |= OwnsArgs;
        switch (typeId) {
        case ALLJOYN_DICT_ENTRY:
            v_dictEntry.key = new MsgArg(*v_dictEntry.key);
            v_dictEntry.val = new MsgArg(*v_dictEntry.val);
            break;

        case ALLJOYN_STRUCT:
            tmp = new MsgArg[v_struct.numMembers];
            for (size_t i = 0; i < v_struct.numMembers; i++) {
                Clone(tmp[i], v_struct.members[i]);
            }
            v_struct.members = tmp;
            break;

        case ALLJOYN_ARRAY:
            tmp = new MsgArg[v_array.numElements];
            for (size_t i = 0; i < v_array.numElements; i++) {
                Clone(tmp[i], v_array.elements[i]);
            }
            v_array.elements = tmp;
            break;

        case ALLJOYN_VARIANT:
            v_variant.val = new MsgArg(*v_variant.val);
            break;

        default:
            /* Nothing to do for the remaining types */
            break;
        }
    }
    /*
     * If the MsgArg doesn't own the data it references directly or indirectly the data needs to be copied.
     */
    if (!(flags & OwnsData)) {
        void* tmp;
        flags |= OwnsData;
        switch (typeId) {
        case ALLJOYN_OBJECT_PATH:
        case ALLJOYN_STRING:
            if (v_string.str) {
                tmp = new char[v_string.len + 1];
                memcpy(tmp, v_string.str, v_string.len + 1);
                v_string.str = (char*)tmp;
            }
            break;

        case ALLJOYN_SIGNATURE:
            if (v_signature.sig) {
                tmp = new char[v_signature.len + 1];
                memcpy(tmp, v_signature.sig, v_signature.len + 1);
                v_signature.sig = (char*)tmp;
            }
            break;

        case ALLJOYN_BOOLEAN_ARRAY:
        case ALLJOYN_INT32_ARRAY:
        case ALLJOYN_UINT32_ARRAY:
            tmp = new uint32_t[v_scalarArray.numElements];
            memcpy(tmp, v_scalarArray.v_uint32, v_scalarArray.numElements * sizeof(uint32_t));
            v_scalarArray.v_uint32 = (uint32_t*)tmp;
            break;

        case ALLJOYN_INT16_ARRAY:
        case ALLJOYN_UINT16_ARRAY:
            tmp = new uint16_t[v_scalarArray.numElements];
            memcpy(tmp, v_scalarArray.v_uint16, v_scalarArray.numElements * sizeof(uint16_t));
            v_scalarArray.v_uint16 = (uint16_t*)tmp;
            break;

        case ALLJOYN_DOUBLE_ARRAY:
        case ALLJOYN_UINT64_ARRAY:
        case ALLJOYN_INT64_ARRAY:
            tmp = new uint64_t[v_scalarArray.numElements];
            memcpy(tmp, v_scalarArray.v_uint64, v_scalarArray.numElements * sizeof(uint64_t));
            v_scalarArray.v_uint64 = (uint64_t*)tmp;
            break;

        case ALLJOYN_BYTE_ARRAY:
            tmp = new uint8_t[v_scalarArray.numElements];
            memcpy(tmp, v_scalarArray.v_byte, v_scalarArray.numElements * sizeof(uint8_t));
            v_scalarArray.v_byte = (uint8_t*)tmp;
            break;

        case ALLJOYN_STRUCT:
            for (size_t i = 0; i < v_struct.numMembers; i++) {
                v_struct.members[i].Stabilize();
            }
            break;

        case ALLJOYN_ARRAY:
            for (size_t i = 0; i < v_array.numElements; i++) {
                v_array.elements[i].Stabilize();
            }
            break;

        case ALLJOYN_DICT_ENTRY:
            v_dictEntry.key->Stabilize();
            v_dictEntry.val->Stabilize();
            break;

        case ALLJOYN_VARIANT:
            v_variant.val->Stabilize();
            break;

        default:
            /* Nothing to do for the remaining types */
            break;
        }
    }
}

bool MsgArg::operator==(const MsgArg& other)
{
    if (typeId != other.typeId) {
        return false;
    }
    switch (typeId) {
    case ALLJOYN_DICT_ENTRY:
        {
            bool keysAreEqual = (*(v_dictEntry.key) == *(other.v_dictEntry.key));
            bool valuesAreEqual = (*(v_dictEntry.val) == *(other.v_dictEntry.val));
            return (keysAreEqual && valuesAreEqual);
        }

    case ALLJOYN_STRUCT:
        if (v_struct.numMembers != other.v_struct.numMembers) {
            return false;
        }
        for (size_t i = 0; i < v_struct.numMembers; i++) {
            if (v_struct.members[i] != other.v_struct.members[i]) {
                return false;
            }
        }
        return true;

    case ALLJOYN_ARRAY:
        if (v_array.numElements != other.v_array.numElements) {
            return false;
        }
        for (size_t i = 0; i < v_array.numElements; i++) {
            if (v_array.elements[i] != other.v_array.elements[i]) {
                return false;
            }
        }
        return true;

    case ALLJOYN_VARIANT:
        return *(v_variant.val) == *(other.v_variant.val);

    case ALLJOYN_OBJECT_PATH:
    case ALLJOYN_STRING:
        return (v_string.len == other.v_string.len) && (strcmp(v_string.str, other.v_string.str) == 0);

    case ALLJOYN_SIGNATURE:
        return (v_signature.len == other.v_signature.len) && (strcmp(v_signature.sig, other.v_signature.sig) == 0);

    case ALLJOYN_BOOLEAN_ARRAY:
        return (v_scalarArray.numElements == other.v_scalarArray.numElements) &&
               (memcmp(v_scalarArray.v_bool, other.v_scalarArray.v_bool, v_scalarArray.numElements * sizeof(bool)) == 0);

    case ALLJOYN_INT32_ARRAY:
    case ALLJOYN_UINT32_ARRAY:
        return (v_scalarArray.numElements == other.v_scalarArray.numElements) &&
               (memcmp(v_scalarArray.v_uint32, other.v_scalarArray.v_uint32, v_scalarArray.numElements * sizeof(uint32_t)) == 0);

    case ALLJOYN_INT16_ARRAY:
    case ALLJOYN_UINT16_ARRAY:
        return (v_scalarArray.numElements == other.v_scalarArray.numElements) &&
               (memcmp(v_scalarArray.v_uint16, other.v_scalarArray.v_uint16, v_scalarArray.numElements * sizeof(uint16_t)) == 0);

    case ALLJOYN_DOUBLE_ARRAY:
    case ALLJOYN_UINT64_ARRAY:
    case ALLJOYN_INT64_ARRAY:
        return (v_scalarArray.numElements == other.v_scalarArray.numElements) &&
               (memcmp(v_scalarArray.v_uint64, other.v_scalarArray.v_uint64, v_scalarArray.numElements * sizeof(uint64_t)) == 0);

    case ALLJOYN_BYTE_ARRAY:
        return (v_scalarArray.numElements == other.v_scalarArray.numElements) &&
               (memcmp(v_scalarArray.v_byte, other.v_scalarArray.v_byte, v_scalarArray.numElements) == 0);

    case ALLJOYN_BYTE:
        return v_byte == other.v_byte;

    case ALLJOYN_INT16:
    case ALLJOYN_UINT16:
        return v_uint16 == other.v_uint16;

    case ALLJOYN_BOOLEAN:
        return v_bool == other.v_bool;

    case ALLJOYN_INT32:
    case ALLJOYN_UINT32:
        return v_uint32 == other.v_uint32;

    case ALLJOYN_DOUBLE:
    case ALLJOYN_UINT64:
    case ALLJOYN_INT64:
        return v_uint64 == other.v_uint64;

    case ALLJOYN_HANDLE:
        return v_handle.fd == other.v_handle.fd;

    default:
        return false;
    }
}

void MsgArg::Clone(MsgArg& dest, const MsgArg& src)
{
    dest.Clear();
    dest.typeId = src.typeId;
    dest.flags = (OwnsData | OwnsArgs);
    switch (dest.typeId) {
    case ALLJOYN_DICT_ENTRY:
        dest.v_dictEntry.key = new MsgArg(*src.v_dictEntry.key);
        dest.v_dictEntry.val = new MsgArg(*src.v_dictEntry.val);
        break;

    case ALLJOYN_STRUCT:
        dest.v_struct.numMembers = src.v_struct.numMembers;
        dest.v_struct.members = new MsgArg[dest.v_struct.numMembers];
        for (size_t i = 0; i < dest.v_struct.numMembers; i++) {
            Clone(dest.v_struct.members[i], src.v_struct.members[i]);
        }
        break;

    case ALLJOYN_ARRAY:
        if (src.v_array.numElements > 0) {
            dest.v_array.elements = new MsgArg[src.v_array.numElements];
            for (size_t i = 0; i < src.v_array.numElements; i++) {
                Clone(dest.v_array.elements[i], src.v_array.elements[i]);
            }
        } else {
            dest.v_array.elements = NULL;
        }
        dest.v_array.SetElements(src.v_array.GetElemSig(), src.v_array.numElements, dest.v_array.elements);
        break;

    case ALLJOYN_VARIANT:
        dest.v_variant.val = new MsgArg(*src.v_variant.val);
        break;

    case ALLJOYN_OBJECT_PATH:
    case ALLJOYN_STRING:
        dest.v_string.len = src.v_string.len;
        if (src.v_string.str) {
            dest.v_string.str = new char[src.v_string.len + 1];
            memcpy((void*)dest.v_string.str, src.v_string.str, src.v_string.len + 1);
        } else {
            dest.v_string.str = NULL;
        }
        break;

    case ALLJOYN_SIGNATURE:
        dest.v_signature.len = src.v_signature.len;
        if (src.v_signature.sig) {
            dest.v_signature.sig = new char[src.v_signature.len + 1];
            memcpy((void*)dest.v_signature.sig, src.v_signature.sig, src.v_signature.len + 1);
        } else {
            dest.v_signature.sig = NULL;
        }
        break;

    case ALLJOYN_BOOLEAN_ARRAY:
        dest.v_scalarArray.numElements = src.v_scalarArray.numElements;
        dest.v_scalarArray.v_bool = new bool[dest.v_scalarArray.numElements];
        memcpy((void*)dest.v_scalarArray.v_bool, src.v_scalarArray.v_bool, dest.v_scalarArray.numElements * sizeof(bool));
        break;

    case ALLJOYN_INT32_ARRAY:
    case ALLJOYN_UINT32_ARRAY:
        dest.v_scalarArray.numElements = src.v_scalarArray.numElements;
        dest.v_scalarArray.v_uint32 = new uint32_t[dest.v_scalarArray.numElements];
        memcpy((void*)dest.v_scalarArray.v_uint32, src.v_scalarArray.v_uint32, dest.v_scalarArray.numElements * sizeof(uint32_t));
        break;

    case ALLJOYN_INT16_ARRAY:
    case ALLJOYN_UINT16_ARRAY:
        dest.v_scalarArray.numElements = src.v_scalarArray.numElements;
        dest.v_scalarArray.v_uint16 = new uint16_t[dest.v_scalarArray.numElements];
        memcpy((void*)dest.v_scalarArray.v_uint16, src.v_scalarArray.v_uint16, dest.v_scalarArray.numElements * sizeof(uint16_t));
        break;

    case ALLJOYN_DOUBLE_ARRAY:
    case ALLJOYN_UINT64_ARRAY:
    case ALLJOYN_INT64_ARRAY:
        dest.v_scalarArray.numElements = src.v_scalarArray.numElements;
        dest.v_scalarArray.v_uint64 = new uint64_t[dest.v_scalarArray.numElements];
        memcpy((void*)dest.v_scalarArray.v_uint64, src.v_scalarArray.v_uint64, dest.v_scalarArray.numElements * sizeof(uint64_t));
        break;

    case ALLJOYN_BYTE_ARRAY:
        dest.v_scalarArray.numElements = src.v_scalarArray.numElements;
        dest.v_scalarArray.v_byte = new uint8_t[dest.v_scalarArray.numElements];
        memcpy((void*)dest.v_scalarArray.v_byte, src.v_scalarArray.v_byte, dest.v_scalarArray.numElements * sizeof(uint8_t));
        break;

    case ALLJOYN_BYTE:
        dest.v_byte = src.v_byte;
        break;

    case ALLJOYN_INT16:
    case ALLJOYN_UINT16:
        dest.v_uint16 = src.v_uint16;
        break;

    case ALLJOYN_BOOLEAN:
        dest.v_bool = src.v_bool;
        break;

    case ALLJOYN_INT32:
    case ALLJOYN_UINT32:
        dest.v_uint32 = src.v_uint32;
        break;

    case ALLJOYN_DOUBLE:
    case ALLJOYN_UINT64:
    case ALLJOYN_INT64:
        dest.v_uint64 = src.v_uint64;
        break;

    case ALLJOYN_HANDLE:
        dest.v_handle = src.v_handle;
        break;

    default:
        break;
    }
}

void MsgArg::Clear()
{
    switch (typeId) {
    case ALLJOYN_OBJECT_PATH:
    case ALLJOYN_STRING:
        if (flags & OwnsData) {
            delete [] v_string.str;
        }
        break;

    case ALLJOYN_SIGNATURE:
        if (flags & OwnsData) {
            delete [] v_signature.sig;
        }
        break;

    case ALLJOYN_BOOLEAN_ARRAY:
        if (flags & OwnsData) {
            delete [] v_scalarArray.v_bool;
        }
        break;

    case ALLJOYN_INT32_ARRAY:
    case ALLJOYN_UINT32_ARRAY:
        if (flags & OwnsData) {
            delete [] v_scalarArray.v_uint32;
        }
        break;

    case ALLJOYN_INT16_ARRAY:
    case ALLJOYN_UINT16_ARRAY:
        if (flags & OwnsData) {
            delete [] v_scalarArray.v_uint16;
        }
        break;

    case ALLJOYN_DOUBLE_ARRAY:
    case ALLJOYN_UINT64_ARRAY:
    case ALLJOYN_INT64_ARRAY:
        if (flags & OwnsData) {
            delete [] v_scalarArray.v_uint64;
        }
        break;

    case ALLJOYN_BYTE_ARRAY:
        if (flags & OwnsData) {
            delete [] v_scalarArray.v_byte;
        }
        break;

    case ALLJOYN_DICT_ENTRY:
        if (flags & OwnsArgs) {
            v_dictEntry.key->Clear();
            delete v_dictEntry.key;
            v_dictEntry.val->Clear();
            delete v_dictEntry.val;
        }
        break;

    case ALLJOYN_STRUCT:
        if (flags & OwnsArgs) {
            for (size_t i = 0; i < v_struct.numMembers; i++) {
                v_struct.members[i].Clear();
            }
            delete [] v_struct.members;
        }
        break;

    case ALLJOYN_ARRAY:
        if (flags & OwnsArgs) {
            for (size_t i = 0; i < v_array.numElements; i++) {
                v_array.elements[i].Clear();
            }
            delete [] v_array.elements;
        }
        /* Always need to delete the element signature */
        delete [] v_array.elemSig;
        v_array.elemSig = NULL;
        break;

    case ALLJOYN_VARIANT:
        if (flags & OwnsArgs) {
            v_variant.val->Clear();
            delete v_variant.val;
        }
        break;

    default:
        /* Nothing to do for the remaining types */
        break;
    }
    flags = 0;
    v_invalid.unused[0] = v_invalid.unused[1] = v_invalid.unused[2] = NULL;
    typeId = ALLJOYN_INVALID;
}

qcc::String MsgArg::ToString(const MsgArg* args, size_t numArgs, size_t indent)
{
    qcc::String outStr;
#ifndef NDEBUG
    for (size_t i = 0; i < numArgs; ++i) {
        outStr += args[i].ToString(indent) + '\n';
    }
#endif
    return outStr;
}

qcc::String MsgArg::Signature(const MsgArg* values, size_t numValues)
{
    char sig[256];
    size_t len = 0;
    QStatus status = SignatureUtils::MakeSignature(values, numValues, sig, len);
    if (status == ER_OK) {
        return qcc::String(sig, len);
    } else {
        return "";
    }
}

/*
 * Note that "arry" is not a typo - Some editors tag "array" as a reserved word.
 */
QStatus MsgArg::BuildArray(MsgArg* arry, const qcc::String& elemSig, va_list* argpIn)
{
    va_list& argp = *argpIn;

    QStatus status = ER_OK;

    size_t numElements = va_arg(argp, size_t);
    void* elems = va_arg(argp, void*);

    if (numElements && !elems) {
        /*
         * See special case for string below
         */
        if (elemSig[0] != 's') {
            return ER_INVALID_ADDRESS;
        }
    }

    MsgArg* elements = NULL;

    /*
     * Check that the number of elements makes sense.  This is a soft-check, the
     * units of MAX_ARRAY_LEN is bytes which is unknown until marshalling.
     */
    if (numElements > ALLJOYN_MAX_ARRAY_LEN) {
        status = ER_BUS_BAD_VALUE;
        QCC_LogError(status, ("Too many array elements - could be an address"));
        arry->typeId = ALLJOYN_INVALID;
        return status;
    }
    switch (elemSig[0]) {
    case '*':
        if (numElements > 0) {
            elements = (MsgArg*)elems;
            const qcc::String sig = elements[0].Signature();
            /*
             * Check elements all have same type as the first element.
             */
            for (size_t i = 1; i < numElements; i++) {
                if (!elements[i].HasSignature(sig.c_str())) {
                    status = ER_BUS_BAD_VALUE;
                    QCC_LogError(status, ("Array element[%d] does not have expected signature \"%s\"", i, sig.c_str()));
                    break;
                }
            }
            if (status == ER_OK) {
                status = arry->v_array.SetElements(sig.c_str(), numElements, elements);
                arry->flags = 0;
            }
        } else {
            status = ER_BUS_BAD_VALUE;
            QCC_LogError(status, ("Wildcard element signature cannot be used with an empty array"));
        }
        break;

    case 'a':
    case 'v':
    case '(':
    case '{':
    case 'h':
        if (numElements > 0) {
            elements = (MsgArg*)elems;
            /*
             * Check elements conform to the expected signature type
             */
            for (size_t i = 0; i < numElements; i++) {
                if (!elements[i].HasSignature(elemSig.c_str())) {
                    status = ER_BUS_BAD_VALUE;
                    QCC_LogError(status, ("Array element[%d] does not have expected signature \"%s\"", i, elemSig.c_str()));
                    break;
                }
            }
        }
        if (status == ER_OK) {
            status = arry->v_array.SetElements(elemSig.c_str(), numElements, elements);
        }
        break;

    case '$':
        if (numElements > 0) {
            const String* strs = (const String*)elems;
            elements = new MsgArg[numElements];
            arry->flags |= OwnsArgs;
            for (size_t i = 0; i < numElements; ++i) {
                elements[i].typeId = ALLJOYN_STRING;
                elements[i].v_string.str = strs[i].c_str();
                elements[i].v_string.len = strs[i].size();
            }
        }
        status = arry->v_array.SetElements("s", numElements, elements);
        break;

    case 'o':
    case 's':
        if (numElements > 0) {
            char** strings = (char**)elems;
            String* strs = strings ? NULL : va_arg(argp, String*);
            elements = new MsgArg[numElements];
            arry->flags |= OwnsArgs;
            for (size_t i = 0; i < numElements; ++i) {
                elements[i].typeId = (AllJoynTypeId)(elemSig[0]);
                elements[i].v_string.str = strings ? strings[i] : strs[i].c_str();
                elements[i].v_string.len = strlen(elements[i].v_string.str);
            }
        }
        status = arry->v_array.SetElements(elemSig.c_str(), numElements, elements);
        break;

    case 'g':
        if (numElements > 0) {
            char** strings = (char**)elems;
            elements = new MsgArg[numElements];
            arry->flags |= OwnsArgs;
            for (size_t i = 0; i < numElements; ++i) {
                elements[i].typeId = ALLJOYN_SIGNATURE;
                elements[i].v_signature.sig = strings[i];
                elements[i].v_signature.len = (uint8_t)strlen(strings[i]);
            }
        }
        status = arry->v_array.SetElements(elemSig.c_str(), numElements, elements);
        break;

    case 'b':
        arry->typeId = ALLJOYN_BOOLEAN_ARRAY;
        arry->v_scalarArray.v_bool = (bool*)elems;
        arry->v_scalarArray.numElements = numElements;
        break;

    case 'd':
        arry->typeId = ALLJOYN_DOUBLE_ARRAY;
        arry->v_scalarArray.v_double = (double*)elems;
        arry->v_scalarArray.numElements = numElements;
        break;

    case 'i':
        arry->typeId = ALLJOYN_INT32_ARRAY;
        arry->v_scalarArray.v_int32 = (int32_t*)elems;
        arry->v_scalarArray.numElements = numElements;
        break;

    case 'n':
        arry->typeId = ALLJOYN_INT16_ARRAY;
        arry->v_scalarArray.v_int16 = (int16_t*)elems;
        arry->v_scalarArray.numElements = numElements;
        break;

    case 'q':
        arry->typeId = ALLJOYN_UINT16_ARRAY;
        arry->v_scalarArray.v_uint16 = (uint16_t*)elems;
        arry->v_scalarArray.numElements = numElements;
        break;

    case 't':
        arry->typeId = ALLJOYN_UINT64_ARRAY;
        arry->v_scalarArray.v_uint64 = (uint64_t*)elems;
        arry->v_scalarArray.numElements = numElements;
        break;

    case 'u':
        arry->typeId = ALLJOYN_UINT32_ARRAY;
        arry->v_scalarArray.v_uint32 = (uint32_t*)elems;
        arry->v_scalarArray.numElements = numElements;
        break;

    case 'x':
        arry->typeId = ALLJOYN_INT64_ARRAY;
        arry->v_scalarArray.v_int64 = (int64_t*)elems;
        arry->v_scalarArray.numElements = numElements;
        break;

    case 'y':
        arry->typeId = ALLJOYN_BYTE_ARRAY;
        arry->v_scalarArray.v_byte = (uint8_t*)elems;
        arry->v_scalarArray.numElements = numElements;
        break;

    default:
        status = ER_BUS_BAD_SIGNATURE;
        QCC_LogError(status, ("Invalid char '\\%d' in array element signature", elemSig[0]));
        break;
    }
    if (status != ER_OK) {
        arry->typeId = ALLJOYN_INVALID;
    }
    return status;
}

QStatus MsgArg::VBuildArgs(const char*& signature, size_t sigLen, MsgArg* arg, size_t maxArgs, va_list* argpIn, size_t* count)
{
    va_list& argp = *argpIn;

    QStatus status = ER_OK;
    size_t numArgs = 0;

    if (!signature) {
        return ER_INVALID_ADDRESS;
    }
    while (sigLen--) {
        switch (*signature++) {
        case '*':
            {
                MsgArg* inArg =  va_arg(argp, MsgArg*);
                if (!inArg) {
                    return ER_INVALID_ADDRESS;
                }
                if (inArg->typeId == ALLJOYN_ARRAY) {
                    status = arg->v_array.SetElements(inArg->v_array.elemSig, inArg->v_array.numElements, inArg->v_array.elements);
                    arg->typeId = ALLJOYN_ARRAY;
                    arg->flags = 0;
                } else {
                    *arg = *inArg;
                }
            }
            break;

        case 'a':
            {
                const char* elemSig = signature;
                if (!elemSig) {
                    return ER_INVALID_ADDRESS;
                }
                arg->typeId = ALLJOYN_ARRAY;
                if (*elemSig == '*' || *elemSig == '$') {
                    ++signature;
                } else {
                    status = SignatureUtils::ParseContainerSignature(*arg, signature);
                }
                if (status == ER_OK) {
                    size_t elemSigLen = signature - elemSig;
                    status = BuildArray(arg, qcc::String(elemSig, elemSigLen), &argp);
                    sigLen -= elemSigLen;
                } else {
                    status = ER_BUS_NOT_A_COMPLETE_TYPE;
                    QCC_LogError(status, ("Signature for array was not a complete type"));
                    arg->typeId = ALLJOYN_INVALID;
                }
            }
            break;

        case 'b':
            arg->typeId = ALLJOYN_BOOLEAN;
            arg->v_bool = (bool)va_arg(argp, int);
            break;

        case 'd':
            arg->typeId = ALLJOYN_DOUBLE;
            arg->v_double = (double)va_arg(argp, double);
            break;

        case 'e':
            arg->typeId = ALLJOYN_DICT_ENTRY;
            arg->v_dictEntry.key = va_arg(argp, MsgArg*);
            arg->v_dictEntry.val = va_arg(argp, MsgArg*);
            break;

        case 'g':
            {
                char* sig = va_arg(argp, char*);
                if (!sig) {
                    arg->v_signature.sig = "";
                    arg->v_signature.len = 0;
                    arg->typeId = ALLJOYN_SIGNATURE;
                } else if (SignatureUtils::IsValidSignature(sig)) {
                    arg->v_signature.sig = sig;
                    arg->v_signature.len = (uint8_t)strlen(sig);
                    arg->typeId = ALLJOYN_SIGNATURE;
                } else {
                    status = ER_BUS_BAD_SIGNATURE;
                    QCC_LogError(status, ("String \"%s\" is not a legal signature", sig));
                }
            }
            break;

        case 'h':
            arg->typeId = ALLJOYN_HANDLE;
            arg->v_handle.fd = (qcc::SocketFd)va_arg(argp, qcc::SocketFd);
            break;

        case 'i':
            arg->typeId = ALLJOYN_INT32;
            arg->v_uint32 = (uint32_t)va_arg(argp, int);
            break;

        case 'n':
            arg->typeId = ALLJOYN_INT16;
            arg->v_int16 = (int16_t)va_arg(argp, int);
            break;

        case 'o':
            {
                char* object_path = va_arg(argp, char*);
                if (IsLegalObjectPath(object_path)) {
                    arg->typeId = ALLJOYN_OBJECT_PATH;
                    arg->v_string.str = object_path;
                    arg->v_string.len = arg->v_string.str ? strlen(arg->v_string.str) : 0;
                } else {
                    status = ER_BUS_BAD_SIGNATURE;
                    QCC_LogError(status, ("String \"%s\" is not a legal object path", object_path));
                }
            }
            break;

        case 'q':
            arg->typeId = ALLJOYN_UINT16;
            arg->v_uint16 = (uint16_t)va_arg(argp, int);
            break;

        case 'r':
            arg->typeId = ALLJOYN_STRUCT;
            arg->v_struct.numMembers = (size_t)va_arg(argp, int);
            arg->v_struct.members = va_arg(argp, MsgArg*);
            break;

        case 's':
            arg->typeId = ALLJOYN_STRING;
            arg->v_string.str = va_arg(argp, char*);
            arg->v_string.len = arg->v_string.str ? strlen(arg->v_string.str) : 0;
            break;

        case 't':
            arg->typeId = ALLJOYN_UINT64;
            arg->v_uint64 = (va_arg(argp, uint64_t));
            break;

        case 'u':
            arg->typeId = ALLJOYN_UINT32;
            arg->v_uint32 = va_arg(argp, uint32_t);
            break;

        case 'v':
            arg->typeId = ALLJOYN_VARIANT;
            arg->v_variant.val = va_arg(argp, MsgArg*);
            break;

        case 'x':
            arg->typeId = ALLJOYN_INT64;
            arg->v_int64 = (va_arg(argp, int64_t));
            break;

        case 'y':
            arg->typeId = ALLJOYN_BYTE;
            arg->v_byte = (uint8_t)va_arg(argp, int);
            break;

        case '(':
            {
                const char* memberSig = signature;
                arg->typeId = ALLJOYN_STRUCT;
                status = SignatureUtils::ParseContainerSignature(*arg, signature);
                if (status == ER_OK) {
                    size_t memSigLen = signature - memberSig - 1;             // -1 to exclude the closing ')'
                    arg->v_struct.members = new MsgArg[arg->v_struct.numMembers];
                    arg->flags |= OwnsArgs;
                    status = VBuildArgs(memberSig, memSigLen, arg->v_struct.members, arg->v_struct.numMembers, &argp);
                    sigLen -= (memSigLen + 1);
                } else {
                    QCC_LogError(status, ("Signature for STRUCT was not a complete type"));
                    arg->typeId = ALLJOYN_INVALID;
                }
            }
            break;

        case '{':
            {
                const char* memberSig = signature;
                arg->typeId = ALLJOYN_DICT_ENTRY;
                status = SignatureUtils::ParseContainerSignature(*arg, signature);
                if (status == ER_OK) {
                    size_t memSigLen = signature - memberSig - 1;             // -1 to exclude the closing '}'
                    arg->v_dictEntry.key = new MsgArg;
                    arg->v_dictEntry.val = new MsgArg;
                    arg->flags |= OwnsArgs;
                    status = VBuildArgs(memberSig, memSigLen, arg->v_dictEntry.key, 1, &argp);
                    if (status != ER_OK) {
                        break;
                    }
                    if (SignatureUtils::IsBasicType(arg->v_dictEntry.key->typeId)) {
                        status = VBuildArgs(memberSig, memSigLen, arg->v_dictEntry.val, 1, &argp);
                    } else {
                        status = ER_BUS_BAD_SIGNATURE;
                        QCC_LogError(status, ("Key type for DICTIONARY ENTRY was not a basic type"));
                    }
                    if (status != ER_OK) {
                        break;
                    }
                    sigLen -= (memSigLen + 1);
                } else {
                    QCC_LogError(status, ("Signature for DICT_ENTRY was not a complete type"));
                    arg->typeId = ALLJOYN_INVALID;
                }
            }
            break;

        default:
            QCC_LogError(ER_BUS_BAD_SIGNATURE, ("Invalid char '\\%d' in signature", *(signature - 1)));
            arg->typeId = ALLJOYN_INVALID;
            break;
        }
        if (status != ER_OK) {
            /*
             * This will free any resources allocated above.
             */
            arg->Clear();
            break;
        }
        if (++numArgs == maxArgs) {
            break;
        }
        arg++;
    }
    if (count) {
        *count = numArgs;
    }
    return status;
}

MsgArg::MsgArg(const char* signature, ...) : typeId(ALLJOYN_INVALID), flags(0)
{
    va_list argp;
    va_start(argp, signature);
    QStatus status = ER_OK;

    size_t sigLen = (signature ? strlen(signature) : 0);
    if ((sigLen < 1) || (sigLen > 255)) {
        status = ER_BUS_BAD_SIGNATURE;
    } else {
        status = VBuildArgs(signature, sigLen, this, 1, &argp);
    }
    if ((status != ER_OK) || (*signature != 0)) {
        QCC_LogError(status, ("MsgArg constructor signature \"%s\" failed", signature));
        Clear();
    }
    va_end(argp);
}

QStatus MsgArg::Set(const char* signature, ...)
{
    va_list argp;
    va_start(argp, signature);
    QStatus status = ER_OK;

    this->Clear();
    size_t sigLen = (signature ? strlen(signature) : 0);
    if ((sigLen < 1) || (sigLen > 255)) {
        status = ER_BUS_BAD_SIGNATURE;
    } else {
        status = VBuildArgs(signature, sigLen, this, 1, &argp);
        if ((status == ER_OK) && (*signature != 0)) {
            status = ER_BUS_NOT_A_COMPLETE_TYPE;
        }
    }
    va_end(argp);
    return status;
}

QStatus MsgArgUtils::SetV(MsgArg* args, size_t& numArgs, const char* signature, va_list* argpIn)
{
    va_list& argp = *argpIn;

    QStatus status = ER_OK;

    for (size_t i = 0; i < numArgs; i++) {
        args[i].Clear();
    }
    size_t sigLen = (signature ? strlen(signature) : 0);
    if ((sigLen < 1) || (sigLen > 255)) {
        status = ER_BUS_BAD_SIGNATURE;
    } else {
        status = MsgArg::VBuildArgs(signature, sigLen, args, numArgs, &argp, &numArgs);
        if ((status == ER_OK) && (*signature != 0)) {
            status = ER_BUS_TRUNCATED;
            QCC_LogError(status, ("Too few MsgArgs truncated at: \"%s\"", signature));
        }
    }
    return status;
}

QStatus MsgArg::Set(MsgArg* args, size_t& numArgs, const char* signature, ...)
{
    va_list argp;
    va_start(argp, signature);
    QStatus status = MsgArgUtils::SetV(args, numArgs, signature, &argp);
    va_end(argp);
    return status;
}

QStatus MsgArg::Get(const MsgArg* args, size_t numArgs, const char* signature, ...)
{
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
    QStatus status = VParseArgs(signature, sigLen, args, numArgs, &argp);
    va_end(argp);
    return status;
}

QStatus MsgArg::Get(const char* signature, ...) const
{
    size_t sigLen = (signature ? strlen(signature) : 0);
    if (sigLen == 0) {
        return ER_BAD_ARG_1;
    }
    va_list argp;
    va_start(argp, signature);
    QStatus status = VParseArgs(signature, sigLen, this, 1, &argp);
    va_end(argp);
    return status;
}

QStatus MsgArg::GetElement(const char* elemSig, ...) const
{

    size_t sigLen = (elemSig ? strlen(elemSig) : 0);
    if (sigLen < 4) {
        return ER_BAD_ARG_1;
    }
    /* Check this MsgArg is an array of dictionary entries */
    if ((typeId != ALLJOYN_ARRAY) || (*v_array.elemSig != '{')) {
        return ER_BUS_NOT_A_DICTIONARY;
    }
    /* Check key has right type */
    if (v_array.elemSig[1] != elemSig[1]) {
        return ER_BUS_SIGNATURE_MISMATCH;
    }
    va_list argp;
    va_start(argp, elemSig);
    /* Get the key as a MsgArg */
    MsgArg key;
    size_t numArgs;
    ++elemSig;
    QStatus status = MsgArg::VBuildArgs(elemSig, 1, &key, 1, &argp, &numArgs);
    if (status == ER_OK) {
        status = ER_BUS_ELEMENT_NOT_FOUND;
        /* Linear search to match the key */
        const MsgArg* entry = v_array.elements;
        for (size_t i = 0; i < v_array.numElements; ++i, ++entry) {
            if (*entry->v_dictEntry.key == key) {
                status = ER_OK;
                break;
            }
        }
        if (status == ER_OK) {
            status = VParseArgs(elemSig, sigLen - 3, entry->v_dictEntry.val, 1, &argp);
        }
    }
    va_end(argp);
    return status;
}

/*
 * Recursively sets the ownership flags on the entire MsgArg tree.
 */
void MsgArg::SetOwnershipDeep()
{
    switch (typeId) {
    case ALLJOYN_DICT_ENTRY:
        v_dictEntry.key->SetOwnershipFlags(flags, true);
        v_dictEntry.val->SetOwnershipFlags(flags, true);
        break;

    case ALLJOYN_STRUCT:
        for (size_t i = 0; i < v_struct.numMembers; i++) {
            v_struct.members[i].SetOwnershipFlags(flags, true);
        }
        break;

    case ALLJOYN_ARRAY:
        for (size_t i = 0; i < v_array.numElements; i++) {
            v_array.elements[i].SetOwnershipFlags(flags, true);
        }
        break;

    case ALLJOYN_VARIANT:
        v_variant.val->SetOwnershipFlags(flags, true);
        break;

    default:
        /* Nothing to do for the remaining types */
        break;
    }
}

/*
 * Note that "arry" is not a typo - Some editors tag "array" as a reserved word.
 */
QStatus MsgArg::ParseArray(const MsgArg* arry, const char* elemSig, size_t elemSigLen, va_list* argpIn)
{
    va_list& argp = *argpIn;

    QStatus status = ER_BUS_SIGNATURE_MISMATCH;
    AllJoynTypeId elemType = (AllJoynTypeId)(*elemSig);

    size_t* l = va_arg(argp, size_t*);
    if (!l) {
        status = ER_INVALID_ADDRESS;
        return status;
    }
    const void** p = va_arg(argp, const void**);
    if (!p) {
        status = ER_INVALID_ADDRESS;
        return status;
    }
    switch (elemType) {
    case ALLJOYN_BYTE:
        if (arry->typeId == ALLJOYN_BYTE_ARRAY) {
            *l = arry->v_scalarArray.numElements;
            *p = (const void*)arry->v_scalarArray.v_byte;
            status = ER_OK;
        }
        break;

    case ALLJOYN_INT16:
    case ALLJOYN_UINT16:
        if ((arry->typeId == ALLJOYN_INT16_ARRAY) || (arry->typeId == ALLJOYN_UINT16_ARRAY)) {
            *l = arry->v_scalarArray.numElements;
            *p = (const void*)arry->v_scalarArray.v_uint16;
            status = ER_OK;
        }
        break;

    case ALLJOYN_BOOLEAN:
        if (arry->typeId == ALLJOYN_BOOLEAN_ARRAY) {
            *l = arry->v_scalarArray.numElements;
            *p = (const void*)arry->v_scalarArray.v_bool;
            status = ER_OK;
        }
        break;

    case ALLJOYN_INT32:
    case ALLJOYN_UINT32:
        if ((arry->typeId == ALLJOYN_INT32_ARRAY) || (arry->typeId == ALLJOYN_UINT32_ARRAY)) {
            *l = arry->v_scalarArray.numElements;
            *p = (const void*)arry->v_scalarArray.v_uint32;
            status = ER_OK;
        }
        break;

    case ALLJOYN_DOUBLE:
    case ALLJOYN_INT64:
    case ALLJOYN_UINT64:
        if ((arry->typeId == ALLJOYN_DOUBLE_ARRAY) || (arry->typeId == ALLJOYN_INT64_ARRAY) || (arry->typeId == ALLJOYN_UINT64_ARRAY)) {
            *l = arry->v_scalarArray.numElements;
            *p = (const void*)arry->v_scalarArray.v_uint64;
            status = ER_OK;
        }
        break;

    case ALLJOYN_ARRAY:
    case ALLJOYN_STRUCT_OPEN:
    case ALLJOYN_DICT_ENTRY_OPEN:
    case ALLJOYN_STRING:
    case ALLJOYN_SIGNATURE:
    case ALLJOYN_OBJECT_PATH:
    case ALLJOYN_VARIANT:
    case ALLJOYN_HANDLE:
        if (strncmp(elemSig, arry->v_array.elemSig, elemSigLen) == 0) {
            *l = arry->v_array.GetNumElements();
            *p = (const void*)arry->v_array.GetElements();
            status = ER_OK;
        }
        break;

    case ALLJOYN_WILDCARD:
        status = ER_BUS_BAD_SIGNATURE;
        QCC_LogError(status, ("Wildcard not allowed as an array element type"));
        break;

    default:
        status = ER_BUS_BAD_SIGNATURE;
        QCC_LogError(status, ("Invalid char '\\%d' in signature", *elemSig));
        break;
    }
    return status;
}

QStatus MsgArg::VParseArgs(const char*& signature, size_t sigLen, const MsgArg* argList, size_t numArgs, va_list* argpIn)
{
    va_list& argp = *argpIn;

    QStatus status = ER_OK;

    while ((status == ER_OK) && sigLen--) {
        if (numArgs == 0) {
            status = ER_BUS_SIGNATURE_MISMATCH;
            break;
        }
        const MsgArg* arg = argList++;
        --numArgs;
        /*
         * Expand variants to the underlying type.
         */
        while (arg->typeId == ALLJOYN_VARIANT) {
            arg = arg->v_variant.val;
        }
        AllJoynTypeId typeId = (AllJoynTypeId)(*signature++);
        switch (typeId) {
        case ALLJOYN_VARIANT:
            if ((argList - 1)->typeId != ALLJOYN_VARIANT) {
                status = ER_BUS_SIGNATURE_MISMATCH;
                break;
            }

        /* Falling through */
        case ALLJOYN_WILDCARD:
            /* argp is a pointer to a MsgArg */
            {
                const MsgArg** p = va_arg(argp, const MsgArg * *);
                if (!p) {
                    status = ER_INVALID_ADDRESS;
                    break;
                }
                *p = arg;
            }
            break;

        case ALLJOYN_ARRAY:
            if ((arg->typeId & 0xFF) != ALLJOYN_ARRAY) {
                status = ER_BUS_SIGNATURE_MISMATCH;
            } else {
                const char* elemSig = signature;
                status = SignatureUtils::ParseCompleteType(signature);
                if (status == ER_OK) {
                    size_t elemSigLen = signature - elemSig;
                    status = ParseArray(arg, elemSig, elemSigLen, &argp);
                    sigLen -= elemSigLen;
                }
            }
            break;

        case ALLJOYN_BYTE:
            /* argp is a pointer to a byte */
            if (arg->typeId != typeId) {
                status = ER_BUS_SIGNATURE_MISMATCH;
            } else {
                uint8_t* p = va_arg(argp, uint8_t*);
                if (!p) {
                    status = ER_INVALID_ADDRESS;
                    break;
                }
                *p = arg->v_byte;
            }
            break;

        case ALLJOYN_INT16:
        case ALLJOYN_UINT16:
            /* argp is a pointer to a 2 byte quantity */
            if (arg->typeId != typeId) {
                status = ER_BUS_SIGNATURE_MISMATCH;
            } else {
                uint16_t* p = va_arg(argp, uint16_t*);
                if (!p) {
                    status = ER_INVALID_ADDRESS;
                    break;
                }
                *p = arg->v_int16;
            }
            break;

        case ALLJOYN_BOOLEAN:
            /* argp is a pointer to a bool-sized quantity */
            if (arg->typeId != typeId) {
                status = ER_BUS_SIGNATURE_MISMATCH;
            } else {
                bool* p = va_arg(argp, bool*);
                if (!p) {
                    status = ER_INVALID_ADDRESS;
                    break;
                }
                *p = arg->v_bool;
            }
            break;

        case ALLJOYN_INT32:
        case ALLJOYN_UINT32:
            /* argp is a pointer to a 4 byte quantity */
            if (arg->typeId != typeId) {
                status = ER_BUS_SIGNATURE_MISMATCH;
            } else {
                uint32_t* p = va_arg(argp, uint32_t*);
                if (!p) {
                    status = ER_INVALID_ADDRESS;
                    break;
                }
                *p = arg->v_uint32;
            }
            break;

        case ALLJOYN_DOUBLE:
        case ALLJOYN_INT64:
        case ALLJOYN_UINT64:
            /* argp is a pointer to an 8 byte quantity */
            if (arg->typeId != typeId) {
                status = ER_BUS_SIGNATURE_MISMATCH;
            } else {
                uint64_t* p = va_arg(argp, uint64_t*);
                if (!p) {
                    status = ER_INVALID_ADDRESS;
                    break;
                }
                *p = arg->v_uint64;
            }
            break;

        case ALLJOYN_STRING:
        case ALLJOYN_SIGNATURE:
        case ALLJOYN_OBJECT_PATH:
            /* argp is a pointer to a char* */
            if (arg->typeId != typeId) {
                status = ER_BUS_SIGNATURE_MISMATCH;
            } else {
                const char** p = va_arg(argp, const char**);
                if (!p) {
                    status = ER_INVALID_ADDRESS;
                    break;
                }
                *p = (arg->typeId == ALLJOYN_SIGNATURE) ? arg->v_signature.sig : arg->v_string.str;
            }
            break;

        case ALLJOYN_STRUCT_OPEN:
            if (arg->typeId != ALLJOYN_STRUCT) {
                status = ER_BUS_SIGNATURE_MISMATCH;
            } else {
                const char* memberSig = signature--;
                status = SignatureUtils::ParseCompleteType(signature);
                if (status != ER_OK) {
                    break;
                }
                size_t memSigLen = signature - memberSig - 1; // -1 to exclude the closing ')'
                status = VParseArgs(memberSig, memSigLen, arg->v_struct.members, arg->v_struct.numMembers, &argp);
                sigLen -= (memSigLen + 1);
            }
            break;

        case ALLJOYN_DICT_ENTRY_OPEN:
            if (arg->typeId != ALLJOYN_DICT_ENTRY) {
                status = ER_BUS_SIGNATURE_MISMATCH;
            } else {
                const char* keySig = signature--;
                /* Validate the complete dictionary entry */
                status = SignatureUtils::ParseCompleteType(signature);
                if (status != ER_OK) {
                    break;
                }
                /* skip over key signature */
                const char* valSig = keySig;
                SignatureUtils::ParseCompleteType(valSig);
                status = VParseArgs(keySig, valSig - keySig, arg->v_dictEntry.key, 1, &argp);
                if (status != ER_OK) {
                    break;
                }
                status = VParseArgs(valSig, signature - valSig - 1, arg->v_dictEntry.val, 1, &argp);
                if (status != ER_OK) {
                    break;
                }
                sigLen -= (signature - keySig + 1);
            }
            break;

        case ALLJOYN_HANDLE:
            /* argp is a pointer to an qcc::SocketFd quantity */
            if (arg->typeId != typeId) {
                status = ER_BUS_SIGNATURE_MISMATCH;
            } else {
                qcc::SocketFd* p = va_arg(argp, SocketFd*);
                if (!p) {
                    status = ER_INVALID_ADDRESS;
                    break;
                }
                *p = arg->v_handle.fd;
            }
            break;

        default:
            status = ER_BUS_BAD_SIGNATURE;
            QCC_LogError(status, ("Invalid char '\\%d' in signature", *(signature - 1)));
            break;
        }
    }
    if ((status == ER_OK) && (numArgs != 0)) {
        status = ER_BUS_SIGNATURE_MISMATCH;
    }
    return status;
}

}

