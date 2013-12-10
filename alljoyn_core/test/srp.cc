/**
 * @file
 *
 * This file tests SRP against the RFC 5054 test vector
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

#include <qcc/Crypto.h>
#include <qcc/Debug.h>
#include <qcc/KeyBlob.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>
#include <qcc/Debug.h>
#include <qcc/BigNum.h>

#include <alljoyn/version.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/AuthListener.h>

#include <alljoyn/Status.h>

/* Private files included for unit testing */
#include <SASLEngine.h>

#define QCC_MODULE "CRYPTO"

using namespace qcc;
using namespace std;
using namespace ajn;

class MyAuthListener : public AuthListener {
    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {
        creds.SetPassword("123456");
        return true;
    }
    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        printf("Authentication %s %s\n", authMechanism, success ? "succesful" : "failed");
    }
};

int main(int argc, char** argv)
{
    String toClient;
    String toServer;
    String verifier;
    QStatus status = ER_OK;
    KeyBlob serverPMS;
    KeyBlob clientPMS;
    String user = "someuser";
    String pwd = "a-secret-password";

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Test vector as defined in RFC 5246 built in to Crypto_SRP class */
    {
        Crypto_SRP srp;
        status = srp.TestVector();
        if (status != ER_OK) {
            printf("Test vector failed\n");
            exit(1);
        }
    }
    printf("############Test vector passed ################\n");
    /*
     * Basic API test.
     */
    for (int i = 0; i < 1; ++i) {
        {
            Crypto_SRP client;
            Crypto_SRP server;

            status = server.ServerInit(user, pwd, toClient);
            if (status != ER_OK) {
                QCC_LogError(status, ("SRP ServerInit failed"));
                goto TestFail;
            }
            status = client.ClientInit(toClient, toServer);
            if (status != ER_OK) {
                QCC_LogError(status, ("SRP ClientInit failed"));
                goto TestFail;
            }
            status = server.ServerFinish(toServer);
            if (status != ER_OK) {
                QCC_LogError(status, ("SRP ServerFinish failed"));
                goto TestFail;
            }
            status = client.ClientFinish(user, pwd);
            if (status != ER_OK) {
                QCC_LogError(status, ("SRP ClientFinish failed"));
                goto TestFail;
            }
            /*
             * Check premaster secrets match
             */
            server.GetPremasterSecret(serverPMS);
            client.GetPremasterSecret(clientPMS);
            if (clientPMS.GetSize() != serverPMS.GetSize()) {
                printf("Premaster secrets have different sizes\n");
                goto TestFail;
            }
            if (memcmp(serverPMS.GetData(), clientPMS.GetData(), serverPMS.GetSize()) != 0) {
                printf("Premaster secrets don't match\n");
                printf("client = %s\n", BytesToHexString(serverPMS.GetData(), serverPMS.GetSize()).c_str());
                printf("server = %s\n", BytesToHexString(clientPMS.GetData(), clientPMS.GetSize()).c_str());
                goto TestFail;
            }
            printf("Premaster secret = %s\n", BytesToHexString(serverPMS.GetData(), serverPMS.GetSize()).c_str());
            verifier = server.ServerGetVerifier();
        }
        printf("###### Checking verifier ########\n");
        /*
         * Test using the verifier from the previous test.
         */
        {
            Crypto_SRP client;
            Crypto_SRP server;

            status = server.ServerInit(verifier, toClient);
            if (status != ER_OK) {
                QCC_LogError(status, ("SRP ServerInit failed"));
                goto TestFail;
            }
            status = client.ClientInit(toClient, toServer);
            if (status != ER_OK) {
                QCC_LogError(status, ("SRP ClientInit failed"));
                goto TestFail;
            }
            status = server.ServerFinish(toServer);
            if (status != ER_OK) {
                QCC_LogError(status, ("SRP ServerFinish failed"));
                goto TestFail;
            }
            status = client.ClientFinish(user, pwd);
            if (status != ER_OK) {
                QCC_LogError(status, ("SRP ClientFinish failed"));
                goto TestFail;
            }
            /*
             * Check premaster secrets match
             */
            server.GetPremasterSecret(serverPMS);
            client.GetPremasterSecret(clientPMS);
            if (clientPMS.GetSize() != serverPMS.GetSize()) {
                printf("Premaster secrets have different sizes\n");
                goto TestFail;
            }
            if (memcmp(serverPMS.GetData(), clientPMS.GetData(), serverPMS.GetSize()) != 0) {
                printf("Premaster secrets don't match\n");
                printf("client = %s\n", BytesToHexString(serverPMS.GetData(), serverPMS.GetSize()).c_str());
                printf("server = %s\n", BytesToHexString(clientPMS.GetData(), clientPMS.GetSize()).c_str());
                goto TestFail;
            }
            printf("Premaster secret = %s\n", BytesToHexString(serverPMS.GetData(), serverPMS.GetSize()).c_str());

            qcc::String serverRand = RandHexString(64);
            qcc::String clientRand = RandHexString(64);
            uint8_t masterSecret[48];

            printf("testing pseudo random function\n");

            status = Crypto_PseudorandomFunction(serverPMS, "foobar", serverRand + clientRand, masterSecret, sizeof(masterSecret));
            if (status != ER_OK) {
                QCC_LogError(status, ("Crypto_PseudoRandomFunction failed"));
                goto TestFail;
            }
            printf("Master secret = %s\n", BytesToHexString(masterSecret, sizeof(masterSecret)).c_str());

        }
        printf("#################################\n");
    }

    /*
     * Now test the SRP authentication mechanism
     */
    {

        BusAttachment bus("srp");
        MyAuthListener myListener;
        bus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", &myListener);

        ProtectedAuthListener listener;
        listener.Set(&myListener);

        SASLEngine responder(bus, ajn::AuthMechanism::RESPONDER, "ALLJOYN_SRP_KEYX", "1:1", listener);
        SASLEngine challenger(bus, ajn::AuthMechanism::CHALLENGER, "ALLJOYN_SRP_KEYX", "1:1", listener);

        SASLEngine::AuthState rState = SASLEngine::ALLJOYN_AUTH_FAILED;
        SASLEngine::AuthState cState = SASLEngine::ALLJOYN_AUTH_FAILED;

        qcc::String rStr;
        qcc::String cStr;

        while (status == ER_OK) {
            status = responder.Advance(cStr, rStr, rState);
            if (status != ER_OK) {
                printf("Responder returned %s\n", QCC_StatusText(status));
                goto TestFail;
            }
            status = challenger.Advance(rStr, cStr, cState);
            if (status != ER_OK) {
                printf("Challenger returned %s\n", QCC_StatusText(status));
                goto TestFail;
            }
            if ((rState == SASLEngine::ALLJOYN_AUTH_SUCCESS) && (cState == SASLEngine::ALLJOYN_AUTH_SUCCESS)) {
                break;
            }
        }
    }

    printf("Passed\n");
    return 0;

TestFail:

    printf("Failed\n");
    return -1;
}
