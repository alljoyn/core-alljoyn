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
