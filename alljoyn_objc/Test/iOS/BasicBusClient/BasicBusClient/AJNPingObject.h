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
////////////////////////////////////////////////////////////////////////////////
//
//  ALLJOYN MODELING TOOL - GENERATED CODE
//
////////////////////////////////////////////////////////////////////////////////
//
//  DO NOT EDIT
//
//  Add a category or subclass in separate .h/.m files to extend these classes
//
////////////////////////////////////////////////////////////////////////////////
//
//  AJNPingObject.h
//
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNBusAttachment.h"
#import "AJNBusInterface.h"
#import "AJNProxyBusObject.h"


////////////////////////////////////////////////////////////////////////////////
//
// PingObjectDelegate Bus Interface
//
////////////////////////////////////////////////////////////////////////////////

@protocol PingObjectDelegate <AJNBusInterface>


// methods
//
- (NSString*)sendPingString:(NSString*)outStr withDelay:(NSNumber*)delay;
- (NSString*)sendPingString:(NSString*)outStr;
- (void)sendPingAtTimeInSeconds:(NSNumber*)sendTimeSecs timeInMilliseconds:(NSNumber*)sendTimeMillisecs receivedTimeInSeconds:(NSNumber**)receivedTimeSecs receivedTimeInMilliseconds:(NSNumber**)receivedTimeMillisecs;

// signals
//
- (void)sendmy_signalInSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath;


@end

////////////////////////////////////////////////////////////////////////////////

    
////////////////////////////////////////////////////////////////////////////////
//
// PingObjectDelegate Signal Handler Protocol
//
////////////////////////////////////////////////////////////////////////////////

@protocol PingObjectDelegateSignalHandler <AJNSignalHandler>

// signals
//
- (void)didReceivemy_signalInSession:(AJNSessionId)sessionId fromSender:(NSString*)sender;


@end

@interface AJNBusAttachment(PingObjectDelegate)

- (void)registerPingObjectDelegateSignalHandler:(id<PingObjectDelegateSignalHandler>)signalHandler;

@end

////////////////////////////////////////////////////////////////////////////////
    

////////////////////////////////////////////////////////////////////////////////
//
// PingObjectValuesDelegate Bus Interface
//
////////////////////////////////////////////////////////////////////////////////

@protocol PingObjectValuesDelegate <AJNBusInterface>


// properties
//
@property (nonatomic, strong) NSNumber* int_val;
@property (nonatomic, readonly) NSString* ro_str;
@property (nonatomic, strong) NSString* str_val;


@end

////////////////////////////////////////////////////////////////////////////////

    

////////////////////////////////////////////////////////////////////////////////
//
//  AJNPingObject Bus Object superclass
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNPingObject : AJNBusObject<PingObjectDelegate, PingObjectValuesDelegate>

// properties
//
@property (nonatomic, strong) NSNumber* int_val;
@property (nonatomic, readonly) NSString* ro_str;
@property (nonatomic, strong) NSString* str_val;


// methods
//
- (NSString*)sendPingString:(NSString*)outStr withDelay:(NSNumber*)delay;
- (NSString*)sendPingString:(NSString*)outStr;
- (void)sendPingAtTimeInSeconds:(NSNumber*)sendTimeSecs timeInMilliseconds:(NSNumber*)sendTimeMillisecs receivedTimeInSeconds:(NSNumber**)receivedTimeSecs receivedTimeInMilliseconds:(NSNumber**)receivedTimeMillisecs;


// signals
//
- (void)sendmy_signalInSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath;


@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//  PingObject Proxy
//
////////////////////////////////////////////////////////////////////////////////

@interface PingObjectProxy : AJNProxyBusObject

// properties
//
@property (nonatomic, strong) NSNumber* int_val;
@property (nonatomic, readonly) NSString* ro_str;
@property (nonatomic, strong) NSString* str_val;


// methods
//
- (NSString*)sendPingString:(NSString*)outStr withDelay:(NSNumber*)delay;
- (NSString*)sendPingString:(NSString*)outStr;
- (void)sendPingAtTimeInSeconds:(NSNumber*)sendTimeSecs timeInMilliseconds:(NSNumber*)sendTimeMillisecs receivedTimeInSeconds:(NSNumber**)receivedTimeSecs receivedTimeInMilliseconds:(NSNumber**)receivedTimeMillisecs;


@end

////////////////////////////////////////////////////////////////////////////////