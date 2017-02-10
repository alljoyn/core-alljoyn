//
//  DefaultECDHEListenerDelegate.m
//  DoorProvider
//
//  Created by Eugene Komarov on 31/01/2017.
//  Copyright © 2017 AllseenAlliance. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "DefaultECDHEListener.h"
#import <alljoyn/AuthListener.h>

@interface DefaultECDHEListener ()

@property (nonatomic) ajn::DefaultECDHEAuthListener* handle;

@end

@implementation DefaultECDHEListener : NSObject

- (id)init
{
    self = [super init];
    if (self != nil) {
        self.handle = new ajn::DefaultECDHEAuthListener();
    }
    return self;
}

- (QStatus)setPSK:(const uint8_t *)secret secretSize:(size_t)secretSize
{
    return self.handle->SetPSK(secret, secretSize);
}

- (QStatus)setPassword:(const uint8_t*)password passwordSize:(size_t)passwordSize
{
    return self.handle->SetPassword(password, passwordSize);
}

- (AJNSecurityCredentials *)requestSecurityCredentialsWithAuthenticationMechanism:(NSString *)authenticationMechanism peerName:(NSString *)peerName authenticationCount:(uint16_t)authenticationCount userName:(NSString *)userName credentialTypeMask:(AJNSecurityCredentialType)mask;

{
    ajn::DefaultECDHEAuthListener::Credentials* creds = new ajn::DefaultECDHEAuthListener::Credentials();
    if (self.handle->RequestCredentials([authenticationMechanism UTF8String], [peerName UTF8String], authenticationCount, [userName UTF8String], mask, *creds)) {
        return [[AJNSecurityCredentials alloc] initWithHandle:creds];
    }

    delete creds;

    return nil;
}

- (void)authenticationUsing:(NSString *)authenticationMechanism forRemotePeer:(NSString *)peerName didCompleteWithStatus:(BOOL)success
{
    return self.handle->AuthenticationComplete([authenticationMechanism UTF8String], [peerName UTF8String], success);
}

@end
