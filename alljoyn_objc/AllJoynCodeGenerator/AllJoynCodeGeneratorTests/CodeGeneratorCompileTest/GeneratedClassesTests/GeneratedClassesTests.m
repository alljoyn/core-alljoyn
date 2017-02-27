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

#import <XCTest/XCTest.h>
#import "AJNInit.h"
#import "AJNBusAttachment.h"
#import "BasicObjectExtention.h"
#import "AJNBasicObject.h"

static NSString *kBasicObjectPath = @"/basic_object";
static NSString *kProviderWellKnowName = @"provider";
static AJNSessionPort kSessionPort = 7078;

@interface GeneratedClassesTests : XCTestCase

@end

@interface GeneratedClassesTests ()<AJNSessionPortListener, AJNSessionListener>

@property AJNBusAttachment *providerBus;
@property BasicObjectExtention *basicObject;

@property AJNBusAttachment *consumerBus;
@property BasicObjectProxy *basicProxyObject;

@end

@implementation GeneratedClassesTests

- (void)setUp {
    [super setUp];

    [AJNInit alljoynInit];
    [AJNInit alljoynRouterInit];

    _providerBus = [[AJNBusAttachment alloc] initWithApplicationName:@"provider" allowRemoteMessages:YES];
    [_providerBus start];
    [_providerBus connectWithArguments:@"null"];

    _basicObject = [[BasicObjectExtention alloc] initWithBusAttachment:_providerBus onPath:kBasicObjectPath];
    [_providerBus registerBusObject:_basicObject enableSecurity:NO];

    AJNSessionOptions *sessionOption = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    [_providerBus bindSessionOnPort:kSessionPort withOptions:sessionOption withDelegate:self];

    _consumerBus = [[AJNBusAttachment alloc] initWithApplicationName:@"consumer" allowRemoteMessages:YES];
    [_consumerBus start];
    [_consumerBus connectWithArguments:@"null"];

    AJNSessionId sessionId = [_consumerBus joinSessionWithName:[_providerBus uniqueName] onPort:kSessionPort withDelegate:self options:sessionOption];

    _basicProxyObject = [[BasicObjectProxy alloc] initWithBusAttachment:_consumerBus serviceName:[_providerBus uniqueName] objectPath:kBasicObjectPath sessionId:sessionId];
    [_basicProxyObject introspectRemoteObject];
}

- (void)tearDown {
    _basicProxyObject = nil;
    [_consumerBus stop];
    [_consumerBus destroy];
    _consumerBus = nil;

    _basicObject = nil;
    [_providerBus stop];
    [_providerBus destroy];
    _providerBus = nil;

    [AJNInit alljoynRouterShutdown];
    [AJNInit alljoynShutdown];

    [super tearDown];
}

- (void)testCallGeneratedMethodsShouldBeSuccess {
    QStatus status = ER_OK;;

    NSString *outString1;
    NSString *outString2;

    NSString *inString1 = @"foo";
    NSString *inString2 = @"bar";
    status = [_basicProxyObject concatenateString:inString1 withString:inString2 outString:&outString1];
    XCTAssertTrue(status == ER_OK, @"Failed to call concatenateString.");
    XCTAssertTrue([outString1 isEqualToString:@"foobar"], @"Enexpected response to concatenateString from proxy object");

    status = [_basicProxyObject methodWithInString:inString1 outString1:&outString1 outString2:&outString2];
    XCTAssertTrue(status == ER_OK, @"Failed to call methodWithInString.");
    XCTAssertTrue([outString1 isEqualToString:@"foo+1"], @"Enexpected response to methodWithInString from proxy object. Enexpected outString1.");
    XCTAssertTrue([outString2 isEqualToString:@"foo+1"], @"Enexpected response to methodWithInString from proxy object. Enexpected outString2.");

    status = [_basicProxyObject methodWithOutString:inString1 inString2:inString2 outString1:&outString1 outString2:&outString2];
    XCTAssertTrue(status == ER_OK, @"Failed to call methodWithOutString.");
    XCTAssertTrue([outString1 isEqualToString:@"foo+1"], @"Enexpected response to methodWithOutString from proxy object. Enexpected outString1.");
    XCTAssertTrue([outString2 isEqualToString:@"bar+1"], @"Enexpected response to methodWithOutString from proxy object. Enexpected outString2.");

    BOOL outBool;
    status = [_basicProxyObject methodWithOnlyOutString:&outString1 outBool:&outBool];
    XCTAssertTrue(status == ER_OK, @"Failed to call methodWithOnlyOutString.");
    XCTAssertTrue([outString1 isEqualToString:@"1st string"], @"Enexpected response to methodWithOnlyOutString from proxy object. Enexpected outString1.");
    XCTAssertTrue(outBool == YES, @"Enexpected response to methodWithOnlyOutString from proxy object. Enexpected outBool.");

    status = [_basicProxyObject methodWithNoReturnAndNoArgs];
    XCTAssertTrue(status == ER_OK, @"Failed to call methodWithOnlyOutString.");

    status = [_basicProxyObject methodWithReturnAndNoInArgs:&outString1];
    XCTAssertTrue(status == ER_OK, @"Failed to call methodWithReturnAndNoInArgs.");
    XCTAssertTrue([outString1 isEqualToString:@"only out string"], @"Enexpected response to methodWithReturnAndNoInArgs from proxy object. Enexpected outString1.");

    //TODO: test methodWithStringArray
}

- (void)testCallGeneratedMethodsShouldFailAfterDestroyBusObject
{
    _basicObject = nil;
    QStatus status = ER_OK;

    NSString *outString1;

    NSString *inString1 = @"foo";
    NSString *inString2 = @"bar";
    status = [_basicProxyObject concatenateString:inString1 withString:inString2 outString:&outString1];
    XCTAssertTrue(status != ER_OK, @"Expected ER_BUS_REPLY_IS_ERROR_MESSAGE in call method after destroy basic object.");
}

- (void)testCallGeneratedMethodsShouldFailAfterDestroyBusObjectAndReplyMessageShouldBeNoBusObject
{
    _basicObject = nil;
    QStatus status = ER_OK;

    NSString *outString1;

    NSString *inString1 = @"foo";
    NSString *inString2 = @"bar";

    AJNMessage *reply;
    status = [_basicProxyObject concatenateString:inString1 withString:inString2 outString:&outString1 replyMessage:&reply];
    XCTAssertTrue(status == ER_BUS_REPLY_IS_ERROR_MESSAGE, @"Expected ER_BUS_REPLY_IS_ERROR_MESSAGE in call method after destroy basic object.");
    XCTAssertTrue([[reply errorDescription] isEqualToString:@"ER_BUS_NO_SUCH_OBJECT"], @"Expected ER_BUS_NO_SUCH_OBJECT in reply from method after destroy basic object.");
}

- (void)testAccessToProperty
{
    _basicObject.testStringProperty = @"foo";

    NSString *testStringProperty;
    QStatus status = ER_OK;
    status = [_basicProxyObject getTestStringProperty:&testStringProperty];
    XCTAssertTrue(status == ER_OK, @"Fail to get property");
    XCTAssertTrue([testStringProperty isEqualToString:_basicObject.testStringProperty], @"Unexpected string value in consumer object after set from basic object.");

    status = [_basicProxyObject setTestStringProperty:@"bar"];
    XCTAssertTrue(status == ER_OK, @"Fail to set property");

    XCTAssertTrue([_basicObject.testStringProperty isEqualToString:@"bar"], @"Unexpected string value in provider object  after set from proxy object.");

    status = [_basicProxyObject getTestStringProperty:&testStringProperty];
    XCTAssertTrue(status == ER_OK, @"Fail to get property");
    XCTAssertTrue([testStringProperty isEqualToString:_basicObject.testStringProperty], @"Unexpected string value in consumer object after set from proxy object.");
}

- (void)testGetAndSetPropertyShouldBeFailerAfterDestroyBusObject
{
    _basicObject = nil;
    QStatus status = ER_OK;

    NSString *testStringProperty;
    status = [_basicProxyObject getTestStringProperty:&testStringProperty];
    XCTAssertTrue(status != ER_OK, @"Expected ER_BUS_REPLY_IS_ERROR_MESSAGE in get property after destroy basic object.");

    status = [_basicProxyObject setTestStringProperty:@"foo"];
    XCTAssertTrue(status != ER_OK, @"Expected ER_BUS_REPLY_IS_ERROR_MESSAGE in set property after destroy basic object.");
}

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    return YES;
}

@end
