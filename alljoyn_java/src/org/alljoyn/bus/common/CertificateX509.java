/*
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
 */
package org.alljoyn.bus.common;

import java.util.Arrays;

public class CertificateX509 {

    /**
     * The Authority key identifier size in bytes
     */
    public static final long AUTHORITY_KEY_ID_SZ = 8;

    /**
     * The validity period
     */
    public class ValidPeriod {
        long validFrom; /**< the date time when the cert becomes valid
                                expressed in the number of seconds in EPOCH Jan 1, 1970 */
        long validTo;  /**< the date time after which the cert becomes invalid
                                expressed in the number of seconds in EPOCH Jan 1, 1970 */
    }

    /**
     * encoding format
     */
    public enum EncodingType {
        /**
         * X.509 DER format
         */
        ENCODING_X509_DER,
        /**
         * X.509 DER PEM format
         */
        ENCODING_X509_DER_PEM
    }

    /**
     * Certificate type
     */
    public enum CertificateType {
        UNRESTRICTED_CERTIFICATE,
        IDENTITY_CERTIFICATE,
        MEMBERSHIP_CERTIFICATE,
        /**
         * certificate not valid for any AllJoyn purpose
         */
        INVALID_CERTIFICATE
    }

    /**
     * Allocate native resources.
     */
    private native void create(CertificateType type) throws Exception;

    /**
     * Default Constructor
     */
    public CertificateX509() throws Exception
    {
        create(null);
    }

    /**
     * Constructor
     * @param type the certificate type.
     */
    CertificateX509(CertificateType type) throws Exception
    {
        create(type);
    }

    /**
     * Release native resources.
     */
    private synchronized native void destroy();

    /**
     * Let the Java garbage collector release resources.
     */
    @Override
    protected void finalize() throws Throwable {
        destroy();
        super.finalize();
    }

    /**
     * Decode a PEM encoded certificate.
     * @param pem the encoded certificate.
     * @throws Exception 
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void decodeCertificatePEM(String pem) throws Exception;

    /**
     * Export the certificate as PEM encoded.
     * @return pem the encoded certificate.
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native String encodeCertificatePEM() throws Exception;

    /**
     * Decode a DER encoded certificate.
     * @param der the encoded certificate.
     * @throws Exception
     * @throws BusException 
     * ER_OK for success; otherwise, error code.
     */
    public native void decodeCertificateDER(String der) throws Exception;

    /**
     * Export the certificate as DER encoded.
     * @return der the encoded certificate.
     * @throws Exception 
     * @throws BusException 
     * ER_OK for success; otherwise, error code.
     */
    public native String encodeCertificateDER(String der) throws Exception;

    /**
     * Export only the TBS section of the certificate as DER encoded.
     * This is suitable for using to generate a signature outside of this class.
     *
     * @return tbsder The binary DER-encoded TBS portion of the certificate.
     * @throws Exception 
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native String encodeCertificateTBS() throws Exception;

    /**
     * Encode the private key in a PEM string.
     * @param privateKey the private key to encode
     * @return the encoded output string holding the resulting PEM string
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     *
     */
    public native static String encodePrivateKeyPEM(ECCPrivateKey privateKey) throws Exception;

    /**
     * Decode the private key from a PEM string.
     * @param encoded the input string holding the PEM string
     * @return privateKey the output private key
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native static ECCPrivateKey decodePrivateKeyPEM(String encoded) throws Exception;

    /**
     * Encode the public key in a PEM string.
     * @param publicKey the public key to encode
     * @return encoded the output string holding the resulting PEM string
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native static String encodePublicKeyPEM(ECCPublicKey publicKey) throws Exception;

    /**
     * Decode the public key from a PEM string.
     * @param encoded the input string holding the PEM string
     * @return publicKey the output public key
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native static ECCPublicKey decodePublicKeyPEM(String encoded) throws Exception;

    /**
     * Sign the certificate.
     * @param key the ECDSA private key.
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void sign(ECCPrivateKey key) throws Exception;

    /**
     * Set the signature to a provided byte array, when signing the certificate externally.
     * This method does not verify the signature is valid, please use Verify with the
     * corresponding public key to make sure.
     *
     * @see qcc::CertificateX509::Verify(const ECCPublicKey* key);
     *
     * @param signature An ECCSignature containing the signature.
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setSignature(ECCSignature sig) throws Exception;

    /**
     * Sign the certificate and generate the authority key identifier.
     * @param privateKey the ECDSA private key.
     * @param publicKey the ECDSA public key to generate the authority key
     *                  identifier.
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void signAndGenerateAuthorityKeyId(ECCPrivateKey privateKey, ECCPublicKey publicKey) throws Exception;

    /**
     * Verify a self-signed certificate.
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void verify() throws Exception;

    /**
     * Verify the certificate.
     * @param key the ECDSA public key.
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void verify(ECCPublicKey key) throws Exception;

    /**
     * Verify the certificate against the trust anchor.
     * @param trustAnchor the trust anchor
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void verify(KeyInfoNISTP256 trustAnchor) throws Exception;

    /**
     * Verify the validity period of the certificate.
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void verifyValidity() throws Exception;

    /**
     * Set the serial number field
     * @param serialNumber The serial number
     * @param len          Length of the serial array
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setSerial(byte[] serialNumber, long len) throws Exception;

    /**
     * Set the serial number to be a random 20-byte string. Callers using this
     * functionality in a Certificate Authority are responsible for keeping track of
     * used serial numbers from previous certificate issuances, checking the serial
     * number after a successful call to this method, and generating new ones until
     * an unused serial number is generated. Repeated failure to generate an unused
     * serial number may suggest a problem with the platform randomness generator.
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void generateRandomSerial() throws Exception;

    /**
     * Get the serial number
     * @return the serial number
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native byte[] getSerial() throws Exception;

    /**
     * Set the issuer organization unit field
     * @param ou the organization unit
     * @param len the length of the organization unit field
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setIssuerOU(byte[] ou, long len) throws Exception;

    /**
     * Get the issuer organization unit field
     * @return the issuer organization unit field
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native byte getIssuerOU() throws Exception;

    /**
     * Set the issuer common name field
     * @param cn the common name
     * @param len the length of the common name field
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setIssuerCN(byte[] cn, long len) throws Exception;

    /**
     * Get the issuer common name field
     * @return the issuer common name field
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native byte[] getIssuerCN() throws Exception;

    /**
     * Set the subject organization unit field
     * @param ou the organization unit
     * @param len the length of the organization unit field
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setSubjectOU(byte[] ou, long len) throws Exception;

    /**
     * Get the subject organization unit field
     * @return the subject organization unit field
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native byte[] getSubjectOU() throws Exception;

    /**
     * Set the subject common name field
     * @param cn the common name
     * @param len the length of the common name field
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setSubjectCN(byte[] cn, long len) throws Exception;

    /**
     * Get the subject common name field
     * @return the subject common name field
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native byte[] getSubjectCN() throws Exception;

    /**
     * Set the subject alt name field
     * @param subjectAltName the subject alt name
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setSubjectAltName(String subjectAltName) throws Exception;

    /**
     * Get the subject alt name field
     * @return the subject alt name
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native String getSubjectAltName() throws Exception;

    /**
     * Generate the authority key identifier.
     * @param issuerPubKey the issuer's public key
     * @return authorityKeyId the authority key identifier
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native static String generateAuthorityKeyId(ECCPublicKey issuerPubKey) throws Exception;

    /**
     * Generate the issuer authority key identifier for the certificate.
     * @param issuerPubKey the issuer's public key
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void generateIssuerAuthorityKeyId(ECCPublicKey issuerPubKey) throws Exception;

    /**
     * Get the Authority Key Identifier
     * @return the Authority Key Identifier
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native String getAuthorityKeyId() throws Exception;

    /**
     * Updates the current AuthorityKeyId with a new one.
     *
     * @param newAki the new AuthorityKeyId for this certificate.
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setAuthorityKeyId(String newAki) throws Exception;

    /**
     * Set the validity field
     * @param validPeriod the validity period
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setValidity(ValidPeriod validPeriod) throws Exception;

    /**
     * Get the validity period.
     * @return the validity period
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native ValidPeriod getValidity() throws Exception;

    /**
     * Set the subject public key field
     * @param key the subject public key
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setSubjectPublicKey(ECCPublicKey key) throws Exception;

    /**
     * Get the subject public key
     * @return the subject public key
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native ECCPublicKey getSubjectPublicKey() throws Exception;

    /**
     * Indicate that the subject may act as a certificate authority.
     * @param flag flag indicating the subject may act as a CA
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setCA(boolean flag) throws Exception;

    /**
     * Can the subject act as a certificate authority?
     * @return true if so.
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native boolean isCA() throws Exception;

    /**
     * Set the digest of the external data.
     * @param digest the digest of the external data
     * @param size the size of the digest.
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void setDigest(byte[] digest, long size) throws Exception;

    /**
     * Get the digest of the external data.
     * @return The digest of the external data
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native byte[] getDigest() throws Exception;

    /**
     * Is the optional digest field present in the certificate?
     * @return whether this optional field is present in the certificate.
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native boolean isDigestPresent() throws Exception;

    /**
     * Get the PEM encoded bytes for the certificate
     * @return the PEM encoded bytes
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native String getPEM() throws Exception;

    /**
     * Load the PEM encoded bytes for the certificate
     * @param PEM the encoded bytes
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native void loadPEM(String PEM) throws Exception;

    /**
     * Returns a human readable string for a cert if there is one associated with this key.
     *
     * @return A string for the cert or and empty string if there is no cert.
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native String toJavaString() throws Exception;

    /**
     * Determine if this certificate issued a given certificate by comparing the distinguished
     * name and verifying the digital signature.
     * @param issuedCertificate Certificate to check if it was issued by this certificate.
     * @return true if so.
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native boolean isIssuerOf(CertificateX509 issuedCertificate) throws Exception;

    /**
     * Is the subject DN of this certificate equal to a given DN?
     * @param cn Common Name component of the DN to compare to.
     * @param cnLength Length of the cn array. Zero if null.
     * @param ou Organizational Unit component of the DN to compare to.
     * @param ouLength Length of the ou array. Zero if null.
     * @return true if so.
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native boolean isDNEqual(byte[] cn, long cnLength, byte[] ou, long ouLength) throws Exception;

    /**
     * Is the subject DN of this certificate equal to a given certificate's DN?
     * @param other CertificateX509 to compare to.
     * @return true if so.
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native boolean isDNEqual(CertificateX509 other) throws Exception;

    /**
     * Is the subject public key of this certificate equal to a given key?
     * @param publicKey Public key to compare to.
     * @return true if so.
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native boolean isSubjectPublicKeyEqual(ECCPublicKey publicKey) throws Exception;

    /**
     * Get the SHA-256 thumbprint of this certificate.
     * @return thumbprint buffer of size Crypto_SHA256::DIGEST_SIZE to receive the thumbprint
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native byte[] getSHA256Thumbprint() throws Exception;

    /**
     * Retrieve the number of X.509 certificates in a PEM string representing a cert chain.
     * @param encoded the input string holding the PEM string
     * @param certChain the input string holding the array of certs.
     * @param count the expected number of certs
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native static CertificateX509[] decodeCertChainPEM(String encoded, CertificateX509[] certChain, long count) throws Exception;

    /**
     * Validate the certificate type of each cert in the certificate chain.
     * The end-entity cert must have a type.
     * Any signing cert in the chain must have the same type or unrestricted
     * type in order to sign the next cert in the chain.
     * @param certChain the array of certs.
     * @param count the number of certs
     * @return true if valid; false, otherwise;
     *
     * @throws Exception
     * @throws BusException
     * ER_OK for success; otherwise, error code.
     */
    public native static boolean validateCertificateTypeInCertChain(CertificateX509[] certChain, long count) throws Exception;

}
