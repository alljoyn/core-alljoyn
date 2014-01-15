/**
 * @file
 *
 * This file implements the STUN Attribute Message Integrity class
 */

/******************************************************************************
 * Copyright (c) 2009,2012 AllSeen Alliance. All rights reserved.
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
#include <qcc/Crypto.h>
#include <StunAttributeBase.h>
#include <StunAttributeMessageIntegrity.h>
#include <StunMessage.h>
#include <StunTransactionID.h>
#include <alljoyn/Status.h>

using namespace qcc;

#define QCC_MODULE "STUN_ATTRIBUTE"


QStatus StunAttributeMessageIntegrity::Parse(const uint8_t*& buf, size_t& bufSize)
{
    QStatus status;
    uint8_t compDigest[Crypto_SHA1::DIGEST_SIZE];

    // Need to do some message length spoofing to work as described in RFC
    // 5389 section 15.4.
    uint16_t fakeLen = ((buf - (message.rawMsg + StunMessage::MIN_MSG_SIZE)) +
                        Crypto_SHA1::DIGEST_SIZE);
    size_t sha1Size = (buf - message.rawMsg) - StunAttribute::RenderSize();
    uint8_t lengthBuf[] = { static_cast<uint8_t>(fakeLen >> 8),
                            static_cast<uint8_t>(fakeLen & 0xff) };
    const uint8_t* pos = message.rawMsg;
    Crypto_SHA1 sha1;

    QCC_DbgPrintf(("StunAttributeMessageIntegrity::Parse(*buf, bufSize = %u, sg = <>)", bufSize));

    digest = buf;
    buf += Crypto_SHA1::DIGEST_SIZE;
    bufSize -= Crypto_SHA1::DIGEST_SIZE;

    status = StunAttribute::Parse(buf, bufSize);
    if (status != ER_OK) {
        goto exit;
    }

    if (message.hmacKey == NULL) {
        QCC_DbgPrintf(("Skipping Message Integrity check due to missing HMAC Key."));
        miStatus = NO_HMAC;
        status = ER_OK;
        goto exit;
    }

    QCC_DbgPrintf(("Computing SHA1 over %u bytes (fakeLen = %u).", sha1Size, fakeLen));
    QCC_DbgLocalData(message.hmacKey, message.hmacKeyLen);

    sha1.Init(message.hmacKey, message.hmacKeyLen);

    // First the first 2 octets of the raw message.
    sha1.Update(pos, sizeof(uint16_t));
    QCC_DbgRemoteData(pos, sizeof(uint16_t));
    pos += sizeof(uint16_t);
    sha1Size -= sizeof(uint16_t);

    // Now the spoofed length.
    sha1.Update(lengthBuf, sizeof(lengthBuf));
    QCC_DbgRemoteData(lengthBuf, sizeof(lengthBuf));
    pos += sizeof(lengthBuf);
    sha1Size -= sizeof(lengthBuf);

    // Now the rest of the message.
    sha1.Update(pos, sha1Size);
    QCC_DbgLocalData(pos, sha1Size);

    sha1.GetDigest(compDigest);

    QCC_DbgPrintf(("Comparing digest with compDigest"));
    QCC_DbgRemoteData(digest, Crypto_SHA1::DIGEST_SIZE);
    QCC_DbgLocalData(compDigest, Crypto_SHA1::DIGEST_SIZE);

    if (memcmp(digest, compDigest, Crypto_SHA1::DIGEST_SIZE) != 0) {
        miStatus = INVALID;
        status = ER_STUN_INVALID_MESSAGE_INTEGRITY;
        QCC_LogError(status, ("Invalid message integrity"));
    } else {
        miStatus = VALID;
        QCC_DbgPrintf(("Verified Integrity"));
    }

exit:
    return status;
}


QStatus StunAttributeMessageIntegrity::RenderBinary(uint8_t*& buf, size_t& bufSize, ScatterGatherList& sg) const
{
    QCC_DbgTrace(("StunAttributeMessageIntegrity::RenderBinary(*buf, bufSize = %u, sg = <>)", bufSize));
    assert(message.hmacKey != NULL);
    QCC_DbgPrintf(("StunAttributeMessageIntegrity::RenderBinary(): message.hmacKey(%s) message.hmacKeyLen(%d)", (char*)message.hmacKey, message.hmacKeyLen));

    // Need to do some message length spoofing to work as described in RFC
    // 5389 section 15.4.
    uint16_t fakeLen;
    uint8_t lengthBuf[2];
    StunMessage::const_iterator attr;
    ScatterGatherList miSG;
    ScatterGatherList::const_iterator sgIter;
    Crypto_SHA1 sha1;
    QStatus status;

    // Need to calculate the length up to us but not beyond for computing the
    // digest.

    fakeLen = 0;
    for (attr = message.Begin(); (*attr) != this; ++attr) {
        fakeLen += (*attr)->Size();
    }
    fakeLen += Size();  // Include our size too.

    lengthBuf[0] = static_cast<uint8_t>(fakeLen >> 8);
    lengthBuf[1] = static_cast<uint8_t>(fakeLen & 0xff);

    miSG = sg;

    QCC_DbgPrintf(("Computing SHA1 over %u bytes (fakeLen = %u).", miSG.DataSize(), fakeLen));
    QCC_DbgLocalData(message.hmacKey, message.hmacKeyLen);


    sha1.Init(message.hmacKey, message.hmacKeyLen);

    // Do the first 4 octets manually for the faked length.
    sha1.Update(reinterpret_cast<uint8_t*>(miSG.Begin()->buf), sizeof(uint16_t));
    QCC_DbgLocalData(miSG.Begin()->buf, sizeof(uint16_t));
    miSG.TrimFromBegining(sizeof(uint16_t));

    sha1.Update(lengthBuf, sizeof(lengthBuf));
    QCC_DbgLocalData(lengthBuf, sizeof(lengthBuf));
    miSG.TrimFromBegining(sizeof(lengthBuf));

    for (sgIter = miSG.Begin(); sgIter != miSG.End(); ++sgIter) {
        sha1.Update(reinterpret_cast<uint8_t*>(sgIter->buf), sgIter->len);
        QCC_DbgLocalData(sgIter->buf, sgIter->len);
    }

    status = StunAttribute::RenderBinary(buf, bufSize, sg);
    if (status != ER_OK) {
        goto exit;
    }

    sha1.GetDigest(buf);

    sg.AddBuffer(&buf[0], Crypto_SHA1::DIGEST_SIZE);
    sg.IncDataSize(Crypto_SHA1::DIGEST_SIZE);

    QCC_DbgPrintf(("Render Digest (%u bytes)", Crypto_SHA1::DIGEST_SIZE));
    QCC_DbgLocalData(&buf[0], Crypto_SHA1::DIGEST_SIZE);

    buf += Crypto_SHA1::DIGEST_SIZE;
    bufSize -= Crypto_SHA1::DIGEST_SIZE;

exit:
    return status;
}
