/**
 * @file
 * @brief  Sample implementation of a local Certificate Authority utility to generate
 * and manage certificates.
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
#include <qcc/FileStream.h>
#include <qcc/time.h>

#include <alljoyn/Status.h>
#include <alljoyn/Init.h>

using namespace std;
using namespace qcc;

static const String CA_CERT_FILE("caCert.pem");
static const String CA_KEY_FILE("caKey.pem");

static const String EE_CERT_FILE("eeCert.pem");
static const String EE_KEY_FILE("eeKey.pem");

static const size_t SERIAL_NUMBER_LENGTH = 20; /* RFC 5280 4.1.2.2 */

/* Print utility usage documentation. */
void PrintUsage()
{
    printf(
        "Usage: SampleCertificateUtility <command>\n"
        "\n"
        " where <command> is one of: \n"
        "\n");

    printf(
        "   -createCA <validity in days> <CA subject name>\n"
        "        Create a self-signed certificate suitable for use as a Certificate\n"
        "        Authority (CA). This CA certificate will be saved as %s\n"
        "        and the private key as %s in the current working directory\n"
        "        for use with this utility.\n"
        "        ex: -createCA 3650 My AllJoyn Certificate Authority\n"
        "\n",
        CA_CERT_FILE.c_str(),
        CA_KEY_FILE.c_str());

    printf(
        "   -createEE <validity in days> <end entity subject name>\n"
        "        Create an end-entity certificate suitable for use as an AllJoyn\n"
        "        authentication certificate. %s and %s must exist\n"
        "        in the current working directory as created by a previous call to\n"
        "        -createCa. %s will contain the certificate and \n"
        "        %s will contain the private key.\n"
        "        ex: -createEE 365 My AllJoyn Node\n"
        "\n",
        CA_CERT_FILE.c_str(),
        CA_KEY_FILE.c_str(),
        EE_CERT_FILE.c_str(),
        EE_KEY_FILE.c_str());

}

/*
 * Create and sign a certificate given an existing CA issuer. This function only
 * supports subjects and issuers that only use the Common Name (CN) portion of the Distinguished
 * Name.
 *
 * @param[subjectCN] Common Name component of the subject's Distinguished Name.
 * @param[subjectCNSize] Size of subjectCN array.
 * @param[subjectPublicKey] Pointer to ECCPublicKey struct containing the subject's public key.
 * @param[issuerCN] Common Name component of the issuer's Distinguished Name. For correct certificate chain
 *                  validation, this must exactly match the issuer certificate's subject CN.
 * @param[issuerCNSize] Size of the issuerCN array.
 * @param[issuerPrivateKey] Pointer to ECCPrivateKey containing the issuer's private key for signing.
 * @param[validity] ValidPeriod structure containing the notValidBefore and notValidAfter dates for the certificate.
 * @param[isCA] Whether or not the certificate should be labeled as a certificate authority (true) or an end entity (false).
 * @param[certificate] Output which will receive the signed certificate.
 * @return A QStatus indicating success or failure; ER_OK means certificate was successfully created and signed, any other
 *         value means failure and the contents of certificate should not be used.
 */
QStatus CreateAndSignCertificate(
    const uint8_t* subjectCN,
    const size_t subjectCNSize,
    const ECCPublicKey* subjectPublicKey,
    const uint8_t* issuerCN,
    const size_t issuerCNSize,
    const ECCPrivateKey* issuerPrivateKey,
    const CertificateX509::ValidPeriod& validity,
    bool isCA,
    CertificateX509& certificate)
{
    /* RFC 5280 requires a non-empty Issuer, and for CAs, a non-empty Subject. Although the standard
     * allows empty Subject fields in certain circumstances, namely the presence of a subjectAltNames extension,
     * this code won't allow it. */
    if (0 >= subjectCNSize) {
        return ER_BAD_ARG_2;
    }
    if (0 >= issuerCNSize) {
        return ER_BAD_ARG_5;
    }

    /* Although not prohibited by the standard, it's highly unlikely NULs should be in either buffer.
     * Nothing except NUL has a zero byte in UTF-8, so we can safely scan the entire array for zero
     * bytes. Anything calling this code must make sure subjectCNSize and issuerCNSize do not include
     * any terminating NULLs if inputs are derived from null-terminated C strings.
     */
    for (size_t iSubjectCN = 0; iSubjectCN < subjectCNSize; iSubjectCN++) {
        if (0 == subjectCN[iSubjectCN]) {
            return ER_BAD_ARG_1;
        }
    }

    for (size_t iIssuerCN = 0; iIssuerCN < issuerCNSize; iIssuerCN++) {
        if (0 == issuerCN[iIssuerCN]) {
            return ER_BAD_ARG_4;
        }
    }


    certificate.SetSubjectCN(subjectCN, subjectCNSize);
    certificate.SetIssuerCN(issuerCN, issuerCNSize);
    certificate.SetSubjectPublicKey(subjectPublicKey);

    /* The best serial numbers are random numbers. RFC 5280 permits a maximum of 20 octets.
     * So we get that much randomness and use that as the serial number. If the high order bit is set,
     * ASN.1 encoding requires a leading zero which could result in 21 octets, violating the standard.
     * As a result, we always set the high order bit to be zero. This gives us 159 random bits which should
     * be more than enough.

     * A real CA MUST keep track of all serial numbers it issues to guarantee it never uses the same
     * serial number twice. This sample doesn't do this. With a good source
     * of randomness this is unlikely, but this should still be checked because if it ever happens, it's
     * likely an indicator that the system's source of randomness is insufficiently random. */
    uint8_t serialNumber[SERIAL_NUMBER_LENGTH];

    QStatus status = Crypto_GetRandomBytes(serialNumber, sizeof(serialNumber));
    if (ER_OK != status) {
        printf("Could not generate random serial number; status is %s\n", QCC_StatusText(status));
        return status;
    }

    /* Clear the high order bit to avoid that leading zero when ASN.1-encoded. */
    serialNumber[0] &= 0x7F;

    String serialNumberString(reinterpret_cast<const char*>(serialNumber), sizeof(serialNumber));
    certificate.SetSerial(serialNumberString);
    certificate.SetCA(isCA);
    certificate.SetValidity(&validity);

    status = certificate.Sign(issuerPrivateKey);

    return status;
}

/*
 * Load a PEM-encoded X.509 certificate from a file.
 *
 * @param[certificateFileName] String containing the file name of the certificate to load.
 * @param[certificate] CertificateX509 object that receives the loaded certificate on success.
 * @return QStatus which will be ER_OK on successful load, and another error code describing the specific
 *         failure otherwise.
 */
QStatus LoadCertificateFromFile(const String& certificateFileName, CertificateX509& certificate)
{
    QStatus status = ER_OK;

    FileSource certFileSource(certificateFileName);

    if (!certFileSource.IsValid()) {
        printf("Could not open cert file source for %s\n", certificateFileName.c_str());
        return ER_OS_ERROR;
    }

    int64_t fileSize = 0;

    status = certFileSource.GetSize(fileSize);

    if (ER_OK != status) {
        printf("Could not get size of cert file; status is %s\n", QCC_StatusText(status));
        return status;
    }

    unique_ptr<char[]> bytes(new char[fileSize]);

    int64_t bytesRead = 0;

    while (bytesRead < fileSize) {
        size_t bytesReadCurrent = 0;
        status = certFileSource.PullBytes(bytes.get() + bytesRead, fileSize - bytesRead, bytesReadCurrent, Event::WAIT_FOREVER);
        if (ER_OK != status) {
            printf("Failed while pulling bytes from cert file; status is %s\n", QCC_StatusText(status));
            return status;
        } else {
            bytesRead += bytesReadCurrent;
        }
    }

    String certPem(bytes.get(), fileSize);
    status = certificate.DecodeCertificatePEM(certPem);

    return status;
}

/*
 * Save a PEM-encoded X.509 certificate to a file.
 *
 * @param[certificateFileName] String containing the file name of the certificate to save. If this file exists,
 *                      will be overwritten.
 * @param[certificate] CertificateX509 object to save.
 * @return QStatus which will be ER_OK on successful save, and another error code describing the specific
 *         failure otherwise.
 */
QStatus SaveCertificateToFile(const String& certificateFileName, CertificateX509& certificate)
{
    QStatus status = ER_OK;

    FileSink certFileSink(certificateFileName, FileSink::Mode::WORLD_READABLE);

    if (!certFileSink.IsValid()) {
        printf("Could not open cert file sink for %s\n", certificateFileName.c_str());
        return ER_OS_ERROR;
    }

    String certPem = certificate.GetPEM();

    size_t bytesWritten = 0;
    const size_t bytesToWrite = certPem.size();
    const char* bytes = certPem.data();

    /* The contract for PushBytes might not write all the bytes we give, so loop until all bytes are written
     * or we fail. */
    while ((ER_OK == status) && (bytesWritten < bytesToWrite)) {
        size_t numSent = 0;

        status = certFileSink.PushBytes(bytes + bytesWritten, bytesToWrite - bytesWritten, numSent);
        if (ER_OK != status) {
            printf("Could not PushBytes to cert file sink; status is %s\n", QCC_StatusText(status));
        } else {
            bytesWritten += numSent;
        }
    }

    if (ER_OK != status) {
        (void)DeleteFile(certificateFileName);
    }

    return status;

}

/*
 * Load a PEM-encoded X.509 certificate and public key from a file.
 *
 * @param[certificateFileName] String containing the file name to load the certificate from.
 * @param[keyFileName] String containing the file name to load the private key from.
 * @param[certificate] CertificateX509 object to receive the loaded certificate on success.
 * @param[privateKey] Pointer to ECCPrivateKey object to receive the loaded private key on success.
 *                    No checking is done to ensure the private key corresponds to the public key of certificate.
 * @return QStatus which will be ER_OK on successful load, and another error code describing the specific
 *         failure otherwise.
 */
QStatus LoadCertificateAndPrivateKeyFromFile(
    const String& certificateFileName,
    const String& keyFileName,
    CertificateX509& certificate,
    ECCPrivateKey* privateKey)
{
    QStatus status = ER_OK;

    FileSource keyFileSource(keyFileName);

    if (!keyFileSource.IsValid()) {
        printf("Could not open source for key file %s\n", keyFileName.c_str());
        return ER_OS_ERROR;
    }

    status = LoadCertificateFromFile(certificateFileName, certificate);
    if (ER_OK != status) {
        return status;
    }

    int64_t fileSize = 0;

    status = keyFileSource.GetSize(fileSize);

    if (ER_OK != status) {
        printf("Could not get size of key file; status is %s\n", QCC_StatusText(status));
        return status;
    }

    unique_ptr<char[]> bytes(new char[fileSize]);

    int64_t bytesRead = 0;

    while (bytesRead < fileSize) {
        size_t bytesReadCurr = 0;
        status = keyFileSource.PullBytes(bytes.get() + bytesRead, fileSize - bytesRead, bytesReadCurr, Event::WAIT_FOREVER);
        if (ER_OK != status) {
            printf("Failed while pulling bytes from key file; status is %s\n", QCC_StatusText(status));
            return status;
        } else {
            bytesRead += bytesReadCurr;
        }
    }

    String keyPem(bytes.get(), fileSize);
    status = CertificateX509::DecodePrivateKeyPEM(keyPem, privateKey->d, sizeof(privateKey->d));

    return status;
}

/*
 * Save a PEM-encoded X.509 certificate and its private key to a file.
 *
 * @param[certificateFileName] String containing the file name where to save the certificate. If this file exists,
 *                      it will be overwritten.
 * @param[keyFileName] String containing the file name where to save the private key. If this file exists,
 *                     it will be overwritten.
 * @param[certificate] CertificateX509 object to save.
 * @param[privateKey] Private key to save. No checking is done to make sure this private key corresponds to
 *                    the public key of certificate.
 * @return QStatus which will be ER_OK on successful save, and another error code describing the specific
 *         failure otherwise.
 */
QStatus SaveCertificateAndPrivateKeyToFile(
    const String& certificateFileName,
    const String& keyFileName,
    CertificateX509& certificate,
    const ECCPrivateKey* privateKey)
{
    QStatus status = ER_OK;

    /* Establish a scope for the keyFileSink object so we can be sure it's written to disk when we exit the scope. */
    do {
        /* For this sample, we won't instruct the OS to make the key file protected. In general, you should
         * keep private keys in a secure place. */
        FileSink keyFileSink(keyFileName, FileSink::Mode::WORLD_READABLE);

        if (!keyFileSink.IsValid()) {
            /* If we fail this early we can just bail directly since there's no cleanup to do. */
            printf("Could not open key file sink for %s", keyFileName.c_str());
            return ER_OS_ERROR;
        }

        String privateKeyPem;
        QStatus status = CertificateX509::EncodePrivateKeyPEM(privateKey->d, sizeof(privateKey->d), privateKeyPem);

        if (ER_OK != status) {
            printf("Could not encode private key as PEM; status is %s\n", QCC_StatusText(status));
        } else {

            size_t bytesWritten = 0;
            const size_t bytesToWrite = privateKeyPem.size();
            const char* bytes = privateKeyPem.data();

            /* The contract for PushBytes might not write all the bytes we give, so loop until all bytes are written
             * or we fail. */
            while ((ER_OK == status) && (bytesWritten < bytesToWrite)) {
                size_t numSent = 0;

                status = keyFileSink.PushBytes(bytes + bytesWritten, bytesToWrite - bytesWritten, numSent);

                if (ER_OK != status) {
                    printf("Could not PushBytes to cert file sink; status is %s\n", QCC_StatusText(status));
                }

                bytesWritten += numSent;
            }
        }
    } while (0);

    if (ER_OK == status) {
        status = SaveCertificateToFile(certificateFileName, certificate);
    }

    if (ER_OK != status) {
        /* Don't leave any files we might have written around if we didn't succeed. Don't worry about failing here. */
        (void)DeleteFile(keyFileName);
        (void)DeleteFile(certificateFileName);
    }

    return status;
}

/*
 * Handler for the -createCA command line option.
 *
 * @param[argc] Number of arguments in the argv array.
 * @param[argv] CreateCA-specific arguments in the form: <validity period in days> <subject CN>
 * @return 0 on success, 1 on failure.
 */
int CreateCA(int argc, char** argv)
{
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    /* FileSink will happily clobber existing files. Check to see if they exist first,
     * and if so, abort before we do. */
    if ((ER_OK == FileExists(CA_CERT_FILE)) ||
        (ER_OK == FileExists(CA_KEY_FILE))) {
        printf("CA cert file %s or key file %s already exists. Aborting.\nDelete these files if you want to regenerate the CA.\n",
               CA_CERT_FILE.c_str(), CA_KEY_FILE.c_str());
        return 1;
    }

    int validityInDays = atoi(argv[0]);

    if (validityInDays <= 0) {
        printf("Invalid validity period %s. Validity period must be an integer >= 1.\n", argv[0]);
        PrintUsage();
        return 1;
    }

    /* Assemble subject string from all the remaining arguments. */
    String subjectCN;

    subjectCN.assign(argv[1]);
    for (int i = 2; i < argc; i++) {
        subjectCN.append(' ');
        subjectCN.append(argv[i]);
    }

    CertificateX509::ValidPeriod validity;
    validity.validFrom = qcc::GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + (validityInDays * 86400);

    Crypto_ECC keypair;
    QStatus status = keypair.GenerateDSAKeyPair();

    if (ER_OK != status) {
        printf("Failed to generate a key pair; status is %s\n", QCC_StatusText(status));
        return 1;
    }

    CertificateX509 certificate;

    status = CreateAndSignCertificate(
        reinterpret_cast<const uint8_t*>(subjectCN.c_str()),
        subjectCN.size(),
        keypair.GetDSAPublicKey(),
        reinterpret_cast<const uint8_t*>(subjectCN.c_str()),
        subjectCN.size(),
        keypair.GetDSAPrivateKey(),
        validity,
        true,
        certificate);
    if (ER_OK != status) {
        printf("Failed to create and sign the certificate; status is %s\n", QCC_StatusText(status));
        return 1;
    }

    status = SaveCertificateAndPrivateKeyToFile(CA_CERT_FILE, CA_KEY_FILE, certificate, keypair.GetDSAPrivateKey());
    if (ER_OK != status) {
        printf("Failed to save the CA certificate and private key to a file; status is %s\n", QCC_StatusText(status));
        return 1;
    }

    printf("Successfully saved new CA certificate to %s\n"
           "and private key to %s.\n", CA_CERT_FILE.c_str(), CA_KEY_FILE.c_str());

    return 0;
}

/*
 * Handler for the -createEE command line option.
 *
 * @param[argc] Number of arguments in the argv array.
 * @param[argv] CreateEE-specific arguments in the form: <validity period in days> <subject CN>
 * @return 0 on success, 1 on failure.
 */

int CreateEE(int argc, char** argv)
{
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    int validityInDays = atoi(argv[0]);

    if (validityInDays <= 0) {
        printf("Invalid validity period %s. Validity period must be an integer >= 1.\n", argv[0]);
        PrintUsage();
        return 1;
    }

    /* Load a pre-existing CA certificate and key pair. */
    CertificateX509 caCertificate;
    ECCPrivateKey caPrivateKey;

    QStatus status = LoadCertificateAndPrivateKeyFromFile(CA_CERT_FILE, CA_KEY_FILE, caCertificate, &caPrivateKey);

    if (ER_OK != status) {
        printf("Failed to load the certificate and private key pair for the CA; status is %s\n", QCC_StatusText(status));
        return 1;
    }

    /* Make sure the loaded certificate has the cA flag set to TRUE. */
    if (!caCertificate.IsCA()) {
        printf("Loaded CA certificate is an end entity certificate and cannot issue certificates.\n"
               "Please provide a valid CA certificate in %s or re-generate.\n", CA_CERT_FILE.c_str());
        return 1;
    }

    Crypto_ECC eeKeyPair;

    status = eeKeyPair.GenerateDSAKeyPair();
    if (ER_OK != status) {
        printf("Failed to generate a new key pair for the EE certificate; status is %s\n", QCC_StatusText(status));
        return 1;
    }

    /* Assemble subject string from all the remaining arguments. */
    String subjectCN;

    subjectCN.assign(argv[1]);
    for (int i = 2; i < argc; i++) {
        subjectCN.append(' ');
        subjectCN.append(argv[i]);
    }

    CertificateX509::ValidPeriod validity;
    validity.validFrom = qcc::GetEpochTimestamp() / 1000;
    validity.validTo = validity.validFrom + (validityInDays * 86400);

    CertificateX509 eeCertificate;

    status = CreateAndSignCertificate(
        reinterpret_cast<const uint8_t*>(subjectCN.c_str()),
        subjectCN.size(),
        eeKeyPair.GetDSAPublicKey(),
        caCertificate.GetSubjectCN(),
        caCertificate.GetSubjectCNLength(),
        &caPrivateKey,
        validity,
        false,
        eeCertificate);
    if (ER_OK != status) {
        printf("Failed to create and sign a new EE certificate; status is %s\n", QCC_StatusText(status));
        return 1;
    }

    status = SaveCertificateAndPrivateKeyToFile(EE_CERT_FILE, EE_KEY_FILE, eeCertificate, eeKeyPair.GetDSAPrivateKey());
    if (ER_OK != status) {
        printf("Failed to save EE certificate and private key; status is %s\n", QCC_StatusText(status));
        return 1;
    }

    printf("Successfully saved new EE certificate to %s and private key to %s.\n", EE_CERT_FILE.c_str(), EE_KEY_FILE.c_str());

    return 0;
}

/** Main entry point */
int CDECL_CALL main(int argc, char** argv, char** envArg)
{
    QCC_UNUSED(envArg);
    int ret;

    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    QStatus status = AllJoynInit();
    if (ER_OK != status) {
        printf("Couldn't initialize AllJoyn; status is %s\n.", QCC_StatusText(status));
        return 1;
    }

    if (0 == strcasecmp("-createCA", argv[1])) {
        ret = CreateCA(argc - 2, argv + 2);
    } else if (0 == strcasecmp("-createEE", argv[1])) {
        ret = CreateEE(argc - 2, argv + 2);
    } else {
        PrintUsage();
        ret = 1;
    }

    AllJoynShutdown();

    return ret;
}
