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
static NSString *const kKeyxEcdheSpeke = @"ALLJOYN_ECDHE_SPEKE";
static NSString *const kKeyxEcdheDsa = @"ALLJOYN_ECDHE_ECDSA";

static NSString *const kDoorObjectPath = @"/sample/security/Door";
static AJNSessionPort const kPort = 12345;

@interface DoorConsumerAllJoynService () <AJNSessionPortListener, AJNSessionListener, DoorFoundDelegate, DoorServiceToogleToClaimedModeListener>

@property (nonatomic, strong) NSCondition* waitCondition;

@property (nonatomic, strong) AJNAboutData *aboutData;
@property (nonatomic, strong) AJNAboutObject *aboutObj;

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

    _appName = appName;

    _busAttachment = [[AJNBusAttachment alloc] initWithApplicationName:appName allowRemoteMessages:YES];

    _aboutData = [[AJNAboutData alloc] initWithLanguage:@"en"];
    _aboutObj = [[AJNAboutObject alloc] initWithBusAttachment:_busAttachment withAnnounceFlag:ANNOUNCED];

    _authListener = [[DefaultECDHEListener alloc] init];

    _doorCommonPCL = [[DoorCommonPCL alloc] initWithBus:_busAttachment service:self];

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
    }

    status = [self setupPermissionConfigurator];
    if (status != ER_OK) {
        NSLog(@"Failed to setup permission configurator - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
    }

    status = [self displayClaimingInformation];
    if (status != ER_OK) {
        NSLog(@"Failed to display claiming status - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
    }

    //host Session
    AJNSessionOptions* opts = [[AJNSessionOptions alloc] init];
    status = [_busAttachment bindSessionOnPort:kPort withOptions:opts withDelegate:self];
    if (ER_OK != status) {
        NSLog(@"Failed to initialize DoorCommon - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    dispatch_queue_t serviceQueue = dispatch_queue_create("org.alljoyn.DoorConsumer.serviceQueue", NULL);
    dispatch_async( serviceQueue, ^{
        [self run];
    });

    _serviceState = STARTED;
    return ER_OK;
}

- (QStatus)enablePeerSecure
{
    QStatus status = ER_OK;


    NSString* securitySuites;

    status = [self setupSecureRules:&securitySuites];
    if (ER_OK != status) {
        NSLog(@"Failed to setup secure rules - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    //check is file exist
    BOOL isKeyStoreExist = NO;

    NSString *keystoreFilePathShort = @"Documents/s_central.ks";
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *keystoreFilePathFull = [NSHomeDirectory() stringByAppendingPathComponent:keystoreFilePathShort];

    if ([fileManager fileExistsAtPath:keystoreFilePathFull]) {
        isKeyStoreExist = YES;
    }

    //try to enable peer secure 1st time
    status = [_busAttachment enablePeerSecurity:securitySuites authenticationListener:_authListener keystoreFileName:keystoreFilePathShort sharing:YES permissionConfigurationListener:_doorCommonPCL];
    if (status == ER_OK) {
        if (isKeyStoreExist) {
            NSLog(@"Application enable peer secure with existed key store.");
        } else {
            NSLog(@"Application enable peer secure with new key store.");
        }
    } else if (status == ER_AUTH_FAIL && isKeyStoreExist) {
        NSLog(@"Application failed to enable peer secure with existed key store.");
        NSLog(@"Try to delete keystore and enable peer secure again.");
        //delete keystore
        NSError *error;
        BOOL isFileDeleted = NO;
        isFileDeleted = [fileManager createFileAtPath:keystoreFilePathFull contents:[NSData data] attributes:nil];

        if (isFileDeleted) {
            //try to enable peer secure 2nd time
            status = [_busAttachment enablePeerSecurity:securitySuites authenticationListener:_authListener keystoreFileName:keystoreFilePathShort sharing:YES permissionConfigurationListener:_doorCommonPCL];
            if (status == ER_OK) {
                NSLog(@"Application delete old key store and enable peer secure with new one.");
            }
        } else {
            return ER_FAIL;
        }
    }

    return status;
}

- (QStatus)setupSecureRules:(NSString **)securitySuites
{
    QStatus status = ER_OK;

    *securitySuites = [NSString stringWithFormat:@"%@ %@", kKeyxEcdheDsa, kKeyxEcdheNull];

    return status;
}

- (QStatus)setupPermissionConfigurator
{
    QStatus status = ER_OK;

    NSLog(@"Allow doors to be claimable using a NULL auth.\n");
    AJNClaimCapabilities capabilities = (CAPABLE_ECDHE_NULL);
    status = [_busAttachment.permissionConfigurator setClaimCapabilities:capabilities];
    if (status != ER_OK) {
        NSLog(@"Failed to setClaimCapabilities - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    NSString* manifestXml =
    @"<manifest>"
    "<node>"
    "<interface name=\"sample.securitymgr.door.Door\">"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";

    status = [_busAttachment.permissionConfigurator setManifestTemplateFromXml:manifestXml];
    if (ER_OK != status) {
        NSLog(@"Failed to setPermissionManifestTemplate - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    return status;
}

- (QStatus)displayClaimingInformation
{
    AJNApplicationState state;
    QStatus status = ER_OK;
    status = [_busAttachment.permissionConfigurator getApplicationState:&state];
    if (ER_OK != status) {
        NSLog(@"Failed to getApplicationState - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    if (state == CLAIMABLE) {
        NSLog(@"Door consumer is not claimed.\n");
        NSLog(@"The consumer can be claimed using NULL auth.\n");
        [_messageListener didReceiveAllJoynStatusMessage:[NSString stringWithFormat:@"The consumer can be claimed using NULL auth.\n"]];
    } else if (state == CLAIMED) {
        NSLog(@"Door cosumer is already claimed.\n");
        [_messageListener didReceiveAllJoynStatusMessage:[NSString stringWithFormat:@"Door consumer is already claimed.\n"]];
    } else {
        NSLog(@"Unexpected application state. Expected CLAIMABLE or CLAIMED, current state = (%u)\n", state);
        return ER_FAIL;
    }

    return ER_OK;
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

    [self setAboutData];

    status = [_aboutObj announceForSessionPort:kPort withAboutDataListener:_aboutData];
    if (ER_OK != status) {
        NSLog(@"Failed to AnnounceAbout - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    //Wait until we are claimed
    [_messageListener didReceiveAllJoynStatusMessage:@"Waiting to be claimed...\n"];
    status = [_doorCommonPCL waitForClaimedState];
    if (ER_OK != status) {
        NSLog(@"Failed to WaitForClaimedState - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    // After claiming, only allow ALLJOYN_ECDHE_ECDSA connections
    status = [self setSecurityForClaimedMode];
    if (ER_OK != status) {
        NSLog(@"Failed to SetSecurityForClaimedMode - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    [_messageListener didReceiveAllJoynStatusMessage:@"Door consumer claimed; Waiting for consumers...\n"];

    //Look for doors and Register About Listener
    status = [_busAttachment whoImplementsInterface:@"sample.securitymgr.door.Door"];
    if (ER_OK != status) {
        NSLog(@"Failed to call WhoImplements - status (%@)\n", [AJNStatus descriptionForStatusCode:status]);
        return status;
    }

    DoorAboutListener *doorAboutListner = [[DoorAboutListener alloc] initWithDoorFoundDelegate:self];
    [_busAttachment registerAboutListener:doorAboutListner];

    [self.waitCondition lock];
    [self.waitCondition wait];
    [self.waitCondition unlock];

    _aboutObj = nil;
    _busAttachment = nil;
    _messageListener = nil;
    _doorProxyMutableArray = nil;
    _doorCommonPCL = nil;

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
