/**
 * @file
 *
 * This file has units tests for the RSA cryto APIs
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

const char hw[] = "hello world";

static const char x509cert[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBszCCARwCCQDuCh+BWVBk2DANBgkqhkiG9w0BAQUFADAeMQ0wCwYDVQQKDARN\n"
    "QnVzMQ0wCwYDVQQDDARHcmVnMB4XDTEwMDUxNzE1MTg1N1oXDTExMDUxNzE1MTg1\n"
    "N1owHjENMAsGA1UECgwETUJ1czENMAsGA1UEAwwER3JlZzCBnzANBgkqhkiG9w0B\n"
    "AQEFAAOBjQAwgYkCgYEArSd4r62mdaIRG9xZPDAXfImt8e7GTIyXeM8z49Ie1mrQ\n"
    "h7roHbn931Znzn20QQwFD6pPC7WxStXJVH0iAoYgzzPsXV8kZdbkLGUMPl2GoZY3\n"
    "xDSD+DA3m6krcXcN7dpHv9OlN0D9Trc288GYuFEENpikZvQhMKPDUAEkucQ95Z8C\n"
    "AwEAATANBgkqhkiG9w0BAQUFAAOBgQBkYY6zzf92LRfMtjkKs2am9qvjbqXyDJLS\n"
    "viKmYe1tGmNBUzucDC5w6qpPCTSe23H2qup27///fhUUuJ/ssUnJ+Y77jM/u1O9q\n"
    "PIn+u89hRmqY5GKHnUSZZkbLB/yrcFEchHli3vLo4FOhVVHwpnwLtWSpfBF9fWcA\n"
    "7THIAV79Lg==\n"
    "-----END CERTIFICATE-----"
};

static const char PEM_DES[] = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: DES-EDE3-CBC,86B9DBED35AEBAB3\n"
    "\n"
    "f28sibgVCkDz3VNoC/MzazG2tFj+KGf6xm9LQki/GsxpMhJsEEvT9dUluT1T4Ypr\n"
    "NjG+nBleLcfdHxOl5XHnusn8r/JVaQQGVSnDaeP/27KiirtB472p+8Wc2wfXexRz\n"
    "uSUv0DJT+Fb52zYGiGzwgaOinQEBskeO9AwRyG34sFKqyyapyJtSZDjh+wUAIMZb\n"
    "wKifvl1KHSCbXEhjDVlxBw4Rt7I36uKzTY5oax2L6W6gzxfHuOtzfVelAaM46j+n\n"
    "KANZgx6KGW2DKk27aad2HEZUYeDwznpwU5Duw9b0DeMTkez6CuayiZHb5qEod+0m\n"
    "pCCMwpqxFCJ/vg1VJjmxM7wpCQTc5z5cjX8saV5jMUJXp09NuoU/v8TvhOcXOE1T\n"
    "ENukIWYBT1HC9MJArroLwl+fMezKCu+F/JC3M0RfI0dlQqS4UWH+Uv+Ujqa2yr9y\n"
    "20zYS52Z4kyq2WnqwBk1//PLBl/bH/awWXPUI2yMnIILbuCisRYLyK52Ge/rS51P\n"
    "vUgUCZ7uoEJGTX6EGh0yQhp+5jGYVdHHZB840AyxzBQx7pW4MtTwqkw1NZuQcdSN\n"
    "IU9y/PferHhMKZeGfVRVEkAOcjeXOqvSi6NKDvYn7osCkvj9h7K388o37VMPSacR\n"
    "jDwDTT0HH/UcM+5v/74NgE/OebaK3YfxBVyMmBzi0WVFXgxHJir4xpj9c20YQVw9\n"
    "hE3kYepW8gGz/JPQmRszwLQpwQNEP60CgQveqtH7tZVXzDkElvSyveOdjJf1lw4B\n"
    "uCz54678UNNeIe7YB4yV1dMVhhcoitn7G/+jC9Qk3FTnuP+Ws5c/0g==\n"
    "-----END RSA PRIVATE KEY-----"
};


static const char PEM_AES[] = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: AES-128-CBC,0AE4BAB94CEAA7829273DD861B067DBA\n"
    "\n"
    "LSJOp+hEzNDDpIrh2UJ+3CauxWRKvmAoGB3r2hZfGJDrCeawJFqH0iSYEX0n0QEX\n"
    "jfQlV4LHSCoGMiw6uItTof5kHKlbp5aXv4XgQb74nw+2LkftLaTchNs0bW0TiGfQ\n"
    "XIuDNsmnZ5+CiAVYIKzsPeXPT4ZZSAwHsjM7LFmosStnyg4Ep8vko+Qh9TpCdFX8\n"
    "w3tH7qRhfHtpo9yOmp4hV9Mlvx8bf99lXSsFJeD99C5GQV2lAMvpfmM8Vqiq9CQN\n"
    "9OY6VNevKbAgLG4Z43l0SnbXhS+mSzOYLxl8G728C6HYpnn+qICLe9xOIfn2zLjm\n"
    "YaPlQR4MSjHEouObXj1F4MQUS5irZCKgp4oM3G5Ovzt82pqzIW0ZHKvi1sqz/KjB\n"
    "wYAjnEGaJnD9B8lRsgM2iLXkqDmndYuQkQB8fhr+zzcFmqKZ1gLRnGQVXNcSPgjU\n"
    "Y0fmpokQPHH/52u+IgdiKiNYuSYkCfHX1Y3nftHGvWR3OWmw0k7c6+DfDU2fDthv\n"
    "3MUSm4f2quuiWpf+XJuMB11px1TDkTfY85m1aEb5j4clPGELeV+196OECcMm4qOw\n"
    "AYxO0J/1siXcA5o6yAqPwPFYcs/14O16FeXu+yG0RPeeZizrdlv49j6yQR3JLa2E\n"
    "pWiGR6hmnkixzOj43IPJOYXySuFSi7lTMYud4ZH2+KYeK23C2sfQSsKcLZAFATbq\n"
    "DY0TZHA5lbUiOSUF5kgd12maHAMidq9nIrUpJDzafgK9JrnvZr+dVYM6CiPhiuqJ\n"
    "bXvt08wtKt68Ymfcx+l64mwzNLS+OFznEeIjLoaHU4c=\n"
    "-----END RSA PRIVATE KEY-----"
};

static const char PEM_PKCS8_V1_5[] = {
    "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"
    "MIICoTAbBgkqhkiG9w0BBQMwDgQIOUsiiy9gId4CAggABIICgM/YtiPQuve9FDVz\n"
    "6kRTKl+6aeIOlURDVkNohPrAjZZL+1n2lckVYgFaUjEEOxutZFYW8F4+UnFy2o/l\n"
    "wK8IZm8EKnXIKHTh8f/5n4V1N3rTJHjY1JHIfw4AhrgBxK2i3I6eIZ7Gt/JTviQ4\n"
    "5MWGC9VI2lrwC3EPQsXbBIKHTg3pxq9NxIwOjvrbqetz9SMYCjMzlsFwvgtFb6Ih\n"
    "B1O9dRAMt3Hh3ZPk9qb2L0NU3581bJV7qDG6MNSTPsvFgbiKpHcLaVZAelpHy69r\n"
    "RlM450FJ/YrzOPEPH89o9Cqk8gZEBxBfwGV9ldMt2uW7LwyIQGAPRYu8IJlvD2fw\n"
    "/CySxgD+LkrkLP1QdMtC3QpBC/C7PEPpg6DoL4VsU/2j6F01K+IgnhTaEsaHLPDa\n"
    "CWt4dRapQvzL2jIy43YcA15GT0qyVBpWZJFvT0ZcTj72lx9nnbkEWMEANfWeqOgC\n"
    "EsUotiEIO6S8+M8MI5oX4DvARd150ePWbu9bNUrQojSjGM2JH/x6kVzsZZP4WG3Q\n"
    "5371FFuXe1QIXtcs2zgj30L397ATHd8979k/8sc+TXd1ba4YzA2j/ncI5jIor0UA\n"
    "hxUYugd1O8FNqahxZpIntxX4dERuX0AT4+4qSG4s10RV1VbbGNot91xq/KM3kZEe\n"
    "r8fvJMIuFNgUqU9ffv0Bt5qeIquPdUH0xhEUoxiTeukz9KobbVZt3hZvG4BrmBC0\n"
    "UYZD6jBcVcA99yDYQ5EUuu7cmHJY2tHdvmhBhAugIfbGldMeripzgiIR1pRblSZB\n"
    "HkY/WUL0IavBvRnAYsYmxXb9Mbp/1vK3xYUTUha2oed2wDPA0ZqBQ+jnb12te1kV\n"
    "kYdjxFM=\n"
    "-----END ENCRYPTED PRIVATE KEY-----"
};

static const char PEM_PKCS8_V2[] = {
    "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"
    "MIICzzBJBgkqhkiG9w0BBQ0wPDAbBgkqhkiG9w0BBQwwDgQIeDCEmXfjzmsCAggA\n"
    "MB0GCWCGSAFlAwQBAgQQpieyiZovXD0OSQPE01x9gASCAoDXhEqWInWJLXyeLKXW\n"
    "bwSXgpQfk38S2jyo7OaNMthNdvQ83K3PctQfwxiiQ9W15FIS27/w4oHXmiukmN5V\n"
    "J+fCPwZ90e4lnuKzyuQcCL0LS+h+EXV5H0b254jOBwmuEfL38tekUa9RnV4e/RxK\n"
    "9uocePeHpFQv1RwwqzLVsptgMNX6NsRQ3YwLpCw9qzPFcejC8WZBLjB9osn4QD18\n"
    "GXORCNUPIJE7LV9/77SNcgchVIXCbSu1sRmiJRpDYc6E91Y6xbDl2KNNgCM3PrU6\n"
    "ERiP/8wetlbZZeX/tKZOCmA+n5pQQmeBkC/JaI8zqH9ZZODIuHDNzJWjtyKENfOT\n"
    "zM4u2RnRFhkp4bzjAZCwfh0Ink1Ge082OHEzN/+4KkSPdxoCKfIPTPS70NQ3vX7F\n"
    "u9IzC+yN1T+pVxluwbhRPQmuOvIX3hca6BIBS+cevppp1E/KXRD5WNtSkJbDknEH\n"
    "3phVQxEu1oaEhb/5e9AgQGg7aEqXX12MQLD+0V3/v65Z4FPvkiejjLL6PU1FuLyG\n"
    "fzZRT+GyiHLfpxZYt7aictQWAT2he7Rn7gJefJLSnFsoKVHoOvmfMvYZU3yZZaZD\n"
    "WenrGheUSrDX5slnqwON0iD/xAh6Z7KVr5U8RNvGrkyYzvXVKS1LTjJ1qfnD7JdF\n"
    "1CbNoCd7rfe5fSxtdKsgP77SMkKO+kN/0Z2P1iIfxE5SsRyxzq/o8dar/olB8Ttz\n"
    "ebDWpX6F16ew1DUDWgi9Dm5Jr17yZjldbcOhpqKYS7Jwe8mQUz+swO/HBIlm7qYg\n"
    "fKdkFYQyjOG2/4nzRPSdw235vs9Bd4R0s+p89cXsZmFHQQU9utYuPl/87a4RwaRT\n"
    "ASbM\n"
    "-----END ENCRYPTED PRIVATE KEY-----\n"
};

class MyAuthListener : public AuthListener {
    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {
        if (credMask & AuthListener::CRED_CERT_CHAIN) {
            creds.SetCertChain(x509cert);
        }
        if (credMask & AuthListener::CRED_PRIVATE_KEY) {
            creds.SetPrivateKey(PEM_AES);
        }
        if (credMask & AuthListener::CRED_PASSWORD) {
            creds.SetPassword("123456");
        }
        return true;
    }
    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        printf("Authentication %s %s\n", authMechanism, success ? "succesful" : "failed");
    }
};

int main(int argc, char** argv)
{
    QStatus status;
    qcc::KeyBlob privKey;
    qcc::String pubStr;

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    Crypto_RSA pk(512);

    size_t pkSize = pk.GetSize();

    size_t inLen;
    uint8_t in[2048];
    size_t outLen;
    uint8_t out[2048];

    // Import PEM encoded PKCS8 (3DES encrypted SSLeay legacy format)
    {
        printf("Testing private key import 3DES SSLeay legacy format\n");
        Crypto_RSA priv;
        status = priv.ImportPKCS8(PEM_DES, "123456");
        if (status != ER_OK) {
            printf("ImportPKCS8 failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
    }
    // Import PEM encoded PKCS8 (AES encrypted SSLeay legacy format)
    {
        printf("Testing private key import AES SSLeay legacy format\n");
        Crypto_RSA priv;
        status = priv.ImportPKCS8(PEM_AES, "123456");
        if (status != ER_OK) {
            printf("ImportPKCS8 failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
    }
    // Import PEM encoded PKCS8 v1.5 encrypted format)
    {
        printf("Testing private key import PKCS8 PKCS#5 v1.5\n");
        Crypto_RSA priv;
        status = priv.ImportPKCS8(PEM_PKCS8_V1_5, "123456");
        if (status != ER_OK) {
            printf("ImportPKCS8 failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
    }
    // Import PEM encoded PKCS8 v1.5 encrypted format)
    {
        printf("Testing private key import PKCS8 PKCS#5 v2.0\n");
        Crypto_RSA priv;
        status = priv.ImportPKCS8(PEM_PKCS8_V2, "123456");
        if (status != ER_OK) {
            printf("ImportPKCS8 failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
    }
    // Import a public key from a cert
    {
        Crypto_RSA pub;
        status = pub.ImportPEM(x509cert);
        if (status != ER_OK) {
            printf("ImportPEM failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
        qcc::String str;
        status = pub.ExportPEM(str);
        if (status != ER_OK) {
            printf("ExportPEM failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
        printf("PEM:\n%s\n", str.c_str());
    }

    {
        printf("Public key:\n%s\n", pubStr.c_str());
        status = pk.ExportPrivateKey(privKey, "pa55pHr@8e");
        if (status != ER_OK) {
            printf("ExportPrivateKey failed %s\n", QCC_StatusText(status));
        }

        /*
         * Test encryption with private key and decryption with public key
         */
        printf("Testing encryption/decryption\n");

        memcpy(in, hw, sizeof(hw));
        inLen = sizeof(hw);
        outLen = pkSize;
        status = pk.PublicEncrypt(in, inLen, out, outLen);
        if (status == ER_OK) {
            printf("Encrypted size %u\n", static_cast<uint32_t>(outLen));
        } else {
            printf("PublicEncrypt failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
        inLen = outLen;
        outLen = pkSize;
        memcpy(in, out, pkSize);
        status = pk.PrivateDecrypt(in, inLen, out, outLen);
        if (status == ER_OK) {
            printf("Decrypted size %u\n", static_cast<uint32_t>(outLen));
            printf("Decrypted %s\n", out);
        } else {
            printf("PrivateDecrypt failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
    }

    {
        printf("Testing cert generation\n");
        /*
         * Test certificate generation
         */
        status = pk.MakeSelfCertificate("my name", "my app");
        if (status != ER_OK) {
            printf("MakeSelfCertificate failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
        printf("Cert:\n%s", pk.CertToString().c_str());
        status = pk.ExportPrivateKey(privKey, "password1234");
        if (status != ER_OK) {
            printf("ExportPrivateKey failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
        status = pk.ExportPEM(pubStr);
        if (status != ER_OK) {
            printf("ExportPEM failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }

        /*
         * Initialize separate private and public key instances and test encryption a decryption
         */
        Crypto_RSA pub;
        status = pub.ImportPEM(pubStr);
        if (status != ER_OK) {
            printf("ImportPEM failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
        printf("PEM:\n%s\n", pubStr.c_str());

        Crypto_RSA pri;
        status = pri.ImportPrivateKey(privKey, "password1234");
        if (status != ER_OK) {
            printf("ImportPrivateKey failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }

        pkSize = pub.GetSize();

        memcpy(in, hw, sizeof(hw));
        inLen = sizeof(hw);
        outLen = pkSize;
        status = pub.PublicEncrypt(in, inLen, out, outLen);
        if (status == ER_OK) {
            printf("Encrypted size %u\n", static_cast<uint32_t>(outLen));
        } else {
            printf("PublicEncrypt failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }

        inLen = outLen;
        outLen = pkSize;
        memcpy(in, out, pkSize);
        status = pri.PrivateDecrypt(in, inLen, out, outLen);
        if (status == ER_OK) {
            printf("Decrypted size %u\n", static_cast<uint32_t>(outLen));
            printf("Decrypted %s\n", out);
        } else {
            printf("PrivateDecrypt failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
    }

    {
        printf("Testing empty passphrase\n");
        /*
         * Check we allow an empty password
         */
        status = pk.ExportPrivateKey(privKey, "");
        if (status != ER_OK) {
            printf("ExportPrivateKey failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
        status = pk.ExportPEM(pubStr);
        if (status != ER_OK) {
            printf("ExportPEM failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
        /*
         * Initialize separate private and public key instances and test signing a verification
         */
        Crypto_RSA pub;
        status = pub.ImportPEM(pubStr);
        if (status != ER_OK) {
            printf("ImportPEM failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
        Crypto_RSA pri;
        status = pri.ImportPrivateKey(privKey, "");
        if (status != ER_OK) {
            printf("ImportPrivateKey failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }

        const char doc[] = "This document requires a signature";
        uint8_t signature[64];
        size_t sigLen = sizeof(signature);

        printf("Testing signature and verification\n");
        /*
         * Test Sign and Verify APIs
         */
        status = pri.Sign((const uint8_t*)doc, sizeof(doc), signature, sigLen);
        if (status != ER_OK) {
            printf("Sign failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
        status = pub.Verify((const uint8_t*)doc, sizeof(doc), signature, sigLen);
        if (status == ER_OK) {
            printf("Digital signature was verified\n");
        } else {
            printf("Verify failed %s\n", QCC_StatusText(status));
            goto FailExit;
        }
    }

    /*
     * Now test the RSA authentication mechanism
     */
    {

        BusAttachment bus("srp");
        MyAuthListener myListener;
        bus.EnablePeerSecurity("ALLJOYN_RSA_KEYX", &myListener);

        ProtectedAuthListener listener;
        listener.Set(&myListener);

        SASLEngine responder(bus, ajn::AuthMechanism::RESPONDER, "ALLJOYN_RSA_KEYX", "1:1", listener);
        SASLEngine challenger(bus, ajn::AuthMechanism::CHALLENGER, "ALLJOYN_RSA_KEYX", "1:1", listener);

        SASLEngine::AuthState rState = SASLEngine::ALLJOYN_AUTH_FAILED;
        SASLEngine::AuthState cState = SASLEngine::ALLJOYN_AUTH_FAILED;

        qcc::String rStr;
        qcc::String cStr;

        while (status == ER_OK) {
            status = responder.Advance(cStr, rStr, rState);
            if (status != ER_OK) {
                printf("Responder returned %s\n", QCC_StatusText(status));
                goto FailExit;
            }
            status = challenger.Advance(rStr, cStr, cState);
            if (status != ER_OK) {
                printf("Challenger returned %s\n", QCC_StatusText(status));
                goto FailExit;
            }
            if ((rState == SASLEngine::ALLJOYN_AUTH_SUCCESS) && (cState == SASLEngine::ALLJOYN_AUTH_SUCCESS)) {
                break;
            }
        }
    }

    printf("!!!PASSED\n");
    return 0;

FailExit:

    printf("!!!FAILED\n");
    return -1;

}
