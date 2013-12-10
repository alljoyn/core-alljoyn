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

#import <alljoyn/AuthListener.h>
#import "AJNSecurityCredentials.h"

const AJNSecurityCredentialType kAJNSecurityCredentialTypePassword     = 0x0001; /**< Bit 0 indicates credentials include a password, pincode, or passphrase */
const AJNSecurityCredentialType kAJNSecurityCredentialTypeUserName    = 0x0002; /**< Bit 1 indicates credentials include a user name */
const AJNSecurityCredentialType kAJNSecurityCredentialTypeCertificateChain   = 0x0004; /**< Bit 2 indicates credentials include a chain of PEM-encoded X509 certificates */
const AJNSecurityCredentialType kAJNSecurityCredentialTypePrivateKey  = 0x0008; /**< Bit 3 indicates credentials include a PEM-encoded private key */
const AJNSecurityCredentialType kAJNSecurityCredentialTypeLogonEntry  = 0x0010; /**< Bit 4 indicates credentials include a logon entry that can be used to logon a remote user */
const AJNSecurityCredentialType kAJNSecurityCredentialTypeExpirationTime   = 0x0020; /**< Bit 5 indicates credentials include an  expiration time */

const uint16_t AJNSecurityCredentialRequestNewPassword = 0x1001; /**< Indicates the credential request is for a newly created password */
const uint16_t AJNSecurityCredentialRequestOneTimePassword = 0x2001; /**< Indicates the credential request is for a one time use password */

@interface AJNObject(Private)

@property (nonatomic) BOOL shouldDeleteHandleOnDealloc;

@end


@implementation AJNSecurityCredentials

- (id)init
{
    self = [super init];
    if (self) {
        self.handle = new ajn::AuthListener::Credentials();
        self.shouldDeleteHandleOnDealloc = YES;
    }
    return self;
}

- (void)dealloc
{
    if (self.shouldDeleteHandleOnDealloc) {
        ajn::AuthListener::Credentials *credentials = static_cast<ajn::AuthListener::Credentials*>(self.handle);
        delete credentials;
        self.handle = nil;
    }
}

/**
 * Helper to return the C++ API object that is encapsulated by this objective-c class
 */
- (ajn::AuthListener::Credentials*)credentials
{
    return static_cast<ajn::AuthListener::Credentials*>(self.handle);
}

- (NSString*)password
{
    return [NSString stringWithCString:self.credentials->GetPassword().c_str() encoding:NSUTF8StringEncoding];
}

- (void)setPassword:(NSString *)password
{
    self.credentials->SetPassword([password UTF8String]);
}

- (NSString*)userName
{
    return [NSString stringWithCString:self.credentials->GetUserName().c_str() encoding:NSUTF8StringEncoding];
}

- (void)setUserName:(NSString *)userName
{
    self.credentials->SetUserName([userName UTF8String]);
}

- (NSString*)logonEntry
{
    return [NSString stringWithCString:self.credentials->GetLogonEntry().c_str() encoding:NSUTF8StringEncoding];
}

- (void)setLogonEntry:(NSString *)logonEntry
{
    self.credentials->SetLogonEntry([logonEntry UTF8String]);
}

- (NSString*)privateKey
{
    return [NSString stringWithCString:self.credentials->GetPrivateKey().c_str() encoding:NSUTF8StringEncoding];
}

- (void)setPrivateKey:(NSString *)privateKey
{
    self.credentials->SetPrivateKey([privateKey UTF8String]);
}

- (NSString*)certificateChain
{
    return [NSString stringWithCString:self.credentials->GetCertChain().c_str() encoding:NSUTF8StringEncoding];
}

- (void)setCertificateChain:(NSString *)certificateChain
{
    self.credentials->SetCertChain([certificateChain UTF8String]);
}

- (uint32_t)expirationTime
{
    return self.credentials->GetExpiration();
}

- (void)setExpirationTime:(uint32_t)expirationTime
{
    self.credentials->SetExpiration(expirationTime);
}

- (BOOL)isCredentialTypeSet:(AJNSecurityCredentialType)type
{
    return self.credentials->IsSet(type) == true;
}

- (void)clear
{
    self.credentials->Clear();
}

@end
