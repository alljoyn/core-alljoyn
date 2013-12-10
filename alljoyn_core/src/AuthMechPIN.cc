/**
 * @file
 *
 * This file defines the class for the ALLJOYN_PIN_KEYX authentication mechanism
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
#include "AuthMechPIN.h"

#define QCC_MODULE "PIN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {

/*
 * Length of the random nonce.
 */
#define NONCE_LEN  28

AuthMechPIN::AuthMechPIN(KeyStore& keyStore, ProtectedAuthListener& listener) : AuthMechanism(keyStore, listener)
{
}

QStatus AuthMechPIN::Init(AuthRole authRole, const qcc::String& authPeer)
{
    AuthMechanism::Init(authRole, authPeer);
    /*
     * Default for AuthMechPIN is to never expire the master key
     */
    expiration = 0xFFFFFFFF;
    return ER_OK;
}

qcc::String AuthMechPIN::ComputeVerifier(const char* label)
{
    uint8_t verifier[12];

    Crypto_PseudorandomFunctionCCM(masterSecret, label, "", verifier, sizeof(verifier));
    QCC_DbgHLPrintf(("Verifier(%s):  %s", label, BytesToHexString(verifier, sizeof(verifier)).c_str()));
    qcc::String ver = BytesToHexString(verifier, sizeof(verifier));
    QCC_DbgHLPrintf(("ComputeVerifier(%s):  %s", label, ver.c_str()));
    return ver;
}

static const char* msLabel = "master secret";

/*
 * Compute the key exchange key from the server and client nonces and the pin code.
 */
void AuthMechPIN::ComputeMS(const qcc::String& otherNonce, const qcc::String& pincode)
{
    uint8_t keymatter[24];
    KeyBlob secret((uint8_t*)pincode.data(), pincode.size(), KeyBlob::GENERIC);
    qcc::String nonceCat = nonce + otherNonce;

    QCC_DbgHLPrintf(("Nonce:  %s", BytesToHexString((uint8_t*)nonceCat.data(), nonceCat.size()).c_str()));
    /*
     * Use the PRF function to compute the master secret and verifier string.
     */
    Crypto_PseudorandomFunctionCCM(secret, msLabel, nonceCat, keymatter, sizeof(keymatter));
    masterSecret.Set(keymatter, sizeof(keymatter), KeyBlob::GENERIC);
    QCC_DbgHLPrintf(("MasterSecret:  %s", BytesToHexString(masterSecret.GetData(), masterSecret.GetSize()).c_str()));
    masterSecret.SetExpiration(expiration);
}

qcc::String AuthMechPIN::InitialResponse(AuthResult& result)
{
    qcc::String response = RandHexString(NONCE_LEN);
    /*
     * Client starts the conversation by sending a random string.
     */
    nonce = HexStringToByteString(response);
    result = ALLJOYN_AUTH_CONTINUE;
    return response;
}

qcc::String AuthMechPIN::Response(const qcc::String& challenge, AuthResult& result)
{
    AuthListener::Credentials creds;
    qcc::String response;

    /*
     * Server sends a random nonce concatenated with a verifier string.
     */
    size_t pos = challenge.find_first_of(":");
    if (pos == qcc::String::npos) {
        /*
         * String is incorrectly formatted - fail the authentication
         */
        QCC_LogError(ER_FAIL, ("AuthMechPIN::Response has wrong format"));
        result = ALLJOYN_AUTH_FAIL;
        return response;
    }
    qcc::String serverNonce = HexStringToByteString(challenge.substr(0, pos));
    if (serverNonce.size() != NONCE_LEN) {
        result = ALLJOYN_AUTH_FAIL;
    } else if (listener.RequestCredentials(GetName(), authPeer.c_str(), authCount, "", AuthListener::CRED_PASSWORD, creds)) {
        if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
            expiration = creds.GetExpiration();
        }
        ComputeMS(serverNonce, creds.GetPassword());
        if (ComputeVerifier("server finish") == challenge.substr(pos + 1)) {
            response = ComputeVerifier("client finish");
            result = ALLJOYN_AUTH_OK;
        } else {
            result = ALLJOYN_AUTH_RETRY;
        }
    } else {
        result = ALLJOYN_AUTH_FAIL;
    }
    return response;
}

qcc::String AuthMechPIN::Challenge(const qcc::String& response, AuthResult& result)
{
    qcc::String challenge;

    if (nonce.empty()) {
        AuthListener::Credentials creds;
        /*
         * Client sent random string. Server returns a random string.
         */
        nonce = HexStringToByteString(response);
        if (nonce.size() != NONCE_LEN) {
            result = ALLJOYN_AUTH_FAIL;
        } else if (listener.RequestCredentials(GetName(), authPeer.c_str(), authCount, "", AuthListener::CRED_PASSWORD, creds)) {
            if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
                expiration = creds.GetExpiration();
            }
            challenge = RandHexString(NONCE_LEN);
            ComputeMS(HexStringToByteString(challenge), creds.GetPassword());
            challenge += ":" + ComputeVerifier("server finish");
            result = ALLJOYN_AUTH_CONTINUE;
        } else {
            result = ALLJOYN_AUTH_FAIL;
        }
    } else {
        /*
         * Client responds with its verifier and we are done.
         */
        if (response == ComputeVerifier("client finish")) {
            result = ALLJOYN_AUTH_OK;
        } else {
            nonce.clear();
            result = ALLJOYN_AUTH_RETRY;
        }
    }
    return challenge;
}

}
