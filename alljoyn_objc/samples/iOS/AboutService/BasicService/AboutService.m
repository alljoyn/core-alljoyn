////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#import "AboutService.h"
#import "AJNBusAttachment.h"
#import "AJNInterfaceDescription.h"
#import "AJNSessionOptions.h"
#import "AJNAboutDataListener.h"
#import "AJNAboutObject.h"
#import "AJNVersion.h"
#import "BasicObject.h"

////////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static NSString* kAboutServiceInterfaceName = @"org.alljoyn.bus.sample";
static NSString* kAboutServiceName = @"org.alljoyn.bus.sample";
static NSString* kAboutServicePath = @"/sample";
static const AJNSessionPort kAboutServicePort = 25;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Basic Service private interface
//

@interface AboutService() <AJNBusListener, AJNSessionPortListener, AJNSessionListener, AJNAboutDataListener>

@property (nonatomic, strong) AJNBusAttachment *bus;
@property (nonatomic, strong) BasicObject *basicObject;
@property (nonatomic, strong) NSCondition *lostSessionCondition;

- (void)run;

@end

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Basic Service implementation
//

@implementation AboutService

@synthesize bus = _bus;
@synthesize basicObject = _basicObject;
@synthesize lostSessionCondition = _lostSessionCondition;
@synthesize delegate = _delegate;

- (void)startService
{
    dispatch_queue_t serviceQueue = dispatch_queue_create("org.alljoyn.basic-service.serviceQueue", NULL);
    dispatch_async( serviceQueue, ^{
        [self run];
    });
    
}

- (void)run
{
    
    NSLog(@"AllJoyn Library version: %@", AJNVersion.versionInformation);
    NSLog(@"AllJoyn Library build info: %@\n", AJNVersion.buildInformation);
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library version: %@\n", [AJNVersion versionInformation]]];
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"AllJoyn Library build info: %@\n", [AJNVersion buildInformation]]];

    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Creating bus attachment                                                                 +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    
    QStatus status;
    
    // create the message bus
    //
    self.bus = [[AJNBusAttachment alloc] initWithApplicationName:@"BasicService" allowRemoteMessages:YES];
    
    self.lostSessionCondition = [[NSCondition alloc] init];
    [self.lostSessionCondition lock];

    // register a bus listener
    //
    [self.bus registerBusListener:self];

    // allocate the service's bus object, which also creates and activates the service's interface
    //
    self.basicObject = [[BasicObject alloc] initWithBusAttachment:self.bus onPath:kAboutServicePath];
    
    // start the message bus
    //
    status =  [self.bus start];
    
    if (ER_OK != status) {
        [self.delegate didReceiveStatusUpdateMessage:@"BusAttachment::Start failed\n"];
        
        NSLog(@"Bus start failed.");
    }
    
    // register the bus object
    //
    status = [self.bus registerBusObject:self.basicObject];
    if (ER_OK != status) {
        NSLog(@"ERROR: Could not register bus object");
    }
    
    [self.delegate didReceiveStatusUpdateMessage:@"Object registered successfully.\n"];
    
    // connect to the message bus
    //
    status = [self.bus connectWithArguments:@"null:"];
    
    if (ER_OK != status) {
        NSLog(@"Bus connect failed.");
        [self.delegate didReceiveStatusUpdateMessage:@"Failed to connect to null: transport"];        
    }
    
    [self.delegate didReceiveStatusUpdateMessage:@"Bus now connected to null: transport\n"];            
    
    // Advertise this service on the bus
    // There are three steps to advertising this service on the bus
    //   1) Request a well-known name that will be used by the client to discover
    //       this service
    //   2) Create a session
    //   3) Advertise the well-known name
    //

    // request the name
    //
    status = [self.bus requestWellKnownName:kAboutServiceName withFlags:kAJNBusNameFlagReplaceExisting | kAJNBusNameFlagDoNotQueue];
    if (ER_OK != status) {
        NSLog(@"ERROR: Request for name failed (%@)", kAboutServiceName);
    }
    
    
    // bind a session to a service port
    //
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:YES proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    
    status = [self.bus bindSessionOnPort:kAboutServicePort withOptions:sessionOptions withDelegate:self];
    if (ER_OK != status) {
        NSLog(@"ERROR: Could not bind session on port (%d)", kAboutServicePort);
    }
    
    AJNAboutObject *aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:self.bus withAnnounceFlag:ANNOUNCED];
    status = [aboutObj announceForSessionPort:kAboutServicePort withAboutDataListener:self];
    if (ER_OK != status) {
        NSLog(@"Error");
    }
    [self.delegate didReceiveStatusUpdateMessage:@"-------------\n"];
    [self.delegate didReceiveStatusUpdateMessage:@"Announce sent\n"];
    [self.delegate didReceiveStatusUpdateMessage:@"-------------\n"];
    
    // wait until the client leaves before tearing down the service
    //
    [self.lostSessionCondition waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:60]];
    
    // clean up
    //
    [self.bus unregisterBusObject:self.basicObject];
    
    [self.lostSessionCondition unlock];
    
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"+ Destroying bus attachment                                                               +");
    NSLog(@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    
    // deallocate the bus
    //
    self.bus = nil;
}

#pragma mark - AJNBusListener delegate methods

- (QStatus)getAboutDataForLanguage:(NSString *)language usingDictionary:(NSMutableDictionary **)aboutData
{
    [self.delegate didReceiveStatusUpdateMessage:@"Inside getAboutDataForLanguage\n"];

    QStatus status = ER_OK;
    *aboutData = [[NSMutableDictionary alloc] initWithCapacity:16];
    
    AJNMessageArgument *appID = [[AJNMessageArgument alloc] init];
    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    [appID setValue:@"ay", sizeof(originalAppId) / sizeof(originalAppId[0]), originalAppId];
    [appID stabilize];
    [*aboutData setValue:appID forKey:@"AppId"];
    
    AJNMessageArgument *defaultLang = [[AJNMessageArgument alloc] init];
    [defaultLang setValue:@"s", "en"];
    [defaultLang stabilize];
    [*aboutData setValue:defaultLang forKey:@"DefaultLanguage"];
    
    AJNMessageArgument *deviceName = [[AJNMessageArgument alloc] init];
    [deviceName setValue:@"s", "Device Name"];
    [deviceName stabilize];
    [*aboutData setValue:deviceName forKey:@"DeviceName"];
    
    AJNMessageArgument *deviceId = [[AJNMessageArgument alloc] init];
    [deviceId setValue:@"s", "avec-awe1213-1234559xvc123"];
    [deviceId stabilize];
    [*aboutData setValue:deviceId forKey:@"DeviceId"];
    
    AJNMessageArgument *appName = [[AJNMessageArgument alloc] init];
    [appName setValue:@"s", "App Name"];
    [appName stabilize];
    [*aboutData setValue:appName forKey:@"AppName"];
    
    AJNMessageArgument *manufacturer = [[AJNMessageArgument alloc] init];
    [manufacturer setValue:@"s", "Manufacturer"];
    [manufacturer stabilize];
    [*aboutData setValue:manufacturer forKey:@"Manufacturer"];
    
    AJNMessageArgument *modelNo = [[AJNMessageArgument alloc] init];
    [modelNo setValue:@"s", "ModelNo"];
    [modelNo stabilize];
    [*aboutData setValue:modelNo forKey:@"ModelNumber"];
    
    AJNMessageArgument *supportedLang = [[AJNMessageArgument alloc] init];
    const char *supportedLangs[] = {"en", "foo"};
    [supportedLang setValue:@"as", 1, supportedLangs];
    [supportedLang stabilize];
    [*aboutData setValue:supportedLang forKey:@"SupportedLanguages"];
    
    AJNMessageArgument *description = [[AJNMessageArgument alloc] init];
    [description setValue:@"s", "Description"];
    [description stabilize];
    [*aboutData setValue:description forKey:@"Description"];
    
    AJNMessageArgument *dateOfManufacture = [[AJNMessageArgument alloc] init];
    [dateOfManufacture setValue:@"s", "1-1-2014"];
    [dateOfManufacture stabilize];
    [*aboutData setValue:dateOfManufacture forKey:@"DateOfManufacture"];
    
    AJNMessageArgument *softwareVersion = [[AJNMessageArgument alloc] init];
    [softwareVersion setValue:@"s", "1.0"];
    [softwareVersion stabilize];
    [*aboutData setValue:softwareVersion forKey:@"SoftwareVersion"];
    
    AJNMessageArgument *ajSoftwareVersion = [[AJNMessageArgument alloc] init];
    [ajSoftwareVersion setValue:@"s", "14.12"];
    [ajSoftwareVersion stabilize];
    [*aboutData setValue:ajSoftwareVersion forKey:@"AJSoftwareVersion"];
    
    AJNMessageArgument *hwSoftwareVersion = [[AJNMessageArgument alloc] init];
    [hwSoftwareVersion setValue:@"s", "14.12"];
    [hwSoftwareVersion stabilize];
    [*aboutData setValue:hwSoftwareVersion forKey:@"HardwareVersion"];
    
    AJNMessageArgument *supportURL = [[AJNMessageArgument alloc] init];
    [supportURL setValue:@"s", "some.random.url"];
    [supportURL stabilize];
    [*aboutData setValue:supportURL forKey:@"SupportUrl"];
    
    return status;
}

-(QStatus)getDefaultAnnounceData:(NSMutableDictionary **)aboutData
{
    [self.delegate didReceiveStatusUpdateMessage:@"Inside getDefaultAnnounceData\n"];

    QStatus status = ER_OK;
    *aboutData = [[NSMutableDictionary alloc] initWithCapacity:16];
    
    AJNMessageArgument *appID = [[AJNMessageArgument alloc] init];
    uint8_t originalAppId[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    [appID setValue:@"ay", sizeof(originalAppId) / sizeof(originalAppId[0]), originalAppId];
    [appID stabilize];
    [*aboutData setValue:appID forKey:@"AppId"];
    
    AJNMessageArgument *defaultLang = [[AJNMessageArgument alloc] init];
    [defaultLang setValue:@"s", "en"];
    [defaultLang stabilize];
    [*aboutData setValue:defaultLang forKey:@"DefaultLanguage"];
    
    AJNMessageArgument *deviceName = [[AJNMessageArgument alloc] init];
    [deviceName setValue:@"s", "Device Name"];
    [deviceName stabilize];
    [*aboutData setValue:deviceName forKey:@"DeviceName"];
    
    AJNMessageArgument *deviceId = [[AJNMessageArgument alloc] init];
    [deviceId setValue:@"s", "avec-awe1213-1234559xvc123"];
    [deviceId stabilize];
    [*aboutData setValue:deviceId forKey:@"DeviceId"];
    
    AJNMessageArgument *appName = [[AJNMessageArgument alloc] init];
    [appName setValue:@"s", "App Name"];
    [appName stabilize];
    [*aboutData setValue:appName forKey:@"AppName"];
    
    AJNMessageArgument *manufacturer = [[AJNMessageArgument alloc] init];
    [manufacturer setValue:@"s", "Manufacturer"];
    [manufacturer stabilize];
    [*aboutData setValue:manufacturer forKey:@"Manufacturer"];
    
    AJNMessageArgument *modelNo = [[AJNMessageArgument alloc] init];
    [modelNo setValue:@"s", "ModelNo"];
    [modelNo stabilize];
    [*aboutData setValue:modelNo forKey:@"ModelNumber"];
    
    AJNMessageArgument *supportedLang = [[AJNMessageArgument alloc] init];
    const char *supportedLangs[] = {"en"};
    [supportedLang setValue:@"as", 1, supportedLangs];
    [supportedLang stabilize];
    [*aboutData setValue:supportedLang forKey:@"SupportedLanguages"];
    
    AJNMessageArgument *description = [[AJNMessageArgument alloc] init];
    [description setValue:@"s", "Description"];
    [description stabilize];
    [*aboutData setValue:description forKey:@"Description"];
    
    AJNMessageArgument *dateOfManufacture = [[AJNMessageArgument alloc] init];
    [dateOfManufacture setValue:@"s", "1-1-2014"];
    [dateOfManufacture stabilize];
    [*aboutData setValue:dateOfManufacture forKey:@"DateOfManufacture"];
    
    AJNMessageArgument *softwareVersion = [[AJNMessageArgument alloc] init];
    [softwareVersion setValue:@"s", "1.0"];
    [softwareVersion stabilize];
    [*aboutData setValue:softwareVersion forKey:@"SoftwareVersion"];
    
    AJNMessageArgument *ajSoftwareVersion = [[AJNMessageArgument alloc] init];
    [ajSoftwareVersion setValue:@"s", "14.12"];
    [ajSoftwareVersion stabilize];
    [*aboutData setValue:ajSoftwareVersion forKey:@"AJSoftwareVersion"];
    
    AJNMessageArgument *hwSoftwareVersion = [[AJNMessageArgument alloc] init];
    [hwSoftwareVersion setValue:@"s", "14.12"];
    [hwSoftwareVersion stabilize];
    [*aboutData setValue:hwSoftwareVersion forKey:@"HardwareVersion"];
    
    AJNMessageArgument *supportURL = [[AJNMessageArgument alloc] init];
    [supportURL setValue:@"s", "some.random.url"];
    [supportURL stabilize];
    [*aboutData setValue:supportURL forKey:@"SupportUrl"];
    
    return status;
}

- (void)listenerDidRegisterWithBus:(AJNBusAttachment*)busAttachment
{
    NSLog(@"AJNBusListener::listenerDidRegisterWithBus:%@",busAttachment);
}

- (void)listenerDidUnregisterWithBus:(AJNBusAttachment*)busAttachment
{
    NSLog(@"AJNBusListener::listenerDidUnregisterWithBus:%@",busAttachment);
}

- (void)didFindAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSLog(@"AJNBusListener::didFindAdvertisedName:%@ withTransportMask:%u namePrefix:%@", name, transport, namePrefix);
}

- (void)didLoseAdvertisedName:(NSString*)name withTransportMask:(AJNTransportMask)transport namePrefix:(NSString*)namePrefix
{
    NSLog(@"AJNBusListener::listenerDidUnregisterWithBus:%@ withTransportMask:%u namePrefix:%@",name,transport,namePrefix);
}

- (void)nameOwnerChanged:(NSString*)name to:(NSString*)newOwner from:(NSString*)previousOwner
{
    NSLog(@"AJNBusListener::nameOwnerChanged:%@ to:%@ from:%@", name, newOwner, previousOwner);
}

- (void)busWillStop
{
    NSLog(@"AJNBusListener::busWillStop");
}

- (void)busDidDisconnect
{
    NSLog(@"AJNBusListener::busDidDisconnect");
}

#pragma mark - AJNSessionListener methods

- (void)sessionWasLost:(AJNSessionId)sessionId
{
    NSLog(@"AJNBusListener::sessionWasLost %u", sessionId);
    
    [self.lostSessionCondition lock];
    [self.lostSessionCondition signal];
    [self.lostSessionCondition unlock];
}


- (void)sessionWasLost:(AJNSessionId)sessionId forReason:(AJNSessionLostReason)reason
{
    NSLog(@"AJNBusListener::sessionWasLost %u forReason:%u", sessionId, reason);
    [self.lostSessionCondition lock];
    [self.lostSessionCondition signal];
    [self.lostSessionCondition unlock];
}

- (void)didAddMemberNamed:(NSString*)memberName toSession:(AJNSessionId)sessionId
{
    NSLog(@"AJNBusListener::didAddMemberNamed:%@ toSession:%u", memberName, sessionId);
}

- (void)didRemoveMemberNamed:(NSString*)memberName fromSession:(AJNSessionId)sessionId
{
    NSLog(@"AJNBusListener::didRemoveMemberNamed:%@ fromSession:%u", memberName, sessionId);
}

#pragma mark - AJNSessionPortListener implementation

- (BOOL)shouldAcceptSessionJoinerNamed:(NSString*)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions*)options
{
    NSLog(@"AJNSessionPortListener::shouldAcceptSessionJoinerNamed:%@ onSessionPort:%u withSessionOptions:", joiner, sessionPort);
    BOOL shouldAcceptSessionJoiner = kAboutServicePort == sessionPort;
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"Request from %@ to join session is %@.\n", joiner, shouldAcceptSessionJoiner ? @"accepted" : @"rejected"]];
    return shouldAcceptSessionJoiner;
}

- (void)didJoin:(NSString*)joiner inSessionWithId:(AJNSessionId)sessionId onSessionPort:(AJNSessionPort)sessionPort
{
    NSLog(@"AJNSessionPortListener::didJoin:%@ inSessionWithId:%u onSessionPort:%u withSessionOptions:", joiner, sessionId, sessionPort);
    [self.bus enableConcurrentCallbacks];
    [self.bus bindSessionListener:self toSession:sessionId];
    [self.delegate didReceiveStatusUpdateMessage:[NSString stringWithFormat:@"%@ successfully joined session %u on port %d.\n", joiner, sessionId, sessionPort]];
    
}

@end
