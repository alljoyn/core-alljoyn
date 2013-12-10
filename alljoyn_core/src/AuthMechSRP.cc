/**
 * @file
 *
 * This file defines the class for the ALLJOYN_SRP_KEYX authentication mechanism
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

#include <assert.h>

#include <qcc/Crypto.h>
#include <qcc/Util.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include "KeyStore.h"
#include "AuthMechSRP.h"

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {

/*
 * Per RFC 5426 (TLS) the random nonce should be 28 bytes which is what we are using also.
 */
#define NONCE_LEN  28

AuthMechSRP::AuthMechSRP(KeyStore& keyStore, ProtectedAuthListener& listener) : AuthMechanism(keyStore, listener), step(255)
{
}

QStatus AuthMechSRP::Init(AuthRole authRole, const qcc::String& authPeer)
{
    AuthMechanism::Init(authRole, authPeer);
    step = 0;
    /*
     * Default for AuthMechSRP is to never expire the master key
     */
    expiration = 0xFFFFFFFF;
    /*
     * msgHash keeps a running hash of all challenges and responses sent and received.
     */
    msgHash.Init();
    return ER_OK;
}

static const char* label = "master secret";

/*
 * Compute the key exchange key from the master secret using HMAC-SHA1 using the algorithm described
 * in RFC 5246.
 */
void AuthMechSRP::ComputeMS()
{
    uint8_t keymatter[48];
    KeyBlob pms;
    srp.GetPremasterSecret(pms);
    QCC_DbgHLPrintf(("PMS:  %s", BytesToHexString(pms.GetData(), pms.GetSize()).c_str()));
    /*
     * Use the PRF function to compute the master secret.
     */
    Crypto_PseudorandomFunction(pms, label, clientRandom + serverRandom, keymatter, sizeof(keymatter));
    masterSecret.Set(keymatter, sizeof(keymatter), KeyBlob::GENERIC);
    QCC_DbgHLPrintf(("MasterSecret:  %s", BytesToHexString(masterSecret.GetData(), masterSecret.GetSize()).c_str()));
    masterSecret.SetExpiration(expiration);
}

/*
 * Verifier is computed following approach in RFC 5246. from the master secret and
 * a hash of the entire authentication conversation.
 */
qcc::String AuthMechSRP::ComputeVerifier(const char* label)
{
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
    uint8_t verifier[12];
    /*
     * Snapshot msg hash and compute the verifier string.
     */
    msgHash.GetDigest(digest, true);
    qcc::String seed((const char*)digest, sizeof(digest));
    Crypto_PseudorandomFunction(masterSecret, label, seed, verifier, sizeof(verifier));
    QCC_DbgHLPrintf(("Verifier(%s):  %s", label, BytesToHexString(verifier, sizeof(verifier)).c_str()));
    return BytesToHexString(verifier, sizeof(verifier));
}

qcc::String AuthMechSRP::InitialResponse(AuthResult& result)
{
    /*
     * Client starts the conversation by sending a random string.
     */
    QCC_DbgHLPrintf(("InitialResponse"));
    qcc::String response = RandHexString(NONCE_LEN);
    clientRandom = HexStringToByteString(response);
    result = ALLJOYN_AUTH_CONTINUE;

    msgHash.Update((const uint8_t*)response.data(), response.size());

    return response;
}

qcc::String AuthMechSRP::Response(const qcc::String& challenge,
                                  AuthResult& result)
{
    QCC_DbgHLPrintf(("Response %d", step + 1));
    QStatus status = ER_OK;
    qcc::String response;
    qcc::String pwd;
    size_t pos;
    AuthListener::Credentials creds;

    result = ALLJOYN_AUTH_CONTINUE;

    switch (++step) {
    case 1:
        msgHash.Update((const uint8_t*)challenge.data(), challenge.size());
        /*
         * Server sends an intialization string, client respond with its initialization string.
         */
        status = srp.ClientInit(challenge, response);
        break;

    case 2:
        /*
         * Server sends a random nonce concatenated with a verifier string.
         */
        pos = challenge.find_first_of(":");
        serverRandom = HexStringToByteString(challenge.substr(0, pos));
        if (pos == qcc::String::npos) {
            /*
             * String is incorrectly formatted - fail the authentication
             */
            QCC_LogError(ER_FAIL, ("AuthMechSRP::Response has wrong format"));
            result = ALLJOYN_AUTH_FAIL;
            break;
        }
        if (listener.RequestCredentials(GetName(), authPeer.c_str(), authCount, "", AuthListener::CRED_PASSWORD, creds)) {
            if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
                expiration = creds.GetExpiration();
            }
            status = srp.ClientFinish("<anonymous>", creds.GetPassword());
            if (status == ER_OK) {
                ComputeMS();
                /*
                 * Client Can now check server's verifier and generate client's verifier.
                 */
                if (ComputeVerifier("server finish") == challenge.substr(pos + 1)) {
                    msgHash.Update((const uint8_t*)challenge.data(), challenge.size());
                    response = ComputeVerifier("client finish");
                    result = ALLJOYN_AUTH_OK;
                } else {
                    result = ALLJOYN_AUTH_RETRY;
                }
            }
        } else {
            result = ALLJOYN_AUTH_FAIL;
        }
        break;

    default:
        result = ALLJOYN_AUTH_ERROR;
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("AuthMechSRP::Response"));
        result = ALLJOYN_AUTH_ERROR;
    }
    /*
     * Update the running message hash that will be used for verification.
     */
    if (result == ALLJOYN_AUTH_CONTINUE) {
        msgHash.Update((const uint8_t*)response.data(), response.size());
    }
    return response;
}

qcc::String AuthMechSRP::Challenge(const qcc::String& response,
                                   AuthResult& result)
{
    QCC_DbgHLPrintf(("Challenge %d", step + 1));
    QStatus status = ER_OK;
    qcc::String challenge;
    qcc::String pwd;
    AuthListener::Credentials creds;

    result = ALLJOYN_AUTH_CONTINUE;

    switch (++step) {
    case 1:
        msgHash.Update((const uint8_t*)response.data(), response.size());
        /*
         * Client sends a random string. Server returns an SRP string.
         */
        clientRandom = HexStringToByteString(response);
        if (listener.RequestCredentials(GetName(), authPeer.c_str(), authCount, "", AuthListener::CRED_ONE_TIME_PWD, creds)) {
            if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
                expiration = creds.GetExpiration();
            }
            status = srp.ServerInit("<anonymous>", creds.GetPassword(), challenge);
        } else {
            result = ALLJOYN_AUTH_FAIL;
        }
        break;

    case 2:
        msgHash.Update((const uint8_t*)response.data(), response.size());
        /*
         * Client sends its SRP string, server response with a random string, and its verifier.
         */
        status = srp.ServerFinish(response);
        if (status == ER_OK) {
            challenge = RandHexString(NONCE_LEN);
            serverRandom = HexStringToByteString(challenge);
            ComputeMS();
            challenge += ":" + ComputeVerifier("server finish");
            result = ALLJOYN_AUTH_CONTINUE;
        }
        break;

    case 3:
        /*
         * Client responds with its verifier and we are done.
         */
        if (response == ComputeVerifier("client finish")) {
            result = ALLJOYN_AUTH_OK;
        } else {
            result = ALLJOYN_AUTH_RETRY;
        }
        break;

    default:
        result = ALLJOYN_AUTH_ERROR;
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("AuthMechSRP::Challenge"));
        result = ALLJOYN_AUTH_ERROR;
    }
    /*
     * Update the running message hash that will be used for verification.
     */
    if (result == ALLJOYN_AUTH_CONTINUE) {
        msgHash.Update((const uint8_t*)challenge.data(), challenge.size());
    }
    return challenge;
}

}
