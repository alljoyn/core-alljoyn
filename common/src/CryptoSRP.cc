/**
 * @file CryptoSRP.cc
 *
 * Class for SRP
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

#include <qcc/platform.h>

#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>

#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/BigNum.h>
#include <qcc/KeyBlob.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

static const uint8_t Prime1024[] = {
    0xEE, 0xAF, 0x0A, 0xB9, 0xAD, 0xB3, 0x8D, 0xD6, 0x9C, 0x33, 0xF8, 0x0A, 0xFA, 0x8F, 0xC5, 0xE8,
    0x60, 0x72, 0x61, 0x87, 0x75, 0xFF, 0x3C, 0x0B, 0x9E, 0xA2, 0x31, 0x4C, 0x9C, 0x25, 0x65, 0x76,
    0xD6, 0x74, 0xDF, 0x74, 0x96, 0xEA, 0x81, 0xD3, 0x38, 0x3B, 0x48, 0x13, 0xD6, 0x92, 0xC6, 0xE0,
    0xE0, 0xD5, 0xD8, 0xE2, 0x50, 0xB9, 0x8B, 0xE4, 0x8E, 0x49, 0x5C, 0x1D, 0x60, 0x89, 0xDA, 0xD1,
    0x5D, 0xC7, 0xD7, 0xB4, 0x61, 0x54, 0xD6, 0xB6, 0xCE, 0x8E, 0xF4, 0xAD, 0x69, 0xB1, 0x5D, 0x49,
    0x82, 0x55, 0x9B, 0x29, 0x7B, 0xCF, 0x18, 0x85, 0xC5, 0x29, 0xF5, 0x66, 0x66, 0x0E, 0x57, 0xEC,
    0x68, 0xED, 0xBC, 0x3C, 0x05, 0x72, 0x6C, 0xC0, 0x2F, 0xD4, 0xCB, 0xF4, 0x97, 0x6E, 0xAA, 0x9A,
    0xFD, 0x51, 0x38, 0xFE, 0x83, 0x76, 0x43, 0x5B, 0x9F, 0xC6, 0x1D, 0x2F, 0xC0, 0xEB, 0x06, 0xE3
};

static const uint8_t Prime1536[] = {
    0x9D, 0xEF, 0x3C, 0xAF, 0xB9, 0x39, 0x27, 0x7A, 0xB1, 0xF1, 0x2A, 0x86, 0x17, 0xA4, 0x7B, 0xBB,
    0xDB, 0xA5, 0x1D, 0xF4, 0x99, 0xAC, 0x4C, 0x80, 0xBE, 0xEE, 0xA9, 0x61, 0x4B, 0x19, 0xCC, 0x4D,
    0x5F, 0x4F, 0x5F, 0x55, 0x6E, 0x27, 0xCB, 0xDE, 0x51, 0xC6, 0xA9, 0x4B, 0xE4, 0x60, 0x7A, 0x29,
    0x15, 0x58, 0x90, 0x3B, 0xA0, 0xD0, 0xF8, 0x43, 0x80, 0xB6, 0x55, 0xBB, 0x9A, 0x22, 0xE8, 0xDC,
    0xDF, 0x02, 0x8A, 0x7C, 0xEC, 0x67, 0xF0, 0xD0, 0x81, 0x34, 0xB1, 0xC8, 0xB9, 0x79, 0x89, 0x14,
    0x9B, 0x60, 0x9E, 0x0B, 0xE3, 0xBA, 0xB6, 0x3D, 0x47, 0x54, 0x83, 0x81, 0xDB, 0xC5, 0xB1, 0xFC,
    0x76, 0x4E, 0x3F, 0x4B, 0x53, 0xDD, 0x9D, 0xA1, 0x15, 0x8B, 0xFD, 0x3E, 0x2B, 0x9C, 0x8C, 0xF5,
    0x6E, 0xDF, 0x01, 0x95, 0x39, 0x34, 0x96, 0x27, 0xDB, 0x2F, 0xD5, 0x3D, 0x24, 0xB7, 0xC4, 0x86,
    0x65, 0x77, 0x2E, 0x43, 0x7D, 0x6C, 0x7F, 0x8C, 0xE4, 0x42, 0x73, 0x4A, 0xF7, 0xCC, 0xB7, 0xAE,
    0x83, 0x7C, 0x26, 0x4A, 0xE3, 0xA9, 0xBE, 0xB8, 0x7F, 0x8A, 0x2F, 0xE9, 0xB8, 0xB5, 0x29, 0x2E,
    0x5A, 0x02, 0x1F, 0xFF, 0x5E, 0x91, 0x47, 0x9E, 0x8C, 0xE7, 0xA2, 0x8C, 0x24, 0x42, 0xC6, 0xF3,
    0x15, 0x18, 0x0F, 0x93, 0x49, 0x9A, 0x23, 0x4D, 0xCF, 0x76, 0xE3, 0xFE, 0xD1, 0x35, 0xF9, 0xBB
};

/* test vector client random number */
static const uint8_t test_a[] = {
    0x60, 0x97, 0x55, 0x27, 0x03, 0x5C, 0xF2, 0xAD, 0x19, 0x89, 0x80, 0x6F, 0x04, 0x07, 0x21, 0x0B,
    0xC8, 0x1E, 0xDC, 0x04, 0xE2, 0x76, 0x2A, 0x56, 0xAF, 0xD5, 0x29, 0xDD, 0xDA, 0x2D, 0x43, 0x93
};

/* test vector server random number */
static const uint8_t test_b[] = {
    0xE4, 0x87, 0xCB, 0x59, 0xD3, 0x1A, 0xC5, 0x50, 0x47, 0x1E, 0x81, 0xF0, 0x0F, 0x69, 0x28, 0xE0,
    0x1D, 0xDA, 0x08, 0xE9, 0x74, 0xA0, 0x04, 0xF4, 0x9E, 0x61, 0xF5, 0xD1, 0x05, 0x28, 0x4D, 0x20,
};

/* test vector premaster secret */
static const uint8_t test_pms[] = {
    0xB0, 0xDC, 0x82, 0xBA, 0xBC, 0xF3, 0x06, 0x74, 0xAE, 0x45, 0x0C, 0x02, 0x87, 0x74, 0x5E, 0x79,
    0x90, 0xA3, 0x38, 0x1F, 0x63, 0xB3, 0x87, 0xAA, 0xF2, 0x71, 0xA1, 0x0D, 0x23, 0x38, 0x61, 0xE3,
    0x59, 0xB4, 0x82, 0x20, 0xF7, 0xC4, 0x69, 0x3C, 0x9A, 0xE1, 0x2B, 0x0A, 0x6F, 0x67, 0x80, 0x9F,
    0x08, 0x76, 0xE2, 0xD0, 0x13, 0x80, 0x0D, 0x6C, 0x41, 0xBB, 0x59, 0xB6, 0xD5, 0x97, 0x9B, 0x5C,
    0x00, 0xA1, 0x72, 0xB4, 0xA2, 0xA5, 0x90, 0x3A, 0x0B, 0xDC, 0xAF, 0x8A, 0x70, 0x95, 0x85, 0xEB,
    0x2A, 0xFA, 0xFA, 0x8F, 0x34, 0x99, 0xB2, 0x00, 0x21, 0x0D, 0xCC, 0x1F, 0x10, 0xEB, 0x33, 0x94,
    0x3C, 0xD6, 0x7F, 0xC8, 0x8A, 0x2F, 0x39, 0xA4, 0xBE, 0x5B, 0xEC, 0x4E, 0xC0, 0xA3, 0x21, 0x2D,
    0xC3, 0x46, 0xD7, 0xE4, 0x74, 0xB2, 0x9E, 0xDE, 0x8A, 0x46, 0x9F, 0xFE, 0xCA, 0x68, 0x6E, 0x5A
};

/* test vector user id */
static const char* test_I = "alice";

/* test vector password */
static const char* test_P = "password123";

/* test vector salt */
const uint8_t test_s[] = {
    0xBE, 0xB2, 0x53, 0x79, 0xD1, 0xA8, 0x58, 0x1E, 0xB5, 0xA7, 0x27, 0x67, 0x3A, 0x24, 0x41, 0xEE,
};

static const bool PAD = true;

/* We only trust primes that we know */
static bool IsValidPrimeGroup(BigNum& N, BigNum& g)
{
    bool ok = true;
    uint32_t group;
    BigNum prime;

    switch (N.bit_len()) {
    case 1024:
        prime.set_bytes(Prime1024, sizeof(Prime1024));
        group = 2;
        break;

    case 1536:
        prime.set_bytes(Prime1536, sizeof(Prime1536));
        group = 2;
        break;

    default:
        ok = false;
    }
    return ok && (g == group) && (N == prime);
}


namespace qcc {


static bool test = false;

static const int SALT_LEN = 40;


class Crypto_SRP::BN {
  public:
    BigNum a;
    BigNum b;
    BigNum g;
    BigNum k;
    BigNum s;
    BigNum u;
    BigNum v;
    BigNum x;
    BigNum A;
    BigNum B;
    BigNum N;
    BigNum pms;

    void Dump(const char* label) {
        printf("**** %s ****\n", label);
        printf("s = %s\n", s.get_hex().c_str());
        printf("N = %s\n", N.get_hex().c_str());
        printf("g = %s\n", g.get_hex().c_str());
        printf("k = %s\n", k.get_hex().c_str());
        printf("x = %s\n", x.get_hex().c_str());
        printf("v = %s\n", v.get_hex().c_str());
        printf("a = %s\n", a.get_hex().c_str());
        printf("b = %s\n", b.get_hex().c_str());
        printf("A = %s\n", A.get_hex().c_str());
        printf("B = %s\n", B.get_hex().c_str());
        printf("u = %s\n", u.get_hex().c_str());
        printf("premaster secret = %s\n", pms.get_hex().c_str());
    }
};

/*
 * Parse 4 BigNums from ":" seperated hex-encoded string.
 */
static QStatus Parse_Parameters(qcc::String str, BigNum& bn1, BigNum& bn2, BigNum& bn3, BigNum& bn4)
{
    BigNum* bn[4] = { &bn1, &bn2, &bn3, &bn4 };
    size_t i;

    for (i = 0; i < ArraySize(bn); ++i) {
        size_t pos = str.find_first_of(':');
        if (!bn[i]->set_hex(str.substr(0, pos))) {
            return ER_BAD_STRING_ENCODING;
        }
        if (pos == qcc::String::npos) {
            break;
        }
        str.erase(0, pos + 1);
    }
    if (i == 3) {
        return ER_OK;
    } else {
        return ER_BAD_STRING_ENCODING;
    }
}

Crypto_SRP::Crypto_SRP() : bn(new BN) {
}

Crypto_SRP::~Crypto_SRP()
{
    if (test) {
        bn->Dump("");
    }
    delete bn;
}


/* fromServer string is N:g:s:B   toServer string is A */
QStatus Crypto_SRP::ClientInit(const qcc::String& fromServer, qcc::String& toServer)
{
    QStatus status = ER_OK;

    /* Parse N, g, s, and B from parameter string */
    status = Parse_Parameters(fromServer, bn->N, bn->g, bn->s, bn->B);
    if (status != ER_OK) {
        return status;
    }

    /*
     * Check that N and g are valid
     */
    if (!IsValidPrimeGroup(bn->N, bn->g)) {
        return ER_CRYPTO_INSUFFICIENT_SECURITY;
    }

    /*
     * Check that B is valid - B is computed %N so should be <= N and cannot be zero
     */
    if ((bn->B == 0) || (bn->B >= bn->N)) {
        return ER_CRYPTO_ILLEGAL_PARAMETERS;
    }

    /* Generate client random number */
    if (test) {
        bn->a.set_bytes(test_a, sizeof(test_a));
    } else {
        bn->a.gen_rand(32);
    }

    /* Compute A = g^a % N */
    bn->A = bn->g.mod_exp(bn->a, bn->N);

    /* Compose string A to send to server */
    toServer = bn->A.get_hex();

    return ER_OK;
}

static void Update(Crypto_SHA1& sha1, const BigNum& n)
{
    size_t len = n.byte_len();
    uint8_t* buf = new uint8_t[len];
    n.get_bytes(buf, len);
    sha1.Update(buf, len);
    delete [] buf;
}

QStatus Crypto_SRP::ClientFinish(const qcc::String& id, const qcc::String& pwd)
{
    Crypto_SHA1 sha1;
    size_t lenN = bn->N.byte_len();
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
    uint8_t* buf = new uint8_t[lenN];

    /* Compute u = SHA1(PAD(A) | PAD(B)); */
    sha1.Init();
    bn->A.get_bytes(buf, lenN, PAD);
    sha1.Update(buf, lenN);
    bn->B.get_bytes(buf, lenN, PAD);
    sha1.Update(buf, lenN);
    sha1.GetDigest(digest);
    bn->u.set_bytes(digest, Crypto_SHA1::DIGEST_SIZE);

    /* Compute k = SHA1(N | PAD(g)) */
    sha1.Init();
    Update(sha1, bn->N);
    bn->g.get_bytes(buf, lenN, PAD);
    sha1.Update(buf, lenN);
    sha1.GetDigest(digest);
    bn->k.set_bytes(digest, Crypto_SHA1::DIGEST_SIZE);

    /* Compute x = SHA1(s | (SHA1(I | ":" | P)) */
    sha1.Init();
    sha1.Update(id);
    sha1.Update(":");
    sha1.Update(pwd);
    sha1.GetDigest(digest);
    /* outer SHA1 */
    sha1.Init();
    Update(sha1, bn->s);
    sha1.Update(digest, sizeof(digest));
    sha1.GetDigest(digest);
    bn->x.set_bytes(digest, Crypto_SHA1::DIGEST_SIZE);

    /* Calculate premaster secret for client = (B - (k * g^x)) ^ (a + (u * x)) % N  */

    /* (B - (k * g^x)) */
    BigNum tmp1 = (bn->B - bn->k * bn->g.mod_exp(bn->x, bn->N)) % bn->N;
    if (tmp1 < 0) {
        tmp1 += bn->N;
    }
    /* (a + (u * x)) */
    BigNum tmp2 = bn->a + (bn->u * bn->x);

    bn->pms = tmp1.mod_exp(tmp2, bn->N);

    delete [] buf;

    return ER_OK;
}

/*
 * Called with N, g. s. and v initialized.
 */
void Crypto_SRP::ServerCommon(qcc::String& toClient)
{
    size_t lenN = bn->N.byte_len();
    Crypto_SHA1 sha1;
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
    uint8_t* buf = new uint8_t[lenN];

    /* Generate server random number */
    if (test) {
        bn->b.set_bytes(test_b, sizeof(test_b));
    } else {
        bn->b.gen_rand(32);
    }

    /* Compute k = SHA1(N | PAD(g)) */
    sha1.Init();
    Update(sha1, bn->N);
    bn->g.get_bytes(buf, lenN, PAD);
    sha1.Update(buf, lenN);
    sha1.GetDigest(digest);
    bn->k.set_bytes(digest, Crypto_SHA1::DIGEST_SIZE);

    /* Compute B = (k*v + g^b % N) %N */
    bn->B = (bn->k * bn->v + bn->g.mod_exp(bn->b, bn->N)) % bn->N;

    /* Compose string s:B to send to client */
    toClient.erase();
    toClient += bn->N.get_hex();
    toClient += ":";
    toClient += bn->g.get_hex();
    toClient += ":";
    toClient += bn->s.get_hex();
    toClient += ":";
    toClient += bn->B.get_hex();

    delete [] buf;
}

QStatus Crypto_SRP::ServerInit(const qcc::String& verifier, qcc::String& toClient)
{
    /* Parse N, g, s, and v from verifier string */
    QStatus status = Parse_Parameters(verifier, bn->N, bn->g, bn->s, bn->v);
    if (status != ER_OK) {
        return status;
    }
    ServerCommon(toClient);
    return ER_OK;
}

QStatus Crypto_SRP::ServerInit(const qcc::String& id, const qcc::String& pwd, qcc::String& toClient)
{
    Crypto_SHA1 sha1;
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];

    /* Prime and generator */
    bn->N.set_bytes(Prime1024, sizeof(Prime1024));
    bn->g = 2;

    /* Generate the salt */
    if (test) {
        bn->s.set_bytes(test_s, sizeof(test_s));
    } else {
        bn->s.gen_rand(SALT_LEN);
    }

    /* Compute x = SHA1(s | (SHA1(I | ":" | P)) */
    sha1.Init();
    sha1.Update(id);
    sha1.Update(":");
    sha1.Update(pwd);
    sha1.GetDigest(digest);
    /* outer SHA1 */
    sha1.Init();
    Update(sha1, bn->s);
    sha1.Update(digest, sizeof(digest));
    sha1.GetDigest(digest);
    bn->x.set_bytes(digest, Crypto_SHA1::DIGEST_SIZE);

    /* Compute v = g^x % N */
    bn->v = bn->g.mod_exp(bn->x, bn->N);

    ServerCommon(toClient);
    return ER_OK;

}

qcc::String Crypto_SRP::ServerGetVerifier()
{
    qcc::String verifier;

    verifier += bn->N.get_hex();
    verifier += ":";
    verifier += bn->g.get_hex();
    verifier += ":";
    verifier += bn->s.get_hex();
    verifier += ":";
    verifier += bn->v.get_hex();

    return verifier;
}

/* fromClient string is A */
QStatus Crypto_SRP::ServerFinish(const qcc::String fromClient)
{
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
    qcc::String str = fromClient;
    Crypto_SHA1 sha1;
    size_t lenN = bn->N.byte_len();

    /* Parse out A */
    bn->A.set_hex(fromClient);

    /*
     * Check that A is valid - A is computed %N so should be < N and cannot be zero.
     */
    if ((bn->A == 0) || (bn->A >= bn->N)) {
        return ER_CRYPTO_ILLEGAL_PARAMETERS;
    }

    uint8_t* buf = new uint8_t[lenN];

    /* Compute u = SHA1(PAD(A) | PAD(B)); */
    sha1.Init();
    bn->A.get_bytes(buf, lenN, PAD);
    sha1.Update(buf, lenN);
    bn->B.get_bytes(buf, lenN, PAD);
    sha1.Update(buf, lenN);
    sha1.GetDigest(digest);
    bn->u.set_bytes(digest, Crypto_SHA1::DIGEST_SIZE);

    delete [] buf;

    /* Calculate premaster secret for server = ((A * v^u) ^ b %N) */

    /* tmp = (A * v^u) */
    BigNum tmp = (bn->A * bn->v.mod_exp(bn->u, bn->N)) % bn->N;
    /* pms = tmp ^ b % N  */
    bn->pms = tmp.mod_exp(bn->b, bn->N);

    return ER_OK;
}

void Crypto_SRP::GetPremasterSecret(KeyBlob& premaster)
{
    size_t sz = bn->pms.byte_len();
    uint8_t* pms = new uint8_t[sz];
    bn->pms.get_bytes(pms, sz);
    premaster.Set(pms, sz, KeyBlob::GENERIC);
    delete [] pms;
}

QStatus Crypto_SRP::TestVector()
{

    QStatus status;
    Crypto_SRP* server = new Crypto_SRP;
    Crypto_SRP* client = new Crypto_SRP;
    qcc::String toClient;
    qcc::String toServer;

    test = true;
    status = server->ServerInit(test_I, test_P, toClient);
    if (status != ER_OK) {
        QCC_LogError(status, ("SRP ServerInit failed"));
        goto TestFail;
    }

    status = client->ClientInit(toClient, toServer);
    if (status != ER_OK) {
        QCC_LogError(status, ("SRP ClientInit failed"));
        goto TestFail;
    }

    status = server->ServerFinish(toServer);
    if (status != ER_OK) {
        QCC_LogError(status, ("SRP ServerFinish failed"));
        goto TestFail;
    }
    status = client->ClientFinish(test_I, test_P);
    if (status != ER_OK) {
        QCC_LogError(status, ("SRP ClientFinish failed"));
        goto TestFail;
    }
    bn->pms.set_bytes(test_pms, sizeof(test_pms));
    if (bn->pms != client->bn->pms) {
        status = ER_FAIL;
        QCC_LogError(status, ("SRP client premaster secret is incorrect"));
        goto TestFail;
    }
    if (bn->pms != server->bn->pms) {
        status = ER_FAIL;
        QCC_LogError(status, ("SRP server premaster secret is incorrect"));
        goto TestFail;
    }

    test = false;
    delete client;
    delete server;
    return ER_OK;

TestFail:

    delete client;
    delete server;
    test = false;
    return ER_FAIL;
}

}
