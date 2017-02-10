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

#import "DoorConsumerAllJoynService.h"
#import "AJNAboutData.h"
#import "AJNAboutObject.h"
#import "AJNPermissionConfigurator.h"
#import "AJNPermissionConfigurationListener.h"
#import "DefaultECDHEListener.h"
#import "AJNBusAttachment.h"
#import "DoorCommonPCL.h"
#import "AJNGUID.h"
#import "SecureDoor.h"
#import "AJNInit.h"

#import "DoorAboutListener.h"

static NSString *const kKeyxEcdheNull = @"ALLJOYN_ECDHE_NULL";
static NSString *const kKeyxEcdhePsk = @"ALLJOYN_ECDHE_PSK";
static NSString *const kKeyxEcdheDsa = @"ALLJOYN_ECDHE_ECDSA";

static NSString *const kDoorObjectPath = @"/sample/security/Door";
static AJNSessionPort const kPort = 12345;

@interface DoorConsumerAllJoynService () <AJNSessionPortListener, AJNSessionListener, DoorFoundDelegate>

@property (nonatomic, strong) NSCondition* waitCondition;

@property (nonatomic, strong) AJNAboutData *aboutData;
@property (nonatomic, strong) AJNAboutObject *aboutObj;

@property (nonatomic, strong) Door *door;

@property (nonatomic, strong) AJNBusAttachment* busAttachment;

@property (nonatomic, strong) DoorCommonPCL* doorCommonPCL;
@property (nonatomic) DefaultECDHEListener *authListener;

@property (nonatomic, weak) id<AllJoynStatusMessageListener> messageListener;

@property (nonatomic, strong) NSMutableArray<DoorProxy *> *doorProxyMutableArray;

@end

@implementation DoorConsumerAllJoynService

- (id)init
{
    return [self initWithMessageListener:nil];
}

- (id)initWithMessageListener:(id<AllJoynStatusMessageListener>)messageListener
{
    self = [super init];
    if (self != nil) {

        if ([AJNInit alljoynInit] != ER_OK) {
            return nil;
        }
        if ([AJNInit alljoynRouterInit] != ER_OK) {
            [AJNInit alljoynShutdown];
            return nil;
        }

        _waitCondition = [[NSCondition alloc] init];
        _serviceState = STOPED;

        _doorProxyMutableArray = [NSMutableArray array];

        _messageListener = messageListener;
    }
    return self;
}

- (QStatus)startWithName:(NSString *)appName;
{
    if (_serviceState != STOPED) {
        return ER_FAIL;
    }

    if (appName == nil || appName.length == 0) {
        return ER_FAIL;
    }
    
    QStatus status = ER_OK;

//    [_waitCondition lock];
    _appName = appName;

    _busAttachment = [[AJNBusAttachment alloc] initWithApplicationName:appName allowRemoteMessages:YES];

    _aboutData = [[AJNAboutData alloc] initWithLanguage:@"en"];
    _aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:_busAttachment withAnnounceFlag:ANNOUNCED];

    _authListener = [[DefaultECDHEListener alloc] init];

    _doorCommonPCL = [[DoorCommonPCL alloc] initWithBus:_busAttachment];

    status = [_busAttachment start];
    if (status != ER_OK) {
        NSLog(@"Failed to start bus attachment - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [_busAttachment connectWithArguments:@"nil"];
    if (status != ER_OK) {
        NSLog(@"Failed to connect bus attachment - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [self enablePeerSecure];
    if (status != ER_OK) {
        NSLog(@"Failed to enablePeerSecurity - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
//        [_waitCondition unlock]; //TODO:refactor
    }

    AJNGUID128* psk = [[AJNGUID128 alloc] init];
    [_authListener setPSK:psk.bytes secretSize:[AJNGUID128 SIZE]];

    [self setupPermissionConfiguratorWithPsk:psk];
    if (status != ER_OK) {
        NSLog(@"Failed to setup permission configurator - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
//        [_waitCondition unlock]; //TODO:refactor
    }

    AJNApplicationState state;
    status = [_busAttachment.permissionConfigurator getApplicationState:&state];
    if (ER_OK != status) {
        NSLog(@"Failed to getApplicationState - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
//        [_waitCondition unlock]; //TODO:refactor
        return status;
    }

    if (state == CLAIMABLE) {
        NSLog(@"Door provider is not claimed.\n");
        NSLog(@"The provider can be claimed using PSK with an application generated secret.\n");
        NSLog(@"PSK = (%@)\n", [psk description]);
        [_messageListener didReceiveAllJoynStatusMessage:[NSString stringWithFormat:@"PSK = (%@)\n", [psk description]]];
    } else {
        NSLog(@"Unexpected application state. Expected CLAIMABLE, current state = (%u)\n", state);
        return ER_FAIL;
    }

    //host Session
    AJNSessionOptions* opts = [[AJNSessionOptions alloc] init];
    status = [_busAttachment bindSessionOnPort:kPort withOptions:opts withDelegate:self];
    if (ER_OK != status) {
        NSLog(@"Failed to initialize DoorCommon - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
//        [_waitCondition unlock]; //TODO:refactor
        return status;
    }

    dispatch_queue_t serviceQueue = dispatch_queue_create("org.alljoyn.DoorProvider.serviceQueue", NULL);
    dispatch_async( serviceQueue, ^{
        [self run];
    });


    _serviceState = STARTED;
//    [_waitCondition unlock];
    return ER_OK;
}

- (QStatus)enablePeerSecure
{
    QStatus status = ER_OK;

    NSString *securitySuites = [NSString stringWithFormat:@"%@ %@", kKeyxEcdheNull, kKeyxEcdheDsa];

    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *docPath = [NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
    NSString *keystoreFilePath = [docPath stringByAppendingPathComponent:@"s_central.ks"];
    NSError *error;
    [fileManager removeItemAtPath:keystoreFilePath error:&error];

    status = [_busAttachment enablePeerSecurity:securitySuites authenticationListener:_authListener keystoreFileName:keystoreFilePath sharing:NO permissionConfigurationListener:_doorCommonPCL];
    return status;
}

- (QStatus)setupPermissionConfiguratorWithPsk:(AJNGUID128 *)psk
{
    QStatus status = ER_OK;

    NSLog(@"Allow doors to be claimable using a PSK.\n");
    AJNClaimCapabilities capabilities = (CAPABLE_ECDHE_NULL);
    status = [_busAttachment.permissionConfigurator setClaimCapabilities:capabilities];
    if (status != ER_OK) {
        NSLog(@"Failed to setClaimCapabilities - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        //        [_waitCondition unlock]; //TODO:refactor
        return status;
    }

//    status = [_busAttachment.permissionConfigurator setClaimCapabilityAdditionalInfo:PSK_GENERATED_BY_APPLICATION];
//    if (status != ER_OK) {
//        NSLog(@"Failed to setClaimCapabilityAdditionalInfo - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
//        //        [_waitCondition unlock]; //TODO:refactor
//        return status;
//    }

    NSString* manifestXml =
    @"<manifest>"
    "<node>"
    "<interface name=\"sample.securitymgr.door.Door\">"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

    status = [_busAttachment.permissionConfigurator setManifestTemplateFromXml:manifestXml];
    if (ER_OK != status) {
        NSLog(@"Failed to setPermissionManifestTemplate - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        //        [_waitCondition unlock]; //TODO:refactor
        return status;
    }

    return status;
}

- (QStatus)stop
{
    [self.waitCondition signal];

    _serviceState = STOPED;

    return ER_OK;
}

- (BOOL)openDoors
{
    for (DoorProxy *proxy in _doorProxyMutableArray) {
        [proxy open];
    }
    return YES;
};

- (BOOL)closeDoors
{
    for (DoorProxy *proxy in _doorProxyMutableArray) {
        [proxy close];
    }
    return YES;
}


//TODO: remove this ??
- (BOOL)shouldAcceptSessionJoinerNamed:(NSString *)joiner onSessionPort:(AJNSessionPort)sessionPort withSessionOptions:(AJNSessionOptions *)options
{
    return YES;
}

- (void)foundDoorObjectAt:(NSString *)busName port:(AJNSessionPort)port;
{
    AJNSessionOptions *sessionOptions = [[AJNSessionOptions alloc] initWithTrafficType:kAJNTrafficMessages supportsMultipoint:NO proximity:kAJNProximityAny transportMask:kAJNTransportMaskAny];
    [_busAttachment enableConcurrentCallbacks];
    AJNSessionId sessionId = [_busAttachment joinSessionWithName:busName onPort:port withDelegate:self options:sessionOptions];
    DoorProxy *doorProxy = [[DoorProxy alloc] initWithBusAttachment:_busAttachment serviceName:busName objectPath:kDoorObjectPath sessionId:sessionId];
    [doorProxy introspectRemoteObject];
    [_doorProxyMutableArray addObject:doorProxy];
}

- (QStatus)run
{
    QStatus status = ER_FAIL;

    // Create bus object
//    _door = [[Door alloc] initWithBusAttachment:_busAttachment onPath:kDoorObjectPath];
//
//    status = [_busAttachment registerBusObject:_door enableSecurity:YES];
//    if (ER_OK != status) {
//        NSLog(@"Failed to RegisterBusObject - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
//        return status;
//    }


    [self setAboutData];

    status = [_aboutObj announceForSessionPort:kPort withAboutDataListener:_aboutData];
    if (ER_OK != status) {
        NSLog(@"Failed to AnnounceAbout - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    //Wait until we are claimed
    [_messageListener didReceiveAllJoynStatusMessage:@"Waiting to be claimed..."];
    status = [_doorCommonPCL waitForClaimedState];
    if (ER_OK != status) {
        NSLog(@"Failed to WaitForClaimedState - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    // Register signal handler DMR ???

    // Register AboutListener and look for doors

    //Look for doors and Register About Listener
    status = [_busAttachment whoImplementsInterface:@"sample.securitymgr.door.Door"];
    if (ER_OK != status) {
        NSLog(@"Failed to call WhoImplements - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    DoorAboutListener *doorAboutListner = [[DoorAboutListener alloc] initWithDoorFoundDelegate:self];
    [_busAttachment registerAboutListener:doorAboutListner];

    // Waiting for commands



    // After claiming, only allow ALLJOYN_ECDHE_ECDSA connections
//    status = [self setSecurityForClaimedMode];
//    if (ER_OK != status) {
//        NSLog(@"Failed to SetSecurityForClaimedMode - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
//        return status;
//    }
//
//    [_messageListener didReceiveAllJoynStatusMessage:@"Door provider initialized; Waiting for consumers ...\n"];

    [self.waitCondition lock];
    [self.waitCondition wait];
    [self.waitCondition unlock];

    //clean up (nil)

    return ER_OK;
}

- (void)setAboutData
{
    AJNGUID128* appId = [[AJNGUID128 alloc] init];
    [_aboutData setAppId:(uint8_t*)appId.bytes];

    char buf[64];
    gethostname(buf, sizeof(buf));
    [_aboutData setDeviceName:[NSString stringWithCString:buf encoding:NSUTF8StringEncoding] andLanguage:@"en"];

    AJNGUID128* deviceId = [[AJNGUID128 alloc] init];
    [_aboutData setDeviceId:[deviceId description]];
    [_aboutData setAppName:_appName andLanguage:@"en"];
    [_aboutData setManufacturer:@"Manufacturer" andLanguage:@"en"];
    [_aboutData setModelNumber:@"1"];
    [_aboutData setDescription:_appName andLanguage:@"en"];
    [_aboutData setDateOfManufacture:@"2015-04-14"];
    [_aboutData setSoftwareVersion:@"0.1"];
    [_aboutData setHardwareVersion:@"0.0.1"];
    [_aboutData setSupportUrl:@"https://allseenalliance.org/"];
}

- (QStatus)setSecurityForClaimedMode
{
    QStatus status = [_busAttachment enablePeerSecurity:@"" authenticationListener:nil keystoreFileName:nil sharing:true];
    if (ER_OK != status) {
        NSLog(@"SetSecurityForClaimedMode: Could not clear peer security - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    status = [_busAttachment enablePeerSecurity:kKeyxEcdheDsa authenticationListener:_authListener keystoreFileName:@"" sharing:NO permissionConfigurationListener:_doorCommonPCL];
    if (ER_OK != status) {
        NSLog(@"Failed to SetSecurityForClaimedMode - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }
    
    return ER_OK;
}

- (void)dealloc
{
    [AJNInit alljoynRouterShutdown];
    [AJNInit alljoynShutdown];
}

@end
