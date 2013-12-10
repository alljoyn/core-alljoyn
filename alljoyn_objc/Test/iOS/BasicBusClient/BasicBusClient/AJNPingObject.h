////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
