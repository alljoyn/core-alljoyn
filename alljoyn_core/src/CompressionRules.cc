/**
 * @file
 * Implementation of CompressionRules methods.
 */

/******************************************************************************
 * Copyright (c) 2010-2012, AllSeen Alliance. All rights reserved.
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

#include <qcc/Util.h>
#include <qcc/Mutex.h>
#include <qcc/Debug.h>
#include <alljoyn/Status.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Message.h>

#include "Adler32.h"
#include "CompressionRules.h"

#define QCC_MODULE "ALLJOYN"

using namespace qcc;
using namespace std;

namespace ajn {

void _CompressionRules::Add(const HeaderFields& hdrFields, uint32_t token)
{
    HeaderFields* expFields = new HeaderFields;
    /*
     * Copy compressible fields.
     */
    for (size_t i = 0; i < ArraySize(expFields->field); i++) {
        if (HeaderFields::Compressible[i]) {
            expFields->field[i] = hdrFields.field[i];
        }
    }
    /*
     * Add forward and reverse mapping.
     */
    tokenMap[token] = expFields;
    fieldMap[expFields] = token;
    QCC_DbgHLPrintf(("Added compression/expansion rule %u <-->\n%s", token, expFields->ToString().c_str()));
}

void _CompressionRules::AddExpansion(const HeaderFields& hdrFields, uint32_t token)
{
    if (token) {
        lock.Lock(MUTEX_CONTEXT);
        if (fieldMap.count(&hdrFields) == 0) {
            Add(hdrFields, token);
        }
        lock.Unlock(MUTEX_CONTEXT);
    }
}

uint32_t _CompressionRules::GetToken(const HeaderFields& hdrFields)
{
    uint32_t token;
    lock.Lock(MUTEX_CONTEXT);
    unordered_map<const HeaderFields*, uint32_t, HdrFieldHash, HdrFieldsEq>::iterator iter = fieldMap.find(&hdrFields);
    if (iter != fieldMap.end()) {
        token = iter->second;
    } else {
        /*
         * Allocate a random token (check it isn't zero and not in use)
         */
        do { token = Rand32(); } while (token && GetExpansion(token));
        Add(hdrFields, token);
    }
    lock.Unlock(MUTEX_CONTEXT);
    return token;
}

const HeaderFields* _CompressionRules::GetExpansion(uint32_t token)
{
    const HeaderFields* expansion = NULL;
    if (token) {
        lock.Lock(MUTEX_CONTEXT);
        map<uint32_t, const HeaderFields*>::iterator iter = tokenMap.find(token);
        expansion = (iter != tokenMap.end()) ? iter->second : NULL;
        lock.Unlock(MUTEX_CONTEXT);
    }
    return expansion;
}

_CompressionRules::~_CompressionRules()
{
    map<uint32_t, const ajn::HeaderFields*>::iterator iter = tokenMap.begin();
    while (iter != tokenMap.end()) {
        delete iter->second;
        iter++;
    }
}

bool _CompressionRules::HdrFieldsEq::operator()(const HeaderFields* k1, const HeaderFields* k2) const
{
    const MsgArg* f1 = k1->field;
    const MsgArg* f2 = k2->field;
    for (int i = 0; i < ALLJOYN_HDR_FIELD_UNKNOWN; i++, f1++, f2++) {
        if (HeaderFields::Compressible[i]) {
            if (f1->typeId != f2->typeId) {
                return false;
            }
            switch (f1->typeId) {
            case ALLJOYN_INVALID:
                break;

            case ALLJOYN_STRING:
            case ALLJOYN_OBJECT_PATH:
                if (strcmp(f1->v_string.str, f2->v_string.str) != 0) {
                    return false;
                }
                break;

            case ALLJOYN_SIGNATURE:
                if (strcmp(f1->v_signature.sig, f2->v_signature.sig) != 0) {
                    return false;
                }
                break;

            case ALLJOYN_UINT16:
                if (f1->v_uint16 != f2->v_uint16) {
                    return false;
                }
                break;

            case ALLJOYN_UINT32:
                if (f1->v_uint32 != f2->v_uint32) {
                    return false;
                }
                break;

            default:
                assert(!"invalid header field type");
            }
        }
    }
    return true;
}

size_t _CompressionRules::HdrFieldHash::operator()(const HeaderFields* k) const
{
    Adler32 adler;
    size_t hash = 0;
    if (k->field[ALLJOYN_HDR_FIELD_MEMBER].typeId == ALLJOYN_STRING) {
        hash = adler.Update((uint8_t*)k->field[ALLJOYN_HDR_FIELD_MEMBER].v_string.str, k->field[ALLJOYN_HDR_FIELD_MEMBER].v_string.len);
    }
    if (k->field[ALLJOYN_HDR_FIELD_INTERFACE].typeId == ALLJOYN_STRING) {
        hash = adler.Update((uint8_t*)k->field[ALLJOYN_HDR_FIELD_INTERFACE].v_string.str, k->field[ALLJOYN_HDR_FIELD_INTERFACE].v_string.len);
    }
    return hash;

}

}
