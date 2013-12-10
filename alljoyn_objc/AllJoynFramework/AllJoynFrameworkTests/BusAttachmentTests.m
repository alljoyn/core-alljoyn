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

#import "BusAttachmentTests.h"
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"

static NSString * const kBusAttachmentTestsAdvertisedName = @"org.alljoyn.bus.objc.tests.AReallyNiftyNameThatNoOneWillUse";
static NSString * const kBusAttachmentTestsInterfaceName = @"org.alljoyn.bus.objc.tests.NNNNNNEEEEEEEERRRRRRRRRRDDDDDDDSSSSSSSS";
static NSString * const kBusAttachmentTestsInterfaceMethod = @"behaveInSociallyAwkwardWay";
static NSString * const kBusAttachmentTestsInterfaceXML = @"<interface name=\"org.alljoyn.bus.objc.tests.NNNNNNEEEEEEEERRRRRRRRRRDDDDDDDSSSSSSSS\">\
                                                                <signal name=\"FigdetingNervously\">\
                                                                    <arg name=\"levelOfAwkwardness\" type=\"s\"/>\
                                                                </signal>\
                                                                <property name=\"nerdiness\" type=\"s\" access=\"read\"/>\
                                                            </interface>";

const NSTimeInterval kBusAttachmentTestsWaitTimeBeforeFailure = 5.0;
const NSInteger kBusAttachmentTestsServicePort = 999;

@interface BusAttachmentTests() <AJNBusListener, AJNSessionListener, AJNSessionPortListener, AJNSessionDelegate>

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
@property (nonatomic) BOOL isTestClient;
@property (nonatomic) BOOL clientConnectionCompleted;
@property (nonatomic) BOOL isAsyncTestClientBlock;
@property (nonatomic) BOOL isAsyncTestClientDelegate;

- (BOOL)waitForBusToStop:(NSTimeInterval)timeoutSeconds;
- (BOOL)waitForCompletion:(NSTimeInterval)timeoutSeconds onFlag:(BOOL*)flag;

@end

@implementation BusAttachmentTests

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
@synthesize isTestClient = _isTestClient;
@synthesize isAsyncTestClientBlock = _isAsyncTestClientBlock;
@synthesize isAsyncTestClientDelegate = _isAsyncTestClientDelegate;
@synthesize clientConnectionCompleted = _clientConnectionCompleted;

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
    self.isAsyncTestClientBlock = NO;
    self.isAsyncTestClientDelegate = NO;
    self.clientConnectionCompleted = NO;
}

- (void)tearDown
{
    // Tear-down code here. Executed after each test case is run.
    //
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
    self.isAsyncTestClientBlock = NO;
    self.isAsyncTestClientDelegate = NO;
    self.clientConnectionCompleted = NO;
    
    self.bus = nil;    
    
    [super tearDown];
}

- (void)testShouldHaveValidHandleAfterIntialization
{
    STAssertTrue(self.bus.handle != NULL, @"The bus attachment should always have a valid handle after initialization.");
}

- (void)testShouldCreateInterface
{
    AJNInterfaceDescription *iface = [self.bus createInterfaceWithName:kBusAttachmentTestsInterfaceName enableSecurity:NO];
    STAssertNotNil(iface, @"Bus failed to create interface.");
    
    [iface activate];
    
    iface = [self.bus interfaceWithName:kBusAttachmentTestsInterfaceName];
    STAssertNotNil(iface, @"Bus failed to retrieve interface that had already been created.");
    
    NSArray *interfaces = self.bus.interfaces;
    BOOL didFindInterface = NO;
    for (AJNInterfaceDescription *interfaceDescription in interfaces) {
        if ([interfaceDescription.name compare:kBusAttachmentTestsInterfaceName] == NSOrderedSame) {
            didFindInterface = YES;
            break;
        }
    }
    STAssertTrue(didFindInterface,@"Bus did not return interface that was activated.");
}

- (void)testShouldCreateInterfaceFromXml
{
    QStatus status = [self.bus createInterfacesFromXml:kBusAttachmentTestsInterfaceXML];
    STAssertTrue(status == ER_OK, @"Bus failed to create interface from XML.");    

    AJNInterfaceDescription *iface = [self.bus interfaceWithName:kBusAttachmentTestsInterfaceName];
    STAssertNotNil(iface, @"Bus failed to retrieve interface that had already been created from XML.");
}

- (void)testShouldDeleteInterface
{
    AJNInterfaceDescription *iface = [self.bus createInterfaceWithName:kBusAttachmentTestsInterfaceName enableSecurity:NO];
    STAssertNotNil(iface, @"Bus failed to create interface.");
    QStatus status = [iface addMethodWithName:kBusAttachmentTestsInterfaceMethod inputSignature:@"s" outputSignature:@"s" argumentNames:[NSArray arrayWithObject:@"behavior"]];
    STAssertTrue(status == ER_OK, @"Interface description failed to add method to interface.");

    status = [self.bus deleteInterface:iface];
    STAssertTrue(status == ER_OK, @"Bus failed to delete interface.");
    
    iface = [self.bus interfaceWithName:kBusAttachmentTestsInterfaceName];
    STAssertNil(iface, @"Bus retrieved interface that had already been deleted.");
}

- (void)testShouldNotDeleteInterface
{
    AJNInterfaceDescription *iface = [self.bus createInterfaceWithName:kBusAttachmentTestsInterfaceName enableSecurity:NO];
    STAssertNotNil(iface, @"Bus failed to create interface.");
    QStatus status = [iface addMethodWithName:kBusAttachmentTestsInterfaceMethod inputSignature:@"s" outputSignature:@"s" argumentNames:[NSArray arrayWithObject:@"behavior"]];
    STAssertTrue(status == ER_OK, @"Interface description failed to add method to interface.");
    [iface activate];
    
    status = [self.bus deleteInterface:iface];
    STAssertTrue(status != ER_OK, @"Bus deleted interface after it was activated.");
    
    iface = [self.bus interfaceWithName:kBusAttachmentTestsInterfaceName];
    STAssertNotNil(iface, @"Bus failed to retrieve interface that had been unsuccessfully deleted.");
}

- (void)testShouldReportConnectionStatusCorrectly
{
    STAssertFalse(self.bus.isStarted, @"Bus attachment indicates that it is started before successful call to start.");        
    STAssertFalse(self.bus.isStopping, @"Bus attachment indicates that it is stopping before successful call to stop.");            
    STAssertFalse(self.bus.isConnected, @"Bus attachment indicates that it is connected before successful call to connect.");                
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");    
    STAssertTrue(self.bus.isStarted, @"Bus attachment indicates that it is not started after successful call to start.");    
    STAssertFalse(self.bus.isStopping, @"Bus attachment indicates that it is stopping before successful call to stop.");            
    STAssertFalse(self.bus.isConnected, @"Bus attachment indicates that it is connected before successful call to connect.");                
    
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    STAssertTrue(self.bus.isConnected, @"Bus attachment indicates that it is not connected after successful call to connect.");                
    STAssertTrue(self.bus.isStarted, @"Bus attachment indicates that it is not started after successful call to start.");        
    STAssertFalse(self.bus.isStopping, @"Bus attachment indicates that it is stopping before successful call to stop.");            
    
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    STAssertFalse(self.bus.isConnected, @"Bus attachment indicates that it is connected after successful call to disconnect.");                    
    
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    STAssertTrue(self.bus.isStopping, @"Bus attachment indicates that it is not stopping after successful call to stop.");            
    STAssertFalse(self.bus.isConnected, @"Bus attachment indicates that it is connected after successful call to disconnect.");                    
}

- (void)testShouldHaveUniqueNameAndIdentifier
{
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");    
    
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    STAssertTrue(self.bus.uniqueName != nil && self.bus.uniqueName.length > 0, @"Bus should be assigned a unique name after starting and connecting.");
    
    STAssertTrue(self.bus.uniqueIdentifier != nil && self.bus.uniqueIdentifier.length > 0, @"Bus should be assigned a unique identifier after starting and connecting.");    

    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    STAssertFalse(self.bus.isConnected, @"Bus attachment indicates that it is connected after successful call to disconnect.");                    
    
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    STAssertTrue(self.bus.isStopping, @"Bus attachment indicates that it is not stopping after successful call to stop.");            
    STAssertFalse(self.bus.isConnected, @"Bus attachment indicates that it is connected after successful call to disconnect.");                    
    
}

- (void)testShouldRegisterBusListener
{
    [self.bus registerBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidRegisterWithBusCompleted], @"The bus listener should have been notified that a listener was registered.");
    
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");    
}

- (void)testShouldNotifyBusListenerWhenStopping
{
    [self.bus registerBusListener:self];

    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");    

    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");

    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busWillStopCompleted], @"The bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");    
    
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");    
}

- (void)testShouldNotifyBusListenerWhenDisconnecting
{
    [self.bus registerBusListener:self];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");    
    
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busWillStopCompleted], @"The bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");    
    
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");    
}

- (void)testShouldNotifyBusListenerWhenAdvertisedNameFound
{
    [self.bus registerBusListener:self];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");    
    
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    status = [self.bus requestWellKnownName:kBusAttachmentTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    
    status = [self.bus advertiseName:kBusAttachmentTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");   
    
    status = [self.bus findAdvertisedName:kBusAttachmentTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Find advertised name failed.");       
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_didFindAdvertisedNameCompleted], @"The bus listener should have been notified that the advertised name was found.");    
    
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busWillStopCompleted], @"The bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");    
    
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");    
}

- (void)testShouldNotifyBusListenerWhenAdvertisedNameLost
{
    [self.bus registerBusListener:self];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");    
    
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    status = [self.bus requestWellKnownName:kBusAttachmentTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    
    status = [self.bus advertiseName:kBusAttachmentTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");   
    
    status = [self.bus findAdvertisedName:kBusAttachmentTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Find advertised name failed.");       
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_didFindAdvertisedNameCompleted], @"The bus listener should have been notified that the advertised name was found.");    

    status = [self.bus cancelAdvertisedName:kBusAttachmentTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");   
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_didLoseAdvertisedNameCompleted], @"The bus listener should have been notified that the advertised name was lost.");    
    
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busWillStopCompleted], @"The bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");    
    
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");    
}

- (void)testShouldNotifyBusListenerWhenNameOwnerChanges
{
    [self.bus registerBusListener:self];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");    
    
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    status = [self.bus requestWellKnownName:kBusAttachmentTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_nameOwnerChangedCompleted], @"The bus listener should have been notified that the name we requested has a new owner now (us).");
    
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busWillStopCompleted], @"The bus listener should have been notified that the bus is stopping.");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");
    
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");
}

- (void)testShouldIndicateThatNameHasOwner
{
    [self.bus registerBusListener:self];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");    
    
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    status = [self.bus requestWellKnownName:kBusAttachmentTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_nameOwnerChangedCompleted], @"The bus listener should have been notified that the name we requested has a new owner now (us).");
    
    BOOL hasOwner = [self.bus doesWellKnownNameHaveOwner:kBusAttachmentTestsAdvertisedName];
    STAssertTrue(hasOwner, @"The doesWellKnownNameHaveOwner message should have returned true after we took ownership of the name.");
    
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busWillStopCompleted], @"The bus listener should have been notified that the bus is stopping.");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");
    
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");
}

- (void)testShouldAllowSessionToBeJoinedByAClient
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
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
    
    status = [self.bus requestWellKnownName:kBusAttachmentTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    status = [self.bus advertiseName:kBusAttachmentTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");
    
    status = [client.bus findAdvertisedName:kBusAttachmentTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Client attempt to find advertised name %@ failed.", kBusAttachmentTestsAdvertisedName);

    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_shouldAcceptSessionJoinerNamed], @"The service did not report that it was queried for acceptance of the client joiner.");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_didJoinInSession], @"The service did not receive a notification that the client joined the session.");
    STAssertTrue(client.clientConnectionCompleted, @"The client did not report that it connected.");
    STAssertTrue(client.testSessionId == self.testSessionId, @"The client session id does not match the service session id.");

    status = [client.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client disconnect from bus via null transport failed.");
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [client.bus stop];
    STAssertTrue(status == ER_OK, @"Client bus failed to stop.");
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([client waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The client bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");    

    [client.bus unregisterBusListener:client];
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");

    [client tearDown];
}

- (void)testShouldNotifyClientWhenLinkIsBroken
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
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
    
    status = [self.bus requestWellKnownName:kBusAttachmentTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    status = [self.bus advertiseName:kBusAttachmentTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");
    
    status = [client.bus findAdvertisedName:kBusAttachmentTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Client attempt to find advertised name %@ failed.", kBusAttachmentTestsAdvertisedName);
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_shouldAcceptSessionJoinerNamed], @"The service did not report that it was queried for acceptance of the client joiner.");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_didJoinInSession], @"The service did not receive a notification that the client joined the session.");
    STAssertTrue(client.clientConnectionCompleted, @"The client did not report that it connected.");
    STAssertTrue(client.testSessionId == self.testSessionId, @"The client session id does not match the service session id.");
    
    uint32_t timeout = 40;
    status = [client.bus setLinkTimeout:&timeout forSession:client.testSessionId];
    STAssertTrue(status == ER_OK, @"Failed to set the link timeout on the client's bus attachment. Error was %@", [AJNStatus descriptionForStatusCode:status]);
    timeout = 40;
    status = [self.bus setLinkTimeout:&timeout forSession:self.testSessionId];
    STAssertTrue(status == ER_OK, @"Failed to set the link timeout on the service's bus attachment. Error was %@", [AJNStatus descriptionForStatusCode:status]);
    
    status = [client.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client disconnect from bus via null transport failed.");
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [client.bus stop];
    STAssertTrue(status == ER_OK, @"Client bus failed to stop.");
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([client waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The client bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");    
    
    [client.bus unregisterBusListener:client];
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");
    
    [client tearDown];
}

- (void)testShouldAllowSessionToBeAsynchronouslyJoinedByAClientUsingBlock
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    client.isAsyncTestClientBlock = YES;
    
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
    
    status = [self.bus requestWellKnownName:kBusAttachmentTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    status = [self.bus advertiseName:kBusAttachmentTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");
    
    status = [client.bus findAdvertisedName:kBusAttachmentTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Client attempt to find advertised name %@ failed.", kBusAttachmentTestsAdvertisedName);
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_shouldAcceptSessionJoinerNamed], @"The service did not report that it was queried for acceptance of the client joiner.");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_didJoinInSession], @"The service did not receive a notification that the client joined the session.");
    STAssertTrue(client.clientConnectionCompleted, @"The client did not report that it connected.");
    STAssertTrue(client.testSessionId == self.testSessionId, @"The client session id does not match the service session id.");
    
    status = [client.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client disconnect from bus via null transport failed.");
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [client.bus stop];
    STAssertTrue(status == ER_OK, @"Client bus failed to stop.");
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([client waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The client bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");    
    
    [client.bus unregisterBusListener:client];
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");
    
    [client tearDown];
}

- (void)testShouldAllowSessionToBeAsynchronouslyJoinedByAClientUsingDelegate
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    client.isAsyncTestClientDelegate = YES;
    
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
    
    status = [self.bus requestWellKnownName:kBusAttachmentTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    status = [self.bus advertiseName:kBusAttachmentTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");
    
    status = [client.bus findAdvertisedName:kBusAttachmentTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Client attempt to find advertised name %@ failed.", kBusAttachmentTestsAdvertisedName);
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_shouldAcceptSessionJoinerNamed], @"The service did not report that it was queried for acceptance of the client joiner.");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_didJoinInSession], @"The service did not receive a notification that the client joined the session.");
    STAssertTrue(client.clientConnectionCompleted, @"The client did not report that it connected.");
    STAssertTrue(client.testSessionId == self.testSessionId, @"The client session id does not match the service session id.");
    
    status = [client.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client disconnect from bus via null transport failed.");
    status = [self.bus disconnectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Disconnect from bus via null transport failed.");
    
    status = [client.bus stop];
    STAssertTrue(status == ER_OK, @"Client bus failed to stop.");
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([self waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([client waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The client bus listener should have been notified that the bus is stopping.");    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_busDidDisconnectCompleted], @"The bus listener should have been notified that the bus was disconnected.");    
    
    [client.bus unregisterBusListener:client];
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");
    
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
    if ([name compare:kBusAttachmentTestsAdvertisedName] == NSOrderedSame) {
        self.didFindAdvertisedNameCompleted = YES;
        if (self.isTestClient) {
            
            [self.bus enableConcurrentCallbacks];
            
            AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
            
            self.testSessionId = [self.bus joinSessionWithName:name onPort:kBusAttachmentTestsServicePort withDelegate:self options:sessionOptions];
            STAssertTrue(self.testSessionId != -1, @"Test client failed to connect to the service %@ on port %u", name, kBusAttachmentTestsServicePort);
            
            self.clientConnectionCompleted = YES;
        }
        else if (self.isAsyncTestClientBlock) {
            
            [self.bus enableConcurrentCallbacks];
            
            AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
            
            [self.bus joinSessionAsyncWithName:name onPort:kBusAttachmentTestsServicePort withDelegate:self options:sessionOptions joinCompletedBlock:^(QStatus status, AJNSessionId sessionId, AJNSessionOptions *opts, void *context) {
                self.testSessionId = sessionId;
                STAssertTrue(self.testSessionId != -1, @"Test client failed to connect asynchronously using block to the service on port %u", kBusAttachmentTestsServicePort);
                
                self.clientConnectionCompleted = YES;            

            } context:nil];
        }
        else if (self.isAsyncTestClientDelegate) {
            
            [self.bus enableConcurrentCallbacks];
            
            AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
            
            [self.bus joinSessionAsyncWithName:name onPort:kBusAttachmentTestsServicePort withDelegate:self options:sessionOptions joinCompletedDelegate:self context:nil];            
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
    if ([name compare:kBusAttachmentTestsAdvertisedName] == NSOrderedSame) {
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
    if (sessionPort == kBusAttachmentTestsServicePort) {
        self.shouldAcceptSessionJoinerNamed = YES;
        return YES;
    }
    return NO;
}

- (void)didJoin:(NSString*)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);    
    if (sessionPort == kBusAttachmentTestsServicePort) {
        self.testSessionId = sessionId;
        self.didJoinInSession = YES;
    }
}

#pragma mark - AJNSessionDelegate implementation

- (void)didJoinSession:(AJNSessionId)sessionId status:(QStatus)status sessionOptions:(AJNSessionOptions *)sessionOptions context:(AJNHandle)context
{
    self.testSessionId = sessionId;
    STAssertTrue(self.testSessionId != -1, @"Test client failed to connect asynchronously using delegate to the service on port %u", kBusAttachmentTestsServicePort);
    
    self.clientConnectionCompleted = YES;            
    
}

@end
