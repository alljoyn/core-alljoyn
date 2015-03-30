////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
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

#import "TestAuthenticationListener.h"

@implementation TestAuthenticationListener

@synthesize bus = _bus;
@synthesize userName = _userName;
@synthesize maximumAuthentications = _maximumAuthentications;
@synthesize keyExpiration = _keyExpiration;

- (id)initOnBus:(AJNBusAttachment*)bus withUserName:(NSString*)userName maximumAuthenticationsAllowed:(NSUInteger)maximumAuthentications
{
    self = [super init];
    if (self) {
        self.bus = bus;
        self.userName = userName;
        self.maximumAuthentications = maximumAuthentications;
    }
    return self;
}

- (AJNSecurityCredentials*)requestSecurityCredentialsWithAuthenticationMechanism:(NSString*)authenticationMechanism peerName:(NSString*)peerName authenticationCount:(uint16_t)authenticationCount userName:(NSString*)userName credentialTypeMask:(AJNSecurityCredentialType)mask
{
    AJNSecurityCredentials *securityCredentials = [[AJNSecurityCredentials alloc] init];
    
    if (authenticationCount > self.maximumAuthentications) {
        return nil;
    }
    
    NSLog(@"RequestCredentials for authenticating %@ using mechanism %@", peerName, authenticationMechanism);
    
    NSString *guid = [self.bus guidForPeerNamed:peerName];
    NSLog(@"Peer guid %@", guid);
    
    if (self.keyExpiration != 0xFFFFFFFF) {
        [securityCredentials setExpirationTime:self.keyExpiration];
    }
    
    if ([authenticationMechanism compare:@"ALLJOYN_SRP_KEYX"] == NSOrderedSame) {
        if (mask & kAJNSecurityCredentialTypePassword) {
            if (authenticationCount == 3) {
                [securityCredentials setPassword:@"123456"];
            } else {
                [securityCredentials setPassword:@"xxxxxx"];
            }
            NSLog(@"AuthListener returning fixed pin \"%@\" for %@\n", securityCredentials.password, authenticationMechanism);
        }
        return securityCredentials;
    }
    
    if ([authenticationMechanism compare:@"ALLJOYN_SRP_LOGON"] == NSOrderedSame) {
        if (mask & kAJNSecurityCredentialTypeUserName) {
            if (authenticationCount == 1) {
                [securityCredentials setUserName:@"Code Monkey"];
            } else {
                [securityCredentials setUserName:userName];
            }
        }
        if (mask & kAJNSecurityCredentialTypePassword) {
            [securityCredentials setPassword:@"123banana321"];
        }
        return securityCredentials;
    }
    
    return nil;    
}

- (void)authenticationUsing:(NSString*)authenticationMechanism forRemotePeer:(NSString*)peer didCompleteWithStatus:(BOOL)success
{
    NSLog(@"Authentication %@ for peer %@ %@\n", authenticationMechanism, peer, success ? @" was succesful" : @" failed");    
}

- (BOOL)verifySecurityCredentials:(AJNSecurityCredentials*)credentials usingAuthenticationMechanism:(NSString*)authenticationMechanism forRemotePeer:(NSString*)peerName
{
    // Not used with the SRP authentication mechanism
    return false;        
}

- (void)securityViolationOccurredWithErrorCode:(QStatus)errorCode forMessage:(AJNMessage*)message
{
    NSLog(@"Security violation occurred. %@\n", [AJNStatus descriptionForStatusCode:errorCode]);
}

@end
