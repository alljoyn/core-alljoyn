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
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <memory>

/* Platform-dependent headers for file I/O */
#if defined(QCC_OS_GROUP_WINDOWS)
/* Map POSIX equivalents in the Microsoft CRT to their POSIX names. */
#include <io.h>
#define open _open
#define close _close
#define read _read
#define write _write
#define fstat _fstat
#define stat _stat
#define unlink _unlink
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_CREAT _O_CREAT
#define O_TRUNC _O_TRUNC
#define S_IREAD _S_IREAD
#define S_IWRITE _S_IWRITE
typedef struct _stat statstruct;
typedef int ssize_t;
#else
#include <unistd.h>
typedef struct stat statstruct;
#define O_BINARY 0 /* O_BINARY is only on Windows; other OSes don't make a distinction. */
#if defined(QCC_OS_ANDROID) /* Android chose not to include the deprecated constants. */
#define S_IREAD S_IRUSR
#define S_IWRITE S_IWUSR
#endif /* QCC_OS_ANDROID */
#endif /* !QCC_OS_GROUP_WINDOWS */

/* Platform-dependent headers for time functions */
#if defined(QCC_OS_GROUP_WINDOWS)
#include <sys/types.h>
#include <sys/timeb.h>
#elif defined(QCC_OS_DARWIN)
#include <sys/time.h>
#include <mach/mach_time.h>
#include <mach/clock.h>
#include <mach/mach.h>
#else
#include <time.h>
#endif

#include <qcc/Log.h>
#include <qcc/String.h>

#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>

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

/**
 * Generate random bytes and deposit them in the provided buffer.
 *
 * @param[out] buf Buffer to receive random bytes.
 * @param[in] count Number of bytes to generate and place in buf.
 * @return ER_OK if buf is successfully filled with count random bytes. Any other value
 *         is an error, and in that case, the contents of buf should not be used.
 */
static QStatus Crypto_GetRandomBytes(uint8_t* buf, const size_t count)
{
    /* Getting good randomness is highly platform-dependent. To keep this sample simple,
     * we will acquire it by generating one or more ECC keys and using the private key for
     * the randomness. */
    QStatus status = ER_OK;
    Crypto_ECC ecc;

    size_t i = 0;
    while (i < count) {
        status = ecc.GenerateDSAKeyPair();
        if (ER_OK != status) {
            return status;
        }
        const ECCPrivateKey* key = ecc.GetDSAPrivateKey();
        memcpy(buf + i, key->GetD(), min(count - i, key->GetDSize()));
        i += min(count - i, key->GetDSize());
    }

    return status;
}

/**
 * Check for a file's existence.
 *
 * @param[in] fileName Filename to check.
 * @return ER_OK if file exists; ER_FAIL if not.
 */
static QStatus FileExists(const String& fileName)
{
    statstruct statBuf;

    if (-1 == stat(fileName.c_str(), &statBuf)) {
        return ER_FAIL;
    } else {
        return ER_OK;
    }
}

#if defined(QCC_OS_GROUP_WINDOWS)
uint64_t GetEpochTimestamp()
{
    struct __timeb64 time_buffer;
    _ftime64(&time_buffer);

    uint64_t ret_val = time_buffer.time * (uint64_t)1000;
    ret_val += time_buffer.millitm;

    return ret_val;
}
#else

static void platform_gettime(struct timespec* ts, bool useMonotonic)
{
#if defined(QCC_OS_DARWIN)
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;

#else
    clock_gettime(useMonotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME, ts);
#endif
}

static uint64_t GetEpochTimestamp()
{
    struct timespec ts;
    uint64_t ret_val;

    platform_gettime(&ts, false);

    ret_val = ((uint64_t)(ts.tv_sec)) * 1000;
    ret_val += (uint64_t)ts.tv_nsec / 1000000;

    return ret_val;
}
#endif

/*
 * Create and sign a certificate given an existing CA issuer. This function only
 * supports subjects and issuers that only use the Common Name (CN) portion of the Distinguished
 * Name.
 *
 * @param[in] subjectCN Common Name component of the subject's Distinguished Name.
 * @param[in] subjectCNSize Size of subjectCN array.
 * @param[in] subjectPublicKey Pointer to ECCPublicKey struct containing the subject's public key.
 * @param[in] issuerCN Common Name component of the issuer's Distinguished Name. For correct certificate chain
 *                     validation, this must exactly match the issuer certificate's subject CN.
 * @param[in] issuerCNSize Size of the issuerCN array.
 * @param[in] issuerPrivateKey Pointer to ECCPrivateKey containing the issuer's private key for signing.
 * @param[in] validity ValidPeriod structure containing the notValidBefore and notValidAfter dates for the certificate.
 * @param[in] isCA Whether or not the certificate should be labeled as a certificate authority (true) or an end entity (false).
 * @param[out] certificate Output which will receive the signed certificate.
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

    certificate.SetSerial(serialNumber, sizeof(serialNumber));
    certificate.SetCA(isCA);
    certificate.SetValidity(&validity);

    status = certificate.Sign(issuerPrivateKey);

    return status;
}

/*
 * Load a PEM-encoded X.509 certificate from a file.
 *
 * @param[in] certificateFileName String containing the file name of the certificate to load.
 * @param[out] certificate CertificateX509 object that receives the loaded certificate on success.
 * @return QStatus which will be ER_OK on successful load, and another error code describing the specific
 *         failure otherwise.
 */
QStatus LoadCertificateFromFile(const String& certificateFileName, CertificateX509& certificate)
{
    QStatus status = ER_OK;

    int certificateFd = open(certificateFileName.c_str(), O_RDONLY | O_BINARY);

    if (-1 == certificateFd) {
        printf("Could not open certificate file %s; errno is %d\n", certificateFileName.c_str(), errno);
        return ER_OS_ERROR;
    }

    statstruct statBuf;

    if (-1 == fstat(certificateFd, &statBuf)) {
        printf("Could not get size of certificate file\n");
        status = ER_OS_ERROR;
    }

    if (ER_OK == status) {
        char* bytes = new char[statBuf.st_size];

        off_t bytesRead = 0;

        while ((ER_OK == status) && (bytesRead < statBuf.st_size)) {
            ssize_t bytesReadCurrent = read(certificateFd, bytes + bytesRead, statBuf.st_size - bytesRead);
            if (-1 == bytesReadCurrent) {
                printf("Failed while reading bytes from certificate file; errno is %d\n", errno);
                status = ER_OS_ERROR;
            } else if (0 == bytesReadCurrent && bytesRead < statBuf.st_size) {
                printf("EOF before reading all bytes\n");
                status = ER_OS_ERROR;
            } else {
                bytesRead += bytesReadCurrent;
            }
        }

        if (ER_OK == status) {
            String certPem(bytes, statBuf.st_size);
            status = certificate.DecodeCertificatePEM(certPem);
        }

        delete[] bytes;
    }

    if (-1 == close(certificateFd)) {
        /* In the case of a load, failure to close the fd isn't fatal. Log but don't fail
         * because we may have gotten a valid certificate anyway. */
        printf("Failed to close certificate file descriptor; errno is %d\n", errno);
    }

    return status;
}

/*
 * Save a PEM-encoded X.509 certificate to a file.
 *
 * @param[in] certificateFileName String containing the file name of the certificate to save. If this file exists,
 *                      will be overwritten.
 * @param[in] certificate CertificateX509 object to save.
 * @return QStatus which will be ER_OK on successful save, and another error code describing the specific
 *         failure otherwise.
 */
QStatus SaveCertificateToFile(const String& certificateFileName, CertificateX509& certificate)
{
    QStatus status = ER_OK;

    int certificateFd = open(certificateFileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);

    if (-1 == certificateFd) {
        printf("Could not open certificate file for %s; errno is %d\n", certificateFileName.c_str(), errno);
        return ER_OS_ERROR;
    }

    String certPem = certificate.GetPEM();

    size_t bytesWritten = 0;
    const size_t bytesToWrite = certPem.size();
    const char* bytes = certPem.data();

    /* Loop until all bytes are written or we fail. */
    while ((ER_OK == status) && (bytesWritten < bytesToWrite)) {
        ssize_t bytesWrittenCurrent = write(certificateFd, bytes + bytesWritten, bytesToWrite - bytesWritten);
        if (-1 == bytesWrittenCurrent) {
            printf("Could not write to certificate file; errno is %d\n", errno);
            status = ER_OS_ERROR;
        } else {
            bytesWritten += bytesWrittenCurrent;
        }
    }

    if (-1 == close(certificateFd)) {
        /* In the case of a save, this is fatal because we may have failed to flush our
         * output properly. */
        printf("Failed to close the certificate file descriptor; errno is %d.\n", errno);
        status = ER_OS_ERROR;
    }

    if (ER_OK != status) {
        /* Don't potentially leave a partly-written file around. */
        (void)unlink(certificateFileName.c_str());
    }

    return status;
}

/*
 * Load a PEM-encoded X.509 certificate and public key from a file.
 *
 * @param[in] certificateFileName String containing the file name to load the certificate from.
 * @param[in] keyFileName String containing the file name to load the private key from.
 * @param[out] certificate CertificateX509 object to receive the loaded certificate on success.
 * @param[out] privateKey Pointer to ECCPrivateKey object to receive the loaded private key on success.
 *                        No checking is done to ensure the private key corresponds to the public key of certificate.
 * @return QStatus which will be ER_OK on successful load, and another error code describing the specific
 *         failure otherwise.
 */
QStatus LoadCertificateAndPrivateKeyFromFile(
    const String& certificateFileName,
    const String& keyFileName,
    CertificateX509& certificate,
    ECCPrivateKey* privateKey)
{
    QStatus status = LoadCertificateFromFile(certificateFileName, certificate);
    if (ER_OK != status) {
        return status;
    }

    int keyFd = open(keyFileName.c_str(), O_RDONLY | O_BINARY);

    if (-1 == keyFd) {
        printf("Could not open key file %s; errno is %d\n", keyFileName.c_str(), errno);
        return ER_OS_ERROR;
    }

    statstruct statBuf;

    if (-1 == fstat(keyFd, &statBuf)) {
        printf("Could not get size of key file\n");
        status = ER_OS_ERROR;
    }

    if (ER_OK == status) {
        char* bytes = new char[statBuf.st_size];

        off_t bytesRead = 0;

        while ((ER_OK == status) && (bytesRead < statBuf.st_size)) {
            ssize_t bytesReadCurrent = read(keyFd, bytes + bytesRead, statBuf.st_size - bytesRead);
            if (-1 == bytesReadCurrent) {
                printf("Failed while reading bytes from key file; errno is %d\n", errno);
                status = ER_OS_ERROR;
            } else if ((0 == bytesReadCurrent) && (bytesRead < statBuf.st_size)) {
                printf("EOF before reading all bytes\n");
                status = ER_OS_ERROR;
            } else {
                bytesRead += bytesReadCurrent;
            }
        }

        if (ER_OK == status) {
            String keyPem(bytes, statBuf.st_size);
            status = CertificateX509::DecodePrivateKeyPEM(keyPem, privateKey);
        }

        delete[] bytes;
    }

    if (-1 == close(keyFd)) {
        /* In the case of a load, failure to close the fd isn't fatal. Log but don't fail
         * because we may have gotten a valid key anyway. */
        printf("Failed to close key file descriptor; errno is %d.\n", errno);
    }

    return status;
}

/*
 * Save a PEM-encoded X.509 certificate and its private key to a file.
 *
 * @param[in] certificateFileName String containing the file name where to save the certificate. If this file exists,
 *                      it will be overwritten.
 * @param[in] keyFileName String containing the file name where to save the private key. If this file exists,
 *                     it will be overwritten.
 * @param[in] certificate CertificateX509 object to save.
 * @param[in] privateKey Private key to save. No checking is done to make sure this private key corresponds to
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
    String privateKeyPem;

    QStatus status = SaveCertificateToFile(certificateFileName, certificate);
    if (ER_OK != status) {
        return status;
    }

    status = CertificateX509::EncodePrivateKeyPEM(privateKey, privateKeyPem);
    if (ER_OK != status) {
        return status;
    }

    int keyFd = open(keyFileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);

    if (-1 == keyFd) {
        printf("Could not open key file for %s; errno is %d\n", keyFileName.c_str(), errno);
        return ER_OS_ERROR;
    }

    size_t bytesWritten = 0;
    const size_t bytesToWrite = privateKeyPem.size();
    const char* bytes = privateKeyPem.data();

    /* Loop until all bytes are written or we fail. */
    while ((ER_OK == status) && (bytesWritten < bytesToWrite)) {
        ssize_t bytesWrittenCurrent = write(keyFd, bytes + bytesWritten, bytesToWrite - bytesWritten);
        if (-1 == bytesWrittenCurrent) {
            printf("Could not write to key file; errno is %d\n", errno);
            status = ER_OS_ERROR;
        } else {
            bytesWritten += bytesWrittenCurrent;
        }
    }

    if (-1 == close(keyFd)) {
        /* In the case of a save, this is fatal because we may have failed to flush our
         * output properly. */
        printf("Failed to close the key file descriptor; errno is %d.\n", errno);
        status = ER_OS_ERROR;
    }

    if (ER_OK != status) {
        /* Don't potentially leave a partly-written file around. */
        (void)unlink(keyFileName.c_str());
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

    /* Check to see if the CA files exist first,
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
    validity.validFrom = GetEpochTimestamp() / 1000;
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
    validity.validFrom = GetEpochTimestamp() / 1000;
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
