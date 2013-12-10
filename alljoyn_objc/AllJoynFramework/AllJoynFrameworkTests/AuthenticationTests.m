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

#import "AuthenticationTests.h"

#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"
#import "AJNBasicObject.h"
#import "BasicObject.h"
#import "TestAuthenticationListener.h"

static NSString * const kAuthenticationTestsAdvertisedName = @"org.alljoyn.bus.sample.strings";
static NSString * const kAuthenticationTestsInterfaceName = @"org.alljoyn.bus.sample.strings";
static NSString * const kAuthenticationTestsObjectPath = @"/basic_object";
const NSTimeInterval kAuthenticationTestsWaitTimeBeforeFailure = 5.0;
const NSInteger kAuthenticationTestsServicePort = 999;

@interface AuthenticationTests()<AJNBusListener, AJNSessionListener, AJNSessionPortListener, BasicStringsDelegateSignalHandler>

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
@property (nonatomic) BOOL didReceiveSignal;
@property (nonatomic) TestAuthenticationListener *authenticationListener;

@end

@implementation AuthenticationTests

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
@synthesize didReceiveSignal = _didReceiveSignal;
@synthesize authenticationListener = _authenticationListener;
@synthesize handle = _handle;

- (void)setUp
{
    [super setUp];
    
    // Set-up code here. Executed before each test case is run.
    //
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"testApp" allowRemoteMessages:YES];
    self.authenticationListener = [[TestAuthenticationListener alloc] initOnBus:self.bus withUserName:@"Code Monkey" maximumAuthenticationsAllowed:5];
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
    self.authenticationListener = nil;
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
    self.didReceiveSignal = NO;
    self.testSessionId = -1;
    self.testSessionJoiner = nil;
    
    [super tearDown];
}

- (void)testAuthenticatedMethodCallShouldSucceed
{
    AuthenticationTests *client = [[AuthenticationTests alloc] init];
    BasicObject *basicObject = nil;
    
    [client setUp];
    
    client.isTestClient = YES;
    
    [self.bus registerBusListener:self];
    [client.bus registerBusListener:client];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    
    basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kAuthenticationTestsObjectPath];
    
    [self.bus registerBusObject:basicObject];    
    
    status = [self.bus enablePeerSecurity:@"ALLJOYN_SRP_LOGON" authenticationListener:self.authenticationListener];
    STAssertTrue(status == ER_OK, @"Unable to enable peer security on service side. %@", [AJNStatus descriptionForStatusCode:status]);
    
    status = [self.bus addLogonEntryToKeyStoreWithAuthenticationMechanism:@"ALLJOYN_SRP_LOGON" userName:@"Code Monkey" password:@"123banana321"];
    STAssertTrue(status == ER_OK, @"Unable to add logon entry to keystore. %@", [AJNStatus descriptionForStatusCode:status]);
    
    status = [client.bus enablePeerSecurity:@"ALLJOYN_SRP_LOGON" authenticationListener:client.authenticationListener];
    STAssertTrue(status == ER_OK, @"Unable to enable peer security on client side. %@", [AJNStatus descriptionForStatusCode:status]);

    status = [client.bus addLogonEntryToKeyStoreWithAuthenticationMechanism:@"ALLJOYN_SRP_LOGON" userName:@"Code Monkey" password:@"123banana321"];
    STAssertTrue(status == ER_OK, @"Unable to add logon entry to keystore. %@", [AJNStatus descriptionForStatusCode:status]);

    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    
    status = [self.bus requestWellKnownName:kAuthenticationTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
        
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kAuthenticationTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kAuthenticationTestsServicePort);
    
    status = [self.bus advertiseName:kAuthenticationTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");
    
    status = [client.bus findAdvertisedName:kAuthenticationTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Client attempt to find advertised name %@ failed.", kAuthenticationTestsAdvertisedName);
    
    STAssertTrue([self waitForCompletion:kAuthenticationTestsWaitTimeBeforeFailure onFlag:&_shouldAcceptSessionJoinerNamed], @"The service did not report that it was queried for acceptance of the client joiner.");
    STAssertTrue([self waitForCompletion:kAuthenticationTestsWaitTimeBeforeFailure onFlag:&_didJoinInSession], @"The service did not receive a notification that the client joined the session.");
    STAssertTrue(client.clientConnectionCompleted, @"The client did not report that it connected.");
    STAssertTrue(client.testSessionId == self.testSessionId, @"The client session id does not match the service session id.");
    
    BasicObjectProxy *proxy = [[BasicObjectProxy alloc] initWithBusAttachment:client.bus serviceName:kAuthenticationTestsAdvertisedName objectPath:kAuthenticationTestsObjectPath sessionId:self.testSessionId];
    
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
    
    STAssertTrue([self waitForBusToStop:kAuthenticationTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");
    STAssertTrue([client waitForBusToStop:kAuthenticationTestsWaitTimeBeforeFailure], @"The client bus listener should have been notified that the bus is stopping.");
    STAssertTrue([self waitForCompletion:kAuthenticationTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");
    
    proxy = nil;
    
    [client.bus unregisterBusListener:self];
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kAuthenticationTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");
    
    [client tearDown];
    
}

#pragma mark - Asynchronous test case support

- (BOOL)waitForBusToStop:(NSTimeInterval)timeoutSeconds
{
    return [self waitForCompletion:timeoutSeconds onFlag:&_busWillStopCompleted];
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
    if ([name compare:kAuthenticationTestsAdvertisedName] == NSOrderedSame) {
        self.didFindAdvertisedNameCompleted = YES;
        if (self.isTestClient) {
            
            AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
            
            self.testSessionId = [self.bus joinSessionWithName:name onPort:kAuthenticationTestsServicePort withDelegate:self options:sessionOptions];
            STAssertTrue(self.testSessionId != -1, @"Test client failed to connect to the service %@ on port %u", name, kAuthenticationTestsServicePort);
            
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
    if ([name compare:kAuthenticationTestsAdvertisedName] == NSOrderedSame) {
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
    if (sessionPort == kAuthenticationTestsServicePort) {
        self.shouldAcceptSessionJoinerNamed = YES;
        return YES;
    }
    return NO;
}

- (void)didJoin:(NSString*)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);    
    if (sessionPort == kAuthenticationTestsServicePort) {
        self.testSessionId = sessionId;
        self.didJoinInSession = YES;
    }
}

#pragma mark - AJNSessionDelegate implementation

- (void)didJoinSession:(AJNSessionId)sessionId status:(QStatus)status sessionOptions:(AJNSessionOptions *)sessionOptions context:(AJNHandle)context
{
    self.testSessionId = sessionId;
    STAssertTrue(self.testSessionId != -1, @"Test client failed to connect asynchronously using delegate to the service on port %u", kAuthenticationTestsServicePort);
    
    self.clientConnectionCompleted = YES;            
    
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
