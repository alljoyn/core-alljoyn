/**
 * @file
 * Class for encapsulating message encryption and decryption operations.
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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

#include <string.h>

#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/KeyBlob.h>
#include <qcc/Util.h>

#include <alljoyn/Status.h>

#include "AllJoynCrypto.h"

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {

const size_t Crypto::MACLength = 8;

static qcc::String ConcatenateCompressedFields(uint8_t* hdr, size_t hdrLen, const HeaderFields& hdrFields)
{
    qcc::String result((char*)hdr, hdrLen, 256);

    for (uint32_t fieldId = ALLJOYN_HDR_FIELD_PATH; fieldId < ArraySize(hdrFields.field); fieldId++) {
        if (!HeaderFields::Compressible[fieldId]) {
            continue;
        }
        const MsgArg* field = &hdrFields.field[fieldId];
        char buf[8];
        size_t pos = 0;
        buf[pos++] = (char)fieldId;
        buf[pos++] = (char)field->typeId;
        switch (field->typeId) {
        case ALLJOYN_SIGNATURE:
            result.append(buf, pos);
            result.append(field->v_signature.sig, field->v_signature.len);
            break;

        case ALLJOYN_OBJECT_PATH:
        case ALLJOYN_STRING:
            result.append(buf, pos);
            result.append(field->v_string.str, field->v_string.len);
            break;

        case ALLJOYN_UINT32:
            /* Write integer as little endian */
            buf[pos++] = (char)(field->v_uint32 >> 0);
            buf[pos++] = (char)(field->v_uint32 >> 8);
            buf[pos++] = (char)(field->v_uint32 >> 16);
            buf[pos++] = (char)(field->v_uint32 >> 24);
            result.append(buf, pos);
            break;

        default:
            break;
        }
    }
    return result;
}

QStatus Crypto::Encrypt(const _Message& message, const KeyBlob& keyBlob, uint8_t* msgBuf, size_t hdrLen, size_t& bodyLen)
{
    QStatus status;
    switch (keyBlob.GetType()) {
    case KeyBlob::AES:
        {
            uint8_t* body = msgBuf + hdrLen;
            uint8_t nd[5];
            uint32_t serial = message.GetCallSerial();

            nd[0] = (uint8_t)keyBlob.GetRole();
            nd[1] = (uint8_t)(serial >> 24);
            nd[2] = (uint8_t)(serial >> 16);
            nd[3] = (uint8_t)(serial >> 8);
            nd[4] = (uint8_t)(serial);
            KeyBlob nonce(nd, sizeof(nd), KeyBlob::GENERIC);

            QCC_DbgHLPrintf(("Encrypt key:   %s", BytesToHexString(keyBlob.GetData(), keyBlob.GetSize()).c_str()));
            QCC_DbgHLPrintf(("        nonce: %s", BytesToHexString(nonce.GetData(), nonce.GetSize()).c_str()));

            Crypto_AES aes(keyBlob, Crypto_AES::CCM);
            if (message.GetFlags() & ALLJOYN_FLAG_COMPRESSED) {
                /*
                 * To prevent an attack where the attacker sends a bogus expansion rule we
                 * authenticate the compressed headers even though we won't be sending them.
                 */
                qcc::String extHdr = ConcatenateCompressedFields(msgBuf, hdrLen, message.GetHeaderFields());
                status = aes.Encrypt_CCM(body, body, bodyLen, nonce, extHdr.data(), extHdr.size(), MACLength);
            } else {
                status = aes.Encrypt_CCM(body, body, bodyLen, nonce, msgBuf, hdrLen, MACLength);
            }
        }
        break;

    default:
        status = ER_BUS_KEYBLOB_OP_INVALID;
        QCC_LogError(status, ("Key type %d not supported for message encryption", keyBlob.GetType()));
        break;
    }
    return status;
}

QStatus Crypto::Decrypt(const _Message& message, const KeyBlob& keyBlob, uint8_t* msgBuf, size_t hdrLen, size_t& bodyLen)
{
    QStatus status;
    switch (keyBlob.GetType()) {
    case KeyBlob::AES:
        {
            uint8_t* body = msgBuf + hdrLen;
            uint8_t nd[5];
            uint32_t serial = message.GetCallSerial();

            nd[0] = (uint8_t)keyBlob.GetAntiRole();
            nd[1] = (uint8_t)(serial >> 24);
            nd[2] = (uint8_t)(serial >> 16);
            nd[3] = (uint8_t)(serial >> 8);
            nd[4] = (uint8_t)(serial);
            KeyBlob nonce(nd, sizeof(nd), KeyBlob::GENERIC);

            QCC_DbgHLPrintf(("Decrypt key:   %s", BytesToHexString(keyBlob.GetData(), keyBlob.GetSize()).c_str()));
            QCC_DbgHLPrintf(("        nonce: %s", BytesToHexString(nonce.GetData(), nonce.GetSize()).c_str()));

            Crypto_AES aes(keyBlob, Crypto_AES::CCM);
            if (message.GetFlags() & ALLJOYN_FLAG_COMPRESSED) {
                /*
                 * To prevent an attack where the attacker sends a bogus expansion rule we
                 * authenticate the compressed headers even though we won't be sending them.
                 */
                qcc::String extHdr = ConcatenateCompressedFields(msgBuf, hdrLen, message.GetHeaderFields());
                status = aes.Decrypt_CCM(body, body, bodyLen, nonce, extHdr.data(), extHdr.size(), MACLength);
            } else {
                status = aes.Decrypt_CCM(body, body, bodyLen, nonce, msgBuf, hdrLen, MACLength);
            }
        }
        break;

    default:
        status = ER_BUS_KEYBLOB_OP_INVALID;
        QCC_LogError(status, ("Key type %d not supported for message decryption", keyBlob.GetType()));
        break;
    }
    if (status != ER_OK) {
        status = ER_BUS_MESSAGE_DECRYPTION_FAILED;
    }
    return status;
}

}
