////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012, 2014, AllSeen Alliance. All rights reserved.
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
#import "AJNAboutDataListener.h"
#import "AJNAboutObject.h"
#import "AJNAboutProxy.h"
#import "AJNAboutIcon.h"
#import "AJNAboutIconObject.h"
#import "AJNAboutIconProxy.h"
#import "AJNAboutObjectDescription.h"
#import "BasicObject.h"
#import "AJNMessageArgument.h"
#import "AJNMessage.h"



static NSString * const kBusAttachmentTestsAdvertisedName = @"org.alljoyn.bus.objc.tests.AReallyNiftyNameThatNoOneWillUse";
static NSString * const kBusAttachmentTestsInterfaceName = @"org.alljoyn.bus.objc.tests.NNNNNNEEEEEEEERRRRRRRRRRDDDDDDDSSSSSSSS";
static NSString * const kBusAttachmentTestsInterfaceMethod = @"behaveInSociallyAwkwardWay";
static NSString * const kBusAttachmentTestsInterfaceXML = @"<interface name=\"org.alljoyn.bus.objc.tests.NNNNNNEEEEEEEERRRRRRRRRRDDDDDDDSSSSSSSS\">\
                                                                <signal name=\"FigdetingNervously\">\
                                                                    <arg name=\"levelOfAwkwardness\" type=\"s\"/>\
                                                                </signal>\
                                                                <property name=\"nerdiness\" type=\"s\" access=\"read\"/>\
                                                            </interface>";

static NSString * const kBusObjectTestsObjectPath = @"/basic_object";
const NSTimeInterval kBusAttachmentTestsWaitTimeBeforeFailure = 10.0;
const NSInteger kBusAttachmentTestsServicePort = 999;
BOOL receiveAnnounce = NO;
static NSMutableDictionary *gDefaultAboutData;
// MAX_ICON_SIZE_IN_BYTES = ALLJOYN_MAX_ARRAY_LEN
static const size_t MAX_ICON_SIZE_IN_BYTES = 131072;
static const uint8_t ICON_BYTE = 0x11;

@interface BusAttachmentTests() <AJNBusListener, AJNSessionListener, AJNSessionPortListener, AJNSessionDelegate, AJNPingPeerDelegate, AJNAboutDataListener, AJNAboutListener>

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
@property (nonatomic) BOOL isPingAsyncComplete;
@property (nonatomic) BOOL didReceiveAnnounce;
@property (nonatomic) BOOL setInvalidData;
@property (nonatomic) BOOL setInvalidLanguage;
@property (nonatomic) NSString *busNameToConnect;
@property (nonatomic) AJNSessionPort sessionPortToConnect;
@property (nonatomic) BOOL testBadAnnounceData;
@property (nonatomic) BOOL testMissingAboutDataField;
@property (nonatomic) BOOL testMissingAnnounceDataField;
@property (nonatomic) BOOL testUnsupportedLanguage;
@property (nonatomic) BOOL testNonDefaultUTFLanguage;
@property (nonatomic) AJNMessageArgument *testAboutObjectDescriptionArg;
@property (nonatomic) AJNMessageArgument *testAboutDataArg;
@property (nonatomic) BOOL testAboutObjectDescription;


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
@synthesize isPingAsyncComplete = _isPingAsyncComplete;
@synthesize didReceiveAnnounce = _didReceiveAnnounce;
@synthesize setInvalidData = _setInvalidData;
@synthesize setInvalidLanguage = _setInvalidLanguage;
@synthesize busNameToConnect = _busNameToConnect;
@synthesize sessionPortToConnect = _sessionPortToConnect;
@synthesize testBadAnnounceData = _testBadAnnounceData;
@synthesize testMissingAboutDataField = _testMissingAboutDataField;
@synthesize testMissingAnnounceDataField = _testMissingAnnounceDataField;
@synthesize testUnsupportedLanguage = _testUnsupportedLanguage;
@synthesize testNonDefaultUTFLanguage = _testNonDefaultUTFLanguage;
@synthesize testAboutObjectDescriptionArg = _testAboutObjectDescriptionArg;
@synthesize testAboutDataArg = _testAboutDataArg;
@synthesize testAboutObjectDescription = _testAboutObjectDescription;

- (void)setUp
{
    [super setUp];
    
    // Set-up code here. Executed before each test case is run.
    //
    [self setUpWithBusAttachement: [[AJNBusAttachment alloc] initWithApplicationName:@"testApp" allowRemoteMessages:YES]];
}

- (void)setUpWithBusAttachement:(AJNBusAttachment *)busAttachment
{
    self.bus = busAttachment;
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
    self.isPingAsyncComplete = NO;
    self.setInvalidData = NO;
    self.setInvalidLanguage = NO;
    receiveAnnounce = NO;
    self.busNameToConnect = nil;
    self.sessionPortToConnect = nil;
    self.testBadAnnounceData = NO;
    self.testMissingAboutDataField = NO;
    self.testMissingAnnounceDataField = NO;
    self.testUnsupportedLanguage = NO;
    self.testNonDefaultUTFLanguage = NO;
    self.testAboutObjectDescription = NO;
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
    self.isPingAsyncComplete = NO;
    self.setInvalidData = NO;
    self.setInvalidLanguage = NO;
    self.didReceiveAnnounce = NO;
    receiveAnnounce = NO;
    self.busNameToConnect = nil;
    self.sessionPortToConnect = nil;
    self.testBadAnnounceData = NO;
    self.testMissingAboutDataField = NO;
    self.testMissingAnnounceDataField = NO;
    self.testUnsupportedLanguage = NO;
    self.testNonDefaultUTFLanguage = NO;
    self.testAboutObjectDescription = NO;
    
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


- (void)testShouldAllowPeerToBeAsynchronouslyPingedByAClientUsingDelegate
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
    
    STAssertTrue(ER_OK == [self.bus pingPeerAsync:kBusAttachmentTestsAdvertisedName withTimeout:5 completionDelegate:self context:nil], @"PingPeerAsync Failed");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_isPingAsyncComplete], @"The service could not be pinged");
    
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

- (void)testShouldAllowPeerToBeAsynchronouslyPingedByAClientUsingBlock
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
    
    STAssertTrue(ER_OK == [self.bus pingPeerAsync:kBusAttachmentTestsAdvertisedName withTimeout:5
                                  completionBlock:^(QStatus status, void *context) {
                                      NSLog(@"Ping Peer Async callback");
                                      STAssertTrue(status == ER_OK, @"Ping Peer Async failed");
                                      self.isPingAsyncComplete = YES;
                                  }context:nil], @"PingPeerAsync Failed");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_isPingAsyncComplete], @"The service could not be pinged");
    
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

- (void)testShouldAllowServiceToLeaveSelfJoin
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUpWithBusAttachement:self.bus];
    client.isTestClient = YES;
    [client.bus registerBusListener:client];
    [self.bus registerBusListener:self];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    status = [self.bus requestWellKnownName:kBusAttachmentTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    
    status = [self.bus advertiseName:kBusAttachmentTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");
    
    status = [client.bus findAdvertisedName:kBusAttachmentTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Client attempt to find advertised name %@ failed.", kBusAttachmentTestsAdvertisedName);
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_shouldAcceptSessionJoinerNamed], @"The service did not report that it was queried for acceptance of the client joiner.");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_didJoinInSession], @"The service did not receive a notification that the client joined the session.");
    STAssertTrue(client.clientConnectionCompleted, @"The client did not report that it connected.");
    STAssertTrue(client.testSessionId == self.testSessionId, @"The client session id does not match the service session id.");
    
    status = [self.bus bindHostedSessionListener:self toSession:self.testSessionId];
    STAssertTrue(status == ER_OK, @"Binding of a Service sessionlistener failed");
    
    status = [client.bus bindJoinedSessionListener:client toSession:client.testSessionId];
    STAssertTrue(status == ER_OK, @"Binding of a Client sessionlistener failed");
    
    status = [self.bus bindHostedSessionListener:nil toSession:self.testSessionId];
    STAssertTrue(status == ER_OK, @"Removal of the Service sessionlistener failed");
    
    status = [client.bus bindJoinedSessionListener:nil toSession:client.testSessionId];
    STAssertTrue(status == ER_OK, @"Removal of the Client sessionlistener failed");
    
    status = [self.bus leaveHostedSession:self.testSessionId];
    STAssertTrue(status == ER_OK, @"Service failed to leave self joined session");
    
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([client waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");
    STAssertTrue([self waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");
    
    [client.bus unregisterBusListener:client];
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");
    
    [client tearDown];
}

- (void)testShouldAllowClientToLeaveSelfJoin
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUpWithBusAttachement:self.bus];
    client.isTestClient = YES;
    [client.bus registerBusListener:client];
    [self.bus registerBusListener:self];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    status = [self.bus requestWellKnownName:kBusAttachmentTestsAdvertisedName withFlags:kAJNBusNameFlagDoNotQueue|kAJNBusNameFlagReplaceExisting];
    STAssertTrue(status == ER_OK, @"Request for well known name failed.");
    
    status = [self.bus advertiseName:kBusAttachmentTestsAdvertisedName withTransportMask:kAJNTransportMaskAny];
    STAssertTrue(status == ER_OK, @"Advertise name failed.");
    
    status = [client.bus findAdvertisedName:kBusAttachmentTestsAdvertisedName];
    STAssertTrue(status == ER_OK, @"Client attempt to find advertised name %@ failed.", kBusAttachmentTestsAdvertisedName);
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_shouldAcceptSessionJoinerNamed], @"The service did not report that it was queried for acceptance of the client joiner.");
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_didJoinInSession], @"The service did not receive a notification that the client joined the session.");
    STAssertTrue(client.clientConnectionCompleted, @"The client did not report that it connected.");
    STAssertTrue(client.testSessionId == self.testSessionId, @"The client session id does not match the service session id.");
    
    status = [self.bus bindHostedSessionListener:self toSession:self.testSessionId];
    STAssertTrue(status == ER_OK, @"Binding of a Service sessionlistener failed");
    
    status = [client.bus bindJoinedSessionListener:client toSession:client.testSessionId];
    STAssertTrue(status == ER_OK, @"Binding of a Client sessionlistener failed");
    
    status = [client.bus leaveJoinedSession:client.testSessionId];
    STAssertTrue(status == ER_OK, @"Client failed to leave self joined session");
    
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_sessionWasLost], @"The Service was not informed that the session was lost.");
    
    status = [self.bus stop];
    STAssertTrue(status == ER_OK, @"Bus failed to stop.");
    
    STAssertTrue([client waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");
    STAssertTrue([self waitForBusToStop:kBusAttachmentTestsWaitTimeBeforeFailure], @"The bus listener should have been notified that the bus is stopping.");
    
    [client.bus unregisterBusListener:client];
    [self.bus unregisterBusListener:self];
    STAssertTrue([self waitForCompletion:kBusAttachmentTestsWaitTimeBeforeFailure onFlag:&_listenerDidUnregisterWithBusCompleted], @"The bus listener should have been notified that a listener was unregistered.");
    
    [client tearDown];
}

- (void)testShouldReceiveAnnounceSignalAndPrintIt
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    client.isTestClient = YES;
    client.didReceiveAnnounce = NO;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);

    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    
    // Client
    [client.bus registerAboutListener:client];
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:@"org.alljoyn.bus.sample.strings"];
    STAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");
    
    STAssertTrue([client waitForCompletion:20 onFlag:&receiveAnnounce], @"The about listener should have been notified that the announce signal is received.");
    
    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];

    [client.bus disconnectWithArguments:@"null:"];
    [client.bus stop];
    
    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
    [client tearDown];
}

- (void)testShouldReceiveAboutIcon
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    client.isTestClient = YES;
    client.didReceiveAnnounce = NO;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    
    // Client
    [client.bus registerAboutListener:client];
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:@"org.alljoyn.bus.sample.strings"];
    STAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");
    
    STAssertTrue([client waitForCompletion:20 onFlag:&receiveAnnounce], @"The about listener should have been notified that the announce signal is received.");
    
    // Service sets the About Icon
    AJNAboutIcon *aboutIcon = [[AJNAboutIcon alloc] init];
    status = [aboutIcon setUrlWithMimeType:@"image/png" url:@"http://www.example.com"];
    STAssertTrue(status == ER_OK, @"Could not set Url for the About Icon");
    uint8_t aboutIconContent[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00,
        0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x0A,
        0x00, 0x00, 0x00, 0x0A, 0x08, 0x02, 0x00, 0x00, 0x00, 0x02,
        0x50, 0x58, 0xEA, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4D,
        0x41, 0x00, 0x00, 0xAF, 0xC8, 0x37, 0x05, 0x8A, 0xE9, 0x00,
        0x00, 0x00, 0x19, 0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 0x66,
        0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x41, 0x64, 0x6F, 0x62,
        0x65, 0x20, 0x49, 0x6D, 0x61, 0x67, 0x65, 0x52, 0x65, 0x61,
        0x64, 0x79, 0x71, 0xC9, 0x65, 0x3C, 0x00, 0x00, 0x00, 0x18,
        0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x62, 0xFC, 0x3F, 0x95,
        0x9F, 0x01, 0x37, 0x60, 0x62, 0xC0, 0x0B, 0x46, 0xAA, 0x34,
        0x40, 0x80, 0x01, 0x00, 0x06, 0x7C, 0x01, 0xB7, 0xED, 0x4B,
        0x53, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44,
        0xAE, 0x42, 0x60, 0x82 };
    
    [aboutIcon setContentWithMimeType:@"image/png" data:aboutIconContent size:(sizeof(aboutIconContent) / sizeof(aboutIconContent[0])) ownsFlag:false];
    
    // Set AboutIconObject
    STAssertTrue(status == ER_OK, @"Could not set Url for the About Icon");
    AJNAboutIconObject *aboutIconObject = [[AJNAboutIconObject alloc] initWithBusAttachment:self.bus aboutIcon:aboutIcon];
    
    //Client gets the About Icon
    AJNAboutIconProxy *aboutIconProxy = [[AJNAboutIconProxy alloc] initWithBusAttachment:client.bus busName:client.busNameToConnect sessionId:client.testSessionId];
    AJNAboutIcon *clientAboutIcon = [[AJNAboutIcon alloc] init];
    [aboutIconProxy getIcon:clientAboutIcon];

    // Check Url
    STAssertTrue([[clientAboutIcon getUrl] isEqualToString:[aboutIcon getUrl]], @"About Icon Url does not match");
    
    // Check content size
    STAssertTrue([clientAboutIcon getContentSize] == [aboutIcon getContentSize], @"About Icon content size does not match");
    
    // Check About Icon content
    uint8_t *clientAboutIconContent = [clientAboutIcon getContent];
    
    for (size_t i=0 ;i < [clientAboutIcon getContentSize] ; i++) {
        STAssertTrue((clientAboutIconContent[i] == aboutIconContent[i]), @"Mistmatch in About Icon content");
        if (clientAboutIconContent[i] != aboutIconContent[i]) {
            break;
        }
        
    }
    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
    
    [client.bus disconnectWithArguments:@"null:"];
    [client.bus stop];
    
    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
    [client tearDown];
}

- (void)testShouldHandleLargeAboutIcon
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    client.isTestClient = YES;
    client.didReceiveAnnounce = NO;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    
    // Client
    [client.bus registerAboutListener:client];
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:@"org.alljoyn.bus.sample.strings"];
    STAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");
    
    STAssertTrue([client waitForCompletion:20 onFlag:&receiveAnnounce], @"The about listener should have been notified that the announce signal is received.");
    
    // Service sets the About Icon
    AJNAboutIcon *aboutIcon = [[AJNAboutIcon alloc] init];
    status = [aboutIcon setUrlWithMimeType:@"image/png" url:@"http://www.example.com"];
    STAssertTrue(status == ER_OK, @"Could not set Url for the About Icon");

    uint8_t aboutIconContent[MAX_ICON_SIZE_IN_BYTES];
    if (aboutIconContent) {
        for (size_t iconByte = 0; iconByte < MAX_ICON_SIZE_IN_BYTES; iconByte++) {
            aboutIconContent[iconByte] = ICON_BYTE;
        }
    }
    
    status = [aboutIcon setContentWithMimeType:@"image/png" data:aboutIconContent size:(sizeof(aboutIconContent) / sizeof(aboutIconContent[0])) ownsFlag:false];
    
    // Set AboutIconObject
    STAssertTrue(status == ER_OK, @"Could not content for About Icon");
    AJNAboutIconObject *aboutIconObject = [[AJNAboutIconObject alloc] initWithBusAttachment:self.bus aboutIcon:aboutIcon];
    
    //Client gets the About Icon
    AJNAboutIconProxy *aboutIconProxy = [[AJNAboutIconProxy alloc] initWithBusAttachment:client.bus busName:client.busNameToConnect sessionId:client.testSessionId];
    AJNAboutIcon *clientAboutIcon = [[AJNAboutIcon alloc] init];
    [aboutIconProxy getIcon:clientAboutIcon];
    
    // Check Url
    STAssertTrue([[clientAboutIcon getUrl] isEqualToString:[aboutIcon getUrl]], @"About Icon Url does not match");
    
    // Check content size
    STAssertTrue([clientAboutIcon getContentSize] == [aboutIcon getContentSize], @"About Icon content size does not match");
    
    // Check About Icon content
    uint8_t *clientAboutIconContent = [clientAboutIcon getContent];
    
    for (size_t i=0 ;i < [clientAboutIcon getContentSize] ; i++) {
        STAssertTrue((clientAboutIconContent[i] == aboutIconContent[i]), @"Mistmatch in About Icon content");
        if (clientAboutIconContent[i] != aboutIconContent[i]) {
            break;
        }
        
    }
    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
    
    [client.bus disconnectWithArguments:@"null:"];
    [client.bus stop];
    
    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
    [client tearDown];
}

- (void)testShouldFailLargeAboutIcon
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    client.isTestClient = YES;
    client.didReceiveAnnounce = NO;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    
    // Client
    [client.bus registerAboutListener:client];
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:@"org.alljoyn.bus.sample.strings"];
    STAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");
    
    STAssertTrue([client waitForCompletion:20 onFlag:&receiveAnnounce], @"The about listener should have been notified that the announce signal is received.");
    
    // Service sets the About Icon
    AJNAboutIcon *aboutIcon = [[AJNAboutIcon alloc] init];
    status = [aboutIcon setUrlWithMimeType:@"image/png" url:@"http://www.example.com"];
    STAssertTrue(status == ER_OK, @"Could not set Url for the About Icon");
    
    uint8_t aboutIconContent[MAX_ICON_SIZE_IN_BYTES + 2];
    if (aboutIconContent) {
        for (size_t iconByte = 0; iconByte < MAX_ICON_SIZE_IN_BYTES; iconByte++) {
            aboutIconContent[iconByte] = ICON_BYTE;
        }
    }
    
    status = [aboutIcon setContentWithMimeType:@"image/png" data:aboutIconContent size:(sizeof(aboutIconContent) / sizeof(aboutIconContent[0])) ownsFlag:false];
    
    // Set AboutIconObject
    STAssertTrue(status == ER_BUS_BAD_VALUE, @"Could not content for About Icon");
    AJNAboutIconObject *aboutIconObject = [[AJNAboutIconObject alloc] initWithBusAttachment:self.bus aboutIcon:aboutIcon];
    
    //Client gets the About Icon
    AJNAboutIconProxy *aboutIconProxy = [[AJNAboutIconProxy alloc] initWithBusAttachment:client.bus busName:client.busNameToConnect sessionId:client.testSessionId];
    AJNAboutIcon *clientAboutIcon = [[AJNAboutIcon alloc] init];
    [aboutIconProxy getIcon:clientAboutIcon];
    
    // Check Url
    STAssertTrue([[clientAboutIcon getUrl] isEqualToString:[aboutIcon getUrl]], @"About Icon Url does not match");
    
    // Check content size
    STAssertTrue([clientAboutIcon getContentSize] == [aboutIcon getContentSize], @"About Icon content size does not match");
    
    // Check About Icon content
    uint8_t *clientAboutIconContent = [clientAboutIcon getContent];
    
    for (size_t i=0 ;i < [clientAboutIcon getContentSize] ; i++) {
        STAssertTrue((clientAboutIconContent[i] == aboutIconContent[i]), @"Mistmatch in About Icon content");
        if (clientAboutIconContent[i] != aboutIconContent[i]) {
            break;
        }
        
    }
    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
    
    [client.bus disconnectWithArguments:@"null:"];
    [client.bus stop];
    
    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
    [client tearDown];
}


- (void)testShouldHandleInconsistentAnnounceData
{
    self.testBadAnnounceData = YES;

    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];

    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");

    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];

    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);

    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    status = [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    STAssertTrue(status == ER_ABOUT_INVALID_ABOUTDATA_LISTENER, @"Inconsistent about announce and about data should be reported as error");

    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
}

- (void)testShouldReportMissingFieldInAboutData
{
    self.testMissingAboutDataField = YES;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    status = [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    STAssertTrue(status == ER_ABOUT_INVALID_ABOUTDATA_LISTENER, @"Missing about data field should be reported as error");
    
    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
}

- (void)testShouldReportMissingFieldInAnnounceData
{
    self.testMissingAnnounceDataField = YES;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    status = [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    STAssertTrue(status == ER_ABOUT_INVALID_ABOUTDATA_LISTENER, @"Missing about data field should be reported as error");
    
    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
}

- (void)testShouldHandleUnsupportedLanguageForAbout
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    client.isTestClient = YES;
    client.didReceiveAnnounce = NO;
    client.testUnsupportedLanguage= YES;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    
    // Client
    [client.bus registerAboutListener:client];
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:@"org.alljoyn.bus.sample.strings"];
    STAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");
    
    STAssertTrue([client waitForCompletion:20 onFlag:&receiveAnnounce], @"The about listener should have been notified that the announce signal is received.");
    
    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
    
    [client.bus disconnectWithArguments:@"null:"];
    [client.bus stop];
    
    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
    [client tearDown];
}

- (void)testForNonDefaultLanguageAbout
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    client.isTestClient = YES;
    client.didReceiveAnnounce = NO;
    client.testUnsupportedLanguage= YES;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    
    // Client
    [client.bus registerAboutListener:client];
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:@"org.alljoyn.bus.sample.strings"];
    STAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");
    
    STAssertTrue([client waitForCompletion:20 onFlag:&receiveAnnounce], @"The about listener should have been notified that the announce signal is received.");
    
    // Create AboutProxy
    AJNAboutProxy *aboutProxy = [[AJNAboutProxy alloc] initWithBusAttachment:client.bus busName:client.busNameToConnect sessionId:client.testSessionId];
    
    NSMutableDictionary *aboutData;
    status = [aboutProxy getAboutDataForLanguage:@"foo" usingDictionary:&aboutData];
    STAssertTrue(status == ER_OK, @"Non default language should not throw error");

    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
    
    [client.bus disconnectWithArguments:@"null:"];
    [client.bus stop];
    
    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
    [client tearDown];
}

- (void)testForNonDefaultLanguageUTFAbout
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    client.isTestClient = YES;
    client.didReceiveAnnounce = NO;
    client.testNonDefaultUTFLanguage= YES;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    
    // Client
    [client.bus registerAboutListener:client];
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:@"org.alljoyn.bus.sample.strings"];
    STAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");
    
    STAssertTrue([client waitForCompletion:20 onFlag:&receiveAnnounce], @"The about listener should have been notified that the announce signal is received.");
    
    // Create AboutProxy
    AJNAboutProxy *aboutProxy = [[AJNAboutProxy alloc] initWithBusAttachment:client.bus busName:client.busNameToConnect sessionId:client.testSessionId];
    
    NSMutableDictionary *aboutData;
    status = [aboutProxy getAboutDataForLanguage:@"foo" usingDictionary:&aboutData];
    STAssertTrue(status == ER_OK, @"Non default language should not throw error");
    
    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
    
    [client.bus disconnectWithArguments:@"null:"];
    [client.bus stop];
    
    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
    [client tearDown];
}

- (void)testForAboutProxyGetAboutObjectDescription
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    client.isTestClient = YES;
    client.testAboutObjectDescription = YES;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    
    // Client
    [client.bus registerAboutListener:client];
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:@"org.alljoyn.bus.sample.strings"];
    STAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");
    
    STAssertTrue([client waitForCompletion:20 onFlag:&receiveAnnounce], @"The about listener should have been notified that the announce signal is received.");

    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
    
    [client.bus disconnectWithArguments:@"null:"];
    [client.bus stop];
    
    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
    [client tearDown];
}

- (void)testWhoImplementsCallForWildCardPositive
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    client.isTestClient = YES;
    client.didReceiveAnnounce = NO;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    
    // Client
    [client.bus registerAboutListener:client];
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:@"*"];
    STAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");
    
    STAssertTrue([client waitForCompletion:20 onFlag:&receiveAnnounce], @"The about listener should have been notified that the announce signal is received.");
    
    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
    
    [client.bus disconnectWithArguments:@"null:"];
    [client.bus stop];
    
    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
    [client tearDown];
}

- (void)testWhoImplementsCallForNull
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    client.isTestClient = YES;
    client.didReceiveAnnounce = NO;
    
    // Service
    BasicObject *basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kBusObjectTestsObjectPath];
    [self.bus registerBusObject:basicObject];
    
    QStatus status = [self.bus start];
    STAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kBusAttachmentTestsServicePort withOptions:sessionOptions withDelegate:self];
    STAssertTrue(status == ER_OK, @"Bind session on port %u failed.", kBusAttachmentTestsServicePort);
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    [aboutObj announceForSessionPort:kBusAttachmentTestsServicePort withAboutDataListener:self];
    
    // Client
    [client.bus registerAboutListener:client];
    status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:nil];
    STAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");
    
    STAssertTrue([client waitForCompletion:20 onFlag:&receiveAnnounce], @"The about listener should have been notified that the announce signal is received.");
    
    [self.bus disconnectWithArguments:@"null:"];
    [self.bus stop];
    
    [client.bus disconnectWithArguments:@"null:"];
    [client.bus stop];
    
    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
    [client tearDown];
}

- (void)testCancelWhoImplementsMismatch
{
    BusAttachmentTests *client = [[BusAttachmentTests alloc] init];
    [client setUp];
    
    // Client
    [client.bus registerAboutListener:client];
    QStatus status = [client.bus start];
    STAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    STAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:@"org.alljoyn.bus.sample.strings"];
    STAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");
    status = [client.bus cancelWhoImplements:@"org.alljoyn.bus.sample.strings.mismatch"];
    STAssertTrue(status == ER_BUS_MATCH_RULE_NOT_FOUND, @"Test for mismatched CancelWhoImplements Failed");
    
    [client.bus disconnectWithArguments:@"null:"];
    [client.bus stop];
    
    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
    [client tearDown];
}

#pragma mark - AJNAboutListener delegate methods

- (void)didReceiveAnnounceOnBus:(NSString *)busName withVersion:(uint16_t)version withSessionPort:(AJNSessionPort)port withObjectDescription:(AJNMessageArgument *)objectDescriptionArg withAboutDataArg:(AJNMessageArgument *)aboutDataArg
{

    NSLog(@"Received Announce signal from %s Version : %d SessionPort: %d", [busName UTF8String], version, port);

    self.didReceiveAnnounce = YES;
    
    if (self.isTestClient) {
        AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
        [self.bus enableConcurrentCallbacks];
        AJNSessionId sessionId = [self.bus joinSessionWithName:busName onPort:port withDelegate:self options:sessionOptions];
        self.testSessionId = sessionId;
        self.sessionPortToConnect = port;
        self.busNameToConnect = busName;
        
        // Create AboutProxy
        AJNAboutProxy *aboutProxy = [[AJNAboutProxy alloc] initWithBusAttachment:self.bus busName:busName sessionId:sessionId];
        
        // Make a call to GetAboutData and GetVersion
        uint16_t version;
        QStatus status;
        NSMutableDictionary *aboutData;
        [aboutProxy getVersion:&version];
        STAssertTrue(version == 1, @"Version value is incorrect");
        if (self.testUnsupportedLanguage == YES) {
            status = [aboutProxy getAboutDataForLanguage:@"bar" usingDictionary:&aboutData];
            STAssertTrue(status != ER_OK, @"Unsupported language not should throw error");
        } else {
            status = [aboutProxy getAboutDataForLanguage:@"en" usingDictionary:&aboutData];
            STAssertTrue(status == ER_OK, @"Default language not should throw error");
        }
        NSLog(@"Version %d Size %lu", version, [aboutData count]);
        // Verify data by comparing the data that you set with the data that you received
        STAssertTrue([gDefaultAboutData isEqualToDictionary:aboutData], @"The announce data is correct");

        if (self.testAboutObjectDescription == YES) {

            AJNAboutObjectDescription *testInitWithMsgArg = [[AJNAboutObjectDescription alloc] initWithMsgArg:objectDescriptionArg];
            STAssertNotNil(testInitWithMsgArg, @"Fail");

            AJNAboutObjectDescription *aboutObjectDescription = [[AJNAboutObjectDescription alloc] init];
            [aboutObjectDescription createFromMsgArg:objectDescriptionArg];
            STAssertNotNil(aboutObjectDescription, @"Fail");

            BOOL test = [aboutObjectDescription hasPath:"/basic_object"];
            STAssertTrue(test == YES, @"hasPath test failed");

            test = [aboutObjectDescription hasPath:"/basic_"];
            STAssertFalse(test, @"Negative hasPath test failed");

            test = [aboutObjectDescription hasInterface:"org.alljoyn.bus.sample.strings" withPath:"/basic_object"];
            STAssertTrue(test == YES, @"hasInterface:withPath test failed");

            test = [aboutObjectDescription hasInterface:"org.alljoyn.bus.sample.strings" withPath:"/basic_"];
            STAssertFalse(test, @"hasInterface:withPath test failed");

            size_t numPaths = [aboutObjectDescription getPaths:nil withSize:0];
            NSMutableArray *paths = [[NSMutableArray alloc] initWithCapacity:numPaths];
            STAssertTrue([aboutObjectDescription getPaths:&paths withSize:numPaths] == 2, @"getPaths:withSize test failed");

            numPaths = [aboutObjectDescription getInterfacePathsForInterface:@"org.alljoyn.bus.sample.strings" paths:nil numOfPaths:0];
            NSMutableArray *interfacePaths = [[NSMutableArray alloc] initWithCapacity:numPaths];
            STAssertTrue([aboutObjectDescription getInterfacePathsForInterface:@"org.alljoyn.bus.sample.strings" paths:&interfacePaths numOfPaths:1] == 1, @"getPaths:withSize test failed");

            size_t numInterfaces = [aboutObjectDescription getInterfacesForPath:@"/basic_object" interfaces:nil numOfInterfaces:0];
            NSMutableArray *interfaces = [[NSMutableArray alloc] initWithCapacity:numInterfaces];
            STAssertTrue([aboutObjectDescription getInterfacesForPath:@"/basic_object" interfaces:&interfaces numOfInterfaces:2] == 2, @"getInterfacesForPath failed");

        }
        receiveAnnounce = YES;
    }
    
}

#pragma mark - AJNAboutDataListener delegate methods

- (QStatus)getAboutDataForLanguage:(NSString *)language usingDictionary:(NSMutableDictionary **)aboutData
{
    NSLog(@"Inside getAboutDataForLanguage");
    QStatus status = ER_OK;
    *aboutData = [[NSMutableDictionary alloc] initWithCapacity:16];
    gDefaultAboutData = [[NSMutableDictionary alloc] initWithCapacity:16];
    
    AJNMessageArgument *appID = [[AJNMessageArgument alloc] init];
    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    [appID setValue:@"ay", sizeof(originalAppId) / sizeof(originalAppId[0]), originalAppId];
    [appID stabilize];
    [*aboutData setValue:appID forKey:@"AppId"];
    [gDefaultAboutData setValue:appID forKey:@"AppId"];
    
    AJNMessageArgument *defaultLang = [[AJNMessageArgument alloc] init];
    [defaultLang setValue:@"s", "en"];
    [defaultLang stabilize];
    [*aboutData setValue:defaultLang forKey:@"DefaultLanguage"];
    [gDefaultAboutData setValue:defaultLang forKey:@"DefaultLanguage"];
    
    AJNMessageArgument *deviceName = [[AJNMessageArgument alloc] init];
    if (self.testBadAnnounceData == YES) {
        [deviceName setValue:@"s", "foo"];
    } else {
        [deviceName setValue:@"s", "Device Name"];
    }
    [deviceName stabilize];
    [*aboutData setValue:deviceName forKey:@"DeviceName"];
    [gDefaultAboutData setValue:deviceName forKey:@"DeviceName"];
    
    AJNMessageArgument *deviceId = [[AJNMessageArgument alloc] init];
    if (self.testMissingAboutDataField == YES) {
        [deviceId setValue:@"s", ""];
    } else {
        [deviceId setValue:@"s", "avec-awe1213-1234559xvc123"];
    }
    
    [deviceId stabilize];
    [*aboutData setValue:deviceId forKey:@"DeviceId"];
    [gDefaultAboutData setValue:deviceId forKey:@"DeviceId"];
    
    AJNMessageArgument *appName = [[AJNMessageArgument alloc] init];
    if (self.testMissingAnnounceDataField == YES) {
        [appName setValue:@"s", ""];
    } else {
        [appName setValue:@"s", "App Name"];
    }
    
    [appName stabilize];
    [*aboutData setValue:appName forKey:@"AppName"];
    [gDefaultAboutData setValue:appName forKey:@"AppName"];
    
    AJNMessageArgument *manufacturer = [[AJNMessageArgument alloc] init];
    [manufacturer setValue:@"s", "Manufacturer"];
    [manufacturer stabilize];
    [*aboutData setValue:manufacturer forKey:@"Manufacturer"];
    [gDefaultAboutData setValue:manufacturer forKey:@"Manufacturer"];
    
    AJNMessageArgument *modelNo = [[AJNMessageArgument alloc] init];
    [modelNo setValue:@"s", "ModelNo"];
    [modelNo stabilize];
    [*aboutData setValue:modelNo forKey:@"ModelNumber"];
    [gDefaultAboutData setValue:modelNo forKey:@"ModelNumber"];
    
    AJNMessageArgument *supportedLang = [[AJNMessageArgument alloc] init];
    const char *supportedLangs[] = {"en", "foo"};
    [supportedLang setValue:@"as", 1, supportedLangs];
    [supportedLang stabilize];
    [*aboutData setValue:supportedLang forKey:@"SupportedLanguages"];
    [gDefaultAboutData setValue:supportedLang forKey:@"SupportedLanguages"];
    
    AJNMessageArgument *description = [[AJNMessageArgument alloc] init];
    if (self.testNonDefaultUTFLanguage == YES) {
        [description setValue:@"s", "Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método"];
    } else {
        [description setValue:@"s", "Description"];
    }
    [description stabilize];
    [*aboutData setValue:description forKey:@"Description"];
    [gDefaultAboutData setValue:description forKey:@"Description"];
    
    AJNMessageArgument *dateOfManufacture = [[AJNMessageArgument alloc] init];
    [dateOfManufacture setValue:@"s", "1-1-2014"];
    [dateOfManufacture stabilize];
    [*aboutData setValue:dateOfManufacture forKey:@"DateOfManufacture"];
    [gDefaultAboutData setValue:dateOfManufacture forKey:@"DateOfManufacture"];
    
    AJNMessageArgument *softwareVersion = [[AJNMessageArgument alloc] init];
    [softwareVersion setValue:@"s", "1.0"];
    [softwareVersion stabilize];
    [*aboutData setValue:softwareVersion forKey:@"SoftwareVersion"];
    [gDefaultAboutData setValue:softwareVersion forKey:@"SoftwareVersion"];
    
    AJNMessageArgument *ajSoftwareVersion = [[AJNMessageArgument alloc] init];
    [ajSoftwareVersion setValue:@"s", "14.12"];
    [ajSoftwareVersion stabilize];
    [*aboutData setValue:ajSoftwareVersion forKey:@"AJSoftwareVersion"];
    [gDefaultAboutData setValue:ajSoftwareVersion forKey:@"AJSoftwareVersion"];
    
    AJNMessageArgument *hwSoftwareVersion = [[AJNMessageArgument alloc] init];
    [hwSoftwareVersion setValue:@"s", "14.12"];
    [hwSoftwareVersion stabilize];
    [*aboutData setValue:hwSoftwareVersion forKey:@"HardwareVersion"];
    [gDefaultAboutData setValue:hwSoftwareVersion forKey:@"HardwareVersion"];
    
    AJNMessageArgument *supportURL = [[AJNMessageArgument alloc] init];
    [supportURL setValue:@"s", "some.random.url"];
    [supportURL stabilize];
    [*aboutData setValue:supportURL forKey:@"SupportUrl"];
    [gDefaultAboutData setValue:supportURL forKey:@"SupportUrl"];

    return status;
}

-(QStatus)getDefaultAnnounceData:(NSMutableDictionary **)aboutData
{
    NSLog(@"Inside getDefaultAnnounceData");
    QStatus status = ER_OK;
    *aboutData = [[NSMutableDictionary alloc] initWithCapacity:16];
    gDefaultAboutData = [[NSMutableDictionary alloc] initWithCapacity:16];

    AJNMessageArgument *appID = [[AJNMessageArgument alloc] init];
    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    [appID setValue:@"ay", sizeof(originalAppId) / sizeof(originalAppId[0]), originalAppId];
    [appID stabilize];
    [*aboutData setValue:appID forKey:@"AppId"];
    [gDefaultAboutData setValue:appID forKey:@"AppId"];
    
    AJNMessageArgument *defaultLang = [[AJNMessageArgument alloc] init];
    [defaultLang setValue:@"s", "en"];
    [defaultLang stabilize];
    [*aboutData setValue:defaultLang forKey:@"DefaultLanguage"];
    [gDefaultAboutData setValue:defaultLang forKey:@"DefaultLanguage"];

    AJNMessageArgument *deviceName = [[AJNMessageArgument alloc] init];
    [deviceName setValue:@"s", "Device Name"];
    [deviceName stabilize];
    [*aboutData setValue:deviceName forKey:@"DeviceName"];
    [gDefaultAboutData setValue:deviceName forKey:@"DeviceName"];

    AJNMessageArgument *deviceId = [[AJNMessageArgument alloc] init];
    [deviceId setValue:@"s", "avec-awe1213-1234559xvc123"];
    [deviceId stabilize];
    [*aboutData setValue:deviceId forKey:@"DeviceId"];
    [gDefaultAboutData setValue:deviceId forKey:@"DeviceId"];

    AJNMessageArgument *appName = [[AJNMessageArgument alloc] init];
    [appName setValue:@"s", "App Name"];
    [appName stabilize];
    [*aboutData setValue:appName forKey:@"AppName"];
    [gDefaultAboutData setValue:appName forKey:@"AppName"];
    
    AJNMessageArgument *manufacturer = [[AJNMessageArgument alloc] init];
    [manufacturer setValue:@"s", "Manufacturer"];
    [manufacturer stabilize];
    [*aboutData setValue:manufacturer forKey:@"Manufacturer"];
    [gDefaultAboutData setValue:manufacturer forKey:@"Manufacturer"];

    AJNMessageArgument *modelNo = [[AJNMessageArgument alloc] init];
    [modelNo setValue:@"s", "ModelNo"];
    [modelNo stabilize];
    [*aboutData setValue:modelNo forKey:@"ModelNumber"];
    [gDefaultAboutData setValue:modelNo forKey:@"ModelNumber"];

    AJNMessageArgument *supportedLang = [[AJNMessageArgument alloc] init];
    const char *supportedLangs[] = {"en"};
    [supportedLang setValue:@"as", 1, supportedLangs];
    [supportedLang stabilize];
    [*aboutData setValue:supportedLang forKey:@"SupportedLanguages"];
    [gDefaultAboutData setValue:supportedLang forKey:@"SupportedLanguages"];

    AJNMessageArgument *description = [[AJNMessageArgument alloc] init];
    if (self.testNonDefaultUTFLanguage == YES) {
        [description setValue:@"s", "Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método"];
    } else {
        [description setValue:@"s", "Description"];
    }
    [description stabilize];
    [*aboutData setValue:description forKey:@"Description"];
    [gDefaultAboutData setValue:description forKey:@"Description"];
    
    AJNMessageArgument *dateOfManufacture = [[AJNMessageArgument alloc] init];
    [dateOfManufacture setValue:@"s", "1-1-2014"];
    [dateOfManufacture stabilize];
    [*aboutData setValue:dateOfManufacture forKey:@"DateOfManufacture"];
    [gDefaultAboutData setValue:dateOfManufacture forKey:@"DateOfManufacture"];

    AJNMessageArgument *softwareVersion = [[AJNMessageArgument alloc] init];
    [softwareVersion setValue:@"s", "1.0"];
    [softwareVersion stabilize];
    [*aboutData setValue:softwareVersion forKey:@"SoftwareVersion"];
    [gDefaultAboutData setValue:softwareVersion forKey:@"SoftwareVersion"];

    AJNMessageArgument *ajSoftwareVersion = [[AJNMessageArgument alloc] init];
    [ajSoftwareVersion setValue:@"s", "14.12"];
    [ajSoftwareVersion stabilize];
    [*aboutData setValue:ajSoftwareVersion forKey:@"AJSoftwareVersion"];
    [gDefaultAboutData setValue:ajSoftwareVersion forKey:@"AJSoftwareVersion"];

    AJNMessageArgument *hwSoftwareVersion = [[AJNMessageArgument alloc] init];
    [hwSoftwareVersion setValue:@"s", "14.12"];
    [hwSoftwareVersion stabilize];
    [*aboutData setValue:hwSoftwareVersion forKey:@"HardwareVersion"];
    [gDefaultAboutData setValue:hwSoftwareVersion forKey:@"HardwareVersion"];

    AJNMessageArgument *supportURL = [[AJNMessageArgument alloc] init];
    [supportURL setValue:@"s", "some.random.url"];
    [supportURL stabilize];
    [*aboutData setValue:supportURL forKey:@"SupportUrl"];
    [gDefaultAboutData setValue:supportURL forKey:@"SupportUrl"];

    return status;
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

- (void) pingPeerHasStatus:(QStatus)status context:(void *)context
{
    NSLog(@"Ping Peer Async callback");
    STAssertTrue(status == ER_OK, @"Ping Peer Async failed");
    self.isPingAsyncComplete = YES;
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
