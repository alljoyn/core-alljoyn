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

#import <Foundation/Foundation.h>
#import "AJNAuthenticationListener.h"
#import "AJNBusAttachment.h"

@interface TestAuthenticationListener : NSObject<AJNAuthenticationListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) NSString *userName;
@property (nonatomic) NSUInteger maximumAuthentications;
@property (nonatomic) uint32_t keyExpiration;

- (id)initOnBus:(AJNBusAttachment *)bus withUserName:(NSString *)userName maximumAuthenticationsAllowed:(NSUInteger)maximumAuthentications;

- (AJNSecurityCredentials *)requestSecurityCredentialsWithAuthenticationMechanism:(NSString *)authenticationMechanism peerName:(NSString *)peerName authenticationCount:(uint16_t) authenticationCount userName:(NSString *)userName credentialTypeMask:(AJNSecurityCredentialType)mask;

- (void)authenticationUsing:(NSString *)authenticationMechanism forRemotePeer:(NSString *)peer didCompleteWithStatus:(BOOL)success;

- (BOOL)verifySecurityCredentials:(AJNSecurityCredentials *)credentials usingAuthenticationMechanism:(NSString *)authenticationMechanism forRemotePeer:(NSString *)peerName;

- (void)securityViolationOccurredWithErrorCode:(QStatus)errorCode forMessage:(AJNMessage *)message;

@end