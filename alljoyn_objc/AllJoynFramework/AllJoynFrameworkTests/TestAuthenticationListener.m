////////////////////////////////////////////////////////////////////////////////
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

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