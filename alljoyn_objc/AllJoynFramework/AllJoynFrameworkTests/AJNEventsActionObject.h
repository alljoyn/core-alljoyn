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
//  AJNEventsActionObject.h
//
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNBusAttachment.h"
#import "AJNBusInterface.h"
#import "AJNProxyBusObject.h"


////////////////////////////////////////////////////////////////////////////////
//
// SampleObjectDelegate Bus Interface
//
////////////////////////////////////////////////////////////////////////////////

@protocol SampleObjectDelegate <AJNBusInterface>


// properties
//
@property (nonatomic, readonly) NSString* TestProperty;

// methods
//
- (NSString*)concatenateString:(NSString*)str1 withString:(NSString*)str2 message:(AJNMessage *)methodCallMessage;

// signals
//
- (void)sendtestEventString(NSString*)outStr inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath;


@end

////////////////////////////////////////////////////////////////////////////////

    
////////////////////////////////////////////////////////////////////////////////
//
// SampleObjectDelegate Signal Handler Protocol
//
////////////////////////////////////////////////////////////////////////////////

@protocol SampleObjectDelegateSignalHandler <AJNSignalHandler>

// signals
//
- (void)didReceivetestEventString(NSString*)outStr inSession:(AJNSessionId)sessionId message:(AJNMessage *)signalMessage;


@end

@interface AJNBusAttachment(SampleObjectDelegate)

- (void)registerSampleObjectDelegateSignalHandler:(id<SampleObjectDelegateSignalHandler>)signalHandler;

@end

////////////////////////////////////////////////////////////////////////////////
    

////////////////////////////////////////////////////////////////////////////////
//
//  AJNSampleObject Bus Object superclass
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNSampleObject : AJNBusObject<SampleObjectDelegate>

// properties
//
@property (nonatomic, readonly) NSString* TestProperty;


// methods
//
- (NSString*)concatenateString:(NSString*)str1 withString:(NSString*)str2 message:(AJNMessage *)methodCallMessage;


// signals
//
- (void)sendtestEventString(NSString*)outStr inSession:(AJNSessionId)sessionId toDestination:(NSString*)destinationPath;


@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//  SampleObject Proxy
//
////////////////////////////////////////////////////////////////////////////////

@interface SampleObjectProxy : AJNProxyBusObject

// properties
//
@property (nonatomic, readonly) NSString* TestProperty;


// methods
//
- (NSString*)concatenateString:(NSString*)str1 withString:(NSString*)str2;


@end

////////////////////////////////////////////////////////////////////////////////
