////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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

#import "BusObjectTests.h"
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"
#import "AJNBasicObject.h"
#import "BasicObject.h"

static NSString * const kBusObjectTestsAdvertisedName = @"org.alljoyn.bus.sample.strings";
static NSString * const kBusObjectTestsInterfaceName = @"org.alljoyn.bus.sample.strings";
static NSString * const kBusObjectTestsObjectPath = @"/basic_object";
const NSTimeInterval kBusObjectTestsWaitTimeBeforeFailure = 5.0;
const NSInteger kBusObjectTestsServicePort = 999;

@interface BusObjectTests()<AJNBusListener, AJNSessionListener, AJNSessionPortListener, BasicStringsDelegateSignalHandler>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic) BOOL listenerDidRegisterWithBusCompleted;
@property (nonatomic) BOOL listenerDidUnregisterWithBusCompleted;
@property (nonatomic) BOOL didFindAdvertisedNameCompleted;
@property (nonatomic) BOOL didLoseAdvertisedNameCompleted;
@property (nonatomic) BOOL nameOwnerChangedCompleted;
@property (nonatomic) BOOL busWillStopCompleted;
@property (nonatomic) BOOL busDidDisconnectCompleted;
@property (nonatomic) BOOL sessionWasLost;
@property (nonatomic) BOOL didAddMemberNamed;
@property (nonatomic) BOOL didRemoveMemberNamed;
@property (nonatomic) BOOL shouldAcceptSessionJoinerNamed;
@property (nonatomic) BOOL didJoinInSession;
@property (nonatomic) AJNSessionId testSessionId;
@property (nonatomic, strong) NSString *testSessionJoiner;
@property (nonatomic) BOOL isTestClient;
@property (nonatomic) BOOL clientConnectionCompleted;
@property (nonatomic) BOOL didSuccessfullyCallMethodSynchronously;
@property (nonatomic) BOOL didReceiveSignal;

@end

@implementation BusObjectTests

@synthesize bus = _bus;
@synthesize listenerDidRegisterWithBusCompleted = _listenerDidRegisterWithBusCompleted;
@synthesize listenerDidUnregisterWithBusCompleted = _listenerDidUnregisterWithBusCompleted;
@synthesize didFindAdvertisedNameCompleted = _didFindAdvertisedNameCompleted;
@synthesize didLoseAdvertisedNameCompleted = _didLoseAdvertisedNameCompleted;
@synthesize nameOwnerChangedCompleted = _nameOwnerChangedCompleted;
@synthesize busWillStopCompleted = _busWillStopCompleted;
@synthesize busDidDisconnectCompleted = _busDidDisconnectCompleted;
@synthesize sessionWasLost = _sessionWasLost;
@synthesize didAddMemberNamed = _didAddMemberNamed;
@synthesize didRemoveMemberNamed = _didRemoveMemberNamed;
@synthesize shouldAcceptSessionJoinerNamed = _shouldAcceptSessionJoinerNamed;
@synthesize didJoinInSession = _didJoinInSession;
@synthesize testSessionId = _testSessionId;
@synthesize testSessionJoiner = _testSessionJoiner;
@synthesize isTestClient = _isTestClient;
@synthesize clientConnectionCompleted = _clientConnectionCompleted;
@synthesize didSuccessfullyCallMethodSynchronously = _didSuccessfullyCallMethodSynchronously;
@synthesize didReceiveSignal = _didReceiveSignal;
@synthesize handle = _handle;

- (void)setUp
{
    [super setUp];
    
    // Set-up code here. Executed before each test case is run.
    //
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"testApp" allowRemoteMessages:YES];
    self.listenerDidRegisterWithBusCompleted = NO;
    self.listenerDidUnregisterWithBusCompleted = NO;
    self.didFindAdvertisedNameCompleted = NO;
    self.didLoseAdvertisedNameCompleted = NO;
    self.nameOwnerChangedCompleted = NO;
    self.busWillStopCompleted = NO;
    self.busDidDisconnectCompleted = NO;
    
    self.sessionWasLost = NO;
    self.didAddMemberNamed = NO;
    self.didRemoveMemberNamed = NO;
    self.shouldAcceptSessionJoinerNamed = NO;
    self.didJoinInSession = NO;
    self.isTestClient = NO;
    self.clientConnectionCompleted = NO;
    self.didSuccessfullyCallMethodSynchronously = NO;
    self.didReceiveSignal = NO;
    self.testSessionId = -1;
    self.testSessionJoiner = nil;
}

- (void)tearDown
{
    // Tear-down code here. Executed after each test case is run.
    //
    [self.bus destroy];
    [self.bus destroyBusListener:self];
    self.bus = nil;    
    self.listenerDidRegisterWithBusCompleted = NO;
    self.listenerDidUnregisterWithBusCompleted = NO;
    self.didFindAdvertisedNameCompleted = NO;
    self.didLoseAdvertisedNameCompleted = NO;
    self.nameOwnerChangedCompleted = NO;
    self.busWillStopCompleted = NO;
    self.busDidDisconnectCompleted = NO;
    
    self.sessionWasLost = NO;
    self.didAddMemberNamed = NO;
    self.didRemoveMemberNamed = NO;
    self.shouldAcceptSessionJoinerNamed = NO;
    self.didJoinInSession = NO;
    self.isTestClient = NO;
    self.clientConnectionCompleted = NO;
    self.didSuccessfullyCallMethodSynchronously = NO;
    self.didReceiveSignal = NO;
    self.testSessionId = -1;
    self.testSessionJoiner = nil;
    
    [super tearDown];
}

- (void)testShouldCallMethodAndReturnResult
{
    BusObjectTests *client = [[BusObjectTests alloc] init];
    BasicObject *basicObject = nil;
    
    [client setUp];
    
    client.isTestClient = YES;
    
    [self.bus registerBusListener:self];
    [client.bus registerBusListener:client];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");    
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");        
    
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    
    status = [self.bus requestWellKnownName:kBusObjectTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    
    basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    
    [self.bus registerBusObject:basicObject];
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusObjectTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusObjectTestsServicePort);
    
    status = [self.bus advertiseName:kBusObjectTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");
    
    status = [client.bus findAdvertisedName:kBusObjectTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Client attempt to find advertised name %@ failed.", kBusObjectTestsAdvertisedName);
    
    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_shouldAcceptSessionJoinerNamed], @"The service did not report that it was queried for acceptance of the client joiner.");
    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_didJoinInSession], @"The service did not receive a notification that the client joined the session.");
    STAssertTrue(client.clientConnectionCompleted, @"The client did not report that it connected.");
    STAssertTrue(client.testSessionId == self.testSessionId, @"The client session id does not match the service session id.");
    
    BasicObjectProxy *proxy = [[BasicObjectProxy alloc] initWithBusAttachment:client.bus serviceName:kBusObjectTestsAdvertisedName objectPath:kBusObjectTestsObjectPath sessionId:self.testSessionId];
    
    [proxy introspectRemoteObject];
    
    NSString *resultantString = [proxy concatenateString:@"Hello " withString:@"World!"];
    STAssertTrue([resultantString compare:@"Hello World!"] == NSOrderedSame, @"Test client call to method via proxy object failed.");
    
    status = [client.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client disconnect from bus via null transport failed.");
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [client.bus stop];
    STAssertTrue(status == ER_OK, @"Client bus failed to stop.");
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForBusToStop:kBusObjectTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([client waitForBusToStop:kBusObjectTestsWaitTimeBeforeFailure], @"The client bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");    
    
    proxy = nil;
    
    [client.bus unregisterBusListener:client];
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");
    
    [client tearDown];    
}

- (void)testShouldSuccessfullyAccessPropertyOfObject
{
    BusObjectTests *client = [[BusObjectTests alloc] init];
    BasicObject *basicObject = nil;

    [client setUp];

    client.isTestClient = YES;

    [self.bus registerBusListener:self];
    [client.bus registerBusListener:client];

    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");

    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");

    status = [self.bus requestWellKnownName:kBusObjectTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");

    basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];

    [self.bus registerBusObject:basicObject];

    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];

    status = [self.bus bindSessionOnPort:kBusObjectTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusObjectTestsServicePort);

    status = [self.bus advertiseName:kBusObjectTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");

    status = [client.bus findAdvertisedName:kBusObjectTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Client attempt to find advertised name %@ failed.", kBusObjectTestsAdvertisedName);

    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_shouldAcceptSessionJoinerNamed], @"The service did not report that it was queried for acceptance of the client joiner.");
    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_didJoinInSession], @"The service did not receive a notification that the client joined the session.");
    STAssertTrue(client.clientConnectionCompleted, @"The client did not report that it connected.");
    STAssertTrue(client.testSessionId == self.testSessionId, @"The client session id does not match the service session id.");

    BasicObjectProxy *proxy = [[BasicObjectProxy alloc] initWithBusAttachment:client.bus serviceName:kBusObjectTestsAdvertisedName objectPath:kBusObjectTestsObjectPath sessionId:self.testSessionId];

    [proxy introspectRemoteObject];

    NSString *proxyPropertyValue;
    NSString *servicePropertyValue;
    
    proxy.testStringProperty = @"Hello World!!!";

    proxyPropertyValue = proxy.testStringProperty;

    servicePropertyValue = basicObject.testStringProperty;

    STAssertTrue([proxyPropertyValue compare:@"Hello World!!!"] == NSOrderedSame, @"The value of the property in the client-side object does not match what it was just set to. Actual value=[%@]",proxyPropertyValue);
    STAssertTrue([servicePropertyValue compare:@"Hello World!!!"] == NSOrderedSame, @"The value of the property in the service-side object does not match what it was just set to. Actual value=[%@]",servicePropertyValue);

    basicObject.testStringProperty = @"Foo bar???";
    proxyPropertyValue = proxy.testStringProperty;
    servicePropertyValue = basicObject.testStringProperty;
    STAssertTrue([proxyPropertyValue compare:@"Foo bar???"] == NSOrderedSame, @"The value of the property in the client-side object does not match what it was just set to. Actual value=[%@]",proxyPropertyValue);
    STAssertTrue([servicePropertyValue compare:@"Foo bar???"] == NSOrderedSame, @"The value of the property in the service-side object does not match what it was just set to. Actual value=[%@]",servicePropertyValue);

    status = [client.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client disconnect from bus via null transport failed.");
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");

    status = [client.bus stop];
    STAssertTrue(status == ER_OK, @"Client bus failed to stop.");
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");

    STAssertTrue([self waitForBusToStop:kBusObjectTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");
    STAssertTrue([client waitForBusToStop:kBusObjectTestsWaitTimeBeforeFailure], @"The client bus listener should have been notified that the bus is stopping.");
    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");

    proxy = nil;

    [client.bus unregisterBusListener:client];
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");

    [client tearDown];
}
- (void)testShouldSendAndReceiveSignalSuccessfully
{
    BusObjectTests *client = [[BusObjectTests alloc] init];
    BasicObject *basicObject = nil;
    BasicObject *clientBasicObject = nil;
    
    [client setUp];
    
    client.isTestClient = YES;

    basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    clientBasicObject = [[BasicObject alloc] initWithBusAttachment:client.bus onPath:kBusObjectTestsObjectPath];
    
    [self.bus registerBusObject:basicObject];
    [client.bus registerBusObject:clientBasicObject];
    [client.bus registerBasicStringsDelegateSignalHandler:client];
    [self.bus registerBasicStringsDelegateSignalHandler:self];
    [self.bus registerBusListener:self];
    [client.bus registerBusListener:client];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");    
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");        
    
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    
    status = [self.bus requestWellKnownName:kBusObjectTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusObjectTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusObjectTestsServicePort);
    
    status = [self.bus advertiseName:kBusObjectTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");
    
    status = [client.bus findAdvertisedName:kBusObjectTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Client attempt to find advertised name %@ failed.", kBusObjectTestsAdvertisedName);
    
    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_shouldAcceptSessionJoinerNamed], @"The service did not report that it was queried //for acceptance of the client joiner.");
    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_didJoinInSession], @"The service did not receive a notification that the client //joined the session.");
    STAssertTrue(client.clientConnectionCompleted, @"The client did not report that it connected.");
    STAssertTrue(client.testSessionId == self.testSessionId, @"The client session id does not match the service session id.");
    
    [basicObject sendTestStringPropertyChangedFrom:@"Hello World" to:@"Foo Bar" inSession:self.testSessionId toDestination:nil];
    STAssertTrue([client waitForSignalToBeReceived:kBusObjectTestsWaitTimeBeforeFailure], @"The signal handler was not called.");
    STAssertFalse([self waitForSignalToBeReceived:kBusObjectTestsWaitTimeBeforeFailure], @"The signal handler was called.");
    self.didReceiveSignal = NO;
    client.didReceiveSignal = NO;
    
    [clientBasicObject sendTestStringPropertyChangedFrom:@"Hello World" to:@"Foo Bar" inSession:self.testSessionId toDestination:nil];
    STAssertFalse([client waitForSignalToBeReceived:kBusObjectTestsWaitTimeBeforeFailure], @"The signal handler was called.");
    STAssertTrue([self waitForSignalToBeReceived:kBusObjectTestsWaitTimeBeforeFailure], @"The signal handler was not called.");
    self.didReceiveSignal = NO;
    client.didReceiveSignal = NO;
    
    [basicObject sendTestStringPropertyChangedFrom:@"Foo Bar" to:@"Bar Baz" inSession:kAJNSessionIdAllHosted toDestination:nil];
    STAssertTrue([client waitForSignalToBeReceived:kBusObjectTestsWaitTimeBeforeFailure], @"The signal handler was not called.");
    STAssertFalse([self waitForSignalToBeReceived:kBusObjectTestsWaitTimeBeforeFailure], @"The signal handler was called.");
    self.didReceiveSignal = NO;
    client.didReceiveSignal = NO;
    
    [clientBasicObject sendTestStringPropertyChangedFrom:@"Foo Bar" to:@"Bar Baz" inSession:kAJNSessionIdAllHosted toDestination:nil];
    STAssertFalse([client waitForSignalToBeReceived:kBusObjectTestsWaitTimeBeforeFailure], @"The signal handler was called.");
    STAssertFalse([self waitForSignalToBeReceived:kBusObjectTestsWaitTimeBeforeFailure], @"The signal handler was called.");
    self.didReceiveSignal = NO;
    client.didReceiveSignal = NO;
    
    status = [client.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client disconnect from bus via null transport failed.");
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [client.bus stop];
    STAssertTrue(status == ER_OK, @"Client bus failed to stop.");
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForBusToStop:kBusObjectTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([client waitForBusToStop:kBusObjectTestsWaitTimeBeforeFailure], @"The client bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");
    
    [client.bus unregisterBusListener:client];
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusObjectTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");
    
    [client tearDown];    
}

#pragma mark - Asynchronous test case support

- (BOOL)waitForBusToStop:(NSTimeInterval)timeoutSeconds
{
    return [self waitForCompletion:timeoutSeconds onFlag:&_busWillStopCompleted];
}

- (BOOL)waitForSignalToBeReceived:(NSTimeInterval)timeoutSeconds
{
    return [self waitForCompletion:timeoutSeconds onFlag:&_didReceiveSignal];
}

- (BOOL)waitForCompletion:(NSTimeInterval)timeoutSeconds onFlag:(BOOL*)flag
{
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeoutSeconds];
    
    do {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:timeoutDate];
        if ([timeoutDate timeIntervalSinceNow] < 0.0) {
            break;
        }
    } while (!*flag);
    
    return *flag;
}

#pragma mark - AJNBusListener delegate methods

- (void)listenerDidRegisterWithBus:(AJNBusAttachment*)busAttachment
{
    NSLog(@"AJNBusListener::listenerDidRegisterWithBus:%@",busAttachment);
    self.listenerDidRegisterWithBusCompleted = YES;
}

- (void)listenerDidUnregisterWithBus:(AJNBusAttachment*)busAttachment
{
    NSLog(@"AJNBusListener::listenerDidUnregisterWithBus:%@",busAttachment);
    self.listenerDidUnregisterWithBusCompleted = YES;    
}

- (void)didFindAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSLog(@"AJNBusListener::didFindAdvertisedName:%@ withTransportMask:%u namePrefix:%@", name, transport, namePrefix);
    if ([name compare:kBusObjectTestsAdvertisedName] == NSOrderedSame) {
        self.didFindAdvertisedNameCompleted = YES;
        if (self.isTestClient) {
            
            [self.bus enableConcurrentCallbacks];
            
            AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
            
            self.testSessionId = [self.bus joinSessionWithName:name onPort:kBusObjectTestsServicePort withDelegate:self options:sessionOptions];
            STAssertTrue(self.testSessionId != -1, @"Test client failed to connect to the service %@ on port %u", name, kBusObjectTestsServicePort);
            
            self.clientConnectionCompleted = YES;
        }
    }
}

- (void)didLoseAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSLog(@"AJNBusListener::listenerDidUnregisterWithBus:%@ withTransportMask:%u namePrefix:%@",name,transport,namePrefix);    
    self.didLoseAdvertisedNameCompleted = YES;    
}

- (void)nameOwnerChanged:(NSString*)name to:(NSString*)newOwner from:(NSString*)previousOwner
{
    NSLog(@"AJNBusListener::nameOwnerChanged:%@ to:%@ from:%@", name, newOwner, previousOwner);    
    if ([name compare:kBusObjectTestsAdvertisedName] == NSOrderedSame) {
        self.nameOwnerChangedCompleted = YES;
    }
}

- (void)busWillStop
{
    NSLog(@"AJNBusListener::busWillStop");
    self.busWillStopCompleted = YES;    
}

- (void)busDidDisconnect
{
    NSLog(@"AJNBusListener::busDidDisconnect");    
    self.busDidDisconnectCompleted = YES;    
}

#pragma mark - AJNSessionListener methods

- (void)sessionWasLost:(AJNSessionId)sessionId
{
    NSLog(@"AJNBusListener::sessionWasLost %u", sessionId);
    if (self.testSessionId == sessionId) {
        self.sessionWasLost = YES;
    }
    
}

- (void)didAddMemberNamed:(NSString*)memberName toSession:(AJNSessionId)sessionId
{
    NSLog(@"AJNBusListener::didAddMemberNamed:%@ toSession:%u", memberName, sessionId);    
    if (self.testSessionId == sessionId) {
        self.didAddMemberNamed = YES;        
    }
}

- (void)didRemoveMemberNamed:(NSString*)memberName fromSession:(AJNSessionId)sessionId
{
    NSLog(@"AJNBusListener::didRemoveMemberNamed:%@ fromSession:%u", memberName, sessionId);    
    if (self.testSessionId == sessionId) {    
        self.didRemoveMemberNamed = YES;
    }
}

#pragma mark - AJNSessionPortListener implementation

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString*)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions*)options
{
    NSLog(@"AJNSessionPortListener::shouldAcceptSessionJoinerNamed:%@ onSessionPort:%u withSessionOptions:", joiner, sessionPort);    
    if (sessionPort == kBusObjectTestsServicePort) {
        self.shouldAcceptSessionJoinerNamed = YES;
        return YES;
    }
    return NO;
}

- (void)didJoin:(NSString*)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);    
    if (sessionPort == kBusObjectTestsServicePort) {
        self.testSessionId = sessionId;
        self.testSessionJoiner = joiner;
        self.didJoinInSession = YES;
    }
}

#pragma mark - BasicStringsDelegateSignalHandler implementation

- (void)didReceiveTestStringPropertyChangedFrom:(NSString *)oldString to:(NSString *)newString inSession:(AJNSessionId)sessionId fromSender:(NSString *)sender
{
    NSLog(@"BasicStringsDelegateSignalHandler::didReceiveTestStringPropertyChangedFrom:%@ to:%@ inSession:%u fromSender:%@", oldString, newString, sessionId, sender);
    self.didReceiveSignal = YES;
}

- (void)didReceiveTestSignalWithNoArgsInSession:(AJNSessionId)sessionId fromSender:(NSString*)sender
{
    
}

@end
