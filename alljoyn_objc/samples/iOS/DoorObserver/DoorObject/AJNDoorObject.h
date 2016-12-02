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
//  AJNDoorObject.h
//
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNBusAttachment.h"
#import "AJNBusInterface.h"
#import "AJNProxyBusObject.h"


////////////////////////////////////////////////////////////////////////////////
//
// DoorObjectDelegate Bus Interface
//
////////////////////////////////////////////////////////////////////////////////

@protocol DoorObjectDelegate <AJNBusInterface>


// properties
//
@property (nonatomic, readonly) BOOL IsOpen;
@property (nonatomic, readonly) NSString* Location;
@property (nonatomic, readonly) NSNumber* KeyCode;

// methods
//
- (void)open:(AJNMessage *)methodCallMessage;
- (void)close:(AJNMessage *)methodCallMessage;
- (void)knockAndRun:(AJNMessage *)methodCallMessage;

// signals
//
- (void)sendPersonPassedThroughName:(NSString*)name inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath;


@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
// DoorObjectDelegate Signal Handler Protocol
//
////////////////////////////////////////////////////////////////////////////////

@protocol DoorObjectDelegateSignalHandler <AJNSignalHandler>

// signals
//
- (void)didReceivePersonPassedThroughName:(NSString*)name inSession:(AJNSessionId)sessionId message:(AJNMessage *)signalMessage;


@end

@interface AJNBusAttachment(DoorObjectDelegate)

- (void)registerDoorObjectDelegateSignalHandler:(id<DoorObjectDelegateSignalHandler>)signalHandler;

@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//  AJNDoorObject Bus Object superclass
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNDoorObject : AJNBusObject<DoorObjectDelegate>

// properties
//
@property (nonatomic) BOOL IsOpen;
@property (nonatomic) NSString* Location;
@property (nonatomic) NSNumber* KeyCode;


// methods
//
- (void)open:(AJNMessage *)methodCallMessage;
- (void)close:(AJNMessage *)methodCallMessage;
- (void)knockAndRun:(AJNMessage *)methodCallMessage;


// signals
//
- (void)sendPersonPassedThroughName:(NSString*)name inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath;


@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//  DoorObject Proxy
//
////////////////////////////////////////////////////////////////////////////////

@interface DoorObjectProxy : AJNProxyBusObject

// properties
//
@property (nonatomic, readonly) BOOL IsOpen;
@property (nonatomic, readonly) NSString* Location;
@property (nonatomic, readonly) NSNumber* KeyCode;


// methods
//
- (void)open;
- (void)close;
- (void)knockAndRun;

@end

////////////////////////////////////////////////////////////////////////////////