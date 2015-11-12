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

#include <qcc/CryptoECCMath.h>

#include <qcc/CryptoECCp256.h>

#include <Status.h>

using namespace std;

namespace qcc {

#define QCC_MODULE "CRYPTO"

#define EXPIRE_DAYS(days)   (long)(60 * 60 * 24 * (days))

/* These values describe why the verify failed.  This simplifies testing. */
typedef enum {
    V_SUCCESS = 0, V_R_ZERO, V_R_BIG, V_S_ZERO, V_S_BIG,
    V_INFINITY, V_UNEQUAL, V_INTERNAL
} verify_res_t;

struct ECDSASig {
    ECCBigVal r;
    ECCBigVal s;
};

typedef ECCBigVal bigval_t;
typedef ECCAffinePoint affine_point_t;
typedef ECDSASig ECDSA_sig_t;

static const size_t U32_ECDSASIG_SZ = 2 * ECC_BIGVAL_SZ;

struct Crypto_ECC::ECCState {
  public:

    ECCPrivateKey dhPrivateKey;
    ECCPublicKey dhPublicKey;
    ECCPrivateKey dsaPrivateKey;
    ECCPublicKey dsaPublicKey;
};

struct ECCSecret::ECCSecretState {
    uint8_t z[ECC_COORDINATE_SZ];

    ECCSecretState() {
        memset(z, 0, ECC_COORDINATE_SZ);
    }
    ~ECCSecretState()
    {
        ClearMemory(z, sizeof(z));
    }
    void operator=(const ECCSecretState& k)
    {
        memcpy(z, k.z, ECC_COORDINATE_SZ);
    }
    bool operator==(const ECCSecretState& k) const
    {
        return Crypto_Compare(z, k.z, ECC_COORDINATE_SZ) == 0;
    }
};

#define ECDSA

/*
 * The exported ECDH derive function.  Differs from ECDH_derive_pt
 * only in that it returns just the X coordinate of the derived point.
 * The ECDH_derive functionality is split in two so that the test
 * program can get the entire point.
 */
bool ECDH_derive(bigval_t* tgt, bigval_t const* k, affine_point_t const* Q)
{
    affine_point_t Q2;
    bool rv;

    rv = ECDH_derive_pt(&Q2, k, Q);
    if (rv) {
        *tgt = Q2.x;
    }
    ClearMemory(&Q2, sizeof(affine_point_t));
    return (rv);
}

#ifdef ECDSA
/*
 * This function sets the r and s fields of sig.  The implementation
 * follows HMV Algorithm 4.29.
 */
static int ECDSA_sign(bigval_t const* msgdgst,
                      bigval_t const* privkey,
                      ECDSA_sig_t* sig)
{
    int rv;
    affine_point_t P1;
    bigval_t k;
    bigval_t t;

startpoint:

    rv = ECDH_generate(&P1, &k);
    if (rv) {
        return (rv);
    }

    big_precise_reduce(&sig->r, &P1.x, &orderP);
    if (big_is_zero(&sig->r)) {
        goto startpoint;
    }

    big_mpyP(&t, privkey, &sig->r, MOD_ORDER);
    big_add(&t, &t, msgdgst);
    big_precise_reduce(&t, &t, &orderP); /* may not be necessary */
    big_divide(&sig->s, &t, &k, &orderP);
    if (big_is_zero(&sig->s)) {
        goto startpoint;
    }

    ClearMemory(k.data, ECC_BIGVAL_SZ);
    ClearMemory(t.data, ECC_BIGVAL_SZ);

    return (0);
}

/*
 * Returns B_TRUE if the signature is valid.
 * The implementation follow HMV Algorithm 4.30.
 */
static verify_res_t ECDSA_verify_inner(bigval_t const* msgdgst,
                                       affine_point_t const* pubkey,
                                       ECDSA_sig_t const* sig)
{

    /* We could reuse variables and save stack space.  If stack space
       is tight, u1 and u2 could be the same variable by interleaving
       the big multiplies and the point multiplies. P2 and X could be
       the same variable.  X.x could be reduced in place, eliminating
       v. And if you really wanted to get tricky, I think one could use
       unions between the affine and jacobian versions of points. But
       check that out before doing it. */

    verify_res_t res;
    bigval_t v;
    bigval_t w;
    bigval_t u1;
    bigval_t u2;
    digit256_t digU1;
    digit256_t digU2;
    ecpoint_t Q;
    ecpoint_t P1;
    ecpoint_t P2;
    ecpoint_t G;
    ecpoint_t X;
    ec_t curve;

    bool status;
    QStatus ajstatus;

    ajstatus = ec_getcurve(&curve, NISTP256r1);
    if (ajstatus != ER_OK) {
        /* curve has already been free'd */
        res = V_INTERNAL;
        goto Exit;
    }

    ec_get_generator(&G, &curve);

    status = bigval_to_digit256(&(pubkey->x), Q.x);
    status = status && bigval_to_digit256(&(pubkey->y), Q.y);
    status = status && ecpoint_validation(&Q, &curve);
    if (!status) {
        res = V_INTERNAL;
        goto Exit;
    }

    if (big_cmp(&sig->r, &big_one) < 0) {
        res = V_R_ZERO;
        goto Exit;
    }
    if (big_cmp(&sig->r, &orderP) >= 0) {
        res = V_R_BIG;
        goto Exit;
    }
    if (big_cmp(&sig->s, &big_one) < 0) {
        res = V_S_ZERO;
        goto Exit;
    }
    if (big_cmp(&sig->s, &orderP) >= 0) {
        res = V_S_BIG;
        goto Exit;
    }

    big_divide(&w, &big_one, &sig->s, &orderP);
    big_mpyP(&u1, msgdgst, &w, MOD_ORDER);
    big_precise_reduce(&u1, &u1, &orderP);
    big_mpyP(&u2, &sig->r, &w, MOD_ORDER);
    big_precise_reduce(&u2, &u2, &orderP);

    status = bigval_to_digit256(&u1, digU1);
    status = status && bigval_to_digit256(&u2, digU2);
    if (!status) {
        res = V_INTERNAL;
        goto Exit;
    }

    ec_scalarmul(&(curve.generator), digU1, &P1, &curve);
    ec_scalarmul(&Q, digU2, &P2, &curve);

    // copy P1 to X
    fpcopy_p256(P1.x, X.x);
    fpcopy_p256(P1.y, X.y);

    ec_add(&X, &P2, &curve);

    if (ec_is_infinity(&X, &curve)) {
        res = V_INFINITY;
        goto Exit;
    }

    digit256_to_bigval(X.x, &v);
    if (big_cmp(&v, &sig->r) != 0) {
        res = V_UNEQUAL;
        goto Exit;
    }

    res = V_SUCCESS;

Exit:
    ec_freecurve(&curve);
    return res;
}

bool ECDSA_verify(bigval_t const* msgdgst,
                  affine_point_t const* pubkey,
                  ECDSA_sig_t const* sig)
{
    return (ECDSA_verify_inner(msgdgst, pubkey, sig) == V_SUCCESS);
}


/*
 * Converts a hash value to a bigval_t.  The rules for this in
 * ANSIX9.62 are strange.  Let b be the number of octets necessary to
 * represent the modulus.  If the size of the hash is less than or
 * equal to b, the hash is interpreted directly as a number.
 * Otherwise the left most b octets of the hash are converted to a
 * number. The hash must be big-endian by byte. There is no alignment
 * requirement on hashp.
 */
void ECC_hash_to_bigval(bigval_t* tgt, void const* hashp, unsigned int hashlen)
{
    unsigned int i;

    /* The "4"s in the rest of this function are the number of bytes in
       a uint32_t (what bigval_t's are made of).  The "8" is the number
       of bits in a byte. */

    /* reduce hashlen to modulus size, if necessary */

    if (hashlen > 4 * (BIGLEN - 1)) {
        hashlen = 4 * (BIGLEN - 1);
    }

    *tgt = big_zero;
    /* move one byte at a time starting with least significant byte */
    for (i = 0; i < hashlen; ++i) {
        tgt->data[i / 4] |=
            ((uint8_t*)hashp)[hashlen - 1 - i] << (8 * (i % 4));
    }
}
#endif /* ECDSA */

ECCSecret::ECCSecret()
{
    eccSecretState = new ECCSecretState();
}

QStatus ECCSecret::SetSecretState(const ECCSecretState* pEccSecretState)
{
    *eccSecretState = *pEccSecretState; // calls the overriden = operator = deep copy

    return ER_OK;
}

QStatus ECCSecret::DerivePreMasterSecret(
    uint8_t* pbPreMasterSecret,
    size_t cbPreMasterSecret)
{
    QStatus status;
    Crypto_SHA256 sha;

    if (Crypto_SHA256::DIGEST_SIZE != cbPreMasterSecret) {
        status = ER_CRYPTO_ILLEGAL_PARAMETERS;
        goto Exit;
    }

    sha.Init();
    sha.Update((const uint8_t*)eccSecretState->z, ECC_COORDINATE_SZ);
    sha.GetDigest(pbPreMasterSecret);

    status = ER_OK;

Exit:

    return status;

}

ECCSecret::~ECCSecret()
{
    delete eccSecretState;
}

/*
 * Generates the key pair.
 * @param[out]   publicKey output public key
 * @param[out]   privateKey output private key
 * @return
 *      ER_OK if the key pair is successfully generated.
 *      ER_FAIL otherwise
 *      Other error status.
 */
static QStatus Crypto_ECC_GenerateKeyPair(ECCPublicKey* publicKey, ECCPrivateKey* privateKey)
{
    affine_point_t ap;
    bigval_t k;
    if (ECDH_generate(&ap, &k) != 0) {
        return ER_FAIL;
    }
    const size_t coordinateSize = publicKey->GetCoordinateSize();
    const size_t privateKeySize = privateKey->GetSize();
    uint8_t* X = new uint8_t[coordinateSize];
    uint8_t* Y = new uint8_t[coordinateSize];
    uint8_t* D = new uint8_t[privateKeySize];
    bigval_to_binary(&ap.x, X, coordinateSize);
    bigval_to_binary(&ap.y, Y, coordinateSize);
    bigval_to_binary(&k, D, privateKeySize);

    QStatus status = publicKey->Import(X, coordinateSize, Y, coordinateSize);
    if (ER_OK == status) {
        status = privateKey->Import(D, privateKeySize);
    }
    delete[] X;
    delete[] Y;
    qcc::ClearMemory(D, privateKeySize);
    delete[] D;
    return status;
}

QStatus Crypto_ECC::GenerateDHKeyPair() {
    return Crypto_ECC_GenerateKeyPair(&eccState->dhPublicKey, &eccState->dhPrivateKey);
}

QStatus Crypto_ECC::GenerateSharedSecret(const ECCPublicKey* peerPublicKey, ECCSecret* secret)
{
    bigval_t sec;
    bigval_t prv;
    affine_point_t pub;

    ECCSecret::ECCSecretState eccSecretState;

    QStatus status = ER_FAIL;

    pub.infinity = 0;
    binary_to_bigval(peerPublicKey->GetX(), &pub.x, peerPublicKey->GetCoordinateSize());
    binary_to_bigval(peerPublicKey->GetY(), &pub.y, peerPublicKey->GetCoordinateSize());
    binary_to_bigval(eccState->dhPrivateKey.GetD(), &prv, eccState->dhPrivateKey.GetDSize());
    if (!ECDH_derive(&sec, &prv, &pub)) {
        goto Exit;
    }
    bigval_to_binary(&sec, eccSecretState.z, sizeof(eccSecretState.z));

    status = secret->SetSecretState(&eccSecretState);
    if (ER_OK != status) {
        goto Exit;
    }

    status = ER_OK;

Exit:

    ClearMemory(&eccSecretState, sizeof(ECCSecret::ECCSecretState));
    ClearMemory(sec.data, sizeof(sec.data));
    ClearMemory(prv.data, sizeof(prv.data));

    return status;
}

QStatus Crypto_ECC::GenerateDSAKeyPair()
{
    return Crypto_ECC_GenerateKeyPair(&eccState->dsaPublicKey, &eccState->dsaPrivateKey);
}

/*
 * Sign a digest using the DSA key
 * @param digest The digest to sign
 * @param len The digest len.  Must be equal to 32.
 * @param signingPrivateKey The signing private key
 * @param[out] sig The output signature
 * @return
 *      ER_OK if the signing process succeeds
 *      ER_FAIL otherwise
 *      Other error status.
 */
static QStatus Crypto_ECC_DSASignDigest(const uint8_t* digest, uint32_t len, const ECCPrivateKey* signingPrivateKey, ECCSignature* sig)
{
    if (len != Crypto_SHA256::DIGEST_SIZE) {
        return ER_FAIL;
    }
    bigval_t source;
    ECC_hash_to_bigval(&source, digest, len);
    bigval_t privKey;
    ECDSA_sig_t localSig;
    QStatus Status = ER_FAIL;


    binary_to_bigval(signingPrivateKey->GetD(), &privKey, signingPrivateKey->GetDSize());
    if (ECDSA_sign(&source, &privKey, &localSig) != 0) {
        Status = ER_FAIL;
        goto Exit;
    }
    bigval_to_binary(&localSig.r, sig->r, sizeof(sig->r));
    bigval_to_binary(&localSig.s, sig->s, sizeof(sig->s));


    Status = ER_OK;

Exit:

    ClearMemory(privKey.data, ECC_BIGVAL_SZ);

    return Status;
}

/*
 * Sign a buffer using the DSA key
 * @param buf The buffer to sign
 * @param len The buffer len
 * @param signingPrivateKey The signing private key
 * @param[out] sig The output signature
 * @return
 *      ER_OK if the signing process succeeds
 *      ER_FAIL otherwise
 *      Other error status.
 */
static QStatus Crypto_ECC_DSASign(const uint8_t* buf, uint32_t len, const ECCPrivateKey* signingPrivateKey, ECCSignature* sig)
{
    Crypto_SHA256 hash;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];

    hash.Init();
    hash.Update((const uint8_t*) buf, len);
    hash.GetDigest(digest);
    return Crypto_ECC_DSASignDigest(digest, sizeof(digest), signingPrivateKey, sig);
}

QStatus Crypto_ECC::DSASignDigest(const uint8_t* digest, uint16_t len, ECCSignature* sig)
{
    return Crypto_ECC_DSASignDigest(digest, len, &eccState->dsaPrivateKey, sig);
}

QStatus Crypto_ECC::DSASign(const uint8_t* buf, uint16_t len, ECCSignature* sig) {
    return Crypto_ECC_DSASign(buf, len, &eccState->dsaPrivateKey, sig);
}

/*
 * export style function to verify DSA signature of a digest
 * @param digest The digest to sign
 * @param len The digest len.  Must be 32.
 * @param signingPubKey The signing public key
 * @param sig The signature
 * @return
 *      ER_OK if the signature verification succeeds
 *      ER_FAIL otherwise
 *      Other error status.
 */
static QStatus Crypto_ECC_DSAVerifyDigest(const uint8_t* digest, uint32_t len, const ECCPublicKey* signingPubKey, const ECCSignature* sig)
{
    if (len != Crypto_SHA256::DIGEST_SIZE) {
        return ER_FAIL;
    }
    bigval_t source;
    affine_point_t pub;
    ECDSA_sig_t localSig;

    pub.infinity = 0;
    binary_to_bigval(signingPubKey->GetX(), &pub.x, signingPubKey->GetCoordinateSize());
    binary_to_bigval(signingPubKey->GetY(), &pub.y, signingPubKey->GetCoordinateSize());
    binary_to_bigval(sig->r, &localSig.r, sizeof(sig->r));
    binary_to_bigval(sig->s, &localSig.s, sizeof(sig->s));

    ECC_hash_to_bigval(&source, digest, len);
    if (!ECDSA_verify(&source, &pub, &localSig)) {
        return ER_FAIL;
    }
    return ER_OK;
}

/*
 * export style function to verify DSA signature of a buffer
 * @param buf The buffer to sign
 * @param len The buffer len
 * @param signingPubKey The signing public key
 * @param sig The signature
 * @return
 *      ER_OK if the signature verification succeeds
 *      ER_FAIL otherwise
 *      Other error status.
 */
static QStatus Crypto_ECC_DSAVerify(const uint8_t* buf, uint32_t len, const ECCPublicKey* signingPubKey, const ECCSignature* sig)
{
    Crypto_SHA256 hash;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];

    hash.Init();
    hash.Update((const uint8_t*) buf, len);
    hash.GetDigest(digest);
    return Crypto_ECC_DSAVerifyDigest(digest, sizeof(digest), signingPubKey, sig);
}

QStatus Crypto_ECC::DSAVerifyDigest(const uint8_t* digest, uint16_t len, const ECCSignature* sig)
{
    return Crypto_ECC_DSAVerifyDigest(digest, len, &eccState->dsaPublicKey, sig);
}

QStatus Crypto_ECC::DSAVerify(const uint8_t* buf, uint16_t len, const ECCSignature* sig)
{
    return Crypto_ECC_DSAVerify(buf, len, &eccState->dsaPublicKey, sig);
}

const ECCPublicKey* Crypto_ECC::GetDHPublicKey() const
{
    return &eccState->dhPublicKey;
}

void Crypto_ECC::SetDHPublicKey(const ECCPublicKey* pubKey)
{
    eccState->dhPublicKey = *pubKey;
}

const ECCPrivateKey* Crypto_ECC::GetDHPrivateKey() const
{
    return &eccState->dhPrivateKey;
}

void Crypto_ECC::SetDHPrivateKey(const ECCPrivateKey* privateKey)
{
    eccState->dhPrivateKey = *privateKey;
}

const ECCPublicKey* Crypto_ECC::GetDSAPublicKey() const
{
    return &eccState->dsaPublicKey;
}

void Crypto_ECC::SetDSAPublicKey(const ECCPublicKey* pubKey)
{
    eccState->dsaPublicKey = *pubKey;
}

const ECCPrivateKey* Crypto_ECC::GetDSAPrivateKey() const
{
    return &eccState->dsaPrivateKey;
}

void Crypto_ECC::SetDSAPrivateKey(const ECCPrivateKey* privateKey)
{
    eccState->dsaPrivateKey = *privateKey;
}

Crypto_ECC::Crypto_ECC()
{
    eccState = new ECCState();
}

Crypto_ECC::~Crypto_ECC()
{
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

const qcc::String ECCPublicKey::ToString() const
{
    qcc::String s = "x=[";
    s.append(BytesToHexString(x, GetCoordinateSize()));
    s.append("], y=[");
    s.append(BytesToHexString(y, GetCoordinateSize()));
    s.append("]");
    return s;
}

QStatus ECCPublicKey::Export(uint8_t* data, size_t* size) const
{
    const size_t coordinateSize = GetCoordinateSize();

    if (data == NULL || size == NULL || *size < (coordinateSize + coordinateSize)) {
        return ER_FAIL;
    }

    memcpy(data, x, coordinateSize);
    memcpy(data + coordinateSize, y, coordinateSize);
    *size = coordinateSize + coordinateSize;
    return ER_OK;
}

static bool EC_point_validation(affine_point_t const* pubkey)
{
    bool status;
    QStatus ajstatus;
    ec_t curve;
    ecpoint_t Q;

    ajstatus = ec_getcurve(&curve, NISTP256r1);
    if (ER_OK != ajstatus) {
        return false;
    }

    status = bigval_to_digit256(&(pubkey->x), Q.x);
    status = status && bigval_to_digit256(&(pubkey->y), Q.y);
    status = status && ecpoint_validation(&Q, &curve);

    ec_freecurve(&curve);

    return status;
}

QStatus ECCPublicKey::Import(const uint8_t* data, size_t size)
{
    if (NULL == data) {
        return ER_BAD_ARG_1;
    }
    if (size != GetSize()) {
        return ER_BAD_ARG_2;
    }

    const size_t coordinateSize = GetCoordinateSize();

    return this->Import(data, coordinateSize, data + coordinateSize, coordinateSize);
}

QStatus ECCPublicKey::Import(const uint8_t* xData, const size_t xSize, const uint8_t* yData, const size_t ySize)
{
    if (NULL == xData) {
        return ER_BAD_ARG_1;
    }
    if (this->GetCoordinateSize() != xSize) {
        return ER_BAD_ARG_2;
    }
    if (NULL == yData) {
        return ER_BAD_ARG_3;
    }
    if (this->GetCoordinateSize() != ySize) {
        return ER_BAD_ARG_4;
    }

    /* Verify that this public key is valid. */
    affine_point_t pub;

    pub.infinity = 0;
    binary_to_bigval(xData, &pub.x, xSize);
    binary_to_bigval(yData, &pub.y, ySize);

    if (!EC_point_validation(&pub)) {
        QCC_LogError(ER_CORRUPT_KEYBLOB, ("Failed to import ECCPublicKey."));
        return ER_CORRUPT_KEYBLOB;
    }

    memcpy(x, xData, xSize);
    memcpy(y, yData, ySize);

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
