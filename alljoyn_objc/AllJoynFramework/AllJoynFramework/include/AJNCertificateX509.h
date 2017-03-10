////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
//
//    SPDX-License-Identifier: Apache-2.0
//
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNObject.h"
#import "AJNKeyInfoECC.h"
#import "AJNCryptoECC.h"

/**
 * The validity period
 */
typedef struct AJNValidPeriod {
    uint64_t validFrom; /**< the date time when the cert becomes valid
                         expressed in the number of seconds in EPOCH Jan 1, 1970 */
    uint64_t validTo;  /**< the date time after which the cert becomes invalid
                        expressed in the number of seconds in EPOCH Jan 1, 1970 */
} AJNValidPeriod;

/**
 * encoding format
 */
typedef enum {
    ENCODING_X509_DER = 0,     ///< X.509 DER format
    ENCODING_X509_DER_PEM = 1  ///< X.509 DER PEM format
} AJNEncodingType;

/**
 * Certificate type
 */
typedef enum {
    UNRESTRICTED_CERTIFICATE,  ///< Unrestricted certificate
    IDENTITY_CERTIFICATE,      ///< identity certificate
    MEMBERSHIP_CERTIFICATE,    ///< membership certificate
    INVALID_CERTIFICATE        ///< certificate not valid for any AllJoyn purpose
} AJNCertificateType;


@interface AJNCertificateX509 : AJNObject

/**
 * Get the serial number
 * @return the serial number
 */
@property (nonatomic, readonly) NSMutableData *serial;

/**
 * Get the length of the serial number
 * @return Length of the serial number
 */
@property (nonatomic, readonly) size_t serialLength;

/**
 * Get the length of the issuer organization unit field
 * @return the length of the issuer organization unit field
 */
@property (nonatomic, readonly) size_t issuerOULength;

/**
 * Get the issuer organization unit field
 * @return the issuer organization unit field
 */
@property (nonatomic, readonly) NSMutableData *issuerOU;

/**
 * Get the length of the issuer common name field
 * @return the length of the issuer common name field
 */
@property (nonatomic, readonly) size_t issuerCNLength;

/**
 * Get the issuer common name field
 * @return the issuer common name field
 */
@property (nonatomic, readonly) NSMutableData *issuerCN;

/**
 * Get the length of the subject organization unit field
 * @return the length of the subject organization unit field
 */
@property (nonatomic, readonly) size_t subjectOULength;

/**
 * Get the subject organization unit field
 * @return the subject organization unit field
 */
@property (nonatomic, readonly) NSMutableData *subjectOU;

/**
 * Get the length of the subject common name field
 * @return the length of the subject common name field
 */
@property (nonatomic, readonly) size_t subjectCNLength;

/**
 * Get the subject common name field
 * @return the subject common name field
 */
@property (nonatomic, readonly) NSMutableData *subjectCN;

/**
 * Get the subject alt name field
 * @return the subject alt name
 */
@property (nonatomic, readonly) NSString *subjectAltName;

/**
 * Get the Authority Key Identifier
 * @return the Authority Key Identifier
 */
@property (nonatomic, readonly) NSString *authorityKeyId;

/**
 * Get the validity period.
 * @return the validity period
 */
@property (nonatomic, readonly) AJNValidPeriod *validity;

/**
 * Get the subject public key
 * @return the subject public key
 */
@property (nonatomic, readonly) AJNECCPublicKey *subjectPublicKey;

/**
 * Can the subject act as a certificate authority?
 * @return true if so.
 */
@property (nonatomic, readonly) BOOL isCA;

/**
 * Get the digest of the external data.
 * @return The digest of the external data
 */
@property (nonatomic, readonly) NSMutableData *digest;

/**
 * Get the size of the digest of the external data.
 * @return the size of the digest of the external data
 */

@property (nonatomic, readonly) size_t digestSize;

/**
 * Is the optional digest field present in the certificate?
 * @return whether this optional field is present in the certificate.
 */
@property (nonatomic, readonly) BOOL isDigestPresent;

/**
 * Get the PEM encoded bytes for the certificate
 * @return the PEM encoded bytes
 */
@property (nonatomic, readonly) NSString *pem;

/**
 * Get the certificate type.
 * @return Certificate type
 */
@property (nonatomic, readonly) AJNCertificateType type;

/**
 * Constructor
 */
- (id)init;

/**
 * Constructor
 * @param type the certificate type.
 */
- (id)initWithCertificateType:(AJNCertificateType)type;

/**
 * Decode a PEM encoded certificate.
 * @param pem the encoded certificate.
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)decodeCertificatePEM:(NSString *)pem;

/**
 * Export the certificate as PEM encoded.
 * @param[out] pem the encoded certificate.
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)encodeCertificatePEM:(NSString **)pem;

/**
 * Helper function to generate PEM encoded string using a DER encoded string.
 * @param der the encoded certificate.
 * @param[out] pem the encoded certificate.
 * @return ER_OK for success; otherwise, error code.
 */
+ (QStatus)encodeCertificatePEM:(NSString *)der pem:(NSString **)pem;

/**
 * Decode a DER encoded certificate.
 * @param der the encoded certificate.
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)decodeCertificateDER:(NSString *)der;

/**
 * Export the certificate as DER encoded.
 * @param[out] der the encoded certificate.
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)encodeCertificateDER:(NSString **)der;

/**
 * Export only the TBS section of the certificate as DER encoded.
 * This is suitable for using to generate a signature outside of this class.
 *
 * @param[out] tbsder The binary DER-encoded TBS portion of the certificate.
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)encodeCertificateTBS:(NSString **)tbsder;

/**
 * Encode the private key in a PEM string.
 * @param privateKey the private key to encode
 * @param[out] encoded the output string holding the resulting PEM string
 * @return ER_OK for success; otherwise, error code.
 */
+ (QStatus)encodePrivateKeyPEM:(AJNECCPrivateKey *)privateKey encoded:(NSString **)encoded;

/**
 * Decode the private key from a PEM string.
 * @param encoded the input string holding the PEM string
 * @param[out] privateKey the output private key
 * @return ER_OK for success; otherwise, error code.
 */
+ (QStatus)decodePrivateKeyPEM:(NSString *)encoded privateKey:(AJNECCPrivateKey *)privateKey;

/**
 * Encode the public key in a PEM string.
 * @param publicKey the public key to encode
 * @param[out] encoded the output string holding the resulting PEM string
 * @return ER_OK for success; otherwise, error code.
 */
+ (QStatus)encodePublicKeyPEM:(AJNECCPublicKey *)publicKey encoded:(NSString **)encoded;

/**
 * Decode the public key from a PEM string.
 * @param encoded the input string holding the PEM string
 * @param[out] publicKey the output public key
 * @return ER_OK for success; otherwise, error code.
 */
+ (QStatus)decodePublicKeyPEM:(NSString *)encoded publicKey:(AJNECCPublicKey *)publicKey;

/**
 * Sign the certificate.
 * @param key the ECDSA private key.
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)sign:(AJNECCPrivateKey*)key;

/**
 * Set the signature to a provided byte array, when signing the certificate externally.
 * This method does not verify the signature is valid, please use Verify with the
 * corresponding public key to make sure.
 *
 * @see qcc::CertificateX509::Verify(const ECCPublicKey* key);
 *
 * @param[in] signature An ECCSignature containing the signature.
 */
- (void)setSignature:(AJNECCSignature *)signature;

/**
 * Sign the certificate and generate the authority key identifier.
 * @param privateKey the ECDSA private key.
 * @param publicKey the ECDSA public key to generate the authority key
 *                  identifier.
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)signAndGenerateAuthorityKeyId:(AJNECCPrivateKey *)privateKey publicKey:(AJNECCPublicKey *)publicKey;

/**
 * Verify a self-signed certificate.
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)verifySelfSigned;

/**
 * Verify the certificate.
 * @param key the ECDSA public key.
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)verifyWithPublicKey:(AJNECCPublicKey *)key;

/**
 * Verify the certificate against the trust anchor.
 * @param trustAnchor the trust anchor
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)verifyWithTrustAnchor:(AJNKeyInfoNISTP256 *)trustAnchor;

/**
 * Verify the validity period of the certificate.
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)verifyValidity;

/**
 * Set the serial number field
 * @param serialNumber The serial number
 */
- (void)setSerial:(NSMutableData *)serialNumber;

/**
 * Set the serial number to be a random 20-byte string. Callers using this
 * functionality in a Certificate Authority are responsible for keeping track of
 * used serial numbers from previous certificate issuances, checking the serial
 * number after a successful call to this method, and generating new ones until
 * an unused serial number is generated. Repeated failure to generate an unused
 * serial number may suggest a problem with the platform randomness generator.
 *
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)generateRandomSerial;

/**
 * Set the issuer organization unit field
 * @param ou the organization unit
  */
- (void)setIssuerOU:(NSMutableData *)ou;

/**
 * Set the issuer common name field
 * @param cn the common name
 */
- (void)setIssuerCN:(NSMutableData *)cn;

/**
 * Set the subject organization unit field
 * @param ou the organization unit
 */
- (void)setSubjectOU:(NSMutableData *)ou;

/**
 * Set the subject common name field
 * @param cn the common name
 */
- (void)setSubjectCN:(NSMutableData *)cn;

/**
 * Set the subject alt name field
 * @param subjectAltName the subject alt name
 */
- (void)setSubjectAltName:(NSString *)subjectAltName;

/**
 * Generate the authority key identifier.
 * @param issuerPubKey the issuer's public key
 * @param[out] authorityKeyId the authority key identifier
 * @return ER_OK for success; otherwise, error code.
 */
+ (QStatus)generateAuthorityKeyId:(AJNECCPublicKey *)issuerPubKey authorityKeyId:(NSString **)authorityKeyId;

/**
 * Generate the issuer authority key identifier for the certificate.
 * @param issuerPubKey the issuer's public key
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)generateAuthorityKeyId:(AJNECCPublicKey *)issuerPubKey;

/**
 * Updates the current AuthorityKeyId with a new one.
 *
 * @param[in] newAki the new AuthorityKeyId for this certificate.
 */
- (void)setAuthorityKeyId:(NSString *)newAki;

/**
 * Set the validity field
 * @param validPeriod the validity period
 */
- (void)setValidity:(AJNValidPeriod *)validPeriod;

/**
 * Set the subject public key field
 * @param key the subject public key
 */
- (void)setSubjectPublicKey:(AJNECCPublicKey *)key;

/**
 * Indicate that the subject may act as a certificate authority.
 * @param flag flag indicating the subject may act as a CA
 */
- (void)setCA:(BOOL)flag;

/**
 * Set the digest of the external data.
 * @param digest the digest of the external data
 */
- (void)setDigest:(NSMutableData *)digest;

/**
 * Load the PEM encoded bytes for the certificate
 * @param pem the encoded bytes
 * @return ER_OK for success; otherwise, error code.
 */
- (QStatus)loadPEM:(NSString *)pem;

/**
 * Returns a human readable string for a cert if there is one associated with this key.
 *
 * @return A string for the cert or and empty string if there is no cert.
 */
- (NSString *)description;

/**
 * Determine if this certificate issued a given certificate by comparing the distinguished
 * name and verifying the digital signature.
 * @param issuedCertificate Certificate to check if it was issued by this certificate.
 * @return true if so.
 */
- (BOOL)isIssuerOf:(AJNCertificateX509 *)issuedCertificate;

/**
 * Is the subject DN of this certificate equal to a given DN?
 * @param cn Common Name component of the DN to compare to.
 * @param ou Organizational Unit component of the DN to compare to.
 * @return true if so.
 */
- (BOOL)isDNEqual:(NSMutableData *)cn ou:(NSMutableData *)ou;

/**
 * Is the subject DN of this certificate equal to a given certificate's DN?
 * @param other CertificateX509 to compare to.
 * @return true if so.
 */
- (BOOL)isDNEqual:(AJNCertificateX509 *)other;

/**
 * Is the subject public key of this certificate equal to a given key?
 * @param publicKey Public key to compare to.
 * @return true if so.
 */
- (BOOL)isSubjectPublicKeyEqual:(AJNECCPublicKey *)publicKey;

/**
 * Get the SHA-256 thumbprint of this certificate.
 * @param[out] thumbprint buffer of size Crypto_SHA256::DIGEST_SIZE to receive the thumbprint
 *
 * @return ER_OK if successful, error code otherwise
 */
- (QStatus)getSHA256Thumbprint:(NSMutableData *)thumbprint;

/**
 * Retrieve the number of X.509 certificates in a PEM string representing a cert chain.
 * @param encoded the input string holding the PEM string
 * @param[in,out] certChain the input string holding the array of certs.
 * @return ER_OK for success; otherwise, error code.
 */
+ (QStatus)decodeCertChainPEM:(NSString *)encoded certChain:(NSMutableArray *)certChain;

/**
 * Validate the certificate type of each cert in the certificate chain.
 * The end-entity cert must have a type.
 * Any signing cert in the chain must have the same type or unrestricted
 * type in order to sign the next cert in the chain.
 * @param[in] certChain the array of certs.
 * @return true if valid; false, otherwise;
 */
+ (BOOL)validateCertificateTypeInCertChain:(NSArray *)certChain;

/**
 * Set the guild GUID
 * @param guid the guild GUID
 */
- (void)setGuild:(NSMutableData *)guid;


@end
