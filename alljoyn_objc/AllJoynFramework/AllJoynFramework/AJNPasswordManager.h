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
#import "AJNStatus.h"
#import "AJNObject.h"

@interface AJNPasswordManager : AJNObject

/**
 * Set credentials used for the authentication of thin clients.
 *
 * @param  authMechanism    The name of the authentication mechanism issuing the request.
 * @param  password         The password
 * @return  - ER_OK if the credentials was successfully set
 *
 */
+(QStatus)setCredentialsForAuthMechanism:(NSString *)authMechanism usingPassword:(NSString *)password;

@end