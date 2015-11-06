/**
 * @file CryptoECC.cc
 *
 * Class for ECC public/private key encryption
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
#include <qcc/Util.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <qcc/Debug.h>
#include <qcc/CryptoECC.h>
#include <qcc/CryptoECCOldEncoding.h>
#include <qcc/Crypto.h>
#include <qcc/CngCache.h>

#include <qcc/CryptoECCMath.h>

#include <Status.h>
#include <bcrypt.h>

using namespace std;

namespace qcc {

#define QCC_MODULE "CRYPTO"

#define CNG_ECC_NIST_P256_KEYSIZE 32

#define EXPIRE_DAYS(days)   (long)(60 * 60 * 24 * (days))

struct ECDSASig {
    ECCBigVal r;
    ECCBigVal s;
};

typedef ECDSASig ECDSA_sig_t;

static const size_t U32_ECDSASIG_SZ = 2 * ECC_BIGVAL_SZ;

struct Crypto_ECC::ECCState {
  public:
    BCRYPT_KEY_HANDLE ecdsaPrivateKey;
    BCRYPT_KEY_HANDLE ecdsaPublicKey;

    BCRYPT_KEY_HANDLE ecdhPrivateKey;
    BCRYPT_KEY_HANDLE ecdhPublicKey;

    /* The authoritative state of the EC{DSA|DH} keys is always the ec{dsa|dh}PrivateKey and
     * ec{dsa|dh}PublicKey handles. The below are used to support key export functions.
     * These fields are only updated to reflect the current state of the keys when
     * the application calls Get{DSA|DH}{Public|Private}Key. Calls to
     * Set{DSA|DH}{Public|Private}Key or Generate{DSA|DH}KeyPair do not update these.
     */
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;

    ECCPrivateKey dhPrivateKey;
    ECCPublicKey dhPublicKey;
};


#define CNG_ECC_ALG_DSA (0x00)
#define CNG_ECC_ALG_DH  (0x01)

DWORD rgAlgAndCurveToPublicBlobMagic[2][1] = {
    { BCRYPT_ECDSA_PUBLIC_P256_MAGIC }, // DSA
    { BCRYPT_ECDH_PUBLIC_P256_MAGIC }   // DH
};
#define GET_PUBLIC_BLOB_MAGIC(__AlgType__, __CurveType__)   (rgAlgAndCurveToPublicBlobMagic[__AlgType__][__CurveType__])

DWORD rgAlgAndCurveToPrivateBlobMagic[2][1] = {
    { BCRYPT_ECDSA_PRIVATE_P256_MAGIC }, // DSA
    { BCRYPT_ECDH_PRIVATE_P256_MAGIC }   // DH
};
#define GET_PRIVATE_BLOB_MAGIC(__AlgType__, __CurveType__)   (rgAlgAndCurveToPrivateBlobMagic[__AlgType__][__CurveType__])

struct ECCSecret::ECCSecretState {
    BCRYPT_SECRET_HANDLE hSecret;

    ECCSecretState() {
        hSecret = NULL;
    }

    ECCSecretState& operator=(const ECCSecretState& other)
    {
        if (this != &other) {
            hSecret = other.hSecret;
        }
        return *this;
    }

    bool operator==(const ECCSecretState& k) const
    {
        return (hSecret == k.hSecret);
    }
};

ECCSecret::ECCSecret()
{
    QCC_DbgTrace(("ECCSecret::ECCSecret"));

    eccSecretState = new ECCSecretState();
}

QStatus ECCSecret::SetSecretState(const ECCSecretState* pEccSecretState)
{
    QStatus status;

    QCC_DbgTrace(("ECCSecret::SetSecretState"));

    if (NULL != eccSecretState->hSecret) {
        NTSTATUS ntStatus;
        ntStatus = BCryptDestroySecret(eccSecretState->hSecret);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to destroy key, ntStatus=%X", ntStatus));
            goto Exit;
        }
    }
    *eccSecretState = *pEccSecretState; // deep not shallow

    status = ER_OK;

Exit:

    return status;
}

QStatus ECCSecret::DerivePreMasterSecret(
    uint8_t* pbPreMasterSecret,
    size_t cbPreMasterSecret)
{
    NTSTATUS ntStatus;
    QStatus status = ER_FAIL;
    errno_t err;

    uint8_t rgbPreMasterSecret[Crypto_SHA256::DIGEST_SIZE];
    DWORD cbResult;

    BCryptBuffer rgParamBuffers[1] = { 0 };

    BCryptBufferDesc BufferDesc;

    QCC_DbgTrace(("ECCSecret::DerivePreMasterSecret"));

    /* set up param buffer */
    rgParamBuffers[0].cbBuffer = sizeof(WCHAR) * (wcslen(BCRYPT_SHA256_ALGORITHM) + 1);
    rgParamBuffers[0].BufferType = KDF_HASH_ALGORITHM;
    rgParamBuffers[0].pvBuffer = BCRYPT_SHA256_ALGORITHM;

    BufferDesc.ulVersion = BCRYPTBUFFER_VERSION;
    BufferDesc.cBuffers = sizeof(rgParamBuffers) / sizeof(BCryptBuffer);
    BufferDesc.pBuffers = rgParamBuffers;

    if (sizeof(rgbPreMasterSecret) != cbPreMasterSecret) {
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        QCC_LogError(status, ("supplied secret %d not equal to SHA256 digest length", cbPreMasterSecret));
        goto Exit;
    }

    if (NULL == eccSecretState->hSecret) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("BCrypt Secret handle is not net set"));
        goto Exit;
    }

    ntStatus = BCryptDeriveKey(eccSecretState->hSecret,
                               BCRYPT_KDF_HASH,
                               &BufferDesc,
                               rgbPreMasterSecret,
                               sizeof(rgbPreMasterSecret),
                               &cbResult,
                               0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to derive key, ntStatus=%X", ntStatus));
        goto Exit;
    }

    err = memcpy_s(pbPreMasterSecret, cbPreMasterSecret, rgbPreMasterSecret, sizeof(rgbPreMasterSecret));
    if (0 != err) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to copy computed premaster secret to output buffer, errno=%d", err));
        goto Exit;
    }

    status = ER_OK;

Exit:

    ClearMemory(rgbPreMasterSecret, sizeof(rgbPreMasterSecret));

    return status;

}

ECCSecret::~ECCSecret()
{
    NTSTATUS ntStatus;

    QCC_DbgTrace(("ECCSecret::~ECCSecret"));

    /* Errors here are non-fatal, but logged for interest because they shouldn't ever fail.
     * If they do fail, we may have some key/secret data floating around in memory still.
     */

    if (NULL != eccSecretState->hSecret) {
        ntStatus = BCryptDestroySecret(eccSecretState->hSecret);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to destroy Secret handle, ntStatus=%X", ntStatus));
        }
    }

    delete eccSecretState;
}

ECCPublicKey* Crypto_ECC_GetPublicKey(
    BCRYPT_KEY_HANDLE hPubKey,
    ECCPublicKey* pEcPubKey)
{
    QStatus status = ER_OK;
    NTSTATUS ntStatus;

    QCC_DbgTrace(("Crypto_ECC_GetPublicKey"));

    PBYTE keyBlob = NULL;
    ULONG blobSize;
    PBCRYPT_ECCKEY_BLOB header = NULL;
    PBYTE x = NULL, y = NULL;

    ntStatus = BCryptExportKey(hPubKey, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, 0, &blobSize, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to get size of key blob, ntStatus=%X", ntStatus));
        goto Exit;
    }

    keyBlob = reinterpret_cast<PBYTE>(malloc(blobSize));

    if (NULL == keyBlob) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("No memory allocating buffer for key blob"));
        goto Exit;
    }

    ntStatus = BCryptExportKey(hPubKey, NULL, BCRYPT_ECCPUBLIC_BLOB, keyBlob, blobSize, &blobSize, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to export public key blob, ntStatus=%X", ntStatus));
        goto Exit;
    }

    header = reinterpret_cast<PBCRYPT_ECCKEY_BLOB>(keyBlob);
    x = keyBlob + sizeof(BCRYPT_ECCKEY_BLOB);
    y = x + header->cbKey;

    status = pEcPubKey->Import(x, header->cbKey, y, header->cbKey);

Exit:

    if (NULL != keyBlob) {
        free(keyBlob);
    }

    if (ER_OK == status) {
        return pEcPubKey;
    } else {
        return NULL;
    }
}

static QStatus Crypto_ECC_SetPublicKey(
    uint8_t CurveType,
    uint8_t AlgType,
    const ECCPublicKey* pubKey,
    BCRYPT_ALG_HANDLE hAlg,
    BCRYPT_KEY_HANDLE*phPubKey)
{
    NTSTATUS ntStatus;
    QStatus status = ER_OK;
    PBYTE keyBlob = NULL;
    PBCRYPT_ECCKEY_BLOB header = NULL;
    ULONG keySize;
    ULONG blobSize;
    size_t exportSize;

    BCRYPT_KEY_HANDLE hPubKey = NULL;

    QCC_DbgTrace(("Crypto_ECC_SetPublicKey"));

    /* Although the above is of type BCRYPT_ECCKEY_BLOB, the actual key data follows the
     * fields of the structure, and so we allocate a block of memory to contain that
     * header and the key data in the following form:
     *
     * BCRYPT_ECCKEY_BLOB { ULONG Magic; ULONG cbKey; }
     * BYTE X[cbKey] // Big-endian.
     * BYTE Y[cbKey] // Big-endian.
     */

    switch (CurveType) {
    case Crypto_ECC::ECC_NIST_P256:
        keySize = CNG_ECC_NIST_P256_KEYSIZE;
        break;

    default:
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        QCC_LogError(status, ("Unrecognized curve type %d", CurveType));
        goto Exit;
    }

    blobSize = sizeof(BCRYPT_ECCKEY_BLOB) + (2 * keySize);
    keyBlob = reinterpret_cast<PBYTE>(malloc(blobSize));

    if (NULL == keyBlob) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("No memory allocating buffer for key blob"));
        goto Exit;
    }

    header = reinterpret_cast<PBCRYPT_ECCKEY_BLOB>(keyBlob);

    switch (CurveType) {
    case Crypto_ECC::ECC_NIST_P256:
        header->dwMagic = GET_PUBLIC_BLOB_MAGIC(AlgType, CurveType);
        break;

    default:
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        QCC_LogError(status, ("Unrecognized curve type %d", CurveType));
        goto Exit;
    }

    header->cbKey = keySize;
    exportSize = 2 * header->cbKey;

    status = pubKey->Export(keyBlob + sizeof(BCRYPT_ECCKEY_BLOB), &exportSize);
    if (ER_OK != status) {
        goto Exit;
    }

    ntStatus = BCryptImportKeyPair(hAlg, NULL, BCRYPT_ECCPUBLIC_BLOB, &hPubKey, keyBlob, blobSize, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to import key blob, ntStatus=%X", ntStatus));
        goto Exit;
    }

    if (NULL != *phPubKey) {
        ntStatus = BCryptDestroyKey(*phPubKey);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Not fatal: Failed to destroy old public key, ntStatus=%X", ntStatus));
            /* Try to carry on anyway. */
        }
    }

    *phPubKey = hPubKey;
    hPubKey = NULL;

Exit:

    if (NULL != keyBlob) {
        free(keyBlob);
    }

    if (NULL != hPubKey) {
        ntStatus = BCryptDestroyKey(hPubKey);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Not fatal: Failed to destroy old public key, ntStatus=%X", ntStatus));
            /* Try to carry on anyway. */
        }
    }

    return status;
}

static ECCPrivateKey* Crypto_ECC_GetPrivateKey(
    BCRYPT_KEY_HANDLE hPrivateKey,
    ECCPrivateKey* pEccPrivateKey)
{
    QStatus status = ER_OK;
    NTSTATUS ntStatus;

    PBYTE keyBlob = NULL;
    ULONG blobSize;
    PBCRYPT_ECCKEY_BLOB header = NULL;
    PBYTE d = NULL;

    QCC_DbgTrace(("Crypto_ECC_GetPrivateKey"));

    ntStatus = BCryptExportKey(hPrivateKey, NULL, BCRYPT_ECCPRIVATE_BLOB, NULL, 0, &blobSize, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to get size of key blob, ntStatus=%X", ntStatus));
        goto Exit;
    }

    keyBlob = reinterpret_cast<PBYTE>(malloc(blobSize));

    if (NULL == keyBlob) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("Out of memory allocating private key blob"));
        goto Exit;
    }

    ntStatus = BCryptExportKey(hPrivateKey, NULL, BCRYPT_ECCPRIVATE_BLOB, keyBlob, blobSize, &blobSize, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to export private key blob, ntStatus=%X", ntStatus));
        goto Exit;
    }

    header = reinterpret_cast<PBCRYPT_ECCKEY_BLOB>(keyBlob);
    d = keyBlob + sizeof(BCRYPT_ECCKEY_BLOB) + (2 * header->cbKey); /* Skip header, X and Y. */

    status = pEccPrivateKey->Import(d, header->cbKey);
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to copy private key out"));
        goto Exit;
    }

Exit:

    if (NULL != keyBlob) {
        ClearMemory(keyBlob, blobSize);
        free(keyBlob);
    }

    if (ER_OK == status) {
        return pEccPrivateKey;
    } else {
        return NULL;
    }
}

static QStatus Crypto_ECC_SetPrivateKey(
    uint8_t CurveType,
    uint8_t AlgType,
    const ECCPrivateKey* privateKey,
    BCRYPT_ALG_HANDLE hAlg,
    BCRYPT_KEY_HANDLE*phPrivateKey)
{
    QStatus status = ER_OK;
    NTSTATUS ntStatus;
    PBYTE keyBlob = NULL;
    PBCRYPT_ECCKEY_BLOB header = NULL;
    PBYTE x = NULL, y = NULL, d = NULL;
    ULONG keySize, blobSize = 0;

    BCRYPT_KEY_HANDLE hPrivateKey = NULL;

    QCC_DbgTrace(("Crypto_ECC_SetPrivateKey"));

    /* Although the above is of type BCRYPT_ECCKEY_BLOB, the actual key data follows the
     * fields of the structure, and so we allocate a block of memory to contain that
     * header and the key data in the following form:
     *
     * BCRYPT_ECCKEY_BLOB { ULONG Magic; ULONG cbKey; }
     * BYTE X[cbKey] // Big-endian.
     * BYTE Y[cbKey] // Big-endian.
     * BYTE d[cbKey] // Big-endian.
     */

    switch (CurveType) {
    case Crypto_ECC::ECC_NIST_P256:
        keySize = CNG_ECC_NIST_P256_KEYSIZE;
        break;

    default:
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        QCC_LogError(status, ("Unrecognized curve type %d", CurveType));
        goto Exit;
    }

    blobSize = sizeof(BCRYPT_ECCKEY_BLOB) + (3 * keySize);
    keyBlob = reinterpret_cast<PBYTE>(malloc(blobSize));

    if (NULL == keyBlob) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("No memory allocating buffer for key blob"));
        goto Exit;
    }

    header = reinterpret_cast<PBCRYPT_ECCKEY_BLOB>(keyBlob);

    switch (CurveType) {
    case Crypto_ECC::ECC_NIST_P256:
        header->dwMagic = GET_PRIVATE_BLOB_MAGIC(AlgType, CurveType);
        break;

    default:
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        QCC_LogError(status, ("Unrecognized curve type %d", CurveType));
        goto Exit;
    }

    header->cbKey = keySize;
    x = keyBlob + sizeof(BCRYPT_ECCKEY_BLOB);
    y = x + header->cbKey;
    d = y + header->cbKey;

    /* Provide zeroes for X and Y since we don't get them from the caller.
     * BCrypt will automatically recompute them based on d.
     */
    memset(x, 0, header->cbKey);
    memset(y, 0, header->cbKey);

    size_t keySizeAsSizeT = header->cbKey;

    status = privateKey->Export(d, &keySizeAsSizeT);
    if (ER_OK != status) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to export private key bytes"));
        goto Exit;
    }
    if (header->cbKey != keySizeAsSizeT) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Change in size of key was unexpected after Export; expected %u, got %" PRIuSIZET, header->cbKey, keySize));
        goto Exit;
    }

    hPrivateKey = *phPrivateKey;

    if (NULL != hPrivateKey) {
        ntStatus = BCryptDestroyKey(hPrivateKey);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to destroy old private key, ntStatus=%X", ntStatus));
            /* Try to carry on anyway. */
        }
    }

    ntStatus = BCryptImportKeyPair(hAlg, NULL, BCRYPT_ECCPRIVATE_BLOB, &hPrivateKey, keyBlob, blobSize, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to import key blob, ntStatus=%X", ntStatus));
        goto Exit;
    }

    *phPrivateKey = hPrivateKey;
    hPrivateKey = NULL;

Exit:

    if (NULL != keyBlob) {
        ClearMemory(keyBlob, blobSize);
        free(keyBlob);
    }

    if (NULL != hPrivateKey) {
        ntStatus = BCryptDestroyKey(hPrivateKey);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to destroy old private key, ntStatus=%X", ntStatus));
            /* Try to carry on anyway. */
        }
    }

    return status;
}

/*
 * Generates the key pair.
 * @param[in]    CurveType identifier of curve to use
 * @param[in]    hAlg cng algorithm handle to use to create keys
 * @param[out]   phPublicKey output public key cng handle
 * @param[out]   phPrivateKey output private key cng handle
 * @return
 *      ER_OK if the key pair is successfully generated.
 *      ER_FAIL otherwise
 *      Other error status.
 */
static QStatus Crypto_ECC_GenerateKeyPair(
    uint8_t CurveType,
    BCRYPT_ALG_HANDLE hAlg,
    BCRYPT_KEY_HANDLE*  phPublicKey,
    BCRYPT_KEY_HANDLE*  phPrivateKey)
{
    QStatus status = ER_OK;
    NTSTATUS ntStatus;
    ULONG keyLength;
    PUCHAR publicBlob = NULL;
    ULONG publicBlobSize;

    BCRYPT_KEY_HANDLE hPublicKey = NULL;
    BCRYPT_KEY_HANDLE hPrivateKey = NULL;

    QCC_DbgTrace(("Crypto_ECC_GenerateKeyPair"));

    QCC_ASSERT(NULL != hAlg);

    switch (CurveType) {
    case Crypto_ECC::ECC_NIST_P256:
        keyLength = 256;
        break;

    default:
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        QCC_LogError(status, ("Unknown curve type %d generating ECC keys", CurveType));
        goto Exit;
    }

    ntStatus = BCryptGenerateKeyPair(hAlg, &hPrivateKey, keyLength, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to generate ECC key pair, ntStatus=%X", status));
        goto Exit;
    }

    ntStatus = BCryptFinalizeKeyPair(hPrivateKey, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to finalize ECC key pair, ntStatus=%X", status));
        goto Exit;
    }

    /* Copy the public part of the generated key to the public key handle.
     * BCryptDuplicateKey only works for summetric keys, so export the public part and
     * import it into the verifier key handle to do the copy.
     */
    ntStatus = BCryptExportKey(hPrivateKey, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, 0, &publicBlobSize, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Could not get blob length for exporting public, ntStatus=%X", ntStatus));
        goto Exit;
    }

    publicBlob = reinterpret_cast<PUCHAR>(malloc(publicBlobSize));
    if (NULL == publicBlob) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("Out of memory allocating public blob"));
        goto Exit;
    }

    ntStatus = BCryptExportKey(hPrivateKey, NULL, BCRYPT_ECCPUBLIC_BLOB, publicBlob, publicBlobSize, &publicBlobSize, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Could not export public part of generated key, ntStatus=%X", ntStatus));
        goto Exit;
    }

    ntStatus = BCryptImportKeyPair(hAlg, NULL, BCRYPT_ECCPUBLIC_BLOB, &hPublicKey, publicBlob, publicBlobSize, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Could not import public key, ntStatus=%X", ntStatus));
        goto Exit;
    }

    *phPrivateKey = hPrivateKey;
    hPrivateKey = NULL;

    *phPublicKey = hPublicKey;
    hPublicKey = NULL;

Exit:

    if (ER_OK != status) {
        if (NULL != hPrivateKey) {
            ntStatus = BCryptDestroyKey(hPrivateKey);
            if (!BCRYPT_SUCCESS(ntStatus)) {
                QCC_LogError(ER_CRYPTO_ERROR, ("Failed to destroy ECC key pair, ntStatus=%X", status));
            }
            hPrivateKey = NULL;
        }
        if (NULL != hPublicKey) {
            ntStatus = BCryptDestroyKey(hPublicKey);
            if (!BCRYPT_SUCCESS(ntStatus)) {
                QCC_LogError(ER_CRYPTO_ERROR, ("Failed to destroy ECC key pair, ntStatus=%X", status));
            }
            hPublicKey = NULL;
        }

    }

    if (NULL != publicBlob) {
        free(publicBlob);
    }

    return status;
}

Crypto_ECC::Crypto_ECC()
{
    QStatus status = ER_OK;
    NTSTATUS ntStatus;

    uint8_t CurveType;

    LPCWSTR ecdsaAlgId = NULL;
    LPCWSTR ecdhAlgId = NULL;

    QCC_DbgTrace(("Crypto_ECC::Crypto_ECC"));

    eccState = new ECCState();

    memset(eccState, 0, sizeof(*eccState));

    CurveType = GetCurveType();

    switch (CurveType) {
    case ECC_NIST_P256:
        ecdsaAlgId = BCRYPT_ECDSA_P256_ALGORITHM;
        ecdhAlgId = BCRYPT_ECDH_P256_ALGORITHM;
        break;

    default:
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        QCC_LogError(status, ("Unrecognized curve type %d", CurveType));
        goto Exit;
    }

    if (NULL == cngCache.ecdsaHandles[CurveType]) {
        ntStatus = BCryptOpenAlgorithmProvider(&cngCache.ecdsaHandles[CurveType], ecdsaAlgId, NULL, 0);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to open ECDSA algorithm provider, ntStatus=%X", ntStatus));
            goto Exit;
        }
    }

    if (NULL == cngCache.ecdhHandles[CurveType]) {
        ntStatus = BCryptOpenAlgorithmProvider(&cngCache.ecdhHandles[CurveType], ecdhAlgId, NULL, 0);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to open ECDH algorithm provider, ntStatus=%X", ntStatus));
            goto Exit;
        }
    }

Exit:

    if (ER_OK != status) {
        /*
         *  Don't clean up any of the BCrypt Algorithm provider handles.
         *  If they have been created on the CNG Cache they'll be freed when that is cleaned up.
         */
        delete eccState;

        /* When better error handling than abort() comes, this assignment can be removed.
         * But to keep things clean, make sure eccState isn't pointing at anything partly initialized.
         */
        eccState = NULL;

        abort();
    }

}

const ECCPublicKey* Crypto_ECC::GetDHPublicKey() const
{
    ECCPublicKey* pEccPubKey = NULL;

    QCC_DbgTrace(("Crypto_ECC::GetDHPublicKey"));

    if (NULL == eccState->ecdhPublicKey) {
        /* No key set. */
        goto Exit;
    }

    pEccPubKey = Crypto_ECC_GetPublicKey(eccState->ecdhPublicKey,
                                         &eccState->dhPublicKey);

Exit:

    return pEccPubKey;

}

void Crypto_ECC::SetDHPublicKey(const ECCPublicKey* pubKey)
{
    uint8_t CurveType = GetCurveType();
    QCC_DbgTrace(("Crypto_ECC::SetDHPublicKey"));

    QStatus status = ER_FAIL;

    status = Crypto_ECC_SetPublicKey(CurveType,
                                     CNG_ECC_ALG_DH,
                                     pubKey,
                                     cngCache.ecdhHandles[CurveType],
                                     &eccState->ecdhPublicKey);

    if (ER_OK != status) {
        abort();
    }

}

const ECCPrivateKey* Crypto_ECC::GetDHPrivateKey() const
{
    ECCPrivateKey*pEccPrivateKey = NULL;

    QCC_DbgTrace(("Crypto_ECC::GetDHPrivateKey"));

    if (NULL == eccState->ecdhPrivateKey) {
        goto Exit;
    }

    pEccPrivateKey = Crypto_ECC_GetPrivateKey(eccState->ecdhPrivateKey,
                                              &eccState->dhPrivateKey);

Exit:

    return pEccPrivateKey;

}

void Crypto_ECC::SetDHPrivateKey(const ECCPrivateKey* privateKey)
{
    uint8_t CurveType = GetCurveType();
    QStatus status = ER_FAIL;

    QCC_DbgTrace(("Crypto_ECC::SetDHPrivateKey"));

    status = Crypto_ECC_SetPrivateKey(CurveType,
                                      CNG_ECC_ALG_DH,
                                      privateKey,
                                      cngCache.ecdhHandles[CurveType],
                                      &eccState->ecdhPrivateKey);

    if (ER_OK != status) {
        abort();
    }
}

QStatus Crypto_ECC::GenerateDHKeyPair() {
    uint8_t CurveType = GetCurveType();

    QCC_DbgTrace(("Crypto_ECC::GenerateDHKeyPair"));

    QCC_ASSERT(NULL != cngCache.ecdhHandles[CurveType]);

    return Crypto_ECC_GenerateKeyPair(CurveType,
                                      cngCache.ecdhHandles[CurveType],
                                      &eccState->ecdhPublicKey,
                                      &eccState->ecdhPrivateKey);
}

QStatus Crypto_ECC::GenerateSharedSecret(const ECCPublicKey* peerPublicKey, ECCSecret* secret)
{
    QStatus status = ER_FAIL;
    NTSTATUS ntStatus;

    Crypto_ECC* pEccPeerPublicKey = NULL;

    BCRYPT_SECRET_HANDLE hSecret = NULL;

    ECCSecret::ECCSecretState eccSecretState;

    QCC_DbgTrace(("Crypto_ECC::GenerateSharedSecret"));

    pEccPeerPublicKey = new Crypto_ECC;

    if (NULL == pEccPeerPublicKey) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("Failed to create a new Elliptic Curve Cryptography object."));
        goto Exit;
    }

    pEccPeerPublicKey->SetDHPublicKey(peerPublicKey);

    ntStatus = BCryptSecretAgreement(eccState->ecdhPrivateKey,
                                     pEccPeerPublicKey->eccState->ecdhPublicKey,
                                     &hSecret,
                                     0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to agree on secret, ntStatus=%X", ntStatus));
        goto Exit;
    }

    eccSecretState.hSecret = hSecret;

    status = secret->SetSecretState(&eccSecretState);
    if (ER_OK != status) {
        QCC_LogError(status, ("Setting secret state failed."));
        goto Exit;
    }

    hSecret = NULL;

    status = ER_OK;

Exit:

    if (NULL != pEccPeerPublicKey) {
        delete pEccPeerPublicKey;
    }

    if (NULL != hSecret) {
        ntStatus = BCryptDestroySecret(hSecret);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            // non fatal logging of destroy secret failing on error path.
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to destroy secret on error path, ntStatus=%X", ntStatus));
        }
    }

    ClearMemory(&eccSecretState, sizeof(ECCSecret::ECCSecretState));

    return status;
}

const ECCPublicKey* Crypto_ECC::GetDSAPublicKey() const
{
    ECCPublicKey* pEccPubKey = NULL;

    QCC_DbgTrace(("Crypto_ECC::GetDSAPublicKey"));

    if (NULL == eccState->ecdsaPublicKey) {
        /* No key set. */
        goto Exit;
    }

    pEccPubKey = Crypto_ECC_GetPublicKey(eccState->ecdsaPublicKey,
                                         &eccState->dsaPublicKey);

Exit:

    return pEccPubKey;

}

void Crypto_ECC::SetDSAPublicKey(const ECCPublicKey* pubKey)
{
    uint8_t CurveType = GetCurveType();
    QStatus status = ER_FAIL;

    QCC_DbgTrace(("Crypto_ECC::SetDSAPublicKey"));

    status = Crypto_ECC_SetPublicKey(CurveType,
                                     CNG_ECC_ALG_DSA,
                                     pubKey,
                                     cngCache.ecdsaHandles[CurveType],
                                     &eccState->ecdsaPublicKey);

    if (ER_OK != status) {
        QCC_LogError(status, ("Crypto_ECC_SetPublicKey failed and aborting."));
        abort();
    }

}

const ECCPrivateKey* Crypto_ECC::GetDSAPrivateKey() const
{
    ECCPrivateKey* pEccPrivateKey = NULL;

    QCC_DbgTrace(("Crypto_ECC::GetDSAPrivateKey"));

    if (NULL == eccState->ecdsaPrivateKey) {
        goto Exit;
    }

    pEccPrivateKey = Crypto_ECC_GetPrivateKey(eccState->ecdsaPrivateKey,
                                              &eccState->dsaPrivateKey);

Exit:

    return pEccPrivateKey;

}

void Crypto_ECC::SetDSAPrivateKey(const ECCPrivateKey* privateKey)
{
    uint8_t CurveType = GetCurveType();
    QStatus status = ER_FAIL;

    QCC_DbgTrace(("Crypto_ECC::SetDSAPrivateKey"));

    status = Crypto_ECC_SetPrivateKey(CurveType,
                                      CNG_ECC_ALG_DSA,
                                      privateKey,
                                      cngCache.ecdsaHandles[CurveType],
                                      &eccState->ecdsaPrivateKey);

    if (ER_OK != status) {
        abort();
    }

}

QStatus Crypto_ECC::GenerateDSAKeyPair()
{
    uint8_t CurveType = GetCurveType();

    QCC_DbgTrace(("Crypto_ECC::GenerateDSAKeyPair"));

    QCC_ASSERT(NULL != cngCache.ecdsaHandles[CurveType]);

    return Crypto_ECC_GenerateKeyPair(CurveType,
                                      cngCache.ecdsaHandles[CurveType],
                                      &eccState->ecdsaPublicKey,
                                      &eccState->ecdsaPrivateKey);
}

QStatus Crypto_ECC::DSASignDigest(const uint8_t* digest, uint16_t len, ECCSignature* sig)
{
    QStatus status = ER_OK;
    NTSTATUS ntStatus;
    errno_t err;
    PBYTE cngSignature = NULL;
    DWORD cngSignatureElementSize, cngSignatureSize;
    DWORD bytesReceived;

    QCC_DbgTrace(("Crypto_ECC::DSASignDigest"));

    switch (GetCurveType()) {
    case ECC_NIST_P256:
        cngSignatureElementSize = CNG_ECC_NIST_P256_KEYSIZE;
        break;

    default:
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        QCC_LogError(status, ("Unrecognized curve type %d", GetCurveType()));
        goto Exit;
    }

    cngSignatureSize = 2 * cngSignatureElementSize * sizeof(BYTE);

    cngSignature = reinterpret_cast<PBYTE>(malloc(cngSignatureSize));

    if (NULL == cngSignature) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("Out of memory allocating CNG signature buffer"));
        goto Exit;
    }

#ifndef NDEBUG
    /* This is a sanity check for only debug builds to make sure our data structures don't change size
     * without us noticing. */
    ntStatus = BCryptSignHash(eccState->ecdsaPrivateKey, NULL, const_cast<PUCHAR>(digest), len, NULL, 0, &bytesReceived, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to get size for signature, ntStatus=%X", ntStatus));
        goto Exit;
    }

    QCC_ASSERT(bytesReceived == cngSignatureSize);
#endif

    /* The type of the buffer input to BCryptSignHash is not marked const, but the function
     * won't try to modify its contents. So it's safe to cast away its const-ness
     * to call it.
     */
    ntStatus = BCryptSignHash(eccState->ecdsaPrivateKey, NULL, const_cast<PUCHAR>(digest), len, cngSignature, cngSignatureSize, &bytesReceived, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to sign digest, ntStatus=%X", ntStatus));
        goto Exit;
    }

    QCC_ASSERT(bytesReceived == cngSignatureSize);


    err = memcpy_s(sig->r, sizeof(sig->r), cngSignature, cngSignatureElementSize);
    if (0 != err) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to copy r into ECCSignature, errno=%d", err));
        goto Exit;
    }

    for (int i = (cngSignatureElementSize / sizeof(sig->r[0]));
         i < (sizeof(sig->r) / sizeof(sig->r[0]));
         i++) {
        sig->r[i] = 0;
    }

    err = memcpy_s(sig->s, sizeof(sig->s), cngSignature + cngSignatureElementSize, cngSignatureElementSize);
    if (0 != err) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to copy s into ECCSignature, errno=%d", err));
        goto Exit;
    }

    for (int i = (cngSignatureElementSize / sizeof(sig->s[0]));
         i < (sizeof(sig->s) / sizeof(sig->s[0]));
         i++) {
        sig->s[i] = 0;
    }

Exit:

    if (NULL != cngSignature) {
        free(cngSignature);
    }

    return status;
}

QStatus Crypto_ECC::DSASign(const uint8_t* buf, uint16_t len, ECCSignature* sig)
{
    Crypto_SHA256 hash;
    QStatus status = ER_OK;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];

    QCC_DbgTrace(("Crypto_ECC::DSASign"));

    status = hash.Init();
    if (ER_OK != status) {
        goto Exit;
    }

    status = hash.Update((const uint8_t*)buf, len);
    if (ER_OK != status) {
        goto Exit;
    }

    status = hash.GetDigest(digest);
    if (ER_OK != status) {
        goto Exit;
    }

    status = DSASignDigest(digest, sizeof(digest), sig);
    if (ER_OK != status) {
        goto Exit;
    }

Exit:

    return status;

}

QStatus Crypto_ECC::DSAVerifyDigest(const uint8_t* digest, uint16_t len, const ECCSignature* sig)
{
    QStatus status = ER_OK;
    NTSTATUS ntStatus;
    errno_t err;
    PBYTE cngSignature = NULL;
    DWORD cngSignatureElementSize, cngSignatureSize;

    QCC_DbgTrace(("Crypto_ECC::DSAVerifyDigest"));

    switch (GetCurveType()) {
    case ECC_NIST_P256:
        cngSignatureElementSize = CNG_ECC_NIST_P256_KEYSIZE;
        break;

    default:
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        QCC_LogError(status, ("Unrecognized curve type %d", GetCurveType()));
        goto Exit;
    }

    /* Interop r/s could be of larger size than what CNG uses. Make sure any extra bytes are zero. */
    for (int i = (cngSignatureElementSize / sizeof(sig->r[0]));
         i < (sizeof(sig->r) / sizeof(sig->r[0]));
         i++) {
        if (sig->r[i] != 0) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Extra high order digits in signature r are not zero"));
            goto Exit;
        }
    }

    for (int i = (cngSignatureElementSize / sizeof(sig->s[0]));
         i < (sizeof(sig->s) / sizeof(sig->s[0]));
         i++) {
        if (sig->s[i] != 0) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Extra high order digits in signature s are not zero"));
            goto Exit;
        }
    }

    cngSignatureSize = 2 * cngSignatureElementSize * sizeof(BYTE);

    cngSignature = reinterpret_cast<PBYTE>(malloc(cngSignatureSize));

    if (NULL == cngSignature) {
        status = ER_OUT_OF_MEMORY;
        QCC_LogError(status, ("Out of memory allocating CNG signature buffer"));
        goto Exit;
    }

    err = memcpy_s(cngSignature, cngSignatureElementSize, sig->r, cngSignatureElementSize);
    if (0 != err) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to copy r into CNG buffer, errno=%d", err));
        goto Exit;
    }

    err = memcpy_s(cngSignature + cngSignatureElementSize, cngSignatureElementSize, sig->s, cngSignatureElementSize);
    if (0 != err) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to copy s into CNG buffer, errno=%d", err));
        goto Exit;
    }

    /* The type of the buffer input to BCryptVerifySignature is not marked const, but the function
     * won't try to modify its contents. So it's safe to cast away its const-ness
     * to call it.
     */
    ntStatus = BCryptVerifySignature(eccState->ecdsaPublicKey, NULL, const_cast<PUCHAR>(digest), len, cngSignature, cngSignatureSize, 0);
    if (!BCRYPT_SUCCESS(ntStatus)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to verify signature, ntStatus=%X", ntStatus));
        goto Exit;
    }

Exit:

    if (NULL != cngSignature) {
        free(cngSignature);
    }

    return status;
}

QStatus Crypto_ECC::DSAVerify(const uint8_t* buf, uint16_t len, const ECCSignature* sig)
{
    Crypto_SHA256 hash;
    QStatus status = ER_OK;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];

    QCC_DbgTrace(("Crypto_ECC::DSAVerify"));

    status = hash.Init();
    if (ER_OK != status) {
        goto Exit;
    }

    status = hash.Update(buf, len);
    if (ER_OK != status) {
        goto Exit;
    }

    status = hash.GetDigest(digest);
    if (ER_OK != status) {
        goto Exit;
    }

    status = DSAVerifyDigest(digest, sizeof(digest), sig);
    if (ER_OK != status) {
        goto Exit;
    }

Exit:

    return status;
}

Crypto_ECC::~Crypto_ECC()
{
    NTSTATUS ntStatus;

    QCC_DbgTrace(("Crypto_ECC::~Crypto_ECC"));

    /* Errors here are non-fatal, but logged for interest because they shouldn't ever fail.
     * If they do fail, we may have some key data floating around in memory still.
     */

    if (NULL != eccState->ecdsaPrivateKey) {
        ntStatus = BCryptDestroyKey(eccState->ecdsaPrivateKey);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to destroy ECDSA key, ntStatus=%X", ntStatus));
        }
    }

    if (NULL != eccState->ecdsaPublicKey) {
        ntStatus = BCryptDestroyKey(eccState->ecdsaPublicKey);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to destroy ECDSA key, ntStatus=%X", ntStatus));
        }
    }

    if (NULL != eccState->ecdhPrivateKey) {
        ntStatus = BCryptDestroyKey(eccState->ecdhPrivateKey);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to destroy ECDH key, ntStatus=%X", ntStatus));
        }
    }

    if (NULL != eccState->ecdhPublicKey) {
        ntStatus = BCryptDestroyKey(eccState->ecdhPublicKey);
        if (!BCRYPT_SUCCESS(ntStatus)) {
            QCC_LogError(ER_CRYPTO_ERROR, ("Failed to destroy ECDH key, ntStatus=%X", ntStatus));
        }
    }

    // deleting eccState recursively calls destructor on private key objects
    // these destructors zero the secrets.
    delete eccState;
}

/** ECCPublicKey **/

/**
 * Empty ECC coordinate
 */
static const uint8_t ECC_COORDINATE_EMPTY[ECC_COORDINATE_SZ] = { 0 };

bool ECCPublicKey::empty() const
{
    return (memcmp(x, ECC_COORDINATE_EMPTY, GetCoordinateSize()) == 0) &&
           (memcmp(y, ECC_COORDINATE_EMPTY, GetCoordinateSize()) == 0);
}

const String ECCPublicKey::ToString() const
{
    String s = "x=[";
    s.append(BytesToHexString(x, GetCoordinateSize()));
    s.append("], y=[");
    s.append(BytesToHexString(y, GetCoordinateSize()));
    s.append("]");
    return s;
}

QStatus ECCPublicKey::Export(uint8_t* data, size_t* size) const
{
    errno_t err;

    const size_t coordinateSize = GetCoordinateSize();

    if (data == NULL || size == NULL || *size < (coordinateSize + coordinateSize)) {
        return ER_FAIL;
    }

    err = memcpy_s(data, *size, x, coordinateSize);
    if (0 != err) {
        QCC_LogError(ER_FAIL, ("Failed to memcpy_s x; err is %d", err));
        return ER_FAIL;
    }
    err = memcpy_s(data + coordinateSize, *size - coordinateSize, y, coordinateSize);
    if (0 != err) {
        QCC_LogError(ER_FAIL, ("Failed to memcpy_s y; err is %d", err));
        return ER_FAIL;
    }
    *size = coordinateSize + coordinateSize;
    return ER_OK;
}

QStatus ECCPublicKey::Import(const uint8_t* data, const size_t size)
{
    errno_t err;

    if (NULL == data) {
        return ER_BAD_ARG_1;
    }
    if (size != GetSize()) {
        return ER_BAD_ARG_2;
    }

    const size_t coordinateSize = GetCoordinateSize();

    err = memcpy_s(x, sizeof(x), data, coordinateSize);
    if (0 != err) {
        QCC_LogError(ER_FAIL, ("Failed to memcpy_s into x; err is %d", err));
        return ER_FAIL;
    }

    err = memcpy_s(y, sizeof(y), data + coordinateSize, coordinateSize);
    if (0 != err) {
        QCC_LogError(ER_FAIL, ("Failed to memcpy_s into y; err is %d", err));
        return ER_FAIL;
    }

    return ER_OK;
}

QStatus ECCPublicKey::Import(const uint8_t* xData, const size_t xSize, const uint8_t* yData, const size_t ySize)
{
    errno_t err;

    if (NULL == xData) {
        return ER_BAD_ARG_1;
    }
    if (GetCoordinateSize() != xSize) {
        return ER_BAD_ARG_2;
    }
    if (NULL == yData) {
        return ER_BAD_ARG_3;
    }
    if (GetCoordinateSize() != ySize) {
        return ER_BAD_ARG_4;
    }

    err = memcpy_s(x, sizeof(x), xData, xSize);
    if (0 != err) {
        QCC_LogError(ER_FAIL, ("Failed to memcpy_s into x; err is %d", err));
        return ER_FAIL;
    }
    err = memcpy_s(y, sizeof(y), yData, ySize);
    if (0 != err) {
        QCC_LogError(ER_FAIL, ("Failed to memcpy_s into y; err is %d", err));
        return ER_FAIL;
    }

    return ER_OK;
}

/** ECCPrivateKey **/

ECCPrivateKey::~ECCPrivateKey()
{
    ClearMemory(d, ECC_COORDINATE_SZ);
}

QStatus ECCPrivateKey::Export(uint8_t* data, size_t* size) const
{
    if (NULL == data) {
        return ER_BAD_ARG_1;
    }
    if (NULL == size) {
        return ER_BAD_ARG_2;
    }
    if (this->GetSize() > *size) {
        *size = this->GetSize();
        return ER_BUFFER_TOO_SMALL;
    }

    *size = this->GetSize();
    memcpy(data, d, *size);

    return ER_OK;
}

const String ECCPrivateKey::ToString() const
{
    qcc::String s = "d=[";
    s.append(BytesToHexString(d, GetSize()));
    s.append("]");
    return s;
}

} /* namespace qcc */
