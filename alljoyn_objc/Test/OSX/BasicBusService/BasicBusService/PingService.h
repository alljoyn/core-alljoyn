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
#import "AJNSessionOptions.h"

////////////////////////////////////////////////////////////////////////////////
//
// A simple delegate for the PingService
//
@protocol PingServiceDelegate <NSObject>

// transport mask to use for network communication
//
@property (nonatomic, readonly) AJNTransportMask transportType;

// The delegate is called once a client joins a session with the service
//
- (void)client:(NSString *)clientName didJoinSession:(AJNSessionId)sessionId;

// The delegate is called when a client leaves a session
//
- (void)client:(NSString *)clientName didLeaveSession:(AJNSessionId)sessionId;

// Send updates on internal state of Ping Service
//
- (void)receivedStatusMessage:(NSString*)message;

@end
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// A simple AllJoyn service that exposes a ping object
//
@interface PingService : NSObject

@property (nonatomic, strong) id<PingServiceDelegate> delegate;

- (id)initWithDelegate:(id<PingServiceDelegate>)delegate;

- (void)start:(NSString *)serviceName;
- (void)stop;

+ (PingService *)sharedInstance;

@end
////////////////////////////////////////////////////////////////////////////////