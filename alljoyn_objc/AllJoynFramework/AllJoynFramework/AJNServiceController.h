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

#import "AJNBusController.h"

////////////////////////////////////////////////////////////////////////////////
//
// Service Delegate Protocol
//

@protocol AJNServiceDelegate <AJNBusControllerDelegate>

@property (nonatomic, readonly) AJNBusObject *object;

- (AJNBusObject *)objectOnBus:(AJNBusAttachment *)bus;
- (void)shouldUnloadObjectOnBus:(AJNBusAttachment *)bus;

@optional

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options;
- (void)didJoin:(NSString *)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort;

@end
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Service Controller
//

@interface AJNServiceController : NSObject<AJNBusController, AJNSessionPortListener>

@property (nonatomic, weak) id<AJNServiceDelegate> delegate;

@end
////////////////////////////////////////////////////////////////////////////////