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

#include <qcc/Crypto.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#include <qcc/CertificateHelper.h>
#include <qcc/FileStream.h>
#include <qcc/time.h>
#include <qcc/Util.h>

#include <alljoyn/Status.h>
#include <alljoyn/Init.h>
#include <alljoyn/AuthListener.h>

#include "SampleCertificateChainEngine.h"

using namespace std;
using namespace qcc;
using namespace ajn;

/* Microsoft compilers use a different printf specifier for printing size_t. This will let
 * us use a token for the correct one depending on what platform we're on.
 */
#ifdef QCC_OS_GROUP_WINDOWS
#define SIZE_T_PRINTF "%Iu"
#else
#define SIZE_T_PRINTF "%zu"
#endif

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

bool VerifyCertificateChain(const AuthListener::Credentials& creds)
{
    /* This certificate chain verification engine does the following:
     *
     * 1. Verifies all certificates in the chain are time-valid.
     * 2. Verifies that all CA's in the chain have the CA flag set to TRUE,
     * 3. Verifies the cryptographic binding between each certificate.
     * 4. Verifies the certificates chain up to one of the trusted roots.
     *
     * Other implementations may make app-dependent decisions, such as:
     * - checking for particular Key Usages or Extended Key Usages (EKUs) on certificates,
     * - verifying the certificate's subject name equals some known value. */

    /* If we didn't get a certificate chain, there's nothing to validate! */
    if (!creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
        printf("VerifyCertificateChain FAILED: No certificate chain provided!\n");
        return false;
    }

    /* Decode the list of roots into an array of CertificateX509 objects to later check against. */
    size_t numberOfRoots = ArraySize(TRUSTED_ROOTS_PEM);
    unique_ptr<CertificateX509[]> trustedRoots(new (std::nothrow) CertificateX509[numberOfRoots]);
    if (!trustedRoots) {
        printf("VerifyCertificateChain FAILED: Out of memory.\n");
        return false;
    }

    QStatus status = ER_OK;

    for (size_t iRoot = 0; iRoot < numberOfRoots; iRoot++) {
        status = trustedRoots[iRoot].DecodeCertificatePEM(TRUSTED_ROOTS_PEM[iRoot]);
        if (ER_OK != status) {
            printf("VerifyCertificateChain FAILED: Failed to decode trusted root at position " SIZE_T_PRINTF ". Status is %s.\n",
                   iRoot, QCC_StatusText(status));
            return false;
        }
    }

    /* Decode the certificates in the chain into another array of CertificateX509 objects. */
    size_t chainLength = 0;
    status = CertificateHelper::GetCertCount(creds.GetCertChain(), &chainLength);
    if (ER_OK != status) {
        printf("VerifyCertificateChain FAILED: Could not get length of certificate chain. Status is %s.\n", QCC_StatusText(status));
        return false;
    }

    unique_ptr<CertificateX509[]> certChain(new (std::nothrow) CertificateX509[chainLength]);
    if (!certChain) {
        printf("VerifyCertificateChain FAILED: Out of memory allocating certificate chain.\n");
        return false;
    }

    status = CertificateX509::DecodeCertChainPEM(creds.GetCertChain(), certChain.get(), chainLength);

    if (ER_OK != status) {
        printf("VerifyCertificateChain FAILED: Failed to decode certificate chain. Status is %s.\n", QCC_StatusText(status));
        return false;
    }

    /* Here is where you could check additional properties of the certificate, depending on your application and scenario's
     * needs.
     *
     * If you make use of the CN or OU fields of the Distinguished Name, remember that these are UTF-8 strings, so
     * make sure you use a string type that understands UTF-8! In particular, you should never interpret these
     * as null-terminated C strings, because a legal UTF-8 string could have a NUL character anywhere in it. */

    for (size_t iCert = 0; iCert < chainLength; iCert++) {
        /* Every certificate must be time-valid. */
        status = certChain[iCert].VerifyValidity();
        if (ER_OK != status) {
            printf("VerifyCertificatechain FAILED; following certificate is not time valid:\n%s\n",
                   certChain[iCert].ToString().c_str());
            return false;
        }

        /* First, if the current certificate is issued by a trusted root, we're done. */
        for (size_t iRoot = 0; iRoot < numberOfRoots; iRoot++) {
            if (trustedRoots[iRoot].IsIssuerOf(certChain[iCert])) {
                printf("VerifyCertificateChain SUCCEEDED; trusted root certificate is:\n%s\n",
                       trustedRoots[iRoot].ToString().c_str());
                return true;
            }
        }

        /* If not, and there's a next certificate in the chain, check the chaining between the i'th certificate
         * and the i+1'th.
         */
        if ((iCert + 1) < chainLength) {
            /* First, the next certificate in the chain must be a CA certificate. */
            if (!certChain[iCert + 1].IsCA()) {
                printf("VerifyCertificateChain FAILED: following certificate is not a CA certificate:\n%s\n",
                       certChain[iCert + 1].ToString().c_str());
                return false;
            }
            /* Now check the chaining. IsIssuerOf checks both that the issuer DN of the i'th certificate equals
             * the subject DN of the i+1'th certificate in the chain, and verifies the cryptographic signature
             * was produced by the i+1'th certificate. */
            if (!certChain[iCert + 1].IsIssuerOf(certChain[iCert])) {
                printf("VerifyCertificateChain FAILED: certificate at position " SIZE_T_PRINTF " did not issue certificate at position " SIZE_T_PRINTF "\n"
                       "Certificate[" SIZE_T_PRINTF "]:\n%s\n"
                       "Certificate[" SIZE_T_PRINTF "]:\n%s\n",
                       iCert + 1,
                       iCert,
                       iCert + 1,
                       certChain[iCert + 1].ToString().c_str(),
                       iCert,
                       certChain[iCert].ToString().c_str());
                return false;
            }
            /* This would be the place to check transitive EKUs or other chain properties, if desired. */
        }
    }

    /* At this point, we've gone through the entire chain and didn't see a trusted root, so fail. */
    printf("VerifyCertificateChain FAILED: did not see a trusted root in the chain.\n");
    return false;
}