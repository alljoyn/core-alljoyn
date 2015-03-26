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

#include <qcc/platform_cpp.h>
#include <qcc/Util.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <qcc/Debug.h>
#include <qcc/CryptoECC.h>
#include <qcc/CryptoECCOldEncoding.h>
#include <qcc/Crypto.h>

#include <qcc/CryptoECCMath.h>

#include <Status.h>

using namespace std;

namespace qcc {

#define QCC_MODULE "CRYPTO"

#define EXPIRE_DAYS(days)   (long)(60 * 60 * 24 * (days))

/* These values describe why the verify failed.  This simplifies testing. */
typedef enum {
    V_SUCCESS = 0, V_R_ZERO, V_R_BIG, V_S_ZERO, V_S_BIG,
    V_INFINITY, V_UNEQUAL
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
    void operator=(const ECCSecretState& k)
    {
        memcpy(z, k.z, ECC_COORDINATE_SZ);
    }
    bool operator==(const ECCSecretState& k) const
    {
        return memcmp(z, k.z, ECC_COORDINATE_SZ) == 0;
    }
};

#define ECDSA

/*
 * The exported ECDH derive function.  Differs from ECHD_derive_pt
 * only in that it returns just the X coordinate of the derived point.
 * The ECDH_derive functionality is split in two so that the test
 * program can get the entire point.
 */
boolean_t ECDH_derive(bigval_t* tgt, bigval_t const* k, affine_point_t const* Q)
{
    affine_point_t Q2;
    boolean_t rv;

    rv = ECDH_derive_pt(&Q2, k, Q);
    if (rv) {
        *tgt = Q2.x;
    }
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

    bigval_t v;
    bigval_t w;
    bigval_t u1;
    bigval_t u2;
    affine_point_t P1;
    affine_point_t P2;
    affine_point_t X;
    jacobian_point_t P2Jacobian;
    jacobian_point_t XJacobian;

    if (big_cmp(&sig->r, &big_one) < 0) {
        return (V_R_ZERO);
    }
    if (big_cmp(&sig->r, &orderP) >= 0) {
        return(V_R_BIG);
    }
    if (big_cmp(&sig->s, &big_one) < 0) {
        return (V_S_ZERO);
    }
    if (big_cmp(&sig->s, &orderP) >= 0) {
        return(V_S_BIG);
    }

    big_divide(&w, &big_one, &sig->s, &orderP);
    big_mpyP(&u1, msgdgst, &w, MOD_ORDER);
    big_precise_reduce(&u1, &u1, &orderP);
    big_mpyP(&u2, &sig->r, &w, MOD_ORDER);
    big_precise_reduce(&u2, &u2, &orderP);
    pointMpyP(&P1, &u1, &base_point);
    pointMpyP(&P2, &u2, pubkey);
    toJacobian(&P2Jacobian, &P2);
    pointAdd(&XJacobian, &P2Jacobian, &P1);
    toAffine(&X, &XJacobian);
    if (X.infinity) {
        return (V_INFINITY);
    }
    big_precise_reduce(&v, &X.x, &orderP);
    if (big_cmp(&v, &sig->r) != 0) {
        return (V_UNEQUAL);
    }
    return (V_SUCCESS);
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

    memset((void*)&sha, 0, sizeof(Crypto_SHA256)); // may get optimized out, need a secure zero memory here.

    return status;

}

ECCSecret::~ECCSecret()
{
    memset(eccSecretState, 0, sizeof(ECCSecretState));
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
    bigval_to_binary(&ap.x, publicKey->x, sizeof(publicKey->x));
    bigval_to_binary(&ap.y, publicKey->y, sizeof(publicKey->y));
    bigval_to_binary(&k, privateKey->d, sizeof(privateKey->d));
    return ER_OK;
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
    binary_to_bigval(peerPublicKey->x, &pub.x, sizeof(peerPublicKey->x));
    binary_to_bigval(peerPublicKey->y, &pub.y, sizeof(peerPublicKey->y));
    binary_to_bigval(eccState->dhPrivateKey.d, &prv, sizeof(eccState->dhPrivateKey.d));
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

    memset((void*)&eccSecretState, 0, sizeof(ECCSecret::ECCSecretState));   // could be optimized out need a secure zero memory

    memset(sec.data, 0, sizeof(sec.data));
    memset(prv.data, 0, sizeof(prv.data));

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

    binary_to_bigval(signingPrivateKey->d, &privKey, sizeof(signingPrivateKey->d));
    if (ECDSA_sign(&source, &privKey, &localSig) != 0) {
        return ER_FAIL;
    }
    bigval_to_binary(&localSig.r, sig->r, sizeof(sig->r));
    bigval_to_binary(&localSig.s, sig->s, sizeof(sig->s));

    return ER_OK;
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
    binary_to_bigval(signingPubKey->x, &pub.x, sizeof(signingPubKey->x));
    binary_to_bigval(signingPubKey->y, &pub.y, sizeof(signingPubKey->y));
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

const qcc::String ECCPublicKey::ToString() const
{
    qcc::String s = "x=[";
    s.append(BytesToHexString(x, ECC_COORDINATE_SZ));
    s.append("], y=[");
    s.append(BytesToHexString(y, ECC_COORDINATE_SZ));
    s.append("]");
    return s;
}

QStatus ECCPublicKey::Export(uint8_t* data, size_t* size) const
{
    if (data == NULL || size == NULL || *size < (ECC_COORDINATE_SZ + ECC_COORDINATE_SZ)) {
        return ER_FAIL;
    }

    memcpy(data, x, ECC_COORDINATE_SZ);
    memcpy(data + ECC_COORDINATE_SZ, y, ECC_COORDINATE_SZ);
    *size = ECC_COORDINATE_SZ + ECC_COORDINATE_SZ;
    return ER_OK;
}

QStatus ECCPublicKey::Import(const uint8_t* data, size_t size)
{
    if ((size != (ECC_COORDINATE_SZ + ECC_COORDINATE_SZ)) || (!data)) {
        return ER_FAIL;
    }

    memcpy(x, data, ECC_COORDINATE_SZ);
    memcpy(y, data + ECC_COORDINATE_SZ, ECC_COORDINATE_SZ);

    return ER_OK;
}

} /* namespace qcc */
