/**
 * @file
 *
 * This file defines the class for the RSA authentication mechanism
 */

/******************************************************************************
 * Copyright (c) 2010-2012, AllSeen Alliance. All rights reserved.
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
#include <qcc/Debug.h>
#include <qcc/KeyBlob.h>
#include <qcc/Util.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>


#include "KeyStore.h"
#include "AuthMechRSA.h"

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {


#define RAND_LEN  28 // This is the length for random hello data used in RFC 5246
#define PMS_LEN   48 // Per RFC 5246 the premaster secret is always 48 bytes


/**
 * GUID for storing and loading a self-signed cert.
 */
static const char* SELF_CERT_GUID = "9D689C804B9C47C1ADA7397AE0215B26";
static const char* SELF_PRIV_GUID = "B125ABEF3724453899E04B6B1D5C2CC4";

AuthMechRSA::AuthMechRSA(KeyStore& keyStore, ProtectedAuthListener& listener) : AuthMechanism(keyStore, listener), step(255)
{
}

bool AuthMechRSA::GetPassphrase(qcc::String& passphrase, bool toWrite)
{
    bool ok;
    AuthListener::Credentials creds;
    if (toWrite) {
        ok = listener.RequestCredentials(GetName(), authPeer.c_str(), authCount, "", AuthListener::CRED_NEW_PASSWORD, creds);
    } else {
        ok = listener.RequestCredentials(GetName(), authPeer.c_str(), authCount, "", AuthListener::CRED_PASSWORD, creds);
    }
    if (ok) {
        passphrase = creds.GetPassword();
        if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
            expiration = creds.GetExpiration();
        }
    }
    return ok;
}

QStatus AuthMechRSA::Init(AuthRole authRole, const qcc::String& authPeer)
{
    AuthListener::Credentials creds;
    QStatus status = AuthMechanism::Init(authRole, authPeer);
    /* These are the credentials we need */
    uint16_t mask = AuthListener::CRED_CERT_CHAIN | AuthListener::CRED_PRIVATE_KEY | AuthListener::CRED_PASSWORD;
    /*
     * GUIDS for storing cert and private key blobs in the key store
     */
    qcc::GUID128 certGuid(SELF_CERT_GUID);
    qcc::GUID128 privGuid(SELF_PRIV_GUID);
    if (!listener.RequestCredentials(GetName(), authPeer.c_str(), authCount, "", mask, creds)) {
        return ER_AUTH_FAIL;
    }
    if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
        expiration = creds.GetExpiration();
    } else {
        /*
         * Default for AuthMechRSA is to never expire the master key
         */
        expiration = 0xFFFFFFFF;
    }
    /*
     * If the listener didn't provide a cert chain see if we have stored credentials.
     */
    if (!creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
        qcc::GUID128 peerGuid;
        keyStore.GetGuid(peerGuid);
        KeyBlob certBlob;
        QStatus status = keyStore.GetKey(certGuid, certBlob);
        if (status != ER_OK) {
            status = local.rsa.MakeSelfCertificate(peerGuid.ToString(), keyStore.GetApplication());
            if (status == ER_OK) {
                KeyBlob privBlob;
                /*
                 * Get the new cert
                 */
                status = local.rsa.ExportPEM(local.certChain);
                if (status == ER_OK) {
                    /*
                     * Encrypt the private key.
                     */
                    status = local.rsa.ExportPrivateKey(privBlob, this);
                }
                /*
                 * Fail the authentication mechanism if the user rejected the passphrase request.
                 */
                if (status == ER_AUTH_USER_REJECT) {
                    return ER_AUTH_FAIL;
                }
                if (status == ER_OK) {
                    /*
                     * Get the cert into a key blobs
                     */
                    KeyBlob certBlob(local.certChain, KeyBlob::PEM);
                    /*
                     * Store cert and private key in keystore.
                     */
                    keyStore.AddKey(privGuid, privBlob);
                    keyStore.AddKey(certGuid, certBlob);
                }
            }
        } else {
            /*
             * Load up the previously stored cert and private key.
             */
            if (certBlob.GetType() == KeyBlob::PEM) {
                qcc::String pem((const char*)certBlob.GetData(), certBlob.GetSize());
                local.certChain = pem;
            } else {
                status = ER_BUS_KEYBLOB_OP_INVALID;
            }
            KeyBlob privBlob;
            status = keyStore.GetKey(privGuid, privBlob);
            if ((status == ER_OK) && (privBlob.GetType() == KeyBlob::PRIVATE)) {
                /*
                 * Decrypt the private key using the passphrase is there is one.
                 */
                if (creds.IsSet(AuthListener::CRED_PASSWORD)) {
                    status = local.rsa.ImportPrivateKey(privBlob, creds.GetPassword());
                    authCount++;
                } else {
                    status = ER_AUTH_FAIL;
                }
                while (status == ER_AUTH_FAIL) {
                    status = local.rsa.ImportPrivateKey(privBlob, this);
                    authCount++;
                }
            } else {
                status = ER_BUS_KEYBLOB_OP_INVALID;
            }
        }
    } else {
        Crypto_RSA rsa;
        local.certChain = creds.GetCertChain();
        /*
         * This verifies that the cert chain string contains at least one certificate.
         */
        status = rsa.ImportPEM(local.certChain);
        /*
         * Get private key.
         */
        if (status == ER_OK) {
            if (!creds.IsSet(AuthListener::CRED_PRIVATE_KEY)) {
                return ER_AUTH_FAIL;
            }
            qcc::String pkcs8(creds.GetPrivateKey());
            /*
             * Load the private key.
             */
            do {
                /*
                 * We might already have the passphrase.
                 */
                if (creds.IsSet(AuthListener::CRED_PASSWORD)) {
                    status = local.rsa.ImportPKCS8(pkcs8, creds.GetPassword());
                    creds.Clear();
                } else {
                    status = local.rsa.ImportPKCS8(pkcs8, this);
                }
                authCount++;
            } while (status == ER_AUTH_FAIL);
        }
        /*
         * Store cert in the keystore. Note we don't store the entire cert chain.
         */
        if (status == ER_OK) {
            qcc::String pem;
            rsa.ExportPEM(pem);
            KeyBlob certBlob(pem, KeyBlob::PEM);
            keyStore.AddKey(certGuid, certBlob);
        }
    }
    /*
     * msgHash keeps a running hash of all challenges and response sent and received.
     */
    msgHash.Init();
    step = 0;
    return status;
}

void AuthMechRSA::ComputeMS(KeyBlob& pms)
{
    qcc::String seed;
    static const char* label = "master secret";
    if (authRole == CHALLENGER) {
        seed = remote.rand + local.rand;
    } else {
        seed = local.rand + remote.rand;
    }
    uint8_t keymatter[48];
    Crypto_PseudorandomFunction(pms, label, seed, keymatter, sizeof(keymatter));
    masterSecret.Set(keymatter, sizeof(keymatter), KeyBlob::GENERIC);
    masterSecret.SetExpiration(expiration);
}

/*
 * Verifier is computed following approach in RFC 5246. from the master secret and
 * a hash of the entire authentication conversation.
 */
qcc::String AuthMechRSA::ComputeVerifier(const char* label)
{
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
    uint8_t verifier[12];
    /*
     * Snapshot msg hash and compute the verifier string.
     */
    msgHash.GetDigest(digest, true);
    qcc::String seed((const char*)digest, sizeof(digest));
    Crypto_PseudorandomFunction(masterSecret, label, seed, verifier, sizeof(verifier));
    return BytesToHexString(verifier, sizeof(verifier));
}

/*
 * Responses flow from clients to servers
 */
qcc::String AuthMechRSA::InitialResponse(AuthResult& result)
{
    qcc::String response;

    /*
     * Responder starts the conversation by sending a random string
     */
    local.rand = RandHexString(RAND_LEN);
    result = ALLJOYN_AUTH_CONTINUE;
    response = local.rand;

    msgHash.Update((const uint8_t*)response.data(), response.size());

    return response;
}

/*
 * Responses flow from clients to servers
 */
qcc::String AuthMechRSA::Response(const qcc::String& challenge,
                                  AuthResult& result)
{
    QStatus status = ER_OK;
    qcc::String response;

    result = ALLJOYN_AUTH_ERROR;

    QCC_DbgHLPrintf(("Response step %d", step + 1));

    switch (++step) {
    case 1:
        msgHash.Update((const uint8_t*)challenge.data(), challenge.size());
        /*
         * Server has sent a random string, client responds with a cert chain.
         */
        remote.rand = challenge;
        response = local.certChain;
        result = ALLJOYN_AUTH_CONTINUE;
        break;

    case 2:
        msgHash.Update((const uint8_t*)challenge.data(), challenge.size());
        /*
         * Server has sent a cert, client sends premaster secret encrypted with the server's public
         * key.
         */
        remote.certChain = challenge;
        /*
         * Check we have a least one correctly encoded cert in the cert chain.
         */
        status = remote.rsa.ImportPEM(remote.certChain);
        /*
         * Call up to app to accept or reject the cert chain
         */
        if (status == ER_OK) {
            AuthListener::Credentials creds;
            creds.SetCertChain(remote.certChain.c_str());
            if (!listener.VerifyCredentials(GetName(), authPeer.c_str(), creds)) {
                status = ER_AUTH_FAIL;
            }
        }
        if (status == ER_OK) {
            /*
             * Generate and encrypt the 48 byte premaster secret
             */
            KeyBlob pms;
            size_t outLen = remote.rsa.GetSize();
            uint8_t* outBytes = new uint8_t[outLen];
            pms.Rand(PMS_LEN, KeyBlob::GENERIC);
            status = remote.rsa.PublicEncrypt(pms.GetData(), pms.GetSize(), outBytes, outLen);
            if (status == ER_OK) {
                ComputeMS(pms);
                response = BytesToHexString(outBytes, outLen);
                result = ALLJOYN_AUTH_CONTINUE;
            }
            delete [] outBytes;
        }
        break;

    case 3:
        msgHash.Update((const uint8_t*)challenge.data(), challenge.size());
        /*
         * Server has sent a random string. client responds with a certificate verification string.
         */
        {
            size_t outLen = local.rsa.GetSize();
            uint8_t* outBytes = new uint8_t[outLen];
            uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
            /*
             * Snapshot msg hash and compute the verifier string.
             */
            msgHash.GetDigest(digest, true);
            /*
             * Sign msg hash with the client's private key.
             */
            status = local.rsa.SignDigest(digest, sizeof(digest), outBytes, outLen);
            if (status == ER_OK) {
                response = BytesToHexString(outBytes, outLen);
                result = ALLJOYN_AUTH_CONTINUE;
            }
            delete [] outBytes;
        }
        break;

    case 4:
        /*
         * Server has sent a verifier, client responds with a different verifier string.
         */
        if (ComputeVerifier("server finished") == challenge) {
            msgHash.Update((const uint8_t*)challenge.data(), challenge.size());
            response = ComputeVerifier("client finished");
            result = ALLJOYN_AUTH_OK;
        } else {
            QCC_DbgHLPrintf(("Server verifier invalid"));
            result = ALLJOYN_AUTH_FAIL;
        }
        break;

    default:
        break;
    }
    /*
     * Update the running message hash that will be used for verification.
     */
    if (result == ALLJOYN_AUTH_CONTINUE) {
        msgHash.Update((const uint8_t*)response.data(), response.size());
    }
    QCC_DbgHLPrintf(("Response %s", QCC_StatusText(status)));
    return response;
}

qcc::String AuthMechRSA::Challenge(const qcc::String& response,
                                   AuthResult& result)
{
    QStatus status = ER_OK;
    qcc::String challenge;

    result = ALLJOYN_AUTH_ERROR;

    QCC_DbgHLPrintf(("Challenge step %d", step + 1));

    switch (++step) {
    case 1:
        msgHash.Update((const uint8_t*)response.data(), response.size());
        /*
         * Client has sent a random string, server responds with a different random string.
         */
        remote.rand = response;
        local.rand = RandHexString(RAND_LEN);
        challenge = local.rand;
        result = ALLJOYN_AUTH_CONTINUE;
        break;

    case 2:
        msgHash.Update((const uint8_t*)response.data(), response.size());
        /*
         * Client has sent a cert chain, server responds with a cert chain.
         */
        remote.certChain = response;
        /*
         * Check we have a least one correctly encoded cert in the cert chain.
         */
        status = remote.rsa.ImportPEM(remote.certChain);
        /*
         * Call up to app to accept or reject the cert chain
         */
        if (status == ER_OK) {
            AuthListener::Credentials creds;
            creds.SetCertChain(remote.certChain.c_str());
            if (!listener.VerifyCredentials(GetName(), authPeer.c_str(), creds)) {
                status = ER_AUTH_FAIL;
            }
        }
        if (status == ER_OK) {
            challenge = local.certChain;
            result = ALLJOYN_AUTH_CONTINUE;
        }
        break;

    case 3:
        /*
         * Client has sent the premaster secret encrypted with server's public key. Server has to
         * send something in reply so sends a random string.
         */
        msgHash.Update((const uint8_t*)response.data(), response.size());
        {
            size_t inLen = response.size() / 2;
            uint8_t* inBytes = new uint8_t[inLen];
            size_t outLen = local.rsa.MaxDigestSize();
            uint8_t* outBytes = new uint8_t[outLen];
            /*
             * Decrypt the premaster secret.
             */
            if (HexStringToBytes(response, inBytes, inLen) != inLen) {
                status = ER_BAD_STRING_ENCODING;
            } else {
                status = local.rsa.PrivateDecrypt(inBytes, inLen, outBytes, outLen);
                if ((status == ER_OK)  && (outLen != PMS_LEN)) {
                    QCC_DbgHLPrintf(("PrivateDecrypt len=%d", outLen));
                    status = ER_AUTH_FAIL;
                }
            }
            if (status == ER_OK) {
                KeyBlob pms(outBytes, outLen, KeyBlob::GENERIC);
                ComputeMS(pms);
                challenge = RandHexString(RAND_LEN);
                result = ALLJOYN_AUTH_CONTINUE;
            } else {
                result = ALLJOYN_AUTH_FAIL;
            }
            delete [] inBytes;
            delete [] outBytes;
        }
        break;

    case 4:
        /*
         * Client has sent a certificate verification string. Server sends a
         * finish message.
         */
        {
            size_t inLen = response.size() / 2;
            uint8_t* inBytes = new uint8_t[inLen];
            /*
             * Decrypt client's certificate verification string.
             */
            if (HexStringToBytes(response, inBytes, inLen) != inLen) {
                status = ER_BAD_STRING_ENCODING;
            } else {
                uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
                /*
                 * Snapshot msg hash and compute the verifier string.
                 */
                msgHash.GetDigest(digest, true);
                status = remote.rsa.VerifyDigest(digest, sizeof(digest), inBytes, inLen);
            }
            if (status == ER_OK) {
                msgHash.Update((const uint8_t*)response.data(), response.size());
                challenge = ComputeVerifier("server finished");
                result = ALLJOYN_AUTH_CONTINUE;
            } else {
                result = ALLJOYN_AUTH_FAIL;
            }
            delete [] inBytes;
        }
        break;

    case 5:
        /*
         * Client has sent a finish message and we are done.
         */
        if (ComputeVerifier("client finished") == response) {
            result = ALLJOYN_AUTH_OK;
        } else {
            QCC_DbgHLPrintf(("Client verifier invalid"));
            result = ALLJOYN_AUTH_FAIL;
        }
        break;

    default:
        break;
    }
    /*
     * Update the running message hash that will be used for verification.
     */
    if (result == ALLJOYN_AUTH_CONTINUE) {
        msgHash.Update((const uint8_t*)challenge.data(), challenge.size());
    }
    QCC_DbgHLPrintf(("Challenge %s", QCC_StatusText(status)));
    return challenge;
}

}
