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

const size_t Crypto::PreviousMACLength = 8;
const size_t Crypto::MACLength = 16;

const size_t Crypto::MaxMACLength = 16;

const size_t Crypto::NonceLength = 13;
const size_t Crypto::PreviousNonceLength = 5;

const size_t Crypto::MaxNonceLength = 13;
const size_t Crypto::MaxExtraNonceLength = 8;

const int32_t Crypto::MIN_AUTH_VERSION_MACLEN16 = 3;
const int32_t Crypto::MIN_AUTH_VERSION_FULLNONCELEN = 3;
const int32_t Crypto::MIN_AUTH_VERSION_USE_CRYPTO_VALUE = 3;

size_t Crypto::GetMACLength(const _Message& message) {
    int32_t authV = message.GetAuthVersion();
    size_t macLen = MACLength;

    if (MIN_AUTH_VERSION_MACLEN16 > authV) {
        macLen = PreviousMACLength;
    }

    return macLen;
}

size_t Crypto::GetNonceLength(const _Message& message) {
    int32_t authV = message.GetAuthVersion();
    size_t nonceLen = NonceLength;

    if (MIN_AUTH_VERSION_FULLNONCELEN > authV) {
        nonceLen = PreviousNonceLength;
    }

    return nonceLen;
}

size_t Crypto::GetExtraNonceLength(const _Message& message) {
    int32_t authV = message.GetAuthVersion();
    size_t extraNonceLen = 0;

    if (MIN_AUTH_VERSION_FULLNONCELEN <= authV) {
        extraNonceLen = NonceLength - PreviousNonceLength;
    }

    return extraNonceLen;
}
/**
 * There are two different Nonce sizes and layouts depending on the Auth Version used.
 * When the Auth Version is less than 3, the Nonce is 5 bytes and has the following layout:
 *
 *  Nonce:
 *      byte0       -   Byte indicating role.
 *      bytes1...4  -   Big Endian Serial Number (32bits/4bytes.)
 *
 *
 * When the Auth Version is greater than or equal to 3, the Nonce is 13 bytes and has the following layout:
 *
 *  Nonce:
 *      byte0       -   Byte indicating role.
 *      bytes1...4  -   Big Endian Serial Number (32bits/4bytes.)
 *      bytes5...12 -   Big Endian Crypto Random Value (64bits/8bytes.)
 * Note that the first 5 bytes of the second Nonce version is the same as the first Nonce version.
 */

QStatus Crypto::Encrypt(const _Message& message, const KeyBlob& keyBlob, uint8_t* msgBuf, size_t hdrLen, size_t& bodyLen)
{
    QStatus status;

    switch (keyBlob.GetType()) {
    case KeyBlob::AES:
        {
            uint8_t* body = msgBuf + hdrLen;
            uint8_t nd[MaxNonceLength];
            uint32_t serial = message.GetCallSerial();
            size_t macLen = GetMACLength(message);
            size_t extraNonceLen = 0;

            nd[0] = (uint8_t)keyBlob.GetRole();
            nd[1] = (uint8_t)(serial >> 24);
            nd[2] = (uint8_t)(serial >> 16);
            nd[3] = (uint8_t)(serial >> 8);
            nd[4] = (uint8_t)(serial);
            if (MIN_AUTH_VERSION_USE_CRYPTO_VALUE <= message.GetAuthVersion()) {
                extraNonceLen = GetExtraNonceLength(message);

                QCC_ASSERT(extraNonceLen <= (MaxNonceLength - PreviousNonceLength));
                if (MaxNonceLength < (extraNonceLen + PreviousNonceLength)) {
                    status = ER_CRYPTO_ILLEGAL_PARAMETERS;
                    break;
                }

                Crypto_GetRandomBytes(&nd[PreviousNonceLength], extraNonceLen);

                memcpy(body + bodyLen + macLen, &nd[PreviousNonceLength], extraNonceLen);

            }
            KeyBlob nonce(nd, min(sizeof(nd), GetNonceLength(message)), KeyBlob::GENERIC);

            QCC_ASSERT(0 <= message.GetAuthVersion());

            // Patch up the body length in header before encrypting
            _Message::MessageHeader* msgHdr = (_Message::MessageHeader*)msgBuf;
            msgHdr->bodyLen = bodyLen + extraNonceLen + macLen;

            QCC_DbgHLPrintf(("bodyLen in %d", bodyLen));

            if (message.endianSwap) {
                msgHdr->bodyLen = EndianSwap32(msgHdr->bodyLen);
            }

            QCC_DbgHLPrintf(("     Header: %s", BytesToHexString(msgBuf, sizeof(_Message::MessageHeader)).c_str()));
            QCC_DbgHLPrintf(("Encrypt key: %s", BytesToHexString(keyBlob.GetData(), keyBlob.GetSize()).c_str()));
            QCC_DbgHLPrintf(("      nonce: %s", BytesToHexString(nonce.GetData(), nonce.GetSize()).c_str()));
            Crypto_AES aes(keyBlob, Crypto_AES::CCM);
            status = aes.Encrypt_CCM(body, body, bodyLen, nonce, msgBuf, hdrLen, macLen);

            bodyLen += extraNonceLen;

            QCC_DbgHLPrintf(("        MAC: %s", BytesToHexString(body + bodyLen - macLen - extraNonceLen, macLen).c_str()));
            QCC_DbgHLPrintf(("extra nonce: %s", BytesToHexString(body + bodyLen - extraNonceLen, extraNonceLen).c_str()));

            QCC_DbgHLPrintf(("bodyLen out %d", bodyLen));
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
    case KeyBlob::AES: {
            uint8_t* body = msgBuf + hdrLen;
            uint8_t nd[MaxNonceLength];
            uint32_t serial = message.GetCallSerial();
            size_t macLen = GetMACLength(message);
            size_t extraNonceLen = 0;

            nd[0] = (uint8_t)keyBlob.GetAntiRole();
            nd[1] = (uint8_t)(serial >> 24);
            nd[2] = (uint8_t)(serial >> 16);
            nd[3] = (uint8_t)(serial >> 8);
            nd[4] = (uint8_t)(serial);

            if (MIN_AUTH_VERSION_USE_CRYPTO_VALUE <= message.GetAuthVersion()) {
                extraNonceLen = GetExtraNonceLength(message);

                QCC_ASSERT(extraNonceLen <= (MaxNonceLength - PreviousNonceLength));
                if (MaxNonceLength < (extraNonceLen + PreviousNonceLength)) {
                    status = ER_CRYPTO_ILLEGAL_PARAMETERS;
                    break;
                }

                memcpy(&nd[PreviousNonceLength], body + bodyLen - extraNonceLen, extraNonceLen);
            }

            KeyBlob nonce(nd, min(sizeof(nd), GetNonceLength(message)), KeyBlob::GENERIC);

            QCC_DbgHLPrintf(("bodyLen in %d", bodyLen));
            bodyLen -= extraNonceLen;

            QCC_DbgHLPrintf(("     Header: %s", BytesToHexString(msgBuf, sizeof(_Message::MessageHeader)).c_str()));
            QCC_DbgHLPrintf(("Decrypt key: %s", BytesToHexString(keyBlob.GetData(), keyBlob.GetSize()).c_str()));
            QCC_DbgHLPrintf(("      nonce: %s", BytesToHexString(nonce.GetData(), nonce.GetSize()).c_str()));
            QCC_DbgHLPrintf(("        MAC: %s", BytesToHexString(body + bodyLen - macLen, macLen).c_str()));
            QCC_DbgHLPrintf(("extra nonce: %s", BytesToHexString(body + bodyLen, extraNonceLen).c_str()));

            Crypto_AES aes(keyBlob, Crypto_AES::CCM);
            status = aes.Decrypt_CCM(body, body, bodyLen, nonce, msgBuf, hdrLen, macLen);
            QCC_DbgHLPrintf(("bodyLen out %d", bodyLen));
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
