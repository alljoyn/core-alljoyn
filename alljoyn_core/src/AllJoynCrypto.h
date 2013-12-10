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
#ifndef _ALLJOYN_CRYPTO_H
#define _ALLJOYN_CRYPTO_H

#ifndef __cplusplus
#error Only include AllJoynCrypto.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/KeyBlob.h>

#include <alljoyn/Message.h>

#include <alljoyn/Status.h>


namespace ajn {

/**
 * Class for encapsulating AllJoyn message encryption and decryption operations.
 */
class Crypto {

  public:

    /**
     * Encrypt a marshaled message inplace using the key blob provided and the encryption algorithm
     * and key stored in the key blob.
     *
     * @param message         The message being encrypted
     * @param keyBlob         The key blob containing the key for the encryption operation.
     * @param msgBuf          The message data to be encrypted. The data buffer must be large enough to handle
     *                        the expansion specified in the ExpansionBytes member variable.
     * @param hdrLen          The length of the header part of the message that will not be encrypted.
     * @param bodyLen[in/out] On input the size of the plaintext body, on output the size of the
     *                        encrypted body.
     *
     * @return - ER_OK if the data was succesfully encrypted.
     *         - ER_BUS_KEYBLOB_OP_INVALID if the key blob cannot be used for encryption.
     *         - Other errors if the arguments are invalid.
     */
    static QStatus Encrypt(const _Message& message, const qcc::KeyBlob& keyBlob, uint8_t* msgBuf, size_t hdrLen, size_t& bodyLen);

    /**
     * Decrypt and authenticate marshaled message inplace using the key blob provided and the
     * decryption algorithm and key stored in the key blob.
     *
     * @param message         The message being decrypted
     * @param keyBlob         The key blob containing the key for the decryption operation.
     * @param msgBuf          The message data to be decrypted.
     * @param hdrLen          The length of the non-encrypted header part of the message.
     * @param bodyLen[in/out] On input the size of the crypttext body, on output the size of the
     *                        decrypted body.
     *
     * @return - ER_OK if the data was succesfully decrypted.
     *         - ER_BUS_KEYBLOB_OP_INVALID if the key blob cannot be used for decryption.
     *         - Other errors if the arguments are invalid.
     */
    static QStatus Decrypt(const _Message& message, const qcc::KeyBlob& keyBlob, uint8_t* msgBuf, size_t hdrLen, size_t& bodyLen);

    /**
     * Compute a SHA1 hash over the header fields and return the result in a key blob.
     *
     * @param hdrFields   The header fields to compute a hash from.
     * @param keyBlob     The key blob to return the hash.
     *
     * @return ER_OK if the hash was succesfully computed.
     */
    static QStatus HashHeaderFields(const HeaderFields& hdrFields, qcc::KeyBlob& keyBlob);

    /**
     * The length of the message authentication field that will be appended to the encrypted data.
     */
    static const size_t MACLength;

};


}

#endif
