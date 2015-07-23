/**
 * @file
 * @brief  Sample implementation of code to validate a certificate chain.
 */

/******************************************************************************
 *
 *
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

#include <qcc/platform.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <memory>

#include <qcc/Log.h>
#include <qcc/String.h>

#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>

#include <alljoyn/Status.h>
#include <alljoyn/Init.h>
#include <alljoyn/AuthListener.h>

#include "SampleCertificateChainEngine.h"

using namespace std;
using namespace qcc;
using namespace ajn;

/* This is a list of PEM-encoded CA certificates which forms our trusted root list.
 * If one of these certificates is ever seen on a chain, the chain is considered trusted.
 */
static const char* TRUSTED_ROOTS_PEM[] = {
    /* AllJoyn ECDHE Sample Unused Certificate Authority
     *
     * This is included to demonstrate having more than one trusted root, but as the
     * name implies, it doesn't issue any certificates in this sample. */
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBijCCATCgAwIBAgIUVSjE1Fv/6jP30BfkRXmSoA8sEkIwCgYIKoZIzj0EAwIw\n"
    "PDE6MDgGA1UEAwwxQWxsSm95biBFQ0RIRSBTYW1wbGUgVW51c2VkIENlcnRpZmlj\n"
    "YXRlIEF1dGhvcml0eTAeFw0xNTA1MDcxNzE0MDdaFw0yNTA1MDQxNzE0MDdaMDwx\n"
    "OjA4BgNVBAMMMUFsbEpveW4gRUNESEUgU2FtcGxlIFVudXNlZCBDZXJ0aWZpY2F0\n"
    "ZSBBdXRob3JpdHkwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAS7SmQ19lKjLo2C\n"
    "yyqubmHPRNAo8Eo/i300UWhNAkurVy/WJ3zFMxYNwJeenZ46qJsYb4faZp3iuXF7\n"
    "mllsClzjoxAwDjAMBgNVHRMEBTADAQH/MAoGCCqGSM49BAMCA0gAMEUCIQD/zB3n\n"
    "0+gxUHOdZZadDfLQjMuFxR3LMzUqdBbYZudOGwIgKPT2KYGTW7P/H1hIM6wAyHBB\n"
    "lBOnPXqXjFLodiM+8zM=\n"
    "-----END CERTIFICATE-----\n",
    /* AllJoyn ECDHE Sample Certificate Authority
     * This CA issued the certificates used for the Client and Service in this sample. */
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBezCCASKgAwIBAgIUDrFhHE80+zbEUOCNTxw219Nd1qwwCgYIKoZIzj0EAwIw\n"
    "NTEzMDEGA1UEAwwqQWxsSm95biBFQ0RIRSBTYW1wbGUgQ2VydGlmaWNhdGUgQXV0\n"
    "aG9yaXR5MB4XDTE1MDUwNzIyMTYzNloXDTI1MDUwNDIyMTYzNlowNTEzMDEGA1UE\n"
    "AwwqQWxsSm95biBFQ0RIRSBTYW1wbGUgQ2VydGlmaWNhdGUgQXV0aG9yaXR5MFkw\n"
    "EwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE6AsCTTviTBWX0Jw2e8Cs8DhwxfRd37Yp\n"
    "IH5ALzBqwUN2sfG1odcthe6GKdE/9oVfy12SXOL3X2bi3yg1XFoWnaMQMA4wDAYD\n"
    "VR0TBAUwAwEB/zAKBggqhkjOPQQDAgNHADBEAiASuD0OrpDM8ziC5GzMbZWKNE/X\n"
    "eboedc0p6YsAZmry2AIgR23cKM4cKkc2bgUDbETNbDcOcwm+EWaK9E4CkOO/tBc=\n"
    "-----END CERTIFICATE-----\n"
};

static QStatus CountChunksFromEncoded(const String& encoded, const char* beginToken, const char* endToken, size_t* count)
{
    size_t pos;

    *count = 0;
    qcc::String remainder = encoded;
    for (;;) {
        pos = remainder.find(beginToken);
        if (pos == qcc::String::npos) {
            /* no more */
            return ER_OK;
        }
        remainder = remainder.substr(pos + strlen(beginToken));
        pos = remainder.find(endToken);
        if (pos == qcc::String::npos) {
            return ER_OK;
        }
        *count += 1;
        remainder = remainder.substr(pos + strlen(endToken));
    }
    /* Unreachable. */
}

static QStatus GetCertCount(const String& encoded, size_t* count)
{
    const char* const BEGIN_CERT_TAG = "-----BEGIN CERTIFICATE-----";
    const char* const END_CERT_TAG = "-----END CERTIFICATE-----";
    *count = 0;
    return CountChunksFromEncoded(encoded, BEGIN_CERT_TAG, END_CERT_TAG, count);
}

bool VerifyCertificateChain(const AuthListener::Credentials& creds)
{
    /* This certificate chain verification engine does the following:
     *
     * 1. Verifies all certificates in the chain are time-valid.
     * 2. Verifies that all CA's in the chain have the CA flag set to TRUE,
     * 3. Verifies the cryptographic binding between each certificate.
     * 4. Verifies the certificates chain up to one of the trusted roots.
     * 5. Verifies the end-entity certificate is an identity certificate, and the chain is valid
     *    for this purpose.
     *
     * Other implementations may make app-dependent decisions, such as verifying the certificate's
     * subject name equals some known value. */

    /* If we didn't get a certificate chain, there's nothing to validate! */
    if (!creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
        printf("VerifyCertificateChain FAILED: No certificate chain provided!\n");
        return false;
    }

    /* Decode the list of roots into an array of CertificateX509 objects to later check against. */
    size_t numberOfRoots = sizeof(TRUSTED_ROOTS_PEM) / sizeof(TRUSTED_ROOTS_PEM[0]);
    CertificateX509* trustedRoots = new CertificateX509[numberOfRoots];
    CertificateX509* certChain = NULL;

    QStatus status = ER_OK;

    for (size_t i = 0; (ER_OK == status) && (i < numberOfRoots); i++) {
        status = trustedRoots[i].DecodeCertificatePEM(TRUSTED_ROOTS_PEM[i]);
        if (ER_OK != status) {
            /* Note that PRIuSIZET is not a standards-defined constant, but is defined in AllJoyn to select the
             * correct platform-dependent format specifier for type size_t.
             */
            printf("VerifyCertificateChain FAILED: Failed to decode trusted root at position %" PRIuSIZET ". Status is %s.\n",
                   i, QCC_StatusText(status));
        }
    }

    size_t chainLength = 0;
    if (ER_OK == status) {
        /* Decode the certificates in the chain into another array of CertificateX509 objects. */
        status = GetCertCount(creds.GetCertChain(), &chainLength);
        if (ER_OK != status) {
            printf("VerifyCertificateChain FAILED: Could not get length of certificate chain. Status is %s.\n", QCC_StatusText(status));
        }
    }

    if (ER_OK == status) {
        certChain = new CertificateX509[chainLength];
        status = CertificateX509::DecodeCertChainPEM(creds.GetCertChain(), certChain, chainLength);

        if (ER_OK != status) {
            printf("VerifyCertificateChain FAILED: Failed to decode certificate chain. Status is %s.\n", QCC_StatusText(status));
        }
    }

    /* Here is where you could check additional properties of the certificate, depending on your application and scenario's
     * needs.
     *
     * If you make use of the CN or OU fields of the Distinguished Name, remember that these are UTF-8 strings, so
     * make sure you use a string type that understands UTF-8! In particular, you should never interpret these
     * as null-terminated C strings, because a legal UTF-8 string could have a NUL character anywhere in it. */
    bool trusted = false;

    if (ER_OK == status) {
        /* Most of the time in your code you'll be writing code to check identity certificates, and so we check for that
         * type of certificate in this sample. */
        if (certChain[0].GetType() == CertificateX509::IDENTITY_CERTIFICATE) {
            /* Ensure that the Extended Key Usages are valid for the whole chain. In AllJoyn, we insist the end-entity
             * certificate is not unrestricted (has at least one EKU). We then make sure every Certificate Authority to the
             * root has that EKU present or is unrestricted. We recommend all CA's, including roots, be issued with AllJoyn
             * EKUs to ensure they are not used for other purposes. */
            trusted = CertificateX509::ValidateCertificateTypeInCertChain(certChain, chainLength);
        }
        /* trusted will remain false if certificate is not an identity certificate. */
    }

    if (trusted && (ER_OK == status)) {
        trusted = false;
        for (size_t iCert = 0; iCert < chainLength; iCert++) {
            /* Every certificate must be time-valid. */
            status = certChain[iCert].VerifyValidity();
            if (ER_OK != status) {
                printf("VerifyCertificatechain FAILED; following certificate is not time valid:\n%s\n",
                       certChain[iCert].ToString().c_str());
                status = ER_OK; /* Reset to ER_OK to signal there was no internal failure. trusted is false. */
                break;
            }

            /* If the current certificate is issued by a trusted root, we're done. */
            for (size_t iRoot = 0; iRoot < numberOfRoots; iRoot++) {
                if (trustedRoots[iRoot].IsIssuerOf(certChain[iCert])) {
                    printf("VerifyCertificateChain SUCCEEDED; trusted root certificate is:\n%s\n",
                           trustedRoots[iRoot].ToString().c_str());
                    trusted = true;
                    /* This break gets us out of this iRoot for loop. */
                    break;
                }
            }
            if (trusted) {
                /* And if we've decided the chain is trusted, break out of the iCert for loop too. */
                break;
            }

            /* If not, and there's a next certificate in the chain, check the chaining between the i'th certificate
             * and the i+1'th.
             */
            if ((iCert + 1) < chainLength) {
                /* First, the next certificate in the chain must be a CA certificate. */
                if (!certChain[iCert + 1].IsCA()) {
                    printf("VerifyCertificateChain FAILED: following certificate is not a CA certificate:\n%s\n",
                           certChain[iCert + 1].ToString().c_str());
                    break;
                }
                /* Now check the chaining. IsIssuerOf checks both that the issuer DN of the i'th certificate equals
                 * the subject DN of the i+1'th certificate in the chain, and verifies the cryptographic signature
                 * was produced by the i+1'th certificate. */
                if (!certChain[iCert + 1].IsIssuerOf(certChain[iCert])) {
                    /* Note that PRIuSIZET is not a standards-defined constant, but is defined in AllJoyn to select the
                     * correct platform-dependent format specifier for type size_t.
                     */
                    printf("VerifyCertificateChain FAILED: certificate at position %" PRIuSIZET " did not issue certificate at position %" PRIuSIZET "\n"
                           "Certificate[%" PRIuSIZET "]:\n%s\n"
                           "Certificate[%" PRIuSIZET "]:\n%s\n",
                           iCert + 1,
                           iCert,
                           iCert + 1,
                           certChain[iCert + 1].ToString().c_str(),
                           iCert,
                           certChain[iCert].ToString().c_str());
                    break;
                }
            }
        }
    }

    /* At this point one of three things has happened:
     * status is not ER_OK: something failed before we could even check the chain, and so we're failing;
     * status is ER_OK and trusted is false: we successfully checked the chain but no trusted root was found
     * or a certificate was not time-valid;
     * or status is ER_OK and trusted is true: we successfully checked the chain and found a trusted root in the path.
     *
     * Clean up and return the appropriate result.
     */

    delete[] trustedRoots;
    delete[] certChain;

    if (ER_OK != status) {
        /* In this case, we're returning false because of some internal failure. */
        return false;
    } else {
        /* Otherwise, return the trusted result. */
        if (!trusted) {
            printf("VerifyCertificateChain FAILED: did not see a trusted root in the chain.\n");
        }
        return trusted;
    }
}