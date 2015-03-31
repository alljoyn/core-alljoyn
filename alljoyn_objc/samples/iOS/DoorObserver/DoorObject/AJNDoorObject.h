////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
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
