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

#include "AJNCa.h"

#include <qcc/Debug.h>

#define QCC_MODULE "SECMGR_STORAGE"

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
QStatus AJNCa::StoreKey(const uint8_t* data, size_t dataSize, KeyBlob::Type keyType)
{
    KeyStore::Key key;
    KeyBlob kb;
    QStatus status = GetLocalKey(keyType, key);
    if (status != ER_OK) {
        return status;
    }
    status = kb.Set(data, dataSize, keyType);
    if (status != ER_OK) {
        QCC_LogError(status, ("Set key blob failure of type %i", keyType));
        return status;
    }

    status = store->AddKey(key, kb);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to store key of type %i", keyType));
        return status;
    }
    return ER_OK;
}

QStatus AJNCa::GetLocalKey(KeyBlob::Type keyType, KeyStore::Key& key)
{
    key.SetType(KeyStore::Key::LOCAL);
    /* each local key will be indexed by a hardcode randomly generated GUID.
       This method is similar to that used by the RSA Key exchange to store
       the private key and cert chain */
    if (keyType == KeyBlob::DSA_PRIVATE) {
        key.SetGUID(GUID128(String("d1b60ce37b271da4b8f0d73b6cd676f5")));
        return ER_OK;
    }
    if (keyType == KeyBlob::DSA_PUBLIC) {
        key.SetGUID(GUID128(String("19409269762da560d78a2cb8a5b2f0c4")));
        return ER_OK;
    }
    QCC_LogError(ER_CRYPTO_KEY_UNAVAILABLE, ("Wrong keytype requested %i", keyType));
    return ER_CRYPTO_KEY_UNAVAILABLE;
}

QStatus AJNCa::GetDSAPublicKey(ECCPublicKey& publicKey) const
{
    KeyStore::Key key;
    KeyBlob kb;
    QStatus status = GetLocalKey(KeyBlob::DSA_PUBLIC, key);
    if (status != ER_OK) {
        return status;
    }
    status = store->GetKey(key, kb);
    if (status != ER_OK) {
        QCC_DbgPrintf(("Failed to retrieve public DSA key from store: %s", QCC_StatusText(status)));
        return status;
    }
    memcpy(&publicKey, kb.GetData(), kb.GetSize());
    return ER_OK;
}

QStatus AJNCa::GetDSAPrivateKey(ECCPrivateKey& privateKey) const
{
    KeyStore::Key key;
    KeyBlob kb;
    QStatus status = GetLocalKey(KeyBlob::DSA_PRIVATE, key);
    if (status != ER_OK) {
        return status;
    }
    status = store->GetKey(key, kb);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to retrieve private DSA key from store."));
        return status;
    }
    memcpy(&privateKey, kb.GetData(), kb.GetSize());
    return ER_OK;
}

QStatus AJNCa::Init(string storeName)
{
    store = new KeyStore(storeName.c_str());

    QStatus status = store->Init(nullptr, true);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to Init key store"));
        return status;
    }
    ECCPublicKey publicKey;
    status = GetDSAPublicKey(publicKey);
    if (status == ER_BUS_KEY_UNAVAILABLE) {
        QCC_DbgPrintf(("Generating new key pair"));
        Crypto_ECC ecc;
        status = ecc.GenerateDSAKeyPair();
        if (status != ER_OK) {
            QCC_LogError(status, ("Failed to generate key pair."));
            return status;
        }

        status = StoreKey((const uint8_t*)ecc.GetDSAPrivateKey(), sizeof(ECCPrivateKey), KeyBlob::DSA_PRIVATE);
        if (status != ER_OK) {
            return status;
        }

        status = StoreKey((const uint8_t*)ecc.GetDSAPublicKey(), sizeof(ECCPublicKey), KeyBlob::DSA_PUBLIC);
        if (status != ER_OK) {
            return status;
        }
        return store->Store();
    }

    return ER_OK;
}

QStatus AJNCa::Reset()
{
    return store->Reset();
}

QStatus AJNCa::SignCertificate(CertificateX509& certificate) const
{
    ECCPrivateKey pk;
    GetDSAPrivateKey(pk);
    return certificate.Sign(&pk);
}
}
}
#undef QCC_MODULE
