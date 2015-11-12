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
#include <gtest/gtest.h>

#include <iostream>
#include <qcc/StringUtil.h>
#include <qcc/Crypto.h>
#include <qcc/CryptoECC.h>
#include <qcc/CryptoECCMath.h>

using namespace qcc;
using namespace std;

class CryptoECCTest : public testing::Test {
};

/* used for injecting errors */
static int ToggleRandomBit(void* buf, size_t len)
{
    struct {
        uint64_t byte;
        uint8_t bit;
    } bytebit;


    int rv = get_random_bytes(&bytebit, sizeof (bytebit));
    if (rv) {
        return rv;
    }
    bytebit.byte %= len;
    bytebit.bit %= 8;

    ((uint8_t*)buf)[bytebit.byte] ^= 1 << bytebit.bit;

    return 0;
}

static void AffinePoint2PublicKey(affine_point_t& ap, ECCPublicKey& publicKey) {
    uint8_t* X = new uint8_t[publicKey.GetCoordinateSize()];
    uint8_t* Y = new uint8_t[publicKey.GetCoordinateSize()];
    bigval_to_binary(&ap.x, X, publicKey.GetCoordinateSize());
    bigval_to_binary(&ap.y, Y, publicKey.GetCoordinateSize());
    delete[] X;
    delete[] Y;
}

static void ECDHFullPointTest(int iteration, bool injectError)
{
    bigval_t alicePrivate, bobPrivate;
    affine_point_t alicePublic, bobPublic, aliceFinal, bobFinal;

    EXPECT_EQ(ER_OK, ECDH_generate(&alicePublic, &alicePrivate)) << "ECDHFullPointTest [" << iteration << "]: Fail to generate Alice's key";
    ECCPublicKey aliceECCPublicKey;
    AffinePoint2PublicKey(alicePublic, aliceECCPublicKey);
    EXPECT_EQ(ER_OK, ECDH_generate(&bobPublic, &bobPrivate)) << "ECDHFullPointTest [" << iteration << "]: Fail to generate Bob's key";
    ECCPublicKey bobECCPublicKey;
    AffinePoint2PublicKey(bobPublic, bobECCPublicKey);
    EXPECT_TRUE(ECDH_derive_pt(&aliceFinal, &alicePrivate, &bobPublic)) << "ECDHFullPointTest [" << iteration << "]: Fail to derive Alice's point";
    if (injectError) {
        EXPECT_EQ(0, ToggleRandomBit(&bobPrivate, sizeof(bobPrivate) - sizeof(int))) << "ECDHFullPointTest [" << iteration << "]: Fail to toggle random bits";
    }
    EXPECT_TRUE(ECDH_derive_pt(&bobFinal, &bobPrivate, &alicePublic)) << "ECDHFullPointTest [" << iteration << "]: Fail to derive Bob's point";

    EXPECT_EQ(B_TRUE, in_curveP(&aliceFinal)) << "ECDHFullPointTest [" << iteration << "]: Alice's point is not on the curve";
    EXPECT_EQ(B_TRUE, in_curveP(&bobFinal)) << "ECDHFullPointTest [" << iteration << "]: Bob's point is not on the curve";

    bool hasError = false;
    for (size_t j = 0; j < sizeof (affine_point_t) / sizeof (int); ++j) {
        if (((int*)&aliceFinal)[j] != ((int*)&bobFinal)[j]) {
            hasError = true;
            break;
        }
    }
    if (injectError) {
        EXPECT_TRUE(hasError) << "ECDHFullPointTest [" << iteration << "]: expected error, got none";
    } else {
        EXPECT_FALSE(hasError) << "ECDHFullPointTest [" << iteration << "]: error occurred";
    }
}

static void ECDHHalfPointTest(int iteration)
{
    Crypto_ECC alice;
    Crypto_ECC bob;
    EXPECT_EQ(ER_OK, alice.GenerateDHKeyPair()) << "ECDHHalfPointTest [" << iteration << "]: Fail to generate Alice's key";
    EXPECT_EQ(ER_OK, bob.GenerateDHKeyPair()) << "ECDHHalfPointTest [" << iteration << "]: Fail to generate Bob's key";

    ECCSecret aliceBobSecret;
    ECCSecret bobAliceSecret;
    EXPECT_EQ(ER_OK, alice.GenerateSharedSecret(bob.GetDHPublicKey(), &aliceBobSecret)) << "ECDHHalfPointTest [" << iteration << "]: Fail to generate shared secret with Alice and Bob";
    EXPECT_EQ(ER_OK, bob.GenerateSharedSecret(alice.GetDHPublicKey(), &bobAliceSecret)) << "ECDHHalfPointTest [" << iteration << "]: Fail to generate shared secret with Bob and Alice";
    uint8_t aliceBobDerivedSecret[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, aliceBobSecret.DerivePreMasterSecret(aliceBobDerivedSecret, sizeof(aliceBobDerivedSecret))) << "ECDHHalfPointTest [" << iteration << "]: fail to derive secret";
    uint8_t bobAliceDerivedSecret[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, bobAliceSecret.DerivePreMasterSecret(bobAliceDerivedSecret, sizeof(bobAliceDerivedSecret))) << "ECDHHalfPointTest [" << iteration << "]: fail to derive secret";
    EXPECT_EQ(0, memcmp(aliceBobDerivedSecret, bobAliceDerivedSecret, sizeof(aliceBobDerivedSecret))) << "ECDHHalfPointTest [" << iteration << "]: shared secret don't match";
}

static void ECDHHalfPointTestUsingKey(const ECCPrivateKey& privateKey, const ECCPublicKey& pubKey, const ECCPublicKey& peerPubKey, const uint8_t* expectedSharedSecretDigest)
{
    Crypto_ECC ecc;
    ecc.SetDHPrivateKey(&privateKey);
    ecc.SetDHPublicKey(&pubKey);

    ECCSecret secret;
    EXPECT_EQ(ER_OK, ecc.GenerateSharedSecret(&peerPubKey, &secret)) << "Fail to GenerateSharedSecret";
    uint8_t derivedSecret[Crypto_SHA256::DIGEST_SIZE];
    EXPECT_EQ(ER_OK, secret.DerivePreMasterSecret(derivedSecret, sizeof(derivedSecret))) << "secret.DerivePreMasterSecret failed";
    EXPECT_EQ(0, memcmp(derivedSecret, expectedSharedSecretDigest, sizeof(derivedSecret))) << "shared secret digests don't match";
}

static void ECDHHalfPointTestUsingHexKeys(const char* hexPrivateKey, const char* hexPublicKey, const char* hexPeerPublicKey, const char* hexDigest)
{
    ECCPrivateKey privateKey;
    ECCPublicKey pubKey;
    ECCPublicKey peerPubKey;
    uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
    uint8_t privateKeyBytes[ECC_COORDINATE_SZ];

    HexStringToBytes(hexPrivateKey, privateKeyBytes, sizeof(privateKeyBytes));
    ASSERT_EQ(ER_OK, privateKey.Import(privateKeyBytes, sizeof(privateKeyBytes)));
    uint8_t* buf = new uint8_t[sizeof(ECCPublicKey)];
    HexStringToBytes(hexPublicKey, buf, sizeof(ECCPublicKey));
    ASSERT_EQ(ER_OK, pubKey.Import(buf, sizeof(ECCPublicKey)));
    HexStringToBytes(hexPeerPublicKey, buf, sizeof(ECCPublicKey));
    ASSERT_EQ(ER_OK, peerPubKey.Import(buf, sizeof(ECCPublicKey)));
    delete [] buf;
    HexStringToBytes(hexDigest, digest, sizeof(digest));
    ECDHHalfPointTestUsingKey(privateKey, pubKey, peerPubKey, digest);
}

static void ECDSATest(int iteration, bool injectError,  int dgstlen)
{

    uint8_t*dgst;

    /* create key */
    Crypto_ECC ecc;
    EXPECT_EQ(ER_OK, ecc.GenerateDSAKeyPair()) << "ECDSATest [" << iteration << "]: Fail to generate DSA key pair";
    /* generate message hash */

    dgst = new uint8_t[dgstlen];
    ASSERT_FALSE(NULL == dgst) << "ECDSATest [" << iteration << "]: error allocating memory";

    ASSERT_EQ(0, get_random_bytes(dgst, dgstlen)) << "ECDSATest [" << iteration << "]: get_random_bytes failed";

    ECCSignature sig;
    EXPECT_EQ(ER_OK, ecc.DSASign(dgst, dgstlen, &sig)) << "ECDSATest [" << iteration << "]: error signing";

    if (injectError) {
        /* if the digest length is greater then the modulus
           size, only the most significant bits are used.
           So we make sure we toggle a bit that will get seen. */
        ToggleRandomBit(dgst, min(dgstlen, 4 * (BIGLEN - 1) - 1));
    }

    QStatus status = ecc.DSAVerify(dgst, dgstlen, &sig);
    if (injectError) {
        EXPECT_NE(ER_OK, status) << "ECDSATest [" << iteration << "]: test verify digest length " << dgstlen << " is supposed to fail";
    } else {
        EXPECT_EQ(ER_OK, status) << "ECDSATest [" << iteration << "]: failed test verify digest length " << dgstlen;
    }

    delete [] dgst;
}

/*
 * Checks conversion by doing a random addition problem, c = a + b.
 * It is done once in straight byte array format, and also converted
 * to bigval_t, added there, and converted back.  Explictly tested is
 * the facility to deal with many leading zeros.
 */

#define BYTEVECLEN (4 * BIGLEN + 8)
static void BinaryConversionTest(size_t iteration)
{
    bigval_t a;
    bigval_t b;
    bigval_t c;

    uint8_t a_binary[BYTEVECLEN];
    uint8_t b_binary[BYTEVECLEN];
    uint8_t c_binary[BYTEVECLEN];
    uint8_t c_binary_via_bigval[BYTEVECLEN];

    int rv;
    unsigned s;
    size_t a_len;
    size_t b_len;
    int full_a; /* non-zero means to convert with all leading zeros */
    int full_b; /* ditto */
    int i;

    rv = get_random_bytes(&a_len, sizeof(size_t));
    rv |= get_random_bytes(&b_len, sizeof(size_t));

    EXPECT_FALSE(rv) << "get_random_bytes a_len and b_len failed at iteration " << iteration;

    /* decide wether to do full or tight testing based on MSB */
    full_a = a_len & 0x80000000;
    full_b = b_len & 0x80000000;
    /* always generate postive values */
    a_len %= 4 * BIGLEN - 1;
    b_len %= 4 * BIGLEN - 1;

    memset(a_binary, 0, BYTEVECLEN);
    memset(b_binary, 0, BYTEVECLEN);

    rv = get_random_bytes(a_binary + BYTEVECLEN - a_len, a_len);
    EXPECT_FALSE(rv) << "get_random_bytes for a failed at iteration " << iteration;

    rv = get_random_bytes(b_binary + BYTEVECLEN - b_len, b_len);
    EXPECT_FALSE(rv) << "get_random_bytes for b failed at iteration " << iteration;

    /* cbinary = a_binary + b_binary */
    s = 0;
    for (i = BYTEVECLEN - 1; i >= 0; --i) {
        s += a_binary[i];
        s += b_binary[i];
        c_binary[i] = (uint8_t)s;
        s >>= 8;
    }

    if (full_a) {
        binary_to_bigval(a_binary, &a, BYTEVECLEN);
    } else {
        binary_to_bigval(a_binary + BYTEVECLEN - a_len, &a, a_len);
    }

    if (full_b) {
        binary_to_bigval(b_binary, &b, BYTEVECLEN);
    } else {
        binary_to_bigval(b_binary + BYTEVECLEN - b_len, &b, b_len);
    }

    big_add(&c, &a, &b);
    /* now c holds the sum a+b */

    bigval_to_binary(&c, c_binary_via_bigval, BYTEVECLEN);

    EXPECT_EQ(0, memcmp(c_binary, c_binary_via_bigval, BYTEVECLEN)) <<
        "conversion failed at iteration " << iteration;
}

TEST_F(CryptoECCTest, BinaryConversion)
{
    for (size_t i = 0; i < 100; ++i) {
        BinaryConversionTest(i);
    }
}

/**
 * Test some full point share secret
 */
TEST_F(CryptoECCTest, ECDHFullPointTest)
{
    for (size_t i = 1; i <= 100; ++i) {
        ECDHFullPointTest(i, false);
        ECDHFullPointTest(i, true);
    }
}

/**
 * Test some half point share secret
 */
TEST_F(CryptoECCTest, ECDHHalfPointTest)
{
    for (size_t i = 1; i <= 100; ++i) {
        ECDHHalfPointTest(i);
    }
}

/**
 * Test some half point share secret using keys and expected result generated
 * by openssl
 */
TEST_F(CryptoECCTest, ECDHHalfPointTestWithKeys)
{
    /* keys and shared secret digest are generated by this codes */
    ECDHHalfPointTestUsingHexKeys(
        "1375138F98DABCFDD1BE7D5C23C0CF665BEBA93E8EDB72EDF29EC64F7EDB8887",
        "09C46D15A2F92E20B7345D68DA930A76DD05A58941D9C8FB2907114C9D103483B299F8CE4B52C812AE145AA454D64B99677E91D3EEC82C20D018EAEB9C6B18EE",
        "9078CE25D43F79D5763614BEC9FA1A52D5E392D2D9D3FC30553EC0608E95DC41E6128B725A0DF58B0D7B81AB7C698429C25FDFA14F6D2A636B7FF18406784C76",
        "E61263B6D967B4EB5523FB42CBBE6126DF8DC4D78E5872D6C822EF12299AB7FC");
    /* keys and shared secret digest are generated by openssl codes */
    ECDHHalfPointTestUsingHexKeys(
        "546F93933C810FB3047D4D9112007CBD7C68C2478FA281368D3D53C3A6352695",
        "B53C6794ECAC433CCD4D048230172DC1240999119AD06AC72163E7A385473D8F30BD4C53658732ADE5A67B78AB6AC84BFD536716F67DF3D9FD3BF0A8921A1600",
        "627F6F767920136002EDCA336EE9542716CE6866A89AAFAD2BC066F291EC3BB88F59A41348AD69A377FCA95822C35E45579106CA8C75845CBD529A0C6D6AFB68",
        "795C23437A818C06900916564DD0C60210366EBD34AE59868F715870050669EB");
    /* keys generated by this code and openssl code.  shared secret digest is generated by openssl codes */
    ECDHHalfPointTestUsingHexKeys(
        "F8BB800750D0E8E98BE0E4F6464296C67945C23EE7BF465744D3753F47586AED",
        "1861EA6BCFC64460929004C5741F3C2BB60383223F619456540644C5E47F9F4279535575E6D7A743FB3930256C8469211FAD591C8EAE1F499A41B7101E5D17B6",
        "A86128BAAF103E2372CEF903BC662C94C047BA5806D07290448E41209488885FADD8F966BB710718F671F96D1CD797BA0E4B880F78FD6D361581129BD13DB403",
        "A01C912CFC339F1E4F76D3E73AE7E168553581B358CE643BB8F25C49244419A4");
}

/**
 * Test ECDSA sign and verify.
 */
TEST_F(CryptoECCTest, ECDHE_ECDSA)
{
    int len = 0;
    for (size_t i = 1; i <= 100; ++i) {
        switch (i % 17) {
        case 1:
            /* test lengths shorter than full size */
            len = sizeof (uint32_t) * (BIGLEN - 1) - i % 5;
            break;

        case 2:
            /* test lengths longer than full size */
            len = sizeof (uint32_t) * (BIGLEN - 1) + i % 5;
            break;

        case 3:
            /* test shortest length */
            len = 1;
            break;

        default:
            len = sizeof (uint32_t) * (BIGLEN - 1);
        }
        /* sign with dmj algorithm */
        ECDSATest(i, false, len);
        /* sign with dmj algorithm, inject error */
        ECDSATest(i, true, len);
    }
}

/**
 * Test detection of invalid public keys on import.
 */
TEST_F(CryptoECCTest, ECCPublicKeyImportInvalid)
{
    Crypto_ECC ecc;

    EXPECT_EQ(ER_OK, ecc.GenerateDHKeyPair()) << "Failed to generate DH key pair";

    ECCPublicKey key(*ecc.GetDHPublicKey());
    size_t size = key.GetSize();
    size_t coordinateSize = key.GetCoordinateSize();
    std::vector<uint8_t> data(size);
    uint8_t* y = data.data() + coordinateSize;

    EXPECT_EQ(ER_OK, key.Export(data.data(), &size)) << "Could not export public key";
    EXPECT_EQ(size, key.GetSize()) << "Exported data was an unexpected size " << size;

    std::vector<uint8_t> originalY(data.begin() + coordinateSize, data.end());

    /* Generate random values for the y-coordinate, and so long as we don't manage to randomly
     * re-generate the same y coordinate (which might indicate a problem with the RNG), make sure
     * it doesn't import with the same x coordinate.
     */
    for (int trials = 0; trials < 20; trials++) {
        EXPECT_EQ(ER_OK, Crypto_GetRandomBytes(y, coordinateSize));
        EXPECT_NE(0, memcmp(originalY.data(), y, coordinateSize)) << "Failed to generate a new Y; RNG may be broken";
        EXPECT_NE(ER_OK, key.Import(data.data(), size)) << "Imported key succeeded when it shouldn't have";
        /* Verify that the key remains unchanged by verifying the original Y value is still present. */
        EXPECT_EQ(ER_OK, key.Export(data.data(), &size)) << "Could not re-export key";
        EXPECT_EQ(0, memcmp(originalY.data(), y, coordinateSize)) << "Key data was modified despite failed import";
    }
}
