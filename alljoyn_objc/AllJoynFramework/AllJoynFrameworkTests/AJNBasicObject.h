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
//  AJNBasicObject.h
//
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/Foundation.h>
#import "AJNBusAttachment.h"
#import "AJNBusInterface.h"
#import "AJNProxyBusObject.h"


////////////////////////////////////////////////////////////////////////////////
//
// BasicStringsDelegate Bus Interface
//
////////////////////////////////////////////////////////////////////////////////

@protocol BasicStringsDelegate <AJNBusInterface>


// properties
//
@property (nonatomic, strong) NSString *testStringProperty;

// methods
//
- (NSString *)concatenateString:(NSString *)str1 withString:(NSString *)str2;
- (void)methodWithOutString:(NSString *)str1 inString2:(NSString *)str2 outString1:(NSString **)outStr1 outString2:(NSString**)outStr2;
- (void)methodWithOnlyOutString:(NSString **)outStr1 outString2:(NSString **)outStr2;
- (void)methodWithNoReturnAndNoArgs;

// signals
//
- (void)sendTestStringPropertyChangedFrom:(NSString *)oldString to:(NSString *)newString inSession:(AJNSessionId) sessionId toDestination:(NSString *)destinationPath;
- (void)sendTestSignalWithNoArgsInSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;


@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
// BasicStringsDelegate Signal Handler Protocol
//
////////////////////////////////////////////////////////////////////////////////

@protocol BasicStringsDelegateSignalHandler <AJNSignalHandler>

// signals
//
- (void)didReceiveTestStringPropertyChangedFrom:(NSString *)oldString to:(NSString *)newString inSession:(AJNSessionId)sessionId fromSender:(NSString *)sender;
- (void)didReceiveTestSignalWithNoArgsInSession:(AJNSessionId)sessionId fromSender:(NSString *)sender;


@end

@interface AJNBusAttachment (BasicStringsDelegate)

- (void)registerBasicStringsDelegateSignalHandler:(id<BasicStringsDelegateSignalHandler>)signalHandler;

@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
// BasicChatDelegate Bus Interface
//
////////////////////////////////////////////////////////////////////////////////

@protocol BasicChatDelegate <AJNBusInterface>


// properties
//
@property (nonatomic, readonly) NSString *name;

// signals
//
- (void)sendMessage:(NSString *)message inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;


@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
// BasicChatDelegate Signal Handler Protocol
//
////////////////////////////////////////////////////////////////////////////////

@protocol BasicChatDelegateSignalHandler <AJNSignalHandler>

// signals
//
- (void)didReceiveMessage:(NSString *)message inSession:(AJNSessionId)sessionId fromSender:(NSString *)sender;


@end

@interface AJNBusAttachment (BasicChatDelegate)

- (void)registerBasicChatDelegateSignalHandler:(id<BasicChatDelegateSignalHandler>)signalHandler;

@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
// PingObjectDelegate Bus Interface
//
////////////////////////////////////////////////////////////////////////////////

@protocol PingObjectDelegate <AJNBusInterface>


// methods
//
- (void)pingWithValue:(NSNumber *)value;


@end

////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
//  AJNBasicObject Bus Object superclass
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNBasicObject : AJNBusObject<BasicStringsDelegate, BasicChatDelegate>

// properties
//
@property (nonatomic, strong) NSString *testStringProperty;
@property (nonatomic, readonly) NSString *name;


// methods
//
- (NSString *)concatenateString:(NSString *)str1 withString:(NSString *)str2;
- (void)methodWithOutString:(NSString *)str1 inString2:(NSString *)str2 outString1:(NSString **)outStr1 outString2:(NSString **)outStr2;
- (void)methodWithOnlyOutString:(NSString **)outStr1 outString2:(NSString **)outStr2;
- (void)methodWithNoReturnAndNoArgs;


// signals
//
- (void)sendTestStringPropertyChangedFrom:(NSString *)oldString to:(NSString *)newString inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;
- (void)sendTestSignalWithNoArgsInSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;
- (void)sendMessage:(NSString *)message inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;


@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//  BasicObject Proxy
//
////////////////////////////////////////////////////////////////////////////////

@interface BasicObjectProxy : AJNProxyBusObject

// properties
//
@property (nonatomic, strong) NSString *testStringProperty;
@property (nonatomic, readonly) NSString *name;


// methods
//
- (NSString *)concatenateString:(NSString *)str1 withString:(NSString *)str2;
- (void)methodWithOutString:(NSString *)str1 inString2:(NSString *)str2 outString1:(NSString **)outStr1 outString2:(NSString **)outStr2;
- (void)methodWithOnlyOutString:(NSString **)outStr1 outString2:(NSString **)outStr2;
- (void)methodWithNoReturnAndNoArgs;


@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//  AJNPingObject Bus Object superclass
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNPingObject : AJNBusObject<PingObjectDelegate>

// properties
//


// methods
//
- (void)pingWithValue:(NSNumber *)value;


// signals
//


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


// methods
//
- (void)pingWithValue:(NSNumber *)value;


@end

////////////////////////////////////////////////////////////////////////////////