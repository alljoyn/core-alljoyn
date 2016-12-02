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

#import "AJNBusAttachment.h"

// A simple delegate for the PingClient
//
@protocol PingClientDelegate <NSObject>

@property (nonatomic, readonly) AJNTransportMask transportType;

// The delegate is called once a connection is established between the client
// and the service
//
- (void)didConnectWithService:(NSString *)serviceName;

// The delegate is called when a service session is lost
//
- (void)shouldDisconnectFromService:(NSString *)serviceName;

// Send updates on internal state of Ping Client
//
- (void)receivedStatusMessage:(NSString*)message;

@end

// A simple AllJoyn client
//
@interface PingClient : NSObject

@property (nonatomic, strong) id<PingClientDelegate> delegate;

- (id)initWithDelegate:(id<PingClientDelegate>)delegate;

- (void)connectToService:(NSString *)serviceName;
- (void)disconnect;

- (NSString *)sendPingToService:(NSString *)str1;

+ (PingClient *)sharedInstance;

@end