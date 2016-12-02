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
//  AJNPerformanceObject.h
//
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNBusAttachment.h"
#import "AJNBusInterface.h"
#import "AJNProxyBusObject.h"


////////////////////////////////////////////////////////////////////////////////
//
// PerformanceObjectDelegate Bus Interface
//
////////////////////////////////////////////////////////////////////////////////

@protocol PerformanceObjectDelegate <AJNBusInterface>


// methods
//
- (BOOL)checkPacketAtIndex:(NSNumber*)packetIndex payLoad:(AJNMessageArgument*)byteArray packetSize:(NSNumber*)packetSize message:(AJNMessage *)methodCallMessage;

// signals
//
- (void)sendPacketAtIndex:(NSNumber*)packetIndex payLoad:(AJNMessageArgument*)byteArray inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath flags:(AJNMessageFlag)flags;


@end

////////////////////////////////////////////////////////////////////////////////

    
////////////////////////////////////////////////////////////////////////////////
//
// PerformanceObjectDelegate Signal Handler Protocol
//
////////////////////////////////////////////////////////////////////////////////

@protocol PerformanceObjectDelegateSignalHandler <AJNSignalHandler>

// signals
//
- (void)didReceivePacketAtIndex:(NSNumber*)packetIndex payLoad:(AJNMessageArgument*)byteArray inSession:(AJNSessionId)sessionId message:(AJNMessage *)signalMessage;


@end

@interface AJNBusAttachment(PerformanceObjectDelegate)

- (void)registerPerformanceObjectDelegateSignalHandler:(id<PerformanceObjectDelegateSignalHandler>)signalHandler;

@end

////////////////////////////////////////////////////////////////////////////////
    

////////////////////////////////////////////////////////////////////////////////
//
//  AJNPerformanceObject Bus Object superclass
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNPerformanceObject : AJNBusObject<PerformanceObjectDelegate>

// properties
//


// methods
//
- (BOOL)checkPacketAtIndex:(NSNumber*)packetIndex payLoad:(AJNMessageArgument*)byteArray packetSize:(NSNumber*)packetSize message:(AJNMessage *)methodCallMessage;


// signals
//
- (void)sendPacketAtIndex:(NSNumber*)packetIndex payLoad:(AJNMessageArgument*)byteArray inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath flags:(AJNMessageFlag)flags;


@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//  PerformanceObject Proxy
//
////////////////////////////////////////////////////////////////////////////////

@interface PerformanceObjectProxy : AJNProxyBusObject

// properties
//


// methods
//
- (BOOL)checkPacketAtIndex:(NSNumber*)packetIndex payLoad:(AJNMessageArgument*)byteArray packetSize:(NSNumber*)packetSize;


@end

////////////////////////////////////////////////////////////////////////////////