/**
 * @file CryptoRSA.cc
 *
 * Class for RSA public/private key encryption
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

#include <assert.h>
#include <ctype.h>

#include <qcc/Debug.h>
#include <qcc/Crypto.h>

#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

bool Crypto_RSA::RSA_Init()
{
    return true;
}

void Crypto_RSA::Generate(uint32_t keyLen)
{
    assert(false);
}

QStatus Crypto_RSA::MakeSelfCertificate(const qcc::String& commonName, const qcc::String& app)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::ImportPEM(const qcc::String& pem)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::ImportPKCS8(const qcc::String& pkcs8, const qcc::String& passphrase)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::ImportPKCS8(const qcc::String& pkcs8, PassphraseListener* listener)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::ImportPrivateKey(const qcc::KeyBlob& keyBlob, const qcc::String& passphrase)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::ImportPrivateKey(const qcc::KeyBlob& keyBlob, PassphraseListener* listener)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::ExportPrivateKey(qcc::KeyBlob& keyBlob, const qcc::String& passphrase)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::ExportPrivateKey(qcc::KeyBlob& keyBlob, PassphraseListener* listener)
{
    return ER_NOT_IMPLEMENTED;
}

qcc::String Crypto_RSA::CertToString()
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::ExportPEM(qcc::String& pem)
{
    return ER_NOT_IMPLEMENTED;
}

size_t Crypto_RSA::GetSize()
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::Sign(const uint8_t* data, size_t len, uint8_t* signature, size_t& sigLen)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::Verify(const uint8_t* data, size_t len, const uint8_t* signature, size_t sigLen)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::PublicEncrypt(const uint8_t* inData, size_t inLen, uint8_t* outData, size_t& outLen)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::PrivateDecrypt(const uint8_t* inData, size_t inLen, uint8_t* outData, size_t& outLen)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::SignDigest(const uint8_t* digest, size_t digLen, uint8_t* signature, size_t& sigLen)
{
    return ER_NOT_IMPLEMENTED;
}

QStatus Crypto_RSA::VerifyDigest(const uint8_t* digest, size_t digLen, const uint8_t* signature, size_t sigLen)
{
    return ER_NOT_IMPLEMENTED;
}

Crypto_RSA::Crypto_RSA() : size(0), cert(NULL), key(NULL), certContext(NULL) {
}

Crypto_RSA::~Crypto_RSA() {
}
