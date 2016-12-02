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
//  AJNClientDelegate protocol
//

@protocol AJNClientDelegate <AJNBusControllerDelegate>

- (AJNProxyBusObject *)proxyObjectOnBus:(AJNBusAttachment *)bus inSession:(AJNSessionId)sessionId;
- (void)shouldUnloadProxyObjectOnBus:(AJNBusAttachment *)bus;

@optional

- (void)didJoinInSession:(AJNSessionId)sessionId withService:(NSString *)serviceName;

@end
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  AJNClientController - abstracts and hides most of the boilerplate needed to
//  interact
//

@interface AJNClientController : NSObject<AJNBusController>

@property (nonatomic, weak) id<AJNClientDelegate> delegate;

@end
////////////////////////////////////////////////////////////////////////////////