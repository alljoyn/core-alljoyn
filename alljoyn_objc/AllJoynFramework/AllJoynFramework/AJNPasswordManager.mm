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

#import <objc/runtime.h>
#import <qcc/String.h>
#import <alljoyn/PasswordManager.h>
#import "AJNPasswordManager.h"

using namespace ajn;

@implementation AJNPasswordManager

+(QStatus)setCredentialsForAuthMechanism:(NSString *)authMechanism usingPassword:(NSString *)password
{
    QStatus status = PasswordManager::SetCredentials([authMechanism UTF8String], [password UTF8String]);
    if (status != ER_OK) {
        NSLog(@"ERROR: AJNPasswordManager::setCredentialsForAuthMechanism:%@ usingPassword:%@ failed. %@",authMechanism, password, [AJNStatus descriptionForStatusCode:status]);
    }
    return status;
}

@end