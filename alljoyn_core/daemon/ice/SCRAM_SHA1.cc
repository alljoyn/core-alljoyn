/**
 * @file
 * This file defines the SCRAM-SHA-1 related functions.
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

#include <qcc/String.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/StringUtil.h>
#include <qcc/GUID.h>
#include "SCRAM_SHA1.h"
#include "RendezvousServerInterface.h"

using namespace std;

#define QCC_MODULE "SCRAM_SHA1"

namespace ajn {

SCRAM_SHA_1::SCRAM_SHA_1() :
    ClientNonce(),
    ChannelBinding(),
    ClientProof(),
    UserName(),
    Password(),
    ClientFirstMessageString(),
    ClientFinalMessageString(),
    ServerFirstResponseString(),
    ServerFinalResponseString(),
    AuthMessage()
{
    ClientFirstMessage.clear();
    ServerFirstResponse.clear();
    ClientFinalMessage.clear();
    ServerFinalResponse.clear();

    for (size_t i = 0; i < Crypto_SHA1::DIGEST_SIZE; ++i) {
        SaltedPassword[i] = 0;
        ClientKey[i] = 0;
        StoredKey[i] = 0;
        ClientSignature[i] = 0;
    }
}

SCRAM_SHA_1::~SCRAM_SHA_1()
{

}

void SCRAM_SHA_1::SetUserCredentials(String userName, String password)
{
    UserName = userName;
    Password = password;
    QCC_DbgPrintf(("SCRAM_SHA_1::SetUserCredentials(): UserName = %s Password = %s", UserName.c_str(), Password.c_str()));
}

void SCRAM_SHA_1::Reset(void)
{
    ClientNonce.erase();
    ChannelBinding.erase();
    ClientProof.erase();
    UserName.erase();
    Password.erase();
    ClientFirstMessage.clear();
    ClientFirstMessageString.erase();
    ServerFirstResponse.clear();
    ServerFirstResponseString.erase();
    ClientFinalMessage.clear();
    ClientFinalMessageString.erase();
    ServerFinalResponse.clear();
    ServerFinalResponseString.erase();
    AuthMessage.erase();

    for (size_t i = 0; i < Crypto_SHA1::DIGEST_SIZE; ++i) {
        SaltedPassword[i] = 0;
        ClientKey[i] = 0;
        StoredKey[i] = 0;
        ClientSignature[i] = 0;
    }
}

String SCRAM_SHA_1::GenerateClientLoginFirstSASLMessage(void)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateClientLoginFirstSASLMessage()"));

    ClientFirstMessage.clear();

    GenerateUserName();
    ClientFirstMessage.Set_n(UserName);

    GenerateNonce();
    ClientFirstMessage.Set_r(ClientNonce);

    ClientFirstMessageString = GenerateSASLMessage(ClientFirstMessage, true);

    return ClientFirstMessageString;
}

String SCRAM_SHA_1::GenerateClientLoginFinalSASLMessage(void)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateClientLoginFinalSASLMessage()"));

    ClientFinalMessage.clear();

    GenerateChannelBinding();
    ClientFinalMessage.Set_c(ChannelBinding);

    ClientFinalMessage.Set_r(ServerFirstResponse.r);

    GenerateSaltedPassword();
    GenerateClientKey();
    GenerateStoredKey();
    GenerateAuthMessage();
    GenerateClientSignature();
    GenerateClientProof();

    ClientFinalMessage.Set_p(ClientProof);

    ClientFinalMessageString = GenerateSASLMessage(ClientFinalMessage, false);

    return ClientFinalMessageString;
}

/**
 * Validate the client login first response.
 */
QStatus SCRAM_SHA_1::ValidateClientLoginFirstResponse(String response)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::ValidateClientLoginFirstResponse(): response = %s", response.c_str()));

    QStatus status = ER_OK;

    ServerFirstResponse.clear();

    ServerFirstResponse = ParseSASLMessage(response);

    if (!ServerFirstResponse.ePresent) {
        /* Verify that the r, s and i attributes are present in the response from the server */
        if ((!ServerFirstResponse.rPresent) || (!ServerFirstResponse.sPresent) || (!ServerFirstResponse.iPresent)) {
            status = ER_FAIL;
            QCC_LogError(status, ("SCRAM_SHA_1::ValidateClientLoginFirstResponse(): rPresent(%d) sPresent(%d) iPresent(%d)",
                                  ServerFirstResponse.rPresent, ServerFirstResponse.sPresent, ServerFirstResponse.iPresent));
            ServerFirstResponse.clear();
        } else {
            ServerFirstResponseString = response;
        }
    } else {
        QCC_DbgPrintf(("SCRAM_SHA_1::ValidateClientLoginFirstResponse(): Error received from the Server"));
    }

    return status;
}

/**
 * Validate the client login final response.
 */
QStatus SCRAM_SHA_1::ValidateClientLoginFinalResponse(ClientLoginFinalResponse response)
{
    //QCC_DbgPrintf(("SCRAM_SHA_1::ValidateClientLoginFinalResponse()"));

    QStatus status = ER_OK;

    ServerFinalResponse.clear();

    ServerFinalResponse = ParseSASLMessage(response.message);

    /* Verify that either v or e attribute is present in the response from the server */
    if ((!ServerFinalResponse.vPresent) && (!ServerFinalResponse.ePresent)) {
        status = ER_FAIL;
        ServerFinalResponse.clear();
    }

    if (ServerFinalResponse.vPresent) {
        /* Validate the Server */
        status = ValidateServer(ServerFinalResponse.v);

        if (status == ER_OK) {
            if ((!response.peerIDPresent) || (!response.peerAddrPresent) || (!response.daemonRegistrationRequiredPresent) ||
                (!response.sessionActivePresent) || (!response.configDataPresent)) {
                status = ER_FAIL;
                QCC_LogError(status, ("SCRAM_SHA_1::ValidateClientLoginFinalResponse(): peerIDPresent(%d) peerAddrPresent(%d) daemonRegistrationRequiredPresent(%d) sessionActivePresent(%d) configDataPresent(%d)",
                                      response.peerIDPresent, response.peerAddrPresent, response.daemonRegistrationRequiredPresent, response.sessionActivePresent, response.configDataPresent));
            } else {
                if (!response.configData.TkeepalivePresent) {
                    status = ER_FAIL;
                    QCC_LogError(status, ("SCRAM_SHA_1::ValidateClientLoginFinalResponse(): TkeepalivePresent(%d)", response.configData.TkeepalivePresent));
                } else {
                    ServerFinalResponseString = response.message;
                }
            }
        } else {
            QCC_LogError(status, ("SCRAM_SHA_1::ValidateClientLoginFinalResponse(): ValidateServer failed"));
        }
    }

    return status;
}

/**
 * Generate the SASL nonce.
 */
void SCRAM_SHA_1::GenerateNonce(void)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateNonce()"));

    GUID128 clientNonce = GUID128();

    ClientNonce = clientNonce.ToString();

    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateNonce(): ClientNonce = %s", ClientNonce.c_str()));
}

/**
 * Generate the SASL channel binding.
 */
void SCRAM_SHA_1::GenerateChannelBinding(void)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateChannelBinding()"));

    ChannelBinding = String("biws");
}


/**
 * Generate the SASL Client Proof.
 */
void SCRAM_SHA_1::GenerateClientProof(void)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateClientProof()"));

    uint8_t rawClientProof[Crypto_SHA1::DIGEST_SIZE];

    XorByteArray(ClientKey, ClientSignature, rawClientProof, Crypto_SHA1::DIGEST_SIZE);

    String rawClientProofString = String(const_cast<char*>(reinterpret_cast<const char*>(rawClientProof)), Crypto_SHA1::DIGEST_SIZE);

    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateClientProof(): rawClientProofString = %s", rawClientProofString.c_str()));

    Crypto_ASN1::EncodeBase64(rawClientProofString, ClientProof);

    /* Remove the trailing \n at the end of the encoded string */
    ClientProof.resize((ClientProof.size() - 1));

    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateClientProof(): ClientProof = %s", ClientProof.c_str()));
}

/**
 * Validate the server.
 */
QStatus SCRAM_SHA_1::ValidateServer(String serverSignature)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::ValidateServer()"));

    uint8_t ServerKey[Crypto_SHA1::DIGEST_SIZE];
    uint8_t ServerSignature[Crypto_SHA1::DIGEST_SIZE];

    String serverKey = String("Server Key");

    Crypto_SHA1 keyhash;

    keyhash.Init(SaltedPassword, sizeof(SaltedPassword));
    keyhash.Update((uint8_t*)serverKey.data(), serverKey.size());
    keyhash.GetDigest(ServerKey);

    Crypto_SHA1 signaturehash;

    signaturehash.Init(ServerKey, sizeof(ServerKey));
    signaturehash.Update((uint8_t*)AuthMessage.data(), AuthMessage.size());
    signaturehash.GetDigest(ServerSignature);

    String encodedServerSignatureString;

    String rawServerSignatureString = String(const_cast<char*>(reinterpret_cast<const char*>(ServerSignature)), Crypto_SHA1::DIGEST_SIZE);

    Crypto_ASN1::EncodeBase64(rawServerSignatureString, encodedServerSignatureString);

    /* Remove the trailing \n at the end of the encoded string */
    encodedServerSignatureString.resize((encodedServerSignatureString.size() - 1));

    if (encodedServerSignatureString == serverSignature) {
        return ER_OK;
    } else {
        return ER_FAIL;
    }
}

void SCRAM_SHA_1::XorByteArray(uint8_t* in1, uint8_t* in2, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        in1[i] ^= in2[i];
    }
}

/**
 * Perform XOR of two byte arrays
 */
void SCRAM_SHA_1::XorByteArray(const uint8_t* in1, const uint8_t* in2, uint8_t* out, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        out[i] = in1[i] ^ in2[i];
    }
}

/**
 * Generate the salted password
 */
void SCRAM_SHA_1::GenerateSaltedPassword(void)
{
    //QCC_DbgPrintf(("SCRAM_SHA_1::GenerateSaltedPassword()"));

    /* Generate a UTF-8 encoded password string */
    GeneratePassword();

    //QCC_DbgPrintf(("SCRAM_SHA_1::GenerateSaltedPassword(): Password = %s", Password.c_str()));

    String salt;

    Crypto_ASN1::DecodeBase64(ServerFirstResponse.s, salt);

    if (salt.size() != SALT_SIZE) {
        QCC_LogError(ER_FAIL, ("%s: Size of the salt(%d) is != %d", __FUNCTION__, salt.size(), SALT_SIZE));
        return;
    }

    uint8_t saltByteArray[SALT_BYTE_ARRAY_SIZE], j;

    /* Extract the contents of the decoded salt string into a byte array */
    for (j = 0; j < SALT_SIZE; j++) {
        saltByteArray[j] = salt[j];
    }
    /* Append 4-octet equivalent of 1 i.e. INT(1) to the salt byte array as per the spec */
    saltByteArray[j] = 0;
    saltByteArray[j + 1] = 0;
    saltByteArray[j + 2] = 0;
    saltByteArray[j + 3] = 1;

    uint32_t i = ServerFirstResponse.i;

    Crypto_SHA1 hash;
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];

    String rawClientProofString;
    String tempPrint;

    /*
     * Initialize SHA1 in HMAC mode with the secret
     */
    for (uint32_t iteration = 0; iteration < i; iteration++) {
        hash.Init((uint8_t*)Password.data(), Password.size());
        if (iteration == 0) {
            hash.Update(saltByteArray, sizeof(saltByteArray));
            hash.GetDigest(digest);

            rawClientProofString.clear();
            tempPrint.clear();
            rawClientProofString = String(const_cast<char*>(reinterpret_cast<const char*>(digest)), Crypto_SHA1::DIGEST_SIZE);
            Crypto_ASN1::EncodeBase64(rawClientProofString, tempPrint);
            /* Remove the trailing \n at the end of the encoded string */
            tempPrint.resize((tempPrint.size() - 1));

            //QCC_DbgPrintf(("SCRAM_SHA_1::GenerateSaltedPassword(): iteration[%d] digest = %s", iteration, tempPrint.c_str()));
            memcpy(reinterpret_cast<void*>(SaltedPassword), const_cast<void*>(reinterpret_cast<const void*>(digest)), Crypto_SHA1::DIGEST_SIZE);

            rawClientProofString.clear();
            tempPrint.clear();
            rawClientProofString = String(const_cast<char*>(reinterpret_cast<const char*>(SaltedPassword)), Crypto_SHA1::DIGEST_SIZE);
            Crypto_ASN1::EncodeBase64(rawClientProofString, tempPrint);
            /* Remove the trailing \n at the end of the encoded string */
            tempPrint.resize((tempPrint.size() - 1));

            //QCC_DbgPrintf(("SCRAM_SHA_1::GenerateSaltedPassword(): iteration[%d] SaltedPassword = %s", iteration, tempPrint.c_str()));
        } else {
            hash.Update(digest, sizeof(digest));
            hash.GetDigest(digest);

            rawClientProofString.clear();
            tempPrint.clear();
            rawClientProofString = String(const_cast<char*>(reinterpret_cast<const char*>(digest)), Crypto_SHA1::DIGEST_SIZE);
            Crypto_ASN1::EncodeBase64(rawClientProofString, tempPrint);
            /* Remove the trailing \n at the end of the encoded string */
            tempPrint.resize((tempPrint.size() - 1));

            //QCC_DbgPrintf(("SCRAM_SHA_1::GenerateSaltedPassword(): iteration[%d] digest = %s", iteration, tempPrint.c_str()));

            XorByteArray(SaltedPassword, digest, sizeof(digest));

            rawClientProofString.clear();
            tempPrint.clear();
            rawClientProofString = String(const_cast<char*>(reinterpret_cast<const char*>(SaltedPassword)), Crypto_SHA1::DIGEST_SIZE);
            Crypto_ASN1::EncodeBase64(rawClientProofString, tempPrint);
            /* Remove the trailing \n at the end of the encoded string */
            tempPrint.resize((tempPrint.size() - 1));

            //QCC_DbgPrintf(("SCRAM_SHA_1::GenerateSaltedPassword(): iteration[%d] SaltedPassword = %s", iteration, tempPrint.c_str()));
        }
    }
}

/**
 * Generate the Client Key
 */
void SCRAM_SHA_1::GenerateClientKey(void)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateClientKey()"));

    Crypto_SHA1 hash;

    String clientKey = String("Client Key");

    /*
     * Initialize SHA1 in HMAC mode with the SaltedPassword
     */
    hash.Init(SaltedPassword, sizeof(SaltedPassword));
    hash.Update((uint8_t*)clientKey.data(), clientKey.size());
    hash.GetDigest(ClientKey);

    String rawClientProofString;
    String tempPrint;

    rawClientProofString = String(const_cast<char*>(reinterpret_cast<const char*>(ClientKey)), Crypto_SHA1::DIGEST_SIZE);
    Crypto_ASN1::EncodeBase64(rawClientProofString, tempPrint);

    /* Remove the trailing \n at the end of the encoded string */
    tempPrint.resize((tempPrint.size() - 1));

    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateClientKey(): ClientKey = %s", tempPrint.c_str()));
}

/**
 * Generate the Stored Key
 */
void SCRAM_SHA_1::GenerateStoredKey(void)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateStoredKey()"));

    Crypto_SHA1 hash;

    /*
     * Initialize SHA1 in HMAC mode
     */
    hash.Init();
    hash.Update(ClientKey, sizeof(ClientKey));
    hash.GetDigest(StoredKey);

    String rawClientProofString;
    String tempPrint;

    rawClientProofString = String(const_cast<char*>(reinterpret_cast<const char*>(StoredKey)), Crypto_SHA1::DIGEST_SIZE);
    Crypto_ASN1::EncodeBase64(rawClientProofString, tempPrint);
    /* Remove the trailing \n at the end of the encoded string */
    tempPrint.resize((tempPrint.size() - 1));

    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateStoredKey(): StoredKey = %s", tempPrint.c_str()));
}

/**
 * Generate the UTF-8 encoded User Name
 */
void SCRAM_SHA_1::GenerateUserName(void)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateUserName()"));

    /* PPN - At some point may need to do a SASLPrep on the UserName */
}

/**
 * Generate the UTF-8 encoded Password
 */
void SCRAM_SHA_1::GeneratePassword(void)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::GeneratePassword()"));

    /* PPN - At some point may need to do a SASLPrep on the Password */
}

/**
 * Generate the Authorization Message
 */
void SCRAM_SHA_1::GenerateAuthMessage(void)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateAuthMessage()"));

    SASLMessage clientFinalMsg;
    clientFinalMsg.clear();

    GenerateChannelBinding();
    clientFinalMsg.Set_c(ChannelBinding);

    clientFinalMsg.Set_r(ServerFirstResponse.r);

    String clientFirstMessagebare = ClientFirstMessageString;

    /* Erase "n,," from ClientFirstMessageString to get the bare message */
    clientFirstMessagebare.erase(0, 3);

    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateAuthMessage(): clientFirstMessagebare = %s", clientFirstMessagebare.c_str()));

    AuthMessage = clientFirstMessagebare + String(",") + ServerFirstResponseString + String(",") + GenerateSASLMessage(clientFinalMsg, false);

    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateAuthMessage(): AuthMessage = %s", AuthMessage.c_str()));
}

/**
 * Generate the Client Signature
 */
void SCRAM_SHA_1::GenerateClientSignature(void)
{
    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateClientSignature()"));

    Crypto_SHA1 hash;

    /*
     * Initialize SHA1 in HMAC mode with the StoredKey
     */
    hash.Init(StoredKey, sizeof(StoredKey));
    hash.Update((uint8_t*)AuthMessage.data(), AuthMessage.size());
    hash.GetDigest(ClientSignature);

    String rawClientProofString;
    String tempPrint;

    rawClientProofString = String(const_cast<char*>(reinterpret_cast<const char*>(ClientSignature)), Crypto_SHA1::DIGEST_SIZE);
    Crypto_ASN1::EncodeBase64(rawClientProofString, tempPrint);
    /* Remove the trailing \n at the end of the encoded string */
    tempPrint.resize((tempPrint.size() - 1));

    QCC_DbgPrintf(("SCRAM_SHA_1::GenerateClientSignature(): ClientSignature = %s", tempPrint.c_str()));
}

}
