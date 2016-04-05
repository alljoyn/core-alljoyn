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

#include <qcc/Util.h>
#include <qcc/Debug.h>

#define QCC_MODULE "SECMGR_STORAGE"

#define AJNCA_HEADER_V1 "ajnca v1.0"

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
QStatus AJNCa::GetDSAPublicKey(ECCPublicKey& publicKey) const
{
    if (nullptr == pkey) {
        QCC_DbgPrintf(("AJNCA is not initialized"));
        return ER_FAIL;
    }
    memcpy(&publicKey, pkey, pkey->GetSize());
    return ER_OK;
}

QStatus AJNCa::GetPrivateKeyBlob(ECCPrivateKey& privateKey, FileSource& fs) const
{
    QStatus status = ER_OK;
    KeyBlob blob;
    size_t total = strlen(AJNCA_HEADER_V1);
    size_t read = 0;
    char buffer[16];
    do {
        size_t numRead = 0;
        status = fs.PullBytes(buffer + read, total - read, numRead, 5000);
        if (ER_OK != status) {
            return status;
        }
        read += numRead;
    } while (total > read);
    if (0 != strncmp(AJNCA_HEADER_V1, buffer, total)) {
        QCC_LogError(ER_FAIL, ("Header doesn't match"));
        return ER_FAIL;
    }
    status = blob.Load(fs);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to read private blob."));
        return status;
    }
    if (KeyBlob::DSA_PRIVATE != blob.GetType()) {
        QCC_LogError(ER_FAIL, ("Corrupt keystore -- key not found. %d", blob.GetType()));
        return ER_FAIL;
    }
    if (privateKey.GetSize() != blob.GetSize()) {
        QCC_LogError(ER_FAIL, ("Keysize mismatch."));
        return ER_FAIL;
    }
    memcpy(&privateKey, blob.GetData(), blob.GetSize());
    return ER_OK;
}

QStatus AJNCa::GetDSAPrivateKey(ECCPrivateKey& privateKey) const
{
    if (store.size() == 0) {
        return ER_FAIL;
    }

    FileSource fs(store);
    return GetPrivateKeyBlob(privateKey, fs);
}

QStatus AJNCa::Init(string storeName)
{
    String store_path = GetHomeDir() + "/.alljoyn_keystore/" + storeName.c_str();

    QCC_DbgPrintf(("Init keyStore at '%s'", store_path.c_str()));
    if (ER_OK != FileExists(store_path)) {
        QCC_DbgPrintf(("Generating new key pair"));
        Crypto_ECC ecc;
        QStatus status = ecc.GenerateDSAKeyPair();
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to generate key pair."));
            return status;
        }
        KeyBlob blob;
        status = blob.Set((const uint8_t*)ecc.GetDSAPrivateKey(),
                          ecc.GetDSAPrivateKey()->GetSize(), KeyBlob::DSA_PRIVATE);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to generate private blob."));
            return status;
        }
        KeyBlob pubblob;
        status = pubblob.Set((const uint8_t*)ecc.GetDSAPublicKey(),
                             ecc.GetDSAPublicKey()->GetSize(), KeyBlob::DSA_PUBLIC);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to generate public blob."));
            return status;
        }
        FileSink fs(store_path, FileSink::PRIVATE);
        if (!fs.IsValid()) {
            QCC_LogError(ER_FAIL, ("Failed to create file '%s'", store_path.c_str()));
            return ER_FAIL;
        }
        size_t total = strlen(AJNCA_HEADER_V1);
        size_t written = 0;
        do {
            size_t numWritten = 0;
            status = fs.PushBytes(AJNCA_HEADER_V1 + written, total - written, numWritten);
            if (ER_OK != status) {
                return status;
            }
            written += numWritten;
        } while (written < total);

        status = blob.Store(fs);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to write private blob."));
            return status;
        }

        status = pubblob.Store(fs);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to write public blob."));
            return status;
        }
        pkey = new ECCPublicKey(*ecc.GetDSAPublicKey());
    } else {
        FileSource fs(store_path);
        ECCPrivateKey key;
        QStatus status = GetPrivateKeyBlob(key, fs);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to read private key."));
            return status;
        }
        KeyBlob pubblob;
        status = pubblob.Load(fs);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to read public blob."));
            return status;
        }
        if (KeyBlob::DSA_PUBLIC != pubblob.GetType()) {
            QCC_LogError(ER_FAIL, ("Corrupt keystore."));
            return ER_FAIL;
        }
        pkey = new ECCPublicKey();
        pkey->Import(pubblob.GetData(), pubblob.GetSize());
    }
    store = store_path;
    return ER_OK;
}

QStatus AJNCa::Reset()
{
    if (store.size() != 0) {
        QStatus status = DeleteFile(store);
        store = "";
        delete pkey;
        pkey = nullptr;
        return status;
    }
    return ER_FAIL;
}

QStatus AJNCa::SignCertificate(CertificateX509& certificate) const
{
    ECCPrivateKey pk;
    QStatus status = GetDSAPrivateKey(pk);
    if (ER_OK != status) {
        return status;
    }
    return certificate.Sign(&pk);
}
}
}
#undef QCC_MODULE
