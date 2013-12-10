////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
