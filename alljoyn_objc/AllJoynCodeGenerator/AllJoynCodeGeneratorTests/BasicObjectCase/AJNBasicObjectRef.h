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
@property (nonatomic, strong) AJNMessageArgument *testArrayProperty;
@property (nonatomic, strong) NSString *testStringProperty;
@property (nonatomic,) BOOL testBoolProperty;
@property (nonatomic, readonly) NSString *testStringOnlyReadProperty;
@property (nonatomic,) NSString *testStringOnlyWriteProperty;

// methods
//
- (QStatus)concatenateString:(NSString *)str1 withString:(NSString *)str2 outString:(NSString **)outStr message:(AJNMessage *)methodCallMessage;
- (QStatus)methodWithInString:(NSString *)str outString1:(NSString **)outStr1 outString2:(NSString **)outStr2 message:(AJNMessage *)methodCallMessage;
- (QStatus)methodWithOutString:(NSString *)str1 inString2:(NSString *)str2 outString1:(NSString **)outStr1 outString2:(NSString **)outStr2 message:(AJNMessage *)methodCallMessage;
- (QStatus) methodWithOnlyOutString:(NSString **)outStr outBool:(BOOL *)outBool message:(AJNMessage *)methodCallMessage;
- (QStatus)methodWithNoReturnAndNoArgs:(AJNMessage *)methodCallMessage;
- (QStatus) methodWithReturnAndNoInArgs:(NSString **)outStr message:(AJNMessage *)methodCallMessage;
- (QStatus)methodWithStringArray:(AJNMessageArgument *)stringArray structWithStringAndInt:(AJNMessageArgument *)aStruct outStr:(NSString **)outStr message:(AJNMessage *)methodCallMessage;
- (QStatus)methodWithBoolIn:(BOOL )inBool outStr:(NSString **)outStr message:(AJNMessage *)methodCallMessage;

// signals
//
- (QStatus)sendTestStringPropertyChangedFrom:(NSString *)oldString to:(NSString *)newString inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;
- (QStatus)sendTestSignalWithComplexArgs:(AJNMessageArgument *)oldString inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;
- (QStatus)sendTestSignalWithNoArgsInSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;

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
- (void)didReceiveTestStringPropertyChangedFrom:(NSString *)oldString to:(NSString *)newString inSession:(AJNSessionId)sessionId message:(AJNMessage *)signalMessage;
- (void)didReceiveTestSignalWithComplexArgs:(AJNMessageArgument *)oldString inSession:(AJNSessionId)sessionId message:(AJNMessage *)signalMessage;
- (void)didReceiveTestSignalWithNoArgsInSession:(AJNSessionId)sessionId message:(AJNMessage *)signalMessage;

@end

@interface AJNBusAttachment(BasicStringsDelegate)

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
- (QStatus)sendMessage:(NSString *)message inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;

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
- (void)didReceiveMessage:(NSString *)message inSession:(AJNSessionId)sessionId message:(AJNMessage *)signalMessage;

@end

@interface AJNBusAttachment(BasicChatDelegate)

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
- (QStatus)pingWithValue:(NSNumber *)value message:(AJNMessage *)methodCallMessage;

@end

////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
//  AJNBasicObject Bus Object superclass
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNBasicObject : AJNBusObject<BasicStringsDelegate, BasicChatDelegate>
{
@protected
    AJNMessageArgument *_testArrayProperty;
    NSString *_testStringProperty;
    BOOL _testBoolProperty;
    NSString *_testStringOnlyReadProperty;
    NSString *_testStringOnlyWriteProperty;
    NSString *_name;

}
// properties
//
@property (nonatomic, strong) AJNMessageArgument *testArrayProperty;
@property (nonatomic, strong) NSString *testStringProperty;
@property (nonatomic,) BOOL testBoolProperty;
@property (nonatomic, readonly) NSString *testStringOnlyReadProperty;
@property (nonatomic,) NSString *testStringOnlyWriteProperty;
@property (nonatomic, readonly) NSString *name;

// methods
//
- (QStatus)concatenateString:(NSString *)str1 withString:(NSString *)str2 outString:(NSString **)outStr message:(AJNMessage *)methodCallMessage;
- (QStatus)methodWithInString:(NSString *)str outString1:(NSString **)outStr1 outString2:(NSString **)outStr2 message:(AJNMessage *)methodCallMessage;
- (QStatus)methodWithOutString:(NSString *)str1 inString2:(NSString *)str2 outString1:(NSString **)outStr1 outString2:(NSString **)outStr2 message:(AJNMessage *)methodCallMessage;
- (QStatus) methodWithOnlyOutString:(NSString **)outStr outBool:(BOOL *)outBool message:(AJNMessage *)methodCallMessage;
- (QStatus)methodWithNoReturnAndNoArgs:(AJNMessage *)methodCallMessage;
- (QStatus) methodWithReturnAndNoInArgs:(NSString **)outStr message:(AJNMessage *)methodCallMessage;
- (QStatus)methodWithStringArray:(AJNMessageArgument *)stringArray structWithStringAndInt:(AJNMessageArgument *)aStruct outStr:(NSString **)outStr message:(AJNMessage *)methodCallMessage;
- (QStatus)methodWithBoolIn:(BOOL )inBool outStr:(NSString **)outStr message:(AJNMessage *)methodCallMessage;

// signals
//
- (QStatus)sendTestStringPropertyChangedFrom:(NSString *)oldString to:(NSString *)newString inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;
- (QStatus)sendTestSignalWithComplexArgs:(AJNMessageArgument *)oldString inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;
- (QStatus)sendTestSignalWithNoArgsInSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;
- (QStatus)sendMessage:(NSString *)message inSession:(AJNSessionId)sessionId toDestination:(NSString *)destinationPath;

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
- (QStatus)getTestArrayProperty:(AJNMessageArgument **)prop;
- (QStatus)setTestArrayProperty:(AJNMessageArgument *)prop;

- (QStatus)getTestStringProperty:(NSString **)prop;
- (QStatus)setTestStringProperty:(NSString *)prop;

- (QStatus)getTestBoolProperty:(BOOL *)prop;
- (QStatus)setTestBoolProperty:(BOOL )prop;

- (QStatus)getTestStringOnlyReadProperty:(NSString **)prop;

- (QStatus)setTestStringOnlyWriteProperty:(NSString *)prop;

- (QStatus)getName:(NSString **)prop;


// methods
//
- (QStatus)concatenateString:(NSString *)str1 withString:(NSString *)str2 outString:(NSString **)outStr;
- (QStatus)concatenateString:(NSString *)str1 withString:(NSString *)str2 outString:(NSString **)outStr replyMessage:(AJNMessage **) replyMessage;

- (QStatus)methodWithInString:(NSString *)str outString1:(NSString **)outStr1 outString2:(NSString **)outStr2;
- (QStatus)methodWithInString:(NSString *)str outString1:(NSString **)outStr1 outString2:(NSString **)outStr2 replyMessage:(AJNMessage **) replyMessage;

- (QStatus)methodWithOutString:(NSString *)str1 inString2:(NSString *)str2 outString1:(NSString **)outStr1 outString2:(NSString **)outStr2;
- (QStatus)methodWithOutString:(NSString *)str1 inString2:(NSString *)str2 outString1:(NSString **)outStr1 outString2:(NSString **)outStr2 replyMessage:(AJNMessage **) replyMessage;

- (QStatus) methodWithOnlyOutString:(NSString **)outStr outBool:(BOOL *)outBool;
- (QStatus) methodWithOnlyOutString:(NSString **)outStr outBool:(BOOL *)outBool replyMessage:(AJNMessage **) replyMessage;

- (QStatus)methodWithNoReturnAndNoArgs;
- (QStatus)methodWithNoReturnAndNoArgs:(AJNMessage **) replyMessage;

- (QStatus) methodWithReturnAndNoInArgs:(NSString **)outStr;
- (QStatus) methodWithReturnAndNoInArgs:(NSString **)outStr replyMessage:(AJNMessage **) replyMessage;

- (QStatus)methodWithStringArray:(AJNMessageArgument *)stringArray structWithStringAndInt:(AJNMessageArgument *)aStruct outStr:(NSString **)outStr;
- (QStatus)methodWithStringArray:(AJNMessageArgument *)stringArray structWithStringAndInt:(AJNMessageArgument *)aStruct outStr:(NSString **)outStr replyMessage:(AJNMessage **) replyMessage;

- (QStatus)methodWithBoolIn:(BOOL )inBool outStr:(NSString **)outStr;
- (QStatus)methodWithBoolIn:(BOOL )inBool outStr:(NSString **)outStr replyMessage:(AJNMessage **) replyMessage;


@end

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//  AJNPingObject Bus Object superclass
//
////////////////////////////////////////////////////////////////////////////////

@interface AJNPingObject : AJNBusObject<PingObjectDelegate>
{
@protected

}
// properties
//

// methods
//
- (QStatus)pingWithValue:(NSNumber *)value message:(AJNMessage *)methodCallMessage;

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
- (QStatus)pingWithValue:(NSNumber *)value;
- (QStatus)pingWithValue:(NSNumber *)value replyMessage:(AJNMessage **) replyMessage;


@end

////////////////////////////////////////////////////////////////////////////////
