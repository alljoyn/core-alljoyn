/**
 * @file
 * This file implements the ConversationHash class, which wraps hashing
 * functionality and provides tracing for the authentication conversation hash.
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
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/Util.h>

#include <alljoyn/Status.h>

#include "ConversationHash.h"

/* SECURITY NOTE: Because the pre-shared key is hashed into the conversation hash
 * for the ECDHE_PSK method in conversation versions <= 3, to avoid unintentional
 * disclosure, the bytes of the PSK not traced in the log, but instead an entry stating
 * that secret data is hashed in at that point is added. To override this behavior and
 * include secret data in the log, define the CONVERSATION_HASH_TRACE_SECRETS constant.
 */
#undef CONVERSATION_HASH_LOG_SECRETS

#define QCC_MODULE "CONVERSATION_HASH"

using namespace qcc;

namespace ajn {

ConversationHash::ConversationHash() : m_hashUtil(), m_sensitiveMode(false)
{
}

ConversationHash::~ConversationHash()
{
}

QStatus ConversationHash::Init()
{
    return m_hashUtil.Init();
}

QStatus ConversationHash::Update(uint8_t byte)
{
    QStatus status = m_hashUtil.Update(&byte, sizeof(byte));

    if (ER_OK == status) {
        QCC_DbgPrintf(("Hashed byte: %02X\n", byte));
    } else {
        QCC_LogError(status, ("Could not hash byte: %02X\n", byte));
    }

    return status;
}

QStatus ConversationHash::Update(const uint8_t* buf, size_t bufSize, bool includeSizeInHash)
{
    QStatus status = ER_OK;

    if (includeSizeInHash) {
        uint32_t bufSizeLE = htole32(bufSize);
        status = m_hashUtil.Update((const uint8_t*)&bufSizeLE, sizeof(bufSizeLE));

        if (ER_OK == status) {
            QCC_DbgPrintf(("Hashed size: %" PRIuSIZET "\n", bufSize));
        } else {
            QCC_LogError(status, ("Could not hash size: %" PRIuSIZET "\n", bufSize));
            return status;
        }
    }

    status = m_hashUtil.Update(buf, bufSize);

    if (ER_OK == status) {

#ifndef CONVERSATION_HASH_LOG_SECRETS
        if (m_sensitiveMode) {
            QCC_DbgPrintf(("Hashed byte array of secret data; data intentionally not logged\n"));
        } else {
#endif
        QCC_DbgPrintf(("Hashed byte array:\n"));
        QCC_DbgLocalData(buf, bufSize);
#ifndef CONVERSATION_HASH_LOG_SECRETS
    }
#endif
    } else {
        QCC_LogError(status, ("Could not hash byte array\n"));
    }

    return status;
}

QStatus ConversationHash::GetDigest(uint8_t* digest, bool keepAlive)
{
    QStatus status = m_hashUtil.GetDigest(digest, keepAlive);

    if (ER_OK == status) {
        QCC_DbgPrintf(("Got conversation digest ------------------------------------\n"));
        QCC_DbgPrintf(("Digest is:\n"));
        QCC_DbgLocalData(digest, Crypto_SHA256::DIGEST_SIZE);
    } else {
        QCC_LogError(status, ("Could not get conversation digest\n"));
    }

    return status;
}

void ConversationHash::SetSensitiveMode(bool mode)
{
    m_sensitiveMode = mode;
}

}