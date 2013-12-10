////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import "AJNObject.h"

/**
 * @name Credential indication Bitmasks
 *  Bitmasks used to indicated what type of credentials are being used.
 */
// @{
typedef uint16_t AJNSecurityCredentialType;

/**< Bit 0 indicates credentials include a password, pincode, or passphrase */
extern const AJNSecurityCredentialType kAJNSecurityCredentialTypePassword;

/**< Bit 1 indicates credentials include a user name */
extern const AJNSecurityCredentialType kAJNSecurityCredentialTypeUserName;

/**< Bit 2 indicates credentials include a chain of PEM-encoded X509 certificates */
extern const AJNSecurityCredentialType kAJNSecurityCredentialTypeCertificateChain;

/**< Bit 3 indicates credentials include a PEM-encoded private key */
extern const AJNSecurityCredentialType kAJNSecurityCredentialTypePrivateKey;

/**< Bit 4 indicates credentials include a logon entry that can be used to logon a remote user */
extern const AJNSecurityCredentialType kAJNSecurityCredentialTypeLogonEntry;

/**< Bit 5 indicates credentials include an expiration time */
extern const AJNSecurityCredentialType kAJNSecurityCredentialTypeExpirationTime;
// @}

////////////////////////////////////////////////////////////////////////////////

/**
 * @name Credential request values
 * These values are only used in a credential request
 */
// @{

typedef uint16_t AJNSecurityCredentialRequest;

/**< Indicates the credential request is for a newly created password */
extern const uint16_t AJNSecurityCredentialRequestNewPassword;

/**< Indicates the credential request is for a one time use password */
extern const uint16_t AJNSecurityCredentialRequestOneTimePassword;
// @}

////////////////////////////////////////////////////////////////////////////////

/**
 * Generic class for describing different authentication credentials.
 */
@interface AJNSecurityCredentials : AJNObject

/**
 * A requested password, pincode, or passphrase.
 */
@property (nonatomic) NSString *password;

/**
 * A requested user name.
 */
@property (nonatomic) NSString *userName;

/**
 * A requested public key certificate chain. The certificates must be PEM encoded.
 */
@property (nonatomic) NSString *certificateChain;

/**
 * A requested private key. The private key must be PEM encoded and may be encrypted. If
 * the private key is encrypted the passphrase required to decrypt it must also be supplied.
 */
@property (nonatomic) NSString *privateKey;

/**
 * A logon entry. For example for the Secure Remote Password protocol in RFC 5054, a
 * logon entry encodes the N,g, s and v parameters. An SRP logon entry string has the form
 * N:g:s:v where N,g,s, and v are ASCII encoded hexadecimal strings and are separated by
 * colons.
 */
@property (nonatomic) NSString *logonEntry;

/**
 * An expiration time in seconds relative to the current time for the credentials. This value is optional and
 * can be set on any response to a credentials request. After the specified expiration time has elapsed any secret
 * keys based on the provided credentials are invalidated and a new authentication exchange will be required. If an
 * expiration is not set the default expiration time for the requested authentication mechanism is used.
 */
@property (nonatomic) uint32_t expirationTime;

/**
 * Tests if one or more credentials are set.
 *
 * @param type  A logical or of the credential bit values.
 * @return true if the credentials are set.
 */
- (BOOL)isCredentialTypeSet:(AJNSecurityCredentialType)type;

/**
 * Clear the credentials.
 */
- (void)clear;

@end
