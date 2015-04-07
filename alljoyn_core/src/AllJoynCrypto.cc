/**
 * @file
 * Class for encapsulating message encryption and decryption operations.
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
#include <qcc/StringUtil.h>

#include <alljoyn/Status.h>

#include "AllJoynCrypto.h"

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {

const size_t Crypto::MACLength = 8;

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
            status = aes.Encrypt_CCM(body, body, bodyLen, nonce, msgBuf, hdrLen, MACLength);
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
            status = aes.Decrypt_CCM(body, body, bodyLen, nonce, msgBuf, hdrLen, MACLength);
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
