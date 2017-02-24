////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
//    
//    SPDX-License-Identifier: Apache-2.0
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//    
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//    
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//    
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import "EventsActionsTests.h"
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"
#import "AJNAboutDataListener.h"
#import "AJNAboutObject.h"
#import "AJNAboutProxy.h"
#import "AJNAboutObjectDescription.h"
#import "EventsActionsObject.h"
#import "BasicObject.h"
#import "AJNMessageArgument.h"
#import "AJNMessage.h"
#import "AJNInit.h"
#import "AJNBusObject.h"
#import "AJNProxyBusObject.h"
#import "AJNTranslator.h"
#import "AJNAboutData.h"

BOOL announceFlag = NO;
static NSString * const eaObjectPath = @"/events_actions";
static NSString * const EVENTS_ACTIONS_SERVICE_NAME = @"org.alljoyn.test.eventsactions";
const NSInteger eaServicePort = 25;
static NSMutableDictionary *gDefaultAboutData;


static NSString * const BUS_OBJECT_DESCRIPTION = @"This is the bus object";
static NSString * const INTERFACE_DESCRIPTION = @"This is the interface";
static NSString * const ACTION_DESCRIPTION = @"This is the test action";
static NSString * const EVENT_DESCRIPTION = @"This is the test event";
static NSString * const PROPERTY_DESCRIPTION = @"This is property description";

static NSString * const BUS_OBJECT_DESCRIPTION_ES = @"es: This is the bus object";
static NSString * const INTERFACE_DESCRIPTION_ES = @"es: This is the interface";
static NSString * const ACTION_DESCRIPTION_ES = @"es: This is the test action";
static NSString * const EVENT_DESCRIPTION_ES = @"es: This is the test event";
static NSString * const PROPERTY_DESCRIPTION_ES = @"es: This is property description";



/* TODO: add unit tests for check full description (new uni description)
    The following test cases from regression test plan are covered
    001		Global translator and Client using empty string to Introspect
    002		Global translator and Client using unsupported language to Introspect
    003		Global translator and Client using supported langauge to Introspect
    004		Bus Object level translator and Client using empty string to Introspect
    005		Bus Object level translator and Client using supported langauge to Introspect
    006		Interface level translator and Client using empty string to Introspect
    007		Interface level translator and Client using supported langauge to Introspect
    008		No Translator.
    009		Service side sets description Client introspect with no description
    010		Sessionless flag.
*/

/*
 * The XML representation of the EventsActionsObject used
 *
 <xml>
     <node name="org/alljoyn/Bus/sample">
         <annotation name="org.alljoyn.lang.objc" value="EventsActionsObject"/>
         <description>This is the bus object</description>
         <interface name="org.alljoyn.bus.sample">
             <annotation name="org.alljoyn.lang.objc" value="EventsActionsObjectDelegate"/>
             <annotation name="org.alljoyn.lang.objc.announced" value="true"/>
             <description>This is the interface</description>
             <method name="TestAction">
                 <description>This is the test action</description>
                 <arg name="str1" type="s" direction="in">
                     <annotation name="org.alljoyn.lang.objc" value="concatenateString:"/>
                 </arg>
                 <arg name="str2" type="s" direction="in">
                     <annotation name="org.alljoyn.lang.objc" value="withString:"/>
                 </arg>
                 <arg name="outStr" type="s" direction="out"/>
             </method>
             <signal name="TestEvent" sessionless='true'>
                 <description>This is the test event</description>
                 <arg name="outStr" type="s">
                     <annotation name="org.alljoyn.lang.objc" value="testEventString:"/>
                 </arg>
             </signal>
             <property name="TestProperty" type="s" access="read">
                 <description language="en">This is property description</description>
             </property>
         </interface>
    </node>
 </xml>
*/

/*
 Seems it's redundant here because of deprecating of IntrospectWithLanguege interface
@interface GlobalTranslator : NSObject<AJNTranslator>

- (size_t)numTargetLanguages;
- (NSString*)getTargetLanguage:(size_t)index;
- (NSString*)translateText:(NSString*)text from:(NSString*)fromLang to:(NSString*)toLang;

@end

@implementation GlobalTranslator
- (size_t)numTargetLanguages
{
    NSLog(@"numTargetLanguages called");
    return 1;
}

- (NSString*)getTargetLanguage:(size_t)index
{
    NSLog(@"getTargetLanguage called");
    return @"en";
}

- (NSString*)targetLanguage:(size_t)index {
    NSLog(@"targetLanguage called");
    return @"en";
}

- (NSString*)bestLanguage:(NSString*)requested useDefaultLanguage:(NSString*)defaultLanguage {
    return @"en";
}

- (NSString*)translateText:(NSString*)text from:(NSString*)fromLang to:(NSString*)toLang
{
    NSLog(@"Global Translator called for text %@ from language %@ to language %@", text, fromLang, toLang);

    if([toLang isEqualToString:(@"abc")] || [toLang isEqualToString:@""] || [toLang isEqualToString:@"en"])
    {
        return text;
    }

    if([toLang isEqualToString:@"es"])
    {
        NSString *es = @"es: ";
        return [es stringByAppendingString:text];
    }

    return nil;
}

@end

@interface BusObjectLevelTranslator : NSObject<AJNTranslator>

- (size_t)numTargetLanguages;
- (NSString*)getTargetLanguage:(size_t)index;
- (NSString*)translateText:(NSString*)text from:(NSString*)fromLang to:(NSString*)toLang;

@end

@implementation BusObjectLevelTranslator
- (size_t)numTargetLanguages
{
    NSLog(@"numTargetLanguages called");
    return 1;
}

- (NSString*)getTargetLanguage:(size_t)index
{
    NSLog(@"getTargetLanguage called");
    return @"en";
}

- (NSString*)targetLanguage:(size_t)index {
    NSLog(@"targetLanguage called");
    return @"en";
}

- (NSString*)bestLanguage:(NSString*)requested useDefaultLanguage:(NSString*)defaultLanguage {
    return @"en";
}

- (NSString*)translateText:(NSString*)text from:(NSString*)fromLang to:(NSString*)toLang
{
    NSLog(@"Bus Object level Translator called for text %@ from language %@ to language %@", text, fromLang, toLang);

    if([toLang isEqualToString:(@"abc")] || [toLang isEqualToString:@""] || [toLang isEqualToString:@"en"])
    {
        return text;
    }

    if([toLang isEqualToString:@"es"])
    {
        NSString *es = @"es: ";
        return [es stringByAppendingString:text];
    }

    return nil;
}

@end

@interface InterfaceLevelTranslator : NSObject<AJNTranslator>

- (size_t)numTargetLanguages;
- (NSString*)getTargetLanguage:(size_t)index;
- (NSString*)translateText:(NSString*)text from:(NSString*)fromLang to:(NSString*)toLang;

@end

@implementation InterfaceLevelTranslator
- (size_t)numTargetLanguages
{
    NSLog(@"numTargetLanguages called");
    return 1;
}

- (NSString*)getTargetLanguage:(size_t)index
{
    NSLog(@"getTargetLanguage called");
    return @"en";
}

- (NSString*)targetLanguage:(size_t)index {
    NSLog(@"targetLanguage called");
    return @"en";
}

- (NSString*)bestLanguage:(NSString*)requested useDefaultLanguage:(NSString*)defaultLanguage {
    return @"en";
}

- (NSString*)translateText:(NSString*)text from:(NSString*)fromLang to:(NSString*)toLang
{
    NSLog(@"Interface Level Translator called for text %@ from language %@ to language %@", text, fromLang, toLang);

    if([toLang isEqualToString:(@"abc")] || [toLang isEqualToString:@""] || [toLang isEqualToString:@"en"])
    {
        return text;
    }

    if([toLang isEqualToString:@"es"])
    {
        NSString *es = @"es: ";
        return [es stringByAppendingString:text];
    }

    return nil;
}

@end
*/

@interface EventsActionsTests() <AJNBusListener, AJNSessionListener, AJNSessionPortListener, AJNAboutDataListener, AJNAboutListener>

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

@property (nonatomic) BOOL testNonDefaultUTFLanguage;
@property (nonatomic) AJNMessageArgument *testAboutObjectDescriptionArg;
@property (nonatomic) AJNMessageArgument *testAboutDataArg;
@property (nonatomic) BOOL testAboutObjectDescription;

@property (nonatomic) BOOL testSupportedLanguage;
@property (nonatomic) BOOL testUnsupportedLanguage;


- (BOOL)waitForBusToStop:(NSTimeInterval)timeoutSeconds;
- (BOOL)waitForCompletion:(NSTimeInterval)timeoutSeconds onFlag:(BOOL*)flag;

@end


@implementation EventsActionsTests

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
@synthesize testNonDefaultUTFLanguage = _testNonDefaultUTFLanguage;
@synthesize testAboutObjectDescriptionArg = _testAboutObjectDescriptionArg;
@synthesize testAboutDataArg = _testAboutDataArg;
@synthesize testAboutObjectDescription = _testAboutObjectDescription;


@synthesize testSupportedLanguage = _testSupportedLanguage;
@synthesize testUnsupportedLanguage = _testUnsupportedLanguage;

- (void)setUp
{
    [super setUp];

    [AJNInit alljoynInit];
    [AJNInit alljoynRouterInit];

    [self setUpWithBusAttachement: [[AJNBusAttachment alloc] initWithApplicationName:@"testApp" allowRemoteMessages:YES]];
}

- (void)tearDown
{
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
    announceFlag = NO;
    self.busNameToConnect = nil;
    self.sessionPortToConnect = 0;
    self.testBadAnnounceData = NO;
    self.testMissingAboutDataField = NO;
    self.testMissingAnnounceDataField = NO;
    self.testNonDefaultUTFLanguage = NO;
    self.testAboutObjectDescription = NO;

    self.testSupportedLanguage = NO;
    self.testUnsupportedLanguage = NO;
    self.bus = nil;

    [AJNInit alljoynRouterShutdown];
    [AJNInit alljoynShutdown];

    [super tearDown];
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
    announceFlag = NO;
    self.busNameToConnect = nil;
    self.sessionPortToConnect = 0;
    self.testBadAnnounceData = NO;
    self.testMissingAboutDataField = NO;
    self.testMissingAnnounceDataField = NO;

    self.testNonDefaultUTFLanguage = NO;
    self.testAboutObjectDescription = NO;

    self.testSupportedLanguage = NO;
    self.testUnsupportedLanguage = NO;
}

// Sessionless flag should be reflected in the Introspection XML

- (void)testSessionlessflagInXML
{
    EventsActionsTests *client = [[EventsActionsTests alloc] init];
    [client setUp];

    client.isTestClient = YES;
    client.didReceiveAnnounce = NO;

    // Service
    EventsActionsObject *eaObject = [[EventsActionsObject alloc] initWithBusAttachment:self.bus onPath:eaObjectPath];
    [self.bus registerBusObject:eaObject];

    // Get and mark org.allseen.Introspectable as ANNOUNCED
    // Note: There is an ordering problem here ASACORE-1893 so do not change the order until problem is fixed

    AJNInterfaceDescription *introspectableIntf = [self.bus interfaceWithName:@"org.allseen.Introspectable"];
    QStatus status = [eaObject setAnnounceFlagForInterface:introspectableIntf value:ANNOUNCED];
    XCTAssertTrue(status == ER_OK, @"Could not set ANNOUNCE flag on org.allseen.Introspectable");

    //Service bus
    status = [self.bus start];
    XCTAssertTrue(status == ER_OK, @"Bus failed to start.");
    status = [self.bus connectWithArguments:@"null:"];
    XCTAssertTrue(status == ER_OK, @"Connection to bus via null transport failed.");
    status = [self.bus requestWellKnownName:EVENTS_ACTIONS_SERVICE_NAME withFlags:kAJNBusNameFlagDoNotQueue];
    XCTAssertTrue(status == ER_OK, @"Request name to bus failed.");

    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];

    status = [self.bus bindSessionOnPort:eaServicePort withOptions:sessionOptions withDelegate:self];
    XCTAssertTrue(status == ER_OK, @"Bind session on port %lu failed.", eaServicePort);

    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    [aboutObj announceForSessionPort:eaServicePort withAboutDataListener:self];

    // Client
    [client.bus registerAboutListener:client];
    status = [client.bus start];
    XCTAssertTrue(status == ER_OK, @"Bus for client failed to start.");
    status = [client.bus connectWithArguments:@"null:"];
    XCTAssertTrue(status == ER_OK, @"Client connection to bus via null transport failed.");
    status = [client.bus whoImplementsInterface:@"org.alljoyn.bus.sample"];
    XCTAssertTrue(status == ER_OK, @"Client call to WhoImplements Failed");

    XCTAssertTrue([client waitForCompletion:20 onFlag:&announceFlag], @"The about listener should have been notified that the announce signal is received.");

    // Join session with the service
    AJNSessionId sessionId = 0;
    if(announceFlag) {
        sessionId = [client.bus joinSessionWithName:EVENTS_ACTIONS_SERVICE_NAME onPort:eaServicePort withDelegate:self options:sessionOptions];
        XCTAssertTrue(sessionId != 0, @"Join session failed");
    }

    // Create a proxy bus object for the events and actions object
    AJNProxyBusObject *proxyObject = [[AJNProxyBusObject alloc] initWithBusAttachment:client.bus serviceName:EVENTS_ACTIONS_SERVICE_NAME objectPath:eaObjectPath sessionId:sessionId];

    //Introspect
    status = [proxyObject introspectRemoteObject];
    XCTAssertTrue(status == ER_OK, @"fail");
    BOOL test = [proxyObject implementsInterfaceWithName:@"org.allseen.Introspectable"];
    XCTAssertTrue(test);

    // Call IntrospectWithDescription
    AJNMessage *methodReply =[[AJNMessage alloc] init];
    AJNMessageArgument *language = [[AJNMessageArgument alloc] init];
    [language setValue:@"s", ""];
    [language stabilize];
    NSArray *args = [[NSArray alloc] initWithObjects:language, nil];
    status = [proxyObject callMethodWithName:@"IntrospectWithDescription" onInterfaceWithName:@"org.allseen.Introspectable" withArguments:args methodReply:&methodReply];
    XCTAssertTrue(status == ER_OK, @"Method call IntrospectWithDescription on org.allseen.Introspectable failed %@",[AJNStatus descriptionForStatusCode:status]);

    NSLog(@"Xml representation of message : %@",[methodReply xmlDescription]);
    NSString *xmlStr = [methodReply xmlDescription];

    // Check if sessionless flag is present in the description xml
    NSLog(@"Interface");
    XCTAssertTrue([xmlStr rangeOfString:@"sessionless=\"true\""].location != NSNotFound, @"Sessionless flag is absent");

    [self.bus disconnect];
    [self.bus stop];

    [client.bus disconnect];
    [client.bus stop];

    [client.bus unregisterBusListener:self];
    [client.bus unregisterAllAboutListeners];
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

#pragma mark - AJNAboutListener delegate methods

- (void)didReceiveAnnounceOnBus:(NSString *)busName withVersion:(uint16_t)version withSessionPort:(AJNSessionPort)port withObjectDescription:(AJNMessageArgument *)objectDescriptionArg withAboutDataArg:(AJNMessageArgument *)aboutDataArg
{

    NSLog(@"Received Announce signal from %s Version : %d SessionPort: %d", [busName UTF8String], version, port);

    AJNAboutObjectDescription *testInitWithMsgArg = [[AJNAboutObjectDescription alloc] initWithMsgArg:objectDescriptionArg];
    XCTAssertNotNil(testInitWithMsgArg, @"Fail");

    AJNAboutObjectDescription *aboutObjectDescription = [[AJNAboutObjectDescription alloc] init];
    [aboutObjectDescription createFromMsgArg:objectDescriptionArg];

    BOOL test = [aboutObjectDescription hasInterface:@"org.alljoyn.bus.sample" withPath:@"/events_actions"];
    XCTAssertTrue(test == YES, @"hasInterface:withPath test failed");

    self.didReceiveAnnounce = YES;
    announceFlag = YES;

}

- (QStatus)getAboutData:(AJNMessageArgument *__autoreleasing *)msgArg withLanguage:(NSString *)language
{
    AJNAboutData *aboutData = [[AJNAboutData alloc] initWithLanguage:@"en"];
    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    [aboutData setAppId:originalAppId];

    [aboutData setDefaultLanguage:@"en"];

    if (self.testBadAnnounceData == YES) {
        [aboutData setDeviceName:@"foo" andLanguage:@"en"];
    } else {
        [aboutData setDeviceName:@"Device Name" andLanguage:@"en"];
    }

    if (self.testMissingAboutDataField == YES) {
        [aboutData setDeviceId:@""];
    } else {
        [aboutData setDeviceId:@"avec-awe1213-1234559xvc123"];
    }

    if (self.testMissingAnnounceDataField == YES) {
        [aboutData setAppName:@"" andLanguage:@"en"];
    } else {
        [aboutData setAppName:@"App Name" andLanguage:@"en"];
    }

    [aboutData setManufacturer:@"Manufacturer" andLanguage:@"en"];

    [aboutData setModelNumber:@"ModelNo"];

    [aboutData setSupportedLanguage:@"en"];
    [aboutData setSupportedLanguage:@"foo"];

    if (self.testNonDefaultUTFLanguage == YES) {
        [aboutData setDescription:@"Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método" andLanguage:@"as"];
    } else {
        [aboutData setDescription:@"Description" andLanguage:@"en"];
    }

    [aboutData setDateOfManufacture:@"1-1-2014"];

    [aboutData setSoftwareVersion:@"1.0"];

    [aboutData setHardwareVersion:@"00.00.01"];

    [aboutData setSupportUrl:@"some.random.url"];

    return [aboutData getAboutData:msgArg withLanguage:language];
}

- (QStatus)getAnnouncedAboutData:(AJNMessageArgument *__autoreleasing *)msgArg
{
    AJNAboutData *aboutData = [[AJNAboutData alloc] initWithLanguage:@"en"];
    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    [aboutData setAppId:originalAppId];

    [aboutData setDefaultLanguage:@"en"];

    [aboutData setDeviceName:@"Device Name" andLanguage:@"en"];

    [aboutData setDeviceId:@"avec-awe1213-1234559xvc123"];

    [aboutData setAppName:@"App Name" andLanguage:@"en"];

    [aboutData setManufacturer:@"Manufacturer" andLanguage:@"en"];

    [aboutData setModelNumber:@"ModelNo"];

    [aboutData setSupportedLanguage:@"en"];

    if (self.testNonDefaultUTFLanguage == YES) {
        [aboutData setDescription:@"Sólo se puede aceptar cadenas distintas de cadenas nada debe hacerse utilizando el método" andLanguage:@"foo"];
    } else {
        [aboutData setDescription:@"Description" andLanguage:@"en"];
    }

    [aboutData setDateOfManufacture:@"1-1-2014"];

    [aboutData setSoftwareVersion:@"1.0"];

    [aboutData setHardwareVersion:@"00.00.01"];

    [aboutData setSupportUrl:@"some.random.url"];

    return [aboutData getAnnouncedAboutData:msgArg];
}

#pragma mark - AJNSessionPortListener implementation

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString*)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions*)options
{
    NSLog(@"AJNSessionPortListener::shouldAcceptSessionJoinerNamed:%@ onSessionPort:%u withSessionOptions:", joiner, sessionPort);
    if (sessionPort == eaServicePort) {
        self.shouldAcceptSessionJoinerNamed = YES;
        return YES;
    }
    return NO;
}

- (void)didJoin:(NSString*)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);
    if (sessionPort == eaServicePort) {
        self.testSessionId = sessionId;
        self.didJoinInSession = YES;
    }
}


@end