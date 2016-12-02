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
#import <alljoyn/Status.h>
#import "AJNKeyInfoECC.h"
#import "AJNPermissionConfigurator.h"

/**
 * Listener used to handle the security State signal.
 */
@protocol AJNApplicationStateListener <NSObject>

@required

/**
 * Handler for the org.allseen.Bus.Application's State sessionless signal.
 *
 * @param[in] busName          unique name of the remote BusAttachment that
 *                             sent the State signal
 * @param[in] publicKeyInfo the application public key
 * @param[in] state the application state
 */
- (void)state:(NSString*)busName publicKeyInfo:(AJNKeyInfoNISTP256*)publicKeyInfo state:(AJNApplicationState)state;

@end