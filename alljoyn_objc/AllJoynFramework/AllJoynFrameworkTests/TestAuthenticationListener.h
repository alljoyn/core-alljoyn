////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
