/**
 * @file
 *
 * This file defines the class for the ALLJOYN_SRP_LOGON authentication mechanism
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
#include "AuthMechLogon.h"

#define QCC_MODULE "ALLJOYN_AUTH"

using namespace std;
using namespace qcc;

namespace ajn {

/*
 * Per RFC 5426 (TLS) the random nonce should be 28 bytes which is what we are using also.
 */
#define NONCE_LEN  28

AuthMechLogon::AuthMechLogon(KeyStore& keyStore, ProtectedAuthListener& listener) : AuthMechanism(keyStore, listener), step(255)
{
}

QStatus AuthMechLogon::Init(AuthRole authRole, const qcc::String& authPeer)
{
    AuthMechanism::Init(authRole, authPeer);
    step = 0;
    /*
     * Default for AuthMechLogon is to immediately expire the master key
     */
    expiration = 0;
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
void AuthMechLogon::ComputeMS()
{
    uint8_t keymatter[48];
    KeyBlob pms;
    srp.GetPremasterSecret(pms);

    /*
     * Use the PRF function to compute the master secret.
     */
    Crypto_PseudorandomFunction(pms, label, clientRandom + serverRandom, keymatter, sizeof(keymatter));
    masterSecret.Set(keymatter, sizeof(keymatter), KeyBlob::GENERIC);
    masterSecret.SetExpiration(expiration);
}

/*
 * Verifier is computed following approach in RFC 5246. from the master secret and
 * a hash of the entire authentication conversation.
 */
qcc::String AuthMechLogon::ComputeVerifier(const char* label)
{
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
    uint8_t verifier[12];
    /*
     * Snapshot msg hash and compute the verifier string.
     */
    msgHash.GetDigest(digest, true);
    qcc::String seed((const char*)digest, sizeof(digest));
    Crypto_PseudorandomFunction(masterSecret, label, seed, verifier, sizeof(verifier));
    QCC_DbgHLPrintf(("Verifier:  %s", BytesToHexString(verifier, sizeof(verifier)).c_str()));
    return BytesToHexString(verifier, sizeof(verifier));
}

qcc::String AuthMechLogon::InitialResponse(AuthResult& result)
{
    qcc::String response;
    result = ALLJOYN_AUTH_FAIL;
    /*
     * Initial response provides the id of the user to authenticate.
     */
    if (listener.RequestCredentials(GetName(), authPeer.c_str(), authCount, "", AuthListener::CRED_PASSWORD | AuthListener::CRED_USER_NAME, creds)) {
        if (creds.IsSet(AuthListener::CRED_EXPIRATION)) {
            expiration = creds.GetExpiration();
        }
        if (creds.IsSet(AuthListener::CRED_USER_NAME) && !creds.GetUserName().empty()) {
            /*
             * Client starts the conversation by sending a random string and user id.
             */
            response = RandHexString(NONCE_LEN);
            clientRandom = HexStringToByteString(response);
            response += ":" + creds.GetUserName();
            result = ALLJOYN_AUTH_CONTINUE;
            msgHash.Update((const uint8_t*)response.data(), response.size());
            QCC_DbgHLPrintf(("InitialResponse() %s", response.c_str()));
        } else {
            result = ALLJOYN_AUTH_FAIL;
            QCC_LogError(ER_AUTH_FAIL, ("InitialResponse() user id is required"));
        }
    }
    return response;
}

qcc::String AuthMechLogon::Response(const qcc::String& challenge,
                                    AuthResult& result)
{
    QCC_DbgHLPrintf(("Response %d", step + 1));
    QStatus status = ER_OK;
    qcc::String response;
    size_t pos;

    result = ALLJOYN_AUTH_CONTINUE;

    switch (++step) {
    case 1:
        msgHash.Update((const uint8_t*)challenge.data(), challenge.size());
        /*
         * Server sends an intialization string, client responds with its initialization string.
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
            result = ALLJOYN_AUTH_ERROR;
            break;
        }
        if (!creds.IsSet(AuthListener::CRED_PASSWORD)) {
            if (!listener.RequestCredentials(GetName(), authPeer.c_str(), authCount, creds.GetUserName().c_str(), AuthListener::CRED_PASSWORD, creds)) {
                result = ALLJOYN_AUTH_FAIL;
                break;
            }
        }
        status = srp.ClientFinish(creds.GetUserName(), creds.GetPassword());
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
        break;

    default:
        result = ALLJOYN_AUTH_ERROR;
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("AuthMechLogon::Response"));
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


/*
 * Generate a hex encoded GUID from a user id.
 */
static void UserNameToGuid(qcc::GUID128& guid, qcc::String userName)
{
    static const char label[] = "SRP Logon Verifier";
    Crypto_SHA1 sha1;
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
    assert(Crypto_SHA1::DIGEST_SIZE >= qcc::GUID128::SIZE);
    /*
     * The label makes the generated Guid unique for this authentication mechanism.
     */
    sha1.Init();
    sha1.Update((const uint8_t*)label, sizeof(label));
    sha1.Update(userName);
    sha1.GetDigest(digest);
    guid.SetBytes(digest);
}

QStatus AuthMechLogon::AddLogonEntry(KeyStore& keyStore, const char* userName, const char* password)
{
    QStatus status = ER_OK;
    Crypto_SRP srp;
    qcc::String tmp;
    qcc::GUID128 userGuid(0);

    UserNameToGuid(userGuid, userName);

    if (password) {
        status = srp.ServerInit(userName, password, tmp);
        if (status == ER_OK) {
            qcc::String logonEntry = srp.ServerGetVerifier();
            if (logonEntry.empty()) {
                status = ER_CRYPTO_ERROR;
            } else {
                KeyBlob userBlob((const uint8_t*)logonEntry.data(), logonEntry.size(), KeyBlob::GENERIC);
                status = keyStore.AddKey(userGuid, userBlob);
            }
        }
    } else {
        status = keyStore.DelKey(userGuid);
    }
    QCC_DbgHLPrintf(("AddLogonEntry for user %s %s", userName, QCC_StatusText(status)));
    return status;
}

qcc::String AuthMechLogon::Challenge(const qcc::String& response,
                                     AuthResult& result)
{
    QCC_DbgHLPrintf(("Challenge %d", step + 1));
    QStatus status = ER_OK;
    qcc::String challenge;
    qcc::String userName;
    qcc::GUID128 userGuid(0);
    KeyBlob userBlob;
    size_t pos;

    result = ALLJOYN_AUTH_CONTINUE;

    switch (++step) {
    case 1:
        msgHash.Update((const uint8_t*)response.data(), response.size());
        /*
         * Client sends a random string and user name. Server returns an SRP string.
         */
        pos = response.find_first_of(":");
        if (pos == qcc::String::npos) {
            result = ALLJOYN_AUTH_ERROR;
            challenge = "User id required";
            break;
        }
        clientRandom = HexStringToByteString(response.substr(0, pos));
        userName = response.substr(pos + 1);
        UserNameToGuid(userGuid, userName);
        QCC_DbgPrintf(("Logon attempt for user \"%s\"", userName.c_str()));
        /*
         * Check if there is already an SRP user logon entry for this user name.
         */
        if (keyStore.GetKey(userGuid, userBlob) == ER_OK) {
            QCC_DbgHLPrintf(("Using precomputed SRP logon entry string for %s", userName.c_str()));
            qcc::String logonEntry((const char*)userBlob.GetData(), userBlob.GetSize());
            status = srp.ServerInit(logonEntry, challenge);
            break;
        }
        /*
         * Application may return a password or a precomputed SRP logon entry string.
         */
        if (listener.RequestCredentials(GetName(), authPeer.c_str(), authCount, userName.c_str(), AuthListener::CRED_PASSWORD | AuthListener::CRED_LOGON_ENTRY, creds)) {
            if (creds.IsSet(AuthListener::CRED_PASSWORD)) {
                status = srp.ServerInit(userName, creds.GetPassword(), challenge);
            } else if (creds.IsSet(AuthListener::CRED_LOGON_ENTRY)) {
                status = srp.ServerInit(creds.GetLogonEntry(), challenge);
            } else {
                challenge = "No logon credentials for user " + userName;
                result = ALLJOYN_AUTH_RETRY;
            }
            /*
             * Store the precomputed logonEntry blob in the keystore.
             */
            if ((result == ALLJOYN_AUTH_CONTINUE) && (status == ER_OK)) {
                qcc::String logonEntry = srp.ServerGetVerifier();
                userBlob.Set((const uint8_t*)logonEntry.data(), logonEntry.size(), KeyBlob::GENERIC);
                keyStore.AddKey(userGuid, userBlob);
            }
        } else {
            challenge = "Logon denied for user " + userName;
            status = ER_AUTH_FAIL;
        }
        break;

    case 2:
        msgHash.Update((const uint8_t*)response.data(), response.size());
        /*
         * Client sends its SRP string, server responds with a random string, and its verifier.
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
        QCC_LogError(status, ("AuthMechLogon::Challenge"));
        result = ALLJOYN_AUTH_FAIL;
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
