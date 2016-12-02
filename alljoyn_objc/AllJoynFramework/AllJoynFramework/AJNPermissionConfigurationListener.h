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

/**
 * Abstract base class implemented by AllJoyn users and called by AllJoyn to
 * inform users of security permission related events.
 */
@protocol AJNPermissionConfigurationListener <NSObject>

/**
 * Handler for doing a factory reset of application state.
 *
 * @return  Return ER_OK if the application state reset was successful.
 */
- (QStatus)factoryReset;

/**
 * Notification that the security manager has updated the security policy
 * for the application.
 */
- (void)didPolicyChange;

/**
 * Notification provided before Security Manager is starting to change settings for this application.
 */
- (void)startManagement;

/**
 * Notification provided after Security Manager finished changing settings for this application.
 */
- (void)endManagement;

@end