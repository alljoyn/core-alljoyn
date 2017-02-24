/*
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *    
 *    SPDX-License-Identifier: Apache-2.0
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *    
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
*/
package org.alljoyn.bus.common;

import java.util.Arrays;
import org.alljoyn.bus.BusException;

public class CertificateX509 {

    /**
     * The Authority key identifier size in bytes
     */
    public static final long AUTHORITY_KEY_ID_SZ = 8;

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
    private native void create(CertificateType type);

    /**
     * Default Constructor
     */
    public CertificateX509()
    {
        create(null);
        m_type = CertificateType.UNRESTRICTED_CERTIFICATE;
    }

    /**
     * Constructor
     * @param type the certificate type.
     */
    public CertificateX509(CertificateType type)
    {
        create(type);
        m_type = type;
    }

    private CertificateType m_type;

    /**
     * Get the certificate type.
     * @return Certificate type
     */
    public CertificateType getType() {
        return m_type;
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
     * @param pemlen the encoded certificate length.
     * @throws BusException
     * error code.
     */
    private native void decodeCertificatePEM(String pem, long pemlen) throws BusException;

    /**
     * Decode a PEM encoded certificate.
     * @param pem the encoded certificate.
     * @throws BusException
     * error code.
     */
    public void decodeCertificatePEM(String pem) throws BusException {
        decodeCertificatePEM(pem, pem.length());
    }

    /**
     * Export the certificate as PEM encoded.
     * @return pem the encoded certificate.
     * @throws BusException
     * error code.
     */
    public native String encodeCertificatePEM() throws BusException;

    /**
     * Decode a DER encoded certificate.
     * @param der the encoded certificate.
     * @param derlen the encoded certificate length.
     * @throws BusException
     * error code.
     */
    private native void decodeCertificateDER(byte[] der, long derlen) throws BusException;

    /**
     * Decode a DER encoded certificate.
     * @param der the encoded certificate.
     * @throws BusException 
     * error code.
     */
    public void decodeCertificateDER(byte[] der) throws BusException {
        decodeCertificateDER(der, der.length);
    }

    /**
     * Export the certificate as DER encoded.
     * @return der the encoded certificate.
     * @throws BusException 
     * error code.
     */
    public native byte[] encodeCertificateDER() throws BusException;

    /**
     * Export only the TBS section of the certificate as DER encoded.
     * This is suitable for using to generate a signature outside of this class.
     *
     * @return tbsder The binary DER-encoded TBS portion of the certificate.
     * @throws BusException
     * error code.
     */
    public native byte[] encodeCertificateTBS() throws BusException;

    /**
     * Encode the private key in a PEM string.
     * @param privateKey the private key to encode
     * @return the encoded output string holding the resulting PEM string
     * @throws BusException
     * error code.
     *
     */
    public native static String encodePrivateKeyPEM(ECCPrivateKey privateKey) throws BusException;

    /**
     * Decode the private key from a PEM string.
     * @param encoded the input string holding the PEM string
     * @param encodedlen the input string length
     * @return privateKey the output private key
     * @throws BusException
     * error code.
     */
    private native static ECCPrivateKey decodePrivateKeyPEM(String encoded, long encodedlen) throws BusException;

    /**
     * Decode the private key from a PEM string.
     * @param encoded the input string holding the PEM string
     * @return privateKey the output private key
     * @throws BusException
     * error code.
     */
    public static ECCPrivateKey decodePrivateKeyPEM(String encoded) throws BusException {
        return decodePrivateKeyPEM(encoded, encoded.length());
    }

    /**
     * Encode the public key in a PEM string.
     * @param publicKey the public key to encode
     * @return encoded the output string holding the resulting PEM string
     * @throws BusException
     * error code.
     */
    public native static String encodePublicKeyPEM(ECCPublicKey publicKey) throws BusException;

    /**
     * Decode the public key from a PEM string.
     * @param encoded the input string holding the PEM string
     * @param encodedlen the input string length
     * @return publicKey the output public key
     * @throws BusException
     * error code.
     */
    private native static ECCPublicKey decodePublicKeyPEM(String encoded, long encodedlen) throws BusException;

    /**
     * Decode the public key from a PEM string.
     * @param encoded the input string holding the PEM string
     * @return publicKey the output public key
     * @throws BusException
     * error code.
     */
    public static ECCPublicKey decodePublicKeyPEM(String encoded) throws BusException {
        return decodePublicKeyPEM(encoded, encoded.length());
    }

    /**
     * Sign the certificate.
     * @param key the ECDSA private key.
     * @throws BusException
     * error code.
     */
    public native void sign(ECCPrivateKey key) throws BusException;

    /**
     * Set the signature to a provided byte array, when signing the certificate externally.
     * This method does not verify the signature is valid, please use Verify with the
     * corresponding public key to make sure.
     *
     * @param signature An ECCSignature containing the signature.
     * @throws BusException
     * error code.
     */
    public native void setSignature(ECCSignature signature) throws BusException;

    /**
     * Sign the certificate and generate the authority key identifier.
     * @param privateKey the ECDSA private key.
     * @param publicKey the ECDSA public key to generate the authority key
     *                  identifier.
     * @throws BusException
     * error code.
     */
    public native void signAndGenerateAuthorityKeyId(ECCPrivateKey privateKey, ECCPublicKey publicKey) throws BusException;

    /**
     * Verify a self-signed certificate.
     *
     * @throws BusException
     * error code.
     */
    public native void verify() throws BusException;

    /**
     * Verify the certificate.
     * @param key the ECDSA public key.
     * @throws BusException
     * error code.
     */
    public native void verify(ECCPublicKey key) throws BusException;

    /**
     * Verify the certificate against the trust anchor.
     * @param trustAnchor the trust anchor
     * @throws BusException
     * error code.
     */
    public void verify(KeyInfoNISTP256 trustAnchor) throws BusException {
        verifyValidity();
        verify(trustAnchor.getPublicKey());
    }

    /**
     * Verify the validity period of the certificate.
     * @throws BusException
     * error code.
     */
    public native void verifyValidity() throws BusException;

    /**
     * Set the serial number field
     * @param serialNumber The serial number
     * @param len          Length of the serial array
     *
     * @throws BusException
     * error code.
     */
    private native void setSerial(byte[] serialNumber, long len) throws BusException;

    /**
     * Set the serial number field
     * @param serialNumber The serial number
     *
     * @throws BusException
     * error code.
     */
    public void setSerial(byte[] serialNumber) throws BusException {
        setSerial(serialNumber, serialNumber.length);
    }

    /**
     * Set the serial number to be a random 20-byte string. Callers using this
     * functionality in a Certificate Authority are responsible for keeping track of
     * used serial numbers from previous certificate issuances, checking the serial
     * number after a successful call to this method, and generating new ones until
     * an unused serial number is generated. Repeated failure to generate an unused
     * serial number may suggest a problem with the platform randomness generator.
     *
     * @throws BusException
     * error code.
     */
    public native void generateRandomSerial() throws BusException;

    /**
     * Get the serial number
     * @return the serial number
     *
     */
    public native byte[] getSerial();

    /**
     * Set the issuer organization unit field
     * @param ou the organization unit
     * @param len the length of the organization unit field
     *
     */
    private native void setIssuerOU(byte[] ou, long len);

    /**
     * Set the issuer organization unit field
     * @param ou the organization unit
     *
     */
    public void setIssuerOU(byte[] ou) {
        setIssuerOU(ou, ou.length);
    }

    /**
     * Get the issuer organization unit field
     * @return the issuer organization unit field
     *
     */
    public native byte[] getIssuerOU();

    /**
     * Set the issuer common name field
     * @param cn the common name
     * @param len the length of the common name field
     *
     */
    private native void setIssuerCN(byte[] cn, long len);

    /**
     * Set the issuer common name field
     * @param cn the common name
     *
     */
    public void setIssuerCN(byte[] cn) {
        setIssuerCN(cn, cn.length);
    }

    /**
     * Get the issuer common name field
     * @return the issuer common name field
     *
     */
    public native byte[] getIssuerCN();

    /**
     * Set the subject organization unit field
     * @param ou the organization unit
     * @param len the length of the organization unit field
     *
     */
    private native void setSubjectOU(byte[] ou, long len);

    /**
     * Set the subject organization unit field
     * @param ou the organization unit
     *
     */
    public void setSubjectOU(byte[] ou) {
        setSubjectOU(ou, ou.length);
    }

    /**
     * Get the subject organization unit field
     * @return the subject organization unit field
     *
     */
    public native byte[] getSubjectOU();

    /**
     * Set the subject common name field
     * @param cn the common name
     * @param len the length of the common name field
     *
     */
    private native void setSubjectCN(byte[] cn, long len);

    /**
     * Set the subject common name field
     * @param cn the common name
     *
     */
    public void setSubjectCN(byte[] cn){
        setSubjectCN(cn, cn.length);
    }

    /**
     * Get the subject common name field
     * @return the subject common name field
     *
     */
    public native byte[] getSubjectCN();

    /**
     * Set the subject alt name field
     * @param subjectAltName the subject alt name
     * @param subjectLength the subject alt name length
     *
     * @throws BusException
     * error code.
     */
    private native void setSubjectAltName(byte[] subjectAltName, long subjectLength) throws BusException;

    /**
     * Set the subject alt name field
     * @param subjectAltName the subject alt name
     *
     * @throws BusException
     * error code.
     */
    public void setSubjectAltName(byte[] subjectAltName) throws BusException {
        setSubjectAltName(subjectAltName, subjectAltName.length);
    }

    /**
     * Get the subject alt name field
     * @return the subject alt name
     *
     * @throws BusException
     * error code.
     */
    public native byte[] getSubjectAltName() throws BusException;

    /**
     * Generate the authority key identifier.
     * @param issuerPubKey the issuer's public key
     * @return authorityKeyId the authority key identifier
     *
     * @throws BusException
     * error code.
     */
    public native static byte[] generateAuthorityKeyId(ECCPublicKey issuerPubKey) throws BusException;

    /**
     * Generate the issuer authority key identifier for the certificate.
     * @param issuerPubKey the issuer's public key
     *
     * @throws BusException
     * error code.
     */
    public native void generateIssuerAuthorityKeyId(ECCPublicKey issuerPubKey) throws BusException;

    /**
     * Get the Authority Key Identifier
     * @return the Authority Key Identifier
     *
     * @throws BusException
     * error code.
     */
    public native byte[] getAuthorityKeyId() throws BusException;

    /**
     * Updates the current AuthorityKeyId with a new one.
     *
     * @param newAki the new AuthorityKeyId for this certificate.
     * @param newAkiLength the new AuthorityKeyId for this certificate.
     *
     * @throws BusException
     * error code.
     */
    private native void setAuthorityKeyId(byte[] newAki, long newAkiLength) throws BusException;

    /**
     * Updates the current AuthorityKeyId with a new one.
     *
     * @param newAki the new AuthorityKeyId for this certificate.
     *
     * @throws BusException
     * error code.
     */
    public void setAuthorityKeyId(byte[] newAki) throws BusException {
        setAuthorityKeyId(newAki, newAki.length);
    }

    /**
     * Set the validity field
     * @param validFrom
     * the date time when the cert becomes valid
     * expressed in the number of seconds in EPOCH Jan 1, 1970
     * @param validTo
     * the date time after which the cert becomes invalid
     * expressed in the number of seconds in EPOCH Jan 1, 1970
     *
     * @throws BusException
     * error code.
     */
    public native void setValidity(long validFrom, long validTo) throws BusException;

    /**
     * Get the validity start date.
     * @return
     * the date time when the cert becomes valid
     * expressed in the number of seconds in EPOCH Jan 1, 1970
     *
     * @throws BusException
     * error code.
     */
    public native long getValidFrom() throws BusException;

    /**
     * Get the validity end date.
     * @return
     * the date time when the cert becomes valid
     * expressed in the number of seconds in EPOCH Jan 1, 1970
     *
     * @throws BusException
     * error code.
     */
    public native long getValidTo() throws BusException;

    /**
     * Set the subject public key field
     * @param key the subject public key
     *
     * @throws BusException
     * error code.
     */
    public native void setSubjectPublicKey(ECCPublicKey key) throws BusException;

    /**
     * Get the subject public key
     * @return the subject public key
     *
     * @throws BusException
     * error code.
     */
    public native ECCPublicKey getSubjectPublicKey() throws BusException;

    /**
     * Indicate that the subject may act as a certificate authority.
     * @param flag flag indicating the subject may act as a CA
     *
     * @throws BusException
     * error code.
     */
    public native void setCA(boolean flag) throws BusException;

    /**
     * Can the subject act as a certificate authority?
     * @return true if so.
     *
     * @throws BusException
     * error code.
     */
    public native boolean isCA() throws BusException;

    /**
     * Set the digest of the external data.
     * @param digest the digest of the external data
     * @param size the size of the digest.
     *
     * @throws BusException
     * error code.
     */
    private native void setDigest(byte[] digest, long size) throws BusException;

    /**
     * Set the digest of the external data.
     * @param digest the digest of the external data
     *
     * @throws BusException
     * error code.
     */
    public void setDigest(byte[] digest) throws BusException {
        setDigest(digest, digest.length);
    }

    /**
     * Get the digest of the external data.
     * @return The digest of the external data
     *
     * @throws BusException
     * error code.
     */
    public native byte[] getDigest() throws BusException;

    /**
     * Is the optional digest field present in the certificate?
     * @return whether this optional field is present in the certificate.
     *
     * @throws BusException
     * error code.
     */
    public native boolean isDigestPresent() throws BusException;

    /**
     * Get the PEM encoded bytes for the certificate
     * @return the PEM encoded bytes
     *
     * @throws BusException
     * error code.
     */
    public native String getPEM() throws BusException;

    /**
     * Load the PEM encoded bytes for the certificate
     * @param PEM the encoded bytes
     * @param PEMlen the encoded bytes
     *
     * @throws BusException
     * error code.
     */
    private native void loadPEM(String PEM, long PEMlen) throws BusException;

    /**
     * Load the PEM encoded bytes for the certificate
     * @param PEM the encoded bytes
     *
     * @throws BusException
     * error code.
     */
    public void loadPEM(String PEM) throws BusException {
        loadPEM(PEM, PEM.length());
    }

    /**
     * Returns a human readable string for a cert if there is one associated with this key.
     *
     * @return A string for the cert or and empty string if there is no cert.
     *
     * @throws BusException
     * error code.
     */
    public native String toJavaString() throws BusException;

    /**
     * Determine if this certificate issued a given certificate by comparing the distinguished
     * name and verifying the digital signature.
     * @param issuedCertificate Certificate to check if it was issued by this certificate.
     * @return true if so.
     *
     * @throws BusException
     * error code.
     */
    public native boolean isIssuerOf(CertificateX509 issuedCertificate) throws BusException;

    /**
     * Is the subject DN of this certificate equal to a given DN?
     * @param cn Common Name component of the DN to compare to.
     * @param cnLength Length of the cn array. Zero if null.
     * @param ou Organizational Unit component of the DN to compare to.
     * @param ouLength Length of the ou array. Zero if null.
     * @return true if so.
     *
     * @throws BusException
     * error code.
     */
    private native boolean isDNEqual(byte[] cn, long cnLength, byte[] ou, long ouLength) throws BusException;

    /**
     * Is the subject DN of this certificate equal to a given DN?
     * @param cn Common Name component of the DN to compare to.
     * @param ou Organizational Unit component of the DN to compare to.
     * @return true if so.
     *
     * @throws BusException
     * error code.
     */
    public boolean isDNEqual(byte[] cn, byte[] ou) throws BusException {
        return isDNEqual(cn, cn.length, ou, ou.length);
    }

    /**
     * Is the subject DN of this certificate equal to a given certificate's DN?
     * @param other CertificateX509 to compare to.
     * @return true if so.
     *
     * @throws BusException
     * error code.
     */
    public native boolean isDNEqual(CertificateX509 other) throws BusException;

    /**
     * Is the subject public key of this certificate equal to a given key?
     * @param publicKey Public key to compare to.
     * @return true if so.
     *
     * @throws BusException
     * error code.
     */
    public native boolean isSubjectPublicKeyEqual(ECCPublicKey publicKey) throws BusException;

    /**
     * Get the SHA-256 thumbprint of this certificate.
     * @return thumbprint buffer of size Crypto_SHA256::DIGEST_SIZE to receive the thumbprint
     *
     * @throws BusException
     * error code.
     */
    public native byte[] getSHA256Thumbprint() throws BusException;

    /**
     * Retrieve the number of X.509 certificates in a PEM string representing a cert chain.
     * @param encoded the input string holding the PEM string
     * @param count the expected number of certs
     *
     * @throws BusException
     * error code.
     */
    private native static CertificateX509[] decodeCertChainPEM(String encoded, long encodedLen, long count) throws BusException;

    /**
     * Retrieve the number of X.509 certificates in a PEM string representing a cert chain.
     * @param encoded the input string holding the PEM string
     * @param count the expected number of certs
     *
     * @throws BusException
     * error code.
     */
    public static CertificateX509[] decodeCertChainPEM(String encoded, long count) throws BusException {
        return decodeCertChainPEM(encoded, encoded.length(), count);
    }

    /**
     * Validate the certificate type of each cert in the certificate chain.
     * The end-entity cert must have a type.
     * Any signing cert in the chain must have the same type or unrestricted
     * type in order to sign the next cert in the chain.
     * @param certChain the array of certs.
     * @param count the number of certs
     * @return true if valid; false, otherwise;
     *
     * @throws BusException
     * error code.
     */
    private native static boolean validateCertificateTypeInCertChain(CertificateX509[] certChain, long count) throws BusException;

    /**
     * Validate the certificate type of each cert in the certificate chain.
     * The end-entity cert must have a type.
     * Any signing cert in the chain must have the same type or unrestricted
     * type in order to sign the next cert in the chain.
     * @param certChain the array of certs.
     * @return true if valid; false, otherwise;
     *
     * @throws BusException
     * error code.
     */
    public static boolean validateCertificateTypeInCertChain(CertificateX509[] certChain) throws BusException {
        return validateCertificateTypeInCertChain(certChain, certChain.length);
    }

    /** The native connection handle. */
    private long handle;
}